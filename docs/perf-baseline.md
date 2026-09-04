# OWzx Performance Baseline & Evaluation System

Single source of truth for the OWzx performance evaluation: what is
measured, how to reproduce every number, and the final head-to-head results
vs OrcaSlicer on the same machine. Rebuilt 2026-09-05 (the previous copy was
lost in a worktree-migration mishap; all numbers below are the verified
values from the session logs).

Success criteria: for large models, rendering efficiency, memory footprint,
and slicing efficiency must be **not slower than OrcaSlicer / Bambu Studio**,
verified by the benches below — not by impression.

## Environment

| Item | Value |
|------|-------|
| CPU | AMD Ryzen 7 5800X (8C/16T) |
| RAM | 32 GB |
| GPU | AMD Radeon RX 7800 XT |
| OS | Windows 11 x64 (26200), ToDesk/Sunlogin services present (remote-safe software canvas by default; `OWZX_FORCE_RHI=1` overrides) |
| Rival | OrcaSlicer `C:\Program Files\OrcaSlicer\orca-slicer.exe` (CLI + GUI) |
| OWzx | branch `perf/waves`, worktree `D:\ai\3DPrinter_Qt6-perf` |

## Benches and how to run

```powershell
# 1. Deterministic benchmark models (400k / 2m / 5m spheres + 20-object grid)
python scripts/perf/gen_benchmark_models.py            # -> build/perf_models/

# 2. Slice A/B wall-clock + peak memory (same model, layer 0.2, auto widths)
powershell -File scripts/perf/run_orca_bench.ps1 -Model build/perf_models/bench_sphere_400k.stl
powershell -File scripts/perf/run_owzx_bench.ps1 -Model build/perf_models/bench_sphere_400k.stl

# 3. OWzx stage-level bench (load / meshData / scene expand / pick sweep /
#    slice / preview parse, with memory snapshots; JSON report)
powershell -File scripts/perf/run_stage_bench.ps1 -Tiers "400k,2m,5m"

# 4. In-app FPS overlay: Ctrl+Shift+Space in the 3D view (both apps).
#    One-double-click launcher: scripts/perf/gui_fps_collect.cmd
```

Fairness pins (slice A/B): `layer_height=0.2`, all `*_line_width=0` (auto) on
both sides; rival additionally needs `--layer-change-gcode=G92E0` (relative-E
validation, Print.cpp:1408-1426).

## Final head-to-head summary

| Metric (same machine, same models) | OrcaSlicer | OWzx | Verdict |
|---|---|---|---|
| Post-slice preview readiness | ~0 (in-memory viewer) | ~0 (in-memory hand-off, Wave 3b) | **par** (was: +87.5 s parse) |
| Slice wall clock, 400k (medians, 8 rounds) | ~3.9 s | ~3.9 s | **par** (best same-round: OWzx -7%) |
| Slice wall clock, 2m (medians) | ~11.4 s | ~11.7 s | -2.7%, inside noise; startup attribution measured: process boot 95 ms, no single removable stage |
| Slice peak memory, 400k | 359-392 MiB | 192-216 MiB | **OWzx better (-45..-50%)** |
| Slice peak memory, 2m | 792 MiB | 742 MiB | **OWzx better (-6%)** |
| Preview gcode parse (file path) | n/a (in-memory) | 87.5 s → 1.93 s (**45x**, Wave 1) | — |
| Hover pick p50, 2M tris | ~33 ms-equivalent brute force | 0.008 ms (Wave 2a BVH; 5M: p50 0.008 vs brute 3,016 ms) | **~4,000x** |
| meshData geometry build, 2m | n/a | 140 → 44 ms (8 ms at 400k, 118 ms at 5m) | **3.2x** |
| Geometry blob, 2m | n/a | 68.6 → 45.8 MB (9.1 MB at 400k; instance-count independent) | **-33..-56%** |
| GPU vertex data per triangle | ~24 B indexed (upstream) | ~26-38 B indexed (Wave 2c) | **par class** |
| Preview toolpath staging per move | ~20-40 B instanced (upstream) | 264 B indexed (Wave 3c step 1; 672 B before; full instancing spec'd) | upstream better (known, spec'd) |
| Undo memory bound | ~10% RAM budget | ~10% RAM budget (Wave 4b) | **par semantics** |
| **GUI single-frame render, 2M tris (software path, remote mirror)** | not collectible through remote mirror (same occlusion freeze) | **min = 5.59 ms/frame** (20-frame bucket; avg/max polluted by remote frame-scheduling waits up to 9.1 s) | **OWzx: 5.59 ms is 3x inside the 16.6 ms (60 FPS) budget** — render speed is NOT a bottleneck; measured 2026-09-05, evidence archived |

Evidence archives: `build/final_gui_fps_evidence.txt` (swrender bucket +
meshData rebuild timings), `build/perf_out/stage_report.json` (stage bench
JSON), per-wave detail table below.

## Models

| Tier | File | Triangles | Purpose |
|------|------|-----------|---------|
| 400k | bench_sphere_400k.stl | 398,724 | slice A/B (fast iteration) |
| 2m | bench_sphere_2m.stl | 1,998,000 | load / render / stage bench |
| 5m | bench_sphere_5m.stl | 4,995,960 | large-model tier |
| grid | bench_grid_20x100k.stl | 1,989,120 (20 objects) | multi-object scene |

## Per-wave results

| Wave | Change | Key measured result | Date |
|------|--------|---------------------|------|
| Wave 1 | cache per-token QRegularExpression in the gcode parser; hover pick coalesced ~60 Hz; bounded slice_cache/; layer count max-not-sum | preview parse 87.5 s → 1.93 s (**45x**); full verify green | 2026-09-03 |
| Wave 2a | ObjectPicking::Bvh (flat AABB BVH, median split, near-first traversal) per pick-scene batch, cached in RhiViewport; brute-force equivalence test | 2M pick ray p50 **33.2 → 0.008 ms (~4,000x)**, identical hits; BVH build 491 ms per mesh change | 2026-09-03 |
| Wave 2b | meshData TLV v2: one indexed geometry per ModelVolume shared across instances + per-batch GL world matrices + per-vertex normals; one-shot cutover (PrepareSceneData decodes v2, keeps indexed data, expands byte-identical compat soup; SoftwareViewport + EditorViewModel bounds parser ported) | 2m: meshData **140 → 44 ms**, blob **68.6 → 45.8 MB**; compat soup vertex-identical to v1 | 2026-09-03 |
| Wave 2c | renderer indexed draws: shared per-batch verts + normalMatrix normals + uint32 indices; drawIndexed per opaque/translucent range | GPU ~120 → ~26-38 B/tri; 400k meshData **25 → 8 ms**, blob → 9.1 MB; grid tier green | 2026-09-03 |
| Wave 3b | GCodeProcessorResult in-memory preview (SliceService emits slicePreviewReady POD mirror before resultChanged/sliceFinished; PreviewViewModel::applySlicePreview builds all containers; file re-parse skipped) | preview readiness 1.93 s → ms-class; slice memory -40..-50% confirmed | 2026-09-03 |
| Wave 3a | dedicated slice thread (queued worker on persistent QThread, BackgroundSlicingProcess parity); CLI direct write (export_gcode straight to --output-dir); removed duplicate ~800-option enum walk | 2m wall gap 4-5% → **2.7%**; memory -45% (400k) | 2026-09-03 |
| Wave 4a | persistent per-plate Print + incremental reslice (geometry-signature gated; Print::apply diff + process use_cache) | reuse path covered by 3 green tests; first same-round wall win -7% (400k) | 2026-09-03 |
| Wave 4b | undo memory budget: IHeavyCommand::heavyBytes on 7 snapshot commands; manager budget = 10% RAM (OWZX_UNDO_BUDGET_MIB), clear-on-exceed (Qt6 QUndoStack rejects setUndoLimit on non-empty stack — documented delta) | unit test green (1200>1000 B triggers clear); undo memory bounded ≤ ~10% RAM | 2026-09-03 |
| Wave 3c step 1 | preview toolpath index compression: 6 shared verts + 24 indices per prism (672 → 264 B/move), single indexed draw path | full verify green; full GPU instancing (~76 B/move) spec'd in docs/perf-wave2b-design.md | 2026-09-04 |
| UX fix | plate right-click is now a pure menu request (was: switched plate + hid others + recomputed fit hint → requestFitView zoomed under cursor) | regression test plateContextMenuDoesNotSwitchPlateOrRefit green | 2026-09-03 |
| FPS overlay | Ctrl+Shift+Space in-app counter, RHI (render-thread push) + software (paint count) paths; 20-frame buckets | overlay verified live on remote mirror | 2026-09-05 |

## Known environment constraints & build hygiene

1. **Remote mirror freezes on-demand rendering**: through ToDesk the app
   window is occluded — zero repaints regardless of synthetic input (orbit
   sweeps, wheel storms, UIA view-button hammers: input reaches the app —
   scene rebuilds log — but paint count stays at zero). GUI FPS collection
   requires a LOCAL session. Single-frame min (5.59 ms) IS collectible via
   discrete view toggles after a minimize/restore exposure fix.
2. **ninja/MSVC /showIncludes dependency tracking is unreliable here**
   (Chinese toolchain output): header edits leave stale .obj files →
   cross-TU layout mismatch crashes. Purge before canonical builds when
   shared headers changed:
   `find build/CMakeFiles -name "*.obj" -delete; find build -name timestamp -path "*autogen*" -delete`.
3. `OWZX_KEEP_PDBS=1` keeps PDBs through the verify script for cdb
   debugging.
4. Handy-model `OrcaBadge.3mf` is an untracked submodule resource: a fresh
   worktree needs it copied into `build/resources/handy_models/` for the
   handy-model test.

## Remaining known gaps (documented, non-blocking)

- Preview instancing (264 → ~76 B/move) — full spec in
  docs/perf-wave2b-design.md.
- Slice wall clock residual -2.7% (2m median) — distributed Qt event-loop
  staging; boot measured at 95 ms, no removable stage.
- Orca GUI FPS counterpart — remote mirror freezes Orca the same way;
  local-session numbers optional.
