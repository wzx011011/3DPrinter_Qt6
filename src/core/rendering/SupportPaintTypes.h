#pragma once

#include <vector>

namespace Crality3D {

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
 * Cursor type for support painting (aligns with upstream TriangleSelector::CursorType)
 */
enum class SupportPaintCursorType : int8_t {
    Circle = 0,   // 2D screen-space circle brush
    Sphere = 1,   // 3D world-space sphere brush
    Pointer = 2,  // Click to select single triangle
    HeightRange = 3, // BBS: select by height range
    GapFill = 4   // BBS: gap fill tool
};

/**
 * Tool type for support painting (aligns with upstream GLGizmoFdmSupports::ToolType)
 */
enum class SupportPaintToolType : int8_t {
    Brush = 0,       // Circle/Sphere brush painting
    BucketFill = 1,  // Bucket fill tool (flood fill)
    SmartFill = 2,   // Smart fill by angle
    GapFill = 3      // Gap fill tool
};

/**
 * Support painting settings (aligns with upstream GLGizmoFdmSupports member variables)
 */
struct SupportPaintSettings {
    SupportPaintState tool = SupportPaintState::Enforcer;  // Current painting tool state
    SupportPaintCursorType cursorType = SupportPaintCursorType::Circle;
    SupportPaintToolType paintToolType = SupportPaintToolType::Brush;
    float cursorRadius = 5.0f;           // Brush radius in mm
    float angleThreshold = 45.0f;        // Overhang angle threshold for highlighting (degrees)
    float smartFillAngle = 30.0f;        // Smart fill angle threshold (degrees)
    float gapArea = 1.0f;                // Gap fill area threshold
    bool paintOnOverhangsOnly = false;   // Restrict painting to overhang areas
    bool enableSupport = false;          // Support enabled flag
    int supportType = 0;                 // Support type: 0=normal, 1=tree
};

/**
 * Triangle selection data for support painting
 * (aligns with upstream TriangleSelector serialization)
 */
struct SupportPaintTriangleData {
    int triangleIndex = -1;
    SupportPaintState state = SupportPaintState::None;
};

/**
 * Clipping plane for support painting view (aligns with upstream TriangleSelector::ClippingPlane)
 */
struct SupportPaintClippingPlane {
    float normalX = 0.0f;
    float normalY = 0.0f;
    float normalZ = 1.0f;
    float offset = 0.0f;
    bool active = false;
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

// ── Seam painting types (aligns with upstream GLGizmoSeam) ─────────────────

/**
 * Seam painting tool mode (aligns with upstream GLGizmoSeam m_current_tool)
 *
 * Maps to:
 * - 0 = Enforcer: Force seam placement on painted area
 * - 1 = Blocker:  Block seam placement on painted area
 */
enum class SeamPaintTool : int8_t {
    None = 0,
    Enforcer = 1,
    Blocker = 2
};

/**
 * Seam painting settings (aligns with upstream GLGizmoSeam member variables)
 */
struct SeamPaintSettings {
    SeamPaintTool tool = SeamPaintTool::None;
    float cursorRadius = 2.0f;           // Cursor radius in mm (min 0.05 upstream)
    bool paintOnOverhangsOnly = false;   // Restrict painting to overhang areas
};

// ── Hollow gizmo types (aligns with upstream GLGizmoHollow) ───────────────

/**
 * Hollow gizmo settings (aligns with upstream GLGizmoHollow member variables)
 */
struct HollowGizmoSettings {
    bool enableHollowing = true;     // m_enable_hollowing
    float newHoleRadius = 2.0f;      // m_new_hole_radius
    float newHoleHeight = 6.0f;      // m_new_hole_height
    float hollowingOffset = 3.0f;    // m_offset_stash (drain hole offset)
    float hollowingQuality = 0.5f;   // m_quality_stash
    float closingDistance = 2.0f;    // m_closing_d_stash
    int selectedHoleCount = 0;       // Number of currently selected drain holes
};

} // namespace Crality3D
