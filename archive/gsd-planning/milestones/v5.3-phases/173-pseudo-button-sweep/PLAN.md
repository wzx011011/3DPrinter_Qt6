# Phase 173: Pseudo-Button Sweep

**Status:** Executed (canonical sites + remaining hand-converted sites)
**Workstream:** CL
**Requirement:** CL-03

## Result

Migrated 9 pseudo-button sites from Rectangle+Text+MouseArea to CxButton:
- MultiMachinePage: Cancel + Send buttons (send-to-device dialog)
- PreparePage: Emboss sync execute + async execute buttons
- PreparePage: drill 执行钻孔 button
- PreparePage: cut 执行切割 button
- PreparePage: meshBoolean 执行运算 button
- PreparePage: faceDetect 执行检测 button
- PreparePage: addText 添加文字 button
- PreparePage: importSVG 导入 SVG button

All now gain press-scale, focus border, ToolTip support via the Cx* control
contract. Combined with Phase 168's MonitorPage refresh/add conversions, the
canonical pseudo-button sites across the app are now CxButton/CxIconButton.

## Verification
- QmlUiAuditTests 131/131 PASS
- OWzxSlicer link OK
