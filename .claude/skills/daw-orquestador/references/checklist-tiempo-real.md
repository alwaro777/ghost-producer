# Checklist de tiempo real y calidad de audio

## En el callback de audio (processBlock y todo lo que llama) está prohibido
- `new`/`delete`/`malloc`, `std::vector::push_back` sin reserva, `std::string`, `std::function` que reserve memoria
- `std::mutex`, `lock_guard`, condition variables, `juce::CriticalSection` (bloqueante)
- I/O de archivo, red, logging, `printf`, `DBG`
- Llamadas al sistema o a la UI (`MessageManager`), excepciones
- Algoritmos de coste no acotado (búsquedas lineales sobre listas que crecen)

## Patrones correctos
- Colas SPSC lock-free (UI→audio: comandos de motor; audio→UI: medidores y posición)
- Snapshot inmutable del grafo, intercambiado de forma atómica. El grafo viejo se libera
  fuera del hilo de audio (cola de basura).
- Pools preasignados para voces, eventos MIDI y buffers
- `std::atomic` para parámetros escalares, con smoothing en el motor (sin saltos = sin clics)
- Denormales desactivados (FTZ/DAZ) en el hilo de audio

## Corrección de audio
- Automatización y MIDI con precisión de muestra (los eventos se dividen dentro del bloque)
- Compensación de latencia (PDC) recalculada al cambiar el grafo
- Sin clics al hacer mute, solo, stop o loop: rampas de 1–5 ms
- El render offline produce exactamente lo mismo que el tiempo real (tests golden)
- Probar con buffers de 32, 64, 128, 512 y 2048, y con sample rates de 44,1, 48 y 96 kHz

## Plugins
- Escaneo fuera de proceso con timeout. Un plugin que se cuelga va a la lista negra.
- Las llamadas a plugins nunca bloquean la UI. Gestión de estado (chunks) robusta.

## Herramientas
- En los builds de debug: detector de allocations y locks en el hilo de audio (hook de
  `operator new` con flag thread-local)
- ASan/UBSan en CI · TSan en los tests de concurrencia del motor
