#pragma once
#include <QObject>
#include <QString>
#include <QStringList>

class CalibrationServiceMock;
class PresetServiceMock;

class CalibrationViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE selectItem NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedTitle READ selectedTitle NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDescription READ selectedDescription NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedPreviewLabel READ selectedPreviewLabel NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCategory READ selectedCategory NOTIFY selectionChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int currentStepIndex READ currentStepIndex NOTIFY stepChanged)
    Q_PROPERTY(QString currentStepTitle READ currentStepTitle NOTIFY stepChanged)
    Q_PROPERTY(QString currentStepDesc READ currentStepDesc NOTIFY stepChanged)
    Q_PROPERTY(int totalStepCount READ totalStepCount NOTIFY selectionChanged)
    Q_PROPERTY(int selectedStatus READ selectedStatus NOTIFY statusChanged)
    /// Filament preset selector for preset step (对齐上游 CalibrationPresetPage)
    Q_PROPERTY(QString currentStepId READ currentStepId NOTIFY stepChanged)
    Q_PROPERTY(bool showPresetSelector READ showPresetSelector NOTIFY stepChanged)
    Q_PROPERTY(QStringList filamentPresetNames READ filamentPresetNames CONSTANT)
    Q_PROPERTY(QString selectedFilamentPreset READ selectedFilamentPreset WRITE setSelectedFilamentPreset NOTIFY stateChanged)
    /// Calibration history (对齐上游 FlowCalibHeaderView 历史记录)
    Q_PROPERTY(int historyCount READ historyCount NOTIFY historyChanged)
    /// K 值参数（对齐上游 CalibrationWizardCaliPage K-value input）
    Q_PROPERTY(float currentKValue READ currentKValue WRITE setCurrentKValue NOTIFY calibrationParamsChanged)
    /// N 值参数（对齐上游 CalibrationWizardCaliPage N-value / nozzle diameter input）
    Q_PROPERTY(float currentNValue READ currentNValue WRITE setCurrentNValue NOTIFY calibrationParamsChanged)
    /// 当前步骤是否为校准/精调步骤（显示参数输入）
    Q_PROPERTY(bool showParamInputs READ showParamInputs NOTIFY calibrationParamsChanged)
    /// 校准结果摘要（对齐上游 CalibrationWizardSavePage）
    Q_PROPERTY(bool hasCalibrationResult READ hasCalibrationResult NOTIFY calibrationParamsChanged)
    Q_PROPERTY(QString calibrationResultSummary READ calibrationResultSummary NOTIFY calibrationParamsChanged)

public:
    explicit CalibrationViewModel(CalibrationServiceMock *service, QObject *parent = nullptr);

    /// Set the preset service for filament preset access
    void setPresetService(PresetServiceMock *service);

    // Individual item accessors - use these from QML to avoid Qt6 V4 VariantAssociationObject crash
    Q_INVOKABLE int calibItemCount() const;
    Q_INVOKABLE QString calibItemIcon(int i) const;
    Q_INVOKABLE QString calibItemName(int i) const;
    Q_INVOKABLE QString calibItemDesc(int i) const;
    Q_INVOKABLE int calibItemStatus(int i) const;

    // Step accessors for current selection
    Q_INVOKABLE int stepCount() const;
    Q_INVOKABLE QString stepTitle(int stepIndex) const;
    Q_INVOKABLE QString stepDesc(int stepIndex) const;
    /// Step state: 0=pending, 1=active, 2=completed (对齐上游 StepCtrl)
    Q_INVOKABLE int stepState(int stepIndex) const;

    int selectedIndex() const { return m_selectedIndex; }
    void selectItem(int index);

    QString selectedTitle() const;
    QString selectedDescription() const;
    QString selectedPreviewLabel() const;
    QString selectedCategory() const;
    bool isRunning() const;
    int progress() const;
    int currentStepIndex() const;
    QString currentStepTitle() const;
    QString currentStepDesc() const;
    QString currentStepId() const;
    bool showPresetSelector() const;
    int totalStepCount() const;
    int selectedStatus() const;

    QStringList filamentPresetNames() const;
    QString selectedFilamentPreset() const { return m_selectedFilamentPreset; }
    void setSelectedFilamentPreset(const QString &name);

    // K/N 参数访问器（对齐上游 CalibrationWizardCaliPage）
    float currentKValue() const { return m_currentKValue; }
    void setCurrentKValue(float v);
    float currentNValue() const { return m_currentNValue; }
    void setCurrentNValue(float v);
    bool showParamInputs() const;
    bool hasCalibrationResult() const { return m_hasResult; }
    QString calibrationResultSummary() const;

    /// 保存校准结果到历史（对齐上游 CalibrationWizardSavePage save）
    Q_INVOKABLE void saveCalibrationResult();
    /// 加载历史记录的 K/N 值（对齐上游 FlowCalibHeaderView load）
    Q_INVOKABLE void loadHistoryEntry(int index);

    // History accessors (对齐上游 FlowCalibHeaderView 历史记录)
    int historyCount() const;
    Q_INVOKABLE QString historyName(int index) const;
    Q_INVOKABLE QString historyFilamentId(int index) const;
    Q_INVOKABLE float historyKValue(int index) const;
    Q_INVOKABLE float historyNozzleDiameter(int index) const;
    Q_INVOKABLE QString historyTimestamp(int index) const;
    Q_INVOKABLE void clearHistory();

signals:
    void selectionChanged();
    void runningChanged();
    void progressChanged();
    void stepChanged();
    void statusChanged(int typeIndex, int status);
    void stateChanged();
    void historyChanged();
    void calibrationParamsChanged();

public slots:
    void startCalibration();
    void cancelCalibration();
    void goToStep(int stepIndex);
    void resetParameters();

private:
    int m_selectedIndex = -1;
    CalibrationServiceMock *m_service = nullptr;
    PresetServiceMock *m_presetService = nullptr;
    QString m_selectedFilamentPreset;
    float m_currentKValue = 0.0f;
    float m_currentNValue = 0.4f;
    bool m_hasResult = false;
};
