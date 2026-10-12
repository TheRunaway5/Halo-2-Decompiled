#!/usr/bin/env python3
"""Research fixture: execute each extracted LVL record's original XBE validator.

This does not reinterpret the record or run game startup. Both checksum routines
and their CRC callees execute unchanged; only file bytes and caller state are
supplied. Also require rejection of a changed payload and an incorrect version.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/reference/halo2-decompiled/tools"))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP

ARENA, STACK, STOP = 0x1000000, 0x200ff00, 0x3000000
PINS = {"default.xbe": "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d",
        "default.xbe.03": "aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377"}


def machine(path):
    x = Xbe(path)
    pin = hashlib.sha256(x.data).hexdigest()
    assert pin == PINS[path.name], "oracle requires the identified original executable"
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x800000)
    for section in x.sections:
        g.mem_write(section.va, x.section_bytes(section))
    g.mem_map(ARENA, 0x40000)
    g.mem_map(0x2000000, 0x10000)
    g.mem_map(STOP, 0x1000)
    return g, pin


def validate(g, address, record):
    g.mem_write(ARENA, bytes(record))
    g.mem_write(STACK, struct.pack("<I", STOP))
    g.reg_write(UC_X86_REG_ESP, STACK)
    g.reg_write(UC_X86_REG_ESI, ARENA)
    g.reg_write(UC_X86_REG_EAX, 0)
    g.emu_start(address, STOP, count=20_000_000)
    assert g.reg_read(UC_X86_REG_EIP) == STOP
    assert bytes(g.mem_read(ARENA, len(record))) == bytes(record), "validator must restore CRC field"
    return bool(g.reg_read(UC_X86_REG_EAX) & 255)


machines = {2: machine(ROOT / "assets/disc/default.xbe"),
            3: machine(ROOT / "assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")}
addresses = {2: 0x19bdc0, 3: 0x19f270}
rows = []
for path in sorted((ROOT / "assets/multiplayer-map-pack/demos/dlc").glob("*/*.lvl")):
    raw = path.read_bytes()
    version = struct.unpack_from("<I", raw)[0]
    count = 1 if version == 2 else struct.unpack_from("<I", raw, 4)[0]
    start, size = (0, 0x2d3fc) if version == 2 else (8, 0x2d400)
    assert len(raw) == start + count * size
    for index in range(count):
        record = bytearray(raw[start + index * size:start + (index + 1) * size])
        g, pin = machines[version]
        assert validate(g, addresses[version], record)
        cleared = bytearray(record)
        struct.pack_into("<I", cleared, 4, 0)
        crc32_complement = zlib.crc32(cleared) ^ 0xffffffff
        assert crc32_complement == struct.unpack_from("<I", record, 4)[0]
        corrupt = bytearray(record)
        corrupt[-1] ^= 1
        assert not validate(g, addresses[version], corrupt)
        wrong_version = bytearray(record)
        struct.pack_into("<I", wrong_version, 0, version + 1)
        assert not validate(g, addresses[version], wrong_version)
        rows.append(dict(file=str(path.relative_to(ROOT)), index=index, offset=start + index * size,
                         version=version, size=size, checksum=f"{struct.unpack_from('<I', record, 4)[0]:08x}",
                         validator=f"{addresses[version]:08x}", xbe_sha256=pin,
                         crc32_complement=f"{crc32_complement:08x}",
                         checksum_accepted=True, modified_payload_rejected=True,
                         wrong_version_rejected=True, input_restored=True))
print(json.dumps(dict(passed=True, cases=len(rows) * 3, records=rows,
    checksum_formula="zlib.crc32(record with little-endian checksum at +4 cleared to zero) XOR 0xffffffff; original routine starts accumulator 0xffffffff and leaves it uncomplemented",
    scope="Original record validator and original CRC instructions only; not file I/O, catalogue registration, update installation or gameplay."), indent=2))
