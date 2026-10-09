#!/usr/bin/env python3
"""ui_diff_compare.py -- pixel diff of a captured OWzx window against the
upstream truth screenshot (docs/ui-reference/restoration-map.md §5 loop).

Usage:
  python scripts/ui_diff_compare.py --ours <png> --upstream <png> --out <dir> [--page prepare]

Outputs into --out:
  diffmask.png   white = pixel differs beyond threshold
  sbs.png        stacked ours/upstream for eyeballing
  metrics.json   diff percentages (overall + sidebar/content bands)

A diff above the ~3.5% font-rasterization noise floor means real layout or
palette deltas; inspect diffmask.png and the restoration-map region table.
"""
import argparse
import json
import os

from PIL import Image, ImageChops

DESIGN_W, DESIGN_H = 1366, 721
SIDEBAR_W = 392          # upstream-measured sidebar width incl. scrollbar (R8)
THRESHOLD = 40


def load_window(path):
    im = Image.open(path).convert("RGB")
    return im.crop((0, 0, min(im.width, DESIGN_W), min(im.height, DESIGN_H)))


def band_diff(a, b, box):
    da = a.crop(box); db = b.crop(box)
    h = ImageChops.difference(da, db).convert("L").histogram()
    total = (box[2] - box[0]) * (box[3] - box[1])
    return 100.0 * sum(h[THRESHOLD + 1:]) / total


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ours", required=True)
    ap.add_argument("--upstream", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--page", default="prepare")
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    ours = load_window(args.ours)
    upstream = load_window(args.upstream)
    if ours.size != (DESIGN_W, DESIGN_H):
        print(f"[diff] WARNING: ours is {ours.size}, expected {DESIGN_W}x{DESIGN_H}; "
              f"check the capture window size (--ClientWidth/--ClientHeight).")

    diff = ImageChops.difference(upstream, ours).convert("L")
    hist = diff.histogram()
    overall = 100.0 * sum(hist[THRESHOLD + 1:]) / (DESIGN_W * DESIGN_H)
    metrics = {
        "page": args.page,
        "diff_percent_overall": round(overall, 2),
        "diff_percent_sidebar_band": round(band_diff(upstream, ours, (0, 0, SIDEBAR_W, DESIGN_H)), 2),
        "diff_percent_content_band": round(band_diff(upstream, ours, (SIDEBAR_W, 0, DESIGN_W, DESIGN_H)), 2),
        "threshold": THRESHOLD,
        "noise_floor_reference": 3.5,
        "upstream": args.upstream,
        "ours": args.ours,
    }

    diff.point(lambda p: 255 if p > THRESHOLD else 0).save(
        os.path.join(args.out, "diffmask.png"))
    comp = Image.new("RGB", (DESIGN_W, DESIGN_H * 2 + 8), (255, 0, 0))
    comp.paste(upstream, (0, 0)); comp.paste(ours, (0, DESIGN_H + 8))
    comp.save(os.path.join(args.out, "sbs.png"))
    with open(os.path.join(args.out, "metrics.json"), "w", encoding="utf-8") as f:
        json.dump(metrics, f, indent=2)

    print(f"[diff] page={metrics['page']} overall={metrics['diff_percent_overall']}% "
          f"sidebar_band={metrics['diff_percent_sidebar_band']}% "
          f"content_band={metrics['diff_percent_content_band']}%")
    print(f"[diff] artifacts in {args.out}: diffmask.png sbs.png metrics.json")


if __name__ == "__main__":
    main()
