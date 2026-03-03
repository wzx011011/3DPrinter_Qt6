#include "PresetListModel.h"

PresetListModel::PresetListModel(QObject *parent)
    : QAbstractListModel(parent)
{
  m_presets = {
      // ── 打印质量 ─────────────────────────────────────────────
      {tr("0.20mm 标准 @Creality K1C"), tr("打印质量"), true},
      {tr("0.16mm 精细"), tr("打印质量"), false},
      {tr("0.24mm 草稿"), tr("打印质量"), false},
      {tr("0.28mm 超草稿"), tr("打印质量"), false},
      {tr("0.12mm 超精细"), tr("打印质量"), false},
      // ── 耗材 ─────────────────────────────────────────────────
      {"Hyper PLA", tr("耗材"), true},
      {"Hyper PETG", tr("耗材"), false},
      {"ABS", tr("耗材"), false},
      {"TPU 95A", tr("耗材"), false},
      {"PLA+", tr("耗材"), false},
      // ── 打印机 ────────────────────────────────────────────────
      {"Creality K1C 0.4 nozzle", tr("打印机"), true},
      {"Creality K1 Max 0.4 nozzle", tr("打印机"), false},
      {"Creality Ender-3 V3", tr("打印机"), false},
  };
}

int PresetListModel::rowCount(const QModelIndex &parent) const
{
  return parent.isValid() ? 0 : m_presets.size();
}

QVariant PresetListModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || index.row() >= m_presets.size())
    return {};
  const auto &p = m_presets[index.row()];
  switch (role)
  {
  case NameRole:
    return p.name;
  case CategoryRole:
    return p.category;
  case IsDefaultRole:
    return p.isDefault;
  default:
    return {};
  }
}

QHash<int, QByteArray> PresetListModel::roleNames() const
{
  return {
      {NameRole, "presetName"},
      {CategoryRole, "presetCategory"},
      {IsDefaultRole, "presetIsDefault"},
  };
}

QString PresetListModel::presetName(int i) const { return (i >= 0 && i < m_presets.size()) ? m_presets[i].name : QString{}; }
QString PresetListModel::presetCategory(int i) const { return (i >= 0 && i < m_presets.size()) ? m_presets[i].category : QString{}; }
bool PresetListModel::presetIsDefault(int i) const { return (i >= 0 && i < m_presets.size()) && m_presets[i].isDefault; }

int PresetListModel::countByCategory(const QString &category) const
{
  int c = 0;
  for (const auto &p : m_presets)
    if (p.category == category)
      ++c;
  return c;
}

int PresetListModel::globalIndex(const QString &category, int localIndex) const
{
  int local = 0;
  for (int i = 0; i < m_presets.size(); ++i)
  {
    if (m_presets[i].category == category)
    {
      if (local == localIndex)
        return i;
      ++local;
    }
  }
  return -1;
}
