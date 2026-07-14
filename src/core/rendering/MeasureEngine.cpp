// ===========================================================================
// Phase 114 (MEASURE-03): MeasureEngine implementation.
// See MeasureEngine.h for the per-volume Measuring cache (ME-01),
// get_feature wiring (ME-02), SurfaceFeature boundary scrubbing
// (ME-03 / pitfall 6), and the AssemblyMeasureGeometry relationship (ME-05).
//
// INSTANTIATE, NOT REIMPLEMENT: this file calls Measure::Measuring and
// Measure::get_measurement directly. The plane extraction, feature
// detection, closest-feature math, and distance/angle math all live in
// libslic3r (Measure.cpp). We only (a) build the per-volume Measuring,
// (b) transform the world hit into mesh-local, (c) read the SurfaceFeature
// VALUES into Qt POD, and (d) destroy the SurfaceFeature before returning.
// ===========================================================================

#include "MeasureEngine.h"

#ifdef HAS_LIBSLIC3R
#include <utility>

#include <libslic3r/Measure.hpp>  // Measure::Measuring, SurfaceFeature, get_measurement, MeasurementResult
#include <libslic3r/TriangleMesh.hpp>  // indexed_triangle_set (complete type)

namespace OWzx {

namespace {
// Pitfall 6 scrubbing helper: read the VALUE fields of a libslic3r
// SurfaceFeature into a Qt-owned QtFeature. Called INSIDE the accessor
// body, BEFORE the SurfaceFeature goes out of scope. The returned QtFeature
// copies only the type/point/radius/plane-index -- it does NOT touch the
// SurfaceFeature's raw back-pointer members (the volume handle + the plane
// index vector handle, Measure.hpp:95-96). Those back-pointers stay on the
// libslic3r side and die with the SurfaceFeature.
//
// The input SurfaceFeature is assumed to already be in WORLD space (its
// translate(world_tran) was applied by Measuring::get_feature at
// Measure.cpp:569/576/582/592). We copy the points verbatim -- no further
// transform here.
QtFeature scrubSurfaceFeature(const Slic3r::Measure::SurfaceFeature &sf)
{
  QtFeature out;
  out.valid = true;
  switch (sf.get_type()) {
    case Slic3r::Measure::SurfaceFeatureType::Point: {
      out.kind = FeatureKind::Point;
      const Slic3r::Vec3d p = sf.get_point();  // Measure.hpp:59
      out.pt1 = QVector3D(float(p.x()), float(p.y()), float(p.z()));
      break;
    }
    case Slic3r::Measure::SurfaceFeatureType::Edge: {
      out.kind = FeatureKind::Edge;
      const auto edge = sf.get_edge();  // Measure.hpp:61 -> pair<Vec3d,Vec3d>
      out.pt1 = QVector3D(float(edge.first.x()), float(edge.first.y()),
                          float(edge.first.z()));
      out.pt2 = QVector3D(float(edge.second.x()), float(edge.second.y()),
                          float(edge.second.z()));
      break;
    }
    case Slic3r::Measure::SurfaceFeatureType::Circle: {
      out.kind = FeatureKind::Circle;
      // get_circle -> tuple<center, radius, normal> (Measure.hpp:64)
      const auto circle = sf.get_circle();
      const Slic3r::Vec3d &center = std::get<0>(circle);
      const double radius = std::get<1>(circle);
      const Slic3r::Vec3d &normal = std::get<2>(circle);
      out.pt1 = QVector3D(float(center.x()), float(center.y()),
                          float(center.z()));
      out.pt2 = QVector3D(float(normal.x()), float(normal.y()),
                          float(normal.z()));
      out.radius = float(radius);
      break;
    }
    case Slic3r::Measure::SurfaceFeatureType::Plane: {
      out.kind = FeatureKind::Plane;
      // get_plane -> tuple<planeIndex, normal, point-in-plane>
      // (Measure.hpp:67)
      const auto plane = sf.get_plane();
      out.planeIndex = std::get<0>(plane);
      const Slic3r::Vec3d &normal = std::get<1>(plane);
      const Slic3r::Vec3d &point = std::get<2>(plane);
      out.pt1 = QVector3D(float(normal.x()), float(normal.y()),
                          float(normal.z()));
      out.pt2 = QVector3D(float(point.x()), float(point.y()),
                          float(point.z()));
      break;
    }
    default:
    case Slic3r::Measure::SurfaceFeatureType::Undef: {
      // Should not happen for a feature get_feature returns, but guard
      // anyway: surface an invalid QtFeature so the caller's valid gate
      // fires.
      out.kind = FeatureKind::Undef;
      out.valid = false;
      break;
    }
  }
  return out;
}

// Rebuild a LOCAL libslic3r SurfaceFeature from a QtFeature for
// get_measurement. The LOCAL feature has its back-pointer members defaulted
// to null (the volume handle + the plane index vector handle, Measure.hpp:
// 95-96) -- it owns NO libslic3r memory, so it is safe to construct, pass
// to get_measurement, and destroy within the measureFeatures call. This is
// the pitfall-6-safe direction: Qt -> libslic3r (local), never libslic3r
// -> Qt (raw back-pointer).
//
// The points are MESH-LOCAL; measureFeatures applies worldTransform before
// calling get_measurement so the result is in world space (mirrors
// GLGizmoMeasure world_tran handling).
Slic3r::Measure::SurfaceFeature
buildLocalSurfaceFeature(const QtFeature &qf)
{
  const auto toVec3d = [](const QVector3D &v) {
    return Slic3r::Vec3d(double(v.x()), double(v.y()), double(v.z()));
  };
  switch (qf.kind) {
    case FeatureKind::Point:
      // SurfaceFeature(const Vec3d& pt) ctor (Measure.hpp:33-34)
      return Slic3r::Measure::SurfaceFeature(toVec3d(qf.pt1));
    case FeatureKind::Edge:
      // SurfaceFeature(type, pt1, pt2, pt3, value) ctor (Measure.hpp:30-31)
      return Slic3r::Measure::SurfaceFeature(
          Slic3r::Measure::SurfaceFeatureType::Edge, toVec3d(qf.pt1),
          toVec3d(qf.pt2));
    case FeatureKind::Circle: {
      // Circle: pt1=center, pt2=normal, value=radius
      // (mirrors Measure.cpp:334 emplace_back(Circle, center, normal,
      // nullopt, radius))
      return Slic3r::Measure::SurfaceFeature(
          Slic3r::Measure::SurfaceFeatureType::Circle, toVec3d(qf.pt1),
          toVec3d(qf.pt2), std::nullopt, double(qf.radius));
    }
    case FeatureKind::Plane: {
      // Plane: pt1=normal, pt2=point-in-plane, value=planeIndex
      // (mirrors Measure.cpp:516 emplace_back(Plane, normal, point, nullopt,
      // planeIndex))
      return Slic3r::Measure::SurfaceFeature(
          Slic3r::Measure::SurfaceFeatureType::Plane, toVec3d(qf.pt1),
          toVec3d(qf.pt2), std::nullopt, double(qf.planeIndex));
    }
    default:
    case FeatureKind::Undef: {
      // Fallback: a point at the origin. Callers gate on QtFeature.valid
      // before invoking measureFeatures, so Undef should not arrive here.
      return Slic3r::Measure::SurfaceFeature(Slic3r::Vec3d::Zero());
    }
  }
}
} // namespace

std::shared_ptr<Slic3r::Measure::Measuring>
MeasureEngine::measuringFor(int objectIndex, int volumeIndex)
{
  const Key key = volumeKey(objectIndex, volumeIndex);
  auto it = m_cache.find(key);
  if (it != m_cache.end())
    return it->second; // cache hit (ME-01): reuse the Measuring built on
                       // first touch -- its plane index lives for the
                       // volume's lifetime.

  // Lazily fetch the ITS from the Phase 112 accessor. nullptr => no mesh for
  // this (object, volume); skip caching so a future reload is re-tried.
  std::shared_ptr<const indexed_triangle_set> its =
      m_itsSource(objectIndex, volumeIndex);
  if (!its)
    return nullptr;

  // Instantiate Measure::Measuring directly (ME-01 -- instantiate, do NOT
  // reimplement). The MeasuringImpl pimpl indexes the mesh in its ctor
  // (Measure.cpp:639-641); caching keeps that cost amortized over the
  // volume's lifetime, NOT the cursor-move rate (pitfall 6/7).
  std::shared_ptr<Slic3r::Measure::Measuring> m =
      std::make_shared<Slic3r::Measure::Measuring>(*its);
  m_cache.emplace(key, m);
  return m;
}

QtFeature MeasureEngine::getFeature(int objectIndex,
                                    int volumeIndex,
                                    std::size_t facetIdx,
                                    const Slic3r::Vec3d &worldPoint,
                                    const Slic3r::Transform3d &worldTransform,
                                    bool onlySelectPlane)
{
  std::shared_ptr<Slic3r::Measure::Measuring> m =
      measuringFor(objectIndex, volumeIndex);
  if (!m)
    return QtFeature{}; // valid=false (no mesh for this volume)

  // World -> mesh-local transform for the hit point. Mirrors upstream
  // MeshUtils.cpp:432-434 (the same inverse-transform the Phase 113
  // SceneRaycaster uses for the ray). get_feature expects the point in
  // MESH-LOCAL space -- it applies world_tran to the OUTPUT feature
  // internally (Measure.cpp:569/576/582/592 translate(world_tran)), so the
  // returned SurfaceFeature is already in world space.
  const Slic3r::Transform3d inv = worldTransform.inverse();
  const Slic3r::Vec3d localPoint = inv * worldPoint;

  // get_feature returns nullopt when facetIdx is out of range
  // (Measure.cpp:527-528). That is a normal miss, not an error.
  std::optional<Slic3r::Measure::SurfaceFeature> sf =
      m->get_feature(facetIdx, localPoint, worldTransform, onlySelectPlane);
  if (!sf)
    return QtFeature{}; // valid=false

  // === PITFALL 6 BOUNDARY =================================================
  // The SurfaceFeature lives entirely in this scope. We read its VALUE
  // fields into the Qt-owned QtFeature right here, then the SurfaceFeature
  // is destroyed at function exit. NO raw back-pointer escapes -- the
  // scrubSurfaceFeature helper reads only the type/point/radius/plane-index
  // accessors (Measure.hpp:56-67) and never touches the back-pointer members
  // (Measure.hpp:95-96). The returned QtFeature is a pure POD the Qt layer
  // can hold indefinitely.
  QtFeature out = scrubSurfaceFeature(*sf);
  return out;
}

QtMeasurement
MeasureEngine::measureFeatures(const QtFeature &a,
                               const QtFeature &b,
                               const Slic3r::Transform3d &worldTransform)
{
  QtMeasurement out;
  if (!a.valid || !b.valid)
    return out; // valid=false -- caller asked for a measurement involving
                // an Undef feature.

  // Build LOCAL SurfaceFeatures from the Qt POD. These are scratch objects
  // local to this call -- they own no libslic3r memory (their back-pointer
  // members default to null). Safe to construct, measure, and destroy here.
  //
  // Phase 114 REVIEW H1 FIX: the QtFeature points are ALREADY in world
  // space (getFeature passes worldTransform to m->get_feature which
  // internally translates the output SurfaceFeature to world space at
  // Measure.cpp:583-594, and scrubSurfaceFeature copies those world-space
  // points verbatim). So buildLocalSurfaceFeature reconstructs from
  // world-space points. Do NOT translate again -- the previous code called
  // sfA.translate(worldTransform) which double-applied the transform,
  // corrupting the A->B readout for any rotated/scaled object. The
  // regression test masked this by using Transform3d::Identity().
  Slic3r::Measure::SurfaceFeature sfA = buildLocalSurfaceFeature(a);
  Slic3r::Measure::SurfaceFeature sfB = buildLocalSurfaceFeature(b);

  // No additional world-transform application needed -- the QtFeature
  // points are already world-space (see H1 FIX comment above).

  // get_measurement (Measure.hpp:183, Measure.cpp:832+) computes the
  // MeasurementResult. We copy the VALUES into QtMeasurement and let the
  // local SurfaceFeatures + the MeasurementResult die at scope exit.
  const Slic3r::Measure::MeasurementResult res =
      Slic3r::Measure::get_measurement(sfA, sfB);

  if (res.angle) {
    out.hasAngle = true;
    out.angleRad = float(res.angle->angle);  // AngleAndEdges.angle (L155)
  }
  if (res.distance_infinite) {
    out.hasPerpendicularDistance = true;
    out.perpendicularDistance =
        float(res.distance_infinite->dist);  // DistAndPoints.dist (L148)
  }
  if (res.distance_strict) {
    out.hasDirectDistance = true;
    out.directDistance =
        float(res.distance_strict->dist);  // DistAndPoints.dist (L148)
  }
  if (res.distance_xyz) {
    out.hasDistanceXyz = true;
    const Slic3r::Vec3d &xyz = *res.distance_xyz;  // MeasurementResult L171
    out.distanceXyz =
        QVector3D(float(xyz.x()), float(xyz.y()), float(xyz.z()));
  }
  out.valid = true;
  return out;
}

} // namespace OWzx
#endif // HAS_LIBSLIC3R
