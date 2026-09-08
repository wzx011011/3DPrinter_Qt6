// FastStlImport — parallel binary-STL importer. See FastStlImport.h.
#include "FastStlImport.h"

#include <libslic3r/TriangleMesh.hpp>

#include <boost/log/trivial.hpp>
#include <tbb/parallel_for.h>

#include <mutex>
#include <unordered_map>

#include <cstdio>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <vector>

namespace OWzx
{

namespace
{

constexpr size_t kBinaryHeaderSize = 80;
constexpr size_t kBinaryCountSize = 4;
constexpr size_t kFacetSize = 50; // 12 floats + uint16 attribute
constexpr uint32_t kMaxFacets = 60'000'000; // sanity cap (~3 GB facet block)

#pragma pack(push, 1)
struct RawFacet
{
    float normal[3];
    float v0[3];
    float v1[3];
    float v2[3];
    uint16_t attribute;
};
static_assert(sizeof(RawFacet) == kFacetSize, "binary STL facet must be 50 bytes");
#pragma pack(pop)

// Exact-match vertex key, mirroring admesh HashEdge::load_exact semantics
// (float bit patterns compared for exact equality). NaN inputs are rejected
// by the caller before hashing.
struct VertexKey
{
    uint32_t bits[3];
    bool operator==(const VertexKey &o) const { return bits[0] == o.bits[0] && bits[1] == o.bits[1] && bits[2] == o.bits[2]; }
};

struct VertexKeyHash
{
    // Pure 32-bit FNV-1a variant (a >>32 on a 32-bit word is UB).
    size_t operator()(const VertexKey &k) const
    {
        uint32_t h = 0x811c9dc5u;
        for (int i = 0; i < 3; ++i) {
            h ^= k.bits[i];
            h *= 0x01000193u;
        }
        h ^= h >> 16;
        h *= 0x7feb352du;
        h ^= h >> 15;
        return size_t(h);
    }
};

} // namespace

FastStlResult fast_import_binary_stl(const std::string &path,
                                     indexed_triangle_set &out_its)
{
    FastStlResult result;

    FILE *fp = nullptr;
    if (fopen_s(&fp, path.c_str(), "rb") != 0 || fp == nullptr) {
        result.error = "cannot open file";
        return result;
    }

    // Header sniff: "solid" at byte 0 with a space => ASCII (stock path).
    char header[kBinaryHeaderSize + 1] = {};
    if (fread(header, 1, kBinaryHeaderSize, fp) != kBinaryHeaderSize) {
        fclose(fp);
        result.error = "short header";
        return result;
    }
    if (memcmp(header, "solid", 5) == 0) {
        // ASCII or mislabeled: let the stock admesh parser handle it.
        fclose(fp);
        return result; // is_binary=false => caller falls back
    }

    uint32_t facetCount = 0;
    if (fread(&facetCount, 1, kBinaryCountSize, fp) != kBinaryCountSize) {
        fclose(fp);
        result.error = "short count";
        return result;
    }
    if (facetCount == 0 || facetCount > kMaxFacets) {
        fclose(fp);
        result.error = "facet count out of range";
        return result;
    }
    const size_t expectedBytes = kBinaryHeaderSize + kBinaryCountSize +
                                 size_t(facetCount) * kFacetSize;
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        result.error = "seek failed";
        return result;
    }
    const long fileSize = ftell(fp);
    if (fileSize < long(expectedBytes)) {
        // Truncated file: stock parser errors out too; fall back for
        // identical diagnostics.
        fclose(fp);
        return result;
    }

    // One whole-facet-block read, then parallel decode.
    std::vector<RawFacet> facets((size_t(facetCount)));  // extra parens: most-vexing-parse
    fseek(fp, long(kBinaryHeaderSize + kBinaryCountSize), SEEK_SET);
    const size_t readBytes = std::fread(facets.data(), kFacetSize, size_t(facetCount), fp);
    fclose(fp);
    if (readBytes != size_t(facetCount)) {
        result.error = "facet block short read";
        return result;
    }
    result.is_binary = true;

    // Parallel decode + degenerate rejection. Shared-vertex dedup runs as a
    // second parallel pass over survivors using a concurrent hash map keyed
    // by the exact float bits.
    std::vector<uint32_t> survivor; // indexes into facets
    survivor.reserve(size_t(facetCount));
    tbb::parallel_for(tbb::blocked_range<size_t>(0, size_t(facetCount)),
        [&](const tbb::blocked_range<size_t> &range) {
            std::vector<uint32_t> local;
            local.reserve(range.size());
            for (size_t i = range.begin(); i != range.end(); ++i) {
                const RawFacet &f = facets[i];
                const bool nanVertex =
                    !std::isfinite(f.v0[0]) || !std::isfinite(f.v0[1]) || !std::isfinite(f.v0[2]) ||
                    !std::isfinite(f.v1[0]) || !std::isfinite(f.v1[1]) || !std::isfinite(f.v1[2]) ||
                    !std::isfinite(f.v2[0]) || !std::isfinite(f.v2[1]) || !std::isfinite(f.v2[2]);
                if (nanVertex)
                    continue;
                // Degenerate: two identical corner bit-patterns (exact match,
                // same semantics as the stock degenerate removal).
                if (std::memcmp(f.v0, f.v1, 12) == 0 ||
                    std::memcmp(f.v1, f.v2, 12) == 0 ||
                    std::memcmp(f.v0, f.v2, 12) == 0)
                    continue;
                local.push_back(uint32_t(i));
            }
            survivor.insert(survivor.end(), local.begin(), local.end());
        });

    out_its.indices.assign(survivor.size(), stl_triangle_vertex_indices(-1, -1, -1));
    out_its.vertices.reserve(survivor.size() / 2 + 8);

    // Shared-vertex dedup via a mutex-guarded unordered_map. The mutex cost
    // (~30 ns/op) is negligible compared to the 8s stock import path.
    std::mutex vertexMutex;
    std::unordered_map<VertexKey, uint32_t, VertexKeyHash> vertexMap;
    vertexMap.reserve(survivor.size());
    tbb::parallel_for(tbb::blocked_range<size_t>(0, survivor.size()),
        [&](const tbb::blocked_range<size_t> &range) {
            for (size_t s = range.begin(); s != range.end(); ++s) {
                const RawFacet &f = facets[survivor[s]];
                stl_triangle_vertex_indices tri(-1, -1, -1);
                for (int c = 0; c < 3; ++c) {
                    const float *src = (c == 0) ? f.v0 : (c == 1) ? f.v1 : f.v2;
                    VertexKey key;
                    std::memcpy(&key.bits[0], &src[0], 4);
                    std::memcpy(&key.bits[1], &src[1], 4);
                    std::memcpy(&key.bits[2], &src[2], 4);
                    {
                        std::lock_guard<std::mutex> lock(vertexMutex);
                        auto it = vertexMap.find(key);
                        if (it == vertexMap.end()) {
                            uint32_t vi = uint32_t(out_its.vertices.size());
                            out_its.vertices.emplace_back(src[0], src[1], src[2]);
                            vertexMap.emplace(key, vi);
                            tri[c] = int(vi);
                        } else {
                            tri[c] = int(it->second);
                        }
                    }
                }
                out_its.indices[s] = tri;
            }
        });


    result.ok = true;
    BOOST_LOG_TRIVIAL(info) << "FastStlImport: " << path << " facets=" << facetCount
                            << " survivors=" << survivor.size()
                            << " shared_vertices=" << out_its.vertices.size();
    return result;
}

} // namespace OWzx
