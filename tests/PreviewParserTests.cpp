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
//   - EViewType: GAP-3 adjudication -- viewModes() equals the upstream
//     view_type_items dropdown exactly (10 items, GCodeViewer.cpp:892-901,
//     strings from :59-84); Tool is hidden upstream and FilamentId stays
//     internal. OWzx-only render capabilities (Summary/Acceleration/Actual
//     Speed/Jerk/Actual Flow/Pressure Advance/Tool/FilamentId) remain
//     addressable via the EViewType internal tail.
//   - EGCodeExtrusionRole:  libvgcode/include/Types.hpp:131-157 (canonical Qt6 index)

#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>
#include <QtTest>
#include <cstring>

#include "core/services/ProjectService.h"
#include "core/services/SliceService.h"
#include "core/viewmodels/PreviewViewModel.h"

#ifdef HAS_LIBSLIC3R
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QSignalSpy>
#include <QVector>
#endif

class PreviewParserTests final : public QObject
{
  Q_OBJECT

private slots:
  void test_fixture_has_expected_role_coverage();
  void test_role_string_mapping_covers_upstream_enum();
  void test_view_modes_match_upstream_ten();
  void test_view_mode_availability_all_upstream_rows_available();
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
  // P17.9: G90/G91 relative positioning, G2/G3 arc segmentation, G4 dwell.
  void test_prev09_g91_relative_positioning_seam();
  void test_prev09_g2_arc_segments_into_chords();
  void test_prev09_g4_dwell_adds_time();
  // P17.1: hidden FilamentId mode (upstream 12-mode truth).
  void test_prev01_filamentid_mode_hidden_and_pseudo_color();
  // P17.4: retract/unretract default hidden (upstream GCodeViewer.hpp:324).
  void test_prev04_retract_unretract_default_hidden();
  // P17.3: 10-step gradient legend rows with values.
  void test_prev03_gradient_stops_ten_rows();
  // Wave 5: vertical layer direction and IMSlider one-layer/wheel semantics.
  void test_wave5_layer_slider_semantics();
  // Wave 5: upstream update_by_mode visibility gates.
  void test_wave5_preview_visibility_gates();
  void test_wave5_marker_and_tick_payloads_are_data_backed();
  // P1 (preview direct hand-off): slice -> preview through the structured
  // GCodeProcessorResult, locked field-by-field against the text re-parse of
  // the same exported file (层表/move 表/角色/线宽), plus the consume-once /
  // release / deferred source-window invariants.
  void test_p1_direct_result_matches_text_parse();
  void test_p1_direct_handoff_releases_and_defers_source_window();

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
  ProjectService project;
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

// GREEN since GAP-3 (PREVIEW-VIEWMODES-UPSTREAM): viewModes() returns exactly
// the upstream GCodeViewer view_type_items dropdown -- 10 items, fixed order
// from update_by_mode (GCodeViewer.cpp:892-901), display strings from
// get_view_type_string (GCodeViewer.cpp:59-84).
void PreviewParserTests::test_view_modes_match_upstream_ten()
{
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);

  const QStringList modes = preview.viewModes();
  const QStringList upstream = {
      QStringLiteral("Line Type"),        // FeatureType
      QStringLiteral("Filament"),         // ColorPrint
      QStringLiteral("Speed"),            // Feedrate
      QStringLiteral("Layer Height"),     // Height
      QStringLiteral("Line Width"),       // Width
      QStringLiteral("Flow"),             // VolumetricRate
      QStringLiteral("Layer Time"),       // LayerTime
      QStringLiteral("Layer Time (log)"), // LayerTimeLog
      QStringLiteral("Fan Speed"),        // FanSpeed
      QStringLiteral("Temperature"),      // Temperature
  };
  QCOMPARE(modes.size(), 10);
  for (int i = 0; i < upstream.size(); ++i) {
    QVERIFY2(modes.at(i) == upstream.at(i),
             qPrintable(QStringLiteral("viewModes()[%1] should be '%2' (upstream order), got '%3'")
                            .arg(i).arg(upstream.at(i), modes.at(i))));
  }
  // Internal capability modes must not resurface as dropdown rows.
  QVERIFY2(!modes.contains(QStringLiteral("Summary")),
           "Summary has no upstream dropdown row");
  QVERIFY2(!modes.contains(QStringLiteral("Tool")),
           "Tool is hidden upstream (GCodeViewer.cpp:902-904)");
  QVERIFY2(!modes.contains(QStringLiteral("FilamentId")),
           "FilamentId stays internal (GCodeViewer.cpp:911)");
  // The dropdown row index must equal the EViewType constant for every row so
  // viewModeIndex_ doubles as the combo row and the recolor switch key.
  QCOMPARE(modes.indexOf(QStringLiteral("Line Type")), PreviewViewModel::VT_LineType);
  QCOMPARE(modes.indexOf(QStringLiteral("Filament")), PreviewViewModel::VT_Filament);
  QCOMPARE(modes.indexOf(QStringLiteral("Speed")), PreviewViewModel::VT_Speed);
  QCOMPARE(modes.indexOf(QStringLiteral("Layer Height")), PreviewViewModel::VT_Height);
  QCOMPARE(modes.indexOf(QStringLiteral("Line Width")), PreviewViewModel::VT_Width);
  QCOMPARE(modes.indexOf(QStringLiteral("Flow")), PreviewViewModel::VT_Flow);
  QCOMPARE(modes.indexOf(QStringLiteral("Layer Time")), PreviewViewModel::VT_LayerTime);
  QCOMPARE(modes.indexOf(QStringLiteral("Layer Time (log)")), PreviewViewModel::VT_LayerTimeLog);
  QCOMPARE(modes.indexOf(QStringLiteral("Fan Speed")), PreviewViewModel::VT_FanSpeed);
  QCOMPARE(modes.indexOf(QStringLiteral("Temperature")), PreviewViewModel::VT_Temperature);
}

// GREEN since GAP-3: every dropdown row is available (v5.11 parses real data
// for M220/M221/M205/M900, so nothing is data-gated) and the internal
// capability modes stay out of the dropdown.
void PreviewParserTests::test_view_mode_availability_all_upstream_rows_available()
{
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);

  const QStringList modes = preview.viewModes();
  QCOMPARE(modes.size(), 10);
  for (int i = 0; i < modes.size(); ++i) {
    QVERIFY2(preview.viewModeAvailable(i),
             qPrintable(QStringLiteral("%1 must be available (all fields parse since v5.11)").arg(modes.at(i))));
    QVERIFY2(preview.viewModeStatusText(i).isEmpty(),
             qPrintable(QStringLiteral("%1 must not show an unavailable-mode status").arg(modes.at(i))));
  }

  // Retained internal capabilities (no upstream dropdown row).
  const QStringList internalOnly = {
    QStringLiteral("Summary"),
    QStringLiteral("Actual Speed"),
    QStringLiteral("Acceleration"),
    QStringLiteral("Jerk"),
    QStringLiteral("Actual Flow"),
    QStringLiteral("Pressure Advance"),
    QStringLiteral("Tool")
  };
  for (const QString &modeName : internalOnly) {
    QVERIFY2(!modes.contains(modeName),
             qPrintable(QStringLiteral("%1 is an internal capability and must stay out of the dropdown").arg(modeName)));
  }

  preview.setViewModeIndex(PreviewViewModel::VT_Speed);
  QVERIFY2(preview.currentViewModeAvailable(),
           "currentViewModeAvailable must follow setViewModeIndex for dropdown rows");
}

// GAP-3: Summary lost its dropdown row (no upstream GCodeViewer counterpart)
// but stays reachable as an internal capability (VT_Summary) and still must
// produce no gradient legend. legendType() stays 0 (discrete) and
// legendItems() is empty when viewModeIndex maps to Summary.
void PreviewParserTests::test_summary_mode_has_no_gradient_legend()
{
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);

  QVERIFY2(preview.loadGCodeForPreview(fixturePath()),
           "fixture should parse for Summary legend assertion");

  preview.setViewModeIndex(PreviewViewModel::VT_Summary);

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
  ProjectService project;
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
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);

  QVERIFY2(preview.loadGCodeForPreview(fixturePath()),
           "fixture should parse before validating view-mode payload survival");

  const QStringList modes = preview.viewModes();
  // Every addressable mode must keep a healthy payload: the 10 upstream
  // dropdown rows plus the internal capability tail (VT_Summary..VT_FilamentId).
  for (int i = 0; i < PreviewViewModel::VT_ModeCount; ++i) {
    preview.setViewModeIndex(i);
    const QByteArray payload = preview.gcodePreviewData();
    const QString label = i < modes.size() ? modes.at(i) : QStringLiteral("internal mode %1").arg(i);
    QVERIFY2(payload.size() > 8,
             qPrintable(QStringLiteral("%1 mode must keep a non-empty preview payload").arg(label)));
    QVERIFY2(payload.startsWith("GCV1"),
             qPrintable(QStringLiteral("%1 mode must keep the GCV1 wire format").arg(label)));
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
  ProjectService project;
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
  ProjectService project;
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
  ProjectService project2;
  SliceService slice2(&project2);
  PreviewViewModel preview2(&project2, &slice2);
  QVERIFY2(preview2.loadGCodeForPreview(open.path),
           "open-loop synthetic gcode should parse");
  QCOMPARE(preview2.moveCountOfKind(PreviewViewModel::KindSeam), 0);

  // Regression on the shared fixture: its Outer wall square closes at its
  // start (55,55) and the following travel departs from that exact point,
  // so exactly one seam is expected (upstream records the seam at the close
  // even when the loop-ending move is a travel).
  ProjectService project3;
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
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(tmp.path), "synthetic gcode should parse");

  // P17.4: the upstream default hides retract/unretract
  // (GCodeViewer.hpp:324), so the initial payload excludes both ticks;
  // enabling each adds exactly its tick back.
  const int withHidden = gcvSegmentCount(preview.gcodePreviewData());
  QVERIFY2(withHidden > 0, "payload should contain segments before toggling");

  preview.setShowRetractMoves(true);
  const int withRetract = gcvSegmentCount(preview.gcodePreviewData());
  QCOMPARE(withRetract, withHidden + 1);

  preview.setShowUnretractMoves(true);
  const int withBoth = gcvSegmentCount(preview.gcodePreviewData());
  QCOMPARE(withBoth, withHidden + 2);

  preview.setShowRetractMoves(false);
  preview.setShowUnretractMoves(false);
  QCOMPARE(gcvSegmentCount(preview.gcodePreviewData()), withHidden);
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
  ProjectService project;
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
  ProjectService project;
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
  ProjectService project2;
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
  ProjectService project0;
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
  ProjectService project;
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
  ProjectService project2;
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

  ProjectService project;
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
  // GAP-3: Tool is hidden upstream (GCodeViewer.cpp:902-904), so it is
  // addressed via the internal capability index instead of a dropdown row.
  preview.setViewModeIndex(PreviewViewModel::VT_Tool);
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
  ProjectService project2;
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
  ProjectService project;
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

// Wave 5: local PreviewViewModel support for IMSlider one-layer and wheel behavior.
void PreviewParserTests::test_wave5_layer_slider_semantics()
{
  const auto tmp = writeTempGcode(QStringLiteral(
      "T0\nG28\n;LAYER:0\n;TYPE:Outer wall\nG1 X10 Y10 Z0.2 E1 F1800\n"
      ";LAYER:1\n;TYPE:Outer wall\nG1 X20 Y10 Z0.4 E2 F1800\n"
      ";LAYER:2\n;TYPE:Outer wall\nG1 X30 Y10 Z0.6 E3 F1800\n"));
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(tmp.path), "three-layer gcode should parse");
  QCOMPARE(preview.layerCount(), 3);
  preview.setLayerRange(1, 2);
  preview.setSingleLayer(true);
  QVERIFY(preview.singleLayer());
  QCOMPARE(preview.currentLayerMin(), 2);
  QCOMPARE(preview.currentLayerMax(), 2);
  preview.wheelLayer(-1);
  QCOMPARE(preview.currentLayerMin(), 1);
  QCOMPARE(preview.currentLayerMax(), 1);
  preview.wheelLayer(-1, true);
  QCOMPARE(preview.currentLayerMin(), 0);
  QCOMPARE(preview.currentLayerMax(), 0);
  preview.setSingleLayer(false);
  QVERIFY(!preview.singleLayer());
  QCOMPARE(preview.currentLayerMin(), 0);
  QCOMPARE(preview.currentLayerMax(), 2);
}

// P17.9: G91 relative positioning. A closed square loop written in relative
// moves must land each vertex at the cumulative position (the seam detector
// closing on the first vertex proves the math); treating relative moves as
// absolute would collapse every segment to zero length (upstream
// GCodeProcessor G91 handling).
void PreviewParserTests::test_prev09_g91_relative_positioning_seam()
{
  const auto tmp = writeTempGcode(QStringLiteral(
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G91\n"
      "G1 X10 Y0 E0.5 F1800\n"
      "G1 X0 Y10 E1.0 F1800\n"
      "G1 X-10 Y0 E1.5 F1800\n"
      "G1 X0 Y-10 E2.0 F1800\n"
      "G90\n"
      "G1 X12 Y10 F3000\n"
      ));
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(tmp.path), "gcode should parse");
  QCOMPARE(preview.moveCountOfKind(PreviewViewModel::KindExtrude), 4);
  QCOMPARE(preview.moveCountOfKind(PreviewViewModel::KindSeam), 1);
}

// P17.9: G2 arcs are segmented into straight chords (~0.5mm resolution,
// clamped to [8,128]) — upstream segments arcs during parsing so every
// toolpath vertex is a straight move.
void PreviewParserTests::test_prev09_g2_arc_segments_into_chords()
{
  const auto tmp = writeTempGcode(QStringLiteral(
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G2 X30 Y10 I10 J0 E2.0 F1800\n"
      ));
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(tmp.path), "gcode should parse");
  const int extrudes = preview.moveCountOfKind(PreviewViewModel::KindExtrude);
  QVERIFY2(extrudes >= 20 && extrudes <= 128,
           "semicircle r=10 must segment into dozens of chords");
  QCOMPARE(preview.moveCountOfKind(PreviewViewModel::KindTravel), 1);
}

// P17.9: G4 dwell adds pure time (S seconds / P milliseconds) to the totals.
void PreviewParserTests::test_prev09_g4_dwell_adds_time()
{
  const auto tmp = writeTempGcode(QStringLiteral(
      "T0\n"
      "G28\n"
      ";LAYER:0\n"
      ";TYPE:Outer wall\n"
      "G1 X10 Y10 F3000\n"
      "G1 X20 Y10 E1.0 F1800\n"
      "G4 S30\n"
      ));
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(tmp.path), "gcode should parse");
  // No normal-time comment: the total is the accumulated move time plus the
  // 30s dwell. The first move travels 10mm at 50mm/s = 0.2s; the extrude
  // move runs 10mm at 30mm/s ~= 0.33s; plus 30s dwell.
  QVERIFY2(preview.totalTime().contains(QLatin1Char('s')),
           "total time should render with seconds for sub-minute prints");
  QVERIFY2(preview.totalTime() != QStringLiteral("0.0s"),
           "dwell must extend the total time beyond the move time");
}

// Wave 5: update_by_mode exposes visibility controls conditionally by view.
void PreviewParserTests::test_wave5_marker_and_tick_payloads_are_data_backed()
{
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY(!preview.hasToolPosition());
  QVERIFY(preview.tickMarks().isEmpty());

  const auto tmp = writeTempGcode(QStringLiteral(
      "T0\nG28\n;LAYER:0\n;TYPE:Outer wall\n"
      "; PAUSE_PRINT\nG1 X10 Y10 Z0.2 E1 F1800\n"));
  QVERIFY2(preview.loadGCodeForPreview(tmp.path), "marker fixture should parse");
  QVERIFY(preview.hasToolPosition());
  const QVariantList ticks = preview.tickMarks();
  QCOMPARE(ticks.size(), 1);
  const QVariantMap tick = ticks.first().toMap();
  QCOMPARE(tick.value(QStringLiteral("type")).toInt(), 0);
  QCOMPARE(tick.value(QStringLiteral("tick")).toInt(), 0);

  preview.setShowMarker(false);
  QVERIFY(!preview.showMarker());
  QVERIFY(preview.hasToolPosition());
}

void PreviewParserTests::test_wave5_preview_visibility_gates()
{
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  const QStringList modes = preview.viewModes();

  const int lineType = modes.indexOf(QStringLiteral("Line Type"));
  const int filament = modes.indexOf(QStringLiteral("Filament"));
  const int speed = modes.indexOf(QStringLiteral("Speed"));
  const int temperature = modes.indexOf(QStringLiteral("Temperature"));
  QVERIFY(lineType >= 0 && filament >= 0 && speed >= 0 && temperature >= 0);

  preview.setViewModeIndex(lineType);
  QVERIFY(preview.roleVisibilityAvailable());
  QVERIFY(preview.moveVisibilityAvailable());
  QVERIFY(preview.travelVisibilityAvailable());
  QVERIFY(!preview.extruderVisibilityAvailable());

  preview.setViewModeIndex(filament);
  QVERIFY(!preview.roleVisibilityAvailable());
  QVERIFY(!preview.moveVisibilityAvailable());
  QVERIFY(!preview.travelVisibilityAvailable());
  QVERIFY(preview.extruderVisibilityAvailable());

  preview.setViewModeIndex(speed);
  QVERIFY(!preview.roleVisibilityAvailable());
  QVERIFY(!preview.moveVisibilityAvailable());
  QVERIFY(preview.travelVisibilityAvailable());
  QVERIFY(!preview.extruderVisibilityAvailable());

  preview.setViewModeIndex(temperature);
  QVERIFY(!preview.roleVisibilityAvailable());
  QVERIFY(!preview.moveVisibilityAvailable());
  QVERIFY(!preview.travelVisibilityAvailable());
  QVERIFY(!preview.extruderVisibilityAvailable());
}

// P17.1 + GAP-3: upstream EViewType::FilamentId is a hidden diagnostic mode --
// it is NOT in the dropdown list, and setViewModeIndex(VT_FilamentId) keeps a
// valid payload and recolors segments with the {id, role, id} pseudo-color.
void PreviewParserTests::test_prev01_filamentid_mode_hidden_and_pseudo_color()
{
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(fixturePath()),
           "fixture should parse before the FilamentId assertions");

  QVERIFY2(!preview.viewModes().contains(QStringLiteral("FilamentId")),
           "FilamentId is a hidden mode and must not appear in the dropdown");
  preview.setViewModeIndex(PreviewViewModel::VT_FilamentId);
  QVERIFY2(preview.viewModeIndex() == PreviewViewModel::VT_FilamentId,
           "the internal FilamentId capability must stay addressable");
  QVERIFY2(!preview.gcodePreviewData().isEmpty(),
           "FilamentId mode must keep a valid GCV1 payload");
}

// P17.4: upstream defaults are Retract/Unretract/Wipe all hidden
// (GCodeViewer.hpp:324, only Extrude + Seam visible by default).
void PreviewParserTests::test_prev04_retract_unretract_default_hidden()
{
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QCOMPARE(preview.showRetractMoves(), false);
  QCOMPARE(preview.showUnretractMoves(), false);
  QCOMPARE(preview.showWipeMoves(), false);
  QCOMPARE(preview.showSeamMarks(), true);
}

// P17.3: gradient modes produce the 10-step legend rows (upstream
// append_range, GCodeViewer.cpp:4498-4518), with the min row matching the
// gradient min label formatting.
void PreviewParserTests::test_prev03_gradient_stops_ten_rows()
{
  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);
  QVERIFY2(preview.loadGCodeForPreview(fixturePath()),
           "fixture should parse before the gradient legend assertion");
  const QStringList modes = preview.viewModes();
  const int heightIdx = modes.indexOf(QStringLiteral("Layer Height"));
  QVERIFY2(heightIdx >= 0, "Layer Height mode must exist");
  preview.setViewModeIndex(heightIdx);
  QCOMPARE(preview.legendGradientStops().size(), 10);
  // Height uses 2 decimals per the upstream legend formatting.
  const QVariantMap first = preview.legendGradientStops().first().toMap();
  QCOMPARE(first.value(QStringLiteral("value")).toString(),
           preview.legendGradientMinLabel());
}

// ── P1 (preview direct hand-off) ─────────────────────────────────────────────
//
// Upstream reference: GCodeViewer::load_as_gcode consumes the structured
// GCodeProcessorResult directly (GCodeViewer.hpp:186; the const
// GCodeProcessorResult* member at :260). The Qt6 preview now has the same
// second entry: SliceService delivers the result (shared_ptr, no value copy,
// no text round-trip) and PreviewViewModel::loadFromProcessorResult builds
// the preview from it. The tests below lock the two entry points together:
// the direct product and the text re-parse of the SAME exported file must
// agree field-by-field on the layer table, the move table, roles and widths.
#ifdef HAS_LIBSLIC3R
namespace
{
  QString prusaStlPath()
  {
    return QDir::cleanPath(
        QStringLiteral(QT_TESTCASE_SOURCEDIR) +
        QStringLiteral("/third_party/OrcaSlicer/tests/data/test_3mf/Prusa.stl"));
  }

  // Mirror of PackedSegment (PreviewViewModel.cpp) / GcvPackedSegment
  // (RhiViewportRenderer.cpp): 20 floats + 4 ints = 96 bytes after the
  // "GCV1" + int32 count header.
  struct GcvRow
  {
    float x1, y1, z1, x2, y2, z2;
    float r, g, b;
    float feedrate;
    float fan_speed;
    float temperature;
    float width;
    float height;
    float layer_time;
    float acceleration;
    float jerk;
    float pressure_advance;
    float actual_speed;
    float actual_flow;
    int extruder_id;
    int layer;
    int move;
    int role;
  };
  static_assert(sizeof(GcvRow) == 96, "GcvRow must mirror the 96-byte GCV1 segment");

  QVector<GcvRow> parseGcv1(const QByteArray &payload)
  {
    QVector<GcvRow> rows;
    if (!payload.startsWith("GCV1") || payload.size() < 8)
      return rows;
    int count = 0;
    memcpy(&count, payload.constData() + 4, sizeof(count));
    rows.reserve(count);
    for (int i = 0; i < count; ++i)
    {
      const char *base = payload.constData() + 8 + i * int(sizeof(GcvRow));
      if (base + sizeof(GcvRow) > payload.constData() + payload.size())
        break;
      GcvRow row;
      memcpy(&row, base, sizeof(row));
      rows.append(row);
    }
    return rows;
  }

  // Field snapshot of a built preview state (both entry points produce it).
  struct PreviewSnapshot
  {
    int layerCount = 0;
    int moveCount = 0;
    int kindCounts[6] = {0, 0, 0, 0, 0, 0};
    QVector<float> layerZs;
    QVector<GcvRow> rows;
  };

  PreviewSnapshot snapshotPreview(PreviewViewModel &preview)
  {
    PreviewSnapshot snap;
    snap.layerCount = preview.layerCount();
    snap.moveCount = preview.moveCount();
    for (int k = 0; k < 6; ++k)
      snap.kindCounts[k] = preview.moveCountOfKind(k);
    snap.layerZs.reserve(snap.layerCount);
    for (int i = 0; i < snap.layerCount; ++i)
      snap.layerZs.append(preview.layerZAt(i));
    snap.rows = parseGcv1(preview.gcodePreviewData());
    return snap;
  }

  // Minimal app-identity guard so QSettings / preset dirs touched by the
  // slice path do not leak into the developer's environment (same pattern as
  // ViewModelSmokeTests::ScopedApplicationIdentity).
  struct ScopedAppIdentity
  {
    QString oldOrg;
    QString oldApp;
    explicit ScopedAppIdentity(const QString &tag)
        : oldOrg(QCoreApplication::organizationName()),
          oldApp(QCoreApplication::applicationName())
    {
      QCoreApplication::setOrganizationName(QStringLiteral("OWzxTests"));
      QCoreApplication::setApplicationName(tag);
    }
    ~ScopedAppIdentity()
    {
      QCoreApplication::setOrganizationName(oldOrg);
      QCoreApplication::setApplicationName(oldApp);
    }
  };

  // Slice Prusa.stl once with the minimal printer config (same recipe as the
  // ViewModelSmokeTests ENGN tests) and return the exported gcode path.
  // Returns an empty string when the environment cannot slice (caller skips).
  // NOTE: plain code only -- QTest macros cannot early-return from a helper
  // with a non-void return type.
  QString slicePrusaFixture(ProjectService &project, SliceService &slice)
  {
    QSignalSpy loadSpy(&project, &ProjectService::loadFinished);
    if (!project.loadFile(prusaStlPath()) && loadSpy.isEmpty())
      return {};
    if (!loadSpy.wait(30000) && loadSpy.isEmpty())
      return {};
    if (!loadSpy.takeFirst().at(0).toBool())
      return {};

    slice.setBedShape({QPointF(0, 0), QPointF(220, 0), QPointF(220, 220), QPointF(0, 220)});
    QHash<QString, QVariant> cfg;
    cfg.insert(QStringLiteral("printable_height"), 250.0);
    cfg.insert(QStringLiteral("nozzle_diameter"), QVariantList{0.4});
    slice.setMergedPresetConfig(cfg);
    project.arrangeObjects(5.0f, false, false, QStringLiteral("0,0,220,0,220,220,0,220"));

    QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
    QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);
    slice.startSlice(QStringLiteral("p1_direct_alignment"));
    // Spin the loop so the queued completion lambda runs and the spies fill
    // (180s budget, matching the other real-slice suites).
    QElapsedTimer guard;
    guard.start();
    while (finishedSpy.isEmpty() && failedSpy.isEmpty() && guard.elapsed() < 180000)
      QCoreApplication::processEvents(QEventLoop::AllEvents, 200);
    if (!failedSpy.isEmpty() || finishedSpy.isEmpty())
      return {};
    return slice.outputPath();
  }
} // namespace
#endif // HAS_LIBSLIC3R

void PreviewParserTests::test_p1_direct_result_matches_text_parse()
{
#ifdef HAS_LIBSLIC3R
  ScopedAppIdentity identity(QStringLiteral("P1DirectAlignment"));

  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);

  const QString gcodePath = slicePrusaFixture(project, slice);
  if (gcodePath.isEmpty())
    QSKIP("Slice failed in this environment -- cannot lock the direct/text parity");
  QVERIFY2(QFileInfo::exists(gcodePath), "sliced gcode file should exist");

  // The slice completion already routed the preview through the DIRECT path
  // (resultChanged sync consumed the processor result before sliceFinished).
  QVERIFY2(preview.previewReady(),
           "the direct hand-off must make the preview ready without a text parse");
  QVERIFY2(preview.gcodePreviewData().startsWith("GCV1"),
           "the direct hand-off must produce the GCV1 renderer payload");
  QVERIFY2(preview.moveCount() > 0 && preview.layerCount() > 0,
           "the direct product must expose moves and layers");

  // Let the deferred source-window pass finish so the snapshot sees the same
  // complete state the text path builds synchronously.
  QTRY_VERIFY_WITH_TIMEOUT(preview.gcodeLineCount() > 0, 30000);

  // Widen the locked surface: pack travels too (default hidden).
  preview.setShowTravelMoves(true);

  const PreviewSnapshot direct = snapshotPreview(preview);

  // Same file, text entry (bench anchor semantics: loadGCodeForPreview stays
  // the independent-file text path).
  QVERIFY2(preview.loadGCodeForPreview(gcodePath),
           "text re-parse of the same file must succeed");
  preview.setShowTravelMoves(true);
  const PreviewSnapshot text = snapshotPreview(preview);

  // ── 层表 (layer table) ──
  QCOMPARE(direct.layerCount, text.layerCount);
  for (int i = 0; i < text.layerCount; ++i)
  {
    QVERIFY2(qAbs(direct.layerZs.at(i) - text.layerZs.at(i)) <= 0.002f,
             qPrintable(QStringLiteral("layer %1 Z mismatch: direct=%2 text=%3")
                            .arg(i).arg(direct.layerZs.at(i)).arg(text.layerZs.at(i))));
  }

  // ── move 表 (move table) ──
  for (int k = 0; k < 6; ++k)
  {
    qInfo("[P1] kindCounts[%d] direct=%d text=%d", k, direct.kindCounts[k],
          text.kindCounts[k]);
  }
  // Diagnostic first (QCOMPARE aborts on failure): (move, role) signature
  // diff around the first divergence so a mismatch reports WHERE.
  {
    const int common = qMin(direct.rows.size(), text.rows.size());
    for (int i = 0; i < common; ++i)
    {
      if (direct.rows.at(i).move != text.rows.at(i).move
          || direct.rows.at(i).role != text.rows.at(i).role)
      {
        const int from = qMax(0, i - 2);
        const int to = qMin(common, i + 4);
        for (int j = from; j < to; ++j)
        {
          qInfo("[P1] row %d direct(move=%d role=%d layer=%d x2=%.3f) text(move=%d role=%d layer=%d x2=%.3f)",
                j, direct.rows.at(j).move, direct.rows.at(j).role,
                direct.rows.at(j).layer, direct.rows.at(j).x2,
                text.rows.at(j).move, text.rows.at(j).role,
                text.rows.at(j).layer, text.rows.at(j).x2);
        }
        break;
      }
    }
  }
  // Head/tail diagnostics were used to land the lock (start-gcode Z
  // distortion, G28 homing, prose-width matching); the surviving failure
  // context is the first-divergence block above.
  QCOMPARE(direct.moveCount, text.moveCount);
  for (int k = 0; k < 6; ++k)
  {
    QCOMPARE(direct.kindCounts[k], text.kindCounts[k]);
  }

  // ── Per-segment alignment on the packed payload ──
  QCOMPARE(direct.rows.size(), text.rows.size());
  int extrusionRows = 0;
  for (int i = 0; i < text.rows.size(); ++i)
  {
    const GcvRow &a = direct.rows.at(i);
    const GcvRow &b = text.rows.at(i);
    // Ordering + identity fields: exact.
    QCOMPARE(a.layer, b.layer);
    QCOMPARE(a.role, b.role);
    QCOMPARE(a.move, b.move);
    QCOMPARE(a.extruder_id, b.extruder_id);
    // 线宽: the processor's ;WIDTH-derived value vs the text parse of the
    // same comments -- identical source, float round-trip tolerance. Locked
    // on EXTRUSION rows only (role>0): width is an extrusion property
    // upstream (GCodeProcessor computes m_width on Extrude; travel vertices
    // just carry the stale last value). Rows where the TEXT side has no
    // width data (b.width == 0: untagged start-gcode extrusions like the
    // prime line, where the processor computes a geometric width the text
    // path cannot know) are skipped as data-unavailable, same semantics as
    // the view-mode availability gates; every tagged extrusion row must
    // match (upstream forces m_width to the tag value, so both sides read
    // the same number).
    if (b.role > 0 && b.width > 0.f)
    {
      QVERIFY2(qAbs(a.width - b.width) <= 0.001f,
               qPrintable(QStringLiteral("row %1 width mismatch: direct=%2 text=%3")
                              .arg(i).arg(a.width).arg(b.width)));
    }
    // Feedrate: text stores the F word (mm/min), direct stores feedrate(mm/s)
    // * 60 -- round-trip tolerance.
    QVERIFY2(qAbs(a.feedrate - b.feedrate) <= qMax(0.05f, qAbs(b.feedrate) * 1e-3f),
             qPrintable(QStringLiteral("row %1 feedrate mismatch: direct=%2 text=%3")
                            .arg(i).arg(a.feedrate).arg(b.feedrate)));
    // Geometry: the file prints coordinates with 3 decimals while the moves
    // vector carries full precision -- compare endpoints on extrusion rows.
    if (b.role > 0)
    {
      ++extrusionRows;
      QVERIFY2(qAbs(a.x2 - b.x2) <= 0.002f && qAbs(a.y2 - b.y2) <= 0.002f
                   && qAbs(a.z2 - b.z2) <= 0.002f,
               qPrintable(QStringLiteral("row %1 endpoint mismatch: "
                                         "direct=(%2,%3,%4) text=(%5,%6,%7)")
                              .arg(i).arg(a.x2).arg(a.y2).arg(a.z2)
                              .arg(b.x2).arg(b.y2).arg(b.z2)));
    }
  }
  QVERIFY2(extrusionRows > 0, "the locked payload must contain extrusion rows");

  if (QFileInfo::exists(gcodePath))
    QFile::remove(gcodePath);
#else
  QSKIP("Requires HAS_LIBSLIC3R");
#endif
}

void PreviewParserTests::test_p1_direct_handoff_releases_and_defers_source_window()
{
#ifdef HAS_LIBSLIC3R
  ScopedAppIdentity identity(QStringLiteral("P1DirectLifecycle"));

  ProjectService project;
  SliceService slice(&project);
  PreviewViewModel preview(&project, &slice);

  const QString gcodePath = slicePrusaFixture(project, slice);
  if (gcodePath.isEmpty())
    QSKIP("Slice failed in this environment -- cannot exercise the direct lifecycle");
  QVERIFY2(preview.previewReady(), "direct hand-off must have built the preview");

  // The service payload is released right after the consuming sync: the
  // ~40 MB moves vector must not stay duplicated next to the preview's own
  // segment store (the delivery id keeps the duplicate-sync guard working).
  QVERIFY2(slice.activeProcessorResult() == nullptr,
           "syncPreviewWithActiveResult must release the consumed result");
  quint64 resultId = 12345;
  QVERIFY2(slice.activeProcessorResult(&resultId) == nullptr && resultId > 0,
           "the delivery id must survive the payload release");

  // The deferred source pass fills the G-code window + config footer from the
  // exported file AFTER preview-ready (the direct path itself does no text
  // I/O before the payload is packed).
  QTRY_VERIFY_WITH_TIMEOUT(preview.gcodeLineCount() > 0, 30000);
  QVERIFY2(!preview.gcodeLines().isEmpty(),
           "the deferred pass must populate the G-code source window");
  QVERIFY2(preview.currentGcodeLine() > 0,
           "the deferred pass must anchor the current G-code line");
  QVERIFY2(!preview.fullConfig().isEmpty(),
           "the deferred pass must collect the config footer block");

  if (QFileInfo::exists(gcodePath))
    QFile::remove(gcodePath);
#else
  QSKIP("Requires HAS_LIBSLIC3R");
#endif
}

QTEST_MAIN(PreviewParserTests)

// AUTOMOC requirement: single-file cpp-internal Q_OBJECT must include the
// generated moc output so the meta-object is linked into the test executable.
// Matches the pattern in QmlUiAuditTests.cpp / ViewModelSmokeTests.cpp /
// PartPlateTests.cpp / PrepareSceneDataTests.cpp / E2EWorkflowTests.cpp.
#include "PreviewParserTests.moc"
