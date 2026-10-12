#!/usr/bin/env python3
"""Whole original geometry request/acquisition/physical-cache/fixup join.

Original12DE70/12E3A0/13D370, actual cache callbacks, raw pool allocation/
retirement and223C70 execute unchanged on one bank.63 calls retain actual
linked headers from unchanged main2CBC0. Declared cold/pressure controls
supply initial allocator banks; no intermediate token/admission/fixup state
is injected. Routed IO, promotion, yield and pump are explicit recorded
boundaries. This does not establish live async chronology or GPU submission.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_UNMAPPED,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
NONE=0xffffffff
PIN='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
def pack(v):return struct.pack('<'+'I'*len(v),*(x&NONE for x in v))
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def sha(v):return hashlib.sha256(v).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);assert sha(s.ram)==PIN;x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
 stack,stop=0x700fe000,0x700fff00;executed={};requested=set()
 def read(vm,at,n):
  regions=list(vm.mem_regions())
  for page in range(at&~4095,(at+n+4095)&~4095,4096):
   if not any(b<=page<=e for b,e,_ in regions):vm.mem_map(page,4096);vm.mem_write(page,s.read(page,4096))
  return bytes(vm.mem_read(at,n))
 def val(vm,at):return u(read(vm,at,4))
 def write(vm,at,raw):read(vm,at,len(raw));vm.mem_write(at,raw)
 def unchanged(vm,pc,size,_):
  if pc not in executed:
   raw=bytes(vm.mem_read(pc,size));assert raw==x.read(pc,size),f'Original instruction differs at{pc:08x}';executed[pc]=raw
 def routing(vm):
  def segment(base,access):return struct.pack('<HHBBBB',65535,base&65535,(base>>16)&255,access,0xcf,(base>>24)&255)
  table=0x70030000;vm.mem_write(table,bytes(0x28));vm.mem_write(table+8,segment(0,0x9b)+segment(0,0x93));vm.mem_write(table+32,segment(0x80035bdc,0x93));vm.reg_write(R.UC_X86_REG_GDTR,(0,table,0x27,0))
  for register,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,32)]:vm.reg_write(register,value)
 def run(vm,entry,flags=0,header=0):
  vm.mem_write(stack,pack([stop,0]));vm.reg_write(R.UC_X86_REG_ESP,stack);vm.reg_write(R.UC_X86_REG_FPCW,0x023f);vm.reg_write(R.UC_X86_REG_MXCSR,0x1fa0);vm.reg_write(R.UC_X86_REG_EAX,flags);vm.reg_write(R.UC_X86_REG_ECX,header)
  vm.emu_start(entry,stop,count=20000000);assert vm.reg_read(R.UC_X86_REG_EIP)==stop,hex(vm.reg_read(R.UC_X86_REG_EIP));return vm.reg_read(R.UC_X86_REG_EAX)&255
 g=s.guest();g.mem_map(0x70000000,0x100000);routing(g);g.hook_add(UC_HOOK_CODE,unchanged)
 def requests(vm,pc,_size,_):
  if pc==0x12de70:requested.add(vm.reg_read(R.UC_X86_REG_ECX))
 g.hook_add(UC_HOOK_CODE,requests);write(g,0x4c0b80,pack([0x547f88]));run(g,0x2cbc0)
 actual=[h for h in sorted(requested)if read(g,h+0x1e,1)[0]and val(g,h+0x20)!=NONE]
 regions=[(b,e,p,bytes(g.mem_read(b,e-b+1)))for b,e,p in g.mem_regions()]
 def clone():
  vm=Uc(UC_ARCH_X86,UC_MODE_32)
  for b,e,p,raw in regions:vm.mem_map(b,e-b+1,p);vm.mem_write(b,raw)
  def missing(guest,_access,at,_n,_v,_d):
   page=at&~4095
   try:raw=s.read(page,4096)
   except Exception:return False
   guest.mem_map(page,4096);guest.mem_write(page,raw);return True
  vm.hook_add(UC_HOOK_MEM_UNMAPPED,missing);vm.hook_add(UC_HOOK_CODE,unchanged);return vm
 variants=[dict(label='actual linked resource',header=h,flags=f)for h in actual for f in [0,3,15]]
 variants += [dict(label='declared request policy',flags=f,policy=p,filter=z,group=k,disable=d,state='none',outcome=1,pumps=2)
   for f in range(16)for p in [-1,0,1,2]for z in [0,1,2]for k in [0x6d6f6465,0x73627370,0x6c746d70]for d in [0,1]]
 variants += [dict(label='declared slot completion',flags=f,state=st,outcome=o,pumps=n,promote_complete=q,priority=pr,index=i)
   for f in [0,1,2,3,4,7,8,15,0xffffff03]for st,o,n,q,pr,i in [
    ('none',0,0,0,0,0),('none',2,0,0,0,3),('pending',0,1,0,0,3),
    ('pending',0,3,0,255,1),('pending',0,0,1,0,0),('ready',0,0,0,0,3),('linked',0,0,0,0,1)]]
 variants += [dict(label='declared size and signed tag',flags=3,section=n,resource=b,tag=t,state='ready')for n,b,t in [(0,4,0),(-1,4,0),(1,-1,0),(128,0,-1),(128,0,1)]]
 variants += [dict(label='declared whole-cache pressure',flags=f,state='none',outcome=2,index=0,pressure=1,size=n,cache_policy=p)for f in [2,3,10]for p in [0,1,2]for n in [512,2048,4096,8192]]
 fixture=[0x43524753,1,len(variants)];cases=[];service_total=0
 for v in variants:
  synthetic='header'not in v;events=[];pump_count=0
  if synthetic:
   vm=Uc(UC_ARCH_X86,UC_MODE_32);vm.mem_map(0x10000,0x600000)
   for section in x.sections:vm.mem_write(section.va,x.section_bytes(section))
   vm.mem_map(0x70000000,0x100000);vm.mem_map(0x71000000,0x10000);vm.hook_add(UC_HOOK_CODE,unchanged)
   rng=random.Random(0x12de70+len(cases));vm.mem_write(0x71000000,bytes(rng.getrandbits(8)for _ in range(0x10000)))
   header,physical,pool,records,flight,flight_rows,tags,allocation,descriptor=0x71000100,0x71001000,0x71001200,0x71002000,0x71001400,0x71003000,0x71004010,0x71008000,0x71000500
   index=v.get('index',0);token=0x80000000|index;entry=flight_rows+index*12
   vm.mem_write(0x4e3b44,pack([tags]));vm.mem_write(0x4e649c,pack([physical]));vm.mem_write(0x4e648c,pack([flight]));vm.mem_write(0x4e6494,pack([allocation]));vm.mem_write(0x468c4c,bytes([v.get('disable',0)]));vm.mem_write(0x4e64a0,pack([v.get('policy',0),v.get('filter',0),0xfffffffe]));vm.mem_write(0x4e64b0,b'\0')
   vm.mem_write(header,pack([0x12345000,512]));vm.mem_write(header+8,pack([v.get('section',128),v.get('resource',0),1,descriptor]));vm.mem_write(header+0x18,struct.pack('<h',v.get('tag',0)));vm.mem_write(header+0x1c,struct.pack('<h',0x24));vm.mem_write(tags+v.get('tag',0)*16,pack([v.get('group',0x6d6f6465)]));vm.mem_write(header+0x20,pack([NONE if v.get('state')=='none'else token]));vm.mem_write(header+0x1e,bytes([v.get('state')=='linked']))
   vm.mem_write(descriptor,struct.pack('<BBHHHII',0,0,0,16,4,12,32));vm.mem_write(physical+0x34,pack([0,0xaabbccdd]));vm.mem_write(physical+0x64,pack([pool]));vm.mem_write(pool+0x44,pack([records]));vm.mem_write(flight+0x44,pack([flight_rows]));vm.mem_write(records+index*24+8,pack([0]));vm.mem_write(entry+2,bytes([v.get('state')=='ready',v.get('priority',7)]));vm.mem_write(entry+4,pack([0x87654321]));vm.mem_write(flight+0x20,pack([8,12]));vm.mem_write(flight+0x34,pack([0,0,0]));vm.mem_write(flight+0x40,struct.pack('<H',0x8005));vm.mem_write(flight+0x48,pack([0x71003800]));vm.mem_write(0x71003800,pack([0]));vm.mem_write(entry,b'\0\0')
   # Complete declared initial physical/inflight allocator banks. The entire
   # original cache chooses its actual slot/token/page; nothing is supplied at
   # the admission call boundary.
   def put(at,n):vm.mem_write(at,pack([n]))
   occupied=4 if v.get('pressure')else index+(v.get('state')!='none')
   for pool_at,rows_at,bitmap_at,stride in [(pool,records,0x71003810,24),(flight,flight_rows,0x71003800,12)]:
    put(pool_at+0x20,4 if v.get('pressure')else 8);put(pool_at+0x24,stride);vm.mem_write(pool_at+0x2a,b'\0');put(pool_at+0x34,occupied);put(pool_at+0x38,occupied);put(pool_at+0x3c,occupied);vm.mem_write(pool_at+0x40,struct.pack('<H',0x8005));put(pool_at+0x44,rows_at);put(pool_at+0x48,bitmap_at);put(bitmap_at,(1<<occupied)-1)
    for j in range(8):vm.mem_write(rows_at+j*stride,b'\0\0')
   put(physical+0x20,0x12e290);put(physical+0x24,0x12e210);put(physical+0x2c,v.get('cache_policy',0));put(physical+0x30,16 if v.get('outcome')else 0);put(physical+0x34,9);put(physical+0x38,1000)
   for j in range(9):put(physical+0x44+j*4,0x7fffffff)
   put(physical+0x64,pool);put(physical+0x3c,0x80000000 if occupied else NONE);put(physical+0x40,0x80000000|(occupied-1)if occupied else NONE)
   for j in range(occupied):
    vm.mem_write(records+j*24,struct.pack('<HHIIIII',0x8000,0,2 if v.get('pressure')else 1,j*(2 if v.get('pressure')else 1),0x80000000|(j+1)if j+1<occupied else NONE,0x80000000|(j-1)if j else NONE,990 if v.get('pressure')else 1000))
    if j!=index or v.get('pressure'):vm.mem_write(flight_rows+j*12,struct.pack('<HBBII',0x8000,1,7,0,0))
    else:vm.mem_write(flight_rows+j*12,struct.pack('<H',0x8000))
   put(header+4,v.get('size',512))
  else:vm=clone();header=v['header'];entry=0;token=NONE
  initial_pages={at:bytes(vm.mem_read(at,4096))for b,e,_ in vm.mem_regions()for at in range(b,e+1,4096)}
  def initial_read(at,n):
   out=b''
   while n:
    page=at&~4095;count=min(n,4096-(at&4095));raw=initial_pages.get(page)
    if raw is None:raw=s.read(page,4096)
    out+=raw[at-page:at-page+count];at+=count;n-=count
   return out
  ranges=[(0x71000000,0x71010000)]if synthetic else [(header-4,header+0x2c)]
  def access(_guest,_access,at,n,_value,_data):
   if not 0x70000000<=at<0x70100000:ranges.append((at&~3,(at+n+3)&~3))
  vm.hook_add(UC_HOOK_MEM_READ,access);vm.hook_add(UC_HOOK_MEM_WRITE,access)
  def service(guest,pc,_size,_data):
   nonlocal pump_count
   if pc not in [0x12e3a0,0x13d370,0x213760,0x120ce0,0x2d2084,0x125d60]:return
   assert synthetic,'Actual linked case unexpectedly reached service'
   sp=guest.reg_read(R.UC_X86_REG_ESP);ret=val(guest,sp);extra=0
   if pc==0x12e3a0:
    events.extend([1,val(guest,sp+4),guest.reg_read(R.UC_X86_REG_EAX)&255,val(guest,sp+8)&255,val(guest,sp+12)])
    return # Observe only: the entire original acquisition body executes.
   elif pc==0x13d370:
    events.extend([5,val(guest,sp+4),val(guest,sp+8),val(guest,sp+12)])
    return # Observe only: physical admission/callbacks/eviction execute unchanged.
   elif pc==0x213760:
    request=[guest.reg_read(R.UC_X86_REG_ECX),*(val(guest,sp+4+i*4)for i in range(6))];events.extend([6,*request])
    if v.get('outcome')==2:guest.mem_write(request[4],b'\1')
    guest.reg_write(R.UC_X86_REG_EAX,0x87654321);extra=24
   elif pc==0x120ce0:
    events.extend([2,guest.reg_read(R.UC_X86_REG_ESI),val(guest,sp+4)])
    if v.get('promote_complete'):guest.mem_write(val(guest,val(guest,0x4e648c)+0x44)+(val(guest,header+0x20)&65535)*12+2,b'\1')
    extra=4
   elif pc==0x2d2084:events.append(3)
   else:
    events.append(4);pump_count+=1
    if pump_count>=v.get('pumps',1):guest.mem_write(val(guest,val(guest,0x4e648c)+0x44)+(val(guest,header+0x20)&65535)*12+2,b'\1')
   guest.reg_write(R.UC_X86_REG_ESP,sp+4+extra);guest.reg_write(R.UC_X86_REG_EIP,ret)
  vm.hook_add(UC_HOOK_CODE,service)
  try:result=run(vm,0x12de70,v['flags'],header)
  except Exception as e:raise RuntimeError(f'Case{len(cases)} {v} at{vm.reg_read(R.UC_X86_REG_EIP):08x}: {e}')from e
  merged=[]
  for start,end in sorted(ranges):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  controls=[header,v['flags'],v.get('outcome',0),v.get('pumps',1),v.get('promote_complete',0),entry,token]
  inputs=[*controls,len(merged)];after=[result,len(events),*events]
  for start,end in merged:inputs.extend([start,(end-start)//4,*words(initial_read(start,end-start))]);after.extend(words(read(vm,start,end-start)))
  fixture.extend([len(inputs),len(after),*inputs,*after]);service_total+=len(events);cases.append(dict(mutation=v,checked_words=len(after),service_words=len(events)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),original_instruction_patches=[],executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),actual_requested_headers=len(requested),actual_linked_calls=len(actual)*3,declared_service_boundaries=['213760 routed IO','120CE0 job promotion','2D2084 yield','125D60 pump'],service_words=service_total,cases=cases,checked_words=sum(c['checked_words']for c in cases),fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
