# Phase 56: Parameter Settings Dialogs Restoration - Pattern Map

**Mapped:** 2026-07-02
**Files analyzed:** 12 new/modified files
**Analogs found:** 11 / 12

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/qml_gui/dialogs/SettingsDialog.qml` | dialog (component) | request-response | `src/qml_gui/dialogs/SavePresetDialog.qml` + `UnsavedChangesDialog.qml` + `CxDialog.qml` | exact |
| `src/qml_gui/components/OptionRow.qml` | component | transform | `src/qml_gui/components/ParamsPage.qml` lines 206-432 | exact |
| `src/qml_gui/components/GroupNavSidebar.qml` | component | transform | `src/qml_gui/components/ParamsPage.qml` lines 39-111 | exact |
| `src/qml_gui/Models/ConfigOptionModel.h` | model | CRUD (extend) | `src/qml_gui/Models/ConfigOptionModel.h` (self) | role-match |
| `src/qml_gui/Models/ConfigOptionModel.cpp` | model | CRUD (extend) | `src/qml_gui/Models/ConfigOptionModel.cpp` lines 588-612, 961-1034 | role-match |
| `src/core/viewmodels/ConfigViewModel.h` | viewmodel | request-response (extend) | `src/core/viewmodels/ConfigViewModel.h` (self) | role-match |
| `src/core/viewmodels/ConfigViewModel.cpp` | viewmodel | request-response (extend) | `src/core/viewmodels/ConfigViewModel.cpp` | role-match |
| `src/qml_gui/controls/CxSpinBox.qml` | control (extend) | transform | `src/qml_gui/controls/CxSpinBox.qml` (self) | role-match |
| `src/qml_gui/BackendContext.h` + `.cpp` | composition-root | event-driven (extend) | `src/qml_gui/BackendContext.cpp` lines 569-578, 88-109 | role-match |
| `src/qml_gui/qml.qrc` | config | batch | `src/qml_gui/qml.qrc` lines 21-43 (dialog entries) | exact |
| `tests/ViewModelSmokeTests.cpp` | test | request-response (extend) | `tests/ViewModelSmokeTests.cpp` lines 340-428 | exact |
| `tests/E2EWorkflowTests.cpp` | test | request-response (extend) | `tests/E2EWorkflowTests.cpp` lines 154-174 | exact |

## Pattern Assignments

### `src/qml_gui/dialogs/SettingsDialog.qml` (new dialog, request-response)

**Analog:** `src/qml_gui/dialogs/SavePresetDialog.qml` (dialog shell) + `src/qml_gui/dialogs/UnsavedChangesDialog.qml` (dirty guard pattern) + `src/qml_gui/controls/CxDialog.qml` (header pattern)

**IMPORTANT: This is an `ApplicationWindow`, NOT a `CxDialog`.** CxDialog is modal-only. The UI-SPEC explicitly requires `ApplicationWindow` with `Qt.NonModal`. However, the title-bar header pattern from CxDialog and the dirty-guard pattern from UnsavedChangesDialog are the analogs.

**Dialog shell pattern -- CxDialog header** (`src/qml_gui/controls/CxDialog.qml` lines 26-76):
```qml
header: Rectangle {
    height: 44
    color: Theme.bgSurface
    radius: Theme.radiusLG

    Text {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: Theme.spacingXL
        text: (root.titleIcon ? root.titleIcon + " " : "") + root.dialogTitle
        color: Theme.textPrimary
        font.pixelSize: Theme.fontSizeLG
        font.bold: true
    }

    // Close button
    Rectangle {
        visible: root.showCloseButton
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingMD
        width: 28; height: 28; radius: Theme.radiusSM
        color: closeMouse.containsMouse ? Theme.chromeDangerHover : "transparent"
        Text { anchors.centerIn: parent; text: "✕"; color: Theme.textMuted; font.pixelSize: Theme.fontSizeMD }
        MouseArea { id: closeMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.reject() }
    }
}
```

**SavePresetDialog -- injection pattern** (`src/qml_gui/dialogs/SavePresetDialog.qml` lines 23-39):
```qml
CxDialog {
    id: root
    modal: true
    title: qsTr("Save Preset")
    required property var configVm
    required property string presetTier
    function tierToCategory(tier) {
        if (tier === "print") return 0
        if (tier === "filament") return 1
        if (tier === "printer") return 2
        return 0
    }
```

**UnsavedChangesDialog -- dirty guard pattern** (`src/qml_gui/dialogs/UnsavedChangesDialog.qml` lines 32-38):
```qml
required property var configVm
property string presetTier: ""
property string action: "cancel"  // "save" / "discard" / "cancel"
function openDialog() {
    root.action = "cancel"
    root.open()
}
```

**Adaptation for SettingsDialog:** The new dialog should be `ApplicationWindow` with a custom title bar (copying CxDialog's header pattern), injected with `required property var configVm` and `required property string presetTier`, and containing the full layout per UI-SPEC. The close handler should check `configVm.isPresetDirty` and show UnsavedChangesDialog.

---

### `src/qml_gui/components/OptionRow.qml` (new component, transform)

**Analog:** `src/qml_gui/components/ParamsPage.qml` lines 206-432 (the delegate item)

This is the most critical analog. The entire option-row rendering pattern -- including typed control dispatch, dirty dot, read-only tag, tooltip, group header, and numeric TextInput+DoubleValidator -- already exists in ParamsPage.qml.

**Typed control dispatch pattern** (ParamsPage.qml lines 311-419):
```qml
// Slider (double/int)
RowLayout {
    visible: paramRow.oType === "double" || paramRow.oType === "int"
    spacing: Theme.spacingSM
    CxSlider {
        Layout.fillWidth: true
        from: paramRow.oMin; to: paramRow.oMax; stepSize: paramRow.oStep
        value: typeof paramRow.oVal === "number" ? paramRow.oVal : paramRow.oMin
        enabled: !paramRow.oRO
        onMoved: root.optionModel.setValue(paramRow.optIdx, value)
    }
    Rectangle {
        Layout.preferredWidth: 64; height: 24; radius: 4
        color: Theme.bgInset
        border.width: 1
        border.color: paramEdit.activeFocus ? Theme.accent : (paramRow.oRO ? Theme.borderSubtle : Theme.borderInput)
        opacity: paramRow.oRO ? 0.6 : 1.0
        TextInput {
            id: paramEdit
            anchors.fill: parent
            anchors.leftMargin: 6; anchors.rightMargin: 6
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
            color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.bold: true
            selectByMouse: true; readOnly: paramRow.oRO
            validator: DoubleValidator { decimals: paramRow.oType === "double" ? 3 : 0; notation: DoubleValidator.StandardNotation }
            text: typeof paramRow.oVal === "number" ? Number(paramRow.oVal).toFixed(paramRow.oType === "double" ? 2 : 0) : ""
            onEditingFinished: {
                if (!root.optionModel || paramRow.oRO) return
                var v = parseFloat(text)
                if (isNaN(v)) return
                if (v < paramRow.oMin) v = paramRow.oMin
                if (v > paramRow.oMax) v = paramRow.oMax
                root.optionModel.setValue(paramRow.optIdx, v)
            }
        }
    }
}

// Switch (bool)
CxSwitch {
    visible: paramRow.oType === "bool"
    checked: paramRow.oVal === true || paramRow.oVal === "true"
    enabled: !paramRow.oRO
    onToggled: root.optionModel.setValue(paramRow.optIdx, checked)
}

// ComboBox (enum)
CxComboBox {
    visible: paramRow.oType === "enum"
    width: 160; enabled: !paramRow.oRO
    currentIndex: typeof paramRow.oVal === "number" ? paramRow.oVal : 0
    model: { /* ... optEnumCount loop ... */ }
    onActivated: (i) => root.optionModel.setValue(paramRow.optIdx, i)
}

// TextArea (string)
ScrollView {
    visible: paramRow.oType === "string"
    height: 60
    TextArea {
        text: typeof paramRow.oVal === "string" ? paramRow.oVal : ""
        font.pixelSize: Theme.fontSizeSM; font.family: "Consolas"
        color: Theme.textSecondary; readOnly: paramRow.oRO; wrapMode: TextArea.Wrap
        onTextChanged: { if (root.optionModel && activeFocus) root.optionModel.setValue(paramRow.optIdx, text) }
    }
}
```

**Dirty dot + read-only pattern** (ParamsPage.qml lines 278-303):
```qml
ColumnLayout {
    Layout.preferredWidth: 180; spacing: 2
    RowLayout { spacing: Theme.spacingXS
        Rectangle {
            visible: paramRow.oDirty
            width: 6; height: 6; radius: 3; color: Theme.statusWarning
        }
        Text {
            text: paramRow.oLabel
            color: paramRow.oRO ? Theme.textDisabled : (paramRow.oDirty ? Theme.statusWarning : Theme.textSecondary)
            font.pixelSize: Theme.fontSizeMD; elide: Text.ElideRight
            font.bold: paramRow.oDirty || root.searchText !== ""
        }
    }
    Text { visible: paramRow.oRO; text: qsTr("只读"); color: "#3a4555"; font.pixelSize: 9 }
}
```

**Group header pattern** (ParamsPage.qml lines 224-247):
```qml
Rectangle {
    visible: paramDelegate.showGroupHeader
    width: parent.width; height: 28; color: Theme.bgSurface
    Text {
        anchors.left: parent.left; anchors.leftMargin: 20; anchors.verticalCenter: parent.verticalCenter
        text: paramDelegate.oGroup; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM; font.bold: true
    }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.borderSubtle }
}
```

**Tooltip pattern** (ParamsPage.qml lines 267-270 + 424-431):
```qml
ToolTip.visible: oTip !== "" && tipMA.containsMouse
ToolTip.text: oTip; ToolTip.delay: 500
// At bottom of delegate:
MouseArea { id: tipMA; anchors.fill: paramRow; hoverEnabled: true; acceptedButtons: Qt.NoButton; z: -1 }
```

**Row zebra striping** (ParamsPage.qml line 255):
```qml
color: (paramDelegate.index + (paramDelegate.showGroupHeader ? 1 : 0)) % 2 === 0 ? "transparent" : "#080b10"
```

**Content height dispatch** (ParamsPage.qml lines 217-218):
```qml
readonly property int contentHeight: paramRow.oType === "double" || paramRow.oType === "int" ? 56
                                    : paramRow.oType === "string" ? 80 : 44
```

**Adaptation for OptionRow.qml:** Extract this delegate into a standalone `OptionRow.qml` component. Add: (a) percent type (same as double, unit always "%"), (b) nullable type (same as underlying + value-source indicator below label), (c) unit suffix Text to the right of the numeric input (read from `optUnit(i)`), (d) multi-value extruder selector above the row.

---

### `src/qml_gui/components/GroupNavSidebar.qml` (new component, transform)

**Analog:** `src/qml_gui/components/ParamsPage.qml` lines 39-111 (category sidebar)

**Group nav sidebar pattern** (ParamsPage.qml lines 39-111):
```qml
Rectangle {
    Layout.preferredWidth: 200; Layout.fillHeight: true; color: Theme.bgPanel
    ColumnLayout {
        anchors.fill: parent; anchors.topMargin: Theme.spacingLG; spacing: 0
        Text {
            Layout.leftMargin: 16
            text: qsTr("参数分类")
            color: Theme.accent; font.pixelSize: Theme.fontSizeSM; font.bold: true
        }
        Item { Layout.preferredHeight: Theme.spacingLG }
        Repeater {
            model: [qsTr("全部")].concat(root.categories)
            delegate: Rectangle {
                required property string modelData; required property int index
                Layout.fillWidth: true; Layout.leftMargin: Theme.spacingSM; Layout.rightMargin: Theme.spacingSM
                height: 34; radius: 4
                color: root.selectedCategory === modelData ? "#1c2a3e" : catHov.containsMouse ? "#161d28" : "transparent"
                border.color: root.selectedCategory === modelData ? Theme.accent : "transparent"; border.width: 1
                RowLayout { anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: Theme.spacingSM; spacing: 0
                    Text { text: modelData; color: root.selectedCategory === modelData ? Theme.accent : Theme.textSecondary; font.pixelSize: Theme.fontSizeMD }
                    Item { Layout.fillWidth: true }
                    Rectangle { // count badge
                        readonly property int cnt: root.optionModel ? root.optionModel.countForCategory(modelData) : 0
                        visible: cnt > 0; width: cnt > 9 ? 26 : 20; height: 16; radius: 3
                        color: root.selectedCategory === modelData ? "#1e3828" : "#1e2535"
                        Text { anchors.centerIn: parent; text: parent.cnt; color: root.selectedCategory === modelData ? Theme.accent : Theme.textDisabled; font.pixelSize: Theme.fontSizeXS }
                    }
                }
                HoverHandler { id: catHov }
                TapHandler { onTapped: root.selectedCategory = modelData }
            }
        }
        Item { Layout.fillHeight: true }
    }
}
```

**Adaptation for GroupNavSidebar.qml:** Extract into standalone component. Replace `categories` model with tab-driven groups from `configVm.filterOptionIndices(tier, ...)` + `optionModel.pageNames()`. Add `CxScrollView` wrapper per UI-SPEC. Add per-group dirty count badge.

---

### `src/qml_gui/Models/ConfigOptionModel.{h,cpp}` (extend model, CRUD)

**Analog:** Self-extend. Current struct + loader already present.

**Current ConfigOption struct** (`ConfigOptionModel.h` lines 12-51):
```cpp
struct ConfigOption
{
  QString key;
  QString label;
  QString type; // "double" | "int" | "bool" | "enum"
  QVariant value;
  double min = 0; double max = 0; double step = 1;
  QStringList enumLabels{};
  QString category{}; // 对齐上游 ConfigOptionsGroup
  QString group{};   // 对齐上游 ConfigOptionsGroup — 子分组标题
  QString page{};     // 对齐上游 Tab Page
  bool readonly = false;
  QString tooltip{};
  QString sidetext{}; // 输入框右侧单位文本
  int mode = 2;       // 0=comSimple, 1=comAdvanced, 2=comDevelop
  // ... constructors ...
  ConfigOption() = default;
};
```

**NEEDED ADDITIONS to ConfigOption:**
```cpp
bool nullable = false;   // accepts nil (inherit from parent)
bool isVector = false;   // multi-value per-extruder (coFloats, coInts, etc.)
```

**Current mapType** (`ConfigOptionModel.cpp` lines 588-612):
```cpp
QString mapType(Slic3r::ConfigOptionType t)
{
  switch (t) {
  case Slic3r::coFloat: case Slic3r::coFloatOrPercent:
  case Slic3r::coFloats: case Slic3r::coPercents:
    return QStringLiteral("double");  // MISSING: coPercent and coFloatOrPercent mapped to same "double" — need "percent" type
  case Slic3r::coInt: case Slic3r::coInts:
    return QStringLiteral("int");
  case Slic3r::coBool: case Slic3r::coBools:
    return QStringLiteral("bool");
  case Slic3r::coEnum: case Slic3r::coEnums:
    return QStringLiteral("enum");
  case Slic3r::coString: case Slic3r::coStrings:
    return QStringLiteral("string");
  default: return QStringLiteral("double");
  }
}
```

**NEEDED CHANGES to mapType:** Add `coPercent`/`coPercents` -> `"percent"`. Add `nullable` and `isVector` field population in `loadSchemaFromKeys`.

**Current loadSchemaFromKeys** (`ConfigOptionModel.cpp` lines 961-1019):
```cpp
void ConfigOptionModel::loadSchemaFromKeys(const char *const keys[])
{
  beginResetModel();
  m_options.clear(); m_baseReadonlyKeys.clear(); m_defaultValues.clear();
  m_referenceValues.clear(); m_dirtyKeys.clear();
  const auto &def = Slic3r::print_config_def;
  for (int ki = 0; keys[ki] != nullptr; ++ki) {
    const auto *opt = def.get(keys[ki]);
    if (!opt) continue;
    ConfigOption entry;
    entry.key = QString::fromUtf8(opt->opt_key.c_str());
    entry.label = QString::fromUtf8(opt->label.c_str());
    entry.type = mapType(opt->type);
    entry.category = mapCategory(opt->category);
    entry.readonly = opt->readonly;
    entry.value = extractDefault(opt);
    entry.min = static_cast<double>(opt->min);
    entry.max = static_cast<double>(opt->max);
    if (opt->type == Slic3r::coFloat || opt->type == Slic3r::coFloatOrPercent)
      entry.max = opt->max_literal;
    entry.step = (opt->type == Slic3r::coFloat || opt->type == Slic3r::coFloatOrPercent) ? 0.01 : 1.0;
    // ... enum labels, tooltip, sidetext, mode ...
    m_options.append(entry);
    m_defaultValues.insert(entry.key, entry.value);
    if (entry.readonly) m_baseReadonlyKeys.insert(entry.key);
  }
  endResetModel(); emit countChanged();
}
```

**NEEDED ADDITIONS to loadSchemaFromKeys:**
```cpp
entry.nullable = opt->nullable;
entry.isVector = (opt->type & Slic3r::coVectorType) != 0;
```

**Current Roles enum** (`ConfigOptionModel.h` lines 60-77):
```cpp
enum Roles {
  KeyRole = Qt::UserRole + 1, LabelRole, TypeRole, ValueRole,
  MinRole, MaxRole, StepRole, EnumLabelsRole, CategoryRole,
  GroupRole, PageRole, ReadonlyRole, DirtyRole, TooltipRole, ModeRole
};
```

**NEEDED ADDITIONS:** `NullableRole`, `IsVectorRole`, `SidetextRole` (if not already exposed via optUnit).

**Current setValue / resetOption** (`ConfigOptionModel.cpp` lines 337-390):
```cpp
void ConfigOptionModel::resetOption(int row) {
  if (row < 0 || row >= m_options.size()) return;
  const auto &key = m_options[row].key;
  const QVariant targetValue = m_referenceValues.contains(key)
      ? m_referenceValues.value(key) : m_defaultValues.value(key);
  if (!targetValue.isValid()) return;
  m_options[row].value = targetValue;
  m_dirtyKeys.remove(key);
  emit dataChanged(idx, idx, {ValueRole, DirtyRole});
  emit optionValueChanged(key, targetValue);
}

void ConfigOptionModel::setValue(int row, const QVariant &value) {
  // ... dirty tracking against reference values ...
  emit optionValueChanged(key, value);
}
```

**NEEDED ADDITIONS to ConfigOptionModel methods:**
- `Q_INVOKABLE bool optNullable(int i) const;`
- `Q_INVOKABLE bool optIsVector(int i) const;`
- `Q_INVOKABLE QStringList groupNames() const;` -- unique group names for the current model
- `Q_INVOKABLE int dirtyCountForGroup(const QString &group) const;` -- per-group dirty count

---

### `src/core/viewmodels/ConfigViewModel.{h,cpp}` (extend viewmodel, request-response)

**Analog:** Self-extend. Current API surface well-documented.

**Current Q_INVOKABLE methods relevant to Phase 56** (`ConfigViewModel.h` lines 98-177):
```cpp
Q_INVOKABLE bool requestSavePendingChanges();
Q_INVOKABLE bool requestDiscardPendingChanges();
Q_INVOKABLE bool requestCancelPendingChanges();
Q_INVOKABLE QList<int> filterOptionIndices(const QString &category, const QString &searchText, bool advancedMode = false) const;
Q_INVOKABLE QString valueSourceForKey(const QString &key) const;
Q_INVOKABLE bool resetOptionToLevel(const QString &key, int level);
Q_INVOKABLE bool resetGlobalOption(const QString &key);
Q_INVOKABLE void resetAllGlobalOptions();
Q_INVOKABLE QHash<QString, QVariant> mergedConfigValues() const;
Q_INVOKABLE void applyProjectConfig(const QHash<QString, QVariant> &config);
```

**NEEDED ADDITIONS:**
```cpp
// Per-group reset (SETTINGS-05)
Q_INVOKABLE void resetGroup(const QString &tier, const QString &groupName);

// Per-option nullable flag proxy (delegates to optionModelForTier)
Q_INVOKABLE bool optNullable(const QString &tier, int index) const;

// Per-option isVector flag proxy
Q_INVOKABLE bool optIsVector(const QString &tier, int index) const;

// Group names for a given tier (delegates to optionModel->groupNames())
Q_INVOKABLE QStringList groupNames(const QString &tier) const;

// Per-group dirty count
Q_INVOKABLE int dirtyCountForGroup(const QString &tier, const QString &groupName) const;
```

**Current filterOptionIndices only operates on printOptions_** -- needs extension to accept tier parameter and dispatch to `optionModelForTier(tier)`. The private helper `optionModelForTier()` already exists at line 204:
```cpp
ConfigOptionModel *optionModelForTier(const QString &tier) const;
```

**Current preset-tier model accessors** (`ConfigViewModel.h` lines 79-82):
```cpp
QObject *printOptions() const;
QObject *machineOptions() const;
QObject *filamentOptions() const;
```

---

### `src/qml_gui/controls/CxSpinBox.qml` (extend control, transform)

**Analog:** Self-extend.

**Current CxSpinBox** (`src/qml_gui/controls/CxSpinBox.qml` lines 1-49):
```qml
SpinBox {
    id: root
    implicitHeight: Theme.controlHeightSM
    implicitWidth: 90
    font.pixelSize: Theme.fontSizeMD

    background: Rectangle {
        radius: Theme.radiusSM
        color: Theme.bgElevated
        border.color: root.activeFocus ? Theme.borderFocus : Theme.borderDefault
        border.width: 1
        opacity: root.enabled ? 1.0 : 0.45
    }

    contentItem: TextInput {
        z: 2
        text: root.textFromValue(root.value, root.locale)
        color: Theme.textPrimary
        font: root.font
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !root.editable
        validator: root.validator
        selectionColor: Theme.selectionColor
        selectedTextColor: Theme.selectionText
    }
    // up/down indicators ...
}
```

**NEEDED ADDITION:** Add unit suffix rendering per UI-SPEC + RESEARCH:
```qml
property string suffix: ""

// In contentItem row: append a Text element after TextInput
RowLayout {
    anchors.fill: parent
    anchors.margins: 2
    spacing: 0
    TextInput { /* existing contentItem */ Layout.fillWidth: true }
    Text {
        visible: root.suffix !== ""
        text: root.suffix
        color: Theme.textTertiary
        font.pixelSize: Theme.fontSizeXS
        Layout.rightMargin: root.up.indicator.width + 4
    }
}
```

Or simpler: use an `Item` wrapper with the TextInput and a right-anchored Text.

**IMPORTANT:** This suffix is for int options ONLY. Float options use `TextInput`+`DoubleValidator` (see shared patterns below), not CxSpinBox.

---

### `src/qml_gui/BackendContext.{h,cpp}` (extend composition-root, event-driven)

**Analog:** Self-extend.

**Current no-op forward** (`BackendContext.cpp` lines 569-578):
```cpp
void BackendContext::forwardSettingsRequest(const QString &category)
{
  // Phase 52 PREPSB-02: forward the sidebar "Setting" entry point.
  qInfo("[Backend] settingsRequested(%s) -- settings dialog pending Phase 56",
        qPrintable(category));
  emit settingsRequested(category);
}
```

**Current signal declaration** (`BackendContext.h` lines 417-419):
```cpp
/// Phase 52 PREPSB-02: request to open an independent settings dialog for a
/// category ("printer" / "filament" / "process").
void settingsRequested(const QString &category);
```

**Phase-52 connect pattern for cross-VM wiring** (`BackendContext.cpp` lines 95-109):
```cpp
// Phase 52 PREPSB-05 (CRITICAL): config change invalidates slice results
connect(configViewModel_, &ConfigViewModel::stateChanged, editorViewModel_,
        [this]() {
          editorViewModel_->invalidateAllSliceResults();
          emit editorViewModel_->stateChanged();
        });
// Wire ConfigViewModel into EditorViewModel for preset injection at slice time
editorViewModel_->setConfigViewModel(configViewModel_);
```

**REPLACEMENT for forwardSettingsRequest:**
```cpp
void BackendContext::forwardSettingsRequest(const QString &category)
{
  // Phase 56: show/create the correct SettingsDialog for this category.
  // Dialog instances are created once and shown/hidden (not destroyed per open).
  // Ensure the active preset tier is set before showing.
  configViewModel_->setActivePresetTier(category);
  // Emit signal; QML side connects to create/show the dialog
  emit settingsRequested(category);
}
```

**QML-side connection in main.qml or via Loader:**
```qml
// main.qml or a Loader that creates the dialogs
Connections {
    target: backend
    function onSettingsRequested(category) {
        // Show the correct dialog instance
        if (category === "printer") printerSettingsDialog.show()
        else if (category === "filament") filamentSettingsDialog.show()
        else if (category === "process") processSettingsDialog.show()
    }
}
```

---

### `src/qml_gui/qml.qrc` (extend config, batch)

**Analog:** Existing dialog entries in qml.qrc.

**Current dialog entries** (`qml.qrc` lines 21-43):
```xml
<!-- Dialogs -->
<file>dialogs/PrintDialog.qml</file>
<file>dialogs/CalibrationDialog.qml</file>
<file>dialogs/CaliHistoryDialog.qml</file>
<file>dialogs/AboutDialog.qml</file>
<file>dialogs/ConfigWizardDialog.qml</file>
<file>dialogs/BedShapeDialog.qml</file>
<file>dialogs/EditGCodeDialog.qml</file>
<file>dialogs/SavePresetDialog.qml</file>
<file>dialogs/UnsavedChangesDialog.qml</file>
<file>dialogs/ExportPresetBundleDialog.qml</file>
```

**NEEDED ADDITIONS:**
```xml
<file>dialogs/SettingsDialog.qml</file>
```

And in the components section (alongside existing ParamsPage.qml at line 69):
```xml
<file>components/OptionRow.qml</file>
<file>components/GroupNavSidebar.qml</file>
```

---

### `tests/ViewModelSmokeTests.cpp` (extend test, request-response)

**Analog:** Existing test methods.

**Test setup pattern** (`ViewModelSmokeTests.cpp` lines 340-367):
```cpp
void ViewModelSmokeTests::config_default_and_switch_preset()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("ConfigSwitchPreset"));
  ScopedSettingsSnapshot snapshot({ /* settings keys to track */ });
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  QSignalSpy spy(&config, &ConfigViewModel::stateChanged);
  const QString initialPreset = config.currentPreset();
  QVERIFY(!initialPreset.isEmpty());

  // ... mutate and assert ...
  QCOMPARE(config.currentPreset(), QStringLiteral("Unit Test Switch Print Preset"));
}
```

**Machine options loaded pattern** (`ViewModelSmokeTests.cpp` lines 389-410):
```cpp
void ViewModelSmokeTests::testMachineOptionsLoaded()
{
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  auto *machineOpts = qobject_cast<ConfigOptionModel *>(config.machineOptions());
  QVERIFY(machineOpts);
  QVERIFY2(machineOpts->rowCount() > 0, "Machine options model is empty");
  QVERIFY2(machineOpts->indexOfKey(QStringLiteral("machine_max_speed_x")) >= 0,
           "machine_max_speed_x missing from machine options");
}
```

**Dirty tracking pattern** (`ViewModelSmokeTests.cpp` lines 612-648):
```cpp
void ViewModelSmokeTests::configOptionModelDirtyUsesPresetReferenceValues()
{
  // ... setup ConfigViewModel, set reference values, modify option ...
  QVERIFY(optModel->optIsDirty(idx));
  QCOMPARE(optModel->dirtyCount(), 1);
  optModel->resetOption(idx);
  QVERIFY(!optModel->optIsDirty(idx));
}
```

**NEEDED NEW TEST METHODS:**
- `testSettingsDialogOpen()` -- verify BackendContext::forwardSettingsRequest emits signal
- `testPerOptionDirtyByTier()` -- verify dirty tracking for printer/filament/process tiers
- `testGroupReset()` -- verify resetGroup resets all options in a named group
- `testNullableOption()` -- verify nullable options inherit from reference when null
- `testSearchFilterByTier()` -- verify filterOptionIndices works for all 3 tiers
- `testPercentTypeDetected()` -- verify coPercent maps to "percent" type

---

### `tests/E2EWorkflowTests.cpp` (extend test, request-response)

**Analog:** Existing test method.

**Config injection test pattern** (`E2EWorkflowTests.cpp` lines 154-174):
```cpp
void E2EWorkflowTests::test_config_injection_applies_preset_values()
{
  PresetServiceMock presetService;
  ProjectServiceMock projectService;
  SliceService slice(&projectService);
  ConfigViewModel config(&presetService, &projectService);
  config.setCurrentPrintPreset(QStringLiteral("0.16mm Fine"));
  const QHash<QString, QVariant> merged = config.mergedConfigValues();
  QVERIFY2(!merged.isEmpty(), "mergedConfigValues() should return non-empty hash");
  slice.setMergedPresetConfig(merged);
  // ... verify config keys present ...
}
```

**NEEDED NEW TEST:**
- `testSettingsEditInvalidatesSlice()` -- verify that an option edit via ConfigOptionModel::setValue triggers stateChanged, which (via BackendContext connect) calls editorViewModel->invalidateAllSliceResults()

---

## Shared Patterns

### Authentication
Not applicable -- desktop application, no auth guards.

### Error Handling
**Source:** `src/core/viewmodels/ServiceTypes.h` (ServiceError / ServiceResult pattern)
**Apply to:** ConfigViewModel save/reset operations that can fail
```cpp
// ServiceResult<T>::success(value) / ServiceResult<T>::failure(error)
// Q_INVOKABLE bool return pattern used by ConfigViewModel
```
Current ConfigViewModel uses `bool` returns for operations like `requestSavePendingChanges()`, `createCustomPreset()`, `deletePreset()` -- follow this pattern.

### Q_PROPERTY + QSignalSpy Test Pattern
**Source:** `tests/ViewModelSmokeTests.cpp` lines 340-367
**Apply to:** All new ViewModelSmoke test methods
```cpp
QSignalSpy spy(&config, &ConfigViewModel::stateChanged);
// ... mutate ...
QVERIFY(spy.count() >= 1);
QCOMPARE(config.currentPreset(), expectedValue);
```

### QML Boundary Rule
**Source:** `.claude/rules/qml-boundaries.md`
**Apply to:** All QML files (SettingsDialog, OptionRow, GroupNavSidebar)
- QML is presentation + wiring only. No business logic in QML.
- All preset lifecycle logic (save, reset, dirty) stays in ConfigViewModel/PresetServiceMock.

### Cx* Control Gate
**Source:** `MEMORY.md` v14-01 Component System Hardening
**Apply to:** All QML consuming code in new dialogs
- Use only Cx* controls (CxButton, CxComboBox, CxSlider, CxSpinBox, CxTextField, CxCheckBox, CxSwitch, CxDialog, CxScrollView, etc.)
- No raw QtQuick.Controls in consuming code.

### Float Numeric Input Pattern (NOT CxSpinBox)
**Source:** `src/qml_gui/components/ParamsPage.qml` lines 336-363
**Apply to:** All float/double and percent option types in OptionRow.qml
```qml
// TextInput + DoubleValidator for float options (Qt6 SpinBox.stepSize is int-only)
Rectangle {
    Layout.preferredWidth: 64; height: 24; radius: 4; color: Theme.bgInset
    border.width: 1
    border.color: paramEdit.activeFocus ? Theme.accent : Theme.borderInput
    TextInput {
        id: paramEdit
        anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
        color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.bold: true
        validator: DoubleValidator { decimals: 3; notation: DoubleValidator.StandardNotation }
        onEditingFinished: { /* clamp + setValue */ }
    }
}
```

### Slice Invalidation Connect Pattern
**Source:** `src/qml_gui/BackendContext.cpp` lines 95-109
**Apply to:** All option edits in the new settings dialogs
- This connect is ALREADY wired. Any option edit that calls `ConfigOptionModel::setValue()` -> `optionValueChanged` -> `ConfigViewModel::handleOptionValueChanged()` -> `emit stateChanged()` will automatically trigger `editorViewModel_->invalidateAllSliceResults()`.
- Phase 56 does NOT need to add a new connect. It just needs to ensure option edits flow through ConfigViewModel.

### AUTOMOC Caveat
**Source:** `tests/ViewModelSmokeTests.cpp` lines 1-8
**Apply to:** All test file modifications
```
After adding a new private slot, re-run cmake configure or delete
build/ViewModelSmokeTests_autogen/timestamp before rebuilding.
```

### Scoped Test Identity Pattern
**Source:** `tests/ViewModelSmokeTests.cpp` lines 342-348
**Apply to:** All new test methods that touch QSettings/presets
```cpp
ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                      QStringLiteral("UniqueTestName"));
ScopedSettingsSnapshot snapshot({ QStringLiteral("key1"), QStringLiteral("key2") });
snapshot.clear();
```

## No Analog Found

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| (none) | -- | -- | All files have at least a role-match analog |

## Notes

### Float Step Trap
Qt6 `SpinBox.stepSize` only accepts `int`. Per `.claude/rules/debugging.md`:
> Qt6 SpinBox.stepSize 只接受 int, 浮点步进需用整数缩放 + textFromValue/valueFromText

The established pattern in the codebase (ParamsPage.qml) uses `TextInput` + `DoubleValidator` for ALL numeric options (both int and float). CxSpinBox is extended only with a `suffix` property for unit rendering on int options. Float options continue using the TextInput approach.

### Page-to-Group Mapping
No upstream metadata field on `ConfigOptionDef` maps an option to a specific group. The mapping is procedural in `Tab.cpp` (`optgroup->append_single_option_line("key")`). Per RESEARCH finding #3, a static `QHash<QString, QString>` per tier in `ConfigOptionModel.cpp` is the pragmatic approach.

### Non-Modal ApplicationWindow Focus
Multiple `ApplicationWindow` instances coexist in Qt6 QML. The main window must not claim focus back. Use `requestActivate()` when showing. Esc handling must be scoped to the dialog (not the main window). This is standard Qt6 functionality but needs runtime verification on Windows (RESEARCH Assumption A4).

## Metadata

**Analog search scope:** `src/qml_gui/dialogs/`, `src/qml_gui/components/`, `src/qml_gui/controls/`, `src/qml_gui/Models/`, `src/core/viewmodels/`, `src/qml_gui/BackendContext.*`, `tests/`, `src/qml_gui/qml.qrc`
**Files scanned:** 14 source files read, 4 grepped
**Pattern extraction date:** 2026-07-02
