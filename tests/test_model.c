/* Tests for the Pillow EPS -> Ghostscript infile model. Run under ASan +
 * UBSan with -fno-sanitize-recover (scripts/build.sh test). */
#include "cx9_util.h"
#include "pil_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks, failures;
#define CHECK(c, what)                                                    \
  do {                                                                    \
    checks++;                                                             \
    if (!(c)) {                                                           \
      failures++;                                                         \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, what);               \
    }                                                                     \
  } while (0)

#define MAXCASES 64
typedef struct {
  char id[64], kind[16], name[CX9_FIELD_MAX], cwd[CX9_FIELD_MAX], hand[2][16];
  int exists;
} tcase;
static tcase cases[MAXCASES];
static int ncases;

static void load_cases(const char *dir) {
  char path[512], line[3 * CX9_FIELD_MAX];
  FILE *f;
  snprintf(path, sizeof path, "%s/cases.tsv", dir);
  f = fopen(path, "r");
  CHECK(f != NULL, "open cases.tsv");
  if (!f) return;
  while (fgets(line, sizeof line, f) && ncases < MAXCASES) {
    char *fl[CX9_MAX_FIELDS];
    tcase *c = &cases[ncases];
    if (line[0] == '#' || line[0] == '\n') continue;
    if (cx9_split_tsv(line, fl, CX9_MAX_FIELDS) != 7 || strlen(fl[0]) >= 64 ||
        strlen(fl[1]) >= 16 || strlen(fl[5]) >= 16 || strlen(fl[6]) >= 16 ||
        cx9_unescape(fl[2], c->name, sizeof c->name) < 0 ||
        cx9_unescape(fl[4], c->cwd, sizeof c->cwd) < 0) {
      CHECK(0, "cases.tsv row");
      continue;
    }
    strcpy(c->id, fl[0]);
    strcpy(c->kind, fl[1]);
    c->exists = fl[3][0] == '1';
    strcpy(c->hand[0], fl[5]);
    strcpy(c->hand[1], fl[6]);
    ncases++;
  }
  fclose(f);
  CHECK(ncases >= 20, "at least 20 fixture cases");
}

static const char *label(const cx9_result *r) {
  return r->ok ? cx9_class_name(r->cls) : "BAD_INPUT";
}

/* Hand-derived labels in fixtures/cases.tsv (pos / neg / edge / bad). */
static void test_hand_labels(void) {
  int i, v, pos = 0, neg = 0;
  for (i = 0; i < ncases; ++i) {
    for (v = 0; v < 2; ++v) {
      cx9_result r = cx9_ghostscript_infile((cx9_version)v, cases[i].name, cases[i].exists,
                                            cases[i].cwd);
      CHECK(strcmp(label(&r), cases[i].hand[v]) == 0, cases[i].id);
    }
    if (strcmp(cases[i].kind, "pos") == 0) {
      cx9_result a = cx9_ghostscript_infile(CX9_PRE, cases[i].name, 1, cases[i].cwd);
      cx9_result b = cx9_ghostscript_infile(CX9_POST, cases[i].name, 1, cases[i].cwd);
      CHECK(a.cls != CX9_TOK_PATH && b.cls == CX9_TOK_PATH, "pos: differs before/after");
      pos++;
    }
    if (strcmp(cases[i].kind, "neg") == 0) {
      cx9_result a = cx9_ghostscript_infile(CX9_PRE, cases[i].name, cases[i].exists, cases[i].cwd);
      CHECK(a.ok && a.cls == CX9_TOK_PATH, "neg: already a path before the fix");
      neg++;
    }
  }
  CHECK(pos >= 5 && neg >= 5, "enough positive and negative fixtures");
}

/* fixtures/expected.tsv comes from scripts/derive_expected.py (Python's
 * posixpath, not this C code). */
static void test_expected(const char *dir) {
  char path[512], line[3 * CX9_FIELD_MAX];
  FILE *f;
  int rows = 0, i;
  snprintf(path, sizeof path, "%s/expected.tsv", dir);
  f = fopen(path, "r");
  CHECK(f != NULL, "open expected.tsv");
  if (!f) return;
  while (fgets(line, sizeof line, f)) {
    char *fl[CX9_MAX_FIELDS];
    int n;
    if (strncmp(line, "id\t", 3) == 0) continue;
    n = cx9_split_tsv(line, fl, CX9_MAX_FIELDS);
    CHECK(n == 6, "expected.tsv row");
    if (n != 6) continue;
    for (i = 0; i < ncases; ++i) {
      cx9_version v = strcmp(fl[1], "post") == 0 ? CX9_POST : CX9_PRE;
      cx9_result r;
      char esc[2 * CX9_PATH_MAX], idx[16];
      if (strcmp(cases[i].id, fl[0]) != 0) continue;
      r = cx9_ghostscript_infile(v, cases[i].name, cases[i].exists, cases[i].cwd);
      rows++;
      if (!r.ok) {
        CHECK(strcmp(fl[2], "BAD_INPUT") == 0, fl[0]);
        continue;
      }
      CHECK(cx9_escape(r.infile, strlen(r.infile), esc, sizeof esc) == 0, "escape");
      snprintf(idx, sizeof idx, "%d", r.argv_index);
      CHECK(strcmp(fl[2], cx9_branch_name(r.branch)) == 0, fl[0]);
      CHECK(strcmp(fl[3], cx9_class_name(r.cls)) == 0, fl[0]);
      CHECK(strcmp(fl[4], idx) == 0, fl[0]);
      CHECK(strcmp(fl[5], esc) == 0, fl[0]);
    }
  }
  fclose(f);
  CHECK(rows == 2 * ncases, "every case has a pre and a post row");
}

static unsigned long long rng = 0x9e3779b97f4a7c15ULL;
static unsigned next(void) {
  rng ^= rng << 13;
  rng ^= rng >> 7;
  rng ^= rng << 17;
  return (unsigned)(rng >> 32);
}

static int normalized(const char *p) {
  size_t n = strlen(p);
  if (p[0] != '/') return 0;
  if (strstr(p, "/./") || strstr(p, "/../")) return 0;
  if (n >= 2 && strcmp(p + n - 2, "/.") == 0) return 0;
  if (n >= 3 && strcmp(p + n - 3, "/..") == 0) return 0;
  if (n > 2 && p[n - 1] == '/') return 0;
  if (strncmp(p, "///", 3) == 0) return 0;
  return 1;
}

/* Generated names: the fix always yields an absolute, normalized path read
 * as PATH; before the fix the class depends only on the first byte. */
static void test_invariants(void) {
  static const char alpha[] = "-@./ab ~\n=";
  static const char *cwds[] = {"/", "/srv/uploads", "/srv/uploads/", "//net", "/a/b/c"};
  int k;
  for (k = 0; k < 20000; ++k) {
    char name[48], again[CX9_PATH_MAX];
    size_t len = 1 + next() % 40, j;
    const char *cwd = cwds[next() % 5];
    cx9_result a, b;
    for (j = 0; j < len; ++j) name[j] = alpha[next() % (sizeof alpha - 1)];
    name[len] = '\0';
    a = cx9_ghostscript_infile(CX9_PRE, name, 1, cwd);
    b = cx9_ghostscript_infile(CX9_POST, name, 1, cwd);
    CHECK(a.ok && b.ok, "generated input in domain");
    CHECK(strcmp(a.infile, name) == 0, "pre passes the name through");
    CHECK((a.cls != CX9_TOK_PATH) == (name[0] == '-' || name[0] == '@'), "pre class = first byte");
    CHECK(b.cls == CX9_TOK_PATH, "post is always PATH");
    CHECK(normalized(b.infile), "post is absolute and normalized");
    CHECK(cx9_abspath("/elsewhere", b.infile, again, sizeof again) == 0 &&
              strcmp(again, b.infile) == 0,
          "abspath is idempotent on its result");
  }
}

/* Adversarial inputs: long names, deep "..", slash runs, buffer limits. */
static void test_adversarial(void) {
  static char big[CX9_PATH_MAX + 16];
  char out[CX9_PATH_MAX];
  cx9_result r;
  size_t i;
  memset(big, '-', CX9_PATH_MAX - 1);
  big[CX9_PATH_MAX - 1] = '\0';
  r = cx9_ghostscript_infile(CX9_PRE, big, 1, "/srv");
  CHECK(r.ok && r.cls == CX9_TOK_OPTION, "8191 dashes, pre");
  r = cx9_ghostscript_infile(CX9_POST, big, 1, "/srv");
  CHECK(!r.ok, "8191 dashes + cwd does not fit: BAD_INPUT, no overflow");
  memset(big, 'a', CX9_PATH_MAX + 8);
  big[CX9_PATH_MAX + 8] = '\0';
  r = cx9_ghostscript_infile(CX9_PRE, big, 1, "/srv");
  CHECK(!r.ok, "name longer than the domain");
  for (i = 0; i + 3 < 6000; i += 3) memcpy(big + i, "../", 3);
  strcpy(big + 5997, "-h");
  CHECK(cx9_abspath("/srv/uploads", big, out, sizeof out) == 0 && strcmp(out, "/-h") == 0,
        "deep .. stops at the root");
  memset(big, '/', 5000);
  big[5000] = '\0';
  CHECK(cx9_abspath("/srv", big, out, sizeof out) == 0 && strcmp(out, "/") == 0, "slash run");
  CHECK(cx9_abspath("/srv", "//", out, sizeof out) == 0 && strcmp(out, "//") == 0,
        "exactly two leading slashes are kept (POSIX)");
  CHECK(cx9_abspath("/srv", "x", out, 2) != 0, "tiny output buffer is refused");
  CHECK(cx9_abspath("", "x", out, sizeof out) != 0, "empty cwd is refused");
  r = cx9_ghostscript_infile(CX9_POST, "-h", 1, "relative");
  CHECK(!r.ok, "relative cwd is outside the domain");
  {
    const char *av[16];
    r = cx9_ghostscript_infile(CX9_PRE, "@x", 1, "/srv");
    CHECK(cx9_build_argv(&r, av, 16) == 15, "argc");
    CHECK(strcmp(av[11], "-f") == 0 && av[12] == r.infile, "infile follows -f");
    CHECK(strcmp(av[6], "-dSAFER") == 0, "-dSAFER is in the default argv");
    CHECK(cx9_build_argv(&r, av, 4) == -1, "short argv array refused");
  }
}

/* PRACTICE.md levels: each answer is checked here. */
static void test_practice(void) {
  cx9_result r;
  r = cx9_ghostscript_infile(CX9_PRE, "image.eps", 1, "/srv/uploads");
  CHECK(r.cls == CX9_TOK_PATH && strcmp(r.infile, "image.eps") == 0, "practice_level_1");
  r = cx9_ghostscript_infile(CX9_PRE, "-h", 1, "/srv/uploads");
  CHECK(r.cls == CX9_TOK_OPTION, "practice_level_2");
  r = cx9_ghostscript_infile(CX9_PRE, "@args.txt", 1, "/srv/uploads");
  CHECK(r.cls == CX9_TOK_ATFILE, "practice_level_3");
  r = cx9_ghostscript_infile(CX9_POST, "-h", 1, "/srv/uploads");
  CHECK(r.cls == CX9_TOK_PATH && strcmp(r.infile, "/srv/uploads/-h") == 0, "practice_level_4");
  r = cx9_ghostscript_infile(CX9_PRE, "./-h", 1, "/srv/uploads");
  CHECK(r.cls == CX9_TOK_PATH, "practice_level_5");
  r = cx9_ghostscript_infile(CX9_PRE, "-h", 0, "/srv/uploads");
  CHECK(r.branch == CX9_BR_TEMP && r.cls == CX9_TOK_PATH, "practice_level_6");
  r = cx9_ghostscript_infile(CX9_POST, "../../../../-h", 1, "/srv/uploads");
  CHECK(strcmp(r.infile, "/-h") == 0 && r.cls == CX9_TOK_PATH, "practice_level_7");
}

int main(int argc, char **argv) {
  const char *dir = argc > 1 ? argv[1] : "fixtures";
  load_cases(dir);
  test_hand_labels();
  test_expected(dir);
  test_invariants();
  test_adversarial();
  test_practice();
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
