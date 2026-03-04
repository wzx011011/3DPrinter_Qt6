// nanosvg_impl.cpp — 提供 nanosvg 头文件库的函数实现
// upstream libslic3r 的 NSVGUtils.cpp 引用了 nsvgParse 等函数，
// 但实现宏 NANOSVG_IMPLEMENTATION 仅在 GUI 层定义。
// 由于我们不编译上游 slic3r GUI，需要在此处提供实现。

#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
