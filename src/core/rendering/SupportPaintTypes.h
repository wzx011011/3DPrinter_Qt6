#pragma once

#include <vector>

namespace OWzx {

/**
 * Support painting state enum (aligns with upstream EnforcerBlockerType in TriangleSelector.hpp)
 *
 * Maps to:
 * - NONE = 0: Default, no support paint applied
 * - ENFORCER = 1: Force support on this area (overrides auto support)
 * - BLOCKER = 2: Block support on this area (prevents auto support)
 */
enum class SupportPaintState : int8_t {
    None = 0,
    Enforcer = 1,
    Blocker = 2
};

/**
 * Per-triangle paint data container (对齐上游 TriangleSelector paint state storage)
 */
struct TrianglePaintData {
    int triangleIndex = -1;
    SupportPaintState state = SupportPaintState::None;
};

/**
 * Object-level paint data container (对齐上游 ModelVolume::mmu_segmentation_facets)
 */
struct ObjectPaintData {
    int objectIndex = -1;
    std::vector<TrianglePaintData> triangles;

    int enforcedCount() const {
        int n = 0;
        for (const auto &t : triangles)
            if (t.state == SupportPaintState::Enforcer) ++n;
        return n;
    }
    int blockedCount() const {
        int n = 0;
        for (const auto &t : triangles)
            if (t.state == SupportPaintState::Blocker) ++n;
        return n;
    }
    int totalCount() const { return static_cast<int>(triangles.size()); }
    void setTriangleState(int triIdx, SupportPaintState st) {
        for (auto &t : triangles) {
            if (t.triangleIndex == triIdx) { t.state = st; return; }
        }
        triangles.push_back({triIdx, st});
    }
    SupportPaintState triangleState(int triIdx) const {
        for (const auto &t : triangles)
            if (t.triangleIndex == triIdx) return t.state;
        return SupportPaintState::None;
    }
    void clearAll() {
        for (auto &t : triangles)
            t.state = SupportPaintState::None;
    }
};

} // namespace OWzx
