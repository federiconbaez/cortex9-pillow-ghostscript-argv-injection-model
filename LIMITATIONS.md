# Limitations

- **A model, not a finding.** Nothing here was reproduced against Pillow
  or Ghostscript. It restates the public advisory and the fix commit.
- **No Ghostscript.** The classifier only encodes what the GHSA and the
  fix comment say: leading `-`/`--` is an option, leading `@` an argument
  file. What Ghostscript actually does with each option, whether a lone
  `-` means standard input, and whether `--` ends option parsing in a given
  Ghostscript version are **not verified**.
- **argv layout.** It comes from the GHSA text, not from the diff; the
  numeric and device arguments are placeholders.
- **`os.path.exists` is an input.** The model does not touch a file
  system; `exists=0` sends a name to the `mkstemp()` branch, whose path is a
  fixed absolute placeholder.
- **POSIX only.** `abspath` is `posixpath` (two leading slashes are kept).
  Windows paths and drive letters are not modelled. `fp.name` as `bytes` or
  as an integer file descriptor is not modelled.
- **Input domain:** printable names plus the TSV escapes, shorter than 8192
  bytes; the working directory must be absolute. Anything else is
  `BAD_INPUT`.
- **Sweep:** `results/sweep.csv` covers the first byte only; it is a grid,
  not a proof.
- Nothing is attributed beyond the commit and advisories in SOURCES.md.
