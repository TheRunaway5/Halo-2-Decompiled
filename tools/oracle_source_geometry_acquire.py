#!/usr/bin/env python3
"""Whole unchanged12E3A0/16B910 allocation binding and IO request construction.

Physical admission13D370 and routed IO213760 are explicit recorded boundaries.
The actual raw pool/body/header/buffer backing is shared; whole original datum
binding, resets, salts, bitmap and request stores execute unchanged. This does
not supply an original physical-cache eviction or async/worker chronology.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
NONE=0xffffffff
BASE,HEADER,PHYS,PHYS2,POOL,RECORDS,FLIGHT,ROWS,BITMAP,ALLOCATION=(0x71000000,0x71000100,0x71001000,0x71001100,0x71001200,0x71002000,0x71001400,0x71003000,0x71005000,0x71008000)
def pack(v):return struct.pack('<'+'I'*len(v),*(x&NONE for x in v))
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def sha(v):return hashlib.sha256(v).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d';executed={}
 variants=[dict(blocking=b,selective=s,priority=p,size=n,index=3,salt=0x8000,counter=0x8005)
  for b,s in [(0,0),(1,0),(0,1),(1,1)]for p in [0,1,2,5,7,255,256,0xffffffff,0x80000000]for n in [0,1,511,512,513,0xfffffe01,0xffffffff]]
 variants += [dict(blocking=0x100+b,selective=0xa500+s,priority=0x12345607,size=513,index=i,salt=z,counter=c,complete=1)
  for b,s,i,z,c in [(0,0,0,1,0xfffe),(1,0,31,0x7fff,0xffff),(0,1,32,0xffff,0x8000),(1,1,63,0x8000,0xfffe)]]
 variants += [dict(blocking=b,selective=s,priority=7,size=513,index=3,salt=0x8000,counter=0x8005,failure=1)for b,s in [(0,0),(1,0),(0,1),(1,1)]]
 variants += [dict(blocking=1,selective=0,priority=2,size=513,index=3,salt=0x8000,counter=0x8005,resize=1,swap=sw,complete=q)for sw in [0,1]for q in [0,1]]
 fixture=[0x41474753,1,len(variants)];cases=[];service_words=0
 for v in variants:
  vm=Uc(UC_ARCH_X86,UC_MODE_32);vm.mem_map(0x10000,0x600000)
  for section in x.sections:vm.mem_write(section.va,x.section_bytes(section))
  vm.mem_map(0x70000000,0x100000);vm.mem_map(BASE,0x10000);rng=random.Random(0x12e3a0+len(cases));vm.mem_write(BASE,bytes(rng.getrandbits(8)for _ in range(0x10000)))
  stack,stop=0x700fe000,0x700fff00;token=(v['salt']<<16)|v['index'];entry=ROWS+v['index']*12
  def val(at):return struct.unpack('<I',vm.mem_read(at,4))[0]
  def put(at,value):vm.mem_write(at,pack([value]))
  put(0x4e649c,PHYS);put(0x4e648c,FLIGHT);put(0x4e6494,ALLOCATION)
  put(HEADER,0xc1234567);put(HEADER+4,v['size']);vm.mem_write(HEADER+0x1e,b'\xa5');put(HEADER+0x20,NONE)
  for phys,shift in [(PHYS,12),(PHYS2,0)]:put(phys+0x34,shift);put(phys+0x64,POOL)
  put(POOL+0x44,RECORDS);put(RECORDS+v['index']*24+8,0)
  put(FLIGHT+0x20,64);put(FLIGHT+0x24,12);put(FLIGHT+0x34,7);put(FLIGHT+0x38,0);put(FLIGHT+0x3c,5);vm.mem_write(FLIGHT+0x40,struct.pack('<H',v['counter']));put(FLIGHT+0x44,ROWS);put(FLIGHT+0x48,BITMAP);vm.mem_write(entry,b'\0\0')
  pages={at:bytes(vm.mem_read(at,4096))for b,e,_ in vm.mem_regions()for at in range(b,e+1,4096)}
  ranges=[(BASE,BASE+0x10000)];events=[]
  def access(_vm,_access,at,n,_value,_data):
   if not 0x70000000<=at<0x70100000:ranges.append((at&~3,(at+n+3)&~3))
  vm.hook_add(UC_HOOK_MEM_READ,access);vm.hook_add(UC_HOOK_MEM_WRITE,access)
  def code(guest,pc,size,_):
   if pc not in executed:
    raw=bytes(guest.mem_read(pc,size));assert raw==x.read(pc,size),hex(pc);executed[pc]=raw
   if pc not in [0x13d370,0x213760]:return
   sp=guest.reg_read(R.UC_X86_REG_ESP);ret=val(sp)
   if pc==0x13d370:
    events.extend([1,val(sp+4),val(sp+8),val(sp+12)])
    if v.get('resize'):put(HEADER+4,1025)
    if v.get('swap'):put(0x4e649c,PHYS2)
    guest.reg_write(R.UC_X86_REG_EAX,NONE if v.get('failure')else token);extra=12
   else:
    request=[guest.reg_read(R.UC_X86_REG_ECX),*(val(sp+4+i*4)for i in range(6))];events.extend([2,*request])
    if v.get('complete'):guest.mem_write(request[4],b'\1')
    guest.reg_write(R.UC_X86_REG_EAX,0x98765432);extra=24
   guest.reg_write(R.UC_X86_REG_ESP,sp+4+extra);guest.reg_write(R.UC_X86_REG_EIP,ret)
  vm.hook_add(UC_HOOK_CODE,code);vm.mem_write(stack,pack([stop,HEADER,v['selective'],v['priority']]));vm.reg_write(R.UC_X86_REG_ESP,stack);vm.reg_write(R.UC_X86_REG_EAX,v['blocking']);vm.reg_write(R.UC_X86_REG_EFLAGS,2)
  try:vm.emu_start(0x12e3a0,stop,count=10000);assert vm.reg_read(R.UC_X86_REG_EIP)==stop
  except Exception as e:raise RuntimeError(f'case{len(cases)} {v} at{vm.reg_read(R.UC_X86_REG_EIP):08x}: {e}')from e
  merged=[]
  for start,end in sorted(ranges):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  controls=[HEADER,v['blocking'],v['selective'],v['priority'],NONE if v.get('failure')else token,0x98765432,v.get('complete',0),v.get('resize',0),v.get('swap',0)]
  inputs=[*controls,len(merged)];after=[vm.reg_read(R.UC_X86_REG_EAX)&255,len(events),*events]
  for start,end in merged:
   initial=b''.join(pages[page][max(start-page,0):min(end-page,4096)]for page in range(start&~4095,end,4096))
   inputs.extend([start,(end-start)//4,*words(initial)]);after.extend(words(bytes(vm.mem_read(start,end-start))))
  fixture.extend([len(inputs),len(after),*inputs,*after]);service_words+=len(events);cases.append(dict(mutation=v,checked_words=len(after)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,original_xbe_sha256=sha(x.data),original_instruction_patches=[],executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),declared_service_boundaries=['13D370 physical admission','213760 routed IO'],service_words=service_words,cases=cases,checked_words=sum(c['checked_words']for c in cases),fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
