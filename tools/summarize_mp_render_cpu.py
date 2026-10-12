#!/usr/bin/env python3
"""Summarize actual submitted runtime frame timing after thirty warmup frames."""
from pathlib import Path
import csv
import json
import statistics
import sys


def summary(path, views):
    with path.open() as stream:
        rows = [r for r in csv.DictReader(stream, delimiter="\t")
                if int(r["frame"]) >= 30 and int(r["submitted"])]
    if len(rows) != 180:
        raise ValueError(f"Expected 180 post-warmup submitted frames: {path}")
    for row in rows:
        for phase in ("camera_lighting", "static_world", "objects", "body", "first_person", "hud"):
            row[phase + "_ms"] = sum(float(row[f"v{i}_{phase}_ms"]) for i in range(views))
        row["prepare_ms"] = float(row["wall_ms"]) - float(row["renderer_ms"])
    phases = {}
    for key in ("input_ms", "common_ms", "camera_lighting_ms", "static_world_ms", "objects_ms",
                "body_ms", "first_person_ms", "hud_ms", "prepare_ms", "renderer_ms", "wall_ms"):
        values = sorted(float(row[key]) for row in rows)
        phases[key] = {"median": statistics.median(values), "mean": statistics.mean(values),
                       "p95": values[int(.95 * (len(values) - 1))]}
    return {"views": views, "frames": len(rows), "phases": phases,
            "inverse_mean_active_frame_ms_per_second": 1000 / phases["wall_ms"]["mean"],
            "body_pose_builds": sorted({int(r["body_pose_builds"]) for r in rows}),
            "bodies_drawn": sorted({int(r["bodies_drawn"]) for r in rows})}


def main():
    root = Path(sys.argv[1])
    result = {}
    for revision in ("before", "after"):
        result[revision] = {name: summary(root / f"runtime-{name}-{revision}.tsv", views)
                            for name, views in (("one", 1), ("four", 4))}
    (root / "runtime-summary.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
