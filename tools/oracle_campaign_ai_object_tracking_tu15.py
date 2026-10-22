#!/usr/bin/env python3
"""Authenticate AI object-tracking type inputs and original selector effects.

The nine authored rows have explicit IDs0..8 and sixteen-byte stride. Export
only the four WORD inputs at+2/+4/+6/+8 used by compiled consumers; record IDs
and unread six-byte tails remain separate. This does not add an engine bounds
rule or assert coverage for other caller indices. Whole267320 is compared for
all rows/statuses/flags.274020 stops after its actual type-field comparison,
before any object-pool or later constructor dependency is reached.
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
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86 import X86_OP_MEM,X86_REG_EDX
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP

BASE,HEADER,ROWS,INPUT,OUTPUT,STACK,STOP=0x2000000,0x2000100,0x2001000,0x2003000,0x2004000,0x200f000,0x3000000
FRAME_PIN='0faad001eff14edaaa31df28eddcde059ab855a5a3639fff4b81e3526ee15308'


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--data-output',type=Path,required=True);ap.add_argument('--frame-source',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert args.data_output.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    raw=args.frame_source.read_bytes();assert hashlib.sha256(raw).hexdigest()==FRAME_PIN
    code=_Code(image,'.text');direct=[]
    for pc in sorted({int(p,16) for p in re.findall(r'services\.instruction\(0x([0-9a-f]+)u?\)',raw.decode())}):
        ins=code.at(pc)
        if not ins:continue
        for op in ins.operands:
            if op.type==X86_OP_MEM and 0x47d630<=op.mem.disp<0x47d6c0:
                assert op.mem.disp in (0x47d632,0x47d634,0x47d636,0x47d638) and op.size==2
                assert op.access&CS_AC_READ and not op.access&CS_AC_WRITE
                direct.append(dict(pc=pc,field=op.mem.disp-0x47d630,instruction_hex=bytes(ins.bytes).hex()))
    assert len(direct)==12
    #267320 also forms a row pointer and consumes its+6/+8 fields.
    assert code.at(0x26734a).op_str=='edx, 0x47d630'
    for pc,field in ((0x267356,6),(0x267362,8)):
        op=code.at(pc).operands[0];assert op.type==X86_OP_MEM and op.mem.base==X86_REG_EDX and op.mem.disp==field and op.size==2
    rows=[];lines=['// Original AI object-tracking WORD inputs; row IDs and unread tails are excluded.']
    for index in range(9):
        address=0x47d630+index*16;raw=image.read(address,16)
        assert struct.unpack_from('<h',raw)[0]==index
        selected=raw[2:10];rows.append(dict(index=index,address=address+2,data_hex=selected.hex()))
        lines.append(f'literal(memory,0x{address+2:x}u,std::array<std::uint8_t,8>{{'+','.join(f'0x{v:02x}' for v in selected)+'});')
    args.data_output.write_text('\n'.join(lines)+'\n')
    cases=[];covered=set();bodies={entry:_trace(code,entry,{entry}) for entry in (0x274020,0x267320)}
    specs=[dict(entry=0x274020,index=i,poison=p,prefix_end=0x27404c) for i in range(9) for p in (0,0xa5)]
    specs += [dict(entry=0x267320,index=i,poison=0xa5,status=status,flag=flag,prefix_end=None)
        for i in range(9) for status in range(3) for flag in (0,1)]
    for spec in specs:
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,bytes([spec['poison']])*0x10000);g.mem_map(STOP,4096)
        patches=[]
        def put(at,value,size=4):
            raw=value.to_bytes(size,'little');g.mem_write(at,raw);patches.append([at,raw.hex()])
        put(STACK,STOP)
        initial=[0xe2760007,0x22334455,INPUT,0x44556677,STACK,0x55667788,0x66778899,OUTPUT]
        if spec['entry']==0x274020:
            initial[1]=0xabcd0000|spec['index'];put(STACK+4,0xf2700000)
        else:
            initial[1]=INPUT
            for at,value in ((0x50eef4,HEADER),(HEADER+0x44,ROWS),(INPUT+8,0xabcd0002)):put(at,value)
            put(INPUT+0x18,spec['index'],2);put(ROWS+2*0xc4+4,spec['status'],2);put(ROWS+2*0xc4+0x25,spec['flag'],1)
        xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        trace=[];stores=[]
        def instruction(_g,pc,size,_u):
            if spec['prefix_end'] is not None and pc==spec['prefix_end']:g.emu_stop();return
            assert pc in bodies[spec['entry']].code and bytes(g.mem_read(pc,size))==image.read(pc,size),hex(pc)
            trace.append(pc);covered.add(pc)
        def store(_g,_kind,address,size,value,_u):
            assert BASE<=address<address+size<=BASE+0x10000
            stores.append([g.reg_read(R.UC_X86_REG_EIP),address,bytes(g.mem_read(address,size)).hex(),
                (value&((1<<(8*size))-1)).to_bytes(size,'little').hex()])
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
        g.emu_start(spec['entry'],STOP,count=200)
        assert g.reg_read(R.UC_X86_REG_EIP)==(spec['prefix_end'] or STOP)
        outputs=[[OUTPUT,bytes(g.mem_read(OUTPUT,0xc4)).hex()]] if spec['entry']==0x274020 else []
        cases.append(dict(spec,patches=patches,gp=initial,xmm=xmm,trace=trace,stores=stores,outputs=outputs,
            final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)]))
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_bank=[BASE,0x10000],rows=rows,direct_consumers=direct,cases=cases)
    data=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==data,'Preserve original object-tracking evidence'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(data)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(cases),whole_predicate_cases=54,
        constructor_prefix_cases=18,immutable_bytes=72,rows=9,direct_consumers=direct,
        executed_predicate_instructions=len(covered&set(bodies[0x267320].code)),static_predicate_instructions=len(bodies[0x267320].code),
        executed_constructor_instructions=len(covered&set(bodies[0x274020].code)),static_constructor_instructions=len(bodies[0x274020].code),
        fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),data_sha256=hashlib.sha256(args.data_output.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),full_constructor_qualified=False,runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
