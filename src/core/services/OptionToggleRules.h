#pragma once

#include <QSet>
#include <QString>
#include <QVariant>

#include <functional>

// OptionToggleRules - port of the upstream dependent-option rules that grey
// out (toggle_field / toggle_option) or hide (toggle_line) settings rows from
// the values of other settings:
//   - Print:    ConfigManipulation::toggle_print_fff_options
//               (ConfigManipulation.cpp:712-1185)
//   - Printer:  TabPrinter::toggle_options (Tab.cpp:6061-6240)
//   - Filament: TabFilament::toggle_options
// Pure functions over value lookups; ConfigViewModel feeds the results to the
// option models. Upstream calls run in order and a later call on the same key
// overrides an earlier one, so the result sets are built the same way.
//
// Not ported (value side effects / dialogs, not visibility): the forced value
// rewrites inside the upstream functions (enable_arc_fitting off,
// fill_multiline = 1, overhang_reverse_threshold = 0, initial_layer_fan_speed
// = -1, the firmware-retraction wipe dialog) and the filament
// "Setting Overrides" page checkboxes (update_filament_overrides_page).
namespace OptionToggleRules
{
using Lookup = std::function<QVariant(const QString &key)>;

struct Context
{
  Lookup print;     ///< effective print (process) values of the edited scope
  Lookup printer;   ///< effective printer preset values
  Lookup filament;  ///< effective filament preset values
  Lookup project;   ///< project-level values (curr_bed_type); may be empty
  bool isBblPrinter = false;  ///< PresetBundle::is_bbl_vendor()
  bool isGlobalConfig = true; ///< false for object / volume / plate scope
};

struct Result
{
  QSet<QString> hidden;   ///< toggle_line(key, false)
  QSet<QString> disabled; ///< toggle_field / toggle_option(key, false)
};

Result printToggles(const Context &ctx);
Result printerToggles(const Context &ctx);
Result filamentToggles(const Context &ctx);
} // namespace OptionToggleRules
