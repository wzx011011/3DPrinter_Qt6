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
// forwards to fromSelectedBatch().
namespace GizmoCenter
{
// Returns the midpoint of the batch whose sourceObjectIndex matches
// `selectedSourceObjectIndex`. Returns origin (0,0,0) when the index is < 0
// or not found in `batches`.
QVector3D fromSelectedBatch(int selectedSourceObjectIndex,
                            const QList<PrepareSceneData::ModelBatch> &batches);
}
