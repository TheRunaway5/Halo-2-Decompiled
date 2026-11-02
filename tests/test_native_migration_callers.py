"""Exact modern C++ migration callers versus production Rust orchestration.

Both callers invoke the same real Rust Session owner. Agreement/QoS/selection
observations, times and IO refusals are declared fixtures. Every sampled field,
call order, effect, state and result is compared. This is not original Live,
genuine provider, native world or installed Linux delivery acceptance.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import subprocess

ROOT = Path(__file__).resolve().parents[1]
U64 = (1 << 64) - 1


def cases():
    for op, phase, binding, agreement, force, fenced, feature, started, failed, decision in itertools.product(
            ["fence", "terminal", "begin"], ["connecting", "loading", "running", "failed"],
            [False, True], [False, True], [False, True], [False, True],
            [False, True], [False, True], [False, True],
            ["none", "valid", "bad_epoch", "bad_bytes", "bad_digest", "bad_voters"]):
        yield dict(op=op, phase=phase, binding=binding, agreement=agreement, force=force,
                   fenced=fenced, feature=feature, started=started, failed=failed, decision=decision)
    for phase, binding, agreement, quality, selection, feature, supported, complete, failed, suspected, fenced in itertools.product(
            ["connecting", "loading", "running", "failed"], *([[False, True]] * 10)):
        yield dict(op="candidate", phase=phase, binding=binding, agreement=agreement, quality=quality,
                   selection=selection, feature=feature, supported=supported, complete=complete,
                   selection_failed=failed, suspected=suspected, fenced=fenced)
    for nodes in [2, 3, 4, 8, 16]:
        for self_peer in range(nodes):
            for ready, ranked in itertools.product([0, 1, 2, 4, 0x8000, 0xffff],
                    [[], [0], [1], [15, 3, 2, 1, 0], [3, 3, 2, 1], [16, 255], [256, 1], [(1 << 32) - 1]]):
                yield dict(op="candidate", nodes=nodes, self=self_peer, ready=ready, ranked=ranked)
    changes = [dict(epoch=1), dict(revision=U64), dict(peers=0), dict(peers=2), dict(host=16), dict(host=255), dict(session=True)]
    for op, target, change in itertools.product(["fence", "begin", "candidate"], ["binding_change", "quality_change"], changes):
        yield dict(op=op, **{target: change})
    for op, fail, decision in itertools.product(["fence", "begin", "candidate", "terminal"],
            ["agreement", "quality_context", "ranked", "supported", "selection", "fence", "apply", "refresh", "audio_fence", "begin", "pick"],
            ["none", "valid", "bad_bytes"]):
        yield dict(op=op, fail=fail, force=True, decision=decision)
    for self_peer, feature, decision in itertools.product([0, 1, 2, 3], [False, True],
            ["none", "valid", "bad_revision", "bad_round", "bad_previous"]):
        yield dict(self=self_peer, feature=feature, decision=decision, commands=[
            dict(op="candidate", now=0), dict(op="begin"), dict(op="fence"),
            dict(op="candidate", now=U64), dict(op="terminal"), dict(op="fence", failed=True),
            dict(op="begin"), dict(op="candidate", now=1), dict(op="fence", force=True)])
    rng = random.Random(0x48524D49)
    for _ in range(3000):
        nodes = rng.choice([2, 3, 4, 8, 16])
        yield dict(op="candidate", nodes=nodes, self=rng.randrange(nodes), host=rng.randrange(nodes),
                   ready=rng.getrandbits(16), electorate=rng.getrandbits(16),
                   silent=rng.getrandbits(16), majority=rng.getrandbits(32), rejected=rng.getrandbits(64),
                   feature=bool(rng.getrandbits(1)), supported=bool(rng.getrandbits(1)),
                   complete=bool(rng.getrandbits(1)), selection_failed=bool(rng.getrandbits(1)),
                   suspected=bool(rng.getrandbits(1)), fenced=bool(rng.getrandbits(1)),
                   ranked=[rng.randrange(20) for _ in range(rng.randrange(19))],
                   decision=rng.choice(["none", "valid", "bad_epoch"]), now=rng.getrandbits(64))


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--binary", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    report = dict(passed=False, scope=__doc__, original_live=False, native_world=False,
                  genuine_provider=False, installed_client=False)
    stderr = (scratch / "migration-callers.log").open("wb")
    child = subprocess.Popen([str(args.binary)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=stderr)
    commands = refusals = steps = 0
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
            digest.update(raw)
            digest.update(json.dumps(result, sort_keys=True, separators=(",", ":")).encode() + b"\n")
            commands += 1
            values = result["result"]["steps"]
            steps += len(values)
            refusals += sum(v["error"] for v in values)
            op = "sequence" if "commands" in command else command.get("op", "candidate")
            ops[op] = ops.get(op, 0) + 1
        report.update(passed=True, commands=commands, steps=steps, expected_state_or_io_refusals=refusals,
                      operations=ops, sha256=digest.hexdigest(),
                      binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),
                      source_pins={n: hashlib.sha256((ROOT / n).read_bytes()).hexdigest() for n in [
                          "tests/native_migration_callers_tests.cpp", "tests/test_native_migration_callers.py",
                          "tests/reference/native_migration_callers_before_rust_20261101.inc"]})
    except BaseException as error:
        report.update(error=str(error), commands=commands, steps=steps, last_command=last)
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
    print(json.dumps({k: report[k] for k in ["passed", "commands", "steps", "expected_state_or_io_refusals", "operations", "sha256"]}))


if __name__ == "__main__":
    main()
