#pragma once

#include <QString>

namespace OWzx {

enum class TickType {
  PausePrint = 0,
  CustomGcode = 1,
  Template = 2,
  ToolChange = 3,
  ColorChange = 4
};

struct TickCode {
  int tick = 0;
  TickType type = TickType::ColorChange;
  int extruder = 0;
  QString color;
  QString extra;

  bool operator<(const TickCode& other) const { return tick < other.tick; }
  bool operator==(const TickCode& other) const { return tick == other.tick; }
};

} // namespace OWzx
