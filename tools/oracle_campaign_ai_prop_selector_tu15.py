#!/usr/bin/env python3
"""Qualify whole original272100 and its source-produced table selector.

Execute the actual actor/property lookup,26DA40 classifier, three-by-four
consumed WORD matrix, bounded switches and SSE result. Inputs are declared
component records, not captured live decisions. No instructions, call returns
or computed matrix selectors are supplied. Existing Frame code is unchanged.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP
from export_final_startup_storage_data_tu15 import regions

BASE,STACK,STOP=0x2000000,0x200f000,0x3000000
ACTOR_HEADER,ACTOR=BASE+0x1000,BASE+0x3000
REFERENCE_HEADER,REFERENCE,CONTEXT=BASE+0x2000,BASE+0x5000,BASE+0x7000
PROP_HEADER,PROP,OTHER_HEADER,OTHER=BASE+0x8000,BASE+0x9000,BASE+0xa000,BASE+0xb000
CLOCK=BASE+0xc000
ENTRIES={0x272100,0x1ed450,0x26da40,0x2673a0}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--data-output',type=Path,required=True);ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert args.data_output.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');bodies={p:_trace(code,p,ENTRIES) for p in ENTRIES}
    frame_path=Path(__file__).resolve().parents[1]/'src/campaign/ai/source_ai_frame_tu15_generated.cpp'
    frame=frame_path.read_text()
    for entry,body in bodies.items():
        source=frame.split(f'static void run_{entry:x}(WorldState &s) {{',1)[1].split('\n}',1)[0]
        assert {int(p,16) for p in re.findall(r'services\.instruction\(0x([0-9a-f]+)u?\)',source)}==set(body.code)
        assert not (set(body.calls)|set(body.tail_jumps))-ENTRIES
    assert len(bodies[0x272100].code)==61
    # 26DA40 itself produces exactly0,1,2; the four declared column controls
    # exercise every consumed cell. Do not annex the adjacent fourth row.
    expected={0x26da56:'6683b88600000003',0x26da60:'b802000000',0x26da68:'6683b88400000004',
        0x26da70:'0f9dc2',0x272161:'8d048a',0x272164:'0fbf0c45e0684500'}
    for pc,bits in expected.items():assert image.read(pc,len(bytes.fromhex(bits))).hex()==bits,(hex(pc),code.at(pc))
    matrix=image.read(0x4568e0,24)
    assert struct.unpack('<12h',matrix)==(0,0,1,3,0,1,2,3,0,2,3,4)
    points=set(range(0x4568e0,0x4568f8))|set(range(0x46a0d0,0x46a0d4))|set(range(0x47d636,0x47d638))
    for entry,body in bodies.items():
        for address,width,count in body.tables:
            assert width==4 and all(int.from_bytes(image.read(address+i,4),'little') in body.code for i in range(0,width*count,4))
            points.update(range(address,address+width*count))
    operands=[[p,image.read(p,n).hex()] for p,n in regions(points)]
    args.data_output.write_text('// Original272100 consumed WORD matrix; adjacent fourth row remains unowned.\n'+
        'literal(memory,0x4568e0u,std::array<std::uint8_t,24>{'+','.join(f'0x{b:02x}' for b in matrix)+'});\n')
    sites=set().union(*(set(b.code) for b in bodies.values()));covered=set();cells=set();cases=[]
    controls=list(itertools.product(range(-2,8),range(3),range(4),(0,0xbf800000,0x3e800000,0x7fc12345),(False,True)))
    controls.extend((0,0,kind,0,False,True) for kind in range(4))
    for control in controls:
        kind,classification,column,response,has_target=control[:5];null_context=len(control)==6
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,b'\xa5'*0x10000);g.mem_map(STOP,4096)
        patches=[]
        def raw(address,data):
            patches.append([address,data.hex()]);g.mem_write(address,data)
        def u32(address,value):raw(address,struct.pack('<I',value))
        def i16(address,value):raw(address,struct.pack('<h',value))
        for cell,value in ((0x5020c8,ACTOR_HEADER),(0x4f0414,REFERENCE_HEADER),(0x50eef0,PROP_HEADER),
            (0x50eef4,OTHER_HEADER),(0x51df54,CLOCK)):
            u32(cell,value)
        for header,data in ((ACTOR_HEADER,ACTOR),(PROP_HEADER,PROP),(OTHER_HEADER,OTHER)):
            u32(header+0x44,data)
        u32(ACTOR+0x54,0xffffffff if null_context else 0xd2340000)
        u32(ACTOR+0x14,0xffffffff);i16(ACTOR+0x84,4 if classification==1 else 0)
        i16(ACTOR+0x86,3 if classification==2 else 0)
        u32(REFERENCE_HEADER+8,REFERENCE);u32(REFERENCE+0x4c,1);u32(REFERENCE+0x50,CONTEXT)
        for offset in (0x24,0x28,0x2c):u32(CONTEXT+offset,response)
        i16(PROP+0x18,kind);u32(PROP+8,0xabcd0000);u32(PROP+0x14,0xffffffff)
        u32(OTHER+0x58+0x3c,0 if has_target else 0xffffffff);u32(CLOCK+4,0x3f000000)
        raw(STACK,struct.pack('<2I',STOP,0xfedc0000|column))
        initial=[0xf1250000,0x22334455,0x33445566,0xe3610000,STACK,0x55667788,0x66778899,0x778899aa]
        xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        trace=[];stores=[]
        def instruction(_g,pc,size,_u):
            assert pc in sites and bytes(g.mem_read(pc,size))==image.read(pc,size),(hex(pc),control)
            trace.append(pc);covered.add(pc)
        def store(_g,_kind,address,size,value,_u):
            stores.append([g.reg_read(R.UC_X86_REG_EIP),address,bytes(g.mem_read(address,size)).hex(),
                (value&((1<<(8*size))-1)).to_bytes(size,'little').hex()])
        def read(_g,_kind,address,size,_value,_u):
            if 0x4568e0<=address<0x4568f8:assert size==2;cells.add((address-0x4568e0)//2)
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store);g.hook_add(UC_HOOK_MEM_READ,read)
        try:g.emu_start(0x272100,STOP,count=1000)
        except Exception as error:
            raise RuntimeError(f'Original input control={control}, pc={g.reg_read(R.UC_X86_REG_EIP):x}, '
                f'gp={[hex(g.reg_read(reg)) for reg in GP]}') from error
        assert g.reg_read(R.UC_X86_REG_EIP)==STOP
        cases.append(dict(entry=0x272100,prefix_end=None,kind=kind,classification=classification,column=column,
            response_bits=response,has_target=has_target,null_context=null_context,poison=0xa5,patches=patches,gp=initial,xmm=xmm,
            final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
            trace=trace,stores=stores,outputs=[[STACK-0x20,bytes(g.mem_read(STACK-0x20,0x28)).hex()]]))
    assert cells==set(range(12)) and set(bodies[0x272100].code)<=covered
    fixture=dict(scope=__doc__,executor='frame',original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=[[BASE,0x10000]]+[[p,4] for p in (0x5020c8,0x4f0414,0x50eef0,0x50eef4,0x51df54)],
        immutable_operands=operands,matrix_address=0x4568e0,matrix_bytes=24,cases=cases)
    data=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==data,'Preserve original selector reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(data)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(cases),whole_cases=len(cases),
        source_body_instructions=61,source_body_observed=61,matrix_cells=sorted(cells),new_immutable_bytes=24,
        original_coverage={f'{entry:08x}':dict(total=len(body.code),observed=len(set(body.code)&covered),
            unexecuted=sorted(set(body.code)-covered)) for entry,body in bodies.items()},
        fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),data_sha256=sha(args.data_output),
        tool_sha256=sha(Path(__file__)),existing_frame_sha256=sha(frame_path),existing_frame_changed=False,
        runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()
