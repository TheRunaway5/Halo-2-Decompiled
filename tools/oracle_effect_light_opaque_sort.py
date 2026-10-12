#!/usr/bin/env python3
"""Whole unchanged13DA70/13DCD0/3BB80 opaque24-byte record sorting.

Controlled complete records test unsigned keys, partition thresholds and
unstable equal-key ordering.3BAB0/4CBF0 selection,layer5 and GPU are separate.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
BASE,STACK,STOP=0x2100000,0x22fff00,0x3000000
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def sha(v):return hashlib.sha256(v).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
 g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 g.mem_map(BASE,0x200000);g.mem_map(STOP,4096);executed={};calls={}
 def trace(vm,pc,size,_):
  if pc not in executed:
   raw=bytes(vm.mem_read(pc,size));assert raw==x.read(pc,size);executed[pc]=raw
  if pc in [0x13da70,0x13dcd0,0x3bb80]:calls[f'{pc:08x}']=calls.get(f'{pc:08x}',0)+1
 g.hook_add(UC_HOOK_CODE,trace)
 def call(entry,args):
  g.mem_write(STACK,pack([STOP,*args]));g.reg_write(R.UC_X86_REG_ESP,STACK);g.emu_start(entry,STOP,count=10000000);assert g.reg_read(R.UC_X86_REG_EIP)==STOP
 rng=random.Random(0x13da703bb80);cases=[]
 def add(n,kind):
  rows=[]
  for i in range(n):
   r=[rng.getrandbits(32)for _ in range(6)]
   if kind=='equal':r[4]=0x80000000;r[0]&=65535;r[1]&=0xffff00ff;r[2]&=0xffff0000
   if kind=='primary':r[4]=i
   if kind=='reverse':r[4]=n-i
   if kind=='duplicate':r[4]=rng.randrange(4);r[0]=(r[0]&65535)|(rng.randrange(4)<<16);r[1]=(r[1]&0xffff00ff)|(rng.randrange(4)<<8);r[2]=(r[2]&0xffff0000)|rng.randrange(4)
   rows.append(r)
  cases.append((kind,rows))
 for n in [0,1,2,3,7,8,9,15,16,31,32,63,64,65,127,512,1024,2560]:
  for kind in ['equal','primary','reverse','duplicate','random']:add(n,kind)
 # Adjacent unsigned boundaries on each consumed field; remaining payload is
 # independently random and must follow its exact source-sorted record.
 for field,values in [(4,[0,1,0x7fffffff,0x80000000,0xffffffff]),(0,[0,1,32767,32768,65535]),(1,[0,1,127,128,255]),(2,[0,1,32767,32768,65535])]:
  rows=[]
  for value in values:
   r=[rng.getrandbits(32)for _ in range(6)];r[4]=0;r[0]&=65535;r[1]&=0xffff00ff;r[2]&=0xffff0000
   r[field]|=value<<(16 if field==0 else 8 if field==1 else 0);rows.append(r)
  cases.append(('unsigned field boundary',rows))
 for i in range(256):add(rng.randrange(1,257),'duplicate'if i%2 else 'random')
 fixture=[0x5453504f,1,len(cases)];report_rows=[];checked=0;comparisons=0
 for label,rows in cases:
  n=len(rows);raw=pack([v for r in rows for v in r]);g.mem_write(BASE,raw)
  matrix=[]
  if n<=64:
   for i in range(n):
    for j in range(n):call(0x3bb80,[BASE+i*24,BASE+j*24,0]);matrix.append(g.reg_read(R.UC_X86_REG_EAX)&255)
  g.reg_write(R.UC_X86_REG_EAX,BASE);g.reg_write(R.UC_X86_REG_ECX,n);call(0x13da70,[24,0x3bb80,0]);after=words(g.mem_read(BASE,n*24));fixture.extend([n,*words(raw),len(matrix),*matrix,*after]);checked+=len(after);comparisons+=len(matrix);report_rows.append(dict(label=label,records=n))
 path=a.output/'fixture.bin';path.write_bytes(pack(fixture));report=dict(boundary=__doc__,original_xbe_sha256=PIN,intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=report_rows,checked_words=checked,comparator_calls=comparisons,calls=calls,fixture_sha256=sha(path.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(path.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');path.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
