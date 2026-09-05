#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate deterministic benchmark STL models for OWzx perf evaluation.

Models (binary STL, 50 bytes/triangle):
  bench_sphere_400k.stl  - single UV sphere, ~400k triangles (slice A/B tier)
  bench_sphere_2m.stl    - single UV sphere, ~2M triangles (load/render tier)
  bench_sphere_5m.stl    - single UV sphere, ~5M triangles (large tier)
  bench_grid_20x100k.stl - 20 spheres x ~100k triangles on a grid (multi-object tier)

All models fit a 256x256x256 bed. Sphere radius 40 mm, z-centered on bed.

Usage: python scripts/perf/gen_benchmark_models.py [--out build/perf_models]
"""

import argparse
import math
import os
import struct
import sys


def sphere_triangles(rings, sectors, radius, center=(0.0, 0.0, 0.0)):
    """Yield triangles of a UV sphere as ((n),(v1),(v2),(v3)) float triples.

    rings: latitude divisions (>=2), sectors: longitude divisions (>=3).
    Triangle count = 2 * (rings - 1) * sectors. Degenerate pole quads are
    emitted as single triangles.
    """
    cx, cy, cz = center
    ring_angle = math.pi / (rings - 1)
    sec_angle = 2.0 * math.pi / sectors
    cos_s = [math.cos(i * sec_angle) for i in range(sectors)]
    sin_s = [math.sin(i * sec_angle) for i in range(sectors)]
    cos_r = [math.cos(i * ring_angle) for i in range(rings)]
    sin_r = [math.sin(i * ring_angle) for i in range(rings)]

    def vertex(r, s):
        sr = sin_r[r]
        return (cx + radius * sr * cos_s[s],
                cy + radius * sr * sin_s[s],
                cz + radius * cos_r[r])

    for r in range(rings - 1):
        s_next = [vertex(r, i) for i in range(sectors)]
        s_prev = [vertex(r + 1, i) for i in range(sectors)]
        for i in range(sectors):
            j = (i + 1) % sectors
            a, b = s_next[i], s_next[j]
            c, d = s_prev[i], s_prev[j]
            if r == 0:
                yield (b, a, c)  # top cap: single triangle
            elif r == rings - 2:
                yield (c, a, d)  # bottom cap: single triangle
            else:
                yield (a, b, c)
                yield (b, d, c)


def tri_normal(a, b, c):
    ux, uy, uz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
    vx, vy, vz = c[0] - a[0], c[1] - a[1], c[2] - a[2]
    nx, ny, nz = uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx
    ln = math.sqrt(nx * nx + ny * ny + nz * nz)
    if ln < 1e-12:
        return 0.0, 0.0, 1.0
    return nx / ln, ny / ln, nz / ln


def write_stl(path, tri_iter, tri_count_hint):
    header = b"OWzx perf benchmark model (binary STL)".ljust(80, b"\0")
    with open(path, "wb") as fh:
        fh.write(header)
        fh.write(struct.pack("<I", 0))  # placeholder count, fixed at close
        count = 0
        pack = struct.Struct("<12fH").pack
        for a, b, c in tri_iter:
            n = tri_normal(a, b, c)
            fh.write(pack(n[0], n[1], n[2],
                          a[0], a[1], a[2],
                          b[0], b[1], b[2],
                          c[0], c[1], c[2], 0))
            count += 1
            if count % 500000 == 0:
                sys.stderr.write("  %d tris written\n" % count)
        fh.seek(80)
        fh.write(struct.pack("<I", count))
    return count


def tri_count(rings, sectors):
    # caps emit one triangle instead of two
    return 2 * (rings - 1) * sectors - 2 * sectors


def gen_sphere(path, target_tris, radius=40.0):
    # solve rings/sectors close to target: use sectors = round(sqrt(target/2))+1
    sectors = max(3, int(round(math.sqrt(target_tris / 2.0))))
    rings = max(2, int(round(target_tris / (2.0 * sectors))) + 1)
    center = (0.0, 0.0, radius)
    count = write_stl(path, sphere_triangles(rings, sectors, radius, center),
                      tri_count(rings, sectors))
    print("%s: rings=%d sectors=%d tris=%d" % (path, rings, sectors, count))
    return count


def gen_grid(path, cols, rows, tris_each, radius, spacing):
    sectors = max(3, int(round(math.sqrt(tris_each / 2.0))))
    rings = max(2, int(round(tris_each / (2.0 * sectors))) + 1)
    tris = []
    total = 0
    for gy in range(rows):
        for gx in range(cols):
            cx = (gx - (cols - 1) / 2.0) * spacing
            cy = (gy - (rows - 1) / 2.0) * spacing
            for t in sphere_triangles(rings, sectors, radius, (cx, cy, radius)):
                tris.append(t)
                total += 1
    count = write_stl(path, iter(tris), total)
    print("%s: %dx%d spheres rings=%d sectors=%d tris=%d"
          % (path, cols, rows, rings, sectors, count))
    return count


def overhang_tower(path, levels=60, side=30.0, shrink=0.55, layer_h=0.5):
    """Stacked shrinking boxes with wide bridges: forces massive support
    generation (every level overhangs the previous one). Deterministic
    binary STL. Triangle count scales with levels."""
    tris = []

    sub = 16  # wall subdivisions per side: tris = levels * (4 walls * sub*sub*2 + caps)

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
                p0 = (ax + (bx - ax) * u0, ay + (by - ay) * u0)
                p1 = (ax + (bx - ax) * u1, ay + (by - ay) * u1)
                tris.append(((p0[0], p0[1], z0), (p1[0], p1[1], z0), (p1[0], p1[1], z1)))
                tris.append(((p0[0], p0[1], z0), (p1[0], p1[1], z1), (p0[0], p0[1], z1)))
        # top + bottom (subdivided grids)
        for u in range(sub):
            for v in range(sub):
                ux0 = x0 + (x1 - x0) * u / sub
                ux1 = x0 + (x1 - x0) * (u + 1) / sub
                vy0 = y0 + (y1 - y0) * v / sub
                vy1 = y0 + (y1 - y0) * (v + 1) / sub
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
    count = write_stl(path, iter(tris), len(tris))
    print("%s: levels=%d tris=%d height=%.1fmm" % (path, levels, count, z))
    return count


def multi_plate_project(path, per_plate_tris=150000, plates=4, seg=400):
    """One STL per plate: slice each plate separately on both slicers by
    invoking the A/B scripts once per file (multi-plate = the plate queue)."""
    import math
    total = 0
    for p in range(plates):
        sectors = max(3, int(round(math.sqrt(per_plate_tris / 2.0))))
        rings = max(2, int(round(per_plate_tris / (2.0 * sectors))) + 1)
        tris = []
        # A gear-like disk: outer ring + spokes + hub -> real slicing load
        # (thin walls + small features) without needing an external model.
        outer_r, inner_r, hub_r = 60.0, 48.0, 20.0
        teeth = 24
        for i in range(seg):
            a0 = 2 * math.pi * i / seg
            a1 = 2 * math.pi * (i + 1) % (2 * math.pi)
            a1 = 2 * math.pi * (i + 1) / seg
            # outer ring band (top+bottom+wall)
            pts = [(outer_r * math.cos(a0), outer_r * math.sin(a0)),
                   (outer_r * math.cos(a1), outer_r * math.sin(a1)),
                   (inner_r * math.cos(a1), inner_r * math.sin(a1)),
                   (inner_r * math.cos(a0), inner_r * math.sin(a0))]
            h = 4.0
            tris.append(((pts[0][0], pts[0][1], 0), (pts[1][0], pts[1][1], 0), (pts[2][0], pts[2][1], 0)))
            tris.append(((pts[0][0], pts[0][1], 0), (pts[2][0], pts[2][1], 0), (pts[3][0], pts[3][1], 0)))
            tris.append(((pts[0][0], pts[0][1], h), (pts[2][0], pts[2][1], h), (pts[1][0], pts[1][1], h)))
            tris.append(((pts[0][0], pts[0][1], h), (pts[3][0], pts[3][1], h), (pts[2][0], pts[2][1], h)))
            tris.append(((pts[0][0], pts[0][1], 0), (pts[3][0], pts[3][1], 0), (pts[3][0], pts[3][1], h)))
            tris.append(((pts[0][0], pts[0][1], 0), (pts[3][0], pts[3][1], h), (pts[0][0], pts[0][1], h)))
            tris.append(((pts[1][0], pts[1][1], 0), (pts[2][0], pts[2][1], h), (pts[2][0], pts[2][1], 0)))
            tris.append(((pts[1][0], pts[1][1], 0), (pts[1][0], pts[1][1], h), (pts[2][0], pts[2][1], h)))
            # tooth block on the rim (the overhang-ish feature)
            tm = 2 * math.pi * (i + 0.5) / seg
            tx, ty = 0.5 * (outer_r + inner_r) * math.cos(tm), 0.5 * (outer_r + inner_r) * math.sin(tm)
            s = 6.0
            tq = [(tx - s, ty - s), (tx + s, ty - s), (tx + s, ty + s), (tx - s, ty + s)]
            for k in range(4):
                k2 = (k + 1) % 4
                tris.append(((tq[k][0], tq[k][1], 0), (tq[k2][0], tq[k2][1], 0), (tq[k2][0], tq[k2][1], h)))
                tris.append(((tq[k][0], tq[k][1], 0), (tq[k2][0], tq[k2][1], h), (tq[k][0], tq[k][1], h)))
            tris.append(((tq[0][0], tq[0][1], h), (tq[1][0], tq[1][1], h), (tq[2][0], tq[2][1], h)))
            tris.append(((tq[0][0], tq[0][1], h), (tq[2][0], tq[2][1], h), (tq[3][0], tq[3][1], h)))
            # hub ring segment
            hp = [(hub_r * math.cos(a0), hub_r * math.sin(a0)),
                  (hub_r * math.cos(a1), hub_r * math.sin(a1)),
                  (hub_r * 0.5 * math.cos(a1), hub_r * 0.5 * math.sin(a1)),
                  (hub_r * 0.5 * math.cos(a0), hub_r * 0.5 * math.sin(a0))]
            tris.append(((hp[0][0], hp[0][1], 0), (hp[1][0], hp[1][1], 0), (hp[2][0], hp[2][1], 0)))
            tris.append(((hp[0][0], hp[0][1], 0), (hp[2][0], hp[2][1], 0), (hp[3][0], hp[3][1], 0)))
            tris.append(((hp[0][0], hp[0][1], h), (hp[2][0], hp[2][1], h), (hp[1][0], hp[1][1], h)))
            tris.append(((hp[0][0], hp[0][1], h), (hp[3][0], hp[3][1], h), (hp[2][0], hp[2][1], h)))
        path_p = path.replace(".stl", "_plate%d.stl" % (p + 1))
        total += write_stl(path_p, iter(tris), len(tris))
        print("%s: tris=%d" % (path_p, len(tris)))
    return total


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="build/perf_models",
                    help="output directory (default build/perf_models)")
    args = ap.parse_args()
    out = args.out
    os.makedirs(out, exist_ok=True)

    gen_sphere(os.path.join(out, "bench_sphere_400k.stl"), 400000)
    gen_sphere(os.path.join(out, "bench_sphere_2m.stl"), 2000000)
    gen_sphere(os.path.join(out, "bench_sphere_5m.stl"), 5000000)
    gen_grid(os.path.join(out, "bench_grid_20x100k.stl"), 5, 4, 100000,
             radius=12.0, spacing=35.0)
    overhang_tower(os.path.join(out, "bench_overhang_tower.stl"),
                   levels=200, side=30.0, layer_h=0.2)
    multi_plate_project(os.path.join(out, "bench_gear.stl"),
                        per_plate_tris=150000, plates=4, seg=1200)
    print("done -> %s" % out)


if __name__ == "__main__":
    main()
