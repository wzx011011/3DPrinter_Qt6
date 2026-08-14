#pragma once

// ===========================================================================
// FlushVolCalculator — port of upstream FlushVolCalc (libslic3r).
// Computes the wiping/flushing volume between two filament colours using the
// upstream BBS colour-distance formula (RGB→HSV → DeltaHS_BBS + luminance →
// triangle 3rd edge). Pure math, no libslic3r dependency.
//
// Upstream: third_party/OrcaSlicer/src/libslic3r/FlushVolCalc.{hpp,cpp}
// ===========================================================================

#include <cmath>

namespace OWzx {

class FlushVolCalculator
{
public:
  FlushVolCalculator(int minFlush = 0, int maxFlush = 800, float multiplier = 1.0f)
      : m_min(minFlush), m_max(maxFlush), m_multiplier(multiplier) {}

  /// Calculate the flush volume from src colour to dst colour.
  /// Colours are 0-255 RGB (a=alpha, 0 means transparent→white).
  /// Returns the flush volume in mm³, clamped to [min, max] after multiplier.
  int calcFlushVol(unsigned char srcA, unsigned char srcR, unsigned char srcG, unsigned char srcB,
                   unsigned char dstA, unsigned char dstR, unsigned char dstG, unsigned char dstB) const
  {
    // Transparent → white (upstream convention).
    if (srcA == 0) { srcR = srcG = srcB = 255; }
    if (dstA == 0) { dstR = dstG = dstB = 255; }

    const float sr = srcR / 255.0f, sg = srcG / 255.0f, sb = srcB / 255.0f;
    const float dr = dstR / 255.0f, dg = dstG / 255.0f, db = dstB / 255.0f;

    float fh, fs, fv, th, ts, tv;
    rgb2hsv(sr, sg, sb, fh, fs, fv);
    rgb2hsv(dr, dg, db, th, ts, tv);

    const float hsDist = deltaHS(fh, fs, fv, th, ts, tv);
    const float fromLumi = luminance(sr, sg, sb);
    const float toLumi = luminance(dr, dg, db);

    float lumiFlush = 0.0f;
    float hsDistAdj = hsDist;
    if (toLumi >= fromLumi) {
      lumiFlush = std::pow(toLumi - fromLumi, 0.7f) * 560.0f;
    } else {
      lumiFlush = (fromLumi - toLumi) * 80.0f;
      const float interV = 0.67f * tv + 0.33f * fv;
      hsDistAdj = std::min(interV, hsDist);
    }
    const float hsFlush = 230.0f * hsDistAdj;
    float vol = triangle3rdEdge(hsFlush, lumiFlush, 120.0f);
    vol = std::max(vol, 60.0f);
    vol += float(m_min);
    vol *= m_multiplier;
    return std::min(int(vol), m_max);
  }

private:
  int m_min;
  int m_max;
  float m_multiplier;

  static float toRad(float deg) { return deg / 180.0f * float(M_PI); }
  static float luminance(float r, float g, float b) { return r * 0.3f + g * 0.59f + b * 0.11f; }
  static float triangle3rdEdge(float a, float b, float degAB)
  {
    return std::sqrt(a * a + b * b - 2.0f * a * b * std::cos(toRad(degAB)));
  }
  static float deltaHS(float h1, float s1, float v1, float h2, float s2, float v2)
  {
    const float h1r = toRad(h1), h2r = toRad(h2);
    const float dx = std::cos(h1r) * s1 * v1 - std::cos(h2r) * s2 * v2;
    const float dy = std::sin(h1r) * s1 * v1 - std::sin(h2r) * s2 * v2;
    return std::min(1.2f, std::sqrt(dx * dx + dy * dy));
  }
  static void rgb2hsv(float r, float g, float b, float &h, float &s, float &v)
  {
    // Standard RGB→HSV (port of upstream ColorSpaceConvert RGB2HSV).
    float mx = std::max(std::max(r, g), b);
    float mn = std::min(std::min(r, g), b);
    v = mx;
    const float delta = mx - mn;
    s = (mx > 0.0f) ? (delta / mx) : 0.0f;
    if (delta < 1e-6f) { h = 0.0f; return; }
    if (mx == r) {
      h = 60.0f * ((g - b) / delta);
    } else if (mx == g) {
      h = 60.0f * (2.0f + (b - r) / delta);
    } else {
      h = 60.0f * (4.0f + (r - g) / delta);
    }
    if (h < 0.0f) h += 360.0f;
  }
};

} // namespace OWzx
