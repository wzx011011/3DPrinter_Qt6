# Phase 88 UI Spec

## Contract

The restored settings work must end with a stable, inspectable UI:

1. Printer, material, and process settings open as independent settings dialogs.
2. The restored shell, tabs, option rows, typed controls, and dirty guard remain intact.
3. Deprecated replacement-era settings routes and files stay absent.
4. Visual evidence is captured from the running app for final review.

## Evidence

Store screenshots under:

` .planning/phases/88-settings-verification-and-cleanup/visual-evidence/`

Expected evidence:

- Running app screenshot.
- Settings dialog screenshot, preferably for printer/material/process if accessible in the current session.

## Constraints

- Do not add LAN/device/cloud/network scope.
- Do not reintroduce old settings pages or page routes.
- Keep QML as presentation only; business logic stays in C++ viewmodels/services.
