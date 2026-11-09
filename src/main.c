/* CLI: regenerate results/ or inspect one case.
 *   pilgs results fixtures/cases.tsv <dir>
 *   pilgs case pre|post <name> <exists 0|1> <cwd>   (name uses TSV escapes) */
#include "cx9_util.h"
#include "pil_model.h"
#include <stdio.h>
#include <string.h>

static void row(FILE *f, const char *id, const char *kind, cx9_version v, const cx9_result *r) {
  char esc[2 * CX9_PATH_MAX * 2];
  fprintf(f, "%s,%s,%s,", id, kind, cx9_version_name(v));
  if (!r->ok) {
    fputs("BAD_INPUT,,,\n", f);
    return;
  }
  cx9_escape(r->infile, strlen(r->infile), esc, sizeof esc);
  fprintf(f, "%s,%s,%d,", cx9_branch_name(r->branch), cx9_class_name(r->cls), r->argv_index);
  cx9_csv_field(f, esc);
  fputc('\n', f);
}

static int results(const char *cases, const char *dir) {
  char path[1024], line[3 * CX9_FIELD_MAX];
  FILE *in = fopen(cases, "r"), *out, *sw;
  int c;
  if (!in) return 1;
  snprintf(path, sizeof path, "%s/cases.csv", dir);
  out = fopen(path, "w");
  if (!out) { fclose(in); return 1; }
  fputs("id,kind,version,branch,class,argv_index,infile\n", out);
  while (fgets(line, sizeof line, in)) {
    char *f[CX9_MAX_FIELDS], name[CX9_FIELD_MAX], cwd[CX9_FIELD_MAX];
    int n, v;
    if (line[0] == '#' || line[0] == '\n') continue;
    n = cx9_split_tsv(line, f, CX9_MAX_FIELDS);
    if (n != 7 || cx9_unescape(f[2], name, sizeof name) < 0 ||
        cx9_unescape(f[4], cwd, sizeof cwd) < 0) {
      fclose(in); fclose(out);
      return 1;
    }
    for (v = 0; v < 2; ++v) {
      cx9_result r = cx9_ghostscript_infile((cx9_version)v, name, f[3][0] == '1', cwd);
      row(out, f[0], f[1], (cx9_version)v, &r);
    }
  }
  fclose(in);
  fclose(out);
  /* First-byte sweep: "<c>x.eps" for every printable ASCII byte. */
  snprintf(path, sizeof path, "%s/sweep.csv", dir);
  sw = fopen(path, "w");
  if (!sw) return 1;
  fputs("first_byte_hex,pre_class,post_class,post_first_byte_hex\n", sw);
  for (c = 0x21; c < 0x7f; ++c) {
    char name[8];
    cx9_result a, b;
    snprintf(name, sizeof name, "%cx.eps", c);
    a = cx9_ghostscript_infile(CX9_PRE, name, 1, "/srv/uploads");
    b = cx9_ghostscript_infile(CX9_POST, name, 1, "/srv/uploads");
    fprintf(sw, "%02x,%s,%s,%02x\n", c, cx9_class_name(a.cls), cx9_class_name(b.cls),
            (unsigned char)b.infile[0]);
  }
  fclose(sw);
  return 0;
}

int main(int argc, char **argv) {
  if (argc == 4 && strcmp(argv[1], "results") == 0) return results(argv[2], argv[3]);
  if (argc == 6 && strcmp(argv[1], "case") == 0) {
    char name[CX9_FIELD_MAX];
    const char *av[16];
    cx9_result r;
    int i, n;
    cx9_version v = strcmp(argv[2], "post") == 0 ? CX9_POST : CX9_PRE;
    if ((strcmp(argv[2], "pre") != 0 && strcmp(argv[2], "post") != 0) ||
        cx9_unescape(argv[3], name, sizeof name) < 0 ||
        (strcmp(argv[4], "0") != 0 && strcmp(argv[4], "1") != 0)) {
      fputs("bad arguments\n", stderr);
      return 2;
    }
    r = cx9_ghostscript_infile(v, name, argv[4][0] == '1', argv[5]);
    if (!r.ok) { puts("BAD_INPUT"); return 0; }
    printf("branch=%s class=%s\nargv (not executed):\n", cx9_branch_name(r.branch),
           cx9_class_name(r.cls));
    n = cx9_build_argv(&r, av, 16);
    for (i = 0; i < n; ++i) {
      char esc[2 * CX9_PATH_MAX];
      cx9_escape(av[i], strlen(av[i]), esc, sizeof esc);
      printf("  [%2d] %s%s\n", i, esc, i == r.argv_index ? "   <- infile" : "");
    }
    return 0;
  }
  fprintf(stderr, "usage: %s results <cases.tsv> <dir> | case pre|post <name> <0|1> <cwd>\n", argv[0]);
  return 2;
}
