# Changelog

Formato: [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/).

## [0.1.0] — 2026-11-20 — F0 Cimientos

### Añadido
- Ventana principal «Sin título — GhostProducer» con menús Archivo y Edición, en español.
- Panel Tiempo (tempo y compás), panel Tono de prueba (activar, frecuencia y volumen) y panel Pistas
  (añadir, renombrar y eliminar).
- Tono de prueba de 440 Hz sin clics al cambiar frecuencia o volumen, con medidores de nivel L/R,
  medidor de CPU y barra de estado.
- Deshacer y rehacer (⌘Z y ⇧⌘Z) para cada cambio. Un arrastre de control cuenta como un solo paso.
- Guardar, guardar como, abrir y nuevo proyecto. Los proyectos son archivos `.gpproj` legibles
  (JSON). Si un proyecto es de una versión anterior, se migra y se guarda una copia de seguridad.
- «Guardar como» crea una carpeta para cada proyecto (`Nombre/Nombre.gpproj`), con nombres con
  acentos y caracteres no latinos.
- Ajustes de audio: elegir la salida y el dispositivo MIDI. La barra de estado muestra el dispositivo activo.
- Aviso cuando la app no encuentra dispositivo de audio: los demás controles siguen funcionando.

### Limitaciones de esta versión
- Solo probada en Linux. Pendiente de probar en macOS (Apple Silicon) y en Windows.
- No hay reproducción de la línea de tiempo, clips ni pistas de audio todavía.
