# Cómo contribuir a GhostProducer

GhostProducer es software libre bajo la **GNU AGPLv3** (`LICENSE.md`). Al contribuir aceptas que
tu aportación se distribuya bajo esa misma licencia. No pedimos cesión de derechos ni CLA: cada
autor conserva el copyright de su código (ver ADR 0001 § Estrategia de licencias).

## Developer Certificate of Origin (DCO)
Cada commit debe llevar la línea `Signed-off-by`, con la que certificas el
[Developer Certificate of Origin 1.1](https://developercertificate.org/): que tienes derecho a
enviar ese código bajo la licencia del proyecto.

```
git commit -s -m "engine: rampa de ganancia del tono para evitar clics"
```

Eso añade al final del mensaje:

```
Signed-off-by: Tu Nombre <tu@correo.example>
```

Usa tu nombre real o un seudónimo estable y un correo en el que se te pueda localizar. La CI
rechazará los PR con commits sin `Signed-off-by`. Si olvidaste firmar:
`git commit --amend -s` (último commit) o `git rebase --signoff main` (varios).

## Reglas del proyecto
- Lee `CLAUDE.md` (reglas no negociables y mapa de propiedad) y `docs/ARQUITECTURA.md`.
- El hilo de audio no reserva memoria, no usa locks, no hace I/O ni escribe logs.
- Todo cambio del documento es un `Command` con undo y redo.
- Código y nombres en inglés; documentación y textos de interfaz en español (por clave i18n).
- Formatea con el `.clang-format` del repo. Commits pequeños: `area: qué y por qué`.
- Nada de assets, textos ni presets copiados de productos comerciales.
- Toda dependencia nueva se anota antes en `THIRD_PARTY.md` con su licencia; solo se aceptan
  licencias compatibles con AGPLv3.

## Antes de abrir un PR
```
cmake --preset dev && cmake --build --preset dev && ctest --preset dev
```
En Linux sin interfaz gráfica, usa el preset `headless`.
