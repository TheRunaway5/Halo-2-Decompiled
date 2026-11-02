"""Compare the unchanged scenery-start HS wrapper at its three ABI boundaries.

Parser, scenery effect and completion calls receive explicit boundary returns.
This is wrapper evidence only; it does not qualify animation execution. Run
through run_test.py and keep the reference inside HALO2_TEST_SCRATCH.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
import zlib

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

GP=(R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
    R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI)
ENTRY,STACK,STOP,ARGS=0x2accf0,0x21fff00,0x3000000,0x2103000
BOUNDARIES={0x212010:20,0x10f710:28,0x211da0:4}


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert a.output.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    descriptor=image.read(0x457914,20)
    assert descriptor.hex()=='04000000f0cc2a00000000000300370024000b00'
    code=_Code(image,'.text');body=complete_body(code,ENTRY)
    allowed={pc:bytes(code.at(pc).bytes)for pc in body.code};assert len(allowed)==29
    effect=complete_body(code,0x10f710)
    assert {code.at(pc).op_str for pc in effect.code if code.at(pc).mnemonic=='ret'}=={'0x18'}
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,size in ((0x10000,0x600000),(0x2100000,0x100000),(STOP,4096)):g.mem_map(at,size)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    def put(at,value):g.mem_write(at,struct.pack('<I',value))
    def word(at):return int.from_bytes(g.mem_read(at,4),'little')
    calls=[];trace=[];reads=[];parsed=False;seed=0;effect_return=0
    def instruction(vm,pc,size,_):
        if pc in BOUNDARIES:
            sp=vm.reg_read(R.UC_X86_REG_ESP)
            calls.append(dict(target=pc,registers=[vm.reg_read(r)for r in GP],stack=bytes(vm.mem_read(sp,28)).hex()))
            result=(ARGS if parsed else 0)if pc==0x212010 else effect_return if pc==0x10f710 else 0xabc00000|seed
            vm.reg_write(R.UC_X86_REG_EAX,result)
            vm.reg_write(R.UC_X86_REG_ECX,0xdef00000|seed);vm.reg_write(R.UC_X86_REG_EDX,0x98700000|seed)
            vm.reg_write(R.UC_X86_REG_EIP,word(sp));vm.reg_write(R.UC_X86_REG_ESP,sp+BOUNDARIES[pc]);return
        assert pc in allowed and bytes(vm.mem_read(pc,size))==allowed[pc],hex(pc)
        trace.append(pc)
    def read(vm,_kind,at,size,value,_):
        if not STACK-2048<=at<STACK+256:reads.append([vm.reg_read(R.UC_X86_REG_EIP),at,size])
    g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,read)
    rows=[]
    for parsed,obj,graph,name,effect_return in itertools.product((False,True),(0xffffffff,0x23450002),
        (0xffffffff,0xe9f1087c),(0,0x06001d05),(0,0x12345678)):
        seed=len(rows);stack=bytes((seed+n*7)&255 for n in range(2048));g.mem_write(STACK-1792,stack)
        g.mem_write(ARGS,struct.pack('<III',obj,graph,name))
        put(STACK,STOP);put(STACK+4,184|(seed<<16));put(STACK+8,0x12340001);put(STACK+12,0x23450002)
        initial=[(0x11223344+n*0x112233+seed*0x10203)&0xffffffff for n in range(8)];initial[4]=STACK
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        xmm=[bytes((seed+n+j)&255 for j in range(16))for n in range(8)]
        for n,value in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+n,int.from_bytes(value,'little'))
        flags=(0x247 if seed&1 else 0x202)|(0x400 if seed&2 else 0)
        g.reg_write(R.UC_X86_REG_EFLAGS,flags);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
        g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        stack_before=bytes(g.mem_read(STACK-1792,2048));calls.clear();trace.clear();reads.clear()
        g.emu_start(ENTRY,STOP,count=1000);assert g.reg_read(R.UC_X86_REG_EIP)==STOP
        assert g.reg_read(R.UC_X86_REG_FPCW)==0x27f and g.reg_read(R.UC_X86_REG_FPSW)==0 and g.reg_read(R.UC_X86_REG_MXCSR)==0x1f80
        assert [g.reg_read(R.UC_X86_REG_XMM0+n).to_bytes(16,'little')for n in range(8)]==xmm
        assert [r['target']for r in calls]==([0x212010,0x10f710,0x211da0]if parsed else[0x212010])
        if parsed:
            assert calls[1]['registers'][0]==graph
            assert struct.unpack('<7I',bytes.fromhex(calls[1]['stack']))[1:]==(obj,name,0,0xffffffff,1,0)
            assert calls[2]['registers'][0]==0
        rows.append(dict(seed=seed,parsed=parsed,object=obj,graph=graph,name=name,effect_return=effect_return,
            initial_registers=initial,initial_flags=flags,stack_before=stack_before.hex(),
            stack_after=bytes(g.mem_read(STACK-1792,2048)).hex(),registers=[g.reg_read(r)for r in GP],
            eflags=g.reg_read(R.UC_X86_REG_EFLAGS),calls=calls.copy(),trace=trace.copy(),reads=reads.copy()))
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        entry=ENTRY,descriptor=descriptor.hex(),instruction_patches=[],effects_intercepted=[0x10f710],
        intercepted_boundaries=list(BOUNDARIES),cases=rows,
        instructions=[[pc,allowed[pc].hex()]for pc in sorted(allowed)],
        all_sites_executed=set(allowed)=={pc for row in rows for pc in row['trace']},engine_effects_qualified=False)
    assert report['all_sites_executed'] and len(rows)==32
    raw=(json.dumps(report,separators=(',',':'))+'\n').encode();packed=zlib.compress(raw,9)
    a.output.write_bytes(packed)
    a.output.with_suffix('').write_text(json.dumps(dict(packed_bytes=len(packed),packed_sha256=hashlib.sha256(packed).hexdigest(),
        decoded_bytes=len(raw),decoded_sha256=hashlib.sha256(raw).hexdigest(),scope=__doc__,cases=len(rows)),indent=2)+'\n')
    print(json.dumps(dict(cases=len(rows),sites=len(allowed),all_sites_executed=True,packed_bytes=len(packed))))


if __name__=='__main__':main()
