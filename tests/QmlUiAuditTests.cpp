#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QtTest>

class QmlUiAuditTests final : public QObject
{
  Q_OBJECT

private slots:
  void topLevelUiHasNoVisiblePlaceholdersOrNoopActions();
  void mainChromeUsesThemeTokens();
  void sidebarCopyIsLocalizedAndOperationalTextIsReadable();
  void mainRegistersSoftwareViewportByDefault();
  void mainRegistersRhiViewportOnlyBehindExplicitGate();
  void visiblePlaceholderSurfacesAreHonest();
  // Phase 22 (UI-3): actively guard the v3.0 Phase 17 plate-lifecycle menu wiring
  void plateContextMenuItemsWiredAndNonEmpty();

private:
  QString readSource(const QString &relativePath) const;
};

QString QmlUiAuditTests::readSource(const QString &relativePath) const
{
  QFile file(QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR)).filePath(relativePath));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return {};
  return QString::fromUtf8(file.readAll());
}

void QmlUiAuditTests::topLevelUiHasNoVisiblePlaceholdersOrNoopActions()
{
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  const QString plater = readSource(QStringLiteral("src/qml_gui/pages/Plater.qml"));
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");
  QVERIFY2(!plater.isEmpty(), "Unable to read Plater.qml");

  QVERIFY2(!mainQml.contains(QStringLiteral("占位 Tab")),
           "main.qml must not expose placeholder tab copy at runtime");
  QVERIFY2(!topbar.contains(QStringLiteral("label: qsTr(\"占位\")")),
           "BBLTopbar must not expose placeholder notebook tabs");
  QVERIFY2(!plater.contains(QStringLiteral("reserved (v2.0 placeholder)")),
           "Plater must not expose implementation-phase placeholder copy");

  const QStringList noOpPatterns = {
    QStringLiteral("onExportProjectRequested: { /* TODO"),
    QStringLiteral("onExportModelRequested: { /* TODO"),
    QStringLiteral("onPreferencesRequested: { /* TODO"),
    QStringLiteral("CxMenuItem { text: qsTr(\"Documentation\"); onTriggered: {}"),
    QStringLiteral("MenuItem { text: qsTr(\"关于 OWzx\"); onTriggered: {}")
  };
  for (const QString &pattern : noOpPatterns) {
    QVERIFY2(!mainQml.contains(pattern) && !topbar.contains(pattern),
             qPrintable(QStringLiteral("Top-level no-op action remains: %1").arg(pattern)));
  }
}

void QmlUiAuditTests::mainChromeUsesThemeTokens()
{
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");
  const QStringList forbiddenTopbarColors = {
    QStringLiteral("#181818"),
    QStringLiteral("#2a2a2a"),
    QStringLiteral("#4CD582"),
    QStringLiteral("#c0c0c0"),
    QStringLiteral("#303030")
  };

  for (const QString &color : forbiddenTopbarColors) {
    QVERIFY2(!topbar.contains(color),
             qPrintable(QStringLiteral("Topbar chrome should use Theme tokens instead of %1").arg(color)));
  }
}

void QmlUiAuditTests::sidebarCopyIsLocalizedAndOperationalTextIsReadable()
{
  const QString sidebar = readSource(QStringLiteral("src/qml_gui/panels/LeftSidebar.qml"));
  QVERIFY2(!sidebar.isEmpty(), "Unable to read LeftSidebar.qml");

  const QStringList mixedLanguageLabels = {
    QStringLiteral("qsTr(\"Process\")"),
    QStringLiteral("qsTr(\"Global\")"),
    QStringLiteral("qsTr(\"Objects\")"),
    QStringLiteral("qsTr(\"Advanced\")"),
    QStringLiteral("qsTr(\"Object Settings\")"),
    QStringLiteral("qsTr(\"Layer Height\")"),
    QStringLiteral("qsTr(\"Infill Density\")"),
    QStringLiteral("qsTr(\"Print Speed\")"),
    QStringLiteral("qsTr(\"Variable Layer Height\")"),
    QStringLiteral("qsTr(\"Variable layer height editor (reserved)\")"),
    QStringLiteral("qsTr(\"(parameter list — reserved)\")"),
    QStringLiteral("qsTr(\"Rename Preset\")")
  };

  for (const QString &label : mixedLanguageLabels) {
    QVERIFY2(!sidebar.contains(label),
             qPrintable(QStringLiteral("Mixed-language or reserved runtime label remains: %1").arg(label)));
  }

  const QRegularExpression tinyFont(QStringLiteral("font\\.pixelSize:\\s*(?:7|8|9)\\b"));
  QVERIFY2(!tinyFont.match(sidebar).hasMatch(),
           "LeftSidebar operational controls should not use sub-10px text");
}

void QmlUiAuditTests::mainRegistersSoftwareViewportByDefault()
{
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  const QString verifyScript = readSource(QStringLiteral("scripts/auto_verify_with_vcvars.ps1"));
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");
  QVERIFY2(!verifyScript.isEmpty(), "Unable to read auto_verify_with_vcvars.ps1");

  QVERIFY2(mainCpp.contains(QStringLiteral("qEnvironmentVariableIsSet(\"OWZX_OPENGL\")")),
           "main_qml.cpp must gate OpenGL viewport selection on OWZX_OPENGL");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<SoftwareViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "default GLViewport registration must use SoftwareViewport");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<GLViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "OpenGL GLViewport registration must remain available behind OWZX_OPENGL");
  QVERIFY2(!verifyScript.contains(QStringLiteral("OWZX_OPENGL")),
           "canonical startup smoke should cover the default software viewport path");
}

void QmlUiAuditTests::mainRegistersRhiViewportOnlyBehindExplicitGate()
{
  const QString mainCpp = readSource(QStringLiteral("src/qml_gui/main_qml.cpp"));
  const QString verifyScript = readSource(QStringLiteral("scripts/auto_verify_with_vcvars.ps1"));
  const QString selectorHeader = readSource(QStringLiteral("src/qml_gui/Renderer/RhiBackendSelector.h"));
  const QString selectorSource = readSource(QStringLiteral("src/qml_gui/Renderer/RhiBackendSelector.cpp"));
  QVERIFY2(!mainCpp.isEmpty(), "Unable to read main_qml.cpp");
  QVERIFY2(!verifyScript.isEmpty(), "Unable to read auto_verify_with_vcvars.ps1");
  QVERIFY2(!selectorHeader.isEmpty(), "Unable to read RhiBackendSelector.h");
  QVERIFY2(!selectorSource.isEmpty(), "Unable to read RhiBackendSelector.cpp");

  QVERIFY2(mainCpp.contains(QStringLiteral("OWZX_RHI_RENDERER")),
           "QRhi viewport selection must be behind OWZX_RHI_RENDERER");
  QVERIFY2(mainCpp.contains(QStringLiteral("qEnvironmentVariableIsSet(\"OWZX_OPENGL\")")),
           "legacy OWZX_OPENGL path must stay independent from QRhi");
  QVERIFY2(mainCpp.contains(QStringLiteral("qmlRegisterType<SoftwareViewport>(\"OWzxGL\", 1, 0, \"GLViewport\")")),
           "SoftwareViewport must remain the default/fallback GLViewport registration");
  QVERIFY2(!verifyScript.contains(QStringLiteral("OWZX_RHI_RENDERER")),
           "canonical verification must not enable QRhi by default");

  QVERIFY2(selectorHeader.contains(QStringLiteral("RhiBackendSelection")),
           "RhiBackendSelector must expose structured backend diagnostics");
  QVERIFY2(selectorSource.contains(QStringLiteral("Direct3D12")),
           "QRhi app selector must try Direct3D12 on Windows");
  QVERIFY2(selectorSource.contains(QStringLiteral("Direct3D11")),
           "QRhi app selector must keep Direct3D11 fallback on Windows");
  QVERIFY2(!selectorSource.contains(QStringLiteral("QRhi::Vulkan")),
           "Vulkan must not be part of the default app selector while QtGui Vulkan is disabled");
}

void QmlUiAuditTests::visiblePlaceholderSurfacesAreHonest()
{
  const QString topbar = readSource(QStringLiteral("src/qml_gui/BBLTopbar.qml"));
  const QString mainQml = readSource(QStringLiteral("src/qml_gui/main.qml"));
  const QString sidebar = readSource(QStringLiteral("src/qml_gui/panels/LeftSidebar.qml"));
  const QString modelMall = readSource(QStringLiteral("src/qml_gui/pages/ModelMallPage.qml"));
  const QString preferences = readSource(QStringLiteral("src/qml_gui/pages/PreferencesPage.qml"));
  QVERIFY2(!topbar.isEmpty(), "Unable to read BBLTopbar.qml");
  QVERIFY2(!mainQml.isEmpty(), "Unable to read main.qml");
  QVERIFY2(!sidebar.isEmpty(), "Unable to read LeftSidebar.qml");
  QVERIFY2(!modelMall.isEmpty(), "Unable to read ModelMallPage.qml");
  QVERIFY2(!preferences.isEmpty(), "Unable to read PreferencesPage.qml");

  const QRegularExpression emptyClick(QStringLiteral("\\bon(?:Clicked|Triggered):\\s*\\{\\s*\\}"));
  QVERIFY2(!emptyClick.match(topbar).hasMatch(), "Topbar must not contain empty click/trigger handlers");
  QVERIFY2(!emptyClick.match(mainQml).hasMatch(), "main.qml must not contain empty click/trigger handlers");
  QVERIFY2(!emptyClick.match(sidebar).hasMatch(), "LeftSidebar must not contain empty click/trigger handlers");
  QVERIFY2(!emptyClick.match(preferences).hasMatch(), "PreferencesPage must not contain empty click/trigger handlers");

  const QStringList forbiddenRuntimeCopy = {
    QStringLiteral("qsTr(\"Search models, authors, tags...\")"),
    QStringLiteral("qsTr(\"Publish\")"),
    QStringLiteral("qsTr(\"Recommended\")"),
    QStringLiteral("qsTr(\"Popular\")"),
    QStringLiteral("qsTr(\"Newest\")"),
    QStringLiteral("qsTr(\"Free\")"),
    QStringLiteral("qsTr(\"Favorites\")")
  };
  for (const QString &copy : forbiddenRuntimeCopy) {
    QVERIFY2(!modelMall.contains(copy),
             qPrintable(QStringLiteral("ModelMall must not expose fake marketplace copy: %1").arg(copy)));
  }

  const QStringList forbiddenSidebarMarkers = {
    QStringLiteral("TODO SIDEBAR-08"),
    QStringLiteral("TODO SIDEBAR-09"),
    QStringLiteral("TODO SIDEBAR-10"),
    QStringLiteral("TODO SIDEBAR-14"),
    QStringLiteral("TODO SIDEBAR-15")
  };
  for (const QString &marker : forbiddenSidebarMarkers) {
    QVERIFY2(!sidebar.contains(marker),
             qPrintable(QStringLiteral("LeftSidebar runtime placeholder marker remains: %1").arg(marker)));
  }

  QVERIFY2(preferences.contains(QStringLiteral("enabled: false")),
           "Unavailable Preferences update check must be visibly disabled");
}

void QmlUiAuditTests::plateContextMenuItemsWiredAndNonEmpty()
{
  // Phase 22 (UI-3): actively guard the v3.0 Phase 17 plate-lifecycle menu wiring.
  // Without this, a regression (deleted menu item / empty onTriggered / broken
  // enabled) would pass the other 7 audit tests, since they only check generic
  // honest-UI rules, not the specific Phase 17 items.
  const QString preparePage = readSource(QStringLiteral("src/qml_gui/pages/PreparePage.qml"));
  QVERIFY2(!preparePage.isEmpty(), "Unable to read PreparePage.qml");

  // The three Phase 17 operations must be wired to real menu actions.
  QVERIFY2(preparePage.contains(QStringLiteral("clonePlate(")),
           "PreparePage must wire a clonePlate plate-context-menu action (PLATE-03)");
  QVERIFY2(preparePage.contains(QStringLiteral("movePlate(")),
           "PreparePage must wire a movePlate plate-context-menu action (PLATE-04)");
  QVERIFY2(preparePage.contains(QStringLiteral("setPlatePrintable(")),
           "PreparePage must wire a setPlatePrintable plate-context-menu action (PLATE-05)");

  // Strengthen the honest-UI contract: no empty onTriggered handlers anywhere in
  // PreparePage (Phase 14 forbade these; Phase 22 extends the guard here).
  QVERIFY2(!preparePage.contains(QStringLiteral("onTriggered: {}")),
           "PreparePage must not contain empty onTriggered: {} handlers");
}

QTEST_MAIN(QmlUiAuditTests)
#include "QmlUiAuditTests.moc"
