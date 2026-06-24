# QML Boundary Rules

QML is the presentation layer for the OWzx Slicer Qt6 migration. Durable product behavior belongs in C++.

## QML May Do

- Layout and visual composition.
- Bind to Q_PROPERTY values exposed by viewmodels.
- Forward user actions to Q_INVOKABLE methods or signals.
- Local presentation-only state such as expanded/collapsed UI sections.
- Simple formatting needed only for display.

## QML Must Not Do

- Implement source-truth business rules.
- Own validation, persistence, slicing, preset inheritance, device protocol, calibration, or project model semantics.
- Duplicate upstream behavior mapping that belongs in services/viewmodels.
- Hide missing backend behavior behind visual-only placeholders.

## C++ Ownership

Use C++ services and viewmodels for:

- Model/project state and persistence.
- Slicing and calibration orchestration.
- Preset/config inheritance and dirty-state behavior.
- Device, MQTT, FTP, SSDP, camera, cloud, and network protocols.
- Undo/redo and object/plate mutation semantics.
- Any workflow that must be tested without visual inspection.

## Completion Rule

A QML button, menu item, dialog, page, or enum is not complete until its user-visible workflow reaches the appropriate viewmodel/service path or is explicitly classified as Placeholder, Hybrid, or Blocked in planning.
