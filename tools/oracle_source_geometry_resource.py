#!/usr/bin/env python3
"""Whole unchanged223C70 resource fixups over actual and declared owned banks.

Actual ready allocations come from unchanged main2CBC0/12DE70 on the pinned
RAM image. Controls exercise all descriptor branches, signed division,
retained guards, absent offsets and pointer masking in a declared native bank.
Every original instruction executes unchanged. Allocation, IO/completion,
12DE70 linked/age state and GPU submission are not supplied by this fixture.
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
 def run(vm,entry,args,registers):
  vm.mem_write(stack,pack([stop,*args]));vm.reg_write(R.UC_X86_REG_ESP,stack);vm.reg_write(R.UC_X86_REG_FPCW,0x023f);vm.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
  for register,value in registers.items():vm.reg_write(register,value)
  vm.emu_start(entry,stop,count=20000000);assert vm.reg_read(R.UC_X86_REG_EIP)==stop,hex(vm.reg_read(R.UC_X86_REG_EIP))
 g=s.guest();g.mem_map(0x70000000,0x100000);routing(g);g.hook_add(UC_HOOK_CODE,unchanged)
 def requests(vm,pc,_size,_):
  if pc==0x12de70:requested.add(vm.reg_read(R.UC_X86_REG_ECX))
 g.hook_add(UC_HOOK_CODE,requests);write(g,0x4c0b80,pack([0x547f88]));run(g,0x2cbc0,[0],{})
 manager=val(g,0x4e649c);records=val(g,val(g,manager+0x64)+0x44);base=val(g,0x4e6494);shift=val(g,manager+0x34)&31
 candidates=[];groups={};unready=[]
 for header in sorted(requested):
  token=val(g,header+0x20)
  if not read(g,header+0x1e,1)[0] or token==NONE:unready.append(header);continue
  tag=struct.unpack('<h',read(g,header+0x18,2))[0];group=val(g,val(g,0x4e3b44)+tag*16)
  groups[group]=groups.get(group,0)+1
  if groups[group]<=8:
   allocation=(base+(val(g,records+(token&65535)*24+8)<<shift))&NONE
   read(g,allocation,12);candidates.append(('actual ready allocation',dict(header=header,allocation=allocation,group=group)))
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
 variants=list(candidates)
 variants += [('declared descriptor/division/mask bank',dict(kind=k,stride=n,negative=z,absent=o))for k in [0,1,2,3,255]for n in [1,7,20,255]for z in [0,1]for o in [0,1]]
 variants += [('indexed vertex buffer bank',dict(kind=2,stride=n,index=i))for n in [1,7,20,255]for i in [0,1,3]]
 variants += [('signed descriptor count and root offset',dict(kind=0,stride=20,count=n,root=r))for n in [0,NONE,0x80000000]for r in [-4,0x24]]
 fixture=[0x52504753,1,len(variants)];cases=[]
 for label,v in variants:
  vm=clone();synthetic='header'not in v
  if synthetic:
   native=0x71000000;vm.mem_map(native,0x10000);rng=random.Random(0x223c70+len(cases));vm.mem_write(native,bytes(rng.getrandbits(8)for _ in range(0x10000)))
   header,allocation,descriptor=native+0x100,native+0x4000,native+0x1000
   write(vm,header+8,pack([0x100,0,v.get('count',1),descriptor]));write(vm,header+0x1c,struct.pack('<h',v.get('root',0x24)))
   offset=NONE if v.get('absent')else 0x200;size=0xffffffe7 if v.get('negative')else 103
   locator=0x200 if v['kind']>=2 else 0x10;selector=v.get('index',65535)if v['kind']>=2 else v['stride']
   if 'index'in v:locator=0x40;write(vm,allocation+8+locator+4,pack([allocation+0x800]));buffer=allocation+0x800+v['index']*32
   else:buffer=allocation+8+locator
   write(vm,descriptor,struct.pack('<BBHHHII',v['kind'],0,0,locator,selector,size,offset))
   if v['kind']>=2:
    write(vm,buffer+1,bytes([v['stride']]));write(vm,buffer+4,pack([0xe1234567,7]))
  else:header,allocation=v['header'],v['allocation']
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
  try:run(vm,0x223c70,[allocation],{R.UC_X86_REG_EDI:header})
  except Exception as e:raise RuntimeError(f'Case{len(cases)} {label} {v} at{vm.reg_read(R.UC_X86_REG_EIP):08x}: {e}')from e
  merged=[]
  for start,end in sorted(ranges):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  inputs=[header,allocation,len(merged)];after=[]
  for start,end in merged:inputs.extend([start,(end-start)//4,*words(initial_read(start,end-start))]);after.extend(words(read(vm,start,end-start)))
  fixture.extend([len(inputs),len(after),*inputs,*after]);cases.append(dict(label=label,mutation=v,checked_words=len(after)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),actual_requested_headers=len(requested),actual_ready_groups={f'{k:08x}':n for k,n in groups.items()},actual_unready_headers=[f'{n:08x}'for n in unready],actual_fixups=len(candidates),cases=cases,checked_words=sum(c['checked_words']for c in cases),fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),retained_fs_base='80035bdc')
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
