"""Compare actual public API reads from preserved C++ and Rust mesh owners.

The deterministic network/clock is a declared test fixture. This proves the
existing replacement H2QO contract, not original XNet or a played world.
"""
import argparse
import hashlib
import heapq
import json
import os
from pathlib import Path
import selectors
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
PEERS = (0, 4, 8, 12)
MEMBERS = sum(1 << p for p in PEERS)


class Replay:
    def __init__(self, reference, rust, scratch):
        self.children = []
        self.streams = []
        self.commands = 0
        self.receives = 0
        self.packets = 0
        self.states = 0
        self.digest = hashlib.sha256()
        self.sdk = []
        self.last = None
        self.pending = {}
        for label, binary in (("cpp", reference), ("rust", rust)):
            stream = (scratch / (label + ".log")).open("wb")
            self.streams.append(stream)
            self.children.append(subprocess.Popen([str(binary)], stdin=subprocess.PIPE,
                stdout=subprocess.PIPE, stderr=stream))
            self.pending[self.children[-1].pid] = bytearray()

    def read(self, process):
        deadline = time.monotonic() + 10
        with selectors.DefaultSelector() as selector:
            selector.register(process.stdout, selectors.EVENT_READ)
            while True:
                buffer = self.pending[process.pid]
                while b"\n" not in buffer:
                    if not selector.select(max(0, deadline - time.monotonic())):
                        raise AssertionError("Native replay response deadline")
                    data = os.read(process.stdout.fileno(), 65536)
                    if not data:
                        raise AssertionError("Native replay ended before a response")
                    buffer.extend(data)
                line, _, remainder = buffer.partition(b"\n")
                self.pending[process.pid] = bytearray(remainder)
                if line.strip() in (b"Shutdown handler: initialize.", b"Shutdown handler: cleanup."):
                    self.sdk.append(line.decode().strip())
                    continue
                return json.loads(line)

    def exchange(self, command):
        self.last = command
        data = (json.dumps(command, separators=(",", ":")) + "\n").encode()
        results = []
        for process in self.children:
            process.stdin.write(data)
            process.stdin.flush()
            results.append(self.read(process))
        if results[0] != results[1]:
            raise AssertionError(dict(command=command, reference=results[0], rust=results[1]))
        self.commands += 1
        self.digest.update(data)
        self.digest.update((json.dumps(results[0], sort_keys=True, separators=(",", ":")) + "\n").encode())
        self.receives += command["op"] == "receive"
        self.states += command["op"] == "state"
        if command["op"] == "poll":
            self.packets += len(results[0]["packets"])
        return results[0]

    def new(self, peer, **changes):
        value = dict(op="new", id=peer, self=changes.pop("self_peer", peer), session="07" + "00" * 31,
            epoch=3, revision=5, peers=MEMBERS, host=0)
        value.update(changes)
        return self.exchange(value)

    def receive(self, target, peer, data, now):
        return self.exchange(dict(op="receive", id=target, peer=peer, bytes=data.hex(), now=now))

    def state(self, peer, now):
        return self.exchange(dict(op="state", id=peer, now=now))

    def close(self):
        for child in self.children:
            try:
                child.stdin.close()
            except BrokenPipeError:
                pass
        for child in self.children:
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait()
            child.stdout.close()
        for stream in self.streams:
            stream.close()


def network(replay, *, loss=False, duplicates=False):
    for peer in PEERS:
        assert replay.new(peer) == {"ok": True}
    now, order, uploads, reports = 1000, 0, 0, 0
    events = []
    next_send = {p: 0 for p in PEERS}
    rates = {p: 1_200_000 - i * 250_000 for i, p in enumerate(PEERS)}
    captured = {}
    active = MEMBERS
    for until in (9000, 15000, 21000):
        if until == 15000:
            active &= ~1
        if until == 21000:
            active &= ~(1 << 12)
        while now < until:
            while events and events[0][0] <= now:
                _, _, source, target, data = heapq.heappop(events)
                assert replay.receive(target, source, data, now) == {"recognized": True}
            for source in PEERS:
                if not active & (1 << source):
                    continue
                result = replay.exchange(dict(op="poll", id=source,
                    ready=active & ~(1 << source), now=now, budget=16))
                assert len(result["packets"]) <= 16
                for packet in result["packets"]:
                    target = packet["peer"]
                    data = bytes.fromhex(packet["bytes"])
                    kind = data[6]
                    captured.setdefault(kind, (source, target, data))
                    assert len(data) <= 1200 and source != target
                    if not active & (1 << target):
                        continue
                    if kind == 3:
                        uploads += 1
                        if loss and source == 12 and data[60] % 8 == 0:
                            continue
                    reports += kind == 5
                    next_send[source] = max(next_send[source], now) + max(1,
                        (len(data) * 1000 + rates[source] - 1) // rates[source])
                    at = next_send[source] + 5 + source
                    if duplicates and kind == 4:
                        heapq.heappush(events, (at + 100, order, source, target, data))
                        order += 1
                    heapq.heappush(events, (at, order, source, target, data))
                    order += 1
            if now % 500 == 0:
                for peer in PEERS:
                    replay.state(peer, now)
            now += 2
        for peer in PEERS:
            replay.state(peer, now)
            replay.state(peer, now + 5001)
    assert uploads == 4 * 3 * 64 and reports > 24
    return captured


def malformed(replay, captured):
    for kind, (source, target, packet) in captured.items():
        for peer in PEERS:
            replay.new(peer)
        for size in range(0, 63):
            replay.receive(target, source, packet[:size], 2000)
        for at in (4, 5, 7, 8, 39, 40, 44, 52, 54, 55, 56, 60):
            value = bytearray(packet)
            value[at] ^= 0x80
            replay.receive(target, source, value, 2001)
        for sender in (1, 16, 255, 256, 2**32 - 1, target):
            replay.receive(target, sender, packet, 2002)
        for now in (0, 1, 999, 10_000, 2**64 - 1):
            replay.receive(target, source, packet, now)
        replay.receive(target, source, packet + b"\0", 2003)
        replay.receive(target, source, packet + bytes(1201), 2004)
        replay.state(target, 2005)
        for budget in (0, 1, 2, 4, 16, 64, 2**32 - 1):
            result = replay.exchange(dict(op="poll", id=target, ready=MEMBERS,
                now=3000, budget=budget))
            assert len(result["packets"]) <= min(budget, 64)
        replay.state(target, 3001)
    for peer in (16, 17, 255, 256, 2**32 - 1):
        assert "error" in replay.new(0, self_peer=peer)
    for host in (1, 17, 255, 256, 2**32 - 1):
        assert "error" in replay.new(0, host=host)
    assert replay.new(0, host=16) == {"ok": True}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--rust", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    source = ROOT / "tests/reference/peer_mesh_quality_before_rust_20261101.cpp"
    metadata = json.loads(source.with_suffix(".json").read_text())
    assert hashlib.sha256(source.read_bytes()).hexdigest() == metadata["sha256"]
    header = ROOT / metadata["header"]["reference"]
    assert hashlib.sha256(header.read_bytes()).hexdigest() == metadata["header"]["sha256"]
    replay = Replay(args.reference.resolve(), args.rust.resolve(), scratch)
    report = dict(passed=False, scope=__doc__.strip(), reference=metadata,
        binaries={name: hashlib.sha256(p.read_bytes()).hexdigest()
                  for name, p in (("cpp", args.reference), ("rust", args.rust))},
        played_world=False, original_xnet=False, retained_large_artifacts=[])
    try:
        for loss, duplicate in ((False, False), (True, False), (False, True)):
            captured = network(replay, loss=loss, duplicates=duplicate)
            malformed(replay, captured)
        report["passed"] = True
    except BaseException as error:
        report["error"] = str(error)[:20_000]
        report["last_command"] = replay.last
        raise
    finally:
        replay.close()
        report["passed"] = report["passed"] and all(p.returncode == 0 for p in replay.children)
        report.update(commands=replay.commands, packet_outputs=replay.packets,
            receives=replay.receives, state_reads=replay.states,
            corpus_and_observations_sha256=replay.digest.hexdigest(),
            sdk_diagnostics=replay.sdk, native_exit_codes=[p.returncode for p in replay.children])
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + "\n")
    assert report["passed"], report
    print(json.dumps({k: report[k] for k in ("passed", "commands", "packet_outputs", "receives", "state_reads")}))


if __name__ == "__main__":
    main()
