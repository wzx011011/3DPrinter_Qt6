#pragma once

#include <QHash>
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
// The same upstream functions also rewrite values (apply(config, &new_conf)
// / load_config): those land in Result::forced, keyed for the tier the
// function edits, and the caller writes them back. The firmware-retraction
// wipe conflict (TabPrinter::toggle_options MessageDialog) is reported via
// Result::firmwareRetractionConflict for the caller to ask about.
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
  /// Printer vectors read this extruder's entry (the Extruder N page shown;
  /// upstream TabPrinter::toggle_options runs per extruder page).
  int extruderIndex = 0;
};

/// Filament "Setting Overrides" row (TabFilament::add_filament_overrides_page
/// near-label checkbox): checked = the filament carries its own value
/// (not nil); unchecked rows show the inherited printer / process value.
struct OverrideState
{
  bool checked = false;
  bool checkEnabled = true;
  QVariant inheritedValue;
};

struct Result
{
  QSet<QString> hidden;   ///< toggle_line(key, false)
  QSet<QString> disabled; ///< toggle_field / toggle_option(key, false)
  /// Upstream value rewrites (only keys whose value actually changes).
  QHash<QString, QVariant> forced;
  /// use_firmware_retraction && wipe && retract_before_wipe < 100%.
  bool firmwareRetractionConflict = false;
  /// Filament tier only: override rows keyed by filament_* option key.
  QHash<QString, OverrideState> overrides;
  /// Tab pages the config removes (TabPrinter::build_unregular_pages).
  QSet<QString> hiddenPages;
};

Result printToggles(const Context &ctx);
Result printerToggles(const Context &ctx);
Result filamentToggles(const Context &ctx);
} // namespace OptionToggleRules
