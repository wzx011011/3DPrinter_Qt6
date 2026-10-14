#pragma once

#include <QString>
#include <QVector3D>

// Phase 92 (ASMMEASURE-02): pure-data measurement between two volume AABBs.
// The bounds type lives in PrepareSceneData.h; that header is lightweight
// (Qt Core only — no QRhi, no libslic3r), so including it here keeps this
// helper testable without linking the full renderer (mirrors the GizmoCenter.h
// pattern at src/core/rendering/GizmoCenter.h).
#include "qml_gui/Renderer/PrepareSceneData.h"

// X3 (2026-10-14): the Phase 92 "screenshot-driven approximation" is retired.
// The struct keeps ONLY the honest geometry — the world AABB centers, the
// center-to-center distance and the per-axis deltas — as the COARSE fallback
// that anchors the assembly overlay's dimension leader line
// (RhiViewportRenderer uploadAssemblyMeasureBuffers consumes centerA/centerB).
// The invented values are gone: angleDeg / axisA / axisB (the
// longest-AABB-axis angle that structurally could only ever produce
// 0°/90.000°, because a world AABB loses object rotation) no longer exist on
// this struct.
//
// The measurement TRUTH is the upstream picked-feature path: user picks
// feature A, then feature B; the readouts (angle / perpendicular distance /
// direct distance / distance XYZ) come from MeasureEngine::measureFeatures —
// a direct instantiation of upstream Measure::get_measurement (see
// MeasureEngine.h, ME-01 "INSTANTIATE, NOT REIMPLEMENT" and the ME-05 note
// about this file's fallback role). On the EditorViewModel the assembly
// readout properties (assemblyMeasureDistanceText / assemblyMeasureAngleText /
// assemblyMeasureDistanceXyz) re-route to that flow and stay EMPTY while no
// feature has been picked — mirroring upstream GLGizmoMeasure, which shows no
// measurement until two features are picked
// (third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoMeasure.cpp two-click
// flow; math in third_party/OrcaSlicer/src/libslic3r/Measure.hpp
// DistAndPoints / AngleAndEdges).
struct AssemblyMeasureResult
{
  QVector3D centerA;
  QVector3D centerB;
  float distance = 0.0f;       // Euclidean center-to-center (mm)
  QVector3D distanceXyz;       // per-axis delta A->B (mm)
  bool valid = false;          // false if either AABB is degenerate
};

// Free functions in a namespace so they can be unit-tested without linking
// the full RhiViewportRenderer (which would drag in QRhi + libslic3r). The
// viewmodel and renderer forward to these (mirrors GizmoGeometry / GizmoCenter).
namespace AssemblyMeasureGeometry
{
// Compute the coarse center-to-center measurement between two volume AABBs
// (overlay anchor fallback; the readout truth is the picked-feature path).
AssemblyMeasureResult measure(const PrepareSceneData::ModelBounds &a,
                              const PrepareSceneData::ModelBounds &b);

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
