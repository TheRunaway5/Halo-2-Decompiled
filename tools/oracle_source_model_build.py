#!/usr/bin/env python3
"""Whole unchanged4B5D0 joined to native admission/cache/resources/nodes/arena.

Actual main model inputs and declared metric/view/capacity/FP controls. Every
original callee executes unchanged; no instruction is intercepted. Explicit
ordered12DE70/BAB40/deepBD610/3D480/23690 services supply consumed results and
deep-owned writes. Native3DB00/3DFE0 owns lighting routing/cache chronology;
D1850/D4080/BA1D0/3E5A0 supply their consumed leaf outputs. Native builds model
output/cache/arena banks; no4B5D0 records or lighting cache writes are replayed.
Live pose/publication/query/conversion/submission remain separate.
"""
import argparse,hashlib,json,re,struct,subprocess,sys
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
 registers=(a.snapshot/'registers.txt').read_text();fs_base=int(re.search(r'^fs_base\s+0x([0-9a-f]+)',registers,re.M)[1],16);assert fs_base==0x80035bdc
 stack,stop=0x700fe000,0x700fff00;executed={};entries=[]
 def read(vm,at,n):
  regions=list(vm.mem_regions())
  for page in range(at&~4095,(at+n+4095)&~4095,4096):
   if not any(b<=page<=e for b,e,_ in regions):vm.mem_map(page,4096);vm.mem_write(page,s.read(page,4096))
  return bytes(vm.mem_read(at,n))
 def val(vm,at):return u(read(vm,at,4))
 def write(vm,at,raw):read(vm,at,len(raw));vm.mem_write(at,raw)
 def routing(vm):
  # Emulator segment routing preserves the actual captured FS base. No
  # kernel/thread data or original instruction is replaced.
  def segment(base,access):return struct.pack('<HHBBBB',0xffff,base&65535,(base>>16)&255,access,0xcf,(base>>24)&255)
  table=0x70030000;vm.mem_write(table,bytes(0x28));vm.mem_write(table+8,segment(0,0x9b)+segment(0,0x93));vm.mem_write(table+0x20,segment(fs_base,0x93))
  vm.reg_write(R.UC_X86_REG_GDTR,(0,table,0x27,0))
  for register,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,32)]:vm.reg_write(register,value)
  assert vm.reg_read(R.UC_X86_REG_FS_BASE)==fs_base
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
  vm.hook_add(UC_HOOK_MEM_UNMAPPED,missing);vm.hook_add(UC_HOOK_CODE,unchanged);routing(vm);return vm
 g=s.guest();g.mem_map(0x70000000,0x100000);routing(g)
 def capture(vm,pc,_size,_):
  if pc!=0x4b5d0:return
  sp=vm.reg_read(R.UC_X86_REG_ESP);args=words(read(vm,sp+4,24))
  read(vm,args[3],0x1e4);read(vm,sp-0x3408,0x3400)
  entries.append((sp,args,vm.reg_read(R.UC_X86_REG_ECX),vm.reg_read(R.UC_X86_REG_EDX),[(b,e,perms,bytes(vm.mem_read(b,e-b+1)))for b,e,perms in vm.mem_regions()]))
 g.hook_add(UC_HOOK_CODE,unchanged);g.hook_add(UC_HOOK_CODE,capture)
 write(g,0x4c0b80,pack([0x547f88]));run(g,0x2cbc0,stack,[0]);assert entries
 variants=[('actual main model build',dict(index=i))for i in range(len(entries))]
 variants += [('authored geometry metric and view',dict(metric=bits(n),view=v))for n in [0.,1.,4.,8.,30.]for v in [0,1,2,3]]
 variants += [('caller render mode',dict(mode=n))for n in [0,1,255,256]]
 variants += [('force world low byte',dict(force=n))for n in [0,1,255,256,257]]
 variants += [('geometry arena capacity',dict(cursor=n))for n in [0,0x26fc0,0x27000,0x27001]]
 variants += [('first person model chain',dict(fp=True,fp_count=n,force=f))for n in [0,1,2]for f in [0,1,256]]
 variants += [('joined model cache allocation',dict(cache_mode=c,view=i))for c in ['empty','hole','pressure']for i in range(4)]
 variants += [('current geometry token reuse',dict(reuse=True,view=i,metric=bits(4.)))for i in range(4)]
 fixture=[0x42444d53,2,len(variants)];cases=[]
 for label,v in variants:
  sp,args,force,mode,regions=entries[v.get('index',0)];vm=clone(regions);args=list(args);owner,metric,lighting,out,center,rebase=args
  tags,objects,pool=val(vm,0x4e3b44),val(vm,0x4e0300),val(vm,0x509434)
  records,bitmap=val(vm,pool+0x44),val(vm,pool+0x48);scratch=sp-0x3408
  if 'metric'in v:args[1]=v['metric']
  if 'view'in v:write(vm,0x4ba050,pack([v['view']]))
  if 'mode'in v:mode=v['mode']
  if 'force'in v:force=v['force']
  if 'cursor'in v:write(vm,0x487b18,pack([v['cursor']]))
  if v.get('fp'):write(vm,0x4c1bd0,pack([owner]));write(vm,0x4c1bd4,struct.pack('<h',v['fp_count']))
  object_body=val(vm,val(vm,objects+0x44)+(owner&65535)*12+8)
  if 'cache_mode'in v:
   # Preserve every existing live root-owner cache used by3DB00. Filling
   # pressure slots must not invalidate those objects' retained cache tokens.
   active=words(read(vm,bitmap,32));write(vm,object_body+0xcc,pack([NONE]))
   if v['cache_mode']in ['hole','pressure']:
    occupied=[NONE]*8
    if v['cache_mode']=='hole':
     assert not(active[77//32]&(1<<(77&31)));occupied[77//32]^=1<<(77&31)
    write(vm,bitmap,pack(occupied));write(vm,pool+0x34,pack([37 if v['cache_mode']=='hole'else 256,256,sum(n.bit_count()for n in occupied)]))
    for i in range(256):
     if not(active[i//32]&(1<<(i&31))):write(vm,records+i*256,pack([0x5a5a0000|((0xe163+i)&65535),owner^0x10000,0x12345678,(val(vm,0x4ba034)-1001)&NONE]))
  if v.get('reuse'):
   token=val(vm,object_body+0xcc);assert token!=NONE
   cached=records+(token&65535)*256;view=val(vm,0x4ba050)
   write(vm,cached+0xbc+view,bytes([2]));write(vm,cached+0x4c,pack([0x100,(val(vm,0x4ba034)&0x3fffffff)|(view<<30)]))
  if label!='actual main model build':
   # Declared synchronous repeated-resident controls: use a genuinely
   # published section's resource header, payload and node counts within
   # each model. This is not authored LOD geometry or live IO publication.
   def metadata(tag):return val(vm,tags+(tag&65535)*16+8)
   object_body=val(vm,val(vm,objects+0x44)+(owner&65535)*12+8)
   model_tags=[val(vm,metadata(val(vm,metadata(val(vm,object_body))+0x38))+4)]
   if v.get('fp'):model_tags += [val(vm,0x4c1bd8+i*0xd0c)for i in range(v['fp_count'])]
   for tag in set(model_tags):
    m=metadata(tag);table=val(vm,m+0x28);n=val(vm,m+0x24)
    published=[table+i*0x5c for i in range(n)if val(vm,table+i*0x5c+0x34)and read(vm,table+i*0x5c+0x38+0x1e,1)!=b'\0']
    assert published,f'No published section for declared model{tag:08x}'
    source=published[0];node_fields=read(vm,source+0x24,2);payload=read(vm,source+0x34,4);header=read(vm,source+0x38,0x24)
    for i in range(n):
     at=table+i*0x5c;write(vm,at+0x24,node_fields);write(vm,at+0x34,payload);write(vm,at+0x38,header)
  # Native temporary bytes occupy caller-owned scratch; this is independent
  # input storage, not original local-stack output replay.
  read(vm,scratch,0x3400)
  owned=[(out,0x1e4),(records,256*256),(pool,0x4c),(bitmap,32),(0x487b18,4),(0x487b1c,0x27000)]
  headers=val(vm,objects+0x44);body=val(vm,headers+(owner&65535)*12+8);owned.append((body+0xcc,4))
  for at,n in owned:read(vm,at,n)
  initial_pages={at:bytes(vm.mem_read(at,4096))for b,e,_ in vm.mem_regions()for at in range(b,e+1,4096)}
  def initial_read(at,n):
   raw=b''
   while n:
    page=at&~4095;count=min(n,4096-(at&4095));data=initial_pages.get(page)
    if data is None:data=s.read(page,4096)
    raw+=data[at-page:at-page+count];at+=count;n-=count
   return raw
  ranges=[(at&~3,(at+n+3)&~3)for at,n in owned]+[(scratch,scratch+0x3400)];events=[];pending={};suspended=[False];service_state=[None];service_words=[0]
  def access(_guest,_access,at,n,_value,_data):
   ranges.append((at&~3,(at+n+3)&~3))
  def writes(_guest,_access,at,n,_value,_data):
   if service_state[0]is not None:
    for word in range(at&~3,(at+n+3)&~3,4):
     if any(base<=word and word+4<=base+extent for base,extent in owned):service_state[0].add(word)
  vm.hook_add(UC_HOOK_MEM_READ,access);vm.hook_add(UC_HOOK_MEM_WRITE,access)
  vm.hook_add(UC_HOOK_MEM_WRITE,writes)
  def observe(guest,pc,_size,_):
   if pc in pending:
    row,detail=pending.pop(pc);eax=guest.reg_read(R.UC_X86_REG_EAX)
    if row[0]==6:row.extend(words(read(guest,eax,84)))
    elif row[0]==4:row.extend([detail[1]//4,*words(read(guest,detail[0],detail[1]))])
    elif row[0]==7:row.extend([eax&255,*words(read(guest,detail[0],20))])
    elif row[0]in [8,9,10]:row.extend(words(read(guest,detail[0],detail[1])))
    else:row.append(eax&(NONE if row[0]in [1,3]else 255))
    if row[0]==2:row.append(val(guest,detail[0]))
    if row[0]in [3,5]:
     patches=sorted(service_state[0]);row.append(len(patches))
     for at in patches:row.extend([at,val(guest,at)])
     service_words[0]+=len(patches);service_state[0]=None
    if row[0]in [3,4,5,7,8,9,10]:suspended[0]=False
   if suspended[0]:return
   esp=guest.reg_read(R.UC_X86_REG_ESP);row=None;detail=[]
   if pc==0x12de70:row=[0,guest.reg_read(R.UC_X86_REG_EAX),guest.reg_read(R.UC_X86_REG_ECX)]
   elif pc==0x3ddd0:row=[1,val(guest,esp+4)]
   elif pc==0xbab40:row=[2,val(guest,esp+4),val(guest,esp+8)];detail=[val(guest,esp+12)]
   elif pc==0xbd610:
    o,sel,n=words(read(guest,esp+4,12));b=val(guest,headers+(o&65535)*12+8)
    if struct.unpack('<h',read(guest,b+0x112,2))[0]!=-1 and (o==NONE or read(guest,b+0xb3,1)[0]==0):row=[3,o,sel,n]
   elif pc==0x3d480:
    n,count,dst=words(read(guest,esp+4,12));row=[4,guest.reg_read(R.UC_X86_REG_EAX),count,n,dst];detail=[dst,(count if count<0x80000000 else 0)*52]
   elif pc==0x23690:row=[5,*words(read(guest,esp+4,28))]
   elif pc==0x3db00:row=[6,guest.reg_read(R.UC_X86_REG_EAX),val(guest,esp+4),esp-20]
   elif pc==0xd1850:
    out=val(guest,esp+8);row=[7,guest.reg_read(R.UC_X86_REG_EDI),val(guest,esp+4)&255,*words(read(guest,out,20))];detail=[out,20]
   elif pc==0xd4080:
    out=guest.reg_read(R.UC_X86_REG_ESI);row=[8,guest.reg_read(R.UC_X86_REG_ECX),val(guest,esp+4)&255,*words(read(guest,guest.reg_read(R.UC_X86_REG_EAX),20)),*words(read(guest,out,84))];detail=[out,84]
   elif pc==0xba1d0:row=[9,guest.reg_read(R.UC_X86_REG_EAX)];detail=[guest.reg_read(R.UC_X86_REG_ECX),12]
   elif pc==0x3e5a0:
    out=guest.reg_read(R.UC_X86_REG_ESI);row=[10,val(guest,esp+4),*words(read(guest,guest.reg_read(R.UC_X86_REG_EAX),12)),*words(read(guest,out,12))];detail=[out,12]
   if row is not None:
    events.append(row);pending[val(guest,esp)]=(row,detail)
    if row[0]in [3,4,5,7,8,9,10]:suspended[0]=True
    if row[0]in [3,5]:service_state[0]=set()
  vm.hook_add(UC_HOOK_CODE,observe)
  world=[tags,objects,pool,val(vm,0x4ba034),val(vm,0x4ba050),val(vm,0x4c1bd0),struct.unpack('<h',read(vm,0x4c1bd4,2))[0]&NONE,read(vm,0x4ba019,1)[0],val(vm,0x4ba01c),val(vm,0x485a80),int(val(vm,0x4e6494)!=0),0x4c1bd8,val(vm,0x4b9ed8),val(vm,0x4ed284),0x487b18,0x487b1c]
  world.extend([val(vm,0x4de2f4),val(vm,0x4e0348),0x55ee60,read(vm,0x467464,1)[0]])
  try:run(vm,0x4b5d0,sp,args,{R.UC_X86_REG_ECX:force,R.UC_X86_REG_EDX:mode})
  except Exception as error:
   registers={name:hex(vm.reg_read(getattr(R,'UC_X86_REG_'+name)))for name in ['EAX','EBX','ECX','EDX','ESI','EDI','ESP','EBP']}
   raise RuntimeError(f'Case{len(cases)} {label} {v}, PC{vm.reg_read(R.UC_X86_REG_EIP):08x}, registers{registers}, recent owner events{events[-5:]}: {error}')from error
  assert not pending and not suspended[0]
  inputs=[*args,force,mode,scratch,*world,len(owned),*[w for at,n in owned for w in [at,n//4]]]
  merged=[]
  for start,end in sorted(ranges):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  inputs.append(len(merged))
  for start,end in merged:inputs.extend([start,(end-start)//4,*words(initial_read(start,end-start))])
  inputs.append(len(events))
  for row in events:inputs.extend([len(row),*row])
  after=[w for at,n in owned for w in words(read(vm,at,n))]
  fixture.extend([len(inputs),len(after),*inputs,*after]);cases.append(dict(label=label,mutation=v,events=len(events),event_kinds=[sum(row[0]==k for row in events)for k in range(11)],explicit_service_write_words=service_words[0],checked_words=len(after)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),declared_main_context='00547f88',original_inputs=len(entries),intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=cases,checked_words=sum(c['checked_words']for c in cases),events=sum(c['events']for c in cases),event_kinds=[sum(c['event_kinds'][k]for c in cases)for k in range(11)],fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 report.update(explicit_deep_service_write_words=sum(c['explicit_service_write_words']for c in cases),retained_fs_base=f'{fs_base:08x}',registers_sha256=sha((a.snapshot/'registers.txt').read_bytes()),emulator_segment_routing='Declared GDT70030000 routes flat code/data and capturedFS20 to80035BDC; original thread bytes retained, no original instruction replaced.')
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
