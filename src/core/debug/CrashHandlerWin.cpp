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

      const bool hasSymbol = SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol) == TRUE;
      const bool hasLine = SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &line) == TRUE;

      if (hasSymbol && hasLine)
      {
        appendCrashLog(QStringLiteral("#%1 0x%2 %3 +0x%4 (%5:%6)")
                           .arg(i)
                           .arg(qulonglong(frame.AddrPC.Offset), 0, 16)
                           .arg(QString::fromUtf8(symbol->Name))
                           .arg(qulonglong(displacement), 0, 16)
                           .arg(QString::fromLocal8Bit(line.FileName))
                           .arg(int(line.LineNumber)));
      }
      else if (hasSymbol)
      {
        appendCrashLog(QStringLiteral("#%1 0x%2 %3 +0x%4")
                           .arg(i)
                           .arg(qulonglong(frame.AddrPC.Offset), 0, 16)
                           .arg(QString::fromUtf8(symbol->Name))
                           .arg(qulonglong(displacement), 0, 16));
      }
      else
      {
        appendCrashLog(QStringLiteral("#%1 0x%2")
                           .arg(i)
                           .arg(qulonglong(frame.AddrPC.Offset), 0, 16));
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

      const auto dumpType = static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithThreadInfo);
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
    appendCrashLog(QStringLiteral("Crash handler installed: %1").arg(dir.absolutePath())); });
#else
    Q_UNUSED(dumpDir);
#endif
  }
} // namespace
