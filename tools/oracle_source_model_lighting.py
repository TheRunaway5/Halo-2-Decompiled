#!/usr/bin/env python3
"""Unchanged3DB00/BAF80/3DFE0 against native routing/cache chronology.

Actual main model inputs with explicitly declared fresh geometry tokens, then
declared lighting cache/parent/probe controls. The retained main reuses geometry
and does not itself call3DB00. Fresh model builds supply these lighting entries.
Original callees execute unchanged. D1850 sample, D4080 conversion, BA1D0
velocity and3E5A0 incident results remain explicit ordered service outputs;
no lighting-cache stamps, validity or inheritance writes are replayed.
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
 fs_base=int(re.search(r'^fs_base\s+0x([0-9a-f]+)',(a.snapshot/'registers.txt').read_text(),re.M)[1],16);assert fs_base==0x80035bdc
 stack,stop=0x700fe000,0x700fff00;executed={};entries=[];builds=[]
 def read(vm,at,n):
  regions=list(vm.mem_regions())
  for page in range(at&~4095,(at+n+4095)&~4095,4096):
   if not any(b<=page<=e for b,e,_ in regions):vm.mem_map(page,4096);vm.mem_write(page,s.read(page,4096))
  return bytes(vm.mem_read(at,n))
 def val(vm,at):return u(read(vm,at,4))
 def write(vm,at,raw):read(vm,at,len(raw));vm.mem_write(at,raw)
 def routing(vm):
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
 def capture_build(vm,pc,_size,_):
  if pc!=0x4b5d0:return
  sp=vm.reg_read(R.UC_X86_REG_ESP)
  builds.append((sp,words(read(vm,sp+4,24)),vm.reg_read(R.UC_X86_REG_ECX),vm.reg_read(R.UC_X86_REG_EDX),[(b,e,perms,bytes(vm.mem_read(b,e-b+1)))for b,e,perms in vm.mem_regions()]))
 def capture(vm,pc,_size,_):
  if pc!=0x3db00:return
  sp=vm.reg_read(R.UC_X86_REG_ESP);read(vm,sp-20,32)
  entries.append((sp,vm.reg_read(R.UC_X86_REG_EAX),val(vm,sp+4),[(b,e,perms,bytes(vm.mem_read(b,e-b+1)))for b,e,perms in vm.mem_regions()]))
 g.hook_add(UC_HOOK_CODE,unchanged);g.hook_add(UC_HOOK_CODE,capture);g.hook_add(UC_HOOK_CODE,capture_build)
 write(g,0x4c0b80,pack([0x547f88]));run(g,0x2cbc0,stack,[0]);actual_lighting_inputs=len(entries)
 for sp,args,force,mode,regions in builds:
  vm=clone(regions);objects=val(vm,0x4e0300);b=val(vm,val(vm,objects+0x44)+(args[0]&65535)*12+8);token=val(vm,b+0xcc)
  assert token!=NONE;c=val(vm,val(vm,0x509434)+0x44)+(token&65535)*256
  write(vm,c+0x4c,pack([NONE]));vm.hook_add(UC_HOOK_CODE,capture)
  run(vm,0x4b5d0,sp,args,{R.UC_X86_REG_ECX:force,R.UC_X86_REG_EDX:mode})
 assert entries
 variants=[('declared fresh model lighting input',dict(index=i))for i in range(len(entries))]
 variants += [('default and probe routing',dict(route=n))for n in ['default','probe0','probe1']]
 variants += [('root validity frozen and query stamp',dict(root=True,valid=v,frozen=f,last=l))for v in range(4)for f in [0,1]for l in [-1,0,99,100,101]]
 variants += [('literal query distance cadence',dict(root=True,valid=3,last=100-n,metric=bits(d),movement=m))for d,interval in [(1.,60),(10.,60),(10.000001,45),(50.,45),(50.00001,15),(100.,15),(100.00001,9),(200.,9),(200.00002,6),(400.,6),(400.00003,3),(float('nan'),60)]for n in [interval,interval+1]for m in [0.,.051]]
 variants += [('retained interpolation and velocity',dict(root=True,valid=3,last=99,interpolation=i,speed=v,movement=m))for i in [0,1]for v in [0.,.15,.16,1.,30.,40.]for m in [0.,.049,.05,.051,.3]]
 variants += [('root direct allocation failure',dict(root=True,fail_cache=True,valid=0))]
 variants += [('child inheritance and cache acquisition',dict(child=True,child_cache=n))for n in ['retained','new','failed']]
 fixture=[0x4c494d53,1,len(variants)];cases=[]
 for label,v in variants:
  sp,owner,metric,regions=entries[v.get('index',0)];vm=clone(regions)
  tags,objects,pool=val(vm,0x4e3b44),val(vm,0x4e0300),val(vm,0x509434);headers=val(vm,objects+0x44)
  records,bitmap=val(vm,pool+0x44),val(vm,pool+0x48)
  def body(o):return val(vm,headers+(o&65535)*12+8)
  ancestor=owner
  while val(vm,body(ancestor)+0x14)!=NONE:ancestor=val(vm,body(ancestor)+0x14)
  if v.get('root'):owner=ancestor
  if v.get('child'):
   # Declared copied child; its real parent/root and native cache allocator
   # remain current inputs. No original owner registration is claimed.
   owner=0xe16307f8;child=0x70024000;write(vm,child,read(vm,body(ancestor),0x180));write(vm,child+0x14,pack([ancestor]));write(vm,child+0xcc,pack([NONE]));write(vm,child+7,bytes([read(vm,child+7,1)[0]&~1]));write(vm,child+0xd0,struct.pack('<h',-1))
   write(vm,headers+(owner&65535)*12,struct.pack('<HBBII',0xe163,0,2,0,child))
   if v['child_cache']=='retained':
    free=next(i for i in range(256)if not(val(vm,bitmap+i//32*4)&(1<<(i&31))))
    token=(0xe164<<16)|free;write(vm,records+free*256+4,pack([owner]));write(vm,child+0xcc,pack([token]))
   if v['child_cache']=='failed':
    write(vm,bitmap,bytes(32));write(vm,pool+0x34,pack([256,256,0]))
  b=body(owner);root_body=body(ancestor);root_token=val(vm,root_body+0xcc);assert root_token!=NONE
  c=records+(root_token&65535)*256
  if label!='declared fresh model lighting input':
   write(vm,0x4ba034,pack([100]));write(vm,root_body+0xc1,bytes([read(vm,root_body+0xc1,1)[0]&~1]))
   write(vm,c+12,pack([99]));write(vm,0x467464,bytes([0]));write(vm,c+0x10,read(vm,root_body+0x30,12))
  if 'valid'in v:
   write(vm,c+2,bytes([v['valid']&1,(v['valid']>>1)&1]));write(vm,c+0x1c,pack([bits(.2),bits(.3),bits(.4),0x00123456,0x00789abc]));write(vm,c+0x30,pack([bits(.4),bits(.2),bits(.8),0x00563412,0x00bc9a78]))
  if 'last'in v:write(vm,c+8,pack([v['last']]))
  if 'frozen'in v:write(vm,root_body+0xc1,bytes([(read(vm,root_body+0xc1,1)[0]&~1)|v['frozen']]))
  if 'metric'in v:metric=v['metric']
  if 'movement'in v:
   origin=struct.unpack('<3f',read(vm,root_body+0x30,12));write(vm,c+0x10,struct.pack('<3f',origin[0]+v['movement'],origin[1],origin[2]))
  if 'interpolation'in v:write(vm,0x467464,bytes([v['interpolation']]))
  if 'speed'in v:write(vm,root_body+0xb4,pack([NONE]));write(vm,root_body+0x88,struct.pack('<3f',v['speed'],0.,0.))
  if v.get('fail_cache'):
   write(vm,root_body+0xcc,pack([NONE]));write(vm,bitmap,bytes(32));write(vm,pool+0x34,pack([256,256,0]))
  if 'route'in v:
   if v['route']=='default':write(vm,b+7,bytes([read(vm,b+7,1)[0]|1]))
   else:
    write(vm,b+7,bytes([read(vm,b+7,1)[0]&~1]));write(vm,b+0xd0,struct.pack('<h',int(v['route'][-1])));write(vm,val(vm,0x4e0348)+0x230,pack([0x70026000]));write(vm,0x70026000,pack([0x9e370000+i for i in range(46)]))
  default,bsp,fallback=val(vm,0x4de2f4),val(vm,0x4e0348),0x55ee60;scratch=sp-20
  read(vm,scratch,32)
  owned=[(records,256*256),(pool,0x4c),(bitmap,32),(root_body+0xcc,4),(b+0xcc,4),(fallback,84)]
  for at,n in owned:read(vm,at,n)
  initial_pages={at:bytes(vm.mem_read(at,4096))for start,end,_ in vm.mem_regions()for at in range(start,end+1,4096)}
  def initial_read(at,n):
   raw=b''
   while n:
    page=at&~4095;count=min(n,4096-(at&4095));data=initial_pages.get(page)
    if data is None:data=s.read(page,4096)
    raw+=data[at-page:at-page+count];at+=count;n-=count
   return raw
  ranges=[(at&~3,(at+n+3)&~3)for at,n in owned]+[(scratch,scratch+32)];events=[];pending={};suspended=[False]
  def access(_guest,_access,at,n,_value,_data):ranges.append((at&~3,(at+n+3)&~3))
  vm.hook_add(UC_HOOK_MEM_READ,access);vm.hook_add(UC_HOOK_MEM_WRITE,access)
  def observe(guest,pc,_size,_):
   if pc in pending:
    row,out,n=pending.pop(pc)
    if row[0]==0:row.append(guest.reg_read(R.UC_X86_REG_EAX))
    elif row[0]==1:row.extend([guest.reg_read(R.UC_X86_REG_EAX)&255,*words(read(guest,out,n))])
    else:row.extend(words(read(guest,out,n)))
    if row[0]!=0:suspended[0]=False
   if suspended[0]:return
   esp=guest.reg_read(R.UC_X86_REG_ESP);row=None;out=n=0
   if pc==0x3ddd0:row=[0,val(guest,esp+4)]
   elif pc==0xd1850:
    out=val(guest,esp+8);n=20;row=[1,guest.reg_read(R.UC_X86_REG_EDI),val(guest,esp+4)&255,*words(read(guest,out,n))]
   elif pc==0xd4080:
    out=guest.reg_read(R.UC_X86_REG_ESI);n=84;row=[2,guest.reg_read(R.UC_X86_REG_ECX),val(guest,esp+4)&255,*words(read(guest,guest.reg_read(R.UC_X86_REG_EAX),20)),*words(read(guest,out,n))]
   elif pc==0xba1d0:
    out=guest.reg_read(R.UC_X86_REG_ECX);n=12;row=[3,guest.reg_read(R.UC_X86_REG_EAX)]
   elif pc==0x3e5a0:
    out=guest.reg_read(R.UC_X86_REG_ESI);n=12;row=[4,val(guest,esp+4),*words(read(guest,guest.reg_read(R.UC_X86_REG_EAX),12)),*words(read(guest,out,12))]
   if row is not None:
    events.append(row);pending[val(guest,esp)]=(row,out,n)
    if row[0]!=0:suspended[0]=True
  vm.hook_add(UC_HOOK_CODE,observe)
  world=[owner,metric,tags,objects,pool,val(vm,0x4ba034),val(vm,0x4ba050),default,bsp,fallback,scratch,read(vm,0x467464,1)[0]]
  try:run(vm,0x3db00,sp,[metric],{R.UC_X86_REG_EAX:owner})
  except Exception as error:raise RuntimeError(f'Case{len(cases)} {label} {v}, PC{vm.reg_read(R.UC_X86_REG_EIP):08x}, events{events[-3:]}: {error}')from error
  assert not pending and not suspended[0];result=vm.reg_read(R.UC_X86_REG_EAX);read(vm,result,84);ranges.append((result,result+84))
  inputs=[*world,len(owned),*[w for at,n in owned for w in [at,n//4]]];merged=[]
  for start,end in sorted(ranges):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  inputs.append(len(merged))
  for start,end in merged:inputs.extend([start,(end-start)//4,*words(initial_read(start,end-start))])
  inputs.append(len(events))
  for row in events:inputs.extend([len(row),*row])
  after=[result,*words(read(vm,result,84)),*[w for at,n in owned for w in words(read(vm,at,n))]]
  fixture.extend([len(inputs),len(after),*inputs,*after]);cases.append(dict(label=label,mutation=v,event_kinds=[sum(row[0]==k for row in events)for k in range(5)],checked_words=len(after)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),retained_fs_base=f'{fs_base:08x}',actual_main_model_inputs=len(builds),actual_main_lighting_inputs=actual_lighting_inputs,declared_fresh_lighting_inputs=len(entries)-actual_lighting_inputs,intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=cases,checked_words=sum(c['checked_words']for c in cases),event_kinds=[sum(c['event_kinds'][k]for c in cases)for k in range(5)],fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
