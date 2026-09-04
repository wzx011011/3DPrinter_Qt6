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
    print("done -> %s" % out)


if __name__ == "__main__":
    main()
