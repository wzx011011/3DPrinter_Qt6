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

}  // namespace OWzx
