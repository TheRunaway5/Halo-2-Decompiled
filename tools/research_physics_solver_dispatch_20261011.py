#!/usr/bin/env python3
"""Record original solver initialization, schema dispatch, and empty-row lifecycle.

This is original-only research, not native solver validation or gameplay proof.
Every executable image is freshly loaded. Original license initialization is
executed; its global flag and result are never forced. Declared schema-header
and end records exercise lifecycle arithmetic with real source-prepared bodies,
not a physical contact or a synthesized solved body.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

BASE,SP,STOP=0x1000000,0x200f000,0x3000000
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--correspondence',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    correspondence=json.loads(a.correspondence.read_text())
    assert correspondence['disc_1_1_sha256']==PIN and correspondence['later_owned_xbe_sha256']==NEW_PIN
    assert correspondence['complete_bodies']==22
    mapping={int(k,16):v for k,v in correspondence['entries'].items()}
    sites={v:int(k,16) for k,v in correspondence['instruction_map'].items()}
    operands={int(k,16):v['later'] for k,v in correspondence['operand_map'].items()}
    images=[]
    for latest in (False,True):
        selected=lambda p:mapping[p] if latest else p
        operand=lambda p:operands[p] if latest else p
        path=NEW_DEFAULT if latest else ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
        assert sha(path)==(NEW_PIN if latest else PIN)
        image=Xbe(path);rows=[]
        checksum_call=_Code(image,'.text').at(correspondence['instruction_map']['00322d6f'] if latest else 0x322d6f)
        assert checksum_call.mnemonic=='call' and checksum_call.operands[0].type==2
        checksum_entry=checksum_call.operands[0].imm
        # Source dispatch operands are addresses in the complete paired body.
        iteration_table=operand(0x324c34);readback_table=operand(0x324c94)
        index_instruction=_Code(image,'.text').at(correspondence['instruction_map']['00324aac'] if latest else 0x324aac)
        assert index_instruction.mnemonic=='movzx' and index_instruction.operands[1].size==1
        readback_indices=index_instruction.operands[1].mem.disp
        first=struct.unpack('<17I',image.read(iteration_table,68))
        second=struct.unpack('<8I',image.read(readback_table,32));indices=image.read(readback_indices,17)
        dispatch={str(i):dict(iteration=first[i],readback=second[indices[i]]) for i in range(17)}
        for iterations in (0,1,2,8):
            g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
            for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
            g.mem_map(BASE,0x10000);g.mem_map(0x2000000,0x10000);g.mem_map(STOP,0x1000)
            w=lambda p,x,n=4:g.mem_write(p,(int(x)&((1<<(8*n))-1)).to_bytes(n,'little'))
            f=lambda p,x:g.mem_write(p,struct.pack('<f',x))
            vec=lambda p,x:g.mem_write(p,struct.pack('<4f',*x))
            active=False;trace=[];writes=[]
            def code(_g,p,n,d):
                if active:
                    trace.append(p)
                    # Expected supplied license follows its own complete
                    # source checksum path. Any other branch remains visible.
            def store(_g,access,p,n,x,d):
                if active and not 0x2000000<=p<0x2010000:
                    actual=g.reg_read(R.UC_X86_REG_EIP)
                    writes.append(dict(instruction=sites.get(actual,actual) if latest else actual,
                        address=p,before=bytes(g.mem_read(p,n)).hex(),
                        written=(x&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
            g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,store)
            def call(entry,receiver,args):
                w(SP,STOP)
                for i,x in enumerate(args):w(SP+4+i*4,x)
                for register,value in [(R.UC_X86_REG_ESP,SP),(R.UC_X86_REG_ECX,receiver),
                    (R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_MXCSR,0x1f80)]:g.reg_write(register,value)
                g.emu_start(selected(entry),STOP,count=1000000)
                assert g.reg_read(R.UC_X86_REG_EIP)==STOP,hex(g.reg_read(R.UC_X86_REG_EIP))
                return g.reg_read(R.UC_X86_REG_EAX)
            position,quaternion,step,gravity=BASE+0x3000,BASE+0x3020,BASE+0x3040,BASE+0x3060
            motion,accumulators,configuration,schemas,jacobians,impulses=BASE+0x1000,BASE+0x4000,BASE+0x5000,BASE+0x5100,BASE+0x5200,BASE+0x6000
            vec(position,(1.5,-2,4,0));vec(quaternion,(0,0,.6,.8));call(0x321320,motion,[position,quaternion])
            vec(motion+0x40,(1,-2,.5,0));vec(motion+0x50,(.25,-.5,.125,0));f(step,1/30);vec(gravity,(0,0,-4.1712594,0))
            w(accumulators,2,1)
            call(0x3218a0,motion,[step,gravity,accumulators+0x80]);w(accumulators+0x100,3,1)
            # Original schema header layout from322F30/324A81 and the
            # terminator emitted by322937. There is no constraint row.
            w(schemas,1,1);w(schemas+4,jacobians);w(schemas+8,BASE+0x7000)
            w(schemas+12,accumulators);w(schemas+16,accumulators+0x80);w(schemas+20,8)
            w(schemas+24,0x400)
            f(configuration+4,.25);f(configuration+8,1);f(configuration+16,.5);w(configuration+28,iterations)
            flag=operand(0x488180);before_flag=bytes(g.mem_read(flag,1)).hex()
            before=bytes(g.mem_read(BASE,0x10000));active=True
            result=call(0x322ed0,0,[configuration,schemas,accumulators,impulses]);active=False
            assert result==1 and int.from_bytes(g.mem_read(flag,1),'little')==1
            original_phase=sites.get(selected(0x322ed0),selected(0x322ed0)) if latest else 0x322ed0
            rows.append(dict(iterations=iterations,entry=selected(0x322ed0),canonical_entry=original_phase,
                before_flag=before_flag,after_flag=bytes(g.mem_read(flag,1)).hex(),return_value=result,
                full_before=before.hex(),full_after=bytes(g.mem_read(BASE,0x10000)).hex(),
                ordered_persistent_writes=writes,executed_instructions=len(trace),
                initialization_executed=selected(0x322d60) in trace,
                initialization_checksum_code_executed=checksum_entry in trace))
        images.append(dict(image='1.5' if latest else '1.1',original_sha256=sha(path),schema_dispatch=dispatch,cases=rows))
    result=dict(schema=1,tool_sha256=sha(__file__),correspondence_sha256=sha(a.correspondence),images=images,
        source_instruction_patches=[],forced_initialization_flags=[],native_solver_implemented=False,scope=__doc__)
    p=a.output/'report.json';p.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
    print(json.dumps(dict(images=len(images),cases=sum(len(i['cases']) for i in images),report_sha256=sha(p))))
if __name__=='__main__':main()
