"""Record unchanged original 2B27B0 at explicit parser/effect/completion ABIs.

These intercepted calls test the complete wrapper, including deferred parsing,
implicit NONE anchors and VOID completion; they do not qualify camera effects.
All generated reference bytes stay under HALO2_TEST_SCRATCH.
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
ENTRY,STACK,STOP,ARGS=0x2b27b0,0x21fff00,0x3000000,0x2103000
BOUNDARIES={0x212010:20,0x16edd0:12,0x211da0:4}


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert a.output.resolve().is_relative_to(owner)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    descriptor=image.read(0x458e90,18)
    assert descriptor.hex()=='04000000b0272b0000000000020024000b00'
    code=_Code(image,'.text');body=complete_body(code,ENTRY)
    encoded={pc:bytes(code.at(pc).bytes)for pc in body.code};assert len(encoded)==26
    assert {code.at(pc).op_str for pc in complete_body(code,0x16edd0).code
        if code.at(pc).mnemonic=='ret'}=={'8'}
    vm=Uc(UC_ARCH_X86,UC_MODE_32)
    for address,size in ((0x10000,0x600000),(0x2100000,0x100000),(STOP,4096)):vm.mem_map(address,size)
    for section in image.sections:vm.mem_write(section.va,image.section_bytes(section))
    def word(at):return int.from_bytes(vm.mem_read(at,4),'little')
    calls=[];trace=[];reads=[];parsed=False;seed=0
    def instruction(machine,pc,size,_):
        if pc in BOUNDARIES:
            sp=machine.reg_read(R.UC_X86_REG_ESP)
            calls.append(dict(target=pc,registers=[machine.reg_read(r)for r in GP],
                stack=bytes(machine.mem_read(sp,28)).hex()))
            result=(ARGS if parsed else 0)if pc==0x212010 else 0xabc00000|seed
            machine.reg_write(R.UC_X86_REG_EAX,result)
            machine.reg_write(R.UC_X86_REG_ECX,0xdef00000|seed)
            machine.reg_write(R.UC_X86_REG_EDX,0x98700000|seed)
            machine.reg_write(R.UC_X86_REG_EIP,word(sp))
            machine.reg_write(R.UC_X86_REG_ESP,sp+BOUNDARIES[pc]);return
        assert pc in encoded and bytes(machine.mem_read(pc,size))==encoded[pc],hex(pc)
        trace.append(pc)
    def read(machine,_kind,at,size,_value,_):reads.append([machine.reg_read(R.UC_X86_REG_EIP),at,size])
    vm.hook_add(UC_HOOK_CODE,instruction);vm.hook_add(UC_HOOK_MEM_READ,read)
    rows=[]
    for parsed,graph,name,opcode in itertools.product((False,True),(0xffffffff,0xf0f90f83),
        (0,0x11002a27),(452,-1,-32768,32767)):
        seed=len(rows);table=0x480c00+opcode*4
        vm.mem_write(table,struct.pack('<I',0x458e90));vm.mem_write(0x458e90,descriptor)
        vm.mem_write(STACK-1792,bytes((seed+n*7)&255 for n in range(2048)))
        vm.mem_write(ARGS,struct.pack('<II',graph,name))
        vm.mem_write(STACK,struct.pack('<IIII',STOP,(opcode&65535)|(seed<<16),0x12340001,0x23450002))
        initial=[(0x11223344+n*0x112233+seed*0x10203)&0xffffffff for n in range(8)];initial[4]=STACK
        for reg,value in zip(GP,initial):vm.reg_write(reg,value)
        xmm=[bytes((seed+n+j)&255 for j in range(16))for n in range(8)]
        for n,value in enumerate(xmm):vm.reg_write(R.UC_X86_REG_XMM0+n,int.from_bytes(value,'little'))
        flags=(0x247 if seed&1 else 0x202)|(0x400 if seed&2 else 0)
        for reg,value in ((R.UC_X86_REG_EFLAGS,flags),(R.UC_X86_REG_FPCW,0x27f),
            (R.UC_X86_REG_FPSW,0x1800),(R.UC_X86_REG_MXCSR,0x1f80)):vm.reg_write(reg,value)
        for n,value in ((3,(0x8000000000000000,0x3ffe)),(4,(0x8000000000000000,0xbfff)),(5,(0x8000000000000000,0x4000))):
            vm.reg_write(R.UC_X86_REG_FP0+n,value)
        vm.reg_write(R.UC_X86_REG_FPTAG,0xf03f)
        fp_registers=(R.UC_X86_REG_FPCW,R.UC_X86_REG_FPSW,R.UC_X86_REG_FPTAG,R.UC_X86_REG_MXCSR)
        before_fp=[vm.reg_read(r)for r in fp_registers]
        before_physical=[vm.reg_read(R.UC_X86_REG_FP0+n)for n in range(8)]
        before_stack=bytes(vm.mem_read(STACK-1792,2048));calls.clear();trace.clear();reads.clear()
        vm.emu_start(ENTRY,STOP,count=200);assert vm.reg_read(R.UC_X86_REG_EIP)==STOP
        assert before_fp==[vm.reg_read(r)for r in fp_registers]
        assert before_physical==[vm.reg_read(R.UC_X86_REG_FP0+n)for n in range(8)]
        assert xmm==[vm.reg_read(R.UC_X86_REG_XMM0+n).to_bytes(16,'little')for n in range(8)]
        assert bytes(vm.mem_read(ARGS,8))==struct.pack('<II',graph,name)
        assert bytes(vm.mem_read(0x458e90,18))==descriptor and word(table)==0x458e90
        assert [c['target']for c in calls]==([0x212010,0x16edd0,0x211da0]if parsed else[0x212010])
        if parsed:
            assert calls[1]['registers'][0]==graph and calls[1]['registers'][3]==name
            assert struct.unpack('<3I',bytes.fromhex(calls[1]['stack'])[:12])[1:]==(0xffffffff,0xffffffff)
            assert calls[2]['registers'][0]==0 and calls[2]['registers'][1]==0x12340001
        rows.append(dict(seed=seed,parsed=parsed,graph=graph,name=name,opcode=opcode,table=table,
            initial_registers=initial,initial_flags=flags,stack_before=before_stack.hex(),
            stack_after=bytes(vm.mem_read(STACK-1792,2048)).hex(),registers=[vm.reg_read(r)for r in GP],
            eflags=vm.reg_read(R.UC_X86_REG_EFLAGS),calls=calls.copy(),trace=trace.copy(),reads=reads.copy(),
            live_fp_simd_unchanged=True))
    assert len(rows)==32 and set(encoded)=={pc for row in rows for pc in row['trace']}
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        entry=ENTRY,descriptor=descriptor.hex(),instruction_patches=[],intercepted_boundaries=list(BOUNDARIES),
        effects_intercepted=[0x16edd0],all_sites_executed=True,cases=rows,
        instructions=[[pc,encoded[pc].hex()]for pc in sorted(encoded)],engine_effects_qualified=False)
    raw=(json.dumps(report,separators=(',',':'))+'\n').encode();packed=zlib.compress(raw,9)
    a.output.write_bytes(packed);a.output.with_suffix('').write_text(json.dumps(dict(cases=32,sites=26,
        packed_bytes=len(packed),packed_sha256=hashlib.sha256(packed).hexdigest(),
        decoded_bytes=len(raw),decoded_sha256=hashlib.sha256(raw).hexdigest()),indent=2)+'\n')
    print(json.dumps(dict(cases=32,sites=26,packed_bytes=len(packed))))


if __name__=='__main__':main()
