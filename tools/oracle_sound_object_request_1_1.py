#!/usr/bin/env python3
"""Original18DDE0 on unchanged owned equipment and declared marker controls.

Only observations and direct helper ABI stimuli are added. Source pool/body/
node/parent/physics/PVS state is never assigned. The inherited clock/world/
local bindings remain explicit; unavailable audio startup still rejects.
"""
import argparse, hashlib, json, struct
from pathlib import Path
import unicorn
from unicorn.x86_const import *
from oracle_smg_fp_sound_1_1 import SoundTrace,ROOT,PIN,CACHE_PIN
from oracle_smg_firing_1_1 import UNIT,STOP,_trace

BACKING=0x7d00000
ENTRIES=(0x18dde0,0xbbcb0,0xbabd0,0xbb1f0,0xb9ac0,0xbb0c0,
 0x1d43c0,0x1d44c0,0x143ce0,0x18a800,0x11ceb0,0x11d4a0,
 0x126ab0,0x24bef0,0x24bf30,0x18eee0,0x18ef20,0xbbea0,0x156eb0)
FINAL=ROOT/'analysis/multiplayer/integration/fp-sound-request-native/final-manifest.json'
FINAL_PIN='e757342c24c736017f5d44dac4148005144266e0bcb81e27309afa8cee89391d'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def encoded(x):return (json.dumps(x,sort_keys=True,indent=2)+'\n').encode()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,('Existing evidence differs',str(p))
 else:p.write_bytes(b)
def pins():
 assert sha(FINAL)==FINAL_PIN
 values=json.loads(FINAL.read_text())['files']
 for p,h in values.items():assert sha(ROOT/p)==h,('Prior sound freeze changed',p)
 return values
def regions(memory):
 out=[]
 for address,value in sorted(memory.items()):
  if out and address==out[-1][0]+len(out[-1][1]):out[-1][1].append(value)
  else:out.append([address,bytearray([value])])
 return [dict(address=f'{p:08x}',size=len(raw),raw_hex=raw.hex())for p,raw in out]

class ObjectTrace(SoundTrace):
 def __init__(self):
  self.object_enabled=False;self.object_scope=None;self.object_rows=[];self.compress_rows=[]
  super().__init__()
  self.g.mem_map(BACKING,0x1000)
  self.object_modes=[]
  for view in range(4):
   self.fp_action('original_read_only_sound_camera_mode',0x156eb0,regs=((UC_X86_REG_EAX,view),))
   self.object_modes.append(self.g.reg_read(UC_X86_REG_EAX))
  self.object_routines={e:_trace(self.code,e,self.starts|set(ENTRIES))for e in ENTRIES}
  self.object_owned={a:e for e,r in self.object_routines.items()for a in r.code}
  self.object_executed=set();self.object_enabled=True
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.object_read)
 def local_context(self):
  views=[]
  for v in range(4):
   player=self.u(self.u(0x4f01f0)+12+v*4)
   unit=0xffffffff if player==0xffffffff else self.u(self.u(self.u(0x4f01f4)+0x44)+(player&0xffff)*0x21c+0x2c)
   mode=self.object_modes[v]
   views.append(dict(view=v,player=f'{player:08x}',unit=f'{unit:08x}',mode=f'{mode:08x}'))
  return views
 def roots(self):
  return dict(object_pool=f'{self.u(0x4e78d0):08x}',physics_pool=f'{self.u(0x526248):08x}',
   sound_system=f'{self.u(0x4ed950):08x}',render_bsp=f'{self.u(0x4e7918):08x}',
   current_bsp=f'{self.u(0x46fb64,2):04x}')
 def close(self,a):
  r=self.object_scope
  if r and a==r['return_address_integer'] and self.g.reg_read(UC_X86_REG_ESP)>r['stack_integer']:
   r['result_al']=self.g.reg_read(UC_X86_REG_EAX)&255
   r['location_after_hex']=bytes(self.g.mem_read(r['location_integer'],48)).hex()
   r['memory']=regions(r.pop('memory_integer'));r['roots_after']=self.roots()
   r['shared_local_rng_after_hex']=bytes(self.g.mem_read(self.u(0x4ee9d8),8)).hex()
   for key in('return_address_integer','stack_integer','location_integer'):r.pop(key)
   self.object_scope=None
 def hook(self,g,a,size,user):
  if self.object_enabled:
   self.close(a)
   if a==0x18dde0:
    assert self.object_scope is None
    sp=g.reg_read(UC_X86_REG_ESP);owner,sound,marker,location=[self.u(sp+4+i*4)for i in range(4)]
    row=dict(id=len(self.object_rows),phase=self.active,tick=self.tick,owner=f'{owner:08x}',sound=f'{sound:08x}',
     marker40_hex=bytes(g.mem_read(marker,40)).hex(),location_before_hex=bytes(g.mem_read(location,48)).hex(),
     roots=self.roots(),locals=self.local_context(),fpcw=f'{g.reg_read(UC_X86_REG_FPCW):04x}',
     shared_local_rng_before_hex=bytes(g.mem_read(self.u(0x4ee9d8),8)).hex(),
     return_address_integer=self.u(sp),stack_integer=sp,location_integer=location,
     memory_integer={},reads=[],stores=[],calls=[],point_queries=[])
    self.object_rows.append(row);self.object_scope=row
   r=self.object_scope
   if r:
    self.object_executed.add(a)
    if a in ENTRIES:
     sp=g.reg_read(UC_X86_REG_ESP)
     r['calls'].append(dict(entry=f'{a:08x}',caller=f'{self.u(sp):08x}',registers=self.registers(),
         stack=[f'{self.u(sp+4+n*4):08x}'for n in range(4)]))
    if a==0x11d4a0:
     point=g.reg_read(UC_X86_REG_EDX);out=g.reg_read(UC_X86_REG_ESI)
     r['point_queries'].append(dict(point12_hex=bytes(g.mem_read(point,12)).hex(),output_address=f'{out:08x}'))
    if r['point_queries'] and a==0x18df82:
     q=r['point_queries'][-1];q['output8_hex']=bytes(g.mem_read(int(q['output_address'],16),8)).hex()
  super().hook(g,a,size,user)
 def run(self,*args,**kwargs):
  super().run(*args,**kwargs)
  if self.object_enabled:self.close(STOP)
 def object_read(self,g,access,address,size,value,user):
  r=self.object_scope
  if not r or g.reg_read(UC_X86_REG_EIP)not in self.object_owned:return
  if 0x2000000<=address<0x2200000 or BACKING<=address<BACKING+0x1000:return
  raw=bytes(g.mem_read(address,size))
  for n,b in enumerate(raw):
   old=r['memory_integer'].setdefault(address+n,b);assert old==b,'Sound owner source backing changed during read-only request'
  r['reads'].append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=raw.hex()))
 def mem_write(self,g,access,address,size,value,user):
  r=self.object_scope
  if r and r['location_integer']<=address and address+size<=r['location_integer']+48:
   r['stores'].append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',offset=address-r['location_integer'],size=size,
    before_hex=bytes(g.mem_read(address,size)).hex(),after_hex=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
  super().mem_write(g,access,address,size,value,user)
 def control(self,*args,**kwargs):return super().control(*args,**kwargs)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 prior=pins();cases=[];compress=[];listings={};services={};contexts=[];definitions={}
 for scenario,ticks in(('ready',2),('reload',48),('empty_reload',125)):
  t=ObjectTrace();t.partial_initialization();t.authored_sound()
  if scenario=='ready':t.g.mem_write(UNIT+0x212,b'\xff')
  t.execute_fp(ticks,'idle'if scenario=='ready'else scenario)
  assert len(t.object_rows)==1 and t.object_scope is None
  old=ROOT/'analysis/multiplayer/map-pack/smg-fp-sound/frozen'/f'{scenario}.json'
  if not old.exists():old=ROOT/'analysis/multiplayer/map-pack/smg-fp-sound'/f'{scenario}.json'
  # Compare only owned lower request outputs from the pinned original report;
  # observation never substitutes that report for the new original execution.
  legacy=json.loads(old.read_text());call=next(r for r in legacy['calls']if r['entry']=='0018dde0')
  actual=t.object_rows[0]
  assert actual['location_before_hex']==call['request_before_hex'][:96]
  assert actual['location_after_hex']==call['request_after_hex'][:96]
  assert actual['result_al']==int(call['return_eax'],16)&255
  actual['authored_correlation']=dict(path=str(old.relative_to(ROOT)),sha256=sha(old),owned_before_after_equal=True)
  cases.extend(t.object_rows)
  services[scenario]={f'{k:08x}':v for k,v in t.services.items()};contexts.append(t.fp_external_inputs)
  if scenario=='ready':
   original_marker=bytes.fromhex(actual['marker40_hex'])
   stimuli=[]
   for name,owner in(('none',0xffffffff),('salt_mismatch',0x20002),('highwater',0x1003f),
                      ('actual_biped_direct_composition',0x10001),('actual_weapon_direct_composition',t.initial_datum)):
    stimuli.append((name,owner,original_marker))
   for n,(node,pos,forward)in enumerate(((255,(0.,0.,0.),(1.,0.,0.)),
      (0,(.125,-.25,.5),(0.,1.,0.)),(0,(-.5,.75,-.125),(-1.,0.,0.)),
      (0,(1.,2.,3.),(3.,-3.,1.5)),(0,(-0.,0.,-0.),(0.,0.,-1.)))):
    marker=bytearray(original_marker);marker[0]=node;struct.pack_into('<3f',marker,4,*pos);struct.pack_into('<3f',marker,0x10,*forward)
    stimuli.append((f'declared_marker_{n}',t.initial_datum,bytes(marker)))
   for name,owner,marker in stimuli:
    t.g.mem_write(BACKING,marker);t.g.mem_write(BACKING+0x100,bytes.fromhex('aabbccdd')*12)
    t.fp_action(name,0x18dde0,(owner,0xe5ac0437,BACKING,BACKING+0x100))
    assert t.object_scope is None
   for owner in(0x10001,t.initial_datum):
    t.g.mem_write(BACKING,original_marker);location=bytearray.fromhex('aabbccdd'*12);location[3]=0x11
    t.g.mem_write(BACKING+0x100,bytes(location))
    t.fp_action('declared_audibility_one_actual_parent_chain',0x18dde0,(owner,0xe5ac0437,BACKING,BACKING+0x100))
    assert t.object_scope is None
   cases.extend(t.object_rows[1:])
   # Explicit standalone scalar inputs. These do not replace any owned matrix,
   # source object, successful location/voice/admission or runtime RNG state.
   values=[0x00000000,0x80000000,0x3f800000,0xbf800000,0x40000000,0xc0000000,
           0x3effffff,0x3f000000,0x3f000001,0xbeffffff,0xbf000000,0xbf000001,
           0x7f800000,0xff800000,0x7fc00000,0x7fa00001,0x00000001,0x80000001]
   state=0x31a927bc
   for n in range(512):
    words=[]
    for j in range(3):state=(state*1664525+1013904223)&0xffffffff;words.append(state)
    if n<len(values):words=[values[n],values[(n+5)%len(values)],values[(n+11)%len(values)]]
    t.g.mem_write(BACKING+0x300,struct.pack('<3I',*words))
    t.fp_action('declared_compression_scalar_input',0x18a800,regs=((UC_X86_REG_EAX,BACKING+0x300),))
    compress.append(dict(input12_hex=struct.pack('<3I',*words).hex(),result=f'{t.g.reg_read(UC_X86_REG_EAX):08x}',source_fpcw='027f'))
  for entry,r in t.object_routines.items():
   seq=[t.code.at(q)for q in sorted(r.code)]
   for i in seq:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
   raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in seq).encode()
   path=f'instructions/{entry:08x}.asm';save(a.output/path,raw);listings[path]=sha(a.output/path)
  c=t.cache;class_tag=next(tag for tag in c.tags.values()if tag['group']=='sncl')
  classes=c.block(class_tag['offset'],92)
  for row in t.object_rows:
   datum=int(row['sound'],16);tag=c.tags[datum];raw=bytes(c.data[tag['offset']:tag['offset']+20]);index=struct.unpack_from('<b',raw,2)[0]
   definitions[row['sound']]=dict(sound20_hex=raw.hex(),class_index=index,class92_hex=bytes(c.data[classes[index]:classes[index]+92]).hex())
  del t
 report=dict(schema='original-sound-object-request-v1',tool_sha256=sha(__file__),prior_manifest_sha256=FINAL_PIN,
  prior_pins=prior,xbe_sha256=PIN,cache_sha256=CACHE_PIN,cases=cases,compression_controls=compress,
  instruction_pins=listings,external_contexts=contexts,inherited_services=services,definitions=definitions,
  new_guest_object_fields=[],new_successful_service_returns=[],instruction_patches=[],native_runtime_connected=False,
  scope='Three original authored ready/reload/empty-reload schedules unchanged. Direct owner/marker controls compose existing allocated weapon or existing explicit biped into ABI; they are not actual owning FP events. All source object/node/location/physics bytes are observed, never initialized by this tool. Compression scalar controls are declared standalone inputs. Old local/world/resource/clock services remain explicit; original audio admission is still unavailable.')
 pins();save(a.output/'report.json',encoded(report))
 print(json.dumps(dict(cases=len(cases),stores=sum(len(r['stores'])for r in cases),compression_controls=len(compress),report_sha256=sha(a.output/'report.json'))))
if __name__=='__main__':main()
