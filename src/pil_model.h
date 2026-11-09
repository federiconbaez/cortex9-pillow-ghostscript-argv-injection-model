/* Model of the input-file choice in Pillow's EpsImagePlugin.Ghostscript()
 * before and after a2c363141da3 (CVE-2026-112900), and of how an argv
 * parser that treats a leading '-' as an option and a leading '@' as an
 * argument file would read that token. Ghostscript is never run. */
#ifndef PIL_MODEL_H
#define PIL_MODEL_H
#include <stddef.h>

#define CX9_PATH_MAX 8192

typedef enum { CX9_PRE = 0, CX9_POST = 1 } cx9_version;
typedef enum { CX9_BR_NAME = 0, CX9_BR_TEMP = 1 } cx9_branch;
typedef enum { CX9_TOK_PATH = 0, CX9_TOK_OPTION = 1, CX9_TOK_ATFILE = 2 } cx9_class;

typedef struct {
  int ok;            /* 0: BAD_INPUT (outside the model's domain) */
  cx9_branch branch; /* fp.name branch or the mkstemp() branch */
  cx9_class cls;     /* how the token after "-f" would be read */
  int argv_index;    /* index of the infile token in the GHSA argv */
  char infile[CX9_PATH_MAX];
} cx9_result;

/* Placeholder for the path tempfile.mkstemp() returns (always absolute:
 * CPython's _mkstemp_inner returns os.path.abspath(file)). */
#define CX9_MKSTEMP_PLACEHOLDER "/tmp/tmpXXXXXXXX"

/* posixpath.abspath(name) with a fixed working directory:
 * normpath(join(cwd, name)). cwd must be absolute. 0 on success. */
int cx9_abspath(const char *cwd, const char *name, char *out, size_t outsz);
cx9_class cx9_classify(const char *tok);
/* exists = os.path.exists(fp.name) as seen by the plugin (a fixture input). */
cx9_result cx9_ghostscript_infile(cx9_version v, const char *name, int exists, const char *cwd);
/* The argv layout quoted in GHSA-w889-685w-c8fh, with placeholders for the
 * numeric and device arguments. Returns the argc (15). */
int cx9_build_argv(const cx9_result *r, const char *argv[], int max);

const char *cx9_version_name(cx9_version v);
const char *cx9_branch_name(cx9_branch b);
const char *cx9_class_name(cx9_class c);
#endif
