#!/usr/bin/env python3
"""Capture two actual native completed-match PCR handoffs through run_frontend.

The test accelerates native CPU match ticks to actual authored expiry, preserves
real-clock source countdowns, and renders bounded native world start/darkening
frames and each real report through the application resource/GPU boundary.
Audio is muted. This is not an original
Vista/Xbox framebuffer comparison, complete audiovisual parity or hardware play.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import time
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE_PINS = {
    "Halo 2 Vista installer/halo2.exe": "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109",
    "Halo 2 Vista installer/xlive.dll": "3829cf82e87e51f5eb4a40d241a23e22c7729612385f56fc9812549dee471d72",
    "Halo 2 Vista installer/maps/mainmenu.map": "eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44",
}


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as file:
        for block in iter(lambda: file.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path, help="fresh native_vista_flow_tests executable")
    parser.add_argument("output", type=Path, help="new capture directory")
    parser.add_argument("--players", type=int, choices=(1,2), default=1)
    parser.add_argument("--maps", type=Path, default=ROOT / "assets/disc/maps")
    parser.add_argument("--vista-map", type=Path, default=ROOT / "Halo 2 Vista installer/maps/mainmenu.map")
    parser.add_argument("--gameplay-xbe", type=Path, required=True,
                        help="actual launcher-selected gameplay update; the base-disc XBE lacks required scoreboard reference data")
    parser.add_argument("--shader-dir", type=Path, help="compiled renderer shaders; defaults to binary directory/shaders")
    parser.add_argument("--source-root", type=Path, default=ROOT,
                        help="exact source tree compiled into the executable; supports frozen delivery builds")
    parser.add_argument("--timeout", type=float, default=180)
    parser.add_argument("--display", type=int, default=199, help="dedicated Xvfb display number")
    args = parser.parse_args()
    binary, output = args.binary.resolve(), args.output.resolve()
    maps, vista_map, gameplay = args.maps.resolve(), args.vista_map.resolve(), args.gameplay_xbe.resolve()
    shaders = (args.shader_dir or binary.parent / "shaders").resolve()
    source_root = args.source_root.resolve()
    xvfb = shutil.which("xvfb-run")
    if not xvfb or not binary.is_file() or not os.access(binary, os.X_OK):
        parser.error("xvfb-run and an executable native_vista_flow_tests binary are required")
    if not (maps / "mainmenu.map").is_file() or not vista_map.is_file() or not gameplay.is_file():
        parser.error("the actual native maps, Vista mainmenu and gameplay XBE are required")
    if not shaders.is_dir() or not any(shaders.glob("*.spv")):
        parser.error("compiled renderer shaders are required; supply --shader-dir when linking elsewhere")
    if args.timeout <= 0:
        parser.error("timeout must be positive")
    output.mkdir(parents=True, exist_ok=False)

    # Pin the complete declared native map bank, including shared resources:
    # the authored frontend, rather than this wrapper, chooses the match map.
    files = {binary, vista_map, gameplay, Path(__file__).resolve()}
    files.update(maps.glob("*.map"))
    # The announcer resolves the actual selected PC scenario by scanning real
    # cache headers, then consumes the PC shared sound namespace.
    files.update(vista_map.parent.glob("*.map"))
    files.update(shaders.glob("*.spv"))
    for directory in (maps / "fonts", vista_map.parent / "fonts"):
        files.update(path for path in directory.rglob("*") if path.is_file())
    files.update(ROOT / name for name in SOURCE_PINS)
    files.update(source_root / name for name in (
        "CMakeLists.txt", "src/app/main.cpp",
        "tests/native_vista_flow_tests.cpp", "src/ui/vista/native_vista_flow.cpp", "src/ui/vista/native_vista_pcr.cpp",
        "src/ui/vista/native_vista_session.cpp", "src/gameplay/multiplayer/multiplayer_runtime.cpp", "src/gameplay/multiplayer/multiplayer.cpp", "src/gameplay/multiplayer/multiplayer_world.cpp", "src/ui/frontend.cpp",
        "src/ui/frontend_state.cpp", "src/ui/frontend_session.cpp", "src/ui/frontend_painter.cpp", "src/ui/frontend_vista.cpp",
        "src/ui/vista/vista_pcr_owner_state.cpp", "src/ui/vista/vista_pcr_results.cpp", "src/ui/vista/vista_pcr_screen.cpp",
        "src/ui/vista/vista_pcr_appearance.cpp", "src/ui/vista/vista_gameplay_fade.cpp",
        "src/ui/vista/native_vista_announcer.cpp", "src/audio/vista_announcer_assets.cpp",
        "src/audio/vista_announcer_events.cpp", "src/audio/vista_announcer_queue.cpp", "src/ui/vista/vista_player_birth.cpp",
        "src/ui/vista/vista_slayer_publication.cpp",
        "src/ui/vista/vista_profile_catalog.cpp", "include/halo2/vista_profile_catalog.hpp", "src/ui/vista/vista_lobby_leave_task.cpp",
        "src/ui/vista/vista_session_disposal.cpp", "src/ui/frontend_pc_settings.cpp",
        "src/ui/vista/vista_menu_return_observer.cpp",
        "src/ui/vista/vista_session_selection.cpp", "include/halo2/vista_session_selection.hpp",
        "tests/fixtures/vista_session_frontend_protocol_original.json",
        "src/app/desktop_loading.cpp", "src/app/desktop_session.cpp", "src/rendering/renderer/renderer.cpp",
        "src/platform/media/native_loading_presentation.cpp", "src/ui/vista/vista_loading_geometry.cpp",
        "src/ui/vista/vista_loading.cpp", "src/ui/vista/vista_loading_thread.cpp",
        "src/ui/vista/native_vista_loading_audio.cpp", "include/halo2/native_vista_loading_audio.hpp",
        "src/ui/vista/vista_loading_audio.cpp", "include/halo2/vista_loading_audio.hpp",
        "include/halo2/frontend_audio.hpp", "include/halo2/frontend_timing.hpp",
        "include/halo2/desktop_session.hpp", "include/halo2/renderer.hpp",
        "include/halo2/native_vista_flow.hpp", "include/halo2/native_vista_session.hpp",
        "include/halo2/vista_pcr_owner_state.hpp", "include/halo2/frontend.hpp",
        "include/halo2/frontend_session.hpp", "include/halo2/multiplayer.hpp", "include/halo2/vista_gameplay_fade.hpp",
        "include/halo2/native_vista_announcer.hpp", "include/halo2/vista_announcer_events.hpp",
        "include/halo2/vista_player_birth.hpp", "include/halo2/vista_slayer_publication.hpp",
        "include/halo2/multiplayer_world.hpp",
        "include/halo2/vista_announcer_queue.hpp", "include/halo2/vista_session_disposal.hpp", "include/halo2/vista_lobby_leave_task.hpp",
        "include/halo2/vista_menu_return_observer.hpp"))
    if (source_root / "source-pins.json").is_file():
        files.add(source_root / "source-pins.json")
    files.update(path for path in (ROOT / "assets/menu-ui/vista/loading").glob("*") if path.is_file())
    if any(not path.is_file() for path in files):
        raise RuntimeError("A declared capture input is missing")
    files = sorted(files)

    def pins():
        return {str(path): digest(path) if path.is_file() else "missing" for path in files}

    before = pins()
    for name, expected in SOURCE_PINS.items():
        if before[str(ROOT / name)] != expected:
            raise RuntimeError("Supplied original authority changed: " + name)
    (output / "input-pins.json").write_text(json.dumps(before, indent=2) + "\n")
    environment = dict(os.environ, SDL_VIDEODRIVER="x11")
    validation = ROOT / ".tools/validation/usr"
    if validation.is_dir():
        environment["VK_LAYER_PATH"] = str(validation / "share/vulkan/explicit_layer.d")
        environment["LD_LIBRARY_PATH"] = os.pathsep.join(filter(None, (
            str(validation / "lib"), environment.get("LD_LIBRARY_PATH"))))
    command = [xvfb, "-n", str(args.display), "-s", "-screen 0 1280x720x24", str(binary), str(maps),
               str(vista_map), str(gameplay), str(output)]
    if args.players==2: command.append("--two-players")
    started = time.monotonic()
    timed_out = False
    with (output / "native.log").open("w") as log:
        process = subprocess.Popen(command, cwd=ROOT, env=environment, stdout=log,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        try:
            returncode = process.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired:
            timed_out, returncode = True, None
            # xvfb-run owns both the native child and X server; close the whole
            # capture process group before inspecting output or stable inputs.
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                process.wait()
    elapsed = time.monotonic() - started
    log = (output / "native.log").read_text(errors="replace")
    errors = re.findall(r"VUID-[^\s]*|Validation Error[^\n]*|validation error[^\n]*|Fatal[^\n]*", log)
    round_passes = [bool(re.search(rf"^PASS {args.players}-player real-clock countdown/native authored expiry/report/Done/fresh lobby round " +
                                  str(round_index) + r"$", log, re.MULTILINE)) for round_index in (1, 2)]
    done_passes = [f"PASS actual SDL Done/rendered lobby/120 later frames round {i}" in log for i in (1, 2)]
    start_passes = [f"PASS actual SDL Start from returned multiplayer lobby round {i}" in log for i in (1, 2)]
    complete = "PASS two actual native rounds cover report/title/new-session restart; no rendered Vista parity claim" in log
    title_reentry = "PASS post-carnage/title/profile/Split Screen reentry; next round must fully load" in log
    navigation_return = "PASS post-carnage Leave/native menu/Settings/title/Split Screen without retired screens" in log
    captures, image_errors = [], []
    for round_index in (1, 2):
        scopes = [("native-world-start", "world start through tick90 with independent virtual SDL input" if args.players==2 else "world start after eight rendered frames")] if round_index == 1 else [
            ("native-world-darkening", "postgame hold frame after forty-five actual runtime presentation ticks, including fifteen darkening simulation ticks; intervening actual CPU ticks accelerated within the same retained GPU world owner")]
        scopes.append(("native-completed-round", "completed match report after actual intro elapsed"))
        scopes.extend((("native-lobby-after-done", "ready multiplayer lobby after actual SDL Done input and120 later rendered frames"),
                       ("native-next-countdown", "next match countdown started by actual SDL Return in the returned lobby")))
        scopes.extend((("native-loading-world", "source loading worker during actual prepared-world resource construction"),
                       ("native-loading-menu", "source loading worker at actual completed-world resource teardown")))
        for basename, scope in scopes:
            ppm = output / f"{basename}-{round_index}.ppm"
            png = ppm.with_suffix(".png")
            try:
                with Image.open(ppm) as image:
                    if image.format != "PPM" or image.size != (1280, 720):
                        raise RuntimeError("Expected actual 1280x720 PPM capture")
                    image.save(png, format="PNG")
                with Image.open(png) as image:
                    if image.format != "PNG" or image.size != (1280, 720):
                        raise RuntimeError("Converted capture is not a 1280x720 PNG")
                captures.append(dict(round=round_index, scope=scope, ppm=str(ppm), png=str(png),
                                     ppm_sha256=digest(ppm), png_sha256=digest(png)))
            except (OSError, RuntimeError) as error:
                image_errors.append(f"round {round_index}, {basename}: {error}")
    for basename, scope in (("native-initial-lobby", "actual retained lobby renderer before first resource load"),
                            ("native-lobby-after-title-1", "actual Split Screen lobby after report, native main menu, title and selected-profile reentry"),
                            ("native-lobby-after-title-2", "actual Split Screen lobby after a second disposal, Settings, title and selected-profile reentry")):
        ppm = output / (basename + ".ppm")
        png = ppm.with_suffix(".png")
        try:
            with Image.open(ppm) as image:
                if image.format != "PPM" or image.size != (1280, 720):
                    raise RuntimeError("Expected actual 1280x720 PPM frontend capture")
                image.save(png, format="PNG")
            captures.append(dict(scope=scope, ppm=str(ppm), png=str(png),
                                 ppm_sha256=digest(ppm), png_sha256=digest(png)))
        except (OSError, RuntimeError) as error:
            image_errors.append(f"{basename}: {error}")
    after = pins()
    changed = [path for path in before if before[path] != after[path]]
    passed = returncode == 0 and not timed_out and not errors and all(round_passes) and all(done_passes) and all(start_passes) and complete and title_reentry and navigation_return and not image_errors and not changed
    report = dict(local_players=args.players, independent_virtual_controller_input=args.players==2, schema="H2_NATIVE_VISTA_COMPLETED_ROUNDTRIP_CAPTURE_V1", passed=passed,
                  argv=command, shader_directory=str(shaders), source_directory=str(source_root),
                  returncode=returncode, elapsed_seconds=elapsed, timed_out=timed_out,
                  round_passes=round_passes, two_round_pass=complete, title_reentry_pass=title_reentry,
                  sdl_done_return_passes=done_passes, sdl_next_start_passes=start_passes,
                  navigation_return_pass=navigation_return, validation_errors=errors,
                  native_log_tail=log.splitlines()[-40:],
                  captures=captures, image_errors=image_errors, pins=before, changed_inputs=changed,
                  inputs_unchanged=not changed, match_execution="actual native CPU ticks accelerated through authored normal expiry; round1 continues headless after its bounded start capture, round2 accelerates intervening ticks within its single retained GPU world owner",
                  countdown_clock="actual native network clock with real pacing",
                  report_rendering="actual run_frontend resource and Vulkan report rendering",
                  world_rendering="actual run_multiplayer resource and Vulkan viewport: round1 eight start frames for one player or tick90 with independent virtual SDL controls for two; round2 ending frame after forty-five actual runtime presentation ticks, including fifteen darkening simulation ticks, following accelerated CPU expiry within the same retained GPU world owner",
                  intervening_gpu_frames_and_audio_rendered=False,
                  full_gpu_playthrough=False,
                  audio_muted=True, original_framebuffer_comparison=False, full_av_parity=False,
                  hardware_play_verified=False, pc_spas_emblem_rendering="unresolved source boundary")
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    if not passed:
        raise SystemExit("FAIL: see " + str(output / "report.json") + " and native.log")
    print("PASS: two real-clock native completed-match round trips, bounded native world frames and actual run_frontend report captures; stable inputs, zero validation errors")


if __name__ == "__main__":
    main()
