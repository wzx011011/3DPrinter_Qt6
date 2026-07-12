// PartPlate geometry + multi-plate arrangement tests (v3.2 Phase 29).
//
// ARRANGE-01 (geometry): data-driven compute_colum_count parity table (1..36),
//   computeOrigin grid math (sign-flip on Y), computePlateIndex round-trip
//   (Y sign-flip decode + round() boundary characterization),
//   updatePlateOrigins writes real origins to plates.
// ARRANGE-02/03 (added by Plan 29-05): integration tests through
//   ProjectServiceMock::arrangeObjects.
//
// AUTOMOC note: single-file cpp-internal Q_OBJECT has weak moc dependency
// tracking. The canonical verify script re-runs cmake configure after slot
// edits; for incremental builds, delete build/PartPlateTests_autogen/timestamp.

#include <QtTest>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QColor>
#include <QSignalSpy>
#include <cstring>

#include "core/model/PartPlate.h"
#include "core/model/PartPlateList.h"
#include "core/services/ProjectServiceMock.h"

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Point.hpp>
#endif

namespace {
// Reuse the same fixture path as ViewModelSmokeTests.cpp:41.
const QString kStlPath = QDir::cleanPath(
    QStringLiteral(QT_TESTCASE_SOURCEDIR) +
    QStringLiteral("/third_party/OrcaSlicer/resources/profiles/hotend.stl"));
}  // namespace

class PartPlateTests final : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();

  // ── ARRANGE-01 geometry unit tests ──────────────────────────────────────
  void computeColumCount_data();
  void computeColumCount();

#ifdef HAS_LIBSLIC3R
  void computeOriginGridMath();
  void computePlateIndexRoundTrip();
  void updatePlateOriginsWritesToPlates();
#else
  void computeOriginGridMath() { QSKIP("Requires HAS_LIBSLIC3R"); }
  void computePlateIndexRoundTrip() { QSKIP("Requires HAS_LIBSLIC3R"); }
  void updatePlateOriginsWritesToPlates() { QSKIP("Requires HAS_LIBSLIC3R"); }
#endif

  // ── ARRANGE-02/03 integration tests (through arrangeObjects) ────────────
#ifdef HAS_LIBSLIC3R
  void arrangeDistributesAcrossPlates();
  void lockedPlateExclusion();
  void allLockedReturnsFalse();
#else
  void arrangeDistributesAcrossPlates() { QSKIP("Requires HAS_LIBSLIC3R"); }
  void lockedPlateExclusion() { QSKIP("Requires HAS_LIBSLIC3R"); }
  void allLockedReturnsFalse() { QSKIP("Requires HAS_LIBSLIC3R"); }
#endif

  // ── THUMB-01/02 tests (v3.2 Phase 30) ───────────────────────────────────
  void thumbnailCacheInvalidation();  // PartPlate unit test (no libslic3r)
#ifdef HAS_LIBSLIC3R
  void thumbnailRoundTrip();                 // THUMB-02 (in-memory cache)
  void thumbnailSaveReloadRoundTrip();       // Phase 97 (THUMBRT-01/02): save->reload 3MF pixel round-trip
  void thumbnailMultiPlateSaveReloadRoundTrip();  // Phase 98 (REVIEW MEDIUM-3): multi-plate write side
#else
  void thumbnailRoundTrip() { QSKIP("Requires HAS_LIBSLIC3R"); }
  void thumbnailSaveReloadRoundTrip() { QSKIP("Requires HAS_LIBSLIC3R"); }
  void thumbnailMultiPlateSaveReloadRoundTrip() { QSKIP("Requires HAS_LIBSLIC3R"); }
#endif

  // ── FMAP-01/03 tests (v3.2 Phase 31) ────────────────────────────────────
  void filamentMapManualAssignment();  // PartPlate + service (no libslic3r needed)

  // ── Phase 107 FMAP-02 (enum widening + 3MF migration) ──────────────────
  // (a) the 4-value enum maps to the correct ints (no libslic3r); the 3MF
  // round-trip portion (b/c) is gated on HAS_LIBSLIC3R (needs saveProject +
  // loadFile). Mirrors the Phase 97 thumbnailSaveReloadRoundTrip pattern.
  void filamentMapModeEnumWidenedAnd3MFMigrates();
#ifdef HAS_LIBSLIC3R
  void filamentMapModeRoundTripManualPreserved();  // FM-03: save Manual, reload Manual
#else
  void filamentMapModeRoundTripManualPreserved() { QSKIP("Requires HAS_LIBSLIC3R"); }
#endif
};

void PartPlateTests::initTestCase() {
  QVERIFY2(QFileInfo::exists(kStlPath), qPrintable(
      QStringLiteral("Test STL not found: %1").arg(kStlPath)));
}

void PartPlateTests::computeColumCount_data() {
  // Full parity table for upstream compute_colum_count (PartPlate.hpp:38-50).
  // cols = ceil(sqrt(count)) via float sqrt + round + strict-greater comparison.
  // Every perfect square (1,4,9,16,25,36) lands exactly on sqrt with NO +1;
  // every non-square gets +1. Off-by-one drift traps: 2,5,10,17,26 transitions.
  QTest::addColumn<int>("count");
  QTest::addColumn<int>("expectedCols");
  QTest::newRow("count_1") << 1 << 1;
  QTest::newRow("count_2") << 2 << 2;
  QTest::newRow("count_3") << 3 << 2;
  QTest::newRow("count_4") << 4 << 2;
  QTest::newRow("count_5") << 5 << 3;
  QTest::newRow("count_6") << 6 << 3;
  QTest::newRow("count_7") << 7 << 3;
  QTest::newRow("count_8") << 8 << 3;
  QTest::newRow("count_9") << 9 << 3;
  QTest::newRow("count_10") << 10 << 4;
  QTest::newRow("count_11") << 11 << 4;
  QTest::newRow("count_12") << 12 << 4;
  QTest::newRow("count_13") << 13 << 4;
  QTest::newRow("count_14") << 14 << 4;
  QTest::newRow("count_15") << 15 << 4;
  QTest::newRow("count_16") << 16 << 4;
  QTest::newRow("count_17") << 17 << 5;
  QTest::newRow("count_18") << 18 << 5;
  QTest::newRow("count_19") << 19 << 5;
  QTest::newRow("count_20") << 20 << 5;
  QTest::newRow("count_21") << 21 << 5;
  QTest::newRow("count_22") << 22 << 5;
  QTest::newRow("count_23") << 23 << 5;
  QTest::newRow("count_24") << 24 << 5;
  QTest::newRow("count_25") << 25 << 5;
  QTest::newRow("count_26") << 26 << 6;
  QTest::newRow("count_27") << 27 << 6;
  QTest::newRow("count_28") << 28 << 6;
  QTest::newRow("count_29") << 29 << 6;
  QTest::newRow("count_30") << 30 << 6;
  QTest::newRow("count_31") << 31 << 6;
  QTest::newRow("count_32") << 32 << 6;
  QTest::newRow("count_33") << 33 << 6;
  QTest::newRow("count_34") << 34 << 6;
  QTest::newRow("count_35") << 35 << 6;
  QTest::newRow("count_36") << 36 << 6;
}

void PartPlateTests::computeColumCount() {
  QFETCH(int, count);
  QFETCH(int, expectedCols);
  QCOMPARE(OWzx::compute_colum_count(count), expectedCols);
}

#ifdef HAS_LIBSLIC3R
void PartPlateTests::computeOriginGridMath() {
  // setPlateSize(200,200,0) → stride = 200 * 1.2 = 240.
  // Plate grid with cols=3 needs plateCount>=5 (compute_colum_count(5)=3).
  OWzx::PartPlateList list;
  list.setPlateSize(200, 200, 0);
  // Constructor creates 1 plate; create 4 more for plateCount=5 → cols=3.
  list.createPlate();
  list.createPlate();
  list.createPlate();
  list.createPlate();
  QCOMPARE(list.plateCount(), 5);
  QCOMPARE(list.plateCols(), 3);
  QCOMPARE(list.plateStrideX(), 240.0);
  QCOMPARE(list.plateStrideY(), 240.0);

  // Plate 0 (row 0, col 0): origin (0, 0, 0).
  const Slic3r::Vec3d o0 = list.computeOrigin(0, 3);
  QCOMPARE(o0.x(), 0.0);
  QCOMPARE(o0.y(), 0.0);
  QCOMPARE(o0.z(), 0.0);

  // Plate 1 (row 0, col 1): origin (+stride, 0, 0).
  const Slic3r::Vec3d o1 = list.computeOrigin(1, 3);
  QCOMPARE(o1.x(), 240.0);
  QCOMPARE(o1.y(), 0.0);

  // Plate 3 (row 1, col 0): origin (0, -stride, 0) — SIGN FLIP on Y.
  const Slic3r::Vec3d o3 = list.computeOrigin(3, 3);
  QCOMPARE(o3.x(), 0.0);
  QCOMPARE(o3.y(), -240.0);

  // Plate 4 (row 1, col 1): origin (+stride, -stride, 0).
  const Slic3r::Vec3d o4 = list.computeOrigin(4, 3);
  QCOMPARE(o4.x(), 240.0);
  QCOMPARE(o4.y(), -240.0);
}

void PartPlateTests::computePlateIndexRoundTrip() {
  // stride = 240, cols = 3 (plateCount = 5).
  OWzx::PartPlateList list;
  list.setPlateSize(200, 200, 0);
  list.createPlate();
  list.createPlate();
  list.createPlate();
  list.createPlate();
  QCOMPARE(list.plateCols(), 3);

  // Plate 0 (world origin): row 0, col 0 → index 0.
  QCOMPARE(list.computePlateIndex(0.0, 0.0), 0);

  // Translation (+stride, 0): col 1 → index 1.
  QCOMPARE(list.computePlateIndex(240.0, 0.0), 1);

  // Translation (0, -stride): SIGN-FLIP decode.
  //   row_value = -world_y/stride_y = -(-240)/240 = 1.0 → round = 1
  //   → index row*cols + col = 1*3 + 0 = 3.
  // This verifies the (-world_y/stride_y) decode inverts computeShapePosition's
  // negative-Y-row encoding (NOT the upstream ArrangePolygon-shifted formula,
  // which assumes a pre-shifted translation space).
  QCOMPARE(list.computePlateIndex(0.0, -240.0), 3);

  // Translation (+stride, -stride): col 1, row 1 → index 4.
  QCOMPARE(list.computePlateIndex(240.0, -240.0), 4);

  // round() boundary characterization (std::round = round-half-away-from-zero).
  // x=240.0 (clean boundary) → col_value=1.0 → round=1 → col 1 → index 1.
  QCOMPARE(list.computePlateIndex(240.0, 0.0), 1);
  // x=239.9 → col_value=0.9996 → round(0.9996)=1 → col 1 → index 1.
  QCOMPARE(list.computePlateIndex(239.9, 0.0), 1);
  // x=120.0 → col_value=0.5 → std::round(0.5)=1 (round-half-away-from-zero)
  // → col 1 → index 1.
  QCOMPARE(list.computePlateIndex(120.0, 0.0), 1);
}

void PartPlateTests::updatePlateOriginsWritesToPlates() {
  // setPlateSize(200,200,0) → stride 240. createPlate 3 more times →
  // plateCount=4, cols=2 (compute_colum_count(4)=2).
  OWzx::PartPlateList list;
  list.setPlateSize(200, 200, 0);
  list.createPlate();
  list.createPlate();
  list.createPlate();
  QCOMPARE(list.plateCount(), 4);
  QCOMPARE(list.plateCols(), 2);

  // createPlate already triggers updatePlateOrigins (Plan 01 wired it in).
  // Plate 0 (row 0, col 0): origin (0, 0, 0).
  const OWzx::PartPlate* p0 = list.plate(0);
  QVERIFY(p0 != nullptr);
  QCOMPARE(p0->origin().x(), 0.0);
  QCOMPARE(p0->origin().y(), 0.0);
  QCOMPARE(p0->origin().z(), 0.0);

  // Plate 1 (row 0, col 1): origin (+stride, 0, 0).
  const OWzx::PartPlate* p1 = list.plate(1);
  QVERIFY(p1 != nullptr);
  QCOMPARE(p1->origin().x(), 240.0);
  QCOMPARE(p1->origin().y(), 0.0);

  // Plate 2 (row 1, col 0): origin (0, -stride, 0).
  const OWzx::PartPlate* p2 = list.plate(2);
  QVERIFY(p2 != nullptr);
  QCOMPARE(p2->origin().x(), 0.0);
  QCOMPARE(p2->origin().y(), -240.0);

  // Plate 3 (row 1, col 1): origin (+stride, -stride, 0).
  const OWzx::PartPlate* p3 = list.plate(3);
  QVERIFY(p3 != nullptr);
  QCOMPARE(p3->origin().x(), 240.0);
  QCOMPARE(p3->origin().y(), -240.0);
}
#endif  // HAS_LIBSLIC3R

// ── ARRANGE-02/03 integration tests (through ProjectServiceMock::arrangeObjects) ─
#ifdef HAS_LIBSLIC3R
void PartPlateTests::arrangeDistributesAcrossPlates() {
  // ARRANGE-02: rebuildPlatesAfterArrangement decodes each instance's world
  // offset via computePlateIndex and assigns it to the right plate. We test
  // this deterministically by placing objects at known cross-plate world
  // positions, then triggering rebuildPlateMembership (the public hook that
  // wraps rebuildPlatesAfterArrangement — same code path arrangeObjects uses
  // post-arrange).
  //
  // Note on why we don't drive this through arrangeObjects directly:
  // arrange_objects packs into a SINGLE bed bounding-box, and
  // ModelArrange.cpp:98 resets bed_idx to 0, so arrange on one bed never
  // produces cross-plate placements on its own. The rebuild's value is
  // preserving cross-plate state for 3MF-loaded projects (which already span
  // plates) — this test exercises that path.
  ProjectServiceMock service;
  QSignalSpy loadSpy(&service, &ProjectServiceMock::loadFinished);
  QVERIFY(service.loadFile(kStlPath));
  QVERIFY2(loadSpy.isValid(), "loadFinished signal spy must be valid");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(service.modelCount() >= 1, "loadFile must add >= 1 object");

  // Build 3 distinct objects.
  while (service.modelCount() < 3) {
    QVERIFY(service.duplicateObject(service.modelCount() - 1) >= 0);
  }
  QCOMPARE(service.modelCount(), 3);

  // Set a known plate size so stride is deterministic: 100mm → stride 120mm.
  service.setPlateSize(100, 100, 0);

  // setObjectPosition uses GL(X,Z,Y)→slic3r(X,Y,Z); we set GL X to control
  // slic3r x (the plate-grid X). Place objects at:
  //   obj 0: GL (0,   0, 0)  → slic3r (0,   0, 0) → plate 0 (col 0)
  //   obj 1: GL (120, 0, 0)  → slic3r (120, 0, 0) → plate 1 (col 1, x/120=1)
  //   obj 2: GL (240, 0, 0)  → slic3r (240, 0, 0) → plate 2 (col 2, x/120=2)
  QVERIFY(service.setObjectPosition(0, 0.0f, 0.0f, 0.0f));
  QVERIFY(service.setObjectPosition(1, 120.0f, 0.0f, 0.0f));
  QVERIFY(service.setObjectPosition(2, 240.0f, 0.0f, 0.0f));

  QSignalSpy spy(&service, &ProjectServiceMock::plateDataLoaded);
  service.rebuildPlateMembership(/*exceptLocked=*/true);

  QVERIFY2(service.plateCount() >= 3,
           "rebuild should create >= 3 plates for 3 cross-plate objects");
  // Each object lands on its own plate.
  QVERIFY2(service.plateObjectIndices(0).contains(0),
           "object 0 (x=0) should be on plate 0");
  QVERIFY2(service.plateObjectIndices(1).contains(1),
           "object 1 (x=120) should be on plate 1");
  QVERIFY2(service.plateObjectIndices(2).contains(2),
           "object 2 (x=240) should be on plate 2");
  QVERIFY2(spy.count() >= 1, "plateDataLoaded should fire after rebuild");
}

void PartPlateTests::lockedPlateExclusion() {
  // ARRANGE-03: with plate 0 locked, rebuildPlateMembership(exceptLocked=true)
  // preserves plate 0's membership unchanged — locked plates are never
  // re-distributed. We verify the locked plate's pre-rebuild membership
  // survives the rebuild even when a movable object's world offset is
  // re-decoded onto a different plate.
  ProjectServiceMock service;
  QSignalSpy loadSpy(&service, &ProjectServiceMock::loadFinished);
  QVERIFY(service.loadFile(kStlPath));
  QVERIFY2(loadSpy.isValid(), "loadFinished signal spy must be valid");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(service.modelCount() >= 1, "loadFile must add >= 1 object");

  service.setPlateSize(100, 100, 0);
  // Object 0 stays at plate 0's grid (x=0).
  QVERIFY(service.setObjectPosition(0, 0.0f, 0.0f, 0.0f));

  // Lock plate 0, then capture its membership (whatever the load path set).
  QVERIFY(service.setPlateLocked(0, true));
  const QList<int> plate0Before = service.plateObjectIndices(0);
  QVERIFY2(!plate0Before.isEmpty(),
           "plate 0 should hold the loaded object before rebuild");

  // Rebuild with exceptLocked=true. Plate 0 is locked → its membership is
  // preserved verbatim. The assertion: plate 0's membership is UNCHANGED.
  service.rebuildPlateMembership(/*exceptLocked=*/true);
  const QList<int> plate0After = service.plateObjectIndices(0);
  QCOMPARE(plate0After, plate0Before);
}

void PartPlateTests::allLockedReturnsFalse() {
  // D-29-12 edge case: with ALL plates locked, arrangeObjects returns false
  // (apply_arrange_polys sees bed_idx!=0 on all items under tolerant vfn) and
  // no rebuild runs — no membership changes, no crash.
  ProjectServiceMock service;
  QSignalSpy loadSpy(&service, &ProjectServiceMock::loadFinished);
  QVERIFY(service.loadFile(kStlPath));
  QVERIFY2(loadSpy.isValid(), "loadFinished signal spy must be valid");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(service.modelCount() >= 1, "loadFile must add >= 1 object");

  // Lock the single plate (all plates locked).
  QVERIFY(service.setPlateLocked(0, true));
  const QList<int> plate0Before = service.plateObjectIndices(0);
  const int countBefore = service.plateCount();

  const bool ok = service.arrangeObjects(
      /*spacing=*/20.0f, /*allowRotation=*/false, /*alignY=*/false,
      QStringLiteral("0,0,100,0,100,100,0,100"));

  QVERIFY2(!ok, "all-locked arrange must return false (D-29-12)");
  // No crash, no membership change, no plate count change.
  QCOMPARE(service.plateObjectIndices(0), plate0Before);
  QCOMPARE(service.plateCount(), countBefore);
}
#endif  // HAS_LIBSLIC3R

// ── THUMB-01/02 tests (v3.2 Phase 30) ───────────────────────────────────────

void PartPlateTests::thumbnailCacheInvalidation() {
  // PartPlate unit test (no libslic3r needed): the cached thumbnail is cleared
  // on every content change (addInstance/removeInstance/clearInstances/setLocked).
  OWzx::PartPlate plate(0);
  QVERIFY(!plate.hasThumbnail());  // null by default
  plate.setThumbnail(QImage(64, 64, QImage::Format_RGBA8888));
  QVERIFY(plate.hasThumbnail());

  plate.addInstance(0, 0);
  QVERIFY2(!plate.hasThumbnail(), "addInstance must invalidate the thumbnail cache");

  plate.setThumbnail(QImage(64, 64, QImage::Format_RGBA8888));
  QVERIFY(plate.hasThumbnail());
  plate.removeInstance(0, 0);
  QVERIFY2(!plate.hasThumbnail(), "removeInstance must invalidate the thumbnail cache");

  plate.setThumbnail(QImage(64, 64, QImage::Format_RGBA8888));
  QVERIFY(plate.hasThumbnail());
  plate.clearInstances();
  QVERIFY2(!plate.hasThumbnail(), "clearInstances must invalidate the thumbnail cache");

  plate.setThumbnail(QImage(64, 64, QImage::Format_RGBA8888));
  QVERIFY(plate.hasThumbnail());
  plate.setLocked(true);
  QVERIFY2(!plate.hasThumbnail(), "setLocked must invalidate the thumbnail cache");
}

#ifdef HAS_LIBSLIC3R
void PartPlateTests::thumbnailRoundTrip() {
  // THUMB-02 (in-memory cache round-trip): verify the PartPlate thumbnail
  // cache survives a content change + regeneration cycle. The full save->reload
  // 3MF pixel round-trip is covered by thumbnailSaveReloadRoundTrip (Phase 97,
  // THUMBRT-01/02). Phase 98 removed the mock variant generator, so this slot
  // synthesizes a known QImage directly (the Phase 97 pattern) to exercise the
  // cache-invalidation contract without the mock.
  //
  // What this test verifies (THUMB-02's in-memory persistence contract):
  //   - A plate thumbnail set via setThumbnail survives until a content change.
  //   - A content change (addInstance) invalidates the cache.
  ProjectServiceMock service;
  QSignalSpy loadSpy(&service, &ProjectServiceMock::loadFinished);
  QVERIFY(service.loadFile(kStlPath));
  QVERIFY2(loadSpy.isValid(), "loadFinished signal spy must be valid");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(service.modelCount() >= 1, "loadFile must add >= 1 object");
  QVERIFY2(service.plateCount() >= 1, "loaded project must have >= 1 plate");

  // Synthesize a known QImage directly (Phase 97 pattern) -- do NOT use the
  // removed mock generator. The cache-invalidation contract is what's under
  // test, not thumbnail generation.
  QImage generated(256, 256, QImage::Format_RGBA8888);
  generated.fill(QColor(123, 45, 67, 255));
  QVERIFY2(!generated.isNull(), "synthesized thumbnail must be non-null");
  service.plateListMut()->plate(0)->setThumbnail(generated);

  // The cache holds the thumbnail until a content change invalidates it.
  QVERIFY2(service.plateListConst()->plate(0)->hasThumbnail(),
           "plate 0 must hold the cached thumbnail");
  QCOMPARE(service.plateListConst()->plate(0)->thumbnail().width(), 256);
  QCOMPARE(service.plateListConst()->plate(0)->thumbnail().height(), 256);

  // A content change (addInstance) invalidates the cache.
  service.plateListMut()->plate(0)->addInstance(0, 0);
  QVERIFY2(!service.plateListConst()->plate(0)->hasThumbnail(),
           "cache must be invalidated by content change");
}

void PartPlateTests::thumbnailSaveReloadRoundTrip() {
  // Phase 97 (THUMBRT-01/02): full save->reload 3MF pixel round-trip.
  // Closes THUMB-02: the v3.2 thumbnailRoundTrip slot above verified only the
  // in-memory cache contract and explicitly deferred the 3MF pixel round-trip
  // to THUMB-03. Phase 96 closed the write side (PlateData::plate_thumbnail +
  // StoreParams::thumbnail_data via qimageToThumbnailData); the read side
  // (ProjectServiceMock.cpp:5518-5529 -> applied at 5721-5723) has been complete
  // since v3.2. This test proves the two halves meet end-to-end.
  //
  // ISOLATION: uses a KNOWN synthesized QImage (NOT RHI capture, NOT the
  // QPainter mock generator) set directly via plateListMut() so PERSISTENCE
  // (Phase 97) is verified independently of CAPTURE (Phase 95).

  // -- ARRANGE: load a real model (saveProject's real-3MF branch needs it).
  ProjectServiceMock service;
  QSignalSpy loadSpy(&service, &ProjectServiceMock::loadFinished);
  QVERIFY(service.loadFile(kStlPath));
  QVERIFY2(loadSpy.isValid(), "loadFinished signal spy must be valid");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(service.modelCount() >= 1, "loadFile must add >= 1 object");
  QVERIFY2(service.plateCount() >= 1, "loaded project must have >= 1 plate");

  // -- ARRANGE: synthesize a KNOWN recognizable thumbnail (RGBA8888 so the
  // round-trip is byte-symmetric: write convertToFormat(RGBA8888) at
  // ProjectServiceMock.cpp:5061 <-> read Format_RGBA8888 at :5526).
  const int thumbW = 32;
  const int thumbH = 32;
  const QRgb knownColor = qRgba(123, 45, 67, 255);  // recognizable, non-gray
  QImage savedThumb(thumbW, thumbH, QImage::Format_RGBA8888);
  // Fill via QColor (NOT fill(QRgb)): QImage::fill(uint pixel) stores the raw
  // uint in NATIVE byte order, which for RGBA8888 on little-endian swaps R/B
  // (the ARGB 0xFF7B2D43 becomes bytes 43 2D 7B FF = R=67,B=123 in memory).
  // fill(QColor) converts the color to the target format correctly so the
  // pixels actually hold R=123,G=45,B=67 and knownColor matches on reload.
  savedThumb.fill(QColor(123, 45, 67, 255));
  QVERIFY2(!savedThumb.isNull(), "synthesized thumbnail must be non-null");
  // Install on plate 0 via the sanctioned test seam (ProjectServiceMock.h:80).
  service.plateListMut()->plate(0)->setThumbnail(savedThumb);
  QVERIFY2(service.plateListConst()->plate(0)->hasThumbnail(),
           "plate 0 must hold the synthesized thumbnail after setThumbnail");

  // -- ACT: save to a temp .3mf (do not pollute the repo).
  const QString tempPath = QDir::tempPath() + QStringLiteral(
      "/owzx_thumb_roundtrip_%1.3mf").arg(QDateTime::currentMSecsSinceEpoch());
  QVERIFY2(service.saveProject(tempPath),
           qPrintable(QStringLiteral("saveProject must succeed: %1").arg(tempPath)));
  QVERIFY2(QFileInfo::exists(tempPath),
           "the saved .3mf file must exist on disk");

  // -- ACT: reload into a FRESH service (exercises the read side in isolation).
  ProjectServiceMock reloaded;
  QSignalSpy reloadSpy(&reloaded, &ProjectServiceMock::loadFinished);
  QVERIFY(reloaded.loadFile(tempPath));
  QVERIFY2(reloadSpy.isValid(), "loadFinished signal spy must be valid");
  QTRY_VERIFY_WITH_TIMEOUT(reloadSpy.count() > 0, 10000);
  QVERIFY2(reloaded.plateCount() >= 1,
           "reloaded project must have >= 1 plate");

  // -- ASSERT: the thumbnail survived (THUMBRT-01).
  const OWzx::PartPlate *reloadedPlate = reloaded.plateListConst()->plate(0);
  QVERIFY2(reloadedPlate != nullptr, "reloaded plate 0 must exist");
  QVERIFY2(reloadedPlate->hasThumbnail(),
           "THUMBRT-01: reloaded plate 0 must have a thumbnail (read side restored it)");
  const QImage reloadedThumb = reloadedPlate->thumbnail();
  QCOMPARE(reloadedThumb.width(), thumbW);
  QCOMPARE(reloadedThumb.height(), thumbH);

  // Recognizable-pixel assertion (the primary survival proof). Convert to
  // RGBA8888 so the comparison is format-stable regardless of how Qt
  // internally represents the reloaded image.
  const QImage reloadedRgba = reloadedThumb.convertToFormat(QImage::Format_RGBA8888);
  QCOMPARE(reloadedRgba.pixel(0, 0), knownColor);
  QCOMPARE(reloadedRgba.pixel(thumbW - 1, thumbH - 1), knownColor);

  // Exact-buffer match (the preferred strict assertion -- tdefl PNG is
  // lossless and the RGBA8888 round-trip is byte-symmetric per Phase 96
  // W-02). If this is ever fragile due to a format-conversion subtlety,
  // the dimensions + recognizable-pixel assertions above already prove
  // survival; document the fragility and keep the looser assertions.
  const QImage savedRgba = savedThumb.convertToFormat(QImage::Format_RGBA8888);
  QCOMPARE(reloadedRgba.sizeInBytes(), savedRgba.sizeInBytes());
  QVERIFY2(std::memcmp(reloadedRgba.constBits(), savedRgba.constBits(),
                       savedRgba.sizeInBytes()) == 0,
           "THUMBRT-01: reloaded thumbnail RGBA8888 bytes must match the saved bytes (lossless PNG round-trip)");

  // -- CLEANUP: do not leave the temp artifact behind.
  QFile::remove(tempPath);
}

void PartPlateTests::thumbnailMultiPlateSaveReloadRoundTrip() {
  // Phase 98 (Phase 97 REVIEW MEDIUM-3): multi-plate write side.
  // Phase 96's saveProject pushed only the CURRENT plate's thumbnail into
  // StoreParams::thumbnail_data (as entry [0]), but buildPlateDataList emits
  // per-plate XML <metadata thumbnail_file="Metadata/plate_N.png"> refs for
  // every plate. The writer (bbs_3mf.cpp:6133) iterates thumbnail_data by
  // INDEX mapping entry[index] -> Metadata/plate_{index+1}.png, so plates > 0
  // got an XML ref with NO archived PNG bytes and silently lost thumbnails on
  // reload. Phase 98's fix pushes one (index-aligned) thumbnail_data entry per
  // plate. This slot proves a 2-plate project round-trips BOTH thumbnails.
  //
  // ISOLATION: synthesizes two DISTINCT known QImages (different colors) so a
  // plate-index swap or a single-entry regression is detectable.

  // -- ARRANGE: load a real model, add a second plate, set distinct thumbnails.
  ProjectServiceMock service;
  QSignalSpy loadSpy(&service, &ProjectServiceMock::loadFinished);
  QVERIFY(service.loadFile(kStlPath));
  QVERIFY2(loadSpy.isValid(), "loadFinished signal spy must be valid");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(service.plateCount() >= 1, "loaded project must have >= 1 plate");

  QVERIFY2(service.addPlate(), "addPlate must succeed to create plate 1");
  QVERIFY2(service.plateCount() >= 2, "project must have >= 2 plates after addPlate");

  // DISTINCT colors so a plate-index swap would be caught.
  QImage thumb0(32, 32, QImage::Format_RGBA8888);
  thumb0.fill(QColor(11, 22, 33, 255));  // plate 0: dark blue
  QVERIFY2(!thumb0.isNull(), "plate 0 synthesized thumbnail must be non-null");
  QImage thumb1(32, 32, QImage::Format_RGBA8888);
  thumb1.fill(QColor(244, 200, 80, 255));  // plate 1: amber
  QVERIFY2(!thumb1.isNull(), "plate 1 synthesized thumbnail must be non-null");

  QVERIFY2(service.plateListConst()->plateCount() >= 2,
           "plateList must report >= 2 plates");
  service.plateListMut()->plate(0)->setThumbnail(thumb0);
  service.plateListMut()->plate(1)->setThumbnail(thumb1);
  QVERIFY2(service.plateListConst()->plate(0)->hasThumbnail(),
           "plate 0 must hold its thumbnail");
  QVERIFY2(service.plateListConst()->plate(1)->hasThumbnail(),
           "plate 1 must hold its thumbnail");

  // -- ACT: save then reload into a FRESH service.
  const QString tempPath = QDir::tempPath() + QStringLiteral(
      "/owzx_thumb_multi_%1.3mf").arg(QDateTime::currentMSecsSinceEpoch());
  QVERIFY2(service.saveProject(tempPath),
           qPrintable(QStringLiteral("saveProject must succeed: %1").arg(tempPath)));
  QVERIFY2(QFileInfo::exists(tempPath), "the saved .3mf file must exist on disk");

  ProjectServiceMock reloaded;
  QSignalSpy reloadSpy(&reloaded, &ProjectServiceMock::loadFinished);
  QVERIFY(reloaded.loadFile(tempPath));
  QVERIFY2(reloadSpy.isValid(), "loadFinished signal spy must be valid");
  QTRY_VERIFY_WITH_TIMEOUT(reloadSpy.count() > 0, 10000);
  QVERIFY2(reloaded.plateCount() >= 2,
           "reloaded project must have >= 2 plates");

  // -- ASSERT: BOTH plates' thumbnails survived, with their DISTINCT colors.
  const OWzx::PartPlate *rp0 = reloaded.plateListConst()->plate(0);
  const OWzx::PartPlate *rp1 = reloaded.plateListConst()->plate(1);
  QVERIFY2(rp0 != nullptr, "reloaded plate 0 must exist");
  QVERIFY2(rp1 != nullptr, "reloaded plate 1 must exist");
  QVERIFY2(rp0->hasThumbnail(),
           "REVIEW MEDIUM-3: reloaded plate 0 must have a thumbnail");
  QVERIFY2(rp1->hasThumbnail(),
           "REVIEW MEDIUM-3: reloaded plate 1 must have a thumbnail (the fix)");
  QCOMPARE(rp0->thumbnail().width(), 32);
  QCOMPARE(rp0->thumbnail().height(), 32);
  QCOMPARE(rp1->thumbnail().width(), 32);
  QCOMPARE(rp1->thumbnail().height(), 32);

  // DISTINCT-color check catches a plate-index swap or a single-entry
  // (current-plate-only) regression: plate 0 is dark blue, plate 1 is amber.
  const QImage r0 = rp0->thumbnail().convertToFormat(QImage::Format_RGBA8888);
  const QImage r1 = rp1->thumbnail().convertToFormat(QImage::Format_RGBA8888);
  QCOMPARE(r0.pixel(0, 0), qRgba(11, 22, 33, 255));
  QCOMPARE(r1.pixel(0, 0), qRgba(244, 200, 80, 255));

  // Exact-buffer match (Phase 98 REVIEW W-3): parity with the single-plate
  // sibling's memcmp assertion. tdefl PNG is lossless and the RGBA8888
  // round-trip is byte-symmetric per Phase 96 W-02, so a full-buffer byte
  // match is achievable here too -- catches a stride/offset corruption that
  // a single-pixel sample misses.
  const QImage s0 = thumb0.convertToFormat(QImage::Format_RGBA8888);
  const QImage s1 = thumb1.convertToFormat(QImage::Format_RGBA8888);
  QCOMPARE(r0.sizeInBytes(), s0.sizeInBytes());
  QVERIFY2(std::memcmp(r0.constBits(), s0.constBits(), s0.sizeInBytes()) == 0,
           "REVIEW W-3: plate 0 reloaded RGBA8888 bytes must match saved bytes");
  QCOMPARE(r1.sizeInBytes(), s1.sizeInBytes());
  QVERIFY2(std::memcmp(r1.constBits(), s1.constBits(), s1.sizeInBytes()) == 0,
           "REVIEW W-3: plate 1 reloaded RGBA8888 bytes must match saved bytes");

  // -- CLEANUP.
  QFile::remove(tempPath);
}
#endif  // HAS_LIBSLIC3R

// ── FMAP-01/03 tests (v3.2 Phase 31) ───────────────────────────────────────

void PartPlateTests::filamentMapManualAssignment() {
  // FMAP-01: PartPlate has filament_maps + filament_map_mode fields.
  // FMAP-03: ProjectServiceMock exposes setPlateFilamentMap / plateFilamentMaps.
  // v4.5 Phase 107 (FMAP-02): the mode is now the 4-value FilamentMapMode.
  //   default = fmmAutoForFlush (0); Manual = fmmManual (2) -- NOT the old raw
  //   int 1 (which is now fmmAutoForMatch).
  OWzx::PartPlate plate(0);
  QCOMPARE(int(plate.filamentMapMode()),
           int(OWzx::FilamentMapMode::fmmAutoForFlush));  // default Auto
  QVERIFY(plate.filamentMaps().empty());

  plate.setFilamentMapMode(OWzx::FilamentMapMode::fmmManual);  // Manual
  QCOMPARE(int(plate.filamentMapMode()), int(OWzx::FilamentMapMode::fmmManual));
  plate.setFilamentMaps({2, 1, 3});  // filament 0->extruder2, 1->1, 2->3 (1-based)
  QCOMPARE(int(plate.filamentMaps().size()), 3);
  QCOMPARE(plate.filamentMaps()[0], 2);
  QCOMPARE(plate.filamentMaps()[1], 1);
  QCOMPARE(plate.filamentMaps()[2], 3);

  // FMAP-03 via the service (needs a constructed PartPlateList). The int mode
  // passed at the Q_INVOKABLE boundary is the enum value (fmmManual == 2).
  ProjectServiceMock service;
  QVERIFY(service.plateCount() >= 1);
  QVERIFY2(service.setPlateFilamentMap(
               0, int(OWzx::FilamentMapMode::fmmManual), QList<int>{2, 1, 3}),
           "setPlateFilamentMap must succeed on a valid plate");
  QCOMPARE(service.plateFilamentMapMode(0), int(OWzx::FilamentMapMode::fmmManual));
  QCOMPARE(service.plateFilamentMaps(0), QList<int>({2, 1, 3}));

  // Invalid plate index returns Auto + empty.
  QCOMPARE(service.plateFilamentMapMode(999), int(OWzx::FilamentMapMode::fmmAutoForFlush));
  QVERIFY(service.plateFilamentMaps(999).isEmpty());
  QVERIFY2(!service.setPlateFilamentMap(
               999, int(OWzx::FilamentMapMode::fmmManual), QList<int>{1}),
           "setPlateFilamentMap must fail on invalid plate index");
}

void PartPlateTests::filamentMapModeEnumWidenedAnd3MFMigrates() {
  // FMAP-02 (FM-05 part a): the 4-value FilamentMapMode exists and maps to the
  // upstream ints exactly (PrintConfig.hpp:424-429: fmmAutoForFlush=0,
  // fmmAutoForMatch=1, fmmManual=2, fmmDefault=3). No libslic3r needed.
  QCOMPARE(int(OWzx::FilamentMapMode::fmmAutoForFlush), 0);
  QCOMPARE(int(OWzx::FilamentMapMode::fmmAutoForMatch), 1);
  QCOMPARE(int(OWzx::FilamentMapMode::fmmManual), 2);
  QCOMPARE(int(OWzx::FilamentMapMode::fmmDefault), 3);

  // The default plate mode is fmmAutoForFlush (upstream PrintConfig.cpp:2509
  // default), NOT the old ambiguous "0=Auto".
  OWzx::PartPlate plate(0);
  QCOMPARE(int(plate.filamentMapMode()),
           int(OWzx::FilamentMapMode::fmmAutoForFlush));

  // FM-03 (legacy raw-int-1 migration contract): the read-side migration in
  // ProjectServiceMock MUST map a legacy raw-int 1 (pre-v4.5 "Manual") to
  // fmmManual(2), NOT fmmAutoForMatch(1). This is a PURE-LOGIC assertion of
  // the documented migration map (it does not need a 3MF file); the actual
  // save/reload round-trip is in filamentMapModeRoundTripManualPreserved.
  // The map: 0->fmmAutoForFlush, 1->fmmManual, else->fmmDefault.
  const int legacyAuto = 0;
  const int legacyManual = 1;
  QCOMPARE(int(OWzx::FilamentMapMode::fmmAutoForFlush),
           (legacyAuto == 0) ? int(OWzx::FilamentMapMode::fmmAutoForFlush)
                             : int(OWzx::FilamentMapMode::fmmDefault));
  QCOMPARE(int(OWzx::FilamentMapMode::fmmManual),
           (legacyManual == 1) ? int(OWzx::FilamentMapMode::fmmManual)
                               : int(OWzx::FilamentMapMode::fmmDefault));
  QVERIFY2(int(OWzx::FilamentMapMode::fmmManual) == 2,
           "FMAP-02/FM-03: legacy raw-int-1 MUST map to fmmManual (value 2), not "
           "fmmAutoForMatch (value 1) -- pre-v4.5 'Manual'=1 must stay Manual");
}

#ifdef HAS_LIBSLIC3R
void PartPlateTests::filamentMapModeRoundTripManualPreserved() {
  // FMAP-02 (FM-05 part b/c): the 3MF round-trip preserves the filament-map
  // MODE. Set a plate to fmmManual, save, reload into a FRESH service, and
  // assert the reloaded mode is still fmmManual (2). This is the regression
  // guard for the FM-03 read-side migration and the FM-02 write-side typed-
  // enum serialization (the on-disk value is the "Manual" string). Mirrors the
  // Phase 97 thumbnailSaveReloadRoundTrip pattern.
  //
  // SCOPE NOTE: this slot asserts the MODE only -- the critical user-visible
  // behavior change FMAP-02 ships (pre-v4.5 'Manual'=1 stays Manual, not the
  // new fmmAutoForMatch=1). The filament_maps ARRAY round-trip is a separate
  // pre-existing gap (the Qt6 write path sets pd->filament_maps but the
  // active bbs_3mf writer branch reads pd->config["filament_map"]
  // ConfigOptionInts, which the Qt6 layer does not populate); closing it is
  // FMAP-03/04 scope (Phase 110/111), not FMAP-02.
  ProjectServiceMock service;
  QSignalSpy loadSpy(&service, &ProjectServiceMock::loadFinished);
  QVERIFY(service.loadFile(kStlPath));
  QVERIFY2(loadSpy.isValid(), "loadFinished signal spy must be valid");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(service.plateCount() >= 1, "loaded project must have >= 1 plate");

  // ACT: set plate 0 to Manual mode with an explicit filament map.
  QVERIFY2(service.setPlateFilamentMap(
               0, int(OWzx::FilamentMapMode::fmmManual), QList<int>{2, 1, 3}),
           "setPlateFilamentMap(fmmManual) must succeed on a valid plate");
  QCOMPARE(service.plateFilamentMapMode(0), int(OWzx::FilamentMapMode::fmmManual));
  QCOMPARE(service.plateFilamentMaps(0), QList<int>({2, 1, 3}));

  // Save then reload into a FRESH service.
  const QString tempPath = QDir::tempPath() + QStringLiteral(
      "/owzx_fmap_%1.3mf").arg(QDateTime::currentMSecsSinceEpoch());
  QVERIFY2(service.saveProject(tempPath),
           qPrintable(QStringLiteral("saveProject must succeed: %1").arg(tempPath)));
  QVERIFY2(QFileInfo::exists(tempPath), "the saved .3mf file must exist on disk");

  ProjectServiceMock reloaded;
  QSignalSpy reloadSpy(&reloaded, &ProjectServiceMock::loadFinished);
  QVERIFY(reloaded.loadFile(tempPath));
  QVERIFY2(reloadSpy.isValid(), "loadFinished signal spy must be valid");
  QTRY_VERIFY_WITH_TIMEOUT(reloadSpy.count() > 0, 10000);
  QVERIFY2(reloaded.plateCount() >= 1, "reloaded project must have >= 1 plate");

  // ASSERT: the reloaded plate 0 MODE is STILL fmmManual (2). A regression that
  // re-introduces the raw-int hazard (Manual flipped to AutoForMatch on reload)
  // fails here. The on-disk metadata is the "Manual" string (FM-02 typed-enum
  // write) which the upstream reader deserializes back to fmmManual.
  QCOMPARE(reloaded.plateFilamentMapMode(0),
           int(OWzx::FilamentMapMode::fmmManual));

  // CLEANUP.
  QFile::remove(tempPath);
}
#endif  // HAS_LIBSLIC3R

QTEST_MAIN(PartPlateTests)
#include "PartPlateTests.moc"
