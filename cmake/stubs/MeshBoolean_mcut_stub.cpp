// Stub implementation for Slic3r::MeshBoolean::mcut functions.
//
// Reason: MeshBoolean.cpp is excluded from the Qt6 build because the CGAL
// section of that file uses CGAL 5.6+ APIs (CGAL::parameters::default_values)
// that are not available in the pre-built CGAL 5.4 we link against.
//
// The mcut section (MeshBoolean.cpp:572-905) is CGAL-independent and provides
// boolean mesh operations used only by ModelObject::make_boolean() — a
// "BBS: production extension" feature wired to the boolean gizmo that the
// Qt6 GUI has not yet migrated. Rather than extracting the mcut code into a
// separate file (which would fork upstream), we provide no-op stubs that
// leave the destination mesh empty. The caller's downstream loop is a no-op
// when dst_mesh remains empty, so behavior degrades gracefully to "boolean
// operation produces no output".

#include <libslic3r/TriangleMesh.hpp>
#include <string>
#include <vector>

namespace Slic3r {
namespace MeshBoolean {
namespace mcut {

// Signature must match MeshBoolean.hpp:99.
// Stub: leaves dst_mesh empty (caller's downstream loop is then a no-op).
void make_boolean(const TriangleMesh& /*src_mesh*/,
                  const TriangleMesh& /*cut_mesh*/,
                  std::vector<TriangleMesh>& dst_mesh,
                  const std::string& /*boolean_opts*/)
{
    dst_mesh.clear();
}

} // namespace mcut
} // namespace MeshBoolean
} // namespace Slic3r
