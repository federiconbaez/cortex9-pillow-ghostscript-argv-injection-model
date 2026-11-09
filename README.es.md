# cortex9-pillow-ghostscript-argv-injection-model

Modelo offline de **CVE-2026-112900** (GHSA-w889-685w-c8fh): el
`EpsImagePlugin.Ghostscript()` de Pillow ponía `fp.name` tal cual después
de `-f` en el argv de Ghostscript, así que un nombre de archivo que empieza
con `-` se leía como opción y uno que empieza con `@` como archivo de
argumentos. El fix, commit a2c363141da3 (Pillow 13.0.0), usa
`os.path.abspath(fp.name)`. Educativo, determinista, en C11 comprobado con
ASan/UBSan. **No se ejecuta Ghostscript, no se importa Pillow, no se abre
ningún archivo y no hay red.** El modelo arma el argv como datos y
clasifica el token.

## Qué se modela (ligado al diff)

| Línea del diff | Código | Modelo |
|---|---|---|
| `EpsImagePlugin.py` 105 (contexto) | `if hasattr(fp, "name") and os.path.exists(fp.name):` | `exists` es una entrada de la fixture |
| vieja 106 | `infile = fp.name` | `pre`: el nombre pasa sin cambios |
| nueva 109 | `infile = os.path.abspath(fp.name)` | `post`: `normpath(join(cwd, name))` |
| vieja 108 / nueva 111 (contexto) | `tempfile.mkstemp()` | rama `TEMP`: un marcador absoluto |

El orden del argv es el que cita el GHSA; no forma parte del diff. El
clasificador sigue el GHSA y el comentario del fix (nuevas 106-108): `-` o
`--` al inicio es opción, `@` al inicio es archivo de argumentos, el resto
es ruta.

## Evidencia diferencial

Antes del fix, `-h`, `--help`, `-dNOSAFER.eps` y `-` dan `OPTION`, y
`@args.txt` da `ATFILE`; después, todos dan `PATH` (`/srv/uploads/-h`...).
`image.eps`, `./-h` y `/data/-h.eps` ya eran `PATH`. En `results/sweep.csv`
solo los bytes `0x2d` y `0x40` salen de `PATH` antes del fix.

## Uso

```sh
sh scripts/build.sh test
python3 scripts/derive_expected.py | diff - fixtures/expected.tsv
sh scripts/build.sh results /tmp/r && diff -r results /tmp/r
```

Un modelo no es un hallazgo: ver [LIMITATIONS.md](LIMITATIONS.md) y
[SOURCES.md](SOURCES.md). Ejercicios en [PRACTICE.md](PRACTICE.md).
Licencia: Apache-2.0.
