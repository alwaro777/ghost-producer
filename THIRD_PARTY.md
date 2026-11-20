# Componentes de terceros

GhostProducer se distribuye bajo **AGPLv3** (`LICENSE.md`). Solo se admiten dependencias con
licencia compatible (ver ADR 0001 § Estrategia de licencias). Añade aquí cada dependencia
ANTES de usarla, con versión fijada y la comprobación de compatibilidad.

## En uso (F0)

| Componente | Versión fijada | Licencia | Compatible AGPLv3 | Uso |
|---|---|---|---|---|
| JUCE | 9.0.6 (`654bdbc7a9db0dae90849b44abe91f339e9f0768`) | AGPLv3 / comercial | Sí (misma) | Audio I/O, MIDI, UI, futuras cargas de plugins |
| VST3 SDK (dentro de JUCE) | 3.8.x | MIT | Sí | Hosting VST3 (F3) |
| nlohmann/json | v3.12.0 (`55f93686c01528224f448c19128836e7df245f72`) | MIT | Sí | Formato de proyecto, Commands |
| Catch2 | v3.16.1 (`08092139210881f01a34e60c5f2fd6afa0ea4024`) | BSL-1.0 | Sí | Tests (no se distribuye) |
| CPM.cmake | 0.43.2 (`cmake/CPM.cmake`) | MIT | Sí | Gestión de dependencias (build) |

Notas:
- `LICENSE.md` es el texto oficial de la AGPLv3 en formato Markdown, tomado del árbol de JUCE.
- Los módulos de JUCE incluyen sus propias dependencias (p. ej. HarfBuzz, FLAC, Ogg/Vorbis); sus
  avisos se recopilarán en un `NOTICES` generado al empaquetar (tarea de release, F12).

## Contribuciones
AGPLv3 + DCO (`Signed-off-by` en cada commit, sin CLA). Ver `CONTRIBUTING.md` y ADR 0001.

## Previstas (se revisan y fijan al integrarlas)

| Componente | Fase | Licencia | Compatible AGPLv3 |
|---|---|---|---|
| Ableton Link (+ asio-standalone) | F1/F2 | GPLv2+ (+ BSL-1.0) | Sí, usado bajo GPLv3 |
| CLAP SDK (headers) | F3 | MIT | Sí |
| Signalsmith Stretch | F6 | MIT | Sí |
| Rubber Band Library (R3) | F6 | GPLv2+ / comercial | Sí, usado bajo GPLv3 |
| ONNX Runtime | F10 | MIT | Sí |
| ASIO SDK (Windows) | pendiente (F0 usa WASAPI) | GPLv3 / propietaria (revisar al integrarlo) | Sí solo bajo GPLv3 |
| tracktion_graph (solo evaluación) | F1/F5 | GPLv3 / comercial | Sí |

## Prohibidas
GPLv2-only, licencias «no comercial» o «solo investigación» (también para pesos de modelos de
IA), SDK de VST2 y cualquier asset, preset o texto de productos comerciales.
