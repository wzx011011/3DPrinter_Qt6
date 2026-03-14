// DelayLoadHook.cpp — Suppresses DLL-load crashes for delay-loaded OCCT libraries.
//
// OCCT DLLs are built with /MD (dynamic CRT) while libslic3r is built with /MT
// (static CRT). Loading both in the same process causes a DllMain deadlock.
// The cmake build marks OCCT DLLs as /DELAYLOAD, and this hook intercepts
// the first use of any OCCT symbol, returning a stub address so the DLLs
// are never actually loaded.

#include <windows.h>
#include <delayimp.h>

namespace {
    // Write diagnostic line using Win32 API directly (no CRT dependency).
    static HANDLE g_hLogFile = INVALID_HANDLE_VALUE;

    static void diagWrite(const char* msg)
    {
        if (g_hLogFile == INVALID_HANDLE_VALUE) {
            g_hLogFile = CreateFileA(
                "delayload_diag.log",
                GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (g_hLogFile == INVALID_HANDLE_VALUE) return;
        }
        DWORD len = 0;
        while (msg[len]) ++len;
        DWORD written = 0;
        WriteFile(g_hLogFile, msg, len, &written, nullptr);
    }

    static void diagFlush()
    {
        if (g_hLogFile != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(g_hLogFile);
            CloseHandle(g_hLogFile);
            g_hLogFile = INVALID_HANDLE_VALUE;
        }
    }

    // Suppress crash dialogs BEFORE libslic3r static initializers run.
    struct SuppressCrashDialogs {
        SuppressCrashDialogs() {
            SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
            SetUnhandledExceptionFilter(nullptr);
            diagWrite("INIT: SuppressCrashDialogs constructed\n");
            diagFlush();
        }
    };
    static SuppressCrashDialogs g_suppress;
}

static bool isDelayLoadedDll(const char* dllName)
{
    return dllName && (
        _strnicmp(dllName, "TK", 2) == 0 ||
        _strnicmp(dllName, "cr_tpms", 7) == 0
    );
}

// Generic stub: returns nullptr (0 in RAX).
static void* __cdecl occtStub()
{
    return nullptr;
}

static HMODULE s_fakeModule = reinterpret_cast<HMODULE>(static_cast<char*>(nullptr) + 1);

FARPROC WINAPI delayLoadHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
    if (pdli == nullptr)
        return nullptr;

    if (!isDelayLoadedDll(pdli->szDll))
        return nullptr;

    char buf[512];

    switch (dliNotify) {
    case dliNotePreLoadLibrary: {
        const char* tag = "HOOK: PreLoadLibrary ";
        diagWrite(tag);
        diagWrite(pdli->szDll);
        diagWrite(" -> FAKE\n");
        diagFlush();
        return reinterpret_cast<FARPROC>(s_fakeModule);
    }

    case dliNotePreGetProcAddress: {
        const char* fname = (pdli->dlp.fImportByName)
            ? pdli->dlp.szProcName
            : "(ordinal)";
        diagWrite("HOOK: GetProc ");
        diagWrite(pdli->szDll);
        diagWrite("!");
        diagWrite(fname);
        diagWrite(" -> stub\n");
        diagFlush();
        return reinterpret_cast<FARPROC>(&occtStub);
    }

    case dliFailLoadLib:
        return reinterpret_cast<FARPROC>(s_fakeModule);

    case dliFailGetProc: {
        const char* fname = (pdli->dlp.fImportByName)
            ? pdli->dlp.szProcName
            : "(ordinal)";
        diagWrite("HOOK: FailGetProc ");
        diagWrite(pdli->szDll);
        diagWrite("!");
        diagWrite(fname);
        diagWrite(" -> stub\n");
        diagFlush();
        return reinterpret_cast<FARPROC>(&occtStub);
    }
    }

    return nullptr;
}

extern "C" const PfnDliHook __pfnDliNotifyHook2 = delayLoadHook;
