#!/usr/bin/env python3
"""Bounded original SMG projectile construction and first-motion oracles.

Default mode isolates the type callback with supplied generic storage. The
allocation suite runs the original upper allocator and later new-object pass,
with explicitly recorded lower storage, location and clear-world boundaries.
"""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import (
    ROOT, PIN, CACHE_PIN, Trace, POOL, HEADERS, BITMAP, PROJECTILE,
    UNIT, WEAPON, TAGS, DEF, SEED, TIME, Cache, UC_X86_REG_EAX,
    UC_X86_REG_EDI, UC_X86_REG_ESP, Xbe, _Code, _trace,
)

PI = 0x10002
DESCRIPTOR = 0x10E0000
ERROR = 0x10E1000


class AllocationTrace(Trace):
    """Original B8A30 with lower allocation and clear-world service fixtures."""

    def __init__(self, mode, aim, owner_velocity):
        super().__init__(mode, aim)
        cache = Cache(ROOT / "assets/disc/maps/lockout.map")
        for datum, tag in cache.tags.items():
            self.g.mem_write(TAGS + (datum & 0xFFFF) * 16, tag["group"].encode()[::-1])
        datum = int(self.authored["tag"]["datum"], 16)
        self.w(WEAPON, datum)
        self.w(TAGS + (datum & 0xFFFF) * 16 + 8, DEF)
        cache.data.close()
        cache.stream.close()
        del self.services[0xB8A30]
        self.services.update({
            0xBD170: ("supplied object storage allocation", PI, 4),
            0x18FD70: ("supplied fixed arena memory growth", 1, 12),
            0xC0270: ("supplied fixed arena heap maintenance", 0, 0),
            0x16B240: ("supplied clear collision world", 0, 24),
            0x18B1F0: ("supplied unavailable projectile sound runtime", 0, 16),
        })
        self.w(UNIT + 0x3E4, -1)
        self.w(UNIT + 0x3E8, -1)
        self.g.mem_write(UNIT + 0x88, struct.pack("<3f", *owner_velocity))
        # The original postpass sets this byte while spawning pending shots.
        self.g.mem_write(0x1040000 + 3, b"\x01")
        self.w(0x526254, 0x10F2000)
        self.f(0x10F2000, 0)
        self.w(0x517FE0, 0x10F0000)
        self.allocation_returns = []

    def hook(self, guest, address, size, user):
        if address in (0x1088B2, 0x109321):
            stack = guest.reg_read(UC_X86_REG_ESP)
            row = {"tick": self.tick, "phase": self.active,
                   "source_velocity_instruction": f"{address:08x}"}
            if address == 0x1088B2:
                row.update({"original_aim": self.vec(stack + 0x28),
                            "source_owner_velocity": self.vec(stack + 0xB4),
                            "clamped_forward_scalar_bits": f"{self.u(stack + 0x8C):08x}",
                            "clamped_forward_scalar": self.fv(stack + 0x8C),
                            "platform_velocity_present": self.u(stack + 0x27, 1)})
            else:
                row.update({"final_forward": self.vec(stack + 0x12C),
                            "request_velocity_before_speed_override": self.vec(stack + 0x144),
                            "source_platform_velocity": self.vec(stack + 0xEC),
                            "speed_override_bits": f"{self.u(stack + 0x64):08x}"})
            self.events.append(row)
        for return_address, row in self.allocation_returns[:]:
            if address == return_address:
                row["returned_eax"] = f"{guest.reg_read(UC_X86_REG_EAX):08x}"
                row["returned_object_hex"] = bytes(guest.mem_read(PROJECTILE, 428)).hex()
                row["returned_header_hex"] = bytes(guest.mem_read(HEADERS + 24, 12)).hex()
                self.allocation_returns.remove((return_address, row))
        if address == 0xBD170:
            stack = guest.reg_read(UC_X86_REG_ESP)
            requested = self.u(stack + 4, 2)
            assert requested == 428
            self.w(POOL + 0x38, 3)
            self.w(BITMAP, 7)
            guest.mem_write(PROJECTILE, bytes(0x1000))
            # Original 16D2C0 zeroes the freshly allocated header, then assigns
            # its salt; BD170 assigns the requested length and storage pointer.
            guest.mem_write(HEADERS + 24, struct.pack("<HBBHHI", 1, 0, 0, 0, requested, PROJECTILE))
        if address == 0xB8A30:
            stack = guest.reg_read(UC_X86_REG_ESP)
            descriptor = self.u(stack + 4)
            row = {
                "tick": self.tick, "phase": self.active, "entry": "000b8a30",
                "request_descriptor_hex": bytes(guest.mem_read(descriptor, 0xC4)).hex(),
                "request_position": self.vec(descriptor + 0x1C),
                "request_forward": self.vec(descriptor + 0x28),
                "request_up": self.vec(descriptor + 0x34),
                "request_velocity": self.vec(descriptor + 0x40),
                "request_definition": f"{self.u(descriptor):08x}",
            }
            self.events.append(row)
            self.allocation_returns.append((self.u(stack), row))
        if address in (0xFC2E0, 0xFE1E0, 0xFCEC0, 0xFCF90, 0xB8040, 0xB8180):
            self.events.append({"tick": self.tick, "phase": self.active, "entry": f"{address:08x}",
                                "projectile_object_hex": bytes(guest.mem_read(PROJECTILE, 428)).hex(),
                                "projectile_header_hex": bytes(guest.mem_read(HEADERS + 24, 12)).hex()})
        super().hook(guest, address, size, user)


def execute_allocation_case(mode, aim, owner_velocity):
    trace = AllocationTrace(mode, aim, owner_velocity)
    trace.execute(1, 0)
    created = bytes(trace.g.mem_read(PROJECTILE, 428)).hex()
    created_header = bytes(trace.g.mem_read(HEADERS + 24, 12)).hex()
    assert trace.u(HEADERS + 24 + 2, 1) == 0x0A
    assert not any(e.get("entry") == "000fcf90" for e in trace.events)
    # Explicit supplied location, followed by the original activation routine.
    # This proves the active/new-object postpass path, not a real BSP lookup.
    trace.active = "supplied_location_activation"
    trace.sh(HEADERS + 24 + 4, 0)
    trace.run(0xB8180, regs=((UC_X86_REG_EDI, PI),))
    assert trace.u(HEADERS + 24 + 2, 1) == 0x0B
    activated_header = bytes(trace.g.mem_read(HEADERS + 24, 12)).hex()
    trace.active = "later_original_b8040"
    trace.run(0xB8040)
    motion = [e for e in trace.events if e.get("entry") == "000fcf90"]
    assert len(motion) == 1 and motion[0]["phase"] == "later_original_b8040"
    rng = [e for e in trace.events if "rng_write_instruction" in e]
    assert len(rng) == 2
    return {
        "mode": mode, "supplied_aim": list(aim), "supplied_owner_velocity": list(owner_velocity),
        "after_creation_object_hex": created, "after_creation_header_hex": created_header,
        "after_original_activation_header_hex": activated_header,
        "after_later_postpass_object_hex": bytes(trace.g.mem_read(PROJECTILE, 428)).hex(),
        "after_later_postpass_header_hex": bytes(trace.g.mem_read(HEADERS + 24, 12)).hex(),
        "position": trace.vec(PROJECTILE + 0x64), "velocity": trace.vec(PROJECTILE + 0x88),
        "distance_bits": f"{trace.u(PROJECTILE + 0x168):08x}",
        "events": trace.events, "rng_stores": rng,
        "scope": "Original B8A30 and type callback, then original B8040, with explicitly supplied lower storage/heap, absent sound runtime, clear collision and location0 activation. Original location lookup, active-world registration and complete original frame are not executed. No current platform; supplied gravity0 has no effect because authored air gravity scale is0.",
    }


def dump_instructions(directory):
    directory = directory.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    image = Xbe(ROOT / "assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
    assert hashlib.sha256(image.data).hexdigest() == PIN
    code = _Code(image, ".text")
    starts = {int(row["address"], 16) for row in csv.DictReader(
        (ROOT / "analysis/map-pack-update/functions.tsv").open(), delimiter="\t")}
    entries = (0xB8A30, 0xBD170, 0x16D2C0, 0xBD270, 0xFC2E0, 0x101990,
               0xFFE20, 0xFE1E0, 0xFCEC0, 0xFCF90, 0xB8180, 0xB8040,
               0xB7E00, 0xB7F50, 0xBD360, 0x10CDF0, 0x139750,
               0x108300, 0xC95F0, 0xBB0C0, 0x10DF90,
               0x1479B0, 0x139880, 0x139540)
    rows = []
    for entry in entries:
        routine = _trace(code, entry, starts | set(entries))
        path = directory / f"{entry:08x}.asm"
        instructions = [code.at(address) for address in sorted(routine.code)]
        path.write_text("".join(f"{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n"
                                for i in instructions))
        rows.append({"start": f"{entry:08x}", "end_exclusive": f"{routine.end:08x}",
                     "instruction_count": len(instructions),
                     "reachable_instruction_bytes_sha256": hashlib.sha256(
                         b"".join(i.bytes for i in instructions)).hexdigest(),
                     "listing": str(path.relative_to(ROOT)),
                     "listing_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                     "direct_calls": [{"site": f"{site:08x}", "target": f"{target:08x}"}
                                      for site, target, kind in routine.sites if kind == "call"]})
    report = {"xbe_sha256": PIN,
              "oracle_tool_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "routines": rows,
              "scope": "Read-only reachable original instruction listings; static operands/calls do not imply every branch was executed."}
    (directory / "inventory.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"routines": len(rows)}))


def execute_case(request, seed):
    trace = Trace(5)
    trace.active = "projectile_type_constructor"
    trace.tick = request["tick"]
    trace.w(TIME + 8, request["tick"])
    trace.w(SEED, int(seed, 16))
    trace.w(POOL + 0x38, 3)
    trace.w(BITMAP, 7)
    # Supplied lower allocation storage; original B8A30 copies these descriptor
    # vectors into these object fields before calling its type constructors.
    raw = bytes.fromhex(request["request_descriptor_hex"])
    trace.g.mem_write(DESCRIPTOR, raw)
    trace.g.mem_write(PROJECTILE, bytes(0x1000))
    trace.g.mem_write(HEADERS + 24, struct.pack("<HBBHHI", 1, 3, 5, 0xFFFF, 428, PROJECTILE))
    trace.w(PROJECTILE, int(request["request_definition"], 16))
    for source, target, length in (
        (0x1C, 0x64, 12), (0x28, 0x70, 12), (0x34, 0x7C, 12),
        (0x40, 0x88, 12), (0x4C, 0x94, 12), (0x58, 0xA0, 4),
    ):
        trace.g.mem_write(PROJECTILE + target, raw[source:source + length])
    trace.sh(PROJECTILE + 0x2C, -1)
    trace.w(PROJECTILE + 0x14, -1)
    # Explicit fixture: the owner has no current platform reference. These
    # state values are not claimed to come from executing the unit constructor.
    trace.w(UNIT + 0x3E4, -1)
    trace.w(UNIT + 0x3E8, -1)
    before = bytes(trace.g.mem_read(PROJECTILE, 428))
    event_start = len(trace.events)
    trace.run(0xFC2E0, (PI, DESCRIPTOR, ERROR))
    after = bytes(trace.g.mem_read(PROJECTILE, 428))
    fields = {
        f"{offset:03x}": {"bits": after[offset:offset + 4].hex(),
                           "float": struct.unpack_from("<f", after, offset)[0]}
        for offset in (0x15C, 0x164, 0x16C, 0x170, 0x180, 0x184, 0x188, 0x18C, 0x190)
    }
    return {
        "tick": request["tick"], "descriptor_hex": raw.hex(),
        "definition": request["request_definition"], "seed_before": seed,
        "seed_after": f"{trace.u(SEED):08x}",
        "returned_eax": f"{trace.g.reg_read(UC_X86_REG_EAX):08x}",
        "before_object_hex": before.hex(), "after_object_hex": after.hex(),
        "position": trace.vec(PROJECTILE + 0x64),
        "forward": trace.vec(PROJECTILE + 0x70),
        "up": trace.vec(PROJECTILE + 0x7C),
        "velocity": trace.vec(PROJECTILE + 0x88),
        "owner_index": f"{trace.u(PROJECTILE + 0x140):08x}",
        "projectile_flags": f"{trace.u(PROJECTILE + 0x12C):08x}",
        "fields": fields, "events": trace.events[event_start:],
        "supplied_generic_allocation": True,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/multiplayer/map-pack/smg-projectile-constructor-1.1-oracle.json")
    parser.add_argument("--allocation-suite", action="store_true", help="Run original allocator and later new-object postpass with recorded lower-service fixtures.")
    parser.add_argument("--instruction-inventory", type=Path, help="Write original projectile/frame/seed instruction listings and hashes.")
    args = parser.parse_args()
    if args.instruction_inventory is not None:
        dump_instructions(args.instruction_inventory)
        return
    if args.allocation_suite:
        cases = []
        for mode in (1, 5):
            for aim in ((1., 0., 0.), (0., 1., 0.), (0., 0., 1.), (.6, .8, 0.)):
                for velocity in ((0., 0., 0.), (5., -3., 2.), (-5., 3., -2.), (0., 0., 5.)):
                    cases.append(execute_allocation_case(mode, aim, velocity))
        report = {"xbe_sha256": PIN, "cache_sha256": CACHE_PIN,
                  "oracle_tool_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  "cases": cases, "case_count": len(cases),
                  "scope": "Original allocator/type callback/new-object postpass, supplied lower services and activation/location. Fixed shared seed12345678 is a fixture. Complete world registration, impacts, other RNG consumers and full multiplayer are unresolved."}
        args.output.write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps({"cases": len(cases), "same_tick_original_motion_cases": len(cases)}))
        return
    cases = []
    sources = []
    for axis in ("x", "y", "z"):
        path = ROOT / f"analysis/multiplayer/map-pack/smg-firing-suite/held_{axis}-mode5.json"
        report = json.loads(path.read_text())
        assert report["xbe_sha256"] == PIN and report["cache_sha256"] == CACHE_PIN
        sources.append({"report": str(path.relative_to(ROOT)), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
        for request in report["events"]:
            if request.get("service") != "000b8a30":
                continue
            case = execute_case(request, request["state"]["seed"])
            case["axis"] = axis
            assert case["seed_before"] == case["seed_after"]
            assert case["returned_eax"][-2:] == "01"
            assert not case["events"]
            cases.append(case)
    result = {
        "xbe_sha256": PIN, "cache_sha256": CACHE_PIN,
        "oracle_tool_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "sources": sources, "entry": "000fc2e0", "cases": cases,
        "case_count": len(cases),
        "scope": "Untouched original projectile type constructor with authored SMG bullet metadata, recorded source creation descriptors and explicitly supplied generic object allocation fields. No current owner platform. Original whole allocator, successful object registration, collision, impact, damage and full simulation RNG order are not executed or established.",
    }
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"cases": len(cases), "constructor_rng_writes": 0}))


if __name__ == "__main__":
    main()
