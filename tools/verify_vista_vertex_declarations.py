#!/usr/bin/env python3
"""Execute the owned Vista declaration builder unchanged; record its D3D ABI.

No Direct3D or game process is started. The fixture supplies one frontend
stream descriptor and calls the original bounded CPU-only builder at 66E828.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

SHA256 = "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109"
FORMATS = (1, 2, 3, 4, 5, 6, 7, 8, 9, 23, 25, 29, 46, 47)


def verify(path):
    executable = path.read_bytes()
    if hashlib.sha256(executable).hexdigest() != SHA256:
        raise ValueError("unsupported Vista executable fingerprint")
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x400000, max(0xB00000, (len(executable)+4095) & ~4095))
    guest.mem_write(0x400000, executable)
    guest.mem_map(0x3000000, 0x20000)
    obj, output, stack, stop = 0x3000000, 0x3001000, 0x3010000, 0x301F000
    instructions = {}

    def checked_code(_guest, address, size, _user):
        if not (0x66E828 <= address < 0x66E9A4 or
                0x666023 <= address < 0x666100 or
                0x67C671 <= address < 0x67C6B4):
            raise ValueError(f"declaration execution escaped its source routines: {address:08x}")
        binary = bytes(guest.mem_read(address, size))
        if binary != executable[address-0x400000:address-0x400000+size]:
            raise ValueError("original declaration instructions changed")
        instructions[address] = binary.hex()

    guest.hook_add(UC_HOOK_CODE, checked_code)
    rows = []
    for fmt in FORMATS:
        guest.mem_write(obj, bytes(0x200))
        guest.mem_write(output, bytes(256))
        guest.mem_write(obj+0xC, struct.pack("<I", 0x7E49F8+fmt*21))
        guest.mem_write(obj+0x2C, struct.pack("<I", 1))
        guest.mem_write(stack, struct.pack("<II", stop, output))
        guest.reg_write(UC_X86_REG_ECX, obj)
        guest.reg_write(UC_X86_REG_ESP, stack)
        guest.emu_start(0x66E828, stop, count=2000)
        if guest.reg_read(UC_X86_REG_EIP) != stop or guest.reg_read(UC_X86_REG_ESP) != stack+8:
            raise ValueError("original declaration builder did not return normally")
        count = int.from_bytes(guest.mem_read(output, 4), "little")
        if count > 16:
            raise ValueError("excessive original vertex declaration")
        elements = [dict(zip(("stream", "offset", "type", "method", "usage", "usage_index"),
            struct.unpack("<HH4B", guest.mem_read(output+4+i*8, 8)))) for i in range(count)]
        rows.append(dict(format=fmt, elements=elements))
    return dict(schema="H2_VISTA_VERTEX_DECLARATION_ORACLE_V1", executable_sha256=SHA256,
        entry="0066e828", formats=rows,
        executed_instructions={f"{at:08x}": instructions[at] for at in sorted(instructions)})


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = verify(args.executable)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+"\n")
    print(f"Verified {len(report['formats'])} declarations against unchanged Vista code")
