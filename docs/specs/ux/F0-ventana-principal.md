# Spec UX — Ventana principal de F0

Nivel de interfaz: único (F0 no tiene niveles Descubrir/Crear/Pro). Textos en `assets/i18n/es.json`
(se citan por clave; el texto exacto vive allí). Rangos de `model::limits`.

## Objetivo del usuario
Abrir la app, oír un tono, cambiar tempo/compás y pistas, deshacer, guardar y volver a abrir.
Valor en 5 minutos: de abrir la app a oír el tono en 3 clics (Sonar).

## Disposición

```
+--------------------------------------------------------------+
| Archivo  Edición                       {window.title}        |
+--------------------------------------------------------------+
| TIEMPO (panel.transport)      | TONO DE PRUEBA (panel.tone)  |
|  Tempo   [ 120.0 ] BPM        |  [x] Sonar                   |
|  Compás  [ 4 ] / [ 4 ]        |  Frecuencia [===o====] 440 Hz|
|                               |  Volumen    [===o====] -18 dB|
+-------------------------------+------------------------------+
| PISTAS (panel.tracks)             [ Añadir pista ]           |
|  1  Pista 1            [Renombrar] [Eliminar]                |
|  (vacío: texto de ayuda)                                     |
+--------------------------------------------------------------+
| Audio: X · 48000 Hz · búfer 256 | CPU 3 % | Pico -20 dB |    |
| MIDI: Y | Tiempo real: sin incidencias                       |
+--------------------------------------------------------------+
```

## Controles, rangos y estados
Todos: estado normal, foco visible, deshabilitado solo si no hay dispositivo (solo «Sonar»).
Arrastrar un control = un solo paso de deshacer (fusión de gestos). Un valor fuera de rango
no se aplica y se muestra `error.invalidArgument` junto al control; el control vuelve al
último valor válido (no se corrige en silencio).

| Control | Claves | Rango / valor inicial | Comando |
|---|---|---|---|
| Tempo | `panel.transport.title`, `panel.transport.tempo`, `panel.transport.tempo.unit`, `panel.transport.tempo.tip` | 20–999 BPM · 120 | `cmd.transport.setTempo` |
| Compás | `panel.transport.timeSignature`, `panel.transport.timeSignature.tip` | numerador 1–32; denominador potencia de 2 entre 1 y 32 · 4/4 | `cmd.transport.setTimeSignature` |
| Sonar | `panel.tone.title`, `panel.tone.enable`, `panel.tone.enable.tip` | apagado al abrir | `cmd.testTone.setEnabled` |
| Frecuencia | `panel.tone.frequency`, `panel.tone.frequency.unit`, `panel.tone.frequency.tip` | 20–20000 Hz · 440 (escala logarítmica) | `cmd.testTone.setFrequency` |
| Volumen | `panel.tone.gain`, `panel.tone.gain.unit`, `panel.tone.gain.tip` | −60–0 dB · −18 | `cmd.testTone.setGain` |
| Añadir pista | `panel.tracks.title`, `panel.tracks.add`, `panel.tracks.defaultName` | máx. 1024 pistas; nombre 1–64 bytes UTF-8 | `cmd.track.add` |
| Eliminar | `panel.tracks.remove` | — | `cmd.track.remove` |
| Renombrar | `panel.tracks.rename`, `panel.tracks.nameRule` | no vacío; máx. 64 bytes | `cmd.track.rename` |

Lista de pistas vacía: `panel.tracks.empty`. Texto de deshacer en el menú:
`menu.edit.undoWith` / `menu.edit.redoWith` con `{0}` = texto de la `cmd.*` del último
cambio. Si no hay historial: `menu.edit.undo` / `menu.edit.redo` deshabilitados.

## Descripciones de comandos (historial)
`cmd.transport.setTempo`, `cmd.transport.setTimeSignature`, `cmd.testTone.setEnabled`,
`cmd.testTone.setFrequency`, `cmd.testTone.setGain`, `cmd.track.add`, `cmd.track.remove`,
`cmd.track.rename`.

## Menús
- `menu.file`: `menu.file.new`, `menu.file.open`, `menu.file.save`, `menu.file.saveAs`,
  `menu.file.audioSettings`, `menu.file.quit`.
- `menu.edit`: `menu.edit.undo`, `menu.edit.undoWith`, `menu.edit.redo`, `menu.edit.redoWith`.

## Título de ventana
`window.untitled` (proyecto nuevo), `window.title` (`{0}` = nombre), `window.title.modified`
(con cambios sin guardar).

## Barra de estado
| Elemento | Claves | Notas |
|---|---|---|
| Dispositivo | `status.device` (`{0}` nombre, `{1}` Hz, `{2}` búfer), `status.device.none` | |
| CPU | `status.cpu` (`{0}` entero, 0–100) | se actualiza ~4 veces/s |
| Pico | `status.peak` (`{0}` dB, un decimal) | |
| MIDI | `status.midi`, `status.midi.none` | |
| Tiempo real | `status.rt.ok`, `status.rt.violations` (`{0}` nº) | solo en builds de depuración |
| Confirmaciones | `status.saved`, `status.opened` (`{0}` nombre) | se ocultan a los 4 s |

## Diálogo de cambios sin guardar
Al elegir Nuevo, Abrir o Salir con cambios. Claves: `dialog.unsaved.title` (`{0}` nombre),
`dialog.unsaved.message`, `dialog.unsaved.save` (por defecto, Intro), `dialog.unsaved.discard`,
`dialog.unsaved.cancel` (Esc; no hace nada y vuelve al proyecto). Si «Guardar» falla,
se muestra el error de guardado y el diálogo no se cierra.

## Aviso sin dispositivo de audio
Banner no modal bajo el menú (no bloquea el resto de la app; «Sonar» deshabilitado).
Claves: `audio.noDevice.title`, `audio.noDevice.message`, `audio.noDevice.openSettings`
(abre los ajustes), `audio.noDevice.dismiss`. Reaparece al intentar activar «Sonar».
Ajustes de audio: `audio.settings.title`, `audio.settings.output`, `audio.settings.sampleRate`,
`audio.settings.bufferSize`, `audio.settings.bufferSize.tip`, `audio.settings.midiInput`,
`audio.settings.close`. Si el dispositivo elegido falla: `error.deviceError` con salida
«Abrir ajustes de audio».

## Errores al abrir y guardar
Diálogo con título (`error.dialog.openTitle` o `error.dialog.saveTitle`), texto según código y
botones. Nunca culpa al usuario; siempre hay salida. El proyecto actual no se toca si abrir falla.

| Código | Clave | Salidas |
|---|---|---|
| invalidArgument | `error.invalidArgument` | `error.dialog.ok` |
| invalidState | `error.invalidState` | `error.dialog.ok` |
| ioError | `error.ioError` | `error.dialog.retry`, `error.dialog.saveAs` (al guardar) o `error.dialog.chooseOther` (al abrir) |
| parseError | `error.parseError` | `error.dialog.chooseOther`, `error.dialog.details` |
| unsupportedVersion | `error.unsupportedVersion` | `error.dialog.chooseOther` |
| migrationFailed | `error.migrationFailed` | `error.dialog.chooseOther`, `error.dialog.details` |
| unknownCommand | `error.unknownCommand` | `error.dialog.ok` |
| deviceError | `error.deviceError` | `audio.noDevice.openSettings` |

Además: `error.dialog.ok` (Entendido), `error.dialog.details` (muestra el `message` técnico,
en inglés, copiable).

## Estados
- Vacío: proyecto nuevo, sin pistas (`panel.tracks.empty`), tono apagado.
- Carga: abrir es casi instantáneo en F0; si tarda >300 ms, cursor de espera, sin diálogo.
- Error: ver tablas anteriores.

## Métricas de éxito (prueba con novato)
- ≥ 4 de 5 personas oyen el tono en < 60 s sin ayuda.
- 5 de 5 encuentran Deshacer en < 30 s.
- Ninguna pierde cambios por cerrar sin guardar.
- Ningún texto de error requiere explicación del moderador.

## Índice de claves (debe coincidir con `assets/i18n/es.json`)
cmd.transport.setTempo · cmd.transport.setTimeSignature · cmd.testTone.setEnabled ·
cmd.testTone.setFrequency · cmd.testTone.setGain · cmd.track.add · cmd.track.remove ·
cmd.track.rename · error.invalidArgument · error.invalidState · error.ioError ·
error.parseError · error.unsupportedVersion · error.migrationFailed · error.unknownCommand ·
error.deviceError · error.dialog.openTitle · error.dialog.saveTitle · error.dialog.ok ·
error.dialog.retry · error.dialog.chooseOther · error.dialog.saveAs · error.dialog.details ·
menu.file · menu.file.new · menu.file.open · menu.file.save · menu.file.saveAs ·
menu.file.audioSettings · menu.file.quit · menu.edit · menu.edit.undo · menu.edit.undoWith ·
menu.edit.redo · menu.edit.redoWith · window.untitled · window.title · window.title.modified ·
dialog.unsaved.title · dialog.unsaved.message · dialog.unsaved.save · dialog.unsaved.discard ·
dialog.unsaved.cancel · panel.tone.title · panel.tone.enable · panel.tone.enable.tip ·
panel.tone.frequency · panel.tone.frequency.unit · panel.tone.frequency.tip · panel.tone.gain ·
panel.tone.gain.unit · panel.tone.gain.tip · panel.transport.title · panel.transport.tempo ·
panel.transport.tempo.unit · panel.transport.tempo.tip · panel.transport.timeSignature ·
panel.transport.timeSignature.tip · panel.tracks.title · panel.tracks.add · panel.tracks.remove ·
panel.tracks.rename · panel.tracks.empty · panel.tracks.defaultName · panel.tracks.nameRule ·
status.device · status.device.none · status.cpu · status.peak · status.midi · status.midi.none ·
status.rt.ok · status.rt.violations · status.saved · status.opened · audio.noDevice.title ·
audio.noDevice.message · audio.noDevice.openSettings · audio.noDevice.dismiss ·
audio.settings.title · audio.settings.output · audio.settings.sampleRate ·
audio.settings.bufferSize · audio.settings.bufferSize.tip · audio.settings.midiInput ·
audio.settings.close
