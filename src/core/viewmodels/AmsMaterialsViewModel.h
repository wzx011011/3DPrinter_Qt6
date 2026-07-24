#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>

/// AmsMaterialsViewModel (Phase 201, v5.6 AMS Architecture Cleanup).
///
/// Replaces the 100% hardcoded mock properties that used to live in
/// AMSSettingsDialog.qml (slotColors / slotNames / slotMaterials /
/// slotAutoSwap / materialTypes / mappingRules / remainingPct). The data
/// source is still mock, but it now lives in C++ and is persisted to
/// QSettings under the "ams/materials/*" namespace, so edits survive
/// dialog close AND application restart (the previous QML-only edit
/// state was lost on close).
///
/// Aligns with upstream AMSMaterialsSetting / AMSSetting / AmsMappingPopup
/// data shapes. No network / device / cloud access: this is local mock only.
///
/// Notification model: a single stateChanged signal fans out to every
/// Q_PROPERTY (batch notification, mirrors EditorViewModel convention).
class AmsMaterialsViewModel final : public QObject
{
  Q_OBJECT

  // ── Slot data (aligns with upstream AMS slot model) ─────────────────
  Q_PROPERTY(int slotCount READ slotCount NOTIFY stateChanged)
  /// Per-slot hex colors (mock defaults mirror Theme.statusInfo/Error/accent/Warning).
  Q_PROPERTY(QVariantList slotColors READ slotColors NOTIFY stateChanged)
  Q_PROPERTY(QStringList slotNames READ slotNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList slotMaterials READ slotMaterials NOTIFY stateChanged)
  Q_PROPERTY(QVariantList slotAutoSwap READ slotAutoSwap NOTIFY stateChanged)
  Q_PROPERTY(QVariantList remainingPct READ remainingPct NOTIFY stateChanged)
  /// Known material type catalog (selectable in the material combo).
  Q_PROPERTY(QStringList materialTypes READ materialTypes NOTIFY stateChanged)

  // ── Mapping rules (aligns with upstream AmsMappingPopup) ────────────
  Q_PROPERTY(QVariantList mappingRules READ mappingRules NOTIFY stateChanged)
  Q_PROPERTY(int mappingRuleCount READ mappingRuleCount NOTIFY stateChanged)

public:
  explicit AmsMaterialsViewModel(QObject *parent = nullptr);
  ~AmsMaterialsViewModel() override;

  // ── Property getters ───────────────────────────────────────────────
  int slotCount() const;
  QVariantList slotColors() const;
  QStringList slotNames() const;
  QStringList slotMaterials() const;
  QVariantList slotAutoSwap() const;
  QVariantList remainingPct() const;
  QStringList materialTypes() const;
  QVariantList mappingRules() const;
  int mappingRuleCount() const;

  // ── Edit API (persisted to QSettings on every successful call) ─────
  Q_INVOKABLE void setSlotName(int idx, const QString &name);
  Q_INVOKABLE void setSlotMaterial(int idx, const QString &material);
  Q_INVOKABLE void setSlotAutoSwap(int idx, bool on);
  Q_INVOKABLE void setRemainingPct(int idx, int pct);

  /// Append a mapping rule {slot, extruder, temp}. New rule is persisted.
  Q_INVOKABLE void addMappingRule(int slot, int extruder, int temp);
  /// Remove the mapping rule at idx. Persisted.
  Q_INVOKABLE void removeMappingRule(int idx);

  /// Restore the original mock defaults and clear persisted overrides.
  Q_INVOKABLE void resetToDefaults();

signals:
  /// Batch notification for every Q_PROPERTY (EditorViewModel convention).
  void stateChanged();

private:
  // Default mock values (mirror the pre-Phase-201 QML literals).
  void initDefaults();
  // Load persisted overrides from QSettings; fall back to defaults.
  void loadFromSettings();
  // Persist current editable state to QSettings under ams/materials/*.
  void saveToSettings() const;
  // Remove persisted overrides (used by resetToDefaults).
  void clearSettings() const;

  // Fixed mock catalog (not editable, not persisted).
  QStringList m_defaultSlotColors;
  QStringList m_materialTypes;

  // Editable, persisted slot state (always sized to slotCount).
  QStringList m_slotNames;
  QStringList m_slotMaterials;
  QList<bool> m_slotAutoSwap;
  QList<int> m_remainingPct;

  // Editable, persisted mapping rules.
  struct MappingRule
  {
    int slot = 0;
    int extruder = 0;
    int temp = 0;
  };
  QList<MappingRule> m_mappingRules;

  // Default slot count (upstream AMS = 4 slots). Constant after construct.
  int m_slotCount = 4;
};
