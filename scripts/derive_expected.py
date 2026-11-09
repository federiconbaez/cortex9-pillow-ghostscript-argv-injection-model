#!/usr/bin/env python3
"""Independent derivation of fixtures/expected.tsv.

Written from the Pillow diff a2c363141da3 (src/PIL/EpsImagePlugin.py, old
line 106 / new line 109) and the argv quoted in GHSA-w889-685w-c8fh, not
from the C model. The post-fix path uses Python's own posixpath with the
fixture's working directory: os.path.abspath(p) is normpath(join(getcwd(),
p)) for a relative str on POSIX, so that is what is called here.
"""
import posixpath
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from cx9_util import escape, read_cases, unescape  # noqa: E402

MKSTEMP = "/tmp/tmpXXXXXXXX"   # placeholder; mkstemp() returns an absolute path
ARGV_INDEX = 12                 # gs -q -g -r -dBATCH -dNOPAUSE -dSAFER -sDEVICE -sOutputFile -c <ps> -f <infile>


def token_class(tok):
    # GHSA: a leading "-" or "--" is an option, a leading "@" an argument file.
    if tok.startswith("-"):
        return "OPTION"
    if tok.startswith("@"):
        return "ATFILE"
    return "PATH"


def infile(version, name, exists, cwd):
    if (exists and name == "") or not cwd.startswith("/"):
        return None
    if exists:
        if version == "pre":
            return "NAME", name
        return "NAME", posixpath.normpath(posixpath.join(cwd, name))
    return "TEMP", MKSTEMP


def main():
    root = Path(__file__).resolve().parent.parent
    print("id\tversion\tbranch\tclass\targv_index\tinfile")
    for row in read_cases(root / "fixtures" / "cases.tsv"):
        cid, _kind, name, exists, cwd = row[0], row[1], unescape(row[2]), row[3] == "1", unescape(row[4])
        for version in ("pre", "post"):
            r = infile(version, name, exists, cwd)
            if r is None:
                print(f"{cid}\t{version}\tBAD_INPUT\t-\t-\t-")
                continue
            branch, path = r
            print(f"{cid}\t{version}\t{branch}\t{token_class(path)}\t{ARGV_INDEX}\t{escape(path)}")


if __name__ == "__main__":
    main()
