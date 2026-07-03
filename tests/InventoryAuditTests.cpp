// InventoryAuditTests -- Phase 58 (VERIFY-01) inventory audit harness.
//
// Encodes the 9 deterministic Phase 50 section 2 inventory checks (lines 170-286
// of .planning/phases/50-.../50-INVENTORY.md) as a permanent ctest-level
// regression guard. Each check runs against BOTH the canonical live doc
//   docs/v3.6-ui-inventory.md
// and the frozen Phase 50 snapshot
//   .planning/phases/50-screenshot-and-source-truth-inventory/50-INVENTORY.md
// so a future drift in region count, schema, status/verification enum,
// region-ID format, INV-02/03/04 coverage anchors, cleanup format, or
// upstream citation fails the canonical verify with a doc-labeled message.
//
// AUTOMOC caveat (v3.0 retrospective, see QmlUiAuditTests / ViewModelSmokeTests
// CMake comment): single-file QtTest with cpp-internal Q_OBJECT has weak moc
// dependency tracking. After adding a new private slot here, re-run cmake
// configure (the canonical verify script does this) or delete
//   build/InventoryAuditTests_autogen/timestamp
// before rebuilding, otherwise the new slot silently does not execute.
//
// Row-count convention (Phase 50 section 2 note): every region-table row has
// exactly 9 pipe-delimited cells => 10 pipes => 11 fields under split('|').
// The section 6 modify-vs-replace summary rows share the same "| PREP-..." prefix
// but have only 3 cells (4 pipes => 5 fields), so the region counter filters on
// parts.size() == 11. This is the C++ port of `awk -F'|' 'NF==11'`.

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtTest>

class InventoryAuditTests final : public QObject
{
  Q_OBJECT

private slots:
  void inventoryDocsArePresentAndNonEmpty();
  void perScreenshotRegionCountsWithinBounds();
  void regionRowsUseNineColumnSchemaAndHeaderMatchesContract();
  void statusCellsUseSevenTermVocabulary();
  void verificationCellsUseSixTermVocabulary();
  void regionIdsMatchAsciiSingleDashRegexAndAreUnique();
  void coverageAnchorsListAllRequiredClusterGlobs();
  void cleanupFormatUsesOnlySixAllowedTags();
  void noBlankUpstreamSourceCells();
  // Cross-reference slot (no body work). The Phase 57 cleanup regression
  // (CLEAN-01/02) is permanently guarded by QmlUiAuditTests::deletedSettingsPaths-
  // StayAbsent / deletedRoutesStayAbsent. This slot exists only so a future
  // reader running `ctest -R Inventory` sees the cross-reference; it does NOT
  // duplicate those assertions.
  void phase57CleanupRegressionRemainsLockedByQmlUiAudit();

private:
  QString readSource(const QString &relativePath) const;
  // Returns the loaded source (empty on failure). Callers MUST QVERIFY2 the
  // result is non-empty with a doc-labeled message -- we cannot QVERIFY2
  // inside this helper because QVERIFY2 expands to `return;` (void) on
  // failure, which conflicts with the QString return type.
  QString loadInventory(const QString &label, const QString &relativePath) const;
  // Returns the number of region-table rows (NF==11) whose first non-empty
  // trimmed cell starts with `prefix` (e.g. "PREP-"). A row is a candidate
  // only if it begins with optional whitespace then '|'.
  int countRegionRowsByPrefix(const QString &src, const QString &prefix) const;
  // Collects region-table rows (NF==11) as parsed cell lists. Each returned
  // row has exactly 9 trimmed cells (cells 0..8). Rows whose split size is
  // not exactly 11 are skipped (matches the awk NF==11 filter).
  QList<QStringList> regionRows(const QString &src) const;

  static QString canonicalPath()
  {
    return QStringLiteral("docs/v3.6-ui-inventory.md");
  }
  static QString snapshotPath()
  {
    return QStringLiteral(".planning/phases/50-screenshot-and-source-truth-inventory/50-INVENTORY.md");
  }
};

QString InventoryAuditTests::readSource(const QString &relativePath) const
{
  QFile file(QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR)).filePath(relativePath));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return {};
  return QString::fromUtf8(file.readAll());
}

QString InventoryAuditTests::loadInventory(const QString &label, const QString &relativePath) const
{
  Q_UNUSED(label);
  return readSource(relativePath);
}

QList<QStringList> InventoryAuditTests::regionRows(const QString &src) const
{
  QList<QStringList> out;
  const QStringList lines = src.split(QLatin1Char('\n'));
  for (const QString &line : lines) {
    // Region rows start with optional whitespace then '|'.
    QString trimmed = line;
    while (!trimmed.isEmpty() && (trimmed.at(0) == QLatin1Char(' ') || trimmed.at(0) == QLatin1Char('\t')))
      trimmed.remove(0, 1);
    if (!trimmed.startsWith(QLatin1Char('|')))
      continue;
    const QStringList parts = line.split(QLatin1Char('|'));
    if (parts.size() != 11)
      continue;
    QStringList cells;
    cells.reserve(9);
    // parts[0] is the leading empty (before the first '|'); cells are
    // parts[1..9]; parts[10] is the trailing empty (after the last '|').
    for (int i = 1; i <= 9; ++i)
      cells.append(parts.at(i).trimmed());
    // Skip the 9-column schema header rows themselves (cell 0 == "region_id").
    // The header row also has 9 cells / 11 fields under split('|'), so it
    // would otherwise inflate the region count. The canonical doc embeds the
    // header 6 times (4 region tables + the section 0 schema block + the
    // section 9 self-check prose); the snapshot embeds it 4 times.
    if (cells.at(0) == QLatin1String("region_id"))
      continue;
    // Skip the Markdown table separator rows (|---|---|...|) that appear
    // directly under each header. These also split into 9 cells of dashes.
    // The separator cell is all dashes (with optional colons for alignment).
    const QString &c0 = cells.at(0);
    bool isSeparator = !c0.isEmpty();
    for (const QChar ch : c0) {
      if (ch != QLatin1Char('-') && ch != QLatin1Char(':')) {
        isSeparator = false;
        break;
      }
    }
    if (isSeparator)
      continue;
    out.append(cells);
  }
  return out;
}

int InventoryAuditTests::countRegionRowsByPrefix(const QString &src, const QString &prefix) const
{
  int count = 0;
  const QList<QStringList> rows = regionRows(src);
  for (const QStringList &cells : rows) {
    if (cells.isEmpty())
      continue;
    if (cells.at(0).startsWith(prefix))
      ++count;
  }
  return count;
}

// --- Check 1: presence + non-empty --------------------------------------

void InventoryAuditTests::inventoryDocsArePresentAndNonEmpty()
{
  const QString canonical = readSource(canonicalPath());
  QVERIFY2(!canonical.isEmpty(),
           qPrintable(QStringLiteral("canonical: missing or empty -> %1").arg(canonicalPath())));
  const QString snapshot = readSource(snapshotPath());
  QVERIFY2(!snapshot.isEmpty(),
           qPrintable(QStringLiteral("snapshot: missing or empty -> %1").arg(snapshotPath())));
}

// --- Check 2: per-screenshot region counts within bounds ----------------

void InventoryAuditTests::perScreenshotRegionCountsWithinBounds()
{
  const QString canonical = loadInventory(QStringLiteral("canonical"), canonicalPath());
  QVERIFY2(!canonical.isEmpty(), qPrintable(QStringLiteral("canonical: missing %1").arg(canonicalPath())));
  const QString snapshot = loadInventory(QStringLiteral("snapshot"), snapshotPath());
  QVERIFY2(!snapshot.isEmpty(), qPrintable(QStringLiteral("snapshot: missing %1").arg(snapshotPath())));

  const QVector<QPair<QString, int>> expected = {
      {QStringLiteral("PREP-"), 9},
      {QStringLiteral("PREV-"), 9},
      {QStringLiteral("SETPRINT-"), 8},
      {QStringLiteral("SETMAT-"), 8},
  };

  for (const QString docLabel : {QStringLiteral("canonical"), QStringLiteral("snapshot")}) {
    const QString src = (docLabel == QLatin1String("canonical")) ? canonical : snapshot;
    int total = 0;
    for (const auto &entry : expected) {
      const int got = countRegionRowsByPrefix(src, entry.first);
      const QString rangeMsg = QStringLiteral("%1: prefix %2 count=%3 out of [6,12]")
                                   .arg(docLabel, entry.first, QString::number(got));
      QVERIFY2(got >= 6 && got <= 12, qPrintable(rangeMsg));
      total += got;
    }
    const QString totalMsg = QStringLiteral("%1: total region count=%2 out of [30,40]")
                                 .arg(docLabel, QString::number(total));
    QVERIFY2(total >= 30 && total <= 40, qPrintable(totalMsg));
  }
}

// --- Check 3: 9-column schema header + every region row NF==11 ----------

void InventoryAuditTests::regionRowsUseNineColumnSchemaAndHeaderMatchesContract()
{
  const QString canonical = loadInventory(QStringLiteral("canonical"), canonicalPath());
  QVERIFY2(!canonical.isEmpty(), qPrintable(QStringLiteral("canonical: missing %1").arg(canonicalPath())));
  const QString snapshot = loadInventory(QStringLiteral("snapshot"), snapshotPath());
  QVERIFY2(!snapshot.isEmpty(), qPrintable(QStringLiteral("snapshot: missing %1").arg(snapshotPath())));

  const QString header = QStringLiteral(
      "| region_id | region_name | visible_controls | qt_target | upstream_source | "
      "status | verification | modify_or_replace | cleanup |");

  for (const QString docLabel : {QStringLiteral("canonical"), QStringLiteral("snapshot")}) {
    const QString src = (docLabel == QLatin1String("canonical")) ? canonical : snapshot;
    const int headerCount = src.count(header);
    const QString headerMsg = QStringLiteral("%1: 9-col header appears %2 times, expected >= 4")
                                  .arg(docLabel, QString::number(headerCount));
    QVERIFY2(headerCount >= 4, qPrintable(headerMsg));

    // Total region rows (NF==11) across all 4 prefixes must equal 34.
    const QList<QStringList> rows = regionRows(src);
    const int regionRowCount = rows.size();
    const QString countMsg = QStringLiteral("%1: total region rows NF==11 = %2, expected 34")
                                 .arg(docLabel, QString::number(regionRowCount));
    QVERIFY2(regionRowCount == 34, qPrintable(countMsg));
  }
}

// --- Check 4: status enum (7 terms) -------------------------------------

void InventoryAuditTests::statusCellsUseSevenTermVocabulary()
{
  const QString canonical = loadInventory(QStringLiteral("canonical"), canonicalPath());
  QVERIFY2(!canonical.isEmpty(), qPrintable(QStringLiteral("canonical: missing %1").arg(canonicalPath())));
  const QString snapshot = loadInventory(QStringLiteral("snapshot"), snapshotPath());
  QVERIFY2(!snapshot.isEmpty(), qPrintable(QStringLiteral("snapshot: missing %1").arg(snapshotPath())));

  const QSet<QString> allowed = {
      QStringLiteral("Real"), QStringLiteral("Hybrid"), QStringLiteral("Mock"),
      QStringLiteral("Blocked"), QStringLiteral("Placeholder"), QStringLiteral("Superseded"),
      QStringLiteral("Missing")};

  // Phase 50 section 2 PASS distribution (frozen): Real 8, Hybrid 16, Mock 0,
  // Blocked 0, Placeholder 8, Superseded 0, Missing 2. Sum 34.
  const QMap<QString, int> expected = {
      {QStringLiteral("Real"), 8}, {QStringLiteral("Hybrid"), 16},
      {QStringLiteral("Mock"), 0}, {QStringLiteral("Blocked"), 0},
      {QStringLiteral("Placeholder"), 8}, {QStringLiteral("Superseded"), 0},
      {QStringLiteral("Missing"), 2}};

  for (const QString docLabel : {QStringLiteral("canonical"), QStringLiteral("snapshot")}) {
    const QString src = (docLabel == QLatin1String("canonical")) ? canonical : snapshot;
    QMap<QString, int> tally;
    for (const auto &allowedTerm : allowed)
      tally.insert(allowedTerm, 0);
    const QList<QStringList> rows = regionRows(src);
    for (const QStringList &cells : rows) {
      const QString status = cells.at(5); // 0-based index 5 == status
      const QString msg = QStringLiteral("%1: out-of-vocab status '%2'")
                              .arg(docLabel, status);
      QVERIFY2(allowed.contains(status), qPrintable(msg));
      tally[status]++;
    }
    for (const QString &term : expected.keys()) {
      const QString msg = QStringLiteral("%1: status '%2' tally=%3 expected %4")
                              .arg(docLabel, term, QString::number(tally.value(term)),
                                   QString::number(expected.value(term)));
      QVERIFY2(tally.value(term) == expected.value(term), qPrintable(msg));
    }
  }
}

// --- Check 5: verification enum (6 terms) -------------------------------

void InventoryAuditTests::verificationCellsUseSixTermVocabulary()
{
  const QString canonical = loadInventory(QStringLiteral("canonical"), canonicalPath());
  QVERIFY2(!canonical.isEmpty(), qPrintable(QStringLiteral("canonical: missing %1").arg(canonicalPath())));
  const QString snapshot = loadInventory(QStringLiteral("snapshot"), snapshotPath());
  QVERIFY2(!snapshot.isEmpty(), qPrintable(QStringLiteral("snapshot: missing %1").arg(snapshotPath())));

  const QSet<QString> allowed = {
      QStringLiteral("automated-test"), QStringLiteral("deterministic-harness"),
      QStringLiteral("manual-visual"), QStringLiteral("manual-uat-checklist"),
      QStringLiteral("build-only"), QStringLiteral("upstream-parity-audit")};

  // Phase 50 section 2 PASS distribution (frozen): manual-visual 28,
  // manual-uat-checklist 3, build-only 1, upstream-parity-audit 2,
  // automated-test 0, deterministic-harness 0. Sum 34.
  const QMap<QString, int> expected = {
      {QStringLiteral("automated-test"), 0}, {QStringLiteral("deterministic-harness"), 0},
      {QStringLiteral("manual-visual"), 28}, {QStringLiteral("manual-uat-checklist"), 3},
      {QStringLiteral("build-only"), 1}, {QStringLiteral("upstream-parity-audit"), 2}};

  for (const QString docLabel : {QStringLiteral("canonical"), QStringLiteral("snapshot")}) {
    const QString src = (docLabel == QLatin1String("canonical")) ? canonical : snapshot;
    QMap<QString, int> tally;
    for (const auto &allowedTerm : allowed)
      tally.insert(allowedTerm, 0);
    const QList<QStringList> rows = regionRows(src);
    for (const QStringList &cells : rows) {
      const QString v = cells.at(6); // 0-based index 6 == verification
      const QString msg = QStringLiteral("%1: out-of-vocab verification '%2'")
                              .arg(docLabel, v);
      QVERIFY2(allowed.contains(v), qPrintable(msg));
      tally[v]++;
    }
    for (const QString &term : expected.keys()) {
      const QString msg = QStringLiteral("%1: verification '%2' tally=%3 expected %4")
                              .arg(docLabel, term, QString::number(tally.value(term)),
                                   QString::number(expected.value(term)));
      QVERIFY2(tally.value(term) == expected.value(term), qPrintable(msg));
    }
  }
}

// --- Check 6: region ID regex + uniqueness ------------------------------

void InventoryAuditTests::regionIdsMatchAsciiSingleDashRegexAndAreUnique()
{
  const QString canonical = loadInventory(QStringLiteral("canonical"), canonicalPath());
  QVERIFY2(!canonical.isEmpty(), qPrintable(QStringLiteral("canonical: missing %1").arg(canonicalPath())));
  const QString snapshot = loadInventory(QStringLiteral("snapshot"), snapshotPath());
  QVERIFY2(!snapshot.isEmpty(), qPrintable(QStringLiteral("snapshot: missing %1").arg(snapshotPath())));

  const QRegularExpression re(QStringLiteral("^[A-Z]+-[A-Z0-9]+$"));

  for (const QString docLabel : {QStringLiteral("canonical"), QStringLiteral("snapshot")}) {
    const QString src = (docLabel == QLatin1String("canonical")) ? canonical : snapshot;
    const QList<QStringList> rows = regionRows(src);
    QVector<QString> ids;
    ids.reserve(rows.size());
    for (const QStringList &cells : rows) {
      const QString id = cells.at(0); // 0-based index 0 == region_id
      const QString reMsg = QStringLiteral("%1: region_id '%2' fails ^[A-Z]+-[A-Z0-9]+$")
                                .arg(docLabel, id);
      QVERIFY2(re.match(id).hasMatch(), qPrintable(reMsg));
      ids.append(id);
    }
    const QString countMsg = QStringLiteral("%1: total region IDs=%2, expected 34")
                                 .arg(docLabel, QString::number(ids.size()));
    QVERIFY2(ids.size() == 34, qPrintable(countMsg));

    const QSet<QString> unique(ids.begin(), ids.end());
    const QString uniqMsg = QStringLiteral("%1: %2 unique IDs, %3 total -> duplicates present")
                                .arg(docLabel, QString::number(unique.size()),
                                     QString::number(ids.size()));
    QVERIFY2(unique.size() == ids.size(), qPrintable(uniqMsg));
  }
}

// --- Check 7: INV-02/03/04 coverage anchors -----------------------------

void InventoryAuditTests::coverageAnchorsListAllRequiredClusterGlobs()
{
  const QString canonical = loadInventory(QStringLiteral("canonical"), canonicalPath());
  QVERIFY2(!canonical.isEmpty(), qPrintable(QStringLiteral("canonical: missing %1").arg(canonicalPath())));
  const QString snapshot = loadInventory(QStringLiteral("snapshot"), snapshotPath());
  QVERIFY2(!snapshot.isEmpty(), qPrintable(QStringLiteral("snapshot: missing %1").arg(snapshotPath())));

  const QStringList inv02 = {
      QStringLiteral("Plater."), QStringLiteral("GLCanvas3D."),
      QStringLiteral("GUI_ObjectList."), QStringLiteral("GUI_ObjectSettings."),
      QStringLiteral("Gizmos/")};
  const QStringList inv03 = {
      QStringLiteral("GUI_Preview."), QStringLiteral("GCodeViewer."),
      QStringLiteral("GLCanvas3D."), QStringLiteral("libslic3r/GCode/")};
  const QStringList inv04 = {
      QStringLiteral("Tab."), QStringLiteral("PresetComboBoxes."),
      QStringLiteral("ConfigManipulation."), QStringLiteral("UnsavedChangesDialog."),
      QStringLiteral("CreatePresetsDialog."), QStringLiteral("PrintConfig."),
      QStringLiteral("Preset."), QStringLiteral("PresetBundle.")};

  // Count anchor comment lines (not prose mentions in backticks). The
  // snapshot's section 2 verification prose records the check results
  // verbatim, including inline mentions of the anchor marker text, which
  // would inflate a naive QString::count. We count lines whose trimmed form
  // starts with the marker -- only real anchor comments begin a line with
  // `<!-- INV-XX coverage`.
  auto countAnchors = [](const QString &src, const QString &marker) {
    int count = 0;
    const QStringList lines = src.split(QLatin1Char('\n'));
    for (QString line : lines) {
      while (!line.isEmpty() && (line.at(0) == QLatin1Char(' ') || line.at(0) == QLatin1Char('\t')))
        line.remove(0, 1);
      if (line.startsWith(marker))
        ++count;
    }
    return count;
  };

  for (const QString docLabel : {QStringLiteral("canonical"), QStringLiteral("snapshot")}) {
    const QString src = (docLabel == QLatin1String("canonical")) ? canonical : snapshot;

    const int inv02Count = countAnchors(src, QStringLiteral("<!-- INV-02 coverage"));
    const QString inv02CountMsg = QStringLiteral("%1: INV-02 anchors=%2, expected 1")
                                      .arg(docLabel, QString::number(inv02Count));
    QVERIFY2(inv02Count == 1, qPrintable(inv02CountMsg));

    const int inv03Count = countAnchors(src, QStringLiteral("<!-- INV-03 coverage"));
    const QString inv03CountMsg = QStringLiteral("%1: INV-03 anchors=%2, expected 1")
                                      .arg(docLabel, QString::number(inv03Count));
    QVERIFY2(inv03Count == 1, qPrintable(inv03CountMsg));

    const int inv04Count = countAnchors(src, QStringLiteral("<!-- INV-04 coverage"));
    const QString inv04CountMsg = QStringLiteral("%1: INV-04 anchors=%2, expected 2")
                                      .arg(docLabel, QString::number(inv04Count));
    QVERIFY2(inv04Count == 2, qPrintable(inv04CountMsg));

    // Anchor presence is checked via count above. Now verify each required
    // cluster glob string appears at least once anywhere in the doc (the
    // canonical prose names each glob in section 1 and repeats them in the
    // coverage anchors, so a missing glob from the doc is a regression).
    for (const QString &glob : inv02 + inv03 + inv04) {
      const QString msg = QStringLiteral("%1: missing required cluster glob '%2'")
                              .arg(docLabel, glob);
      QVERIFY2(src.contains(glob), qPrintable(msg));
    }
  }
}

// --- Check 8: cleanup format (6-tag vocabulary, format-only) -----------
//
// Phase 50 section 2 check 8 had two halves:
//   (a) cleanup cells in region tables use only the 6-tag vocabulary, and
//   (b) every `file:` path listed in the section 7 aggregate cleanup
//       checklist exists on disk.
// Half (b) was captured at Phase 50 close (before Phase 57 deleted the four
// Settings files: SettingsPage.qml, ConfigPage.qml, ParamsPage.qml,
// SearchDialog.qml). After Phase 57 the post-removal invariant is the
// OPPOSITE: those four paths must STAY ABSENT. That regression is permanently
// guarded by QmlUiAuditTests::deletedSettingsPathsStayAbsent and
// QmlUiAuditTests::deletedRoutesStayAbsent. Re-asserting disk-existence here
// would invert the Phase 57 regression guard. This test therefore asserts
// only half (a) -- the 6-tag vocabulary -- and refers the reader to
// QmlUiAuditTests for the file-presence half.

void InventoryAuditTests::cleanupFormatUsesOnlySixAllowedTags()
{
  const QString canonical = loadInventory(QStringLiteral("canonical"), canonicalPath());
  QVERIFY2(!canonical.isEmpty(), qPrintable(QStringLiteral("canonical: missing %1").arg(canonicalPath())));
  const QString snapshot = loadInventory(QStringLiteral("snapshot"), snapshotPath());
  QVERIFY2(!snapshot.isEmpty(), qPrintable(QStringLiteral("snapshot: missing %1").arg(snapshotPath())));

  const QStringList allowedTags = {
      QStringLiteral("file:"), QStringLiteral("qrc:"), QStringLiteral("route:"),
      QStringLiteral("import:"), QStringLiteral("test:"), QStringLiteral("doc:")};
  const QSet<QString> allowedNa = {
      QStringLiteral("n/a"), QStringLiteral("n/a (no prior Qt target)")};

  auto cellConforms = [&](const QString &cell) -> bool {
    if (allowedNa.contains(cell))
      return true;
    // Empty cleanup cell is not allowed.
    if (cell.isEmpty())
      return false;
    // Split on whitespace; every non-empty token must start with one of the
    // 6 allowed tags. Region-table cleanup cells are all `n/a` per the §2
    // PASS record, so this branch is effectively never exercised on a region
    // row -- but the format lock is asserted here for completeness.
    const QStringList tokens = cell.split(QRegularExpression(QStringLiteral("\\s+")),
                                          Qt::SkipEmptyParts);
    if (tokens.isEmpty())
      return false;
    for (const QString &tok : tokens) {
      bool ok = false;
      for (const QString &tag : allowedTags) {
        if (tok.startsWith(tag)) {
          ok = true;
          break;
        }
      }
      if (!ok)
        return false;
    }
    return true;
  };

  for (const QString docLabel : {QStringLiteral("canonical"), QStringLiteral("snapshot")}) {
    const QString src = (docLabel == QLatin1String("canonical")) ? canonical : snapshot;
    const QList<QStringList> rows = regionRows(src);
    for (const QStringList &cells : rows) {
      const QString cleanup = cells.at(8); // 0-based index 8 == cleanup
      const QString msg = QStringLiteral("%1: cleanup cell '%2' not in 6-tag vocab / n/a")
                              .arg(docLabel, cleanup);
      QVERIFY2(cellConforms(cleanup), qPrintable(msg));
    }

    // Section 7 aggregate cleanup checklist format lock: every line in the
    // fenced ```text ... ``` block (the actual cleanup checklist) that begins
    // with a known cleanup tag must use one of the 6 allowed tags. We scan
    // only lines inside a fenced code block (delimited by ``` on their own
    // line) so that prose mentions of "tags:" or "tag-1|tag-2" elsewhere in
    // the doc do not trip the format lock. This catches a future typo like
    // `path:` or `remove:` inside the checklist while ignoring prose.
    const QStringList lines = src.split(QLatin1Char('\n'));
    bool inFence = false;
    for (const QString &raw : lines) {
      const QString trimmed = raw.trimmed();
      if (trimmed == QStringLiteral("```text") || trimmed == QStringLiteral("```")) {
        if (trimmed == QStringLiteral("```"))
          inFence = !inFence;
        else
          inFence = true;
        continue;
      }
      if (!inFence)
        continue;
      if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#')))
        continue;
      const int colon = trimmed.indexOf(QLatin1Char(':'));
      if (colon <= 0)
        continue;
      const QString firstToken = trimmed.left(colon + 1);
      // Only enforce the 6-tag set on lines that match the cleanup-tag
      // pattern (lowercase letters then ':').
      static const QRegularExpression tagShape(QStringLiteral("^[a-z]+:$"));
      if (!tagShape.match(firstToken).hasMatch())
        continue;
      const QString msg = QStringLiteral("%1: cleanup-tag line '%2' not in 6-tag vocab")
                              .arg(docLabel, firstToken);
      QVERIFY2(allowedTags.contains(firstToken), qPrintable(msg));
    }
  }
}

// --- Check 9: no blank upstream_source cells ---------------------------

void InventoryAuditTests::noBlankUpstreamSourceCells()
{
  const QString canonical = loadInventory(QStringLiteral("canonical"), canonicalPath());
  QVERIFY2(!canonical.isEmpty(), qPrintable(QStringLiteral("canonical: missing %1").arg(canonicalPath())));
  const QString snapshot = loadInventory(QStringLiteral("snapshot"), snapshotPath());
  QVERIFY2(!snapshot.isEmpty(), qPrintable(QStringLiteral("snapshot: missing %1").arg(snapshotPath())));

  for (const QString docLabel : {QStringLiteral("canonical"), QStringLiteral("snapshot")}) {
    const QString src = (docLabel == QLatin1String("canonical")) ? canonical : snapshot;
    const QList<QStringList> rows = regionRows(src);
    for (const QStringList &cells : rows) {
      const QString upstream = cells.at(4).trimmed(); // 0-based index 4 == upstream_source
      const QString msg = QStringLiteral("%1: blank upstream_source cell")
                              .arg(docLabel);
      QVERIFY2(!upstream.isEmpty(), qPrintable(msg));
    }
  }
}

// --- Phase 57 cleanup regression cross-reference (no-op assertion) -----

void InventoryAuditTests::phase57CleanupRegressionRemainsLockedByQmlUiAudit()
{
  // The Phase 57 cleanup removed:
  //   - 4 Settings files: SettingsPage.qml, ConfigPage.qml, ParamsPage.qml,
  //     SearchDialog.qml
  //   - 3 qrc entries for the same
  //   - 3 BackendContext/ConfigViewModel leave-page routes
  //   - 2 imports of the deleted components
  // The permanent regression guard for "these stay ABSENT" lives in
  // QmlUiAuditTests::deletedSettingsPathsStayAbsent and
  // QmlUiAuditTests::deletedRoutesStayAbsent. This slot only records the
  // cross-reference so a reader running `ctest -R Inventory` finds it.
  const QString pointer = QStringLiteral(
      "Phase 57 cleanup regression is guarded by QmlUiAuditTests::"
      "deletedSettingsPathsStayAbsent and QmlUiAuditTests::deletedRoutesStayAbsent "
      "(the post-removal invariant is the 4 files + 3 routes must STAY ABSENT, "
      "which supersedes the Phase 50 section 2 check 8 file-existence half).");
  QVERIFY2(true, qPrintable(pointer));
}

#include "InventoryAuditTests.moc"
QTEST_MAIN(InventoryAuditTests)
