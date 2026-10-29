#include "GizmoMath.h"

#include "GizmoGeometry.h"

#include <cfloat>
#include <cmath>

// GizmoMath implementation.
//
// computeRay / rayXZIntersect / rayToAxisT / computeRotateAngle were extracted
// VERBATIM from GLViewportRenderer.cpp (Phase 65); the floating-point
// operations, constants, and control flow are bit-identical so the GL path
// retains its original behavior.
//
// X3 (2026-10-14) - pickMoveAxis / pickRotateAxis / pickScaleAxis no longer use
// the Phase 65 self-invented heuristic (ray-to-axis-line distance with a
// world-unit threshold). They now port the upstream grabber precise picking:
// the cursor ray is intersected EXACTLY against the same grabber-body
// triangles the renderer draws (GizmoGeometry), and the closest hit along the
// ray wins. This mirrors the upstream mechanism where every grabber body is
// registered as a SceneRaycaster::EType::Gizmo raycaster backed by a
// MeshRaycaster over its rendered mesh, and hover resolves to the closest hit
// (third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoBase.cpp:142-165
// Grabber::render -> add_raycaster_for_picking; GLGizmoMove.cpp:153-157 cone
// grabbers at the arrow tips, :275 gizmos-on-top). Because the pick consumes
// the very vertex tables the RHI gizmo shader draws
// (world = gizmoCenter + gizmoScale * local, shaders/rhi_gizmo.vert:29), what
// you can grab is exactly what you can see - the property upstream gets from
// raycasting the rendered PickingModel.
//
// Source locations in GLViewportRenderer.cpp at HEAD (pre-X3, for the
// verbatim-extracted functions below):
//   computeRay         lines 1250-1266
//   rayXZIntersect     lines 1271-1278
//   rayToAxisT         lines 1346-1359
//   computeRotateAngle lines 1491-1517

namespace
{
// Two-sided Moller-Trumbore ray-triangle intersection. Returns the ray
// parameter t (>= 0) of the hit, or -1 on miss. Two-sided like the upstream
// MeshRaycaster, so a grabber body is hittable from either winding.
float rayTriangleHit(const QVector3D &orig, const QVector3D &dir,
                     const QVector3D &v0, const QVector3D &v1,
                     const QVector3D &v2)
{
  const QVector3D e1 = v1 - v0;
  const QVector3D e2 = v2 - v0;
  const QVector3D p = QVector3D::crossProduct(dir, e2);
  const float det = QVector3D::dotProduct(e1, p);
  if (std::abs(det) < 1e-12f)
    return -1.0f;  // ray parallel to the triangle plane
  const float invDet = 1.0f / det;
  const QVector3D tvec = orig - v0;
  const float u = QVector3D::dotProduct(tvec, p) * invDet;
  if (u < 0.0f || u > 1.0f)
    return -1.0f;
  const QVector3D q = QVector3D::crossProduct(tvec, e1);
  const float v = QVector3D::dotProduct(dir, q) * invDet;
  if (v < 0.0f || u + v > 1.0f)
    return -1.0f;
  const float t = QVector3D::dotProduct(e2, q) * invDet;
  return (t >= 0.0f) ? t : -1.0f;
}

// The immutable grabber-body vertex tables shared with the renderer. Built
// once; the pick and the RHI upload consume the SAME builders
// (GizmoGeometry::build*GizmoVertices), which is what makes pick == render.
struct GizmoBodyGeometry
{
  QVector<GizmoVertex> moveVerts;
  QVector<GizmoVertex> rotateVerts;
  QVector<GizmoVertex> scaleVerts;
  GizmoGeometryOffsets moveOffsets;
  GizmoGeometryOffsets rotateOffsets;
  GizmoGeometryOffsets scaleOffsets;

  static const GizmoBodyGeometry &instance()
  {
    static const GizmoBodyGeometry geometry = [] {
      GizmoBodyGeometry g;
      g.moveVerts = GizmoGeometry::buildMoveGizmoVertices(&g.moveOffsets);
      g.rotateVerts = GizmoGeometry::buildRotateGizmoVertices(&g.rotateOffsets);
      g.scaleVerts = GizmoGeometry::buildScaleGizmoVertices(&g.scaleOffsets);
      return g;
    }();
    return geometry;
  }
};

// Exact closest-hit pick across the three per-axis body meshes. `basePerAxis`
// gives the first vertex of each axis' body (vertCountPerAxis / 3 triangles
// each); vertices are mapped to world as gizmoCenter + scale * local exactly
// like the gizmo vertex shader. Returns 1/2/3 for X/Y/Z, 0 when no body is
// hit - there is no proximity threshold: the ray either crosses a drawn body
// or it does not (upstream SceneRaycaster closest-hit semantics).
int pickAxisBody(const QVector3D &orig, const QVector3D &dir,
                 const QVector3D &gizmoCenter, float scale,
                 const QVector<GizmoVertex> &verts,
                 const int (&basePerAxis)[3], int vertCountPerAxis)
{
  const GizmoVertex *data = verts.constData();
  const int triCount = vertCountPerAxis / 3;
  float bestT = FLT_MAX;
  int best = 0;
  for (int ax = 0; ax < 3; ++ax)
  {
    const int base = basePerAxis[ax];
    for (int tri = 0; tri < triCount; ++tri)
    {
      const GizmoVertex *a = data + base + tri * 3;
      const GizmoVertex *b = a + 1;
      const GizmoVertex *c = a + 2;
      const float t = rayTriangleHit(
          orig, dir,
          gizmoCenter + scale * QVector3D(a->x, a->y, a->z),
          gizmoCenter + scale * QVector3D(b->x, b->y, b->z),
          gizmoCenter + scale * QVector3D(c->x, c->y, c->z));
      if (t >= 0.0f && t < bestT)
      {
        bestT = t;
        best = ax + 1;
      }
    }
  }
  return best;
}
}  // namespace

// ===========================================================================
// computeRay
// ===========================================================================
std::pair<QVector3D, QVector3D>
GizmoMath::computeRay(float sx, float sy, const QSize &viewSize,
                     const QMatrix4x4 &projMatrix, const QMatrix4x4 &viewMatrix)
{
  const float w = float(viewSize.width()), h = float(viewSize.height());
  if (w <= 0 || h <= 0)
    return {{}, {1, 0, 0}};
  QMatrix4x4 invPV = (projMatrix * viewMatrix).inverted();
  float ndcX = 2.f * sx / w - 1.f, ndcY = 1.f - 2.f * sy / h;
  QVector4D nearH = invPV * QVector4D(ndcX, ndcY, -1.f, 1.f);
  QVector4D farH = invPV * QVector4D(ndcX, ndcY, 1.f, 1.f);
  nearH /= nearH.w();
  farH /= farH.w();
  QVector3D orig = nearH.toVector3D();
  return {orig, (farH.toVector3D() - orig).normalized()};
}

// ===========================================================================
// rayXZIntersect
// ===========================================================================
QVector3D GizmoMath::rayXZIntersect(float sx, float sy, const QSize &viewSize,
                                    const QMatrix4x4 &projMatrix,
                                    const QMatrix4x4 &viewMatrix)
{
  auto [orig, dir] = computeRay(sx, sy, viewSize, projMatrix, viewMatrix);
  if (std::abs(dir.y()) < 1e-6f)
    return {0, 0, 0};
  float t = -orig.y() / dir.y();
  return orig + t * dir;
}

// ===========================================================================
// rayToAxisT  -- closest t along axisDir at gizmoCenter from screen ray
// ===========================================================================
float GizmoMath::rayToAxisT(const QVector3D &orig, const QVector3D &dir,
                            const QVector3D &axisDir,
                            const QVector3D &gizmoCenter)
{
  // Solve shortest distance between two lines:
  // L1: orig + t*dir   L2: gizmoCenter + s*axisDir
  QVector3D w = orig - gizmoCenter;
  float b = QVector3D::dotProduct(dir, axisDir);
  float e = QVector3D::dotProduct(axisDir, w);
  float d = QVector3D::dotProduct(dir, w);
  float denom = 1.f - b * b;
  if (std::abs(denom) < 1e-8f)
    return e; // parallel
  return (e - b * d) / denom;
}

// ===========================================================================
// pickMoveAxis  -- returns 1/2/3 for X/Y/Z, 0 if no hit
//
// X3: exact ray vs the drawn CONE grabber body of each move arrow (local
// axial span [0.78, 1.0], base radius 0.055, tip at 1.0 - the same triangles
// the RHI gizmo path draws). The shaft is a dashed line primitive exactly
// like the upstream grabber connection (GLGizmoMove.cpp render_grabber_
// connection) and is not pickable upstream either - only grabber bodies are.
// ===========================================================================
int GizmoMath::pickMoveAxis(float sx, float sy, const QSize &viewSize,
                           const QMatrix4x4 &projMatrix,
                           const QMatrix4x4 &viewMatrix,
                           const QVector3D &gizmoCenter,
                           const QVector3D &cameraEye, bool hasSelection)
{
  if (!hasSelection)
    return 0;

  auto [orig, dir] = computeRay(sx, sy, viewSize, projMatrix, viewMatrix);
  const GizmoBodyGeometry &geometry = GizmoBodyGeometry::instance();
  return pickAxisBody(orig, dir, gizmoCenter,
                      GizmoMath::gizmoWorldScale(gizmoCenter, cameraEye),
                      geometry.moveVerts, geometry.moveOffsets.coneBase,
                      geometry.moveOffsets.coneVertCount);
}

// ===========================================================================
// computeRotateAngle  -- compute angle for rotate gizmo interaction
// ===========================================================================
float GizmoMath::computeRotateAngle(float sx, float sy, int axis,
                                   const QSize &viewSize,
                                   const QMatrix4x4 &projMatrix,
                                   const QMatrix4x4 &viewMatrix,
                                   const QVector3D &gizmoCenter,
                                   float rotateStartAngle)
{
  // Project mouse position onto the rotation plane and compute angle
  QVector3D planeNormal;
  if (axis == 1) planeNormal = QVector3D(1, 0, 0);
  else if (axis == 2) planeNormal = QVector3D(0, 1, 0);
  else planeNormal = QVector3D(0, 0, 1);

  auto [orig, dir] = computeRay(sx, sy, viewSize, projMatrix, viewMatrix);
  float denom = QVector3D::dotProduct(dir, planeNormal);
  if (std::abs(denom) < 1e-6f)
    return rotateStartAngle; // ray parallel to plane

  float t = QVector3D::dotProduct(gizmoCenter - orig, planeNormal) / denom;
  QVector3D hit = orig + t * dir;
  QVector3D v = hit - gizmoCenter;

  // Compute angle on the rotation plane
  // Reference axis: the "right" direction on this plane
  QVector3D ref;
  if (axis == 1) ref = QVector3D(0, 1, 0); // Y-axis as reference for X rotation
  else if (axis == 2) ref = QVector3D(1, 0, 0); // X-axis as reference for Y rotation
  else ref = QVector3D(1, 0, 0); // X-axis as reference for Z rotation

  return std::atan2(QVector3D::dotProduct(QVector3D::crossProduct(ref, v).normalized(), planeNormal),
                    QVector3D::dotProduct(ref, v));
}

int GizmoMath::rotateSnapBand(
    float sx, float sy, int axis, const QSize &viewSize,
    const QMatrix4x4 &projMatrix, const QMatrix4x4 &viewMatrix,
    const QVector3D &gizmoCenter, const QVector3D &cameraEye)
{
  // RG-R4-01: see the header note. Same plane projection as
  // computeRotateAngle, plus the cursor's radial distance from the center
  // against the drawn ring radius (kRotateMajorR is the geometry constant
  // behind GizmoGeometry::buildRotateGizmoVertices).
  QVector3D planeNormal;
  if (axis == 1) planeNormal = QVector3D(1, 0, 0);
  else if (axis == 2) planeNormal = QVector3D(0, 1, 0);
  else planeNormal = QVector3D(0, 0, 1);

  auto [orig, dir] = computeRay(sx, sy, viewSize, projMatrix, viewMatrix);
  float denom = QVector3D::dotProduct(dir, planeNormal);
  if (std::abs(denom) < 1e-6f)
    return 0; // ray parallel to the plane: cannot classify

  float t = QVector3D::dotProduct(gizmoCenter - orig, planeNormal) / denom;
  QVector3D hit = orig + t * dir;
  const float len = (hit - gizmoCenter).length();

  constexpr float kRingR = 0.7f;             // GizmoGeometry kRotateMajorR
  const float r = kRingR * gizmoWorldScale(gizmoCenter, cameraEye);
  constexpr float kScaleLongTooth = 0.1f;    // upstream ScaleLongTooth (percent of radius)
  if (r / 3.0f <= len && len <= 2.0f * r / 3.0f)
    return 1; // coarse band: 45-degree steps
  if (r <= len && len <= r + r * kScaleLongTooth)
    return 2; // fine tooth: 5-degree steps
  return 0;
}

// ===========================================================================
// gizmoWorldScale  -- S4 single source of the gizmo world scale
//
// max(distance(gizmoCenter, cameraEye) * 0.15, 5.0). The pick side
// (pickMoveAxis/pickRotateAxis/pickScaleAxis -- the drawn grabber bodies) and
// the render side (RhiViewportRenderer's selection-center sphere rebuild +
// the CameraBlock uniform) all resolve the scale through THIS function, so
// what you can grab is exactly what you can see (the formula was previously
// inlined on both sides of that implicit contract).
// ===========================================================================
float GizmoMath::gizmoWorldScale(const QVector3D &gizmoCenter,
                                 const QVector3D &cameraEye)
{
  return std::max((gizmoCenter - cameraEye).length() * 0.15f, 5.0f);
}

// ===========================================================================
// pickRotateAxis  -- returns 1/2/3 for X/Y/Z ring, 0 if no hit
//
// X3: exact ray vs the drawn torus ring bodies (majorR 0.7, minorR 0.035 in
// gizmo-local units). Upstream rides the same principle one level up: the
// ring grabber elements are registered as SceneRaycaster bodies and the
// closest hit along the ray wins (GLGizmoRotate.cpp grabber at
// m_radius * (1 + GrabberOffset); GLGizmoBase.cpp Grabber::render). Testing
// the drawn ring triangles keeps that closest-hit contract against exactly
// the geometry the user sees.
// ===========================================================================
int GizmoMath::pickRotateAxis(float sx, float sy, const QSize &viewSize,
                             const QMatrix4x4 &projMatrix,
                             const QMatrix4x4 &viewMatrix,
                             const QVector3D &gizmoCenter,
                             const QVector3D &cameraEye, bool hasSelection)
{
  if (!hasSelection)
    return 0;

  auto [orig, dir] = computeRay(sx, sy, viewSize, projMatrix, viewMatrix);
  const GizmoBodyGeometry &geometry = GizmoBodyGeometry::instance();
  return pickAxisBody(orig, dir, gizmoCenter,
                      GizmoMath::gizmoWorldScale(gizmoCenter, cameraEye),
                      geometry.rotateVerts, geometry.rotateOffsets.ringBase,
                      geometry.rotateOffsets.ringVertCount);
}

// ===========================================================================
// pickScaleAxis  -- returns 1/2/3 for X/Y/Z, 0 if no hit
//
// X3: exact ray vs the drawn box grabber handle at the end of each scale
// shaft (0.08 cubed centered at 0.78, gizmo-local). The scale grabber body
// upstream is likewise a box registered for picking; the shaft line is not
// pickable. (The GL original returned 1/2/3 only; uniform (4) is reserved for
// a future center handle and not exercised by the current drag loop.)
// ===========================================================================
int GizmoMath::pickScaleAxis(float sx, float sy, const QSize &viewSize,
                            const QMatrix4x4 &projMatrix,
                            const QMatrix4x4 &viewMatrix,
                            const QVector3D &gizmoCenter,
                            const QVector3D &cameraEye, bool hasSelection)
{
  if (!hasSelection)
    return 0;

  auto [orig, dir] = computeRay(sx, sy, viewSize, projMatrix, viewMatrix);
  const GizmoBodyGeometry &geometry = GizmoBodyGeometry::instance();
  return pickAxisBody(orig, dir, gizmoCenter,
                      GizmoMath::gizmoWorldScale(gizmoCenter, cameraEye),
                      geometry.scaleVerts, geometry.scaleOffsets.boxBase,
                      geometry.scaleOffsets.boxVertCount);
}
