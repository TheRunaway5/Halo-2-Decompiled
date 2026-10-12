#!/usr/bin/env python3
"""Exercise actual launcher binaries with disposable source-setup/runtime recorders.

Checks argument routing, required dual-source setup, setup failure/cancellation,
help without assets and explicit overrides. Does not decode assets or render.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
LAUNCHERS = ("Halo2-Launcher", "Halo2-Title-Launcher")
RECORDER = '''import json, os, sys
from pathlib import Path
Path(os.environ["HALO2_LAUNCHER_ARGV_CAPTURE"]).write_text(json.dumps({"argv":sys.argv,"cwd":os.getcwd(),"discord_launcher":os.environ.get("H2_DISCORD_LAUNCHER")}))
'''
SETUP = '''import json, os, sys
from pathlib import Path
Path(os.environ["HALO2_SETUP_CAPTURE"]).write_text(json.dumps(sys.argv[1:]))
status=int(os.environ.get("HALO2_SETUP_STATUS", "0"))
if status: sys.exit(status)
root=Path(sys.argv[sys.argv.index("--root")+1])
(root/"assets/asset-sources.tsv").write_text("H2_ASSET_SOURCES_V1\\nxbox\\t"+str(root/"Xbox files")+"\\nvista\\t"+str(root/"Vista UI")+"\\n")
'''
MCC_SETUP = '''import json, os, sys
from pathlib import Path
Path(os.environ["HALO2_SETUP_CAPTURE"]).write_text(json.dumps(sys.argv[1:]))
status=int(os.environ.get("HALO2_SETUP_STATUS", "0"))
if status: sys.exit(status)
root=Path(sys.argv[sys.argv.index("--root")+1])
(root/"assets/hud-source.tsv").write_text("H2_HUD_SOURCE_V1\\nmcc\\t"+str(root/"MCC Classic HUD")+"\\n")
'''
MCC_MENU_SETUP = MCC_SETUP.replace('assets/hud-source.tsv', 'assets/menu-source.tsv').replace(
    'H2_HUD_SOURCE_V1', 'H2_MENU_SOURCE_V1').replace('MCC Classic HUD', 'MCC Classic Menus')

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(build):
    cases = []
    with tempfile.TemporaryDirectory(prefix="halo2-launcher-setup-") as directory:
        root = Path(directory) / "project with spaces"
        binaries = root / "bin"
        (binaries / "shaders").mkdir(parents=True)
        source = (ROOT / "src/launcher.cpp").read_text()
        names = re.findall(r'"([^"\n]+\.spv)"', re.search(r'shader_names\{(.*?)\};', source, re.S).group(1))
        for name in names:
            (binaries / "shaders" / name).write_bytes(b"locate marker")
        runtime = binaries / "halo2_native"
        runtime.write_text("#!" + sys.executable + "\n" + RECORDER)
        runtime.chmod(0o755)
        (root / "tools").mkdir()
        (root / "tools/setup_assets.py").write_text(SETUP)
        (root / "tools/import_mcc_hud.py").write_text(MCC_SETUP)
        (root / "tools/import_mcc_menus.py").write_text(MCC_MENU_SETUP)
        xbox, vista = root / "Xbox files", root / "Vista UI"
        for base, files in ((xbox, ["default.xbe", "maps/mainmenu.map", "maps/shared.map", "maps/lockout.map", "maps/fonts/font_table.txt"]),
                            (vista, ["pack.tsv", "hud.tsv", "maps/mainmenu.map", "maps/shared.map", "maps/lockout.map", "fonts/font"])):
            for name in files:
                p = base / name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_bytes(b"setup marker")
        (root / "assets").mkdir()
        registry = root / "assets/asset-sources.tsv"
        registry_text = f"H2_ASSET_SOURCES_V1\nxbox\t{xbox}\nvista\t{vista}\n"
        capture, setup_capture = root / "runtime.json", root / "setup.json"
        env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
                   HALO2_LAUNCHER_ARGV_CAPTURE=str(capture), HALO2_SETUP_CAPTURE=str(setup_capture),
                   H2_DISCORD_LAUNCHER="/stale/other-install/Halo2-Launcher")
        for name in LAUNCHERS:
            launcher = binaries / name
            shutil.copy2(build / name, launcher)
            def invoke(label, supplied, expected=None, status=0, setup_args=None):
                capture.unlink(missing_ok=True)
                setup_capture.unlink(missing_ok=True)
                run = subprocess.run([str(launcher), *supplied], cwd=directory, env=env,
                                     capture_output=True, text=True, timeout=15)
                actual = json.loads(capture.read_text()) if capture.exists() else None
                prepared = json.loads(setup_capture.read_text()) if setup_capture.exists() else None
                checks = [run.returncode == status, prepared == setup_args]
                if expected is None:
                    checks.append(actual is None)
                else:
                    expected=list(expected)
                    compared=actual.copy() if actual else {}
                    # Pause assets have their own Vista default, independent
                    # of the selected shell pack. Compare that required pair
                    # separately; keep every other routed argument exact.
                    if "--help" not in supplied and "--pause-ui-assets" not in supplied and actual:
                        argv=actual["argv"].copy()
                        checks.append(argv.count("--pause-ui-assets")==1)
                        if "--pause-ui-assets" in argv:
                            index=argv.index("--pause-ui-assets")
                            checks.append(argv[index+1]==str(vista))
                            del argv[index:index+2]
                        if "--pause-ui-assets" in expected:
                            index=expected.index("--pause-ui-assets")
                            del expected[index:index+2]
                        compared["argv"]=argv
                    checks.append(compared == {"argv":[str(runtime), *expected], "cwd":str(root),
                                             "discord_launcher":str(launcher)})
                cases.append(dict(launcher=name, case=label, passed=all(checks), status=run.returncode,
                                  supplied=supplied, recorded=actual, setup=prepared, stderr=run.stderr))
            # No registry or legacy assets/disc directory is needed to locate/setup.
            registry.unlink(missing_ok=True)
            invoke("info-without-assets", ["--launcher-info"])
            invoke("help-without-assets", ["--help"], ["--help"])
            env["HALO2_SETUP_STATUS"] = "5"
            invoke("cancel-blocks-runtime", [], status=5,
                   setup_args=["--root", str(root), "--interactive"])
            env.pop("HALO2_SETUP_STATUS")
            invoke("initial-setup-only", ["--setup-only", "--vista-source", "Vista installer.zip", "--xbox-source", "Xbox.iso"],
                   setup_args=["--root", str(root), "--vista", "Vista installer.zip", "--xbox", "Xbox.iso"])
            assert registry.read_text() == registry_text
            default = ["--menu-screen", "9"] + (["--skip-intro"] if name == "Halo2-Title-Launcher" else [])
            menu_options = ["--menu-maps", str(xbox / "maps"), "--menu-ui-assets", str(vista),
                            "--menu-loading-assets", str(root / "assets/menu-ui/vista/loading"),
                            "--menu-vista-settings", str(vista / "maps/mainmenu.map")]
            hud_options = ["--hud-ui-assets", str(vista)]
            routes = [("default", [], default), ("menu", ["--menu"], []),
                      ("screen", ["--menu-screen", "19"], []), ("sweep", ["--menu-sweep", "output"], []),
                      ("audit", ["--menu-audit"], []), ("movie", ["--menu-movie", "intro"], [])]
            for label, supplied, prefix in routes:
                invoke(label, supplied, prefix + menu_options + hud_options + supplied)
                override = supplied + ["--menu-ui-assets", "", "--hud-ui-assets", ""]
                invoke(label + "-explicit-original", override,
                       prefix + ["--menu-maps", str(xbox / "maps"),
                                 "--menu-loading-assets", str(root / "assets/menu-ui/vista/loading"),
                                 "--menu-vista-settings", str(vista / "maps/mainmenu.map")] + override)
            invoke("explicit-menu-maps", ["--menu-maps", "alternate/maps"],
                   menu_options[2:] + hud_options + ["--menu-maps", "alternate/maps"])
            for route in (["--multiplayer"], ["--host", "2002"], ["--local-players", "2"], ["--dedicated"], ["--play"], ["--simulate", "1"]):
                invoke("game-" + route[0], route, hud_options + ["--map", str(xbox / "maps/lockout.map")] + route)
            invoke("explicit-map", ["--map", "another.map"], hud_options + ["--map", "another.map"])
            # The normal title route must carry the same selected gameplay
            # executable into the eventual in-window multiplayer session.
            update = root / "TU 1.5/default.xbe"
            update.parent.mkdir(exist_ok=True)
            update.write_bytes(b"runtime validates the executable")
            gameplay = root / "assets/gameplay-source.tsv"
            gameplay.write_text("H2_GAMEPLAY_SOURCE_V1\nxbe\tTU 1.5/default.xbe\n")
            selected = ["--gameplay-xbe", str(update)]
            invoke("title-retains-selected-update", [], selected + default + menu_options + hud_options)
            invoke("multiplayer-retains-selected-update", ["--multiplayer"],
                   selected + hud_options + ["--map", str(xbox / "maps/lockout.map"), "--multiplayer"])
            invoke("explicit-gameplay-override", ["--multiplayer", "--gameplay-xbe", "other.xbe"],
                   hud_options + ["--map", str(xbox / "maps/lockout.map"),
                                  "--multiplayer", "--gameplay-xbe", "other.xbe"])
            gameplay.write_text("H2_GAMEPLAY_SOURCE_V1\nxbe\tmissing.xbe\n")
            invoke("missing-selected-update-blocks-fallback", [], status=1)
            invoke("explicit-gameplay-bypasses-stale-selection", ["--gameplay-xbe", "other.xbe"],
                   default + menu_options + hud_options + ["--gameplay-xbe", "other.xbe"])
            invoke("help-with-stale-selection", ["--help"], ["--help"])
            gameplay.write_text("H2_GAMEPLAY_SOURCE_V1\nxbe\tTU 1.5/default.xbe\nextra\tunexpected\n")
            invoke("malformed-gameplay-selection-blocks-fallback", [], status=1)
            gameplay.unlink()
            # A selected MCC import replaces HUD/font sampling only. Menus
            # retain their independently selected Vista resource owner.
            mcc = root / "MCC Classic HUD"
            for asset in ("hud.tsv", "hud-source.tsv", "scoreboard.tsv", "fonts/font_table.txt"):
                path = mcc / asset
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"MCC import marker")
            hud_selection = root / "assets/hud-source.tsv"
            hud_selection.write_text("H2_HUD_SOURCE_V1\nmcc\t" + str(mcc) + "\n")
            invoke("mcc-import-only", ["--import-mcc-hud", "--setup-only"],
                   setup_args=["--root", str(root), "--activate"])
            invoke("mcc-import-explicit-path", ["--mcc-source", "MCC owned/game path", "--setup-only"],
                   setup_args=["--root", str(root), "--activate", "--mcc-root", "MCC owned/game path"])
            before_hud = hud_selection.read_text()
            env["HALO2_SETUP_STATUS"] = "7"
            invoke("mcc-failed-import-keeps-selection", ["--import-mcc-hud", "--setup-only"], status=7,
                   setup_args=["--root", str(root), "--activate"])
            assert hud_selection.read_text() == before_hud
            env.pop("HALO2_SETUP_STATUS")
            invoke("mcc-hud-default-keeps-vista-menu", [], default + menu_options + ["--hud-ui-assets", str(mcc)])
            invoke("mcc-hud-multiplayer", ["--multiplayer"],
                   ["--hud-ui-assets", str(mcc), "--map", str(xbox / "maps/lockout.map"), "--multiplayer"])
            invoke("mcc-hud-explicit-override", ["--hud-ui-assets", str(vista)],
                   default + menu_options + ["--hud-ui-assets", str(vista)])
            (mcc / "hud.tsv").unlink()
            invoke("mcc-hud-incomplete-blocks-fallback", [], status=1)
            hud_selection.unlink()
            menus = root / "MCC Classic Menus"
            for asset in ("pack.tsv", "vista.tsv", "pause.tsv", "menu-source.tsv", "menu-ui-source.tsv", "fonts/font_table.txt"):
                path = menus / asset
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"MCC menu marker")
            menu_selection = root / "assets/menu-source.tsv"
            invoke("mcc-menus-import-only", ["--import-mcc-menus", "--setup-only"],
                   setup_args=["--root", str(root), "--activate"])
            invoke("mcc-menus-import-explicit-source", ["--mcc-source", "Owned MCC", "--import-mcc-menus", "--setup-only"],
                   setup_args=["--root", str(root), "--activate", "--mcc-root", "Owned MCC"])
            before_menu = menu_selection.read_text()
            env["HALO2_SETUP_STATUS"] = "7"
            invoke("mcc-menus-failed-import-keeps-selection", ["--import-mcc-menus", "--setup-only"], status=7,
                   setup_args=["--root", str(root), "--activate"])
            assert menu_selection.read_text() == before_menu
            env.pop("HALO2_SETUP_STATUS")
            selected_menu = menu_options.copy()
            selected_menu[3] = str(menus)
            pause_options = ["--pause-ui-assets", str(menus)]
            invoke("mcc-menus-default", [], default + selected_menu + hud_options + pause_options)
            for route in (["--multiplayer"], ["--play"], ["--campaign"]):
                invoke("selected-menu-keeps-vista-pause-" + route[0], route,
                       hud_options + pause_options + ["--map", str(xbox / "maps/lockout.map")] + route)
            invoke("mcc-menus-explicit-overrides", ["--menu-ui-assets", str(vista), "--pause-ui-assets", ""],
                   default + selected_menu[:2] + selected_menu[4:] + hud_options +
                   ["--menu-ui-assets", str(vista), "--pause-ui-assets", ""])
            (mcc / "hud.tsv").write_bytes(b"MCC HUD marker")
            hud_selection.write_text("H2_HUD_SOURCE_V1\nmcc\t" + str(mcc) + "\n")
            invoke("mcc-menus-and-hud-independent", [],
                   default + selected_menu + ["--hud-ui-assets", str(mcc)] + pause_options)
            hud_selection.unlink()
            (menus / "pause.tsv").unlink()
            invoke("mcc-menus-incomplete-blocks-fallback", [], status=1)
            (menus / "pause.tsv").write_bytes(b"MCC pause marker")
            menu_selection.write_text("H2_MENU_SOURCE_V1\nmcc\tmissing\n")
            invoke("mcc-menus-missing-blocks-fallback", [], status=1)
            menu_selection.write_text("H2_MENU_SOURCE_V1\nmcc\t" + str(menus) + "\nextra\tunexpected\n")
            invoke("mcc-menus-malformed-blocks-fallback", [], status=1)
            menu_selection.unlink()
            before = registry.read_text()
            env["HALO2_SETUP_STATUS"] = "7"
            invoke("failed-reimport-keeps-selection", ["--setup"], status=7,
                   setup_args=["--root", str(root), "--interactive"])
            assert registry.read_text() == before
            env.pop("HALO2_SETUP_STATUS")
            (vista / "hud.tsv").unlink()
            env["HALO2_SETUP_STATUS"] = "5"
            invoke("missing-hud-requires-setup", [], status=5,
                   setup_args=["--root", str(root), "--interactive"])
            env.pop("HALO2_SETUP_STATUS")
            (vista / "hud.tsv").write_bytes(b"setup marker")
            for dependency in ("shared.map", "lockout.map"):
                path = vista / "maps" / dependency
                path.unlink()
                env["HALO2_SETUP_STATUS"] = "5"
                invoke("missing-match-" + dependency + "-requires-setup", [], status=5,
                       setup_args=["--root", str(root), "--interactive"])
                env.pop("HALO2_SETUP_STATUS")
                path.write_bytes(b"setup marker")
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build-frontend")
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/asset-setup/launcher.json")
    args = parser.parse_args()
    report = dict(schema="halo2-launcher-dual-source-v2", scope=__doc__, passed=False)
    try:
        report["binaries"] = {name:digest(args.build / name) for name in LAUNCHERS}
        report["cases"] = verify(args.build.resolve())
        report["passed"] = all(case["passed"] for case in report["cases"])
        report["case_count"] = len(report["cases"])
    except Exception as error:
        report["error"] = str(error)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(("PASS" if report["passed"] else "FAIL") + ": " + str(args.output))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
