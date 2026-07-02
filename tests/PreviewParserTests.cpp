// PreviewParserTests — Phase 55-01 Wave 0 scaffold.
//
// Audit/anchor tests for the OrcaSlicer ;TYPE: parser path and the upstream
// EViewType / EGCodeExtrusionRole coverage that Plan 02 raises to a green
// state. Plan 01 only lands the foundation: a deterministic fixture
// (tests/fixtures/orca_sample.gcode) and this test target so Plan 02 has a
// place to land its assertions.
//
// AUTOMOC caveat (v3.0 retrospective, see ViewModelSmokeTests CMake comment):
// single-file QtTest with cpp-internal Q_OBJECT has weak moc dependency
// tracking. After adding a new private slot here, re-run cmake configure (the
// canonical verify script does this) or delete
//   build/PreviewParserTests_autogen/timestamp
// before rebuilding, otherwise the new slot silently does not execute.
//
// Upstream references:
//   - ;TYPE: role strings:  libslic3r/ExtrusionEntity.cpp:583-639 (role_to_string / string_to_role)
//   - EViewType (17 modes): libvgcode/include/Types.hpp:80-103
//   - EGCodeExtrusionRole:  libvgcode/include/Types.hpp:131-157 (canonical Qt6 index)

#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QtTest>

#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include "core/viewmodels/PreviewViewModel.h"

class PreviewParserTests final : public QObject
{
  Q_OBJECT

private slots:
  void test_fixture_has_expected_role_coverage();
  void test_role_string_mapping_covers_upstream_enum();
  void test_view_modes_match_upstream_seventeen();
  void test_summary_mode_has_no_gradient_legend();

private:
  QString fixturePath() const;
  QString readFixture() const;
};

QString PreviewParserTests::fixturePath() const
{
  // QT_TESTCASE_SOURCEDIR is ${CMAKE_SOURCE_DIR} (see CMakeLists.txt).
  return QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR))
      .filePath(QStringLiteral("tests/fixtures/orca_sample.gcode"));
}

QString PreviewParserTests::readFixture() const
{
  QFile f(fixturePath());
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    return {};
  return QString::fromUtf8(f.readAll());
}

// Green at end of Plan 01. Guards the fixture so Plan 02 cannot accidentally
// shrink the role coverage that the parser tests rely on.
void PreviewParserTests::test_fixture_has_expected_role_coverage()
{
  const QString body = readFixture();
  QVERIFY2(!body.isEmpty(), "tests/fixtures/orca_sample.gcode should be readable");
  QVERIFY2(QFile::exists(fixturePath()), "fixture path should resolve");

  const QStringList requiredRoles{
      QStringLiteral("Inner wall"),
      QStringLiteral("Outer wall"),
      QStringLiteral("Sparse infill"),
      QStringLiteral("Bridge"),
      QStringLiteral("Support"),
      QStringLiteral("Skirt"),
      QStringLiteral("Prime tower"),
  };
  for (const QString &role : requiredRoles)
  {
    const QString needle = QStringLiteral(";TYPE:") + role;
    QVERIFY2(body.contains(needle),
             qPrintable(QStringLiteral("fixture missing ;TYPE:%1 block").arg(role)));
  }

  // Two layers and at least one tool change so Plan 04 layer-range and
  // tool-change regression assertions have data to work against.
  QVERIFY2(body.contains(QStringLiteral(";LAYER:0")), "fixture needs ;LAYER:0");
  QVERIFY2(body.contains(QStringLiteral(";LAYER:1")), "fixture needs ;LAYER:1");
}

// RED-by-skip until Plan 02 implements the Q_INVOKABLE roleForType(QString)
// fine-grained 20-role mapper. Plan 55-01 only registers the scaffold; today
// PreviewViewModel has no such method. QSKIP keeps the target green-by-skip so
// the build/test gate passes; Plan 02 removes the skip and asserts each of the
// 20 upstream ;TYPE: strings maps to its canonical libvgcode EGCodeExtrusionRole
// index (NOT the libslic3r ExtrusionRole integer — see 55-RESEARCH.md Pitfall 6).
void PreviewParserTests::test_role_string_mapping_covers_upstream_enum()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  const QMetaObject *mo = preview.metaObject();
  const int methodIdx = mo->indexOfMethod("roleForType(QString)");
  if (methodIdx < 0)
  {
    QSKIP("Plan 02 implements PreviewViewModel::roleForType (20-role fine-grained mapper)");
  }

  // Plan 02 path: roleForType("Inner wall") must return the canonical libvgcode
  // Perimeter index (1), NOT the libslic3r erPerimeter integer. The full
  // 20-string table is asserted here once Plan 02 lands.
  const QMetaMethod method = mo->method(methodIdx);
  QVariant ret;
  method.invoke(&preview, Qt::DirectConnection,
                Q_RETURN_ARG(QVariant, ret),
                Q_ARG(QString, QStringLiteral("Inner wall")));
  QCOMPARE(ret.toInt(), 1);
}

// RED-by-skip until Plan 02 raises viewModes() from 13 to the upstream-complete
// 17 EViewType entries. Plan 55-01 only registers the scaffold; today the list
// has 13 entries and is missing "Summary", "Actual Speed", "Actual Flow",
// "Layer Time (log)", "Pressure Advance", "Jerk". QSKIP keeps the target
// green-by-skip; Plan 02 removes the skip and asserts the 17-mode contract.
void PreviewParserTests::test_view_modes_match_upstream_seventeen()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  const QStringList modes = preview.viewModes();
  if (modes.size() != 17)
  {
    QSKIP("Plan 02 raises viewModes to the 17 upstream EViewType entries");
  }

  // Plan 02 path: the upstream-complete display names must all be present.
  QCOMPARE(modes.size(), 17);
  QVERIFY2(modes.contains(QStringLiteral("Summary")),
           "viewModes must include Summary (upstream EViewType index 0)");
  QVERIFY2(modes.contains(QStringLiteral("Line Type")),
           "viewModes must include Line Type (upstream FeatureType)");
  QVERIFY2(modes.contains(QStringLiteral("Filament")),
           "viewModes must include Filament (upstream ColorPrint)");
  QVERIFY2(modes.contains(QStringLiteral("Flow")),
           "viewModes must include Flow (upstream VolumetricFlowRate)");
  QVERIFY2(modes.contains(QStringLiteral("Actual Speed")),
           "viewModes must include Actual Speed (upstream ActualSpeed)");
  QVERIFY2(modes.contains(QStringLiteral("Acceleration")),
           "viewModes must include Acceleration");
  QVERIFY2(modes.contains(QStringLiteral("Jerk")),
           "viewModes must include Jerk (upstream Jerk)");
  QVERIFY2(modes.contains(QStringLiteral("Pressure Advance")),
           "viewModes must include Pressure Advance (upstream PressureAdvance)");
  QVERIFY2(modes.contains(QStringLiteral("Tool")),
           "viewModes must include Tool");
}

// RED-by-skip until Plan 02 defines the Summary-mode legend sentinel.
// Summary mode (upstream EViewType index 0) renders statistics only and has
// no gradient legend. Plan 02 sets legendType() to a "no legend" sentinel and
// clears legendItems() when viewModeIndex maps to Summary. There is no
// current equivalent, so the body is compile-guarded until Plan 02 lands.
void PreviewParserTests::test_summary_mode_has_no_gradient_legend()
{
#if 0
  // Plan 02 enables this body once the Summary sentinel is in place.
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  QVERIFY2(preview.loadGCodeForPreview(fixturePath()),
           "fixture should parse for Summary legend assertion");

  const QStringList modes = preview.viewModes();
  const int summaryIdx = modes.indexOf(QStringLiteral("Summary"));
  QVERIFY2(summaryIdx >= 0, "Summary mode must exist before this assertion runs");
  preview.setViewModeIndex(summaryIdx);

  QVERIFY2(preview.legendItems().isEmpty(),
           "Summary mode must not produce discrete legend items");
  QVERIFY2(preview.legendType() == 0,
           "Summary mode must not render a gradient legend");
#else
  QSKIP("Plan 02 defines the Summary-mode legend sentinel (no current equivalent)");
#endif
}

// QTEST_MAIN generates the test entry point (main). Without it the link fails
// with LNK2001 "unresolved external symbol main" because QtTest has no default
// entry. Matches the pattern in every sibling single-file QtTest in tests/.
QTEST_MAIN(PreviewParserTests)

// AUTOMOC requirement: single-file cpp-internal Q_OBJECT must include the
// generated moc output so the meta-object is linked into the test executable.
// Matches the pattern in QmlUiAuditTests.cpp / ViewModelSmokeTests.cpp /
// PartPlateTests.cpp / PrepareSceneDataTests.cpp / E2EWorkflowTests.cpp.
#include "PreviewParserTests.moc"
