#!/usr/bin/env python3
"""Verify base-disc player animation operands against original TU1.1 consumers.

Opcode prefixes locate candidates; complete normalized instruction streams,
internal branches and jump tables must match. The biped motion owner changed
between releases, so only its actual three-axis deadzone block is paired.
This does not establish whole-game or external-helper equivalence.
"""
import argparse
import csv
import json
import struct
from pathlib import Path

from oracle_mp_title_update_display import ROOT, Xbe, _Code, _trace, sha, save, encoded
from research_mp_live_locomotion_owners_u5 import raw_prefix_candidates
from research_mp_original_data_u5 import norm

BASE_SHA = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
UPDATE_SHA = "aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377"
OWNERS = (0xe1800, 0x1e1020, 0x1cfde0, 0x1d0a60, 0x1ca090,
          0x1ca2b0, 0x1cf100, 0x1ceda0, 0x1cefe0, 0x1d0ed0,
          0x1dcd10, 0x1f52f0, 0xe5960, 0xe5a80,
          0xeb000, 0xeb080, 0xeb700, 0xfcf90, 0xfeb40,
          0x114f70, 0x110bf0, 0x2205c0,
          0xfc2e0)  # Existing Needler adapter shares the animation one/RNG words.
FIELDS = {
    "animation_one": (0x465030, 4),
    "animation_zero": (0x465048, 4),
    "animation_frame_subtractor": (0x46504c, 4),
    "animation_rng_scale": (0x4650c0, 4),
    "animation_unsigned_correction": (0x465028, 4),
    "facing_epsilon": (0x46504c, 4),
    "facing_reversal_cosine": (0x465844, 4),
    "facing_cosine": (0x465584, 4),
    "melee_candidates": (0x446e34, 48),
    "melee_frame_seconds": (0x4650dc, 4),
    "melee_impact_scale": (0x465024, 4),
    "melee_moving_threshold": (0x4650f4, 4),
    "melee_special_seconds": (0x46507c, 4),
    "grenade_contact_up_threshold": (0x452000, 4),
    "grenade_rest_speed_squared": (0x465420, 4),
    "normal_blend": (0x44c6b4, 4),
    "special_blend": (0x465140, 4),
    "activity_pairs": (0x46fb24, 32),
}
POINTERS = {"basis_forward": 0x46fc48, "basis_up": 0x46fc50}


def tokens(rows, image):
    positions = {ins.address: n for n, ins in enumerate(rows)}
    return [norm(ins, image, positions) for ins in rows]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", type=Path, default=ROOT / "assets/disc/default.xbe")
    parser.add_argument("--update", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    paths = (args.update, args.base)
    for path, pin in zip(paths, (UPDATE_SHA, BASE_SHA)):
        assert sha(path) == pin, path
    images = [Xbe(path) for path in paths]
    codes = [_Code(image, ".text") for image in images]
    inventories = (ROOT / "analysis/map-pack-update/functions.tsv",
                   ROOT / "analysis/retail/functions.tsv")
    starts = [{int(row["address"], 16) for row in csv.DictReader(path.open(), delimiter="\t")}
              for path in inventories]
    owners, uses = [], []
    for entry in OWNERS:
        body = _trace(codes[0], entry, starts[0])
        before = [codes[0].at(address) for address in sorted(body.code)]
        candidates = raw_prefix_candidates(codes[0], images[0], codes[1], entry)
        matches = []
        for target in candidates:
            other = _trace(codes[1], target, starts[1])
            after = [codes[1].at(address) for address in sorted(other.code)]
            if tokens(before, images[0]) == tokens(after, images[1]):
                matches.append((target, other, after))
        assert len(matches) == 1, (hex(entry), [hex(row[0]) for row in matches])
        target, other, after = matches[0]
        address_map = {a.address: b.address for a, b in zip(before, after)}
        assert len(body.tables) == len(other.tables)
        for (a, stride, count), (b, stride2, count2) in zip(body.tables, other.tables):
            assert stride == stride2 == 4 and count == count2
            old = struct.unpack("<" + "I" * count, images[0].read(a, count * 4))
            new = struct.unpack("<" + "I" * count, images[1].read(b, count * 4))
            assert all(address_map[x] == y for x, y in zip(old, new))
        for a, b in zip(before, after):
            for left, right in zip(a.operands, b.operands):
                if left.type == right.type == 3:
                    old_va, base_va = left.mem.disp, right.mem.disp
                elif left.type == right.type == 2:
                    old_va, base_va = left.imm, right.imm
                else:
                    continue
                if old_va in {p for p, _ in FIELDS.values()} | set(POINTERS.values()):
                    uses.append(dict(update_va=old_va, base_va=base_va,
                                     update_site=a.address, base_site=b.address,
                                     owner=entry))
        for label, rows, image in zip(("update", "base"), (before, after), images):
            for ins in rows:
                assert image.read(ins.address, ins.size) == ins.bytes
            save(args.output / "instructions" / f"{entry:08x}-{label}.asm",
                 "".join(f"{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n"
                         for i in rows).encode())
        owners.append(dict(update_entry=entry, base_entry=target,
                           instructions=len(before), complete_body_and_internal_branches_equal=True))
    fields = {}
    for name, (update_va, size) in FIELDS.items():
        consumers = [use for use in uses if use["update_va"] == update_va]
        addresses = {use["base_va"] for use in consumers}
        assert len(addresses) == 1, (name, addresses)
        base_va = addresses.pop()
        original, base = images[0].read(update_va, size), images[1].read(base_va, size)
        assert original == base, name
        fields[name] = dict(update_va=update_va, base_va=base_va,
                            bytes=size, raw_hex=base.hex(), consumers=consumers)
    for name, update_va in POINTERS.items():
        consumers = [use for use in uses if use["update_va"] == update_va]
        addresses = {use["base_va"] for use in consumers}
        assert len(addresses) == 1, (name, addresses)
        base_va = addresses.pop()
        value_addresses = [int.from_bytes(image.read(p, 4), "little")
                           for image, p in zip(images, (update_va, base_va))]
        values = [image.read(p, 12) for image, p in zip(images, value_addresses)]
        assert values[0] == values[1], name
        fields[name] = dict(update_va=update_va, base_va=base_va,
                            update_value_va=value_addresses[0], base_value_va=value_addresses[1],
                            bytes=12, raw_hex=values[1].hex(), consumers=consumers)
    # This owner has release-specific changes outside the deadzone. Pair only
    # the actual input loads, squared sum, original operand and COMISS/JBE.
    blocks = []
    for code, image, entry, operand in zip(codes, images, (0xde960, 0xdd360), (0x4654c0, 0x45e02c)):
        rows = [code.at(p) for p in sorted(_trace(code, entry, {entry}).code)]
        sites = [n for n, ins in enumerate(rows) if any(
            o.type == 3 and o.mem.disp == operand for o in ins.operands)]
        assert len(sites) == 1
        n = sites[0]
        block = rows[n - 9:n + 14]
        assert [i.mnemonic for i in block[:14]] == [
            "movss", "movss", "movss", "movaps", "mulss", "movaps", "mulss",
            "movaps", "mulss", "movss", "addss", "addss", "comiss", "jbe"]
        blocks.append(block)
        save(args.output / "instructions" / f"{entry:08x}-deadzone.asm",
             "".join(f"{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n"
                     for i in block).encode())
    assert tokens(blocks[0], images[0]) == tokens(blocks[1], images[1])
    assert images[0].read(0x4654c0, 4) == images[1].read(0x45e02c, 4)
    fields["locomotion_deadzone"] = dict(update_va=0x4654c0, base_va=0x45e02c,
        raw_hex=images[1].read(0x45e02c, 4).hex(), update_site=blocks[0][9].address,
        base_site=blocks[1][9].address, scope="Paired deadzone block only; complete motion owners differ")
    report = dict(schema=1, tool_sha256=sha(__file__),
        source_pins={str(p): sha(p) for p in paths}, owners=owners, fields=fields,
        boundary="Static original operand/consumer correspondence. External helpers, full Xbox gameplay and framebuffer parity remain separate.")
    save(args.output / "report.json", encoded(report))
    print(json.dumps(dict(complete_owners=len(owners), fields=len(fields), output=str(args.output))))


if __name__ == "__main__":
    main()
