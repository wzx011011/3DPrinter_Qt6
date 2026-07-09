#pragma once

#include <QString>
#include <QVector3D>

// Phase 92 (ASMMEASURE-02): pure-data measurement between two volume AABBs.
// The bounds type lives in PrepareSceneData.h; that header is lightweight
// (Qt Core only — no QRhi, no libslic3r), so including it here keeps this
// helper testable without linking the full renderer (mirrors the GizmoCenter.h
// pattern at src/core/rendering/GizmoCenter.h).
#include "qml_gui/Renderer/PrepareSceneData.h"

// AssemblyMeasureResult approximates upstream Measure::MeasurementResult
// (third_party/OrcaSlicer/src/libslic3r/Measure.hpp:147-180) for the common
// multi-volume Assembly-measure case: center-to-center distance + per-axis XYZ
// delta + angle between the two volumes' longest-AABB-axis directions.
//
// Full feature-picking (point/edge/circle/plane via ITS + scene raycaster) is
// a documented Phase 92 simplification deferred to Phase 93 / future (needs the
// per-volume indexed_triangle_set + the AssembleViewDataPool). This struct is
// the load-bearing shape the screenshot-parity measurement produces: the
// 装配页_测量.png distance value + the 90.000°-style angle box.
struct AssemblyMeasureResult
{
  QVector3D centerA;
  QVector3D centerB;
  float distance = 0.0f;       // Euclidean center-to-center (mm)
  QVector3D distanceXyz;       // per-axis delta A->B (mm)
  float angleDeg = 0.0f;       // angle between longest axes (degrees)
  QVector3D axisA;             // volume A longest-axis direction (unit)
  QVector3D axisB;             // volume B longest-axis direction (unit)
  bool valid = false;          // false if either AABB is degenerate
};

// Free functions in a namespace so they can be unit-tested without linking
// the full RhiViewportRenderer (which would drag in QRhi + libslic3r). The
// viewmodel and renderer forward to these (mirrors GizmoGeometry / GizmoCenter).
namespace AssemblyMeasureGeometry
{
// Compute the measurement between two volume AABBs.
AssemblyMeasureResult measure(const PrepareSceneData::ModelBounds &a,
                              const PrepareSceneData::ModelBounds &b);

// Longest-axis unit direction of an AABB (the X/Y/Z whose extent is largest).
// Returns the Z axis (arbitrary) when the AABB is degenerate (all extents ~0).
QVector3D longestAxis(const PrepareSceneData::ModelBounds &b);

// Center of an AABB.
QVector3D center(const PrepareSceneData::ModelBounds &b);

// Format a distance value to upstream's precision (GLGizmoMeasure.cpp:24
// format_double uses %.3f). Returns "<value> mm".
QString formatDistance(float mm);

// Format an angle value as degrees with the degree sign
// (GLGizmoMeasure.cpp:1558 format_double(rad2deg) + degree glyph).
// Returns "<value>°".
QString formatAngle(float deg);
}
