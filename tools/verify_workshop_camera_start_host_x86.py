"""Cross-check the unchanged camera HS wrapper on physical i386.

Parser, camera effect and completion are explicitly intercepted ABI inputs.
The entire wrapper remains unchanged; GP/flags/stack, boundary calls, complete
ordinary banks and defined live FP/SIMD state are checked. No camera effect,
World routing or MCC declaration is qualified by this component experiment.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import zlib

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    root=Path(__file__).resolve().parents[1];owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not a.report.resolve().is_relative_to(owner)
    fixture=root/'tests/fixtures/workshop_camera_start_tu15.json.zlib';packed=fixture.read_bytes()
    descriptor=json.loads(fixture.with_suffix('').read_text())
    assert len(packed)==descriptor['packed_bytes']and hashlib.sha256(packed).hexdigest()==descriptor['packed_sha256']
    decoder=zlib.decompressobj();raw=decoder.decompress(packed,4*1024*1024+1)
    assert decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail
    assert len(raw)==descriptor['decoded_bytes']and hashlib.sha256(raw).hexdigest()==descriptor['decoded_sha256']
    ref=json.loads(raw);assert ref['xbe_sha256']==NEW_PIN and len(ref['cases'])==32
    assert ref['instruction_patches']==[]and ref['intercepted_boundaries']==[0x212010,0x16edd0,0x211da0]
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    for pc,value in ref['instructions']:assert image.read(pc,len(value)//2).hex()==value
    # Reuse the already-qualified, separate return recorder and its complete
    # defined FXSAVE checks; no original instruction is replaced by that code.
    template=root/'tools/verify_object_name_lookup_host_x86.py';text=template.read_text()
    host=text.split("    code=r'''",1)[1].split("'''.replace('CASES'",1)[0]
    prefix=host[:host.index('struct Case {')]
    assembly=host[host.index('extern void invoke_original'):host.index('int main(void){')]
    array=lambda values:'{'+','.join(str(v)+'u'for v in values)+'}'
    rows=[]
    for row in ref['cases']:
        calls=[]
        for call in row['calls']:
            stack=bytes.fromhex(call['stack']);words=[int.from_bytes(stack[n:n+4],'little')for n in range(0,28,4)]
            calls.append('{'+str(call['target'])+'u,'+array(call['registers'])+','+array(words)+'}')
        while len(calls)<3:calls.append('{0,{0},{0}}')
        rows.append('{'+','.join((array(row['initial_registers']),str(row['initial_flags'])+'u',
            array(row['registers']),str(row['eflags'])+'u',str(row['table'])+'u',str(row['graph'])+'u',
            str(row['name'])+'u',str((row['opcode']&65535)|(row['seed']<<16))+'u',
            str(len(row['calls']))+'u','{'+','.join(calls)+'}',
            '"'+''.join('\\x'+f'{v:02x}'for v in bytes.fromhex(row['stack_after']))+'"'))+'}')
    code=prefix+'\n#include <signal.h>\n#include <ucontext.h>\n#include <unistd.h>\n'+r'''
struct Boundary {uint32_t target,gp[8],stack[7];};
struct Case {uint32_t in[8],flags,out[8],out_flags,table,graph,name,opcode,calls;struct Boundary boundary[3];unsigned char stack[2049];};
static const struct Case cases[]={CASES};
static const unsigned char original[]={ORIGINAL};
static const unsigned char descriptor[]={DESCRIPTOR};
static const struct Case*current;
static unsigned calls,seed;
static void boundary(int signal,siginfo_t*info,void*state){
 (void)signal;(void)info;ucontext_t*ctx=state;greg_t*r=ctx->uc_mcontext.gregs;
 uint32_t pc=r[REG_EIP],sp=r[REG_ESP];
 if(calls>=current->calls){_exit(121);}
 const struct Boundary*b=&current->boundary[calls++];
 const unsigned regs[]={REG_EAX,REG_ECX,REG_EDX,REG_EBX,REG_ESP,REG_EBP,REG_ESI,REG_EDI};
 if(pc!=b->target||memcmp((void*)sp,b->stack,28))_exit(122);
 for(unsigned n=0;n<8;++n)if((uint32_t)r[regs[n]]!=b->gp[n])_exit(123);
 r[REG_EAX]=pc==0x212010?(current->calls==3?0x2103000:0):(0xabc00000u|seed);
 r[REG_ECX]=0xdef00000u|seed;r[REG_EDX]=0x98700000u|seed;
 r[REG_EIP]=*(uint32_t*)sp;r[REG_ESP]=sp+(pc==0x212010?20:pc==0x16edd0?12:4);
}
ASSEMBLY
int main(void){
 map(0x2b2000,4096);map(0x450000,0x60000);map(0x2100000,0x100000);map(0x3000000,4096);map(0x2200000,0x4000);
 map(0x211000,0x2000);map(0x16e000,4096);map(0x2300000,65536);
 memcpy((void*)0x2b27b0,original,sizeof(original));
 size_t recorder=(size_t)(capture_end-capture_begin);if(!recorder||recorder>4096)return 2;
 memcpy((void*)0x3000000,capture_begin,recorder);
 if(mprotect((void*)0x2b2000,4096,PROT_READ|PROT_EXEC)||mprotect((void*)0x3000000,4096,PROT_READ|PROT_EXEC))return 2;
 /* Protected execute boundaries must not place the host's signal frame
    in the original script stack whose exact ordinary bytes are checked. */
 stack_t signal_stack={0};signal_stack.ss_sp=(void*)0x2300000;signal_stack.ss_size=65536;
 if(sigaltstack(&signal_stack,0))return 2;
 struct sigaction action={0};action.sa_sigaction=boundary;action.sa_flags=SA_SIGINFO|SA_ONSTACK;sigemptyset(&action.sa_mask);
 if(sigaction(SIGSEGV,&action,0))return 2;
 unsigned char before_table[0x60000];
 for(seed=0;seed<sizeof(cases)/sizeof(cases[0]);++seed){
  const struct Case*c=&cases[seed];current=c;calls=0;memset((void*)0x2200000,0,0x4000);
  memcpy((void*)0x2200000,c->in,32);word(0x2200020,c->flags);word(0x2200900,0x2b27b0);
  memset((void*)0x450000,0xa7,sizeof(before_table));memcpy((void*)0x458e90,descriptor,sizeof(descriptor));word(c->table,0x458e90);
  memcpy(before_table,(void*)0x450000,sizeof(before_table));
  word(0x2103000,c->graph);word(0x2103004,c->name);
  for(unsigned n=0;n<2048;++n)((unsigned char*)0x21ff800)[n]=(unsigned char)(seed+n*7);
  word(0x21fff00,0x3000000);word(0x21fff04,c->opcode);word(0x21fff08,0x12340001);word(0x21fff0c,0x23450002);
  unsigned char*fx=(unsigned char*)0x2201200;half(0x2201200,0x27f);half(0x2201202,0x1800);fx[4]=0x38;word(0x2201218,0x1f80);
  const uint16_t exponents[3]={0x3ffe,0xbfff,0x4000};
  for(unsigned i=0;i<3;++i){uint64_t m=UINT64_C(0x8000000000000000);memcpy(fx+32+i*16,&m,8);memcpy(fx+40+i*16,&exponents[i],2);}
  for(unsigned i=0;i<8;++i)for(unsigned k=0;k<16;++k)fx[160+i*16+k]=(unsigned char)(seed+i+k);
  invoke_original();
  if(calls!=c->calls||memcmp((void*)0x2200100,c->out,32)||*(uint32_t*)0x2200120!=c->out_flags){fprintf(stderr,"Physical camera GP/flags/boundary mismatch case%u\n",seed);return 1;}
  const unsigned char*entered=(const unsigned char*)0x2201400,*out=(const unsigned char*)0x2200200;
  if(memcmp(fx,entered,5)||memcmp(fx+24,entered+24,4)||memcmp(fx+160,entered+160,128))return 2;
  for(unsigned i=0;i<3;++i)if(memcmp(fx+32+i*16,entered+32+i*16,10))return 2;
  if(memcmp(entered,out,5)||memcmp(entered+8,out+8,16)||memcmp(entered+24,out+24,4)||memcmp(entered+160,out+160,128)){fprintf(stderr,"Physical camera FP header/SIMD mismatch case%u\n",seed);return 1;}
  for(unsigned i=0;i<8;++i)if(memcmp(entered+32+i*16,out+32+i*16,10)){fprintf(stderr,"Physical camera x87 mismatch case%u\n",seed);return 1;}
  if(memcmp(before_table,(void*)0x450000,sizeof(before_table))||memcmp(c->stack,(void*)0x21ff800,2048)||
   *(uint32_t*)0x2103000!=c->graph||*(uint32_t*)0x2103004!=c->name||memcmp(original,(void*)0x2b27b0,sizeof(original))){
   fprintf(stderr,"Physical camera ordinary/code bank mismatch case%u table=%d stack=%d graph=%d name=%d code=%d\n",seed,
    memcmp(before_table,(void*)0x450000,sizeof(before_table)),memcmp(c->stack,(void*)0x21ff800,2048),
    *(uint32_t*)0x2103000!=c->graph,*(uint32_t*)0x2103004!=c->name,memcmp(original,(void*)0x2b27b0,sizeof(original)));return 1;}
 }
 puts("PASS unchanged physical original camera wrapper:32 cases, full GP/flags/stack/live FP/SIMD/banks and three explicit ABI boundaries");return 0;
}
'''
    code=code.replace('CASES',',\n'.join(rows)).replace('ORIGINAL',','.join(hex(v)for v in image.read(0x2b27b0,0x46)))
    code=code.replace('DESCRIPTOR',','.join(hex(v)for v in bytes.fromhex(ref['descriptor']))).replace('ASSEMBLY',assembly)
    source=owner/'original.c';source.write_text(code);exe=owner/'original'
    command=['gcc','-m32','-std=c11','-O2','-fno-pie','-no-pie','-fcf-protection=none',str(source),'-o',str(exe)]
    compile=subprocess.run(command,capture_output=True,text=True,timeout=120)
    run=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30)if compile.returncode==0 else None
    report=dict(scope=__doc__,passed=compile.returncode==0 and run is not None and run.returncode==0,
        cases=32,xbe_sha256=NEW_PIN,fixture_sha256=hashlib.sha256(packed).hexdigest(),
        shared_return_recorder_source_sha256=hashlib.sha256(template.read_bytes()).hexdigest(),
        harness_source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),compile_command=command,
        compile=dict(exit_code=compile.returncode,stdout=compile.stdout[-2000:],stderr=compile.stderr[-4000:]),
        run=None if run is None else dict(exit_code=run.returncode,stdout=run.stdout[-2000:],stderr=run.stderr[-4000:]),
        instruction_patches=[],intercepted_boundaries=ref['intercepted_boundaries'],engine_effects_qualified=False,
        owned_scratch=str(owner),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],compile=report['compile'],run=report['run'])))
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
