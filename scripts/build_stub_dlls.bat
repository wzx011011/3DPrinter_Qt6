@echo off
setlocal enabledelayedexpansion

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

set STUB_DIR=E:\ai\3DPrinter_Qt6\build\_stub_gen
set OUT_DIR=E:\ai\3DPrinter_Qt6\build

set OK=0
set FAIL=0

for %%D in (
  TKernel TKMath TKBRep TKGeomBase TKG2d TKG3d
  TKGeomAlgo TKTopAlgo TKShHealing TKMesh TKPrim
  TKCDF TKLCAF TKV3d TKService TKCAF TKVCAF
  TKXSBase TKSTEPBase TKSTEPAttr TKSTEP TKSTEP209
  TKXDESTEP TKXCAF TKBO TKHLR
) do (
  set DLL=%%D
  set DIR=!STUB_DIR!\%%D
  set DEF=!STUB_DIR!\%%D\%%D.def
  set OUT=!OUT_DIR!\%%D.dll

  if not exist "!DEF!" (
    echo SKIP %%D - no .def file
  ) else (
    echo Building %%D.dll ...
    pushd "!STUB_DIR!\%%D"
    cl /nologo /LD /MD /O2 /DNDEBUG stub.cpp /Fe:"!OUT_DIR!\%%D.dll" /link /DEF:%%D.def
    popd
    if exist "!OUT_DIR!\%%D.dll" (
      echo OK   %%D.dll
      set /a OK+=1
    ) else (
      echo FAIL %%D.dll
      set /a FAIL+=1
    )
  )
)

echo.
echo Result: !OK! OK, !FAIL! FAIL
endlocal
