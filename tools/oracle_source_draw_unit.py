#!/usr/bin/env python3
"""Whole unchanged2CB30/2CBF0 shared draw-unit bank owner.

Retained original main units supply BSP/instance/dynamic request identities.
Declared inputs vary counters, flags, override/fade/section state and actual
metadata resource pointers.44940/2D000/449E0/30290 execute unchanged. The
native30290 consumes the actual rendering bank, eye and short viewport.
Fixture3 also exports independent pre-call data for native44940/2D000/449E0;
only12DE70 publication remains an ordered explicit native service. No
instruction or return is intercepted. This is not live
collector production or framebuffer parity.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_READ
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
 stack,stop,center=0x700fe000,0x700fff00,0x70001000
 executed={}
 def read(vm,at,n):
  regions=list(vm.mem_regions())
  for page in range(at&~4095,(at+n+4095)&~4095,4096):
   if not any(b<=page<=e for b,e,_ in regions):vm.mem_map(page,4096);vm.mem_write(page,s.read(page,4096))
  return bytes(vm.mem_read(at,n))
 def val(vm,at):return u(read(vm,at,4))
 def half(vm,at):return struct.unpack('<H',read(vm,at,2))[0]
 def write(vm,at,raw):read(vm,at,len(raw));vm.mem_write(at,raw)
 def run(vm,entry,args=(),regs=None):
  vm.mem_write(stack,pack([stop,*args]));vm.reg_write(R.UC_X86_REG_ESP,stack);vm.reg_write(R.UC_X86_REG_FPCW,0x023f);vm.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
  for register,value in (regs or {}).items():vm.reg_write(register,value)
  vm.emu_start(entry,stop,count=10000000);assert vm.reg_read(R.UC_X86_REG_EIP)==stop,hex(vm.reg_read(R.UC_X86_REG_EIP))
 def trace(vm,pc,size,_):
  if pc not in executed:
   raw=bytes(vm.mem_read(pc,size));assert raw==x.read(pc,size),f'Original instruction differs at{pc:08x}';executed[pc]=raw
 g=s.guest();g.mem_map(0x70000000,0x100000)
 run(g,0xc15a0)
 g.mem_write(stack,pack([stop]));g.reg_write(R.UC_X86_REG_ESP,stack);g.emu_start(0x31c80,0x2cb30,count=10000000);assert g.reg_read(R.UC_X86_REG_EIP)==0x2cb30
 ctx=val(g,0x4c0b80);table=val(g,0x4e3b44);bsp=val(g,0x4e0348)
 # A fresh execution engine avoids Unicorn retaining the previous stop-PC TB.
 h=s.guest();h.mem_map(0x70000000,0x100000)
 for b,e,permissions in g.mem_regions():
  if b>=0x70000000:continue
  h.mem_map(b,e-b+1,permissions);h.mem_write(b,bytes(g.mem_read(b,e-b+1)))
 g=h;run(g,0x2cb30) # Actual secondary reset restores the retained main prefix.
 assert val(g,0x4c0b7c)==31 and val(g,0x4c0b78)==31
 source_units=[words(read(g,0x4ba138+i*32,32))for i in range(31)]
 def state(vm):
  return [val(vm,at)for at in [0x4c0b7c,0x4c0b78,0x4c1528,0x4c19ac]]+words(read(vm,0x4ba138,850*32))+list(read(vm,0x4c0b84,850))+list(read(vm,0x4c0ed6,850))+words(read(vm,0x4c1228,192*4))+words(read(vm,0x4c152c,32*36))+[half(vm,ctx+0xa5c)]+words(read(vm,ctx+0x25c,2048))
 variants=[]
 for mode in [0,1,2,3,NONE]:variants.append(('begin reset/prefix',dict(begin=mode)))
 for index in [0,6,7,22,23,30]:
  for mode in [0,2]:variants.append(('retained actual main unit identities',dict(index=index,mode=mode)))
 for count in [0,1,849,850,851]:variants.append(('draw capacity',dict(count=count)))
 variants.append(('NONE geometry',dict(geometry=NONE)))
 for used in [0,510,511,512]:variants.append(('shared mask arena boundary',dict(used=used,flags=0x40)))
 for count in [0,191,192,193]:variants.append(('override capacity',dict(override_count=count,override=True)))
 for section in [0,1,30,31,63]:
  for kind in [1,3]:variants.append(('section writes',dict(section=section,kind=kind,override=True,outer=1)))
 for outer,inner,gouter,ginner in [(1,0,0,0),(0,1,0,0),(1,1,0,0),(0,0,1,0),(0,0,0,1),(0,1,1,0),(0x100,1,0,0),(0,0,0,0)]:variants.append(('section rectangle branches',dict(section=0,override=True,outer=outer,inner=inner,gouter=gouter,ginner=ginner)))
 for fade in [0.,.5,1.,-.1,10.,float('nan'),1.e30]:
  for selected in [0,1,256]:variants.append(('literal fade bytes',dict(override=True,fade=bits(fade),selected=selected,index=23)))
 for index in [0,7,23,30]:variants.append(('actual null resident geometry',dict(index=index,null_geometry=True,override=True,section=1,outer=1)))
 variants.append(('retained random banks',dict(random=True,index=23,override=True)))
 for gate in ['absent_lightmaps','zero_groups','negative_groups','absent_bsp','absent_bitmap','checksum_mismatch']:
  for flag in [0,1]:variants.append(('original lighting gate and header fallback',dict(index=23,lightmap_gate=gate,body_flag=flag)))
 for mutation in ['owner_none','stale_salt','empty_salt','past_highwater']:
  variants.append(('original BADC0 full-owner rejection',dict(index=23,lightmap_gate='absent_lightmaps',body_flag=1,owner_rejection=mutation)))
 for index in [0,7]:variants.append(('original BSP geometry request failure',dict(index=index,geometry_request_fail=True)))
 for policy in [0,1,65535]:variants.append(('original instance lighting policy',dict(index=7,lighting_policy=policy)))
 variants.append(('original lightmap request failure',dict(index=7,lighting_request_fail=True)))
 for matching,failed in [(1,0),(0,0),(1,1)]:variants.append(('declared valid dynamic lightmap reference banks',dict(index=23,dynamic_lightmap=True,model_checksum_matches=matching,lighting_request_fail=failed)))
 fixture=[0x55443248,3,len(variants)];cases=[];rng=random.Random(0x2cbf02cb30)
 for label,v in variants:
  vm=s.guest();vm.mem_map(0x70000000,0x100000);vm.hook_add(UC_HOOK_CODE,trace)
  # Clone actual post-view/pre-secondary state, not an inferred main prefix.
  for b,e,permissions in g.mem_regions():
   if b>=0x70000000:continue
   vm.mem_map(b,e-b+1,permissions);vm.mem_write(b,bytes(g.mem_read(b,e-b+1)))
  vm.mem_write(center,struct.pack('<3f',-1.2,.8,2.4))
  mode=v.get('mode',0);write(vm,ctx+0x2a78,pack([mode]));write(vm,0x509410,bytes([v.get('gouter',0),v.get('ginner',0)]))
  if 'count'in v:write(vm,0x4c0b78,pack([v['count']]))
  if 'used'in v:write(vm,ctx+0xa5c,struct.pack('<H',v['used']))
  if 'override_count'in v:write(vm,0x4c1528,pack([v['override_count']]))
  if v.get('random'):
   for at,n in [(0x4ba138,850*8),(0x4c152c,32*9),(0x4c1228,192),(ctx+0x25c,512)]:write(vm,at,pack([rng.getrandbits(32)for _ in range(n)]))
  index=v.get('index',23);unit=source_units[index]
  geometry=v.get('geometry',unit[3]>>9&511);instance=unit[3]>>18&2047;distance=unit[0]>>21&255
  flags=v.get('flags',unit[0]&0x1fffff)
  args=[flags,unit[1],unit[2],unit[3]&15,unit[4],unit[6]&65535,unit[6]>>16&255,unit[7],0xe1a2002d if v.get('override')else NONE,bits(float(read(g,0x4c0b84+index,1)[0])*4),v.get('section',31),v.get('kind',1),v.get('outer',0),v.get('inner',0),v.get('selected',0),v.get('fade',bits(.4)),v.get('fade',bits(.6)),bits(-2.),unit[6]>>24&15,center,bits(.5)]
  contexts=[unit[4]]if unit[4]else[]
  def context_words():
   out=[len(contexts)]
   for pointer in contexts:out.extend([pointer,*words(read(vm,pointer,0x78))])
   return out
  if v.get('null_geometry'):
   if unit[1]!=NONE:
    root=val(vm,table+(unit[1]&65535)*16+8);metadata=val(vm,root+0x28)+(unit[3]>>9&511)*0x5c+4;write(vm,metadata+0x30,pack([0]))
   elif flags&0x1000:
    inst=val(vm,bsp+0x144)+instance*88;definition=half(vm,inst+0x34);metadata=val(vm,bsp+0x13c)+definition*200;write(vm,metadata+0x50,pack([0]))
   else:metadata=val(vm,bsp+0xa0)+(unit[3]>>9&511)*176;write(vm,metadata+0x50,pack([0]))
  lightmaps=val(vm,0x4e0344);group=val(vm,lightmaps+0x84)
  if v.get('dynamic_lightmap'):
   instance=0;model=val(vm,table+(unit[1]&65535)*16+8)
   write(vm,group+0x5c,pack([0x70002000]));write(vm,group+0x64,pack([0x70003000]))
   write(vm,0x70002000,pack([0,0,val(vm,model+8)^(not v['model_checksum_matches'])]))
   write(vm,0x70003000,read(vm,val(vm,group+0x54),12))
  if 'lighting_policy'in v:write(vm,val(vm,bsp+0x144)+instance*88+0x56,struct.pack('<H',v['lighting_policy']))
  if v.get('geometry_request_fail'):
   metadata=val(vm,bsp+0xa0)+(unit[3]>>9&511)*176 if unit[3]>>18&2047==2047 else val(vm,bsp+0x13c)+half(vm,val(vm,bsp+0x144)+instance*88+0x34)*200
   write(vm,metadata+0x28+0x1e,b'\0');write(vm,metadata+0x28+8,pack([0]))
  if v.get('lighting_request_fail'):
   reference=val(vm,group+(0x64 if v.get('dynamic_lightmap')else 0x54))+instance*12
   resource=val(vm,group+0x44)+half(vm,reference+2)*56+12
   write(vm,resource+0x1e,b'\0');write(vm,resource+8,pack([0]))
  gate=v.get('lightmap_gate')
  if gate=='absent_lightmaps':write(vm,0x4e0344,pack([0]))
  elif gate=='zero_groups':write(vm,lightmaps+0x80,pack([0]))
  elif gate=='negative_groups':write(vm,lightmaps+0x80,pack([NONE]))
  elif gate=='absent_bsp':write(vm,0x4e0348,pack([0]))
  elif gate=='absent_bitmap':write(vm,group+0x1c,pack([NONE]))
  elif gate=='checksum_mismatch':write(vm,group+4,pack([val(vm,bsp+8)^1]))
  if 'body_flag'in v:
   owner=val(vm,unit[4]);pool=val(vm,0x4e0300);header=val(vm,pool+0x44)+(owner&65535)*val(vm,pool+0x24);body=val(vm,header+8)
   write(vm,body+7,bytes([(read(vm,body+7,1)[0]&254)|v['body_flag']]))
   rejection=v.get('owner_rejection')
   if rejection=='owner_none':write(vm,unit[4],pack([NONE]))
   elif rejection=='stale_salt':write(vm,header,struct.pack('<H',half(vm,header)^1))
   elif rejection=='empty_salt':write(vm,header,b'\0\0')
   elif rejection=='past_highwater':write(vm,pool+0x38,pack([owner&65535]))
  before=state(vm)+context_words();services=[];pending={};requests=[];request_pending={};memory_reads=[]
  # Independent pre-call data supplies native44940/2D000/449E0; original
  # service returns remain expectations.12DE70 alone remains an explicit
  # native publication service, including its ordered mode/address/result.
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
   if pc in request_pending:
    request_pending.pop(pc).append(guest.reg_read(R.UC_X86_REG_EAX)&255)
   if pc==0x12de70:
    esp=guest.reg_read(R.UC_X86_REG_ESP);row=[guest.reg_read(R.UC_X86_REG_EAX),guest.reg_read(R.UC_X86_REG_ECX)];requests.append(row);request_pending[val(guest,esp)]=row
   if pc in pending:
    entry,row,output=pending.pop(pc);result=guest.reg_read(R.UC_X86_REG_EAX)
    if entry==0x44940:
     row.extend([half(guest,result+0x14),half(guest,result+0x24),result])
     # The native metadata value carries both fields even when this specific
     # allocator call does not later request a mask. Export their independent
     # pre-call bytes as owned metadata, not the observed result words.
     memory_reads.extend([((result+offset)&~3,(result+offset+5)&~3)for offset in [0x14,0x24]])
    elif entry in (0x2d000,0x449e0):row.append(result)
    else:row.extend(words(read(guest,output,16)))
    return
   if pc not in [0x44940,0x2d000,0x449e0,0x30290]:return
   esp=guest.reg_read(R.UC_X86_REG_ESP);return_pc=val(guest,esp);eax,ecx,edx=[guest.reg_read(reg)for reg in [R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX]]
   if pc==0x44940:row=[pc,eax&65535,val(guest,esp+4),*words(read(guest,0x4ba138+(eax&65535)*32,32))];output=0
   elif pc==0x449e0:row=[pc,eax&65535,val(guest,esp+4),edx&255,*words(read(guest,0x4ba138+(eax&65535)*32,32))];output=0
   elif pc==0x2d000:row=[pc,eax,edx,ecx];output=0
   else:row=[pc,eax,val(guest,esp+4)];output=guest.reg_read(R.UC_X86_REG_ESI)
   services.append(row);pending[return_pc]=(pc,row,output)
  vm.hook_add(UC_HOOK_CODE,calls)
  if 'begin'in v:write(vm,ctx+0x2a78,pack([v['begin']]));mode=v['begin'];run(vm,0x2cb30);result=0xffff
  else:run(vm,0x2cbf0,args,{R.UC_X86_REG_EAX:geometry,R.UC_X86_REG_ECX:instance,R.UC_X86_REG_EDX:distance});result=vm.reg_read(R.UC_X86_REG_EAX)&65535
  assert not pending and not request_pending,(pending,request_pending)
  after=state(vm)+context_words()+[result]
  inputs=[int('begin'in v),mode,ctx+0x25c,v.get('gouter',0),v.get('ginner',0),geometry,instance,distance,*args,*words(read(vm,center,12)),*words(read(vm,0x4b9e14,192)),*words(read(vm,0x4b9da0,12)),*[half(vm,0x4b9dd0+i*2)for i in range(4)],*before,len(services)]
  for row in services:inputs.extend([len(row),*row])
  merged=[]
  for start,end in sorted(memory_reads):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  inputs.extend([val(vm,0x4e3b44),val(vm,0x4e0348),val(vm,0x4e0344),val(vm,0x4e0300),len(merged)])
  for start,end in merged:inputs.extend([start,(end-start)//4,*words(initial_read(start,end-start))])
  inputs.extend([len(requests),*[w for row in requests for w in row]])
  fixture.extend([len(inputs),len(after),*inputs,*after]);cases.append(dict(label=label,mutation=v,result=result,checked_words=len(after),services=len(services),resource_requests=len(requests),source_memory_bytes=sum(end-start for start,end in merged)))
 path=a.output/'fixture.bin';path.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=cases,checked_words=sum(c['checked_words']for c in cases),service_calls=sum(c['services']for c in cases),resource_requests=sum(c['resource_requests']for c in cases),fixture_sha256=sha(path.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(path.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');path.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
