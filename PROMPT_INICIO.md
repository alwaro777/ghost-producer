# GhostProducer — kit multiagente: cómo usarlo

## Qué contiene
```
GhostProducer/
├── PROMPT_INICIO.md            ← este archivo: prompt de arranque y prompts de iteración
├── CLAUDE.md                   ← constitución corta del proyecto (la leen todos los agentes)
├── .claude/
│   ├── agents/                 ← 9 subagentes con rol, herramientas y modelo propios
│   │   arquitecto (opus) · motor-audio · dsp-dispositivos · ui-ux · ia-ml · producto-ux
│   │   qa (sonnet) · revisor (opus, solo lectura) · documentalista (haiku)
│   └── skills/daw-orquestador/ ← skill del orquestador: ciclo de fases, gates, delegación,
│       ├── SKILL.md               reglas de ahorro de tokens
│       └── references/         roadmap · plantilla de tarjeta e informe · checklist de tiempo real
└── docs/
    ├── PRODUCTO.md             ← especificación completa (el QUÉ)
    ├── REFERENCIA_LIVE12.md    ← inventario funcional de Live 12 Suite (alcance)
    └── tareas/                 ← aquí nacen las tarjetas y los informes
```

## Instalación (1 minuto)
1. Clona este repo. Ya trae el kit en la raíz, incluida la carpeta oculta `.claude/`.
2. Abre Claude Code en ese repo. Comprueba con `/agents` que aparecen los 9 agentes y con
   `/skills` que aparece `daw-orquestador`.
3. Pega el prompt de arranque.

## Por qué así cuesta menos y sale mejor
- **Modelo según la tarea**: opus solo donde se piensa (arquitectura y revisión), sonnet para
  implementar y haiku para la documentación.
- **Contexto aislado**: cada agente lee su tarjeta y los 3–5 archivos que necesita, no el
  repo entero. El orquestador solo lee informes de ≤10 líneas.
- **Contract-first**: el arquitecto fija las interfaces antes de implementar, así que varios
  agentes trabajan en paralelo sin pisarse (con propiedad de carpetas y worktrees).
- **Memoria en archivos, no en chat**: tarjetas, informes, ESTADO y ADR. Cualquier sesión nueva
  retoma el trabajo leyendo `docs/ESTADO.md`, sin repetir contexto.
- **Gates**: QA y revisor detectan los fallos antes de que llegues a probar, lo que reduce las
  vueltas.

---

## PROMPT DE ARRANQUE (pégalo en la primera sesión)

```
Carga la skill daw-orquestador y actúa como orquestador del proyecto descrito en
docs/PRODUCTO.md: una DAW de escritorio inspirada en el flujo Sesión + Arreglo de Live 12
Suite (alcance en docs/REFERENCIA_LIVE12.md), con Modo Principiante e IA integrada, identidad
y contenido 100 % originales.

Objetivo: construirla por fases, con calidad profesional y el mínimo coste de tokens,
delegando en los subagentes de .claude/agents según la skill. Tú no programas: planificas,
creas tarjetas, lanzas agentes (en paralelo cuando la propiedad es disjunta), haces cumplir
los gates (plan → construir → qa → revisor → documentar) y me entregas cada fase lista para
probar.

Mis preferencias:
- Español en docs, textos de UI y comunicación conmigo; inglés en el código.
- Plataforma principal de pruebas: [Windows | macOS]. Interfaz de audio: [modelo o "integrada"].
- Prioridad: estabilidad del audio > funcionalidad > estética.
- Pregúntame solo lo que sea caro de revertir (formato de proyecto, licencias, dependencias
  grandes). Lo demás, decídelo con el estándar de la industria y anótalo en un ADR.
- Al terminar cada fase PARA y dame: novedades (3–6 viñetas), la guía de prueba y las
  limitaciones conocidas.

Empieza por la Fase 0: que el arquitecto haga el plan y los ADR del stack (incluida la
decisión sobre Tracktion Engine y las licencias de JUCE, VST3, Link y time-stretch).
Muéstrame el plan resumido antes de construir.
```

---

## PROMPTS DE ITERACIÓN

**Retomar en una sesión nueva**
```
Carga la skill daw-orquestador. Lee docs/ESTADO.md y dime en 5 líneas dónde estamos y qué
toca ahora. No lances agentes hasta que te lo confirme.
```

**Reportar lo que probaste** (pega la plantilla de la guía de prueba rellenada)
```
Feedback de la Fase N:
- Funciona: ...
- Falla: [pasos] → esperaba [X], obtuve [Y]
- UX / no me gusta: ...
Clasifícalo P0/P1/P2, crea las tarjetas de bug y ejecútalas con test de regresión. No avances
de fase.
```

**Aprobar y avanzar**
```
Fase N aprobada. Que el documentalista etiquete la versión vN-alpha y arranca la Fase N+1
(plan resumido primero).
```

**Pedir una feature nueva**
```
Quiero añadir [feature]. Que el arquitecto diga en qué fase encaja, sus dependencias, su
impacto en Commands, motor y formato de proyecto, y su coste estimado en tarjetas. Espera mi OK.
```

**Pulido de UX**
```
Que producto-ux audite [pantalla] con ojos de principiante y de productor pro: los 10
problemas principales y una solución para cada uno. Después ui-ux implementa las 5 de más
impacto. Revisor y qa como siempre.
```

**Auditoría de rendimiento o estabilidad**
```
Que qa ejecute el test de estrés y que revisor audite el hilo de audio con la checklist de
tiempo real. Informe ordenado por gravedad y tarjetas para lo bloqueante.
```

**Control de gasto**
```
Resume el gasto de esta fase: cuántos agentes lancé, de qué modelo y cuántas vueltas por
gate. Propón 3 ajustes en las tarjetas o en la skill para gastar menos en la siguiente fase.
```

---

## Licencias a decidir pronto
JUCE (AGPLv3 o comercial) · VST3 SDK (licencia propia o GPLv3) · Ableton Link (GPLv2 o
comercial) · Rubber Band (GPL o comercial) · Tracktion Engine (GPL o comercial) · cada modelo
de IA (revisa su checkpoint). Si vas a vender la app, presupuesta las licencias comerciales o
elige alternativas permisivas.
