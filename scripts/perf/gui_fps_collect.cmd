@echo off
rem Wave-final GUI FPS collector (docs/perf-baseline.md runbook, one double-click).
rem Steps after the app opens:
rem   1. Press Ctrl+Shift+Space in the 3D view  -> bottom-left "FPS: N" overlay
rem   2. Orbit/zoom with the mouse ~15 s, note the FPS number
rem   3. Close the app, then send back:
rem        build\perf_lines_gui.txt  (render.frame buckets)
rem        + the overlay FPS you saw
rem   4. Open the SAME model in OrcaSlicer, Ctrl+Shift+Space there,
rem      orbit the same way, note its FPS panel number.
rem (Remote-desktop note: run this LOCALLY at the machine — through a
rem  ToDesk/remote mirror the window never repaints and both counters stay 0.)
set OWZX_PERF_LOG=1
set OWZX_PERF_LOG_FILE=E:\ai\3DPrinter_Qt6\build\perf_lines_gui.txt
set OWZX_FORCE_RHI=1
set PATH=E:\Qt6.10\bin;%PATH%
cd /d E:\ai\3DPrinter_Qt6\build
del perf_lines_gui.txt 2>nul
start "" OWzxSlicer.exe --skip-first-run --load-model E:/ai/3DPrinter_Qt6/build/perf_models/bench_sphere_2m.stl
echo App launching with the 2M-triangle benchmark model.
echo Press Ctrl+Shift+Space in the 3D view, orbit ~15 s, read the FPS overlay.
echo Log file: build\perf_lines_gui.txt
pause
