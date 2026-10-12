#!/usr/bin/env python3
"""Execute whole unchanged133AB0 and compare native effect/child orchestration.

Actual133F70 and4B5D0 still execute in the reference. Native133F70 is joined
to2CBF0/44940/2D000/449E0/30290.4B5D0 output records and render contexts remain
an explicit callee boundary, not native upstream production or gameplay proof.
Native direct child/body/weapon/resource reads use independent pre-call memory.
Parent/counter/shared draw/mutable context words are compared. Temporary stack
identities are omitted from call comparison; all121 collector words stay exact.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_UNMAPPED,UC_HOOK_CODE,UC_HOOK_MEM_READ
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
  if pc!=0x133ab0:return
  sp=vm.reg_read(R.UC_X86_REG_ESP)
  if 0x133ab0<=val(vm,sp)<0x133f65:return
  pointer=val(vm,sp+4);read(vm,pointer,0x1e4)
  entries.append((pointer,sp,words(read(vm,sp+8,12)),[(b,e,perms,bytes(vm.mem_read(b,e-b+1)))for b,e,perms in vm.mem_regions()]))
 g.hook_add(UC_HOOK_CODE,unchanged);g.hook_add(UC_HOOK_CODE,capture)
 write(g,0x4c0b80,pack([0x547f88]));run(g,0x2cbc0,stack,[0]);assert len(entries)==5,len(entries)
 variants=[('actual main presentation',dict(index=i))for i in range(5)]
 variants += [('effect ordering and mutations',dict(flags=f,children=0))for f in [0,1,2,32,128,3,33,129,130,160,163,255,0xffffff00]]
 variants += [('already assigned section',dict(flags=f,assigned=1,children=0))for f in [2,128,130,163]]
 variants += [('low byte child argument',dict(children=n,flags=0))for n in [0,1,255,256,257,NONE]]
 variants += [('low byte skip argument',dict(skip=n,flags=163,children=0))for n in [0,1,255,256,257,NONE]]
 variants += [('signed model count',dict(models=n,children=0,flags=163))for n in [0,65535]]
 variants += [('first person child suppression',dict(first_person=1,flags=163))]
 variants += [('counter wrapping',dict(counter=n,flags=130,children=0))for n in [0,30,31,32,NONE]]
 variants += [('child owner and weapon policy',dict(index=i,flags=0,children=1,kind=k,policy=v))for i in [0,1,2,3,4]for k in [1,3]for v in [0,1]]
 variants += [('controlled links to actual retained weapon',dict(index=3,first_person=0,children=1,forced_child=True,flags=f,kind=k,policy=v))for f in [0,2,128,163]for k in [1,3]for v in [0,1]]
 variants += [('empty parent still visits child',dict(index=3,first_person=0,children=1,forced_child=True,models=n,flags=163,kind=1,policy=1))for n in [0,65535]]
 variants += [('child build low byte flags',dict(index=3,first_person=0,children=1,forced_child=True,flags=0,kind=1,policy=1,build_flags=n))for n in [0,1,255,256,257,NONE]]
 rng=random.Random(0x133ab0)
 variants += [('projected depth order',dict(flags=f,children=0,projection=[bits(rng.uniform(-100,100))for _ in range(9)]))for f in [2,128,130,163]]
 def state(vm,context):
  return [val(vm,at)for at in [0x4c0b7c,0x4c0b78,0x4c1528,0x4c19ac]]+words(read(vm,0x4ba138,850*32))+list(read(vm,0x4c0b84,850))+list(read(vm,0x4c0ed6,850))+words(read(vm,0x4c1228,192*4))+words(read(vm,0x4c152c,32*36))+[struct.unpack('<H',read(vm,context+0xa5c,2))[0]]+words(read(vm,context+0x25c,2048))
 fixture=[0x50524f53,2,len(variants)];cases=[]
 for label,v in variants:
  pointer,sp,args,regions=entries[v.get('index',0)];vm=clone(regions);context=val(vm,0x4c0b80);args=list(args)
  for field,key in enumerate(['children','skip','build_flags']):
   if key in v:args[field]=v[key]
  for key,at in [('flags',0x170),('kind',0x1c4)]:
   if key in v:write(vm,pointer+at,pack([v[key]]))
  for key,at in [('assigned',0x1cc),('first_person',0x16c)]:
   if key in v:write(vm,pointer+at,bytes([v[key]]))
  if 'models'in v:write(vm,pointer+0x166,struct.pack('<H',v['models']))
  # Supply the actual live counter pointer with a controlled counter value;
  # original effect metadata and all section selectors stay source inputs.
  counter=val(vm,pointer+0x1c8)
  if 'counter'in v:write(vm,counter,pack([v['counter']]))
  if 'projection'in v:
   projection=v['projection'];write(vm,pointer+0x194,pack(projection[:3]));write(vm,context+0x2a68,pack(projection[3:6]));write(vm,0x4b9dac,pack(projection[6:]))
  bodies=val(vm,val(vm,0x4e0300)+0x44);tags=val(vm,0x4e3b44)
  if v.get('forced_child'):
   # A declared isolated child link to an existing, fully retained weapon
   # body/model; this is not claimed as the original live attachment graph.
   child_pointer,_,_,child_regions=entries[0]
   child_vm=clone(child_regions);child=val(child_vm,child_pointer)
   parent_body=val(vm,bodies+(val(vm,pointer)&65535)*12+8)
   child_body=val(vm,bodies+(child&65535)*12+8)
   write(vm,parent_body+0x10,pack([child]));write(vm,child_body+0xc,pack([NONE,NONE]))
  if 'policy'in v:
   # Only existing source object bodies and definitions are modified.
   seen=set();queue=[val(vm,pointer)]
   while queue:
    owner=queue.pop()
    if owner==NONE or owner in seen:continue
    seen.add(owner);body=val(vm,bodies+(owner&65535)*12+8)
    if read(vm,body+0xaa,1)[0]==2:
     definition=val(vm,body);metadata=val(vm,tags+(definition&65535)*16+8);write(vm,metadata+0x290,struct.pack('<H',v['policy']))
    queue.extend([val(vm,body+0x10),val(vm,body+0xc)])
  initial_pages={at:bytes(vm.mem_read(at,4096))for b,e,_ in vm.mem_regions()for at in range(b,e+1,4096)}
  def initial_read(at,n):
   out=b''
   while n:
    page=at&~4095;count=min(n,4096-(at&4095));raw=initial_pages.get(page)
    if raw is None:raw=s.read(page,4096)
    out+=raw[at-page:at-page+count];at+=count;n-=count
   return out
  calls=[];pending={};ranges=[];counters={};contexts=set();drawing=False;requests=[];request_pending={};metadata_pending=set()
  before_state=state(vm,context)
  def memory_read(guest,_access,at,n,_value,_data):
   pc=guest.reg_read(R.UC_X86_REG_EIP)
   if drawing or 0x133ab0<=pc<0x133f65:ranges.append((at&~3,(at+n+3)&~3))
  vm.hook_add(UC_HOOK_MEM_READ,memory_read)
  def retain_context(guest,token):
   contexts.add(token)
   if token:
    owner=val(guest,token)
    if owner!=NONE:
     body=val(guest,bodies+(owner&65535)*12+8);ranges.append((body,body+4))
  def observe(guest,pc,_size,_):
   nonlocal drawing
   if pc==0x133de7:drawing=False
   if pc in metadata_pending:
    metadata_pending.remove(pc);metadata=guest.reg_read(R.UC_X86_REG_EAX)
    ranges.extend([(metadata+at,metadata+at+4)for at in [0x14,0x24]])
   if pc in request_pending:request_pending.pop(pc).append(guest.reg_read(R.UC_X86_REG_EAX)&255)
   if pc in pending:
    row,out=pending.pop(pc);row.extend(words(read(guest,out,0x1e4)))
    models=struct.unpack('<h',read(guest,out+0x166,2))[0]
    current=sorted(set(val(guest,out+0x48+i*4)for i in range(max(0,models))))
    row.append(len(current))
    for token in current:
     retain_context(guest,token);row.extend([token,*words(read(guest,token,0x78))])
   if pc in [0x133b59,0x133c4c]:
    token=val(guest,guest.reg_read(R.UC_X86_REG_EBX)+0x1c8);counters.setdefault(token,u(initial_read(token,4)))
   if pc==0x133e5c:
    body=guest.reg_read(R.UC_X86_REG_ECX);ranges.append((body,body+4))
   if pc==0x133f70:
    drawing=True;esp=guest.reg_read(R.UC_X86_REG_ESP);out=guest.reg_read(R.UC_X86_REG_EAX);calls.append([1,val(guest,esp+4)&255,*words(read(guest,out,0x1e4))])
    models=struct.unpack('<h',read(guest,out+0x166,2))[0]
    for i in range(max(0,models)):retain_context(guest,val(guest,out+0x48+i*4))
   if drawing and pc==0x44940:
    # Native44940 consumes both actual metadata halfwords even when this
    # allocator branch subsequently needs only the geometry class.
    metadata_pending.add(val(guest,guest.reg_read(R.UC_X86_REG_ESP)))
   if drawing and pc==0x12de70:
    esp=guest.reg_read(R.UC_X86_REG_ESP);row=[guest.reg_read(R.UC_X86_REG_EAX),guest.reg_read(R.UC_X86_REG_ECX)];requests.append(row);request_pending[val(guest,esp)]=row
   if pc==0x4b5d0:
    esp=guest.reg_read(R.UC_X86_REG_ESP);values=words(read(guest,esp+4,24));out=values[3]-4
    row=[2,values[0],values[1],values[2],values[5],guest.reg_read(R.UC_X86_REG_EDX),guest.reg_read(R.UC_X86_REG_ECX)&255,*words(read(guest,values[4],12)),*words(read(guest,out,0x1e4))]
    calls.append(row);pending[val(guest,esp)]=(row,out)
  vm.hook_add(UC_HOOK_CODE,observe)
  inputs=[*args,*words(read(vm,context+0x2a68,12)),*words(read(vm,0x4b9dac,12)),val(vm,context+0x2a78),*words(read(vm,pointer,0x1e4))]
  try:run(vm,0x133ab0,sp,[pointer,*args],{R.UC_X86_REG_ECX:context})
  except Exception as error:raise RuntimeError(f'Case{len(cases)} {label} {v}, PC{vm.reg_read(R.UC_X86_REG_EIP):08x}: {error}')from error
  assert not pending
  assert not request_pending
  assert not metadata_pending
  for token in contexts:ranges.append((token,token+0x78))
  inputs.extend([val(vm,context+0x2a78),context+0x25c,*read(vm,0x509410,2),*words(initial_read(0x4b9e14,192)),*words(initial_read(0x4b9da0,12)),*[struct.unpack('<H',initial_read(0x4b9dd0+i*2,2))[0]for i in range(4)],*before_state,len(contexts)])
  for token in sorted(contexts):inputs.extend([token,*words(initial_read(token,0x78))])
  inputs.extend([len(counters),*[w for token,value in sorted(counters.items())for w in [token,value]],val(vm,0x4e0300),val(vm,0x4e3b44)])
  inputs.extend([val(vm,0x4e0348),val(vm,0x4e0344)])
  merged=[]
  for start,end in sorted(ranges):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  inputs.append(len(merged))
  for start,end in merged:inputs.extend([start,(end-start)//4,*words(initial_read(start,end-start))])
  inputs.append(len(calls))
  for row in calls:inputs.extend([len(row),*row])
  inputs.extend([len(requests),*[w for row in requests for w in row]])
  after=words(read(vm,pointer,0x1e4))+[len(counters),*[w for token in sorted(counters)for w in [token,val(vm,token)]]]+state(vm,context)+[len(contexts),*[w for token in sorted(contexts)for w in [token,*words(read(vm,token,0x78))]]]
  fixture.extend([len(inputs),len(after),*inputs,*after]);cases.append(dict(label=label,mutation=v,collects=sum(row[0]==1 for row in calls),builds=sum(row[0]==2 for row in calls),checked_words=len(after)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),declared_main_context='00547f88',original_main_inputs=len(entries),intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=cases,checked_words=sum(c['checked_words']for c in cases),collects=sum(c['collects']for c in cases),builds=sum(c['builds']for c in cases),fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
