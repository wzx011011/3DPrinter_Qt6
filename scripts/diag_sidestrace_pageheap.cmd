@rem ============================================================================
@rem AI-SIDECAR-STARTUP-RACE evidence collector (RUN AS ADMINISTRATOR).
@rem
@rem Reproduces the SIDECAR-RACE startup crash (exit 0xC0000374 / AV) with
@rem full PageHeap enabled so the corrupting write faults at the culprit
@rem access, then captures:
@rem   1. the crash handler's module-name stack (crash_stack.log entry)
@rem   2. a full minidump (crash_dumps\\*.dmp)
@rem   3. the cdb session log with the PageHeap AV stack (cdb_pageheap.log)
@rem
@rem Usage (elevated cmd):
@rem   scripts\\diag_sidestrace_pageheap.cmd
@rem Output: build\\pageheap_evidence\\  (log + dump + cdb trace)
@rem
@rem After the run, hand pageheap_evidence back to the session for culprit
@rem analysis; disable PageHeap with:
@rem   "gflags /p /disable OWzxSlicer.exe"
@rem ============================================================================
@echo off
setlocal
set REPO=E:\ai\3DPrinter_Qt6
set GFLAGS="C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\gflags.exe"
set CDB="C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe"
set EVI=%REPO%\build\pageheap_evidence

if not exist %GFLAGS% set GFLAGS="C:\Program Files\Windows Kits\10\Debuggers\x64\gflags.exe"
if not exist %CDB% set CDB="C:\Program Files\Windows Kits\10\Debuggers\x64\cdb.exe"

echo [1/4] Enabling full PageHeap for OWzxSlicer.exe (requires admin)...
%GFLAGS% /p /enable OWzxSlicer.exe /full
if errorlevel 1 (
  echo [!] gflags failed -- is this shell elevated?
  %GFLAGS% /p
  exit /b 1
)

echo [2/4] Reproducing with the model load...
cd /d %REPO%\build
set PATH=%REPO%\build;%PATH%
%CD% 1>nul 2>nul
start "" /wait OWzxSlicer.exe --load-model %REPO%\third_party\OrcaSlicer\tests\data\test_3mf\Prusa.stl --open-page prepare --skip-first-run

echo [3/4] Disabling PageHeap...
%GFLAGS% /p /disable OWzxSlicer.exe

echo [4/4] Collecting evidence...
mkdir %EVI% 2>nul
copy /y crash_dumps\crash_stack.log %EVI%\ 1>nul 2>nul
for /f "delims=" %%f in ('dir /b /od crash_dumps\*.dmp') do set NEWDMP=%%f
copy /y crash_dumps\%NEWDMP% %EVI%\ 1>nul 2>nul
echo Evidence in %EVI%\(crash_stack.log last entry + %NEWDMP%)
echo Done. Hand the folder back to the session for culprit analysis.
