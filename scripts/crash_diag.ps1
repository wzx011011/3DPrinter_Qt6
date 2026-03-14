$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'

# Set up environment for cdb
$envDump = cmd /c ("`"" + $vcvars + "`" >nul & set") 2>&1
foreach ($line in $envDump) {
    $idx = $line.IndexOf('=')
    if ($idx -gt 0) {
        $name = $line.Substring(0, $idx)
        $value = $line.Substring($idx + 1)
        Set-Item -Path ("env:" + $name) -Value $value
    }
}

Set-Location 'E:\ai\3DPrinter_Qt6\build'

# Try to run the exe and capture exception info using cdb
$cdbOutput = cmd /c ("`"" + $vcvars + "`" >nul & cdb -c `"g -c `"l`"" E:\ai\3DPrinter_Qt6\build\FramelessDialogDemo.exe") 2>&1 | Out-String
Write-Host $cdbOutput

# Alternative: use a simple exception filter in a wrapper
Write-Host ""
Write-Host "=== Trying with exception filter ==="

$env:PATH = "E:\ai\3DPrinter_Qt6\build;" + $env:PATH

# Create a simple C program that loads and runs the exe with exception handling
$wrapperSrc = @'
#include <windows.h>
#include <stdio.h>
LONG WINAPI VectoredHandler(PEXCEPTION_POINTERS ep) {
    fprintf(stderr, "EXCEPTION: Code=0x%08X at %p\n",
        ep->ExceptionRecord->ExceptionCode,
        ep->ExceptionRecord->ExceptionAddress);
    return EXCEPTION_CONTINUE_SEARCH;
}
int main() {
    AddVectoredExceptionHandler(1, VectoredHandler);
    fprintf(stderr, "Before ShellExecute\n");
    fflush(stderr);
    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpFile = L"E:\\ai\\3DPrinter_Qt6\\build\\FramelessDialogDemo.exe";
    sei.nShow = SW_NORMAL;
    if (ShellExecuteExW(&sei)) {
        fprintf(stderr, "Started, waiting...\n");
        fflush(stderr);
        WaitForSingleObject(sei.hProcess, 8000);
        DWORD code;
        GetExitCodeProcess(sei.hProcess, $code);
        fprintf(stderr, "Exit code: %d\n", code);
    }
    return 0;
}
'@

$wrapperObj = 'E:\ai\3DPrinter_Qt6\build\crash_wrapper.obj'
$wrapperExe = 'E:\ai\3DPrinter_Qt6\build\crash_wrapper.exe'

$compileOutput = cmd /c ("`"" + $vcvars + "`" >nul & cl /Fe$wrapperExe /Fo$wrapperObj -DUNICODE -D_UNICODE /MD") 2>&1 | Out-String
Write-Host "Compile output:"
Write-Host $compileOutput

if (Test-Path $wrapperExe) {
    $proc = Start-Process $wrapperExe -PassThru -Wait
    Write-Host "Wrapper exit: $($proc.ExitCode)"
}
