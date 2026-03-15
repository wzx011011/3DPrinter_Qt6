#include "CalibrationServiceMock.h"
#include <QTimer>
#include <QDateTime>
#include <algorithm>

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
    // Additional CalibMode values from upstream calib.hpp:
    //   Temp_Tower, Vol_speed_Tower, Retraction_tower
    //
    // We expose 5 calibration types covering both slice and hardware domains,
    // each with 2-4 wizard steps aligned to upstream CalibrationWizard page chain:
    //   Start -> Preset -> Calibration -> [CoarseSave -> FineCalibration -> FineSave ->] Save

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
    flowRate.steps = {
        {"start",        tr("Introduction"), tr("Learn when to use Flow Rate calibration.")},
        {"preset",       tr("Select Filament"), tr("Choose filament, bed type, and nozzle diameter.")},
        {"cali",         tr("Coarse Calibration"), tr("Run coarse flow rate calibration pass.")},
        {"coarse_save",  tr("Coarse Result"), tr("Review coarse calibration result.")},
        {"fine_cali",    tr("Fine Calibration"), tr("Run fine flow rate calibration pass.")},
        {"fine_save",    tr("Save Result"), tr("Review and save fine calibration result.")}
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
    vibration.steps = {
        {"start",  tr("Introduction"), tr("Learn about vibration compensation calibration.")},
        {"cali",   tr("Measure"), tr("Printer performs resonance measurement.")},
        {"save",   tr("Apply Result"), tr("Review resonance data and apply compensation.")}
    };

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
    maxVolSpeed.steps = {
        {"start",  tr("Introduction"), tr("Learn about max volumetric speed calibration.")},
        {"preset", tr("Select Parameters"), tr("Set calibration range parameters.")},
        {"cali",   tr("Calibrate"), tr("Send speed tower calibration to printer.")},
        {"save",   tr("Save Result"), tr("Review result and save to filament preset.")}
    };

    m_calibTypes = {flowDynamics, flowRate, bedLeveling, vibration, maxVolSpeed};

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
    // 对齐上游 StepCtrl: 0=pending, 1=active, 2=completed
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

    m_currentItem = itemIndex;
    m_progress = 0;
    m_currentStepIndex = 0;
    m_isRunning = true;

    setStatus(itemIndex, CalibrationStatus::InProgress);
    emit isRunningChanged();
    emit progressChanged();
    emit stepChanged();
    m_timer->start();
}

void CalibrationServiceMock::cancelCalibration()
{
    if (!m_isRunning) return;
    m_timer->stop();
    m_isRunning = false;

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

void CalibrationServiceMock::addHistoryEntry(const QString &name, const QString &filamentId,
                                              float kValue, float nozzleDiameter, const QString &timestamp)
{
    CalibrationHistoryEntry entry;
    entry.name = name;
    entry.filamentId = filamentId;
    entry.kValue = kValue;
    entry.nozzleDiameter = nozzleDiameter;
    entry.timestamp = timestamp;
    m_history.prepend(entry); // Most recent first
    emit historyChanged();
}

void CalibrationServiceMock::clearHistory()
{
    if (m_history.isEmpty()) return;
    m_history.clear();
    emit historyChanged();
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

            // Add history entry (对齐上游 FlowCalibHeaderView)
            addHistoryEntry(
                m_calibTypes[m_currentItem].name,
                QString("filament_%1").arg(m_currentItem), // Mock filament ID
                0.04f + (m_currentItem * 0.01f),           // Mock K-value
                0.4f,                                       // Default nozzle diameter
                QDateTime::currentDateTime().toString(Qt::ISODate)
            );
        }

        emit isRunningChanged();
        emit calibrationFinished(true);
    }
}
