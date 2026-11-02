"""Exact preserved-reference/production authority public-API and wire comparison.

Clocks, private byte banks, authenticated endpoint inputs and explicit native
validation calls are declared fixtures. No original Live, genuine provider,
prepared world or installed runtime claim is made. Exceptions compare their
invalid_argument/other category; message text is not compared.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
from test_peer_mesh_quality_replay import Replay

ROOT = Path(__file__).resolve().parents[1]
U32 = (1 << 32) - 1
U64 = (1 << 64) - 1


def context(peers=(0, 4), **changes):
    c = dict(session="42" * 32, epoch=3, revision=5,
             peers=sum(1 << p for p in peers), host=peers[0])
    c.update(changes)
    return c


def limits(**changes):
    l = dict(maximum=262144, rate=262144, burst=4096, retry=10,
             offer=50, idle=5000, lifetime=30000, window=8, features=128)
    l.update(changes)
    return l


def data(n):
    return bytes((i * 37 + 11) & 255 for i in range(n))


def new(r, p, c, l, **changes):
    v = dict(op="new", id=p, self=p, context=c, limits=l)
    v.update(changes)
    return r.exchange(v)


def query(r, op, p, **v):
    return r.exchange(dict(op=op, id=p, **v))


class Network:
    def __init__(self, r, peers=(0, 4), configured=None):
        self.r, self.peers, self.c = r, peers, context(peers)
        self.l = configured or limits()
        self.now, self.serial, self.captured, self.chunks = 1000, 0, {}, {}
        for p in peers:
            assert new(r, p, self.c, self.l) == dict(ok=True)
        for _ in range(4):
            self.step(False)
        for p in peers:
            s = query(r, "state", p)
            assert s["context"] == self.c
            for q in peers:
                if q != p:
                    assert s["supported"][q]

    def step(self, loss=True, budget=16):
        self.now += 5
        events = []
        for p in self.peers:
            packets = query(self.r, "poll", p, now=self.now, budget=budget)["packets"]
            assert len(packets) <= min(budget, 64)
            for packet in reversed(packets):
                b = bytes.fromhex(packet["bytes"])
                assert 65 <= len(b) <= 1100
                q = packet["peer"]
                key = (p, q, b[8])
                if key not in self.captured or len(b) > len(self.captured[key]):
                    self.captured[key] = b
                if b[8] == 3:
                    self.chunks.setdefault((p, q, int.from_bytes(b[57:65], "little"),
                                            int.from_bytes(b[65:69], "little")), b)
                self.serial += 1
                if loss and self.serial % 13 == 0:
                    continue
                events.append((p, q, b))
                if loss and self.serial % 17 == 0:
                    events.append((p, q, b))
        for p, q, b in reversed(events):
            assert query(self.r, "receive", q, peer=p, bytes=b.hex(), now=self.now) == dict(value=True)

    def publish(self, size, tick, targets=None):
        targets = list(self.peers[1:]) if targets is None else targets
        return query(self.r, "publish", self.c["host"], context=self.c,
                     tick=tick, bytes=data(size).hex(), targets=targets, now=self.now)["seal"]

    def finish_bytes(self, seal, targets=None, loss=True):
        targets = list(self.peers[1:]) if targets is None else targets
        received = {}
        for _ in range(4000):
            self.step(loss, 1 if self.now % 30 else U32)
            for p in targets:
                if p not in received:
                    v = query(self.r, "completed", p)["completed"]
                    if v is not None:
                        assert v["context"] == self.c and v["seal"] == seal
                        assert v["peer"] == self.c["host"] and v["bytes"] == data(seal["bytes"]).hex()
                        received[p] = v
            if len(received) == len(targets) and all(query(self.r, "delivered", self.c["host"], context=self.c, seal=seal, peer=p)["value"] for p in targets):
                return received
        raise AssertionError("deterministic authority byte delivery deadline")

    def acknowledge(self, seal, targets=None):
        targets = list(self.peers[1:]) if targets is None else targets
        for p in targets:
            assert query(self.r, "ack", p, context=self.c, seal=seal) == dict(value=True)
        received = []
        for _ in range(1000):
            self.step()
            received += query(self.r, "validated", self.c["host"])["receipts"]
            if all(any(v["peer"] == p and v["seal"] == seal for v in received) for p in targets):
                return received
        raise AssertionError("explicit authority validation receipt deadline")


def transfers(r):
    for size in [1, 1023, 1024, 1025, 8193, 65539]:
        n = Network(r)
        seal = n.publish(size, 100)
        assert query(r, "ack", 4, context=n.c, seal=seal) == dict(value=False)
        n.finish_bytes(seal)
        assert query(r, "validated", 0) == dict(receipts=[])
        n.acknowledge(seal)
        assert query(r, "preparing", 0, context=n.c, seal=seal, peer=4, now=n.now) == dict(value=False)
    for peers in [(0, 4, 8, 12), tuple(range(16))]:
        n = Network(r, peers)
        seal = n.publish(2051, 200)
        n.finish_bytes(seal)
        n.acknowledge(seal)
        for p in peers:
            query(r, "state", p)
    # One physical owner has no recipients; empty publications still refuse.
    c = context((0,))
    assert new(r, 0, c, limits()) == dict(ok=True)
    assert query(r, "poll", 0, now=0, budget=U32) == dict(packets=[])
    assert "error" in query(r, "publish", 0, context=c, tick=0, bytes="aa", targets=[], now=0)


def pipeline(r):
    n = Network(r)
    one = n.publish(4097, 10)
    n.finish_bytes(one)
    n.now += 300
    two = query(r, "ahead", 0, context=n.c, previous=one, tick=11,
                bytes=data(6003).hex(), peer=4, now=n.now)["seal"]
    n.finish_bytes(two)
    assert query(r, "validated", 0) == dict(receipts=[])
    assert "error" in query(r, "ahead", 0, context=n.c, previous=two, tick=12,
                            bytes=data(100).hex(), peer=4, now=n.now)
    # The native executor's declared ordered-chain validation recovers a lost
    # predecessor Ready only through its explicit successor acknowledgement.
    received = n.acknowledge(two)
    assert [v["seal"] for v in received] == [one, two]
    n.now += 300
    three = query(r, "ahead", 0, context=n.c, previous=two, tick=12,
                  bytes=data(3077).hex(), peer=4, now=n.now)["seal"]
    n.finish_bytes(three)
    assert query(r, "reject", 4, context=n.c, seal=three) == dict(value=True)
    n.step(False)
    assert query(r, "failed", 0, context=n.c, seal=three, peer=4, now=n.now) == dict(value=True)
    assert query(r, "ack", 4, context=n.c, seal=three) == dict(value=False)
    assert query(r, "validated", 0) == dict(receipts=[])
    n.now += 300
    four = n.publish(1025, 13)
    n.finish_bytes(four)
    n.acknowledge(four)
    # The legacy maximum-only Hello does not grant pipeline capabilities.
    n = Network(r, configured=limits(features=0))
    one = n.publish(1025, 10)
    n.finish_bytes(one)
    assert query(r, "feature", 0, peer=4, feature=128) == dict(value=False)
    assert "error" in query(r, "ahead", 0, context=n.c, previous=one, tick=11,
                            bytes=data(100).hex(), peer=4, now=n.now + 300)


def deadlines_and_reset(r):
    for renew in [False, True]:
        n = Network(r)
        one = n.publish(4097, 10)
        n.finish_bytes(one)
        began = n.now
        wrong = dict(n.c, revision=n.c["revision"] + 1)
        assert query(r, "failed", 0, context=wrong, seal=one, peer=4, now=began + 40000) == dict(value=False)
        assert query(r, "failed", 0, context=n.c, seal=one, peer=4, now=began + 4999) == dict(value=False)
        assert query(r, "preparing", 0, context=n.c, seal=one, peer=4, now=began - 1) == dict(value=True)
        if renew:
            for offset in range(4000, 28001, 4000):
                assert query(r, "preparing", 0, context=n.c, seal=one, peer=4, now=began + offset) == dict(value=True)
            n.now = began + 30001
        else:
            n.now = began + 6000
        assert query(r, "failed", 0, context=n.c, seal=one, peer=4, now=n.now) == dict(value=True)
        assert query(r, "preparing", 0, context=n.c, seal=one, peer=4, now=n.now) == dict(value=False)
        assert query(r, "ack", 4, context=n.c, seal=one) == dict(value=True)
        n.step(False)
        assert query(r, "validated", 0) == dict(receipts=[])
        next_context = dict(n.c, epoch=n.c["epoch"] + 1, revision=0, host=4)
        for p in n.peers:
            assert query(r, "replace", p, expected=n.c, context=next_context) == dict(value=True)
            assert query(r, "state", p)["context"] == next_context
            assert query(r, "ack", p, context=n.c, seal=one) == dict(value=False)
        assert query(r, "replace", 0, expected=n.c, context=next_context) == dict(value=False)
    # The native scan cursor survives a reset, even if no frame can be emitted.
    c = context((0, 4, 8, 12))
    new(r, 0, c, limits())
    assert query(r, "poll", 0, now=1000, budget=1)["packets"][0]["peer"] == 4
    for now in [1000, 999, 0, U64]:
        query(r, "poll", 0, now=now, budget=0)
    after = dict(c, revision=c["revision"] + 1)
    assert query(r, "replace", 0, expected=c, context=after) == dict(value=True)
    assert query(r, "poll", 0, now=1, budget=1)["packets"][0]["peer"] == 8


def malformed(r):
    n = Network(r)
    seal = n.publish(2049, 77)
    n.finish_bytes(seal, loss=False)
    n.acknowledge(seal)
    n.now += 300
    denied = n.publish(1027, 78)
    n.finish_bytes(denied, loss=False)
    assert query(r, "reject", 4, context=n.c, seal=denied) == dict(value=True)
    n.step(False)
    assert set(k[2] for k in n.captured) == set(range(7))
    # Correct-direction Hello frames seed only actual decoder capabilities.
    hello_client = n.captured[(0, 4, 0)]
    hello_host = n.captured[(4, 0, 0)]
    offer = n.captured[(0, 4, 2)]
    representatives = {}
    for (source, target, kind), packet in n.captured.items():
        representatives.setdefault(kind, (source, target, packet))
    assert len(representatives[3][2]) == 1093
    for kind, (source, target, packet) in representatives.items():
        cases = [packet[:i] for i in range(len(packet))]
        cases += [packet + b"\0", bytes(1401), packet]
        for i in range(len(packet)):
            changed = bytearray(packet)
            changed[i] ^= 0x80
            cases.append(bytes(changed))
        for b in cases:
            new(r, target, n.c, n.l)
            hello = hello_host if target == 0 else hello_client
            query(r, "receive", target, peer=source, bytes=hello.hex(), now=1000)
            if target == 0:
                query(r, "publish", 0, context=n.c, tick=77, bytes=data(2049).hex(), targets=[4], now=1000)
                query(r, "poll", 0, now=1000, budget=64)
            elif kind == 3:
                query(r, "receive", target, peer=0, bytes=offer.hex(), now=1000)
                selected_id = int.from_bytes(packet[57:65], "little")
                selected_index = int.from_bytes(packet[65:69], "little")
                # Reach the actual hash on the selected final chunk, rather
                # than testing only a partially filled receive bank.
                for (p, q, ident, index), good in sorted(n.chunks.items()):
                    if p == source and q == target and ident == selected_id and index != selected_index:
                        query(r, "receive", target, peer=source, bytes=good.hex(), now=1000)
            query(r, "is_packet", target, bytes=b.hex())
            query(r, "receive", target, peer=source, bytes=b.hex(), now=1001)
            query(r, "completed", target)
            query(r, "validated", target)
            query(r, "state", target)
            query(r, "poll", target, now=1002, budget=16)
    rng = random.Random(0xA07150)
    for _ in range(400):
        b = bytearray(rng.randbytes(rng.randrange(0, 1450)))
        if len(b) >= 7:
            b[:4], b[6] = b"H2N2", 7
        query(r, "is_packet", 0, bytes=b.hex())
        query(r, "receive", 0, peer=rng.choice([0, 4, 16, 256, U32]), bytes=b.hex(), now=1003)
    for peer in [0, 1, 4, 15, 16, 255, 256, 257, U32]:
        query(r, "receive", 4, peer=peer, bytes=offer.hex(), now=1004)
        query(r, "supported", 4, peer=peer)
        for feature in [0, 1, 128, 0xFFFFFFFF]:
            query(r, "feature", 4, peer=peer, feature=feature)
    return dict(packet_lengths_by_type={str(k):len(v[2]) for k,v in representatives.items()},
                full_chunk_payload_bytes=1024, other_chunks_seeded_before_selected_final_chunk=True,
                every_representative_byte_mutated=True, every_representative_truncation_compared=True)


def invalid_api(r):
    c, l = context(), limits()
    new(r, 0, c, l)
    for bad in [16, 255, 256, 257, U32]:
        assert "error" in new(r, 0, c, l, **{"self": bad})
        assert "error" in new(r, 0, dict(c, host=bad), l)
        assert query(r, "state", 0)["context"] == c
    for changes in [dict(session="00" * 32), dict(peers=0), dict(peers=16), dict(host=1)]:
        assert "error" in new(r, 0, dict(c, **changes), l)
    for key, value in [("maximum", 0), ("maximum", 1 << 30), ("rate", 0), ("rate", 1 << 31),
                       ("burst", 1099), ("burst", 1 << 21), ("retry", 9), ("offer", 9),
                       ("idle", 9), ("lifetime", 9), ("window", 0), ("window", 65537)]:
        assert "error" in new(r, 0, c, dict(l, **{key: value}))
    n = Network(r)
    for targets in [[], [0], [16], [256], [U32], [4, 4]]:
        assert "error" in query(r, "publish", 0, context=c, tick=10, bytes="aa", targets=targets, now=n.now)
    for wrong in [dict(c, host=256), dict(c, revision=6), dict(c, epoch=U32)]:
        assert "error" in query(r, "publish", 0, context=wrong, tick=10, bytes="aa", targets=[4], now=n.now)
    for b in ["", data(l["maximum"] + 1).hex()]:
        assert "error" in query(r, "publish", 0, context=c, tick=10, bytes=b, targets=[4], now=n.now)
    assert "error" in query(r, "publish", 4, context=c, tick=10, bytes="aa", targets=[0], now=n.now)
    one = n.publish(1025, 10)
    assert "error" in query(r, "publish", 0, context=c, tick=9, bytes="aa", targets=[4], now=n.now)
    assert "error" in query(r, "ahead", 0, context=c, previous=one, tick=11, bytes="aa", peer=4, now=n.now)
    for change in [dict(epoch=2), dict(revision=4), dict(host=4)]:
        assert query(r, "replace", 0, expected=c, context=dict(c, **change)) == dict(value=False)
    for bad in [dict(c, session="00" * 32), dict(c, peers=0), dict(c, host=256)]:
        assert "error" in query(r, "replace", 0, expected=c, context=bad)
    for edge in [dict(c, epoch=U32), dict(c, revision=U64)]:
        new(r, 0, edge, l)
        after = dict(edge, epoch=(edge["epoch"] + 1) & U32, revision=(edge["revision"] + 1) & U64)
        query(r, "replace", 0, expected=edge, context=after)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--rust", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]) / "authority-replay"
    scratch.mkdir()
    replay = Replay(args.reference, args.rust, scratch)
    reference = ROOT / "tests/reference/peer_authority_transfer_before_rust_20261101.json"
    provenance = json.loads(reference.read_text())
    report = dict(passed=False, scope=__doc__, reference=provenance,
                  production_source_sha256=hashlib.sha256((ROOT / "src/network/peer_authority_transfer.cpp").read_bytes()).hexdigest(),
                  comparison_role="preserved_cpp_vs_configured_production; qualification identifies the compiled production owner",
                  binaries={k:hashlib.sha256(v.read_bytes()).hexdigest() for k, v in [("cpp",args.reference),("rust",args.rust)]},
                  native_world=False, original_xnet=False, retained_large_artifacts=[])
    try:
        assert hashlib.sha256((ROOT / "tests/reference/peer_authority_transfer_before_rust_20261101.cpp").read_bytes()).hexdigest() == provenance["sha256"]
        assert hashlib.sha256((ROOT / provenance["header"]["reference"]).read_bytes()).hexdigest() == provenance["header"]["sha256"]
        for run in [transfers, pipeline, deadlines_and_reset]:
            run(replay)
        report["malformed_coverage"] = malformed(replay)
        invalid_api(replay)
        report["passed"] = True
    except BaseException as error:
        report["error"] = repr(error)[:8192]
        report["last_command"] = {k:(v[:256] + "..." if isinstance(v,str) and len(v)>256 else v) for k,v in (replay.last or {}).items()}
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
    print("PASS authority reference/production public API: " + str(replay.commands) + " commands, " + replay.digest.hexdigest())


if __name__ == "__main__":
    main()
