// PartPlateList implementation.
// Source truth: third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:559-937 (PartPlateList),
// PartPlate.cpp:4407 (create_plate), :4554 (delete_plate).

#include "PartPlateList.h"

#include <QSet>  // for distinct-object collapse in objectIndicesOnPlate

namespace OWzx {

PartPlateList::PartPlateList() {
  // Upstream starts with a single default plate; Qt6 preserves the same invariant
  // (a project always has >= 1 plate). createPlate handles index assignment.
  createPlate();
  m_current_plate = 0;
}

PartPlate* PartPlateList::plate(int index) {
  if (index < 0 || index >= plateCount()) return nullptr;
  return m_plate_list[index].get();
}

const PartPlate* PartPlateList::plate(int index) const {
  if (index < 0 || index >= plateCount()) return nullptr;
  return m_plate_list[index].get();
}

void PartPlateList::setCurrentPlateIndex(int index) {
  if (index < 0 || index >= plateCount()) return;
  m_current_plate = index;
}

PartPlate* PartPlateList::createPlate() {
  // Upstream create_plate guards against exceeding MAX_PLATE_COUNT.
  if (plateCount() >= kMaxPlateCount) return nullptr;
  auto p = std::make_unique<PartPlate>(plateCount());
  PartPlate* raw = p.get();
  m_plate_list.push_back(std::move(p));
  return raw;
}

bool PartPlateList::deletePlate(int index) {
  if (plateCount() <= 1) return false;  // upstream invariant: keep >= 1 plate
  if (index < 0 || index >= plateCount()) return false;
  m_plate_list.erase(m_plate_list.begin() + index);
  reindex();
  // Keep current plate index valid after deletion.
  if (m_current_plate >= plateCount()) {
    m_current_plate = plateCount() - 1;
  }
  return true;
}

bool PartPlateList::renamePlate(int index, const std::string& name) {
  PartPlate* p = plate(index);
  if (!p) return false;
  p->setName(name);
  return true;
}

void PartPlateList::setPlateLocked(int index, bool locked) {
  PartPlate* p = plate(index);
  if (p) p->setLocked(locked);
}

bool PartPlateList::movePlate(int oldIndex, int newIndex) {
  // D-07: pure metadata reorder (vector shift + reindex). No geometry recompute
  // (Qt6 doesn't compute plate origins yet — deferred per Phase 17 CONTEXT).
  if (oldIndex < 0 || oldIndex >= plateCount()) return false;
  if (newIndex < 0 || newIndex >= plateCount()) return false;
  if (oldIndex == newIndex) return false;

  // Extract the plate, shift the in-between elements, reinsert at newIndex.
  std::unique_ptr<PartPlate> moved = std::move(m_plate_list[oldIndex]);
  if (oldIndex < newIndex) {
    for (int i = oldIndex; i < newIndex; ++i)
      m_plate_list[i] = std::move(m_plate_list[i + 1]);
  } else {
    for (int i = oldIndex; i > newIndex; --i)
      m_plate_list[i] = std::move(m_plate_list[i - 1]);
  }
  m_plate_list[newIndex] = std::move(moved);
  reindex();

  // Track the current plate through the shift.
  if (m_current_plate == oldIndex) {
    m_current_plate = newIndex;
  } else if (oldIndex < newIndex) {
    // shift was [old+1..new] -> [old..new-1]; current in (old, new] decrements
    if (m_current_plate > oldIndex && m_current_plate <= newIndex)
      --m_current_plate;
  } else {  // oldIndex > newIndex
    // shift was [new..old-1] -> [new+1..old]; current in [new, old) increments
    if (m_current_plate >= newIndex && m_current_plate < oldIndex)
      ++m_current_plate;
  }
  return true;
}

void PartPlateList::setPlatePrintable(int index, bool printable) {
  PartPlate* p = plate(index);
  if (p) p->setPrintable(printable);
}

int PartPlateList::plateIndexForObject(int objIdx) const {
  for (int i = 0; i < plateCount(); ++i) {
    if (m_plate_list[i]->hasObject(objIdx)) return i;
  }
  return -1;
}

QList<int> PartPlateList::objectIndicesOnPlate(int plateIndex) const {
  QList<int> result;
  const PartPlate* p = plate(plateIndex);
  if (!p) return result;
  // Collapse instance pairs to distinct object indices, preserving first-seen order.
  QSet<int> seen;
  for (const auto& pair : p->objToInstanceSet()) {
    if (!seen.contains(pair.first)) {
      seen.insert(pair.first);
      result.append(pair.first);
    }
  }
  return result;
}

void PartPlateList::resetToSinglePlate() {
  // Keep the invariant: always >= 1 plate. Used by the 3MF load path.
  if (m_plate_list.empty()) {
    createPlate();
    m_current_plate = 0;
    return;
  }
  // Erase all but the first, then clear its state for a fresh load.
  m_plate_list.erase(m_plate_list.begin() + 1, m_plate_list.end());
  PartPlate* first = m_plate_list[0].get();
  first->clearInstances();
  first->setName({});
  first->setPlateIndex(0);
  first->setLocked(false);
  m_current_plate = 0;
}

void PartPlateList::reindex() {
  for (int i = 0; i < plateCount(); ++i) {
    m_plate_list[i]->setPlateIndex(i);
  }
}

}  // namespace OWzx
