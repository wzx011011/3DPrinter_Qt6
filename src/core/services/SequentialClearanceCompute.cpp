#include "core/services/SequentialClearanceCompute.h"

#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"

#ifdef HAS_LIBSLIC3R
#include <libslic3r/ClipperUtils.hpp>
#include <libslic3r/Flow.hpp>
#include <libslic3r/Geometry.hpp>
#include <libslic3r/Model.hpp>
#include <libslic3r/Point.hpp>
#include <libslic3r/Print.hpp>       // MAX_OUTER_NOZZLE_DIAMETER + clearance rule source
#include <libslic3r/PrintConfig.hpp>
#include <algorithm>
#include <cmath>
#endif

namespace SequentialClearanceCompute
{
namespace
{
#ifdef HAS_LIBSLIC3R
  // Read helpers tolerant of the generic-enum post-full_print_config state:
  // getInt()/getFloat() are virtual on ConfigOption, so both the restored
  // native enum options and ConfigOptionEnumsGeneric answer correctly. The
  // single-arg option(key) is the CONST overload (Config.hpp:2042); the
  // two-arg form is non-const and rejects a const config.
  double configFloat(const Slic3r::DynamicPrintConfig &config, const char *key, double fallback)
  {
    const Slic3r::ConfigOption *option = config.option(key);
    return option ? double(option->getFloat()) : fallback;
  }

  int configInt(const Slic3r::DynamicPrintConfig &config, const char *key, int fallback)
  {
    const Slic3r::ConfigOption *option = config.option(key);
    return option ? option->getInt() : fallback;
  }

  double configFloatsFront(const Slic3r::DynamicPrintConfig &config, const char *key, double fallback)
  {
    const Slic3r::ConfigOptionFloats *values = config.option<Slic3r::ConfigOptionFloats>(key);
    return (values && !values->values.empty()) ? double(values->values.front()) : fallback;
  }

  // Resolves a ConfigOptionFloatOrPercent the way Flow::new_from_config_width
  // does (percentages against the nozzle diameter, Flow.cpp:111-129).
  double resolvedWidthValue(const Slic3r::DynamicPrintConfig &config, const char *key, double nozzleDiameter)
  {
    const Slic3r::ConfigOptionFloatOrPercent *option = config.option<Slic3r::ConfigOptionFloatOrPercent>(key);
    if (!option)
      return 0.0;
    return option->percent ? option->value * 0.01 * nozzleDiameter : double(option->value);
  }

  // Print::object_skirt_offset (Print.cpp:2959-2979) evaluated from resolved
  // config values only -- no Print object exists on the drag-preview path
  // (margin_height = 0, the value update_sequential_clearance passes).
  // The skirt flow width mirrors Print::skirt_flow (Print.cpp:1689-1704):
  // initial_layer_line_width at the first layer height; the upstream
  // m_objects.front()->config() fallbacks (line_width / support_filament
  // nozzle pick) are approximated with the print-config line_width and the
  // first nozzle -- identical for single-extruder profiles.
  double objectSkirtOffsetMm(const ConfigValues &config, bool allObjectsShort)
  {
    if (config.skirtLoops <= 0.0 || !config.perObjectSkirt)
      return 0.0;

    double flowWidth = config.initialLayerLineWidth;
    if (flowWidth <= 0.0)
      flowWidth = config.lineWidthFallback;
    if (flowWidth <= 0.0)
      flowWidth = double(Slic3r::Flow::auto_extrusion_width(Slic3r::frPerimeter, float(config.nozzleDiameter)));
    if (config.initialLayerPrintHeight <= 0.0)
      return 0.0;

    const Slic3r::Flow skirtFlow(float(flowWidth), float(config.initialLayerPrintHeight),
                                 float(config.nozzleDiameter));
    const double skirtWidth = skirtFlow.width() + (config.skirtLoops - 1.0) * skirtFlow.spacing();

    if (allObjectsShort)
      return config.skirtDistance + skirtWidth;
    if (config.draftShieldEnabled || config.skirtHeight * config.maxLayerHeight > config.nozzleHeight)
      return config.skirtDistance + config.initialLayerLineWidth;
    if (config.skirtDistance + skirtWidth > config.extruderClearanceRadius / 2.0)
      return config.skirtDistance + skirtWidth - config.extruderClearanceRadius / 2.0;
    return 0.0;
  }
#endif
} // namespace

ConfigValues resolveConfigValues(ProjectServiceMock *projectService,
                                 int plateIndex,
                                 const QHash<QString, QVariant> &mergedPreset,
                                 double bedMinX, double bedMinY,
                                 double bedMaxX, double bedMaxY)
{
  ConfigValues values;
  values.bedMinX = bedMinX;
  values.bedMinY = bedMinY;
  values.bedMaxX = bedMaxX;
  values.bedMaxY = bedMaxY;
#ifdef HAS_LIBSLIC3R
  if (!projectService)
    return values;
  // Same merge sequence as the SliceService worker (full_print_config
  // defaults + preset injection + per-plate overrides + normalize_fdm).
  const Slic3r::DynamicPrintConfig config =
      SliceService::makeResolvedPlateConfig(mergedPreset,
                                            projectService->plateDynamicConfig(plateIndex));
  values.extruderClearanceRadius = configFloat(config, "extruder_clearance_radius", 0.0);
  values.extruderClearanceHeightToLid = configFloat(config, "extruder_clearance_height_to_lid", 0.0);
  values.extruderClearanceHeightToRod = configFloat(config, "extruder_clearance_height_to_rod", 0.0);
  values.printableHeight = configFloat(config, "printable_height", 0.0);
  values.nozzleHeight = configFloat(config, "nozzle_height", 0.0);
  values.skirtLoops = double(configInt(config, "skirt_loops", 0));
  values.perObjectSkirt = configInt(config, "skirt_type", 0) == int(Slic3r::stPerObject);
  values.skirtDistance = configFloat(config, "skirt_distance", 0.0);
  values.draftShieldEnabled = configInt(config, "draft_shield", 0) == int(Slic3r::dsEnabled);
  values.skirtHeight = double(configInt(config, "skirt_height", 0));
  values.maxLayerHeight = configFloatsFront(config, "max_layer_height", 0.0);
  values.nozzleDiameter = configFloatsFront(config, "nozzle_diameter", 0.4);
  values.initialLayerLineWidth = resolvedWidthValue(config, "initial_layer_line_width", values.nozzleDiameter);
  values.lineWidthFallback = resolvedWidthValue(config, "line_width", values.nozzleDiameter);
  values.initialLayerPrintHeight = configFloat(config, "initial_layer_print_height", 0.0);
#else
  Q_UNUSED(projectService);
  Q_UNUSED(plateIndex);
  Q_UNUSED(mergedPreset);
#endif
  return values;
}

HullCache captureHullCache(ProjectServiceMock *projectService, const ConfigValues &config)
{
  HullCache cache;
#ifdef HAS_LIBSLIC3R
  if (!projectService)
    return cache;
  const Slic3r::Model *model = projectService->rawModel();
  if (!model)
    return cache;
  const QList<int> plateObjects = projectService->currentPlateObjectIndices();
  if (plateObjects.isEmpty())
    return cache;

  // Short-object rule input. Upstream Print::is_all_objects_are_short
  // (Print.hpp:981-984) reads the PrintObject heights; without Print::apply
  // the model instance max z evaluates the same predicate.
  std::vector<double> objectMaxZ;
  objectMaxZ.reserve(size_t(plateObjects.size()));
  for (int objectIndex : plateObjects)
  {
    const bool validObject =
        objectIndex >= 0 && objectIndex < int(model->objects.size()) && model->objects[size_t(objectIndex)]
        && !model->objects[size_t(objectIndex)]->instances.empty();
    objectMaxZ.push_back(validObject
                             ? model->objects[size_t(objectIndex)]->get_instance_max_z(0)
                             : 0.0);
  }
  bool allObjectsShort = true;
  for (double z : objectMaxZ)
  {
    if (z >= config.nozzleHeight)
    {
      allObjectsShort = false;
      break;
    }
  }
  cache.allObjectsShort = allObjectsShort;

  // Shrink rule of Print::sequential_print_horizontal_clearance_valid
  // (Print.cpp:593) replicated from update_sequential_clearance
  // (GLCanvas3D.cpp:5231-5236). The -0.1mm relaxes an exact arrangement fit.
  const double skirtOffset = objectSkirtOffsetMm(config, allObjectsShort);
  const double shrinkMm = allObjectsShort
                              ? (std::max)(0.5 * double(MAX_OUTER_NOZZLE_DIAMETER), skirtOffset) - 0.1
                              : 0.5 * config.extruderClearanceRadius + skirtOffset - 0.1;
  // scale_ is a MACRO (libslic3r.h:91), so it cannot be namespace-qualified.
  const float shrinkFactor = scale_(shrinkMm);
  const double miterLimit = scale_(0.1);

  cache.hulls.reserve(plateObjects.size());
  for (int objectIndex : plateObjects)
  {
    HullPolygon hullOut;
    const bool validObject =
        objectIndex >= 0 && objectIndex < int(model->objects.size()) && model->objects[size_t(objectIndex)]
        && !model->objects[size_t(objectIndex)]->instances.empty();
    if (validObject)
    {
      const Slic3r::ModelObject *modelObject = model->objects[size_t(objectIndex)];
      const Slic3r::ModelInstance *instance0 = modelObject->instances.front();
      // GLCanvas3D.cpp:5239-5243: hull of the printable volumes projected to
      // XY with the FIRST instance transform (z offset kept, world XY zeroed).
      const Slic3r::Transform3d trafo = Slic3r::Geometry::assemble_transform(
          Slic3r::Vec3d{0.0, 0.0, instance0->get_offset().z()},
          instance0->get_rotation(),
          instance0->get_scaling_factor(),
          instance0->get_mirror());
      const Slic3r::Polygon hullNoOffset = modelObject->convex_hull_2d(trafo);
      // GLCanvas3D.cpp:5244-5249: clipper may return an empty offset
      // (STUDIO-2452); fall back to the unshrunk hull like upstream.
      const auto offsetResult = Slic3r::offset(hullNoOffset, shrinkFactor, Slic3r::ClipperLib::jtRound, miterLimit);
      const Slic3r::Polygon &hull2d = !offsetResult.empty() ? offsetResult.front() : hullNoOffset;
      hullOut.xs.reserve(hull2d.points.size());
      hullOut.ys.reserve(hull2d.points.size());
      for (const Slic3r::Point &point : hull2d.points)
      {
        hullOut.xs.push_back(Slic3r::unscale<double>(point.x()));
        hullOut.ys.push_back(Slic3r::unscale<double>(point.y()));
      }
    }
    cache.hulls.push_back(std::move(hullOut));
  }
  cache.valid = true;
#else
  Q_UNUSED(projectService);
  Q_UNUSED(config);
#endif
  return cache;
}

std::vector<std::vector<InstancePose>> collectInstancePoses(ProjectServiceMock *projectService)
{
  std::vector<std::vector<InstancePose>> poses;
#ifdef HAS_LIBSLIC3R
  if (!projectService)
    return poses;
  const Slic3r::Model *model = projectService->rawModel();
  if (!model)
    return poses;
  const QList<int> plateObjects = projectService->currentPlateObjectIndices();
  poses.reserve(plateObjects.size());
  for (int objectIndex : plateObjects)
  {
    poses.emplace_back();
    if (objectIndex < 0 || objectIndex >= int(model->objects.size()))
      continue;
    const Slic3r::ModelObject *modelObject = model->objects[size_t(objectIndex)];
    if (!modelObject || modelObject->instances.empty())
      continue;
    // Upstream applies only the z-rotation DELTA against the object's first
    // instance per displacement (GLCanvas3D.cpp:5271-5274); scale/mirror stay
    // baked in the cached hull exactly like upstream.
    const double rotationZ0 = modelObject->instances.front()->get_rotation().z();
    poses.back().reserve(modelObject->instances.size());
    int instanceIndex = 0;
    for (const Slic3r::ModelInstance *instance : modelObject->instances)
    {
      InstancePose pose;
      pose.x = instance->get_offset().x();
      pose.y = instance->get_offset().y();
      pose.rotationZDelta = instance->get_rotation().z() - rotationZ0;
      pose.instanceMaxZ = modelObject->get_instance_max_z(size_t(instanceIndex));
      ++instanceIndex;
      poses.back().push_back(pose);
    }
  }
#else
  Q_UNUSED(projectService);
#endif
  return poses;
}

SequentialPrintClearance runCompute(const HullCache &cache,
                                    const ConfigValues &config,
                                    const std::vector<std::vector<InstancePose>> &poses)
{
  SequentialPrintClearance out;
#ifdef HAS_LIBSLIC3R
  if (!cache.valid || poses.size() != cache.hulls.size())
    return out;

  // GLCanvas3D.cpp:5286-5290 height_info: instance height + bbox + hull.
  struct HeightInfo
  {
    double instanceHeight;
    Slic3r::BoundingBox boundingBox;
    Slic3r::Polygon hullPolygon;
  };
  Slic3r::Polygons polygons;
  std::vector<std::pair<Slic3r::Polygon, float>> heightPolygons;
  std::vector<HeightInfo> boxes;
  const Slic3r::Point plateMin(Slic3r::scaled<double>(config.bedMinX), Slic3r::scaled<double>(config.bedMinY));
  const Slic3r::Point plateMax(Slic3r::scaled<double>(config.bedMaxX), Slic3r::scaled<double>(config.bedMaxY));

  for (size_t i = 0; i < poses.size(); ++i)
  {
    const HullPolygon &hull = cache.hulls[i];
    if (hull.xs.empty() || hull.xs.size() != hull.ys.size())
      continue;
    for (const InstancePose &pose : poses[i])
    {
      Slic3r::Points instPts;
      instPts.reserve(hull.xs.size());
      const double ca = std::cos(pose.rotationZDelta);
      const double sa = std::sin(pose.rotationZDelta);
      for (size_t j = 0; j < hull.xs.size(); ++j)
      {
        const double rx = hull.xs[j] * ca - hull.ys[j] * sa;
        const double ry = hull.xs[j] * sa + hull.ys[j] * ca;
        instPts.emplace_back(Slic3r::scaled<double>(rx + pose.x), Slic3r::scaled<double>(ry + pose.y));
      }
      Slic3r::Polygon convexHull(std::move(instPts));
      const Slic3r::BoundingBox boundingBox = convexHull.bounding_box();
      // Current-plate overlap gate (upstream plate_bb.overlap(bounding_box),
      // BoundingBox::overlap semantics -- touch counts as overlap).
      if (boundingBox.max.x() < plateMin.x() || boundingBox.min.x() > plateMax.x()
          || boundingBox.max.y() < plateMin.y() || boundingBox.min.y() > plateMax.y())
        continue;
      boxes.push_back({pose.instanceMaxZ, boundingBox, convexHull});
      polygons.emplace_back(convexHull);
    }
  }

  // Print-order sort (GLCanvas3D.cpp:5305-5320): y-band intersection first,
  // then x (leftmost prints first); non-interlaced orders by min y.
  std::sort(boxes.begin(), boxes.end(), [](const HeightInfo &l, const HeightInfo &r)
  {
    const auto ly1 = l.boundingBox.min.y();
    const auto ly2 = l.boundingBox.max.y();
    const auto ry1 = r.boundingBox.min.y();
    const auto ry2 = r.boundingBox.max.y();
    const auto interMin = (std::max)(ly1, ry1);
    const auto interMax = (std::min)(ly2, ry2);
    const auto lx = l.boundingBox.min.x();
    const auto rx = r.boundingBox.min.x();
    if (interMax - interMin > 0)
      return (lx < rx) || ((lx == rx) && (ly1 < ry1));
    return ly1 < ry1;
  });

  // Height rules (GLCanvas3D.cpp:5349-5377): the last printed object may use
  // the full printable_height, earlier ones need the lid clearance; any y
  // overlap with a LATER object tightens the requirement to the rod
  // clearance. Violations emit a height polygon at the required height.
  const int boxCount = int(boxes.size());
  for (int k = 0; k < boxCount; ++k)
  {
    double height = (k == boxCount - 1) ? config.printableHeight : config.extruderClearanceHeightToLid;
    for (int i = k + 1; i < boxCount; ++i)
    {
      const auto interMin = (std::max)(boxes[k].boundingBox.min.y(), boxes[i].boundingBox.min.y());
      const auto interMax = (std::min)(boxes[k].boundingBox.max.y(), boxes[i].boundingBox.max.y());
      if (interMax - interMin > 0)
      {
        height = config.extruderClearanceHeightToRod;
        break;
      }
    }
    if (height < boxes[k].instanceHeight)
      heightPolygons.emplace_back(boxes[k].hullPolygon, float(height));
  }

  // Same packer the Print::validate worker uses -> identical stream format.
  out = SliceService::packSequentialClearance(polygons, heightPolygons);
#else
  Q_UNUSED(config);
#endif
  return out;
}
} // namespace SequentialClearanceCompute
