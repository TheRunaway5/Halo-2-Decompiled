#!/usr/bin/env python3
"""Whole unchanged133F70 joined to2CBF0/44940/2D000/449E0/30290.

Five collector inputs are produced by unchanged2CBC0/133720/133790/4B5D0
from the snapshot's actual retained main context. Controlled copies vary
filter, model/section counts, flags, selected sections, capacity, actual
weapon policy and source pointer/resource state. Every original callee runs.
Fixture2 supplies independent pre-call banks for native body/weapon lookups,
44940/2D000/449E0 and30290.12DE70 publication remains explicit; this does not
establish live4B5D0 production, model vertex binding or gameplay frames.
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
 stack,stop=0x700fe000,0x700fff00;executed={};collectors=[]
 def read(vm,at,n):
  regions=list(vm.mem_regions())
  for page in range(at&~4095,(at+n+4095)&~4095,4096):
   if not any(b<=page<=e for b,e,_ in regions):vm.mem_map(page,4096);vm.mem_write(page,s.read(page,4096))
  return bytes(vm.mem_read(at,n))
 def val(vm,at):return u(read(vm,at,4))
 def half(vm,at):return struct.unpack('<H',read(vm,at,2))[0]
 def write(vm,at,raw):read(vm,at,len(raw));vm.mem_write(at,raw)
 def unchanged(vm,pc,size,_):
  if pc not in executed:
   raw=bytes(vm.mem_read(pc,size));assert raw==x.read(pc,size),f'Original instruction differs at{pc:08x}';executed[pc]=raw
 def run(vm,entry,sp,args=(),regs=None):
  vm.mem_write(sp,pack([stop,*args]));vm.reg_write(R.UC_X86_REG_ESP,sp);vm.reg_write(R.UC_X86_REG_FPCW,0x023f);vm.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
  for register,value in (regs or {}).items():vm.reg_write(register,value)
  vm.emu_start(entry,stop,count=10000000);assert vm.reg_read(R.UC_X86_REG_EIP)==stop,hex(vm.reg_read(R.UC_X86_REG_EIP))
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
  if pc!=0x133f70:return
  pointer=vm.reg_read(R.UC_X86_REG_EAX);sp=vm.reg_read(R.UC_X86_REG_ESP)
  read(vm,pointer,0x1e4)
  collectors.append((pointer,sp,val(vm,sp+4),[(b,e,perms,bytes(vm.mem_read(b,e-b+1)))for b,e,perms in vm.mem_regions()]))
 g.hook_add(UC_HOOK_CODE,unchanged);g.hook_add(UC_HOOK_CODE,capture)
 write(g,0x4c0b80,pack([0x547f88]));run(g,0x2cbc0,stack,[0]);assert len(collectors)==5
 def state(vm,context):
  return [val(vm,at)for at in [0x4c0b7c,0x4c0b78,0x4c1528,0x4c19ac]]+words(read(vm,0x4ba138,850*32))+list(read(vm,0x4c0b84,850))+list(read(vm,0x4c0ed6,850))+words(read(vm,0x4c1228,192*4))+words(read(vm,0x4c152c,32*36))+[half(vm,context+0xa5c)]+words(read(vm,context+0x25c,2048))
 variants=[('actual main collector',dict(index=i))for i in range(5)]
 variants += [('owner filter byte',dict(index=1,filter=f))for f in [0,1,255,256,257,NONE]]
 variants += [('signed model count',dict(models=n))for n in [0,65535,1,2,3,4,5]]
 variants += [('signed section count',dict(sections=n))for n in [0,NONE,1,2,16]]
 variants += [('literal collector flags',dict(flags=f))for f in [1,2,4,8,16,32,64,128,0xfffffff0,0xffffffff]]
 variants += [('selected NONE section',dict(section_none=True))]
 variants += [('section and weapon policy',dict(index=i,kind=k,policy=v,override=True))for i in [0,1]for k in [0,1,3]for v in [0,1]]
 variants += [('capacity boundary',dict(count=n))for n in [849,850,851]]
 variants += [('actual null geometry pointer',dict(null_geometry=True))]
 variants += [('fade/region/model bytes',dict(model_bytes=True,override=True,kind=1,section=2))]
 variants += [('random retained allocation banks',dict(random=True,flags=0x44,override=True,kind=3,section=4))]
 layouts=[(0,0,480,640),(0,0,720,1280),(0,0,1080,1920),(0,0,239,426),(0,426,239,853),(239,0,479,426),(239,426,479,853),(-1,-3,241,319)]
 variants += [('section rectangle viewport',dict(viewport=layout,kind=3,section=2,override=True))for layout in layouts]
 variants += [('consumed section rectangle flags',dict(flags=f,kind=3,section=2,override=True))for f in [0,2,8,16,10,18,26,0xffffffff]]
 variants += [('global rectangle forcing',dict(gouter=outer,ginner=inner,kind=3,section=2,override=True))for outer,inner in [(1,0),(0,1),(1,1)]]
 # These actual models have no authored lightmapped-object reference bank.
 # Only the original NONE encodings are valid; instance0 would dereference an
 # absent bank in unchanged2D000, not exercise a valid collector state.
 variants += [('dynamic NONE instance bits',dict(instance=n))for n in [2047,65535]]
 rng=random.Random(0x133f70);fixture=[0x434f444f,2,len(variants)];cases=[]
 for label,v in variants:
  pointer,sp,argument,regions=collectors[v.get('index',0)];vm=clone(regions);context=val(vm,0x4c0b80);argument=v.get('filter',argument)
  if 'models'in v:
   # Additional rows are declared copies of the real first model, not new
   # owners or fabricated metadata. Each slot retains its original raw ABI.
   for i in range(1,min(v['models'],5)):
    for at,n,stride in [(4,4,4),(0x18,4,4),(0x48,4,4),(0x5c,4,4),(0x70,1,1),(0x2c,1,1),(0x75,16,16),(0xc6,32,32)]:write(vm,pointer+at+i*stride,read(vm,pointer+at,n))
   write(vm,pointer+0x166,struct.pack('<H',v['models']))
  if 'sections'in v:
   write(vm,pointer+0x18,pack([v['sections']]))
   # Repeat the actual first valid source section for capacity/order cases.
   if 0<v['sections']<=16:write(vm,pointer+0x75,read(vm,pointer+0x75,1)*v['sections'])
  if 'flags'in v:write(vm,pointer+0x170,pack([v['flags']]))
  if 'viewport'in v:write(vm,0x4b9dd0,struct.pack('<4h',*v['viewport']))
  if 'gouter'in v:write(vm,0x509410,bytes([v['gouter'],v['ginner']]))
  if 'instance'in v:write(vm,pointer+0x16a,struct.pack('<H',v['instance']))
  if 'kind'in v:write(vm,pointer+0x1c4,pack([v['kind']]))
  if 'section'in v:write(vm,pointer+0x1c0,pack([v['section']]))
  if v.get('override'):write(vm,pointer+0x1d0,pack([0xe1a2002d]))
  if v.get('section_none'):write(vm,pointer+0x75,b'\xff')
  if 'count'in v:write(vm,0x4c0b78,pack([v['count']]))
  if v.get('model_bytes'):
   write(vm,pointer+0x2c,b'\xd9');write(vm,pointer+0x70,b'\x01');write(vm,pointer+0x16c,b'\x01');write(vm,pointer+0x1bc,b'\x7b');write(vm,pointer+0x1be,b'\xa1\xff');write(vm,pointer+0x1d4,b'\x01');write(vm,pointer+0x1d8,pack([bits(.5)]));write(vm,pointer+0xc6,b'\xab\xcd')
  if v.get('random'):
   for at,n in [(0x4ba138,850*8),(0x4c152c,32*9),(0x4c1228,192),(context+0x25c,512)]:write(vm,at,pack([rng.getrandbits(32)for _ in range(n)]))
  model_count=struct.unpack('<h',read(vm,pointer+0x166,2))[0]
  contexts=sorted(set(val(vm,pointer+0x48+i*4)for i in range(max(0,model_count))))
  owners={};policies={};body_table=val(vm,val(vm,0x4e0300)+0x44);tags=val(vm,0x4e3b44)
  for token in contexts:
   owner=val(vm,token);body=val(vm,body_table+(owner&65535)*12+8);definition=val(vm,body);definition_pointer=val(vm,tags+(definition&65535)*16+8);kind=read(vm,body+0xaa,1)[0]
   if 'policy'in v:write(vm,definition_pointer+0x290,struct.pack('<H',v['policy']))
   owners[owner]=(kind,definition)
   if kind==2:policies[definition]=int(half(vm,definition_pointer+0x290)!=0)
  if v.get('null_geometry'):
   tag=val(vm,pointer+0x5c);model=val(vm,tags+(tag&65535)*16+8);section=read(vm,pointer+0x75,1)[0];write(vm,val(vm,model+0x28)+section*0x5c+0x34,pack([0]))
  def context_words():
   return [len(contexts),*[w for token in contexts for w in [token,*words(read(vm,token,0x78))]]]
  before=state(vm,context)+context_words();collector_before=words(read(vm,pointer,0x1e4));services=[];pending={};requests=[];resource_requests=[];resource_pending={};memory_reads=[]
  for owner in owners:
   body=val(vm,body_table+(owner&65535)*12+8);memory_reads.append((body,body+4))
  initial_pages={at:bytes(vm.mem_read(at,4096))for b,e,_ in vm.mem_regions()for at in range(b,e+1,4096)}
  def initial_read(at,n):
   out=b''
   while n:
    page=at&~4095;count=min(n,4096-(at&4095));raw=initial_pages.get(page)
    if raw is None:raw=s.read(page,4096)
    out+=raw[at-page:at-page+count];at+=count;n-=count
   return out
  def memory_read(_guest,_access,at,n,_value,_data):memory_reads.append((at&~3,(at+n+3)&~3))
  vm.hook_add(UC_HOOK_MEM_READ,memory_read)
  def calls(guest,pc,_size,_):
   if pc in resource_pending:resource_pending.pop(pc).append(guest.reg_read(R.UC_X86_REG_EAX)&255)
   if pc==0x12de70:
    esp=guest.reg_read(R.UC_X86_REG_ESP);row=[guest.reg_read(R.UC_X86_REG_EAX),guest.reg_read(R.UC_X86_REG_ECX)];resource_requests.append(row);resource_pending[val(guest,esp)]=row
   if pc in pending:
    entry,row=pending.pop(pc);result=guest.reg_read(R.UC_X86_REG_EAX)
    if entry==0x44940:
     row.extend([half(guest,result+0x14),half(guest,result+0x24),result]);memory_reads.extend([((result+offset)&~3,(result+offset+5)&~3)for offset in [0x14,0x24]])
    else:row.append(result)
    return
   if pc==0x2cbf0:
    esp=guest.reg_read(R.UC_X86_REG_ESP);requests.append([guest.reg_read(reg)for reg in [R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX]]+words(read(guest,esp+4,84)))
   if pc not in [0x44940,0x2d000,0x449e0]:return
   esp=guest.reg_read(R.UC_X86_REG_ESP);eax,ecx,edx=[guest.reg_read(reg)for reg in [R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX]]
   if pc==0x44940:row=[pc,eax&65535,val(guest,esp+4),*words(read(guest,0x4ba138+(eax&65535)*32,32))]
   elif pc==0x449e0:row=[pc,eax&65535,val(guest,esp+4),edx&255,*words(read(guest,0x4ba138+(eax&65535)*32,32))]
   else:row=[pc,eax,edx,ecx]
   services.append(row);pending[val(guest,esp)]=(pc,row)
  vm.hook_add(UC_HOOK_CODE,calls)
  #133F70 aligns ESP to8, subtracts214 and pushes3 registers before copy+38.
  temporary=(sp-4)&~7;temporary=temporary-0x214-12+0x38
  inputs=[pointer,temporary,argument,val(vm,context+0x2a78),context+0x25c,*read(vm,0x509410,2),*collector_before,*words(read(vm,0x4b9e14,192)),*words(read(vm,0x4b9da0,12)),*[half(vm,0x4b9dd0+i*2)for i in range(4)],len(owners),*[w for owner,(kind,definition)in sorted(owners.items())for w in [owner,kind,definition]],len(policies),*[w for definition,policy in sorted(policies.items())for w in [definition,policy]],*before]
  try:run(vm,0x133f70,sp,[argument],{R.UC_X86_REG_EAX:pointer})
  except Exception as error:raise RuntimeError(f'Case{len(cases)} {label} {v}, original PC{vm.reg_read(R.UC_X86_REG_EIP):08x}: {error}')from error
  assert not pending and not resource_pending
  assert words(read(vm,pointer,0x1e4))==collector_before
  after=state(vm,context)+context_words()
  inputs.append(len(services))
  for row in services:inputs.extend([len(row),*row])
  merged=[]
  for start,end in sorted(memory_reads):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  inputs.extend([val(vm,0x4e3b44),val(vm,0x4e0348),val(vm,0x4e0344),val(vm,0x4e0300),len(merged)])
  for start,end in merged:inputs.extend([start,(end-start)//4,*words(initial_read(start,end-start))])
  inputs.extend([len(resource_requests),*[w for row in resource_requests for w in row]])
  fixture.extend([len(inputs),len(after),*inputs,*after]);cases.append(dict(label=label,mutation=v,requests=len(requests),request_words=requests,checked_words=len(after),services=len(services),resource_requests=len(resource_requests)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),declared_main_context='00547f88',original_main_collectors=len(collectors),viewport_layouts=layouts,intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=cases,checked_words=sum(c['checked_words']for c in cases),requests=sum(c['requests']for c in cases),service_calls=sum(c['services']for c in cases),resource_requests=sum(c['resource_requests']for c in cases),fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
