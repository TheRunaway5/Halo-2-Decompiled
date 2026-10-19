"""Dispatch the sandbox shortcut through the admitted MCC-only regular launcher.

The default is the separately validated local Lockout test. Menus require their
own frontend capability. No runtime, map, source or asset path is user supplied.
"""
import argparse
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

import mcc_onboarding
from native_gameplay_test import native_gameplay_test_available
from frontend_runtime_selection import selected_frontend_runtime

SCHEMA = "HALO2_MCC_SANDBOX_CHECK_V1"
POLICY = "MCC imported cache only; Xbox ISO only validates converted tags offline; no Xbox/XBE or Vista runtime assets."
SCOPE = "Interim local MCC Lockout test; full-game/TU1.5 parity remains incomplete."
MAP_NAMES = ("mainmenu.map", "shared.map", "lockout.map", "single_player_shared.map")


def positive(value):
    try:
        result = int(value, 10)
    except ValueError as error:
        raise argparse.ArgumentTypeError("expected a positive integer") from error
    if result <= 0 or result > 0xffffffff:
        raise argparse.ArgumentTypeError("expected a positive 32-bit integer")
    return result


def launcher_info(root):
    launcher = root / ("Halo2-Launcher.exe" if os.name == "nt" else "Halo2-Launcher")
    if launcher.is_symlink() or not launcher.is_file() or not os.access(launcher, os.X_OK):
        raise RuntimeError("The regular MCC launcher is unavailable; existing packages were preserved")
    outputs = []
    for option in ("--launcher-info", "--help"):
        result = subprocess.run([str(launcher), option], cwd=root,
                                capture_output=True, text=True, timeout=60)
        if result.returncode or len(result.stdout) > 65536:
            raise RuntimeError("The regular MCC launcher could not report its selection and supported actions")
        outputs.append(result.stdout)
    return launcher, *outputs


def inspect(root, launcher, info, help_text):
    selected = mcc_onboarding.selected(root)
    if selected is None:
        raise RuntimeError("No prepared MCC cache is selected; use the regular launcher's setup")
    cache, provenance = selected
    # Reuse the normal importer contract; this wrapper never grants conversion
    # readiness or turns an offline reference registry into a runtime source.
    if (provenance.get("schema") != "H2_MCC_TU15_PREPARATION_V1" or
            provenance.get("external_installation_requirement") != "Halo MCC only"):
        raise RuntimeError("The selected cache has no MCC-only preparation provenance")
    maps = cache / "halo2/h2_maps_win64_dx11"
    for name in MAP_NAMES:
        path = maps / name
        if path.is_symlink() or not path.is_file() or not path.resolve().is_relative_to(cache):
            raise RuntimeError("The MCC map lost its imported-cache owner: " + name)
        with path.open("rb") as stream:
            header = stream.read(8)
        if len(header) != 8 or struct.unpack("<II", header) != (0x68656164, 13):
            raise RuntimeError("Runtime testing requires an MCC version13 cache: " + name)

    def field(name):
        values = [line[len(name) + 2:] for line in info.splitlines() if line.startswith(name + ": ")]
        if len(values) != 1 or not values[0]:
            raise RuntimeError("The regular launcher did not identify exactly one " + name)
        return values[0]

    def same_owner(name, expected):
        value = Path(field(name))
        if not value.is_absolute() or value.resolve(strict=True) != expected.resolve(strict=True):
            raise RuntimeError("The regular launcher's " + name + " differs from the selected MCC owner")

    same_owner("Project", root)
    same_owner("MCC cache", cache)
    same_owner("MCC maps", maps)
    if field("Asset source") != "Halo MCC only" or field("MCC cache prepared") != "yes":
        raise RuntimeError("The regular launcher has not admitted the selected MCC cache")
    runtime = Path(field("Runtime"))
    if not runtime.is_absolute() or runtime.is_symlink() or not runtime.is_file():
        raise RuntimeError("The regular launcher has no selected native runtime owner")
    runtime = runtime.resolve(strict=True)
    native = native_gameplay_test_available(root, runtime, cache)
    frontend_runtime = selected_frontend_runtime(root, runtime, cache)
    frontend = frontend_runtime is not None
    if frontend and frontend_runtime != runtime:
        same_owner("Frontend runtime", frontend_runtime)
    explicit_native = bool(re.search(r"(?<![\w-])--native-test(?=[\s,;\[\]]|$)", help_text))
    # Older installed launchers expose their qualified Lockout action through
    # Play. That fallback is admissible only when actual selection is exact
    # and no independently admitted frontend could change its meaning.
    native_flag = "--native-test" if native and explicit_native else None
    if native and not explicit_native and not frontend and field("Play") == "native Lockout gameplay test":
        native_flag = "--launch-game"
    menu_flag = "--launch-game" if (explicit_native and frontend and
        field("Native MCC frontend") == "available") else None
    return dict(schema=SCHEMA, root=str(root), launcher=str(launcher), runtime=str(runtime),
        cache=str(cache), maps=str(maps), cache_version=13, checked_maps=list(MAP_NAMES),
        native_test_available=native, frontend_available=frontend, asset_policy=POLICY,
        frontend_runtime=str(frontend_runtime) if frontend_runtime is not None else None,
        explicit_native_test_supported=explicit_native, selected_flag=native_flag,
        menu_launch_flag=menu_flag,
        scope=SCOPE, full_game_parity=False, runtime_launched=False)


def main(root, argv=None):
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    route = parser.add_mutually_exclusive_group()
    route.add_argument("--menu", action="store_true", help="Open the separately admitted MCC frontend")
    route.add_argument("--launcher-info", action="store_true", help="Read the regular launcher's actual selection")
    route.add_argument("--check", action="store_true", help="Report read-only MCC/native admission as JSON")
    parser.add_argument("--hidden", action="store_true")
    parser.add_argument("--mute", action="store_true")
    parser.add_argument("--validation", action="store_true")
    parser.add_argument("--frames", type=positive)
    parser.add_argument("--viewport", type=positive, nargs=2, metavar=("WIDTH", "HEIGHT"))
    args = parser.parse_args(argv)
    if args.viewport and any(value < 64 or value > 4096 for value in args.viewport):
        parser.error("native viewport dimensions must be64..4096")
    if (args.check or args.launcher_info) and (
            args.hidden or args.mute or args.validation or args.frames or args.viewport):
        parser.error("read-only inspection accepts no runtime controls")
    root = Path(root).resolve(strict=True)
    try:
        launcher, info, help_text = launcher_info(root)
        if args.launcher_info:
            print(info, end="" if info.endswith("\n") else "\n")
            print("Sandbox asset policy: " + POLICY)
            print("Sandbox default: " + SCOPE)
        report = inspect(root, launcher, info, help_text)
        if args.launcher_info:
            print("Sandbox native test: " + ("available" if report["native_test_available"] else "unavailable"))
            print("Sandbox MCC menus: " + ("available" if report["frontend_available"] else "unavailable"))
            print("Sandbox native action: " + (report["selected_flag"] or "unavailable"))
            return 0
        if args.check:
            report["passed"] = bool(report["selected_flag"])
            print(json.dumps(report, indent=2))
            return 0 if report["passed"] else 1
        if args.menu and not report["menu_launch_flag"]:
            raise RuntimeError("The selected runtime has no independently validated MCC frontend; menus were not launched")
        if not args.menu and not report["selected_flag"]:
            raise RuntimeError("The selected launcher has no unambiguous current validated MCC Lockout action; no preview fallback was launched")
        command = [str(launcher), report["menu_launch_flag"] if args.menu else report["selected_flag"]]
        if args.menu:
            command.append("--menu")
        for option in ("hidden", "mute", "validation"):
            if getattr(args, option):
                command.append("--" + option)
        if args.frames:
            command.extend(("--frames", str(args.frames)))
        if args.viewport:
            command.extend(("--viewport", *map(str, args.viewport)))
        os.execv(str(launcher), command)
        raise RuntimeError("The regular MCC launcher did not replace the shortcut process")
    except (OSError, ValueError, KeyError, RuntimeError, subprocess.SubprocessError) as error:
        if args.check:
            print(json.dumps(dict(schema=SCHEMA, passed=False, runtime_launched=False,
                asset_policy=POLICY, full_game_parity=False, error=str(error)[:1500]), indent=2))
        else:
            print("Halo2 Sandbox: " + str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(Path(__file__).resolve().parents[1]))
