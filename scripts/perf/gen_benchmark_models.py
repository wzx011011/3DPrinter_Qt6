#!/usr/bin/env python3
"""Generate the synthetic benchmark STL models used by tests/PerfBench.cpp.

Outputs (default build/perf_models/):
  bench_sphere_400k.stl   UV sphere, ~400k triangles (slice tier)
  bench_sphere_2m.stl     UV sphere, ~2M triangles
  bench_sphere_5m.stl     UV sphere, ~5M triangles
  bench_grid_20x100k.stl  20 disjoint ~100k-triangle shells spread over the
                          bed (wide-spread ray/picking tier; a single STL is
                          one meshData batch, the spread exercises the BVH)
  bench_overhang_tower.stl  shrinking/stacked box tower, every level
                          overhangs the previous one; sliced with
                          enable_support it loads the support steps (the P4
                          step-profile tier, docs/开发计划_2026-10-10.md §6d)

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


def overhang_tower(path: str, levels: int = 60, side: float = 30.0,
                   shrink: float = 0.55, layer_h: float = 0.5,
                   sub: int = 16, cx: float = 0.0, cy: float = 0.0):
    """Stacked shrinking boxes with wide bridges: forces massive support
    generation (every level overhangs the previous one). Deterministic
    binary STL. Triangle count scales with levels.

    Ported from the perf/waves branch generator (commit 09dad79), adapted to
    this generator's write_stl helper and bed coordinates (spheres sit at
    cx=110, cy=110). `sub` tunes the wall/cap mesh density.
    """
    tris = []

    # wall subdivisions per side: tris = levels * (4*sub*2 walls + 8*sub^2 caps)
    def box(z0, z1, half):
        x0, x1 = -half, half
        y0, y1 = -half, half
        # subdivided side walls
        corners = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
        for i in range(4):
            j = (i + 1) % 4
            ax, ay = corners[i]
            bx, by = corners[j]
            for u in range(sub):
                u0, u1 = u / sub, (u + 1) / sub
                p0 = (cx + ax + (bx - ax) * u0, cy + ay + (by - ay) * u0)
                p1 = (cx + ax + (bx - ax) * u1, cy + ay + (by - ay) * u1)
                tris.append(((p0[0], p0[1], z0), (p1[0], p1[1], z0), (p1[0], p1[1], z1)))
                tris.append(((p0[0], p0[1], z0), (p1[0], p1[1], z1), (p0[0], p0[1], z1)))
        # top + bottom (subdivided grids)
        for u in range(sub):
            for v in range(sub):
                ux0 = cx + x0 + (x1 - x0) * u / sub
                ux1 = cx + x0 + (x1 - x0) * (u + 1) / sub
                vy0 = cy + y0 + (y1 - y0) * v / sub
                vy1 = cy + y0 + (y1 - y0) * (v + 1) / sub
                tris.append(((ux0, vy0, z1), (ux1, vy0, z1), (ux1, vy1, z1)))
                tris.append(((ux0, vy0, z1), (ux1, vy1, z1), (ux0, vy1, z1)))
                tris.append(((ux0, vy0, z0), (ux1, vy1, z0), (ux1, vy0, z0)))
                tris.append(((ux0, vy0, z0), (ux0, vy1, z0), (ux1, vy1, z0)))

    z = 0.0
    half = side / 2.0
    for level in range(levels):
        box(z, z + layer_h, half)
        z += layer_h
        half *= shrink if (level % 2 == 0) else (1.0 / shrink)
    write_stl(path, tris)
    print(f"{path}: levels={levels} tris={len(tris)} height={z:.1f}mm")
    return len(tris)


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
    # P4 step-profile tier (§6d): support-heavy tower. levels=200, sub=16 ->
    # 230,400 tris (~23 万, the §6d triangle budget), 100 mm tall, centered
    # like the spheres; each of the 50 expansion steps is a wide bridge that
    # forces support columns (tree support, enable_support is set by the
    # PerfBench tower tier). Profile sanity (2026-10-13): support is the top
    # non-export step (~0.7 s, ~14x the sphere's support share); gcode_export
    # stays the single largest step on this engine for every model because
    # exporting the support extrusions costs ~4x the support computation.
    overhang_tower(os.path.join(out_dir, "bench_overhang_tower.stl"),
                   levels=200, side=100.0, shrink=0.5, layer_h=0.5, sub=16,
                   cx=110.0, cy=110.0)


if __name__ == "__main__":
    main()
