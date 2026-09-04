#pragma once

// Env-gated performance instrumentation for the OWzx perf evaluation
// (docs/perf-baseline.md, scripts/perf/). Enable with OWZX_PERF_LOG=1.
// Every line carries the [PERF] tag so scripts can parse the output.
// Set OWZX_PERF_LOG_FILE=<path> to additionally append the lines to a file
// (qInfo can be lost to encoding/console redirection on Windows).
// Header-only on purpose: no CMake changes needed for consumers.

#include <QElapsedTimer>
#include <QString>
#include <QtGlobal>

#include <cstdio>

#ifdef Q_OS_WIN
// Guards so this header can be included from any translation unit: without
// NOMINMAX the windows.h min/max macros break std::min/std::max users.
// <share.h> provides _SH_DENYNO for the tail-friendly perf log file.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <share.h>
#include <windows.h>
#include <psapi.h>
#endif

namespace PerfLog
{

inline bool enabled()
{
  static const int value = qEnvironmentVariableIntValue("OWZX_PERF_LOG");
  return value != 0;
}

inline void writeLine(const char *text)
{
  qInfo("%s", text);
  static FILE *file = []() -> FILE * {
    const QByteArray path = qgetenv("OWZX_PERF_LOG_FILE");
    if (path.isEmpty())
      return nullptr;
    // _SH_DENYNO lets bench scripts tail the file while the app runs
    // (plain fopen_s("a") takes an exclusive lock).
#ifdef Q_OS_WIN
    FILE *f = _fsopen(path.constData(), "a", _SH_DENYNO);
    return f;
#else
    return std::fopen(path.constData(), "a");
#endif
  }();
  if (file != nullptr) {
    std::fprintf(file, "%s\n", text);
    std::fflush(file);
  }
}

inline void event(const char *label, qint64 nanos, const QString &extra = QString())
{
  if (!enabled())
    return;
  const double ms = double(nanos) / 1e6;
  char line[512];
  if (extra.isEmpty())
    std::snprintf(line, sizeof(line), "[PERF] %-28s %10.2f ms", label, ms);
  else
    std::snprintf(line, sizeof(line), "[PERF] %-28s %10.2f ms | %s", label, ms,
                  extra.toUtf8().constData());
  writeLine(line);
}

// Working-set / peak-working-set snapshot in MiB (Windows only).
inline void memory(const char *label)
{
  if (!enabled())
    return;
#ifdef Q_OS_WIN
  PROCESS_MEMORY_COUNTERS counters{};
  if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
  {
    char line[256];
    std::snprintf(line, sizeof(line),
                  "[PERF] %-28s ws=%8.1f MiB peak=%8.1f MiB", label,
                  double(counters.WorkingSetSize) / 1048576.0,
                  double(counters.PeakWorkingSetSize) / 1048576.0);
    writeLine(line);
  }
#else
  Q_UNUSED(label);
#endif
}

// RAII scope timer. Usage: PerfLog::Scope scope("meshData.build");
// No cost at all when OWZX_PERF_LOG is unset.
class Scope
{
public:
  explicit Scope(const char *label, QString extra = QString())
      : label_(label), extra_(std::move(extra))
  {
    if (PerfLog::enabled())
      timer_.start();
  }

  ~Scope()
  {
    if (PerfLog::enabled())
      PerfLog::event(label_, timer_.nsecsElapsed(), extra_);
  }

  Scope(const Scope &) = delete;
  Scope &operator=(const Scope &) = delete;

private:
  QElapsedTimer timer_;
  const char *label_;
  QString extra_;
};

} // namespace PerfLog
