// Early crash diagnostic — registers an unhandled-exception filter before
// any dynamic initializer runs, so a crash in pre-main() code prints the
// exception code/address instead of dying silently. CrashHandlerWin::install()
// replaces this filter once main() is reached; this TU covers only the window
// before that.
//
// Uses only the Windows API by design (no CRT stdio, no Qt), which is why the
// platform guard must be the compiler macro _WIN32 and NOT the Qt macro
// Q_OS_WIN: Q_OS_WIN only becomes defined after including a Qt header, and
// this TU's original "#ifdef Q_OS_WIN" therefore compiled to an empty object
// file (audit 2026-10-10 MU1 / Q13 revision note).
#ifdef _WIN32
#include <windows.h>

namespace
{
// Pre-main crash context: avoid CRT formatters (they are not guaranteed
// initialized at the earliest point this fires) — format hex by hand and
// WriteFile directly to the stderr handle.
void writeStderr(const char *text, unsigned len)
{
  HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
  DWORD written = 0;
  if (err && err != INVALID_HANDLE_VALUE)
    WriteFile(err, text, len, &written, nullptr);
}

unsigned hexUintptr(uintptr_t value, char *out)
{
  const char digits[] = "0123456789ABCDEF";
  char tmp[16];
  unsigned n = 0;
  do
    tmp[n++] = digits[value & 0xF];
  while ((value >>= 4) != 0);
  for (unsigned i = 0; i < n; ++i)
    out[i] = tmp[n - 1 - i];
  return n;
}

void appendHex(char *buffer, unsigned &len, const char *label, unsigned labelLen,
               uintptr_t value)
{
  for (unsigned i = 0; i < labelLen; ++i)
    buffer[len++] = label[i];
  len += hexUintptr(value, buffer + len);
  buffer[len++] = '\n';
}

LONG WINAPI EarlyCrashHandler(PEXCEPTION_POINTERS ep)
{
  char buffer[160];
  unsigned len = 0;
  for (const char *p = "\n=== CRASH (unhandled exception; before CrashHandlerWin::install) ===\n"; *p; ++p)
    buffer[len++] = *p;
  const uintptr_t code = ep && ep->ExceptionRecord
                             ? static_cast<uintptr_t>(ep->ExceptionRecord->ExceptionCode)
                             : 0;
  const uintptr_t address = ep && ep->ExceptionRecord
                                ? reinterpret_cast<uintptr_t>(ep->ExceptionRecord->ExceptionAddress)
                                : 0;
  appendHex(buffer, len, "Exception code: 0x", 17, code);
  appendHex(buffer, len, "Exception address: 0x", 21, address);
  writeStderr(buffer, len);
  // Keep searching: CrashHandlerWin (installed from main()) appends the
  // symbolized stack + minidump for crashes that happen after it took over.
  return EXCEPTION_CONTINUE_SEARCH;
}

void NTAPI TlsInit(PVOID, DWORD reason, PVOID)
{
  if (reason == DLL_PROCESS_ATTACH)
  {
    SetUnhandledExceptionFilter(EarlyCrashHandler);
    // Self-verification hook: proves the TLS callback actually ran in this
    // process (the original TU compiled to an empty object, so "is this live?"
    // was a standing question — audit MU1). Opt-in via env var, zero output
    // otherwise.
    char flag[2];
    if (GetEnvironmentVariableA("OWZX_EARLY_CRASH_DIAG", flag, 2) != 0)
    {
      const char *msg = "[EarlyCrashDiag] pre-main unhandled-exception filter installed\n";
      unsigned msgLen = 0;
      while (msg[msgLen] != '\0')
        ++msgLen;
      writeStderr(msg, msgLen);
    }
  }
}
} // namespace

// Loader-invoked TLS callbacks live in .CRT$XL*; .CRT$XLB is the documented
// user slot between the CRT's anchors __xl_a (.CRT$XLA, start of the walked
// callback array) and __xl_z (.CRT$XLZ, end) — i.e. it runs before the CRT
// runs C dynamic initializers, before main() and before any other TU's global
// constructors. Do NOT use .CRT$XLA: contributing to the anchor's own section
// makes the linker place the pointer BEFORE __xl_a, outside the walked range
// (observed 2026-10-12: callback compiled and linked but never invoked).
//
// MU1 root fix (audit 2026-10-10): the earlier revision ALSO registered this
// handler as a first-position vectored exception handler for the whole
// process lifetime, which printed the crash banner for every first-chance
// exception — including ordinary C++ throws that are caught normally. Only
// true unhandled exceptions are reported now (SetUnhandledExceptionFilter).
#pragma section(".CRT$XLB", long, read)
extern "C" __declspec(allocate(".CRT$XLB"))
    PIMAGE_TLS_CALLBACK g_earlyCrashTlsCallback = TlsInit;

#endif // _WIN32
