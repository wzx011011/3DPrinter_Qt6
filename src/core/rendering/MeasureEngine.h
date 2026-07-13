#pragma once

// ===========================================================================
// Phase 114 (MEASURE-03): per-volume Measure::Measuring instantiation +
// get_feature wiring + SurfaceFeature boundary scrubbing (pitfall 6).
//
// Upstream reference:
//   third_party/OrcaSlicer/src/libslic3r/Measure.hpp:119-200 -- the
//   Measure::Measuring class (pimpl over MeasuringImpl) + SurfaceFeature +
//   DistAndPoints + AngleAndEdges + MeasurementResult + get_measurement.
//
//   third_party/OrcaSlicer/src/libslic3r/Measure.cpp:525-594 --
//   MeasuringImpl::get_feature: builds the plane feature list once per
//   plane (extract_features), finds the closest feature to the cursor
//   within feature_hover_limit (0.5 mm, Measure.cpp:38), and returns the
//   feature with world_tran applied.
//
//   third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoMeasure.cpp:1990-
//   2048 -- the upstream readout table that drives the Qt6 readout API
//   (angle / perpendicular distance / direct distance / distance XYZ).
//
// INSTANTIATE, NOT REIMPLEMENT (ME-01):
//   This class wraps `Measure::Measuring` directly. It does NOT reimplement
//   the plane extraction / feature detection / closest-feature math. The
//   engine builds one `Measuring` per volume from the Phase 112 ITS
//   (ProjectServiceMock::volumeMeshIts -- the ITS source injected as a
//   callable), caches it alongside the Phase 113 SceneRaycaster, and
//   invalidates on model change (lazy + invalidated on model change).
//
// SURFACEFEATURE BOUNDARY SCRUBBING (ME-03 / pitfall 6):
//   Measure::SurfaceFeature (Measure.hpp:27-112) carries RAW libslic3r
//   back-pointers as public members (the volume handle + the plane-facet
//   index vector handle, Measure.hpp:95-96), plus a shared_ptr to
//   world_plane_features and origin_surface_feature. The SurfaceFeature
//   copy ctor (Measure.hpp:36-43) shallow-copies these -- so copying a
//   SurfaceFeature across the libslic3r->Qt boundary with those back-
//   pointers still aimed at libslic3r-owned memory is a latent UAF
//   (pitfall 6).
//
//   This class NEVER lets a libslic3r SurfaceFeature escape into Qt. Every
//   accessor extracts the measurement VALUES (feature type, point(s),
//   radius, plane index, normal) into Qt-owned POD types (QtFeature /
//   QVector3D / float / int) and returns those. The libslic3r SurfaceFeature
//   is constructed, read, and destroyed entirely WITHIN the .cpp accessor
//   body. A grep for the raw back-pointer member names in this file MUST
//   return ZERO -- no libslic3r back-pointer is stored on the Qt side.
//
// RELATIONSHIP TO AssemblyMeasureGeometry (ME-05):
//   AssemblyMeasureGeometry (Phase 92, AABB-center distance + longest-axis
//   angle) stays as a COARSE FALLBACK for the Assembly-mode multi-volume
//   case (two volumes, no per-triangle pick needed -- the AABB center is
//   the truth). MeasureEngine is the PRECISE single-feature path: it takes
//   one Phase 113 hit (facetIdx + point + worldTransform) and returns the
//   real picked feature (point / edge / circle / plane) + the per-feature
//   measurement readouts. The two do not overlap -- AssemblyMeasureGeometry
//   is volume-vs-volume, MeasureEngine is cursor-vs-feature. Both remain.
//
// The whole class is #ifdef HAS_LIBSLIC3R -- the Measure::Measuring /
// SurfaceFeature / indexed_triangle_set types only exist when libslic3r is
// built (Phase 112 ITS accessor + Phase 113 raycaster have the same guard).
// ===========================================================================

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include <QVector3D>
#include <QString>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Point.hpp>  // Vec3d, Transform3d (via Point.hpp -> Eigen)
struct indexed_triangle_set;
namespace Slic3r {
namespace Measure {
class Measuring;  // forward-declared; the complete type (Measure.hpp) is
                  // pulled in only by the .cpp. Keeps this header free of
                  // the Measure.hpp transitive includes (CGAL etc.).
}
} // namespace Slic3r
#endif

#ifdef HAS_LIBSLIC3R
namespace OWzx {

// Qt-owned mirror of Measure::SurfaceFeatureType (Measure.hpp:16-22).
// Values match the upstream enum exactly so a future bridge can cast if
// needed. Undef is the "no feature / ray missed" sentinel.
enum class FeatureKind : int
{
  Undef = 0,
  Point = 1,
  Edge = 2,
  Circle = 4,
  Plane = 8
};

// Qt-owned POD mirroring the VALUE subset of Measure::SurfaceFeature
// (Measure.hpp:27-112). This is what crosses the libslic3r->Qt boundary:
// floats + an enum + QVector3D -- NO raw libslic3r pointers. The
// translation from SurfaceFeature to QtFeature happens inside the
// MeasureEngine accessor BEFORE the SurfaceFeature goes out of scope
// (pitfall 6 scrubbing).
//
// Field mapping (Measure.hpp accessors):
//   Point  -> kind=Point,  pt1 = get_point()                       (L59)
//   Edge   -> kind=Edge,   pt1 = get_edge().first,
//                          pt2 = get_edge().second                  (L61)
//   Circle -> kind=Circle, pt1 = center, pt2 = normal,
//                          radius = get_circle() radius             (L64)
//   Plane  -> kind=Plane,  pt1 = normal, pt2 = point-in-plane,
//                          planeIndex = get_plane() first           (L67)
struct QtFeature
{
  FeatureKind kind = FeatureKind::Undef;
  QVector3D   pt1;          // Point / Edge-start / Circle-center / Plane-normal
  QVector3D   pt2;          // Edge-end / Circle-normal / Plane-point-in-plane
  float       radius = 0.0f; // Circle only
  int         planeIndex = -1; // Plane only (index into Measuring planes)
  bool        valid = false;  // false if no feature (ray missed / undef)
};

// Qt-owned mirror of Measure::MeasurementResult
// (Measure.hpp:167-180). Mirrors upstream GLGizmoMeasure.cpp:1990-2048
// readouts: angle (rad), perpendicular distance (distance_infinite),
// direct distance (distance_strict), distance XYZ. Each optional is
// represented by a bool has flag + a value -- avoids std::optional in the
// Qt-facing POD (cleaner QML bridge).
struct QtMeasurement
{
  // Angle between two features (AngleAndEdges.angle, Measure.hpp:155).
  bool  hasAngle = false;
  float angleRad = 0.0f;

  // Perpendicular distance (DistAndPoints from distance_infinite,
  // Measure.hpp:169). Upstream labels this "Perpendicular distance" when
  // strict also exists, else "Distance" (GLGizmoMeasure.cpp:2009).
  bool  hasPerpendicularDistance = false;
  float perpendicularDistance = 0.0f;

  // Direct distance (DistAndPoints from distance_strict, Measure.hpp:170).
  // Upstream labels this "Direct distance" (GLGizmoMeasure.cpp:2022).
  bool  hasDirectDistance = false;
  float directDistance = 0.0f;

  // Distance XYZ (MeasurementResult.distance_xyz, Measure.hpp:171).
  bool    hasDistanceXyz = false;
  QVector3D distanceXyz;

  bool valid = false; // false if no measurement was computed
};

// MeasureEngine -- per-volume Measure::Measuring cache + get_feature wiring.
//
// The ITS source is an injected callable (VolumeMeshItsFn) so this class has
// no hard dependency on ProjectServiceMock -- it just needs something that
// returns shared_ptr<const indexed_triangle_set> for (object, volume). In
// production this is the SAME accessor SceneRaycaster (Phase 113) uses
// (ProjectServiceMock::volumeMeshIts). Reusing one accessor instance across
// the raycaster and the engine keeps the per-volume ITS identity consistent.
//
// CACHE (ME-01): one Measure::Measuring per (objectIndex, volumeIndex),
// built lazily on the first getFeature() call and reconstructed ONLY when
// invalidate()/invalidateVolume() is called (the model change signal). No
// reconstruction happens on cursor move (pitfall 6/7 -- the MeasuringImpl
// ctor indexes the mesh; rebuilding per-frame melts the framerate).
class MeasureEngine
{
public:
  // Callable: (objectIndex, volumeIndex) -> shared_ptr<const ITS> (nullptr
  // if invalid / no mesh). Same type as SceneRaycaster::VolumeMeshItsFn
  // (Phase 113); the production wiring passes the same accessor instance.
  using VolumeMeshItsFn =
      std::function<std::shared_ptr<const indexed_triangle_set>(int, int)>;

  explicit MeasureEngine(VolumeMeshItsFn itsSource)
      : m_itsSource(std::move(itsSource))
  {}

  // Extract the picked feature at a Phase 113 ray hit (ME-02).
  //
  // Inputs come straight from SceneRaycasterHit (Phase 113): objectIndex,
  // volumeIndex, facetIdx (the ITS triangle index), point (WORLD-space hit
  // position), worldTransform (the volume's world transform). The engine
  // transforms the world point back into mesh-local space before calling
  // Measuring::get_feature, then the returned SurfaceFeature is read into a
  // QtFeature and the SurfaceFeature is destroyed -- all within this call.
  // The QtFeature carries no libslic3r pointer (pitfall 6 scrubbing).
  //
  // onlySelectPlane mirrors Measuring::get_feature's last arg
  // (Measure.hpp:128): true forces the plane (no edge/point/circle
  // sniffing). The Phase 115 snap UX will toggle this; Phase 114 uses the
  // default (false = full feature sniff).
  //
  // Returns QtFeature{valid=false} when the ITS source has no mesh for the
  // (object, volume) pair, the facet index is out of range, or
  // get_feature returned nullopt.
  QtFeature getFeature(int objectIndex,
                       int volumeIndex,
                       std::size_t facetIdx,
                       const Slic3r::Vec3d &worldPoint,
                       const Slic3r::Transform3d &worldTransform,
                       bool onlySelectPlane = false);

  // Compute the measurement readouts between two QtFeatures (ME-04).
  //
  // This bridges Measure::get_measurement (Measure.hpp:183) to the Qt layer
  // WITHOUT exposing a SurfaceFeature: the two QtFeatures are rebuilt into
  // LOCAL SurfaceFeatures inside the .cpp (with their back-pointer members
  // defaulted to null -- no libslic3r ownership), get_measurement runs,
  // and the MeasurementResult values are copied into the Qt-owned
  // QtMeasurement BEFORE the local SurfaceFeatures go out of scope.
  //
  // The world transform is applied to both features so the measurement is
  // in world space (mirrors GLGizmoMeasure.cpp world_tran handling).
  QtMeasurement measureFeatures(const QtFeature &a,
                                const QtFeature &b,
                                const Slic3r::Transform3d &worldTransform);

  // Drop every cached Measuring. Call on model change (load / cut /
  // boolean / simplify / drill / orient / arrange / delete-volume). The
  // next getFeature() lazily rebuilds only the volumes it touches.
  void invalidate() { m_cache.clear(); }

  // Drop the cache entry for a single volume (granular invalidation keeps
  // the cost proportional to the change).
  void invalidateVolume(int objectIndex, int volumeIndex)
  {
    m_cache.erase(volumeKey(objectIndex, volumeIndex));
  }

  // Number of currently-cached Measuring instances (diagnostic / test hook).
  std::size_t cachedMeasuringCount() const { return m_cache.size(); }

private:
  using Key = std::pair<int, int>;

  static Key volumeKey(int objectIndex, int volumeIndex)
  {
    return std::make_pair(objectIndex, volumeIndex);
  }

  // Lazily fetch-or-build the Measure::Measuring for one volume. Returns
  // nullptr if the ITS source has no mesh for that (object, volume) pair.
  std::shared_ptr<Slic3r::Measure::Measuring>
  measuringFor(int objectIndex, int volumeIndex);

  VolumeMeshItsFn m_itsSource;
  // std::map (not unordered_map) so we don't need a std::hash<std::pair>
  // specialization. Cache size is bounded by the touched-volume count.
  std::map<Key, std::shared_ptr<Slic3r::Measure::Measuring>> m_cache;
};

} // namespace OWzx
#endif // HAS_LIBSLIC3R
