#pragma once
#include <QString>

// ── Error severity levels (architecture doc §4.6) ────────────────────────────
// 0  Info    — Toast, 3 s auto-dismiss
// 1  Warning — Top Banner, manual close
// 2  Error   — Modal Dialog, user confirm
// 3  Fatal   — Dialog + auto-save + suggest restart
// ─────────────────────────────────────────────────────────────────────────────

struct ServiceError
{
  int code = 0;
  QString message;         // User-visible (already i18n'd)
  int severity = 0;        // See levels above
  QString technicalDetail; // Dev/debug only, not shown in Release UI
};

template <typename T>
struct ServiceResult
{
  bool ok = false;
  T value = {};
  ServiceError error;

  static ServiceResult success(const T &v) { return {true, v, {}}; }
  static ServiceResult failure(const ServiceError &e) { return {false, {}, e}; }
};

// Specialisation for void operations
template <>
struct ServiceResult<void>
{
  bool ok = false;
  ServiceError error;

  static ServiceResult success() { return {true, {}}; }
  static ServiceResult failure(const ServiceError &e) { return {false, e}; }
};
