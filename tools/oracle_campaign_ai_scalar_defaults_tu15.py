#!/usr/bin/env python3
"""Execute complete original AI scalar _cinit callbacks, without result seeds.

Three callbacks in46DD00 order produce four geometry inputs before29BE40.
Raw operand bit patterns are declared transport controls; the actual engine
data profile contains only authored immutable operands and cold destinations.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from capstone.x86 import X86_OP_MEM, X86_REG_XMM0
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP

ENTRIES=(0x3826a0,0x3826c0,0x3826f0)
BASE,STACK,STOP=0x2000000,0x2008000,0x3000000


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--data-output',type=Path,required=True);ap.add_argument('--verify',action='store_true')
    args=ap.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.data_output.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');functions=[];operands={};covered=set();cases=[]
    assert struct.unpack('<3I',image.read(0x46dd00,12))==ENTRIES
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES));pcs=sorted(body.code)
        assert code.at(pcs[-1]).mnemonic=='ret' and len(pcs) in (3,5)
        copies=[]
        for load_pc,store_pc in zip(pcs[:-1:2],pcs[1:-1:2]):
            load,store=code.at(load_pc),code.at(store_pc)
            assert load.mnemonic==store.mnemonic=='movss'
            assert load.operands[0].reg==store.operands[1].reg==X86_REG_XMM0
            assert load.operands[1].type==store.operands[0].type==X86_OP_MEM
            source,destination=load.operands[1].mem.disp,store.operands[0].mem.disp
            assert not load.operands[1].mem.base and not store.operands[0].mem.base
            data=image.read(source,4);operands[source]=data
            section=next(s for s in image.sections if s.va<=destination<destination+4<=s.va+s.vsize)
            assert destination>=section.va+section.rsize and image.read(destination,4)==bytes(4)
            copies.append(dict(load=load_pc,store=store_pc,source=source,destination=destination,default_hex=data.hex()))
        functions.append(dict(entry=entry,ret=pcs[-1],copies=copies,instructions=[[p,bytes(code.at(p).bytes).hex()] for p in pcs]))
        for profile in range(6):
            for poison in (0,0xa5):
                g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
                for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
                g.mem_map(BASE,0x10000);g.mem_map(STOP,4096);g.mem_write(STACK,struct.pack('<I',STOP))
                declared=[]
                for copy in copies:
                    bits=bytes.fromhex(copy['default_hex']) if profile==0 else struct.pack('<I',
                        (0x80000000,0x7fa12345,0x7fc12345,0x7f800000,0xff800000)[profile-1])
                    g.mem_write(copy['source'],bits);g.mem_write(copy['destination'],bytes([poison])*4)
                    declared.append([copy['source'],bits.hex()])
                initial=[0x11223344,0x22334455,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
                xmm=[[0x3f000000+i*4+lane for lane in range(4)] for i in range(8)]
                flags=0x202 if poison==0 else 0xed7
                for reg,value in zip(GP,initial):g.reg_write(reg,value)
                for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
                g.reg_write(R.UC_X86_REG_EFLAGS,flags);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
                trace=[];stores=[]
                def instruction(_g,pc,size,_u):
                    assert pc in body.code and bytes(g.mem_read(pc,size))==image.read(pc,size)
                    trace.append(pc);covered.add(pc)
                def store(_g,_kind,address,size,value,_u):
                    stores.append([g.reg_read(R.UC_X86_REG_EIP),address,bytes(g.mem_read(address,size)).hex(),
                        (value&((1<<(size*8))-1)).to_bytes(size,'little').hex()])
                g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
                g.emu_start(entry,STOP,count=20);assert g.reg_read(R.UC_X86_REG_EIP)==STOP
                cases.append(dict(entry=entry,profile=profile,poison=poison,operands=declared,gp=initial,xmm=xmm,
                    flags=flags,final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
                    final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
                    trace=trace,stores=stores,outputs=[[copy['destination'],bytes(g.mem_read(copy['destination'],4)).hex()] for copy in copies]))
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        functions=functions,callback_table=0x46dd00,callback_table_hex=image.read(0x46dd00,12).hex(),
        stack=STACK,return_address=STOP,cases=cases)
    data=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==data,'Preserve original scalar-default evidence'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(data)
    lines=['// Authored TU1.5 AI scalar operands and _cinit identities; no completed scalar values.']
    for address,raw in [(0x46dd00,image.read(0x46dd00,12)),*sorted(operands.items())]:
        lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{len(raw)}>{{'+','.join(f'0x{b:02x}' for b in raw)+'});')
    output=('\n'.join(lines)+'\n').encode();args.data_output.write_bytes(output)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,functions=functions,cases=len(cases),
        whole_instructions=sum(len(f['instructions']) for f in functions),executed_instructions=len(covered),
        original_callback_order=ENTRIES,fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),
        data_sha256=hashlib.sha256(output).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        source_producer_required=True,runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()
