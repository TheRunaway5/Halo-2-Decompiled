"""Preserved modern C++ / Rust snapshot public API and exact wire replay.

Private native leases are real. The archive bytes, clock, authenticated peer
inputs and foreground assertions are declared fixtures, not native world,
genuine provider, original Live or installed-client acceptance.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
from test_peer_mesh_quality_replay import Replay

U32 = (1 << 32) - 1
U64 = (1 << 64) - 1
ROOT = Path(__file__).resolve().parents[1]


def context(peers=(0, 4), **changes):
    c = dict(session="42" * 32, epoch=3, revision=5,
             peers=sum(1 << p for p in peers), host=peers[0])
    c.update(changes)
    return c


def limits(**changes):
    l = dict(maximum=131072, retry=10, idle=5000, window=8)
    l.update(changes)
    return l


def data(n, p=4):
    return bytes((i * 37 + p * 11 + i // 13) & 255 for i in range(n))


def new(r, p, c=None, l=None, **changes):
    return r.exchange(dict(dict(op="new", id=p, self=p, context=c or context(), limits=l or limits()), **changes))


def q(r, op, p=0, **v):
    return r.exchange(dict(op=op, id=p, **v))


class Network:
    def __init__(self, r, peers=(0, 4), configured=None):
        self.r, self.peers, self.c = r, peers, context(peers)
        self.l = configured or limits()
        self.now, self.serial = 1000, 0
        self.samples = {}
        for p in peers:
            assert new(r, p, self.c, self.l) == dict(ok=True)
        self.token = None

    def step(self, loss=True, budget=16, allow=lambda p, q, b: True):
        self.now += 5
        events = []
        for p in self.peers:
            packets = q(self.r, "poll", p, now=self.now, budget=budget)["packets"]
            assert len(packets) <= min(budget, 64)
            for row in packets:
                b, target = bytes.fromhex(row["bytes"]), row["peer"]
                assert 67 <= len(b) <= 1095 and target in self.peers and target != p
                assert self.r.exchange(dict(op="is_packet", bytes=b.hex())) == dict(value=True)
                self.samples.setdefault((b[8], p, target), b)
                self.serial += 1
                if not allow(p, target, b) or loss and self.serial % 13 == 0:
                    continue
                events.append((p, target, b))
                if loss and self.serial % 17 == 0:
                    events.append((p, target, b))
        for p, target, b in reversed(events):
            assert q(self.r, "receive", target, peer=p, bytes=b.hex(), now=self.now) == dict(value=True)

    def wait(self, ready, **kwargs):
        for _ in range(4000):
            if ready():
                return
            self.step(**kwargs)
        raise AssertionError("deterministic snapshot schedule deadline")

    def arm(self):
        t = q(self.r, "begin", self.c["host"], now=self.now)["token"]
        assert t is not None
        self.token = t
        self.wait(lambda: all(q(self.r, "state", p)["pending"] == t for p in self.peers[1:]))
        assert q(self.r, "state", self.c["host"])["armed"] == 0
        for p in self.peers[1:]:
            assert q(self.r, "arm", p, token=t) == dict(value=True)
        self.wait(lambda: q(self.r, "state", self.c["host"])["armed"] == t["recipients"])
        return t

    def publish(self, size):
        batch = [dict(peer=p, bytes=data(size, p).hex()) for p in self.peers[1:]]
        assert q(self.r, "publish", self.c["host"], token=self.token, archives=batch, now=self.now) == dict(value=True)

    def take(self, size):
        held = set()

        def ready():
            for p in self.peers[1:]:
                if p in held:
                    continue
                d = q(self.r, "take", p, lease=p)
                if d["present"]:
                    assert d["delivery"] == dict(token=self.token, source=self.c["host"], recipient=p, bytes=data(size, p).hex())
                    held.add(p)
            return len(held) == len(self.peers) - 1
        self.wait(ready, budget=1)

    def close(self, mode, held=False):
        assert q(self.r, "close", self.c["host"], token=self.token, mode=mode) == dict(value=True)
        self.wait(lambda: all(q(self.r, "state", p)["closing"] == mode for p in self.peers[1:]))
        for p in self.peers[1:]:
            lease = p if held else None
            before = q(self.r, "state", p)
            assert q(self.r, "can_retire", p, token=self.token, lease=lease)["value"]
            assert q(self.r, "state", p) == before
            assert q(self.r, "retire", p, token=self.token, lease=lease) == dict(value=True, delivery=None)
            assert q(self.r, "retire", p, token=self.token, lease=None) == dict(value=True, delivery=None)
        self.wait(lambda: q(self.r, "state", self.c["host"])["retired"] == self.token["recipients"])


def normal_and_abort(r):
    for peers in [(0, 4), (12, 0, 4, 8), tuple(reversed(range(16)))]:
        n = Network(r, peers)
        for size in ([1, 1023, 1024, 1025, 8193, 65539] if len(peers) == 2 else [2051]):
            t = n.arm()
            n.publish(size)
            assert not q(r, "close", peers[0], token=t, mode=0)["value"]
            n.take(size)
            for p in peers[1:]:
                assert not q(r, "take", p, lease=p)["present"]
                assert not q(r, "retire", p, token=t, lease=p)["value"]
                assert q(r, "capture", p, lease=p)["value"]
                assert not q(r, "capture", p, lease=p)["value"]
            n.wait(lambda: q(r, "state", peers[0])["captured"] == t["recipients"])
            assert q(r, "state", peers[0])["buffered"] == 0
            n.close(0)
    # A genuine same-context foreign owner issues an indistinguishable public
    # token but has a different private native identity.
    n = Network(r)
    t = n.arm()
    n.publish(2051)
    n.take(2051)
    r.exchange(dict(op="move", **{"from": 4, "to": 44}))
    assert not q(r, "capture", 4, lease=4)["value"]  # engaged moved-from lease
    assert q(r, "state", 4)["outstanding"]
    assert new(r, 40, n.c, n.l, self=4) == dict(ok=True)
    assert not q(r, "capture", 40, lease=44)["value"]
    q(r, "close", 0, token=t, mode=1)
    n.wait(lambda: q(r, "state", 4)["closing"] == 1)
    assert not q(r, "retire", 4, token=t, lease=None)["value"]
    assert not q(r, "retire", 4, token=t, lease=4)["value"]
    wrong = dict(t, nonce=t["nonce"] + 1)
    assert not q(r, "retire", 4, token=wrong, lease=44)["value"]
    assert q(r, "retire", 4, token=t, lease=44) == dict(value=True, delivery=None)
    # Foreground preflight is observational; retirement rechecks real identity.
    n.wait(lambda: q(r, "state", 0)["retired"] == t["recipients"])
    n = Network(r)
    t = q(r, "begin", 0, now=0)["token"]
    assert q(r, "close", 0, token=t, mode=1)["value"]
    n.token = t
    n.close(1)
    n = Network(r)
    t = n.arm()
    assert q(r, "reject", 4, token=t)["value"]
    n.wait(lambda: q(r, "state", 0)["closing"] == 1)
    n.close(1)
    # Partial bytes and a completed, still internal bank both retire explicitly.
    for complete in [False, True]:
        n = Network(r)
        t = n.arm()
        n.publish(4097)
        if complete:
            n.wait(lambda: q(r, "state", 4)["delivered"] == 16)
        else:
            n.wait(lambda: q(r, "state", 4)["buffered"] > 0,
                   allow=lambda p, target, b: b[8] != 3)
        n.close(1)
        assert q(r, "state", 4)["buffered"] == 0
    # Destruction by the caller alone never becomes an implicit Retired ACK.
    n = Network(r)
    t = n.arm()
    n.publish(13)
    n.take(13)
    r.exchange(dict(op="drop", lease=4))
    q(r, "close", 0, token=t, mode=1)
    n.wait(lambda: q(r, "state", 4)["closing"] == 1)
    assert not q(r, "retire", 4, token=t, lease=None)["value"]
    q(r, "poll", 0, now=U64, budget=0)
    assert q(r, "state", 0)["stalled"]
    assert q(r, "begin", 0, now=0)["token"] is None


def guards(r):
    for p in [1, 16, 256, 257, U32]:
        assert new(r, 0, self=p) == dict(error=True, category="invalid_argument")
    for c in [context(host=256), context(peers=(0,)), context(session="00" * 32)]:
        assert new(r, 0, c) == dict(error=True, category="invalid_argument")
    for field, values in dict(maximum=[0, 9437185, U64], retry=[0, 60001, U64],
                              idle=[1, 86400001, U64], window=[0, 257, 65537, U32]).items():
        for value in values:
            assert new(r, 0, l=limits(**{field: value})) == dict(error=True, category="invalid_argument")
    n = Network(r)
    t = n.arm()
    before = q(r, "state", 0)
    for mode in [2, 256, 257, U32]:
        assert not q(r, "close", 0, token=t, mode=mode)["value"]
    assert q(r, "state", 0) == before
    for batch in [[], [dict(peer=4, bytes="")], [dict(peer=256, bytes="aa")],
                  [dict(peer=4, bytes="aa")] * 2, [dict(peer=4, bytes="ff" * 131073)]]:
        assert not q(r, "publish", 0, token=t, archives=batch, now=U64)["value"]
    assert q(r, "state", 0) == before
    for field, values in dict(nonce=[0, 2, U64], recipients=[0, 17]).items():
        for value in values:
            wrong = dict(t, **{field: value})
            assert not q(r, "arm", 4, token=wrong)["value"]
            assert not q(r, "close", 0, token=wrong, mode=1)["value"]


def wire(t, target, kind, body=b"", protocol=124):
    c = t["context"]
    return (b"H2N2" + protocol.to_bytes(2, "little") + bytes([12, 1, kind]) +
            bytes.fromhex(c["session"]) + c["epoch"].to_bytes(4, "little") +
            c["revision"].to_bytes(8, "little") + c["peers"].to_bytes(2, "little") +
            bytes([c["host"], target]) + t["recipients"].to_bytes(2, "little") +
            t["nonce"].to_bytes(8, "little") + body)


def attack_seed(kind):
    """Reach the actual state branch with complete prior bytes before mutation."""
    commands = [dict(op="new", id=p, self=p, context=context(), limits=limits()) for p in [0, 4]]
    t = dict(context=context(), recipients=16, nonce=1)
    commands.append(dict(op="begin", id=0, now=0))
    receive = lambda target, source, b: dict(op="receive", id=target, peer=source, bytes=b.hex(), now=0)
    bank = data(2048)
    digest = hashlib.sha256(bank).digest()
    offer = wire(t, 4, 2, len(bank).to_bytes(8, "little") + digest + (2).to_bytes(4, "little"))
    one = wire(t, 4, 3, (0).to_bytes(4, "little") + bank[:1024])
    two = wire(t, 4, 3, (1).to_bytes(4, "little") + bank[1024:])
    captured = wire(t, 0, 5, len(bank).to_bytes(8, "little") + digest)
    if kind != 0:
        commands += [receive(4, 0, wire(t, 4, 0)), dict(op="arm", id=4, token=t)]
    if kind not in [0, 1]:
        commands += [receive(0, 4, wire(t, 0, 1))]
    if kind in [3, 6, 7]:
        commands += [receive(4, 0, offer), receive(4, 0, one)]
    if kind in [6, 7]:
        commands += [receive(4, 0, two), dict(op="take", id=4, lease=4)]
    if kind == 6:
        commands += [dict(op="capture", id=4, lease=4)]
    if kind in [4, 5, 8]:
        commands += [dict(op="publish", id=0, token=t, archives=[dict(peer=4, bytes=bank.hex())], now=0),
                     dict(op="poll", id=0, now=0, budget=64)]
    if kind == 8:
        commands += [receive(0, 4, captured), dict(op="close", id=0, token=t, mode=0)]
    if kind == 9:
        commands += [dict(op="close", id=0, token=t, mode=1)]
    target, peer = (4, 0) if kind in [0, 2, 3, 6, 7] else (0, 4)
    packets = {0: wire(t, 4, 0), 1: wire(t, 0, 1), 2: offer, 3: two,
               4: wire(t, 0, 4, (0).to_bytes(4, "little") + (1).to_bytes(8, "little")),
               5: captured, 6: wire(t, 4, 6), 7: wire(t, 4, 7),
               8: wire(t, 0, 8), 9: wire(t, 0, 9), 10: wire(t, 0, 10)}
    return commands, t, target, peer, packets[kind]


def attacks(r):
    coverage = []
    for kind in range(11):
        seed, t, target, peer, original = attack_seed(kind)
        variants = [original, original + b"\x00", original.ljust(1401, b"\x00")]
        variants += [original[:end] for end in range(len(original))]
        for index in range(len(original)):
            b = bytearray(original)
            b[index] ^= 0x80
            variants.append(bytes(b))
        for variant in variants:
            for command in seed:
                assert "error" not in r.exchange(command)
            r.exchange(dict(op="is_packet", bytes=variant.hex()))
            q(r, "receive", target, peer=peer, bytes=variant.hex(), now=100)
            q(r, "state", target)
            q(r, "take", target, lease=100)
            q(r, "poll", target, now=100, budget=64)
        # Full-width authenticated-source refusals and rollback clocks.
        for source in [target, 1, 16, 256, 257, U32]:
            for command in seed:
                r.exchange(command)
            q(r, "receive", target, peer=source, bytes=original.hex(), now=100000)
            q(r, "poll", target, now=0, budget=0)
            q(r, "state", target)
        coverage.append(dict(kind=kind, bytes=len(original), variants=len(variants),
                             seeded_commands=len(seed), every_byte_and_truncation=True))
    return coverage


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--rust", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    r = Replay(args.reference, args.rust, scratch)
    report = dict(passed=False, scope=__doc__, original_live=False, native_world=False,
                  genuine_provider=False, installed_runtime=False)
    try:
        guards(r)
        normal_and_abort(r)
        report["wire_coverage"] = attacks(r)
        report.update(passed=True, commands=r.commands, packets=r.packets,
                      receives=r.receives, states=r.states, sha256=r.digest.hexdigest(),
                      binary_pins={label: hashlib.sha256(path.read_bytes()).hexdigest()
                                   for label, path in [("reference", args.reference), ("rust", args.rust)]},
                      source_pins={name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
                                   for name in ["tests/peer_snapshot_candidate_replay.cpp", "tests/test_peer_snapshot_candidate_replay.py",
                                                "tests/reference/peer_snapshot_candidate_before_rust_20261101.cpp"]})
    except BaseException as error:
        report.update(error=str(error), last_command=r.last, commands=r.commands)
        raise
    finally:
        r.close()
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: report[k] for k in ["passed", "commands", "packets", "receives", "states", "sha256"]}))


if __name__ == "__main__":
    main()
