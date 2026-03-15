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

} // namespace Crality3D
