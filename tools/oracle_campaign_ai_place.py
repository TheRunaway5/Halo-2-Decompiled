#!/usr/bin/env python3
"""Execute whole TU1.5 ai_place wrappers on controlled interpreter inputs.

Argument delivery, void-result publication and action request observation are
declared services. This fixture proves overload scheduling and the pending AI
reference word, not actor creation or campaign gameplay. No instruction patch.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from tag_match_memory_budget import enter_memory_budget

ap = argparse.ArgumentParser(description=__doc__)
ap.add_argument('--output-dir', type=Path, required=True)
ap.add_argument('--fixture', type=Path, required=True)
args = ap.parse_args()
status = enter_memory_budget(args.output_dir, 120)
if status is not None:
    raise SystemExit(status)
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX,
                              UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP)

path = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
pin = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
image = Xbe(path)
assert hashlib.sha256(image.data).hexdigest() == pin
g = Uc(UC_ARCH_X86, UC_MODE_32)
g.mem_map(0x10000, 0x600000)
for section in image.sections:
    g.mem_write(section.va, image.section_bytes(section))
STATE, ARGUMENT, STACK, STOP = 0x1000000, 0x1001000, 0x200f000, 0x2000000
g.mem_map(STATE, 0x10000)
g.mem_map(STOP, 0x10000)

def write(p, value):
    g.mem_write(p, struct.pack('<I', value & 0xffffffff))

def word(p):
    return struct.unpack('<I', g.mem_read(p, 4))[0]

def ret(value, cleanup=0):
    sp = g.reg_read(UC_X86_REG_ESP)
    g.reg_write(UC_X86_REG_EAX, value & 0xffffffff)
    g.reg_write(UC_X86_REG_ESP, sp + 4 + cleanup)
    g.reg_write(UC_X86_REG_EIP, word(sp))

calls, published = [], []
ready = True

def hook(cpu, pc, size, user):
    sp = g.reg_read(UC_X86_REG_ESP)
    if pc == 0x212010:
        assert word(sp + 4) == 0x12345678 and word(sp + 16) == 1
        assert word(sp + 8) == (2 if counted else 1)
        ret(ARGUMENT if ready else 0, 16)
    elif pc == 0x211da0:
        published.append([g.reg_read(UC_X86_REG_ECX), g.reg_read(UC_X86_REG_EAX)])
        ret(0)
    elif pc == 0x27d190:
        calls.append(dict(kind='reference', reference=g.reg_read(UC_X86_REG_EDI)))
        ret(0)
    elif pc == 0x2097e0:
        raw_count = g.reg_read(UC_X86_REG_EAX) & 0xffff
        calls.append(dict(kind='squad', squad=struct.unpack('<i', struct.pack('<I', word(sp + 4)))[0],
                          count=struct.unpack('<h', struct.pack('<H', raw_count))[0],
                          arguments=[word(sp + 8), word(sp + 12), word(sp + 16)]))
        ret(0, 16)

g.hook_add(UC_HOOK_CODE, hook)
write(0x5020a8, STATE)
references = [0xffffffff, 0, 1, 0x7fff, 0x8000, 0xffff, 0x12348000,
              0x3fffffff, 0x40000000, 0x40010002, 0x80000000,
              0x80010002, 0xc0000000, 0xfffffffe, 0x00010000, 0x3fff0001]
counts = [-32768, -1, 0, 1, 2, 255, 256, 32767]
cases = []
for reference in references:
    for count in counts:
        for counted in (False, True):
            ready = len(cases) % 11 != 0
            initial = (0xdead0000 + len(cases)) & 0xffffffff
            write(STATE + 0x364, initial)
            write(ARGUMENT, reference)
            write(ARGUMENT + 4, count)
            write(STACK, STOP)
            for i, value in enumerate((291 if counted else 290, 0x12345678, 1)):
                write(STACK + 4 + i * 4, value)
            g.reg_write(UC_X86_REG_ESP, STACK)
            g.reg_write(UC_X86_REG_ECX, 0xa5a50000)
            calls.clear()
            published.clear()
            g.emu_start(0x2aed30 if counted else 0x2aecf0, STOP, count=10000)
            assert g.reg_read(UC_X86_REG_EIP) == STOP
            assert g.reg_read(UC_X86_REG_ESP) == STACK + 16
            cases.append(dict(reference=reference, count=count if counted else None,
                              arguments_ready=ready, pending_before=initial,
                              pending_after=word(STATE + 0x364),
                              calls=list(calls), published=list(published)))
assert not args.fixture.exists()
args.fixture.parent.mkdir(parents=True, exist_ok=True)
args.fixture.write_text(json.dumps(dict(scope=__doc__, original_xbe_sha256=pin,
    original_instruction_patches=[], tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    declared_services=['HS argument delivery', 'HS void-result publication',
                       'action request observation without state mutation'], cases=cases), indent=2) + '\n')
print('PASS', len(cases), 'whole original ai_place overload cases')
