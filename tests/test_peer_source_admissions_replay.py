"""Exact public-API C++/Rust admission replay with synthetic private identities.

The deterministic network, clocks and identity rows are declared fixtures.
No native allocation/world, provider, original Live or installed claim is made.
"""
import argparse
import copy
import hashlib
import heapq
import json
import os
from pathlib import Path
import random
from test_peer_mesh_quality_replay import Replay

ROOT = Path(__file__).resolve().parents[1]
U32 = (1 << 32) - 1
U64 = (1 << 64) - 1


def context(peers, **changes):
    value = dict(session="42" * 32, epoch=3, revision=5,
        peers=sum(1 << p for p in peers), host=peers[0])
    value.update(changes)
    return value


def roster(count, group):
    return [[i, i // group * group, i % group != 0,
        bytes([i + 1] * 8).hex()] for i in range(count)]


def players(count):
    return [[i, 0x10000 + i, bytes([i + 3] * 12).hex()] for i in range(count)]


def new(replay, peer, c, rows, **changes):
    value = dict(op="new", id=peer, self=peer, context=c, roster=rows)
    value.update(changes)
    return replay.exchange(value)


def state(replay, peer):
    return replay.exchange(dict(op="state", id=peer))


def publish(replay, peer, c, rows):
    return replay.exchange(dict(op="publish", id=peer, context=c, players=rows))


def poll(replay, peer, now, budget=16):
    return replay.exchange(dict(op="poll", id=peer, now=now, budget=budget))["packets"]


def receive(replay, target, peer, data):
    return replay.exchange(dict(op="receive", id=target, peer=peer, bytes=data.hex()))


def resign(packet):
    value = bytearray(packet)
    value[57:89] = hashlib.sha256(b"H2 SOURCE ADMISSIONS 1" + value[9:56] + value[89:]).digest()
    return bytes(value)


def deliver(replay, peers, start, loss):
    queue = []
    serial = 0
    for now in range(start, start + 2601, 50):
        for peer in peers:
            for packet in poll(replay, peer, now, 1 if now % 150 else U32):
                serial += 1
                data = bytes.fromhex(packet["bytes"])
                assert 89 <= len(data) <= 522
                if loss and serial % 7 == 1:
                    continue
                heapq.heappush(queue, (now + 25 + serial % 73, serial * 2, peer, packet["peer"], data))
                if serial % 11 == 3:
                    heapq.heappush(queue, (now + 160, serial * 2 + 1, peer, packet["peer"], data))
        while queue and queue[0][0] <= now:
            _, _, source, target, data = heapq.heappop(queue)
            assert receive(replay, target, source, data) == dict(recognized=True)
        if now % 250 == 0:
            for peer in peers:
                state(replay, peer)
    while queue:
        _, _, source, target, data = heapq.heappop(queue)
        receive(replay, target, source, data)


def exchange_case(replay, count, group, loss, legacy=False):
    peers = tuple(range(0, count, group))
    c = context(peers)
    seats = roster(count, group)
    if legacy:
        assert group == 1
        for seat in seats:
            seat[1] = U32
    rows = players(count)
    for peer in peers:
        assert new(replay, peer, c, seats) == dict(ok=True)
        assert state(replay, peer)["accepted"] is None
        assert not poll(replay, peer, 0, 0)
    assert publish(replay, peers[0], c, rows) == dict(published=True)
    assert publish(replay, peers[0], c, rows) == dict(published=True)
    for peer in peers[1:]:
        assert publish(replay, peer, c, rows) == dict(published=False)
    deliver(replay, peers, 0, loss)
    original = state(replay, peers[0])["accepted"]
    for peer in peers:
        observed = state(replay, peer)
        assert observed["accepted"] == original
        if peer == peers[0]:
            assert all(observed["confirmed"][p] for p in peers[1:])
        assert not poll(replay, peer, 4000)
    if len(peers) == 1:
        return
    # Ordinary committed reset clears values, queues and receipts, but not the
    # original native scan cursor. A valid new epoch may change the host.
    next_context = dict(c, epoch=c["epoch"] + 1, revision=c["revision"] + 1, host=peers[-1])
    for peer in peers:
        assert replay.exchange(dict(op="replace", id=peer, expected=c, context=next_context)) == dict(replaced=True)
        assert state(replay, peer)["accepted"] is None
    assert publish(replay, peers[-1], next_context, rows) == dict(published=True)
    deliver(replay, peers, 5000, loss)
    # Carry every original row over an independently committed host-loss input.
    # A resumed client emits no acknowledgement until it sees a successor offer.
    alive = list(peers)
    current = next_context
    for _ in range(min(2, len(peers) - 1)):
        departed = current["host"]
        alive.remove(departed)
        after = dict(current, epoch=current["epoch"] + 1, revision=current["revision"] + 1,
            host=alive[0], peers=current["peers"] & ~(1 << departed))
        for peer in alive:
            previous = state(replay, peer)["accepted"]
            assert replay.exchange(dict(op="resume", id=peer, self=peer, context=after,
                roster=seats, previous=previous)) == dict(ok=True)
            observed = state(replay, peer)
            assert observed["accepted"]["players"] == rows
            assert not any(observed["confirmed"])
            if peer != after["host"]:
                assert not poll(replay, peer, 9000)
        changed = copy.deepcopy(rows)
        changed[0][2] = "ff" * 12
        assert publish(replay, after["host"], after, changed) == dict(published=False)
        # Skip delivery in the first successor generation for a delayed learner.
        current = after


def malformed_case(replay):
    seats = roster(16, 4)
    rows = players(16)
    c = context((0, 4, 8, 12))
    new(replay, 0, c, seats)
    publish(replay, 0, c, rows)
    offer = bytes.fromhex(poll(replay, 0, 1000, 1)[0]["bytes"])
    assert len(offer) == 522
    assert not poll(replay, 0, 999, 0)
    new(replay, 4, c, seats)
    # Recognized truncations, every mutated byte (with and without a valid hash)
    # and malformed endpoint IDs are compared through actual API reads.
    corpus = [offer[:n] for n in range(len(offer))]
    corpus += [offer + b"\0", b"", bytes(7)]
    for index in range(len(offer)):
        changed = bytearray(offer)
        changed[index] ^= 0x80
        corpus.append(bytes(changed))
        if index >= 89:
            corpus.append(resign(changed))
    # Correctly rehashed duplicate native identity and invalid native handle.
    changed = bytearray(offer)
    changed[132:144] = changed[105:117]
    corpus.append(resign(changed))
    changed = bytearray(offer)
    changed[101:105] = b"\xff" * 4
    corpus.append(resign(changed))
    rng = random.Random(0xAD815510)
    for _ in range(300):
        value = bytearray(rng.randbytes(rng.randrange(0, 540)))
        if len(value) >= 7:
            value[:4] = b"H2N2"
            value[6] = 8
        corpus.append(bytes(value))
    for data in corpus:
        new(replay, 4, c, seats)
        replay.exchange(dict(op="is_packet", bytes=data.hex()))
        receive(replay, 4, 0, data)
        state(replay, 4)
        poll(replay, 4, 1001)
    for source in [1, 4, 15, 16, 255, 256, U32]:
        new(replay, 4, c, seats)
        receive(replay, 4, source, offer)
        assert state(replay, 4)["accepted"] is None
    new(replay, 4, c, seats)
    receive(replay, 4, 0, offer)
    accepted = state(replay, 4)["accepted"]
    ack = bytes.fromhex(poll(replay, 4, 1002)[0]["bytes"])
    for source in [0, 1, 16, 256, U32]:
        receive(replay, 0, source, ack)
        assert not state(replay, 0)["confirmed"][4]
    receive(replay, 0, 4, ack)
    assert state(replay, 0)["confirmed"][4]
    for data in corpus:
        receive(replay, 4, 0, data)
        assert state(replay, 4)["accepted"] == accepted
    for bad in [16, 255, 256, U32]:
        assert "error" in new(replay, 4, c, seats, **{"self": bad})
        wrong = dict(c, host=bad)
        assert "error" in new(replay, 4, wrong, seats)
    for column, value in [(0, 256), (1, 256), (2, True)]:
        changed = copy.deepcopy(seats)
        changed[4][column] = value
        assert "error" in new(replay, 4, c, changed)
    for column, value in [(0, 256), (1, U32), (2, rows[0][2])]:
        changed = copy.deepcopy(rows)
        changed[4][column] = value
        assert "error" in publish(replay, 0, c, changed)
    for bad_rows in [[], rows[:1], rows + [rows[0]]]:
        assert "error" in publish(replay, 0, c, bad_rows)
    for bad_roster in [[], seats[:1], seats + [seats[0]]]:
        assert "error" in new(replay, 4, c, bad_roster)
    for bad in [16, 255, 256, U32]:
        assert publish(replay, 0, dict(c, host=bad), rows) == dict(published=False)
    for key, value in [("epoch", U32), ("revision", U64)]:
        edge = dict(c, **{key: value})
        new(replay, 0, edge, seats)
        publish(replay, 0, edge, rows)
        previous = state(replay, 0)["accepted"]
        after = dict(edge, epoch=(edge["epoch"] + 1) & U32, revision=(edge["revision"] + 1) & U64, host=4, peers=c["peers"] & ~1)
        assert "error" in replay.exchange(dict(op="resume", id=4, self=4, context=after, roster=seats, previous=previous))
    # Exhaustive full-width constructor fields, zero session and wrong fixed
    # physical masks; all failed attempts must leave the prior owner intact.
    before = state(replay, 4)
    for changes in [dict(session="00" * 32), dict(peers=0), dict(peers=0xffff), dict(host=1)]:
        assert "error" in new(replay, 4, dict(c, **changes), seats)
        assert state(replay, 4) == before


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--rust", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]) / "source-admissions-replay"
    scratch.mkdir()
    replay = Replay(args.reference, args.rust, scratch)
    report = dict(passed=False, scope=__doc__, reference=json.loads((ROOT / "tests/reference/peer_source_admissions_before_rust_20261101.json").read_text()),
        binaries={label:hashlib.sha256(path.read_bytes()).hexdigest() for label,path in [("cpp",args.reference),("rust",args.rust)]},
        native_world=False, original_xnet=False, retained_large_artifacts=[])
    try:
        reference_source = ROOT / "tests/reference/peer_source_admissions_before_rust_20261101.cpp"
        reference_header = ROOT / report["reference"]["header"]["reference"]
        assert hashlib.sha256(reference_source.read_bytes()).hexdigest() == report["reference"]["sha256"]
        assert hashlib.sha256(reference_header.read_bytes()).hexdigest() == report["reference"]["header"]["sha256"]
        for count, group in [(2, 1), (4, 4), (8, 4), (16, 4), (16, 1)]:
            for loss in [False, True]:
                exchange_case(replay, count, group, loss)
        for count in [2, 16]:
            for loss in [False, True]:
                exchange_case(replay, count, 1, loss, legacy=True)
        malformed_case(replay)
        report["passed"] = True
    except BaseException as error:
        report["error"] = repr(error)[:8192]
        report["last_command"] = replay.last
        raise
    finally:
        replay.close()
        report.update(commands=replay.commands, packet_outputs=replay.packets, receives=replay.receives, state_reads=replay.states,
            corpus_and_observations_sha256=replay.digest.hexdigest(), sdk_diagnostics=replay.sdk,
            native_exit_codes=[p.returncode for p in replay.children])
        report["passed"] = report["passed"] and report["native_exit_codes"] == [0, 0]
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + "\n")
    assert report["passed"], report
    print("PASS actual C++/Rust admission public API: " + str(replay.commands) + " commands, " + replay.digest.hexdigest())


if __name__ == "__main__":
    main()
