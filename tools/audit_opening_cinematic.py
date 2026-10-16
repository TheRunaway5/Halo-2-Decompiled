#!/usr/bin/env python3
"""Inventory the authored Xbox opening and reproduce campaign script failures.

This reads the actual 00a_introduction cache, including linked HS expressions
and scene helpers. Optional native probes run the production importer/VM under
the existing aggregate memory guard. They do not render, play audio, or certify
Xbox TU1.5 visual parity. Independently invoked scenes are diagnostic probes,
not a substitute for the normal startup sequence.
"""
import argparse
from collections import Counter
import hashlib
import json
import mmap
from pathlib import Path
import struct

from tag_match_memory_budget import run_in_memory_budget

ROOT = Path(__file__).resolve().parents[1]
NONE = 0xffffffff
SCENES = ("x01_scene_01a", "x01_scene_01b", *(
    f"x01_scene_{n:02d}" for n in range(2, 11)),
    "c01_intro_scene_01", "c01_intro_scene_02")


def digest(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def inventory(path):
    with path.open("rb") as source, mmap.mmap(
            source.fileno(), 0, access=mmap.ACCESS_READ) as data:
        def unpack(form, at):
            size = struct.calcsize(form)
            if at < 0 or at + size > len(data):
                raise ValueError(f"Cache read outside file at {at:#x}")
            return struct.unpack_from(form, data, at)

        def u(at):
            return unpack("<I", at)[0]

        def string(at, limit=None):
            limit = len(data) if limit is None else limit
            if not 0 <= at < limit <= len(data):
                raise ValueError("Invalid cache string address")
            end = data.find(b"\0", at, limit)
            if end < 0:
                raise ValueError("Unterminated cache string")
            return data[at:end].decode("latin1")

        if unpack("<3I", 0) != (0x68656164, 8, len(data)):
            raise ValueError("Expected an unmodified original Xbox cache")
        build = string(288)
        if build != "02.09.27.09809" or string(408) != "00a_introduction":
            raise ValueError("Expected the retail Xbox 00a_introduction cache")
        index = u(16)
        table = u(index + 8) - (u(index) - (index + 32))
        magic = u(table + 8) - (index + u(20))
        tags = {u(table + i * 16 + 4): table + i * 16
                for i in range(u(index + 24))}
        scenario = u(tags[u(index + 12)] + 8) - magic
        node_count, node_pointer = unpack("<II", scenario + 0x238)
        nodes = node_pointer - magic
        strings_size, strings_pointer = unpack("<II", scenario + 0x1b0)
        strings = strings_pointer - magic
        script_count, scripts_pointer = unpack("<II", scenario + 0x1b8)
        scripts = scripts_pointer - magic

        def node(datum):
            i = datum & 65535
            if i >= node_count or datum == NONE:
                raise ValueError(f"Invalid HS node {datum:08x}")
            values = unpack("<4H3I", nodes + i * 20)
            if values[0] != datum >> 16:
                raise ValueError(f"Stale HS node {datum:08x}")
            return values

        def token(offset):
            return string(strings + offset, strings + strings_size)

        def expression(datum, ancestors=()):
            if datum in ancestors or len(ancestors) >= 128:
                raise ValueError("Cyclic or excessively deep HS expression")
            salt, opcode, typ, flags, following, offset, value = node(datum)
            result = dict(datum=f"{datum:08x}", opcode=opcode,
                          type=typ, flags=flags)
            if flags & 1:
                result["bits"] = f"{value:08x}"
                if typ == 9:
                    result["string"] = token(offset)
                elif typ == 6 and not flags & 4:
                    result["real"] = struct.unpack("<f", struct.pack("<I", value))[0]
                return result
            function = node(value)
            result["call"] = token(function[5])
            result["script_call"] = bool(flags & 2)
            result["arguments"] = []
            argument = function[4]
            seen = set()
            while argument != NONE:
                if argument in seen:
                    raise ValueError("Cyclic HS argument list")
                seen.add(argument)
                result["arguments"].append(expression(argument, (*ancestors, datum)))
                argument = node(argument)[4]
            return result

        records = []
        for i in range(script_count):
            at = scripts + i * 40
            typ, return_type, root = unpack("<HHI", at + 32)
            records.append(dict(index=i, name=string(at, at + 32), type=typ,
                                return_type=return_type, root=expression(root)))
        by_name = {row["name"]: row for row in records}
        if len(by_name) != script_count or any(name not in by_name for name in SCENES):
            raise ValueError("Opening scene table is incomplete")

        def calls(expr):
            if "call" in expr:
                yield expr
                for arg in expr["arguments"]:
                    yield from calls(arg)

        counts = Counter()
        for record in records:
            counts.update(call["call"] for call in calls(record["root"])
                          if not call["script_call"])
        intro_calls = [call["call"] for call in calls(by_name["intro"]["root"])
                       if call["script_call"] and call["call"] in SCENES]
        if intro_calls != list(SCENES):
            raise ValueError("Authored intro does not call the expected scene sequence")
        scene_calls = []
        for name in SCENES:
            direct = list(calls(by_name[name]["root"]))
            cameras = [call for call in direct
                       if call["call"] == "camera_set_animation_relative"]
            if len(cameras) != 1:
                raise ValueError(f"Expected one authored camera command in {name}")
            camera = cameras[0]
            graph = int(camera["arguments"][0]["bits"], 16)
            sid = int(camera["arguments"][1]["bits"], 16)
            graph_root = u(tags[graph] + 8) - magic
            clip_count, clips_pointer = unpack("<II", graph_root + 0x2c)
            clips = clips_pointer - magic
            matches = [i for i in range(clip_count) if u(clips + i * 0x6c) == sid]
            if len(matches) != 1:
                raise ValueError(f"Camera clip has no unique local graph owner in {name}")
            frame_count = unpack("<H", clips + matches[0] * 0x6c + 0x14)[0]
            scene_calls.append(dict(name=name, camera_graph=f"{graph:08x}",
                                    camera_sid=f"{sid:08x}", camera_frames=frame_count,
                                    camera_duration_ticks=frame_count - 1,
                                    direct_calls=Counter(c["call"] for c in direct)))
        return dict(map=str(path), map_sha256=digest(path), cache_build=build,
                    evidence_scope="Original disc metadata; not TU1.5 rendered output",
                    node_count=node_count, script_count=script_count,
                    scene_order=list(SCENES), scenes=scene_calls,
                    builtin_calls=dict(sorted(counts.items())), scripts=records)


def probe(native, cache, shared, xbe, ticks, output, scene, timeout):
    command = [str(native), "--map", str(cache), "--shared-maps", str(shared),
               "--gameplay-xbe", str(xbe), "--simulate", str(ticks)]
    if scene is None:
        command.append("--campaign")
    else:
        command.extend(("--script", scene))
    status = run_in_memory_budget(command, output, timeout)
    budget = json.loads((output / "memory-budget.json").read_text())
    log = output / "memory-budget-runtime.log"
    lines = log.read_text(encoding="utf-8", errors="replace").splitlines() if log.exists() else []
    faults = [line.removeprefix("Script fault: ")
              for line in lines if line.startswith("Script fault: ")]
    frontiers = [line for line in lines if line.startswith("tick=")]
    runtime_errors = [line.removeprefix("error: ") for line in lines if line.startswith("error: ")]
    # A native exception after scene setup is a playback failure, distinct
    # from an unavailable guard. Keep probing other independent helpers.
    native_failure_recorded = bool(runtime_errors) and any(
        line.startswith("collision_triangles=") for line in lines) and (
        budget.get("kernel", {}).get("Result") == "exit-code")
    return dict(scene=scene, normal_campaign_startup=scene is None, command=command,
                exit_status=status, script_faults=faults, status=frontiers[-1:] or [],
                runtime_errors=runtime_errors, native_failure_recorded=native_failure_recorded,
                guard_errors=budget.get("errors", []),
                simulation_status_recorded=bool(frontiers),
                startup_passed=status == 0 and not faults and bool(frontiers),
                log=str(log))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--map", type=Path, default=ROOT / "assets/disc/maps/00a_introduction.map")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--native", type=Path)
    parser.add_argument("--xbe", type=Path)
    parser.add_argument("--shared", type=Path)
    parser.add_argument("--ticks", type=int, default=120)
    parser.add_argument("--timeout", type=int, default=120)
    parser.add_argument("--probe-scenes", action="store_true")
    args = parser.parse_args()
    if not 1 <= args.ticks <= 100000 or not 10 <= args.timeout <= 600:
        parser.error("Use 1..100000 ticks and a 10..600 second timeout")
    if bool(args.native) != bool(args.xbe) or (args.probe_scenes and not args.native):
        parser.error("Native probes require --native and --xbe")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    cache = args.map.resolve()
    authored = inventory(cache)
    (output / "authored.json").write_text(json.dumps(authored, indent=2, allow_nan=False) + "\n")
    report = dict(map_sha256=authored["map_sha256"], scenes=authored["scenes"],
                  script_count=authored["script_count"], probes=[],
                  metadata_verified=True, full_sequence_completed=False,
                  rendering_verified=False, audio_playback_verified=False,
                  xbox_tu15_visual_parity_verified=False)
    if args.native:
        native, xbe = args.native.resolve(), args.xbe.resolve()
        report.update(native=str(native), native_sha256=digest(native),
                      xbe=str(xbe), xbe_sha256=digest(xbe))
        shared = (args.shared or cache.parent).resolve()
        for scene in (None, *(SCENES if args.probe_scenes else ())):
            result = probe(native, cache, shared, xbe, args.ticks,
                           output / (scene or "normal-startup"), scene, args.timeout)
            report["probes"].append(result)
            if not result["simulation_status_recorded"] and not result["native_failure_recorded"]:
                # A busy/unsupported containment environment says nothing
                # about any scene. Do not turn it into thirteen scene failures.
                break
        report["inputs_unchanged"] = (digest(native) == report["native_sha256"] and
                                     digest(xbe) == report["xbe_sha256"] and
                                     digest(cache) == report["map_sha256"])
    (output / "report.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    print(f"Inventoried {len(SCENES)} authored scenes and {authored['script_count']} scripts")
    for row in report["probes"]:
        print((row["scene"] or "normal-startup") + ": " +
              ("; ".join(row["script_faults"] or row["runtime_errors"] or row["guard_errors"]) or
               f"status {row['exit_status']}"))
    print("Full cinematic, rendering, audio and TU1.5 pixel parity remain unverified")
    print(output / "report.json")
    return int(any(not row["startup_passed"] for row in report["probes"]) or
               report.get("inputs_unchanged") is False)


if __name__ == "__main__":
    raise SystemExit(main())
