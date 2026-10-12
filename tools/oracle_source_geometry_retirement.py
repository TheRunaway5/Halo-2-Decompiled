#!/usr/bin/env python3
"""Unchanged original geometry cache callbacks and physical-cache join.

Whole12E210/12E290/223D80/223E00 execute with original13D370/13D830
and raw datum retirement where reached. D3D busy/wait and platform mutex
operations are explicit recorded boundaries; declared completion progress
at those boundaries is not an original async timeline or live GPU proof.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
NONE=0xffffffff;BASE=0x71000000;A=BASE+0x1000;P=BASE+0x1200;F=BASE+0x1400;ROWS=BASE+0x2000;FLIGHT=BASE+0x3000;HEADERS=BASE+0x4000;DESC=BASE+0x5000;BACKING=BASE+0x8000

def pack(v):return struct.pack('<'+'I'*len(v),*(x&NONE for x in v))
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def sha(v):return hashlib.sha256(v).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d';executed={}
 variants=[dict(op=op,kind=k,selector=s,query=q,count=n,offset=z)for op in [0,1]for k in [0,1,2,3,255]for s in [0,2,65535]for q in [NONE,0,1,0x80000000]for n in [-1,0,1,3]for z in [0,NONE]]
 variants += [dict(op=2,age=age,done=d,linked=l,query=q)for age in [0,1,4,5,6,0x80000000,NONE]for d in [0,1]for l in [0,1]for q in [0,1,NONE]]
 variants += [dict(op=3,age=age,done=d,linked=l,query=q,nodes=n,progress=t,root=r,debug=b)for age in [0,1,5,NONE]for d,l,q,n,t,r,b in [(1,1,0,0,1,36,0),(1,1,1,3,1,36,0),(0,0,0,1,3,-8,0),(0,1,1,3,2,36,1),(1,0,0,0,1,-8,1)]]
 variants += [dict(op=3,header_null=1,debug=b)for b in [0,1]]
 variants += [dict(op=4,policy=p,threshold=t,size=n,done=d)for p in [0,1,2]for t in [0,1,5]for n in [256,1024,2048,4096]for d in [0,1]]
 fixture=[0x54524753,1,len(variants)];cases=[];service_words=0
 for v in variants:
  vm=Uc(UC_ARCH_X86,UC_MODE_32);vm.mem_map(0x10000,0x600000)
  for section in x.sections:vm.mem_write(section.va,x.section_bytes(section))
  vm.mem_map(0x70000000,0x100000);vm.mem_map(BASE,0x10000);rng=random.Random(0x12e210+len(cases));vm.mem_write(BASE,bytes(rng.getrandbits(8)for _ in range(0x10000)))
  stack,stop=0x700fe000,0x700fff00
  def val(at):return struct.unpack('<I',vm.mem_read(at,4))[0]
  def put(at,value):vm.mem_write(at,pack([value]))
  def byte(at,value):vm.mem_write(at,bytes([value&255]))
  put(0x4e649c,A);put(0x4e648c,F);put(0x4e6494,BACKING);put(0x4e035c,0x1234);put(0x4e28ec,BASE+0x6000 if v.get('nodes',0)else 0);put(0x4e28f0,0xaabbccdd)
  for i in range(v.get('nodes',0)):put(BASE+0x6000+i*64+0x3c,BASE+0x6000+(i+1)*64 if i+1<v['nodes']else 0)
  count=3 if v['op']==4 else 1;tokens=[(0x8000+i)<<16|i for i in range(count)]
  for pool,rows,bitmap,stride in [(P,ROWS,BASE+0x1800,24),(F,FLIGHT,BASE+0x1900,12)]:
   put(pool+0x20,8);put(pool+0x24,stride);byte(pool+0x2a,8 if v.get('debug')else 0);put(pool+0x34,count);put(pool+0x38,count);put(pool+0x3c,count);vm.mem_write(pool+0x40,struct.pack('<H',0x8005));put(pool+0x44,rows);put(pool+0x48,bitmap);put(bitmap,(1<<count)-1)
   for i in range(8):vm.mem_write(rows+i*stride,b'\0\0')
  put(A+0x20,0x12e290);put(A+0x24,0x12e210);put(A+0x2c,v.get('policy',0));put(A+0x30,16);put(A+0x34,8);put(A+0x38,1000);put(A+0x3c,tokens[0]);put(A+0x40,tokens[-1]);put(A+0x64,P)
  for i in range(9):put(A+0x44+i*4,0x7fffffff)
  put(A+0x64,P)
  for i,token in enumerate(tokens):
   page=[0,6,12][i];extent=[4,3,4][i];age=[10,1,0][i]if v['op']==4 else v.get('age',1);header=HEADERS+i*128;desc=DESC+i*128;data=BACKING+(page<<8)+8
   vm.mem_write(ROWS+i*24,struct.pack('<HHIIIII',token>>16,0,extent,page,tokens[i+1]if i+1<count else NONE,tokens[i-1]if i else NONE,(1000-age)&NONE))
   vm.mem_write(FLIGHT+i*12,struct.pack('<HBBII',token>>16,v.get('done',1),7,0x12345678,0 if v.get('header_null')else header))
   put(header+0x10,v.get('count',3));put(header+0x14,desc);vm.mem_write(header+0x1c,struct.pack('<h',v.get('root',36)));byte(header+0x1e,v.get('linked',1));put(header+0x20,token)
   for j in range(3):
    selector=v.get('selector',65535);locator=j*32;vm.mem_write(desc+j*16,struct.pack('<BBHHHII',v.get('kind',2),0,0,locator,selector,16,v.get('offset',0)))
    buffer=data+locator if selector==65535 else BASE+0x6800+i*0x100+selector*32
    if selector!=65535:put(data+locator+4,BASE+0x6800+i*0x100)
    put(buffer+0x10,BASE+0x7000+i*0x100+j*32)
  pages={at:bytes(vm.mem_read(at,4096))for b,e,_ in vm.mem_regions()for at in range(b,e+1,4096)};ranges=[(BASE,BASE+0x10000)];events=[];remaining=v.get('query',0);waits=0
  def access(_vm,_access,at,n,_value,_data):
   if not 0x70000000<=at<0x70100000:ranges.append((at&~3,(at+n+3)&~3))
  vm.hook_add(UC_HOOK_MEM_READ,access);vm.hook_add(UC_HOOK_MEM_WRITE,access)
  def code(guest,pc,size,_):
   nonlocal remaining,waits
   if pc not in executed:
    raw=bytes(guest.mem_read(pc,size));assert raw==x.read(pc,size),hex(pc);executed[pc]=raw
   if pc not in [0x3fb710,0x3fad50,0x2d63fa,0x2d62c4]:return
   sp=guest.reg_read(R.UC_X86_REG_ESP);ret=val(sp);extra=0
   if pc==0x3fb710:events.extend([1,guest.reg_read(R.UC_X86_REG_ECX),remaining]);guest.reg_write(R.UC_X86_REG_EAX,remaining)
   elif pc==0x3fad50:events.extend([2,guest.reg_read(R.UC_X86_REG_EAX)]);remaining=0
   elif pc==0x2d63fa:
    events.extend([3,val(sp+4),val(sp+8)]);extra=8;waits+=1
    if waits>=v.get('progress',1):
     for i in range(count):byte(FLIGHT+i*12+2,1)
   else:events.extend([4,val(sp+4)]);extra=4
   guest.reg_write(R.UC_X86_REG_ESP,sp+4+extra);guest.reg_write(R.UC_X86_REG_EIP,ret)
  vm.hook_add(UC_HOOK_CODE,code);op=v['op'];vm.mem_write(stack,pack([stop,BACKING if op==0 else tokens[0]]));vm.reg_write(R.UC_X86_REG_ESP,stack);vm.reg_write(R.UC_X86_REG_EFLAGS,2);vm.reg_write(R.UC_X86_REG_ESI,HEADERS);vm.reg_write(R.UC_X86_REG_EAX,BACKING)
  entry=[0x223d80,0x223e00,0x12e210,0x12e290,0x13d370][op]
  if op==4:vm.mem_write(stack,pack([stop,A,v['size'],v['threshold']]))
  try:vm.emu_start(entry,stop,count=2000000);assert vm.reg_read(R.UC_X86_REG_EIP)==stop
  except Exception as e:raise RuntimeError(f'case{len(cases)} {v} at{vm.reg_read(R.UC_X86_REG_EIP):08x}: {e}')from e
  merged=[]
  for start,end in sorted(ranges):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  inputs=[op,HEADERS,BACKING,tokens[0],A,v.get('size',0),v.get('threshold',0),v.get('query',0),v.get('progress',1),FLIGHT,count,len(merged)];after=[vm.reg_read(R.UC_X86_REG_EAX)&255 if op in [0,2]else(vm.reg_read(R.UC_X86_REG_EAX)if op==4 else 0),len(events),*events]
  for start,end in merged:
   initial=b''.join(pages[page][max(start-page,0):min(end-page,4096)]for page in range(start&~4095,end,4096));inputs.extend([start,(end-start)//4,*words(initial)]);after.extend(words(bytes(vm.mem_read(start,end-start))))
  fixture.extend([len(inputs),len(after),*inputs,*after]);service_words+=len(events);cases.append(dict(mutation=v,checked_words=len(after)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,original_xbe_sha256=sha(x.data),original_instruction_patches=[],executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),declared_service_boundaries=['3FB710 D3D busy','3FAD50 D3D wait','2D63FA platform mutex wait with declared completion progress','2D62C4 platform mutex release'],service_words=service_words,cases=cases,checked_words=sum(c['checked_words']for c in cases),fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
