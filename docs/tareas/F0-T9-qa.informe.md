# Informe F0-T9 — QA: integración, matriz RT, CI y verificación de F0 (gate G4)
Estado: HECHO en Linux (dev/headless/headless+ASan/tsan). Sin bugs nuevos. NO verificable aquí: macOS, Windows, CI en PR.

## Resultados (compilación desde limpio, `rm -rf build/dev`)
| Configuración | Build | Tests |
|---|---|---|
| dev (GUI, xvfb disponible) | OK, 0 warnings | 54/54 (1 SKIP: carpeta solo lectura como root) |
| headless | OK (-Werror) | 46/46 (1 SKIP idem) |
| headless + ASan/UBSan (`-DGP_SANITIZERS=address,undefined`) | OK | 46/46, detector RT activo |
| tsan | OK | 46/46 pero 9 SKIP (8 detector T3 + matriz T9), ver deuda |
| `xvfb-run GhostProducer --smoke-test` (dev) | — | rc=0 (solo el `jassertfalse` de ALSA MIDI sin /dev/snd/seq, entorno) |

## Tests añadidos (tests/integration/, recogidos por GLOB; tests/CMakeLists.txt no cambia)
- ProjectLifecycleTests.cpp: Document+CommandHistory+ProjectFile. 6 execute (tempo, gesto fusionado de frecuencia = 1 paso, transacción de 3 comandos, tono, compás, remove) -> save -> load en Document nuevo `==`; undo x6 -> `== ProjectState{}`; redo -> `== expected`.
- CommandRegistryTests.cpp: por cada uno de los 8 typeId de builtinCommands(): contains, apply/revert/apply, JSON ida y vuelta (toJson igual y mismo efecto).
- RenderMatrixTests.cpp: buildSnapshot desde ProjectState, {32,64,128,512,2048} x {44.1,48,96 kHz}, 1 s cada una dentro de ScopedRealtimeSection: 0 violaciones, sin NaN/Inf, pico <= ganancia*1.001 (sin clipping), pico >= 0.95*ganancia, takePeaks ≈ ganancia. SKIP si el detector está OFF.

## Criterios de F0-T9
- ✓ Integración headless (ProjectLifecycleTests, pasa en dev/headless/ASan/tsan).
- ✓ Prueba genérica builtinCommands() (8/8 tipos). Limitación: el registro no es enumerable; la lista es manual. Propuesta al arquitecto: `CommandRegistry::typeIds()` para que el test falle si se añade un tipo sin cubrir.
- ✓ Matriz 5x3, 0 violaciones (headless, dev y ASan; en tsan se omite).
- ◐ CI: editado `.github/workflows/ci.yml`: añadidos `--smoke-test` tras dev y tras asan en macOS (timeout 2 min). Linux headless/ASan/TSan y Windows dev ya existían. "Verde en el PR" NO verificable aquí (macOS/Windows/CI remoto).
- ◐ Lista T1–T7: abajo. Prueba manual en macOS: pendiente (no hay macOS).

## Verificación T1–T7 (evidencia: ejecución local + lectura de informes)
- T1 ✓ 9 casos de comandos/historial en verde en dev/headless/ASan; la salvedad del informe T1 (-Werror en ProjectSerializer) ya no ocurre: headless compila.
- T2 ✓ ProjectFormatTests verdes; guardado atómico sin .tmp (comprobado en la integración); migración/backup cubiertos por sus tests. SKIP root esperado.
- T3 ✓ tests [rt] verdes con detector ON (headless, dev, ASan). tsan: ver deuda.
- T4 ✓ RenderCoreTests verdes; matriz nueva confirma 0 violaciones, sin NaN, nivel acotado.
- T5 ✓ AudioEngineTests verdes en dev (ahora compila: libasound2-dev instalado). ✗/no verificado: callback con dispositivo real (sin hardware, ni /dev/snd/seq).
- T6 ✓ app compila; smoke-test rc=0 bajo xvfb. No verificado: macOS, asan app, .app.
- T7 ✓ gp_tests_gui verdes (8). Captura: docs/capturas/F0-ventana.png (xvfb 1280x800, `import -window root`): menús Archivo/Edición, banner sin dispositivo, paneles Tiempo/Tono/Pistas, barra de estado "Tiempo real: sin incidencias". Sin barra de título porque no hay gestor de ventanas (no se ve `window.untitled`).

## CI: dependencias Linux
El job `linux-headless` solo instala ninja-build y NO necesita libxi-dev ni el resto: el preset headless no compila gp_ui/gp_engine_device (confirmado: headless construye 46 tests sin GUI). Hoy ningún job Linux compila la GUI. Propuesta (no aplicada: fuera de la tarjeta): job `linux-gui` (ubuntu-24.04, preset dev, ctest + `xvfb-run -a ... --smoke-test`) con `apt-get install -y ninja-build xvfb libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libxi-dev libfreetype-dev libfontconfig1-dev libcurl4-openssl-dev libgl1-mesa-dev`. Ganaría cobertura de dev+GUI en Linux además de macOS/Windows.

## Deuda T3: tsan y detector
tsan usa GP_RT_CHECKS=OFF (el hook global de new/delete interfiere con TSan), así que 9 tests se omiten allí. Valoración: NO hace falta cubrirlo en tsan; el detector se ejecuta en headless, headless+ASan y dev/asan (macOS). TSan sí ejecuta SpscQueue/SnapshotExchange concurrentes (su objetivo). Riesgo residual: nada ejecuta el detector junto con TSan, y los tests de concurrencia con detector ON corren sin TSan. Aceptable; si se quiere, el arquitecto podría añadir un caso en ctest con la cuenta de SKIP esperada para detectar que no se omiten más de 9 tests por accidente.

## Observaciones menores (sin tarjeta de bug; ninguna rompe criterios)
- Sin carga DSP medida (F0 no tiene benchmark; el estrés 64 pistas/20 dispositivos no aplica aún: no hay pistas con audio).
- Sin golden de render con hash: la matriz usa umbrales de nivel; T4 ya compara bloques bit a bit. Golden con referencia binaria queda para F1.
- Bugs F0-BUG{k}: ninguno.
