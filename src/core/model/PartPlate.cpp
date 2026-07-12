// PartPlate implementation.
// Source truth: third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:77-557.
// Most accessors are inline in the header (pure value object); only the
// derived membership query needs a definition file.

#include "PartPlate.h"

namespace OWzx {

bool PartPlate::hasObject(int objIdx) const {
  // An object is on this plate if ANY of its instances is in the membership set.
  // The set is keyed by (objectIndex, instanceIndex) pairs.
  for (const auto& pair : m_obj_to_instance_set) {
    if (pair.first == objIdx) {
      return true;
    }
  }
  return false;
}

FilamentMapMode migrateLegacyFilamentMapMode(int legacyRawInt) {
  // Phase 111 (FMAP-04 / Phase 107 REVIEW R-01): the FM-03 legacy raw-int ->
  // widened-enum migration predicate. Factored out of ProjectServiceMock's two
  // read sites (loadFile ~646, loadProject ~5526) so the legacy branch is unit-
  // testable in isolation -- R-01 observed the round-trip test always took the
  // trusted coEnum branch (the write side produces typed values), so the legacy
  // discriminator (the actual headline FMAP-02 fix) had NO runtime coverage.
  //
  // Pre-v4.5 Qt6 files wrote filament_map_mode as a raw int (0=Auto, 1=Manual)
  // via the old 2-value enum. After the Phase 107 widening, raw 1 MUST map to
  // fmmManual (2) -- NOT the new fmmAutoForMatch (1) -- so a pre-v4.5 "Manual"
  // plate stays Manual after reload. See PartPlate.h for the contract.
  if (legacyRawInt == 0)
    return FilamentMapMode::fmmAutoForFlush;
  if (legacyRawInt == 1)
    return FilamentMapMode::fmmManual;
  return FilamentMapMode::fmmDefault;
}

}  // namespace OWzx
