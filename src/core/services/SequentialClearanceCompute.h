#pragma once

// P15.11: drag-time sequential-print clearance preview, mirroring upstream
// GLCanvas3D::update_sequential_clearance (third_party/OrcaSlicer/src/slic3r/
// GUI/GLCanvas3D.cpp:5190-5388). The preview NEVER runs Print::apply: the
// per-object 2D hulls are captured once per drag sequence from the live
// Slic3r::Model (upstream SequentialPrintClearance::m_hull_2d_cache) and each
// debounced displacement recomputes only the cheap per-instance hull
// transforms plus the print-order / height rules.
//
// Threading contract (Frozen Decision 1 pattern, same as WipeTowerGeometry):
// captureHullCache / collectInstancePoses / resolveConfigValues run on the
// GUI thread against the live model and return PLAIN VALUES; runCompute
// executes on a QtConcurrent worker over those copies and produces the same
// SequentialPrintClearance value payload the Print::validate path packs, so
// no libslic3r type ever crosses a thread boundary.

#include "core/services/SliceService.h" // SequentialPrintClearance value payload

#include <QHash>
#include <QVariant>
#include <vector>

class ProjectServiceMock;

namespace SequentialClearanceCompute
{
// One cached per-object hull: outline points in millimeters (z dropped),
// first-instance transform baked in, clearance offset applied (upstream
// m_sequential_print_clearance.m_hull_2d_cache stores Pointf3s; xs/ys keep
// the same value-only shape without the libslic3r type).
struct HullPolygon
{
  std::vector<double> xs;
  std::vector<double> ys;
};

struct HullCache
{
  bool valid = false;           // false in mock mode / when the plate is empty
  bool allObjectsShort = false; // upstream Print::is_all_objects_are_short
  std::vector<HullPolygon> hulls;
};

// Resolved config values the preview rules consume (millimeters). Includes
// the inputs of Print::object_skirt_offset (Print.cpp:2959-2979) so the
// shrink term matches Print::sequential_print_horizontal_clearance_valid
// (Print.cpp:582-593) without constructing a Print.
struct ConfigValues
{
  double extruderClearanceRadius = 0.0;
  double extruderClearanceHeightToLid = 0.0;
  double extruderClearanceHeightToRod = 0.0;
  double printableHeight = 0.0;
  double nozzleHeight = 0.0;
  // Print::object_skirt_offset inputs.
  double skirtLoops = 0.0;             // skirt_loops
  bool perObjectSkirt = false;         // skirt_type == stPerObject
  double skirtDistance = 0.0;          // skirt_distance
  bool draftShieldEnabled = false;     // draft_shield == dsEnabled
  double skirtHeight = 0.0;            // skirt_height
  double maxLayerHeight = 0.0;         // max(max_layer_height)
  double nozzleDiameter = 0.0;         // max(nozzle_diameter)
  double initialLayerLineWidth = 0.0;  // initial_layer_line_width, percent resolved
  double lineWidthFallback = 0.0;      // line_width fallback, percent resolved
  double initialLayerPrintHeight = 0.0; // initial_layer_print_height
  // Current plate printable rectangle in world mm (upstream
  // PartPlate::get_bounding_box_crd overlap gate, GLCanvas3D.cpp:5290).
  double bedMinX = 0.0;
  double bedMinY = 0.0;
  double bedMaxX = 0.0;
  double bedMaxY = 0.0;
};

// Per-instance pose read fresh on every debounced tick (cheap transform reads
// on the GUI thread). Mirrors the upstream per-displacement
// instance_transforms cache + instance_height (GLCanvas3D.cpp:5265-5296).
struct InstancePose
{
  double x = 0.0;              // world offset X (mm)
  double y = 0.0;              // world offset Y (mm)
  double rotationZDelta = 0.0; // rotation z relative to the object's first instance (rad)
  double instanceMaxZ = 0.0;   // ModelObject::get_instance_max_z (mm)
};

// GUI-thread capture of the resolved config values (merged preset + plate
// overrides through the SAME SliceService merge path the slice worker uses).
// The bed rectangle arrives in world millimeters.
ConfigValues resolveConfigValues(ProjectServiceMock *projectService,
                                 int plateIndex,
                                 const QHash<QString, QVariant> &mergedPreset,
                                 double bedMinX, double bedMinY,
                                 double bedMaxX, double bedMaxY);

// GUI-thread capture of the per-object hull cache. Built ONCE per drag
// sequence (upstream m_sequential_print_clearance_first_displacement set at
// LeftDown, GLCanvas3D.cpp:4197). Applies the exact shrink rule of
// Print::sequential_print_horizontal_clearance_valid (Print.cpp:593).
HullCache captureHullCache(ProjectServiceMock *projectService, const ConfigValues &config);

// GUI-thread capture of the CURRENT instance poses of the current plate.
std::vector<std::vector<InstancePose>> collectInstancePoses(ProjectServiceMock *projectService);

// Pure value computation (QtConcurrent worker): transforms the cached hulls
// by the current poses, sorts the print order (y-band then x, upstream
// GLCanvas3D.cpp:5305-5320), applies the extruder_clearance_height_to_lid /
// _to_rod / printable_height rules (GLCanvas3D.cpp:5349-5377) and packs the
// SAME SequentialPrintClearance streams the Print::validate overlay renders.
SequentialPrintClearance runCompute(const HullCache &cache,
                                    const ConfigValues &config,
                                    const std::vector<std::vector<InstancePose>> &poses);
} // namespace SequentialClearanceCompute
