#!/usr/bin/env python3
"""Replay retained ordinary SMG/Warthog inputs through the actual native CLI.

Run through tools/run_test.py. The paired legacy CLI projection deliberately
omits the adaptive input's accept==fire alias; this is not a claim of byte or
complete Controls identity. Native state, sound ownership and rendering remain
source owned. Only bounded JSON and a log tail leave the private directory.
"""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile
import time

from weapon_test_cache import require_mcc_map


SCHEMA = "HALO2_NATIVE_SMG_WARTHOG_AUDIO_REPLAY_V1"
EXACT_ERROR = "TU1.5 live world owner missing at 18fa45 -> 17e160 return 18fa4a"
SHADOW_FIELDS = ("world_native_shadow_generate_prepared_draws",
                 "world_native_shadow_apply_prepared_draws")
PROJECTION = ("Paired legacy native CLI projection of retained delivered SMG inputs; "
              "accept==fire is deliberately omitted. No raw-byte or complete Controls "
              "identity, audible speaker output, or framebuffer "
              "equivalence is inferred by this replay.")


def require(value, message):
    if not value:
        raise RuntimeError(message)


def sha256(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def pin(path):
    return dict(path=str(path), bytes=path.stat().st_size, sha256=sha256(path))


def integer(value, minimum=0, maximum=None):
    return type(value) is int and value >= minimum and (maximum is None or value <= maximum)


def salted(value):
    return integer(value, 1, 0xfffffffe) and value >> 16 != 0


def validate_script(path, capture_tick):
    require(path.is_file() and 0 < path.stat().st_size <= 1048576,
            "Use a nonempty retained input projection no larger than1MiB")
    rows = 0
    fires = reloads = jumps = crouches = moving = 0
    grenade_keys = []
    first_fire = None
    for line in path.read_text(encoding="ascii").splitlines():
        fields = line.split()
        require(len(fields) >= 10 and fields[0] == str(rows) and fields[1] == "0" and
                all(value in ("0", "1") for value in fields[2:5]) and fields[7] == "move",
                "Input projection needs contiguous tick keys, player0 and explicit fire/reload/grenade/look/move")
        require(rows < capture_tick, "Input projection extends through or beyond the captured tick")
        if fields[4] == "1":
            require(first_fire is None and fields[2] == "0" and not grenade_keys,
                    "SMG replay allows only one ordinary grenade pulse strictly before fire")
            grenade_keys.append(rows)
        require(all(re.fullmatch(r"[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?", fields[n])
                    for n in (5, 6, 8, 9)), "Input projection contains a non-native numeric token")
        yaw, pitch, forward, right = (float(fields[n]) for n in (5, 6, 8, 9))
        require(all(math.isfinite(value) and abs(value) <= 3.4028234663852886e38
                    for value in (yaw, pitch, forward, right)) and
                -1.5 <= pitch <= 1.5 and abs(forward) <= 1 and abs(right) <= 1,
                "Input projection contains an invalid native float or movement extent")
        extra = fields[10:]
        require(len(set(extra)) == len(extra) and all(value in ("jump", "crouch") for value in extra),
                "Input projection contains an unsupported or duplicate action token")
        fire, reload = int(fields[2]), int(fields[3])
        if fire and first_fire is None:
            first_fire = rows
        fires += fire
        reloads += reload
        jumps += "jump" in extra
        crouches += "crouch" in extra
        moving += forward != 0 or right != 0
        rows += 1
    require(rows > 1 and first_fire is not None and first_fire > 0 and fires > 60 and reloads > 0,
            "SMG projection must include a neutral baseline, sustained fire and ordinary reload input")
    return dict(rows=rows, first_key=0, last_key=rows - 1, contiguous=True, first_fire_key=first_fire,
                fire_rows=fires, reload_rows=reloads, moving_rows=moving,
                jump_rows=jumps, crouch_rows=crouches, grenade_rows=len(grenade_keys), grenade_keys=grenade_keys)


def image_receipt(path):
    require(path.is_file() and not path.is_symlink(), "Native replay lacks its rendered capture")
    with path.open("rb") as stream:
        require(stream.readline() == b"P6\n" and stream.readline() == b"640 480\n" and
                stream.readline() == b"255\n", "Native capture lost its requested640x480 P6 surface")
        header = stream.tell()
    require(path.stat().st_size == header + 640 * 480 * 3,
            "Native capture contains a partial rendered surface")
    return dict(width=640, height=480, bytes=path.stat().st_size, sha256=sha256(path))


def scan_log(log, retained):
    exact = unknown = diagnostics = False
    captures = []
    capture_count = 0
    summaries = []
    if log.exists():
        # The native capture line can be long; retain only its bounded prefix,
        # while scanning every chunk for the exact failure and option errors.
        with log.open("rb") as stream:
            carry = b""
            while chunk := stream.readline(65536):
                text = (carry + chunk).decode("utf-8", errors="replace")
                exact |= EXACT_ERROR in text
                unknown |= "unknown option:" in text.lower() or "unrecognized arguments:" in text.lower()
                diagnostics |= chunk.startswith((b"error:", b"Vulkan validation:"))
                if chunk.startswith(b"Input capture tick="):
                    capture_count += 1
                    if len(captures) < 2:
                        captures.append(chunk[:8192].decode("utf-8", errors="replace"))
                if chunk.startswith(b"Multiplayer frames=") and len(summaries) < 2:
                    summaries.append(chunk[:1024].decode("utf-8", errors="replace"))
                carry = chunk[-256:]
        with log.open("rb") as stream:
            stream.seek(max(0, log.stat().st_size - 131072))
            retained.write_bytes(stream.read(131072))
    else:
        retained.write_bytes(b"")
    return dict(exact_audio_owner_error=exact, unknown_option_error=unknown,
                native_error_diagnostic=diagnostics, capture_frame_count=capture_count,
                capture_lines=captures, completion_lines=summaries,
                log_bytes=log.stat().st_size if log.exists() else 0,
                log_sha256=sha256(log) if log.exists() else None,
                retained_log=dict(path=str(retained), bytes=retained.stat().st_size,
                                  sha256=sha256(retained),
                                  tail_truncated=log.exists() and log.stat().st_size > 131072))


def validate_native(native, capture_tick, map_path, script_grenade_rows):
    require(isinstance(native, dict) and native.get("schema") == "HALO2_NATIVE_LOOP_AUDIO_LOCATION_V1" and
            native.get("collector_schema") == "HALO2_SOURCE_LOOP_AUDIO_LOCATION_TU15_V1" and
            native.get("passed") is True and native.get("qualified") is True,
            "Native CLI did not qualify its real source-owned loop-audio execution")
    require(native.get("map") == str(map_path) and native.get("graphics_mode") == "Original" and
            native.get("runtime_cache_source") == "MCC" and
            native.get("mute") is False and native.get("run_result") == 0 and
            native.get("requested_capture_tick") == capture_tick and native.get("final_tick") == capture_tick and
            native.get("captured_tick") is True,
            "Native CLI receipt lost its exact unmuted MCC/Original tick completion")
    require(all(integer(native.get(key), 1) for key in
                ("input_samples", "submitted_frames", "shadow_generate_draws", "shadow_apply_draws")) and
            integer(native.get("native_projectile_publications"), 20),
            "Native CLI lacks actual projectiles, frames or Generate/Apply submissions")
    player, mode, audio = native.get("player", {}), native.get("mode3", {}), native.get("audio", {})
    require(isinstance(player, dict) and all(salted(player.get(key)) for key in
                ("source_player", "source_unit", "source_weapon", "weapon_definition", "expected_smg_definition")) and
            player.get("weapon_definition") == player.get("expected_smg_definition") and
            integer(player.get("rounds_consumed"), 61) and integer(player.get("shot_transitions"), 1) and
            integer(player.get("reload_commands"), 1) and integer(player.get("reload_transfers"), 1) and
            integer(player.get("reload_rounds"), 1) and
            all(player.get(key) == 0 for key in
                ("identity_changes", "post_pin_non_smg_inputs", "input_errors", "input_read_failures")),
            "Native CLI lacks actual pinned SMG shot consumption and reload transfers")
    require(isinstance(audio, dict) and integer(audio.get("packets"), 1) and
            integer(audio.get("smg_packets"), 1) and integer(audio.get("reload_packets"), 1) and
            audio.get("invalid_packets") == 0,
            "Native CLI lacks actual valid unmuted SMG and first-person reload packets")
    target, precondition = native.get("target", {}), native.get("precondition", {})
    require(isinstance(target, dict) and salted(target.get("owner")) and salted(target.get("definition")) and
            target.get("precondition_observed") is True,
            "Native CLI lacks its actual full salted precondition target and definition")
    require(isinstance(precondition, dict) and precondition.get("observed") is True and
            type(precondition.get("grenade_commands")) is int and
            precondition["grenade_commands"] == script_grenade_rows == 1,
            "Native CLI did not observe its one ordinary pre-fire grenade command")
    before, after = precondition.get("body_before"), precondition.get("body_after")
    require(type(before) in (int, float) and type(after) in (int, float) and
            math.isfinite(before) and math.isfinite(after) and 0 < after < before,
            "Native CLI did not observe a damaged, surviving Warthog precondition")
    projectiles, detonations = (precondition.get(key) for key in ("grenade_projectiles", "grenade_detonations"))
    require(isinstance(projectiles, list) and 0 < len(projectiles) <= 64 and
            isinstance(detonations, list) and 0 < len(detonations) <= 64,
            "Native CLI lacks bounded actual grenade birth/release/detonation identities")
    identities = set()
    released = False
    for projectile in projectiles:
        require(isinstance(projectile, dict) and salted(projectile.get("projectile")) and
                salted(projectile.get("definition")) and type(projectile.get("type")) is int and
                projectile["type"] == 0 and projectile.get("player") == player["source_player"] and
                projectile.get("unit") == player["source_unit"] and
                type(projectile.get("released")) is bool,
                "Native frag precondition lost its full salted authored projectile or actual source owner")
        identities.add((projectile["projectile"], projectile["definition"]))
        released |= projectile["released"]
    require(len(identities) == 1 and released,
            "Native frag precondition did not detach its one actual projectile")
    for detonation in detonations:
        require(isinstance(detonation, dict) and
                (detonation.get("projectile"), detonation.get("definition")) in identities and
                detonation.get("player") == player["source_player"] and detonation.get("unit") == player["source_unit"] and
                integer(detonation.get("serial"), 1) and integer(detonation.get("tick"), 0, capture_tick - 1),
                "Native frag precondition lacks a matching actual full-generation detonation")
    require(isinstance(mode, dict) and mode.get("observer_bound") is True and
            mode.get("baseline_recorded") is True and mode.get("baseline_frozen") is True and
            integer(mode.get("entries"), 1) and mode.get("entries") == mode.get("returns") and
            all(integer(mode.get(key), 1) for key in
                ("validated_entries", "successful_returns", "post_fire_successful_returns",
                 "target_owned_post_fire_successful_returns")) and
            integer(mode.get("new_generation_successful_returns")) and
            integer(mode.get("preexisting_target_owned_successful_returns")) and
            mode["target_owned_post_fire_successful_returns"] == mode["new_generation_successful_returns"] +
                mode["preexisting_target_owned_successful_returns"] and
            integer(mode.get("post_fire_successful_ticks"), 2) and
            all(mode.get(key) == 0 for key in
                ("invalid_calls", "read_failures", "unfinished_calls", "late_baseline_inputs")),
            "Native CLI lacks balanced, valid target-owned mode3 returns over two source ticks")
    if mode["preexisting_target_owned_successful_returns"]:
        require(mode.get("precondition_generation_seen") is True and
                integer(mode.get("precondition_generation_count"), 1, 64) and
                integer(mode.get("precondition_target_owned_successful_returns"), 1),
                "Existing mode3 generations were not actually observed before the native SMG shot sample")
    first, last, shot = (mode.get(key) for key in
                         ("first_success_source_tick", "last_success_source_tick", "first_shot_source_tick"))
    require(all(integer(value) for value in (first, last, shot)) and shot <= first < last,
            "Native CLI source-owner progress did not occur after its actual SMG shot")
    require(integer(mode.get("first_shot_tick")) and
            all(event["tick"] < mode["first_shot_tick"] for event in detonations),
            "Native grenade detonation did not precede the first actual SMG shot sample")
    before_smg, after_smg = (target.get(key) for key in
                             ("body_before_smg", "minimum_body_after_smg"))
    require(type(before_smg) in (int, float) and type(after_smg) in (int, float) and
            math.isfinite(before_smg) and math.isfinite(after_smg) and 0 <= after_smg < before_smg and
            target.get("smg_damage_observed") is True and
            integer(target.get("post_smg_vitality_samples"), 2) and
            integer(target.get("first_smg_command_tick"), 1) and
            integer(target.get("first_smg_damage_tick"), 1, capture_tick - 1) and
            target["first_smg_command_tick"] <= target["first_smg_damage_tick"] and
            mode["first_shot_tick"] <= target["first_smg_damage_tick"],
            "Native CLI lacks actual full-generation Warthog vitality decrease after observed SMG fire")
    samples = mode.get("samples")
    require(isinstance(samples, list) and 0 < len(samples) <= 64,
            "Native CLI lacks bounded actual mode3 caller samples")
    for sample in samples:
        require(isinstance(sample, dict) and sample.get("caller_return") == 0x18fa4a and
                all(salted(sample.get(key)) for key in
                    ("effect", "loop", "effect_definition", "loop_definition")) and
                sample.get("effect_object_owner") == target["owner"] and
                sample.get("effect_object_definition") == target["definition"] and
                integer(sample.get("tick"), first, last) and integer(sample.get("eax"), 0, 0xffffffff) and
                sample["eax"] & 255,
                "Native mode3 sample lost its actual caller, target/full salted identities or true AL return")
        require(type(sample.get("new_generation")) is bool, "Native mode3 sample lacks its generation classification")
        if sample["new_generation"]:
            require(mode["new_generation_successful_returns"] > 0,
                    "Native new-generation sample disagrees with its actual source counter")
        else:
            require(mode["preexisting_target_owned_successful_returns"] > 0 and
                    sample.get("precondition_generation_seen") is True and
                    integer(sample.get("precondition_first_seen_source_tick"), 0, shot - 1),
                    "Existing target-owned pair was not successfully observed on an earlier source tick before the SMG shot sample")
        bits = sample.get("output_bits")
        require(isinstance(bits, list) and len(bits) == 7 and
                all(integer(value, 0, 0xffffffff) and value & 0x7f800000 != 0x7f800000 for value in bits),
                "Native mode3 output sample contains missing or nonfinite source bits")
    require(len({sample["tick"] for sample in samples}) >= 2,
            "Native mode3 retained samples do not independently show two actual source ticks")


def exercise(args, scratch, report):
    native_report, log = scratch / "native-owner.json", scratch / "native.log"
    timings, image = scratch / "renderer.tsv", scratch / "completed.ppm"
    command = ["xvfb-run", "-a", "-s", "-screen 0 800x600x24", str(args.runtime),
               "--multiplayer", "--graphics-mode", "Original", "--map", str(args.map),
               "--shared-maps", str(args.shared_maps), "--local-players", "1",
               "--mp-input-script", str(args.input_script), "--mp-capture-tick", str(args.capture_tick),
               "--mp-reference-tick-cadence", "--viewport", "640", "480", "--screenshot", str(image)]
    if args.expect == "complete":
        command += ["--mp-loop-audio-report", str(native_report)]
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith(("HALO2_", "H2_"))}
    environment.update(HALO2_TEST_SCRATCH=str(scratch), SDL_VIDEODRIVER="x11",
                       SDL_AUDIODRIVER="dummy", HALO2_RENDER_TIMINGS=str(timings))
    for key, name in (("XDG_CONFIG_HOME", "config"), ("XDG_DATA_HOME", "data"),
                      ("XDG_CACHE_HOME", "cache"), ("TMPDIR", "tmp"), ("TMP", "tmp"), ("TEMP", "tmp"),
                      ("HALO2_PIPELINE_CACHE_DIRECTORY", "pipelines"),
                      ("MESA_SHADER_CACHE_DIR", "mesa"), ("__GL_SHADER_DISK_CACHE_PATH", "nvidia")):
        environment[key] = str(scratch / name)
        (scratch / name).mkdir(exist_ok=True)
    report["command"] = command
    started = time.monotonic()
    process = None
    try:
        with log.open("wb") as stream:
            process = subprocess.Popen(command, cwd=args.root, env=environment, stdout=stream,
                                       stderr=subprocess.STDOUT, start_new_session=True)
            try:
                report["exit_code"] = process.wait(timeout=args.timeout)
            except subprocess.TimeoutExpired:
                report["exit_code"] = "timeout"
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
    finally:
        if process is not None and process.poll() is None:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
        report["elapsed_seconds"] = round(time.monotonic() - started, 3)
        report.update(scan_log(log, args.report_output.with_suffix(".log")))
        if native_report.exists():
            require(native_report.is_file() and not native_report.is_symlink() and
                    native_report.stat().st_size <= 262144, "Native owner receipt exceeds its bound")
            report["native"] = json.loads(native_report.read_text())
    require(not report["unknown_option_error"], "Native runtime rejected a requested CLI option")
    if args.expect == "missing-owner":
        require(type(report.get("exit_code")) is int and report["exit_code"] != 0 and
                report["exact_audio_owner_error"], "Old runtime did not reproduce the exact missing17E160 owner")
        report["passed"] = True
        return
    require(not report["exact_audio_owner_error"], EXACT_ERROR)
    require(report.get("exit_code") == 0 and not report["native_error_diagnostic"],
            "Native replay did not complete cleanly")
    validate_native(report.get("native"), args.capture_tick, args.map, report["input_schedule"]["grenade_rows"])
    require(report["capture_frame_count"] == 1 and
            re.search(rf"^Input capture tick={args.capture_tick}\b", report["capture_lines"][0]) and
            " capture_tick_cadence=1 " in report["capture_lines"][0],
            "Native replay did not capture its requested tick exactly once")
    require(len(report["completion_lines"]) == 1, "Native replay lacks its final submitted-frame summary")
    match = re.search(r"Multiplayer frames=(\d+) ticks=(\d+) players=(\d+) connected=(\d+) peak_players=(\d+)",
                      report["completion_lines"][0])
    require(match is not None, "Native replay summary has an unsupported shape")
    frames, ticks, players, connected, peak = map(int, match.groups())
    report.update(frames=frames, ticks=ticks, players=players)
    require(frames > 0 and ticks == args.capture_tick and players == connected == peak == 1,
            "Native replay lost its exact tick or ordinary single-player owner")
    require(timings.is_file() and timings.stat().st_size <= 16777216,
            "Native replay lacks bounded renderer timings")
    draws = dict.fromkeys(SHADOW_FIELDS, 0)
    with timings.open() as stream:
        rows = csv.DictReader(stream, delimiter="\t")
        require(rows.fieldnames and all(field in rows.fieldnames for field in SHADOW_FIELDS),
                "Native replay lost its actual Generate/Apply renderer counters")
        row_count = 0
        for row in rows:
            row_count += 1
            for field in SHADOW_FIELDS:
                value = int(row[field])
                require(value >= 0, "Native renderer emitted a negative shadow counter")
                draws[field] += value
    require(row_count and all(draws.values()), "Native replay submitted no ordinary Generate/Apply draws")
    report["shadow_draws"] = draws
    report["capture"] = image_receipt(image)
    report["passed"] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("runtime", "map", "shared-maps", "input-script", "report-output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--capture-tick", type=int, default=1200)
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--expect", choices=("complete", "missing-owner"), default="complete")
    args = parser.parse_args()
    if "HALO2_TEST_SCRATCH" not in os.environ:
        parser.error("Run through tools/run_test.py for owned temporary storage")
    if not 600 <= args.capture_tick <= 1800 or not 1 <= args.timeout <= 900:
        parser.error("Use capture tick600..1800 and finite timeout1..900 seconds")
    args.runtime, args.root = args.runtime.resolve(strict=True), args.root.resolve(strict=True)
    args.map, args.shared_maps = require_mcc_map(args.map), args.shared_maps.resolve(strict=True)
    args.input_script = args.input_script.resolve(strict=True)
    if args.map.parent != args.shared_maps:
        parser.error("The MCC map must belong to its explicitly selected shared-map directory")
    caches = [require_mcc_map(args.shared_maps / name) for name in
              ("mainmenu.map", "shared.map", "single_player_shared.map")]
    args.report_output = args.report_output.absolute()
    if args.report_output.is_symlink() or args.report_output.suffix != ".json":
        parser.error("Use a regular JSON report output path")
    args.report_output = args.report_output.resolve()
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    metadata_path = args.input_script.with_suffix(".json")
    paths = list(dict.fromkeys([args.runtime, args.input_script, args.map] + caches +
                              ([metadata_path] if metadata_path.exists() else [])))
    if args.report_output in paths or args.report_output.with_suffix(".log") in paths:
        parser.error("Report/log output cannot replace a runtime or reference input")
    before = [pin(path) for path in paths]
    report = dict(schema=SCHEMA, passed=False, expect=args.expect, scope=__doc__,
                  runtime=str(args.runtime), runtime_sha256=before[0]["sha256"],
                  map=str(args.map), map_sha256=sha256(args.map), shared_maps=str(args.shared_maps),
                  input_script=str(args.input_script), input_script_sha256=before[1]["sha256"],
                  capture_tick=args.capture_tick, viewport=[640, 480], graphics_mode="Original",
                  runtime_cache_source="MCC", mute=False, audio_driver="dummy",
                  input_pins_before=before, test_sha256=sha256(__file__), retained_large_artifacts=[],
                  input_projection=dict(legacy_cli_projection_omits_accept=True, scope=PROJECTION))
    try:
        require(args.runtime.is_file() and os.access(args.runtime, os.X_OK), "Native runtime is not executable")
        if metadata_path in paths:
            require(metadata_path.is_file() and not metadata_path.is_symlink() and
                    metadata_path.stat().st_size <= 65536,
                    "Reference input metadata must be a bounded regular JSON file")
            metadata = json.loads(metadata_path.read_text())
            require(isinstance(metadata, dict) and metadata.get("capture_tick") == args.capture_tick and
                    metadata.get("map_sha256") == report["map_sha256"] and
                    metadata.get("input_script_sha256") == report["input_script_sha256"],
                    "Reference metadata does not authenticate the requested tape, MCC map and capture tick")
            report.update(reference_metadata=metadata, reference_metadata_path=str(metadata_path),
                          reference_metadata_sha256=sha256(metadata_path))
        report["input_schedule"] = validate_script(args.input_script, args.capture_tick)
        with tempfile.TemporaryDirectory(prefix="smg-warthog-cli-", dir=os.environ["HALO2_TEST_SCRATCH"]) as directory:
            report["scratch"] = directory
            exercise(args, Path(directory), report)
    except Exception as error:
        report.update(passed=False, error=str(error))
    finally:
        report["scratch_removed"] = "scratch" in report and not Path(report["scratch"]).exists()
        after = []
        for path in paths:
            try:
                after.append(pin(path))
            except OSError as error:
                after.append(dict(path=str(path), error=str(error)))
        report["input_pins_after"] = after
        report["inputs_unchanged"] = before == after
        report["passed"] &= report["scratch_removed"] and report["inputs_unchanged"]
        text = json.dumps(report, indent=2) + "\n"
        if len(text.encode()) > 524288:
            report.update(passed=False, error="Replay receipt exceeds its512KiB bound")
            report.pop("native", None)
            text = json.dumps(report, indent=2) + "\n"
        args.report_output.write_text(text)
    print(json.dumps({key: report.get(key) for key in
                     ("passed", "expect", "exit_code", "elapsed_seconds", "exact_audio_owner_error", "error")}))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
