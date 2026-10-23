#include "BackendContext.h"

#include <QFileInfo>

#include "core/services/DeviceServiceMock.h"
#include "core/services/NetworkServiceMock.h"
#include "core/services/CameraServiceMock.h"
#include <QSettings>
#include <QUuid>

#include "core/services/CloudServiceMock.h"
#include "core/services/CrealityAuthService.h"
#include "core/services/CrealityCloudService.h"
#include "core/services/CrealityCloudConfig.h"
#include "core/services/ModelLibraryBridge.h"
#include "core/services/ModelDownloadService.h"
#include "core/services/CalibrationServiceMock.h"
#include "core/services/PresetService.h"
#include "core/services/ProjectService.h"
#include "core/services/PluginService.h"
#include "core/services/SliceService.h"
#include "core/services/UndoRedoManager.h"
#include "core/services/AppSettingsService.h"
#include "core/viewmodels/ConfigViewModel.h"
#include "core/viewmodels/EditorViewModel.h"
#include "core/viewmodels/MonitorViewModel.h"
#include "core/viewmodels/PreviewViewModel.h"
#include "core/viewmodels/HomeViewModel.h"
#include "core/viewmodels/SettingsViewModel.h"
#include "core/viewmodels/ProjectViewModel.h"
#include "core/viewmodels/CalibrationViewModel.h"
#include "core/viewmodels/MultiMachineViewModel.h"
#include "core/viewmodels/AmsMaterialsViewModel.h"
#include "core/viewmodels/AiViewModel.h"
// S6: ThemeTokens/ThemeState live in the renderer header (owzx_app_core side
// of the one-way owzx_qml -> owzx_app_core target edge; see RhiViewport.h).
#include "Renderer/RhiViewport.h"
#include "AiChatBridge.h"
#include "core/ai/AiAgentService.h"
#include "core/ai/AppToolRegistry.h"
#include "core/ai/McpHttpServer.h"

#include <QByteArray>
#include <QGuiApplication>
#include <QFont>
#include <iterator>
#include <type_traits>
#include <utility>
#include <QUrl>
#include <algorithm>
#include <QTimer>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonArray>
#include <QWebChannel>
#include <QJsonObject>
#include <QDesktopServices>
#include <QHostInfo>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMetaEnum>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>
#include <QSysInfo>
#include <QDir>
#ifdef Q_OS_WINDOWS
// U04 (systemInfo totalRamMb): upstream SysInfoDialog samples the total
// physical RAM (SysInfoDialog.cpp:59 "Total RAM size [MB]" via
// total_physical_memory()) — the Qt6 build reads it through GlobalMemoryStatusEx.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif /* Q_OS_WINDOWS */

// 主题：S6 起 C++ 侧不再私有调色板——kThemes 三档纯深色表已删，主题真值是
// RhiViewport.h 的 ThemeTokens 明暗双档常量表，经 ThemeState 单例分发；
// Backend 只负责把活动档投影成 themeTokens QVariantMap 给 QML（Theme.qml）。
// uiScale 预设
static const double kScales[] = {1.0, 1.25, 1.5, 1.75, 2.0};
// 语言代码映射
static const char *kLangCodes[] = {"zh_CN", "en", "ja", "ko", "de", "fr"};

// ── X1（审计④章）：数据面窄接口的装配点 ──────────────────────────────
// 编译期钉死：mock 必须实现各自的数据面窄接口。真后端到达时以"实现接口 +
// 改本文件这一处装配"接入，消费方/测试面向接口（注入缝），不再原位改写
// 12.5k 行具体类。
static_assert(std::is_base_of_v<IDeviceDataPlane, DeviceServiceMock>,
              "DeviceServiceMock must implement IDeviceDataPlane (X1 data-plane seam)");
static_assert(std::is_base_of_v<ICloudDataPlane, CloudServiceMock>,
              "CloudServiceMock must implement ICloudDataPlane (X1 data-plane seam)");
static_assert(std::is_base_of_v<ICameraStreamService, CameraServiceMock>,
              "CameraServiceMock must implement ICameraStreamService (X1 data-plane seam)");

IDeviceDataPlane *BackendContext::deviceDataPlane() const { return deviceService_; }
ICloudDataPlane *BackendContext::cloudDataPlane() const { return cloudService_; }

QObject *BackendContext::crealityAuthService() const { return crealityAuthService_; }

QObject *BackendContext::modelLibraryBridge() const { return modelLibraryBridge_; }

QObject *BackendContext::modelDownloadService() const { return modelDownloadService_; }

QString BackendContext::crealityWebViewUserAgent() const
{
  return CrealityCloudConfig::webViewUserAgent(QCoreApplication::applicationVersion());
}

QString BackendContext::qwebchannelScript() const
{
  QFile f(QStringLiteral(":/web/cx/qwebchannel.js"));
  if (!f.open(QIODevice::ReadOnly)) {
    qWarning("[Backend] qwebchannel.js missing from resources");
    return {};
  }
  return QString::fromUtf8(f.readAll());
}

void BackendContext::attachModelLibraryChannel(QObject *channel)
{
  // See attachAiChatChannel: QQmlWebChannel derives from QWebChannel; the
  // panel's injected shim resolves the object as channel.objects.cxbridge.
  auto *webChannel = qobject_cast<QWebChannel *>(channel);
  if (!webChannel) {
    qWarning("[Backend] attachModelLibraryChannel: not a QWebChannel instance");
    return;
  }
  webChannel->registerObject(QStringLiteral("cxbridge"), modelLibraryBridge_);
}
ICameraStreamService *BackendContext::cameraStreamService() const { return cameraService_; }

BackendContext::BackendContext(QObject *parent)
    : QObject(parent)
{
  m_latencyClock.start();
  const QByteArray compareMode = qgetenv("OWZX_VISUAL_COMPARE_MODE");
  visualCompareMode_ = (compareMode == "1" || compareMode.compare("true", Qt::CaseInsensitive) == 0);

  calibrationService_ = new CalibrationServiceMock(this);
  presetService_ = new PresetService(this);
  deviceService_ = new DeviceServiceMock(this);
  projectService_ = new ProjectService(this);
  sliceService_ = new SliceService(projectService_, this);
  // v2.7 P1：注入 SliceService 到 CalibrationService（校准时触发 calib slice）
  calibrationService_->setSliceService(sliceService_);
  // Phase 197: inject ProjectService so the tower modes can load a
  // dedicated tower model (qrc:/qml/assets/calib/*.stl/.step) before slicing.
  calibrationService_->setProjectService(projectService_);
  networkService_ = new NetworkServiceMock(this);
  cameraService_ = new CameraServiceMock(this);
  // X1: 云数据面装配（接口访问器 cloudDataPlane() 出口；此前为构造函数局部量）
  // feature/cloud-model-library (M2): Cloud/Backend 装配开关 —— "mock"
  // （默认，演示模式不变）或 "creality"（真后端：OAuth 账号面 + 诚实的
  // 空/空闲设备与同步面）。AppSettingsService 提前到本处创建仅为读取
  // cloudRegion（原本 v2.8 W3 的注入点行为不变）。
  appSettings_ = new AppSettingsService(this);
  crealityAuthService_ = new CrealityAuthService(this);
  crealityAuthService_->setRegion(appSettings_->cloudRegion()
                                          == QStringLiteral("CN")
                                      ? CrealityCloudConfig::Region::CN
                                      : CrealityCloudConfig::Region::International);
  {
    QSettings backendSettings;
    const QString backend = backendSettings
                                .value(QStringLiteral("Cloud/Backend"),
                                       QStringLiteral("mock"))
                                .toString();
    cloudBackendKind_ = (backend == QStringLiteral("creality"))
                            ? QStringLiteral("creality")
                            : QStringLiteral("mock");
  }
  if (cloudBackendKind_ == QStringLiteral("creality"))
    cloudService_ = new CrealityCloudService(crealityAuthService_, this);
  else
    cloudService_ = new CloudServiceMock(this);
  // feature/cloud-model-library (M4): downloader for the mall's
  // *_download_start commands; files land under the configured
  // AppSettings.modelDownloadPath.
  modelDownloadService_ = new ModelDownloadService(this);
  modelDownloadService_->setDownloadRoot(appSettings_->modelDownloadPath());
  connect(modelDownloadService_, &ModelDownloadService::stateChanged, this, [this]() {
    // First active download pops the progress dialog (official opens a
    // modal per 3mf download; one aggregate dialog covers model groups).
    static bool dialogShown = false;
    const bool busy = modelDownloadService_->busy();
    if (busy && !dialogShown) {
      dialogShown = true;
      emit showCloudDownloadDialogRequested();
    } else if (!busy) {
      dialogShown = false;
    }
  });
  connect(modelDownloadService_, &ModelDownloadService::downloadFinished, this,
          [this](const QString &fileId, const QString &finalPath) {
            // Upstream parity (CloudDownloadProgressDialog.cpp:114-126): the
            // completed 3mf auto-imports after the dialog closes — deferred
            // so the QML dialog observes the settled state first.
            if (finalPath.endsWith(QStringLiteral(".3mf"), Qt::CaseInsensitive)) {
              QTimer::singleShot(0, this, [this, finalPath]() {
                topbarOpenProject(finalPath);
                setCurrentPage(1);
              });
            } else {
              Q_UNUSED(fileId);
            }
          });
  // feature/cloud-model-library (M3): the webview JS bridge serves both
  // backend kinds (anonymous browsing works in mock mode too).
  modelLibraryBridge_ = new ModelLibraryBridge(crealityAuthService_, modelDownloadService_, this);
  connect(modelLibraryBridge_, &ModelLibraryBridge::importProjectRequested, this,
          [this](const QString &path) {
            topbarOpenProject(path);
            setCurrentPage(1);
          });
  connect(modelLibraryBridge_, &ModelLibraryBridge::importModelFilesRequested, this,
          [this](const QStringList &paths) {
            if (paths.isEmpty())
              return;
            if (paths.size() == 1)
              topbarImportModel(paths.first());
            else if (editorViewModel_)
              editorViewModel_->addFilesToCurrentPlate(paths);
            setCurrentPage(1);
          });
  {
    // DUID fallback id: generated once per install (upstream get_client_id
    // uses a first-boot UUID when no MAC is available).
    QSettings duidSettings;
    QString duidFallback =
        duidSettings.value(QStringLiteral("Cloud/DuidFallback")).toString();
    if (duidFallback.isEmpty()) {
      duidFallback = QUuid::createUuid().toString(QUuid::WithoutBraces);
      duidSettings.setValue(QStringLiteral("Cloud/DuidFallback"), duidFallback);
    }
    modelLibraryBridge_->setDuid(CrealityCloudConfig::deriveDuid(
        CrealityCloudConfig::macAddressCandidates(), duidFallback));
  }

  // U11: 发送链路真实性桥。DeviceServiceMock 的 FTP+MQTT 真发送路径此前
  // 只在 uploadFinished 里打日志：publish 失败仍无条件把设备置 "printing"，
  // 上传失败静默，uploadProgress 零订阅。这里把服务的进度/终态信号桥到：
  //   - printSendUpload* 属性（PrintDialog 发送页仪表的真相源）；
  //   - 失败 toast（NotiError，U1 severity 纪律：失败绝不可以播成功色）；
  //   - 取消不播 toast（对话框自身回 prepare 页即反馈，对齐上游
  //     on_print_job_cancel 的安静语义，SendToPrinter.cpp:1055-1085）；
  //   - 成功不播 toast（对话框 finish 页 + 设备 printing 状态即反馈）。
  connect(deviceService_, &DeviceServiceMock::sendPrintUploadStarted, this, [this]() {
    m_printSendUploadActive = true;
    m_printSendUploadProgress = 0;
    emit printSendUploadChanged();
  });
  connect(deviceService_, &DeviceServiceMock::sendPrintUploadProgress, this,
          [this](int percent) {
    m_printSendUploadActive = true;
    m_printSendUploadProgress = percent;
    emit printSendUploadChanged();
  });
  connect(deviceService_, &DeviceServiceMock::sendPrintSucceeded, this, [this]() {
    m_printSendUploadActive = false;
    m_printSendUploadProgress = 0;
    emit printSendUploadChanged();
    emit printSendFinished(true, QString());
  });
  connect(deviceService_, &DeviceServiceMock::sendPrintFailed, this,
          [this](const QString &reason) {
    m_printSendUploadActive = false;
    m_printSendUploadProgress = 0;
    emit printSendUploadChanged();
    postError(tr("发送到打印机失败: %1").arg(reason), NotiError);
    emit printSendFinished(false, reason);
  });
  connect(deviceService_, &DeviceServiceMock::sendPrintCancelled, this, [this]() {
    m_printSendUploadActive = false;
    m_printSendUploadProgress = 0;
    emit printSendUploadChanged();
    emit printSendFinished(false, QString());
  });

  editorViewModel_ = new EditorViewModel(projectService_, sliceService_, this);
  connect(editorViewModel_, &EditorViewModel::stateChanged, this, &BackendContext::displayProjectTitleChanged);
  // GAP-7: multi-select info notification (upstream
  // Plater::bbl_show_objects_info_notification, Plater.cpp:14121-14165 --
  // entering a multi-selection posts an informational "N objects selected"
  // notification; leaving it lets the info expire). Post only on the
  // transition so repeated stateChanged ticks do not stack toasts.
  connect(editorViewModel_, &EditorViewModel::stateChanged, this, [this]() {
    const int count = editorViewModel_ ? editorViewModel_->selectedObjectCount() : 0;
    if (count >= 2 && count != m_lastSelectedObjectsInfoCount)
      postNotification(tr("%1 objects selected").arg(count),
                       QString(), int(NotiInfo));
    m_lastSelectedObjectsInfoCount = count;
  });

  // Undo/Redo 框架（对齐上游 UndoRedo）：创建管理器并注入到 EditorViewModel
  auto *undoManager = new UndoRedoManager(this);
  editorViewModel_->setUndoRedoManager(undoManager);
  // Phase 118 (TICK-02/TICK-03): inject projectService_ so PreviewViewModel can
  // write tickMarks_ back into libslic3r's plates_custom_gcodes and re-slice.
  previewViewModel_ = new PreviewViewModel(projectService_, sliceService_, this);
  // Phase 51 SHELL-02/SHELL-03: forward viewmodel stateChanged so shell gates
  // (canImport/canSlice/isSlicing/canExport/canSave/canUndo/canRedo/isBusy) and
  // Prepare/Preview state stay live across page round-trips.
  connect(editorViewModel_, &EditorViewModel::stateChanged, this, [this]() {
    emit stateChanged();
  });
  connect(previewViewModel_, &PreviewViewModel::stateChanged, this, [this]() {
    emit stateChanged();
  });
  // U4: state drivers for the central busy cursor. Direct (same-thread)
  // connections are what make the synchronous runs work: EditorViewModel
  // sets arranging/exporting/importing and emits stateChanged BEFORE the
  // blocking call, so updateGuiBusyCursor applies the override cursor inline,
  // and the matching reset after the call removes it -- pairing is linear by
  // construction. The service signals cover the async worker families
  // (slice / 3MF+project load / G-code export), whose terminal events
  // (sliceFinished / sliceFailed / exportFinished / exportFailed /
  // loadFinished success or failure) always flip the observed state back.
  connect(editorViewModel_, &EditorViewModel::stateChanged, this,
          &BackendContext::updateGuiBusyCursor);
  connect(projectService_, &ProjectService::loadingChanged, this,
          &BackendContext::updateGuiBusyCursor);
  connect(sliceService_, &SliceService::slicingChanged, this,
          &BackendContext::updateGuiBusyCursor);
  connect(sliceService_, &SliceService::exportStarted, this,
          &BackendContext::updateGuiBusyCursor);
  connect(sliceService_, &SliceService::exportFinished, this,
          &BackendContext::updateGuiBusyCursor);
  connect(sliceService_, &SliceService::exportFailed, this,
          &BackendContext::updateGuiBusyCursor);
  monitorViewModel_ = new MonitorViewModel(deviceService_, networkService_, cameraService_, this);
  configViewModel_ = new ConfigViewModel(presetService_, projectService_, this);
  // Phase 52 PREPSB-05 (CRITICAL): a real slicing config change makes any
  // previously-sliced/previewed/exported result stale. UI-only settings state
  // changes (opening a dialog or switching tabs) must not clear results.
  connect(configViewModel_, &ConfigViewModel::sliceAffectingConfigChanged, editorViewModel_,
          [this]() {
            editorViewModel_->invalidateAllSliceResults();
            emit editorViewModel_->stateChanged();
          });

  // Wire ConfigViewModel into EditorViewModel for preset injection at slice time
  // (对齐上游 PresetBundle::full_fff_config → BackgroundSlicingProcess)
  editorViewModel_->setConfigViewModel(configViewModel_);
  // v5.16 (PSET2-06): keep the per-extruder filament preset slot vector sized
  // to the extruder count (upstream update_multi_material_filament_presets).
  // setExtruderCount no-ops when the count is unchanged, so riding
  // stateChanged is cheap.
  configViewModel_->setExtruderCount(editorViewModel_->extruderCount());
  connect(editorViewModel_, &EditorViewModel::stateChanged, this, [this]() {
    configViewModel_->setExtruderCount(editorViewModel_->extruderCount());
  });
  homeViewModel_ = new HomeViewModel(cloudService_, this);
  settingsViewModel_ = new SettingsViewModel(this);
  projectViewModel_ = new ProjectViewModel(this);
  // Phase 241 (PAGE-01/PAGE-02): Home mirrors the persisted recent list from
  // ProjectViewModel (single source of truth, upstream app_config
  // "recent_projects"); the recentChanged -> refresh wiring lives INSIDE
  // HomeViewModel::setProjectViewModel. ProjectViewModel's resource tree
  // reads the live project service. Recent-card activation routes through
  // topbarOpenProject (upstream Plater::load_file).
  homeViewModel_->setProjectViewModel(projectViewModel_);
  projectViewModel_->setProjectService(projectService_);
  calibrationViewModel_ = new CalibrationViewModel(calibrationService_, this);
  calibrationViewModel_->setPresetService(presetService_);
  multiMachineViewModel_ = new MultiMachineViewModel(this);
  // Phase 201 (v5.6 AMS Architecture Cleanup): mock data + persistence for
  // AMSSettingsDialog. Data source stays mock; persistence is local QSettings.
  amsMaterialsViewModel_ = new AmsMaterialsViewModel(this);
  // v2.8 W3: application-level persisted settings (instance created before
  // the cloud data-plane assembly above; see the Cloud/Backend comment).
  // v2.8 W3: inject settings into SliceService for persisted bed size lookup.
  sliceService_->setAppSettings(appSettings_);
  // Phase 202 (v5.6 Plugin Manager UI Real Backend): plugin registry + mock
  // install/enable state with QSettings persistence under plugins/*.
  pluginService_ = new PluginService(this);

  // AI 助手（OWzx-only，docs/ai-control.md）：工具注册表把整个应用暴露为
  // JSON-Schema 工具；MCP 服务器在 127.0.0.1 上提供 tools/list + tools/call；
  // sidecar harness（Claude Agent SDK → GLM 智谱兼容端点）经 QProcess 接入。
  // BackendContext 本体实现 AppToolUiProvider 的三个 UI 动作。
  aiRegistry_ = new OWzx::AppToolRegistry(projectService_, sliceService_,
                                          configViewModel_, editorViewModel_,
                                          this);
  aiMcp_ = new OWzx::McpHttpServer(aiRegistry_, this);
  aiAgentService_ = new AiAgentService(this);
  aiViewModel_ = new AiViewModel(aiAgentService_, this);
  // Web chat page bridge (QWebChannel) — the WebEngine chat panel talks to
  // the same ViewModel/harness through this object. Registration happens in
  // attachAiChatChannel() once QML hands over its channel element.
  aiChatBridge_ = new AiChatBridge(aiViewModel_, aiAgentService_, this);
  // Preferences drive start/stop; re-apply on every settings change so the
  // sidebar reacts immediately to enable/key/model/port edits.
  connect(settingsViewModel_, &SettingsViewModel::settingsChanged, this,
          &BackendContext::applyAiSettings);
  applyAiSettings();

  // 初始化提示数据库（对齐上游 HintDatabase::init）
  initHintDatabase();
  m_hintTimer = new QTimer(this);
  m_hintTimer->setInterval(60000); // 每 60 秒尝试显示提示（对齐上游 30s 间隔，Mock 模式降低频率）
  connect(m_hintTimer, &QTimer::timeout, this, [this]()
          {
    if (m_hintsEnabled && m_notificationsEnabled &&
        lastErrorMessage_.isEmpty() && m_hints.size() > 0)
      postHint();
  });
  m_hintTimer->start();

  // Phase 241 (PAGE-04): periodic project backup (upstream backup_switch,
  // Preferences.cpp:1179 "Backup your project periodically for restoring
  // from the occasional crash"). Interval follows the autoSaveInterval
  // preference in minutes; each tick backs up only when autoSave is on AND
  // the project has unsaved changes (save-on-change semantics — documented
  // delta from the upstream seconds-level crash backup timer).
  m_backupTimer = new QTimer(this);
  m_backupTimer->setInterval(settingsViewModel_->autoSaveInterval() * 60 * 1000);
  connect(m_backupTimer, &QTimer::timeout, this, [this]()
          {
    if (settingsViewModel_ && settingsViewModel_->autoSave()
        && projectViewModel_ && projectViewModel_->isDirty())
      triggerProjectBackup();
  });
  connect(settingsViewModel_, &SettingsViewModel::settingsChanged, this, [this]()
          {
    if (m_backupTimer && settingsViewModel_)
      m_backupTimer->setInterval(
          qMax(1, settingsViewModel_->autoSaveInterval()) * 60 * 1000);
  });
  m_backupTimer->start();

  connect(projectService_, &ProjectService::loadFinished, this,
          [this](bool success, const QString &message)
          {
            // U3: the import progress toast retires on EVERY terminal state
            // (success / failure / user cancel). Success surfaces through the
            // editor refresh below, failure/cancel through the postError.
            retireNotificationType(NotiTypeProgressBar);
            if (success)
              clearError();
            else
              postError(message.isEmpty() ? tr("导入失败") : message, NotiWarning);
            // Phase 236 (DLG-03): after a successful load/drop, re-run the
            // outside-bed detection (upstream shows the outside-bed prompt on
            // import). A non-empty result raises the RecenterDialog.
            if (success && editorViewModel_ && editorViewModel_->checkObjectsOutsideBed() > 0)
              emit recenterPromptRequested();
            // G-13: adopt the in-project embedded presets carried by the
            // loaded 3MF (upstream Preset::is_project_embedded semantics).
            if (success && presetService_) {
              const QVariantList embedded = projectService_->takeProjectEmbeddedPresets();
              for (const QVariant &entryVar : embedded) {
                const QVariantMap entry = entryVar.toMap();
                QHash<QString, QVariant> values;
                const QVariantMap valuesMap = entry.value(QStringLiteral("values")).toMap();
                for (auto it = valuesMap.constBegin(); it != valuesMap.constEnd(); ++it)
                  values.insert(it.key(), it.value());
                presetService_->adoptProjectEmbeddedPreset(
                    entry.value(QStringLiteral("category")).toInt(),
                    entry.value(QStringLiteral("name")).toString(),
                    values,
                    entry.value(QStringLiteral("inherits")).toString());
              }
              if (!embedded.isEmpty() && configViewModel_)
                configViewModel_->refreshPresetLists();
            }
          });
  // U8: the 3MF write runs on the ProjectService save worker, so the
  // terminal outcomes land here. Project-save success applies the
  // ProjectViewModel bookkeeping that used to run inline in
  // topbarSaveProject(As) (dirty clear + recent projects; SaveAs to a NEW
  // path moves the current project identity), and both outcomes surface with
  // their real reason — a failed write is reported as a failure, never as
  // success (U1 severity discipline).
  connect(projectService_, &ProjectService::saveFinished, this,
          [this](bool success, const QString &filePath, const QString &error)
          {
            if (!projectViewModel_)
              return;
            if (success)
            {
              if (filePath != projectViewModel_->currentProjectPath())
                projectViewModel_->saveProjectAs(filePath);
              else
                projectViewModel_->saveProject();
              postError(tr("项目已保存到: %1").arg(QFileInfo(filePath).fileName()),
                        NotiSuccess);
            }
            else
            {
              postError(error.isEmpty() ? tr("项目保存失败") : error, NotiError);
            }
          });
  // U8: auto-backup snapshots report here (triggerProjectBackup dispatches
  // and returns; the archive outcome arrives asynchronously). U13: a
  // completion that belongs to an async 3MF model export is consumed by the
  // EditorViewModel first (it owns the dispatch attribution and the export
  // status/report) so an explicit export never surfaces as "Project backup".
  connect(projectService_, &ProjectService::snapshotFinished, this,
          [this](bool success, const QString &filePath, const QString &error)
          {
            if (editorViewModel_
                && editorViewModel_->consumePendingExportSnapshot(success, filePath, error))
              return;
            Q_UNUSED(error)
            if (success)
              postNotification(tr("Project backup saved: %1").arg(filePath),
                               tr("Auto backup"));
            else
              postError(tr("Project backup failed"), NotiError);
          });
  // U13: async model export + zip-archive import sequences report through
  // the EditorViewModel completion signals (the workers own the terminal
  // report, same discipline as the SliceService export family).
  connect(editorViewModel_, &EditorViewModel::modelExportFinished, this,
          [this](bool success, const QString &filePath, const QString &error)
          {
            if (success)
              postNotification(tr("模型已导出到: %1").arg(QFileInfo(filePath).fileName()),
                               tr("导出完成"), NotiSuccess);
            else
              postError(error.isEmpty() ? tr("模型导出失败") : error, NotiError);
          });
  connect(editorViewModel_, &EditorViewModel::archiveImportFinished, this,
          [this](int imported, int requested)
          {
            if (imported > 0)
              postNotification(tr("压缩包导入完成: %1/%2 个模型").arg(imported).arg(requested),
                               tr("导入完成"), NotiSuccess);
            else
              postError(tr("压缩包导入失败（%1 个模型均未加载）").arg(requested), NotiError);
          });
  // U8: fold an in-flight save into the canSave gate (and keep it fresh) so a
  // Ctrl+S during a background write is a silent no-op — with the gate open,
  // main.qml's `backend.canSave && !backend.topbarSaveProject()` fallback
  // would open the Save-As dialog on the busy rejection.
  connect(projectService_, &ProjectService::savingChanged, this,
          &BackendContext::stateChanged);

  // Phase 237 (VIEW-04): forward the editor viewmodel's post-import prompts.
  // Zero-volume removal mirrors upstream Model::removed_objects_with_
  // zero_volume + the Plater.cpp:4231-4233 info dialog (OWzx uses a
  // notification); the unit prompt re-emits so main.qml opens the confirm.
  if (editorViewModel_)
  {
    connect(editorViewModel_, &EditorViewModel::zeroVolumeObjectsRemoved, this,
            [this](int removedCount)
            {
              postNotification(tr("%1 object(s) with zero volume were removed.")
                                   .arg(removedCount),
                               tr("Objects with zero volume removed"));
            });
    connect(editorViewModel_, &EditorViewModel::unitConversionPromptRequested, this,
            &BackendContext::unitConversionPromptRequested);
  }

  // 切片进度通知（对齐上游 NotificationManager::SlicingProgressNotification）
  if (sliceService_)
  {
    connect(sliceService_, &SliceService::progressUpdated, this,
            [this](int percent, const QString &label)
            { postSlicingProgress(percent, label); });
    connect(sliceService_, &SliceService::sliceFinished, this,
            [this](const QString &estimatedTime)
            {
              Q_UNUSED(estimatedTime)
              postSlicingComplete();
              requestSelectTab(static_cast<int>(TabPosition::tpPreview));
            });
    connect(sliceService_, &SliceService::sliceFailed, this,
            [this](const QString &message)
            {
              // U3: a slice attempt that terminates as failed OR cancelled
              // must retire its live progress toast. The toasts posted below
              // are Custom-type entries, so they never hit the same-type
              // dedup that replaces the progress entry on success
              // (postSlicingComplete) -- without the retirement the
              // persistent "切片中" toast would linger forever.
              retireNotificationType(NotiTypeSlicingProgress);
              // U3: a user cancel arrives here with State::Cancelled (the
              // cancel-flag delivery path sets it, SliceService.cpp:1346).
              // Upstream renders the cancelled state as the plain SP_CANCELLED
              // message that fades (SlicingProgressNotification.hpp:19), not
              // the error block -- posting NotiError would call a deliberate
              // cancel a failure.
              if (sliceService_->sliceState() == SliceService::State::Cancelled)
                postNotification(message, tr("切片已取消"), NotiInfo);
              else
                postNotification(message, tr("切片失败"), NotiError);
            });
    // Phase 239 (ENGN-03): upstream distinguishes non-fatal Print::validate
    // warnings from errors (Plater.cpp:13742-13759 process_validation_warning
    // shows them as a notification while the slice continues). Errors keep
    // flowing through sliceFailed above.
    connect(sliceService_, &SliceService::validateWarning, this,
            [this](const QString &message)
            { postValidateWarning(message); });

    connect(sliceService_, &SliceService::exportStarted, this,
            [this](const QString &stage)
            { postExportOngoing(stage); });
    connect(sliceService_, &SliceService::exportFinished, this,
            [this](const QString &filePath)
            {
              // U3: retire the export-ongoing toast on the terminal signal.
              // The G-code copy progress reuses the progressUpdated channel
              // (SliceService copyGcodeChunked onProgress), which revives a
              // NotiTypeSlicingProgress entry mid-export; retire that too,
              // but only while the slice engine is idle -- a slice started
              // during the export owns a live progress toast that must
              // survive.
              retireNotificationType(NotiTypeExportOngoing);
              if (!sliceService_->slicing())
                retireNotificationType(NotiTypeSlicingProgress);
              postExportFinished(filePath);
            });
    connect(sliceService_, &SliceService::exportFailed, this,
            [this](const QString &message)
            {
              retireNotificationType(NotiTypeExportOngoing);
              if (!sliceService_->slicing())
                retireNotificationType(NotiTypeSlicingProgress);
              // Same cancel-vs-failure split as the slice path above: the
              // cancel-flag delivery sets State::Cancelled before emitting.
              if (sliceService_->sliceState() == SliceService::State::Cancelled)
                postNotification(message, tr("导出已取消"), NotiInfo);
              else
                postNotification(message, tr("Export failed"), NotiError);
            });

    // U3: 3MF 导入进度 toast。项目/模型加载在 QtConcurrent worker 里跑，
    // loadProgressUpdated 是它唯一的 GUI 线程进度通道；此前只喂
    // EditorViewModel::statusText，而 StatusBar 已随 layout-1 移除——大文件
    // 导入因此"白等"（UX 报告 U3 根因）。取消按钮经 cancelNotificationAction
    // 落到 ProjectService::cancelLoad 的取消标志。
    connect(projectService_, &ProjectService::loadProgressUpdated, this,
            [this](int progress, const QString &stage)
            { postImportProgress(progress, stage); });

    // Propagate 3MF embedded config to ConfigViewModel on project load
    connect(projectService_, &ProjectService::projectConfigLoaded,
            configViewModel_, &ConfigViewModel::applyProjectConfig);
    // Phase 236 (DLG-03): 3MF generator-version notice (upstream
    // Newer3mfVersion / MsgDataIncompatible warning family). projectVersionInfo
    // is parsed on the load worker, so the check is signal-driven; the notice
    // is a non-modal plater warning + log line rather than upstream's modal
    // (no update server — out of scope).
    connect(projectService_, &ProjectService::projectVersionInfoChanged, this, [this]()
            {
      if (!projectService_)
        return;
      const QString versionInfo = projectService_->projectVersionInfo();
      if (versionInfo.isEmpty() || versionInfo.startsWith(QStringLiteral("OWzx"), Qt::CaseInsensitive))
        return;
      qInfo("[Backend] 3mf generator=%s", versionInfo.toUtf8().constData());
      postPlaterWarning(tr("Project was saved by %1. Some settings may differ from this version.")
                            .arg(versionInfo));
    });
    // v5.16 (PLATE-05): real edits (objects/plates/configs) drive the
    // unsaved-changes indicator. Loads are excluded — ProjectViewModel's
    // openProject/newProject clear the flag after loadFinished anyway.
    // Phase 241 (PAGE-02): the same edits refresh the ProjectPage resource
    // tree so object add/delete/rename show up without a page re-entry.
    connect(projectService_, &ProjectService::projectChanged, this, [this]()
            {
              if (projectService_ && !projectService_->loading() && projectViewModel_)
              {
                projectViewModel_->markDirty();
                projectViewModel_->refreshFileTree();
              }
            });
  }

  // 实时监听偏好设置变化
  connect(settingsViewModel_, &SettingsViewModel::themeIndexChanged, this,
          [this]()
          { applyTheme(settingsViewModel_->themeIndex()); });
  connect(settingsViewModel_, &SettingsViewModel::uiScaleIndexChanged, this,
          [this]()
          { applyUiScale(settingsViewModel_->uiScaleIndex()); });
  connect(settingsViewModel_, &SettingsViewModel::fontSizeChanged, this,
          [this]()
          { applyFontSize(settingsViewModel_->fontSize()); });
  connect(settingsViewModel_, &SettingsViewModel::languageIndexChanged, this,
          [this]()
          { applyLanguage(settingsViewModel_->languageIndex()); });
  // 初始同步：启动时立即加载持久化的语言翻译（默认 zh_CN）。
  // applyLanguage 只连接到 languageIndexChanged 信号，而构造时 m_languageIndex
  // 默认已是 0 不会触发信号，导致程序以英文启动。此处补上初始调用。
  applyLanguage(settingsViewModel_->languageIndex());
  // S6 初始同步：同一道理——构造期不发 themeIndexChanged，而 ThemeState /
  // themeTokens 是 main.qml 加载前必须就位的（Theme.qml 的 token 绑定、
  // RhiViewport 图集变体都从它取值），所以构造尾部先跑一次分发。旧 kThemes
  // 链从未做过这次同步（壳层背景靠成员默认值），合并后这是唯一真值入口。
  applyTheme(settingsViewModel_->themeIndex());
  // Q11-① 初始同步（迁移语义）：仅当 pre-launch 的 ui/scaleFactor 真键尚
  // 不存在时，才把持久化的 uiScaleIndex 镜像进去——旧版本只写过
  // SettingsViewModel 的 index，从未写真键，补一次迁移。绝不能无条件写：
  // 命令面板（main.qml scale-* 项）直接 appSettings.setValue 写真键而不
  // 回写 index，启动时 main() 先读键生效、随后这里的无条件镜像会用陈旧
  // index 把面板直写的值覆写回去（双写者分叉，恰是 Q11 要消灭的双真值）。
  {
    QSettings uiStore(QSettings::NativeFormat, QSettings::UserScope,
                      QStringLiteral("OWzx"), QStringLiteral("UI"));
    if (!uiStore.contains(QStringLiteral("ui/scaleFactor")))
      applyUiScale(settingsViewModel_->uiScaleIndex());
  }

  // 同步撤销栈上限设置到 UndoRedoManager（对齐上游 UndoRedo::UndoRedoStackLimit）
  connect(settingsViewModel_, &SettingsViewModel::settingsChanged, this,
          [this, undoManager]()
          { undoManager->setUndoLimit(settingsViewModel_->undoLimit()); });
  // 初始同步
  undoManager->setUndoLimit(settingsViewModel_->undoLimit());

  // 同步通知偏好设置到 BackendContext（对齐上游 notification_manager preferences）
  connect(settingsViewModel_, &SettingsViewModel::settingsChanged, this,
          [this]()
          {
            m_notificationsEnabled = settingsViewModel_->notificationsEnabled();
            m_hintsEnabled = settingsViewModel_->hintsEnabled();
            m_autoDismissSec = settingsViewModel_->autoDismissSec();
            m_showProgressNotifications = settingsViewModel_->showProgressNotifications();
          });

  // v5.16 (P17.12): 同步“变暗下层”预览偏好到 PreviewViewModel（对齐上游
  // preview_dim_previous_layers，Preferences.cpp:1908-1915；GCodeViewer 在
  // 序贯视图刷层时对下层做 0.60 变暗，ViewerImpl.cpp:1225-1290）。
  connect(settingsViewModel_, &SettingsViewModel::settingsChanged, this,
          [this]()
          {
            if (previewViewModel_)
              previewViewModel_->setDimPreviousLayers(
                  settingsViewModel_->previewDimPreviousLayers());
          });
  if (previewViewModel_)
    previewViewModel_->setDimPreviousLayers(settingsViewModel_->previewDimPreviousLayers());

  // Load config wizard state from QSettings (对齐上游 ConfigWizard 首次运行检测)
  QSettings settings;
  m_configWizardCompleted = settings.value(QStringLiteral("wizard/completed"), false).toBool();

  // U04 (EnableLiteModeDialog): persisted G-code preview lite-mode flag
  // (fork-only QSettings key "gcode_preview_lite_mode" — no upstream counterpart).
  m_gcodePreviewLiteMode =
      settings.value(QStringLiteral("gcode_preview_lite_mode"), false).toBool();

  // Phase 4: Load sidebar dockable state from QSettings
  // 对齐上游 app_config collapsed_sidebar；width/dockArea 为增强（上游无）
  // 命名空间 owzx/sidebar/* 避免与其它 key 冲突
  sidebarCollapsed_ = settings.value(QStringLiteral("owzx/sidebar/collapsed"), false).toBool();
  const QVariant savedSidebarWidthValue = settings.value(QStringLiteral("owzx/sidebar/width"));
  int savedWidth = savedSidebarWidthValue.isValid()
                     ? savedSidebarWidthValue.toInt()
                     : kSidebarDefaultWidth;
  const int sidebarSettingsVersion =
      settings.value(QStringLiteral("owzx/sidebar/settingsVersion"), 1).toInt();
  if (sidebarSettingsVersion < kSidebarSettingsVersion &&
      savedSidebarWidthValue.isValid()) {
    savedWidth = kSidebarDefaultWidth;
    settings.setValue(QStringLiteral("owzx/sidebar/width"), savedWidth);
    settings.setValue(QStringLiteral("owzx/sidebar/settingsVersion"), kSidebarSettingsVersion);
    settings.sync();
  }
  sidebarWidth_ = qBound(kSidebarMinWidth, savedWidth, kSidebarMaxWidth);  // clamp 防御损坏的存储值
  const int savedArea = settings.value(QStringLiteral("owzx/sidebar/dockArea"),
                                       static_cast<int>(SidebarDockArea::Left)).toInt();
  sidebarDockArea_ = (savedArea == static_cast<int>(SidebarDockArea::Right))
                       ? SidebarDockArea::Right
                       : SidebarDockArea::Left;  // 防御：非 Right 一律按 Left
}

BackendContext::~BackendContext()
{
  // U4: release the override cursor if the app tears down mid-long-operation
  // (shutdown during a busy scope or an in-flight worker). The aggregate may
  // legitimately still read busy at destruction (the workers outlive the
  // context's guards), so restore directly instead of going through
  // updateGuiBusyCursor's transition logic.
  if (guiBusyActive_) {
    QGuiApplication::restoreOverrideCursor();
    guiBusyActive_ = false;
  }
  // Review P1-2: Qt deletes children in creation order, so ProjectService
  // (created first) would die before SliceService (created second) -- but
  // ~SliceService joins the slice worker, and that worker dereferences
  // projectService_. Delete the slice service (and its in-flight worker join)
  // while the project service is still alive.
  delete sliceService_;
  sliceService_ = nullptr;
}

bool BackendContext::openExternalGcode(const QString &filePath)
{
  // GAP-6 (upstream Plater::load_gcode, Plater.cpp:10037-10058): an
  // externally sliced .gcode opens as a NEW project in a preview-only
  // context -- the workspace resets (upstream load_gcode clears the plater)
  // and the Preview page consumes the file through PreviewViewModel's
  // parser. The plate grid stays empty, so there is nothing to slice or
  // export until the user imports models again.
  if (filePath.isEmpty() || !QFileInfo::exists(filePath))
    return false;
  if (!previewViewModel_)
    return false;
  if (editorViewModel_)
    editorViewModel_->clearWorkspace();
  {
    // U4: rebuildFromGCode parses the (possibly multi-hundred-MB) G-code on
    // the GUI thread. The RAII scope applies the central busy cursor for the
    // parse and releases it on every path -- the early return below included.
    BusyWorkScope busyScope(this);
    if (!previewViewModel_->loadGCodeForPreview(filePath))
      return false;
  }
  requestSelectTab(tpPreview());
  return true;
}

bool BackendContext::dirtyProjectDiscardRemembered() const
{
  // GAP-7: upstream Plater.cpp:11125-11140 persists "save_project_choise".
  QSettings settings;
  return settings.value(QStringLiteral("owzx/project/discardWarningRemembered"),
                        false)
      .toBool();
}

void BackendContext::setDirtyProjectDiscardRemembered(bool remembered)
{
  {
    QSettings settings;
    settings.setValue(QStringLiteral("owzx/project/discardWarningRemembered"),
                      remembered);
  }
  emit dirtyProjectDiscardRememberedChanged();
}

QObject *BackendContext::editorViewModel() const { return editorViewModel_; }
QObject *BackendContext::sliceService() const { return sliceService_; }

QObject *BackendContext::aiViewModel() const { return aiViewModel_; }

QObject *BackendContext::aiChatBridge() const { return aiChatBridge_; }

void BackendContext::attachAiChatChannel(QObject *channel) {
  // channel is the QML WebChannel element (QQmlWebChannel) hosting the chat
  // page's transport; QQmlWebChannel derives from QWebChannel, so the public
  // base-API registration applies. The page resolves it as
  // channel.objects.bridge right after its own load.
  auto *webChannel = qobject_cast<QWebChannel *>(channel);
  if (!webChannel) {
    qWarning("[Backend] attachAiChatChannel: not a QWebChannel instance");
    return;
  }
  webChannel->registerObject(QStringLiteral("bridge"), aiChatBridge_);
}

void BackendContext::applyAiSettings()
{
  const bool enabled = settingsViewModel_->aiEnabled()
      && !settingsViewModel_->aiApiKey().isEmpty();

  if (!enabled) {
    aiAgentService_->stop();
    aiMcp_->stop();
    if (aiControlActive_) {
      aiControlActive_ = false;
      emit stateChanged();
    }
    return;
  }

  // Loopback MCP server first (the sidecar connects to it at startup).
  if (!aiMcp_->isActive()) {
    if (!aiMcp_->start(quint16(settingsViewModel_->aiPort()),
                       settingsViewModel_->aiControlToken())) {
      aiAgentService_->stop();
      postError(QStringLiteral("AI MCP 服务器启动失败（端口被占用？）"),
                NotiWarning);
      if (aiControlActive_) {
        aiControlActive_ = false;
        emit stateChanged();
      }
      return;
    }
  }

  // Sidecar layout: <appDir>/ai_sidecar/{agent.py, python/python.exe}
  // (assembled by scripts/package_ai_sidecar.ps1; optional component).
  const QString appDir = QCoreApplication::applicationDirPath();
  const QString sidecarDir = QDir(appDir).filePath(QStringLiteral("ai_sidecar"));
  AiAgentService::Config cfg;
  cfg.sidecarScriptPath = QDir(sidecarDir).filePath(QStringLiteral("agent.py"));
  cfg.pythonPath = QDir(sidecarDir).filePath(QStringLiteral("python/python.exe"));
  cfg.mcpUrl = QStringLiteral("http://127.0.0.1:%1/mcp")
                   .arg(aiMcp_->port());
  cfg.mcpToken = settingsViewModel_->aiControlToken();
  cfg.model = settingsViewModel_->aiModel();
  cfg.anthropicBaseUrl = settingsViewModel_->aiBaseUrl();
  cfg.apiKey = settingsViewModel_->aiApiKey();
  aiAgentService_->configure(cfg);
  if (!aiAgentService_->running())
    aiAgentService_->start();

  aiViewModel_->setEnabled(true);
  if (!aiControlActive_) {
    aiControlActive_ = true;
    emit stateChanged();
  }
}

bool BackendContext::switchPage(int position)
{
  if (position < 0 || position > 8)
    return false;
  requestSelectTab(position);
  return true;
}

bool BackendContext::toggleSidebar()
{
  requestToggleSidebar();
  return true;
}
QObject *BackendContext::previewViewModel() const { return previewViewModel_; }
QObject *BackendContext::monitorViewModel() const { return monitorViewModel_; }
QObject *BackendContext::configViewModel() const { return configViewModel_; }
QObject *BackendContext::homeViewModel() const { return homeViewModel_; }
QObject *BackendContext::settingsViewModel() const { return settingsViewModel_; }
QObject *BackendContext::projectViewModel() const { return projectViewModel_; }
QObject *BackendContext::calibrationViewModel() const { return calibrationViewModel_; }
QObject *BackendContext::multiMachineViewModel() const { return multiMachineViewModel_; }
QObject *BackendContext::amsMaterialsViewModel() const { return amsMaterialsViewModel_; }
QObject *BackendContext::appSettings() const { return appSettings_; }

QObject *BackendContext::pluginService() const { return pluginService_; }

// Phase 199 (WIZ-01): expose the preset data service so the ConfigWizard can
// enumerate vendors / printer models / materials / bed surfaces.
QObject *BackendContext::presetService() const { return presetService_; }

bool BackendContext::visualCompareMode() const
{
  return visualCompareMode_;
}

int BackendContext::currentPage() const
{
  return currentPage_;
}

// ── Phase 51: shell action gates (forward to EditorViewModel/PreviewViewModel) ──
// These gates aggregate upstream-aligned viewmodel state so the shell binds a
// single property per action (SHELL-03: action states live in C++, not QML-only).

bool BackendContext::isSlicing() const
{
  // Aggregate slice state across both viewmodels (editor prepares, preview renders).
  return (editorViewModel_ && editorViewModel_->isSlicing())
      || (previewViewModel_ && previewViewModel_->slicing());
}

bool BackendContext::isBusy() const
{
  // Any blocking operation in flight: slicing or mid-load.
  return isSlicing() || (editorViewModel_ && editorViewModel_->loading());
}

// ── U4: central busy-cursor manager ──────────────────────────────────────────
// Upstream marks long operations with wxBusyCursor / progress notifications
// (Plater::on_action_arrange...); this codebase had no busy signal at all
// (UX scan 2026-10-11 finding 4). One aggregate + one transition point here
// cover all four long-operation families; EditorViewModel exposes the state
// for its synchronous GUI-blocking runs (arrangement / export writes /
// drag-drop file parse) because the cursor must be applied BEFORE the GUI
// thread blocks -- a post-hoc state flip after the blocking call would never
// be observed until the work is already done.

/// U4: the long-operation aggregate. Any true term applies the busy cursor.
bool BackendContext::guiBusyNow() const
{
  return busyWorkDepth_ > 0
      || isSlicing()
      || (editorViewModel_ && editorViewModel_->loading())
      || (editorViewModel_ && editorViewModel_->importing())
      || (editorViewModel_ && editorViewModel_->arranging())
      || (editorViewModel_ && editorViewModel_->exporting())
      || (sliceService_ && sliceService_->exportBusy());
}

/// U4: apply/remove the override cursor exactly on an aggregate transition.
/// All state drivers (worker signals, viewmodel stateChanged umbrellas, busy
/// scopes) funnel through here, so success, failure, cancel and early-return
/// paths all restore the cursor by construction.
void BackendContext::updateGuiBusyCursor()
{
  const bool busy = guiBusyNow();
  if (busy == guiBusyActive_)
    return;
  guiBusyActive_ = busy;
  if (busy)
    QGuiApplication::setOverrideCursor(Qt::BusyCursor);
  else
    QGuiApplication::restoreOverrideCursor();
  emit stateChanged();
}

bool BackendContext::guiBusy() const
{
  return guiBusyActive_;
}

void BackendContext::beginBusyWork()
{
  ++busyWorkDepth_;
  updateGuiBusyCursor();
}

void BackendContext::endBusyWork()
{
  if (busyWorkDepth_ > 0)
    --busyWorkDepth_;
  updateGuiBusyCursor();
}

bool BackendContext::canImport() const
{
  // Import available whenever no blocking op is in flight (mirrors upstream File menu).
  return !isBusy();
}

bool BackendContext::canSlice() const
{
  return editorViewModel_ && editorViewModel_->canRequestSlice() && !isSlicing();
}

bool BackendContext::canExport() const
{
  return editorViewModel_ && editorViewModel_->canExportGCode() && !isSlicing();
}

bool BackendContext::canSave() const
{
  // Mutating the project mid-slice is unsafe (CONTEXT safety decision).
  // U8: an in-flight async save gates too — the service busy-rejects a second
  // job, and main.qml pairs `backend.canSave && !backend.topbarSaveProject()`
  // so an open gate on a rejection would open the Save-As dialog.
  return !isSlicing() && !isBusy()
         && !(projectService_ && projectService_->isSaving());
}

bool BackendContext::canUndo() const
{
  // Raw undo-stack availability; the page gate (currentPage === tp3DEditor)
  // is applied in QML in Plan 51-02.
  // Phase 90 (ASMROUTE-01): the UndoRedoManager is a single shared stack
  // (90-CONTEXT.md decision 8), so undo/redo operates on the active canvas
  // regardless of view mode — mirroring upstream Plater.cpp:11744 (undo entry
  // routing) and Plater.cpp:11823 (selection passed to undo/redo). When
  // currentViewMode_ == ViewMode::AssembleView (CanvasAssembleView=2) the
  // stack still routes through the same editorViewModel_; no isolated
  // per-canvas stack is created.
  return editorViewModel_ && editorViewModel_->canUndo();
}

bool BackendContext::canRedo() const
{
  // Phase 90 (ASMROUTE-01): see canUndo() — shared single undo stack applies
  // to the AssembleView canvas host the same as Prepare/Preview.
  return editorViewModel_ && editorViewModel_->canRedo();
}

QString BackendContext::exportActionLabel() const
{
  // Stable label; exportActionHint() carries the blocked reason.
  return tr("Export G-code");
}

QString BackendContext::exportActionHint() const
{
  // No hint once a slice result exists; otherwise forward the viewmodel reason.
  if (editorViewModel_ && editorViewModel_->hasSliceResult())
    return {};
  return editorViewModel_ ? editorViewModel_->exportActionHint() : QString{};
}

QString BackendContext::saveActionLabel() const
{
  return tr("Save Project");
}

QString BackendContext::saveActionHint() const
{
  return (isSlicing() || isBusy()) ? tr("busy") : QString{};
}

void BackendContext::setCurrentPage(int page)
{
  if (currentPage_ == page)
    return;
  currentPage_ = page;
  emit currentPageChanged();
}

void BackendContext::requestSelectTab(int position)
{
  // 越界拒绝（对齐 Pitfall A3 — 防止 StackLayout currentIndex 越界破坏绑定）
  // Use the final semantic tab value instead of a numeric bound.
  constexpr int kLastTab = static_cast<int>(TabPosition::tpPreferences);
  if (position < 0 || position > kLastTab) {
    qWarning("[Backend] requestSelectTab: invalid position %d (valid 0..%d)", position, kLastTab);
    return;
  }
  // 先广播信号（对齐上游 wxQueueEvent 语义：消费者在 tab 实际切换前看到事件）
  emit tabSelectRequested(position);
  // 再切换页面（setCurrentPage 已含去重 + emit currentPageChanged）
  setCurrentPage(position);
  // Phase 3: tab 联动 viewMode（对齐上游 Plater 在 Notebook 切 tab 时自动切 view3D/preview）
  // tp3DEditor → View3D；tpPreview → Preview；其他 tab 不改 viewMode（切回 Plater 时维持上次视图）
  if (position == static_cast<int>(TabPosition::tp3DEditor))
    setCurrentViewMode(static_cast<int>(ViewMode::View3D));
  else if (position == static_cast<int>(TabPosition::tpPreview))
    setCurrentViewMode(static_cast<int>(ViewMode::Preview));
}

void BackendContext::setCurrentViewMode(int mode)
{
  // 内联 setter：供 requestSelectTab 联动 + requestChangeViewMode 复用
  // 越界拒绝（对齐 Pitfall A3）
  constexpr int kLastVm = static_cast<int>(ViewMode::AssembleView);
  if (mode < 0 || mode > kLastVm) {
    qWarning("[Backend] setCurrentViewMode: invalid mode %d (valid 0..%d)", mode, kLastVm);
    return;
  }
  if (static_cast<int>(currentViewMode_) == mode)
    return;
  currentViewMode_ = static_cast<ViewMode>(mode);
  emit currentViewModeChanged();
  // Phase 90 (ASMROUTE-01): push the active canvas type to EditorViewModel so
  // selection / gizmo / undo-redo routing branches on the active canvas,
  // mirroring upstream Plater.cpp:7322 (CanvasAssembleView render branch on
  // selection change) and Plater.cpp:11744/11823 (undo/redo routing). The
  // ViewMode value equals the RhiViewport::CanvasType value
  // (View3D=0/CanvasView3D, Preview=1/CanvasPreview, AssembleView=2/
  // CanvasAssembleView). When AssembleView is active the shared UndoRedoManager
  // (single stack) routes to the AssembleView canvas host.
  if (editorViewModel_)
    editorViewModel_->setActiveCanvasType(static_cast<int>(currentViewMode_));
}

void BackendContext::requestChangeViewMode(int mode)
{
  // 越界拒绝
  constexpr int kLastVm = static_cast<int>(ViewMode::AssembleView);
  if (mode < 0 || mode > kLastVm) {
    qWarning("[Backend] requestChangeViewMode: invalid mode %d (valid 0..%d)", mode, kLastVm);
    return;
  }
  // 先广播（对齐 tabSelectRequested 语义）
  emit viewModeChangeRequested(mode);
  setCurrentViewMode(mode);
}

// ── Phase 4: Sidebar Dockable 操作（对齐上游 collapse_sidebar + 持久化增强）──
// 设计：所有 request 方法去重 + emit NOTIFY + 持久化到 QSettings owzx/sidebar/*。
// 对齐上游 Plater.cpp:4452 collapse_sidebar(bool) 的显隐语义。

void BackendContext::requestToggleSidebar()
{
  // 对齐上游 EVT_GLCANVAS_COLLAPSE_SIDEBAR 触发的 toggle 语义
  requestSetSidebarCollapsed(!sidebarCollapsed_);
}

void BackendContext::requestSetSidebarCollapsed(bool c)
{
  if (sidebarCollapsed_ == c)
    return;  // 去重
  sidebarCollapsed_ = c;
  QSettings settings;
  settings.setValue(QStringLiteral("owzx/sidebar/collapsed"), c);  // 持久化（对齐上游 app_config）
  settings.sync();
  emit sidebarCollapsedChanged();
}

void BackendContext::requestSetSidebarWidth(int w)
{
  // clamp 到 [min, max]，防御 QML 传入越界值
  const int clamped = qBound(kSidebarMinWidth, w, kSidebarMaxWidth);
  if (sidebarWidth_ == clamped)
    return;  // 去重（含 clamp 后相等）
  sidebarWidth_ = clamped;
  QSettings settings;
  settings.setValue(QStringLiteral("owzx/sidebar/width"), clamped);  // 持久化
  settings.setValue(QStringLiteral("owzx/sidebar/settingsVersion"), kSidebarSettingsVersion);
  settings.sync();
  emit sidebarWidthChanged();
}

void BackendContext::requestSetSidebarDockArea(int area)
{
  // 防御：非 Right 一律按 Left（避免越界枚举值破坏 QML 绑定）
  const SidebarDockArea newArea = (area == static_cast<int>(SidebarDockArea::Right))
                                    ? SidebarDockArea::Right
                                    : SidebarDockArea::Left;
  if (sidebarDockArea_ == newArea)
    return;  // 去重
  sidebarDockArea_ = newArea;
  QSettings settings;
  settings.setValue(QStringLiteral("owzx/sidebar/dockArea"), static_cast<int>(newArea));  // 持久化
  settings.sync();
  emit sidebarDockAreaChanged();
}
void BackendContext::resetWindowLayout()
{
  requestSetSidebarCollapsed(false);
  requestSetSidebarWidth(kSidebarDefaultWidth);
  requestSetSidebarDockArea(static_cast<int>(SidebarDockArea::Left));
}

void BackendContext::showPreferencesDialog()
{
  emit showPreferencesDialogRequested();
}

// ── ConfigWizard first-run trigger ──

bool BackendContext::configWizardCompleted() const
{
  return m_configWizardCompleted;
}

void BackendContext::setConfigWizardCompleted(bool completed)
{
  if (m_configWizardCompleted == completed)
    return;
  m_configWizardCompleted = completed;

  // Persist to QSettings (对齐上游 app_config 首次启动标记)
  QSettings settings;
  settings.setValue(QStringLiteral("wizard/completed"), completed);

  emit configWizardCompletedChanged();
}

// ── U04 (EnableLiteModeDialog): persisted G-code preview lite-mode flag ──

bool BackendContext::gcodePreviewLiteMode() const
{
  return m_gcodePreviewLiteMode;
}

void BackendContext::setGcodePreviewLiteMode(bool enabled)
{
  if (m_gcodePreviewLiteMode == enabled)
    return; // 去重（同 requestSetSidebarCollapsed 模式）
  m_gcodePreviewLiteMode = enabled;

  // Persist to QSettings (fork-only key "gcode_preview_lite_mode").
  QSettings settings;
  settings.setValue(QStringLiteral("gcode_preview_lite_mode"), enabled);
  settings.sync();

  emit gcodePreviewLiteModeChanged();
}

void BackendContext::showConfigWizard()
{
  emit showConfigWizardRequested();
}

void BackendContext::forwardSettingsRequest(const QString &category)
{
  // Phase 56: show/create the correct SettingsDialog for this category.
  // Dialog instances are created once and shown/hidden (not destroyed per open).
  // Step 1: ensure the active preset tier is set before any consumer reads it.
  configViewModel_->setActivePresetTier(category);
  // Step 2: emit signal; QML side connects to create/show the dialog.
  emit settingsRequested(category);
}

void BackendContext::showBedShapeDialog()
{
  emit showBedShapeDialogRequested();
}

void BackendContext::showEditGCodeDialog(const QString &key, const QString &value)
{
  emit showEditGCodeDialogRequested(key, value);
}

void BackendContext::showAMSSettingsDialog()
{
  emit showAMSSettingsDialogRequested();
}

void BackendContext::showFirmwareDialog()
{
  emit showFirmwareDialogRequested();
}

void BackendContext::showWipeTowerDialog()
{
  emit showWipeTowerDialogRequested();
}

void BackendContext::showPrintHostDialog()
{
  emit showPrintHostDialogRequested();
}

void BackendContext::showPluginManagerDialog()
{
  emit showPluginManagerDialogRequested();
}

void BackendContext::showEnableLiteModeDialog()
{
  emit showEnableLiteModeDialogRequested();
}

void BackendContext::showExportPresetBundleDialog()
{
  // Phase 236 (DLG-01): File > Export Preset Bundle entry point (upstream
  // Mainframe menu opens ExportPresetBundleDialog).
  emit showExportPresetBundleDialogRequested();
}

void BackendContext::showSysInfoDialog()
{
  // Phase 236 (DLG-03): Help menu entry for the system-info dump (upstream
  // Help > About > System Information opens a SysInfoDialog).
  emit showSysInfoDialogRequested();
}

QVariantMap BackendContext::systemInfo() const
{
  // Phase 236 (DLG-03): runtime environment dump for SysInfoDialog —
  // compile-time constants, graphics-API/GL strings (when an OpenGL context
  // is current), and the key config paths.
  QVariantMap info;
  info.insert(QStringLiteral("appName"), QStringLiteral("OWzx Slicer"));
  info.insert(QStringLiteral("appVersion"), QStringLiteral("2.4.0-dev (Qt6 QML)"));
  info.insert(QStringLiteral("qtVersion"), QString::fromLatin1(qVersion()));
  info.insert(QStringLiteral("buildDate"), QString::fromLatin1(__DATE__));
  info.insert(QStringLiteral("platform"),
              QSysInfo::prettyProductName() + QStringLiteral(" [") + QSysInfo::buildCpuArchitecture() +
                  QStringLiteral("]"));
  // U04: total physical RAM in MB — upstream SysInfoDialog "Total RAM size
  // [MB]" (SysInfoDialog.cpp:59 via total_physical_memory()); the Qt6 build
  // samples it with GlobalMemoryStatusEx on Windows.
#ifdef Q_OS_WINDOWS
  {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo))
      info.insert(QStringLiteral("totalRamMb"),
                  static_cast<qint64>(memInfo.ullTotalPhys / (1024 * 1024)));
  }
#endif
  // U04: build commit injected by the build system (consumed by the
  // AboutDialog "Build" line, upstream SysInfoDialog.cpp:47 SLIC3R_BUILD_ID);
  // degrades to an empty string when the macro is absent.
#ifdef OWZX_BUILD_COMMIT
  info.insert(QStringLiteral("buildCommit"), QString::fromLatin1(OWZX_BUILD_COMMIT));
#else
  info.insert(QStringLiteral("buildCommit"), QString());
#endif
  // Graphics API selected by the QML runtime (OpenGL is forced at startup).
  // Qt6: QSGRendererInterface::GraphicsApi is not a Q_ENUM, so QMetaEnum
  // cannot stringify it — map manually (main_qml.cpp forces OpenGL).
  const QSGRendererInterface::GraphicsApi api = QQuickWindow::graphicsApi();
  const QString graphicsApi =
      api == QSGRendererInterface::OpenGL ? QStringLiteral("OpenGL") :
      api == QSGRendererInterface::Vulkan ? QStringLiteral("Vulkan") :
      api == QSGRendererInterface::Direct3D11 ? QStringLiteral("Direct3D11") :
      api == QSGRendererInterface::Direct3D12 ? QStringLiteral("Direct3D12") :
      api == QSGRendererInterface::Metal ? QStringLiteral("Metal") :
      api == QSGRendererInterface::Software ? QStringLiteral("Software") :
      api == QSGRendererInterface::Null ? QStringLiteral("Null") :
      QStringLiteral("Unknown");
  info.insert(QStringLiteral("graphicsApi"), graphicsApi);
  // Surface format actually used by the render loop.
  const QSurfaceFormat format = QSurfaceFormat::defaultFormat();
  info.insert(QStringLiteral("surfaceFormat"),
              QStringLiteral("OpenGL %1.%2 %3")
                  .arg(format.majorVersion())
                  .arg(format.minorVersion())
                  .arg(format.profile() == QSurfaceFormat::CoreProfile
                           ? QStringLiteral("Core")
                           : QStringLiteral("Compatibility")));
  // GL vendor/renderer: only readable with a current context; the QML scene
  // thread usually owns one. Missing values degrade to "n/a".
  QString glVendor = QStringLiteral("n/a");
  QString glRenderer = QStringLiteral("n/a");
  QString glVersion = QStringLiteral("n/a");
  QString glslVersion = QStringLiteral("n/a");
  if (QOpenGLContext *context = QOpenGLContext::currentContext())
  {
    QOpenGLFunctions functions(context);
    functions.initializeOpenGLFunctions();
    const unsigned char *vendor = functions.glGetString(GL_VENDOR);
    const unsigned char *renderer = functions.glGetString(GL_RENDERER);
    const unsigned char *version = functions.glGetString(GL_VERSION);
    const unsigned char *glsl = functions.glGetString(GL_SHADING_LANGUAGE_VERSION);
    if (vendor)
      glVendor = QString::fromLatin1(reinterpret_cast<const char *>(vendor));
    if (renderer)
      glRenderer = QString::fromLatin1(reinterpret_cast<const char *>(renderer));
    if (version)
      glVersion = QString::fromLatin1(reinterpret_cast<const char *>(version));
    if (glsl)
      glslVersion = QString::fromLatin1(reinterpret_cast<const char *>(glsl));
  }
  info.insert(QStringLiteral("glVendor"), glVendor);
  info.insert(QStringLiteral("glRenderer"), glRenderer);
  info.insert(QStringLiteral("glVersion"), glVersion);
  // U04: GLSL version (upstream get_gl_info reports it in the OpenGL info
  // block, SysInfoDialog.cpp:186).
  info.insert(QStringLiteral("glslVersion"), glslVersion);
  // Key configuration paths.
  info.insert(QStringLiteral("appDataLocation"),
              QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
  if (presetService_)
    info.insert(QStringLiteral("userPresetDir"), presetService_->userPresetDir());
  return info;
}

void BackendContext::topbarNewProject()
{
  const qint64 start = m_latencyClock.elapsed();
  // clearWorkspace BEFORE newProject: clearProject() emits projectChanged and
  // the blanket markDirty() handler below would re-dirty the fresh empty
  // project, making the very next open/quit prompt a bogus unsaved-changes
  // guard. newProject() runs last so isDirty=false is the final state.
  if (editorViewModel_)
    editorViewModel_->clearWorkspace();
  if (projectViewModel_)
    projectViewModel_->newProject();
  setCurrentPage(1);
  pushLatencySample(QStringLiteral("topbar-new-project"), int(m_latencyClock.elapsed() - start), QString());
}

void BackendContext::applyStartupPagePreference()
{
  // Phase 241 (PAGE-04): the persisted startup-page preference now actually
  // drives the initial page. showHomePage=false + defaultPage=0 (Home) still
  // lands on Home — only showHomePage=false + defaultPage=1 (Prepare) starts
  // on the editor, matching the two upstream app_config keys
  // "show_home_page" / "default_page" consumed at MainFrame startup.
  if (!settingsViewModel_)
    return;
  const int target = (settingsViewModel_->showHomePage()
                          || settingsViewModel_->defaultPage() == 0)
                         ? static_cast<int>(TabPosition::tpHome)
                         : static_cast<int>(TabPosition::tp3DEditor);
  setCurrentPage(target);
}

QString BackendContext::triggerProjectBackup()
{
  // Phase 241 (PAGE-04): write one project snapshot into the app-data backup
  // directory. Upstream backup_switch copies the live project on a
  // seconds-level crash timer (Preferences.cpp:1179); the OWzx delta is a
  // periodic save-on-change backup (minute granularity, driven by the
  // autoSave/autoSaveInterval preferences) — the setting label says exactly
  // that. Returns the backup path, or an empty string when there is no
  // project content to back up or the write failed.
  if (!projectService_ || projectService_->modelCount() <= 0)
    return QString();
  const QString backupDir = QStandardPaths::writableLocation(
                                QStandardPaths::AppDataLocation)
                            + QStringLiteral("/backup");
  if (!QDir().mkpath(backupDir))
    return QString();
  const QString stamp = QDateTime::currentDateTime().toString(
      QStringLiteral("yyyyMMdd-HHmmss"));
  const QString base = projectService_->projectName().isEmpty()
                           ? QStringLiteral("project")
                           : projectService_->projectName();
  // ASCII-safe file-stem sanitize (keep CJK letters, drop path separators).
  QString safeBase;
  for (const QChar &c : base)
    safeBase += (c.isLetterOrNumber() || c == QLatin1Char('-')
                 || c == QLatin1Char('_'))
                    ? c
                    : QLatin1Char('_');
  if (safeBase.isEmpty())
    safeBase = QStringLiteral("project");
  const QString backupPath = backupDir + QStringLiteral("/") + safeBase
                             + QStringLiteral("-") + stamp + QStringLiteral(".3mf");
  // writeProjectSnapshot keeps currentProjectPath_ intact (a backup must not
  // hijack the user's current project file). U8: the snapshot write runs on
  // the save worker (writeProjectSnapshotAsync) so the autoSave tick never
  // stalls the GUI thread mid-frame; the archive outcome lands via
  // snapshotFinished (saved/failed notifications moved there). Returning the
  // path now means "a backup was issued for this path". A busy save engine
  // defers silently — the next autoSave tick retries — instead of raising a
  // false failure for a merely postponed backup.
  if (!projectService_->writeProjectSnapshotAsync(backupPath))
  {
    if (projectService_->isSaving())
      return QString();
    postError(tr("Project backup failed"), NotiError);
    return QString();
  }
  return backupPath;
}

bool BackendContext::topbarOpenProject(const QString &filePath)
{
  const qint64 start = m_latencyClock.elapsed();
  const QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  if (localPath.isEmpty())
    return false;

  // U8 复核（阻塞 1）: 在飞保存持有工程身份——其交付会把派发时工程的记账
  // 盖到这里的加载结果上。service 层 loadProject/loadFile 已加 saving_ 忙拒；
  // 这里提前拒绝的原因是双重的：省得用户等加载失败，以及防止下方的
  // loadProject→editor 导入回退链把忙拒绝误当坏文件、把工程当模型导入。
  if (projectService_ && projectService_->isSaving())
  {
    postError(tr("保存任务进行中，暂无法打开项目"), NotiWarning);
    pushLatencySample(QStringLiteral("topbar-open-project"), int(m_latencyClock.elapsed() - start), localPath);
    return false;
  }

  // Phase 237 (VIEW-05): .3mf/.cxprj project opens route through
  // ProjectService::loadProject, which loads with LoadModel|LoadConfig
  // and emits projectConfigLoaded so the embedded config/presets apply
  // (upstream Plater::load_project / load_files is_project_file path;
  // the wiring above forwards projectConfigLoaded to
  // ConfigViewModel::applyProjectConfig). Plain model imports keep the
  // loadFile path (LoadModel only).
  const QString suffix = QFileInfo(localPath).suffix().toLower();
  bool loaded = false;
  if ((suffix == QStringLiteral("3mf") || suffix == QStringLiteral("cxprj"))
      && projectService_)
  {
    loaded = projectService_->loadProject(localPath);
    if (loaded && editorViewModel_)
      editorViewModel_->refreshAfterLoad();
  }
  if (!loaded)
    loaded = editorViewModel_ ? editorViewModel_->loadFile(localPath) : false;

  // In Mock mode (no libslic3r), loadFile() fails for 3MF — try JSON project load
  if (!loaded && projectService_)
  {
    loaded = projectService_->loadProject(localPath);
    if (loaded && editorViewModel_)
    {
      editorViewModel_->refreshAfterLoad();
      setCurrentPage(1);
    }
  }

  if (loaded)
  {
    if (projectViewModel_)
      projectViewModel_->openProject(localPath);
    setCurrentPage(1);
    // Phase 236 (DLG-03): the 3MF version notice is emitted by the
    // projectVersionInfoChanged connection above (the parse runs on the
    // load worker thread; this call site would race it).
  }
  pushLatencySample(QStringLiteral("topbar-open-project"), int(m_latencyClock.elapsed() - start), localPath);
  return loaded;
}

// Phase 237 (VIEW-02): Import Configs (upstream MainFrame::load_config_file,
// MainFrame.cpp:3204-3247). The upstream file dialog accepts
// *.json/zip/orca_* files; the OWzx import surface with a real consumer is
// the .json bundle (PresetService::importBundle), so the QML dialog
// filters .json and this entry routes there. The import result surfaces as
// a notification (upstream shows a summary dialog).
bool BackendContext::topbarImportConfigs(const QString &filePath)
{
  const qint64 start = m_latencyClock.elapsed();
  const QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  if (localPath.isEmpty() || !presetService_)
    return false;

  const bool imported = presetService_->importBundle(localPath);
  if (imported)
    postNotification(tr("There is 1 config imported."),
                     tr("Import result"));
  else
    postError(tr("Config import failed: %1").arg(QFileInfo(localPath).fileName()), NotiError);
  pushLatencySample(QStringLiteral("topbar-import-configs"),
                    int(m_latencyClock.elapsed() - start), localPath);
  return imported;
}

bool BackendContext::topbarImportModel(const QString &filePath)
{
  const qint64 start = m_latencyClock.elapsed();
  const QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  if (localPath.isEmpty())
    return false;

  const bool loaded = editorViewModel_ ? editorViewModel_->loadFile(localPath) : false;
  if (loaded)
  {
    if (projectViewModel_)
    {
      projectViewModel_->importModel(QStringList{localPath});
      // Phase 241 (PAGE-02): keep the ProjectPage resource tree in sync with
      // the freshly imported objects (import marks dirty, tree re-reads the
      // live service state).
      projectViewModel_->refreshFileTree();
    }
    setCurrentPage(1);
  }
  pushLatencySample(QStringLiteral("topbar-import-model"), int(m_latencyClock.elapsed() - start), localPath);
  return loaded;
}

QVariantList BackendContext::collectProjectEmbeddedPresets() const
{
  // G-13: the current tier selections ride the saved 3MF as project presets
  // (upstream save_project -> project_presets). Values come from the preset
  // service store; names from the tier selections.
  QVariantList embed;
  if (!presetService_ || !configViewModel_)
    return embed;
  struct Tier
  {
    int category;
    QString (ConfigViewModel::*nameFn)() const;
  };
  const Tier tiers[] = {
      {PresetService::PrinterCat, &ConfigViewModel::currentPrinterPreset},
      {PresetService::FilamentCat, &ConfigViewModel::currentFilamentPreset},
      {PresetService::PrintCat, &ConfigViewModel::currentPrintPreset},
  };
  for (const Tier &tier : tiers) {
    const QString name = (configViewModel_->*tier.nameFn)();
    if (name.isEmpty() || !presetService_->hasPreset(name))
      continue;
    QVariantMap entry;
    entry.insert(QStringLiteral("category"), tier.category);
    entry.insert(QStringLiteral("name"), name);
    QVariantMap values;
    const QHash<QString, QVariant> presetValues = presetService_->presetValues(name);
    for (auto it = presetValues.constBegin(); it != presetValues.constEnd(); ++it)
      values.insert(it.key(), it.value());
    entry.insert(QStringLiteral("values"), values);
    entry.insert(QStringLiteral("inherits"), presetService_->presetInherits(name));
    embed.append(entry);
  }
  return embed;
}

bool BackendContext::topbarSaveProject()
{
  const qint64 start = m_latencyClock.elapsed();
  if (!projectViewModel_)
    return false;

  // R-P1.I (CONTEXT safety, canSave()): U8 复核更新依据——保存不再在 GUI 线程
  // 序列化 Model（写盘已移 save worker），但快照构建仍在 GUI 线程读取 live
  // Model/盘列表，且交付会改工程记账，与切片 worker 的模型读仍然互斥，门
  // 保留。The gate lives here (not per-QML-entry) so Ctrl+S and both File
  // menus inherit it.
  if (!canSave())
  {
    // Transient gate: the action is deferred, nothing failed/broken —
    // warning grade, not the red error block (U1 severity sweep).
    postError(tr("切片/任务进行中，暂无法保存项目"), NotiWarning);
    return false;
  }

  if (projectViewModel_->currentProjectPath().isEmpty())
    return false;

  // 实际保存项目数据到磁盘（对齐上游 Plater::save_project）
  // v5.16 (PSET2-06): overlay the preset selection state (tier preset ids +
  // per-extruder filament_presets vector) into the stored project config so
  // a reload restores it (upstream embeds the PresetBundle selections).
  if (projectService_ && configViewModel_) {
    projectService_->setProjectConfigOverlay(configViewModel_->projectPresetConfigOverlay());
    // G-13: embed the current tier selections (upstream save_project carries
    // them as project presets in the archive).
    projectService_->setProjectEmbeddedPresets(collectProjectEmbeddedPresets());
  }
  // U8: dispatch the write to the save worker instead of serializing the 3MF
  // on this (GUI) thread — Ctrl+S no longer freezes for the duration of the
  // archive write. The terminal outcome (success toast + ProjectViewModel
  // bookkeeping + failure reason) lands via saveFinished.
  const QString savePath = projectViewModel_->currentProjectPath();
  const QString saveSuffix = QFileInfo(savePath).suffix().toLower();
  const bool jsonTarget = saveSuffix != QLatin1String("3mf")
                       && saveSuffix != QLatin1String("cxprj");
  const bool dispatched = projectService_ && projectService_->saveProjectAsync(savePath);
  if (!dispatched)
    return handleSaveDispatchRejected(jsonTarget, "topbar-save-project", start, QString());
  // U8: the sample now measures GUI dispatch latency; the write completes on
  // the worker and reports through saveFinished.
  pushLatencySample(QStringLiteral("topbar-save-project"), int(m_latencyClock.elapsed() - start), QString());
  return true;
}

bool BackendContext::topbarSaveProjectAs(const QString &filePath)
{
  const qint64 start = m_latencyClock.elapsed();
  if (!projectViewModel_)
    return false;

  // R-P1.I: same gate as topbarSaveProject (U8 复核依据：快照构建读 live
  // Model、交付改工程记账，与切片 worker 互斥)。
  if (!canSave())
  {
    // Transient gate: the action is deferred, nothing failed/broken —
    // warning grade, not the red error block (U1 severity sweep).
    postError(tr("切片/任务进行中，暂无法保存项目"), NotiWarning);
    return false;
  }

  const QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  if (localPath.isEmpty())
    return false;

  // 实际保存项目数据到磁盘
  // v5.16 (PSET2-06): same preset-selection overlay as topbarSaveProject.
  if (projectService_ && configViewModel_)
    projectService_->setProjectConfigOverlay(configViewModel_->projectPresetConfigOverlay());
  // G-13: embed the current tier selections as above.
  if (projectService_)
    projectService_->setProjectEmbeddedPresets(collectProjectEmbeddedPresets());
  // U8: same save-worker dispatch as topbarSaveProject; the SaveAs
  // bookkeeping (current project identity + dirty clear + recent projects)
  // and the success toast land via saveFinished.
  const QString asSuffix = QFileInfo(localPath).suffix().toLower();
  const bool jsonTarget = asSuffix != QLatin1String("3mf")
                       && asSuffix != QLatin1String("cxprj");
  const bool dispatched = projectService_
                              && projectService_->saveProjectAsAsync(localPath);
  if (!dispatched)
    return handleSaveDispatchRejected(jsonTarget, "topbar-save-project-as", start, localPath);
  pushLatencySample(QStringLiteral("topbar-save-project-as"), int(m_latencyClock.elapsed() - start), localPath);
  return true;
}

bool BackendContext::handleSaveDispatchRejected(bool jsonTarget,
                                                const char *latencyName,
                                                qint64 start,
                                                const QString &detail)
{
  // U8 复核（中 3）: the shared rejection tail of topbarSaveProject /
  // topbarSaveProjectAs (the two near-identical isSaving fallback blocks).
  if (projectService_ && projectService_->isSaving())
  {
    // A save job is already writing: treat as accepted so the Ctrl+S
    // fallback (main.qml opens Save-As on false) does not fire; the running
    // job delivers the real result.
    postError(tr("保存任务进行中，已忽略本次保存"), NotiWarning);
    pushLatencySample(QLatin1String(latencyName), int(m_latencyClock.elapsed() - start), detail);
    return true;
  }
  if (!jsonTarget)
    postError(projectService_ ? projectService_->lastError() : tr("项目保存失败"), NotiError);
  // U8 复核（低 5）: the JSON branch completed synchronously and already
  // reported through saveFinished (the handler posted the toast/error) —
  // posting here again produced a double toast. Keep the pre-U8 bool
  // contract only (false → the Ctrl+S fallback opens Save-As, as before U8).
  pushLatencySample(QLatin1String(latencyName), int(m_latencyClock.elapsed() - start), detail);
  return false;
}

int BackendContext::beginLatency(const QString &operation, const QString &detail)
{
  if (operation.isEmpty())
    return -1;
  const int token = m_latencyNextToken++;
  m_pendingLatencies.insert(token, PendingLatency{operation, detail, m_latencyClock.elapsed()});
  return token;
}

void BackendContext::endLatency(int token)
{
  const auto it = m_pendingLatencies.find(token);
  if (it == m_pendingLatencies.end())
    return;
  const PendingLatency pending = it.value();
  m_pendingLatencies.erase(it);
  const int elapsedMs = int(m_latencyClock.elapsed() - pending.startMs);
  pushLatencySample(pending.operation, elapsedMs, pending.detail);
}

void BackendContext::recordLatency(const QString &operation, int elapsedMs, const QString &detail)
{
  pushLatencySample(operation, elapsedMs, detail);
}

void BackendContext::resetLatency()
{
  m_pendingLatencies.clear();
  m_latencyStats.clear();
  m_lastLatencyOperation.clear();
  m_lastLatencyMs = 0;
  emit latencyChanged();
}

QString BackendContext::latencyBrief() const
{
  const auto tabIt = m_latencyStats.find(QStringLiteral("tab-switch"));
  if (tabIt == m_latencyStats.end() || tabIt->count == 0)
    return QStringLiteral("Latency: --");

  const auto &s = tabIt.value();
  const int avgMs = s.totalMs / s.count;
  const int p95Ms = percentile95(s.samples);
  return QStringLiteral("Tab %1ms | Avg %2ms | P95 %3ms")
      .arg(s.lastMs)
      .arg(avgMs)
      .arg(p95Ms);
}

QString BackendContext::lastLatencyOperation() const
{
  return m_lastLatencyOperation;
}

int BackendContext::lastLatencyMs() const
{
  return m_lastLatencyMs;
}

void BackendContext::pushLatencySample(const QString &operation, int elapsedMs, const QString &detail)
{
  if (operation.isEmpty() || elapsedMs < 0)
    return;

  auto &stats = m_latencyStats[operation];
  stats.count += 1;
  stats.totalMs += elapsedMs;
  stats.lastMs = elapsedMs;
  stats.maxMs = std::max(stats.maxMs, elapsedMs);
  stats.samples.push_back(elapsedMs);
  constexpr int kMaxSamples = 120;
  if (stats.samples.size() > kMaxSamples)
    stats.samples.remove(0, stats.samples.size() - kMaxSamples);

  m_lastLatencyOperation = detail.isEmpty() ? operation : QStringLiteral("%1 (%2)").arg(operation, detail);
  m_lastLatencyMs = elapsedMs;

  if (elapsedMs > 120)
    qWarning() << "[Latency] slow" << operation << elapsedMs << "ms" << detail;
  else
    qInfo() << "[Latency]" << operation << elapsedMs << "ms" << detail;

  emit latencyChanged();
}

int BackendContext::percentile95(const QVector<int> &samples)
{
  if (samples.isEmpty())
    return 0;
  QVector<int> sorted = samples;
  std::sort(sorted.begin(), sorted.end());
  const int sampleCount = int(sorted.size());
  const int idx = std::min(sampleCount - 1, int((sampleCount - 1) * 0.95));
  return sorted[idx];
}

void BackendContext::cancelPrintSend()
{
  // U11: PrintDialog 发送页的取消按钮。真实上传必须真的中止——否则对话框
  // 已回 prepare 页而任务仍会落到打印机，且重发会被在途守卫吞掉。
  if (deviceService_)
    deviceService_->cancelSendPrint();
}

void BackendContext::postError(const QString &message, int severity)
{
  if (!m_notificationsEnabled)
    return;

  // Phase 240 (NOTI-01): errors join the stacked surface instead of
  // replacing the current toast (upstream pushes a new PopNotification and
  // keeps the old ones visible, NotificationManager.cpp:2479-2505).
  NotificationEntry entry;
  entry.message = message;
  entry.severity = severity;
  entry.timestamp = QDateTime::currentDateTime();
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/false);
}

void BackendContext::postNotification(const QString &message, const QString &title, int severity)
{
  if (!m_notificationsEnabled)
    return;

  NotificationEntry entry;
  entry.message = message;
  entry.title = title;
  entry.severity = severity;
  entry.timestamp = QDateTime::currentDateTime();
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/false);
}

int BackendContext::notificationImportanceRank(const NotificationEntry &e)
{
  // Upstream NotificationLevel (NotificationManager.hpp:158-180) is sorted by
  // importance: ProgressBar(1) < Hint(2) < Regular(3) < PrintInfo(4) <
  // PrintInfoShort(5) < Important(6) < Warning(7) < SeriousWarning(8) <
  // Error(9). Map the Qt NotiLevel enum onto that ladder.
  switch (e.severity)
  {
  case NotiProgress:
  case NotiSlicingProgress:
    return 1; // ProgressBarNotificationLevel
  case NotiHint:
    return 2; // HintNotificationLevel
  case NotiPrintInfo:
    return 4; // PrintInfoNotificationLevel
  case NotiPrintInfoShort:
    return 5; // PrintInfoShortNotificationLevel
  case NotiWarning:
    return 7; // WarningNotificationLevel
  case NotiSeriousWarning:
    return 8; // SeriousWarningNotificationLevel
  case NotiError:
    return 9; // ErrorNotificationLevel
  case NotiSuccess:
  case NotiInfo:
  default:
    // Persistent info (export/arrange ongoing) behaves like upstream
    // ImportantNotificationLevel (no fade-out); everything else is Regular.
    return e.persistent ? 6 : 3;
  }
}

void BackendContext::archiveNotification(const NotificationEntry &entry)
{
  m_notificationHistory.prepend(entry);
  // NOTI-02: upstream sorts the live notification vector by importance before
  // rendering (sort_notifications, NotificationManager.cpp:3136 + 3232-3242;
  // stable, so equal levels keep insertion order; the level ladder in
  // NotificationManager.hpp:199-221 puts ErrorNotificationLevel at the
  // "Top most position"). The Qt6 history list renders top-down, so the same
  // stable sort with the same comparator as the live stack
  // (notificationImportanceRank, most important first) puts the most severe
  // entry on top while equal-importance entries stay newest first (prepend
  // order).
  std::stable_sort(m_notificationHistory.begin(), m_notificationHistory.end(),
                   [](const NotificationEntry &a, const NotificationEntry &b)
                   { return notificationImportanceRank(a) > notificationImportanceRank(b); });
  ++m_unreadHistoryCount;
  if (m_notificationHistory.size() > 100)
    m_notificationHistory.removeLast();
  emit historyChanged();
}

void BackendContext::retireNotificationType(int type)
{
  bool removed = false;
  for (int i = m_activeNotifications.size() - 1; i >= 0; --i)
  {
    if (m_activeNotifications[i].type != type)
      continue;
    const NotificationEntry retired = m_activeNotifications.takeAt(i);
    archiveNotification(retired);
    removed = true;
  }
  if (removed)
    showNextNotification(); // pumps the overflow queue + syncs/emits
}

void BackendContext::pushNotificationEntry(NotificationEntry entry, bool dedupByTypeOnly)
{
  // Duplicate compression (upstream activate_existing, NotificationManager.
  // cpp:2643-2675): a matching live entry is UPDATED in place instead of a
  // second toast stacking on top. Progress-family types (slicing progress /
  // export ongoing / arrange ongoing / hint) dedup on type alone because
  // their message text changes on every tick; everything else dedups on
  // type + message + title (the upstream multiple-types rule: "multiple of
  // one type allowed, but must have different text").
  for (int i = 0; i < m_activeNotifications.size(); ++i)
  {
    NotificationEntry &live = m_activeNotifications[i];
    const bool sameType = (live.type == entry.type);
    const bool sameText = (live.message == entry.message && live.title == entry.title);
    if (!sameType || (!dedupByTypeOnly && !sameText))
      continue;
    // Escalation counter (upstream UpdatedItemsInfoNotification counter
    // pattern, NotificationManager.hpp:818): repeated identical posts bump
    // the xN badge instead of spamming the stack.
    if (sameText || dedupByTypeOnly)
      live.repeatCount += 1;
    live.message = entry.message;
    live.title = entry.title;
    live.severity = entry.severity;
    live.persistent = entry.persistent;
    live.hasProgress = entry.hasProgress;
    live.cancellable = entry.cancellable;
    live.progressValue = entry.progressValue;
    live.progressMin = entry.progressMin;
    live.progressMax = entry.progressMax;
    live.requiresConfirm = entry.requiresConfirm;
    live.showExportButton = entry.showExportButton;
    live.showPreviewButton = entry.showPreviewButton;
    live.hypertext = entry.hypertext;
    live.timestamp = entry.timestamp;
    syncCurrentNotificationFromStack();
    emit errorChanged();
    return;
  }

  entry.id = m_nextNotificationId++;
  if (m_activeNotifications.size() < kMaxVisibleNotifications)
    m_activeNotifications.append(entry);
  else
    m_notificationQueue.enqueue(entry);

  // Importance ordering (upstream sort_notifications, NotificationManager.
  // cpp:2633-2639): stable sort so equal-importance entries keep insertion
  // order. The list is MOST important first so the QML column renders errors
  // at the top (upstream renders bottom-up with Error at the "Top most
  // position").
  std::stable_sort(m_activeNotifications.begin(), m_activeNotifications.end(),
                   [](const NotificationEntry &a, const NotificationEntry &b)
                   { return notificationImportanceRank(a) > notificationImportanceRank(b); });

  syncCurrentNotificationFromStack();
  emit errorChanged();
}

void BackendContext::syncCurrentNotificationFromStack()
{
  // Legacy single-toast view: the NEWEST entry (last in insertion order)
  // keeps driving lastError*/currentNotification* so all pre-Phase-240
  // callers and tests see "last post wins" semantics.
  if (!m_activeNotifications.isEmpty())
  {
    // Find the newest by id (ids are monotonic; the sort reordered the list).
    int newestIdx = 0;
    for (int i = 1; i < m_activeNotifications.size(); ++i)
      if (m_activeNotifications[i].id > m_activeNotifications[newestIdx].id)
        newestIdx = i;
    m_currentNotification = m_activeNotifications[newestIdx];
    lastErrorMessage_ = m_currentNotification.message;
    lastErrorSeverity_ = m_currentNotification.severity;
    lastErrorTitle_ = m_currentNotification.title;
  }
  else
  {
    m_currentNotification = {};
    lastErrorMessage_.clear();
    lastErrorTitle_.clear();
    lastErrorSeverity_ = -1;
  }
}

void BackendContext::dismissNotification()
{
  // Dismiss the NEWEST entry (legacy single-toast behavior: the visible
  // toast being closed was the current one).
  if (!m_activeNotifications.isEmpty())
  {
    int newestIdx = 0;
    for (int i = 1; i < m_activeNotifications.size(); ++i)
      if (m_activeNotifications[i].id > m_activeNotifications[newestIdx].id)
        newestIdx = i;
    const NotificationEntry dismissed = m_activeNotifications.takeAt(newestIdx);
    archiveNotification(dismissed);
  }
  showNextNotification();
}

void BackendContext::dismissNotificationById(int id)
{
  for (int i = 0; i < m_activeNotifications.size(); ++i)
  {
    if (m_activeNotifications[i].id != id)
      continue;
    const NotificationEntry dismissed = m_activeNotifications.takeAt(i);
    archiveNotification(dismissed);
    showNextNotification();
    return;
  }
}

void BackendContext::confirmNotificationById(int id)
{
  for (const NotificationEntry &entry : std::as_const(m_activeNotifications))
  {
    if (entry.id == id && entry.requiresConfirm)
    {
      dismissNotificationById(id);
      return;
    }
  }
}

void BackendContext::cancelNotificationById(int id)
{
  for (const NotificationEntry &entry : std::as_const(m_activeNotifications))
  {
    if (entry.id == id && entry.requiresConfirm)
    {
      dismissNotificationById(id);
      return;
    }
  }
}

void BackendContext::cancelNotificationAction(int id)
{
  // U3: upstream on_cancel_button semantics (SlicingProgressNotification
  // .cpp:161-170): run the cancel callback; when it reports nothing to
  // cancel any more (service already idle), drop the notification outright
  // (callback-false -> set_progress_state(SP_NO_SLICING)). The service
  // workers own the terminal report -- cancelSlice/cancelExport/cancelLoad
  // only flip their shared atomic flags, and the completion lambdas deliver
  // sliceFailed / exportFailed / loadFinished on the GUI thread, which
  // retire the entry via the connections in the constructor.
  for (int i = 0; i < m_activeNotifications.size(); ++i)
  {
    NotificationEntry &entry = m_activeNotifications[i];
    if (entry.id != id || !entry.cancellable)
      continue;

    QString cancellingText;
    switch (entry.type)
    {
    case NotiTypeSlicingProgress:
      if (!sliceService_ || !sliceService_->slicing())
      {
        dismissNotificationById(id);
        return;
      }
      sliceService_->cancelSlice();
      cancellingText = tr("正在取消切片...");
      break;
    case NotiTypeExportOngoing:
      if (!sliceService_ || !sliceService_->exportBusy())
      {
        dismissNotificationById(id);
        return;
      }
      sliceService_->cancelExport();
      cancellingText = tr("正在取消导出...");
      break;
    case NotiTypeProgressBar:
      if (!projectService_ || !projectService_->loading())
      {
        dismissNotificationById(id);
        return;
      }
      projectService_->cancelLoad();
      cancellingText = tr("正在取消导入...");
      break;
    default:
      return; // no cancel target behind this type; QML only renders the
              // button for cancellable entries
    }
    entry.message = cancellingText;
    syncCurrentNotificationFromStack();
    emit errorChanged();
    return;
  }
}

void BackendContext::activateNotificationHypertext(int id)
{
  // Upstream render_hypertext (NotificationManager.cpp:734-753): the click
  // handler runs the notification callback and, when it returns true, calls
  // close(). The mock backend carries no jump-to-object/option targets, so
  // activation only acknowledges + closes the clicked entry (upstream
  // ValidateError links also close, NotificationManager.cpp:1997).
  for (const NotificationEntry &entry : std::as_const(m_activeNotifications))
  {
    if (entry.id == id && !entry.hypertext.isEmpty())
    {
      dismissNotificationById(id);
      return;
    }
  }
}

void BackendContext::clearError()
{
  dismissNotification();
}

void BackendContext::showNextNotification()
{
  // Pull one queued entry into the freed visible slot (upstream pops from
  // m_pop_notifications only on finish; the overflow queue is the Qt6
  // screen-space bound).
  if (m_activeNotifications.size() < kMaxVisibleNotifications && !m_notificationQueue.isEmpty())
  {
    const auto entry = m_notificationQueue.dequeue();
    m_activeNotifications.append(entry);
    std::stable_sort(m_activeNotifications.begin(), m_activeNotifications.end(),
                     [](const NotificationEntry &a, const NotificationEntry &b)
                     { return notificationImportanceRank(a) > notificationImportanceRank(b); });
  }
  syncCurrentNotificationFromStack();
  emit errorChanged();
}

QVariantList BackendContext::notificationStack() const
{
  QVariantList list;
  list.reserve(m_activeNotifications.size());
  for (const NotificationEntry &e : m_activeNotifications)
  {
    QVariantMap m;
    m.insert(QStringLiteral("id"), e.id);
    m.insert(QStringLiteral("message"), e.message);
    m.insert(QStringLiteral("title"), e.title);
    m.insert(QStringLiteral("severity"), e.severity);
    m.insert(QStringLiteral("type"), e.type);
    m.insert(QStringLiteral("persistent"), e.persistent);
    m.insert(QStringLiteral("hasProgress"), e.hasProgress);
    m.insert(QStringLiteral("cancellable"), e.cancellable);
    m.insert(QStringLiteral("progressValue"), e.progressValue);
    m.insert(QStringLiteral("repeatCount"), e.repeatCount);
    m.insert(QStringLiteral("requiresConfirm"), e.requiresConfirm);
    m.insert(QStringLiteral("showExportButton"), e.showExportButton);
    m.insert(QStringLiteral("showPreviewButton"), e.showPreviewButton);
    m.insert(QStringLiteral("hypertext"), e.hypertext);
    list.append(m);
  }
  return list;
}

int BackendContext::pendingNotificationCount() const
{
  return m_notificationQueue.size();
}

int BackendContext::currentNotificationProgress() const
{
  return m_currentNotification.progressValue;
}

bool BackendContext::currentNotificationHasProgress() const
{
  return m_currentNotification.hasProgress;
}

bool BackendContext::currentNotificationPersistent() const
{
  return m_currentNotification.persistent;
}

void BackendContext::updateNotificationProgress(int value)
{
  // Update the progress on the newest entry AND its stacked twin so the
  // visible toast and the legacy getter stay in lockstep.
  m_currentNotification.progressValue = qBound(
    m_currentNotification.progressMin,
    value,
    m_currentNotification.progressMax);
  for (NotificationEntry &e : m_activeNotifications)
  {
    if (e.id == m_currentNotification.id)
    {
      e.progressValue = m_currentNotification.progressValue;
      break;
    }
  }
  emit errorChanged();
}

void BackendContext::confirmCurrentNotification()
{
  // 对齐上游 notification_manager::confirm
  m_currentNotification.persistent = false;
  m_currentNotification.requiresConfirm = false;
  for (NotificationEntry &e : m_activeNotifications)
  {
    if (e.id == m_currentNotification.id)
    {
      e.persistent = false;
      e.requiresConfirm = false;
      break;
    }
  }
  dismissNotification();
}

void BackendContext::cancelCurrentNotification()
{
  m_currentNotification.persistent = false;
  m_currentNotification.requiresConfirm = false;
  for (NotificationEntry &e : m_activeNotifications)
  {
    if (e.id == m_currentNotification.id)
    {
      e.persistent = false;
      e.requiresConfirm = false;
      break;
    }
  }
  dismissNotification();
}

// ------- 专用通知类型实现（对齐上游 NotificationManager 专用通知） -------

int BackendContext::currentNotificationType() const
{
  return m_currentNotification.type;
}

bool BackendContext::currentNotificationShowExport() const
{
  return m_currentNotification.showExportButton;
}

bool BackendContext::currentNotificationShowPreview() const
{
  return m_currentNotification.showPreviewButton;
}

void BackendContext::postSlicingProgress(int percent, const QString &stage)
{
  if (!m_notificationsEnabled || !m_showProgressNotifications)
    return;

  NotificationEntry entry;
  entry.type = NotiTypeSlicingProgress;
  entry.severity = NotiSlicingProgress;
  entry.persistent = true;
  entry.hasProgress = true;
  // U3: the cancel button (upstream SP_PROGRESS state: "never fades outs,
  // no close button, has cancel button", SlicingProgressNotification.hpp:18).
  entry.cancellable = true;
  entry.progressValue = qBound(0, percent, 100);
  entry.title = tr("切片中");
  entry.message = stage.isEmpty()
      ? tr("正在切片... %1%").arg(percent)
      : tr("%1... %2%").arg(stage, QString::number(percent));
  entry.timestamp = QDateTime::currentDateTime();

  // Phase 240 (NOTI-01): progress notifications update the LIVE slicing
  // entry in place (upstream SlicingProgressNotification updates the same
  // PopNotification, NotificationManager.cpp:2248) via the type-only dedup
  // in pushNotificationEntry.
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/true);
}

void BackendContext::postSlicingComplete()
{
  if (!m_notificationsEnabled)
    return;

  NotificationEntry entry;
  entry.type = NotiTypeSlicingProgress;
  entry.severity = NotiSuccess;
  entry.title = tr("切片完成");
  entry.message = tr("切片已完成，可以预览或导出 G-code");
  entry.persistent = true;
  entry.requiresConfirm = false;
  entry.showExportButton = true;
  entry.showPreviewButton = true;
  entry.timestamp = QDateTime::currentDateTime();

  // Replaces the slicing progress entry (same type-only dedup).
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/true);
}

void BackendContext::postExportFinished(const QString &filePath)
{
  postNotification(tr("已导出到: %1").arg(QFileInfo(filePath).fileName()),
                   tr("导出完成"), NotiSuccess);
}

void BackendContext::postExportOngoing(const QString &stage)
{
  NotificationEntry entry;
  entry.type = NotiTypeExportOngoing;
  entry.severity = NotiInfo;
  entry.title = tr("导出中");
  entry.message = stage.isEmpty() ? tr("正在导出 G-code...") : stage;
  entry.persistent = true;
  // U3: export joins the cancel family (upstream cancel_callback stops the
  // background process regardless of slice/export phase; the Qt side
  // carries the export cancel on SliceService::cancelExport's own flag).
  entry.cancellable = true;
  entry.timestamp = QDateTime::currentDateTime();

  // Phase 240 (NOTI-01): type-only dedup updates the live export-ongoing
  // entry in place (upstream ExportOngoing updates the same notification).
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/true);
}

void BackendContext::postImportProgress(int percent, const QString &stage)
{
  // U3: 3MF/模型导入进度（上游 ProgressBarNotification，NotificationManager
  // .hpp:107 "Progress bar with cancel button, cannot be closed"）。只在通
  // 知开启时投递；小文件从首个 tick 到 loadFinished 只闪现一瞬，大文件则
  // 获得全程进度 + 取消。
  if (!m_notificationsEnabled || !m_showProgressNotifications)
    return;

  NotificationEntry entry;
  entry.type = NotiTypeProgressBar;
  entry.severity = NotiProgress;
  entry.persistent = true;
  entry.hasProgress = true;
  entry.cancellable = true;
  entry.progressValue = qBound(0, percent, 100);
  entry.title = tr("导入中");
  entry.message = stage.isEmpty()
      ? tr("正在导入... %1%").arg(percent)
      : tr("%1... %2%").arg(stage, QString::number(percent));
  entry.timestamp = QDateTime::currentDateTime();

  // 类型内去重：进度 tick 原地更新同一条目（同 postSlicingProgress）。
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/true);
}

void BackendContext::postPlaterWarning(const QString &message)
{
  // PlaterWarning: 对象超出打印范围但不禁止切片（对齐上游 NormalNotificationLevel）
  postNotification(message, tr("工作区警告"), NotiWarning);
}

void BackendContext::postPlaterError(const QString &message)
{
  // PlaterError: 对象超出打印范围，无法切片（对齐上游 ErrorNotificationLevel）
  postNotification(message, tr("工作区错误"), NotiError);
}

void BackendContext::postValidateError(const QString &message)
{
  // ValidateError: 切片验证错误（对齐上游 NormalNotificationLevel）
  postNotification(message, tr("验证错误"), NotiError);
}

void BackendContext::postValidateWarning(const QString &message)
{
  postNotification(message, tr("验证警告"), NotiWarning);
}

void BackendContext::postArrangeOngoing(int percent)
{
  if (!m_notificationsEnabled)
    return;
  NotificationEntry entry;
  entry.type = NotiTypeArrangeOngoing;
  entry.severity = NotiInfo;
  entry.title = tr("排列中");
  entry.message = tr("正在自动排列... %1%").arg(qBound(0, percent, 100));
  entry.persistent = true;
  entry.timestamp = QDateTime::currentDateTime();

  // Phase 240 (NOTI-01): type-only dedup updates the live arrange entry.
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/true);
}

// ------- 提示数据库实现（对齐上游 HintDatabase） -------

void BackendContext::initHintDatabase()
{
  // 对齐上游 HintDatabase::load_hints_from_file
  // 从 Qt 资源中的 hints.json 加载提示数据（替代上游 hints.ini + boost::property_tree）
  QFile file(QStringLiteral(":/qml/data/hints.json"));
  if (!file.open(QIODevice::ReadOnly))
  {
    qWarning("[Backend] hints.json not found, using fallback hints");
    initFallbackHintDatabase();
    return;
  }

  const QByteArray data = file.readAll();
  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if (doc.isNull() || !doc.isArray())
  {
    qWarning("[Backend] hints.json parse error: %s", qPrintable(parseError.errorString()));
    initFallbackHintDatabase();
    return;
  }

  const QJsonArray arr = doc.array();
  m_hints.reserve(arr.size());
  for (const auto &val : arr)
  {
    if (!val.isObject())
      continue;
    const QJsonObject obj = val.toObject();
    HintData h;
    h.id = obj.value(QLatin1String("id")).toString();
    h.text = obj.value(QLatin1String("text")).toString();
    h.weight = obj.value(QLatin1String("weight")).toInt(1);
    h.documentationLink = obj.value(QLatin1String("documentation_link")).toString();
    m_hints.append(h);
  }

  m_currentHintIndex = -1;
  qInfo("[Backend] Loaded %d hints from hints.json", m_hints.size());
}

void BackendContext::initFallbackHintDatabase()
{
  // 内置最小提示集（当 JSON 加载失败时的兜底）
  auto add = [this](const QString &id, const QString &text, int weight, const QString &docLink)
  {
    HintData h;
    h.id = id;
    h.text = text;
    h.weight = weight;
    h.documentationLink = docLink;
    m_hints.append(h);
  };

  add(QStringLiteral("hint_layer_height"),
      tr("层高越小打印越精细，但耗时越长。常用范围: 0.1mm - 0.3mm。"),
      10, QStringLiteral("https://github.com/SoftFever/OrcaSlicer/wiki/Layer-height"));
  add(QStringLiteral("hint_infill"),
      tr("填充密度影响模型强度和重量。20% 适合大多数场景，100% 为实心。"),
      10, QStringLiteral("https://github.com/SoftFever/OrcaSlicer/wiki/Infill"));
  add(QStringLiteral("hint_support"),
      tr("悬空角度超过 45° 的部分需要支撑。合理使用支撑可以提升打印质量。"),
      8, QStringLiteral("https://github.com/SoftFever/OrcaSlicer/wiki/Supports"));
  add(QStringLiteral("hint_speed"),
      tr("打印速度越快效率越高，但可能影响表面质量。建议先慢后快测试。"),
      7, {});
  add(QStringLiteral("hint_brims"),
      tr("Brim（裙边）可以增加模型与热床的附着力，防止翘边。"),
      6, {});

  m_currentHintIndex = -1;
}

int BackendContext::selectNextHint(bool random)
{
  if (m_hints.isEmpty())
    return -1;

  // 对齐上游 HintDatabase::get_next_hint_id — 加权随机选择未显示提示
  QVector<int> candidates;
  int totalWeight = 0;
  for (int i = 0; i < m_hints.size(); ++i)
  {
    if (!m_displayedHintIds.contains(m_hints[i].id))
    {
      candidates.append(i);
      totalWeight += m_hints[i].weight;
    }
  }

  // 所有提示都已显示，重置
  if (candidates.isEmpty())
  {
    m_displayedHintIds.clear();
    for (int i = 0; i < m_hints.size(); ++i)
    {
      candidates.append(i);
      totalWeight += m_hints[i].weight;
    }
  }

  if (candidates.isEmpty())
    return 0;

  if (random && totalWeight > 0)
  {
    // 加权随机选择
    int r = QRandomGenerator::global()->bounded(totalWeight);
    int cumulative = 0;
    for (int idx : candidates)
    {
      cumulative += m_hints[idx].weight;
      if (r < cumulative)
        return idx;
    }
    return candidates.last();
  }

  // 顺序选择
  return (m_currentHintIndex + 1) % m_hints.size();
}

void BackendContext::postHint()
{
  if (!m_hintsEnabled || m_hints.isEmpty())
    return;

  const int idx = selectNextHint(true);
  if (idx < 0 || idx >= m_hints.size())
    return;

  m_currentHintIndex = idx;
  m_displayedHintIds.insert(m_hints[idx].id);

  NotificationEntry entry;
  entry.type = NotiTypeDidYouKnowHint;
  entry.severity = NotiHint;
  entry.title = tr("你知道吗");
  entry.message = m_hints[idx].text;
  entry.persistent = true;
  entry.timestamp = QDateTime::currentDateTime();
  entry.hintHasPrev = (m_currentHintIndex > 0 || m_displayedHintIds.size() > 1);
  entry.hintHasNext = true;

  // Phase 240 (NOTI-01): a new hint replaces the live hint entry (upstream
  // keeps at most one HintNotification). Non-hint notifications stay
  // visible (stacked surface).
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/true);
  emit dailyTipChanged();
}

void BackendContext::showDailyTip()
{
  // Phase 241 (PAGE-01): advance the hint cursor for the HomePage Daily Tips
  // card WITHOUT posting a notification (postHint is the notification-path
  // twin). Mirrors upstream DailyTips rotation over the hints database
  // (hints.ini upstream, hints.json here); weighted random selection.
  if (m_hints.isEmpty())
    return;
  const int idx = selectNextHint(true);
  if (idx < 0 || idx >= m_hints.size())
    return;
  m_currentHintIndex = idx;
  m_displayedHintIds.insert(m_hints[idx].id);
  emit dailyTipChanged();
}

void BackendContext::nextHint()
{
  const int idx = selectNextHint(true);
  if (idx < 0 || idx >= m_hints.size())
    return;

  m_currentHintIndex = idx;
  m_displayedHintIds.insert(m_hints[idx].id);
  // Phase 240 (NOTI-01): keep the stacked hint entry + the legacy getter in
  // lockstep when navigating hints.
  const QString text = m_hints[idx].text;
  m_currentNotification.message = text;
  lastErrorMessage_ = text;
  for (NotificationEntry &e : m_activeNotifications)
  {
    if (e.id == m_currentNotification.id && e.type == NotiTypeDidYouKnowHint)
    {
      e.message = text;
      break;
    }
  }
  emit errorChanged();
  emit dailyTipChanged();
}

void BackendContext::prevHint()
{
  if (m_hints.isEmpty() || m_currentHintIndex < 0)
    return;
  m_currentHintIndex = (m_currentHintIndex - 1 + m_hints.size()) % m_hints.size();
  const QString text = m_hints[m_currentHintIndex].text;
  m_currentNotification.message = text;
  lastErrorMessage_ = text;
  for (NotificationEntry &e : m_activeNotifications)
  {
    if (e.id == m_currentNotification.id && e.type == NotiTypeDidYouKnowHint)
    {
      e.message = text;
      break;
    }
  }
  emit errorChanged();
  emit dailyTipChanged();
}

int BackendContext::hintCount() const
{
  return m_hints.size();
}

int BackendContext::currentHintIndex() const
{
  return m_currentHintIndex;
}

QString BackendContext::currentHintText() const
{
  if (m_currentHintIndex >= 0 && m_currentHintIndex < m_hints.size())
    return m_hints[m_currentHintIndex].text;
  return {};
}

QString BackendContext::currentHintHypertext() const
{
  if (m_currentHintIndex >= 0 && m_currentHintIndex < m_hints.size())
    return m_hints[m_currentHintIndex].hypertext;
  return {};
}

QString BackendContext::currentHintFollowText() const
{
  if (m_currentHintIndex >= 0 && m_currentHintIndex < m_hints.size())
    return m_hints[m_currentHintIndex].followText;
  return {};
}

bool BackendContext::currentHintHasDocumentationLink() const
{
  if (m_currentHintIndex >= 0 && m_currentHintIndex < m_hints.size())
    return !m_hints[m_currentHintIndex].documentationLink.isEmpty();
  return false;
}

bool BackendContext::openHintDocumentation() const
{
  // 对齐上游 HintNotification hypertext_type=documentation 链接点击
  if (m_currentHintIndex < 0 || m_currentHintIndex >= m_hints.size())
    return false;
  const QString link = m_hints[m_currentHintIndex].documentationLink;
  if (link.isEmpty())
    return false;
  return QDesktopServices::openUrl(QUrl(link));
}

QString BackendContext::lastErrorMessage() const { return lastErrorMessage_; }
int BackendContext::lastErrorSeverity() const { return lastErrorSeverity_; }
QString BackendContext::lastErrorTitle() const { return lastErrorTitle_; }

// ------- 通知中心实现（对齐上游 notification_manager） -------

int BackendContext::historyCount() const
{
  return m_notificationHistory.size();
}

int BackendContext::unreadHistoryCount() const
{
  return m_unreadHistoryCount;
}

QVariantList BackendContext::notificationHistory() const
{
  // NOTI-02: QVariantList (same pattern as notificationStack) so the
  // NotificationCenter ListView resets on every historyChanged. The previous
  // count + index-accessor model left stale rows whenever the list was
  // reordered (importance sort) or trimmed at the cap without a count change.
  QVariantList list;
  list.reserve(m_notificationHistory.size());
  for (const NotificationEntry &entry : std::as_const(m_notificationHistory))
  {
    QVariantMap map;
    map.insert(QStringLiteral("id"), entry.id);
    map.insert(QStringLiteral("message"), entry.message);
    map.insert(QStringLiteral("title"), entry.title);
    map.insert(QStringLiteral("severity"), entry.severity);
    list.append(map);
  }
  return list;
}

void BackendContext::removeHistoryById(int id)
{
  // NOTI-02: per-entry removal (upstream every PopNotification owns a close
  // button that calls PopNotification::close(), NotificationManager.cpp:861-864).
  for (int i = 0; i < m_notificationHistory.size(); ++i)
  {
    if (m_notificationHistory[i].id != id)
      continue;
    m_notificationHistory.removeAt(i);
    // The unread tally has no per-entry read flag; dropping a row consumes
    // one tally so the header badge cannot outlive its rows.
    if (m_unreadHistoryCount > 0)
      --m_unreadHistoryCount;
    emit historyChanged();
    return;
  }
}

void BackendContext::clearHistory()
{
  m_notificationHistory.clear();
  m_unreadHistoryCount = 0;
  emit historyChanged();
}

void BackendContext::markHistoryRead()
{
  m_unreadHistoryCount = 0;
  emit historyChanged();
}

void BackendContext::setNotificationsEnabled(bool v)
{
  if (m_notificationsEnabled == v)
    return;
  m_notificationsEnabled = v;
  emit settingsChanged();
}

void BackendContext::setHintsEnabled(bool v)
{
  if (m_hintsEnabled == v)
    return;
  m_hintsEnabled = v;
  emit settingsChanged();
}

void BackendContext::setAutoDismissSec(int sec)
{
  sec = qBound(2, sec, 30);
  if (m_autoDismissSec == sec)
    return;
  m_autoDismissSec = sec;
  emit settingsChanged();
}

void BackendContext::setShowProgressNotifications(bool v)
{
  if (m_showProgressNotifications == v)
    return;
  m_showProgressNotifications = v;
  emit settingsChanged();
}

// ------- 外观实现 -------

// S6: the single theme dispatch (see BackendContext.h). The stored index is
// a raw int (draft/commit transaction semantics in SettingsViewModel keep
// out-of-range values round-trippable); consumption clamps onto the
// two-value axis: <=0 dark, anything else light.
void BackendContext::applyTheme(int idx)
{
  ThemeState::instance().setDark(idx <= 0);
  emit themeChanged();
}

QString BackendContext::qmlTheme() const
{
  return ThemeState::instance().isDark() ? QStringLiteral("dark")
                                         : QStringLiteral("light");
}

QVariantMap BackendContext::themeTokens() const
{
  const auto &palette = ThemeTokens::palette(ThemeState::instance().isDark());
  const auto &staticTokens = ThemeTokens::staticTokens();
  const auto color = [](const char *hex) { return QColor(QLatin1String(hex)); };
  QVariantList severity;
  severity.reserve(10);
  for (const char *hex : palette.severityColors)
    severity.append(color(hex));
  QVariantMap map;
  map.insert(QStringLiteral("bgBase"), color(palette.bgBase));
  map.insert(QStringLiteral("bgSurface"), color(palette.bgSurface));
  map.insert(QStringLiteral("bgPanel"), color(palette.bgPanel));
  map.insert(QStringLiteral("bgCard"), color(palette.bgCard));
  map.insert(QStringLiteral("surfaceDeep"), color(palette.surfaceDeep));
  map.insert(QStringLiteral("bannerSurface"), color(palette.bannerSurface));
  map.insert(QStringLiteral("overlayBorder"), color(palette.overlayBorder));
  map.insert(QStringLiteral("separator"), color(palette.separator));
  map.insert(QStringLiteral("bgElevated"), color(palette.bgElevated));
  map.insert(QStringLiteral("bgInset"), color(palette.bgInset));
  map.insert(QStringLiteral("bgFloating"), color(palette.bgFloating));
  map.insert(QStringLiteral("bgHover"), color(palette.bgHover));
  map.insert(QStringLiteral("bgPressed"), color(palette.bgPressed));
  map.insert(QStringLiteral("bgTooltip"), color(palette.bgTooltip));
  map.insert(QStringLiteral("accentSubtle"), color(palette.accentSubtle));
  map.insert(QStringLiteral("accentSubtlePressed"),
             color(palette.accentSubtlePressed));
  map.insert(QStringLiteral("textPrimary"), color(palette.textPrimary));
  map.insert(QStringLiteral("textSecondary"), color(palette.textSecondary));
  map.insert(QStringLiteral("textTertiary"), color(palette.textTertiary));
  map.insert(QStringLiteral("textDisabled"), color(palette.textDisabled));
  map.insert(QStringLiteral("textMuted"), color(palette.textMuted));
  map.insert(QStringLiteral("borderDefault"), color(palette.borderDefault));
  map.insert(QStringLiteral("borderSubtle"), color(palette.borderSubtle));
  map.insert(QStringLiteral("borderStrong"), color(palette.borderStrong));
  map.insert(QStringLiteral("borderInput"), color(palette.borderInput));
  map.insert(QStringLiteral("borderActive"), color(palette.borderActive));
  map.insert(QStringLiteral("chromeSurface"), color(palette.chromeSurface));
  map.insert(QStringLiteral("chromeSurfaceAlt"), color(palette.chromeSurfaceAlt));
  map.insert(QStringLiteral("chromeHover"), color(palette.chromeHover));
  map.insert(QStringLiteral("chromePressed"), color(palette.chromePressed));
  map.insert(QStringLiteral("chromeBorder"), color(palette.chromeBorder));
  map.insert(QStringLiteral("chromeText"), color(palette.chromeText));
  map.insert(QStringLiteral("chromeTextMuted"), color(palette.chromeTextMuted));
  map.insert(QStringLiteral("statusSuccess"), color(palette.statusSuccess));
  map.insert(QStringLiteral("statusWarning"), color(palette.statusWarning));
  map.insert(QStringLiteral("statusError"), color(palette.statusError));
  map.insert(QStringLiteral("statusInfo"), color(palette.statusInfo));
  map.insert(QStringLiteral("bgErrorSubtle"), color(palette.bgErrorSubtle));
  map.insert(QStringLiteral("bgWarningSubtle"), color(palette.bgWarningSubtle));
  map.insert(QStringLiteral("scrollBarColor"), color(palette.scrollBarColor));
  map.insert(QStringLiteral("scrollBarHoverColor"),
             color(palette.scrollBarHoverColor));
  map.insert(QStringLiteral("scrollBarTrackColor"),
             color(palette.scrollBarTrackColor));
  map.insert(QStringLiteral("switchTrackOff"), color(palette.switchTrackOff));
  map.insert(QStringLiteral("switchKnob"), color(palette.switchKnob));
  map.insert(QStringLiteral("menuBackground"), color(palette.menuBackground));
  map.insert(QStringLiteral("controlDisabledBg"),
             color(palette.controlDisabledBg));
  map.insert(QStringLiteral("controlDisabledText"),
             color(palette.controlDisabledText));
  map.insert(QStringLiteral("severityColors"), severity);
  map.insert(QStringLiteral("accent"), color(staticTokens.accent));
  map.insert(QStringLiteral("plateLabel"), color(staticTokens.plateLabel));
  return map;
}

void BackendContext::requestTheme(const QString &name)
{
  // Same validation Theme.setTheme always applied; unknown names are
  // ignored so a stale caller cannot corrupt the stored key.
  if (name != QLatin1String("dark") && name != QLatin1String("light"))
    return;
  const int idx = name == QLatin1String("light") ? 1 : 0;
  // Persist the merged single storage key, then run the one dispatch
  // through SettingsViewModel: setThemeIndex guards on a real draft change,
  // so a redundant toggle is a no-op and the signal fires exactly when the
  // theme actually flips. Persist-then-set keeps the store and the draft
  // coherent for a later Preferences Cancel (which reloads from the store).
  QSettings s;
  s.setValue(QStringLiteral("themeIndex"), idx);
  if (settingsViewModel_)
    settingsViewModel_->setThemeIndex(idx);
}

QString BackendContext::displayProjectTitle() const
{
  // 优先使用 EditorViewModel 的 projectName
  if (editorViewModel_) {
    const QString name = editorViewModel_->property("projectName").toString();
    if (!name.isEmpty())
      return name;
  }
  // 回退：从 projectViewModel 的 currentProjectPath 提取文件名
  if (projectViewModel_) {
    const QString path = projectViewModel_->property("currentProjectPath").toString();
    if (!path.isEmpty()) {
      const QFileInfo fi(path);
      return fi.fileName();
    }
  }
  return tr("未命名");
}

void BackendContext::applyLanguage(int idx)
{
  if (idx < 0 || idx >= static_cast<int>(std::size(kLangCodes)))
    return;

  if (m_translator)
  {
    QCoreApplication::removeTranslator(m_translator);
    delete m_translator;
    m_translator = nullptr;
  }

  m_translator = new QTranslator(this);
  const QString qmFile = QString(":/i18n/%1.qm").arg(kLangCodes[idx]);
  if (m_translator->load(qmFile))
    QCoreApplication::installTranslator(m_translator);
  else
  {
    delete m_translator;
    m_translator = nullptr;
  }
  emit languageChanged();
  // WR-06: tr("未命名") 占位符在 displayProjectTitle 内惰性求值，需要显式 emit
  // displayProjectTitleChanged 让 QML 绑定重新求值并刷新本地化占位符。
  emit displayProjectTitleChanged();
}

void BackendContext::applyFontSize(int size)
{
  if (QGuiApplication *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
  {
    QFont f = app->font();
    f.setPixelSize(size);
    app->setFont(f);
  }
}

void BackendContext::applyUiScale(int idx)
{
  if (idx < 0 || idx >= static_cast<int>(std::size(kScales)))
    return;
  // Q11-①: QT_SCALE_FACTOR is only honored before QGuiApplication exists
  // (main_qml.cpp maps the pre-launch "ui/scaleFactor" key of the dedicated
  // "OWzx/UI" store onto the env var), so a scale change can never take
  // effect in the live process. Write that real key instead of the former
  // in-memory m_uiScale — which had zero consumers and never reached the
  // key, making the preference a silent no-op.
  QSettings uiStore(QSettings::NativeFormat, QSettings::UserScope,
                    QStringLiteral("OWzx"), QStringLiteral("UI"));
  uiStore.setValue(QStringLiteral("ui/scaleFactor"), kScales[idx]);
}

// ── Dead-control elimination: real behaviors for the former placeholder
// Help-menu items / dialog buttons (upstream MainFrame help menu,
// NetworkTestDialog, PrintHostDialog). ──────────────────────────────────────

namespace {
// Mirrors the OWZX_VERSION compat constant generated into
// build/libslic3r_generated/buildinfo.h (2.4.0-dev line). Kept local so the
// QML layer does not depend on the generated include path.
constexpr const char *kOwzxUpdateVersion = "2.4.0";
constexpr const char *kOwzxUpdateRepoApi =
    "https://api.github.com/repos/wzx011011/3DPrinter_Qt6/releases/latest";

QVector<int> splitVersionParts(const QString &version)
{
  QVector<int> parts;
  QString current;
  for (const QChar ch : version) {
    if (ch.isDigit()) {
      current += ch;
    } else if (!current.isEmpty()) {
      parts.append(current.toInt());
      current.clear();
      if (parts.size() == 3)
        break;
    }
  }
  if (!current.isEmpty() && parts.size() < 3)
    parts.append(current.toInt());
  while (parts.size() < 3)
    parts.append(0);
  return parts;
}

bool remoteVersionNewer(const QString &remoteTag)
{
  // Strip a leading 'v'/'V' and any -suffix; compare the first three numeric
  // components component-wise.
  QString remote = remoteTag.trimmed();
  if (remote.startsWith(QLatin1Char('v')) || remote.startsWith(QLatin1Char('V')))
    remote.remove(0, 1);
  const int dash = remote.indexOf(QLatin1Char('-'));
  if (dash >= 0)
    remote.truncate(dash);
  const QVector<int> r = splitVersionParts(remote);
  const QVector<int> l = splitVersionParts(QString::fromLatin1(kOwzxUpdateVersion));
  for (int i = 0; i < 3; ++i) {
    if (r[i] != l[i])
      return r[i] > l[i];
  }
  return false;
}
}  // namespace

QNetworkAccessManager *BackendContext::probeNam()
{
  if (!probeNam_)
    probeNam_ = new QNetworkAccessManager(this);
  return probeNam_;
}

void BackendContext::setUpdateCheckRunning(bool running)
{
  if (updateCheckRunning_ == running)
    return;
  updateCheckRunning_ = running;
  emit updateCheckRunningChanged();
}

void BackendContext::checkForUpdates()
{
  // Upstream Help > Check for Update (MainFrame.cpp:2160): async release
  // check with a user-visible result either way.
  if (updateCheckRunning_)
    return;
  setUpdateCheckRunning(true);
  QNetworkRequest request(QUrl(QString::fromLatin1(kOwzxUpdateRepoApi)));
  request.setRawHeader("User-Agent", "OWzxSlicer");
  request.setTransferTimeout(10000);
  QNetworkReply *reply = probeNam()->get(request);
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    reply->deleteLater();
    setUpdateCheckRunning(false);
    if (reply->error() != QNetworkReply::NoError) {
      const QString message = tr("更新检查失败：%1").arg(reply->errorString());
      qWarning("[Backend] update check failed: %s", reply->errorString().toUtf8().constData());
      postNotification(message, tr("检查更新"), NotiError);
      emit updateCheckFinished(false, false, message);
      return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    const QString tag = doc.object().value(QStringLiteral("tag_name")).toString();
    if (tag.isEmpty()) {
      const QString message = tr("更新服务器返回了无法解析的数据");
      postNotification(message, tr("检查更新"), NotiError);
      emit updateCheckFinished(false, false, message);
      return;
    }
    const bool available = remoteVersionNewer(tag);
    const QString message = available
        ? tr("发现新版本 %1，请到项目发布页下载。").arg(tag)
        : tr("当前已是最新版本（%1）。").arg(QString::fromLatin1(kOwzxUpdateVersion));
    postNotification(message, tr("检查更新"), NotiInfo);
    emit updateCheckFinished(true, available, message);
  });
}

bool BackendContext::openConfigFolder()
{
  // Upstream Help > Show Configuration Folder (MainFrame.cpp:2146,
  // desktop_open_datadir_folder).
  const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (!QDir(dir).exists())
    QDir().mkpath(dir);
  return QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void BackendContext::runNetworkTest()
{
  // Upstream NetworkTestDialog connectivity probe: DNS resolve followed by an
  // HTTPS GET whose round-trip time is reported as the latency.
  QElapsedTimer dnsTimer;
  dnsTimer.start();
  QHostInfo::lookupHost(QStringLiteral("api.github.com"), this,
                        [this](const QHostInfo &dns) {
    const bool dnsOk = dns.error() == QHostInfo::NoError && !dns.addresses().isEmpty();
    // U04: the resolved address feeds the dialog log's ip event (upstream
    // Http::on_ip_resolve, NetworkTestDialog.cpp:285-288).
    const QString resolvedIp = dnsOk ? dns.addresses().first().toString() : QString();
    if (!dnsOk) {
      emit networkTestFinished(false, false, -1, tr("DNS 解析失败：%1").arg(dns.errorString()));
      return;
    }
    QNetworkRequest request(QUrl(QStringLiteral("https://api.github.com")));
    request.setRawHeader("User-Agent", "OWzxSlicer");
    // U04: 10 s to match upstream http.timeout_max(10)
    // (NetworkTestDialog.cpp:270).
    request.setTransferTimeout(10000);
    QElapsedTimer latencyTimer;
    latencyTimer.start();
    QNetworkReply *reply = probeNam()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, latencyTimer, resolvedIp] {
      reply->deleteLater();
      // U04: only HTTP 200 counts as online, matching upstream
      // `if (status == 200) result = 0;` (NetworkTestDialog.cpp:275-277).
      // The previous `status.isValid()` check treated HTTP 500 as online.
      const QVariant httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
      const bool online = httpStatus.isValid() && httpStatus.toInt() == 200;
      const int latencyMs = static_cast<int>(latencyTimer.elapsed());
      QString detail;
      if (online)
        detail = tr("HTTPS 连接正常（ip %1，HTTP 200）").arg(resolvedIp);
      else if (httpStatus.isValid())
        detail = tr("HTTPS 连接失败：HTTP %1").arg(httpStatus.toInt());
      else
        detail = tr("HTTPS 连接失败：%1").arg(reply->errorString());
      emit networkTestFinished(true, online, latencyMs, detail);
    });
  });
  Q_UNUSED(dnsTimer);
}

void BackendContext::testPrintHost(const QString &hostUrl, const QString &apiKey,
                                   const QString &user, const QString &password)
{
  // Upstream PrintHostDialog "test connection": reachability GET against the
  // configured host (PhysicalPrinterDialog.cpp:210-265 -> PrintHost::test).
  // Any HTTP response counts as reachable. The dialog's credential fields are
  // carried along: apiKey goes out as the OctoPrint-style X-Api-Key header
  // (upstream atKeyPassword auth), user/password as HTTP Basic credentials
  // (upstream atUserPassword uses digest -- Http::set_auth; Qt's
  // challenge-based QAuthenticator is not wired into this probe, so a
  // digest-only server will still reject the request).
  QString url = hostUrl.trimmed();
  if (url.isEmpty()) {
    emit printHostTestFinished(false, tr("主机地址为空"));
    return;
  }
  if (!url.startsWith(QStringLiteral("http://")) && !url.startsWith(QStringLiteral("https://")))
    url.prepend(QStringLiteral("http://"));
  const QUrl targetUrl(url);
  QNetworkRequest hostProbe(targetUrl);
  hostProbe.setRawHeader("User-Agent", "OWzxSlicer");
  if (!apiKey.isEmpty())
    hostProbe.setRawHeader("X-Api-Key", apiKey.toUtf8());
  if (!user.isEmpty()) {
    QByteArray credentials = (user + ":" + password).toUtf8().toBase64();
    hostProbe.setRawHeader("Authorization", "Basic " + credentials);
  }
  hostProbe.setTransferTimeout(8000);
  QNetworkReply *reply = probeNam()->get(hostProbe);
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    reply->deleteLater();
    const QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const bool ok = reply->error() == QNetworkReply::NoError || status.isValid();
    const QString detail = ok
        ? tr("连接成功（HTTP %1）").arg(status.isValid() ? status.toString() : QStringLiteral("OK"))
        : tr("连接失败：%1").arg(reply->errorString());
    emit printHostTestFinished(ok, detail);
  });
}
