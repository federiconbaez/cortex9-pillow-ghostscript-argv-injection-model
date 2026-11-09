# Sources

Local copies under `security-research-45/sources/cve2026/` (not in this
repo), fetched read-only; the SHA-256 of each copy is listed in that
directory's `SHA256SUMS` and was re-checked on 2026-11-09.

| Source | URL | Used for | SHA-256 of copy |
|---|---|---|---|
| Fix commit a2c363141da31a265dd536c945677db4b64f3ec9 diff ("Transform EPS file name to an absolute path before invoking Ghostscript (#10090)", 2026-10-01, 2 files +20/-1) | https://github.com/python-pillow/Pillow/commit/a2c363141da31a265dd536c945677db4b64f3ec9 | `EpsImagePlugin.py` 105, old 106, new 106-109, 108/111; the test `test_filename_not_parsed_as_ghostscript_argument` (name `-h`) | 00a2ab708877deba108cf5d2b738f3144f6b7b4eda1f181d40f7836ea4d10c0f |
| GHSA-w889-685w-c8fh (GitHub advisory JSON) | https://github.com/python-pillow/Pillow/security/advisories/GHSA-w889-685w-c8fh | argv layout with `-f infile`, `-`/`--`/`@` semantics, `>= 2.6.0, < 13.0.0` -> 13.0.0, CWE-78/CWE-88 | a5c4d2eb7ec57122878c5e7ae5e215adc9df9f1b22abb027c01043d17231b826 |
| NVD CVE-2026-112900 (Analyzed, published 2026-10-19) | https://nvd.nist.gov/vuln/detail/CVE-2026-112900 | description ("after -f"), CWE-78, CWE-88 | 84ec1e548b485890211810cf9d4056fa52292431ae5da4fc8a275a879df50b1b |

`tempfile.mkstemp()` returning an absolute path was checked in the local
CPython 3.13 `tempfile.py` (`return _os.path.abspath(file)`).

No source contained instructions addressed to a tool or agent. The GHSA
includes a third-party PoC description; it was not copied or run.
