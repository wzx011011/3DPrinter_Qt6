// ProjectMirrorTests -- S5 (audit 2026-10-10, chapter 4) permanent behavior
// locks for the first m_mockVolumes mirror-retirement cut.
//
// S5-CHANGEVOL: ProjectService::changeVolumeType used to gate on the
// m_mockVolumes parallel mirror, which the real-engine build never populates
// (every writer path goes straight to the Slic3r::Model). File-imported
// objects therefore ALWAYS failed with "part index invalid" -- the audit's
// "恒失败" bug. The fix reads the real ModelVolume (same contract as
// setVolumeExtruderId / P16.4); the mirror entry is updated when present but
// no longer required to exist.
//
// S5-CONVERGE: the 11 copy-pasted ~30-line metadata-mirror rebuild blocks are
// converged into ProjectService::syncObjectMetadataMirrors. The copy-paste
// blocks had drifted (one appended +off.y for GL.z where the loader and the
// objectPosition accessor use the AXES-ROT -off.y mapping; one used English
// fallback names; one defaulted printable to false). These tests pin the
// converged contract through public API: the JSON project format serializes
// objectPositions_ verbatim (ProjectService.cpp, saveProject JSON branch) and
// the JSON loader resets model_, so a save -> load round-trip reads the
// mirror back through objectPosition() -- any mirror/accessor drift shows up
// as a position flip.
//
// AUTOMOC note: single-file cpp-internal Q_OBJECT has weak moc dependency
// tracking. The canonical verify script re-runs cmake configure after slot
// edits; for incremental builds, delete
// build/ProjectMirrorTests_autogen/timestamp.

#include <QtTest>
#include <QDir>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QVector3D>

#include "core/services/ProjectService.h"

namespace {
// Same fixture as ViewModelSmokeTests.cpp:41 / PartPlateTests.cpp:41.
const QString kStlPath = QDir::cleanPath(
    QStringLiteral(QT_TESTCASE_SOURCEDIR) +
    QStringLiteral("/third_party/OrcaSlicer/resources/profiles/hotend.stl"));
}  // namespace

class ProjectMirrorTests final : public QObject {
  Q_OBJECT

 private slots:
  // Loads a REAL model file through the async import path (the
  // mirror-less-object repro: file imports never populate m_mockVolumes).
  void changeVolumeTypeWorksOnFileImportedObject();
  void changeVolumeTypeStillRejectsInvalidIndices();

  // Mirror-consistency contract of the converged rebuild helper.
  void mirrorsTrackModelCountAfterStructuralOps();
  void jsonRoundTripPreservesInstancePositions();
};

#ifdef HAS_LIBSLIC3R

void ProjectMirrorTests::changeVolumeTypeWorksOnFileImportedObject()
{
  ProjectService service;
  QSignalSpy loadSpy(&service, &ProjectService::loadFinished);
  QVERIFY2(QFileInfo::exists(kStlPath),
           qPrintable(QStringLiteral("Test STL not found: %1").arg(kStlPath)));
  QVERIFY(service.loadFile(kStlPath));
  QVERIFY(loadSpy.isValid());
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(service.modelCount() >= 1, "loadFile must add >= 1 object");

  // A file-imported object: single model-part volume, no mirror entry.
  QVERIFY(service.objectVolumeCount(0) >= 1);
  const int originalType = service.objectVolumeType(0, 0);
  QCOMPARE(originalType, 0);  // ModelPart

  // THE S5 ACCEPTANCE: right-click "modify part type" on a file-imported
  // object. Before the fix this returned false with "部件索引无效" for EVERY
  // object in the real-engine build.
  QVERIFY2(service.changeVolumeType(0, 0, 2),  // -> ParameterModifier
           qPrintable(service.lastError()));
  QCOMPARE(service.objectVolumeType(0, 0), 2);

  // 5/6 are Qt-only emboss labels: they must not corrupt the real
  // ModelVolumeType (only 0-4 map onto Slic3r::ModelVolumeType) -- the real
  // volume keeps the modifier type set one line above.
  QVERIFY(service.changeVolumeType(0, 0, 5));
  QCOMPARE(service.objectVolumeType(0, 0), 2);  // real type untouched

  // Round-trip back to a plain model part.
  QVERIFY(service.changeVolumeType(0, 0, originalType));
  QCOMPARE(service.objectVolumeType(0, 0), originalType);
}

void ProjectMirrorTests::changeVolumeTypeStillRejectsInvalidIndices()
{
  ProjectService service;
  QSignalSpy loadSpy(&service, &ProjectService::loadFinished);
  QVERIFY(service.loadFile(kStlPath));
  QVERIFY(loadSpy.isValid());
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);

  QVERIFY(!service.changeVolumeType(-1, 0, 2));
  QVERIFY(!service.changeVolumeType(service.modelCount(), 0, 2));
  QVERIFY(!service.changeVolumeType(0, -1, 2));
  QVERIFY(!service.changeVolumeType(0, 99, 2));
  QVERIFY(!service.changeVolumeType(0, 0, -1));
  QVERIFY(!service.changeVolumeType(0, 0, 7));
}

void ProjectMirrorTests::mirrorsTrackModelCountAfterStructuralOps()
{
  ProjectService service;
  QSignalSpy loadSpy(&service, &ProjectService::loadFinished);
  QVERIFY(service.loadFile(kStlPath));
  QVERIFY(loadSpy.isValid());
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);

  // duplicateObject -> 2 objects; mirrors must all describe 2 entries.
  QVERIFY(service.duplicateObject(0) >= 0);
  QCOMPARE(service.modelCount(), 2);
  QCOMPARE(service.objectNames().size(), 2);
  // Clone offset: duplicateObject shifts instances +20mm on slic3r X = GL X.
  QVERIFY2(service.objectPosition(1).x() > service.objectPosition(0).x(),
           "clone must sit at +20mm GL X of its source");

  // addObject -> 3; deleteObject(1) -> back to 2. Every structural op goes
  // through the single converged rebuild helper now.
  QVERIFY(service.addObject(QStringLiteral("added")) >= 0);
  QCOMPARE(service.modelCount(), 3);
  QVERIFY(service.deleteObject(1));
  QCOMPARE(service.modelCount(), 2);
  QCOMPARE(service.objectNames().size(), 2);
  // Deleted clone gone; source + added object remain.
  QCOMPARE(service.objectNames().at(1), QStringLiteral("added"));
}

void ProjectMirrorTests::jsonRoundTripPreservesInstancePositions()
{
  ProjectService service;
  QSignalSpy loadSpy(&service, &ProjectService::loadFinished);
  QVERIFY(service.loadFile(kStlPath));
  QVERIFY(loadSpy.isValid());
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);

  // A structural op runs the converged rebuild helper; then move the source
  // so Y (the AXES-ROT sign-drift axis) is definitely nonzero.
  QVERIFY(service.duplicateObject(0) >= 0);
  QVERIFY(service.setObjectPosition(0, 30.0f, 40.0f, 10.0f));
  QCOMPARE(service.modelCount(), 2);
  const QVector3D pos0 = service.objectPosition(0);
  const QVector3D pos1 = service.objectPosition(1);
  QCOMPARE(pos0, QVector3D(30.0f, 40.0f, 10.0f));

  // JSON project round-trip: save writes the mirrors verbatim; the JSON
  // loader resets model_, so objectPosition() reads the mirror afterwards.
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  const QString projectPath = tempDir.filePath(QStringLiteral("mirror_rt.json"));
  QVERIFY(service.saveProject(projectPath));

  ProjectService reloaded;
  QVERIFY(reloaded.loadProject(projectPath));
  QCOMPARE(reloaded.modelCount(), 2);
  QCOMPARE(reloaded.objectPosition(0), pos0);
  QCOMPARE(reloaded.objectPosition(1), pos1);
}

#else  // !HAS_LIBSLIC3R

void ProjectMirrorTests::changeVolumeTypeWorksOnFileImportedObject() { QSKIP("requires libslic3r"); }
void ProjectMirrorTests::changeVolumeTypeStillRejectsInvalidIndices() { QSKIP("requires libslic3r"); }
void ProjectMirrorTests::mirrorsTrackModelCountAfterStructuralOps() { QSKIP("requires libslic3r"); }
void ProjectMirrorTests::jsonRoundTripPreservesInstancePositions() { QSKIP("requires libslic3r"); }

#endif

QTEST_MAIN(ProjectMirrorTests)
#include "ProjectMirrorTests.moc"
