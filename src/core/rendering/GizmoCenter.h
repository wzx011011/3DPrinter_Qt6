#pragma once

#include <QList>
#include <QVector3D>

// GizmoCenter only needs the POD bounds fields, but the batch type lives in
// PrepareSceneData.h. That header is lightweight (Qt Core only - no QRhi,
// no libslic3r), so including it here keeps this header test-friendly.
#include "qml_gui/Renderer/PrepareSceneData.h"

// Gizmo center computation (Phase 67, GWIRE-02). Free functions in a namespace
// so they can be unit-tested without linking the full RhiViewportRenderer
// (which would drag in QRhi + libslic3r). RhiViewportRenderer::computeGizmoCenter
// forwards to fromSelectedIndices().
namespace GizmoCenter
{
// P15.11 (MULTICENTER): midpoint of the union AABB of all batches belonging to
// ANY of `selectedSourceObjectIndices` (upstream Selection::get_bounding_box()
// unions the bounds of every selected volume before taking the center).
// Returns origin (0,0,0) when the list is empty or no listed index is found in
// `batches` (stale selection).
QVector3D fromSelectedIndices(const QList<int> &selectedSourceObjectIndices,
                              const QList<PrepareSceneData::ModelBatch> &batches);

// Single-index convenience overload (kept for the picking-path callers and the
// GWIRE-02 tests); delegates to fromSelectedIndices().
// Returns the midpoint of the union AABB of all batches whose sourceObjectIndex
// matches `selectedSourceObjectIndex`. Returns origin (0,0,0) when the index is
// < 0 or not found in `batches`.
QVector3D fromSelectedBatch(int selectedSourceObjectIndex,
                            const QList<PrepareSceneData::ModelBatch> &batches);
}
