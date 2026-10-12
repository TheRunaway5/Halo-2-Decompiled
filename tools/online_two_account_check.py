#!/usr/bin/env python3
"""Prepare and launch isolated, manually authenticated ONLINE test clients.

This uses the installed native game and its existing Discord login UI. It does
not automate OAuth, send invitations, join queues or open a microphone.
"""
import argparse
import ctypes
import ctypes.util
import datetime
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
STATE = ROOT / ".local/two-account-check"
ORIGIN = "https://master.halotoforever.com"


def digest(path):
    with path.open("rb") as file:
        return hashlib.file_digest(file, "sha256").hexdigest()


def preference_path():
    """Use the same SDL path producer as the game, without starting SDL."""
    lib = ctypes.CDLL(ctypes.util.find_library("SDL2"))
    lib.SDL_GetPrefPath.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.SDL_GetPrefPath.restype = ctypes.c_void_p
    lib.SDL_free.argtypes = [ctypes.c_void_p]
    value = lib.SDL_GetPrefPath(b"Halo2Native", b"Halo2")
    if not value:
        raise RuntimeError("SDL could not locate the native profile directory")
    try:
        return Path(os.fsdecode(ctypes.string_at(value))) / "menu.cfg"
    finally:
        lib.SDL_free(value)


def shader_digest(directory):
    checksum = hashlib.sha256()
    shaders = sorted(directory.glob("*.spv"))
    if not shaders:
        raise RuntimeError("The native shader directory is empty")
    for path in shaders:
        checksum.update(path.name.encode() + b"\0" + digest(path).encode() + b"\n")
    return checksum.hexdigest()


def inspect_installation(launcher=None):
    launcher = launcher or ROOT / "Halo2-Title-Launcher"
    info = subprocess.run([str(launcher), "--launcher-info"], cwd=ROOT,
                          check=True, text=True, capture_output=True).stdout
    fields = dict(line.split(": ", 1) for line in info.splitlines() if ": " in line)
    runtime = Path(fields["Runtime"])
    vista = Path(fields["Vista HUD/GUI"])
    xbox = Path(fields["Xbox game files"])
    sdk = runtime.parent / "libdiscord_partner_sdk.so"
    notices = runtime.parent / "Discord-Social-SDK-Notices.txt"
    if fields.get("Game files ready") != "yes":
        raise RuntimeError("The installed launcher needs its game assets")
    for path in [runtime, sdk, notices, vista / "pack.tsv",
                 vista / "maps/mainmenu.map", xbox / "maps/lockout.map"]:
        if not path.is_file():
            raise RuntimeError("A required installed game/SDK asset is missing: " + str(path))
    linked = subprocess.run(["ldd", str(runtime)], check=True, text=True,
                            capture_output=True).stdout
    if "libdiscord_partner_sdk.so => " + str(sdk) not in linked:
        raise RuntimeError("The installed runtime does not load its bundled Discord SDK")
    return {"launcher": str(launcher), "runtime": str(runtime),
            "runtime_sha256": digest(runtime), "launcher_sha256": digest(launcher),
            "sdk_sha256": digest(sdk), "vista": str(vista), "xbox": str(xbox),
            "shaders_sha256": shader_digest(runtime.parent / "shaders"),
            "asset_registry_sha256": digest(ROOT / "assets/asset-sources.tsv")}


def pin_installation(launcher=None):
    source = inspect_installation(launcher)
    runtime = Path(source["runtime"])
    directory = STATE / ("runtime-" + source["runtime_sha256"][:16])
    if not directory.exists():
        directory.mkdir(mode=0o700)
        for path in [Path(source["launcher"]), runtime,
                     runtime.parent / "libdiscord_partner_sdk.so",
                     runtime.parent / "Discord-Social-SDK-Notices.txt"]:
            shutil.copy2(path, directory / path.name)
        # The native renderer's shader path is compiled into the executable.
        # Keep it explicit and check its content hash before each launch.
        (directory / "shaders").symlink_to(runtime.parent / "shaders", target_is_directory=True)
    pinned = inspect_installation(directory / "Halo2-Title-Launcher")
    for key in ["runtime_sha256", "launcher_sha256", "sdk_sha256", "shaders_sha256"]:
        if pinned[key] != source[key]:
            raise RuntimeError("The installed build changed while copying; run prepare again")
    return pinned


def player_dir(player):
    return STATE / ("player-" + str(player))


def environment(player, smoke=False):
    env = os.environ.copy()
    env["XDG_DATA_HOME"] = str(player_dir(player) / "data")
    env["H2_ACCOUNT_URL"] = env["H2_MATCHMAKING_URL"] = ORIGIN
    env["H2_MASTER_URLS"] = ORIGIN
    for name in ["H2_ACCOUNT_ALLOW_LOOPBACK_HTTP", "H2_MATCHMAKING_ALLOW_LOOPBACK_HTTP",
                 "H2_MASTER_ALLOW_LOOPBACK_HTTP"]:
        env.pop(name, None)
    if smoke:
        env["SDL_AUDIODRIVER"] = "dummy"
    return env


def actual_player_preferences(player):
    output = subprocess.run([sys.executable, str(Path(__file__).resolve()),
                             "--print-preference-path"], env=environment(player),
                            text=True, capture_output=True, check=True).stdout.strip()
    expected = player_dir(player) / "data/Halo2Native/Halo2/menu.cfg"
    if Path(output) != expected:
        raise RuntimeError("SDL profile isolation did not select the prepared directory")
    return expected


def locked_file(player):
    path = player_dir(player) / "running.lock"
    fd = os.open(path, os.O_CREAT | os.O_RDWR, 0o600)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        os.close(fd)
        raise RuntimeError("Player " + str(player) + " already has a running test client") from None
    return fd


def desktop_quote(value):
    text = str(value)
    for character in ("\\", '"', '`', '$'):
        text = text.replace(character, "\\" + character)
    return '"' + text + '"'


def prepare(launcher=None):
    STATE.mkdir(parents=True, exist_ok=True, mode=0o700)
    installation = pin_installation(launcher)
    for player in [1, 2]:
        directory = player_dir(player)
        directory.mkdir(exist_ok=True, mode=0o700)
        fd = locked_file(player)
        try:
            pref = actual_player_preferences(player)
            pref.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
            # Preserve any test settings already chosen during an earlier run.
            if not pref.exists():
                with pref.open("x") as file:
                    file.write('halo2-native-menu-v1\npc_video 1280 720 0\n'
                               f'active "Test Pilot {player}"\nprofile "Test Pilot {player}"\n')
                pref.chmod(0o600)
        finally:
            os.close(fd)
        entry = ("[Desktop Entry]\nType=Application\n"
                 f"Name=Halo 2 ONLINE Test - Player {player}\n"
                 "Comment=Isolated profile; sign into Discord manually\n"
                 f"Exec={desktop_quote(sys.executable)} {desktop_quote(Path(__file__).resolve())} launch {player}\n"
                 f"Icon={ROOT / 'resources/launcher.svg'}\nTerminal=true\nCategories=Game;\n")
        shortcut = STATE / f"Player {player}.desktop"
        shortcut.write_text(entry)
        shortcut.chmod(0o755)
    normal = preference_path()
    manifest = {"prepared_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                "installation": installation, "origin": ORIGIN,
                "normal_profile": str(normal), "callback_port": 2003,
                "profile_directories": [str(player_dir(p)) for p in [1, 2]],
                "automatic_login": False, "automatic_invites": False,
                "automatic_microphone": False}
    (STATE / "prepared.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (STATE / "prepared.json").chmod(0o600)
    print("Prepared Player 1 and Player 2 with separate SDL profile directories.")
    print("Pinned native runtime; installed Vista assets and bundled Discord SDK verified.")
    print("No login, invitation, queue or microphone was started.")
    print("Open either Player shortcut in " + str(STATE))


def launch(player, smoke=False, validation=False):
    manifest = json.loads((STATE / "prepared.json").read_text())
    installation = inspect_installation(Path(manifest["installation"]["launcher"]))
    if installation != manifest["installation"]:
        raise RuntimeError("The prepared build/assets/shaders changed; run prepare again before testing")
    pref = actual_player_preferences(player)
    if not pref.is_file():
        raise RuntimeError("Run prepare before launching this player")
    normal = preference_path()
    normal_before = digest(normal) if normal.is_file() else None
    fd = locked_file(player)
    arguments = [installation["launcher"], "--menu-screen", "20", "--skip-intro",
                 "--viewport", "1280", "720"]
    if smoke:
        arguments += ["--frames", "3", "--hidden", "--screenshot",
                      str(player_dir(player) / "signed-out.ppm")]
        if validation:
            arguments.append("--validation")
    log = player_dir(player) / ("smoke.log" if smoke else "client.log")
    try:
        with log.open("w") as output:
            log.chmod(0o600)
            print(f"Player {player}: the ONLINE sign-in screen will open. Login is manual.", flush=True)
            result = subprocess.run(arguments, cwd=ROOT, env=environment(player, smoke),
                                    pass_fds=(fd,), stdout=output, stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError("The native client exited with code " + str(result.returncode) + "; see " + str(log))
        if smoke:
            text = log.read_text()
            if "VUID-" in text or "Validation Error" in text:
                raise RuntimeError("Vulkan validation failed; see " + str(log))
            normal_after = digest(normal) if normal.is_file() else None
            if normal_after != normal_before:
                raise RuntimeError("The normal profile changed during the isolated smoke check")
            # Read the native PPM's header rather than infer its presentation size.
            with (player_dir(player) / "signed-out.ppm").open("rb") as image:
                if image.readline().strip() != b"P6":
                    raise RuntimeError("Native smoke did not produce a PPM screenshot")
                size = [int(n) for n in image.readline().split()]
            if size != [1280, 720]:
                raise RuntimeError("The isolated client did not retain its prepared window size")
            report = {"passed": True, "actual_size": size, "frames": 3,
                      "vulkan_validation": validation,
                      "normal_profile_unchanged": True, "manually_authenticated": False,
                      "invitation_sent": False, "microphone_opened": False}
            (player_dir(player) / "smoke.json").write_text(json.dumps(report, indent=2) + "\n")
            print(f"PASS Player {player}: isolated sign-in screen, 1280x720, original profile unchanged.")
    finally:
        os.close(fd)


def status():
    for player in [1, 2]:
        try:
            fd = locked_file(player)
        except RuntimeError:
            print(f"Player {player}: running")
        else:
            os.close(fd)
            print(f"Player {player}: prepared, stopped")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--print-preference-path", action="store_true", help=argparse.SUPPRESS)
    commands = parser.add_subparsers(dest="command")
    commands.add_parser("prepare").add_argument("--launcher", type=Path,
        help="Select a specific built title launcher instead of the installed default")
    commands.add_parser("status")
    for command in ["launch", "smoke"]:
        subparser = commands.add_parser(command)
        subparser.add_argument("player", type=int, choices=[1, 2])
        if command == "smoke":
            subparser.add_argument("--validation", action="store_true",
                                  help="Require the optional Vulkan validation layer")
    args = parser.parse_args()
    if args.print_preference_path:
        print(preference_path())
    elif args.command == "prepare":
        prepare(args.launcher.resolve() if args.launcher else None)
    elif args.command in ("launch", "smoke"):
        launch(args.player, args.command == "smoke", getattr(args, "validation", False))
    elif args.command == "status":
        status()
    else:
        parser.print_help()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, KeyError, ValueError, subprocess.CalledProcessError) as error:
        print("Two-account check: " + str(error), file=sys.stderr)
        raise SystemExit(1)
