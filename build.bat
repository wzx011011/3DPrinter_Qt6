@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build.ps1"
set "EXIT_CODE=%ERRORLEVEL%"

if not "%EXIT_CODE%"=="0" (
  echo Build failed with exit code %EXIT_CODE%
) else (
  echo Build succeeded.
)

endlocal & exit /b %EXIT_CODE%
