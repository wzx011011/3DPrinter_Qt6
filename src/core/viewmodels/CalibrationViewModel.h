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
    Q_PROPERTY(float currentFlowRate READ currentFlowRate WRITE setCurrentFlowRate NOTIFY calibrationParamsChanged)
    /// N 值参数（对齐上游 CalibrationWizardCaliPage N-value / nozzle diameter input）
    Q_PROPERTY(float currentNValue READ currentNValue WRITE setCurrentNValue NOTIFY calibrationParamsChanged)
    /// 当前步骤是否为校准/精调步骤（显示参数输入）
    Q_PROPERTY(bool showParamInputs READ showParamInputs NOTIFY calibrationParamsChanged)
    /// 校准结果摘要（对齐上游 CalibrationWizardSavePage）
    Q_PROPERTY(bool hasCalibrationResult READ hasCalibrationResult NOTIFY calibrationParamsChanged)
    Q_PROPERTY(QString calibrationResultSummary READ calibrationResultSummary NOTIFY calibrationParamsChanged)
    /// Phase 125 (CALIB-02): user-editable calibration sweep range. READ returns
    /// the selected CalibrationType's current range (Phase 124 hardcoded default
    /// until the user edits it); WRITE forwards the override to
    /// CalibrationServiceMock::setCalibRange so the edited sweep flows into
    /// setCalibParams -> startSlice. Re-emits on selectionChanged (new mode's
    /// defaults) and on the explicit setter (user edit).
    Q_PROPERTY(double calibStart READ calibStart WRITE setCalibStart NOTIFY selectionChanged)
    Q_PROPERTY(double calibEnd READ calibEnd WRITE setCalibEnd NOTIFY selectionChanged)
    Q_PROPERTY(double calibStep READ calibStep WRITE setCalibStep NOTIFY selectionChanged)
    /// U03: hardware-calibration capability bits, one per gated option of the
    /// upstream CalibrationDialog (update_cali, Calibration.cpp:210-254):
    /// lidar = SupportAIMonitor() && SupportCalibrationLidar(), bed leveling =
    /// is_support_bed_leveling, motor noise = is_support_motor_noise_cali,
    /// nozzle offset = SupportCalibrationNozzleOffset(), high-temp bed =
    /// SupportCalibrationHighTempBed(), clump = SupportCaliClumpPos().
    /// Vibration compensation is never gated upstream. The mock device stack
    /// is a full-capability stance (all true) until a real device channel
    /// feeds these bits -- registered gap, mirrors the runtime-stage gap below.
    Q_PROPERTY(bool supportLidarCali READ supportLidarCali CONSTANT)
    Q_PROPERTY(bool supportBedLeveling READ supportBedLeveling CONSTANT)
    Q_PROPERTY(bool supportMotorNoiseCali READ supportMotorNoiseCali CONSTANT)
    Q_PROPERTY(bool supportNozzleOffsetCali READ supportNozzleOffsetCali CONSTANT)
    Q_PROPERTY(bool supportHighTempBedCali READ supportHighTempBedCali CONSTANT)
    Q_PROPERTY(bool supportClumpPosCali READ supportClumpPosCali CONSTANT)
    /// U03: nozzle-diameter filter for the calibration history list (upstream
    /// HistoryWindow requests per-nozzle records through the device,
    /// CaliHistoryDialog.cpp:144-159/:247-255 -- the combo lists
    /// 0.2/0.4/0.6/0.8 mm and defaults to the machine's current nozzle). The
    /// OWzx history is local, so the VM holds the selected filter value and
    /// the dialog filters entries against it.
    Q_PROPERTY(float historyNozzleFilter READ historyNozzleFilter WRITE setHistoryNozzleFilter NOTIFY historyFilterChanged)

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
    Q_INVOKABLE QString calibItemId(int i) const;
    Q_INVOKABLE QString calibItemCategory(int i) const;
    Q_INVOKABLE bool calibItemImplemented(int i) const;
    Q_INVOKABLE bool calibItemStartable(int i) const;
    Q_INVOKABLE QString calibItemUnavailableReason(int i) const;

    // Step accessors for current selection
    Q_INVOKABLE int stepCount() const;
    Q_INVOKABLE QString stepTitle(int stepIndex) const;
    Q_INVOKABLE QString stepDesc(int stepIndex) const;
    /// Step state: 0=pending, 1=active, 2=completed (对齐上游 StepCtrl)
    Q_INVOKABLE int stepState(int stepIndex) const;

    int selectedIndex() const { return m_selectedIndex; }
    void selectItem(int index);
    Q_INVOKABLE bool selectItemById(const QString &id);

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
    float currentFlowRate() const { return m_currentFlowRate; }
    void setCurrentFlowRate(float v);
    float currentNValue() const { return m_currentNValue; }
    void setCurrentNValue(float v);
    bool showParamInputs() const;
    bool hasCalibrationResult() const;
    QString calibrationResultSummary() const;

    // Phase 125 (CALIB-02): range accessors/setters. The getter reads the
    // selected mode's current range from the service; the setter applies the
    // user edit to the service (overrides the Phase 124 hardcoded default)
    // before startSlice dispatches setCalibParams.
    double calibStart() const;
    double calibEnd() const;
    double calibStep() const;
    void setCalibStart(double v);
    void setCalibEnd(double v);
    void setCalibStep(double v);

    // U03: capability bits (see Q_PROPERTY docs above).
    bool supportLidarCali() const { return m_supportLidarCali; }
    bool supportBedLeveling() const { return m_supportBedLeveling; }
    bool supportMotorNoiseCali() const { return m_supportMotorNoiseCali; }
    bool supportNozzleOffsetCali() const { return m_supportNozzleOffsetCali; }
    bool supportHighTempBedCali() const { return m_supportHighTempBedCali; }
    bool supportClumpPosCali() const { return m_supportClumpPosCali; }

    // U03: history nozzle-diameter filter.
    float historyNozzleFilter() const { return m_historyNozzleFilter; }
    void setHistoryNozzleFilter(float v);

    /// U03: record the seven hardware-calibration checkboxes on the shared VM
    /// before startCalibration (upstream on_start_calibration ->
    /// command_start_calibration passes the 7 live checkbox values,
    /// Calibration.cpp:324-341). The dialog previously kept these selections
    /// as dead dialog-local state that never reached the VM. Defaults: all
    /// checked except bed_cali (high-temp heatbed), per STUDIO-10091
    /// (Calibration.cpp:62-65).
    Q_INVOKABLE void setHardwareOptions(bool lidar, bool bedLevel, bool vibration,
                                        bool motor, bool nozzleOffset, bool heatbed,
                                        bool clump);

    /// U03: delete one history entry (upstream row Delete button ->
    /// CalibUtils::delete_PA_calib_result, CaliHistoryDialog.cpp:418-442).
    /// CalibrationServiceMock exposes no per-entry removal, so the deletion
    /// rebuilds the list through the existing clearHistory + addHistoryEntry
    /// channel (all eight fields round-trip), preserving newest-first order.
    Q_INVOKABLE void deleteHistoryEntry(int index);
    /// U03: rewrite the name + K value of one history entry (upstream
    /// EditCalibrationHistoryDialog on_save -> set_PA_calib_result,
    /// CaliHistoryDialog.cpp:649-689). K range/empty-name validation stays in
    /// the dialog (mirrors upstream CalibUtils::validate_input_*).
    /// Returns false when the index is out of range or the write failed.
    Q_INVOKABLE bool updateHistoryEntry(int index, const QString &name, float kValue);
    /// U03: append a manual history record (upstream
    /// NewCalibrationHistoryDialog on_ok, CaliHistoryDialog.cpp:909-998).
    /// A manual record has no machine readback by definition, so
    /// hasRealReadback stays false (honest bookkeeping).
    Q_INVOKABLE void addManualHistoryEntry(const QString &name, const QString &filamentName,
                                           float nozzleDiameter, float kValue);
    /// U03: filament preset display name for a history entry (upstream
    /// get_preset_name_by_filament_id, CaliHistoryDialog.cpp:72-107). The
    /// OWzx history stores filament preset names (the writer passes
    /// m_selectedFilamentPreset); the literal "default" resolves to the
    /// service's default filament preset name. Read-only reuse of
    /// PresetServiceMock queries.
    Q_INVOKABLE QString historyFilamentName(int index) const;

    /// 保存校准结果到历史（对齐上游 CalibrationWizardSavePage save）
    Q_INVOKABLE void saveCalibrationResult();
    /// Phase 241 (PAGE-03): write the measured value into the selected
    /// filament preset (upstream CalibrationWizardSavePage on_save →
    /// preset value write). PA modes write pressure_advance; FlowRate
    /// writes filament_flow_ratio. Also records a history entry. Returns
    /// false when there is no preset service / the preset is read-only /
    /// the write failed.
    Q_INVOKABLE bool saveCalibrationResultToPreset();
    /// Phase 241 (PAGE-03): Flow Rate fine pass (upstream
    /// CalibState::FineCalibration loads flowrate-test-pass2.3mf).
    /// Resets to the coarse pass on the next plain startCalibration.
    Q_INVOKABLE void startFineCalibration();
    /// 加载历史记录的 K/N 值（对齐上游 FlowCalibHeaderView load）
    Q_INVOKABLE void loadHistoryEntry(int index);

    // History accessors (对齐上游 FlowCalibHeaderView 历史记录)
    int historyCount() const;
    Q_INVOKABLE QString historyName(int index) const;
    Q_INVOKABLE QString historyFilamentId(int index) const;
    Q_INVOKABLE float historyKValue(int index) const;
    Q_INVOKABLE float historyFlowRate(int index) const;
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
    void historyFilterChanged();

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
    float m_currentFlowRate = 1.0f;
    float m_currentNValue = 0.4f;
    bool m_hasResult = false;
    int m_resultMode = -1;
    // U03: hardware option bits (defaults mirror Calibration.cpp:53-65 --
    // every option checked except bed_cali).
    bool m_hwLidar = true;
    bool m_hwBedLevel = true;
    bool m_hwVibration = true;
    bool m_hwMotor = true;
    bool m_hwNozzleOffset = true;
    bool m_hwHeatbed = false;
    bool m_hwClump = true;
    // U03: capability bits (mock full-capability device until a real device
    // channel exists; see Q_PROPERTY docs).
    bool m_supportLidarCali = true;
    bool m_supportBedLeveling = true;
    bool m_supportMotorNoiseCali = true;
    bool m_supportNozzleOffsetCali = true;
    bool m_supportHighTempBedCali = true;
    bool m_supportClumpPosCali = true;
    // U03: history nozzle-diameter filter (upstream combo defaults to the
    // machine's current nozzle; the mock stance is the 0.4 default).
    float m_historyNozzleFilter = 0.4f;
};
