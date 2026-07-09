#include "AssembleViewDataPool.h"

// Phase 93 (ASMROUTE-02): AssembleView data pool implementation mirroring
// upstream GLGizmosCommon.cpp:433-468. See the header for the scope table
// (full enum/base/pool shape + ModelObjectsInfo; ModelObjectsClipper
// deferred). Pure data — Qt Core + PrepareSceneData only, no libslic3r, no
// QRhi — so it stays unit-testable like AssemblyMeasureGeometry.

AssembleViewDataPool::AssembleViewDataPool() = default;

void AssembleViewDataPool::update(AssembleViewDataID required)
{
  // Mirrors GLGizmosCommon.cpp:441-451: update resources in the required
  // bitmask, release the rest. Upstream iterates a std::map and dispatches
  // per-id; with a single registered resource the explicit branch is
  // equivalent and avoids the map + dynamic_cast machinery. When
  // ModelObjectsClipper is registered in the future it gets an analogous
  // branch guarded by the ModelObjectsClipper bit.
  if (int(required) & int(AssembleViewDataID::ModelObjectsInfo))
    m_modelObjectsInfo.update();
  else if (m_modelObjectsInfo.is_valid())
    m_modelObjectsInfo.release();
}

AssembleViewModelObjectsInfo *AssembleViewDataPool::model_objects_info() const
{
  // Mirrors GLGizmosCommon.cpp:454-459: return the resource when it is valid,
  // nullptr otherwise. Callers must null-check (the Assembly gizmo only
  // consumes the info when on AssembleView, where the pool is kept valid).
  return m_modelObjectsInfo.is_valid() ? &m_modelObjectsInfo : nullptr;
}

AssembleViewModelObjectsInfo *AssembleViewDataPool::model_objects_info_for_refresh()
{
  // Minimal-port refresh seam (see header). Returns the mutable resource
  // unconditionally so the owner can pre-fill it before update().
  return &m_modelObjectsInfo;
}
