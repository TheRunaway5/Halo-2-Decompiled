"""Preserved C++ feature/view/postgame caller versus production Rust policy.

Audio devices, buffers, phase races, readiness and IO failures are declared
fixtures. Frames are compared in full. No original Live, genuine provider,
played world, waveform or installed Linux delivery acceptance is implied.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
U32 = (1 << 32) - 1
U64 = (1 << 64) - 1


def cases():
    for phase, audio, focused, enabled, ready, outgoing, incoming, after, generation in itertools.product(
            [0, 1, 2, 3, 4, 5, 6, 7, 8, 256, 257, U32], [False, True], [False, True],
            [False, True], [False, True], [0, 1, 4], [0, 1, 3], [None, 0, 3, 4, 5, U32], [0, 999, U64]):
        v = dict(phase=phase, audio=audio, focused=focused, enabled=enabled, ready=ready,
                 outgoing=outgoing, incoming=incoming, generation=generation)
        if after is not None:
            v["after_phase"] = after
        yield v
    for phase, fail in itertools.product([0, 3, 4, 5, U32], ["pin", "phase", "enable", "focus", "radio",
            "blocks_read", "blocks", "preferences", "outgoing", "fence", "ready", "exchange", "receive"]):
        yield dict(phase=phase, fail=fail)
    for muted, blocked in itertools.product([0, 1, 0x8000, 0xffff], repeat=2):
        yield dict(muted=muted, blocked=blocked)
    for common, supported, candidate in itertools.product([False, True], repeat=3):
        yield dict(op="snapshot", common=common, supported=supported, candidate=candidate)
    for snapshot, selection, terminal in [(False, False, False), (True, False, False),
                                          (False, True, False), (True, True, False), (False, False, True)]:
        yield dict(op="limits", snapshot=snapshot, selection=selection, terminal=terminal)
    for n, group, legacy in itertools.product([2, 4, 8, 16], [1, 2, 4], [False, True]):
        for self_peer in range(0, n, group):
            yield dict(op="seats", seats=n, group=group, legacy=legacy, self=self_peer)
    for n, group, stage, mode in itertools.product([8, 16], [1, 2, 4], [0, 1, 2], ["native", "continuity", "terminal"]):
        yield dict(op="metadata", seats=n, group=group, stage=stage,
                   continuity=mode == "continuity", terminal=mode == "terminal")
    for direct in [False, True]:
        yield dict(op="control", direct=direct)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    stderr = (scratch / "session-access-fixture.log").open("wb")
    child = subprocess.Popen([str(args.binary)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=stderr)
    report = dict(passed=False, scope=__doc__, original_live=False, native_world=False,
                  genuine_provider=False, installed_client=False)
    commands = refusals = 0
    ops = {}
    startup = []
    digest = hashlib.sha256()
    last = None
    try:
        for command in cases():
            last = command
            raw = (json.dumps(command, separators=(",", ":")) + "\n").encode()
            child.stdin.write(raw)
            child.stdin.flush()
            while True:
                line = child.stdout.readline()
                if line.strip() in (b"Shutdown handler: initialize.", b"Shutdown handler: cleanup."):
                    startup.append(line.decode().strip())
                    continue
                result = json.loads(line)
                break
            assert result.get("matched"), dict(command=command, result=result)
            assert not result["result"].get("held", False), dict(command=command, result=result)
            digest.update(raw)
            digest.update(json.dumps(result, sort_keys=True, separators=(",", ":")).encode() + b"\n")
            commands += 1
            refusals += result["result"].get("error", False)
            op = command.get("op", "pump")
            ops[op] = ops.get(op, 0) + 1
        report.update(passed=True, commands=commands, declared_io_refusals=refusals, operations=ops,
                      sha256=digest.hexdigest(), binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),
                      source_pins={n: hashlib.sha256((ROOT / n).read_bytes()).hexdigest() for n in [
                          "tests/native_session_access_tests.cpp", "tests/test_native_session_access.py",
                          "tests/reference/native_session_limits_before_rust_20261101.inc",
                          "tests/reference/native_session_postgame_before_rust_20261101.inc",
                          "tests/reference/native_session_views_before_rust_20261101.inc"]})
    except BaseException as error:
        report.update(error=str(error), commands=commands, last_command=last)
        raise
    finally:
        child.stdin.close()
        try:
            child.wait(timeout=5)
        except subprocess.TimeoutExpired:
            child.kill()
            child.wait()
        child.stdout.close()
        stderr.close()
        report.update(startup_messages=startup, child_exit_code=child.returncode)
        if child.returncode != 0:
            report["passed"] = False
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + "\n")
    assert child.returncode == 0
    print(json.dumps({k: report[k] for k in ["passed", "commands", "declared_io_refusals", "operations", "sha256"]}))


if __name__ == "__main__":
    main()
