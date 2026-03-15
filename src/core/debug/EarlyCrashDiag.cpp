// Early crash diagnostic — compiled separately, runs before main()
// Uses only Windows API to avoid Boost/Qt include issues
#ifdef Q_OS_WIN
#include <windows.h>
#include <stdio.h>

static LONG WINAPI EarlyCrashHandler(PEXCEPTION_POINTERS ep) {
    fprintf(stderr, "\n=== CRASH BEFORE main() ===\n");
    fprintf(stderr, "Exception code: 0x%08lX\n", (unsigned long)ep->ExceptionRecord->ExceptionCode);
    fprintf(stderr, "Exception address: %p\n", ep->ExceptionRecord->ExceptionAddress);
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

static void NTAPI TlsInit(PVOID, DWORD reason, PVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        SetUnhandledExceptionFilter(EarlyCrashHandler);
        AddVectoredExceptionHandler(1, EarlyCrashHandler);
    }
}

// .CRT$XLC runs after C++ initializers, before main — use XLA for before init
#pragma const_seg(".CRT$XLA")
PIMAGE_TLS_CALLBACK g_tlsInit = TlsInit;
#pragma const_seg()

#endif
