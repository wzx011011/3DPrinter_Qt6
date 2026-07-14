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
  void test_view_mode_availability_reports_data_unavailable_modes();
  void test_summary_mode_has_no_gradient_legend();
  void test_divergent_role_colors_correct();
  void test_all_view_modes_keep_valid_gcv1_payload();

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

// GREEN since Plan 55-02: PreviewViewModel::roleForType maps each upstream
// ;TYPE: display string DIRECTLY to its canonical libvgcode EGCodeExtrusionRole
// index (NOT the libslic3r ExtrusionRole integer -- the two enums diverge past
// index 6; see 55-RESEARCH.md Pitfall 6). The divergent indices (Ironing->7,
// Bottom surface->15, etc.) are asserted explicitly as the regression guard.
void PreviewParserTests::test_role_string_mapping_covers_upstream_enum()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);

  // Indices 0..6 are identical across both enums; 7..18 diverge.
  QCOMPARE(preview.roleForType(QStringLiteral("Inner wall")), 1);
  QCOMPARE(preview.roleForType(QStringLiteral("Outer wall")), 2);
  QCOMPARE(preview.roleForType(QStringLiteral("Overhang wall")), 3);
  QCOMPARE(preview.roleForType(QStringLiteral("Sparse infill")), 4);
  QCOMPARE(preview.roleForType(QStringLiteral("Internal solid infill")), 5);
  QCOMPARE(preview.roleForType(QStringLiteral("Top surface")), 6);
  // Divergent roles -- the libslic3r integer would be WRONG here.
  QCOMPARE(preview.roleForType(QStringLiteral("Ironing")), 7);              // NOT 8
  QCOMPARE(preview.roleForType(QStringLiteral("Bridge")), 8);               // NOT 9
  QCOMPARE(preview.roleForType(QStringLiteral("Gap infill")), 9);           // NOT 11
  QCOMPARE(preview.roleForType(QStringLiteral("Skirt")), 10);               // NOT 12
  QCOMPARE(preview.roleForType(QStringLiteral("Support")), 11);             // NOT 14
  QCOMPARE(preview.roleForType(QStringLiteral("Support interface")), 12);   // NOT 15
  QCOMPARE(preview.roleForType(QStringLiteral("Prime tower")), 13);         // NOT 17
  QCOMPARE(preview.roleForType(QStringLiteral("Custom")), 14);              // NOT 18
  QCOMPARE(preview.roleForType(QStringLiteral("Bottom surface")), 15);      // NOT 7
  QCOMPARE(preview.roleForType(QStringLiteral("Internal Bridge")), 16);     // NOT 10
  QCOMPARE(preview.roleForType(QStringLiteral("Brim")), 17);                // NOT 13
  QCOMPARE(preview.roleForType(QStringLiteral("Support transition")), 18);  // NOT 16
  QCOMPARE(preview.roleForType(QStringLiteral("Multiple")), 19);
  // Travel / unrecognized -> 0 (None).
  QCOMPARE(preview.roleForType(QStringLiteral("")), 0);
  QCOMPARE(preview.roleForType(QStringLiteral("Nonexistent role")), 0);
}

// GREEN since Plan 55-02: viewModes() returns the 17 upstream EViewType display
// names in upstream update_by_mode order (libvgcode/include/Types.hpp:80-103).
void PreviewParserTests::test_view_modes_match_upstream_seventeen()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);

  const QStringList modes = preview.viewModes();
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
  // Head/tail ordering guard: Summary first, Tool last (upstream order).
  QCOMPARE(modes.first(), QStringLiteral("Summary"));
  QCOMPARE(modes.last(), QStringLiteral("Tool"));
}

void PreviewParserTests::test_view_mode_availability_reports_data_unavailable_modes()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);

  const QStringList modes = preview.viewModes();
  const QStringList unavailableModes = {
    QStringLiteral("Actual Speed"),
    QStringLiteral("Jerk"),
    QStringLiteral("Actual Flow"),
    QStringLiteral("Pressure Advance")
  };

  for (const QString &modeName : unavailableModes) {
    const int index = modes.indexOf(modeName);
    QVERIFY2(index >= 0, qPrintable(QStringLiteral("Missing mode: %1").arg(modeName)));
    QVERIFY2(!preview.viewModeAvailable(index),
             qPrintable(QStringLiteral("%1 must be marked unavailable for the current Qt data path").arg(modeName)));
    QVERIFY2(!preview.viewModeStatusText(index).isEmpty(),
             qPrintable(QStringLiteral("%1 must expose an honest status string").arg(modeName)));
  }

  const QStringList availableModes = {
    QStringLiteral("Summary"),
    QStringLiteral("Line Type"),
    QStringLiteral("Filament"),
    QStringLiteral("Fan Speed"),
    QStringLiteral("Tool")
  };
  for (const QString &modeName : availableModes) {
    const int index = modes.indexOf(modeName);
    QVERIFY2(index >= 0, qPrintable(QStringLiteral("Missing mode: %1").arg(modeName)));
    QVERIFY2(preview.viewModeAvailable(index),
             qPrintable(QStringLiteral("%1 must not be incorrectly gated").arg(modeName)));
    QVERIFY2(preview.viewModeStatusText(index).isEmpty(),
             qPrintable(QStringLiteral("%1 must not show an unavailable-mode status").arg(modeName)));
  }

  const int actualSpeedIndex = modes.indexOf(QStringLiteral("Actual Speed"));
  preview.setViewModeIndex(actualSpeedIndex);
  QVERIFY2(!preview.currentViewModeAvailable(),
           "currentViewModeAvailable must follow setViewModeIndex");
  QVERIFY2(!preview.currentViewModeStatus().isEmpty(),
           "currentViewModeStatus must follow setViewModeIndex");
}

// GREEN since Plan 55-02: Summary mode (upstream EViewType index 0) renders
// statistics only and produces no gradient legend. legendType() stays 0
// (discrete) and legendItems() is empty when viewModeIndex maps to Summary.
void PreviewParserTests::test_summary_mode_has_no_gradient_legend()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);

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
}

// NEW regression guard (Plan 55-02): the libslic3r ExtrusionRole and libvgcode
// EGCodeExtrusionRole enums DIVERGE past index 6. This test verifies the
// string->color mapping end-to-end so a future edit that accidentally indexes
// kRoleColors by the libslic3r integer is caught. 'Ironing' must map to the
// canonical libvgcode index 7 -> orange (255,140,105); 'Bottom surface' must
// map to index 15 -> purple (102,92,199). If the two indices were swapped
// (the bug), Ironing would render purple and Bottom surface orange.
void PreviewParserTests::test_divergent_role_colors_correct()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);

  // roleForType returns the canonical libvgcode index (NOT the libslic3r int).
  const int ironingRole = preview.roleForType(QStringLiteral("Ironing"));
  QCOMPARE(ironingRole, 7);   // libslic3r would give 8 -> wrong color slot
  const int bottomRole = preview.roleForType(QStringLiteral("Bottom surface"));
  QCOMPARE(bottomRole, 15);   // libslic3r would give 7 -> wrong color slot

  // roleColor returns the upstream DEFAULT_EXTRUSION_ROLES_COLORS at the
  // canonical index. Ironing(7) == (255,140,105); Bottom surface(15) == (102,92,199).
  const QColor ironing = preview.roleColor(ironingRole);
  QCOMPARE(ironing.red(), 255);
  QCOMPARE(ironing.green(), 140);
  QCOMPARE(ironing.blue(), 105);

  const QColor bottom = preview.roleColor(bottomRole);
  QCOMPARE(bottom.red(), 102);
  QCOMPARE(bottom.green(), 92);
  QCOMPARE(bottom.blue(), 199);
}

void PreviewParserTests::test_all_view_modes_keep_valid_gcv1_payload()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);

  QVERIFY2(preview.loadGCodeForPreview(fixturePath()),
           "fixture should parse before validating view-mode payload survival");

  const QStringList modes = preview.viewModes();
  for (int i = 0; i < modes.size(); ++i) {
    preview.setViewModeIndex(i);
    const QByteArray payload = preview.gcodePreviewData();
    QVERIFY2(payload.size() > 8,
             qPrintable(QStringLiteral("%1 mode must keep a non-empty preview payload").arg(modes.at(i))));
    QVERIFY2(payload.startsWith("GCV1"),
             qPrintable(QStringLiteral("%1 mode must keep the GCV1 wire format").arg(modes.at(i))));
  }
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
