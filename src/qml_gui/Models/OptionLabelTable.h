#pragma once

#include <QHash>
#include <QString>

// zh_CN row labels for print-config option keys, consumed by
// ConfigOptionModel::DisplayLabelRole ("displayLabel"). Static per-key data —
// never routed through tr(). The table is built once instead of once per
// delegate instantiation (the former OptionLabels.js ROW_LABELS hoist).
namespace OptionLabelTable
{
/// zh_CN display labels keyed by option key; keys outside the map fall back
/// to the model label (ConfigOption::label).
const QHash<QString, QString> &rowLabels();
}
