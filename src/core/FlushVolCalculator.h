#pragma once

// ===========================================================================
// FlushVolCalculator — faithful port of upstream FlushVolCalc (libslic3r).
//
// X3 (2026-10-14): the v5.12 partial copy (colour-distance formula only, a
// `multiplier` parameter that has no upstream counterpart, and an 800 clamp)
// is replaced by the full upstream class:
//   - constructor (min, max, flush_dataset)   (FlushVolCalc.hpp:18)
//   - calc_flush_vol / calc_flush_vol_rgb / get_flush_vol_from_data
//                                             (FlushVolCalc.cpp:43-129)
//   - g_max_flush_volume = 20000              (FlushVolCalc.cpp:13; Orca
//     raised the clamp from the BBS 900 to guard extreme UI-entered values)
//   - the flush_dataset data-driven predictor  (FlushVolPredictor.{hpp,cpp}:
//     RGB -> LAB, DeltaE2000 similar-colour match (threshold 5), 6-byte RGB
//     pair hash lookup)
//   - the dark->light x1.3 flush correction for dataset != 0
//                                             (FlushVolCalc.cpp:120-125)
//
// Upstream behavioural notes kept verbatim (do not "fix" them here):
//   * calc_flush_vol tries the predictor only when m_flush_dataset != 0 and
//     returns the predicted volume clamped to m_max; calc_flush_vol_rgb tries
//     it only when m_flush_dataset == 0 (the standard dataset) and returns
//     the predicted volume UNCLAMPED (upstream FlushVolCalc.cpp:62-63).
//   * the x1.3 correction condition reads, verbatim,
//     `is_from_dark = luminance(src) > 180/255` and
//     `is_to_light = luminance(dst) < 75/255` (FlushVolCalc.cpp:120-124) —
//     the upstream variable names are inverted relative to their comparisons
//     (a bright source into a dark destination), the behaviour is what ships.
//
// Dataset data: the three upstream resources/flush/*.txt files are embedded
// below byte-verbatim (CRLF normalised to LF, matching what upstream's
// text-mode ifstream sees) and parsed by the ported parser, so the pure-math
// header stays self-contained with no Qt/libslic3r/file-system dependency.
// Data source of truth: third_party/OrcaSlicer/resources/flush/.
// Pure math, no libslic3r dependency.
// ===========================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace OWzx {

// Upstream global clamp (FlushVolCalc.cpp:13).
inline constexpr int g_max_flush_volume = 20000;

namespace detail {
inline constexpr double kTwoPi = 6.283185307179586476925286766559;
inline constexpr double kPi = 3.141592653589793238462643383279;
}  // namespace detail

// ===========================================================================
// FlushPredict — colour-space helpers (FlushVolPredictor.{hpp,cpp}).
// ===========================================================================
namespace FlushPredict {

struct RGBColor
{
  unsigned char r{0};
  unsigned char g{0};
  unsigned char b{0};
  RGBColor(unsigned char r_, unsigned char g_, unsigned char b_) : r(r_), g(g_), b(b_) {}
  RGBColor() = default;
};

struct LABColor
{
  double l{0};
  double a{0};
  double b{0};
  LABColor() = default;
  LABColor(double l_, double a_, double b_) : l(l_), a(a_), b(b_) {}
};

// Transfer colour in RGB space to LAB space (FlushVolPredictor.cpp:18-61).
inline LABColor RGB2LAB(const RGBColor &color)
{
  using XYZColor = std::tuple<double, double, double>;
  auto gamma = [](double x) {
    if (x > 0.04045)
      return pow((x + 0.055) / 1.055, 2.4);
    else
      return x / 12.92;
  };
  auto RGB2XYZ = [gamma](const RGBColor &color) -> XYZColor {
    double R = gamma(static_cast<double>(color.r) / 255.0) * 100;
    double G = gamma(static_cast<double>(color.g) / 255.0) * 100;
    double B = gamma(static_cast<double>(color.b) / 255.0) * 100;

    double x = 0.412453 * R + 0.357580 * G + 0.180423 * B;
    double y = 0.212671 * R + 0.715160 * G + 0.072169 * B;
    double z = 0.019334 * R + 0.119193 * G + 0.950227 * B;
    return {x, y, z};
  };

  static const double XN = 95.0489;
  static const double YN = 100;
  static const double ZN = 108.8840;

  auto f = [](double t) {
    static const double threshold = 0.008856;
    if (t > threshold)
      return pow(t, 1.0 / 3.0);
    else
      return 7.787 * t + 0.137931;
  };

  auto xyz_color = RGB2XYZ(color);
  double x = std::get<0>(xyz_color);
  double y = std::get<1>(xyz_color);
  double z = std::get<2>(xyz_color);
  double xn = f(x / XN);
  double yn = f(y / YN);
  double zn = f(z / ZN);

  double L = 116.0 * yn - 16.0;
  double A = 500.0 * (xn - yn);
  double B = 200.0 * (yn - zn);
  return LABColor(L, A, B);
}

// DeltaE2000 (FlushVolPredictor.cpp:63-151).
inline float calc_color_distance(const LABColor &lab1, const LABColor &lab2)
{
  static const double pow_25_to_7 = pow(25, 7);

  const double C1 = sqrt(lab1.a * lab1.a + lab1.b * lab1.b);
  const double C2 = sqrt(lab2.a * lab2.a + lab2.b * lab2.b);
  const double CMean = (C1 + C2) / 2.0;
  const double pow_CMean_to_7 = pow(CMean, 7);
  const double G = 0.5 * (1 - sqrt(pow_CMean_to_7 / (pow_CMean_to_7 + pow_25_to_7)));

  const double p_l1 = lab1.l;
  const double p_l2 = lab2.l;
  const double p_a1 = (1. + G) * lab1.a;
  const double p_a2 = (1. + G) * lab2.a;
  const double p_b1 = lab1.b;
  const double p_b2 = lab2.b;
  const double p_c1 = sqrt(p_a1 * p_a1 + p_b1 * p_b1);
  const double p_c2 = sqrt(p_a2 * p_a2 + p_b2 * p_b2);
  double p_h1;
  if (p_a1 == 0 && p_b1 == 0)
    p_h1 = 0;
  else {
    p_h1 = atan2(p_b1, p_a1);
    if (p_h1 < 0)
      p_h1 += detail::kTwoPi;
  }
  double p_h2;
  if (p_a2 == 0 && p_b2 == 0)
    p_h2 = 0;
  else {
    p_h2 = atan2(p_b2, p_a2);
    if (p_h2 < 0)
      p_h2 += detail::kTwoPi;
  }

  const double delta_L = p_l2 - p_l1;
  const double delta_C = p_c2 - p_c1;

  double delta_H;
  const double p_c_multi = p_c1 * p_c2;
  if (p_c_multi == 0)
    delta_H = 0;
  else {
    delta_H = p_h2 - p_h1;
    if (delta_H < -detail::kPi)
      delta_H += 2 * detail::kPi;
    else if (delta_H > detail::kPi)
      delta_H -= 2 * detail::kPi;
    delta_H = 2 * sqrt(p_c_multi) * sin(delta_H / 2.);
  }

  double p_L_mean = (p_l1 + p_l2) / 2.0;
  double p_C_mean = (p_c1 + p_c2) / 2.0;

  double p_H_mean, p_H_sum = p_h1 + p_h2;
  if (p_c1 * p_c2 == 0) {
    p_H_mean = p_H_sum;
  } else {
    if (fabs(p_h1 - p_h2) <= detail::kPi)
      p_H_mean = p_H_sum / 2;
    else {
      if (p_H_sum < 2 * detail::kPi)
        p_H_mean = (p_H_sum + 2 * detail::kPi) / 2.0;
      else
        p_H_mean = (p_H_sum - 2 * detail::kPi) / 2.0;
    }
  }

  const double T = 1 - 0.17 * cos(p_H_mean - (30.0 * detail::kPi / 180.0)) +
                   0.24 * cos(2 * p_H_mean) + 0.32 * cos(3 * p_H_mean + (6.0 * detail::kPi / 180.0)) -
                   0.2 * cos(4 * p_H_mean - (63.0 * detail::kPi / 180.0));
  const double dtheta = (30.0 * detail::kPi / 180.0) * exp(-pow((p_H_mean - (275.0 * detail::kPi / 180.0)) / (25.0 * detail::kPi / 180.0), 2));

  const double pow_p_cmean_to_7 = pow(p_C_mean, 7);
  const double R_C = 2 * sqrt(pow_p_cmean_to_7 / (pow_p_cmean_to_7 + pow_25_to_7));

  const double pow_p_lmean_to_2 = pow(p_L_mean - 50, 2);
  const double S_L = 1 + (0.015 * pow_p_lmean_to_2) / sqrt(20 + pow_p_lmean_to_2);
  const double S_C = 1 + 0.045 * p_C_mean;
  const double S_H = 1 + 0.015 * p_C_mean * T;
  const double R_T = -sin(2 * dtheta) * R_C;

  const double K_L = 1.0, K_C = 1.0, K_H = 1.0;

  double de = sqrt(
      pow(delta_L / (K_L * S_L), 2) + pow(delta_C / (K_C * S_C), 2) + pow(delta_H / (K_H * S_H), 2) + (R_T * (delta_C / (K_C * S_C)) * (delta_H / (K_H * S_H))));
  return static_cast<float>(de);
}

inline float calc_color_distance(const RGBColor &color1, const RGBColor &color2)
{
  LABColor lab1 = RGB2LAB(color1);
  LABColor lab2 = RGB2LAB(color2);
  return calc_color_distance(lab1, lab2);
}

// DeltaE within the threshold -> same colour (FlushVolPredictor.cpp:159-165).
inline bool is_similar_color(const RGBColor &from, const RGBColor &to, float distance_threshold = 5.0)
{
  float color_distance = calc_color_distance(from, to);
  if (color_distance > distance_threshold)
    return false;
  return true;
}

}  // namespace FlushPredict

// ===========================================================================
// FlushVolPredictor — data-driven prediction over an upstream flush dataset
// (FlushVolPredictor.cpp:170-307). The dataset arrives as the verbatim
// resources/flush/*.txt content; the parser is the upstream one.
// ===========================================================================
class FlushVolPredictor
{
  using RGB = FlushPredict::RGBColor;

public:
  FlushVolPredictor() = default;
  FlushVolPredictor(const char *data_text)
  {
    parse(data_text);
  }

  bool predict(const RGB &from, const RGB &to, float &flush) const
  {
    if (!m_valid)
      return false;

    // find similar colors in color list
    std::optional<RGB> similar_from, similar_to;
    for (const auto &color : m_colors) {
      if (FlushPredict::is_similar_color(color, from)) {
        similar_from = color;
        break;
      }
    }
    for (const auto &color : m_colors) {
      if (FlushPredict::is_similar_color(color, to)) {
        similar_to = color;
        break;
      }
    }

    // `from` and `to` should have similar colors in list
    if (!similar_from || !similar_to)
      return false;

    uint64_t key = generate_hash_key(*similar_from, *similar_to);
    auto iter = m_flush_map.find(key);
    if (iter == m_flush_map.end())
      return false;

    flush = iter->second;
    return true;
  }

  int get_min_flush_volume() const
  {
    if (!m_valid)
      return std::numeric_limits<int>::max();
    return static_cast<int>(
        std::min_element(m_flush_map.begin(), m_flush_map.end(),
                         [](const auto &a, const auto &b) { return a.second < b.second; })
            ->second);
  }

private:
  void parse(const char *data_text)
  {
    auto rgb_hex_to_dec = [](const std::string &hexstr, FlushPredict::RGBColor &color) -> bool {
      if (hexstr.empty() || hexstr.length() != 7 || hexstr[0] != '#') {
        color.r = 0, color.g = 0, color.b = 0;
        return false;
      }

      auto hexToByte = [](const std::string &hex) -> int {
        unsigned int byte = 0;
        std::istringstream(hex) >> std::hex >> byte;
        return byte;
      };
      color.r = static_cast<unsigned char>(hexToByte(hexstr.substr(1, 2)));
      color.g = static_cast<unsigned char>(hexToByte(hexstr.substr(3, 2)));
      color.b = static_cast<unsigned char>(hexToByte(hexstr.substr(5, 2)));
      return true;
    };

    std::istringstream in(data_text);
    std::string line;
    std::getline(in, line);  // skip color description line
    if (!std::getline(in, line)) {
      m_valid = false;
      return;
    }
    // read and save color lists
    {
      std::istringstream in(line);
      std::string color;
      while (in >> color) {
        RGB c;
        if (!rgb_hex_to_dec(color, c)) {
          m_valid = false;
          return;
        }
        m_colors.emplace_back(c);
      }
    }
    std::getline(in, line);  // skip column name line
    while (std::getline(in, line)) {
      if (line.empty())
        continue;  // trailing newline of the embedded literal
      std::istringstream iss(line);
      std::string rgb_from, rgb_to;
      float value;
      if (iss >> rgb_from >> rgb_to >> value) {
        RGB from, to;
        // transfer hex str to rgb format
        if (!rgb_hex_to_dec(rgb_from, from)) {
          m_valid = false;
          return;
        }
        if (!rgb_hex_to_dec(rgb_to, to)) {
          m_valid = false;
          return;
        }
        // generate hash key for two rgb color
        uint64_t key = generate_hash_key(from, to);
        m_flush_map.emplace(key, value);
      } else {
        m_valid = false;
        return;
      }
    }
    m_valid = true;
  }

  static uint64_t generate_hash_key(const RGB &from, const RGB &to)
  {
    uint64_t key = 0;
    key |= (static_cast<uint64_t>(from.r) << 40);
    key |= (static_cast<uint64_t>(from.g) << 32);
    key |= (static_cast<uint64_t>(from.b) << 24);
    key |= (static_cast<uint64_t>(to.r) << 16);
    key |= (static_cast<uint64_t>(to.g) << 8);
    key |= static_cast<uint64_t>(to.b);
    return key;
  }

  std::unordered_map<uint64_t, float> m_flush_map;
  std::vector<RGB> m_colors;
  bool m_valid{false};
};

// ===========================================================================
// GenericFlushPredictor — per-dataset predictor instances with the upstream
// static cache (FlushVolPredictor.cpp:310-329). Dataset value -> dataset:
//   0 = flush_data_standard.txt (the PrintConfig "nozzle_flush_dataset"
//       default, PrintConfig.cpp:2969)
//   1 = flush_data_dual_standard.txt
//   2 = flush_data_dual_highflow.txt
// Upstream resolves resources_dir() + "/flush/<file>"; here the same file
// contents are embedded below, so the mapping (not a file path) selects.
// ===========================================================================
namespace detail {

// Embedded upstream datasets — byte-verbatim copies of
// third_party/OrcaSlicer/resources/flush/*.txt (CRLF normalised to LF).
inline constexpr char kFlushDataStandard[] = R"FLUSH(colors
#000000 #C12E1F #00AE42 #545454 #D1D3D5 #5B6579 #F4EE2A #9D432C #5E43B7 #0A2989 #FF6A13 #8E9089
src dst flush
#000000 #545454 236
#000000 #F4EE2A 386
#000000 #5E43B7 266
#000000 #0A2989 236
#C12E1F #000000 26
#C12E1F #00AE42 326
#C12E1F #F4EE2A 356
#C12E1F #FF6A13 146
#00AE42 #000000 26
#00AE42 #C12E1F 146
#00AE42 #545454 56
#00AE42 #D1D3D5 266
#00AE42 #F4EE2A 176
#00AE42 #9D432C 86
#00AE42 #FF6A13 206
#00AE42 #8E9089 206
#545454 #C12E1F 146
#545454 #00AE42 116
#545454 #D1D3D5 176
#545454 #5B6579 86
#545454 #F4EE2A 206
#545454 #9D432C 86
#545454 #5E43B7 56
#545454 #FF6A13 236
#545454 #8E9089 56
#D1D3D5 #C12E1F 116
#D1D3D5 #00AE42 116
#D1D3D5 #F4EE2A 56
#D1D3D5 #9D432C 86
#D1D3D5 #5E43B7 86
#D1D3D5 #FF6A13 86
#5B6579 #000000 26
#5B6579 #C12E1F 56
#5B6579 #00AE42 26
#5B6579 #545454 26
#5B6579 #D1D3D5 56
#5B6579 #F4EE2A 116
#5B6579 #9D432C 56
#5B6579 #5E43B7 26
#5B6579 #0A2989 26
#5B6579 #FF6A13 116
#5B6579 #8E9089 26
#F4EE2A #000000 56
#F4EE2A #C12E1F 26
#F4EE2A #00AE42 86
#F4EE2A #545454 146
#F4EE2A #9D432C 86
#F4EE2A #5E43B7 146
#F4EE2A #FF6A13 26
#9D432C #000000 56
#9D432C #C12E1F 56
#9D432C #00AE42 176
#9D432C #545454 86
#9D432C #D1D3D5 236
#9D432C #F4EE2A 206
#9D432C #5E43B7 116
#9D432C #0A2989 86
#9D432C #FF6A13 116
#9D432C #8E9089 146
#5E43B7 #C12E1F 176
#5E43B7 #00AE42 116
#5E43B7 #545454 56
#5E43B7 #D1D3D5 206
#5E43B7 #5B6579 116
#5E43B7 #F4EE2A 206
#5E43B7 #9D432C 86
#5E43B7 #0A2989 56
#5E43B7 #FF6A13 206
#5E43B7 #8E9089 146
#0A2989 #000000 26
#0A2989 #C12E1F 266
#0A2989 #00AE42 146
#0A2989 #545454 86
#0A2989 #D1D3D5 386
#0A2989 #5B6579 176
#0A2989 #F4EE2A 566
#0A2989 #9D432C 206
#0A2989 #5E43B7 116
#0A2989 #FF6A13 326
#0A2989 #8E9089 206
#FF6A13 #000000 56
#FF6A13 #C12E1F 26
#FF6A13 #545454 86
#FF6A13 #D1D3D5 146
#FF6A13 #F4EE2A 146
#FF6A13 #9D432C 56
#FF6A13 #5E43B7 146
#FF6A13 #0A2989 146
#FF6A13 #8E9089 116
#8E9089 #C12E1F 86
#8E9089 #00AE42 56
#8E9089 #D1D3D5 86
#8E9089 #F4EE2A 206
#8E9089 #9D432C 86
#8E9089 #FF6A13 86)FLUSH";
inline constexpr char kFlushDataDualStandard[] = R"FLUSH(colors
#000000 #FFFFFF #545454 #8E9089 #C12E1F #F4EE2A #0086D6 #F7E6DE #00AE42 #5E43B7 #482960 #0056B8 #FEC600 #EC008C #F5547C #6F5034 #FF9016 #00B1B7 #BECF00
src dst flush
#000000 #FFFFFF 900
#000000 #545454 450
#000000 #8E9089 540
#000000 #C12E1F 600
#000000 #F4EE2A 900
#000000 #0086D6 570
#000000 #F7E6DE 900
#000000 #00AE42 810
#000000 #5E43B7 480
#000000 #482960 270
#000000 #0056B8 540
#000000 #FEC600 900
#000000 #EC008C 900
#000000 #F5547C 900
#000000 #FF9016 900
#000000 #00B1B7 630
#000000 #BECF00 900
#FFFFFF #000000 90
#FFFFFF #545454 240
#FFFFFF #8E9089 120
#FFFFFF #C12E1F 90
#FFFFFF #F4EE2A 90
#FFFFFF #0086D6 90
#FFFFFF #F7E6DE 90
#FFFFFF #00AE42 120
#FFFFFF #5E43B7 90
#FFFFFF #0056B8 90
#FFFFFF #FEC600 150
#FFFFFF #EC008C 150
#FFFFFF #F5547C 120
#FFFFFF #6F5034 120
#FFFFFF #FF9016 120
#FFFFFF #00B1B7 120
#FFFFFF #BECF00 90
#545454 #000000 90
#545454 #FFFFFF 360
#545454 #8E9089 120
#545454 #C12E1F 270
#545454 #F4EE2A 330
#545454 #0086D6 270
#545454 #F7E6DE 390
#545454 #00AE42 270
#545454 #5E43B7 120
#545454 #482960 150
#545454 #0056B8 180
#545454 #FEC600 300
#545454 #EC008C 240
#545454 #F5547C 300
#545454 #6F5034 120
#545454 #FF9016 240
#545454 #00B1B7 270
#545454 #BECF00 300
#8E9089 #000000 270
#8E9089 #FFFFFF 330
#8E9089 #545454 300
#8E9089 #C12E1F 240
#8E9089 #F4EE2A 240
#8E9089 #0086D6 240
#8E9089 #F7E6DE 390
#8E9089 #00AE42 210
#8E9089 #5E43B7 270
#8E9089 #482960 300
#8E9089 #0056B8 180
#8E9089 #FEC600 240
#8E9089 #EC008C 240
#8E9089 #F5547C 240
#8E9089 #6F5034 210
#8E9089 #FF9016 240
#8E9089 #00B1B7 210
#8E9089 #BECF00 270
#C12E1F #000000 150
#C12E1F #FFFFFF 900
#C12E1F #545454 300
#C12E1F #8E9089 570
#C12E1F #F4EE2A 450
#C12E1F #0086D6 390
#C12E1F #F7E6DE 630
#C12E1F #00AE42 420
#C12E1F #5E43B7 330
#C12E1F #482960 210
#C12E1F #0056B8 300
#C12E1F #FEC600 660
#C12E1F #EC008C 240
#C12E1F #F5547C 180
#C12E1F #6F5034 210
#C12E1F #FF9016 270
#C12E1F #00B1B7 540
#C12E1F #BECF00 360
#F4EE2A #000000 150
#F4EE2A #FFFFFF 900
#F4EE2A #545454 390
#F4EE2A #8E9089 450
#F4EE2A #C12E1F 180
#F4EE2A #0086D6 270
#F4EE2A #F7E6DE 570
#F4EE2A #00AE42 120
#F4EE2A #5E43B7 330
#F4EE2A #482960 330
#F4EE2A #0056B8 240
#F4EE2A #FEC600 90
#F4EE2A #EC008C 330
#F4EE2A #F5547C 420
#F4EE2A #6F5034 240
#F4EE2A #FF9016 150
#F4EE2A #00B1B7 360
#F4EE2A #BECF00 240
#0086D6 #000000 150
#0086D6 #FFFFFF 420
#0086D6 #545454 120
#0086D6 #8E9089 480
#0086D6 #C12E1F 240
#0086D6 #F4EE2A 360
#0086D6 #F7E6DE 390
#0086D6 #00AE42 120
#0086D6 #5E43B7 150
#0086D6 #482960 150
#0086D6 #0056B8 120
#0086D6 #EC008C 330
#0086D6 #F5547C 330
#0086D6 #6F5034 150
#0086D6 #FF9016 300
#0086D6 #00B1B7 150
#0086D6 #BECF00 270
#F7E6DE #000000 90
#F7E6DE #FFFFFF 90
#F7E6DE #545454 120
#F7E6DE #8E9089 120
#F7E6DE #C12E1F 90
#F7E6DE #F4EE2A 60
#F7E6DE #0086D6 90
#F7E6DE #00AE42 90
#F7E6DE #5E43B7 90
#F7E6DE #482960 120
#F7E6DE #0056B8 120
#F7E6DE #FEC600 120
#F7E6DE #EC008C 150
#F7E6DE #F5547C 120
#F7E6DE #6F5034 150
#F7E6DE #FF9016 120
#F7E6DE #00B1B7 90
#F7E6DE #BECF00 120
#00AE42 #000000 150
#00AE42 #FFFFFF 900
#00AE42 #545454 240
#00AE42 #8E9089 330
#00AE42 #C12E1F 210
#00AE42 #F4EE2A 270
#00AE42 #F7E6DE 360
#00AE42 #5E43B7 180
#00AE42 #482960 180
#00AE42 #0056B8 240
#00AE42 #FEC600 300
#00AE42 #EC008C 300
#00AE42 #F5547C 390
#00AE42 #6F5034 180
#00AE42 #FF9016 300
#00AE42 #00B1B7 360
#00AE42 #BECF00 270
#5E43B7 #000000 90
#5E43B7 #FFFFFF 630
#5E43B7 #545454 150
#5E43B7 #8E9089 210
#5E43B7 #C12E1F 210
#5E43B7 #F4EE2A 330
#5E43B7 #0086D6 180
#5E43B7 #F7E6DE 510
#5E43B7 #00AE42 240
#5E43B7 #482960 150
#5E43B7 #0056B8 120
#5E43B7 #FEC600 540
#5E43B7 #EC008C 270
#5E43B7 #F5547C 420
#5E43B7 #6F5034 150
#5E43B7 #FF9016 330
#5E43B7 #00B1B7 270
#5E43B7 #BECF00 330
#482960 #000000 90
#482960 #FFFFFF 900
#482960 #545454 240
#482960 #8E9089 510
#482960 #C12E1F 360
#482960 #F4EE2A 420
#482960 #0086D6 330
#482960 #F7E6DE 510
#482960 #00AE42 390
#482960 #5E43B7 270
#482960 #0056B8 300
#482960 #FEC600 660
#482960 #EC008C 360
#482960 #F5547C 510
#482960 #6F5034 180
#482960 #FF9016 540
#482960 #00B1B7 450
#482960 #BECF00 600
#0056B8 #000000 90
#0056B8 #FFFFFF 780
#0056B8 #545454 270
#0056B8 #8E9089 270
#0056B8 #C12E1F 330
#0056B8 #F4EE2A 630
#0056B8 #0086D6 180
#0056B8 #F7E6DE 840
#0056B8 #00AE42 270
#0056B8 #5E43B7 270
#0056B8 #482960 150
#0056B8 #FEC600 630
#0056B8 #EC008C 450
#0056B8 #6F5034 240
#0056B8 #FF9016 510
#0056B8 #00B1B7 240
#0056B8 #BECF00 510
#FEC600 #000000 90
#FEC600 #FFFFFF 900
#FEC600 #545454 390
#FEC600 #8E9089 600
#FEC600 #C12E1F 180
#FEC600 #F4EE2A 150
#FEC600 #0086D6 330
#FEC600 #F7E6DE 600
#FEC600 #00AE42 180
#FEC600 #5E43B7 270
#FEC600 #482960 330
#FEC600 #0056B8 300
#FEC600 #EC008C 390
#FEC600 #F5547C 570
#FEC600 #6F5034 240
#FEC600 #FF9016 150
#FEC600 #00B1B7 510
#FEC600 #BECF00 300
#EC008C #000000 90
#EC008C #FFFFFF 900
#EC008C #545454 240
#EC008C #8E9089 270
#EC008C #C12E1F 120
#EC008C #F4EE2A 360
#EC008C #0086D6 270
#EC008C #F7E6DE 660
#EC008C #00AE42 330
#EC008C #5E43B7 270
#EC008C #482960 270
#EC008C #0056B8 210
#EC008C #FEC600 510
#EC008C #F5547C 120
#EC008C #6F5034 180
#EC008C #00B1B7 360
#EC008C #BECF00 570
#F5547C #000000 90
#F5547C #FFFFFF 900
#F5547C #545454 180
#F5547C #8E9089 180
#F5547C #C12E1F 150
#F5547C #F4EE2A 270
#F5547C #0086D6 270
#F5547C #F7E6DE 540
#F5547C #00AE42 300
#F5547C #5E43B7 210
#F5547C #482960 240
#F5547C #0056B8 210
#F5547C #FEC600 330
#F5547C #EC008C 120
#F5547C #6F5034 180
#F5547C #FF9016 150
#F5547C #00B1B7 300
#F5547C #BECF00 330
#6F5034 #000000 180
#6F5034 #FFFFFF 660
#6F5034 #545454 180
#6F5034 #8E9089 240
#6F5034 #C12E1F 240
#6F5034 #F4EE2A 390
#6F5034 #0086D6 330
#6F5034 #F7E6DE 420
#6F5034 #00AE42 300
#6F5034 #5E43B7 300
#6F5034 #482960 180
#6F5034 #0056B8 300
#6F5034 #FEC600 270
#6F5034 #EC008C 210
#6F5034 #F5547C 240
#6F5034 #FF9016 240
#6F5034 #00B1B7 270
#6F5034 #BECF00 360
#FF9016 #FFFFFF 900
#FF9016 #545454 240
#FF9016 #8E9089 270
#FF9016 #C12E1F 150
#FF9016 #F4EE2A 330
#FF9016 #0086D6 240
#FF9016 #F7E6DE 390
#FF9016 #00AE42 240
#FF9016 #5E43B7 270
#FF9016 #482960 180
#FF9016 #0056B8 240
#FF9016 #FEC600 210
#FF9016 #EC008C 210
#FF9016 #F5547C 210
#FF9016 #6F5034 180
#FF9016 #00B1B7 300
#FF9016 #BECF00 270
#00B1B7 #000000 210
#00B1B7 #FFFFFF 480
#00B1B7 #545454 300
#00B1B7 #8E9089 180
#00B1B7 #C12E1F 300
#00B1B7 #F4EE2A 300
#00B1B7 #0086D6 150
#00B1B7 #F7E6DE 390
#00B1B7 #00AE42 120
#00B1B7 #5E43B7 270
#00B1B7 #482960 270
#00B1B7 #0056B8 150
#00B1B7 #FEC600 330
#00B1B7 #EC008C 270
#00B1B7 #F5547C 270
#00B1B7 #6F5034 210
#00B1B7 #FF9016 270
#00B1B7 #BECF00 240
#BECF00 #000000 270
#BECF00 #FFFFFF 450
#BECF00 #545454 270
#BECF00 #8E9089 270
#BECF00 #C12E1F 150
#BECF00 #F4EE2A 90
#BECF00 #0086D6 300
#BECF00 #F7E6DE 300
#BECF00 #00AE42 180
#BECF00 #5E43B7 270
#BECF00 #482960 210
#BECF00 #0056B8 240
#BECF00 #FEC600 210
#BECF00 #EC008C 240
#BECF00 #F5547C 150
#BECF00 #6F5034 150
#BECF00 #FF9016 150
#BECF00 #00B1B7 270)FLUSH";
inline constexpr char kFlushDataDualHighflow[] = R"FLUSH(colors
#000000 #FFFFFF #545454 #8E9089 #C12E1F #F4EE2A #0086D6 #F7E6DE #00AE42 #5E43B7 #482960 #0056B8 #FEC600 #EC008C #F5547C #6F5034 #FF9016 #00B1B7 #BECF00
src dst flush
#000000 #FFFFFF 900
#000000 #545454 270
#000000 #C12E1F 900
#000000 #F4EE2A 900
#000000 #0086D6 900
#000000 #F7E6DE 900
#000000 #00AE42 900
#000000 #5E43B7 900
#000000 #482960 360
#000000 #0056B8 660
#000000 #FEC600 900
#000000 #EC008C 900
#000000 #F5547C 900
#000000 #6F5034 510
#000000 #FF9016 900
#000000 #BECF00 900
#FFFFFF #000000 120
#FFFFFF #545454 150
#FFFFFF #8E9089 120
#FFFFFF #C12E1F 120
#FFFFFF #F4EE2A 150
#FFFFFF #0086D6 90
#FFFFFF #F7E6DE 120
#FFFFFF #00AE42 120
#FFFFFF #5E43B7 150
#FFFFFF #482960 120
#FFFFFF #0056B8 150
#FFFFFF #FEC600 120
#FFFFFF #EC008C 150
#FFFFFF #F5547C 120
#FFFFFF #6F5034 150
#FFFFFF #FF9016 120
#FFFFFF #00B1B7 90
#FFFFFF #BECF00 120
#545454 #000000 150
#545454 #FFFFFF 480
#545454 #8E9089 210
#545454 #C12E1F 270
#545454 #F4EE2A 390
#545454 #0086D6 240
#545454 #F7E6DE 360
#545454 #00AE42 330
#545454 #5E43B7 180
#545454 #482960 150
#545454 #0056B8 180
#545454 #FEC600 300
#545454 #EC008C 240
#545454 #F5547C 330
#545454 #6F5034 120
#545454 #FF9016 330
#545454 #00B1B7 270
#545454 #BECF00 330
#8E9089 #000000 180
#8E9089 #FFFFFF 330
#8E9089 #545454 180
#8E9089 #C12E1F 180
#8E9089 #F4EE2A 360
#8E9089 #0086D6 180
#8E9089 #F7E6DE 360
#8E9089 #00AE42 120
#8E9089 #5E43B7 240
#8E9089 #482960 240
#8E9089 #0056B8 150
#8E9089 #FEC600 330
#8E9089 #EC008C 270
#8E9089 #F5547C 240
#8E9089 #6F5034 300
#8E9089 #FF9016 300
#8E9089 #00B1B7 180
#8E9089 #BECF00 300
#C12E1F #000000 150
#C12E1F #FFFFFF 600
#C12E1F #545454 570
#C12E1F #8E9089 390
#C12E1F #F4EE2A 900
#C12E1F #0086D6 900
#C12E1F #F7E6DE 900
#C12E1F #00AE42 540
#C12E1F #5E43B7 570
#C12E1F #482960 210
#C12E1F #0056B8 630
#C12E1F #FEC600 420
#C12E1F #EC008C 270
#C12E1F #F5547C 300
#C12E1F #6F5034 300
#C12E1F #FF9016 390
#C12E1F #00B1B7 600
#C12E1F #BECF00 660
#F4EE2A #000000 150
#F4EE2A #FFFFFF 660
#F4EE2A #545454 420
#F4EE2A #8E9089 300
#F4EE2A #C12E1F 180
#F4EE2A #0086D6 900
#F4EE2A #F7E6DE 900
#F4EE2A #00AE42 180
#F4EE2A #5E43B7 690
#F4EE2A #0056B8 360
#F4EE2A #FEC600 120
#F4EE2A #EC008C 600
#F4EE2A #F5547C 240
#F4EE2A #6F5034 270
#F4EE2A #FF9016 120
#F4EE2A #00B1B7 630
#F4EE2A #BECF00 120
#0086D6 #000000 120
#0086D6 #FFFFFF 510
#0086D6 #545454 150
#0086D6 #8E9089 180
#0086D6 #C12E1F 300
#0086D6 #F4EE2A 360
#0086D6 #F7E6DE 480
#0086D6 #00AE42 120
#0086D6 #5E43B7 210
#0086D6 #482960 120
#0086D6 #0056B8 150
#0086D6 #FEC600 330
#0086D6 #EC008C 330
#0086D6 #F5547C 330
#0086D6 #6F5034 150
#0086D6 #FF9016 300
#0086D6 #00B1B7 120
#0086D6 #BECF00 360
#F7E6DE #000000 150
#F7E6DE #FFFFFF 150
#F7E6DE #545454 120
#F7E6DE #8E9089 150
#F7E6DE #C12E1F 150
#F7E6DE #F4EE2A 120
#F7E6DE #0086D6 180
#F7E6DE #00AE42 150
#F7E6DE #5E43B7 210
#F7E6DE #482960 150
#F7E6DE #0056B8 150
#F7E6DE #FEC600 120
#F7E6DE #EC008C 150
#F7E6DE #F5547C 150
#F7E6DE #6F5034 150
#F7E6DE #FF9016 120
#F7E6DE #00B1B7 120
#F7E6DE #BECF00 120
#00AE42 #000000 120
#00AE42 #FFFFFF 690
#00AE42 #545454 330
#00AE42 #8E9089 330
#00AE42 #C12E1F 270
#00AE42 #F4EE2A 480
#00AE42 #0086D6 750
#00AE42 #F7E6DE 900
#00AE42 #5E43B7 450
#00AE42 #482960 210
#00AE42 #0056B8 240
#00AE42 #FEC600 420
#00AE42 #F5547C 390
#00AE42 #6F5034 210
#00AE42 #FF9016 420
#00AE42 #00B1B7 390
#00AE42 #BECF00 300
#5E43B7 #000000 150
#5E43B7 #FFFFFF 360
#5E43B7 #545454 150
#5E43B7 #8E9089 180
#5E43B7 #C12E1F 270
#5E43B7 #F4EE2A 570
#5E43B7 #0086D6 240
#5E43B7 #F7E6DE 660
#5E43B7 #00AE42 240
#5E43B7 #482960 240
#5E43B7 #0056B8 150
#5E43B7 #FEC600 390
#5E43B7 #EC008C 330
#5E43B7 #F5547C 270
#5E43B7 #6F5034 150
#5E43B7 #FF9016 300
#5E43B7 #00B1B7 270
#5E43B7 #BECF00 390
#482960 #000000 120
#482960 #FFFFFF 570
#482960 #545454 240
#482960 #8E9089 330
#482960 #C12E1F 480
#482960 #F4EE2A 780
#482960 #0086D6 420
#482960 #F7E6DE 900
#482960 #00AE42 510
#482960 #5E43B7 270
#482960 #0056B8 420
#482960 #FEC600 540
#482960 #EC008C 480
#482960 #F5547C 390
#482960 #6F5034 270
#482960 #FF9016 510
#482960 #00B1B7 480
#482960 #BECF00 540
#0056B8 #000000 120
#0056B8 #FFFFFF 900
#0056B8 #545454 240
#0056B8 #8E9089 330
#0056B8 #C12E1F 570
#0056B8 #F4EE2A 900
#0056B8 #0086D6 210
#0056B8 #F7E6DE 780
#0056B8 #00AE42 300
#0056B8 #5E43B7 210
#0056B8 #482960 270
#0056B8 #FEC600 630
#0056B8 #EC008C 540
#0056B8 #F5547C 660
#0056B8 #6F5034 300
#0056B8 #FF9016 690
#0056B8 #00B1B7 240
#0056B8 #BECF00 540
#FEC600 #000000 120
#FEC600 #FFFFFF 630
#FEC600 #545454 540
#FEC600 #8E9089 480
#FEC600 #C12E1F 180
#FEC600 #F4EE2A 180
#FEC600 #0086D6 900
#FEC600 #F7E6DE 720
#FEC600 #00AE42 210
#FEC600 #5E43B7 900
#FEC600 #482960 330
#FEC600 #0056B8 900
#FEC600 #EC008C 660
#FEC600 #F5547C 390
#FEC600 #6F5034 330
#FEC600 #FF9016 270
#FEC600 #00B1B7 630
#FEC600 #BECF00 210
#EC008C #000000 120
#EC008C #FFFFFF 540
#EC008C #545454 270
#EC008C #8E9089 330
#EC008C #C12E1F 120
#EC008C #F4EE2A 900
#EC008C #0086D6 330
#EC008C #00AE42 450
#EC008C #5E43B7 210
#EC008C #482960 180
#EC008C #0056B8 300
#EC008C #FEC600 360
#EC008C #F5547C 210
#EC008C #6F5034 240
#EC008C #FF9016 240
#EC008C #00B1B7 450
#EC008C #BECF00 510
#F5547C #000000 120
#F5547C #FFFFFF 570
#F5547C #545454 210
#F5547C #8E9089 240
#F5547C #C12E1F 150
#F5547C #F4EE2A 750
#F5547C #0086D6 270
#F5547C #F7E6DE 900
#F5547C #00AE42 540
#F5547C #5E43B7 150
#F5547C #482960 180
#F5547C #0056B8 150
#F5547C #FEC600 330
#F5547C #EC008C 150
#F5547C #6F5034 180
#F5547C #FF9016 270
#F5547C #00B1B7 390
#F5547C #BECF00 360
#6F5034 #000000 150
#6F5034 #FFFFFF 600
#6F5034 #545454 150
#6F5034 #8E9089 270
#6F5034 #C12E1F 180
#6F5034 #F4EE2A 360
#6F5034 #0086D6 360
#6F5034 #F7E6DE 390
#6F5034 #00AE42 330
#6F5034 #5E43B7 240
#6F5034 #482960 180
#6F5034 #0056B8 300
#6F5034 #FEC600 300
#6F5034 #EC008C 270
#6F5034 #F5547C 240
#6F5034 #FF9016 270
#6F5034 #00B1B7 270
#6F5034 #BECF00 390
#FF9016 #000000 120
#FF9016 #FFFFFF 450
#FF9016 #545454 240
#FF9016 #8E9089 300
#FF9016 #C12E1F 150
#FF9016 #F4EE2A 480
#FF9016 #0086D6 660
#FF9016 #F7E6DE 690
#FF9016 #00AE42 360
#FF9016 #5E43B7 360
#FF9016 #482960 210
#FF9016 #0056B8 330
#FF9016 #FEC600 210
#FF9016 #EC008C 240
#FF9016 #F5547C 210
#FF9016 #6F5034 270
#FF9016 #00B1B7 390
#FF9016 #BECF00 330
#00B1B7 #FFFFFF 630
#00B1B7 #8E9089 240
#00B1B7 #F4EE2A 390
#00B1B7 #EC008C 240
#00B1B7 #6F5034 360
#00B1B7 #FF9016 300
#00B1B7 #00B1B7 390
#00B1B7 #BECF00 300
#BECF00 #000000 150
#BECF00 #FFFFFF 630
#BECF00 #8E9089 300
#BECF00 #C12E1F 150
#BECF00 #F4EE2A 150
#BECF00 #0086D6 270
#BECF00 #F7E6DE 540
#BECF00 #00AE42 150
#BECF00 #5E43B7 270
#BECF00 #482960 210
#BECF00 #0056B8 240
#BECF00 #FEC600 120
#BECF00 #EC008C 270
#BECF00 #F5547C 270
#BECF00 #6F5034 150
#BECF00 #FF9016 120
#BECF00 #00B1B7 330)FLUSH";


inline const FlushVolPredictor &predictorForDataset(int dataset_value)
{
  static const FlushVolPredictor standard(kFlushDataStandard);
  static const FlushVolPredictor dualStandard(kFlushDataDualStandard);
  static const FlushVolPredictor dualHighflow(kFlushDataDualHighflow);
  switch (dataset_value) {
    case 1: return dualStandard;
    case 2: return dualHighflow;
    default: return standard;  // 0 (upstream default) and any other value
  }
}

}  // namespace detail

// ===========================================================================
// FlushVolCalculator — the upstream class (FlushVolCalc.{hpp,cpp}).
// ===========================================================================
class FlushVolCalculator
{
public:
  FlushVolCalculator(int minFlush = 0, int maxFlush = g_max_flush_volume, int flush_dataset = 0)
      : m_min_flush_vol(minFlush), m_max_flush_vol(maxFlush), m_flush_dataset(flush_dataset)
  {
  }

  /// Upstream get_flush_vol_from_data (FlushVolCalc.cpp:48-56): data-driven
  /// prediction for the configured dataset. False = no similar colours in
  /// the dataset or the pair is absent.
  bool getFlushVolFromData(unsigned char src_r, unsigned char src_g, unsigned char src_b,
                           unsigned char dst_r, unsigned char dst_g, unsigned char dst_b,
                           float &flush) const
  {
    const FlushVolPredictor &pd = detail::predictorForDataset(m_flush_dataset);
    FlushPredict::RGBColor src(src_r, src_g, src_b);
    FlushPredict::RGBColor dst(dst_r, dst_g, dst_b);
    return pd.predict(src, dst, flush);
  }

  /// Upstream calc_flush_vol_rgb (FlushVolCalc.cpp:58-100). NOTE verbatim
  /// semantics: with m_flush_dataset == 0 the STANDARD dataset predicts first
  /// and its value returns UNCLAMPED.
  int calcFlushVolRgb(unsigned char src_r, unsigned char src_g, unsigned char src_b,
                      unsigned char dst_r, unsigned char dst_g, unsigned char dst_b) const
  {
    float flush_volume;
    if (m_flush_dataset == 0 && getFlushVolFromData(src_r, src_g, src_b, dst_r, dst_g, dst_b, flush_volume))
      return static_cast<int>(flush_volume);

    const float sr = src_r / 255.0f, sg = src_g / 255.0f, sb = src_b / 255.0f;
    const float dr = dst_r / 255.0f, dg = dst_g / 255.0f, db = dst_b / 255.0f;

    // Calculate color difference in HSV color space
    float from_hsv_h, from_hsv_s, from_hsv_v;
    float to_hsv_h, to_hsv_s, to_hsv_v;
    rgb2hsv(sr, sg, sb, from_hsv_h, from_hsv_s, from_hsv_v);
    rgb2hsv(dr, dg, db, to_hsv_h, to_hsv_s, to_hsv_v);
    const float hs_dist = deltaHS(from_hsv_h, from_hsv_s, from_hsv_v, to_hsv_h, to_hsv_s, to_hsv_v);

    // 1. Color difference is more obvious if the dest color has high luminance
    // 2. Color difference is more obvious if the source color has low luminance
    const float from_lumi = luminance(sr, sg, sb);
    const float to_lumi = luminance(dr, dg, db);
    float lumi_flush = 0.0f;
    float hs_dist_adj = hs_dist;
    if (to_lumi >= from_lumi) {
      lumi_flush = std::pow(to_lumi - from_lumi, 0.7f) * 560.0f;
    } else {
      lumi_flush = (from_lumi - to_lumi) * 80.0f;
      const float inter_hsv_v = 0.67f * to_hsv_v + 0.33f * from_hsv_v;
      hs_dist_adj = std::min(inter_hsv_v, hs_dist);
    }
    const float hs_flush = 230.0f * hs_dist_adj;

    flush_volume = triangle3rdEdge(hs_flush, lumi_flush, 120.0f);
    flush_volume = std::max(flush_volume, 60.0f);

    return static_cast<int>(flush_volume);
  }

  /// Upstream calc_flush_vol (FlushVolCalc.cpp:102-129).
  int calcFlushVol(unsigned char srcA, unsigned char srcR, unsigned char srcG, unsigned char srcB,
                   unsigned char dstA, unsigned char dstR, unsigned char dstG, unsigned char dstB) const
  {
    // BBS: transparent materials are treated as white materials
    if (srcA == 0) {
      srcR = srcG = srcB = 255;
    }
    if (dstA == 0) {
      dstR = dstG = dstB = 255;
    }

    float flush_volume;
    if (m_flush_dataset != 0 && getFlushVolFromData(srcR, srcG, srcB, dstR, dstG, dstB, flush_volume))
      return std::min(static_cast<int>(flush_volume), m_max_flush_vol);

    flush_volume = static_cast<float>(calcFlushVolRgb(srcR, srcG, srcB, dstR, dstG, dstB));

    // Upstream verbatim (FlushVolCalc.cpp:120-125): variable names are the
    // upstream ones — the comparisons mean "bright source into dark
    // destination" (see the header note above). Applied only when a
    // dual-nozzle dataset (1/2) is configured, like upstream.
    constexpr float dark_color_thres = 180.0f / 255.0f;
    constexpr float light_color_thres = 75.0f / 255.0f;
    const bool is_from_dark = luminance(srcR / 255.0f, srcG / 255.0f, srcB / 255.0f) > dark_color_thres;
    const bool is_to_light = luminance(dstR / 255.0f, dstG / 255.0f, dstB / 255.0f) < light_color_thres;
    if (m_flush_dataset != 0 && is_from_dark && is_to_light)
      flush_volume *= 1.3f;

    flush_volume += static_cast<float>(m_min_flush_vol);
    return std::min(static_cast<int>(flush_volume), m_max_flush_vol);
  }

private:
  int m_min_flush_vol;
  int m_max_flush_vol;
  int m_flush_dataset;

  static float toRad(float deg) { return deg / 180.0f * float(detail::kPi); }
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
    // Standard RGB->HSV (port of upstream ColorSpaceConvert RGB2HSV).
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

}  // namespace OWzx
