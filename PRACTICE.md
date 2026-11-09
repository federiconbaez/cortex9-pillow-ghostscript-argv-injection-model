# Guía práctica (7 niveles)

Ejercicios **offline y locales** sobre cómo un nombre de archivo termina
leído como opción. No se ejecuta Ghostscript ni Pillow. Cada respuesta está
comprobada en `tests/test_model.c` (`practice_level_N`), compilado con
ASan/UBSan. Para practicar con binarios reales, hacelo solo en un lab
propio (una VM o contenedor tuyo, sin red), nunca contra sistemas de
terceros.

Comandos: `sh scripts/build.sh test`; después de `sh scripts/build.sh
results /tmp/r`: `./build/pilgs case pre|post '<nombre>' <0|1> <cwd>`.

**Básico**

1. **El camino normal.** `image.eps` en `/srv/uploads`. ¿Qué token queda
   después de `-f`? (`practice_level_1`: `image.eps`, `PATH`)
2. **Un guion al inicio.** El mismo archivo se llama `-h`. ¿Cómo lo lee un
   parser de argumentos? (`practice_level_2`: `OPTION`)
3. **La arroba.** `@args.txt`. (`practice_level_3`: `ATFILE`; según el
   GHSA, Ghostscript leería más argumentos desde ese archivo)

**Intermedio**

4. **El fix.** `-h` en `post`. (`practice_level_4`: `/srv/uploads/-h`,
   `PATH`: empieza con `/`)
5. **La defensa clásica.** `./-h` antes del fix. ¿Por qué ya era seguro?
   (`practice_level_5`: `PATH`)
6. **La otra rama.** `-h` con `exists=0`. (`practice_level_6`: rama
   `TEMP`, `PATH`: el nombre no llega al argv)

**Avanzado**

7. **Normalización.** `../../../../-h` desde `/srv/uploads` en `post`.
   (`practice_level_7`: `/-h`; `..` no sube más allá de la raíz)

Más allá: agregá un caso a `fixtures/cases.tsv` con su etiqueta a mano,
regenerá `fixtures/expected.tsv` con `scripts/derive_expected.py` y
comprobá que el modelo en C coincide con la derivación independiente.
Pensá cómo auditarías en tu propio código otros sitios donde un nombre
controlado por el usuario llega a un argv (`--`, rutas absolutas, `./`).
