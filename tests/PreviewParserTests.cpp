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
#include <QTemporaryFile>
#include <QtTest>
#include <cstring>

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
  // Phase 238 (PREV-03): retract/unretract/wipe/seam move classification.
  void test_prev03_move_kind_classification();
  void test_prev03_seam_detected_on_closed_outer_wall_loop();
  void test_prev03_move_kind_toggles_repack_payload();
  // Phase 238 (PREV-05): filament split, stealth time, filament price.
  void test_prev05_filament_split_model_support_flushed_tower();
  void test_prev05_stealth_time_comment_and_estimate_flag();
  void test_prev05_filament_price_from_config_block();
  // Phase 238 (PREV-06): configured extruder colors + legend visibility.
  void test_prev06_configured_extruder_colors_override_fixed_cycle();
  void test_prev06_extruder_visibility_gates_filament_payload();

private:
  QString fixturePath() const;
  QString readFixture() const;
  // Phase 238: write synthetic gcode to a temp file and return its path.
  // The RAII guard removes the file even when QVERIFY aborts mid-test.
  struct TempGcode
  {
    QString path;
    ~TempGcode()
    {
      if (!path.isEmpty())
        QFile::remove(path);
    }
  };
  TempGcode writeTempGcode(const QString &content) const;
  // Segment count from the GCV1 payload header (int32 at offset 4).
  static int gcvSegmentCount(const QByteArray &payload);
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
  // v5.11: these 4 modes now parse real data (M220/M221/M205/M900) and are
  // available. They were previously gated as unavailable.
  const QStringList nowAvailableModes = {
    QStringLiteral("Actual Speed"),
    QStringLiteral("Jerk"),
    QStringLiteral("Actual Flow"),
    QStringLiteral("Pressure Advance")
  };

  for (const QString &modeName : nowAvailableModes) {
    const int index = modes.indexOf(modeName);
    QVERIFY2(index >= 0, qPrintable(QStringLiteral("Missing mode: %1").arg(modeName)));
    QVERIFY2(preview.viewModeAvailable(index),
             qPrintable(QStringLiteral("%1 must be available after v5.11 G-code parse extension").arg(modeName)));
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
  // v5.11: Actual Speed is now available (parses M220); currentViewModeAvailable
  // must follow setViewModeIndex and report true.
  QVERIFY2(preview.currentViewModeAvailable(),
           "currentViewModeAvailable must follow setViewModeIndex (Actual Speed is available after v5.11)");
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

// ── Phase 238 (PREV-03/05/06) helpers ──────────────────────────────────────

PreviewParserTests::TempGcode PreviewParserTests::writeTempGcode(const QString &content) const
{
  QTemporaryFile file;
  file.setAutoRemove(false);
  if (!file.open())
    return {};
  file.write(content.toUtf8());
  file.close();
  return {file.fileName()};
}

int PreviewParserTests::gcvSegmentCount(const QByteArray &payload)
{
  if (payload.size() < 8 || !payload.startsWith("GCV1"))
    return -1;
  int count = 0;
  std::memcpy(&count, payload.constData() + 4, 4);
  return count;
}

// PREV-03: upstream EMoveType classification port (GCodeProcessor.cpp:
// 2954-2968). E-only negative move = Retract, E-only positive move without
// displacement = Unretract, G10/G11 firmware retract/unretract synthesize the
// same event (GCodeProcessor.cpp:3818-3854), any move inside a
// "; WIPE_START".." WIPE_END" region = Wipe (GCodeProcessor.cpp:2260-2267).
void PreviewParserTests::test_prev03_move_kind_classification()
{
  const auto tmp = writeTempGcode(QStringLiteral(
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"       // Travel (no E, moved)
      "G1 X20 Y10 E1.0 F1800\n"  // Extrude
      "G1 E0.6 F2400\n"          // Retract (E-only negative, absolute mode)
      "G1 E1.0 F2400\n"          // Unretract (E-only positive)
      "G10\n"                    // firmware retract
      "G11\n"                    // firmware unretract
      "; WIPE_START\n"
      "G1 X22 Y10 F3000\n"       // Wipe (move while wiping)
      "; WIPE_END\n"
      "G1 X30 Y10 E1.5 F1800\n"  // Extrude again
      ));
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(!tmp.path.isEmpty(), "temp gcode file should be writable");
  QVERIFY2(preview.loadGCodeForPreview(tmp.path), "synthetic gcode should parse");

  QCOMPARE(preview.moveCountOfKind(PreviewViewModel::KindTravel), 1);
  QCOMPARE(preview.moveCountOfKind(PreviewViewModel::KindExtrude), 2);
  // E-only retract + G10.
  QCOMPARE(preview.moveCountOfKind(PreviewViewModel::KindRetract), 2);
  // E-only unretract + G11.
  QCOMPARE(preview.moveCountOfKind(PreviewViewModel::KindUnretract), 2);
  // The single move inside the WIPE region.
  QCOMPARE(preview.moveCountOfKind(PreviewViewModel::KindWipe), 1);
  // Open loop (never returns to its start) -> no seam.
  QCOMPARE(preview.moveCountOfKind(PreviewViewModel::KindSeam), 0);
}

// PREV-03: seam detection port (GCodeProcessor.cpp:3305-3345). A closed
// Outer wall loop stores one Seam marker at the loop's first vertex when the
// next non-wall move starts within the upstream 0.25mm threshold; an open
// loop must NOT store one.
void PreviewParserTests::test_prev03_seam_detected_on_closed_outer_wall_loop()
{
  const auto closed = writeTempGcode(QStringLiteral(
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G1 X20 Y10 E1.0 F1800\n"  // loop first vertex (10,10)
      "G1 X20 Y20 E2.0 F1800\n"
      "G1 X10 Y20 E3.0 F1800\n"
      "G1 X10 Y10 E4.0 F1800\n"  // loop closes back at (10,10)
      ";TYPE:Sparse infill\n"
      "G1 X15 Y15 E4.5 F1800\n"  // non-wall extrusion starts AT the first vertex
      ));
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(closed.path),
           "closed-loop synthetic gcode should parse");
  QCOMPARE(preview.moveCountOfKind(PreviewViewModel::KindSeam), 1);

  // Open loop: the wall extrusions end far from the first vertex, so the
  // close check fails and no seam is stored.
  const auto open = writeTempGcode(QStringLiteral(
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G1 X20 Y10 E1.0 F1800\n"  // loop first vertex (10,10)
      "G1 X20 Y20 E2.0 F1800\n"
      "G1 X10 Y20 E3.0 F1800\n"  // ends at (10,20) -- loop NOT closed
      ";TYPE:Sparse infill\n"
      "G1 X15 Y15 E3.5 F1800\n"  // starts 10mm away from (10,10)
      ));
  ProjectServiceMock project2;
  SliceService slice2(&project2);
  PreviewViewModel preview2(&project2, &slice2);
  QVERIFY2(preview2.loadGCodeForPreview(open.path),
           "open-loop synthetic gcode should parse");
  QCOMPARE(preview2.moveCountOfKind(PreviewViewModel::KindSeam), 0);

  // Regression on the shared fixture: its Outer wall square closes at its
  // start (55,55) and the following travel departs from that exact point,
  // so exactly one seam is expected (upstream records the seam at the close
  // even when the loop-ending move is a travel).
  ProjectServiceMock project3;
  SliceService slice3(&project3);
  PreviewViewModel preview3(&project3, &slice3);
  QVERIFY2(preview3.loadGCodeForPreview(fixturePath()), "fixture should parse");
  QCOMPARE(preview3.moveCountOfKind(PreviewViewModel::KindSeam), 1);
}

// PREV-03: the move-kind toggles must repack the GCV1 payload (upstream
// options_items checkboxes toggle the buffer visibility and refresh the
// render paths, GCodeViewer.cpp:4936-4950).
void PreviewParserTests::test_prev03_move_kind_toggles_repack_payload()
{
  const auto tmp = writeTempGcode(QStringLiteral(
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G1 X20 Y10 E1.0 F1800\n"
      "G1 E0.6 F2400\n"   // retract
      "G1 E1.0 F2400\n"   // unretract
      "G1 X30 Y10 E1.5 F1800\n"
      ));
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(tmp.path), "synthetic gcode should parse");

  const int withAll = gcvSegmentCount(preview.gcodePreviewData());
  QVERIFY2(withAll > 0, "payload should contain segments before toggling");

  preview.setShowRetractMoves(false);
  const int withoutRetract = gcvSegmentCount(preview.gcodePreviewData());
  QCOMPARE(withoutRetract, withAll - 1);

  preview.setShowUnretractMoves(false);
  const int withoutBoth = gcvSegmentCount(preview.gcodePreviewData());
  QCOMPARE(withoutBoth, withAll - 2);

  preview.setShowRetractMoves(true);
  preview.setShowUnretractMoves(true);
  QCOMPARE(gcvSegmentCount(preview.gcodePreviewData()), withAll);
}

// PREV-05: filament usage split into the upstream statistics columns
// Model/Support/Flushed/Tower (GCodeViewer.cpp:5161-5277; role accounting in
// GCodeProcessor.cpp:3002-3010, flush accounting = Unretract inside a
// FLUSH_START..FLUSH_END region, GCodeProcessor.cpp:3066-3074).
void PreviewParserTests::test_prev05_filament_split_model_support_flushed_tower()
{
  const auto tmp = writeTempGcode(QStringLiteral(
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G1 X20 Y10 E1.0 F1800\n"    // +1.0mm model
      ";TYPE:Support\n"
      "G1 X20 Y20 E1.5 F1800\n"    // +0.5mm support (role 11)
      ";TYPE:Prime tower\n"
      "G1 X30 Y20 E2.0 F1800\n"    // +0.5mm tower (role 13)
      "; FLUSH_START\n"
      "G1 E2.4 F2400\n"            // unretract while flushing: +0.4mm flushed
      "; FLUSH_END\n"
      ));
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(tmp.path), "split gcode should parse");

  const QVariantList rows = preview.filamentSplit();
  QCOMPARE(rows.size(), 4);
  QCOMPARE(rows.at(0).toMap().value(QStringLiteral("key")).toString(), QStringLiteral("model"));
  QCOMPARE(rows.at(1).toMap().value(QStringLiteral("key")).toString(), QStringLiteral("support"));
  QCOMPARE(rows.at(2).toMap().value(QStringLiteral("key")).toString(), QStringLiteral("flushed"));
  QCOMPARE(rows.at(3).toMap().value(QStringLiteral("key")).toString(), QStringLiteral("tower"));

  const double modelM = rows.at(0).toMap().value(QStringLiteral("lengthM")).toDouble();
  const double supportM = rows.at(1).toMap().value(QStringLiteral("lengthM")).toDouble();
  const double flushedM = rows.at(2).toMap().value(QStringLiteral("lengthM")).toDouble();
  const double towerM = rows.at(3).toMap().value(QStringLiteral("lengthM")).toDouble();
  // Tolerance covers float accumulation on the parser side (e.g. 2.4f-2.0f).
  QVERIFY2(qAbs(modelM - 0.001) < 1e-6, "model split must be 1.0mm = 0.001m");
  QVERIFY2(qAbs(supportM - 0.0005) < 1e-6, "support split must be 0.5mm");
  QVERIFY2(qAbs(flushedM - 0.0004) < 1e-6, "flushed split must be 0.4mm");
  QVERIFY2(qAbs(towerM - 0.0005) < 1e-6, "tower split must be 0.5mm");

  // Weights use the same area/density conversion as the totals.
  QVERIFY2(preview.filamentUsedGrams() > 0.0, "total grams must be positive");
  for (int i = 0; i < 4; ++i)
  {
    const QVariantMap row = rows.at(i).toMap();
    QVERIFY2(row.value(QStringLiteral("lengthText")).toString().contains(QStringLiteral("m")),
             "each split row must format a length");
    QVERIFY2(row.value(QStringLiteral("weightText")).toString().contains(QStringLiteral("g")),
             "each split row must format a weight");
  }
}

// PREV-05: the stealth total switches to the parsed
// "; estimated printing time (silent mode)" comment when present
// (GCodeProcessor TimeProcessor export); without it the x1.4 heuristic stands
// in and stealthTimeEstimated() flags it as an estimate.
void PreviewParserTests::test_prev05_stealth_time_comment_and_estimate_flag()
{
  const auto withSilent = writeTempGcode(QStringLiteral(
      "; estimated printing time (normal mode) = 1h30m\n"
      "; estimated printing time (silent mode) = 1h10m\n"
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G1 X20 Y10 E1.0 F1800\n"
      ));
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(withSilent.path), "silent gcode should parse");
  // The normal-mode comment replaces the placeholder total.
  QCOMPARE(preview.estimatedTime(), QStringLiteral("1h30m"));

  preview.setStealthMode(true);
  QCOMPARE(preview.totalTime(), QStringLiteral("1h10m"));
  QVERIFY2(!preview.stealthTimeEstimated(),
           "parsed silent-mode comment must clear the estimate flag");
  preview.setStealthMode(false);
  QCOMPARE(preview.totalTime(), QStringLiteral("1h30m"));

  // Without a silent comment the heuristic is used and flagged.
  const auto noSilent = writeTempGcode(QStringLiteral(
      "; estimated printing time (normal mode) = 1h30m\n"
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G1 X20 Y10 E1.0 F1800\n"
      ));
  ProjectServiceMock project2;
  SliceService slice2(&project2);
  PreviewViewModel preview2(&project2, &slice2);
  QVERIFY2(preview2.loadGCodeForPreview(noSilent.path), "no-silent gcode should parse");
  preview2.setStealthMode(true);
  // 5400s * 1.4 = 7560s == "2h06m".
  QCOMPARE(preview2.totalTime(), QStringLiteral("2h06m"));
  QVERIFY2(preview2.stealthTimeEstimated(),
           "the x1.4 heuristic must be flagged as an estimate");
  // Normal mode never flags.
  preview2.setStealthMode(false);
  QVERIFY2(!preview2.stealthTimeEstimated(), "normal mode never flags an estimate");
}

// PREV-05: the cost uses the "; filament_cost" gcode config block per kg
// (upstream filament_cost key; DEFAULT_FILAMENT_COST 29.99 fallback,
// GCodeProcessor.cpp:49 + :4387-4398).
void PreviewParserTests::test_prev05_filament_price_from_config_block()
{
  // Fresh viewmodel without a parse: no price known at all.
  ProjectServiceMock project0;
  SliceService slice0(&project0);
  PreviewViewModel preview0(&project0, &slice0);
  QCOMPARE(preview0.filamentPricePerKg(), 0.0);

  const auto priced = writeTempGcode(QStringLiteral(
      "; filament_cost = 55.50\n"
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G1 X20 Y10 E1.0 F1800\n"
      ));
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(priced.path), "priced gcode should parse");
  QCOMPARE(preview.filamentPricePerKg(), 55.5);
  // Cost = grams * price/kg * 0.001 (single extruder).
  const QString expectedCost =
      QStringLiteral("$%1").arg(preview.filamentUsedGrams() * 55.5 * 0.001, 0, 'f', 2);
  QCOMPARE(preview.estimatedCost(), expectedCost);

  // No filament_cost line -> upstream DEFAULT_FILAMENT_COST fallback.
  const auto unpriced = writeTempGcode(QStringLiteral(
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G1 X20 Y10 E1.0 F1800\n"
      ));
  ProjectServiceMock project2;
  SliceService slice2(&project2);
  PreviewViewModel preview2(&project2, &slice2);
  QVERIFY2(preview2.loadGCodeForPreview(unpriced.path), "unpriced gcode should parse");
  QCOMPARE(preview2.filamentPricePerKg(), 29.99);
  const QString expectedDefaultCost =
      QStringLiteral("$%1").arg(preview2.filamentUsedGrams() * 29.99 * 0.001, 0, 'f', 2);
  QCOMPARE(preview2.estimatedCost(), expectedDefaultCost);
}

// PREV-06: Tool/Filament view modes must color by the CONFIGURED extruder
// colors (upstream m_tool_colors from the plater extruder_colors config,
// GCodeViewer.cpp:1109-1127), not the legacy fixed 8-color cycle.
void PreviewParserTests::test_prev06_configured_extruder_colors_override_fixed_cycle()
{
  const auto tmp = writeTempGcode(QStringLiteral(
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G1 X20 Y10 E1.0 F1800\n"
      "T1\n"
      "G1 X30 Y10 E2.0 F1800\n"
      "G1 X40 Y10 E3.0 F1800\n"
      ));

  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  project.setActiveFilamentColours({QStringLiteral("#FF0000"), QStringLiteral("#00FF00")});
  QVERIFY2(preview.loadGCodeForPreview(tmp.path), "two-extruder gcode should parse");

  QCOMPARE(preview.configuredExtruderColors(),
           QStringList({QStringLiteral("#FF0000"), QStringLiteral("#00FF00")}));
  // colorHex formats with lowercase hex digits.
  QCOMPARE(preview.extruderColor(0), QStringLiteral("#ff0000"));
  QCOMPARE(preview.extruderColor(1), QStringLiteral("#00ff00"));

  // Tool view mode legend rows carry the configured colors + visibility.
  const QStringList modes = preview.viewModes();
  preview.setViewModeIndex(modes.indexOf(QStringLiteral("Tool")));
  const QVariantList legendRows = preview.legendItems();
  QCOMPARE(legendRows.size(), 2);
  QCOMPARE(legendRows.at(0).toMap().value(QStringLiteral("color")).toString(), QStringLiteral("#ff0000"));
  QCOMPARE(legendRows.at(0).toMap().value(QStringLiteral("extruderId")).toInt(), 0);
  QCOMPARE(legendRows.at(0).toMap().value(QStringLiteral("visible")).toBool(), true);
  QCOMPARE(legendRows.at(1).toMap().value(QStringLiteral("color")).toString(), QStringLiteral("#00ff00"));

  // An INVALID configured colour (QColor::isValid() false) must fall through
  // to the legacy fixed cycle (extruder 0 = 0.95/0.55/0.22 -> #f28c38),
  // proving the configured list actually overrides the fallback when valid.
  // (With HAS_LIBSLIC3R the plate config supplies a valid default colour, so
  // the fallback is exercised via the invalid-entry path.)
  ProjectServiceMock project2;
  SliceService slice2(&project2);
  PreviewViewModel preview2(&project2, &slice2);
  project2.setActiveFilamentColours({QStringLiteral("not-a-colour")});
  QVERIFY2(preview2.loadGCodeForPreview(tmp.path), "two-extruder gcode should parse (legacy)");
  QVERIFY2(!QColor(QStringLiteral("not-a-colour")).isValid(),
           "precondition: the fallback entry colour must be invalid");
  QCOMPARE(preview2.extruderColor(0), QStringLiteral("#f28c38"));
}

// PREV-06: per-extruder visibility (upstream m_tool_visibles, applied in the
// ColorPrint/Filament view, GCodeViewer.cpp:3337 + :5088) filters the packed
// payload per extruder.
void PreviewParserTests::test_prev06_extruder_visibility_gates_filament_payload()
{
  const auto tmp = writeTempGcode(QStringLiteral(
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G1 X20 Y10 E1.0 F1800\n"
      "T1\n"
      "G1 X30 Y10 E2.0 F1800\n"
      "G1 X40 Y10 E3.0 F1800\n"
      ));
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(tmp.path), "two-extruder gcode should parse");

  const QStringList modes = preview.viewModes();
  preview.setViewModeIndex(modes.indexOf(QStringLiteral("Filament")));
  const int allCount = gcvSegmentCount(preview.gcodePreviewData());
  QVERIFY2(allCount > 0, "Filament mode payload must contain segments");

  QVERIFY2(preview.isExtruderVisible(0), "extruders default to visible");
  preview.toggleExtruderVisibility(0);
  QVERIFY2(!preview.isExtruderVisible(0), "toggle must flip the visibility");
  // Hiding extruder 0 drops its single extrusion segment (the travel is
  // hidden by the default travel visibility too).
  const int withoutExtruder0 = gcvSegmentCount(preview.gcodePreviewData());
  QCOMPARE(withoutExtruder0, allCount - 1);

  preview.toggleExtruderVisibility(0);
  QCOMPARE(gcvSegmentCount(preview.gcodePreviewData()), allCount);
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
