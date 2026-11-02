#!/usr/bin/env python3
"""Exact captured native caller vs Rust bridge, with declared UI/account IO."""
import argparse
import copy
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess


def countdown(seconds, serial=1, complete=False):
    return {"seconds": seconds, "serial": serial, "complete": complete}


def command(op, facts=None, **rest):
    return {"op": op, "facts": facts or {}, **rest}


def cases():
    yield "ordinary-publications", [command("enter"), *[
        command("observe", {"countdown": countdown(s, complete=s == 0)})
        for s in (10, 10, 5, 3, 3, 2, 1, 0, 0)], command("start_error")]
    flags = ["account_client", "authenticated", "party", "leader", "supported", "queue_client", "busy", "command_accepted"]
    for values in itertools.product((False, True), repeat=len(flags)):
        facts = dict(zip(flags, values))
        yield "eligibility-" + "".join(map(lambda v: str(int(v)), values)), [
            command("enter", facts), command("start_error", facts), command("accept", facts),
            command("leave", facts), command("leave_observe", facts)]
    for selected, playlists, members in itertools.product(
        ("", "ranked", "other"),
        ([], [{"id": "ranked", "revision": "v7", "eligible": False}],
         [{"id": "ranked", "revision": "v7", "eligible": True}],
         [{"id": "ranked", "revision": "first", "eligible": False}, {"id": "ranked", "revision": "second", "eligible": True}]),
        ([], [{"handle": "self", "ready": "ranked"}], [{"handle": "self", "ready": ""}],
         [{"handle": "self", "ready": "ranked"}, {"handle": "peer", "ready": "other"}])):
        facts = {"selected": selected, "playlists": playlists, "members": members}
        yield "consent", [command("enter", facts), command("start_error", facts), command("accept", facts)]
    for serial, seconds, complete, size in itertools.product(
        (0, 1, 2, 2**63, 2**64 - 1), (None, -2, -1, 0, 1, 3, 4, 10, 2147483647), (False, True), (0, 1, 2, 4)):
        members = [{"handle": f"m{i}", "ready": "ranked"} for i in range(size)]
        f = {"countdown": countdown(seconds, serial, complete), "members": members}
        changed = {**f, "countdown": countdown(seconds, serial ^ 1, complete)}
        yield "serial-stage", [command("enter"), command("observe", f), command("observe", f),
                               command("observe", changed), command("observe", {"members": members})]
    for mode, screens, supported, party in itertools.product(
        (0, 2, 3), ([6], [6, 14], [6, 14, 7], [6, 186, 210]), (False, True), (False, True)):
        f = {"mode": mode, "screens": screens, "supported": supported, "party": party,
             "countdown": countdown(3)}
        yield "scope", [command("enter"), command("observe", {"countdown": countdown(4)}),
                        command("observe", f), command("start_error", f), command("accept", f)]
    for leader, account, accepted, op, size in itertools.product(
        (False, True), (False, True), (False, True), ("back", "auxiliary", "accept", "leave"), (0, 15, 16, 17)):
        f = {"leader": leader, "account_client": account, "command_accepted": accepted,
             "countdown": countdown(3), "sounds": size}
        yield "input", [command("enter"), command("observe", f), command(op, f), command("leave_observe", f)]
    for seconds, destination, accepted in itertools.product((-2, -1, 0, 1, 3, 10), (-17, 23, 0, -1), (False, True)):
        f = {"countdown": countdown(seconds), "command_accepted": accepted}
        yield "start-focus", [command("enter"), command("observe", f), command("accept", f, destination=destination)]
    for removed, replace, published in itertools.product((False, True), (False, True), (False, True)):
        members = [{"handle": "self", "ready": "ranked"}]
        if not removed:
            members.append({"handle": "new" if replace else "peer", "ready": "ranked"})
        f = {"members": members}
        if published:
            f["countdown"] = countdown(2)
        yield "departure", [command("enter"), command("observe", {"countdown": countdown(3)}),
                            command("observe", f), command("observe", f)]
    for busy, party, message, account in itertools.product((False, True), (False, True), ("", "refused"), (False, True)):
        f = {"busy": busy, "party": party, "party_message": message, "account_client": account}
        yield "leave-ack", [command("enter"), command("leave"), command("observe", {"countdown": countdown(0, complete=True)}),
                            command("leave_observe", f), command("leave_observe", f)]
    fault_points = ("populate", "open", "dialog", "command", "cancel_queue", "sign_out", "reset_progress", "unavailable", "update_account")
    sequences = [
        [command("enter")],
        [command("enter"), command("accept")],
        [command("enter"), command("accept", {"busy": True})],
        [command("enter"), command("leave"), command("leave_observe", {"party": False})],
        [command("enter"), command("observe", {"countdown": countdown(3)}), command("back", {"countdown": countdown(3)})],
        [command("enter"), command("observe", {"countdown": countdown(3)}), command("observe", {"members": [{"handle": "self", "ready": "ranked"}]})],
        [command("enter"), command("observe", {"countdown": countdown(0, complete=True)})]]
    for point, seq in itertools.product(fault_points, sequences):
        seq = copy.deepcopy(seq)
        seq[-1]["facts"]["fault"] = point
        yield "native-refusal-" + point, seq


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    digest = hashlib.sha256()
    inventory = hashlib.sha256()
    report = {"passed": False, "scope": "Exact modern native pregame caller and input branches versus Rust; UI/account IO is declared, not installed delivery or original Live proof", "cases": 0, "commands": 0, "native_refusals": 0, "wrapper_native_exception_identity_controls": 0, "mismatches": []}
    stderr = scratch / "native-stderr.log"
    try:
        with stderr.open("wb") as errors:
            process = subprocess.Popen([str(args.binary.resolve())], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=errors, text=True)
            try:
                preamble = []
                for label, commands in cases():
                    request = json.dumps({"commands": commands}, separators=(",", ":"))
                    inventory.update(request.encode() + b"\n")
                    process.stdin.write(request + "\n")
                    process.stdin.flush()
                    while True:
                        line = process.stdout.readline()
                        if not line:
                            raise RuntimeError("Native comparison ended before its response")
                        if line.startswith("{"):
                            break
                        preamble.append(line.strip())
                        if len(preamble) > 16:
                            raise RuntimeError("Unexpected native diagnostic framing")
                    result = json.loads(line)
                    report["cases"] += 1
                    digest.update(line.encode())
                    if len(result["results"]) != len(commands):
                        raise RuntimeError("Native comparison omitted a command")
                    for index, row in enumerate(result["results"]):
                        report["commands"] += 1
                        if row["cpp"] != row["rust"] and len(report["mismatches"]) < 8:
                            report["mismatches"].append({"case": label, "command": index, "input": commands, "result": row})
                        if row["cpp"]["error"].startswith("actual native lobby refusal:"):
                            report["native_refusals"] += 1
                            if not row["cpp"]["native_exception_identity"] or not row["rust"]["native_exception_identity"]:
                                raise RuntimeError("Native exception identity changed")
                    if not all(result[k] for k in ("wrapper_callback_completed", "wrapper_native_exception", "wrapper_retained_identity")):
                        raise RuntimeError("Actual native wrapper exception identity control failed")
                    if result["wrapper_copy_seconds"] != result["wrapper_initial_seconds"] or result["wrapper_reentrant_seconds"] != -1:
                        raise RuntimeError("Actual native wrapper clone or reentry changed")
                    report["wrapper_native_exception_identity_controls"] += 1
                process.stdin.close()
                code = process.wait(timeout=30)
                if code:
                    raise RuntimeError(f"Native comparison failed with {code}")
                report.update(passed=not report["mismatches"], sha256=digest.hexdigest(), input_sha256=inventory.hexdigest(), native_preamble=preamble,
                              binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest())
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
    except BaseException as error:
        report["error"] = str(error)
        if stderr.exists():
            report["stderr_tail"] = stderr.read_text(errors="replace")[-8192:]
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k not in ("mismatches", "stderr_tail")}))
    if not report["passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
