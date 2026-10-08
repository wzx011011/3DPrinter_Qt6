#pragma once

#include <QObject>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QList>
#include <QVariant>

class PresetServiceMock;
class ProjectServiceMock;
class ConfigOptionModel;
class PresetListModel;

class ConfigViewModel final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY stateChanged)
  Q_PROPERTY(QString currentPreset READ currentPreset NOTIFY stateChanged)
  Q_PROPERTY(double layerHeight READ layerHeight NOTIFY stateChanged)
  Q_PROPERTY(int printSpeed READ printSpeed NOTIFY stateChanged)
  Q_PROPERTY(bool supportEnabled READ supportEnabled NOTIFY stateChanged)
  Q_PROPERTY(int infillDensity READ infillDensity NOTIFY stateChanged)
  Q_PROPERTY(int nozzleTemp READ nozzleTemp NOTIFY stateChanged)
  Q_PROPERTY(int bedTemp READ bedTemp NOTIFY stateChanged)
  Q_PROPERTY(int wallCount READ wallCount NOTIFY stateChanged)
  Q_PROPERTY(int topLayers READ topLayers NOTIFY stateChanged)
  Q_PROPERTY(int bottomLayers READ bottomLayers NOTIFY stateChanged)
  Q_PROPERTY(bool enableBrim READ enableBrim NOTIFY stateChanged)
  Q_PROPERTY(QObject *printOptions READ printOptions CONSTANT)
  Q_PROPERTY(QObject *machineOptions READ machineOptions CONSTANT)
  Q_PROPERTY(QObject *filamentOptions READ filamentOptions CONSTANT)
  Q_PROPERTY(QObject *presetList READ presetList CONSTANT)
  Q_PROPERTY(bool isPresetDirty READ isPresetDirty NOTIFY stateChanged)
  Q_PROPERTY(QString settingsScope READ settingsScope NOTIFY stateChanged)
  Q_PROPERTY(QString settingsTargetType READ settingsTargetType NOTIFY stateChanged)
  Q_PROPERTY(QString settingsTargetName READ settingsTargetName NOTIFY stateChanged)
  Q_PROPERTY(int settingsTargetObjectIndex READ settingsTargetObjectIndex NOTIFY stateChanged)
  Q_PROPERTY(int settingsTargetVolumeIndex READ settingsTargetVolumeIndex NOTIFY stateChanged)
  Q_PROPERTY(QString activePresetTier READ activePresetTier NOTIFY stateChanged)
  Q_PROPERTY(QStringList printerPresetNames READ printerPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList filamentPresetNames READ filamentPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList printPresetNames READ printPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList compatibleFilamentPresetNames READ compatibleFilamentPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList compatiblePrintPresetNames READ compatiblePrintPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QString currentPrinterPreset READ currentPrinterPreset NOTIFY stateChanged)
  // v5.15 (BEDTEX): absolute bed texture path for the selected printer
  // preset (empty when none). Feeds the viewport's textured bed quad.
  Q_PROPERTY(QString bedTextureFile READ bedTextureFile NOTIFY stateChanged)
  // v5.16 (BEDMODEL/BEDTYPE-TEX): bed_model STL + the BBL-only bed-type
  // texture system gates (upstream Tab.cpp on_presets_changed).
  Q_PROPERTY(QString bedModelFile READ bedModelFile NOTIFY stateChanged)
  Q_PROPERTY(bool bedTypeTexturesActive READ bedTypeTexturesActive NOTIFY stateChanged)
  Q_PROPERTY(bool bedCaliLinesActive READ bedCaliLinesActive NOTIFY stateChanged)
  Q_PROPERTY(QString bedTypeImagesDir READ bedTypeImagesDir NOTIFY stateChanged)
  Q_PROPERTY(QString currentFilamentPreset READ currentFilamentPreset NOTIFY stateChanged)
  Q_PROPERTY(QString currentPrintPreset READ currentPrintPreset NOTIFY stateChanged)
  Q_PROPERTY(QString lastPresetError READ lastPresetError NOTIFY stateChanged)
  Q_PROPERTY(bool currentPresetCombinationValid READ currentPresetCombinationValid NOTIFY stateChanged)
  Q_PROPERTY(QString currentPresetCompatibilityMessage READ currentPresetCompatibilityMessage NOTIFY stateChanged)
  Q_PROPERTY(QString pendingUnsavedAction READ pendingUnsavedAction NOTIFY stateChanged)
  Q_PROPERTY(QString pendingUnsavedTarget READ pendingUnsavedTarget NOTIFY stateChanged)
  Q_PROPERTY(bool hasPendingUnsavedChanges READ hasPendingUnsavedChanges NOTIFY stateChanged)
  Q_PROPERTY(int globalModifiedCount READ globalModifiedCount NOTIFY stateChanged)
  // v5.16 (PSET2-03): single dirty-guard modal gate. Three SettingsDialog
  // instances share this viewmodel and all connect
  // pendingUnsavedChangesRequested; begin/endUnsavedDialog makes only the
  // first listener open its modal (upstream shows exactly one
  // UnsavedChangesDialog at a time).
  Q_PROPERTY(bool unsavedDialogActive READ unsavedDialogActive NOTIFY stateChanged)
  // v5.16 (PSET2-05): sectioned display lists ("— 用户预设 —" / "— 系统预设 —"
  // separators + " (不兼容)" gray-out suffixes) for the preset combos
  // (upstream PresetComboBoxes.cpp:1281-1317 Project/User/System sections +
  // LABEL_ITEM_DISABLED graying).
  Q_PROPERTY(QStringList decoratedPrinterPresetNames READ decoratedPrinterPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList decoratedFilamentPresetNames READ decoratedFilamentPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList decoratedPrintPresetNames READ decoratedPrintPresetNames NOTIFY stateChanged)
  // Per-extruder filament compatibility bitmap (item i = 1 when
  // isFilamentCompatibleForSlot(i), else 0; length = 1 + slot count). NOTIFY
  // property replacement for the former QML-side compatRefreshTick counter —
  // consumers read list[i] with a binding-tracked dependency instead of
  // re-evaluating Q_INVOKABLEs on every tick.
  Q_PROPERTY(QList<int> filamentSlotCompatibility READ filamentSlotCompatibility NOTIFY filamentSlotCompatibilityChanged)

public:
  explicit ConfigViewModel(PresetServiceMock *presetService, ProjectServiceMock *projectService, QObject *parent = nullptr);

  /// Phase 222 (FIL-COLOUR): expose the PresetServiceMock so EditorViewModel
  /// can sync active filament colours into ProjectServiceMock.
  PresetServiceMock *presetService() const { return presetService_; }

  QStringList presetNames() const;
  QString currentPreset() const;
  double layerHeight() const;

  int printSpeed() const { return printSpeed_; }
  bool supportEnabled() const { return supportEnabled_; }
  int infillDensity() const { return infillDensity_; }
  int nozzleTemp() const { return nozzleTemp_; }
  int bedTemp() const { return bedTemp_; }
  int wallCount() const { return wallCount_; }
  int topLayers() const { return topLayers_; }
  int bottomLayers() const { return bottomLayers_; }
  bool enableBrim() const { return enableBrim_; }
  QString settingsScope() const { return settingsScope_; }
  QString settingsTargetType() const { return settingsTargetType_; }
  QString settingsTargetName() const { return settingsTargetName_; }
  int settingsTargetObjectIndex() const { return settingsTargetObjectIndex_; }
  int settingsTargetVolumeIndex() const { return settingsTargetVolumeIndex_; }
  QString activePresetTier() const { return activePresetTier_; }

  QObject *printOptions() const;
  QObject *machineOptions() const;
  QObject *filamentOptions() const;
  QObject *presetList() const;

  QStringList printerPresetNames() const;
  QStringList filamentPresetNames() const;
  QStringList printPresetNames() const;
  Q_INVOKABLE QStringList userPresetNamesForCategory(int category) const;
  QStringList compatibleFilamentPresetNames() const;
  QStringList compatiblePrintPresetNames() const;
  QString currentPrinterPreset() const { return currentPrinterPreset_; }
  QString bedTextureFile() const;
  QString bedModelFile() const;
  bool bedTypeTexturesActive() const;
  bool bedCaliLinesActive() const;
  QString bedTypeImagesDir() const;
  QString currentFilamentPreset() const { return currentFilamentPreset_; }
  QString currentPrintPreset() const { return currentPrintPreset_; }
  QString lastPresetError() const { return lastPresetError_; }
  bool currentPresetCombinationValid() const;
  QString currentPresetCompatibilityMessage() const;
  QString pendingUnsavedAction() const { return pendingUnsavedAction_; }
  QString pendingUnsavedTarget() const { return pendingUnsavedTarget_; }
  bool hasPendingUnsavedChanges() const { return !pendingUnsavedAction_.isEmpty(); }
  bool unsavedDialogActive() const { return m_unsavedDialogActive; }
  QStringList decoratedPrinterPresetNames() const;
  QStringList decoratedFilamentPresetNames() const;
  QStringList decoratedPrintPresetNames() const;
  /// Per-extruder compatibility bitmap (see the Q_PROPERTY above).
  QList<int> filamentSlotCompatibility() const { return filamentSlotCompatibility_; }

  // ── SavePresetDialog support (upstream SavePresetDialog.cpp) ────────────
  /// upstream Item::m_save_to_project (SavePresetDialog.hpp:75): the
  /// "User Preset" / "Preset Inside Project" radio state. The radio writes it
  /// (SavePresetDialog.cpp:163-165), update() force-sets it when the typed
  /// name hits an existing preset (cpp:240-252), and Tab::save_preset reads
  /// it back via get_save_to_project_selection (cpp:371-376, Tab.cpp:7378).
  Q_PROPERTY(bool saveToProjectSelection READ saveToProjectSelection WRITE setSaveToProjectSelection NOTIFY stateChanged)

  /// Save-name validation severity (upstream Item::ValidationType, hpp:40-45).
  enum SavePresetValidationType
  {
    SavePresetValid = 0,
    SavePresetWarning = 1,
    SavePresetNoValid = 2
  };
  /// Error codes returned by savePresetNameErrorCode, in the upstream
  /// Item::update() check order (SavePresetDialog.cpp:182-237).
  enum SavePresetNameError
  {
    SavePresetNameValid = 0,
    SavePresetNameIllegalChars = 1,     // cpp:182-191
    SavePresetNameIllegalSuffix = 2,    // cpp:193-196, " (modified)"
    SavePresetNameReserved = 3,         // cpp:198-202, three exact names
    SavePresetNameSystemOverwrite = 4,  // cpp:204-208, can_overwrite()
    SavePresetNameEmpty = 5,            // cpp:219-222
    SavePresetNameLeadingSpace = 6,     // cpp:224-227
    SavePresetNameTrailingSpace = 7,    // cpp:229-232
    SavePresetNameAliasConflict = 8     // cpp:234-237
  };
  /// Warning codes returned by savePresetNameWarningCode (cpp:210-217); a
  /// warning keeps the save allowed (upstream Warning type).
  enum SavePresetNameWarning
  {
    SavePresetNameNoWarning = 0,
    SavePresetNameExists = 1,
    SavePresetNameExistsIncompatible = 2
  };
  /// Physical-printer save actions (upstream ActionType, hpp:29-35).
  enum PhysicalPrinterAction
  {
    PhysicalPrinterChangePreset = 0,
    PhysicalPrinterAddPreset = 1,
    PhysicalPrinterSwitch = 2,
    PhysicalPrinterUndefAction = 3
  };

  bool saveToProjectSelection() const { return m_saveToProject; }
  void setSaveToProjectSelection(bool saveToProject);
  /// cpp:167-168: initial radio state = the edited preset's
  /// is_project_embedded.
  Q_INVOKABLE bool editedPresetIsProjectEmbedded(int category) const;
  /// cpp:240-252: an existing preset drives the forced/disabled radio state.
  Q_INVOKABLE bool presetIsProjectEmbedded(const QString &name) const;
  /// A successful "Preset Inside Project" save marks the preset embedded
  /// (upstream save_current_preset carries save_to_project onto the preset).
  Q_INVOKABLE void markPresetProjectEmbedded(const QString &name);
  /// cpp:39-45: upstream suggested-name derivation. is_system ->
  /// "<name> - <suffix>", a trailing ".ini" is stripped; the is_default and
  /// bundle-alias branches have no counterpart in this backend and never
  /// apply (documented in the implementation).
  Q_INVOKABLE QString suggestedSavePresetName(int category, const QString &suffix) const;
  /// cpp:182-237 minus the cpp:210 overwrite notice (see
  /// savePresetNameWarningCode): a SavePresetNameError code for the typed
  /// name. Validates the raw text, so callers must not trim first.
  Q_INVOKABLE int savePresetNameErrorCode(int category, const QString &name) const;
  /// cpp:210-217: the overwrite-notice severity for an existing preset name
  /// (0 = no warning; the save itself stays allowed).
  Q_INVOKABLE int savePresetNameWarningCode(int category, const QString &name) const;
  /// cpp:116: parent preset for the Detach row -- a system preset is its own
  /// parent, a user preset uses its inherits() link (empty = "Unique preset").
  Q_INVOKABLE QString savePresetParentName(int category) const;

  /// cpp:394-424: physical-printer info row + 3-action radio group on the
  /// printer tier. The Qt6 backend has no physical-printer store yet, so the
  /// selection lives on this viewmodel (empty = no selection = the block is
  /// hidden, exactly as upstream with no physical printer selected).
  Q_INVOKABLE bool physicalPrinterHasSelection() const;
  Q_INVOKABLE QString physicalPrinterSelectedName() const;
  Q_INVOKABLE QString physicalPrinterSelectedPresetName() const;
  Q_INVOKABLE void setPhysicalPrinterSelection(const QString &printerName, const QString &presetName);
  /// cpp:472-493 update_physical_printers: apply the chosen action on accept.
  Q_INVOKABLE void applyPhysicalPrinterAction(int action, const QString &presetName);

  Q_INVOKABLE void loadDefault();
  Q_INVOKABLE void setCurrentPreset(const QString &presetName);
  Q_INVOKABLE void setCurrentPrinterPreset(const QString &name);
  Q_INVOKABLE void setCurrentFilamentPreset(const QString &name);
  Q_INVOKABLE void setCurrentPrintPreset(const QString &name);
  Q_INVOKABLE bool requestCurrentPrinterPreset(const QString &name);
  Q_INVOKABLE bool requestCurrentFilamentPreset(const QString &name);
  /// v5.16 (CIRC-04): per-slot filament preset selection (upstream
  /// PresetBundle::filament_presets vector semantics). Slot 0 mirrors the
  /// global current selection; slots 1..N hold their own preset.
  Q_INVOKABLE QString filamentPresetForSlot(int slot) const;
  Q_INVOKABLE bool requestFilamentPresetForSlot(int slot, const QString &name);
  Q_INVOKABLE bool isFilamentCompatibleForSlot(int slot) const;
  Q_INVOKABLE bool requestCurrentPrintPreset(const QString &name);
  Q_INVOKABLE bool saveCurrentPreset();
  /// Phase 147 (PSET-02): request opening the CreatePresetsDialog. Emits
  /// createPresetRequired which SettingsDialog binds to dialog.open().
  Q_INVOKABLE void requestCreatePreset() { emit createPresetRequired(); }
  Q_INVOKABLE bool exportBundle(const QString &filePath) const;
  Q_INVOKABLE bool importBundle(const QString &filePath);
  /// v5.16 (PSET2-04): per-preset upstream-shape JSON bundle export/import
  /// (directory tree + index.json manifest). Proxies PresetServiceMock.
  Q_INVOKABLE int exportBundleIni(const QString &dirPath) const;
  Q_INVOKABLE int exportBundleIni(const QString &dirPath, const QStringList &presetNames) const;
  Q_INVOKABLE int importBundleIni(const QString &dirPath);
  bool isPresetDirty() const;
  int globalModifiedCount() const;

  // ── R-P1.J layering governance: wizard / wipe-tower preset proxies ──
  // ConfigWizardDialog / WipeTowerDialog must not touch the service
  // directly (qml-boundaries rule); these thin wrappers keep the QML on
  // the viewmodel layer. Pure pass-throughs to PresetServiceMock.
  Q_INVOKABLE QStringList wizardVendors() const;
  Q_INVOKABLE QStringList wizardAvailableVendorNames() const;
  Q_INVOKABLE QString wizardSelectedVendor() const;
  Q_INVOKABLE void wizardLoadVendor(const QString &vendor);
  Q_INVOKABLE QStringList wizardPrinterModelsForVendor(const QString &vendor) const;
  Q_INVOKABLE QStringList wizardMaterialsForVendorAndPrinter(const QString &vendor, const QString &printerModel) const;
  Q_INVOKABLE QStringList wizardDefaultBedTypes() const;
  Q_INVOKABLE void wizardSetSelectedVendor(const QString &vendor);
  Q_INVOKABLE void wizardSetSelectedPrinterModel(const QString &model);
  /// U-WIZ (upstream ConfigWizard PrinterPicker grid): flat printer preset
  /// names of `vendor` grouped into PrinterPicker cells. Each entry is a
  /// QVariantMap {model: QString, variants: QStringList} where `model` is the
  /// display family name and `variants` holds the full preset names of its
  /// nozzle variants (upstream ConfigWizard.cpp:198-373 renders one grid cell
  /// per model with a checkbox per variant; the Qt6 preset store is flat, so
  /// the "<model> <v> nozzle" trailing token is the grouping key and
  /// non-suffixed names become single-variant cells labelled from the
  /// preset's nozzle_diameter, falling back to 0.4).
  Q_INVOKABLE QVariantList wizardPrinterPickerGroups(const QString &vendor) const;
  /// U-WIZ (upstream PageFirmware gcode_flavor picker): enum labels of the
  /// upstream print_config_def "gcode_flavor" option (PrintConfig.cpp:4289,
  /// order marlin/klipper/reprapfirmware/repetier/marlin2; default index 0 =
  /// Marlin(legacy), gcfMarlinLegacy at PrintConfig.cpp:4321).
  Q_INVOKABLE QStringList wizardGcodeFlavorLabels() const;
  /// U-WIZ (upstream AppConfig::set_variant): per-variant checkbox
  /// persistence for the PrinterPicker grid, stored AppConfig-lite under
  /// "wizard/variant/<vendor>/<model>/<variant>".
  Q_INVOKABLE bool wizardPrinterVariantEnabled(const QString &vendor, const QString &model, const QString &variant) const;
  Q_INVOKABLE void wizardSetPrinterVariantEnabled(const QString &vendor, const QString &model, const QString &variant, bool enabled);
  /// U-WIZ (upstream PageMaterials Type column): distinct filament type
  /// tokens (PLA/ABS/PETG/...) parsed from the filament preset names of all
  /// loaded vendors. Unknown tokens are skipped.
  Q_INVOKABLE QStringList wizardFilamentTypes() const;
  /// U-WIZ (upstream set_compatible_printers_html_window): printer presets of
  /// `vendor` compatible with the filament preset `filamentName`
  /// (compatible_printers match via isPresetCompatibleWithPrinter).
  Q_INVOKABLE QStringList wizardCompatiblePrinters(const QString &vendor, const QString &filamentName) const;
  /// U-WIZ (upstream apply_custom_config + on_bnt_finish): create a USER
  /// printer preset from the wizard's custom-printer path (gcode_flavor /
  /// printable_area / nozzle_diameter / filament_diameter values) and refresh
  /// the preset lists. Fails when the name is empty or already taken.
  Q_INVOKABLE bool wizardCreateCustomPrinterPreset(const QString &name, const QVariantMap &values);
  Q_INVOKABLE QVariant wizardPresetValue(const QString &presetName, const QString &key) const;
  Q_INVOKABLE QVariantList wizardFlushMatrix() const;
  Q_INVOKABLE bool wizardSaveFlushVolumes(const QVariantList &rows);
  /// U05 (WipeTowerDialog): real filament colours for the flush-matrix
  /// badges (upstream feeds filament_colour into the web table and derives
  /// the matrix from it; WipingDialog.html:532-534 also picks the badge text
  /// colour by luminance). Proxy to PresetServiceMock::activeFilamentColours.
  Q_INVOKABLE QStringList wizardFilamentColours() const;
  /// U05 (WipeTowerDialog): flush_multiplier persistence. Upstream writes it
  /// onto the project config next to flush_volumes_matrix
  /// (WipeTowerDialog.cpp:245-246) with the valid range [0, 3]
  /// (g_min/g_max_flush_multiplier, WipeTowerDialog.cpp:199-200). The OWzx
  /// service sink (PresetServiceMock::saveFlushVolumes) only accommodates the
  /// matrix, so the multiplier persists beside it in QSettings
  /// ("flush/multiplier") and is read back when the dialog opens.
  Q_INVOKABLE double flushMultiplier() const;
  Q_INVOKABLE void setFlushMultiplier(double multiplier);

  Q_INVOKABLE bool createCustomPreset(int category, const QString &name);
  /// v5.16 (PSET2-02): create a preset inheriting from `inherits` — the new
  /// preset's values start from the parent's resolved chain (CreatePresetsDialog
  /// "inherits from" selection, upstream CreatePresetsDialog.cpp).
  Q_INVOKABLE bool createCustomPreset(int category, const QString &name, const QString &inherits);
  /// True when any stored preset already uses this exact name -- the probe
  /// behind the upstream create-flow duplicate warnings (CreatePresetsDialog
  /// is_alias_exist / find_preset, CreatePresetsDialog.cpp:1132/2778).
  Q_INVOKABLE bool presetExists(const QString &name) const;
  /// Create-flow clone primitive for the redesigned CreatePresetsDialog: the
  /// new preset starts from `baseName`'s resolved chain (recorded as the
  /// inherits parent) with `overrides` overlaid -- upstream
  /// clone_presets_for_filament / clone_presets_for_printer
  /// (CreatePresetsDialog.cpp:1144-1199 and the Page2 create flow). With
  /// `overwrite` an existing USER preset of the same name is replaced
  /// (upstream rewritten=true, CreatePresetsDialog.cpp:2778-2788).
  Q_INVOKABLE bool createPresetFromBase(int category, const QString &name,
                                        const QString &baseName,
                                        const QVariantMap &overrides,
                                        bool overwrite);
  /// Preset lists for an arbitrary base printer (printer-wizard template
  /// panel, upstream update_presets_list filters by the selected printer,
  /// not only the current one). Proxies
  /// PresetServiceMock::compatiblePresetNamesForCategory.
  Q_INVOKABLE QStringList compatiblePresetsForPrinter(int category, const QString &printerName) const;
  /// G-01: replace an existing USER preset with the current tier edits
  /// (upstream SavePresetDialog replace path). Keeps the current selection;
  /// fails for builtin/read-only targets.
  Q_INVOKABLE bool overwriteUserPreset(int category, const QString &name);
  /// G-13: rebuild the preset lists from the service (fired after in-project
  /// embedded presets from a loaded 3MF were adopted).
  Q_INVOKABLE void refreshPresetLists();
  /// G-13: Detach — flatten an inherited USER preset (cut the inherits link,
  /// the resolved values become its own) and persist it. Keeps the current
  /// selection; fails for builtin/read-only targets.
  Q_INVOKABLE bool detachPresetFromParent(int category, const QString &name);
  /// G-13: name of the parent preset `name` inherits from (empty = none).
  Q_INVOKABLE QString presetInheritsParent(const QString &name) const;
  Q_INVOKABLE bool deletePreset(int category, const QString &name);
  Q_INVOKABLE bool renamePreset(int category, const QString &oldName, const QString &newName);
  Q_INVOKABLE bool canDeletePreset(const QString &name) const;
  /// v5.16 (PSET2-07): true when the preset is the current selection of any
  /// tier or per-extruder slot (drives the in-use delete warning).
  Q_INVOKABLE bool isPresetInUse(const QString &name) const;
  // v5.16 (PSET2-05): reverse of the decorated display lists — strips the
  // " (不兼容)" suffix so QML activation handlers can pass plain preset names.
  Q_INVOKABLE QString plainPresetName(const QString &displayName) const;
  Q_INVOKABLE QStringList comparePresets(const QString &presetA, const QString &presetB) const;
  /// Phase 154 (CLOS-01): structured diff variant — proxies to
  /// PresetServiceMock::comparePresets(A, B) which returns a QVariantList of
  /// {key, valueA, valueB, status} entries (status ∈ added/removed/changed).
  /// Each row is additionally enriched with the presentation data the
  /// PresetDiffDialog tree groups by (upstream DiffViewCtrl::Append input):
  /// category/group/label searcher-equivalent metadata, 30-char short
  /// valueA/valueB display strings (upstream get_short_string), the
  /// untruncated fullValueA/fullValueB for FullCompareDialog, and isLong.
  /// The legacy QStringList overload above stays for any older callers.
  Q_INVOKABLE QVariantList comparePresetsDetailed(const QString &presetA, const QString &presetB) const;
  /// PresetDiffDialog (upstream DiffPresetDialog) transfer primitive: copies
  /// the selected option values from the left preset onto the right preset
  /// (upstream Tab::transfer_options via UnsavedChangesDialog
  /// Action::Transfer). The source preset is never written; read-only
  /// targets are refused by PresetServiceMock::mergePresetValues. Returns
  /// the number of keys written.
  Q_INVOKABLE int transferPresetValues(const QString &leftPreset, const QString &rightPreset, const QStringList &keys);
  /// True when the preset is read-only (system/builtin). PresetDiffDialog
  /// disables Transfer into read-only targets with this.
  Q_INVOKABLE bool presetIsReadOnly(const QString &presetName) const;
  /// Phase 154 (CLOS-01): request opening the PresetDiffDialog. Emits
  /// comparePresetsRequired which SettingsDialog binds to dialog.open().
  Q_INVOKABLE void requestComparePresets() { emit comparePresetsRequired(); }
  Q_INVOKABLE void autoMatchFilament();
  Q_INVOKABLE bool isCurrentFilamentCompatible() const;
  Q_INVOKABLE bool isFilamentCompatible(const QString &filamentName) const;
  Q_INVOKABLE bool canUseCurrentPresetCombination() const;
  Q_INVOKABLE QString presetActionBlocker(int category, const QString &presetName, const QString &action) const;
  Q_INVOKABLE void setLayerHeight(double v);
  Q_INVOKABLE void setPrintSpeed(int v);
  Q_INVOKABLE void setSupportEnabled(bool v);
  Q_INVOKABLE void setInfillDensity(int v);
  Q_INVOKABLE void setNozzleTemp(int v);
  Q_INVOKABLE void setBedTemp(int v);
  Q_INVOKABLE void setWallCount(int v);
  Q_INVOKABLE void setEnableBrim(bool v);
  Q_INVOKABLE void activateGlobalScope();
  Q_INVOKABLE void activateObjectScope(const QString &targetType, const QString &targetName, int objectIndex = -1, int volumeIndex = -1);
  Q_INVOKABLE void activatePlateScope(int plateIndex);
  Q_INVOKABLE bool requestGlobalScope();
  Q_INVOKABLE bool requestObjectScope(const QString &targetType, const QString &targetName, int objectIndex = -1, int volumeIndex = -1);
  Q_INVOKABLE bool requestPlateScope(int plateIndex);
  Q_INVOKABLE void setActivePresetTier(const QString &tier);

  // Per-group reset (SETTINGS-05: per upstream Tab.cpp reset_group)
  Q_INVOKABLE void resetGroup(const QString &tier, const QString &groupName);
  // Per-option nullable flag proxy (delegates to optionModelForTier)
  Q_INVOKABLE bool optNullable(const QString &tier, int index) const;
  // Per-option isVector flag proxy
  Q_INVOKABLE bool optIsVector(const QString &tier, int index) const;
  // Per-option sidetext proxy
  Q_INVOKABLE QString optSidetext(const QString &tier, int index) const;
  // Group names for a given tier (delegates to optionModel->groupNames())
  Q_INVOKABLE QStringList groupNames(const QString &tier) const;
  // Per-group dirty count
  Q_INVOKABLE int dirtyCountForGroup(const QString &tier, const QString &groupName) const;

  Q_INVOKABLE bool requestSavePendingChanges();
  Q_INVOKABLE bool requestDiscardPendingChanges();
  Q_INVOKABLE bool requestCancelPendingChanges();
  /// v5.16 (PSET2-03): single dirty-guard modal gate. Returns false when a
  /// dialog is already up (the caller must not open a second one).
  Q_INVOKABLE bool beginUnsavedDialog();
  Q_INVOKABLE void endUnsavedDialog();
  /// v5.16 (PSET2-03): Transfer — apply the selected modified keys onto the
  /// pending target preset without saving the source (upstream
  /// UnsavedChangesDialog Action::Transfer, UnsavedChangesDialog.cpp:1087,
  /// 2380). Only valid for preset-switch pending actions. Reverts the
  /// source tier's edits and proceeds with the pending switch.
  Q_INVOKABLE bool transferPendingChanges(const QStringList &keys);
  /// v5.16 (PSET2-06): resize the per-extruder filament preset slot vector
  /// (upstream update_multi_material_filament_presets resizes
  /// filament_presets with the extruder count).
  Q_INVOKABLE void setExtruderCount(int count);
  /// v5.16 (PSET2-06): preset-selection overlay persisted into the saved
  /// project config (printer/filament/print preset ids + the
  /// ";"-separated filament_presets slot vector, slot 0 = global selection).
  QVariantMap projectPresetConfigOverlay() const;

  Q_INVOKABLE QList<int> filterOptionIndices(const QString &category, const QString &searchText, bool advancedMode = false) const;
  Q_INVOKABLE QList<int> moveListItem(int fromRow, int toRow) const;
  Q_INVOKABLE QList<int> searchOptions(const QString &query) const;
  Q_INVOKABLE QString valueSourceForKey(const QString &key) const;
  Q_INVOKABLE QString valueChainForKey(const QString &key) const;
  /// Phase 236 (DLG-02): write a single option value by key (EditGCodeDialog
  /// save path). Routes through the owning ConfigOptionModel::setValue so the
  /// existing tier-mapping / dirty / scope pipeline applies — identical to an
  /// inline OptionRow edit. Returns false when no option model owns the key.
  Q_INVOKABLE bool setValue(const QString &key, const QVariant &value);
  /// EditGCodeDialog placeholder tree (upstream EditGCodeDialog::init_params_list,
  /// EditGCodeDialog.cpp:153-243): the 9 top-level groups built from the real
  /// ConfigDefs (cgp_* defs, PrintConfig.cpp:12410-12689) plus the
  /// custom-G-code "Specific for <key>" group (custom_gcode_specific_placeholders,
  /// PrintConfig.cpp:12695-12720) and the Presets group fed by
  /// Preset::print/filament/printer_options(). Each group is
  /// {id, params: [{key, kind, typeStr, label, fullLabel, tooltip}],
  ///  subgroups: [{id, params}]}; kind: 0=Scalar, 1=Vector (upstream get_type,
  /// EditGCodeDialog.cpp:148-151 -- FilamentVector is never constructed there).
  Q_INVOKABLE QVariantList editGcodeParamGroups(const QString &customGcodeKey) const;
  Q_INVOKABLE bool resetOptionToLevel(const QString &key, int level);
  Q_INVOKABLE QString searchResultSource(int searchIndex) const;
  Q_INVOKABLE QString searchResultPath(int searchIndex) const;
  Q_INVOKABLE QString searchResultGroup(int searchIndex) const;
  Q_INVOKABLE QString searchResultCategory(int searchIndex) const;
  Q_INVOKABLE QString searchResultPage(int searchIndex) const;
  Q_INVOKABLE QList<int> filterIndicesByCategory(const QList<int> &indices, const QString &category) const;
  Q_INVOKABLE QList<int> filterIndicesByPage(const QList<int> &indices, const QString &page) const;
  Q_INVOKABLE QString scopeDiffSummary(const QString &key) const;

  Q_INVOKABLE int scopeOverrideCount() const;
  Q_INVOKABLE QString scopeOverriddenKey(int index) const;
  Q_INVOKABLE bool resetScopeOverride(const QString &key);
  Q_INVOKABLE void resetAllScopeOverrides();

  Q_INVOKABLE QHash<QString, QVariant> mergedConfigValues() const;
  Q_INVOKABLE void applyProjectConfig(const QHash<QString, QVariant> &config);

  Q_INVOKABLE QString globalModifiedKey(int index) const;
  Q_INVOKABLE QString globalModifiedCurrentValue(const QString &key) const;
  Q_INVOKABLE QString globalModifiedDefaultValue(const QString &key) const;
  // Upstream UnsavedChangesDialog rows carry the option's category / group /
  // display name (PresetItem, UnsavedChangesDialog.cpp:1396-1448) and group
  // the list as category -> group -> option (update_list, :1329-1460).
  // Resolve those from the active tier's option model; empty when the key
  // has no model entry (globalModifiedLabel falls back to the raw key).
  Q_INVOKABLE QString globalModifiedCategory(const QString &key) const;
  Q_INVOKABLE QString globalModifiedGroup(const QString &key) const;
  Q_INVOKABLE QString globalModifiedLabel(const QString &key) const;
  Q_INVOKABLE bool resetGlobalOption(const QString &key);
  Q_INVOKABLE void resetAllGlobalOptions();
  /// Restore the active tier to its resolved system/parent values without
  /// writing the selected preset (upstream Tab::on_roll_back_value(true)).
  Q_INVOKABLE bool restoreAllSystemValues();
  Q_INVOKABLE QString materialPresetName(int localIndex) const;

  Q_INVOKABLE int layerRangeCount() const;
  Q_INVOKABLE double layerRangeMinZ(int rangeIndex) const;
  Q_INVOKABLE double layerRangeMaxZ(int rangeIndex) const;
  Q_INVOKABLE bool addLayerRange(double minZ, double maxZ);
  Q_INVOKABLE bool removeLayerRange(int rangeIndex);
  Q_INVOKABLE bool setLayerRangeValue(int rangeIndex, const QString &key, const QVariant &value);
  Q_INVOKABLE QVariant layerRangeValue(int rangeIndex, const QString &key, const QVariant &fallback = QVariant()) const;

signals:
  void stateChanged();
  void sliceAffectingConfigChanged();
  void pendingUnsavedChangesRequested();
  void pendingActionApplied(const QString &action, const QString &target);
  void pendingActionCleared();
  void saveAsRequired();
  /// Phase 147 (PSET-02): emitted by requestCreatePreset; SettingsDialog binds
  /// it to open CreatePresetsDialog.
  void createPresetRequired();
  /// Phase 154 (CLOS-01): emitted by requestComparePresets; SettingsDialog
  /// binds it to open PresetDiffDialog.
  void comparePresetsRequired();
  /// filamentSlotCompatibility content changed (only when the diff guard sees
  /// a real change).
  void filamentSlotCompatibilityChanged();

private:
  PresetServiceMock *presetService_ = nullptr;
  ProjectServiceMock *projectService_ = nullptr;
  ConfigOptionModel *printOptions_ = nullptr;
  ConfigOptionModel *machineOptions_ = nullptr;
  ConfigOptionModel *filamentOptions_ = nullptr;
  PresetListModel *presetList_ = nullptr;

  void applyScopeValues();
  void handleOptionValueChanged(const QString &key, const QVariant &value);
  QVariant scopedValueForKey(const QString &key, const QVariant &fallback) const;
  QHash<QString, QVariant> buildScopeValues() const;
  QSet<QString> readonlyKeysForCurrentScope() const;
  QHash<QString, QVariant> effectivePresetValuesForTier(const QString &tier) const;
  QHash<QString, QVariant> editableValuesForTier(const QString &tier) const;
  QHash<QString, QVariant> referenceValuesForTier(const QString &tier) const;
  QHash<QString, QVariant> selectedPresetValuesForTier(const QString &tier) const;
  QHash<QString, QVariant> persistedEffectiveValuesForTier(const QString &tier) const;
  ConfigOptionModel *optionModelForTier(const QString &tier) const;
  QString normalizedTier(const QString &tier) const;
  void refreshOptionModelReferences();
  void updateMergedPresetValues();
  /// Push the valueSources_ mirror onto the three option models
  /// (ConfigOptionModel::setValueSources) so delegates can read it as the
  /// valueSource role. Call after every valueSources_ rebuild/patch.
  void pushValueSourcesToModels();
  /// Recompute filamentSlotCompatibility_ (length = 1 + filamentSlotPresets_
  /// size, item i = isFilamentCompatible(filamentPresetForSlot(i))); emits
  /// filamentSlotCompatibilityChanged only on a real diff.
  void refreshFilamentSlotCompatibility();
  bool queuePendingAction(const QString &action, const QString &target);
  void clearPendingAction();
  bool applyPendingAction();
  void setCurrentPresetTierValue(const QString &tier, const QString &presetName);
  void mergePresetHierarchy();
  /// v5.16 (PSET2-05): shared builder for the three decorated* Q_PROPERTYs.
  QStringList decoratedPresetNamesForCategory(int category) const;
  /// v5.16 (PSET2-05): refresh presetList_ from the service including the
  /// per-preset section + compatibility data for the active printer.
  void refreshPresetListModel();
  /// Current tier selection used by the SavePresetDialog helpers (mirrors
  /// saveCurrentPreset's tier lookup, including the legacy currentPreset_
  /// fallback for the print tier).
  QString currentPresetNameForCategory(int category) const;
  /// Upstream cpp:234-237 alias lookup. The Qt6 preset backend carries no
  /// alias metadata yet, so this always returns empty and the alias-collision
  /// check stays dormant until the backend exposes aliases.
  QString presetAliasFor(const QString &name) const;

  QString currentPreset_;
  double layerHeight_ = 0.2;
  int printSpeed_ = 300;
  bool supportEnabled_ = false;
  int infillDensity_ = 15;
  int nozzleTemp_ = 220;
  int bedTemp_ = 65;
  int wallCount_ = 3;
  int topLayers_ = 4;
  int bottomLayers_ = 4;
  bool enableBrim_ = false;
  QString activePresetTier_ = QStringLiteral("print");
  QString lastPresetError_;
  QString settingsScope_ = QStringLiteral("global");
  QString settingsTargetType_;
  QString settingsTargetName_;
  int settingsTargetObjectIndex_ = -1;
  int settingsTargetVolumeIndex_ = -1;
  int settingsTargetPlateIndex_ = -1;
  bool applyingScopeValues_ = false;
  QHash<QString, QVariant> globalOptionValues_;
  QSet<QString> scopedWritableKeys_;

  QString currentPrinterPreset_;
  QString currentFilamentPreset_;
  QString currentPrintPreset_;
  /// v5.16 (CIRC-04): slots 1..N; slot 0 is currentFilamentPreset_ itself.
  QStringList filamentSlotPresets_;
  /// Compatibility bitmap mirror of filamentSlotPresets_ (+ slot 0), see
  /// refreshFilamentSlotCompatibility().
  QList<int> filamentSlotCompatibility_;
  QHash<QString, QVariant> printerPresetValues_;
  QHash<QString, QVariant> filamentPresetValues_;
  QHash<QString, QVariant> printPresetValues_;
  QHash<QString, QString> valueSources_;
  QString pendingUnsavedAction_;
  QString pendingUnsavedTarget_;
  /// v5.16 (PSET2-03): dirty-guard modal re-entry gate.
  bool m_unsavedDialogActive = false;
  mutable QList<int> m_lastSearchResults_;

  /// SavePresetDialog radio state (upstream Item::m_save_to_project,
  /// SavePresetDialog.hpp:75).
  bool m_saveToProject = false;
  /// Presets saved with "Preset Inside Project" (upstream
  /// Preset::is_project_embedded). Presets adopted from a loaded 3MF are
  /// registered by BackendContext straight on the service, so they are not
  /// tracked in this set.
  QSet<QString> m_projectEmbeddedPresetNames;
  /// Physical-printer selection mirror (upstream
  /// PhysicalPrinterCollection::get_selected_*). Empty = no selection.
  QString m_physicalPrinterName;
  QString m_physicalPrinterPresetName;
};
