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

QTEST_MAIN(QmlUiAuditTests)
#include "QmlUiAuditTests.moc"
