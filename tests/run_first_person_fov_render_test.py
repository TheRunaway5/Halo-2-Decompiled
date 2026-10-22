"""Render actual authored weapon poses into owned disposable framebuffers."""
import os
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from validate_weapon_runtime_tu15 import private_display
from weapon_test_cache import require_mcc_map


def main():
    executable, maps, weapon = sys.argv[1:]
    if not maps or not (Path(maps) / "lockout.map").is_file():
        return 77
    require_mcc_map(Path(maps) / "lockout.map")
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    output = scratch / weapon
    output.mkdir()
    root = Path(__file__).resolve().parents[1]
    with private_display(output) as display:
        environment = dict(os.environ, DISPLAY=display, SDL_VIDEODRIVER="x11",
                           XDG_CACHE_HOME=str(scratch / "cache"))
        layers = root / ".tools/validation/usr/share/vulkan/explicit_layer.d"
        if layers.is_dir():
            environment["VK_LAYER_PATH"] = str(layers)
            environment["LD_LIBRARY_PATH"] = ":".join([
                str(root / ".tools/validation/usr/lib"),
                str(root / ".tools/validation/usr/lib/x86_64-linux-gnu"),
                environment.get("LD_LIBRARY_PATH", "")])
        return subprocess.run([executable, maps, str(output), weapon,
                               str(root / "tests/fixtures/tu15_split_first_person_fov.tsv")],
                              env=environment, timeout=80, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
