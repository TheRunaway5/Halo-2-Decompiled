#!/usr/bin/env python3
"""Compare pending-step ABI cases with unchanged instructions on physical i386.

The loop entry and stop boundaries are declared; no callee or result is supplied.
This validates a CPU component, not original Xbox hardware or live destruction.
"""
import argparse,ctypes,hashlib,json,os,struct,subprocess,tempfile
from pathlib import Path
from original_audio_cpu import OriginalAudioCpu
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN


def execute(a,scratch):
    ref=json.loads(a.reference.read_text())
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==ref['xbe_sha256']==NEW_PIN;code=_Code(image,'.text')
    layout=scratch/'ptrace-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"ptrace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc',str(layout),'-o',str(scratch/'ptrace-layout')],check=True);subprocess.run([str(scratch/'ptrace-layout')],check=True)
    aliases=dict(eax='rax',ecx='rcx',edx='rdx',ebx='rbx',esp='rsp',ebp='rbp',esi='rsi',edi='rdi');names=list(aliases.values());steps=stores=0;sites=set()
    bank=ref['bank'];extent=ref['extent'];assert bank==0x2100000 and extent==256
    host=OriginalAudioCpu(image,scratch/'pending-original32.elf',0x37f,0x1f80,source_bank_bytes=0x1101000)
    try:
        host.trace(6,host.debug_offset+7*8,0)
        for row in ref['abi_cases']:
            host.write(bank,bytes.fromhex(row['before_bank_hex']));host.prepare(ref['entry'],ref['stack'],dict(zip(names,row['before_gp'])))
            regs=host.registers();regs.eflags=row['before_eflags'];host.trace(13,0,ctypes.addressof(regs));trace=[];accesses=[]
            for _ in range(32):
                pc=regs.rip
                if pc in(ref['due_boundary'],ref['continue_boundary']):break
                ins=code.at(pc);assert ins.bytes.hex()==ref['original_instruction_sites'][f'{pc:08x}'] and host.read(pc,ins.size)==ins.bytes
                trace.append(pc);sites.add(pc)
                def reg(name):return 0 if not name else getattr(regs,aliases[name])&0xffffffff
                for op in ins.operands:
                    if op.type!=3 or ins.mnemonic=='lea':continue
                    mem=op.mem;address=(reg(ins.reg_name(mem.base))+reg(ins.reg_name(mem.index))*mem.scale+mem.disp)&0xffffffff
                    assert bank<=address and address+op.size<=bank+extent
                    assert op.access in(1,2)
                    accesses.append(dict(pc=pc,kind='write' if op.access==2 else 'read',address=address,size=op.size))
                    stores+=op.access==2
                host.trace(9);regs=host.wait_stop()
            else:raise RuntimeError('Original pending step exceeded bounded instruction count')
            assert regs.rip==row['stop'] and trace==row['trace'] and accesses==row['accesses'],(row['word'],'ordered control/access')
            assert [getattr(regs,n)&0xffffffff for n in names]==row['after_gp'],(row['word'],'GP')
            assert (regs.eflags&ref['defined_flags_mask'])==(row['after_eflags']&ref['defined_flags_mask']),(row['word'],'defined flags')
            assert host.read(bank,extent).hex()==row['after_bank_hex'],(row['word'],'whole declared bank')
            steps+=len(trace)
    finally:host.close()
    assert sites=={int(pc,16) for pc in ref['original_instruction_sites']}
    result=dict(passed=True,scope=__doc__,cases=len(ref['abi_cases']),instruction_sites=len(sites),instructions=steps,ordered_stores=stores,
        declared_bank_bytes=len(ref['abi_cases'])*extent,original_xbe_sha256=NEW_PIN,reference_sha256=hashlib.sha256(a.reference.read_bytes()).hexdigest(),
        backend='Physical Linux x86-64 CS23 i386 compatibility, PTRACE_SINGLESTEP',original_code_patches=[],supplied_callee_results=[],
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),retained_large_artifacts=[])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if a.output.exists():p.error('Preserve prior physical observations')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix='pending-original-',dir=scratch) as owned:execute(a,Path(owned))


if __name__=='__main__':main()
