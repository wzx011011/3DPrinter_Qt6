#!/usr/bin/env python3
"""Generate the synthetic benchmark STL models used by tests/PerfBench.cpp.

Outputs (default build/perf_models/):
  bench_sphere_400k.stl   UV sphere, ~400k triangles (slice tier)
  bench_sphere_2m.stl     UV sphere, ~2M triangles
  bench_sphere_5m.stl     UV sphere, ~5M triangles
  bench_grid_20x100k.stl  20 disjoint ~100k-triangle shells spread over the
                          bed (wide-spread ray/picking tier; a single STL is
                          one meshData batch, the spread exercises the BVH)

Usage:  python scripts/perf/gen_benchmark_models.py [output_dir]
Binary STL: 80-byte header, uint32 count, then per triangle
50 bytes (3 float32 normal + 9 float32 vertices + uint16 attribute).
"""

import math
import os
import struct
import sys

TARGETS = {
    "bench_sphere_400k.stl": 400_000,
    "bench_sphere_2m.stl": 2_000_000,
    "bench_sphere_5m.stl": 5_000_000,
}
GRID_SHELLS = 20
GRID_SHELL_TRIS = 100_000


def sphere_triangles(rings: int, sectors: int, radius: float, cx: float, cy: float):
    """Yield triangles of a UV sphere. rings*sectors*2 + 2*sectors tris."""
    tris = []
    pt = []
    for i in range(rings + 1):
        phi = math.pi * i / rings
        pt.append([])
        for j in range(sectors):
            theta = 2.0 * math.pi * j / sectors
            pt[i].append((
                cx + radius * math.sin(phi) * math.cos(theta),
                cy + radius * math.sin(phi) * math.sin(theta),
                radius * math.cos(phi),
            ))
    for i in range(rings):
        for j in range(sectors):
            j2 = (j + 1) % sectors
            a, b, c, d = pt[i][j], pt[i][j2], pt[i + 1][j2], pt[i + 1][j]
            if i != 0:
                tris.append((a, c, b))
            if i != rings - 1:
                tris.append((a, d, c))
    return tris


def rings_sectors_for(target: int):
    # tris = 2*rings*sectors + 2*sectors; keep sectors ~= 2*rings for a
    # well-conditioned aspect ratio.
    rings = int(math.sqrt((target - 4) / 6.0))
    sectors = 2 * rings
    while 2 * rings * sectors + 2 * sectors < target:
        rings += 1
        sectors = 2 * rings
    return rings, sectors


def write_stl(path: str, tris):
    with open(path, "wb") as f:
        f.write(b"OWzx PerfBench synthetic model".ljust(80, b"\0"))
        f.write(struct.pack("<I", len(tris)))
        pack = struct.Struct("<12fH").pack
        buf = bytearray()
        for (a, b, c) in tris:
            ux, uy, uz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
            vx, vy, vz = c[0] - a[0], c[1] - a[1], c[2] - a[2]
            nx, ny, nz = uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx
            n = math.sqrt(nx * nx + ny * ny + nz * nz) or 1.0
            buf += pack(nx / n, ny / n, nz / n,
                        *a, *b, *c, 0)
            if len(buf) > (1 << 22):
                f.write(buf)
                buf.clear()
        f.write(buf)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else "build/perf_models"
    os.makedirs(out_dir, exist_ok=True)
    for name, target in TARGETS.items():
        path = os.path.join(out_dir, name)
        rings, sectors = rings_sectors_for(target)
        tris = sphere_triangles(rings, sectors, 50.0, 110.0, 110.0)
        write_stl(path, tris)
        print(f"{name}: {len(tris)} tris ({rings}x{sectors} sphere)")
    shells = []
    per_side = 5
    for s in range(GRID_SHELLS):
        col, row = s % per_side, s // per_side
        rings, sectors = rings_sectors_for(GRID_SHELL_TRIS)
        shells += sphere_triangles(rings, sectors, 18.0,
                                   40.0 + col * 40.0, 40.0 + row * 40.0)
    path = os.path.join(out_dir, "bench_grid_20x100k.stl")
    write_stl(path, shells)
    print(f"bench_grid_20x100k.stl: {len(shells)} tris ({GRID_SHELLS} shells)")


if __name__ == "__main__":
    main()
