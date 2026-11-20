# Prueba de F0 en macOS — GhostProducer 0.1.0

Objetivo: compilar la app en tu Mac, oír un tono de prueba y comprobar que guardar, abrir y deshacer
funcionan. No necesitas saber programar: solo copiar comandos en Terminal.

Tiempo: unos 10 minutos de prueba. La primera compilación tarda entre 15 y 40 minutos, porque
descarga JUCE y compila todo.

## Requisitos
- Un Mac con Apple Silicon (M1 o posterior) y macOS 12 o superior.
- **Xcode Command Line Tools**: abre Terminal y ejecuta `xcode-select --install`. Si ya están
  instaladas, te lo dirá y no pasa nada.
- **Homebrew** (si no lo tienes): instálalo con el comando que aparece en https://brew.sh.
- **CMake 3.25 o superior y Ninja**: ejecuta `brew install cmake ninja`.
  Comprueba con `cmake --version`: la primera línea debe mostrar 3.25 o más.
- **Audio**: usa la salida integrada del Mac (altavoces) o unos auriculares. Sube el volumen
  despacio: el tono es continuo.

## Compilar y abrir la app
1. En Terminal, descarga la rama de prueba y entra en la carpeta:
   ```
   git clone -b claude/happy-clarke-1dfcdf https://github.com/alwaro777/ghost-producer.git
   cd ghost-producer
   ```
   → deberías ver «Cloning into…» y, al terminar, ningún mensaje de error.
2. Configura la compilación:
   ```
   cmake --preset dev
   ```
   → la última línea dice «Build files have been written to…», sin líneas que empiecen por «error».
3. Compila:
   ```
   cmake --build --preset dev
   ```
   → tarda varios minutos la primera vez. Al final no debe quedar ninguna línea que empiece por «error».
4. Abre la app:
   ```
   open build/dev/GhostProducerApp_artefacts/Debug/GhostProducer.app
   ```
   → se abre la ventana. Si macOS muestra un aviso de seguridad, no lo hemos probado: copia el texto
   exacto del aviso en tu feedback.

## Pasos de prueba
1. **Abre la app.** → la ventana se titula «Sin título — GhostProducer» y hay menús Archivo y Edición.
2. **Archivo → Ajustes de audio.** Elige la salida integrada del Mac. → la barra de estado deja de decir
   «Audio: sin dispositivo» y muestra la salida, la frecuencia y el búfer (p. ej. «Audio: … · 48000 Hz · búfer 256»).
3. **En «Tono de prueba», activa «Sonar».** → oyes un tono continuo de 440 Hz (la nota La), los
   medidores L y R se mueven y la barra de estado muestra la CPU.
4. **Arrastra el deslizador de frecuencia de 440 a 880 Hz.** → el tono sube sin clics ni chasquidos.
   Pulsa **⌘Z** → vuelve a 440 Hz en un solo paso. Pulsa **⇧⌘Z** → vuelve a 880 Hz.
5. **Pulsa «Añadir pista» dos veces, renombra una con doble clic y escribe 140 en el tempo.**
   → aparecen dos pistas, y el título pasa a «Sin título (sin guardar) — GhostProducer».
   Edición muestra «Deshacer: cambiar el tempo».
6. **Archivo → Guardar como… → escribe «Canción ñandú».** → se crea
   `Documentos/Canción ñandú/Canción ñandú.gpproj`, y las carpetas de apoyo quedan dentro de esa carpeta.
   El título es «Canción ñandú — GhostProducer», sin «(sin guardar)».
7. **Cierra la app con ⌘Q y vuelve a abrirla con el comando `open` de la sección de compilación.
   Archivo → Abrir… → elige «Canción ñandú.gpproj».** → el tempo es 140, están las dos pistas
   (una con el nombre nuevo), el tono está a 880 Hz y el título no lleva «(sin guardar)».
   Edición → Deshacer aparece deshabilitado.
8. **Con el tono sonando, cambia la salida en Ajustes de audio (o desconecta los auriculares).**
   → la app sigue respondiendo y no se cuelga. La barra de estado debe mostrar el nuevo estado o
   «sin dispositivo». *Este paso no está probado todavía: es justo lo que queremos saber.*

## Qué NO funciona todavía
- No hay reproducción de línea de tiempo, clips ni pistas de audio: las pistas solo se añaden,
  renombran y eliminan.
- No hay plugins, autoguardado, recuperación de proyectos, temas claros, Modo Principiante ni IA.
- Los atajos no se pueden cambiar.
- Si cierras la app de golpe (sin Salir), no se guarda la salida elegida en Ajustes de audio.
- El último proyecto no se reabre solo al iniciar la app.
- Si «Sonar» está deshabilitado porque no hay dispositivo, el aviso no vuelve a aparecer al pulsarlo.
- Los nombres por defecto de pista («Pista N») pueden repetirse después de eliminar una.
- Windows y dispositivos de audio profesionales (ASIO) no están probados ni incluidos.

## Cómo reportar un fallo
Copia esta plantilla, rellénala y envíala tal cual:

```
Feedback de F0 (macOS)
- Mac (modelo y chip):
- Versión de macOS (Apple → Acerca de este Mac):
- Salida de audio usada (altavoces, auriculares, otra):
- Paso de la guía en el que pasa (1–8):
- Qué hice:
- Esperaba:
- Obtuve:
- Texto de la barra de estado o captura de pantalla:
- Funciona bien:
- Lo que más te molesta (UX):
```

Incluye siempre la captura de la barra de estado y los pasos exactos. Si la app se cierra sola, dime
en qué paso ocurrió.
