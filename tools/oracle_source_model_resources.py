#!/usr/bin/env python3
"""Whole unchanged4C2B0 and4C190 model residency/LOD fallback comparison.

Inputs come from unchanged main2CBC0/133790/4B5D0, with controlled active,
cached, resource and LOD mutations. Every original callee executes. Native
4C190/4C2B0/3D430/4C640/3DDD0/3DD10 read independent pre-call memory. Actual12DE70
returns remain explicit ordered boundaries. Successful4C190 outputs also feed
whole original/native137650. No live cache/node producer or framebuffer parity
is asserted.
Controlled cases explicitly copy an actual published status/cache token to
the model's resource headers before varying selectors/failures. This isolates
selection from unresolved kernel-backed loading; it is not live residency.
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
 stack,stop=0x700fe000,0x700fff00;executed={};entries={0:[],1:[]}
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
  if pc not in [0x4c190,0x4c2b0]:return
  kind=int(pc==0x4c190);sp=vm.reg_read(R.UC_X86_REG_ESP)
  entries[kind].append((vm.reg_read(R.UC_X86_REG_EAX),sp,words(read(vm,sp+4,(7 if kind else 6)*4)),[(b,e,perms,bytes(vm.mem_read(b,e-b+1)))for b,e,perms in vm.mem_regions()]))
 g.hook_add(UC_HOOK_CODE,unchanged);g.hook_add(UC_HOOK_CODE,capture)
 write(g,0x4c0b80,pack([0x547f88]));run(g,0x2cbc0,stack,[0]);assert entries[0] and entries[1]
 variants=[]
 for kind in [0,1]:
  variants += [('actual model resource call',dict(kind=kind,index=i))for i in range(len(entries[kind]))]
  variants += [('authored geometry LOD',dict(kind=kind,lod=n))for n in range(6)]
  variants += [('active and cached NONE',dict(kind=kind,active=a,cached=c))for a,c in [(255,255),(255,0),(0,255),(1,0),(0,1)]]
  variants += [('resource unavailable',dict(kind=kind,fail='all',cached=c))for c in [0,255]]
  variants += [('empty signed region count',dict(kind=kind,regions=n))for n in [0,NONE]]
  variants += [('retained selected bytes',dict(kind=kind,random_selected=True,fail='all',cached=0))]
 variants += [('low-byte section request',dict(kind=0,request=n))for n in [0,1,255,256,257,NONE]]
 variants += [('LOD fallback selection',dict(kind=1,fail='lod',lod=n,prior=p))for n,p in [(0,1),(1,0),(1,2),(2,1),(0,255),(0,256),(0,257),(0,NONE)]]
 variants += [('distinct authored LOD fallback',dict(kind=1,fail='lod',lod=n,prior=p))for n,p in [(0,3),(3,0),(1,4),(4,1),(2,5),(5,2)]]
 variants += [('signed object region byte extent',dict(kind=1,body_extent=n))for n in [-32768,-11,-10,-9,-1,0,9,10,19,20,160,32767]]
 variants += [('declared duplicate-region success mask',dict(kind=0,duplicate_regions=n))for n in [15,16,17,31,32,33]]
 variants += [('first person local region bank',dict(kind=1,first_person=n,view=v))for n in [1,255,256,257]for v in [0,1,3]]
 variants += [('first person invalid view cache gate',dict(kind=1,first_person=1,view=v))for v in [-1,4]]
 variants += [('joined native cache allocation/rebind',dict(kind=1,cache_mode=c,first_person=f,view=v))for c in ['empty','hole','pressure']for f in [0,1]for v in range(4)]
 variants += [('cache current region/color inputs',dict(kind=2,cache_mode=c,cache_regions=n,cache_colors=k))for c in ['empty','pressure']for n in [0,1,2,15,16]for k in range(6)]
 variants += [('cache salt increment and retained upper half',dict(kind=2,cache_mode='empty',generation=n))for n in [0xe163,0xfffe,0xffff,0x7fff,0x8000]]
 variants += [('cache pressure signed age and tie order',dict(kind=2,cache_mode='pressure',frame=n))for n in [0,1,0x7fffffff,0x80000000,NONE]]
 variants += [('matching owner skips invalid region extent',dict(kind=2,cache_mode='same_owner',body_extent=-32768))]
 variants += [('exhausted empty pool skips invalid region extent',dict(kind=2,cache_mode='exhausted',body_extent=-32768))]
 variants += [('current colors force/definition/token gates',dict(kind=3,cache_mode='same_owner',refresh_force=f,refresh_flags=g,cache_colors=c,absent_cache=n))for f in [0,1]for g in [0,1]for c in range(6)for n in [0,1]]
 variants += [('current color force low byte',dict(kind=3,cache_mode='same_owner',refresh_force=f,refresh_flags=0,cache_colors=4))for f in [0,1,255,256,257,NONE]]
 variants += [('signed current color byte extent',dict(kind=3,cache_mode='same_owner',refresh_force=1,color_extent=n))for n in [0,1,23,24,25,47,48,71,72,95,96,119,120]]
 for label,v in variants:
  if label!='actual model resource call':v['controlled_residency']=True
 rng=random.Random(0x4c190);fixture=[0x52534d53,3,len(variants)];cases=[]
 for label,v in variants:
  kind=v['kind'];eax,sp,args,regions=entries[1 if kind>=2 else kind][v.get('index',0)];args=list(args);vm=clone(regions)
  tags=val(vm,0x4e3b44);objects=val(vm,0x4e0300);cache_pool=val(vm,0x509434);cache_records=val(vm,cache_pool+0x44);view=val(vm,0x4ba050)
  if 'view'in v:view=v['view']&NONE;write(vm,0x4ba050,pack([view]))
  if kind:
   owner,fp,model,lod_ptr,prior,selected,changed=args
   if 'first_person'in v:fp=args[1]=v['first_person']
   if 'prior'in v:prior=args[4]=v['prior']
   body=val(vm,val(vm,objects+0x44)+(owner&65535)*12+8)
   if 'body_extent'in v:write(vm,body+0x118,struct.pack('<h',v['body_extent']))
   active=body+struct.unpack('<h',read(vm,body+0x11a,2))[0]
   token=val(vm,body+0xcc);cached=cache_records+(token&65535)*256+0xc0+(view&3)*16
   lod=struct.unpack('<h',read(vm,lod_ptr,2))[0];request=1
  else:
   model=eax;active,cached,lod,request,selected,changed=args;lod=struct.unpack('<i',pack([lod]))[0]
  m=val(vm,tags+(model&65535)*16+8);region_count=val(vm,m+0x1c)
  assert region_count<=16,region_count
  if v.get('controlled_residency'):
   sections=val(vm,m+0x28);section_count=val(vm,m+0x24);published=[]
   for i in range(section_count):
    resource=sections+i*0x5c+0x38
    if read(vm,resource+0x1e,1)!=b'\0':published.append((read(vm,resource+0x1e,1),read(vm,resource+0x20,4)))
   assert published,'No actual published header available for declared control'
   status,token=published[0]
   for i in range(section_count):
    resource=sections+i*0x5c+0x38;write(vm,resource+0x1e,status);write(vm,resource+0x20,token)
  if 'lod'in v:
   lod=v['lod']
   if kind:write(vm,lod_ptr,struct.pack('<h',lod))
   else:args[2]=lod
  if 'request'in v:args[3]=v['request']
  if 'active'in v:write(vm,active,bytes([v['active']])*16)
  if 'cached'in v:write(vm,cached,bytes([v['cached']])*16)
  if 'regions'in v:write(vm,m+0x1c,pack([v['regions']]))
  if 'duplicate_regions'in v:
   # Isolated larger pointer banks copy the actual first region descriptor;
   # they exercise4C2B0's literal16-bit mask/low5 shifts, not authored models
   # or4B5D0's16-slot destination layout.
   n=v['duplicate_regions'];descriptor=read(vm,val(vm,m+0x20),16)
   table,active,cached,selected,changed=0x70004000,0x70005000,0x70005100,0x70005200,0x70005300
   write(vm,table,descriptor*n);write(vm,m+0x1c,pack([n]));write(vm,m+0x20,pack([table]))
   write(vm,active,bytes(64));write(vm,cached,bytes(64));write(vm,selected,bytes([0xcd])*64);write(vm,changed,pack([0xcdcdcdcd]))
   args=[active,cached,lod,request,selected,changed]
  if v.get('random_selected'):write(vm,selected,bytes(rng.randrange(256)for _ in range(32)))
  if 'fail'in v:
   table=val(vm,m+0x20);sections=val(vm,m+0x28)
   for i in range(region_count):
    r=table+i*16;perms=val(vm,r+0xc)
    for permutation in range(val(vm,r+8)):
     for level in ([lod]if v['fail']=='lod'else range(6)):
      section=struct.unpack('<h',read(vm,perms+permutation*16+4+level*2,2))[0]
      if section>=0:
       resource=sections+section*0x5c+0x38;write(vm,resource+0x1e,b'\0');write(vm,resource+8,pack([0]))
  if 'cache_mode'in v:
   # Declared current owner/pool controls. No original result or write output
   # is replayed into the native implementation. First two active regions use
   # the existing model's valid permutation0; remaining bytes are distinctive.
   cache_mode=v['cache_mode'];bits=val(vm,cache_pool+0x48)
   write(vm,cache_records,pack([0xa53c7e91])*16384)
   write(vm,cache_pool+0x34,pack([0,0,0,0xa5a50000|v.get('generation',0xe163)]))
   write(vm,bits,bytes(32));write(vm,body+0xcc,pack([NONE]))
   if cache_mode in ['hole','pressure','exhausted']:
    occupied=[NONE]*8
    if cache_mode=='hole':occupied[77//32]^=1<<(77&31)
    if cache_mode=='exhausted':occupied=[0]*8
    write(vm,bits,pack(occupied))
    write(vm,cache_pool+0x34,pack([37 if cache_mode=='hole'else 256,256,sum(n.bit_count()for n in occupied)]))
    for i in range(256):
     write(vm,cache_records+i*256,pack([0x5a5a0000|((0xe163+i)&65535),owner^0x10000,0x12345678,(val(vm,0x4ba034)-[1,1000,1001,-1,0][i%5])&NONE]))
   elif cache_mode=='same_owner':
    write(vm,body+0xcc,pack([0xabcd0077]));write(vm,cache_records+0x77*256+4,pack([owner]))
   if 'frame'in v:write(vm,0x4ba034,pack([v['frame']]))
   if cache_mode not in ['same_owner','exhausted'] or kind==3:
    nr,nc=v.get('cache_regions',2),v.get('cache_colors',4)
    write(vm,body+0x118,struct.pack('<hh',nr*10,0x200));active=body+0x200
    write(vm,active,bytes([0,0,*range(2,16)]))
    write(vm,body+0x124,struct.pack('<hh',nc*24,0x300))
    write(vm,body+0x300+nc*12,struct.pack('<12f',0.,1.,.5,-.5,2.,.25,.8,.1,.3,.6,.2,.9))
   if kind==3:
    if v.get('absent_cache'):write(vm,body+0xcc,pack([NONE]))
    definition=val(vm,tags+(val(vm,body)&65535)*16+8)
    if 'refresh_flags'in v:
     flags=read(vm,definition+0x1c,1)[0];write(vm,definition+0x1c,bytes([(flags&~1)|v['refresh_flags']]))
    if 'color_extent'in v:
     extent=v['color_extent'];write(vm,body+0x124,struct.pack('<h',extent))
     write(vm,body+0x300+(extent//24)*12,struct.pack('<12f',0.,1.,.5,-.5,2.,.25,.8,.1,.3,.6,.2,.9))
  extent=64 if 'duplicate_regions'in v else 16
  bits=val(vm,cache_pool+0x48);frame=val(vm,0x4ba034)
  owned=[(selected,64 if 'duplicate_regions'in v else 32),(changed&~3,4),(cache_records,256*256),(cache_pool,0x4c),(bits,32)]
  # Native cache acquisition consumes the entire current pool, not original
  # return/write patches. Retain independently initialized owner banks.
  read(vm,cache_records,256*256);read(vm,cache_pool,0x4c);read(vm,bits,32)
  if kind:owned += [(body+0xcc,4)]
  if kind==1:owned += [(eax,4),(lod_ptr&~3,4),(active,16),(sp-16,16)]
  elif kind>=2:owned += [(active,16)]
  else:owned += [(active,extent),(cached,extent)]
  # Declared output and guard bytes are part of the independent input bank.
  for at,n in owned:read(vm,at,n)
  initial_pages={at:bytes(vm.mem_read(at,4096))for b,e,_ in vm.mem_regions()for at in range(b,e+1,4096)}
  def initial_read(at,n):
   out=b''
   while n:
    page=at&~4095;count=min(n,4096-(at&4095));raw=initial_pages.get(page)
    if raw is None:raw=s.read(page,4096)
    out+=raw[at-page:at-page+count];at+=count;n-=count
   return out
  ranges=[(at&~3,(at+n+3)&~3)for at,n in owned];requests=[];request_pending={};acquires=[];acquire_pending={};events=[]
  def memory_read(_guest,_access,at,n,_value,_data):ranges.append((at&~3,(at+n+3)&~3))
  def memory_write(_guest,_access,at,n,_value,_data):
   ranges.append((at&~3,(at+n+3)&~3))
  vm.hook_add(UC_HOOK_MEM_READ,memory_read);vm.hook_add(UC_HOOK_MEM_WRITE,memory_write)
  def observe(guest,pc,_size,_):
   if pc in request_pending:request_pending.pop(pc).append(guest.reg_read(R.UC_X86_REG_EAX)&255)
   if pc in acquire_pending:
    acquire_pending.pop(pc).append(guest.reg_read(R.UC_X86_REG_EAX))
   if pc==0x12de70:
    esp=guest.reg_read(R.UC_X86_REG_ESP);row=[guest.reg_read(R.UC_X86_REG_EAX),guest.reg_read(R.UC_X86_REG_ECX)];events.append([0,len(requests)]);requests.append(row);request_pending[val(guest,esp)]=row
   if pc==0x3ddd0:
    esp=guest.reg_read(R.UC_X86_REG_ESP);row=[val(guest,esp+4)];events.append([1,len(acquires)]);acquires.append(row);acquire_pending[val(guest,esp)]=row
  vm.hook_add(UC_HOOK_CODE,observe)
  if kind==2:args=[owner]
  if kind==3:args=[owner,v['refresh_force']]
  try:run(vm,0x3dd10 if kind==3 else 0x3ddd0 if kind==2 else 0x4c190 if kind else 0x4c2b0,sp,args,{R.UC_X86_REG_EAX:eax})
  except Exception as error:raise RuntimeError(f'Case{len(cases)} {label} {v}, PC{vm.reg_read(R.UC_X86_REG_EIP):08x}: {error}')from error
  if kind==2:acquire_pending.pop(stop).append(vm.reg_read(R.UC_X86_REG_EAX))
  assert not request_pending and not acquire_pending
  result=0 if kind==3 else vm.reg_read(R.UC_X86_REG_EAX)&(NONE if kind==2 else 255)
  nodes=0
  if kind==1 and result:
   final_lod=struct.unpack('<h',read(vm,lod_ptr,2))[0]
   run(vm,0x137650,0x700fd000,regs={R.UC_X86_REG_EAX:model,R.UC_X86_REG_EDX:final_lod&NONE,R.UC_X86_REG_EBX:selected})
   nodes=vm.reg_read(R.UC_X86_REG_EAX)
  inputs=[kind,eax,*args,tags,objects,view,cache_pool,frame,sp-16,len(owned),*[w for at,n in owned for w in [at,n//4]]]
  merged=[]
  for start,end in sorted(ranges):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  inputs.append(len(merged))
  for start,end in merged:inputs.extend([start,(end-start)//4,*words(initial_read(start,end-start))])
  inputs.extend([len(requests),*[w for row in requests for w in row],len(acquires),*[w for row in acquires for w in row]])
  inputs.extend([len(events),*[w for row in events for w in row]])
  after=[result,nodes,*[w for at,n in owned for w in words(read(vm,at,n))]]
  fixture.extend([len(inputs),len(after),*inputs,*after]);cases.append(dict(label=label,mutation=v,result=result,nodes=nodes,requests=len(requests),acquires=len(acquires),checked_words=len(after)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),declared_main_context='00547f88',original_inputs={str(k):len(v)for k,v in entries.items()},intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=cases,checked_words=sum(c['checked_words']for c in cases),requests=sum(c['requests']for c in cases),acquires=sum(c['acquires']for c in cases),fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
