# F0-T8 — Spec UX de la ventana de F0 y catálogo es.json
Dueño: producto-ux · Modelo: sonnet · Depende de: — · Paralelo con: T1, T2, T3

## Objetivo
Existen la spec de la ventana principal de F0 (estados, textos, errores) y el catálogo
`assets/i18n/es.json` con todas las claves que usarán T6 y T7.

## Contexto mínimo (lee SOLO esto)
- docs/tareas/F0-PLAN.md § Objetivos y cierre
- include/ui/Localisation.h (convención de claves)
- include/commands/ProjectCommands.h (tabla de `descriptionKey`)
- include/core/Status.h (`ErrorCode`, uno por mensaje de error)
- docs/PRODUCTO.md § «MODO PRINCIPIANTE» (solo tono de los textos: claro, sin jerga)

## Puedes editar
- docs/specs/ux/F0-ventana-principal.md
- assets/i18n/es.json

## No hagas
- Diseñar funciones fuera de F0. Copiar textos de otros productos.

## Criterios de aceptación (comprobables)
- [ ] Spec con: disposición (boceto ASCII), cada control con su rango y estado, barra de
      estado, diálogo de cambios sin guardar, aviso sin dispositivo de audio y errores al abrir.
- [ ] `es.json` es un objeto plano UTF-8 válido; claves con la convención de Localisation.h.
- [ ] Contiene: todas las `cmd.*` de ProjectCommands.h; `error.<ErrorCode>` para cada código
      salvo `none`; `menu.file.*`, `menu.edit.*` (incluido «Deshacer: {0}»), `window.untitled`,
      `dialog.unsaved.*`, `panel.tone.*`, `panel.transport.*`, `panel.tracks.*`,
      `status.*` (dispositivo, CPU, MIDI, violaciones RT), `audio.noDevice.*`.
- [ ] La spec lista todas las claves del JSON y viceversa (sin claves huérfanas).
- [ ] Textos en español neutro, ≤ 60 caracteres en botones y menús.

## Prueba manual (para la guía del usuario)
(Los textos se ven en la app en T6/T7.)
