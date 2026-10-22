#!/usr/bin/env python3
"""Execute whole original object-decision owners from declared live inputs.

No original instructions, dependency returns or completed decisions are
patched. Full bodies and existing original character/RNG dependencies execute.
The HS handoff case stops before the original213D30 call, retaining a prefix
rather than supplying a script service. Raw component caller/pool/tag fields
and prior bytes are declared inputs, never a runtime campaign snapshot.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86 import X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP
from export_final_startup_storage_data_tu15 import regions

ENTRIES=(0x26efa0,0x26eef0,0x265620,0x2753c0)
NESTED=(0x1ecd00,0x25980)
BASE,STACK,STOP=0x2000000,0x200f000,0x3000000
AH,PH,OH,ACTORS,PROPS,OBJECTS,OBJECT,TAGS,TAG,CHAR,PARAM,RNG,OUTPUT=(BASE+p for p in
    (0x100,0x200,0x300,0x1000,0x4000,0x5000,0x6000,0x7000,0x8000,0x9000,0xa000,0xb000,0xc000))
ACTOR,PROP=ACTORS+2*0x888,PROPS+0x3c
ACTOR_ID,PROP_ID,OBJECT_ID=0xe3610002,0xf2810001,0xf2700005
GLOBALS=(0x5020c8,0x50eef0,0x4ecbd0,0x4f0414,0x4f3cd8)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');bodies={p:_trace(code,p,set(ENTRIES+NESTED)) for p in ENTRIES+NESTED}
    allowed=set().union(*(set(b.code) for b in bodies.values()));points=set()
    for pc in sorted(allowed):
        for op in code.at(pc).operands:
            if op.type!=X86_OP_MEM or op.mem.base or op.mem.index:continue
            section=next((s for s in image.sections if s.va<=op.mem.disp<op.mem.disp+op.size<=s.va+s.vsize),None)
            if section and section.name=='.rdata':
                assert op.access&CS_AC_READ and not op.access&CS_AC_WRITE
                points.update(range(op.mem.disp,op.mem.disp+op.size))
    specs=[]
    for bits,poison in itertools.product((0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc12345,0x7fa12345),(0,0xa5)):
        specs.append(dict(entry=0x2753c0,bits=bits,poison=poison))
    patterns=((),(0,),(7,),(8,),(0,7,16),(8,16,24),(0xfff8,))
    for flag,parts,pointer in itertools.product((0,4),patterns,(False,True)):
        specs.append(dict(entry=0x26eef0,flag=flag,parts=parts,pointer=pointer,poison=0xa5))
    for state,changed,notify,blocked,unit in itertools.product((0,1,2,3,4), (False,True), (0,1), (0,1),(False,True)):
        specs.append(dict(entry=0x265620,state=state,changed=changed,notify=notify,blocked=blocked,unit=unit,poison=0xa5))
    for vehicle,distance,state,same,parts in itertools.product((False,True),(0.0,0.5,14.0),(0,3,4,5),(False,True),(False,True)):
        specs.append(dict(entry=0x26efa0,vehicle=vehicle,distance=distance,state=state,same=same,parts=parts,poison=0xa5,alternate=False))
    # Reach the original velocity/tag-driven branch and both accepted and
    # rejected priority/distance states, without a supplied team service.
    for state,velocity,priority,distance in itertools.product((0,3,4),(0.0,2.0),(False,True),(0.5,14.0)):
        specs.append(dict(entry=0x26efa0,vehicle=True,distance=distance,state=state,same=False,parts=False,
            poison=0xa5,alternate=True,velocity=velocity,priority=priority))
    cases=[];covered=set();prefixes=0
    for spec in specs:
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,bytes([spec['poison']])*0x10000);g.mem_map(STOP,4096)
        patches=[]
        def put(at,value,size=4):
            raw=(value&((1<<(8*size))-1)).to_bytes(size,'little');g.mem_write(at,raw);patches.append([at,raw.hex()])
        def fp(at,value):put(at,struct.unpack('<I',struct.pack('<f',value))[0])
        for at,value in ((STACK,STOP),(0x5020c8,AH),(AH+0x44,ACTORS),(0x50eef0,PH),(PH+0x44,PROPS),
                (0x4ecbd0,OH),(OH+0x44,OBJECTS),(OBJECTS+5*12+8,OBJECT),(0x4f0414,TAGS),(0x4f3cd8,RNG),
                (PROP+0x20,OBJECT_ID),(OBJECT,0xabcd0003),(TAGS+3*16+8,TAG),(TAGS+4*16+8,CHAR),
                (CHAR+0x5c,1),(CHAR+0x60,PARAM),(ACTOR+0x54,0xabcd0004),(RNG,0x12345678),
                (ACTOR+0x18,0xffffffff),(ACTOR+0x26c,0xffffffff)):put(at,value)
        fp(PARAM+0xc,0.5);put(ACTOR+0x35e,0,1);put(ACTOR+0x35a,0,2)
        initial=[ACTOR_ID,PROP_ID,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
        if spec['entry']==0x2753c0:
            put(STACK+12,PARAM);put(PARAM+8,spec['bits'])
        elif spec['entry']==0x26eef0:
            initial[0]=OBJECT_ID;put(STACK+4,OUTPUT if spec['pointer'] else 0)
            put(OBJECT+0x10a,spec['flag'],1);put(OBJECT+0x120,len(spec['parts'])*8,2);put(OBJECT+0x122,0x180,2)
            for i,value in enumerate(spec['parts']):put(OBJECT+0x180+i*8+4,value,2)
        elif spec['entry']==0x265620:
            put(ACTOR+0x358,spec['state'],2);put(ACTOR+0x368,PROP_ID+int(spec['changed']));
            put(ACTOR+0x35e,spec['notify'],1);put(ACTOR+0x35a,spec['blocked'],2)
            if spec['unit']:put(ACTOR+0x18,0xabcd0000)
        else:
            put(STACK+4,ACTOR_ID);put(STACK+8,PROP_ID)
            put(ACTOR+0x358,spec['state'],2);put(ACTOR+0x360,OBJECT_ID if spec['same'] else 0xffffffff)
            fp(ACTOR+0x394,2.0);fp(PROP+0x28,spec['distance'])
            put(OBJECT+0x248,0 if spec['vehicle'] and not spec['alternate'] else 0xffffffff)
            put(OBJECT+0x10a,4 if spec['parts'] else 0,1);put(OBJECT+0x120,8,2);put(OBJECT+0x122,0x180,2);put(OBJECT+0x184,8,2)
            put(TAG+0x1ec,0x80 if spec['alternate'] else 0);fp(TAG+4,0.0)
            for i in range(3):fp(OBJECT+0x88+i*4,spec.get('velocity',0.0) if i==0 else 0.0)
            put(ACTOR+0x26c,0xffffffff if spec.get('priority',False) else OBJECT_ID)
            put(TAG+0x244,1,2)
        xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        trace=[];stores=[];prefix_end=None
        def instruction(_g,pc,size,_u):
            nonlocal prefix_end
            if pc==0x26568a:prefix_end=pc;g.emu_stop();return
            assert pc in allowed and bytes(g.mem_read(pc,size))==image.read(pc,size),hex(pc)
            # Unicorn reports REP at the same PC once per element, plus its
            # final exhausted check. The native owner observes one site.
            if not trace or trace[-1]!=pc:trace.append(pc)
            covered.add(pc)
        def store(_g,_kind,address,size,value,_u):
            assert BASE<=address<address+size<=BASE+0x10000,hex(address)
            stores.append([g.reg_read(R.UC_X86_REG_EIP),address,bytes(g.mem_read(address,size)).hex(),
                (value&((1<<(8*size))-1)).to_bytes(size,'little').hex()])
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
        g.emu_start(spec['entry'],STOP,count=10000);assert g.reg_read(R.UC_X86_REG_EIP)==(prefix_end or STOP)
        prefixes+=prefix_end is not None
        cases.append(dict(spec,prefix_end=prefix_end,patches=patches,gp=initial,xmm=xmm,trace=trace,stores=stores,
            final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
            x87_status=g.reg_read(R.UC_X86_REG_FPSW),
            outputs=[[ACTOR+0x358,bytes(g.mem_read(ACTOR+0x358,0x58)).hex()],
                [OUTPUT,bytes(g.mem_read(OUTPUT,4)).hex()],[RNG,bytes(g.mem_read(RNG,4)).hex()]]))
    literal_ranges=[[p,image.read(p,n).hex()] for p,n in regions(points)]
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=[[BASE,0x10000],*[[p,4] for p in GLOBALS]],immutable_operands=literal_ranges,cases=cases)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original object-decision reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(cases),whole_cases=len(cases)-prefixes,hs_call_prefixes=prefixes,
        entry_coverage={f'{p:08x}':dict(observed=len(set(bodies[p].code)&covered),static=len(bodies[p].code),
            unexecuted=[hex(pc) for pc in sorted(set(bodies[p].code)-covered)]) for p in ENTRIES+NESTED},
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        script_handoff_qualified=False,full_campaign_decisions_qualified=False,runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
