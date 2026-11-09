# cortex9-pillow-ghostscript-argv-injection-model

Offline model of **CVE-2026-112900** (GHSA-w889-685w-c8fh): Pillow's
`EpsImagePlugin.Ghostscript()` put `fp.name` as-is after `-f` in the
Ghostscript argv, so a file name starting with `-` was read as an option
and one starting with `@` as an argument file. The fix, commit
a2c363141da3 (Pillow 13.0.0), uses `os.path.abspath(fp.name)`.
Educational, deterministic, C11 checked with ASan/UBSan. **Ghostscript is
never run, Pillow is not imported, no file is opened and nothing touches the
network.** The model builds the argv as data and classifies the token.

Spanish: [README.es.md](README.es.md). Exercises: [PRACTICE.md](PRACTICE.md).

## What is modelled (tied to the diff)

| Diff line | Code | Model |
|---|---|---|
| `EpsImagePlugin.py` 105 (context) | `if hasattr(fp, "name") and os.path.exists(fp.name):` | `exists` is a fixture input |
| old 106 | `infile = fp.name` | `pre`: the name passes through unchanged |
| new 109 | `infile = os.path.abspath(fp.name)` | `post`: `normpath(join(cwd, name))` |
| old 108 / new 111 (context) | `tempfile.mkstemp()` | `TEMP` branch: an absolute placeholder |

The argv layout (`gs -q -g.. -r.. -dBATCH -dNOPAUSE -dSAFER -sDEVICE=..
-sOutputFile=.. -c <ps> -f <infile> -c showpage`) is the one quoted in the
GHSA; it is not part of the diff. The token classifier follows the GHSA
and the comment in the fix (new lines 106-108): leading `-` or `--` is an
option, leading `@` an argument file, anything else a path.

## Differential evidence (`results/`)

| Case | pre | post |
|---|---|---|
| `-h`, `--help`, `-dNOSAFER.eps`, `-sOutputFile=x.eps`, `-` | `OPTION` | `PATH` (`/srv/uploads/-h`, ...) |
| `@args.txt`, `@` | `ATFILE` | `PATH` |
| `image.eps`, `./-h`, `/data/-h.eps` | `PATH` | `PATH` |
| `-h` that does not exist (`exists=0`) | `PATH` (mkstemp branch) | `PATH` |

`results/sweep.csv` tries every printable first byte: before the fix only
`0x2d` (`-`) and `0x40` (`@`) leave `PATH`; after it, every result starts
with `/` (`0x2f`). On 20,000 generated names (`tests/test_model.c`) the
post-fix path is always absolute and normalized, and the pre-fix class
depends only on the first byte.

## Run

```sh
sh scripts/build.sh test              # -Wall -Wextra -Werror, ASan + UBSan
python3 scripts/derive_expected.py | diff - fixtures/expected.tsv
sh scripts/build.sh results /tmp/r && diff -r results /tmp/r
./build/pilgs case pre '-h' 1 /srv/uploads      # prints the argv; runs nothing
```

`fixtures/cases.tsv` holds positive, negative, edge and out-of-domain cases
with labels derived by hand. `fixtures/expected.tsv` comes from
`scripts/derive_expected.py`, which uses Python's own `posixpath` instead of
the C code; the C tests compare against both.

A model is not a finding: see [LIMITATIONS.md](LIMITATIONS.md) and
[SOURCES.md](SOURCES.md). License: Apache-2.0.
