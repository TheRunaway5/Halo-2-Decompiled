#!/usr/bin/env python3
"""Verify base-disc audio operands against complete original TU1.5 bodies.

Prefix signatures only locate candidates. Complete instruction streams,
internal branches, jump tables and literal immediates must correspond before
extracting immutable operands. External helpers and live driver state are not
proven by this static comparison.
"""
import argparse
import csv
from pathlib import Path
import struct

from oracle_mp_title_update_display import ROOT, NEW, Xbe, _Code, _trace, sha, save, encoded
from research_mp_inventory_switch_u5 import prefix, raw_prefix_candidates, norm

BASE_SHA = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
UPDATE_SHA = "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"
OWNERS = (0x226490, 0x226060, 0x224e20, 0x12db00, 0x128b10,
          0x127d70, 0x12a810, 0x220000, 0x220200, 0x129b20)
DRIVER = (
    ("source_pitch_to_driver_pitch_bits", 0x46a15c),
    ("driver_pitch_min_bits", 0x46a158),
    ("driver_pitch_max_bits", 0x46a154),
    ("gain_normalized_to_driver_volume_bits", 0x46a53c),
    ("minimum_source_gain_bits", 0x455b0c),
    ("parameter_change_threshold_bits", 0x46a178),
    ("driver_pitch_change_threshold_bits", 0x449fd4),
    ("driver_pitch_octave_scale_bits", 0x46a38c),
    ("relative_rate_ln2_bits", 0x455aa4),
    ("mono_channel_count_bits", 0x46a0d0),
    ("stereo_channel_count_bits", 0x46a11c),
    ("routing_db_ln10_bits", 0x455b14),
    ("routing_db_scale_bits", 0x46a12c),
    ("routing_linear_db_scale_bits", 0x46a118),
    ("normalized_parameter_range_epsilon_bits", 0x46a0ec),
    ("silent_source_gain_bits", 0x455c94),
)
CREATE = (
    ("default_extra_gain_bits", 0x44ad40),
    ("minimum_gain_bits", 0x455b0c),
    ("maximum_float_bits", 0x45695c),
    ("rng_scale_bits", 0x46a160),
    ("unsigned_conversion_correction_bits", 0x46a0c8),
    ("one_bits", 0x46a0d0),
    ("upper_gain_bits", 0x46a0e8),
    ("db_exponent_scale_bits", 0x46a12c),
    ("db_exponent_log_bits", 0x455b14),
    ("linear_db_scale_bits", 0x46a118),
    ("fade_epsilon_bits", 0x46a180),
)
# These two original MOV immediates forward immutable word pointers. Other
# address immediates (for example vtables) are retained without importing data.
POINTER_SITES = {0x2264d0, 0x128b43}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", type=Path, default=ROOT / "assets/disc/default.xbe")
    parser.add_argument("--update", type=Path, default=NEW)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), "Preserve previous research output"
    paths = (args.update, args.base)
    for path, pin in zip(paths, (UPDATE_SHA, BASE_SHA)):
        assert sha(path) == pin, path
    images = [Xbe(path) for path in paths]
    codes = [_Code(image, ".text") for image in images]
    inventories = (ROOT / "analysis/multiplayer/title-update-1.5/decompiled/functions.tsv",
                   ROOT / "analysis/retail/functions.tsv")
    starts = [{int(row["address"], 16) for row in csv.DictReader(path.open(), delimiter="\t")}
              for path in inventories]
    buckets = {}
    for entry in starts[1]:
        buckets.setdefault(tuple(prefix(codes[1], images[1], entry)), []).append(entry)
    owners, uses = [], []
    for entry in OWNERS:
        body = _trace(codes[0], entry, starts[0])
        before = [codes[0].at(address) for address in sorted(body.code)]
        positions = {ins.address: n for n, ins in enumerate(before)}
        expected = [norm(ins, images[0], positions) for ins in before]
        candidates = set(buckets.get(tuple(prefix(codes[0], images[0], entry)), []))
        candidates.update(raw_prefix_candidates(codes[0], images[0], codes[1], entry))
        matches = []
        for candidate in sorted(candidates):
            other = _trace(codes[1], candidate, starts[1])
            after = [codes[1].at(address) for address in sorted(other.code)]
            positions = {ins.address: n for n, ins in enumerate(after)}
            if [norm(ins, images[1], positions) for ins in after] == expected:
                matches.append((candidate, other, after))
        assert len(matches) == 1, (hex(entry), [hex(row[0]) for row in matches])
        target, other, after = matches[0]
        address_map = {a.address: b.address for a, b in zip(before, after)}
        assert len(body.tables) == len(other.tables)
        for (a, stride, count), (b, stride2, count2) in zip(body.tables, other.tables):
            assert stride == stride2 == 4 and count == count2
            old = struct.unpack("<" + "I" * count, images[0].read(a, count * 4))
            new = struct.unpack("<" + "I" * count, images[1].read(b, count * 4))
            assert all(address_map[x] == y for x, y in zip(old, new))
        calls = []
        for a, b in zip(before, after):
            if a.mnemonic == "call" and a.operands[0].type == 2:
                calls.append({"update": a.operands[0].imm, "base": b.operands[0].imm})
            for left, right in zip(a.operands, b.operands):
                pointer = a.address in POINTER_SITES and left.type == right.type == 2
                absolute = (left.type == right.type == 3 and
                            left.mem.base == left.mem.index == right.mem.base == right.mem.index == 0)
                if left.type == right.type == 2 and not (a.mnemonic.startswith("j") or a.mnemonic == "call"):
                    # A changed literal must actually be an address in both images.
                    assert left.imm == right.imm or all(
                        any(s.va <= operand.imm < s.va + s.vsize for s in image.sections)
                        for operand, image in zip((left, right), images))
                if not (pointer or absolute):
                    continue
                assert left.size == right.size
                update_va = left.imm if pointer else left.mem.disp
                base_va = right.imm if pointer else right.mem.disp
                width = 4 if pointer else left.size
                uses.append(dict(update_va=update_va, base_va=base_va,
                                 update_site=a.address, base_site=b.address,
                                 update_hex=images[0].read(update_va, width).hex(),
                                 base_hex=images[1].read(base_va, width).hex(),
                                 kind="immutable_pointer" if pointer else "absolute_memory"))
        for label, image, sequence in zip(("update", "base"), images, (before, after)):
            for ins in sequence:
                assert image.read(ins.address, ins.size) == ins.bytes
            save(args.output / "instructions" / f"{entry:08x}-{label}.asm",
                 "".join(f"{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n"
                         for i in sequence).encode())
        owners.append(dict(update=entry, base=target, instructions=len(before), calls=calls))
        print(f"{entry:08x} -> {target:08x}: {len(before)} complete instructions", flush=True)
    mapping = {row["update"]: row["base"] for row in owners}
    for owner in owners:
        for call in owner["calls"]:
            call["paired_here"] = call["update"] in mapping
            if call["paired_here"]:
                assert mapping[call["update"]] == call["base"]
    fields, fixture = [], [f"# Base executable SHA256 {BASE_SHA}"]
    for group, definitions in (("D", DRIVER), ("C", CREATE)):
        words = []
        for name, address in definitions:
            selected = [row for row in uses if row["update_va"] == address]
            assert selected and len({row["base_va"] for row in selected}) == 1, name
            assert all(row["update_hex"] == row["base_hex"] and len(row["base_hex"]) == 8
                       for row in selected), name
            words.append(f"{int.from_bytes(bytes.fromhex(selected[0]['base_hex']), 'little'):08x}")
            fields.append(dict(group=group, name=name, uses=selected))
        fixture.append(group + "\t" + "\t".join(words))
    save(args.output / "constants.tsv", ("\n".join(fixture) + "\n").encode())
    save(args.output / "report.json", encoded(dict(
        scope=__doc__, tool_sha256=sha(__file__), owners=owners, fields=fields,
        images={str(path): sha(path) for path in paths},
        inventories={str(path): sha(path) for path in inventories},
        instruction_patches=[], intercepted_callees=[], mutable_state_imported=False)))


if __name__ == "__main__":
    main()
