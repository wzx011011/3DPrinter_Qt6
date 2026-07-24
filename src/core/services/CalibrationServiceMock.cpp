#include "CalibrationServiceMock.h"
#include "SliceService.h"
#include "ProjectServiceMock.h"
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <cmath>
#include <algorithm>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Calib.hpp>
#include <libslic3r/PrintConfig.hpp>
#endif

CalibrationServiceMock::CalibrationServiceMock(QObject *parent)
    : QObject(parent), m_timer(new QTimer(this))
{
    m_timer->setInterval(200);
    connect(m_timer, &QTimer::timeout, this, &CalibrationServiceMock::onTick);
    buildMockData();
}

CalibrationServiceMock::~CalibrationServiceMock() = default;

void CalibrationServiceMock::buildMockData()
{
    // Upstream CalibrationPanel uses CALI_MODE_COUNT = 2 tabs:
    //   PressureAdvanceWizard (Flow Dynamics / Calib_PA_Line)
    //   FlowRateWizard (Flow Rate / Calib_Flow_Rate)
    // Upstream CalibrationDialog provides hardware calibration:
    //   xcam_cali, bed_leveling, vibration, motor_noise
    // Additional CalibMode values from upstream calib.hpp:16-30:
    //   Temp_Tower=6, Vol_speed_Tower=7, VFA_Tower=8, Retraction_tower=9
    //
    // We expose 7 calibration types covering both slice and hardware domains:
    //   6 software-sliceable modes (PA=1, FlowRate=5, TempTower=6,
    //     Vol_speed=7, VFA=8, Retraction=9) dispatched via the generic
    //     calibMode!=0 path to SliceService -> libslic3r GCode branches
    //     (GCode.cpp:4608 TempTower / :4617 Vol_speed / :4612 VFA / :4622 Retraction).
    //   2 hardware modes (bed_leveling, vibration) kept unavailable.
    // Each type has 2-4 wizard steps aligned to upstream CalibrationWizard page chain:
    //   Start -> Preset -> Calibration -> [CoarseSave -> FineCalibration -> FineSave ->] Save
    //
    // Tech-debt: upstream CalibUtils loads dedicated test-tower models
    // (resources/calib/*.stl/.3mf/.step) and applies per-mode config overrides
    // (spiral_mode, wall_loops). Qt6 slices the current-plate geometry via
    // cloneCurrentPlateModel(). The GCode parameter sweep
    // (speed/temperature/retraction) works regardless of the tower shape; only
    // the precision geometry differs. Documented as deferred in 124-CONTEXT.md,
    // NOT a Phase 124 blocker. Phase 197 closes this gap for the four tower
    // modes (TempTower/Vol_speed/VFA/Retraction) by loading the bundled tower
    // model from qrc:/qml/assets/calib/ as an extra object on the current plate.

    CalibrationType flowDynamics;
    flowDynamics.id = "flow_dynamics";
    flowDynamics.name = tr("Flow Dynamics");
    flowDynamics.icon = "\u{1F4A8}";  // wind
    flowDynamics.category = "slice";
    flowDynamics.description = tr("Pressure Advance line calibration");
    flowDynamics.longDesc = tr(
        "Flow Dynamics Calibration measures the pressure advance parameters for your filament.\n\n"
        "You need this calibration when:\n"
        "1. Introducing a new filament of different brands/models or the filament is damp;\n"
        "2. The nozzle is worn out or replaced with a new one;\n"
        "3. The max volumetric speed or print temperature is changed.\n\n"
        "The calibration results have about 10 percent jitter, which may cause the result "
        "not exactly the same in each calibration.");
    flowDynamics.previewLabel = tr("Pressure Advance test pattern");
    flowDynamics.implemented = true;
    flowDynamics.startable = true;
    flowDynamics.calibMode = 1;
    flowDynamics.calibStart = 0.0;
    flowDynamics.calibEnd = 0.1;
    flowDynamics.calibStep = 0.002;
    flowDynamics.printNumbers = true;
    flowDynamics.steps = {
        {"start",   tr("Introduction"), tr("Learn when and why to perform Flow Dynamics calibration.")},
        {"preset",  tr("Select Filament"), tr("Choose the filament preset and nozzle diameter for calibration.")},
        {"cali",    tr("Calibrate"), tr("Send calibration job to printer. Wait for completion.")},
        {"save",    tr("Save Result"), tr("Review calibration result and save to preset.")}
    };

    CalibrationType flowRate;
    flowRate.id = "flow_rate";
    flowRate.name = tr("Flow Rate");
    flowRate.icon = "\u{1F4CF}";  // chart
    flowRate.category = "slice";
    flowRate.description = tr("Extrusion flow rate calibration");
    flowRate.longDesc = tr(
        "After using Flow Dynamics Calibration, there might still be some extrusion issues:\n"
        "1. Over-Extrusion: Excess material, blobs or zits, layers seem thicker than expected.\n"
        "2. Under-Extrusion: Very thin layers, weak infill strength, gaps in top layer.\n"
        "3. Poor Surface Quality: Surface seems rough or uneven.\n"
        "4. Weak Structural Integrity: Prints break easily.\n\n"
        "Flow Rate Calibration is crucial for foaming materials like LW-PLA.");
    flowRate.previewLabel = tr("Flow rate test pattern");
    flowRate.implemented = true;
    flowRate.startable = true;
    flowRate.calibMode = 5;
    flowRate.calibStart = 0.90;
    flowRate.calibEnd = 1.10;
    flowRate.calibStep = 0.01;
    flowRate.printNumbers = true;
    flowRate.steps = {
        {"start",        tr("Introduction"), tr("Learn when to use Flow Rate calibration.")},
        {"preset",       tr("Select Filament"), tr("Choose filament, bed type, and nozzle diameter.")},
        {"cali",         tr("Coarse Calibration"), tr("Run coarse flow rate calibration pass.")},
        {"coarse_save",  tr("Coarse Result"), tr("Review coarse calibration result.")},
        {"fine_cali",    tr("Fine Calibration"), tr("Run fine flow rate calibration pass.")},
        {"fine_save",    tr("Save Result"), tr("Review and save fine calibration result.")}
    };

    CalibrationType tempTower;
    tempTower.id = "temp_tower";
    tempTower.name = tr("Temp Tower");
    tempTower.icon = "T";
    tempTower.category = "slice";
    tempTower.description = tr("Temperature tower calibration");
    tempTower.longDesc = tr(
        "Temp Tower calibration prints a tower across a temperature range so you can "
        "choose a stable nozzle temperature for the selected filament.");
    tempTower.previewLabel = tr("Temperature tower test pattern");
    tempTower.implemented = true;
    tempTower.startable = true;
    tempTower.calibMode = 6;
    tempTower.calibStart = 190.0;
    tempTower.calibEnd = 240.0;
    tempTower.calibStep = 5.0;
    tempTower.printNumbers = true;
    tempTower.steps = {
        {"start",  tr("Introduction"), tr("Learn when to use Temp Tower calibration.")},
        {"preset", tr("Select Filament"), tr("Choose filament and temperature range.")},
        {"cali",   tr("Calibrate"), tr("Send temperature tower calibration to slicer.")},
        {"save",   tr("Save Result"), tr("Review selected temperature and save result.")}
    };

    CalibrationType bedLeveling;
    bedLeveling.id = "bed_leveling";
    bedLeveling.name = tr("Bed Leveling");
    bedLeveling.icon = "\u{1F3E0}";  // house
    bedLeveling.category = "hardware";
    bedLeveling.description = tr("Auto bed leveling calibration");
    bedLeveling.longDesc = tr(
        "Bed Leveling automatically detects and compensates for any unevenness "
        "in the build plate surface.\n\n"
        "The calibration program moves the nozzle across multiple points on the bed, "
        "measuring the distance at each point to create a height map.\n\n"
        "Ensure the build plate is clean and free of debris before starting.");
    bedLeveling.previewLabel = tr("Bed height map");
    bedLeveling.unavailableReason = tr("Blocked: requires live printer hardware calibration support.");
    bedLeveling.steps = {
        {"start",  tr("Introduction"), tr("Ensure build plate is clean and clear.")},
        {"cali",   tr("Leveling"), tr("Printer probes multiple points on the build plate.")},
        {"save",   tr("Complete"), tr("Review height map and confirm result.")}
    };

    CalibrationType vibration;
    vibration.id = "vibration";
    vibration.name = tr("Vibration Compensation");
    vibration.icon = "\u{1F4E2}";  // megaphone
    vibration.category = "hardware";
    vibration.description = tr("Input shaping / resonance test");
    vibration.longDesc = tr(
        "Vibration Compensation measures the resonant frequencies of your printer "
        "and generates input shaping parameters to reduce ringing and ghosting "
        "on printed surfaces.\n\n"
        "This calibration involves accelerating the print head at various frequencies "
        "to measure the structural response of the printer.");
    vibration.previewLabel = tr("Frequency response chart");
    vibration.unavailableReason = tr("Blocked: requires live printer resonance measurement support.");
    vibration.steps = {
        {"start",  tr("Introduction"), tr("Learn about vibration compensation calibration.")},
        {"cali",   tr("Measure"), tr("Printer performs resonance measurement.")},
        {"save",   tr("Apply Result"), tr("Review resonance data and apply compensation.")}
    };

    // Calib_Vol_speed_Tower = 7 (calib.hpp:24). libslic3r GCode.cpp:4617 sweeps
    // outer_wall_speed per mm of tower height using start/end/step. The generic
    // dispatch path (calibMode != 0 -> SliceService::setCalibParams) forwards
    // these transparently; no SliceService/Print/GCode change is needed.
    CalibrationType maxVolSpeed;
    maxVolSpeed.id = "max_volumetric_speed";
    maxVolSpeed.name = tr("Max Volumetric Speed");
    maxVolSpeed.icon = "\u{26A1}";  // lightning
    maxVolSpeed.category = "slice";
    maxVolSpeed.description = tr("Maximum volumetric speed test");
    maxVolSpeed.longDesc = tr(
        "Max Volumetric Speed calibration is recommended when printing with:\n"
        "- Materials with significant thermal shrinkage/expansion;\n"
        "- Materials with inaccurate filament diameter.\n\n"
        "Over-extrusion or under-extrusion at high speeds indicates the need "
        "for this calibration.");
    maxVolSpeed.previewLabel = tr("Speed tower test pattern");
    maxVolSpeed.implemented = true;
    maxVolSpeed.startable = true;
    maxVolSpeed.calibMode = 7;
    // Sweep outer_wall_speed across the tower (mm/s). Range aligned to the
    // volumetric-speed sweep upstream exposes for the speed tower.
    maxVolSpeed.calibStart = 5.0;
    maxVolSpeed.calibEnd = 30.0;
    maxVolSpeed.calibStep = 0.5;
    maxVolSpeed.printNumbers = true;
    maxVolSpeed.steps = {
        {"start",  tr("Introduction"), tr("Learn about max volumetric speed calibration.")},
        {"preset", tr("Select Parameters"), tr("Set calibration range parameters.")},
        {"cali",   tr("Calibrate"), tr("Send speed tower calibration to slicer.")},
        {"save",   tr("Save Result"), tr("Review result and save to filament preset.")}
    };

    // Calib_VFA_Tower = 8 (calib.hpp:25). libslic3r GCode.cpp:4612 sweeps
    // outer_wall_speed in 5mm height bands using start/end/step. Transparently
    // dispatched via the generic calibMode != 0 path.
    CalibrationType vfaTower;
    vfaTower.id = "vfa_tower";
    vfaTower.name = tr("VFA Tower");
    vfaTower.icon = "\u{1F4CA}";  // bar chart
    vfaTower.category = "slice";
    vfaTower.description = tr("Volumetric flow artifact (VFA) test");
    vfaTower.longDesc = tr(
        "VFA (Volumetric Flow Artifact) calibration prints a speed tower so you "
        "can identify the outer wall speeds at which surface artifacts (vertical "
        "fine lines / ringing) appear for the selected filament.\n\n"
        "Use the result to choose a maximum outer wall speed that minimizes VFAs "
        "while keeping print time reasonable.");
    vfaTower.previewLabel = tr("VFA speed tower test pattern");
    vfaTower.implemented = true;
    vfaTower.startable = true;
    vfaTower.calibMode = 8;
    // Sweep outer_wall_speed across the tower (mm/s). 5mm bands per step,
    // matching GCode.cpp:4613 (std::floor(print_z / 5.0) * step).
    vfaTower.calibStart = 10.0;
    vfaTower.calibEnd = 100.0;
    vfaTower.calibStep = 5.0;
    vfaTower.printNumbers = true;
    vfaTower.steps = {
        {"start",  tr("Introduction"), tr("Learn when to use VFA calibration.")},
        {"preset", tr("Select Parameters"), tr("Choose the speed sweep range.")},
        {"cali",   tr("Calibrate"), tr("Send VFA tower calibration to slicer.")},
        {"save",   tr("Save Result"), tr("Review artifact-free speed and save result.")}
    };

    // Calib_Retraction_tower = 9 (calib.hpp:26). libslic3r GCode.cpp:4622 sweeps
    // retraction_length per layer using start/end/step. Transparently dispatched
    // via the generic calibMode != 0 path.
    CalibrationType retractionTune;
    retractionTune.id = "retraction_tune";
    retractionTune.name = tr("Retraction Tune");
    retractionTune.icon = "\u{21BA}";  // anticlockwise arrow
    retractionTune.category = "slice";
    retractionTune.description = tr("Retraction length tower test");
    retractionTune.longDesc = tr(
        "Retraction Tune calibration prints a tower where each band uses a "
        "different retraction length, so you can pick the value that best "
        "eliminates stringing and blobs for your filament and hotend.\n\n"
        "Inspect the tower after printing and choose the cleanest band.");
    retractionTune.previewLabel = tr("Retraction tower test pattern");
    retractionTune.implemented = true;
    retractionTune.startable = true;
    retractionTune.calibMode = 9;
    // Sweep retraction_length across the tower (mm). GCode.cpp:4623 applies the
    // length per layer above 0.4mm (std::floor(max(0.0,print_z-0.4)) * step).
    retractionTune.calibStart = 0.0;
    retractionTune.calibEnd = 2.0;
    retractionTune.calibStep = 0.1;
    retractionTune.printNumbers = true;
    retractionTune.steps = {
        {"start",  tr("Introduction"), tr("Learn when to use retraction calibration.")},
        {"preset", tr("Select Parameters"), tr("Choose the retraction length range.")},
        {"cali",   tr("Calibrate"), tr("Send retraction tower calibration to slicer.")},
        {"save",   tr("Save Result"), tr("Review cleanest retraction length and save result.")}
    };

    m_calibTypes = {
        flowDynamics, flowRate, tempTower,
        bedLeveling, vibration,
        maxVolSpeed, vfaTower, retractionTune
    };

    // Initialize all statuses as NotStarted
    for (int i = 0; i < m_calibTypes.size(); ++i)
        m_statusMap[i] = CalibrationStatus::NotStarted;
}

// --- Type accessors ---

int CalibrationServiceMock::calibTypeCount() const { return m_calibTypes.size(); }

QString CalibrationServiceMock::calibTypeId(int index) const
{
    return (index >= 0 && index < m_calibTypes.size()) ? m_calibTypes[index].id : QString{};
}

QString CalibrationServiceMock::calibTypeName(int index) const
{
    return (index >= 0 && index < m_calibTypes.size()) ? m_calibTypes[index].name : QString{};
}

QString CalibrationServiceMock::calibTypeIcon(int index) const
{
    return (index >= 0 && index < m_calibTypes.size()) ? m_calibTypes[index].icon : QString{};
}

QString CalibrationServiceMock::calibTypeCategory(int index) const
{
    return (index >= 0 && index < m_calibTypes.size()) ? m_calibTypes[index].category : QString{};
}

QString CalibrationServiceMock::calibTypeDesc(int index) const
{
    return (index >= 0 && index < m_calibTypes.size()) ? m_calibTypes[index].description : QString{};
}

QString CalibrationServiceMock::calibTypeLongDesc(int index) const
{
    return (index >= 0 && index < m_calibTypes.size()) ? m_calibTypes[index].longDesc : QString{};
}

QString CalibrationServiceMock::calibTypePreviewLabel(int index) const
{
    return (index >= 0 && index < m_calibTypes.size()) ? m_calibTypes[index].previewLabel : QString{};
}

int CalibrationServiceMock::calibTypeIndexById(const QString &id) const
{
    for (int i = 0; i < m_calibTypes.size(); ++i) {
        if (m_calibTypes[i].id == id)
            return i;
    }
    return -1;
}

bool CalibrationServiceMock::calibTypeImplemented(int index) const
{
    return (index >= 0 && index < m_calibTypes.size()) ? m_calibTypes[index].implemented : false;
}

bool CalibrationServiceMock::calibTypeStartable(int index) const
{
    return (index >= 0 && index < m_calibTypes.size()) ? m_calibTypes[index].startable : false;
}

QString CalibrationServiceMock::calibTypeUnavailableReason(int index) const
{
    return (index >= 0 && index < m_calibTypes.size()) ? m_calibTypes[index].unavailableReason : QString{};
}

// Phase 125 (CALIB-02): per-mode default range readback + user override.
// Defaults are the Phase 124 hardcoded seeds (buildMockData); the user can
// override them via setCalibRange before startCalibration. The edited sweep
// then flows into SliceService::setCalibParams, replacing the hardcoded values.

double CalibrationServiceMock::calibTypeStart(int typeIndex) const
{
    return (typeIndex >= 0 && typeIndex < m_calibTypes.size()) ? m_calibTypes[typeIndex].calibStart : 0.0;
}

double CalibrationServiceMock::calibTypeEnd(int typeIndex) const
{
    return (typeIndex >= 0 && typeIndex < m_calibTypes.size()) ? m_calibTypes[typeIndex].calibEnd : 0.0;
}

double CalibrationServiceMock::calibTypeStep(int typeIndex) const
{
    return (typeIndex >= 0 && typeIndex < m_calibTypes.size()) ? m_calibTypes[typeIndex].calibStep : 0.0;
}

void CalibrationServiceMock::setCalibRange(int typeIndex, double start, double end, double step)
{
    if (typeIndex < 0 || typeIndex >= m_calibTypes.size()) return;
    // Reject non-finite overrides so a NaN never reaches the slice engine.
    if (!std::isfinite(start) || !std::isfinite(end) || !std::isfinite(step)) return;
    auto &calibType = m_calibTypes[typeIndex];
    calibType.calibStart = start;
    calibType.calibEnd = end;
    calibType.calibStep = step;
    qDebug("[Calib] range override type='%s' start=%.4f end=%.4f step=%.4f",
           calibType.id.toUtf8().constData(), start, end, step);
}

// --- Step accessors ---

int CalibrationServiceMock::stepCount(int typeIndex) const
{
    if (typeIndex < 0 || typeIndex >= m_calibTypes.size()) return 0;
    return m_calibTypes[typeIndex].steps.size();
}

QString CalibrationServiceMock::stepId(int typeIndex, int stepIndex) const
{
    if (typeIndex < 0 || typeIndex >= m_calibTypes.size()) return {};
    const auto &steps = m_calibTypes[typeIndex].steps;
    return (stepIndex >= 0 && stepIndex < steps.size()) ? steps[stepIndex].id : QString{};
}

QString CalibrationServiceMock::stepTitle(int typeIndex, int stepIndex) const
{
    if (typeIndex < 0 || typeIndex >= m_calibTypes.size()) return {};
    const auto &steps = m_calibTypes[typeIndex].steps;
    return (stepIndex >= 0 && stepIndex < steps.size()) ? steps[stepIndex].title : QString{};
}

QString CalibrationServiceMock::stepDesc(int typeIndex, int stepIndex) const
{
    if (typeIndex < 0 || typeIndex >= m_calibTypes.size()) return {};
    const auto &steps = m_calibTypes[typeIndex].steps;
    return (stepIndex >= 0 && stepIndex < steps.size()) ? steps[stepIndex].description : QString{};
}

int CalibrationServiceMock::stepState(int typeIndex, int stepIndex) const
{
    // Align upstream StepCtrl: 0=pending, 1=active, 2=completed
    if (typeIndex < 0 || typeIndex >= m_calibTypes.size()) return 0;
    if (stepIndex < 0) return 0;
    const auto &steps = m_calibTypes[typeIndex].steps;
    if (stepIndex >= steps.size()) return 0;

    // If the calibration type is completed, all steps are completed
    if (m_statusMap.value(typeIndex, CalibrationStatus::NotStarted) == CalibrationStatus::Completed)
        return 2;

    // Only the currently running calibration tracks step states
    if (m_isRunning && m_currentItem == typeIndex) {
        if (stepIndex < m_currentStepIndex) return 2;       // completed
        if (stepIndex == m_currentStepIndex) return 1;      // active
        return 0;                                           // pending
    }

    return 0; // pending for non-running calibrations
}

// --- Status ---

int CalibrationServiceMock::calibStatus(int typeIndex) const
{
    return (int)m_statusMap.value(typeIndex, CalibrationStatus::NotStarted);
}

void CalibrationServiceMock::setStatus(int typeIndex, CalibrationStatus status)
{
    if (!m_statusMap.contains(typeIndex)) return;
    if (m_statusMap[typeIndex] == status) return;
    m_statusMap[typeIndex] = status;
    emit statusChanged(typeIndex, (int)status);
}

// --- Actions ---

void CalibrationServiceMock::startCalibration(int itemIndex)
{
    if (m_isRunning) return;
    if (itemIndex < 0 || itemIndex >= m_calibTypes.size()) return;
    if (!m_calibTypes[itemIndex].startable) {
        qInfo("[Calib] '%s' is not startable: %s",
              m_calibTypes[itemIndex].id.toUtf8().constData(),
              m_calibTypes[itemIndex].unavailableReason.toUtf8().constData());
        return;
    }

    m_currentItem = itemIndex;
    m_progress = 0;
    m_currentStepIndex = 0;
    m_isRunning = true;

    setStatus(itemIndex, CalibrationStatus::InProgress);
    emit isRunningChanged();
    emit progressChanged();
    emit stepChanged();

#ifdef HAS_LIBSLIC3R
    bool dispatchedRealSlice = false;
    // v2.7 P1: real calibration via SliceService (path B, mirrors upstream CalibUtils::send_to_print).
    // Set Print.calib_params, run the full slice->export pipeline; GCode::do_export then takes the
    // Calib_PA_Line / Calib_Flow_Rate / Calib_Temp_Tower branch and auto-generates calib G-code.
    // We never construct CalibPressureAdvanceLine directly (its only construction site is inside
    // do_export, needing a live GCode engine).
    const auto &calibType = m_calibTypes[itemIndex];
    qDebug("[Calib] starting real calibration: %s", calibType.id.toUtf8().constData());

    if (m_sliceService) {
        const QString projectName = QStringLiteral("calib_%1").arg(calibType.id);
        if (calibType.calibMode != 0) {
            emit calibrationSliceRequested(calibType.calibMode, calibType.calibStart, calibType.calibEnd,
                                           calibType.calibStep, calibType.printNumbers, projectName);

            // Phase 197: for the four tower modes, load the dedicated upstream
            // tower model onto the current plate BEFORE slicing -- mirrors
            // upstream Plater::calib_temp/vol_speed/VFA/retraction which call
            // new_project()+add_model(<calib>/<tower>.stl). This replaces the
            // user's current-plate geometry with the precision tower; the
            // G-code parameter sweep (temp/speed/retraction injection in
            // GCode.cpp) is mode-driven and tower-shape-agnostic, so it is
            // unchanged. PA (1) and FlowRate (5) keep the current-plate model.
            // loadFile is async (QtConcurrent::run + loadFinished), so we defer
            // setCalibParams/startSlice to onCalibTowerLoadFinished. When
            // ProjectServiceMock is unavailable or extraction fails, fall back
            // to the legacy cloneCurrentPlateModel() geometry path.
            const QString towerQrc = towerModelQrcPathForMode(calibType.calibMode);
            if (m_projectService && !towerQrc.isEmpty()) {
                const QString towerTempPath = extractQrcToTempFile(towerQrc);
                if (!towerTempPath.isEmpty()) {
                    m_pendingCalibTowerLoad = true;
                    // Wire one-shot loadFinished -> setCalibParams + startSlice.
                    // The connection removes itself after firing so a later user
                    // loadFile is not intercepted.
                    connect(m_projectService, &ProjectServiceMock::loadFinished, this,
                            [this, mode = calibType.calibMode, start = calibType.calibStart,
                             end = calibType.calibEnd, step = calibType.calibStep,
                             printNumbers = calibType.printNumbers, projectName]
                            (bool ok, const QString & /*msg*/) {
                        if (!m_pendingCalibTowerLoad) return;
                        m_pendingCalibTowerLoad = false;
                        disconnect(m_projectService, &ProjectServiceMock::loadFinished, this, nullptr);
                        if (!ok || !m_sliceService) {
                            qWarning("[Calib] tower load failed or no SliceService - aborting slice");
                            if (m_currentItem >= 0)
                                setStatus(m_currentItem, CalibrationStatus::Failed);
                            m_isRunning = false;
                            emit isRunningChanged();
                            emit calibrationFinished(false);
                            return;
                        }
                        m_sliceService->setCalibParams(mode, start, end, step, printNumbers);
                        m_sliceService->startSlice(projectName);
                    });
                    m_projectService->loadFile(towerTempPath);
                    dispatchedRealSlice = true; // slice will start after loadFinished
                    qDebug("[Calib] tower model load requested: mode=%d qrc=%s",
                           calibType.calibMode, towerQrc.toUtf8().constData());
                } else {
                    qWarning("[Calib] tower extract failed - falling back to current plate; mode=%d",
                             calibType.calibMode);
                }
            }

            if (!dispatchedRealSlice) {
                // Legacy / fallback path: slice the current-plate geometry as-is.
                m_sliceService->setCalibParams(calibType.calibMode, calibType.calibStart, calibType.calibEnd,
                                               calibType.calibStep, calibType.printNumbers);
                m_sliceService->startSlice(projectName);
                dispatchedRealSlice = true;
                qDebug("[Calib] calib slice dispatched: mode=%d start=%.3f end=%.3f step=%.4f",
                       calibType.calibMode, calibType.calibStart, calibType.calibEnd, calibType.calibStep);
            }
        } else {
            qDebug("[Calib] type '%s' has no CalibMode mapping - mock only", calibType.id.toUtf8().constData());
        }
    } else {
        qDebug("[Calib] no SliceService - mock-only mode");
    }
#endif

    // v2.8 W7: only skip the mock timer when a real SliceService job was dispatched.
    // Unsupported calibration modes keep the fallback timer behavior.
#ifdef HAS_LIBSLIC3R
    if (!dispatchedRealSlice)
#else
    if (true)
#endif
    {
        m_timer->start();
    }
}

void CalibrationServiceMock::cancelCalibration()
{
    if (!m_isRunning) return;
    m_timer->stop();
    m_isRunning = false;

    // Phase 197: if a dedicated tower-model load is in flight, drop the
    // one-shot loadFinished connection so the deferred slice never starts.
    if (m_pendingCalibTowerLoad && m_projectService) {
        disconnect(m_projectService, &ProjectServiceMock::loadFinished, this, nullptr);
    }
    m_pendingCalibTowerLoad = false;

    // Revert to NotStarted on cancel
    if (m_currentItem >= 0)
        setStatus(m_currentItem, CalibrationStatus::NotStarted);

    m_progress = 0;
    m_currentStepIndex = -1;
    emit isRunningChanged();
    emit progressChanged();
    emit stepChanged();
}

void CalibrationServiceMock::goToStep(int stepIndex)
{
    if (!m_isRunning) return;
    if (m_currentItem < 0 || m_currentItem >= m_calibTypes.size()) return;

    int totalSteps = m_calibTypes[m_currentItem].steps.size();
    if (stepIndex < 0 || stepIndex >= totalSteps) return;

    m_currentStepIndex = stepIndex;
    // Recalculate progress based on step position
    m_progress = qMin(99, (int)((double)stepIndex / totalSteps * 100));
    emit progressChanged();
    emit stepChanged();
}

void CalibrationServiceMock::resetCalibration(int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= m_calibTypes.size()) return;
    if (m_isRunning && m_currentItem == itemIndex) {
        cancelCalibration();
    }
    setStatus(itemIndex, CalibrationStatus::NotStarted);
}

// --- History accessors ---

int CalibrationServiceMock::historyCount() const
{
    return m_history.size();
}

QString CalibrationServiceMock::historyName(int index) const
{
    return (index >= 0 && index < m_history.size()) ? m_history[index].name : QString{};
}

QString CalibrationServiceMock::historyFilamentId(int index) const
{
    return (index >= 0 && index < m_history.size()) ? m_history[index].filamentId : QString{};
}

float CalibrationServiceMock::historyKValue(int index) const
{
    return (index >= 0 && index < m_history.size()) ? m_history[index].kValue : 0.0f;
}

float CalibrationServiceMock::historyNozzleDiameter(int index) const
{
    return (index >= 0 && index < m_history.size()) ? m_history[index].nozzleDiameter : 0.4f;
}

QString CalibrationServiceMock::historyTimestamp(int index) const
{
    return (index >= 0 && index < m_history.size()) ? m_history[index].timestamp : QString{};
}

bool CalibrationServiceMock::historyHasRealReadback(int index) const
{
    return (index >= 0 && index < m_history.size()) ? m_history[index].hasRealReadback : false;
}

QString CalibrationServiceMock::historyNotes(int index) const
{
    return (index >= 0 && index < m_history.size()) ? m_history[index].notes : QString{};
}

void CalibrationServiceMock::addHistoryEntry(const QString &name, const QString &filamentId,
                                              float kValue, float nozzleDiameter, const QString &timestamp,
                                              bool hasRealReadback, const QString &notes)
{
    CalibrationHistoryEntry entry;
    entry.name = name;
    entry.filamentId = filamentId;
    entry.kValue = kValue;
    entry.nozzleDiameter = nozzleDiameter;
    entry.timestamp = timestamp;
    entry.hasRealReadback = hasRealReadback;
    entry.notes = notes;
    m_history.prepend(entry); // Most recent first
    emit historyChanged();
}

void CalibrationServiceMock::clearHistory()
{
    if (m_history.isEmpty()) return;
    m_history.clear();
    emit historyChanged();
}

// Phase 125 (CALIB-03): parse the last PA K-value the slice engine wrote into
// the generated G-code. Mirrors upstream GCodeWriter::set_pressure_advance
// (GCodeWriter.cpp:370-392) which emits one of:
//   Marlin/BBL   -> "M900 K<value> [L1000 M10 ] ; Override pressure advance"
//   Klipper      -> "SET_PRESSURE_ADVANCE ADVANCE=<value>"
//   RepRap       -> "M572 D0 S<value>"
//   Repetier     -> "M233 X<value> Y<value>"
// We scan the whole file and keep the LAST match (the engine writes the final
// PA value last, after any per-layer overrides). Returns false when no marker
// is present (the honest "no machine-readable readback" case for non-PA modes
// or a failed slice). Reference: upstream CalibUtils never reads the K back
// from G-code -- it stores the configured value -- so this is the OWzx-native
// readback path that closes the CALIB-03 mock gap.
bool CalibrationServiceMock::parsePressureAdvanceFromGcode(const QString &gcodePath, float &outK)
{
    if (gcodePath.isEmpty()) return false;
    QFile file(gcodePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("[Calib] PA readback: cannot open gcode %s",
                 gcodePath.toUtf8().constData());
        return false;
    }

    // Order matters: check the Klipper/RepRap/Repetier patterns BEFORE the
    // generic M900 K so a flavor that emits a different marker still matches.
    // Each regex is anchored to the start of a G-code line (after optional
    // whitespace) and requires the numeric value immediately after the prefix
    // (mirrors the exact string GCodeWriter writes).
    static const QRegularExpression patterns[] = {
        // Klipper: "SET_PRESSURE_ADVANCE ADVANCE=0.045"
        QRegularExpression(QStringLiteral("(?:^|\\n)\\s*SET_PRESSURE_ADVANCE\\s+ADVANCE=([0-9]+(?:\\.[0-9]+)?)")),
        // RepRap: "M572 D0 S0.045"
        QRegularExpression(QStringLiteral("(?:^|\\n)\\s*M572\\s+D0\\s+S([0-9]+(?:\\.[0-9]+)?)")),
        // Repetier: "M233 X0.045 Y..." (X is the K/linear component)
        QRegularExpression(QStringLiteral("(?:^|\\n)\\s*M233\\s+X([0-9]+(?:\\.[0-9]+)?)")),
        // Marlin/BBL: "M900 K0.045" (optionally followed by L1000 M10)
        QRegularExpression(QStringLiteral("(?:^|\\n)\\s*M900\\s+K([0-9]+(?:\\.[0-9]+)?)")),
    };

    bool found = false;
    float lastK = 0.0f;
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        for (const auto &re : patterns) {
            const QRegularExpressionMatch m = re.match(line);
            if (m.hasMatch()) {
                bool ok = false;
                const float v = m.captured(1).toFloat(&ok);
                if (ok && v >= 0.0f) {
                    lastK = v;
                    found = true;
                }
                break; // a line matches at most one flavor
            }
        }
    }

    if (found) {
        outK = lastK;
        qDebug("[Calib] PA readback: parsed K=%.4f from %s",
               outK, gcodePath.toUtf8().constData());
    } else {
        qDebug("[Calib] PA readback: no M900 K / SET_PRESSURE_ADVANCE marker in %s",
               gcodePath.toUtf8().constData());
    }
    return found;
}

// Phase 125 (CALIB-03): honest manual-interpretation guidance for the tower
// modes whose result is read from the physical print (band/layer inspection),
// not from a machine-readable marker. Upstream CalibUtils never auto-parses
// these outcomes either -- the user picks the best band by eye. We store this
// note in the history entry instead of a fabricated K so the UI can show
// "interpret the print manually" honestly.
QString CalibrationServiceMock::manualInterpretationNote(const QString &modeName)
{
    return QStringLiteral(
        "%1 result is read by inspecting the printed tower: choose the cleanest "
        "band/layer and apply that value to the filament preset. No K-value is "
        "auto-read back for this mode.").arg(modeName);
}

void CalibrationServiceMock::advanceStep()
{
    if (m_currentItem < 0 || m_currentItem >= m_calibTypes.size()) return;
    int totalSteps = m_calibTypes[m_currentItem].steps.size();
    if (m_currentStepIndex < totalSteps - 1) {
        m_currentStepIndex++;
        emit stepChanged();
    }
}

void CalibrationServiceMock::onTick()
{
    m_progress += 2;
    emit progressChanged();

    // Advance wizard step at key progress thresholds
    if (m_currentItem >= 0 && m_currentItem < m_calibTypes.size()) {
        int totalSteps = m_calibTypes[m_currentItem].steps.size();
        int newStep = qMin(totalSteps - 1, (int)((double)m_progress / 100.0 * totalSteps));
        if (newStep != m_currentStepIndex) {
            m_currentStepIndex = newStep;
            emit stepChanged();
        }
    }

    if (m_progress >= 100)
    {
        m_progress = 100;
        m_isRunning = false;
        m_timer->stop();

        if (m_currentItem >= 0) {
            // Set to the last step
            m_currentStepIndex = m_calibTypes[m_currentItem].steps.size() - 1;
            setStatus(m_currentItem, CalibrationStatus::Completed);
            emit stepChanged();

            // Phase 125 (CALIB-03): the mock-timer path has no real G-code to
            // parse (no SliceService dispatched), so we never fabricate a K.
            // Store the honest manual-interpretation note instead -- the user
            // reads the calibration print by eye (mirrors upstream CalibUtils,
            // which has no auto-readback for the timer-only fallback either).
            addHistoryEntry(
                m_calibTypes[m_currentItem].name,
                QString("filament_%1").arg(m_currentItem), // Mock filament ID
                0.0f,                                      // No fabricated K
                0.4f,                                      // Default nozzle diameter
                QDateTime::currentDateTime().toString(Qt::ISODate),
                false,                                     // No machine-readable readback
                manualInterpretationNote(m_calibTypes[m_currentItem].name)
            );
        }

        emit isRunningChanged();
        emit calibrationFinished(true);
    }
}

// v2.8 W7: receive real progress from SliceService
void CalibrationServiceMock::onSliceProgressUpdated(int percent, const QString &label)
{
    Q_UNUSED(label);
    if (!m_isRunning) return;
    m_progress = qBound(0, percent, 100);
    emit progressChanged();

    // Advance wizard step based on progress
    if (m_currentItem >= 0 && m_currentItem < m_calibTypes.size()) {
        int totalSteps = m_calibTypes[m_currentItem].steps.size();
        int newStep = qMin(totalSteps - 1, (int)((double)m_progress / 100.0 * totalSteps));
        if (newStep != m_currentStepIndex) {
            m_currentStepIndex = newStep;
            emit stepChanged();
        }
    }
}

// v2.8 W7: slice finished callback
void CalibrationServiceMock::onSliceFinished(const QString &estimatedTime)
{
    Q_UNUSED(estimatedTime);
    if (!m_isRunning) return;
    m_progress = 100;
    m_isRunning = false;
    m_timer->stop();

    if (m_currentItem >= 0) {
        m_currentStepIndex = m_calibTypes[m_currentItem].steps.size() - 1;
        setStatus(m_currentItem, CalibrationStatus::Completed);
        emit stepChanged();

        // Phase 125 (CALIB-03): real K-value readback replaces the mock
        // 0.04f + item*0.01. For PA (calibMode==1) the slice engine wrote an
        // M900 K / SET_PRESSURE_ADVANCE marker into the generated G-code; we
        // parse it now and store the REAL value with hasRealReadback=true.
        // For every other mode (FlowRate/TempTower/Vol_speed/VFA/Retraction)
        // the outcome is read from the physical print (band/layer inspection)
        // -- upstream CalibUtils never auto-parses those either -- so we store
        // the honest manual-interpretation note with kValue=0 and
        // hasRealReadback=false. No fabricated values.
        const int calibMode = m_calibTypes[m_currentItem].calibMode;
        const QString modeName = m_calibTypes[m_currentItem].name;
        float realK = 0.0f;
        bool parsed = false;
        if (calibMode == 1 /* Calib_PA_Line */ && m_sliceService) {
            const QString gcodePath = m_sliceService->outputPath();
            parsed = parsePressureAdvanceFromGcode(gcodePath, realK);
        }

        if (parsed) {
            addHistoryEntry(
                modeName,
                QString("filament_%1").arg(m_currentItem),
                realK,
                0.4f,
                QDateTime::currentDateTime().toString(Qt::ISODate),
                true,                                       // hasRealReadback
                QStringLiteral("PA K-value read back from sliced G-code (M900 K / SET_PRESSURE_ADVANCE).")
            );
        } else {
            // Honest path: no machine-readable marker (non-PA mode, or PA slice
            // that produced no marker). Document that the user interprets the
            // print manually; never store a fabricated K.
            addHistoryEntry(
                modeName,
                QString("filament_%1").arg(m_currentItem),
                0.0f,
                0.4f,
                QDateTime::currentDateTime().toString(Qt::ISODate),
                false,                                      // no machine-readable readback
                manualInterpretationNote(modeName)
            );
        }
    }

    emit isRunningChanged();
    emit calibrationFinished(true);
}

// v2.8 W7: slice failed callback
void CalibrationServiceMock::onSliceFailed(const QString &message)
{
    Q_UNUSED(message);
    if (!m_isRunning) return;
    m_isRunning = false;
    m_timer->stop();

    if (m_currentItem >= 0) {
        setStatus(m_currentItem, CalibrationStatus::Failed);
    }

    emit isRunningChanged();
    emit calibrationFinished(false);
}

void CalibrationServiceMock::setSliceService(SliceService *slice)
{
    if (m_sliceService == slice) return;
    if (m_sliceService) {
        disconnect(m_sliceService, nullptr, this, nullptr);
    }
    m_sliceService = slice;
    if (m_sliceService) {
        connect(m_sliceService, &SliceService::progressUpdated,
                this, &CalibrationServiceMock::onSliceProgressUpdated);
        connect(m_sliceService, &SliceService::sliceFinished,
                this, &CalibrationServiceMock::onSliceFinished);
        connect(m_sliceService, &SliceService::sliceFailed,
                this, &CalibrationServiceMock::onSliceFailed);
    }
}

void CalibrationServiceMock::setProjectService(ProjectServiceMock *project)
{
    // Phase 197: weak reference; ProjectServiceMock owns its own lifetime
    // (created in BackendContext). We only store the pointer for the tower
    // loadFile() path; no signal wiring is needed because loadFile emits its
    // own loadFinished and we do not block on it here.
    m_projectService = project;
}

// Phase 197: map a CalibMode to its bundled tower-model qrc path. Mirrors the
// per-mode add_model() call site in upstream Plater.cpp:
//   calib_temp            -> resources/calib/temperature_tower/temperature_tower.stl  (Plater.cpp:9804)
//   calib_max_vol_speed   -> resources/calib/volumetric_speed/SpeedTestStructure.step (Plater.cpp:9853)
//   calib_VFA             -> resources/calib/vfa/VFA.stl                              (Plater.cpp:9971)
//   calib_retraction      -> resources/calib/retraction/retraction_tower.stl         (Plater.cpp:9930)
// The bundled copies live under qrc:/qml/assets/calib/ (registered in
// qml.qrc). PA (mode 1) and FlowRate (mode 5) intentionally have no tower
// model here: upstream generates their geometry in-code (pa_pattern.3mf /
// flowrate-test-pass*.3mf are handled by separate wizard paths), so the Qt6
// calibration slice keeps using the current-plate geometry for those.
QString CalibrationServiceMock::towerModelQrcPathForMode(int calibMode)
{
    switch (calibMode) {
        case 6: return QStringLiteral(":/qml/assets/calib/temperature_tower.stl");
        case 7: return QStringLiteral(":/qml/assets/calib/SpeedTestStructure.step");
        case 8: return QStringLiteral(":/qml/assets/calib/VFA.stl");
        case 9: return QStringLiteral(":/qml/assets/calib/retraction_tower.stl");
        default: return QString{};
    }
}

// Phase 197: libslic3r's Model::read_from_file uses plain filesystem I/O
// (load_stl/load_step via boost::nowide), so it cannot read Qt's virtual qrc
// path directly. We materialize the bundled resource into a temp file with the
// correct extension (load_step keys off the .step suffix, Model.cpp:213-215)
// and hand back the filesystem path. QTemporaryFile::close() + autoRemove
// semantics would delete the file before read_from_file runs, so we set
// autoRemove=false and leave cleanup to the OS temp dir.
QString CalibrationServiceMock::extractQrcToTempFile(const QString &qrcPath)
{
    if (qrcPath.isEmpty()) return QString{};
    QFile in(qrcPath);
    if (!in.open(QIODevice::ReadOnly)) {
        qWarning("[Calib] tower extract: cannot open qrc %s",
                 qrcPath.toUtf8().constData());
        return QString{};
    }
    const QByteArray bytes = in.readAll();
    in.close();
    if (bytes.isEmpty()) {
        qWarning("[Calib] tower extract: empty qrc %s", qrcPath.toUtf8().constData());
        return QString{};
    }

    // Preserve the extension so load_step / load_stl dispatch correctly.
    const QString suffix = QFileInfo(qrcPath).suffix();
    QString templateName = QStringLiteral("calib_tower_XXXXXX");
    if (!suffix.isEmpty())
        templateName += QStringLiteral(".") + suffix;

    QTemporaryFile tmp(QDir::tempPath() + QDir::separator() + templateName);
    tmp.setAutoRemove(false);
    if (!tmp.open()) {
        qWarning("[Calib] tower extract: cannot create temp file");
        return QString{};
    }
    if (tmp.write(bytes) != bytes.size()) {
        qWarning("[Calib] tower extract: short write to %s",
                 tmp.fileName().toUtf8().constData());
        return QString{};
    }
    tmp.close();
    qDebug("[Calib] tower extract: %s -> %s (%lld bytes)",
           qrcPath.toUtf8().constData(),
           tmp.fileName().toUtf8().constData(),
           static_cast<qint64>(bytes.size()));
    return tmp.fileName();
}
