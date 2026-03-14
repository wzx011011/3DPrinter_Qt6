@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

cd /d E:\ai\3DPrinter_Qt6\build

REM sxe av: break on access violation as second-chance
REM g: continue past initial breakpoint
REM .ecxr: show exception context
REM kL 30: show stack trace
REM q: quit
"C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe" -c "sxe av;g;.ecxr;kL 30;q" E:\ai\3DPrinter_Qt6\build\FramelessDialogDemo.exe >cdb_output4.txt 2>&1

type cdb_output4.txt
