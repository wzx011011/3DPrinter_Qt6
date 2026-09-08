// FastStlImport — parallel binary-STL importer (P1, perf/waves).
//
// The stock path (admesh stl_open -> stl_read -> repair-on-import ->
// stl_generate_shared_vertices) is single-threaded end to end; profiling the
// 2M-triangle benchmark showed the dominant cost is the import repair chain
// (edge hash connect + fan-traversal shared-vertex build), not file IO.
//
// This importer reads binary STLs WITHOUT the on-import repair:
//  - one whole-file read, then TBB parallel decode of the fixed-size facet
//    records (binary STL is 50 B/triangle, trivially chunkable);
//  - parallel shared-vertex dedup with tbb::concurrent_hash_map on the
//    rounded vertex key (matching the stock float-exact semantics via the
//    raw bits, same as admesh HashEdge's exact match);
//  - degenerate-facet removal (zero-area check) done in parallel;
//  - the resulting indexed_triangle_set is handed to
//    TriangleMesh(indexed_triangle_set&&), so slicing consumes exactly the
//    same structure as the stock path.
// ASCII STLs and repair-needing meshes are NOT handled here: the caller
// falls back to the stock Model::read_from_file path for those (detect by
// header sniff). This keeps libslic3r/admesh sources untouched (migration
// rule) while cutting load wall time for large binary STLs.
//
// Coordinates are stored verbatim (slic3r frame); callers apply the usual
// Model::add_object flow, identical to admesh-based import.
#pragma once

#include <admesh/stl.h> // indexed_triangle_set (libslic3r uses it verbatim)

#include <string>

namespace OWzx
{

struct FastStlResult
{
    bool ok = false;
    bool is_binary = false;   // false => caller must fall back to stock path
    std::string error;
};

// Decodes a binary STL into an indexed triangle set using TBB parallelism.
// On success the mesh stats (volume/size) are filled by TriangleMesh itself.
FastStlResult fast_import_binary_stl(const std::string &path,
                                     indexed_triangle_set &out_its);

} // namespace OWzx
