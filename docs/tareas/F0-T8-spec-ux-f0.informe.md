# Informe F0-T8
Estado: HECHO (sin compilar: tarea solo de docs/JSON).

- Creados: `docs/specs/ux/F0-ventana-principal.md`, `assets/i18n/es.json` (86 claves).
- Cubre cmd.* (8), error.<código> (8, sin `none`), menu, window, dialog.unsaved, panel.*, status.*, audio.noDevice.*.
- Claves extra (para T6/T7): error.dialog.*, audio.settings.*, window.title*, status.saved/opened, *.tip.
- Spec lista todas las claves en su índice final; sin huérfanas (revisado a mano).
- Decisión: las `cmd.*` van en minúscula para encajar en «Deshacer: {0}».
- Validez JSON y ≤60 caracteres en botones/menús: revisados a mano, no ejecutados.
- Pendiente para el arquitecto: confirmar que `{0}` en `menu.edit.undoWith` recibe `tr(cmd.*)`.
