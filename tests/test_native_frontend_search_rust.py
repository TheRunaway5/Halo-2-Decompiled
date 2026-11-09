#!/usr/bin/env python3
"""Replay the captured native controller over declared IO against production Rust."""
import argparse
import copy
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess


def command(op, facts=None, **kw):
    return dict(op=op, facts=facts or {}, **kw)


def party(leader=True, ready="ranked", members=2):
    return {"leader": leader, "members": [dict(handle="self", self=True, ready=ready)] +
            [dict(handle="peer" + str(i), ready=ready) for i in range(1, members)]}


def cases():
    for op, identity, queue, account, mode in itertools.product(
            ("reset", "prepare", "quickmatch", "refresh", "update", "open_lobby", "open_search", "select"),
            (False, True), (False, True), (False, True), (0, 3)):
        if op == "open_lobby" and mode != 3:
            continue  # Native open14 invokes this operation only in ONLINE mode.
        f = dict(identity=identity, queue=queue, account=account, mode=mode)
        yield "entry", [command(op, f)]
    catalogs = [[], [dict(id="ranked", eligible=False)],
                [dict(id="ranked", eligible=False), dict(id="fallback", eligible=True, quick=True)],
                [dict(id="clan", eligible=True, clan=True), dict(id="party", eligible=True)],
                [dict(id="ranked", eligible=True, quick=True), dict(id="second", eligible=True, quick=True)]]
    for catalog, locals_, p, account, busy, accepted in itertools.product(
            catalogs, (1, 2, 4), (None, party()), (False, True), (False, True), (False, True)):
        f = dict(snapshot=dict(playlists=catalog), locals=locals_, party=p, account=account,
                 busy=busy, accepted=accepted, screens=[6, 186, 219])
        yield "catalog-selection", [command("select", f), command("update", f), command("update", f)]
        yield "catalog-quickmatch", [command("open_search", dict(f, screens=[6, 186, 210])),
                                     command("update", dict(f, screens=[6, 186, 210]))]
    for catalog, locals_, p, changed in itertools.product(catalogs, (1, 2), (None, party()), (False, True)):
        f = dict(snapshot=dict(playlists=catalog), locals=locals_, party=p,
                 changed=changed, screens=[6, 186, 210])
        yield "retained-invalid-selection", [command("select"), command("open_search", f), command("update", f)]
    for leader, busy, accepted, message in itertools.product((False, True), (False, True),
                                                            (False, True), ("", "worker refused")):
        f = dict(party=party(leader, ""), busy=busy, accepted=accepted, party_message=message,
                 screens=[6, 186, 210])
        ack = dict(f, busy=False, worker_party=party(leader))
        yield "consent", [command("open_search", f), command("update", f),
                          command("update", ack), command("update", ack)]
    for state, matched, gathering, countdown, supported in itertools.product(
            range(10), (False, True), (False, True), (None, 0, 1, 999, 1000, 10000, 10001, 0xffffffff),
            (False, True)):
        s = dict(status=state, matched=matched, gathering=gathering,
                 countdown=countdown, supported=supported)
        f = dict(snapshot=s, screens=[6, 186, 14, 210, 216])
        yield "phase-countdown-overlay", [command("update", f), command("update", f)]
    for screen, failure in itertools.product((14, 210), (-1, 0, 2, 3, 4, 5, 6, 14, 19, 99)):
        f = dict(snapshot=dict(failure=failure), screens=[6, 186, screen])
        yield "failure", [command("update", f), command("update", f)]
    for leader, mode, members, shown, returned in itertools.product(
            (False, True), (0, 3), (1, 2, 4), (False, True), (None, party(), party(members=1))):
        start = dict(party=party(leader, members=members), mode=mode,
                     screens=[6, 186, 14] if shown else [6], snapshot=dict(status=5))
        end = dict(start, snapshot=dict(status=3), party=returned)
        yield "party-return", [command("party", start), command("party", start),
                               command("party", end), command("party", end)]
    for profile, couch, screen in itertools.product(("self", "new", ""), (0, 1, 0xffffffffffffffff), (14, 210, 185)):
        f = dict(screens=[6, 186, screen], party=party(), profile=profile, couch=couch)
        yield "profile-generation", [command("refresh"), command("update", f), command("update", f)]
    for raw in ([255, 120], [0, 255], [], [0xe2, 0x82, 0xac]):
        f = dict(profile_bytes=raw)
        yield "raw-profile", [command("refresh", f), command("update", f)]
    for clock, context, changed in itertools.product((0, 1, 0xffffffff), (0, 42, 99), (False, True)):
        f = dict(clock=clock, changed=changed,
                 screens=[6, 186, dict(screen=219, context=context)],
                 snapshot=dict(status=5, gathering=True))
        yield "cached-view-and-clock", [command("update", f), command("update", f)]
    for map_, self_, variant, transfer, teams, locals_, slots, admitted, reject in itertools.product(
            ("lockout", "", "../lockout", "LOCKOUT", "lockout_2"),
            (False, True), (False, True), (False, True), (None, False, True),
            (1, 2), (1, 2), ("self", "other"), (False, True)):
        # Pairwise failures still retain the original first-error ordering.
        f = dict(snapshot=dict(status=8, matched=True), map=map_, self=self_, variant_available=variant,
                 transfer=transfer, teams=teams, locals=locals_, slots=slots, admitted=admitted,
                 reject_content=reject)
        yield "launch-order", [command("launch", f)]
    for sounds, top in itertools.product((0, 15, 16, 20),
                                         (216, 217, 211, 26, 31, 7, 99)):
        for state, countdown in ((5, None), (8, None), (8, 10000), (8, 0)):
            f = dict(sounds=sounds, snapshot=dict(status=state, matched=True, gathering=True, countdown=countdown),
                     screens=[6, 186, 14, 210, dict(screen=top, destination=-16)])
            yield "overlay-sound-bound", [command("update", f)]
    for screens, mode, queue, reentry, fault in itertools.product(
            ([6], [186], [6, 210, 7], [6, 186, 14, 210, 7], [6, 186, 14, 186, 210, 7]),
            (0, 3), (False, True), (False, True), ("", "cancel", "open", "enter_lobby")):
        seeded = dict(identity=False, screens=[6, 186, 210])
        facts = dict(screens=screens, mode=mode, queue=queue, reenter_open=reentry, fault=fault,
                     clock=0xffffffff)
        yield "confirmed-search-cancel", [command("open_search", seeded),
                                           command("cancel_and_return", facts)]
    faults = ("identity", "profile", "lobby_countdown", "pull_after", "account", "command", "open",
              "enter_lobby", "populate_lobby", "populate_search", "refresh", "cancel", "join", "pull",
              "local_players", "take", "team_mode", "local_slots", "roster", "validate_map", "reject_content")
    sequences = [[command("quickmatch")], [command("refresh")],
                 [command("open_search", dict(party=party(ready=""), screens=[6, 186, 210]))],
                 [command("open_search", dict(screens=[6, 186, 210]))],
                 [command("update", dict(snapshot=dict(failure=5), screens=[6, 186, 210]))],
                 [command("launch", dict(snapshot=dict(status=8, matched=True)))],
                 [command("launch", dict(snapshot=dict(status=8, matched=True), reject_content=True))]]
    for point, sequence, format_ in itertools.product(faults, sequences, (False, True)):
        sequence = copy.deepcopy(sequence)
        sequence[-1]["facts"].update(fault=point, format_fault=format_)
        yield "native-fault-" + point, sequence


def variants():
    for name, content, bundled in itertools.product(
            ("Slayer", "Oddball", "", "Slayer \ue008"),
            ([], ["Slayer"], ["Slayer \ue008", "Slayer"], ["Oddball", "Slayer"]),
            ([], ["Slayer"], ["Slayer \ue008"], ["Oddball", "Slayer"])):
        yield dict(name=name, content=content, bundled=bundled)
    presets = Path(__file__).resolve().parents[1] / "rust/online_protocol/data/tu15-result-presets.json"
    rows = json.loads(presets.read_text())["variants"]
    assert len(rows) == 39
    for row in rows:
        raw = list(bytes.fromhex(row["settings"]))
        assert len(raw) == 304
        yield dict(name=row["name"], content=[], bundled=[], raw=raw)
        for engine in (-1, 0, 1, 2, 3, 4, 7, 8, 9, 99):
            changed = list(raw)
            changed[0x44:0x48] = int(engine).to_bytes(4, "little", signed=True)
            yield dict(name=row["name"], content=[row["name"]], bundled=[], raw=changed)
        changed = list(raw)
        changed[0x48:0x4c] = [255] * 4
        yield dict(name=row["name"], content=[row["name"]], bundled=[], raw=changed)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    report = dict(passed=False, scope="Exact captured modern C++ search controller versus production Rust/CXX over declared native IO; original service and installed runtime are not certified", cases=0, commands=0, native_refusals=0, callback_observations=0, wrapper_clones=0, wrapper_reentry_identity_controls=0, variant_controls=0, mismatches=[])
    inventory = hashlib.sha256()
    output = hashlib.sha256()
    stderr = scratch / "search-stderr.log"
    try:
        with stderr.open("wb") as errors:
            process = subprocess.Popen([str(args.binary.resolve())], stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE, stderr=errors, text=True)
            try:
                preamble = []

                def response():
                    line = process.stdout.readline()
                    while line and not line.startswith("{"):
                        # The production CMake link emits this initializer
                        # line before main; it is not a replay response.
                        if line.strip() != "Shutdown handler: initialize." or preamble:
                            raise RuntimeError("Unexpected native replay framing")
                        preamble.append(line.strip())
                        line = process.stdout.readline()
                    if not line:
                        raise RuntimeError("Native replay ended before its response")
                    return line

                for label, commands in cases():
                    request = json.dumps(dict(commands=commands), separators=(",", ":"))
                    inventory.update(request.encode() + b"\n")
                    process.stdin.write(request + "\n")
                    process.stdin.flush()
                    line = response()
                    output.update(line.encode())
                    result = json.loads(line)
                    report["cases"] += 1
                    if len(result["results"]) != len(commands):
                        raise RuntimeError("Native replay omitted an operation")
                    for index, row in enumerate(result["results"]):
                        report["commands"] += 1
                        report["callback_observations"] += len(row["rust"]["io"]["observations"])
                        if row["cpp"] != row["rust"] and len(report["mismatches"]) < 6:
                            report["mismatches"].append(dict(case=label, command=index, input=commands, result=row))
                        if row["cpp"]["error"].startswith("actual native search refusal:"):
                            report["native_refusals"] += 1
                            if not row["cpp"]["native_identity"] or not row["rust"]["native_identity"]:
                                raise RuntimeError("Native exception object identity changed")
                    if not all(result[k] for k in ("wrapper_clone", "wrapper_reentry", "wrapper_exception_identity")):
                        raise RuntimeError("Actual public wrapper clone, reentry or exception identity changed")
                    report["wrapper_clones"] += 1
                    report["wrapper_reentry_identity_controls"] += 1
                for v in variants():
                    request = json.dumps(dict(variant=v), separators=(",", ":"))
                    inventory.update(request.encode() + b"\n")
                    process.stdin.write(request + "\n")
                    process.stdin.flush()
                    line = response()
                    output.update(line.encode())
                    result = json.loads(line)
                    report["variant_controls"] += 1
                    if result["cpp"] != result["rust"] and len(report["mismatches"]) < 6:
                        report["mismatches"].append(dict(case="variant-choice", input=v, result=result))
                process.stdin.close()
                if process.wait(timeout=30):
                    raise RuntimeError("Native replay failed")
                report.update(passed=not report["mismatches"], input_sha256=inventory.hexdigest(),
                              output_sha256=output.hexdigest(), native_preamble=preamble,
                              binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest())
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
    except BaseException as error:
        report["error"] = str(error)
        report["stderr_tail"] = stderr.read_text(errors="replace")[-4096:] if stderr.exists() else ""
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k not in ("mismatches", "stderr_tail")}))
    if not report["passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
