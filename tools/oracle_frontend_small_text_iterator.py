#!/usr/bin/env python3
"""Trace unchanged iterator/kerning registers for actual Settings small text."""
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.dont_write_bytecode = True
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,
                               UC_X86_REG_EDI,UC_X86_REG_EBX,UC_X86_REG_ESP)

source=ROOT/"analysis/frontend/settings-text-source-audit/glyph-layout-original/original-layout.json"
raw=source.read_bytes()
layout=json.loads(raw)
out=ROOT/"analysis/frontend/settings-text-source-audit/glyph-layout-original/iterator-trace.json"
assert not out.exists()
original=Retail()
guest=original.g
font_path=ROOT/"assets/disc/maps/fonts/conduit-12"
font=font_path.read_bytes()
guest.mem_write(0x4e2920,font[0x200:0x3c0])
guest.mem_write(0x4e2920+0x1c8,b"\1")
original.w(0x4e28f4+4,0)
headers=layout["glyph_headers"]
chars=[g["codepoint"] for g in headers]
pool=original.alloc(bytes(0x48))
records=original.alloc(bytes(0x38*len(chars)))
original.w(pool+0x44,records)
original.w(0x54d574,pool)
for index,glyph in enumerate(headers):
    original.w(records+index*0x38+0x10,4)
    guest.mem_write(records+index*0x38+0x1c,bytes.fromhex(glyph["header"]))
phase=""
line=0
trace=[]


def hook(machine,address,_size,_context):
    global phase,line
    if address==0x13ef40:
        phase="measure"
    elif address==0x13f700:
        phase="draw"
        line+=1
    elif address==0x140b20:
        c=machine.reg_read(UC_X86_REG_ECX)
        original.ret(chars.index(c) if c in chars else 0xffffffff)
    elif address==0x140870:
        stack=machine.reg_read(UC_X86_REG_ESP)
        original.ret(1)
        machine.reg_write(UC_X86_REG_ESP,stack+8)
    elif address==0x122570:
        stack=machine.reg_read(UC_X86_REG_ESP)
        trace.append(dict(phase=phase,line=line,
            previous=machine.reg_read(UC_X86_REG_EDI),
            current=machine.reg_read(UC_X86_REG_EBX),
            caller=f"{original.r(stack):08x}"))
    elif address==0x3000200:
        stack=machine.reg_read(UC_X86_REG_ESP)
        original.ret(0)
        machine.reg_write(UC_X86_REG_ESP,stack+60)


guest.hook_add(UC_HOOK_CODE,hook)
cases=[]
for case in layout["cases"]:
    guest.mem_write(0x4e73a0,bytes(0x70))
    original.w(0x4e73a0,1)
    original.w(0x4e73a0+4,1)
    original.w(0x4e73a0+12,0)
    original.run(0x13ec70,registers=[(UC_X86_REG_EAX,
        original.alloc(struct.pack("<4f",1,.247059,.392157,.537255)))])
    original.run(0x13ed50,registers=[(UC_X86_REG_EAX,0)])
    box=original.alloc(struct.pack("<4h",*case["projected_bounds"]))
    endpoint=original.alloc(bytes(4))
    value=case["text"]
    text=original.alloc(struct.pack(f"<{len(value)+1}I",*map(ord,value),0))
    trace.clear()
    phase=""
    line=0
    original.run(0x13f0e0,argv=[0x3000200,box,endpoint,0,0,
        int(case["scale_word"],16),text])
    cases.append(dict(index=case["index"],text=value,kerning_calls=list(trace)))
last=cases[-1]
first=[t for t in last["kerning_calls"] if t["phase"]=="draw" and t["line"]==1]
assert [(t["previous"],t["current"]) for t in first[:3]]==[(0,78),(0,97),(97,110)]
report=dict(executable_sha256=PIN,original_layout_sha256=hashlib.sha256(raw).hexdigest(),
    font_sha256=hashlib.sha256(font).hexdigest(),
    generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    patched_instructions=[],cases=cases,
    scope="Unchanged13F0E0/13EF40/13F700/13F5A0/122570. Actual seven Settings labels/font1/shared0.3 and explicit canonical bounds; ready font-cache services and final glyph callback supplied. Registers EDI(previous),EBX(current) at original122570 retained. No GPU, projection, or whole-frame claim.")
out.write_text(json.dumps(report,indent=2)+"\n")
print(out)
print(json.dumps(dict(Nanotech_first_draw_pairs=first[:3],cases=len(cases)),indent=2))
