# Logging And Diagnostics

UniversalOverlay uses `gabime/spdlog` through `UniversalOverlay::Log`.

Logs are written under `%TEMP%\UniversalOverlay\<process-id>\`.

Subsystem loggers:

- core
- renderer
- hooks
- ui
- assets
- fonts
- windows
- config
- marvel-runtime

Each logger uses a rotating file sink capped at 1 MB with 3 retained files.
Normal per-frame drawing must not emit logs.
