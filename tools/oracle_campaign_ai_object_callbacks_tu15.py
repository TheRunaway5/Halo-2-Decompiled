#!/usr/bin/env python3
"""Authenticate the complete original object-callback table consumed by269D70.

Eleven records supply a signed WORD type mask and DWORD callback; the following
null callback terminates the eight-byte-stride walk. Export only those70 read
bytes, excluding padding and the unused terminator mask. All byte type controls
stop before the first original callback CALL; no callback result is supplied.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP

BASE,STACK,OBJECT,OUTPUT,STOP=0x2000000,0x200f000,0x2001000,0x2002000,0x3000000


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--data-output',type=Path,required=True);ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert args.data_output.resolve().is_relative_to(scratch)
    im=Xbe(NEW_DEFAULT);assert hashlib.sha256(im.data).hexdigest()==NEW_PIN
    code=_Code(im,'.text');body=_trace(code,0x269d70,{0x269d70})
    expected={0x269d70:'a11c684500',0x269d8d:'0fbf4efc',0x269d9d:'ff16',0x269d9f:'8b4608',0x269da2:'83c608'}
    for pc,bits in expected.items():assert bytes(code.at(pc).bytes).hex()==bits
    frame=(Path(__file__).resolve().parents[1]/'src/campaign/ai/source_ai_frame_tu15_generated.cpp').read_text()
    entries={int(p,16) for p in re.findall(r'static void run_([0-9a-f]+)\(WorldState &s\)',frame)}
    rows=[];operands=[]
    for index in range(64):
        address=0x456818+index*8;mask,pad,target=struct.unpack('<HHI',im.read(address,8))
        if not target:operands.append([address+4,im.read(address+4,4).hex()]);break
        assert code.inside(target)
        rows.append(dict(address=address,mask=mask,target=target,compiled=target in entries))
        operands.extend(([address,im.read(address,2).hex()],[address+4,im.read(address+4,4).hex()]))
    assert len(rows)==11 and index==11
    lines=['// Original eleven object-callback masks/identities and null terminator; padding excluded.']
    for address,bits in operands:
        raw=bytes.fromhex(bits);lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{len(raw)}>{{'+
            ','.join(f'0x{b:02x}' for b in raw)+'});')
    args.data_output.write_text('\n'.join(lines)+'\n');cases=[];covered=set()
    for kind in (*range(32),32,63,127,255):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for section in im.sections:g.mem_write(section.va,im.section_bytes(section))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,b'\xa5'*0x10000);g.mem_map(STOP,4096)
        patches=[[STACK,struct.pack('<3I',STOP,OUTPUT,0xe3610000).hex()],[OBJECT,bytes([kind]).hex()]]
        for address,bits in patches:g.mem_write(address,bytes.fromhex(bits))
        initial=[0x11223344,0x22334455,0x33445566,OUTPUT,STACK,0x55667788,0x66778899,OBJECT]
        xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        trace=[];stores=[]
        def instruction(_g,pc,size,_u):
            if pc==0x269d9d:g.emu_stop();return
            assert pc in body.code and bytes(g.mem_read(pc,size))==im.read(pc,size)
            trace.append(pc);covered.add(pc)
        def store(_g,_kind,address,size,value,_u):
            stores.append([g.reg_read(R.UC_X86_REG_EIP),address,bytes(g.mem_read(address,size)).hex(),
                (value&((1<<(8*size))-1)).to_bytes(size,'little').hex()])
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
        g.emu_start(0x269d70,STOP,count=100);assert g.reg_read(R.UC_X86_REG_EIP)==0x269d9d
        cases.append(dict(entry=0x269d70,prefix_end=0x269d9d,kind=kind,poison=0xa5,gp=initial,xmm=xmm,patches=patches,
            final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
            trace=trace,stores=stores,outputs=[]))
    fixture=dict(scope=__doc__,executor='frame',original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=[[BASE,0x10000]],immutable_operands=operands,rows=rows,cases=cases)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original object callback reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,callback_rows=11,immutable_bytes=70,first_callback_prefix_cases=len(cases),
        observed_parent_instructions=len(covered),static_parent_instructions=len(body.code),
        uncompiled_callbacks=[r['target'] for r in rows if not r['compiled']],full_callbacks_qualified=False,
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),data_sha256=hashlib.sha256(args.data_output.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
