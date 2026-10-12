#!/usr/bin/env python3
"""Whole unchanged13D370/13D830 physical-cache admission and ordered eviction.

All page selection, linked-list/bitmap/salt allocation and retirement bodies
execute unchanged. Busy/delete callbacks are declared recorded boundaries,
not original geometry callback ownership or actual IO/async chronology.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
NONE=0xffffffff;BASE,A,POOL,ROWS,BITMAP=0x71000000,0x71000100,0x71001000,0x71002000,0x71006000
BUSY,DELETED=0x12e210,0x12e290

def pack(v):return struct.pack('<'+'I'*len(v),*(x&NONE for x in v))
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def sha(v):return hashlib.sha256(v).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d';executed={}
 layouts=[[],[(0,4,1,0),(6,3,2,0),(12,4,3,0)],[(0,4,990,1),(4,4,992,0),(8,4,993,0),(12,4,994,1)],[(2,2,10,0),(7,3,850,0),(12,2,999,0)],[(0,4,1000,0),(4,4,1001,0),(8,4,NONE,0),(12,4,0x80000000,0)]]
 variants=[dict(policy=p,threshold=t,size=n,layout=k,shift=12,clock=1000,capacity=8,callbacks=1)
  for p in [0,1,2,255]for t in [0,1,5,8,9,NONE]for n in [0,1,4095,4096,8192,16384,65535,NONE,0x80000000]for k in range(len(layouts))]
 variants += [dict(policy=p,threshold=0,size=n,layout=k,shift=s,clock=c,capacity=4,callbacks=cb)
  for p,s,n,k,c,cb in [(0,0,7,1,1000,0),(1,4,31,2,1000,1),(2,31,NONE,3,1000,1),(0,12,4096,1,0,1),(1,12,8192,1,NONE,1),(2,12,16384,3,0x80000000,1)]]
 variants += [dict(policy=p,threshold=t,size=4096*n,layout=5,shift=12,clock=1000,capacity=512,callbacks=1)for p in [0,1,2]for t in [0,1,5]for n in [1,30,255,300]]
 variants += [dict(operation=1,policy=0,threshold=0,size=0,layout=k,shift=12,clock=1000,capacity=8,callbacks=cb,release=i,debug=d)for k in [1,2,3]for cb in [0,1]for i in [0,1,2]for d in [0,1]]
 fixture=[0x43504753,1,len(variants)];cases=[];total_events=0
 for v in variants:
  vm=Uc(UC_ARCH_X86,UC_MODE_32);vm.mem_map(0x10000,0x600000)
  for section in x.sections:vm.mem_write(section.va,x.section_bytes(section))
  vm.mem_map(0x70000000,0x100000);vm.mem_map(BASE,0x10000);rng=random.Random(0x13d370+len(cases));vm.mem_write(BASE,bytes(rng.getrandbits(8)for _ in range(0x10000)))
  stack,stop=0x700fe000,0x700fff00
  def val(at):return struct.unpack('<I',vm.mem_read(at,4))[0]
  def put(at,value):vm.mem_write(at,pack([value]))
  rows=layouts[v['layout']]if v['layout']<5 else [(i*2,1,i%100,0)for i in range(300)]
  tokens=[(0x8000+i)<<16|i for i in range(len(rows))];busy_mask=sum(b<<i for i,(_s,_n,_t,b)in enumerate(rows))
  put(A+0x20,DELETED if v['callbacks']else 0);put(A+0x24,BUSY if v['callbacks']else 0);put(A+0x2c,v['policy']);put(A+0x30,16 if v['layout']<5 else 1024);put(A+0x34,v['shift']);put(A+0x38,v['clock']);put(A+0x3c,tokens[0]if tokens else NONE);put(A+0x40,tokens[-1]if tokens else NONE)
  for i in range(9):put(A+0x44+i*4,0x7fffffff)
  put(A+0x64,POOL);put(POOL+0x20,v['capacity']);put(POOL+0x24,24);vm.mem_write(POOL+0x2a,bytes([8 if v.get('debug')else 0]));put(POOL+0x34,len(rows));put(POOL+0x38,len(rows));put(POOL+0x3c,len(rows));vm.mem_write(POOL+0x40,struct.pack('<H',0xfffe));put(POOL+0x44,ROWS);put(POOL+0x48,BITMAP)
  for i in range((v['capacity']+31)//32):put(BITMAP+i*4,0)
  for i in range(v['capacity']):vm.mem_write(ROWS+i*24,b'\0\0')
  for i,(start,n,time,busy)in enumerate(rows):
   vm.mem_write(ROWS+i*24,struct.pack('<HHIIIII',0x8000+i,0,n,start,tokens[i+1]if i+1<len(tokens)else NONE,tokens[i-1]if i else NONE,time));put(BITMAP+(i//32)*4,val(BITMAP+(i//32)*4)|(1<<(i%32)))
  initial=bytes(vm.mem_read(BASE,0x10000));events=[]
  def code(guest,pc,size,_):
   if pc not in executed:
    raw=bytes(guest.mem_read(pc,size));assert raw==x.read(pc,size),hex(pc);executed[pc]=raw
   if pc not in [BUSY,DELETED]:return
   sp=guest.reg_read(R.UC_X86_REG_ESP);ret=val(sp);datum=val(sp+4)
   if pc==BUSY:
    result=(busy_mask>>(datum&65535))&1;events.extend([1,pc,datum,result]);guest.reg_write(R.UC_X86_REG_EAX,result)
   else:events.extend([2,pc,datum])
   guest.reg_write(R.UC_X86_REG_ESP,sp+8);guest.reg_write(R.UC_X86_REG_EIP,ret)
  vm.hook_add(UC_HOOK_CODE,code);operation=v.get('operation',0)
  if operation:
   vm.mem_write(stack,pack([stop]));vm.reg_write(R.UC_X86_REG_EDI,A);vm.reg_write(R.UC_X86_REG_EBX,tokens[v['release']]);entry=0x13d830
  else:vm.mem_write(stack,pack([stop,A,v['size'],v['threshold']]));entry=0x13d370
  vm.reg_write(R.UC_X86_REG_ESP,stack);vm.reg_write(R.UC_X86_REG_EFLAGS,2)
  try:vm.emu_start(entry,stop,count=20000000);assert vm.reg_read(R.UC_X86_REG_EIP)==stop
  except Exception as e:raise RuntimeError(f'case{len(cases)} {v} at{vm.reg_read(R.UC_X86_REG_EIP):08x}: {e}')from e
  inputs=[operation,A,v['size'],v['threshold'],tokens[v['release']]if operation else 0,busy_mask&NONE,1,BASE,0x10000//4,*words(initial)]
  after=[0 if operation else vm.reg_read(R.UC_X86_REG_EAX),len(events),*events,*words(bytes(vm.mem_read(BASE,0x10000)))]
  fixture.extend([len(inputs),len(after),*inputs,*after]);total_events+=len(events);cases.append(dict(mutation=v,checked_words=len(after)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,original_xbe_sha256=sha(x.data),original_instruction_patches=[],executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),declared_service_boundaries=['12E210 busy callback','12E290 delete callback'],service_words=total_events,cases=cases,checked_words=sum(c['checked_words']for c in cases),fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
