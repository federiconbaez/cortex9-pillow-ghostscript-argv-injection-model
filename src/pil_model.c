#include "pil_model.h"
#include <string.h>

const char *cx9_version_name(cx9_version v) { return v == CX9_PRE ? "pre" : "post"; }
const char *cx9_branch_name(cx9_branch b) { return b == CX9_BR_NAME ? "NAME" : "TEMP"; }
const char *cx9_class_name(cx9_class c) {
  switch (c) {
    case CX9_TOK_OPTION: return "OPTION";
    case CX9_TOK_ATFILE: return "ATFILE";
    default: return "PATH";
  }
}

/* GHSA-w889-685w-c8fh: a leading "-" (or "--") is read as an option and a
 * leading "@" as Ghostscript's argument-file token. Anything else is a path. */
cx9_class cx9_classify(const char *tok) {
  if (tok[0] == '-') return CX9_TOK_OPTION;
  if (tok[0] == '@') return CX9_TOK_ATFILE;
  return CX9_TOK_PATH;
}

/* normpath(join(cwd, name)) following posixpath: two leading slashes are
 * kept, three or more collapse to one; "." and empty components vanish;
 * ".." pops a component and never climbs above the root. */
int cx9_abspath(const char *cwd, const char *name, char *out, size_t outsz) {
  char joined[2 * CX9_PATH_MAX + 2];
  size_t lc = strlen(cwd), ln = strlen(name), o = 0, i = 0, j;
  size_t starts[CX9_PATH_MAX];
  size_t ncomp = 0;
  int initial;
  if (lc == 0 || cwd[0] != '/' || lc >= CX9_PATH_MAX || ln >= CX9_PATH_MAX) return -1;
  if (name[0] == '/') {
    memcpy(joined, name, ln + 1);
  } else {
    memcpy(joined, cwd, lc);
    j = lc;
    if (cwd[lc - 1] != '/') joined[j++] = '/';
    memcpy(joined + j, name, ln + 1);
  }
  if (joined[0] == '/' && joined[1] == '/' && joined[2] != '/') initial = 2;
  else initial = 1; /* joined always starts with '/' here */
  if (outsz < 3) return -1;
  for (j = 0; j < (size_t)initial; ++j) out[o++] = '/';
  while (joined[i]) {
    size_t s, e;
    while (joined[i] == '/') i++;
    s = i;
    while (joined[i] && joined[i] != '/') i++;
    e = i;
    if (e == s || (e - s == 1 && joined[s] == '.')) continue;
    if (e - s == 2 && joined[s] == '.' && joined[s + 1] == '.') {
      if (ncomp > 0) {
        o = starts[--ncomp];
        if (o > (size_t)initial) o--; /* drop the separator before it */
      }
      continue;
    }
    if (ncomp > 0) {
      if (o + 1 >= outsz) return -1;
      out[o++] = '/';
    }
    if (ncomp >= CX9_PATH_MAX || o + (e - s) + 1 > outsz) return -1;
    starts[ncomp++] = o;
    memcpy(out + o, joined + s, e - s);
    o += e - s;
  }
  out[o] = '\0';
  return 0;
}

cx9_result cx9_ghostscript_infile(cx9_version v, const char *name, int exists, const char *cwd) {
  cx9_result r;
  memset(&r, 0, sizeof r);
  r.argv_index = 12;
  /* Domain: os.path.exists("") is False, so exists=1 needs a name; the cwd
   * a process has is absolute; lengths are bounded. */
  if ((exists && name[0] == '\0') || cwd[0] != '/' || strlen(name) >= CX9_PATH_MAX ||
      strlen(cwd) >= CX9_PATH_MAX)
    return r;
  r.ok = 1;
  /* EpsImagePlugin.py 105: if hasattr(fp, "name") and os.path.exists(fp.name) */
  if (exists) {
    r.branch = CX9_BR_NAME;
    if (v == CX9_PRE) {
      /* old line 106: infile = fp.name */
      memcpy(r.infile, name, strlen(name) + 1);
    } else if (cx9_abspath(cwd, name, r.infile, sizeof r.infile) != 0) {
      /* new line 109: infile = os.path.abspath(fp.name) */
      memset(&r, 0, sizeof r);
      return r;
    }
  } else {
    /* old 108 / new 111: in_fd, infile_temp = tempfile.mkstemp() */
    r.branch = CX9_BR_TEMP;
    memcpy(r.infile, CX9_MKSTEMP_PLACEHOLDER, sizeof CX9_MKSTEMP_PLACEHOLDER);
  }
  r.cls = cx9_classify(r.infile);
  return r;
}

int cx9_build_argv(const cx9_result *r, const char *argv[], int max) {
  static const char *const layout[15] = {
      "gs", "-q", "-g<width>x<height>", "-r<res_x>x<res_y>", "-dBATCH", "-dNOPAUSE",
      "-dSAFER", "-sDEVICE=<device>", "-sOutputFile=<outfile>", "-c",
      "<-bbox0> <-bbox1> translate", "-f", NULL, "-c", "showpage"};
  int i;
  if (max < 15) return -1;
  for (i = 0; i < 15; ++i) argv[i] = layout[i];
  argv[12] = r->infile;
  return 15;
}
