#include "CrashHandlerWin.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dbghelp.h>

#include <cstdio>
#include <mutex>
#include <atomic>

#pragma comment(lib, "dbghelp.lib")

namespace
{
  std::wstring g_dumpDir;
  std::once_flag g_once;

  void appendCrashLog(const QString &line)
  {
    const QString path = QString::fromStdWString(g_dumpDir) + QStringLiteral("/crash_stack.log");
    QFile file(path);
    if (!file.open(QIODevice::Append | QIODevice::Text))
      return;
    QTextStream out(&file);
    out << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " " << line << "\n";
  }

  void dumpStackTrace(CONTEXT *ctx)
  {
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(process, nullptr, TRUE))
    {
      appendCrashLog(QStringLiteral("SymInitialize failed"));
      return;
    }

    STACKFRAME64 frame = {};
#if defined(_M_X64)
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = ctx->Rip;
    frame.AddrFrame.Offset = ctx->Rbp;
    frame.AddrStack.Offset = ctx->Rsp;
#elif defined(_M_IX86)
    DWORD machineType = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = ctx->Eip;
    frame.AddrFrame.Offset = ctx->Ebp;
    frame.AddrStack.Offset = ctx->Esp;
#else
    DWORD machineType = 0;
#endif

    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;

    appendCrashLog(QStringLiteral("=== stack begin ==="));
    for (int i = 0; i < 80; ++i)
    {
      if (!StackWalk64(machineType,
                       process,
                       thread,
                       &frame,
                       ctx,
                       nullptr,
                       SymFunctionTableAccess64,
                       SymGetModuleBase64,
                       nullptr))
        break;

      if (frame.AddrPC.Offset == 0)
        break;

      constexpr DWORD kNameMax = 1024;
      BYTE symbolBuffer[sizeof(SYMBOL_INFO) + kNameMax] = {};
      auto *symbol = reinterpret_cast<SYMBOL_INFO *>(symbolBuffer);
      symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
      symbol->MaxNameLen = kNameMax - 1;

      DWORD64 displacement = 0;
      IMAGEHLP_LINE64 line = {};
      line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
      DWORD lineDisplacement = 0;

      // WIN-CRASH-DIAG: export-only symbols (deployed Qt6 DLLs) resolve to
      // generic names; the module file name is what makes a frame
      // attributable, so print it on every branch.
      IMAGEHLP_MODULE64 mi = {};
      mi.SizeOfStruct = sizeof(mi);
      char moduleName[kNameMax] = {};
      if (SymGetModuleInfo64(process, frame.AddrPC.Offset, &mi))
      {
        const char *sep = strrchr(mi.ImageName, '\\');
        strncpy_s(moduleName, sizeof(moduleName), sep ? sep + 1 : mi.ImageName, _TRUNCATE);
      }

      const bool hasSymbol = SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol) == TRUE;
      const bool hasLine = SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &line) == TRUE;

      if (hasSymbol && hasLine)
      {
        appendCrashLog(QStringLiteral("#%1 0x%2 %3!%4 +0x%5 (%6:%7)")
                           .arg(i)
                           .arg(qulonglong(frame.AddrPC.Offset), 0, 16)
                           .arg(QString::fromLocal8Bit(moduleName))
                           .arg(QString::fromUtf8(symbol->Name))
                           .arg(qulonglong(displacement), 0, 16)
                           .arg(QString::fromLocal8Bit(line.FileName))
                           .arg(int(line.LineNumber)));
      }
      else if (hasSymbol)
      {
        appendCrashLog(QStringLiteral("#%1 0x%2 %3!%4 +0x%5")
                           .arg(i)
                           .arg(qulonglong(frame.AddrPC.Offset), 0, 16)
                           .arg(QString::fromLocal8Bit(moduleName))
                           .arg(QString::fromUtf8(symbol->Name))
                           .arg(qulonglong(displacement), 0, 16));
      }
      else
      {
        appendCrashLog(QStringLiteral("#%1 0x%2 %3")
                           .arg(i)
                           .arg(qulonglong(frame.AddrPC.Offset), 0, 16)
                           .arg(QString::fromLocal8Bit(moduleName)));
      }
    }
    appendCrashLog(QStringLiteral("=== stack end ==="));
    SymCleanup(process);
  }

  LONG WINAPI topLevelFilter(EXCEPTION_POINTERS *ep)
  {
    const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
    const DWORD pid = GetCurrentProcessId();
    const QString dumpPath = QString::fromStdWString(g_dumpDir) +
                             QStringLiteral("/FramelessDialogDemo_%1_pid%2.dmp").arg(ts).arg(pid);

    appendCrashLog(QStringLiteral("Unhandled exception code=0x%1 at=0x%2")
                       .arg(ep && ep->ExceptionRecord ? QString::number(ep->ExceptionRecord->ExceptionCode, 16) : QStringLiteral("unknown"))
                       .arg(ep && ep->ExceptionRecord ? QString::number(qulonglong(ep->ExceptionRecord->ExceptionAddress), 16) : QStringLiteral("unknown")));

    if (ep && ep->ContextRecord)
      dumpStackTrace(ep->ContextRecord);

    HANDLE hFile = CreateFileW(reinterpret_cast<LPCWSTR>(dumpPath.utf16()),
                               GENERIC_WRITE,
                               0,
                               nullptr,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               nullptr);

    if (hFile != INVALID_HANDLE_VALUE)
    {
      MINIDUMP_EXCEPTION_INFORMATION mei = {};
      mei.ThreadId = GetCurrentThreadId();
      mei.ExceptionPointers = ep;
      mei.ClientPointers = FALSE;

      // WIN-CRASH-DIAG (2026-09-15): MiniDumpWithUnloadedModules records
      // DLLs that unmounted before the crash -- frames landing in unmapped
      // memory (observed in the AI-sidecar startup race: worker thread RIPs
      // pointing outside any module) become attributable. ThreadInfo keeps
      // per-thread teardown state for thread-exit crashes.
      const auto dumpType = static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);
      const BOOL ok = MiniDumpWriteDump(GetCurrentProcess(),
                                        pid,
                                        hFile,
                                        dumpType,
                                        ep ? &mei : nullptr,
                                        nullptr,
                                        nullptr);
      CloseHandle(hFile);
      appendCrashLog(ok ? QStringLiteral("Minidump written: %1").arg(dumpPath)
                        : QStringLiteral("MiniDumpWriteDump failed: %1").arg(GetLastError()));
    }
    else
    {
      appendCrashLog(QStringLiteral("CreateFile dump failed: %1").arg(GetLastError()));
    }

    return EXCEPTION_EXECUTE_HANDLER;
  }

  // WIN-CRASH-DIAG: fail-fast terminations (__fastfail / stack cookie,
  // 0xC0000409; invalid parameter, 0xC0000417) bypass
  // SetUnhandledExceptionFilter entirely -- the process dies with no log
  // and no dump. That silent-exit variant was observed in the AI sidecar
  // startup race (AI-SIDECAR-STARTUP-RACE ledger entry). A first-chance
  // vectored handler sees them before death: log the faulting context,
  // then always continue so the normal termination proceeds.
  std::atomic<bool> g_inFailFastHandler{false};
  LONG WINAPI failFastVectoredHandler(EXCEPTION_POINTERS *ep)
  {
    if (!ep || !ep->ExceptionRecord)
      return EXCEPTION_CONTINUE_SEARCH;
    const DWORD code = ep->ExceptionRecord->ExceptionCode;
    if (code != 0xC0000409u && code != 0xC0000417u)
      return EXCEPTION_CONTINUE_SEARCH;
    bool expected = false;
    if (!g_inFailFastHandler.compare_exchange_strong(expected, true))
      return EXCEPTION_CONTINUE_SEARCH;
    appendCrashLog(QStringLiteral("Fail-fast exception code=0x%1 at=0x%2")
                       .arg(QString::number(code, 16))
                       .arg(ep->ExceptionRecord->ExceptionAddress
                                ? QString::number(qulonglong(reinterpret_cast<uintptr_t>(ep->ExceptionRecord->ExceptionAddress)), 16)
                                : QStringLiteral("unknown")));
    if (ep->ContextRecord)
      dumpStackTrace(ep->ContextRecord);
    g_inFailFastHandler.store(false);
    return EXCEPTION_CONTINUE_SEARCH;
  }
} // namespace
#endif

namespace CrashHandlerWin
{
  void install(const QString &dumpDir)
  {
#ifdef Q_OS_WIN
    std::call_once(g_once, [&dumpDir]()
                   {
    QDir dir(dumpDir);
    if (!dir.exists())
      dir.mkpath(QStringLiteral("."));
    g_dumpDir = QDir::toNativeSeparators(dir.absolutePath()).toStdWString();
    SetUnhandledExceptionFilter(topLevelFilter);
    // Fail-fast events bypass the unhandled filter; see the handler above.
    AddVectoredExceptionHandler(1, failFastVectoredHandler);
    appendCrashLog(QStringLiteral("Crash handler installed: %1").arg(dir.absolutePath())); });
#else
    Q_UNUSED(dumpDir);
#endif
  }
} // namespace
