#!/usr/bin/env python3
"""Whole unchanged133790 and1332F0 with native metric/coverage/4BAF0 routing.

Actual main2CBC0/133720 entries and declared pass, queue, viewport and fog
controls supply inputs. Every original callee executes unchanged. Native
4B5D0 and133AB0 are explicit ordered record boundaries in this schema; their
complete input and returned collector bytes are retained. No claim of live
object/cache/geometry production or framebuffer parity follows this fixture.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_UNMAPPED,UC_HOOK_CODE
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
 def routing(vm):
  def segment(base,access):return struct.pack('<HHBBBB',65535,base&65535,(base>>16)&255,access,0xcf,(base>>24)&255)
  table=0x70030000;vm.mem_write(table,bytes(0x28));vm.mem_write(table+8,segment(0,0x9b)+segment(0,0x93));vm.mem_write(table+32,segment(0x80035bdc,0x93));vm.reg_write(R.UC_X86_REG_GDTR,(0,table,0x27,0))
  for register,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,32)]:vm.reg_write(register,value)
  assert vm.reg_read(R.UC_X86_REG_FS_BASE)==0x80035bdc
 def unchanged(vm,pc,size,_):
  if pc not in executed:
   raw=bytes(vm.mem_read(pc,size));assert raw==x.read(pc,size),f'Original instruction differs at{pc:08x}';executed[pc]=raw
 def run(vm,entry,sp,args,context):
  vm.mem_write(sp,pack([stop,*args]));vm.reg_write(R.UC_X86_REG_ESP,sp);vm.reg_write(R.UC_X86_REG_ESI,context);vm.reg_write(R.UC_X86_REG_FPCW,0x023f);vm.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
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
  if pc!=0x133790:return
  sp=vm.reg_read(R.UC_X86_REG_ESP);read(vm,sp-0x1e4,0x1e4)
  entries.append((sp,words(read(vm,sp+4,20)),vm.reg_read(R.UC_X86_REG_ESI),[(b,e,perms,bytes(vm.mem_read(b,e-b+1)))for b,e,perms in vm.mem_regions()]))
 g.hook_add(UC_HOOK_CODE,unchanged);g.hook_add(UC_HOOK_CODE,capture);write(g,0x4c0b80,pack([0x547f88]));run(g,0x2cbc0,stack,[0],0);assert entries
 variants=[('actual main object entry',dict(index=i))for i in range(len(entries))]
 variants += [('secondary/world and alternate low byte',dict(mode=m,alternate=n))for m in [0,1,2,255]for n in [0,1,255,256,257,NONE]]
 variants += [('queue flags and association',dict(flags=f,association=n))for f in [0,0x200,0x400,0x800,0x7e00,0xffff]for n in [65535,0,31]]
 variants += [('source coverage resolution',dict(height=bits(n)))for n in [119.75,120,239.5,240,360,540,1080]]
 variants += [('query sign',dict(query_flags=n))for n in [0,1,127,128,255]]
 variants += [('planar fog gates',dict(fog=3,plane=True,fraction=bits(f),planar_only=o,planar_flags=n))for f in [0.,1.]for o in [0,1]for n in [0,2]]
 variants += [('first fade zero queue marker',dict(fade_zero=True,flags=f))for f in [0,1,0x200,0x800]]
 variants += [('retained collector bytes',dict(retained=n))for n in range(8)]
 variants += [('ordered fog equality',dict(fog=3,plane=True,fraction=f,planar_only=1,planar_flags=0))for f in [0x80000000,0x7fc00001,0x7f800001,0x7f800000]]
 variants += [('first-person repeat presentation',dict(mode=m,alternate=n,first_person=c))for m in [0,1,2]for n in [0,1,256]for c in [0,1,2]]
 fixture=[0x454e5253,1,len(variants)];cases=[]
 for label,v in variants:
  sp,args,context,regions=entries[v.get('index',0)];vm=clone(regions);args=list(args);collector=sp-0x1e4
  if 'alternate'in v:args[1]=v['alternate']
  if 'mode'in v:write(vm,context+0x2a78,pack([v['mode']]))
  if 'height'in v:write(vm,0x4b9ed0,pack([v['height']]))
  if 'query_flags'in v:write(vm,context+0x2a60,bytes([v['query_flags']]))
  if 'retained'in v:
   rng=random.Random(0x133790+v['retained']);write(vm,collector,bytes(rng.getrandbits(8)for _ in range(0x1e4)))
  queue=val(vm,context+0x14);flags=val(vm,queue+8)+args[0]*2;assoc=val(vm,queue+0x14)+args[0]*2;owner=val(vm,val(vm,queue+12)+args[0]*4)
  if 'first_person'in v:write(vm,0x4c1bd0,pack([owner]));write(vm,0x4c1bd4,struct.pack('<h',v['first_person']))
  if 'flags'in v:write(vm,flags,struct.pack('<H',v['flags']))
  if 'association'in v:write(vm,assoc,struct.pack('<H',v['association']))
  tags=val(vm,0x4e3b44);headers=val(vm,val(vm,0x4e0300)+0x44)
  def body(o):return val(vm,headers+(o&65535)*12+8)
  def metadata(t):return val(vm,tags+(t&65535)*16+8)
  if 'fog'in v:write(vm,0x4b9ef0,pack([v['fog']]));write(vm,0x4b9ffc,pack([v['fraction']]));write(vm,0x4ba004,bytes([v['planar_only']]))
  if v.get('plane'):
   # A declared fog metadata owner, independent from actual render fog choice.
   plane=0x7fff;write(vm,0x4b9f8c,pack([plane]));write(vm,tags+plane*16+8,pack([0x70012000]));write(vm,0x70012000,pack([v['planar_flags']]))
  owners=[];definitions={};models={};seen=set();o=owner
  while o!=NONE:
   assert o not in seen;seen.add(o);b=body(o);definition=val(vm,b);model=val(vm,metadata(definition)+0x38);parent=val(vm,b+0x14)
   owners.extend([o,val(vm,b+4),parent,definition,val(vm,b+0x3c)]);definitions[definition]=model
   if model!=NONE:
    m=metadata(model)
    if v.get('fade_zero'):write(vm,m+0x28,pack([bits(0.00001),bits(0.)]))
    models[model]=[val(vm,m+0x28),val(vm,m+0x2c),*[val(vm,m+0x34+i*4)for i in range(5)],struct.unpack('<H',read(vm,m+0x4c,2))[0]]
   o=parent
  plane=val(vm,0x4b9f8c);plane_flags=read(vm,metadata(plane),1)[0]if plane!=NONE else 0
  initial=words(read(vm,collector,0x1e4));row=[owner,struct.unpack('<H',read(vm,flags,2))[0],struct.unpack('<H',read(vm,assoc,2))[0]]
  camera=[val(vm,context+0x2a78),*words(read(vm,0x4b9da0,12)),val(vm,0x4b9dcc),val(vm,0x4ba028),*[val(vm,at)for at in [0x4b9e20,0x4b9e2c,0x4b9e38,0x4b9e44,0x4b9ed0]],read(vm,context+0x2a60,1)[0],val(vm,0x4b9ef0),plane,val(vm,0x4b9ffc),read(vm,0x4ba004,1)[0],plane_flags]
  sphere=words(read(vm,body(owner)+0x30,16));modifier=read(vm,0x4ba019,1)[0];events=[];pending={};depth=[0]
  def observe(guest,pc,_size,_):
   if pc in pending:
    event,out=pending.pop(pc)
    if event[0]==2:event.extend([read(guest,out[0],1)[0],read(guest,out[1],1)[0]])
    else:event.extend(words(read(guest,out,0x1e4)));depth[0]-=1
   if depth[0]:return
   esp=guest.reg_read(R.UC_X86_REG_ESP);event=None;out=None
   if pc==0x4baf0:
    event=[2,guest.reg_read(R.UC_X86_REG_EBX),guest.reg_read(R.UC_X86_REG_XMM1)&NONE];out=words(read(guest,esp+4,8))
   elif pc==0x4b5d0:
    a0=words(read(guest,esp+4,24));out=a0[3]-4
    assert out==collector
    event=[0,a0[0],a0[1],a0[2],a0[5],guest.reg_read(R.UC_X86_REG_EDX),guest.reg_read(R.UC_X86_REG_ECX)&255,*words(read(guest,a0[4],12)),*words(read(guest,out,0x1e4))];depth[0]+=1
   elif pc==0x133ab0:
    assert val(guest,esp+4)==collector
    out=collector;event=[1,*words(read(guest,esp+8,12)),*words(read(guest,out,0x1e4))];depth[0]+=1
   if event is not None:events.append(event);pending[val(guest,esp)]=(event,out)
  vm.hook_add(UC_HOOK_CODE,observe)
  try:run(vm,0x133790,sp,args,context)
  except Exception as e:raise RuntimeError(f'Case{len(cases)} {label} {v} at{vm.reg_read(R.UC_X86_REG_EIP):08x}: {e}')from e
  assert not pending and not depth[0]
  inputs=[*row,*args[1:],*camera,*sphere,modifier,len(owners)//5,*owners,len(definitions),*[w for t,m in sorted(definitions.items())for w in [t,m]],len(models),*[w for t,raw in sorted(models.items())for w in [t,*raw]],*initial,len(events)]
  for event in events:inputs.extend([len(event),*event])
  after=[vm.reg_read(R.UC_X86_REG_EAX)&255,struct.unpack('<H',read(vm,flags,2))[0],*words(read(vm,collector,0x1e4))]
  fixture.extend([len(inputs),len(after),*inputs,*after]);cases.append(dict(label=label,mutation=v,events=len(events),builds=sum(e[0]==0 for e in events),presentations=sum(e[0]==1 for e in events),checked_words=len(after)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),original_inputs=len(entries),intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=cases,checked_words=sum(c['checked_words']for c in cases),fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),retained_fs_base='80035bdc',emulator_segment_routing='Declared GDT70030000 routes capturedFS20; thread bytes retained, no original instruction replaced.')
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
