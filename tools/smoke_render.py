#!/usr/bin/env python3
"""Exercise every recovered BSP on a real SDL/Vulkan surface; write scoped evidence."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=Path("build/halo2_native"))
    parser.add_argument("--maps", type=Path, default=Path("assets/disc/maps"))
    parser.add_argument("--output", type=Path, default=Path("analysis/render-smoke"))
    parser.add_argument("--validation", action="store_true")
    parser.add_argument("--screenshots", action="store_true")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    binary_sha256 = hashlib.sha256(args.binary.read_bytes()).hexdigest()
    results = []
    for path in sorted(args.maps.glob("*.map")):
        info = subprocess.run([str(args.binary.resolve()), "--inspect", str(path.resolve())], capture_output=True, text=True, timeout=30, check=True)
        metadata = json.loads(info.stdout)
        for bsp in range(metadata["bsps"]):
            name = f"{path.stem}-bsp-{bsp}"
            command = [str(args.binary.resolve()), "--map", str(path.resolve()), "--bsp", str(bsp), "--frames", "3", "--hidden", "--overview"]
            if args.validation:
                command.append("--validation")
            if args.screenshots:
                command += ["--screenshot", str((args.output / f"{name}.ppm").resolve())]
            try:
                run = subprocess.run(command, capture_output=True, text=True, timeout=45)
                output = run.stdout + run.stderr
                ok = run.returncode == 0 and "Rendered 3 frames" in run.stdout and "Vulkan validation:" not in output
            except subprocess.TimeoutExpired as error:
                output, ok = str(error), False
            (args.output / f"{name}.log").write_text(output)
            results.append({"map": path.name, "bsp": bsp, "passed": ok})
            print(f"{name}: {'passed' if ok else 'FAILED'}", flush=True)
    if not results:
        parser.error("no BSPs found")
    failed = sum(not item["passed"] for item in results)
    report = {"binary": str(args.binary.resolve()), "binary_sha256": binary_sha256,
              "bsps": len(results), "passed": len(results) - failed, "failed": failed,
              "validation": args.validation, "frames_per_bsp": 3,
              "scope": "SDL/Vulkan rendering smoke, not gameplay or retail visual parity",
              "results": results}
    (args.output / "summary.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Render smoke: {len(results) - failed}/{len(results)} BSPs passed")
    raise SystemExit(1 if failed else 0)


if __name__ == "__main__":
    main()
