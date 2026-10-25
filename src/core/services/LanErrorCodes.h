#pragma once

#include <QString>

/// LanErrorCodes (feature/lan-printers wrap-up #1).
///
/// Creality LAN printer error-code → user-facing text, ported from the
/// official CrealityPrint device page frontend bundle (AGPL-3.0,
/// resources/web/deviceMgr). The ws push carries `err.errcode`; this table
/// maps the codes the bundle maps, so error surfaces read identically to the
/// official client. Unknown codes fall back to a generic message — never a
/// fabricated cause (audit rule).
class LanErrorCodes
{
public:
  /// 中文文案 for a device errcode; unknown → 通用文案（设备报告了错误码）。
  static QString text(int errcode);
  /// True when the code is in the known table.
  static bool isKnown(int errcode);
};
