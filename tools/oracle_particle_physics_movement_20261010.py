#!/usr/bin/env python3
"""Execute original physics0 movement over authored canister controllers.

The source-created SMG particles/system and physics constants are explicit
cross-composed kernel inputs, not an observed canister particle lifetime.
No terrain, allocation, animation, IO or effect success is intercepted.
"""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from inspect_multiplayer import Cache
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_REG,X86_OP_MEM,X86_OP_IMM
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBX,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_MXCSR,UC_X86_REG_XMM0
OLD=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
NEW=Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
INPUTS={
 'analysis/multiplayer/integration/inventory-source/smg-local-effect-owner/audio-unavailable/report.json':'e30bf2ee9f87012a8778e370196679d2ee9739bbbb3409b85e238f611656dcc5',
 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03':'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
 str(NEW):'21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'}
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def raw(x,p,n):
 for s in x.sections:
  b=x.section_bytes(s)
  if s.va<=p and p+n<=s.va+len(b):return b[p-s.va:p-s.va+n]
 raise ValueError(hex(p))
def find(x,b):
 hits=[]
 for s in x.sections:
  data=x.section_bytes(s);at=data.find(b)
  while at>=0:hits.append(s.va+at);at=data.find(b,at+1)
 assert len(hits)==1,hits
 return hits[0]
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();assert not a.output.exists()
 for p,h in INPUTS.items():assert digest(ROOT/p)==h,p
 original=json.loads((ROOT/next(iter(INPUTS))).read_text())['final_state']
 gravity_path=ROOT/'analysis/multiplayer/integration/spawn-source/live-body-world/report.json'
 gravity_report=json.loads(gravity_path.read_text());gravity=bytes.fromhex(gravity_report['cases'][0]['before']['physics_constants_hex'])
 assert len(gravity)==0x1c
 metadata_path=ROOT/'analysis/multiplayer/sandbox-source/live-prop/explosion-definitions.json'
 metadata=json.loads(metadata_path.read_text());systems=[]
 for event in metadata['events']:
  for system in event['systems']:
   for emitter in system['emitters']:
    m=emitter['movement']
    if m:
     controller=m['controllers'][0];assert controller['type']==0
     systems.append(dict(event=event['index'],system=system['index'],emitter=emitter['index'],particle=system['particle'],movement=m,
       controller_address=int(controller['record']['source_address'],16),controller=controller))
 assert len(systems)==6
 pool=next(p for p in original['pools']if p['global_address']=='00526514')
 system=next(p for p in original['pools']if p['global_address']=='00518464')['occupied'][0]
 location=next(p for p in original['pools']if p['global_address']=='0052651c')['occupied'][0]
 particles=pool['occupied'];assert len(particles)==2
 before=bytes.fromhex(''.join(p['raw_hex']for p in particles));assert len(before)==128
 dt=struct.unpack_from('<I',bytes.fromhex(original['clock']['raw24_hex']),4)[0]
 cases=[]
 for s,head,elapsed,flags,age,angular in itertools.product(range(6),range(2),(0,dt,0x3f800000,0x42f00000,dt|0x80000000),(0,1,8),(0,0x3f800000,0x3f800001),(None,0x41880000)):
  b=bytearray(before);at=head*64;oldflags=struct.unpack_from('<H',b,at+2)[0]
  struct.pack_into('<H',b,at+2,(oldflags&~9)|flags);struct.pack_into('<I',b,at+8,age)
  if angular is not None:struct.pack_into('<I',b,at+0x34,angular)
  cases.append(dict(system=s,head=head,elapsed_bits=elapsed,flag_control=flags,age_control=age,angular_control=angular,before=bytes(b)))
 old=Xbe(OLD);new=Xbe(NEW);entry=0x2bf130;newentry=find(new,raw(old,entry,32));end=entry+0x245
 assert raw(old,end-3,3)==b'\xc2\x0c\x00'
 cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True;canonical=[];images=[];signatures=[]
 a.output.mkdir(parents=True)
 cache=Cache(ROOT/'assets/disc/maps/lockout.map')
 for name,x,start in [('tu11',old,entry),('u5',new,newentry)]:
  ins=list(cs.disasm(raw(x,start,end-entry),start));assert ins[-1].mnemonic=='ret'
  absolute=[];calls=[];signature=[]
  for i in ins:
   operands=[]
   for o in i.operands:
    if o.type==X86_OP_MEM and not o.mem.base and not o.mem.index:
     if o.mem.disp not in absolute:absolute.append(o.mem.disp)
     operands.append(('absolute',absolute.index(o.mem.disp),o.size))
    elif o.type==X86_OP_REG:operands.append(('register',o.reg,o.size))
    elif o.type==X86_OP_IMM and i.group(1):operands.append(('branch',o.imm-start))
    elif o.type==X86_OP_IMM and i.group(2):
     if o.imm not in calls:calls.append(o.imm)
     operands.append(('call',calls.index(o.imm)))
    elif o.type==X86_OP_IMM:operands.append(('immediate',o.imm,o.size))
    else:operands.append(('memory',o.mem.base,o.mem.index,o.mem.scale,o.mem.disp,o.size))
   signature.append((i.mnemonic,operands))
  signatures.append(signature);assert len(absolute)==3 and len(calls)==2,(absolute,calls)
  # Full actual section/cache bytes and actual source-created retained records.
  # Only their owning global locator slots are bound at this kernel ABI.
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x900000)
  for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
  base=cache.magic&~4095;g.mem_map(base,(cache.magic+len(cache.data)-base+4095)&~4095);g.mem_write(cache.magic,bytes(cache.data))
  g.mem_map(0x82ec0000,0x60000);g.mem_map(0x1000000,0x200000);g.mem_map(0x2000000,0x100000);g.mem_map(0x3000000,4096)
  w=lambda p,v:g.mem_write(p,struct.pack('<I',v&0xffffffff))
  for p in original['pools']:
   g.mem_write(int(p['pointer'],16),bytes.fromhex(p['header4c_hex']))
   for item in p['occupied']:g.mem_write(int(item['pointer'],16),bytes.fromhex(item['raw_hex']))
  # Source one/global-pool/gravity identities follow the complete paired leaf.
  w(absolute[0],int(pool['pointer'],16));w(absolute[2],0x1000100);g.mem_write(0x1000100,gravity)
  assert raw(x,absolute[1],4)==struct.pack('<f',1.)
  stack=0x20ff000;particle_base=int(pool['data_pointer'],16);rows=[];stores=[];observed=set();evaluations=[];pending=[]
  def code(uc,address,size,user):
   observed.add(address)
   if pending and address==pending[-1][0]:
    _,row=pending.pop();row['result_bits']=uc.reg_read(UC_X86_REG_XMM0)&0xffffffff
   if address==calls[1]:
    sp=uc.reg_read(UC_X86_REG_ESP);param=uc.reg_read(UC_X86_REG_ESI);values=uc.reg_read(UC_X86_REG_EBX)
    row=dict(property=f'{param:08x}',property16_hex=bytes(uc.mem_read(param,16)).hex(),values68_hex=bytes(uc.mem_read(values,68)).hex())
    evaluations.append(row);pending.append((struct.unpack('<I',uc.mem_read(sp,4))[0],row))
  def write(uc,access,address,size,value,user):
   if particle_base<=address<particle_base+128:stores.append(dict(instruction=f'{uc.reg_read(UC_X86_REG_EIP):08x}',offset=address-particle_base,size=size,value=value&((1<<(size*8))-1)))
  g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,write)
  for index,c in enumerate(cases):
   g.mem_write(stack-0x1000,b'\0'*0x1100);g.mem_write(particle_base,c['before']);s=systems[c['system']]
   g.mem_write(stack,struct.pack('<4I',0x3000000,s['controller_address'],int(particles[c['head']]['datum'],16),c['elapsed_bits']))
   for reg,value in [(UC_X86_REG_ESP,stack),(UC_X86_REG_ECX,int(system['pointer'],16)),(UC_X86_REG_FPCW,0x27f),(UC_X86_REG_MXCSR,0x1f80),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)]:g.reg_write(reg,value)
   stores.clear();evaluations.clear();pending.clear();g.emu_start(start,0x3000000,count=100000)
   assert g.reg_read(UC_X86_REG_EIP)==0x3000000 and not pending
   after=bytes(g.mem_read(particle_base,128));rows.append({**{k:v for k,v in c.items()if k!='before'},'id':index,'before_hex':c['before'].hex(),'after_hex':after.hex(),'stores':stores.copy(),'evaluations':evaluations.copy()})
   if name=='tu11':canonical.append(after)
   else:assert after==canonical[index],index
  listing=''.join(f'{i.address:08x} {bytes(i.bytes).hex()} {i.mnemonic} {i.op_str}\n'for i in ins);(a.output/(name+'.asm')).write_text(listing)
  # Executed lower function bytes are recorded separately, including real
  # 175620/24AEC0 and248DA0/13CB00 operations. There are no emulated services.
  lines=[]
  for p in sorted(observed):
   i=next(cs.disasm(raw(x,p,16),p));lines.append(f'{p:08x} {bytes(i.bytes).hex()} {i.mnemonic} {i.op_str}\n')
  (a.output/(name+'-executed.asm')).write_text(''.join(lines))
  images.append(dict(name=name,sha256=digest(OLD if name=='tu11'else NEW),entry=f'{start:08x}',end=f'{start+end-entry:08x}',absolute_refs=[f'{p:08x}'for p in absolute],call_targets=[f'{p:08x}'for p in calls],cases=rows))
 assert signatures[0]==signatures[1]
 fixture=b'PMV0'+struct.pack('<I',len(cases))+b''.join(struct.pack('<III',c['system'],c['head'],c['elapsed_bits'])+c['before']+after for c,after in zip(cases,canonical))
 (a.output/'fixtures.bin').write_bytes(fixture)
 effect_pool=next(p for p in original['pools']if p['global_address']=='004f1f0c')
 effect=next(p for p in effect_pool['occupied']if int(p['datum'],16)==struct.unpack_from('<I',bytes.fromhex(system['raw_hex']),0x18)[0])
 context=b'PMVC'+struct.pack('<I',len(systems))+bytes.fromhex(system['raw_hex'])+bytes.fromhex(effect['raw_hex'])+bytes.fromhex(original['clock']['raw24_hex'])[:12]+bytes(36)+gravity[:4]+raw(old,0x465030,4)
 context+=b''.join(struct.pack('<I',int(s['movement']['tag']['datum'],16))for s in systems)
 (a.output/'context.bin').write_bytes(context)
 store_fixture=b'PMVS'+struct.pack('<I',len(cases))
 for c in images[0]['cases']:
  store_fixture+=struct.pack('<I',len(c['stores']))
  store_fixture+=b''.join(struct.pack('<4I',int(s['instruction'],16),s['offset'],s['size'],s['value'])for s in c['stores'])
 (a.output/'stores.bin').write_bytes(store_fixture)
 report=dict(schema='authored-particle-physics0-v1',tool_sha256=digest(Path(__file__)),input_pins={**INPUTS,str(gravity_path.relative_to(ROOT)):digest(gravity_path),str(metadata_path.relative_to(ROOT)):digest(metadata_path)},
 source_system=system,source_location=location,source_pools=original['pools'],source_clock=original['clock'],source_camera_hex='00'*36,source_gravity_hex=gravity.hex(),systems=systems,images=images,fixture_sha256=hashlib.sha256(fixture).hexdigest(),
 boundaries=['Complete physics0 leaf2BF130 only; no collider1/swarm2/wind3 success or complete249210/2C06C0 schedule claim.',
 'Actual canister metadata is cross-composed with genuine source-created SMG particles/system and actual1EEF40 gravity record. This is an explicit CPU kernel fixture, not original canister birth/lifetime.',
 'Elapsed/flags/age/angular variations are declared scalar controls. Complete128 particle bytes retained, all original property/value functions execute; no supplied returns, contacts or allocated identities.',
 'Private caller stack bytes are declared zero raw ABI storage. Unrequested value-cache slots are unspecified source temporaries; only evaluated arguments/results are meaningful.',
 'Masked x87/MXCSR fixture; original/native output comparison does not establish physical Xbox FP status.'])
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
 print(json.dumps(dict(cases=len(cases),report_sha256=digest(a.output/'report.json'),fixture_sha256=report['fixture_sha256'],u5_entry=f'{newentry:08x}')))
if __name__=='__main__':main()
