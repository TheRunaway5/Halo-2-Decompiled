#!/usr/bin/env python3
"""Observe unchanged retained constructors during genuine original player creation.

All allocations, graph selection and component contents are original outputs.
The old whole compatibility/storage/options/resource boundaries remain explicit.
"""
import argparse, hashlib, json, os, struct, subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EDX
import oracle_mp_spawn_player_visibility as owning
from oracle_smg_firing_1_1 import STOP, PIN, CACHE_PIN
from functions import _trace

ROOT = owning.ROOT
OWNERS = (0x1ce8c0, 0x1ce9b0, 0x1ceac0, 0xbd270, 0xc52c0)
LEAVES = (0x1d0a60, 0x18fd70, 0x110a20, 0x116680, 0x110bf0)
FROZEN = {**owning.FROZEN,
 'tools/oracle_mp_spawn_player_visibility.py': 'aeafe79cff80e9d607e843d5bdb9f7c86b2814df15ec1b1179ebf4fe505e8572',
 'analysis/multiplayer/integration/spawn-source/player-visibility/report.json': 'f58232e963b26bcb4a6471db8c40488f2bb195c4be5cf8e0617331ae4d5f78dd'}

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def encoded(x): return (json.dumps(x, sort_keys=True, indent=2) + '\n').encode()
def save(p, raw):
 p.parent.mkdir(parents=True, exist_ok=True)
 if p.exists(): assert p.read_bytes() == raw, ('Immutable output differs', str(p))
 else: p.write_bytes(raw)

class ConstructorTrace(owning.PlayerTrace):
 def __init__(self, data):
  self.ctor_enabled=False; self.ctor_calls=[]; self.ctor_pending=[]; self.ctor_stores=[]; self.ctor_reads=[]; self.ctor_order=0; self.ctor_executed=set()
  super().__init__(data)
  self.ctor_routines={e:_trace(self.code,e,self.starts|set(OWNERS+LEAVES)) for e in OWNERS+LEAVES}
  self.ctor_addresses={a:e for e,r in self.ctor_routines.items() for a in r.code}
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.ctor_read)
 def raw(self,p,n): return bytes(self.g.mem_read(p,n)).hex()
 def pool(self):
  p=self.u(0x4e78d0); return dict(pointer=f'{p:08x}',raw4c_hex=self.raw(p,0x4c),headers=f'{self.u(p+0x44):08x}',capacity=self.u(p+0x20),high_water=self.u(p+0x38),slot_bytes=self.u(p+0x24))
 def object(self,unit):
  pool=self.pool(); i=unit&0xffff; assert i<pool['capacity']
  h=int(pool['headers'],16)+i*12; p=self.u(h+8); n=self.u(h+6,2)
  return dict(header=f'{h:08x}',header_hex=self.raw(h,12),body=f'{p:08x}',size=n,body_hex=self.raw(p,n) if p and n else '')
 def buffers(self,row):
  result={}
  if row.get('manager') is not None: result['M']=self.raw(row['manager'],0x90)
  if row.get('unit') is not None:
   o=self.object(row['unit']); result.update(H=o['header_hex'],U=o['body_hex'])
  if row.get('descriptor'): result['D']=self.raw(row['descriptor'],0xc4)
  if row.get('failure'): result['F']=self.raw(row['failure'],1)
  return result
 def metadata(self,row):
  e=int(row['entry'],16); out={}
  if e==0x1ceac0:
   g,m=row['registers']['edi'],row['registers']['edx']; table=self.u(0x4eb114)
   if g!=0xffffffff:
    p=self.u(table+(g&0xffff)*16+8);out['graph']=dict(datum=f'{g:08x}',pointer=f'{p:08x}',header_hex=self.raw(p,0x10),nodes=struct.unpack('<h',bytes(self.g.mem_read(p+12,2)))[0])
   if m!=0xffffffff:
    p=self.u(table+(m&0xffff)*16+8);out['model']=dict(datum=f'{m:08x}',pointer=f'{p:08x}',header_hex=self.raw(p,0x80),nodes=self.u(p+0x78))
  if e==0xc52c0:
   o=self.object(row['unit']); p=int(o['body'],16);tag=self.u(p);d=self.u(self.u(0x4eb114)+(tag&0xffff)*16+8)
   out['unit_definition']=dict(datum=f'{tag:08x}',pointer=f'{d:08x}',raw1b8_hex=self.raw(d,0x1b8))
   model=self.u(d+0x38)
   if model!=0xffffffff:
    m=self.u(self.u(0x4eb114)+(model&0xffff)*16+8); count=self.u(m+0x78);at=self.u(m+0x7c);assert count<1000
    out['model_regions']=dict(datum=f'{model:08x}',header80_hex=self.raw(m,0x80),record5c_hex=[self.raw(at+i*0x5c,0x5c) for i in range(count)])
   out['game_type']=self.u(self.u(0x4edf18)+8)
  return out
 def finish(self,a):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  # Tail-jumped reset has the same return frame as its constructor. Complete
  # both actual owning calls; no synthetic return or intermediate buffer.
  for q in reversed(self.ctor_pending[:]):
   ret,entry_sp,row=q
   if ret!=a or sp<=entry_sp: continue
   row.update(after=self.buffers(row),return_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',return_order=self.ctor_order)
   self.ctor_order+=1
   if 'unit' in row: row['object_after']=self.object(row['unit'])
   if int(row['entry'],16)==0x18fd70 and 'unit' in row and (self.g.reg_read(UC_X86_REG_EAX)&255):
    n=row['registers']['eax'];assert 0<=n<0x8000
    row['growth_backing_after_hex']=self.raw(int(row['object_after']['body'],16),n)
   self.ctor_pending.remove(q)
 def hook(self,g,a,size,user):
  if self.ctor_enabled:
   self.finish(a)
   if a in self.ctor_addresses: self.ctor_executed.add(a)
   if a in OWNERS+LEAVES:
    sp=g.reg_read(UC_X86_REG_ESP);regs={k:int(v,16) for k,v in self.registers().items()};s=[self.u(sp+4+i*4) for i in range(8)]
    row=dict(id=len(self.ctor_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.ctor_order,registers=regs,stack=[f'{v:08x}'for v in s],ancestors=[q[2]['id']for q in self.ctor_pending],pool=self.pool())
    if a in(0x1ce8c0,0x1ce9b0): row['manager']=regs['eax']
    elif a==0x1ceac0: row['manager']=regs['esi']
    elif a==0x1d0a60: row['manager']=regs['ecx']
    elif a==0xbd270: row.update(unit=regs['eax'],component_field=struct.unpack('<h',struct.pack('<H',s[0]&65535))[0],component_size=struct.unpack('<h',struct.pack('<H',s[1]&65535))[0],alignment=s[2]&255)
    elif a==0xc52c0: row.update(unit=s[0],descriptor=s[1],failure=s[2])
    else:
     parent=next((q[2]for q in reversed(self.ctor_pending)if 'unit'in q[2]),None)
     if parent: row['unit']=parent['unit']
    if 'unit'in row: row['object_before']=self.object(row['unit'])
    row['before']=self.buffers(row);row['metadata']=self.metadata(row)
    self.ctor_calls.append(row);self.ctor_pending.append((self.u(sp),sp,row));self.ctor_order+=1
  super().hook(g,a,size,user)
 def label(self,p,n,row):
  if row.get('manager') is not None and row['manager']<=p and p+n<=row['manager']+0x90: return ('M',p-row['manager'])
  if row.get('unit') is not None:
   o=self.object(row['unit']);h=int(o['header'],16);b=int(o['body'],16)
   if h<=p and p+n<=h+12:return('H',p-h)
   # Newly grown bytes already belong to the source header extent by the
   # time BD270 zeroes them. No guessed/reserved body extent is captured.
   if b<=p and p+n<=b+o['size']:return('U',p-b)
  if row.get('failure') and row['failure']<=p and p+n<=row['failure']+1:return('F',p-row['failure'])
  return None
 def mem_write(self,g,access,p,n,value,user):
  if self.ctor_enabled and self.ctor_pending:
   ip=g.reg_read(UC_X86_REG_EIP)
   if ip in self.ctor_addresses:
    active=[q[2]for q in self.ctor_pending];labels={}
    for row in active:
     label=self.label(p,n,row)
     if label: labels[str(row['id'])]=dict(buffer=label[0],offset=label[1])
    self.ctor_stores.append(dict(order=self.ctor_order,instruction=f'{ip:08x}',address=f'{p:08x}',size=n,before=self.raw(p,n),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex(),calls=[r['id']for r in active],labels=labels));self.ctor_order+=1
  super().mem_write(g,access,p,n,value,user)
 def ctor_read(self,g,access,p,n,value,user):
  if self.ctor_enabled and self.ctor_pending and g.reg_read(UC_X86_REG_EIP)in self.ctor_addresses:
   self.ctor_reads.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',size=n,raw_hex=self.raw(p,n),calls=[q[2]['id']for q in self.ctor_pending]))

def execute(data):
 t=owning.prepare(data,trace_type=ConstructorTrace);assert t.player_stages[-1]['completion']=='complete_original_return'
 t.ctor_enabled=True;assert owning.spawn_and_activate(t,activate=False)
 t.finish(STOP);assert not t.ctor_pending
 old=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/player-visibility/report.json').read_text())
 # Correlate literal original join/spawn/cluster stage records. Do not rerun
 # motion or promote this observer to complete title/device/world startup.
 positive=old['cases'][2]
 assert t.player_stages == positive['stages'][:4], 'Original owning stages changed'
 t.correlated_stages=4
 # Retain an exact live manager reset as an explicit root, after the measured
 # source join/spawn/cluster prefix. No manager bytes or bindings are supplied.
 unit=t.spawned_unit;obj=t.object(unit);at=int(obj['body'],16)+struct.unpack('<h',bytes.fromhex(obj['body_hex'])[0x12a:0x12c])[0]
 t.active='explicit_original_live_manager_reset'
 t.run(0x1ce9b0,regs=((UC_X86_REG_EAX,at),));t.finish(STOP);assert not t.ctor_pending
 binds=[c for c in t.ctor_calls if c['entry']=='001ceac0']
 spartan=binds[0]['metadata'];smg=binds[2]['metadata']
 graph=int(spartan['graph']['datum'],16);model=int(spartan['model']['datum'],16);weapon_graph=int(smg['graph']['datum'],16)
 # Declared caller arguments vary only graph/model/flag, using actual imported
 # tag identities and NONE. Storage remains the same source-owned manager.
 t.constructor_stops=[];t.caller_variations=[]
 for name,g,m,flag in (('missing_graph',0xffffffff,model,0),('real_node_mismatch',weapon_graph,model,1),('without_model',graph,0xffffffff,0),('without_graph_or_model',0xffffffff,0xffffffff,1)):
  t.active='explicit_original_bind_'+name
  t.caller_variations.append(dict(name=name,graph=f'{g:08x}',model=f'{m:08x}',flag=flag,source_owned_manager=f'{at:08x}'))
  t.new_invalid=None
  try:
   t.run(0x1ceac0,(flag,),((UC_X86_REG_ESI,at),(UC_X86_REG_EDI,g),(UC_X86_REG_EDX,m)));t.finish(STOP);assert not t.ctor_pending
  except unicorn.UcError as err:
   t.constructor_stops.append(dict(name=name,error=str(err),eip=f'{t.g.reg_read(UC_X86_REG_EIP):08x}',invalid=t.new_invalid,registers=t.registers(),pending_calls=[q[2]['id']for q in t.ctor_pending]))
   break
 return t

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 # The two owning pins are filled from the immutable files, never labels.
 for p,h in FROZEN.items():assert sha(ROOT/p)==h,(p,sha(ROOT/p),h)
 a.output.mkdir(parents=True,exist_ok=True)
 with(a.output/'host-provider.stderr.log').open('w')as err:
  proc=subprocess.Popen(['wine',str(owning.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owning.CONTEXT_OUT/'wine-prefix')))
  try:
   data=json.loads(proc.stdout.readline());t=execute(data)
   listings=[]
   for e,r in t.ctor_routines.items():
    instructions=[t.code.at(i)for i in sorted(r.code)]
    for i in instructions:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
    raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in instructions).encode();p=a.output/'instructions'/f'{e:08x}.asm';save(p,raw)
    listings.append(dict(entry=f'{e:08x}',file=str(p.relative_to(a.output)),sha256=sha(p),rows=len(instructions),executed_rows=sum(i.address in t.ctor_executed for i in instructions)))
   report=dict(schema='genuine-retained-unit-constructors-v1',tool_sha256=sha(__file__),frozen_pins=FROZEN,xbe_sha256=PIN,cache_sha256=CACHE_PIN,
    declared_caller_variations=t.caller_variations,exact_original_stops=t.constructor_stops,correlated_owning_stages=t.correlated_stages,calls=t.ctor_calls,stores=t.ctor_stores,reads=t.ctor_reads,listings=listings,one_bits=f'{t.u(0x465030):08x}',
    source_stages=t.player_stages,source_state=t.state(),services=t.events,instruction_patches=[],new_guest_inputs=[],new_supplied_allocation_graph_or_constructor_results=[],
    boundaries=['Unchanged source prepare, whole compatibility KPCR/host storage, bounded selected CRT/new-map roots, original game-options/property/resource delivery remain explicit inherited boundaries.',
     'Original actual player creation selects authored start37 and allocates biped/manager/components/SMG. No new unit/body zero fill, pool assignment, component offset, graph state or success return is supplied.',
     'Constructor/bind/default-selection and body-growth calls execute original bodies. Native differential leaf outputs are recorded services only, not runtime implementations of heap/default graph selection/variant/reset owners.',
     'Explicit bind caller variants use only actual imported Spartan/SMG graph/model identities or NONE and the source-owned post-equip manager; failed original paths are recorded, never admitted by a substitute selector.',
     'This extension captures constructor ownership only and stops before genuine PVS/ordinary motion. It is not local device readiness, positive pickup/switch, full title allocation order, Xbox arithmetic or Havok parity.'])
   save(a.output/'report.json',encoded(report));print(json.dumps(dict(report=str(a.output/'report.json'),sha256=sha(a.output/'report.json'),calls=len(t.ctor_calls),stores=len(t.ctor_stores))),flush=True)
   for p,h in FROZEN.items():assert sha(ROOT/p)==h,p
  finally:
   if proc.poll()is None:
    proc.stdin.write('x\n');proc.stdin.flush()
    try:proc.wait(timeout=15)
    except subprocess.TimeoutExpired:proc.kill();proc.wait(timeout=5)
   proc.stdin.close();proc.stdout.close()
if __name__=='__main__':main()
