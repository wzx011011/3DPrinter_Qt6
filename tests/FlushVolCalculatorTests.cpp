// FlushVolCalculatorTests - pure-math tests for the X3 faithful port of
// upstream FlushVolCalc + FlushVolPredictor (libslic3r).
//
// Expected values come from the embedded upstream datasets
// (third_party/OrcaSlicer/resources/flush/*.txt rows) and hand-derived
// formula results, never from calling the same code path twice.
//
// AUTOMOC caveat (single-file QtTest with cpp-internal Q_OBJECT, see
// tests/GizmoMathTests.cpp:1-12): after editing private slots here, re-run
// cmake configure (the canonical verify script does this automatically).

#include <QtTest>
#include <QSize>

#include "core/FlushVolCalculator.h"

class FlushVolCalculatorTests final : public QObject
{
  Q_OBJECT

private slots:
  void standardDatasetPredictsKnownPair();
  void dualDatasetsPredictKnownPairs();
  void missingDualPairFallsBackToFormulaWithDarkToLightCorrection();
  void standardDatasetNeverAppliesCorrection();
  void defaultMaxClampIsUpstream20000();
  void minFlushAddsBeforeClamp();
};

void FlushVolCalculatorTests::standardDatasetPredictsKnownPair()
{
  // Dataset 0 (default) routes the prediction through calcFlushVolRgb
  // (upstream FlushVolCalc.cpp:62): the standard dataset row
  // "#000000 #545454 236" returns verbatim (unclamped, min added later).
  const OWzx::FlushVolCalculator calc(/*min*/ 0, /*max*/ OWzx::g_max_flush_volume,
                                      /*flush_dataset*/ 0);
  QCOMPARE(calc.calcFlushVol(255, 0x00, 0x00, 0x00, 255, 0x54, 0x54, 0x54), 236);

  // The raw accessor agrees.
  float flush = 0.0f;
  QVERIFY(calc.getFlushVolFromData(0x00, 0x00, 0x00, 0x54, 0x54, 0x54, flush));
  QCOMPARE(flush, 236.0f);
}

void FlushVolCalculatorTests::dualDatasetsPredictKnownPairs()
{
  // Dataset != 0 predicts inside calcFlushVol and clamps to m_max
  // (FlushVolCalc.cpp:114-115). Dual-standard row "#000000 #545454 450",
  // dual-highflow row 270.
  const OWzx::FlushVolCalculator dualStandard(/*min*/ 0,
                                              /*max*/ OWzx::g_max_flush_volume,
                                              /*flush_dataset*/ 1);
  QCOMPARE(dualStandard.calcFlushVol(255, 0x00, 0x00, 0x00, 255, 0x54, 0x54, 0x54), 450);
  const OWzx::FlushVolCalculator dualHighflow(/*min*/ 0,
                                              /*max*/ OWzx::g_max_flush_volume,
                                              /*flush_dataset*/ 2);
  QCOMPARE(dualHighflow.calcFlushVol(255, 0x00, 0x00, 0x00, 255, 0x54, 0x54, 0x54), 270);
}

void FlushVolCalculatorTests::
    missingDualPairFallsBackToFormulaWithDarkToLightCorrection()
{
  // White -> #482960: both colours are in the dual-standard palette but the
  // row "#FFFFFF #482960" does not exist, so prediction misses and the HSV
  // formula runs. The formula value is multiplied by 1.3 for a bright source
  // into a dark destination (upstream FlushVolCalc.cpp:120-125), which only
  // ever happens with a dual dataset.
  const OWzx::FlushVolCalculator dualStandard(/*min*/ 0,
                                              /*max*/ OWzx::g_max_flush_volume,
                                              /*flush_dataset*/ 1);
  // With dataset 1, calcFlushVolRgb skips prediction entirely (upstream
  // FlushVolCalc.cpp:62) -> pure formula base.
  const int base = dualStandard.calcFlushVolRgb(255, 255, 255, 0x48, 0x29, 0x60);
  QVERIFY2(base > 0, "formula base must be positive");
  const int expected = std::min(static_cast<int>(static_cast<float>(base) * 1.3f),
                                OWzx::g_max_flush_volume);
  QCOMPARE(dualStandard.calcFlushVol(255, 255, 255, 255, 255, 0x48, 0x29, 0x60),
           expected);
}

void FlushVolCalculatorTests::standardDatasetNeverAppliesCorrection()
{
  // White -> black with dataset 0: #FFFFFF is not in the standard palette so
  // prediction misses; the formula path must NOT apply the x1.3 correction
  // (upstream guards it with m_flush_dataset != 0). Hand-derived: white and
  // black both have HSV saturation 0, so hs_dist = 0 and the darker-
  // destination branch gives lumi_flush = (1-0)*80 = 80 with inter_v = 0.33
  // clamping hs_adj to 0; triangle third edge = sqrt(0 + 80^2 - 0) = 80.
  const OWzx::FlushVolCalculator calc(/*min*/ 0, /*max*/ OWzx::g_max_flush_volume,
                                      /*flush_dataset*/ 0);
  QCOMPARE(calc.calcFlushVol(255, 255, 255, 255, 255, 0x00, 0x00, 0x00), 80);
}

void FlushVolCalculatorTests::defaultMaxClampIsUpstream20000()
{
  // g_max_flush_volume = 20000 (FlushVolCalc.cpp:13) is the DEFAULT clamp:
  // min padding of 19990 plus the 80 white->black formula value would land
  // at 20070 and must clamp to 20000.
  const OWzx::FlushVolCalculator calc(/*min*/ 19990);
  QCOMPARE(calc.calcFlushVol(255, 255, 255, 255, 255, 0x00, 0x00, 0x00), 20000);
}

void FlushVolCalculatorTests::minFlushAddsBeforeClamp()
{
  // Upstream order (FlushVolCalc.cpp:127-128): += m_min_flush_vol, then the
  // m_max clamp.
  const OWzx::FlushVolCalculator calc(/*min*/ 15, /*max*/ OWzx::g_max_flush_volume,
                                      /*flush_dataset*/ 0);
  QCOMPARE(calc.calcFlushVol(255, 255, 255, 255, 255, 0x00, 0x00, 0x00), 95);
}

QTEST_MAIN(FlushVolCalculatorTests)
#include "FlushVolCalculatorTests.moc"
