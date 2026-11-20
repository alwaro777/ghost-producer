# F0-T10 — ESTADO, CHANGELOG y guía de prueba de F0
Dueño: documentalista · Modelo: haiku · Depende de: T9 y revisión · Paralelo con: —

## Objetivo
`docs/ESTADO.md`, `CHANGELOG.md` y `docs/guias/F0-prueba-manual.md` describen lo que hay en F0
y cómo probarlo en macOS en 10 minutos.

## Contexto mínimo (lee SOLO esto)
- docs/tareas/F0-PLAN.md y las 10 primeras líneas de cada F0-T*.informe.md
- Sección «Prueba manual» de cada tarjeta F0-T*.md
- CLAUDE.md § Comandos

## Puedes editar
- docs/ESTADO.md, CHANGELOG.md, docs/guias/F0-prueba-manual.md

## No hagas
- Documentar funciones que no estén en los informes como HECHO.

## Criterios de aceptación (comprobables)
- [ ] ESTADO.md: fase actual, qué funciona, deuda conocida, preguntas abiertas de F0-PLAN.
- [ ] CHANGELOG.md con la entrada `0.1.0 — F0 Cimientos` (formato Keep a Changelog, español).
- [ ] Guía: requisitos (Xcode CLT, CMake ≥ 3.25, Ninja), compilar, abrir la app y los pasos de
      cierre con el resultado esperado de cada uno:
      1. Abrir la app → «Sin título — GhostProducer».
      2. Ajustes de audio → salida integrada → el estado muestra el dispositivo.
      3. Activar el tono → se oye 440 Hz, los medidores se mueven, CPU visible.
      4. Arrastrar la frecuencia → sin clics; ⌘Z vuelve al valor anterior en un paso; ⇧⌘Z rehace.
      5. Añadir dos pistas, renombrar una, cambiar el tempo → el título muestra «•».
      6. Guardar como «Prueba» → cerrar → abrir → todo igual y sin «•»; ⌘Z no hace nada.
      7. Desconectar o cambiar la salida durante la reproducción → la app no se cuelga.
- [ ] Qué reportar si algo falla (pasos, captura de la barra de estado, versión de macOS).

## Prueba manual (para la guía del usuario)
(Es la propia guía.)
