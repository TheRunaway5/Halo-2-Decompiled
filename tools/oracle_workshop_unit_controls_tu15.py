"""Capture complete unchanged unit-control and jump-test HS evaluators.

Only the real HS parsing/completion boundaries receive declared ABI inputs.
The unit header table, object flags and player-action bank are ordinary input
storage. No effect is intercepted or supplied. Run through run_test.py.
"""
import argparse
import hashlib
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
STACK,STOP,ARGS,UNIT,ACTIONS=0x21fff00,0x3000000,0x2103000,0x2106000,0x2107000
ENTRIES=(0x2ad5b0,0x2b2da0)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--reset-only',action='store_true',help='Capture the unchanged reset companion separately; preserve the existing130-case reference')
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert args.output.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    entries=(0x2b2d80,)if args.reset_only else ENTRIES
    code=_Code(image,'.text');bodies={entry:complete_body(code,entry)for entry in entries}
    allowed={pc:bytes(code.at(pc).bytes)for body in bodies.values()for pc in body.code}
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,size in ((0x10000,0x600000),(0x2100000,0x100000),(STOP,4096)):g.mem_map(at,size)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    def put(at,value,width=4):g.mem_write(at,value.to_bytes(width,'little'))
    def word(at):return int.from_bytes(g.mem_read(at,4),'little')
    calls=[];trace=[];reads=[];writes=[];parsed=False
    def instruction(vm,pc,size,_):
        if pc in (0x212010,0x211da0):
            sp=vm.reg_read(R.UC_X86_REG_ESP)
            calls.append(dict(target=pc,registers=[vm.reg_read(r)for r in GP],stack=bytes(vm.mem_read(sp,24)).hex()))
            if pc==0x212010:vm.reg_write(R.UC_X86_REG_EAX,ARGS if parsed else 0)
            vm.reg_write(R.UC_X86_REG_EIP,word(sp));vm.reg_write(R.UC_X86_REG_ESP,sp+(20 if pc==0x212010 else 4));return
        assert pc in allowed and bytes(vm.mem_read(pc,size))==allowed[pc],hex(pc)
        trace.append(pc)
    def read(vm,_kind,at,size,value,_):
        if not STACK-2048<=at<STACK+256:reads.append([vm.reg_read(R.UC_X86_REG_EIP),at,size])
    def write(vm,_kind,at,size,value,_):
        if not STACK-2048<=at<STACK+256:writes.append([vm.reg_read(R.UC_X86_REG_EIP),at,size,value&((1<<(size*8))-1)])
    g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
    rows=[]
    for entry in entries:
        if entry==0x2ad5b0:
            inputs=[dict(parsed=p,object=o,enabled=e,flags=f)for p in (False,True)
                for o in (0xffffffff,0x23450002)for e in (0,1,0x100,0xffffff00,0xffffffff)
                for f in (0,2,0x20000,0xffffffff,0x12345678,0x87654321)]
        else:inputs=[dict(parsed=False,object=0xffffffff,enabled=0,flags=f,**({'armed':armed}if args.reset_only else {}))for f in
            (0,1,2,3,4,0xffffffff,0x80000000,0xaaaaaaaa,0x55555555,0xfffffffd)
            for armed in ((0,0x12345678)if args.reset_only else (0,))]
        for controls in inputs:
            seed=len(rows)&255;parsed=controls['parsed']
            bank=bytes((seed+n*13)&255 for n in range(0x800));g.mem_write(UNIT,bank)
            stack=bytes((seed+n*7)&255 for n in range(2048));g.mem_write(STACK-1792,stack)
            put(0x4ecbd0,0x2104000);put(0x2104044,0x2105000);put(0x2105020,UNIT)
            if args.reset_only:g.mem_write(ACTIONS,bytes((seed+n*17)&255 for n in range(64)))
            put(0x4f9b54,ACTIONS);put(ACTIONS+4,controls['flags'])
            if args.reset_only:put(ACTIONS+8,controls['armed'])
            put(UNIT+0x134,controls['flags']);put(ARGS,controls['object']);put(ARGS+4,controls['enabled'])
            put(STACK,STOP);put(STACK+4,210 if entry==0x2ad5b0 else 479 if args.reset_only else 480)
            put(STACK+8,0x12340001);put(STACK+12,0x23450002)
            initial=[(0x11223344+n*0x112233+seed*0x10203)&0xffffffff for n in range(8)];initial[4]=STACK
            for reg,value in zip(GP,initial):g.reg_write(reg,value)
            xmm=[bytes((seed+n+j)&255 for j in range(16))for n in range(8)]
            for n,value in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+n,int.from_bytes(value,'little'))
            flags=0x247 if seed&1 else 0x202
            g.reg_write(R.UC_X86_REG_EFLAGS,flags);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
            g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
            before=bytes(g.mem_read(UNIT,0x800));stack_before=bytes(g.mem_read(STACK-1792,2048))
            actions_before=bytes(g.mem_read(ACTIONS,64))if args.reset_only else None
            calls.clear();trace.clear();reads.clear();writes.clear();g.emu_start(entry,STOP,count=1000)
            assert g.reg_read(R.UC_X86_REG_EIP)==STOP
            assert g.reg_read(R.UC_X86_REG_FPCW)==0x27f and g.reg_read(R.UC_X86_REG_FPSW)==0 and g.reg_read(R.UC_X86_REG_MXCSR)==0x1f80
            assert [g.reg_read(R.UC_X86_REG_XMM0+n).to_bytes(16,'little')for n in range(8)]==xmm
            rows.append(dict(entry=entry,seed=seed,**controls,initial_registers=initial,initial_flags=flags,
                bank_before=before.hex(),bank_after=bytes(g.mem_read(UNIT,0x800)).hex(),
                stack_before=stack_before.hex(),stack_after=bytes(g.mem_read(STACK-1792,2048)).hex(),
                **(dict(actions_before=actions_before.hex(),actions_after=bytes(g.mem_read(ACTIONS,64)).hex())if args.reset_only else {}),
                registers=[g.reg_read(r)for r in GP],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
                calls=calls.copy(),trace=trace.copy(),reads=reads.copy(),writes=writes.copy()))
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        instruction_patches=[],effects_intercepted=[],intercepted_boundaries=[0x212010,0x211da0],cases=rows,
        owners=[dict(entry=entry,instructions=[[pc,bytes(code.at(pc).bytes).hex()]for pc in sorted(body.code)],
            executed_sites=sorted({pc for row in rows if row['entry']==entry for pc in row['trace']}))for entry,body in bodies.items()])
    raw=(json.dumps(report,separators=(',',':'))+'\n').encode();args.output.write_bytes(zlib.compress(raw,9))
    print(json.dumps(dict(cases=len(rows),fixture_bytes=args.output.stat().st_size,uncompressed_bytes=len(raw),
        instruction_sites=sum(len(b.code)for b in bodies.values()),all_sites_executed=all(set(b.code)=={pc for r in rows if r['entry']==e for pc in r['trace']}for e,b in bodies.items()))))


if __name__=='__main__':main()
