#!/usr/bin/env python3
"""Execute retail object prediction and its resource traversal on authored tags.

HS argument evaluation and return storage are supplied boundaries. Bitmap and
geometry cache services record requests and supply readiness; the evaluator,
dispatcher and model section traversal run unchanged. This proves dispatch,
not Xbox allocation, streaming, rendering or live campaign execution.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / "tools"),
               str(ROOT / "tools/reference/halo2-decompiled/tools")]
from audit_rendering import Cache
from xbe import Xbe
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EIP,
                              UC_X86_REG_EAX, UC_X86_REG_ESI,
                              UC_X86_REG_ECX, UC_X86_REG_FPCW)

IMAGE = ROOT / "assets/disc/default.xbe"
PIN = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
assert hashlib.sha256(IMAGE.read_bytes()).hexdigest() == PIN
xbe = Xbe(IMAGE)
import argparse
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--output", type=Path,
                    default=ROOT / "analysis/tags/object-prediction-original.json")
args = parser.parse_args()
manifest = json.loads((ROOT / "tests/fixtures/object_prediction_assets.json").read_text())
words = iter((ROOT / "tests/fixtures/object_prediction_resources.tsv").read_text().split())
maps = {}
while True:
    name = next(words, None)
    if name is None:
        break
    datum, count = next(words), int(next(words))
    resources = [dict(type=int(next(words)), index=int(next(words)), datum=next(words))
                 for _ in range(count)]
    maps.setdefault(name, []).append(dict(node=None, datum=datum, resources=resources))
requests = dict(maps=[dict(map=name, calls=calls) for name, calls in maps.items()])
STACK, STOP, TABLE, ARGUMENT = 0x200ff00, 0x3000000, 0x1000000, 0x1040000
rows = []
for map_row in requests["maps"]:
    cache_path = ROOT / "assets/disc/maps" / map_row["map"]
    assert hashlib.sha256(cache_path.read_bytes()).hexdigest() == manifest["maps"][map_row["map"]]
    cache = Cache(cache_path)
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    for section in xbe.sections:
        guest.mem_write(section.va, xbe.section_bytes(section))
    guest.mem_map(TABLE, 0x100000)
    guest.mem_map(0x2000000, 0x10000)
    guest.mem_map(STOP, 0x1000)
    first = min(t["address"] for t in cache.tags.values() if t["address"]) & ~4095
    last = (max(t["address"] + t["size"] for t in cache.tags.values()
                if t["address"]) + 4095) & ~4095
    guest.mem_map(first, last - first)
    for tag in cache.tags.values():
        if not tag["address"]:
            continue
        offset = cache.pointer(tag["address"], cache.magic, tag["size"])
        guest.mem_write(tag["address"], bytes(cache.data[offset:offset + tag["size"]]))
        entry = TABLE + (tag["datum"] & 65535) * 16
        guest.mem_write(entry, struct.pack("<4I", 0, tag["datum"], tag["address"], tag["size"]))
    def read(address):
        return struct.unpack("<I", guest.mem_read(address, 4))[0]
    def write(address, value):
        guest.mem_write(address, struct.pack("<I", value & 0xffffffff))
    def supplied_return(pop=0, value=None):
        sp = guest.reg_read(UC_X86_REG_ESP)
        guest.reg_write(UC_X86_REG_EIP, read(sp))
        guest.reg_write(UC_X86_REG_ESP, sp + 4 + pop)
        if value is not None:
            guest.reg_write(UC_X86_REG_EAX, value & 0xffffffff)
    actual = []
    stored = []
    readiness = True
    def hook(_guest, address, _size, _context):
        if address == 0x209d50:
            supplied_return(16, ARGUMENT)
        elif address == 0x209ae0:
            stored.append(guest.reg_read(UC_X86_REG_EAX))
            supplied_return()
        elif address in (0x3bcb0, 0x12cc10):
            actual.append(["bitmap", guest.reg_read(UC_X86_REG_EAX)])
            supplied_return()
        elif address == 0x12dcb0:
            actual.append(["geometry", guest.reg_read(UC_X86_REG_ESI)])
            supplied_return(value=int(readiness))
    guest.hook_add(UC_HOOK_CODE, hook)
    write(0x4e3b44, TABLE)
    for call in [dict(node=None, datum="ffffffff", resources=[])] + map_row["calls"]:
        datum = int(call["datum"], 16)
        expected = []
        for resource in call["resources"]:
            tag = cache.tags[int(resource["datum"], 16)]
            offset = cache.pointer(tag["address"], cache.magic, tag["size"])
            if resource["type"] == 0:
                expected.append(["bitmap", cache.u32(offset + 0x48) + resource["index"] * 0x74])
            elif resource["type"] == 2:
                count, sections = cache.u32(offset + 0x24), cache.u32(offset + 0x28)
                expected.extend(["geometry", sections + index * 0x5c + 0x38]
                                for index in range(count))
            else:
                raise ValueError("This authored fixture requires another supplied cache boundary")
        for ready in (False, True):
            actual.clear()
            stored.clear()
            readiness = ready
            write(ARGUMENT, datum)
            for index, value in enumerate((STOP, 0x68, 0x12340000, 1)):
                write(STACK + index * 4, value)
            guest.reg_write(UC_X86_REG_ESP, STACK)
            guest.reg_write(UC_X86_REG_FPCW, 0x27f)
            guest.emu_start(0x2a2400, STOP, count=2000000)
            assert guest.reg_read(UC_X86_REG_EIP) == STOP
            assert actual == expected, (map_row["map"], call["datum"], actual, expected)
            assert stored == [0], stored
            rows.append(dict(map=map_row["map"], node=call["node"], datum=call["datum"],
                             supplied_geometry_ready=ready, requests=list(actual),
                             script_result=stored[0], passed=True))
    cache.close()

disassembly = []
disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
for start, size in ((0x2a2400, 0x62), (0x16e5e0, 0x167), (0x16e950, 0x13a)):
    section = next(s for s in xbe.sections if s.va <= start < s.va + s.rsize)
    offset = section.raw + start - section.va
    instructions = [f"{i.address:08x} {i.mnemonic} {i.op_str}"
                    for i in disassembler.disasm(xbe.data[offset:offset + size], start)]
    disassembly.append(dict(entry=f"{start:08x}", instructions=instructions))
report = dict(method=__doc__, xbe_sha256=PIN, passed=True,
              cases=len(rows), rows=rows, disassembly=disassembly)
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(report, indent=2) + "\n")
print(f"{len(rows)} unchanged retail evaluator/dispatcher cases passed")
