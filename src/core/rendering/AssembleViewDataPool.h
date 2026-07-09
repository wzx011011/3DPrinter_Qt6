#pragma once

// Phase 93 (ASMROUTE-02): AssembleView data pool mirroring upstream
// AssembleViewDataID / AssembleViewDataPool / AssembleViewDataBase
// (third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmosCommon.hpp:268,274,299).
// Caches per-object data needed by the AssembleView gizmos, isolated from
// Prepare/Preview state by the EditorViewModel m_activeCanvasType==2 gate
// (mirrors upstream GLGizmosManager.cpp:427-431 running in the
// AssembleView-only gizmo update). Pure data — no libslic3r, no QRhi — so it
// stays unit-testable like AssemblyMeasureGeometry (same directory).
//
// Scope simplification (documented in 93-CONTEXT.md + 93-01-SUMMARY.md):
//   - AssembleViewDataID enum (bitmask): FULL port (GLGizmosCommon.hpp:268-272).
//   - AssembleViewDataBase lifecycle (update()/release()/is_valid()): FULL
//     port (GLGizmosCommon.hpp:299-332).
//   - AssembleViewDataPool::update(required) + getters: FULL port of the
//     release-what-is-not-used semantics (GLGizmosCommon.cpp:441-468).
//   - ModelObjectsInfo resource: MATCHING port — caches per-object info
//     (source-object index -> bounds) sourced from the EditorViewModel mesh
//     blob instead of upstream model->objects. The bounds source is unchanged
//     from Phase 92's selectedVolumeBoundsForAssemblyMeasure(); Phase 93
//     formalizes it as a cached pool resource.
//   - ModelObjectsClipper resource (cut-plane): DEFERRED. The enum slot is
//     reserved (bit 4) but the resource is intentionally NOT registered.
//     Needs per-volume MeshClipper + indexed_triangle_set — the same hard
//     dependency that blocked the full Phase 92 feature-picking engine. It is
//     documented as a future resource that slots into the same pool API
//     without an API change (just register it in the ctor + add a getter).

#include <QList>

// PrepareSceneData.h is lightweight (Qt Core only — no QRhi, no libslic3r);
// included so the cached per-object info can carry ModelBounds values
// (mirrors the AssemblyMeasureGeometry.h include pattern in this directory).
#include "qml_gui/Renderer/PrepareSceneData.h"

// Bitmask of AssembleView resources (GLGizmosCommon.hpp:268-272).
enum class AssembleViewDataID {
  None = 0,
  ModelObjectsInfo = 1 << 0,
  // Reserved for the deferred ModelObjectsClipper resource (needs per-volume
  // ITS). Kept at bit 4 to match upstream's enum shape exactly so a future
  // registration drops in without renumbering.
  ModelObjectsClipper = 1 << 4,
};

// Base class for a wrapper object managing a single pool resource
// (GLGizmosCommon.hpp:299-332). Each registered AssembleViewDataID value
// (save None) is backed by one AssembleViewDataBase subclass. The lifecycle
// is the pool's API: update() refreshes the resource and marks it valid;
// release() clears it and marks it invalid; is_valid() reports state.
class AssembleViewDataBase {
public:
  AssembleViewDataBase() = default;
  virtual ~AssembleViewDataBase() = default;

  // Update the resource (GLGizmosCommon.hpp:308).
  void update() { on_update(); m_isValid = true; }

  // Release any data stored internally (GLGizmosCommon.hpp:311).
  void release() { on_release(); m_isValid = false; }

  // Whether the resource is currently maintained (GLGizmosCommon.hpp:314).
  bool is_valid() const { return m_isValid; }

protected:
  virtual void on_update() = 0;
  virtual void on_release() = 0;

private:
  bool m_isValid = false;
};

// Per-object info cached for the AssembleView gizmos. One entry per source
// object. The bounds mirror the renderer's per-batch AABB union (same source
// as Phase 92's selectedVolumeBoundsForAssemblyMeasure()). center /
// longest-axis direction are derived alongside so consumers (the Assembly
// measure overlay) can read them without recomputing.
struct AssembleViewObjectInfo {
  int sourceObjectIndex = -1;
  PrepareSceneData::ModelBounds bounds;
};

// ModelObjectsInfo resource (GLGizmosCommon.hpp:336-353 +
// AssembleViewDataObjects::ModelObjectsInfo). The minimal-port seam: upstream
// on_update() pulls model->objects; here on_update() is a no-op because the
// caller (EditorViewModel::refreshAssembleViewDataPool) sets the objects via
// setObjects() before calling the pool's update(), since the bounds source is
// the viewmodel's mesh blob, not model->objects. This keeps the pool free of
// libslic3r while preserving the update()/release()/is_valid() contract.
class AssembleViewModelObjectsInfo : public AssembleViewDataBase {
public:
  const QList<AssembleViewObjectInfo> &objects() const { return m_objects; }

  // Minimal-port seam: populate the cache before the pool calls update().
  void setObjects(QList<AssembleViewObjectInfo> objects) { m_objects = std::move(objects); }

protected:
  void on_update() override { /* no-op minimal port — caller pre-fills via setObjects() */ }
  void on_release() override { m_objects.clear(); }

private:
  QList<AssembleViewObjectInfo> m_objects;
};

// AssembleViewDataPool (GLGizmosCommon.hpp:274-295 + GLGizmosCommon.cpp:433-468).
// update(required) refreshes resources in the required bitmask and releases
// the rest; getters return a valid resource or nullptr. The minimal port uses
// a direct stack member for the one registered resource (ModelObjectsInfo)
// instead of upstream's std::map<...unique_ptr> + dynamic_cast — the API and
// semantics are identical for a single registered resource. The
// ModelObjectsClipper slot is reserved in the enum but not registered here
// (deferred — see file header).
class AssembleViewDataPool {
public:
  AssembleViewDataPool();

  // Update all registered resources and release what is not used
  // (GLGizmosCommon.cpp:441-451). Accepts a bitmask of currently required
  // resources; resources whose bit is set are refreshed, the rest released.
  void update(AssembleViewDataID required);

  // Getter for the ModelObjectsInfo resource (GLGizmosCommon.cpp:454-459).
  // Returns nullptr when the resource is not currently valid (i.e. when the
  // last update() was called with a bitmask that did not include
  // ModelObjectsInfo, or update() has not been called yet).
  AssembleViewModelObjectsInfo *model_objects_info() const;

private:
  // Stack-owned registered resource (minimal port — upstream uses a map of
  // unique_ptrs so dynamic_cast getters work for arbitrary resources; with a
  // single registered resource a direct member is equivalent and avoids the
  // map + dynamic_cast machinery). Mutable so the const getter
  // model_objects_info() can return a mutable resource pointer, mirroring
  // upstream's const-getter / mutable-resource semantics.
  mutable AssembleViewModelObjectsInfo m_modelObjectsInfo;
};
