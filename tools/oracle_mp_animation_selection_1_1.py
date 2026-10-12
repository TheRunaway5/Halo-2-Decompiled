#!/usr/bin/env python3
"""Independent observer of original retained animation selection/tree owners.

The genuine allocated/equipped constructor prefix is read-only. No new guest
data, successful services, graph choice, variant, pose or readiness is supplied.
"""
import argparse, hashlib, json, os, struct, subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EDX
import oracle_mp_unit_constructors_1_1 as ctor
from functions import _trace

ROOT=ctor.ROOT
TOP=0x1d0a60
ENTRIES=(TOP,0x1d0590,0x1cfde0,0x1d0790,0xe7e30,0x1c9e30,0x1d06c0,0x1cee00,0x1cacf0,0x1ceda0,0x1c9e60,0x1c9f50,0x1d1a00,0xb7650,0x1d1b70,0x1de890,0x1cfce0,0x1deb10,0x29c6a0,0x1e0f50,0x1deb60)
FROZEN={**ctor.FROZEN,
 'tools/oracle_mp_unit_constructors_1_1.py':'14cff57ecf1d23160e1222916e85a6b1f553900f29ce492f0e552e6ec2500487',
 'analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/report.json':'8e03219bae06eaf2968d2dfb24142a9446cee1d9ff36da9b246b229c821d3c0e'}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,('Immutable output differs',str(p))
 else:p.write_bytes(b)
def encoded(x):return(json.dumps(x,sort_keys=True,indent=2)+'\n').encode()

class SelectionTrace(ctor.ConstructorTrace):
 def __init__(self,data):
  self.selection_enabled=False;self.selection_calls=[];self.selection_pending=[];self.selection_stores=[];self.selection_order=0;self.selection_executed=set();self.banks={};self.pure_root_manager=None;self.pure_roots=[]
  super().__init__(data)
  self.selection_routines={e:_trace(self.code,e,self.starts|set(ENTRIES))for e in ENTRIES}
  self.selection_addresses={a:e for e,r in self.selection_routines.items()for a in r.code}
 def outputs(self,row):return{k:self.raw(p,n)for k,(p,n)in row['output_locations'].items()}
 def selection_finish(self,a):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  for q in reversed(self.selection_pending[:]):
   ret,entry_sp,row=q
   if ret!=a or sp<=entry_sp:continue
   row.update(after_manager_hex=self.raw(row['manager'],0x90),outputs_after=self.outputs(row),return_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',return_order=self.selection_order)
   self.selection_order+=1
   if int(row['entry'],16)==0x1de890:
    p=self.g.reg_read(UC_X86_REG_EAX);row['definition_return']=dict(pointer=f'{p:08x}',raw1c_hex=self.raw(p,0x1c),frame_count=struct.unpack('<h',bytes(self.g.mem_read(p+0x14,2)))[0],authored_flags=self.u(p+0x18,1))
   self.selection_pending.remove(q)
 def hook(self,g,a,size,user):
  if self.selection_enabled:
   self.selection_finish(a)
   if a in self.selection_addresses:self.selection_executed.add(a)
   if a in ENTRIES and(a==TOP or self.selection_pending or(a==0x1deb10 and self.pure_root_manager is not None)):
    sp=g.reg_read(UC_X86_REG_ESP);regs={k:int(v,16)for k,v in self.registers().items()};s=[self.u(sp+4+i*4)for i in range(10)]
    manager=regs['ecx']if a==TOP else self.selection_pending[-1][2]['manager']if self.selection_pending else self.pure_root_manager
    out={}
    if a==0x1cfde0:out=dict(resolved=(regs['ecx'],16),requested=(regs['edx'],16),packed=(s[5],4))
    elif a==0x1d0790:out=dict(packed=(s[0],4),eligible=(s[3],1))
    elif a==0x1d06c0:out=dict(packed=(regs['ebx'],4),activity=(s[4],4))
    elif a==0x1ceda0:out=dict(packed=(regs['esi'],4))
    elif a==0x1cfce0:out=dict(tuple=(regs['esi'],16))
    elif a==0x29c6a0:out=dict(state=(regs['esi'],28),mode=(s[0],4),weapon_class=(s[1],4),weapon=(regs['ebx'],4))
    row=dict(id=len(self.selection_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.selection_order,registers=regs,stack=[f'{v:08x}'for v in s],ancestors=[q[2]['id']for q in self.selection_pending],manager=manager,before_manager_hex=self.raw(manager,0x90),output_locations=out)
    row['outputs_before']=self.outputs(row)
    if a==0x1e0f50:
     p,n,stride=s[:3];count=struct.unpack('<i',bytes(g.mem_read(p,4)))[0];assert 0<=count<=100000 and stride in(8,0x14,0x34)
     key=f'{p:08x}/{stride:x}';bank=dict(pointer=f'{p:08x}',header8_hex=self.raw(p,8),count=count,stride=stride,records_hex=self.raw(self.u(p+4),count*stride)if count else '')
     if key in self.banks:assert self.banks[key]==bank
     else:self.banks[key]=bank
     row['bank']=key
    self.selection_calls.append(row);self.selection_pending.append((self.u(sp),sp,row));self.selection_order+=1
  super().hook(g,a,size,user)
 def mem_write(self,g,access,p,n,value,user):
  if self.selection_enabled and self.selection_pending:
   ip=g.reg_read(UC_X86_REG_EIP)
   if ip in self.selection_addresses:
    labels={}
    for _,_,row in self.selection_pending:
     m=row['manager']
     if m<=p and p+n<=m+0x90:labels[str(row['id'])]=p-m
    self.selection_stores.append(dict(order=self.selection_order,instruction=f'{ip:08x}',address=f'{p:08x}',size=n,before=self.raw(p,n),written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex(),calls=[q[2]['id']for q in self.selection_pending],manager_offsets=labels));self.selection_order+=1
  super().mem_write(g,access,p,n,value,user)

def execute(data):
 t=ctor.owning.prepare(data,trace_type=SelectionTrace);assert t.player_stages[-1]['completion']=='complete_original_return'
 t.ctor_enabled=t.selection_enabled=True;assert ctor.owning.spawn_and_activate(t,activate=False)
 t.selection_finish(ctor.STOP);t.finish(ctor.STOP);assert not t.selection_pending and not t.ctor_pending
 old=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/player-visibility/report.json').read_text());assert t.player_stages==old['cases'][2]['stages'][:4]
 obj=t.object(t.spawned_unit);at=int(obj['body'],16)+struct.unpack('<h',bytes.fromhex(obj['body_hex'])[0x12a:0x12c])[0]
 t.active='explicit_original_live_manager_reset';t.run(0x1ce9b0,regs=((UC_X86_REG_EAX,at),));t.selection_finish(ctor.STOP);t.finish(ctor.STOP)
 binds=[c for c in t.ctor_calls if c['entry']=='001ceac0'];spartan=binds[0]['metadata'];smg=binds[2]['metadata']
 graph=int(spartan['graph']['datum'],16);model=int(spartan['model']['datum'],16);weapon_graph=int(smg['graph']['datum'],16)
 for name,g,m,flag in(('missing_graph',0xffffffff,model,0),('real_node_mismatch',weapon_graph,model,1),('without_model',graph,0xffffffff,0)):
  t.active='explicit_original_bind_'+name;t.run(0x1ceac0,(flag,),((UC_X86_REG_ESI,at),(UC_X86_REG_EDI,g),(UC_X86_REG_EDX,m)));t.selection_finish(ctor.STOP);t.finish(ctor.STOP);assert not t.selection_pending and not t.ctor_pending
 previous=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/report.json').read_text())
 a=[(c['before']['M'],c['after']['M'],c['return_eax'])for c in previous['calls']if c['entry']=='001d0a60'and'after'in c]
 b=[(c['before_manager_hex'],c['after_manager_hex'],c['return_eax'])for c in t.selection_calls if c['entry']=='001d0a60'];assert a==b and len(a)==9
 # Independent pure tree roots use only both identifier packets actually
 # produced by each owning1cfde0 call, and its actual source master graph.
 # No input manager/tree bytes are assigned, and the tree owner has no stores.
 t.pure_root_manager=at
 for c in [x for x in t.selection_calls if x['entry']=='001cfde0']:
  graph=struct.unpack_from('<I',bytes.fromhex(c['before_manager_hex']),0x68)[0];p=t.u(t.u(0x4eb114)+(graph&65535)*16+8)
  for packet in('requested','resolved'):
   ids=struct.unpack('<4I',bytes.fromhex(c['outputs_after'][packet]));t.active=f'explicit_original_tree_from_{c["id"]}_{packet}'
   t.pure_roots.append(dict(source_call=c['id'],source_output_packet=packet,graph=f'{graph:08x}',graph_address=f'{p:08x}',identifiers=[f'{v:08x}'for v in ids],new_call=len(t.selection_calls)))
   t.run(0x1deb10,(ids[1],ids[2]),((UC_X86_REG_EAX,ids[0]),(UC_X86_REG_ECX,p)));t.selection_finish(ctor.STOP);t.finish(ctor.STOP);assert not t.selection_pending
 t.pure_root_manager=None
 return t

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 for f,h in FROZEN.items():assert sha(ROOT/f)==h,f
 a.output.mkdir(parents=True,exist_ok=True)
 with(a.output/'host-provider.stderr.log').open('w')as err:
  proc=subprocess.Popen(['wine',str(ctor.owning.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(ctor.owning.CONTEXT_OUT/'wine-prefix')))
  try:
   data=json.loads(proc.stdout.readline());t=execute(data);listings=[]
   for e,r in t.selection_routines.items():
    ins=[t.code.at(i)for i in sorted(r.code)]
    for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
    raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode();f=a.output/'instructions'/f'{e:08x}.asm';save(f,raw)
    listings.append(dict(entry=f'{e:08x}',file=str(f.relative_to(a.output)),sha256=sha(f),rows=len(ins),executed_rows=sum(i.address in t.selection_executed for i in ins)))
   # Addresses are retained as observed identities; data-only packets contain
   # complete original records with original counts/strides/lookup order.
   graphs=[]
   for tag in t.cache.tags.values():
    if tag['group']=='jmad':
     datum=tag['datum']if'datum'in tag else next(k for k,v in t.cache.tags.items()if v is tag)
     datum=int(datum,16)if isinstance(datum,str)else int(datum);ptr=t.u(t.u(0x4eb114)+(datum&65535)*16+8)
     assert ptr==tag['offset']+t.cache.magic
     graphs.append(dict(datum=f'{datum:08x}',name=tag['name'],pointer=f'{ptr:08x}',bytes=tag['size'],headerc4_hex=t.raw(ptr,0xc4)))
   extents=[]
   for key,bank in t.banks.items():
    for label,address,raw in(('header',int(bank['pointer'],16),bank['header8_hex']),('records',int.from_bytes(bytes.fromhex(bank['header8_hex'])[4:8],'little'),bank['records_hex'])):
     n=len(raw)//2
     if not n:continue
     owned=[x for x in graphs if int(x['pointer'],16)<=address and address+n<=int(x['pointer'],16)+x['bytes']];assert len(owned)==1,(key,label,hex(address),n)
     authored=bytes(t.cache.data[address-t.cache.magic:address-t.cache.magic+n]).hex();assert authored==raw,('Pure graph packet differs from authored cache',key,label)
     extents.append(dict(bank=key,part=label,graph=owned[0]['datum'],address=f'{address:08x}',bytes=n,authored_cache_equal=True))
   report=dict(schema='source-animation-selection-v1',tool_sha256=sha(__file__),frozen_pins=FROZEN,xbe_sha256=ctor.PIN,cache_sha256=ctor.CACHE_PIN,correlated_complete_manager_calls=9,explicit_pure_roots=t.pure_roots,calls=t.selection_calls,stores=t.selection_stores,banks=t.banks,graphs=graphs,bank_extent_audit=extents,listings=listings,constant_words={f'{p:08x}':f'{t.u(p):08x}'for p in(0x465030,0x465048,0x46504c)},source_stages=t.player_stages,services=t.events,new_guest_inputs=[],instruction_patches=[],new_supplied_results=[],boundaries=['Same unchanged selected owning preparation and explicit bind caller variants as constructor freeze; complete nonlocal join/loadout and authored start37. Not complete title/device/session startup.','All selection/graph/variant/channel/resource bodies execute unchanged; recorded lower results will be explicit native differential services, not live successful defaults.','No source pool/body/manager fields, graph/activity/clip, RNG/variant result, transform, resource readiness or allocation success supplied by this extension.','18 added pure1deb10 roots take both actual produced packets from the nine1cfde0 calls and their original master graph. They assign only declared caller registers/arguments, no manager or graph fields.'])
   save(a.output/'report.json',encoded(report));print(json.dumps(dict(report=str(a.output/'report.json'),sha256=sha(a.output/'report.json'),calls=len(t.selection_calls),stores=len(t.selection_stores),banks=len(t.banks))))
   for f,h in FROZEN.items():assert sha(ROOT/f)==h,f
  finally:
   if proc.poll()is None:
    proc.stdin.write('x\n');proc.stdin.flush()
    try:proc.wait(timeout=15)
    except subprocess.TimeoutExpired:proc.kill();proc.wait(timeout=5)
   proc.stdin.close();proc.stdout.close()
if __name__=='__main__':main()
