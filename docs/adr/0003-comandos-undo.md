# ADR 0003 — Sistema Command con undo/redo serializable

Estado: aceptado (F0, G1) · Fecha: 2026-11-20 · Contratos: `include/model/Document.h`,
`include/commands/*.h`

## Contexto
Todo cambio del usuario o de la IA debe ser un `Command` serializable con undo/redo. Sobre él
se construirán el historial visual, las macros, el log de acciones, los diffs que enseña el
copiloto y, más adelante, el historial persistente. Cambiarlo cuando haya cientos de comandos
es muy caro.

## Decisión
- **Documento mutable en el hilo de mensajes**: `model::Document` es dueño de un
  `ProjectState` (tipos de valor puros, sin JUCE). Solo `CommandHistory` lo muta (acceso por
  `friend`); `reset()` sirve para Nuevo/Abrir. Todo lo demás lee `state()` y escucha cambios.
- **Command = apply/revert explícitos**: `apply(state)` valida, aplica y guarda en el propio
  objeto los datos de undo (valor anterior, índice, id asignado); `revert(state)` restaura el
  estado exacto. Si `apply` falla, el estado no cambia. Redo = volver a llamar a `apply`, que
  reutiliza los identificadores ya asignados (determinismo para los comandos siguientes).
- **Serializable = parámetros**: `toJson()` → `{"type","version","params"}`. `CommandRegistry`
  reconstruye un Command a partir de ese JSON. Los datos de undo no se serializan: se recalculan
  al aplicar sobre el estado. `typeId` es estable para siempre.
- **Transacciones**: `begin/commit/abortTransaction` agrupan varios Commands en un paso de undo
  (acciones de la IA, operaciones compuestas). Sin anidamiento en F0.
- **Gestos continuos**: `execute(cmd, MergePolicy::withPrevious)` fusiona con la cima mediante
  `Command::mergeWith` (un arrastre de slider = un paso de undo).
- **Estado sucio**: `markSaved()/isDirty()` por posición en la pila, no por comparación.
- **Textos**: los Commands devuelven claves i18n (`cmd.*`), nunca texto visible.
- **Hilos**: todo en el hilo de mensajes. La IA y otros hilos construyen Commands y los
  entregan al hilo de mensajes; nunca tocan el documento.
- **Undo ilimitado** en F0; límite por memoria como punto de extensión.

## Alternativas
- **Snapshots inmutables del estado completo por paso** (undo = volver al estado anterior):
  muy simple y robusto, pero coste de memoria O(estado) por paso salvo con estructuras
  persistentes, y no da por sí solo un Command serializable para la IA. Se deja como posible
  optimización interna (p. ej. para comandos masivos), sin cambiar el contrato.
- **juce::UndoManager + ValueTree**: acopla el modelo a JUCE y no da serialización de acciones.
- **Event sourcing completo** (el proyecto es el log de comandos): potente para historial y
  colaboración, pero carga lenta y migraciones de comandos antiguos muy caras. Descartado; el
  formato serializado de Commands deja la puerta abierta a persistir el historial más adelante.

## Consecuencias
- Cada Command necesita tests de apply → revert (estado igual) → apply (estado igual al primero)
  y de ida y vuelta JSON. QA lo convierte en una prueba genérica parametrizada.
- El motor no ve Commands: ve el estado resultante compilado (ADR 0004).
- Notificación de cambios de grano grueso («algo cambió») en F0; el aviso por ámbito (pista,
  clip) es un punto de extensión para que la UI no repinte todo con 100 pistas.

## Licencias
Sin dependencias nuevas (nlohmann/json, MIT, ya anotada).
