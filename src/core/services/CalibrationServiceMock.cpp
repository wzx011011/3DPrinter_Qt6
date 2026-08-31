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
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>
#include <algorithm>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Calib.hpp>
#include <libslic3r/PrintConfig.hpp>

// v5.16 (CIRC-02): upstream deleted Calib_Auto_PA_Line (cb35e89f) and the
// CalibMode enum shifted down by one. The previously hardcoded ints made every
// tower mode sweep the wrong axis (FlowRate=5 executed Temp_Tower, ..., 
// Retraction=9 matched nothing). Modes are now pinned by symbol only.
namespace {
constexpr int kCalibModePA_Line    = static_cast<int>(Slic3r::CalibMode::Calib_PA_Line);
constexpr int kCalibModePA_Pattern = static_cast<int>(Slic3r::CalibMode::Calib_PA_Pattern);
constexpr int kCalibModePA_Tower   = static_cast<int>(Slic3r::CalibMode::Calib_PA_Tower);
constexpr int kCalibModeFlowRate   = static_cast<int>(Slic3r::CalibMode::Calib_Flow_Rate);
constexpr int kCalibModeTempTower  = static_cast<int>(Slic3r::CalibMode::Calib_Temp_Tower);
constexpr int kCalibModeVolSpeed   = static_cast<int>(Slic3r::CalibMode::Calib_Vol_speed_Tower);
constexpr int kCalibModeVFA        = static_cast<int>(Slic3r::CalibMode::Calib_VFA_Tower);
constexpr int kCalibModeRetract    = static_cast<int>(Slic3r::CalibMode::Calib_Retraction_tower);
} // namespace
#else
// Non-HAS builds never reach the slicing engine; mirror the current enum.
namespace {
constexpr int kCalibModePA_Line    = 1;
constexpr int kCalibModePA_Pattern = 2;
constexpr int kCalibModePA_Tower   = 3;
constexpr int kCalibModeFlowRate   = 4;
constexpr int kCalibModeTempTower  = 5;
constexpr int kCalibModeVolSpeed   = 6;
constexpr int kCalibModeVFA        = 7;
constexpr int kCalibModeRetract    = 8;
} // namespace
#endif

CalibrationServiceMock::CalibrationServiceMock(QObject *parent)
    : QObject(parent), m_timer(new QTimer(this))
{
    m_timer->setInterval(200);
    connect(m_timer, &QTimer::timeout, this, &CalibrationServiceMock::onTick);
    buildMockData();
    // Phase 241 (PAGE-03): restore persisted calibration history so the
    // history dialog survives restarts.
    loadHistoryFromDisk();
}

CalibrationServiceMock::~CalibrationServiceMock() = default;

void CalibrationServiceMock::buildMockData()
{
    // Upstream CalibrationPanel uses CALI_MODE_COUNT = 2 tabs:
    //   PressureAdvanceWizard (Flow Dynamics / Calib_PA_Line)
    //   FlowRateWizard (Flow Rate / Calib_Flow_Rate)
    // Upstream CalibrationDialog provides hardware calibration:
    //   xcam_cali, bed_leveling, vibration, motor_noise
    // Upstream calib.hpp values are pinned symbolically at the top of this
    // file (kCalibMode*). v5.16 (CIRC-02): upstream deleted Calib_Auto_PA_Line
    // so the enum shifted; never restore the old ints (FlowRate was wrongly 5,
    // Retraction wrongly 9, etc.).
    //
    // We expose 7 calibration types covering both slice and hardware domains:
    //   6 software-sliceable modes (PA_Line, Flow_Rate, Temp_Tower,
    //     Vol_speed_Tower, VFA_Tower, Retraction_tower) dispatched via the
    //     generic calibMode!=0 path to SliceService -> libslic3r GCode branches.
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
    flowDynamics.calibMode = kCalibModePA_Line;
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

    // Phase 241 (PAGE-03): PA Pattern — upstream Plater::calib_pa routes
    // Calib_PA_Pattern to _calib_pa_pattern (Plater.cpp:9408), which builds
    // the pattern in-code via CalibPressureAdvancePattern (handle cube +
    // per-layer custom G-code) and applies the SuggestedConfigCalibPAPattern
    // overrides. OWzx mirrors the generation + dispatch with default config
    // values (documented delta: preset-derived accel/jerk normalization is
    // upstream-only; the sweep geometry itself is engine-generated).
    CalibrationType paPattern;
    paPattern.id = "pa_pattern";
    paPattern.name = tr("PA Pattern");
    paPattern.icon = "\u{1F4CF}";
    paPattern.category = "slice";
    paPattern.description = tr("Pressure Advance pattern calibration");
    paPattern.longDesc = tr(
        "PA Pattern generates the upstream OrcaSlicer pressure-advance test "
        "pattern in-code: an anchoring frame plus pattern rows whose "
        "pressure advance sweeps from start to end. Read the cleanest row "
        "off the print and save it to the filament preset.");
    paPattern.previewLabel = tr("Pressure Advance pattern (in-code)");
    paPattern.implemented = true;
    paPattern.startable = true;
    paPattern.calibMode = kCalibModePA_Pattern;
    paPattern.calibStart = 0.0;
    paPattern.calibEnd = 0.08;
    paPattern.calibStep = 0.005;
    paPattern.printNumbers = true;
    paPattern.steps = {
        {"start",  tr("Introduction"), tr("Learn when to use the PA pattern calibration.")},
        {"preset", tr("Select Filament"), tr("Choose filament and nozzle diameter.")},
        {"cali",   tr("Calibrate"), tr("Generate the PA pattern and slice it.")},
        {"save",   tr("Save Result"), tr("Pick the cleanest row and save its PA value to the preset.")}
    };

    // Phase 241 (PAGE-03): PA Tower — upstream _calib_pa_tower
    // (Plater.cpp:9584) adds resources/calib/pressure_advance/
    // tower_with_seam.stl; the engine sweeps PA per layer
    // (GCode.cpp:3721: start + int(print_z) * step).
    CalibrationType paTower;
    paTower.id = "pa_tower";
    paTower.name = tr("PA Tower");
    paTower.icon = "\u{1F5FC}";
    paTower.category = "slice";
    paTower.description = tr("Pressure Advance tower calibration");
    paTower.longDesc = tr(
        "PA Tower prints the upstream tower-with-seam model; every millimeter "
        "of height applies a new pressure advance value (start + z * step). "
        "Find the height band with the most consistent seam and save its PA "
        "value to the filament preset.");
    paTower.previewLabel = tr("Pressure Advance tower");
    paTower.implemented = true;
    paTower.startable = true;
    paTower.calibMode = kCalibModePA_Tower;
    paTower.calibStart = 0.0;
    paTower.calibEnd = 0.06;
    paTower.calibStep = 0.005;
    paTower.printNumbers = false;
    paTower.steps = {
        {"start",  tr("Introduction"), tr("Learn when to use the PA tower calibration.")},
        {"preset", tr("Select Filament"), tr("Choose filament and the PA sweep range.")},
        {"cali",   tr("Calibrate"), tr("Load the tower model and slice it.")},
        {"save",   tr("Save Result"), tr("Read the best height band and save its PA value.")}
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
    flowRate.calibMode = kCalibModeFlowRate;
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
    tempTower.calibMode = kCalibModeTempTower;
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
    maxVolSpeed.calibMode = kCalibModeVolSpeed;
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
    vfaTower.calibMode = kCalibModeVFA;
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
    retractionTune.calibMode = kCalibModeRetract;
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
        flowDynamics, paPattern, paTower, flowRate, tempTower,
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

int CalibrationServiceMock::calibTypeMode(int index) const
{
    if (index < 0 || index >= m_calibTypes.size())
        return -1;
    return m_calibTypes[index].calibMode;
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

    const int mode = m_calibTypes[itemIndex].calibMode;
    const bool needsIsolatedGeometry = mode == kCalibModePA_Pattern
        || !towerModelQrcPathForMode(mode, m_flowRatePass).isEmpty();
    if (needsIsolatedGeometry && m_projectService && m_projectService->modelCount() > 0) {
        // Upstream starts these jobs in a new calibration project. Until OWzx
        // owns an isolated model context, fail closed instead of replacing or
        // mutating the user's live workspace.
        qWarning("[Calib] calibration requires an empty workspace; live project preserved");
        emit calibrationFinished(false);
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

            // Phase 241 (PAGE-03): PA Pattern bypasses the tower-model path —
            // its geometry + sweep come from in-code generation
            // (CalibPressureAdvancePattern), mirroring upstream
            // Plater::_calib_pa_pattern (Plater.cpp:9418-9563).
            if (calibType.calibMode == kCalibModePA_Pattern) {
                if (generateAndDispatchPaPattern(projectName)) {
                    dispatchedRealSlice = true;
                } else {
                    qWarning("[Calib] PA pattern generation failed - aborting");
                    setStatus(itemIndex, CalibrationStatus::Failed);
                    m_isRunning = false;
                    emit isRunningChanged();
                    emit calibrationFinished(false);
                    return;
                }
            }

            // Phase 197: for the tower modes, load the dedicated upstream
            // tower model onto the current plate BEFORE slicing -- mirrors
            // upstream Plater::calib_temp/vol_speed/VFA/retraction which call
            // new_project()+add_model(<calib>/<tower>.stl). This replaces the
            // user's current-plate geometry with the precision tower; the
            // G-code parameter sweep (temp/speed/retraction injection in
            // GCode.cpp) is mode-driven and tower-shape-agnostic, so it is
            // unchanged. PA_Line (engine-drawn lines, GCode.cpp:2451) keeps
            // the current-plate model. loadFile is async
            // (QtConcurrent::run + loadFinished), so we defer
            // setCalibParams/startSlice to onCalibTowerLoadFinished. When
            // ProjectServiceMock is unavailable or extraction fails, fall
            // back to the legacy cloneCurrentPlateModel() geometry path.
            const QString towerQrc = towerModelQrcPathForMode(calibType.calibMode, m_flowRatePass);
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

float CalibrationServiceMock::historyFlowRate(int index) const
{
    return (index >= 0 && index < m_history.size()) ? m_history[index].flowRate : 0.0f;
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
                                              bool hasRealReadback, const QString &notes, float flowRate)
{
    CalibrationHistoryEntry entry;
    entry.name = name;
    entry.filamentId = filamentId;
    entry.kValue = kValue;
    entry.flowRate = flowRate;
    entry.nozzleDiameter = nozzleDiameter;
    entry.timestamp = timestamp;
    entry.hasRealReadback = hasRealReadback;
    entry.notes = notes;
    m_history.prepend(entry); // Most recent first
    // Phase 241 (PAGE-03): history survives restarts (JSON in AppData).
    persistHistoryToDisk();
    emit historyChanged();
}

void CalibrationServiceMock::clearHistory()
{
    if (m_history.isEmpty()) return;
    m_history.clear();
    persistHistoryToDisk();
    emit historyChanged();
}

// Phase 241 (PAGE-03): calibration history persistence. Upstream keeps the
// calibration history inside the printer's device profile
// (CaliHistoryPane/FlowCalibHeaderView read MachineObject::cali_history);
// the OWzx device channel is out of scope, so the history lands in a local
// JSON file under AppDataLocation instead. Same honest data: name,
// filament id, K value, nozzle diameter, timestamp, readback flag, notes.
QString CalibrationServiceMock::historyFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/calibration_history.json");
}

void CalibrationServiceMock::loadHistoryFromDisk()
{
    QFile file(historyFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray())
        return;
    m_history.clear();
    const QJsonArray array = doc.array();
    for (const QJsonValue &value : array) {
        const QJsonObject obj = value.toObject();
        CalibrationHistoryEntry entry;
        entry.name = obj.value(QStringLiteral("name")).toString();
        entry.filamentId = obj.value(QStringLiteral("filamentId")).toString();
        entry.kValue = float(obj.value(QStringLiteral("kValue")).toDouble());
        entry.flowRate = float(obj.value(QStringLiteral("flowRate")).toDouble());
        entry.nozzleDiameter = float(obj.value(QStringLiteral("nozzleDiameter")).toDouble());
        entry.timestamp = obj.value(QStringLiteral("timestamp")).toString();
        entry.hasRealReadback = obj.value(QStringLiteral("hasRealReadback")).toBool(false);
        entry.notes = obj.value(QStringLiteral("notes")).toString();
        if (!entry.name.isEmpty())
            m_history.append(entry);
    }
    std::stable_sort(m_history.begin(), m_history.end(),
                     [](const CalibrationHistoryEntry &a, const CalibrationHistoryEntry &b) {
        return QDateTime::fromString(a.timestamp, Qt::ISODate)
             > QDateTime::fromString(b.timestamp, Qt::ISODate);
    });
    if (!m_history.isEmpty())
        emit historyChanged();
}

void CalibrationServiceMock::persistHistoryToDisk()
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QJsonArray array;
    // Persist the public newest-first order directly.
    for (const auto &e : m_history) {
        QJsonObject obj;
        obj.insert(QStringLiteral("name"), e.name);
        obj.insert(QStringLiteral("filamentId"), e.filamentId);
        obj.insert(QStringLiteral("kValue"), double(e.kValue));
        obj.insert(QStringLiteral("flowRate"), double(e.flowRate));
        obj.insert(QStringLiteral("nozzleDiameter"), double(e.nozzleDiameter));
        obj.insert(QStringLiteral("timestamp"), e.timestamp);
        obj.insert(QStringLiteral("hasRealReadback"), e.hasRealReadback);
        obj.insert(QStringLiteral("notes"), e.notes);
        array.append(obj);
    }
    QFile file(historyFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("[Calib] cannot write history file: %s",
                 historyFilePath().toUtf8().constData());
        return;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Compact));
    file.close();
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
            const int mode = m_calibTypes[m_currentItem].calibMode;
            if (mode != kCalibModePA_Pattern && mode != kCalibModePA_Tower) {
                addHistoryEntry(
                    m_calibTypes[m_currentItem].name,
                    QString("filament_%1").arg(m_currentItem),
                    0.0f,
                    0.4f,
                    QDateTime::currentDateTime().toString(Qt::ISODate),
                    false,
                    manualInterpretationNote(m_calibTypes[m_currentItem].name)
                );
            }
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

        // Phase 125 (CALIB-03) + Phase 241 (PAGE-03): real K-value readback
        // replaces the mock 0.04f + item*0.01. For the PA modes (PA_Line,
        // PA_Pattern, PA_Tower) the slice engine wrote an M900 K /
        // SET_PRESSURE_ADVANCE marker into the generated G-code; we parse it
        // now and store the REAL value with hasRealReadback=true. For every
        // other mode (FlowRate/TempTower/Vol_speed/VFA/Retraction) the
        // outcome is read from the physical print (band/layer inspection)
        // -- upstream CalibUtils never auto-parses those either -- so we
        // store the honest manual-interpretation note with kValue=0 and
        // hasRealReadback=false. No fabricated values.
        const int calibMode = m_calibTypes[m_currentItem].calibMode;
        const QString modeName = m_calibTypes[m_currentItem].name;
        float realK = 0.0f;
        bool parsed = false;
        if ((calibMode == kCalibModePA_Line || calibMode == kCalibModePA_Pattern
             || calibMode == kCalibModePA_Tower)
            && m_sliceService) {
            const QString gcodePath = m_sliceService->outputPath();
            parsed = parsePressureAdvanceFromGcode(gcodePath, realK);
        }

        const bool requiresUserSelection = calibMode == kCalibModePA_Pattern
            || calibMode == kCalibModePA_Tower;
        if (requiresUserSelection) {
            // Upstream saves Pattern/Tower only after the user selects the best
            // row or height. The sweep endpoint is not a measured result.
        } else if (parsed) {
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

// Phase 197 + Phase 241 (PAGE-03): map a CalibMode to its bundled tower-model
// qrc path. Mirrors the per-mode add_model() call site in upstream Plater.cpp:
//   calib_temp            -> resources/calib/temperature_tower/temperature_tower.stl  (Plater.cpp:9804)
//   calib_max_vol_speed   -> resources/calib/volumetric_speed/SpeedTestStructure.step (Plater.cpp:9853)
//   calib_VFA             -> resources/calib/vfa/VFA.stl                              (Plater.cpp:9971)
//   calib_retraction      -> resources/calib/retraction/retraction_tower.stl         (Plater.cpp:9930)
//   _calib_pa_tower       -> resources/calib/pressure_advance/tower_with_seam.stl    (Plater.cpp:9585)
//   calib_flow_rate       -> resources/calib/filament_flow/flowrate-test-passN.3mf   (Plater.cpp:9784-9791)
// The bundled copies live under qrc:/qml/assets/calib/ (registered in
// qml.qrc). PA_Line (engine-drawn lines) and PA_Pattern (in-code generated)
// intentionally have no tower model here: their geometry is generated by the
// slicing engine / CalibPressureAdvancePattern, so those modes keep using the
// current-plate geometry / the generation path.
QString CalibrationServiceMock::towerModelQrcPathForMode(int calibMode, int flowRatePass)
{
    switch (calibMode) {
        // v5.16 (CIRC-02): symbolic cases — the old ints 6/7/8/9 predate the
        // upstream enum shift and no longer match the dispatched modes.
        case kCalibModeTempTower: return QStringLiteral(":/qml/assets/calib/temperature_tower.stl");
        case kCalibModeVolSpeed: return QStringLiteral(":/qml/assets/calib/SpeedTestStructure.step");
        case kCalibModeVFA: return QStringLiteral(":/qml/assets/calib/VFA.stl");
        case kCalibModeRetract: return QStringLiteral(":/qml/assets/calib/retraction_tower.stl");
        // Phase 241 (PAGE-03): PA Tower uses the upstream tower-with-seam
        // model; the engine sweeps PA per layer (GCode.cpp:3721).
        case kCalibModePA_Tower: return QStringLiteral(":/qml/assets/calib/tower_with_seam.stl");
        // Phase 241 (PAGE-03): Flow Rate two-pass flow — pass1 coarse /
        // pass2 fine (upstream Plater.cpp:9784-9791 loads
        // flowrate-test-pass1.3mf / flowrate-test-pass2.3mf).
        case kCalibModeFlowRate:
            return flowRatePass == 2
                       ? QStringLiteral(":/qml/assets/calib/flowrate-test-pass2.3mf")
                       : QStringLiteral(":/qml/assets/calib/flowrate-test-pass1.3mf");
        default: return QString{};
    }
}

void CalibrationServiceMock::setFlowRatePass(int pass)
{
    // Phase 241 (PAGE-03): clamp to the two upstream passes (1=coarse, 2=fine).
    m_flowRatePass = (pass == 2) ? 2 : 1;
}

// Phase 241 (PAGE-03): symbolic mode accessors (see header). Keep these in
// sync with the kCalibMode* constants pinned at the top of this file.
int CalibrationServiceMock::calibModePaLine() { return kCalibModePA_Line; }
int CalibrationServiceMock::calibModePaPattern() { return kCalibModePA_Pattern; }
int CalibrationServiceMock::calibModePaTower() { return kCalibModePA_Tower; }
int CalibrationServiceMock::calibModeFlowRate() { return kCalibModeFlowRate; }

// Phase 241 (PAGE-03): in-code PA Pattern generation, mirroring upstream
// Plater::_calib_pa_pattern (Plater.cpp:9418-9563). The upstream flow:
// add a handle cube, apply the SuggestedConfigCalibPAPattern overrides
// (retraction off, fixed accel/jerk, optimal PA speed), construct
// CalibPressureAdvancePattern over the full config, and let it write the
// pattern into the model as per-layer custom G-code (consumed by
// Print.cpp:470 through model.plates_custom_gcodes). OWzx mirrors the
// generation with default-config values. Documented deltas: (a) the preset-
// derived accel/jerk normalization collapses to engine defaults, (b) the
// anchor is a regular printable cube primitive instead of an INVALID-type
// handle cube (the pattern itself is drawn entirely by the generated custom
// G-code, the cube only anchors the print).
bool CalibrationServiceMock::generateAndDispatchPaPattern(const QString &projectName)
{
#ifdef HAS_LIBSLIC3R
    if (!m_projectService || !m_sliceService)
        return false;
    Slic3r::Model *model = m_projectService->rawModel();
    if (!model)
        return false;

    try {
        // Anchor object: upstream adds a Cube handle first
        // (Plater.cpp:9421 load_generic_subobject("Cube", INVALID)).
        if (model->objects.empty() || model->objects.front()->volumes.empty())
            if (m_projectService->addPrimitiveToPlate(0) < 0)
                return false;

        const auto &calibType = m_calibTypes[m_currentItem];
        Slic3r::Calib_Params params;
        params.mode = Slic3r::CalibMode::Calib_PA_Pattern;
        params.start = calibType.calibStart;
        params.end = calibType.calibEnd;
        params.step = calibType.calibStep;
        params.print_numbers = calibType.printNumbers;

        Slic3r::DynamicPrintConfig config =
            Slic3r::DynamicPrintConfig::full_print_config();
        // Upstream retraction overrides for a clean PA readout
        // (Plater.cpp:9430-9433).
        config.set_key_value("wipe", new Slic3r::ConfigOptionBool{false});
        config.set_key_value("retract_when_changing_layer", new Slic3r::ConfigOptionBool{false});
        config.set_key_value("filament_retract_when_changing_layer",
                             new Slic3r::ConfigOptionBoolsNullable{false});
        config.set_key_value("filament_wipe", new Slic3r::ConfigOptionBoolsNullable{false});
        // SuggestedConfigCalibPAPattern overrides (Calib.hpp:233-244).
        for (const auto &pair : Slic3r::SuggestedConfigCalibPAPattern().float_pairs)
            config.set_key_value(pair.first, new Slic3r::ConfigOptionFloat(pair.second));
        for (const auto &pair : Slic3r::SuggestedConfigCalibPAPattern().nozzle_ratio_pairs) {
            const double nozzle = config.opt_float("nozzle_diameter", 0);
            config.set_key_value(pair.first,
                                 new Slic3r::ConfigOptionFloatOrPercent(
                                     nozzle * pair.second / 100, false));
        }
        for (const auto &pair : Slic3r::SuggestedConfigCalibPAPattern().int_pairs)
            config.set_key_value(pair.first, new Slic3r::ConfigOptionInt(pair.second));
        config.set_key_value(Slic3r::SuggestedConfigCalibPAPattern().brim_pair.first,
                             new Slic3r::ConfigOptionEnum<Slic3r::BrimType>(
                                 Slic3r::SuggestedConfigCalibPAPattern().brim_pair.second));

        // Generate the pattern into the live model (writes
        // model.plates_custom_gcodes for the current plate; consumed by
        // Print.cpp:470 during slicing).
        Slic3r::CalibPressureAdvancePattern paPattern(
            params, config, /*is_bbl_machine=*/false, *model, Slic3r::Vec3d::Zero());
        paPattern.generate_custom_gcodes(config, /*is_bbl_machine=*/false, *model,
                                         Slic3r::Vec3d::Zero());

        m_sliceService->setCalibParams(kCalibModePA_Pattern, calibType.calibStart,
                                       calibType.calibEnd, calibType.calibStep,
                                       calibType.printNumbers);
        m_sliceService->startSlice(projectName);
        qDebug("[Calib] PA pattern generated in-code and dispatched");
        return true;
    } catch (const std::exception &ex) {
        qWarning("[Calib] PA pattern generation threw: %s", ex.what());
        return false;
    } catch (...) {
        qWarning("[Calib] PA pattern generation threw an unknown exception");
        return false;
    }
#else
    Q_UNUSED(projectName)
    return false;
#endif
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
