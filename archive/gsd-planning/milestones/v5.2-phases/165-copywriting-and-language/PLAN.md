# Phase 165: Copywriting & Language Sweep

**Status:** Executed (script-based EN→ZH + targeted tooltip rewrite)
**Workstream:** CW
**Requirements:** CW-01, CW-02

## Result

- 42 EN→ZH replacements across the 3 English-source dialogs:
  PresetDiffDialog, CreatePresetsDialog, FilamentGroupPopup (now match
  the project's ZH source-language policy)
- Phase 158 Emboss tooltips no longer leak dev jargon (Emboss.hpp /
  ProjectCurve / 上游 / 持久化) into user-facing copy
- Replaced with user-appropriate copy: "将文字贴附到模型表面（实验性，完整
  效果将在后续版本提供）" / "沿曲面进行投影（实验性，完整效果将在后续版本提供）"

## Verification
- QmlUiAuditTests 123/123 PASS
