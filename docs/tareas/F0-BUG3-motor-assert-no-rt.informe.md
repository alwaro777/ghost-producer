# F0-BUG3 — Informe
Estado: HECHO
- GP_RT_ASSERT_NOT_REALTIME() al inicio de AudioEngine::setProject, timerCallback, audioDeviceAboutToStart y audioDeviceStopped (todas las secciones con writerLock_), y además en RenderCore::publish y collectGarbage (liberan memoria).
- Tests: núcleo (publish/collectGarbage en ScopedRealtimeSection → violación) y dispositivo (setProject → violación).
- headless (build/bug3-h): 47/47 verdes. Test de dispositivo verificado en el proyecto aparte del scratchpad (no compilado con el preset dev).
