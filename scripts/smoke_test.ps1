# smoke_test.ps1 — Automated smoke test for FramelessDialogDemo
# Validates: build success, app startup, 0 QML warnings, no crash within timeout
# Usage: powershell -ExecutionPolicy Bypass -File scripts/smoke_test.ps1
# Exit codes: 0=pass, 1=build fail, 2=app crash, 3=qml warnings, 4=missing binary

param(
    [int]$AppTimeout = 6,
    [string]$BuildDir = "E:\ai\3DPrinter_Qt6\build",
    [string]$SourceDir = "E:\ai\3DPrinter_Qt6"
)

$ErrorActionPreference = "Continue"
$passCount = 0
$failCount = 0
$warnings = @()

function Report-Test($name, $passed, $detail = "") {
    if ($passed) {
        Write-Host "  [PASS] $name" -ForegroundColor Green
        $script:passCount++
    } else {
        Write-Host "  [FAIL] $name $detail" -ForegroundColor Red
        $script:failCount++
    }
}

Write-Host "========================================"
Write-Host " Smoke Test — $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
Write-Host "========================================"
Write-Host ""

# --- Test 1: Binary exists ---
Write-Host "[Phase 1] Binary check"
$exe = Join-Path $BuildDir "FramelessDialogDemo.exe"
Report-Test "Executable exists" (Test-Path $exe) "(not found at $exe)"
if (-not (Test-Path $exe)) { exit 4 }

$fileSize = (Get-Item $exe).Length
Report-Test "Binary size > 100KB" ($fileSize -gt 100KB) "($fileSize bytes)"
Write-Host ""

# --- Test 2: Qt DLLs present ---
Write-Host "[Phase 2] Qt runtime dependencies"
$requiredDlls = @("Qt6Core.dll", "Qt6Gui.dll", "Qt6Qml.dll", "Qt6Quick.dll", "Qt6QuickControls2.dll")
foreach ($dll in $requiredDlls) {
    $path = Join-Path $BuildDir $dll
    Report-Test "$dll present" (Test-Path $path)
}
Write-Host ""

# --- Test 3: App startup (no crash within timeout) ---
Write-Host "[Phase 3] Application startup"
$diagLog = Join-Path $BuildDir "startup_diagnostics.log"
Remove-Item $diagLog -ErrorAction SilentlyContinue

$proc = Start-Process -FilePath $exe -WorkingDirectory $BuildDir -PassThru
Start-Sleep -Seconds $AppTimeout

if ($proc.HasExited) {
    $exitCode = $proc.ExitCode
    Report-Test "App stays alive for ${AppTimeout}s" $false "(exited with code $exitCode)"
} else {
    Report-Test "App stays alive for ${AppTimeout}s" $true "(PID $($proc.Id))"
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
}
Write-Host ""

# --- Test 4: No QML warnings in diagnostics log ---
Write-Host "[Phase 4] QML runtime warnings"
if (Test-Path $diagLog) {
    $logContent = Get-Content $diagLog -Raw
    $qmlWarns = [regex]::Matches($logContent, 'QML WARNING.*') | Measure-Object
    $qmlCount = $qmlWarns.Count
    Report-Test "0 QML warnings" ($qmlCount -eq 0) "($qmlCount warnings found)"

    $criticalErrors = [regex]::Matches($logContent, '(CRITICAL|fatal|segmentation|assert)') | Measure-Object
    Report-Test "No critical errors" ($criticalErrors.Count -eq 0) "($($criticalErrors.Count) found)"

    # Check for common upstream alignment issues
    $engineErrors = [regex]::Matches($logContent, 'QQmlApplicationEngine failed to load') | Measure-Object
    Report-Test "QQmlEngine loads successfully" ($engineErrors.Count -eq 0)

    # Show first few warnings if any
    if ($qmlCount -gt 0) {
        Write-Host "  Warning samples:" -ForegroundColor Yellow
        $matches = [regex]::Matches($logContent, 'QML WARNING[^\n]*')
        foreach ($m in $matches | Select-Object -First 5) {
            Write-Host "    $($m.Value)" -ForegroundColor Yellow
        }
    }
} else {
    Report-Test "Diagnostics log exists" $false "(startup_diagnostics.log not created)"
}
Write-Host ""

# --- Test 5: Verify key ViewModel files are compiled ---
Write-Host "[Phase 5] Build artifacts check"
$objFiles = @(
    "BackendContext.cpp.obj",
    "EditorViewModel.cpp.obj",
    "PreviewViewModel.cpp.obj",
    "ConfigViewModel.cpp.obj",
    "ProjectServiceMock.cpp.obj",
    "GLViewportRenderer.cpp.obj",
    "GCodeRenderer.cpp.obj",
    "MonitorViewModel.cpp.obj",
    "MultiMachineViewModel.cpp.obj",
    "CalibrationViewModel.cpp.obj",
    "DeviceServiceMock.cpp.obj",
    "CameraServiceMock.cpp.obj",
    "CalibrationServiceMock.cpp.obj",
    "SettingsViewModel.cpp.obj",
    "HomeViewModel.cpp.obj",
    "ModelMallViewModel.cpp.obj",
    "SliceService.cpp.obj",
    "ConfigOptionModel.cpp.obj",
    "CameraController.cpp.obj",
    "GLViewport.cpp.obj",
    "GLShaderUtil.cpp.obj",
    "UndoCommands.cpp.obj",
    "JobBase.cpp.obj",
    "JobManager.cpp.obj",
    "UndoRedoManager.cpp.obj"
)
foreach ($obj in $objFiles) {
    $found = Get-ChildItem -Path $BuildDir -Filter $obj -Recurse -ErrorAction SilentlyContinue
    Report-Test "$obj compiled" ($found.Count -gt 0)
}
Write-Host ""

# --- Test 6: QML resource integrity ---
Write-Host "[Phase 6] QML resource integrity"
$qmlFiles = @(
    "main.qml",
    "pages/PreparePage.qml",
    "pages/PreviewPage.qml",
    "pages/MonitorPage.qml",
    "pages/SettingsPage.qml",
    "pages/ConfigPage.qml",
    "pages/ModelMallPage.qml",
    "pages/CalibrationPage.qml",
    "pages/MultiMachinePage.qml",
    "pages/HomePage.qml",
    "pages/ProjectPage.qml",
    "components/StatsPanel.qml",
    "components/LayerSlider.qml",
    "components/MoveSlider.qml",
    "components/Legend.qml",
    "panels/PrintSettings.qml",
    "panels/ObjectList.qml",
    "panels/Sidebar.qml",
    "panels/SliceProgress.qml",
    "components/SearchDialog.qml",
    "components/NotificationCenter.qml",
    "components/ToolPositionTooltip.qml",
    "components/CollapsibleSection.qml",
    "pages/PreferencesPage.qml",
    "controls/CxButton.qml",
    "controls/CxComboBox.qml",
    "controls/CxSlider.qml",
    "controls/CxCheckBox.qml",
    "controls/CxIconButton.qml"
)
foreach ($qml in $qmlFiles) {
    $sourcePath = Join-Path (Join-Path $SourceDir "src\qml_gui") $qml
    Report-Test "$qml source exists" (Test-Path $sourcePath)
}
Write-Host ""

# --- Test 7: Standalone startup (no vcvars) ---
Write-Host "[Phase 7] Standalone startup (deployment check)"
$exe2 = Join-Path $BuildDir "FramelessDialogDemo.exe"
$standaloneLog = Join-Path $BuildDir "standalone_test.log"
Remove-Item $standaloneLog -ErrorAction SilentlyContinue
try {
    $proc2 = Start-Process -FilePath $exe2 -WorkingDirectory $BuildDir -PassThru -RedirectStandardError $standaloneLog -WindowStyle Hidden
    Start-Sleep -Seconds $AppTimeout
    if ($proc2.HasExited) {
        $exitCode2 = $proc2.ExitCode
        # 0xC0000135 = DLL_NOT_FOUND
        if ($exitCode2 -eq -1073741515) {
            Report-Test "Standalone DLL dependencies" $false "(0xC0000135 DLL_NOT_FOUND — missing runtime DLLs in build dir)"
        } else {
            Report-Test "Standalone startup" $false "(exited with code $exitCode2)"
        }
    } else {
        Report-Test "Standalone startup (no vcvars)" $true "(PID $($proc2.Id), app survives ${AppTimeout}s"
        Stop-Process -Id $proc2.Id -Force -ErrorAction SilentlyContinue
    }
    # Show DLL diagnostics if failed
    if (Test-Path $standaloneLog) {
        $logContent2 = Get-Content $standaloneLog -Raw -ErrorAction SilentlyContinue
        if ($logContent2) {
            $missingDlls = [regex]::Matches($logContent2, 'error while loading shared libraries: (.+?\.dll)') | Measure-Object
            if ($missingDlls.Count -gt 0) {
                Write-Host "  Missing DLLs:" -ForegroundColor Yellow
                foreach ($m in $missingDlls | Select-Object -First 5) {
                    Write-Host "    $($m.Value)" -ForegroundColor Yellow
                }
            }
        }
    }
} catch {
    Report-Test "Standalone startup (no vcvars)" $false "(exception: $_)"
}
Write-Host ""

# --- Summary ---
Write-Host "========================================"
Write-Host " Results: $passCount passed, $failCount failed"
Write-Host "========================================"

if ($failCount -gt 0) {
    exit 2
}
exit 0
