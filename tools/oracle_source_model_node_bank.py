#!/usr/bin/env python3
"""Whole unchanged137800/1376C0 against the native raw node/palette owners.

Actual main4B5D0 inputs and declared current metadata/section/compound controls.
All original instructions execute unchanged. DeepBD610 and12DE70 remain ordered
explicit services; this does not establish live pose/publication or framebuffer
parity. Output includes every retained byte of the256-matrix node/region bank.
"""
import argparse,hashlib,json,struct,subprocess,sys
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
def bits(v):return u(struct.pack('<f',v))
def sha(v):return hashlib.sha256(v).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);assert sha(s.ram)==PIN;x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
 stack,stop=0x700fe000,0x700fff00;executed={};entries=[]
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
 def run(vm,entry,sp,args=(),regs=None):
  vm.mem_write(sp,pack([stop,*args]));vm.reg_write(R.UC_X86_REG_ESP,sp);vm.reg_write(R.UC_X86_REG_FPCW,0x023f);vm.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
  for register,value in (regs or {}).items():vm.reg_write(register,value)
  vm.emu_start(entry,stop,count=20000000);assert vm.reg_read(R.UC_X86_REG_EIP)==stop,hex(vm.reg_read(R.UC_X86_REG_EIP))
 def clone(regions):
  vm=Uc(UC_ARCH_X86,UC_MODE_32)
  for b,e,perms,raw in regions:vm.mem_map(b,e-b+1,perms);vm.mem_write(b,raw)
  def missing(guest,_access,at,_n,_v,_d):
   page=at&~4095
   try:raw=s.read(page,4096)
   except Exception:return False
   guest.mem_map(page,4096);guest.mem_write(page,raw);return True
  vm.hook_add(UC_HOOK_MEM_UNMAPPED,missing);vm.hook_add(UC_HOOK_CODE,unchanged);return vm
 g=s.guest();g.mem_map(0x70000000,0x100000)
 def capture(vm,pc,_size,_):
  if pc!=0x137800:return
  sp=vm.reg_read(R.UC_X86_REG_ESP);args=[vm.reg_read(R.UC_X86_REG_ECX),vm.reg_read(R.UC_X86_REG_EDX),*words(read(vm,sp+4,20))]
  read(vm,args[6],0x3044)
  entries.append((sp,args,[(b,e,perms,bytes(vm.mem_read(b,e-b+1)))for b,e,perms in vm.mem_regions()]))
 g.hook_add(UC_HOOK_CODE,unchanged);g.hook_add(UC_HOOK_CODE,capture)
 write(g,0x4c0b80,pack([0x547f88]));run(g,0x2cbc0,stack,[0]);assert entries
 variants=[('actual main model node bank',dict(index=i))for i in range(len(entries))]
 variants += [('authored LOD and first-person low byte',dict(lod=l,fp=f))for l in range(6)for f in [0,1,255,256,257]]
 variants += [('compact palette and NONE sections',dict(compact=c,none=n,lod=l))for c in [0,1]for n in [0,1]for l in range(6)]
 variants += [('resident request null gate',dict(cache_present=c))for c in [0,1]]
 variants += [('section no-map count',dict(no_map=n,compact=c))for n in [0,1,40,255]for c in [0,1]]
 variants += [('signed region count',dict(region_count=n,none=True))for n in [-1,0,1,2,16]]
 variants += [('declared compound palette',dict(compound=n,compact=c,group=g,weight=k))for n in [1,2]for c in [0,1]for g in [-128,-1,0,1,127]for k in [0,1,2]]
 variants += [('literal empty base count and signed fallback',dict(node_count=n,none=True,compact=0))for n in [-1,0,1]]
 variants += [('duplicate selected sections and current node maps',dict(map_count=n,compact=c,cache_present=r))for n in [0,1,2,4]for c in [0,1]for r in [0,1]]
 variants += [('deferred-node early return fields',dict(deferred_early=e,fp=f))for e in ['absent_locals','counter1','counter255','owner_NONE']for f in [0,1,255,256,257]]
 fixture=[0x4e424d53,1,len(variants)];cases=[]
 for label,v in variants:
  sp,args,regions=entries[v.get('index',0)];vm=clone(regions);args=list(args);owner,model,nodes,selected,lod,fp,out=args
  tags,objects=val(vm,0x4e3b44),val(vm,0x4e0300)
  m=val(vm,tags+(model&65535)*16+8)
  if 'lod'in v:args[4]=v['lod']
  if 'fp'in v:args[5]=v['fp']
  if 'compact'in v:
   flags=read(vm,m+4,1)[0];write(vm,m+4,bytes([(flags&~4)|(4 if v['compact']else 0)]))
  if v.get('none'):write(vm,selected,bytes([255])*16)
  if 'cache_present'in v:write(vm,0x4e6494,pack([val(vm,0x4e6494)if v['cache_present']else 0]))
  if 'region_count'in v:write(vm,m+0x1c,pack([v['region_count']]))
  if 'node_count'in v:write(vm,m+0x48,pack([v['node_count']]))
  if 'deferred_early'in v:
   if v['deferred_early']=='owner_NONE':args[0]=NONE
   else:
    body=val(vm,val(vm,objects+0x44)+(owner&65535)*12+8)
    if v['deferred_early']=='absent_locals':write(vm,body+0x112,struct.pack('<h',-1))
    else:write(vm,body+0x112,struct.pack('<h',0));write(vm,body+0xb3,bytes([1 if v['deferred_early']=='counter1'else 255]))
  if 'map_count'in v:
   sections=val(vm,m+0x28);valid=[i for i in range(val(vm,m+0x24))if val(vm,sections+i*0x5c+0x34)]
   assert valid;section=valid[0];write(vm,selected,bytes([section])*16)
   resident=val(vm,sections+section*0x5c+0x34);table=0x70024000
   write(vm,resident+0x64,pack([v['map_count'],table]));write(vm,table,bytes(i%val(vm,m+0x48)for i in range(4)))
  if 'no_map'in v:
   sections=val(vm,m+0x28)
   for i in range(val(vm,m+0x24)):
    at=sections+i*0x5c;resident=val(vm,at+0x34)
    if resident:write(vm,resident+0x64,pack([0]));write(vm,at+0x24,bytes([v['no_map']]))
  if 'compound'in v:
   # Declared group/weight arithmetic, independent of authored live poses.
   group,table=0x70022000,0x70023000;write(vm,m+0x38,pack([group]));write(vm,m+0x3c+args[4],struct.pack('<b',v['group']))
   write(vm,group+v['group']*12+4,pack([v['compound'],table]))
   for i in range(v['compound']):
    ids=bytes([0,1,2,3])if val(vm,m+0x48)>=4 else bytes([0,0,0,0])
    weights=[[.1,.2,.3],[0.,-0.,1.],[1.5,-.5,.25]][v['weight']]
    write(vm,table+i*16,ids+struct.pack('<3f',*weights))
  cache_present=val(vm,0x4e6494)!=0;owned=[(out,0x3044)]
  # Preserve every guard byte and all source-owned read/write ranges. Original
  # stack scratch is not replayed into any native output or callback result.
  initial_pages={at:bytes(vm.mem_read(at,4096))for b,e,_ in vm.mem_regions()for at in range(b,e+1,4096)}
  def initial_read(at,n):
   raw=b''
   while n:
    page=at&~4095;count=min(n,4096-(at&4095));data=initial_pages.get(page)
    if data is None:data=s.read(page,4096)
    raw+=data[at-page:at-page+count];at+=count;n-=count
   return raw
  ranges=[(at&~3,(at+n+3)&~3)for at,n in owned];events=[];pending={}
  def access(_guest,_access,at,n,_value,_data):ranges.append((at&~3,(at+n+3)&~3))
  vm.hook_add(UC_HOOK_MEM_READ,access);vm.hook_add(UC_HOOK_MEM_WRITE,access)
  def observe(guest,pc,_size,_):
   if pc in pending:
    row=pending.pop(pc);row.append(guest.reg_read(R.UC_X86_REG_EAX)&(255 if row[0]==0 else NONE))
   if pc==0x12de70:
    esp=guest.reg_read(R.UC_X86_REG_ESP);row=[0,guest.reg_read(R.UC_X86_REG_EAX),guest.reg_read(R.UC_X86_REG_ECX)]
    events.append(row);pending[val(guest,esp)]=row
   if pc==0xbd610:
    esp=guest.reg_read(R.UC_X86_REG_ESP);o,sel,n=words(read(guest,esp+4,12))
    body=val(guest,val(guest,objects+0x44)+(o&65535)*12+8)
    if struct.unpack('<h',read(guest,body+0x112,2))[0]!=-1 and (o==NONE or read(guest,body+0xb3,1)[0]==0):
     row=[1,o,sel,n];events.append(row);pending[val(guest,esp)]=row
  vm.hook_add(UC_HOOK_CODE,observe)
  try:run(vm,0x137800,sp,args[2:],{R.UC_X86_REG_ECX:args[0],R.UC_X86_REG_EDX:args[1]})
  except Exception as error:raise RuntimeError(f'Case{len(cases)} {label} {v}, PC{vm.reg_read(R.UC_X86_REG_EIP):08x}: {error}')from error
  assert not pending
  inputs=[*args,tags,objects,int(cache_present),len(owned),*[w for at,n in owned for w in [at,n//4]]]
  merged=[]
  for start,end in sorted(ranges):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  inputs.append(len(merged))
  for start,end in merged:inputs.extend([start,(end-start)//4,*words(initial_read(start,end-start))])
  inputs.append(len(events))
  for row in events:inputs.extend([len(row),*row])
  after=[w for at,n in owned for w in words(read(vm,at,n))]
  fixture.extend([len(inputs),len(after),*inputs,*after]);cases.append(dict(label=label,mutation=v,events=len(events),event_kinds=[sum(row[0]==k for row in events)for k in range(2)],checked_words=len(after)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),declared_main_context='00547f88',original_inputs=len(entries),intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=cases,checked_words=sum(c['checked_words']for c in cases),events=sum(c['events']for c in cases),event_kinds=[sum(c['event_kinds'][k]for c in cases)for k in range(2)],fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
