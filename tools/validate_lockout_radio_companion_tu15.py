#!/usr/bin/env python3
"""Compare one authored Lockout radio request with the original TU1.5 call."""

import argparse
from contextlib import closing
from dataclasses import asdict, dataclass
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

from audit_rendering import Cache, file_sha
from mcc_content import MccCache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EFLAGS, UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/reference/halo2-decompiled/tools"))
from xbe import Xbe

NATIVE_SOURCE = ROOT / "tests/lockout_radio_companion_request_tu15.cpp"
XBE_HASH = "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"
LOCKOUT_HASH = "14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9"
COLLECTION = r"sound\global_fx"
CHILD = r"sound\dsp_effects\radio_static\radio_static_in"
ENTRY, LOOP_CALL = 0x224040, 0x224342
NONE = 0xffffffff


def require(condition, message):
    if not condition:
        raise ValueError(message)


def pack_words(words):
    return struct.pack(f"<{len(words)}I", *words)


def words(data):
    return struct.unpack(f"<{len(data) // 4}I", data)


def float_word(value):
    return words(struct.pack("<f", value))[0]


@dataclass(frozen=True)
class Companion:
    collection_datum: int
    instance_offset: int
    child_datum: int
    gain_bits: int
    component_flags: int
    loop_flags: int


def companion(cache, mcc):
    matching = [t for t in cache.tags.values() if t["group"] == "sfx+" and t["name"] == COLLECTION]
    require(len(matching) == 1, f"{cache.path}: missing unique radio effect collection")
    collection_datum = matching[0]["datum"]
    _, collection_offset = cache.tag(collection_datum, "sfx+")
    rows = cache.block(collection_offset, 0x38)
    require(len(rows) > 11, f"{cache.path}: missing radio effect row")
    row = rows[11]
    string_count, string_size, index_table, strings = (48, 56, 60, 52) if mcc else (356, 360, 364, 368)
    index = cache.u32(row) & 0xffff
    require(index < cache.u32(string_count), f"{cache.path}: radio row string ID is outside table")
    offset = cache.u32(cache.u32(index_table) + index * 4)
    require(offset < cache.u32(string_size), f"{cache.path}: radio row name is outside table")
    name = cache.string(cache.u32(strings) + offset, cache.u32(string_size) - offset)
    require(name == "radio_default_in", f"{cache.path}: unexpected radio row name {name}")

    instances = cache.block(row + 0x30, 0x28)
    require(len(instances) == 1, f"{cache.path}: expected one authored effect instance")
    parts = cache.block(instances[0] + 8, 0x10)
    require(len(parts) == 1, f"{cache.path}: expected one authored radio companion")
    group, datum, gain_bits, flags = struct.unpack_from("<IIII", cache.data, parts[0])
    require(group == int.from_bytes(b"lsnd", "big"), f"{cache.path}: expected looping sound reference")
    tag, at = cache.tag(datum, "lsnd")
    require(tag["name"] == CHILD, f"{cache.path}: unexpected radio companion path {tag['name']}")
    # MCC places its remastered reference before the original flags.
    return Companion(collection_datum, instances[0], datum, gain_bits, flags, cache.u32(at + (8 if mcc else 0)))


def pairs(original, local, shared, source, mcc):
    require(source.gain_bits == mcc.gain_bits == float_word(-3.0), "Radio companion gain mismatch")
    require(source.component_flags == mcc.component_flags == 0, "Unsupported radio companion component flags")
    require(not (source.loop_flags & 0x48 or mcc.loop_flags & 0x248), "Unexpected radio child admission flags")
    for datum, group, name in ((mcc.collection_datum, "sfx+", COLLECTION), (mcc.child_datum, "lsnd", CHILD)):
        local_tag = local.tags.get(datum)
        require(local_tag and local_tag["group"] == group and local_tag["name"] == name and
                local_tag["offset"] is None, f"{local.path}: absent or conflicting shared {group} reference")
        tag, _ = shared.tag(datum, group)
        require(tag["name"] == name, f"{shared.path}: mismatched retained shared {group}")
    require(source.child_datum != mcc.child_datum, f"{original.path}: expected independent salted identities")


def xbox_requests(image, cache, component, cases):
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x590000)
    for section in image.sections:
        if section.rsize:
            cpu.mem_write(section.va, image.section_bytes(section))
    page = cache.magic & ~0xfff
    cpu.mem_map(page, (cache.magic + len(cache.data) - page + 0xfff) & ~0xfff)
    cpu.mem_write(cache.magic, cache.data[:])
    index = cache.u32(16)
    table = cache.u32(index + 8) - (cache.u32(index) - index - 32)

    base = 0x1000000
    cpu.mem_map(base, 0x10000)
    effect_pool, controller_pool, sound_pool = (base + at for at in (0, 0x100, 0x200))
    effect, controller, parent = (base + at for at in (0x400, 0x500, 0x600))
    stack = base + 0xf000
    actor = next(t for t in cache.tags.values() if t["group"] == "snd!" and t["name"].endswith("mc_warthog_p_exit"))

    def write(address, *values):
        cpu.mem_write(address, pack_words(values))

    for address, value in ((0x4f0414, table + cache.magic), (0x52d670, effect_pool),
                           (0x52d668, controller_pool), (0x4f2c4c, sound_pool),
                           (effect_pool + 0x44, effect), (controller_pool + 0x44, controller),
                           (sound_pool + 0x44, parent), (parent + 0xc, actor["datum"])):
        write(address, value)

    observed = []

    def capture(machine, _address, _size, _data):
        arguments = words(machine.mem_read(machine.reg_read(UC_X86_REG_ESP), 32))
        location = bytes(machine.mem_read(arguments[4], 68))
        observed.append((arguments, location))
        machine.emu_stop()

    cpu.hook_add(UC_HOOK_CODE, capture, begin=LOOP_CALL, end=LOOP_CALL)
    for effect_id, parent_id, phase, priority, location in cases:
        controller_id = 0x81240000
        write(controller + 4, effect_id | 0x4000)
        write(parent + 0x18, *location)
        # The 0x18 stop-state flags from the original witness are part of the
        # declared phase-2 fixture, rather than a simulated scheduler result.
        flags = 0x18 if phase == 2 else 0
        cpu.mem_write(effect, struct.pack("<HBBHHIII", effect_id >> 16, 0, 1, flags, priority,
                                        controller_id, parent_id, component.instance_offset + cache.magic))
        write(stack, 0xeeeeeeee, effect_id, phase)
        cpu.reg_write(UC_X86_REG_ESP, stack)
        cpu.reg_write(UC_X86_REG_EFLAGS, 0x202)
        before = len(observed)
        cpu.emu_start(ENTRY, 0x224374, count=10000)
        require(len(observed) == before + 1, f"Original phase {phase} never reached its loop call")
    return observed


def native_requests(scratch, compiler, component, cases):
    binary = scratch / "lockout_radio_companion_request_tu15"
    command = [compiler, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "include"),
               str(NATIVE_SOURCE), "-o", str(binary)]
    subprocess.run(command, check=True, timeout=60)
    test_input = "".join(
        " ".join(f"{v:x}" for v in (component.child_datum, component.gain_bits, phase,
                                  priority, parent_id, effect_id, *location)) + "\n"
        for effect_id, parent_id, phase, priority, location in cases)
    raw = subprocess.run([str(binary)], input=test_input, text=True, capture_output=True,
                         check=True, timeout=15).stdout.splitlines()
    require(len(raw) == len(cases), "Native fixture lost a request")
    observed = []
    for line in raw:
        values = tuple(int(field, 16) for field in line.split())
        require(len(values) == 76, "Invalid native request event size")
        observed.append((values[:8], bytes(values[8:])))
    return command, observed


def validate(args, evidence):
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    image = Xbe(args.xbe)
    evidence["inputs"] = {str(p): file_sha(p) for p in (args.xbox_lockout, args.mcc_lockout, args.mcc_shared)}
    evidence["inputs"][str(args.xbe)] = hashlib.sha256(image.data).hexdigest()
    require(evidence["inputs"][str(args.xbe)] == XBE_HASH and
            evidence["inputs"][str(args.xbox_lockout)] == LOCKOUT_HASH, "Unexpected Xbox TU1.5 or original Lockout bytes")
    with closing(Cache(args.xbox_lockout)) as original, closing(MccCache(args.mcc_lockout)) as local, \
            closing(MccCache(args.mcc_shared)) as shared:
        source, mcc = companion(original, False), companion(shared, True)
        pairs(original, local, shared, source, mcc)
        evidence["authored"] = {"xbox": asdict(source), "mcc": asdict(mcc), "effect": "radio_default_in", "child": CHILD}

        # Each phase observes a current parent, with changing location and
        # explicit distance bounds. The second cohort is centered.
        cases = []
        for phase in range(3):
            vector = tuple(map(float_word, (0.75, -3.0 + phase / 4, 1.0 + phase, 2.0,
                                          3.0, 0.5, 1.0, -0.5, 0.0, 0.0, 0.0, 0.0,
                                          4.0 + phase, 32.0 + phase, 0.0, 0.0)))
            cases.append((0x81230000, 0x81250000, phase, 0x1237, (0x0100010c, *vector)))
            cases.append((0x83460000, 0x83560000, phase, 0x1253, (0x0000000c, *vector)))

        expected = xbox_requests(image, original, source, cases)
        evidence["compiler"], actual = native_requests(scratch, args.cxx, mcc, cases)
        for index, ((arguments, location), (native_args, native_location)) in enumerate(zip(expected, actual, strict=True)):
            require(arguments[0] == source.child_datum, "Xbox witness requested the wrong authored sound")
            translated = (mcc.child_datum, *arguments[1:4], NONE, *arguments[5:])
            comparison = {"phase": cases[index][2], "original_arguments": arguments, "native_arguments": native_args,
                          "original_location_hex": location.hex(), "native_location_hex": native_location.hex()}
            evidence["comparisons"].append(comparison)
            require(native_args == translated and native_location == location, f"Native request mismatch at case {index}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ("xbe", "xbox-lockout", "mcc-lockout", "mcc-shared", "report-output"):
        parser.add_argument(f"--{arg}", type=Path, required=True)
    parser.add_argument("--cxx", default="c++")
    args = parser.parse_args()
    evidence = {"passed": False, "scope": __doc__, "original_entry": ENTRY, "original_call_site": LOOP_CALL,
                "comparisons": [], "validator_sha256": file_sha(Path(__file__)),
                "native_fixture_sha256": file_sha(NATIVE_SOURCE),
                "native_request_header_sha256": file_sha(ROOT / "include/halo2/source_loop_sound_request_tu15.hpp"),
                "scratch": os.environ.get("HALO2_TEST_SCRATCH")}
    try:
        validate(args, evidence)
        evidence["passed"] = True
        print(f"PASS {len(evidence['comparisons'])} Lockout radio request comparisons across phases 0–2")
    except Exception as error:
        evidence["error"] = str(error)
        raise
    finally:
        args.report_output.parent.mkdir(parents=True, exist_ok=True)
        args.report_output.write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
