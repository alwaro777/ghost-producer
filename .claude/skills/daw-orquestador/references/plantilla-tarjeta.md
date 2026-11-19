# Plantilla de tarjeta de tarea

Archivo: `docs/tareas/F{n}-T{m}-{slug}.md`. Una tarjeta buena se ejecuta sin preguntar nada.

```markdown
# F{n}-T{m} — {título corto}
Dueño: {agente} · Modelo: {sonnet|haiku|opus} · Depende de: {tarjetas o "—"} · Paralelo con: {tarjetas}

## Objetivo
Una o dos frases: qué existe al terminar que antes no existía.

## Contexto mínimo (lee SOLO esto)
- include/engine/Transport.h (contrato a implementar)
- docs/PRODUCTO.md § "Núcleo > Transporte"
- docs/adr/0003-reloj-y-tempo.md

## Puedes editar
- src/engine/transport/**
- tests/engine/transport/**

## No hagas
- Cambiar include/ (si el contrato no alcanza, para y repórtalo)
- {otras exclusiones}

## Criterios de aceptación (comprobables)
- [ ] ...
- [ ] Test unitario X pasa
- [ ] Sin allocations en processBlock (detector RT en debug)

## Prueba manual (para la guía del usuario)
1. Haz ... → deberías oír/ver ...
```

# Plantilla de informe

Archivo: `docs/tareas/F{n}-T{m}-{slug}.informe.md`. Al orquestador se le devuelven solo
las primeras 10 líneas.

```markdown
Estado: HECHO | PARCIAL | BLOQUEADO
Archivos: src/engine/transport/Transport.cpp (+210), tests/... (+90)
Criterios: 4/4 ✓
Bloqueos: — | {descripción + quién lo desbloquea}
Decisiones locales: {1–3 líneas}
Deuda/TODO: {1–3 líneas}
---
(detalle opcional: notas técnicas, mediciones de CPU y latencia)
```
