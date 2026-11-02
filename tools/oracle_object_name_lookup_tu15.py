"""Capture complete unchanged BD790, without intercepted engine calls.

Cover signed AX bounds, poisoned upper bits, arbitrary stored datums, complete
GP/flags, ordered reads and unchanged FP/SIMD state. Reference output belongs
to HALO2_TEST_SCRATCH; no runtime fallback or declaration admission is supplied.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
import zlib

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

GP=(R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
    R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI)
ENTRY,STACK,STOP,TABLE=0xbd790,0x21fff00,0x3000000,0x2108000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert args.output.resolve().is_relative_to(owner)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');body=complete_body(code,ENTRY)
    encoded={pc:bytes(code.at(pc).bytes)for pc in body.code};assert len(encoded)==10
    assert not any(code.at(pc).mnemonic=='call'for pc in body.code)
    vm=Uc(UC_ARCH_X86,UC_MODE_32)
    for address,size in ((0x10000,0x600000),(0x2100000,0x100000),(STOP,4096)):vm.mem_map(address,size)
    for section in image.sections:vm.mem_write(section.va,image.section_bytes(section))
    trace=[];reads=[];writes=[]
    def instruction(machine,pc,size,_):
        assert pc in encoded and bytes(machine.mem_read(pc,size))==encoded[pc],hex(pc)
        trace.append(pc)
    def read(machine,_kind,address,size,_value,_):reads.append([machine.reg_read(R.UC_X86_REG_EIP),address,size])
    def write(machine,_kind,address,size,value,_):writes.append([machine.reg_read(R.UC_X86_REG_EIP),address,size,value])
    vm.hook_add(UC_HOOK_CODE,instruction);vm.hook_add(UC_HOOK_MEM_READ,read);vm.hook_add(UC_HOOK_MEM_WRITE,write)
    rows=[]
    for low,high,value in itertools.product((0x8000,0xffff,0,1,639,640,32767),
        (0,0xffff,0x8000,0x1234),(0xffffffff,0x82010023,0,0xf0010002)):
        seed=len(rows);eax=(high<<16)|low;valid=low<640
        vm.mem_write(0x4eaba0,struct.pack('<I',TABLE));vm.mem_write(STACK,struct.pack('<I',STOP))
        if valid:vm.mem_write(TABLE+low*4,struct.pack('<I',value))
        initial=[(0x11223344+n*0x112233+seed*0x10203)&0xffffffff for n in range(8)]
        initial[0]=eax;initial[4]=STACK
        for reg,item in zip(GP,initial):vm.reg_write(reg,item)
        xmm=[bytes((seed+n+j)&255 for j in range(16))for n in range(8)]
        for n,item in enumerate(xmm):vm.reg_write(R.UC_X86_REG_XMM0+n,int.from_bytes(item,'little'))
        flags=(0x247 if seed&1 else 0x202)|(0x400 if seed&2 else 0)
        vm.reg_write(R.UC_X86_REG_EFLAGS,flags);vm.reg_write(R.UC_X86_REG_FPCW,0x27f)
        vm.reg_write(R.UC_X86_REG_FPSW,0x1800);vm.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        for reg,item in ((3,(0x8000000000000000,0x3ffe)),(4,(0x8000000000000000,0xbfff)),(5,(0x8000000000000000,0x4000))):
            vm.reg_write(R.UC_X86_REG_FP0+reg,item)
        vm.reg_write(R.UC_X86_REG_FPTAG,0xf03f)
        floating=[vm.reg_read(reg)for reg in (R.UC_X86_REG_FPCW,R.UC_X86_REG_FPSW,R.UC_X86_REG_FPTAG,R.UC_X86_REG_MXCSR)]
        assert floating==[0x27f,0x1800,0xf03f,0x1f80]
        physical=[vm.reg_read(R.UC_X86_REG_FP0+n)for n in range(8)]
        trace.clear();reads.clear();writes.clear();vm.emu_start(ENTRY,STOP,count=100)
        assert vm.reg_read(R.UC_X86_REG_EIP)==STOP and not writes
        assert floating==[vm.reg_read(reg)for reg in (R.UC_X86_REG_FPCW,R.UC_X86_REG_FPSW,R.UC_X86_REG_FPTAG,R.UC_X86_REG_MXCSR)]
        assert physical==[vm.reg_read(R.UC_X86_REG_FP0+n)for n in range(8)]
        assert xmm==[vm.reg_read(R.UC_X86_REG_XMM0+n).to_bytes(16,'little')for n in range(8)]
        registers=[vm.reg_read(reg)for reg in GP];assert registers[0]==(value if valid else 0xffffffff)
        expected=([[0xbd79b,0x4eaba0,4],[0xbd7a4,TABLE+low*4,4]]if valid else[])+[[0xbd7a7 if valid else 0xbd7ab,STACK,4]]
        assert reads==expected
        rows.append(dict(seed=seed,index=low,table=TABLE,slot=value,valid=valid,
            initial_registers=initial,initial_flags=flags,registers=registers,eflags=vm.reg_read(R.UC_X86_REG_EFLAGS),
            reads=reads.copy(),writes=[],trace=trace.copy(),floating_unchanged=True))
    assert len(rows)==112 and set(encoded)=={pc for row in rows for pc in row['trace']}
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,entry=ENTRY,stop=STOP,instruction_patches=[],
        intercepted_boundaries=[],instructions=[[pc,encoded[pc].hex()]for pc in sorted(encoded)],
        all_sites_executed=True,cases=rows,source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        runtime_admission_performed=False)
    raw=(json.dumps(report,separators=(',',':'))+'\n').encode();packed=zlib.compress(raw,9)
    args.output.write_bytes(packed);args.output.with_suffix('').write_text(json.dumps(dict(cases=112,sites=10,
        packed_bytes=len(packed),packed_sha256=hashlib.sha256(packed).hexdigest(),
        decoded_bytes=len(raw),decoded_sha256=hashlib.sha256(raw).hexdigest()),indent=2)+'\n')
    print(json.dumps(dict(cases=112,sites=10,all_sites_executed=True,packed_bytes=len(packed))))


if __name__=='__main__':main()
