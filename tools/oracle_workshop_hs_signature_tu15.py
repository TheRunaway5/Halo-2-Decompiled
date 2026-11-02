"""Prove a per-invocation signature lifetime in the unchanged original HS parser.

All parsing, primitive evaluation and deferred frame construction execute the
authenticated TU1.5 instructions. No dependency result or instruction is
replaced. Run inside run_test.py; retain only the bounded report.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_PROT_ALL, UC_PROT_NONE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

GP=(R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
    R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI)
STACK,STOP=0x21fff00,0x3000000
POOL,SYNTAX,THREAD,NODES=0x2100000,0x2101000,0x2102000,0x2104000
FRAME=THREAD+0x40
SIGNATURE_A,SIGNATURE_B=0x3100000,0x3101000


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not a.report.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text')
    entries=(0x212010,0x211cb0,0x212560)
    bodies={entry:complete_body(code,entry)for entry in entries}
    allowed={pc:bytes(code.at(pc).bytes)for body in bodies.values()for pc in body.code}
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,size in ((0x10000,0x600000),(0x2100000,0x100000),(STOP,4096),(SIGNATURE_A,8192)):
        g.mem_map(at,size)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    def put(at,value,width=4):g.mem_write(at,int(value).to_bytes(width,'little'))
    def word(at,width=4):return int.from_bytes(g.mem_read(at,width),'little')
    trace=[];reads=[];writes=[]
    def instruction(vm,pc,size,_):
        assert pc in allowed and bytes(vm.mem_read(pc,size))==allowed[pc],hex(pc)
        trace.append(pc)
    def read(vm,_kind,at,size,value,_):reads.append([vm.reg_read(R.UC_X86_REG_EIP),at,size])
    def write(vm,_kind,at,size,value,_):writes.append([vm.reg_read(R.UC_X86_REG_EIP),at,size,value&((1<<(8*size))-1)])
    g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
    def invoke(signature,initial,seed):
        initial_gp=[(0x11223344+n*0x10203+seed)&0xffffffff for n in range(8)];initial_gp[4]=STACK
        for reg,value in zip(GP,initial_gp):g.reg_write(reg,value)
        put(STACK,STOP);put(STACK+4,0xa0010000);put(STACK+8,1)
        put(STACK+12,signature);put(STACK+16,initial)
        g.reg_write(R.UC_X86_REG_EFLAGS,0x247 if seed&1 else 0x202)
        g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_FPSW,0)
        g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        xmm=[int.from_bytes(bytes((seed+n+j)&255 for j in range(16)),'little')for n in range(8)]
        for n,value in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+n,value)
        trace.clear();reads.clear();writes.clear();g.emu_start(0x212010,STOP,count=2000)
        assert g.reg_read(R.UC_X86_REG_EIP)==STOP and g.reg_read(R.UC_X86_REG_ESP)==STACK+20
        for n in (3,5,6,7):assert g.reg_read(GP[n])==initial_gp[n]
        assert g.reg_read(R.UC_X86_REG_FPCW)==0x27f and g.reg_read(R.UC_X86_REG_FPSW)==0
        assert g.reg_read(R.UC_X86_REG_MXCSR)==0x1f80
        assert [g.reg_read(R.UC_X86_REG_XMM0+n)for n in range(8)]==xmm
        assert not any(SIGNATURE_A<=at<SIGNATURE_B+4096 for _,at,_,_ in writes)
        return dict(result=g.reg_read(R.UC_X86_REG_EAX),sites=sorted(set(trace)),
            signature_reads=[row for row in reads if SIGNATURE_A<=row[1]<SIGNATURE_B+4096],
            ordinary_writes=[row for row in writes if not STACK-2048<=row[1]<STACK+256])
    rows=[];covered=set()
    for nested in (False,True):
        for value in (0,1,0x7fff,0x8000,0xffff,0x1234ffff,0xffffffff):
            for seed in (0,1):
                g.mem_protect(SIGNATURE_A,8192,UC_PROT_ALL)
                g.mem_write(0x2100000,bytes(0x10000))
                put(0x505e5c,POOL);put(POOL+0x44,THREAD);put(0x505e6c,SYNTAX)
                put(SYNTAX+0x44,NODES);put(THREAD+0x10,FRAME);put(FRAME+4,0x90000000)
                put(FRAME+0xc,0,2);put(NODES+16,0x90010001)
                put(NODES+20+8,0x90020002)
                argument=NODES+40;put(argument,0x9002,2);put(argument+2,7,2)
                put(argument+4,7,2);put(argument+6,8 if nested else 9,2)
                put(argument+8,0xffffffff);put(argument+16,value)
                put(SIGNATURE_A,7,2);put(SIGNATURE_B,7,2)
                first=invoke(SIGNATURE_A,1,seed);covered.update(first['sites'])
                assert first['result']==0 and word(FRAME+18,2)==1
                assert first['signature_reads']==[[0x2120e7,SIGNATURE_A,2]]
                # The expired invocation's type list is now unreadable. A fresh
                # allocation must suffice even when a child frame was created.
                g.mem_protect(SIGNATURE_A,4096,UC_PROT_NONE)
                if nested:
                    assert word(THREAD+0x10)!=FRAME and word(word(THREAD+0x10)+4)==0x90020002
                    # This is a parser lifetime test, not a supplied child result.
                    # Leave the child outstanding and inspect only saved pointers.
                    assert not any(v==SIGNATURE_A for _,_,_,v in first['ordinary_writes'])
                    rows.append(dict(nested=True,value=value,seed=seed,first=first));continue
                assert word(THREAD+0x10)==FRAME and word(FRAME+14)==value
                put(FRAME+0xc,0,2)  # Exact211BF4 per-invocation scratch reset.
                second=invoke(SIGNATURE_B,0,seed);covered.update(second['sites'])
                assert second['result']==FRAME+14 and not second['signature_reads']
                assert word(second['result'],2)==(value&0xffff)
                assert not any(v in (SIGNATURE_A,SIGNATURE_B)for _,_,_,v in second['ordinary_writes'])
                rows.append(dict(nested=False,value=value,seed=seed,first=first,second=second))
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        owned_scratch=str(scratch),instruction_patches=[],intercepted_boundaries=[],
        effects_supplied=[],cases=rows,executed_sites=sorted(covered),
        complete_bodies=[dict(entry=entry,instructions=len(body.code))for entry,body in bodies.items()],
        nested_child_completion_tested=False,native_adapter_enabled=False,retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(rows),actual_parser_invocations=42,expired_signature_unreadable=True,report=str(a.report))))


if __name__=='__main__':main()
