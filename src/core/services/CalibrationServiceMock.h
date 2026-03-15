#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>

class QTimer;

// Calibration step in a wizard flow
struct CalibrationStep
{
    QString id;          // e.g. "start", "preset", "cali", "coarse_save", "fine_cali", "fine_save", "save"
    QString title;       // Display title
    QString description; // Guidance text shown on this step
};

// A calibration type (e.g. Flow Dynamics, Bed Leveling)
struct CalibrationType
{
    QString     id;           // e.g. "flow_dynamics", "bed_leveling"
    QString     name;         // Display name
    QString     icon;         // Icon character
    QString     category;     // "slice" or "hardware"
    QString     description;  // Short description
    QString     longDesc;     // Detailed guidance
    QString     previewLabel; // Preview area label
    QList<CalibrationStep> steps;
};

// Status of a calibration type
enum class CalibrationStatus : int
{
    NotStarted  = 0,
    InProgress  = 1,
    Completed   = 2,
    Failed      = 3
};

// Calibration history entry (对齐上游 FlowCalibHeaderView 历史记录)
struct CalibrationHistoryEntry
{
    QString name;           // Calibration type name
    QString filamentId;     // Filament preset identifier
    float kValue;           // K-value (Pressure Advance)
    float nozzleDiameter;   // Nozzle diameter used
    QString timestamp;      // ISO timestamp
};

// Mock calibration service - simulates calibration wizard flow
// Upstream reference: CalibrationPanel + CalibrationWizard + CalibrationDialog
// Replace with real CalibrationService when device communication is integrated
class CalibrationServiceMock final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(int currentStepIndex READ currentStepIndex NOTIFY stepChanged)

public:
    explicit CalibrationServiceMock(QObject *parent = nullptr);
    ~CalibrationServiceMock() override;

    // Calibration type list
    Q_INVOKABLE int calibTypeCount() const;
    Q_INVOKABLE QString calibTypeId(int index) const;
    Q_INVOKABLE QString calibTypeName(int index) const;
    Q_INVOKABLE QString calibTypeIcon(int index) const;
    Q_INVOKABLE QString calibTypeCategory(int index) const;
    Q_INVOKABLE QString calibTypeDesc(int index) const;
    Q_INVOKABLE QString calibTypeLongDesc(int index) const;
    Q_INVOKABLE QString calibTypePreviewLabel(int index) const;

    // Steps for a given calibration type
    Q_INVOKABLE int stepCount(int typeIndex) const;
    Q_INVOKABLE QString stepId(int typeIndex, int stepIndex) const;
    Q_INVOKABLE QString stepTitle(int typeIndex, int stepIndex) const;
    Q_INVOKABLE QString stepDesc(int typeIndex, int stepIndex) const;
    /// Step state: 0=pending, 1=active, 2=completed (对齐上游 StepCtrl state)
    Q_INVOKABLE int stepState(int typeIndex, int stepIndex) const;

    // Status of a calibration type
    Q_INVOKABLE int calibStatus(int typeIndex) const;

    // Progress
    int progress() const { return m_progress; }
    bool isRunning() const { return m_isRunning; }
    int currentStepIndex() const { return m_currentStepIndex; }

    // Actions
    Q_INVOKABLE void startCalibration(int itemIndex);
    Q_INVOKABLE void cancelCalibration();
    Q_INVOKABLE void goToStep(int stepIndex);
    Q_INVOKABLE void resetCalibration(int itemIndex);

    // History accessors (对齐上游 FlowCalibHeaderView 历史记录)
    Q_INVOKABLE int historyCount() const;
    Q_INVOKABLE QString historyName(int index) const;
    Q_INVOKABLE QString historyFilamentId(int index) const;
    Q_INVOKABLE float historyKValue(int index) const;
    Q_INVOKABLE float historyNozzleDiameter(int index) const;
    Q_INVOKABLE QString historyTimestamp(int index) const;
    Q_INVOKABLE void addHistoryEntry(const QString &name, const QString &filamentId,
                                      float kValue, float nozzleDiameter, const QString &timestamp);
    Q_INVOKABLE void clearHistory();

signals:
    void progressChanged();
    void isRunningChanged();
    void calibrationFinished(bool success);
    void stepChanged();
    void statusChanged(int typeIndex, int status);
    void historyChanged();

private slots:
    void onTick();

private:
    void buildMockData();
    void setStatus(int typeIndex, CalibrationStatus status);
    void advanceStep();

    QList<CalibrationType> m_calibTypes;
    QMap<int, CalibrationStatus> m_statusMap; // typeIndex -> status
    QList<CalibrationHistoryEntry> m_history; // Calibration history records
    int m_progress = 0;
    bool m_isRunning = false;
    int m_currentItem = -1;
    int m_currentStepIndex = -1;
    QTimer *m_timer = nullptr;
};
