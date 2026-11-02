"""Cross-check all BD790 references on unchanged physical i386 code.

Only this owned child maps the original code/data addresses. No instruction or
callee is patched; a separate return recorder captures GP/flags and FXSAVE.
Compiler, executable and raw banks are removed by the enclosing run_test.py.
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
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[1];owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.report.resolve().is_relative_to(owner)
    fixture=root/'tests/fixtures/object_name_lookup_tu15.json.zlib'
    reference=json.loads(zlib.decompress(fixture.read_bytes()))
    assert reference['xbe_sha256']==NEW_PIN and len(reference['cases'])==112
    assert reference['instruction_patches']==reference['intercepted_boundaries']==[]
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    encoded=image.read(0xbd790,28)
    for pc,value in reference['instructions']:assert image.read(pc,len(value)//2).hex()==value
    rows=[]
    for row in reference['cases']:
        array=lambda values:'{'+','.join(str(v)+'u'for v in values)+'}'
        rows.append('{'+array(row['initial_registers'])+','+str(row['initial_flags'])+'u,'+
            array(row['registers'])+','+str(row['eflags'])+'u,'+str(row['index'])+'u,'+str(row['slot'])+'u}')
    code=r'''
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
_Static_assert(sizeof(void*)==4,"Physical original requires i386");
struct Case {uint32_t in[8],flags,out[8],out_flags,index,value;};
static const struct Case cases[]={CASES};
static const unsigned char original[]={ORIGINAL};
extern void invoke_original(void);
extern unsigned char capture_begin[],capture_end[];
__asm__(
".text\n.globl invoke_original\ninvoke_original:\n"
"pushfl\npushal\nmovl %esp,0x2200904\nfxsave 0x2201000\nfxrstor 0x2201200\nfxsave 0x2201400\n"
"pushl 0x2200020\npopfl\n"
"movl 0x2200000,%eax\nmovl 0x2200004,%ecx\nmovl 0x2200008,%edx\nmovl 0x220000c,%ebx\n"
"movl 0x2200014,%ebp\nmovl 0x2200018,%esi\nmovl 0x220001c,%edi\nmovl 0x2200010,%esp\njmp *0x2200900\n"
".globl capture_begin\ncapture_begin:\n"
"movl %eax,0x2200100\nmovl %ecx,0x2200104\nmovl %edx,0x2200108\nmovl %ebx,0x220010c\n"
"movl %esp,0x2200110\nmovl %ebp,0x2200114\nmovl %esi,0x2200118\nmovl %edi,0x220011c\n"
"leal 0x2203000,%esp\npushfl\npopl 0x2200120\nfxsave 0x2200200\nfxrstor 0x2201000\n"
"cld\nmovl 0x2200904,%esp\npopal\npopfl\nret\n"
".globl capture_end\ncapture_end:\n");
static void map(uintptr_t at,size_t size){
 void*p=mmap((void*)at,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
 if(p==MAP_FAILED||p!=(void*)at){fprintf(stderr,"Owned i386 mapping %x failed: %s\n",(unsigned)at,strerror(errno));exit(2);}
}
static void word(uintptr_t at,uint32_t value){memcpy((void*)at,&value,4);}
static void half(uintptr_t at,uint16_t value){memcpy((void*)at,&value,2);}
int main(void){
 map(0xbd000,4096);map(0x4ea000,4096);map(0x2100000,0x100000);map(0x3000000,4096);map(0x2200000,0x4000);
 memcpy((void*)0xbd790,original,sizeof(original));
 const size_t recorder=(size_t)(capture_end-capture_begin);if(!recorder||recorder>4096)return 2;
 memcpy((void*)0x3000000,capture_begin,recorder);
 if(mprotect((void*)0xbd000,4096,PROT_READ|PROT_EXEC)||mprotect((void*)0x3000000,4096,PROT_READ|PROT_EXEC))return 2;
 unsigned char before[640*4],stack[256];
 for(unsigned n=0;n<sizeof(cases)/sizeof(cases[0]);++n){
  const struct Case*c=&cases[n];memset((void*)0x2200000,0,0x4000);
  memcpy((void*)0x2200000,c->in,32);word(0x2200020,c->flags);word(0x2200900,0xbd790);
  word(0x4eaba0,0x2108000);memset((void*)0x2108000,0xa7,sizeof(before));
  if(c->index<640)word(0x2108000+c->index*4,c->value);
  memset((void*)0x21ffe80,0x71,sizeof(stack));word(0x21fff00,0x3000000);
  memcpy(before,(void*)0x2108000,sizeof(before));memcpy(stack,(void*)0x21ffe80,sizeof(stack));
  unsigned char*fx=(unsigned char*)0x2201200;
  half(0x2201200,0x27f);half(0x2201202,0x1800);fx[4]=0x38;word(0x2201218,0x1f80);
  const uint16_t exponents[3]={0x3ffe,0xbfff,0x4000};
  for(unsigned i=0;i<3;++i){uint64_t m=UINT64_C(0x8000000000000000);memcpy(fx+32+i*16,&m,8);memcpy(fx+40+i*16,&exponents[i],2);}
  for(unsigned i=0;i<8;++i)for(unsigned k=0;k<16;++k)fx[160+i*16+k]=(unsigned char)(n+i+k);
  invoke_original();
  if(memcmp((void*)0x2200100,c->out,32)||*(uint32_t*)0x2200120!=c->out_flags){
   fprintf(stderr,"Physical original GP/flags mismatch case%u eax=%08x/%08x flags=%08x/%08x\n",n,*(uint32_t*)0x2200100,c->out[0],*(uint32_t*)0x2200120,c->out_flags);return 1;}
  const unsigned char*entered=(const unsigned char*)0x2201400;
  const unsigned char*out=(const unsigned char*)0x2200200;
  if(memcmp(fx,entered,5)||memcmp(fx+24,entered+24,4)||memcmp(fx+160,entered+160,128)){
   fprintf(stderr,"Physical original input FP/SIMD fixture differs case%u\n",n);return 1;}
  for(unsigned i=0;i<3;++i)if(memcmp(fx+32+i*16,entered+32+i*16,10))return 2;
  if(memcmp(entered,out,5)||memcmp(entered+8,out+8,16)||memcmp(entered+24,out+24,4)||memcmp(entered+160,out+160,128)){
   fprintf(stderr,"Physical original FP header/SIMD mismatch case%u\n",n);return 1;}
  for(unsigned i=0;i<8;++i)if(memcmp(entered+32+i*16,out+32+i*16,10)){
   fprintf(stderr,"Physical original x87 register mismatch case%u reg%u\n",n,i);return 1;}
  if(*(uint32_t*)0x4eaba0!=0x2108000||memcmp(before,(void*)0x2108000,sizeof(before))||memcmp(stack,(void*)0x21ffe80,sizeof(stack))||memcmp(original,(void*)0xbd790,sizeof(original))){
   fprintf(stderr,"Physical original changed input/code bank case%u\n",n);return 1;}
 }
 puts("PASS physical unchanged original object-name lookup:112 cases, GP/flags/RET, complete defined FP/SIMD and untouched banks; zero patched instructions/callees");return 0;
}
'''.replace('CASES',',\n'.join(rows)).replace('ORIGINAL',','.join(hex(v)for v in encoded))
    source=owner/'original.c';source.write_text(code);exe=owner/'original'
    command=['gcc','-m32','-std=c11','-O2','-fno-pie','-no-pie','-fcf-protection=none',str(source),'-o',str(exe)]
    compile=subprocess.run(command,capture_output=True,text=True,timeout=120)
    run=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30)if compile.returncode==0 else None
    report=dict(scope=__doc__,passed=compile.returncode==0 and run is not None and run.returncode==0,
        xbe_sha256=NEW_PIN,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        harness_source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),compile_command=command,
        compile=dict(exit_code=compile.returncode,stdout=compile.stdout[-2000:],stderr=compile.stderr[-4000:]),
        run=None if run is None else dict(exit_code=run.returncode,stdout=run.stdout[-2000:],stderr=run.stderr[-4000:]),
        instruction_patches=[],intercepted_boundaries=[],cases=112,owned_scratch=str(owner),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],compile=report['compile'],run=report['run'])))
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
