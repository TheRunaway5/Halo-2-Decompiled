#!/usr/bin/env python3
"""Inventory and disassemble the local May 2003 renderer without modifying it.

The linker MAP addresses are not XBE virtual addresses. Resolve section:offset
against the XBE section table, including the alignment gaps between sections.
Requires capstone (the repository's .tools/python environment supplies it).
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from disc import inspect_xbe

SYMBOL = re.compile(r"^\s+([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\s+(\S+)\s+([0-9a-fA-F]{8})\s+(.*?)\s+(\S+\.obj)\s*$")
OWNERS = {"rasterizer_xbox_stencilshadow_draw", "rasterizer_xbox_stencilshadow_compute",
          "rasterizer_xbox_stencilshadow_helpers", "rasterizer_xbox_models",
          "rasterizer_xbox_lights", "rasterizer_lights", "render_lights", "render_layers",
          "rasterizer_xbox_environment", "old_shaders"}
SOURCE_SHA = "5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32

    xbe = args.build / "game/halo2debug.xbe"
    symbols = args.build / "tools/bin/halo2debug.map"
    metadata = inspect_xbe(xbe)
    data = xbe.read_bytes()
    rows = []
    for line in symbols.read_text(errors="strict").splitlines():
        match = SYMBOL.match(line)
        if not match:
            continue
        segment, offset, name, linked, flags, owner = match.groups()
        segment, offset = int(segment, 16), int(offset, 16)
        if segment < 1 or segment > len(metadata["sections"]):
            raise ValueError(f"unmapped linker section: {line}")
        section = metadata["sections"][segment - 1]
        rows.append(dict(address=section["address"] + offset, offset=offset,
                         section=segment, linked_address=int(linked, 16), name=name,
                         function="f" in flags.split(), owner=owner.split(":")[-1][:-4]))
    # A duplicated public/private alias must not truncate a function to zero.
    starts = sorted({row["address"] for row in rows if row["function"]})
    names = {row["address"]: row["name"] for row in rows}
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    selected = []
    args.output.mkdir(parents=True, exist_ok=True)
    assembly = args.output / "disassembly"
    assembly.mkdir(exist_ok=True)
    for row in rows:
        if not row["function"] or not (row["owner"] in OWNERS or "stencil" in row["name"]):
            continue
        section = metadata["sections"][row["section"] - 1]
        end = next((address for address in starts if address > row["address"]),
                   section["address"] + section["file_size"])
        end = min(end, section["address"] + section["file_size"])
        offset = section["file_offset"] + row["offset"]
        code = data[offset:offset + end - row["address"]]
        row = dict(row, size=len(code), sha256=hashlib.sha256(code).hexdigest())
        lines = [f"; {row['name']} ({row['owner']})", f"; XBE {row['address']:08x}; linker {row['linked_address']:08x}"]
        for instruction in decoder.disasm(code, row["address"]):
            annotation = ""
            if instruction.mnemonic in ("call", "jmp") and instruction.op_str.startswith("0x"):
                annotation = " ; " + names.get(int(instruction.op_str, 16), "unknown")
            lines.append(f"{instruction.address:08x} {instruction.mnemonic:10} {instruction.op_str}{annotation}")
        (assembly / f"{row['address']:08x}.asm").write_text("\n".join(lines) + "\n")
        selected.append(row)
    report = dict(xbe=metadata, symbol_map=str(symbols),
                  symbol_map_sha256=hashlib.sha256(symbols.read_bytes()).hexdigest(),
                  map_timestamp=next(line.strip() for line in symbols.read_text().splitlines() if "Timestamp" in line),
                  functions=selected)
    (args.output / "build.json").write_text(json.dumps(report, indent=2) + "\n")
    (args.output / "seeds.tsv").write_text("".join(
        f"{row['address']:08x}\t{re.sub('[^a-zA-Z0-9_]', '_', row['name'].split('@@')[0].lstrip('?'))}\n"
        for row in selected))
    # The original caps/stitches callers select these ten literal programs.
    # Their table ABI/count is established for this executable only. An
    # inspection of a different build must not reuse those offsets by guess.
    programs = []
    if metadata["sha256"] == SOURCE_SHA:
        def read(address, length):
            for section in metadata["sections"]:
                relative = address - section["address"]
                if 0 <= relative and relative + length <= section["file_size"]:
                    offset = section["file_offset"] + relative
                    return data[offset:offset + length]
            raise ValueError(f"unbacked XBE range: {address:x}+{length:x}")

        table = next(row["address"] for row in rows if row["name"].startswith("?vertex_shader_table@@"))
        program_dir = args.output / "vertex-programs"
        program_dir.mkdir(exist_ok=True)
        for index in range(0x89, 0x93):
            _, address, _, length, _ = struct.unpack("<5I", read(table + index * 20, 20))
            code = read(address, length)
            version, count = struct.unpack_from("<HH", code)
            if version != 0x2078 or not 0 < count <= 136 or length != 4 + count * 16:
                raise ValueError(f"invalid original vertex program {index:x}")
            for ordinal in range(count):
                final = struct.unpack_from("<I", code, 4 + ordinal * 16 + 12)[0] & 1
                if final != (ordinal == count - 1):
                    raise ValueError(f"invalid final marker in vertex program {index:x}")
            filename = f"{index:02x}.bin"
            (program_dir / filename).write_bytes(code)
            programs.append(dict(index=index, address=address, bytes=length, instructions=count,
                                 sha256=hashlib.sha256(code).hexdigest(), file=filename))
        (program_dir / "programs.json").write_text(json.dumps(dict(
            schema="halo2-pstencil-2003-shadow-programs-v1", source_sha256=metadata["sha256"],
            table_address=table, programs=programs), indent=2) + "\n")
    print(f"{len(selected)} renderer functions, {len(programs)} shadow vertex programs; "
          f"{metadata['sha256']}; {args.output}")


if __name__ == "__main__":
    main()
