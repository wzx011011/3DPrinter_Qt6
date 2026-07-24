#include "AmsMaterialsViewModel.h"

#include <QSettings>

// P8.4 / Phase 201 (v5.6) -- AmsMaterialsViewModel implementation.
//
// Data source remains mock; the architectural cleanup moves the hardcoded
// AMSSettingsDialog.qml literals into C++ and adds QSettings persistence so
// user edits survive dialog close and application restart.
//
// No network / device / cloud access. All persistence is local QSettings
// under the ams/materials/* namespace.

namespace {
// QSettings key prefix for every persisted AMS materials value.
constexpr const char *kSettingsPrefix = "ams/materials";
// Sentinel version key; bump when the persisted shape changes.
constexpr const char *kSettingsVersionKey = "ams/materials/version";
constexpr int kSettingsVersion = 1;

// Default mock colors mirror Theme.statusInfo / statusError / accent /
// statusWarning hex tokens (Theme.qml). Kept in sync so the default visual
// is byte-identical to pre-Phase-201.
const QStringList kDefaultSlotColors = {
    QStringLiteral("#3b9eff"),  // statusInfo   (blue)
    QStringLiteral("#e04040"),  // statusError  (red)
    QStringLiteral("#18c75e"),  // accent       (green)
    QStringLiteral("#f5a623"),  // statusWarning (yellow)
};

// Known material catalog (selectable in the material combo). Mirrors the
// pre-Phase-201 QML materialTypes literal verbatim.
const QStringList kDefaultMaterialTypes = {
    QStringLiteral("PLA"), QStringLiteral("ABS"), QStringLiteral("PETG"),
    QStringLiteral("TPU"), QStringLiteral("ASA"), QStringLiteral("PC"),
    QStringLiteral("PA-CF"), QStringLiteral("PVA"),
};
}  // namespace

AmsMaterialsViewModel::AmsMaterialsViewModel(QObject *parent)
  : QObject(parent)
{
  initDefaults();
  loadFromSettings();
}

AmsMaterialsViewModel::~AmsMaterialsViewModel() = default;

void AmsMaterialsViewModel::initDefaults()
{
  m_defaultSlotColors = kDefaultSlotColors;
  m_materialTypes = kDefaultMaterialTypes;

  // Default mock slot names (aligned with upstream AMS slot model). Wrapped
  // in tr() so they remain translatable like the original QML qsTr() usage.
  m_slotNames = {
      tr("蓝色 PLA"),
      tr("红色 ABS"),
      tr("绿色 PETG"),
      tr("黄色 TPU"),
  };
  m_slotMaterials = {
      QStringLiteral("PLA"),
      QStringLiteral("ABS"),
      QStringLiteral("PETG"),
      QStringLiteral("TPU"),
  };
  m_slotAutoSwap = {true, false, true, false};
  m_remainingPct = {65, 42, 88, 15};

  // Default mock mapping rules (aligned with upstream AmsMappingPopup).
  m_mappingRules = {
      {1, 1, 215},
      {2, 2, 230},
      {3, 3, 240},
  };
}

void AmsMaterialsViewModel::loadFromSettings()
{
  QSettings settings;
  const int version = settings.value(QLatin1String(kSettingsVersionKey), 0).toInt();
  if (version < kSettingsVersion) {
    // Shape changed or fresh install: keep defaults, persist a baseline so
    // later edits write back cleanly.
    saveToSettings();
    return;
  }

  // Slot names: load if we have exactly slotCount entries.
  const QStringList names =
      settings.value(QStringLiteral("%1/slotNames").arg(kSettingsPrefix)).toStringList();
  if (names.size() == m_slotCount)
    m_slotNames = names;

  const QStringList materials =
      settings.value(QStringLiteral("%1/slotMaterials").arg(kSettingsPrefix)).toStringList();
  if (materials.size() == m_slotCount)
    m_slotMaterials = materials;

  // QVariantList round-trips bools reliably through QSettings JSON value.
  const QVariant autoSwapVar =
      settings.value(QStringLiteral("%1/slotAutoSwap").arg(kSettingsPrefix));
  if (autoSwapVar.isValid()) {
    const QVariantList list = autoSwapVar.toList();
    if (list.size() == m_slotCount) {
      m_slotAutoSwap.clear();
      for (const auto &v : list)
        m_slotAutoSwap.append(v.toBool());
    }
  }

  const QVariant pctVar =
      settings.value(QStringLiteral("%1/remainingPct").arg(kSettingsPrefix));
  if (pctVar.isValid()) {
    const QVariantList list = pctVar.toList();
    if (list.size() == m_slotCount) {
      m_remainingPct.clear();
      for (const auto &v : list)
        m_remainingPct.append(v.toInt());
    }
  }

  // Mapping rules are stored as a QVariantList of QVariantMap rows.
  const QVariant rulesVar =
      settings.value(QStringLiteral("%1/mappingRules").arg(kSettingsPrefix));
  if (rulesVar.isValid()) {
    const QVariantList list = rulesVar.toList();
    if (!list.isEmpty()) {
      m_mappingRules.clear();
      for (const auto &v : list) {
        const QVariantMap row = v.toMap();
        MappingRule r;
        r.slot = row.value(QStringLiteral("slot"), 0).toInt();
        r.extruder = row.value(QStringLiteral("extruder"), 0).toInt();
        r.temp = row.value(QStringLiteral("temp"), 0).toInt();
        m_mappingRules.append(r);
      }
    }
  }
}

void AmsMaterialsViewModel::saveToSettings() const
{
  QSettings settings;
  settings.setValue(QLatin1String(kSettingsVersionKey), kSettingsVersion);

  settings.setValue(QStringLiteral("%1/slotNames").arg(kSettingsPrefix), m_slotNames);
  settings.setValue(QStringLiteral("%1/slotMaterials").arg(kSettingsPrefix), m_slotMaterials);

  QVariantList autoSwapList;
  for (bool v : m_slotAutoSwap)
    autoSwapList.append(v);
  settings.setValue(QStringLiteral("%1/slotAutoSwap").arg(kSettingsPrefix), autoSwapList);

  QVariantList pctList;
  for (int v : m_remainingPct)
    pctList.append(v);
  settings.setValue(QStringLiteral("%1/remainingPct").arg(kSettingsPrefix), pctList);

  QVariantList rulesList;
  for (const auto &r : m_mappingRules) {
    rulesList.append(QVariantMap{
        {QStringLiteral("slot"), r.slot},
        {QStringLiteral("extruder"), r.extruder},
        {QStringLiteral("temp"), r.temp},
    });
  }
  settings.setValue(QStringLiteral("%1/mappingRules").arg(kSettingsPrefix), rulesList);
}

void AmsMaterialsViewModel::clearSettings() const
{
  QSettings settings;
  settings.remove(QLatin1String(kSettingsPrefix));
  settings.remove(QLatin1String(kSettingsVersionKey));
}

// ── Property getters ──────────────────────────────────────────

int AmsMaterialsViewModel::slotCount() const
{
  return m_slotCount;
}

QVariantList AmsMaterialsViewModel::slotColors() const
{
  QVariantList result;
  result.reserve(m_defaultSlotColors.size());
  for (const auto &c : m_defaultSlotColors)
    result.append(c);
  return result;
}

QStringList AmsMaterialsViewModel::slotNames() const
{
  return m_slotNames;
}

QStringList AmsMaterialsViewModel::slotMaterials() const
{
  return m_slotMaterials;
}

QVariantList AmsMaterialsViewModel::slotAutoSwap() const
{
  QVariantList result;
  result.reserve(m_slotAutoSwap.size());
  for (bool v : m_slotAutoSwap)
    result.append(v);
  return result;
}

QVariantList AmsMaterialsViewModel::remainingPct() const
{
  QVariantList result;
  result.reserve(m_remainingPct.size());
  for (int v : m_remainingPct)
    result.append(v);
  return result;
}

QStringList AmsMaterialsViewModel::materialTypes() const
{
  return m_materialTypes;
}

QVariantList AmsMaterialsViewModel::mappingRules() const
{
  QVariantList result;
  result.reserve(m_mappingRules.size());
  for (const auto &r : m_mappingRules) {
    result.append(QVariantMap{
        {QStringLiteral("slot"), r.slot},
        {QStringLiteral("extruder"), r.extruder},
        {QStringLiteral("temp"), r.temp},
    });
  }
  return result;
}

int AmsMaterialsViewModel::mappingRuleCount() const
{
  return m_mappingRules.size();
}

// ── Edit API ──────────────────────────────────────────────────

void AmsMaterialsViewModel::setSlotName(int idx, const QString &name)
{
  if (idx < 0 || idx >= m_slotCount)
    return;
  if (m_slotNames[idx] == name)
    return;
  m_slotNames[idx] = name;
  saveToSettings();
  emit stateChanged();
}

void AmsMaterialsViewModel::setSlotMaterial(int idx, const QString &material)
{
  if (idx < 0 || idx >= m_slotCount)
    return;
  if (m_slotMaterials[idx] == material)
    return;
  m_slotMaterials[idx] = material;
  saveToSettings();
  emit stateChanged();
}

void AmsMaterialsViewModel::setSlotAutoSwap(int idx, bool on)
{
  if (idx < 0 || idx >= m_slotCount)
    return;
  if (m_slotAutoSwap[idx] == on)
    return;
  m_slotAutoSwap[idx] = on;
  saveToSettings();
  emit stateChanged();
}

void AmsMaterialsViewModel::setRemainingPct(int idx, int pct)
{
  if (idx < 0 || idx >= m_slotCount)
    return;
  pct = qBound(0, pct, 100);
  if (m_remainingPct[idx] == pct)
    return;
  m_remainingPct[idx] = pct;
  saveToSettings();
  emit stateChanged();
}

void AmsMaterialsViewModel::addMappingRule(int slot, int extruder, int temp)
{
  m_mappingRules.append({slot, extruder, temp});
  saveToSettings();
  emit stateChanged();
}

void AmsMaterialsViewModel::removeMappingRule(int idx)
{
  if (idx < 0 || idx >= m_mappingRules.size())
    return;
  m_mappingRules.removeAt(idx);
  saveToSettings();
  emit stateChanged();
}

void AmsMaterialsViewModel::resetToDefaults()
{
  clearSettings();
  initDefaults();
  saveToSettings();
  emit stateChanged();
}
