#!/usr/bin/env python3
"""Offline unchanged TU1.5 evidence for two native semantic AI leaves.

No instruction patches, service returns, supplied decisions, or emulator code
are part of the production engine. Run through tools/run_test.py. Inputs are
record layouts, datums, flags and actual caller state; the original produces
every branch, score, write, read order and ABI postimage.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

BASE, STACK, STOP = 0x2000000, 0x201f000, 0x3000000
AH, PH, QH = BASE+0x100, BASE+0x200, BASE+0x300
ACTORS, PROPS, QUERIES, RECORD = BASE+0x1000, BASE+0x6000, BASE+0x8000, BASE+0xa000
ACTOR_ID, PROP_ID, QUERY_ID = 0xe3630002, 0xe4710001, 0xf1280001
ACTOR, PROP, QUERY = ACTORS+2*0x888, PROPS+0x3c, QUERIES+0x124
GP = (R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX, R.UC_X86_REG_EBX,
      R.UC_X86_REG_ESP, R.UC_X86_REG_EBP, R.UC_X86_REG_ESI, R.UC_X86_REG_EDI)
BANKS = ((BASE,0xa0000),(0x5020c8,4),(0x50eef0,4),(0x50eeec,4),(0x47d6c0,4))


def execute(image, code, bodies, spec, covered, fail_read=None):
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    g.mem_map(0x10000,0x600000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(BASE,0xa0000);g.mem_map(STOP,4096)
    patches=[]
    def put(at,value,size=4):
        data=(value&((1<<(8*size))-1)).to_bytes(size,'little')
        g.mem_write(at,data);patches.append([at,data.hex()])
    actor_id=spec.get('actor_id',ACTOR_ID);prop_id=spec.get('prop_id',PROP_ID);query_id=spec.get('query_id',QUERY_ID)
    actor=ACTORS+(actor_id&65535)*0x888;prop=PROPS+(prop_id&65535)*0x3c;query=QUERIES+(query_id&65535)*0x124
    for at,value in ((0x5020c8,AH),(0x50eef0,PH),(0x50eeec,QH),(0x47d6c0,spec.get('default',0x81234567)),
                     (AH+0x44,ACTORS),(PH+0x44,PROPS),(QH+0x44,QUERIES),
                     (STACK,STOP),(STACK+4,actor_id),(STACK+8,RECORD),(STACK+12,spec.get('preserve',0)),
                     (actor+0x338,prop_id if spec.get('property',True) else 0xffffffff),
                     (prop+0x14,query_id if spec.get('query',True) else 0xffffffff)):
        put(at,value)
    put(prop+0x24,spec.get('state',3),2)
    put(query+0xd9,spec.get('blocked',0),1)
    put(RECORD+0x14,spec.get('record',0xa5a5a5a5))
    put(RECORD+0x17,spec.get('marked',0),1)
    if spec.get('wrap')=='zero':put(QH+0x44,-(query_id&65535)*0x124)
    if spec.get('wrap')=='offset_zero':put(QH+0x44,-(query_id&65535)*0x124-0x70)
    initial=[0x11223344,0x22334455,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
    xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
    for reg,value in zip(GP,initial):g.reg_write(reg,value)
    for i,value in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*value),'little'))
    g.reg_write(R.UC_X86_REG_EFLAGS,0x202)
    g.reg_write(R.UC_X86_REG_MXCSR,0x1f80);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
    g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
    trace=[];reads=[];stores=[];pending=[];flag_mask=0x8d5
    def flush():
        for pc,at,before in pending:stores.append([pc,at,before,bytes(g.mem_read(at,len(before)//2)).hex()])
        pending.clear()
    def instruction(_g,pc,size,_u):
        nonlocal flag_mask
        flush()
        assert pc in bodies[spec['entry']].code and bytes(g.mem_read(pc,size))==image.read(pc,size),hex(pc)
        trace.append(pc);covered.add(pc)
        mnemonic=code.at(pc).mnemonic
        # The hook precedes execution. A fault on a read operand leaves flags
        # from the previous completed instruction, recorded by read() below.
        if mnemonic in ('test','xor','and'):flag_mask=0x8c5
        elif mnemonic in ('cmp','add'):flag_mask=0x8d5
        elif mnemonic=='imul':flag_mask=0x801
    previous_mask=flag_mask
    current_pc=spec['entry']
    def step(_g,pc,size,_u):
        nonlocal previous_mask,current_pc
        previous_mask=flag_mask;current_pc=pc
        instruction(_g,pc,size,_u)
    stopped=False
    def read(_g,_kind,at,size,_value,_u):
        nonlocal stopped,flag_mask
        assert any(a<=at<at+size<=a+n for a,n in BANKS),(hex(at),size)
        reads.append([current_pc,at,size])
        if fail_read is not None and len(reads)-1==fail_read:
            stopped=True;flag_mask=previous_mask
            raise RuntimeError('Declared actual read boundary')
    def write(_g,_kind,at,size,_value,_u):
        assert any(a<=at<at+size<=a+n for a,n in BANKS),(hex(at),size)
        pending.append((current_pc,at,bytes(g.mem_read(at,size)).hex()))
    g.hook_add(UC_HOOK_CODE,step);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
    try:g.emu_start(spec['entry'],STOP,count=200)
    except RuntimeError:
        if not stopped:raise
    flush()
    if not stopped:assert g.reg_read(R.UC_X86_REG_EIP)==STOP
    return dict(name=json.dumps(spec,sort_keys=True)+(' fault='+str(fail_read) if fail_read is not None else ''),
                entry=spec['entry'],patches=patches,gp=initial,xmm=xmm,
                fail_read=fail_read,final_gp=[g.reg_read(r)&0xffffffff for r in GP],
                final_xmm=[[int.from_bytes(g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little')[j:j+4],'little')
                            for j in range(0,16,4)] for i in range(8)],
                final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),flags_mask=flag_mask,
                reads=reads,stores=stores,trace=trace,
                mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),fpcw=g.reg_read(R.UC_X86_REG_FPCW),
                fpstatus=g.reg_read(R.UC_X86_REG_FPSW),fptag=g.reg_read(R.UC_X86_REG_FPTAG))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');bodies={e:_trace(code,e,{0x1be5b0,0x1bae40}) for e in (0x1be5b0,0x1bae40)}
    assert not any(b.calls or b.tail_jumps for b in bodies.values())
    specs=[dict(entry=0x1be5b0,property=False),dict(entry=0x1be5b0,query=False)]
    specs += [dict(entry=0x1be5b0,state=state,blocked=blocked)
              for state,blocked in itertools.product((-32768,-1,0,2,3,4,32767),(0,1,128,255))]
    specs += [dict(entry=0x1be5b0,wrap=wrap) for wrap in ('zero','offset_zero')]
    specs += [dict(entry=0x1bae40,preserve=preserve,marked=marked,default=default)
              for preserve,marked,default in itertools.product((0,1,2,255),(0,0x7f,0x80,0xff),(0,0xffffffff,0x81234567))]
    specs += [dict(entry=0x1be5b0,actor_id=actor,prop_id=prop,query_id=query)
              for actor,prop,query in itertools.product((0,0xffff00ff),(0,0x81230007),(0,0xffff0007))]
    covered=set();cases=[execute(image,code,bodies,spec,covered) for spec in specs]
    # Every read boundary on representative complete/early exits. Missing
    # backing must preserve actual partial state, never a successful default.
    for spec in [specs[i] for i in (0,1,2,10,18,30,31,32,35,44,70)]:
        whole=execute(image,code,bodies,spec,covered)
        cases += [execute(image,code,bodies,spec,covered,index) for index,read in enumerate(whole['reads'])
                  if read[0]!=0x1be623] # Retired private ESI-save stack is outside the semantic API.
    sites=set().union(*(set(body.code) for body in bodies.values()))
    assert covered==sites,(len(covered),len(sites),sorted(sites-covered))
    fixture=dict(schema='native-campaign-ai-queries-original-v1',original_xbe_sha256=NEW_PIN,
        instruction_patches=[],supplied_dependency_returns=[],banks=BANKS,cases=cases,
        native_contract='public score/void result, callee-preserved state, argument consumption, ordered semantic record access/store, unchanged FP; excludes dead volatile registers/flags and retired private stack',
        body_sites={f'{e:08x}':sorted(b.code) for e,b in bodies.items()},
        body_sha256={f'{e:08x}':hashlib.sha256(b''.join(image.read(pc,code.at(pc).size) for pc in sorted(b.code))).hexdigest() for e,b in bodies.items()})
    data=(json.dumps(fixture,sort_keys=True,separators=(',',':'))+'\n').encode()
    if args.verify:assert args.fixture.read_bytes()==data
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(data)
    caller_contract={
        '1b0195':dict(continuation_start=0x1b0197,continuation_end=0x1b01cf,
            bytes=image.read(0x1b0197,0x1b01d2-0x1b0197).hex(),
            owed='AX score, preserved EBX/EBP/ESI/EDI and stack consumption; EDX overwritten, ECX dead until overwritten, EFLAGS overwritten before conditional use'),
        '1ea652':dict(continuation_start=0x1ea654,continuation_end=0x1ea661,
            bytes=image.read(0x1ea654,0x1ea664-0x1ea654).hex(),
            owed='void result, preserved EBX/EBP/ESI/EDI and stack consumption; volatile registers not read; CMP overwrites flags before loop branch')}
    report=dict(passed=True,cases=len(cases),complete_cases=len(specs),read_fault_prefixes=len(cases)-len(specs),
        original_xbe_sha256=NEW_PIN,fixture_sha256=hashlib.sha256(data).hexdigest(),fixture_bytes=len(data),
        all_original_sites_covered=True,original_sites=len(sites),instruction_patches=[],supplied_dependency_returns=[],
        production_original_execution=False,production_instruction_decoder=False,caller_contract=caller_contract)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))

if __name__=='__main__':main()
