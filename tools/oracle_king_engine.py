#!/usr/bin/env python3
"""Whole original King initialization, spline building, selection and occupancy.

Unchanged retail, TU1.1 and TU1.5 instructions execute every reached owner.
Literal scenario netgame rows, raw options/clock, isolated player/object pools,
unparented unit positions, RNG and prior state are declared ABI inputs. No game,
geometry, allocator or platform return is supplied. This is not a live match,
score owner, multiplayer transport, rendered hill or frontend admission proof.
"""
import argparse,csv,json,random,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDI
from oracle_game_variants import ROOT,Source
from oracle_game_over import SPECS as BASE
from oracle_mp_full_scoreboard_1_5 import _Code,literal,complete_tokens,references,sha
from inspect_multiplayer import Cache,MAPS

OUT=ROOT/'analysis/multiplayer/game-variants/king-engine-20261011'
OWNERS={
 'retail':dict(initialize=0x2bd140,start=0x2bd200,build=0x2bdc70,select=0x2bd0b0,contains=0x2be880,player=0x2bd2c0,
  scenario=0x4e0350,state=0x51ecc8,count=0x5092e8,groups=0x5092f0,rng=0x4e7408,pool=0x4e8c24,objects=0x4e0300,position=0xb9dd0),
 'tu11':dict(initialize=0x2c34d0,start=0x2c3590,build=0x2c3fb0,select=0x2c3440,contains=0x2c4bc0,player=0x2c3650,
  scenario=0x4e7920,state=0x526558,count=0x510ad8,groups=0x510ae0,rng=0x4ee9d8,pool=0x4f01f4,objects=0x4e78d0,position=0xbacc0),
 'u5':dict(initialize=0x2c6f50,start=0x2c7010,build=0x2c7a30,select=0x2c6ec0,contains=0x2c8640,player=0x2c70d0,
  scenario=0x4ecc20,state=0x52d8a0,count=0x515dc8,groups=0x515dd0,rng=0x4f3cd8,pool=0x4f54f4,objects=0x4ecbd0,position=0xbbe00)}
D,S,M,G,C,F,Q,P,R,O,H,U=[0x4000000+v for v in(0,0,0x1000,0x3000,0x5000,0x6000,0x16000,0x17000,0x18000,0x1c000,0x1d000,0x1e000)]
K=M+0xfc
def short(v):return struct.unpack('<h',struct.pack('<H',v&65535))[0]
class KingTrace(Source):
 def __init__(self,name):
  super().__init__(BASE[name]);self.name=name;self.a=OWNERS[name];self.g.mem_map(D,0x40000)
  self.code=_Code(self.image,'.text');self.starts={int(r['address'],16)for r in csv.DictReader((ROOT/self.spec['tsv']).open(),delimiter='\t')}
  self.call(self.spec['ctor']);v=self.u(self.u(self.spec['table']+16))
  for slot,key in ((1,'initialize'),(3,'start'),(15,'player')):assert self.u(v+slot*4)==self.a[key]
  for key,pointer in (('scenario',S),('state',K),('rng',Q),('pool',P),('objects',O)):self.w(self.a[key],pointer)
  for key,pointer in (('engine',M),('globals',G),('clock',C)):self.w(self.spec[key],pointer)
  self.w(P+0x24,0x21c);self.w(P+0x38,16);self.w(P+0x44,R);self.w(O+0x44,H)
  self.steps=0
 def w(self,a,v):self.g.mem_write(a,struct.pack('<I',v&0xffffffff))
 def h(self,a,v):self.g.mem_write(a,struct.pack('<H',v&65535))
 def raw(self):return bytes(self.g.mem_read(K,0x1dc))
 def groups(self):return bytes(self.g.mem_read(self.a['count'],2)+self.g.mem_read(self.a['groups'],16))
 def setup(self,raw):
  self.g.mem_write(G,bytes(0x1200));self.g.mem_write(M,b'\xa5'*0x1000)
  self.g.mem_write(F,raw);self.w(S+0x118,len(raw)//32);self.w(S+0x11c,F)
  self.h(C+2,30);self.h(self.a['count'],0xa5a5);self.g.mem_write(self.a['groups'],b'\xa5'*16)
 def run(self,key,args=(),regs=()):self.steps+=1;return self.call(self.a[key],args,regs)
def source_maps():
 maps=[]
 for name in MAPS:
  path=ROOT/'assets/disc/maps'/f'{name}.map';c=Cache(path)
  raw=b''.join(c.data[r:r+32]for r in c.block(c.scenario+0x118,32))
  maps.append(dict(name=name,cache_sha256=sha(path),rows_hex=raw.hex()))
 return maps
def synthetic_maps():
 result=[]
 for count in (0,1,3,4,5,6,7,8,10,12,14,16,17,20):
  rows=[]
  # Irregular controls exercise all segment distributions and signed indices.
  for i in range(count):
   rows.append(struct.pack('<4f3h10s',float((i*7)%19)-9,float((i*11)%23)-11,
       float(i%5)*0.125,0,11,short(i*7919),0,b'\x5a'*10))
  rows.extend(struct.pack('<4f3h10s',0,0,z,0,11,selector,channel,b'\xa5'*10)
       for z,selector,channel in ((-2.5,-1,1),(4.25,32767,1),(99,0,2)))
  # A partial later group must retain the preceding count and remaining indices.
  rows.extend(struct.pack('<4f3h10s',i+0.25,i-0.5,0,0,12,-1,0,bytes(10))for i in range(3))
  result.append(dict(name=f'synthetic-{count}',rows_hex=b''.join(rows).hex()))
 return result
def selections():
 rng=random.Random(0x2c6ec0);rows=[]
 for n in range(512):
  count=(-32768,-1,0,1,2,3,4,5,6,7,8)[n%11]
  rows.append(dict(count=count,indices=[rng.choice((-32768,-1,0,1,2,7,32767))for _ in range(8)],
   current=(-2147483648,-32768,-1,0,1,7,32767,65536,2147483647)[n%9],seed=rng.getrandbits(32)))
 return rows
def points(state):
 x,y,z=struct.unpack_from('<3f',state,0x164);radius,lo,hi=struct.unpack_from('<3f',state,0x170)
 rows=[]
 for a in (-1.01,-0.7,-0.1,0,0.25,0.7,1.01):
  for b in (-1.01,-0.7,-0.1,0,0.25,0.7,1.01):rows.append([x+radius*a,y+radius*b,z])
 for height in (lo-0.001,lo,lo+0.001,hi-0.001,hi,hi+0.001):rows.append([x,y,height])
 for i in range(32):
  px,py=struct.unpack_from('<2f',state,0x64+i*8)
  rows.extend(([px,py,z],[px+0.00001,py+0.00001,z],[px-0.00001,py-0.00001,z]))
 return [struct.pack('<3f',*row).hex()for row in rows]
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/king_engine_original.json');a=ap.parse_args()
 a.output.parent.mkdir(parents=True,exist_ok=True);a.fixture.parent.mkdir(parents=True,exist_ok=True)
 maps=source_maps()+synthetic_maps();blobs={}
 def blob(raw):
  digest=__import__('hashlib').sha256(raw).hexdigest();blobs[digest]=raw.hex();return digest
 versions={};sources={};selection_inputs=selections();occupancy_inputs=[]
 for name in BASE:
  t=KingTrace(name);sources[name]=t;geometry_rows=[];occupancy_rows=[];select_rows=[]
  for map_index,m in enumerate(maps):
   t.setup(bytes.fromhex(m['rows_hex']));records=[]
   def record(operation,**inputs):
    before=blob(t.raw());before_groups=t.groups().hex()
    if operation=='initialize':result=t.run('initialize')&255
    elif operation=='start':
     t.h(G+0x230,inputs['move_seconds']);t.h(C+2,inputs['rate']);result=t.run('start')&255
    else:result=t.run('build',(K,inputs['hill']));result=None
    records.append(dict(operation=operation,inputs=inputs,before=before,after=blob(t.raw()),
     before_groups_hex=before_groups,after_groups_hex=t.groups().hex(),returned_al=result))
   record('initialize');groups=list(struct.unpack('<8h',t.g.mem_read(t.a['groups'],16)))[:short(t.u(t.a['count'])&65535)]
   record('start',move_seconds=0,rate=30)
   for hill in groups:
    record('build',hill=hill);state=t.raw()
    if map_index<12:
     for point_index,point in enumerate(points(state)):
      occupancy_inputs.append(dict(map=map_index,state=blob(state),point_hex=point,
       slot=(0,3,15)[point_index%3],unit_present=True,mode=1,game_over=False,
       inside_ticks=(-32768,-1,0,1,29,30,32767)[point_index%7]))if name=='retail'else None
   record('build',hill=-1)
   for seconds,rate in ((-32768,32767),(-1,30),(1,-1),(30,60),(32767,-32768)):
    record('start',move_seconds=seconds,rate=rate)
   geometry_rows.append(records)
  if name=='retail':
   sample=occupancy_inputs[24]
   for mode in (0,1,2,4,255):
    for game_over in (False,True):
     for unit_present in (False,True):
      for inside_ticks in (-32768,-1,0,1,29,30,32767):
       occupancy_inputs.append(dict(**{**sample,'mode':mode,'game_over':game_over,
          'unit_present':unit_present,'inside_ticks':inside_ticks}))
  for index,row in enumerate(occupancy_inputs):
   slot=row['slot'];datum=0x9abc0000+slot;unit=0x8aaa0000+slot;at=R+slot*0x21c
   t.g.mem_write(K,bytes.fromhex(blobs[row['state']]))
   t.g.mem_write(R,b'\x5a'*(16*0x21c));t.w(at+0x2c,unit if row['unit_present']else -1);t.h(at+0x1b8,row['inside_ticks'])
   t.w(H+slot*12+8,U+slot*0x100);t.w(U+slot*0x100+0x14,-1)
   t.g.mem_write(U+slot*0x100+0x64,bytes.fromhex(row['point_hex']))
   t.g.mem_write(G+0xc,bytes([row['mode']]));t.g.mem_write(G+0x1128,bytes([int(row['game_over'])]))
   before=bytes(t.g.mem_read(R,16*0x21c));state_before=t.raw()
   contains=t.run('contains',regs=((UC_X86_REG_EAX,datum),(UC_X86_REG_EDI,K)))&255
   t.run('player',(datum,));after=bytearray(t.g.mem_read(R,16*0x21c));timer=struct.unpack_from('<h',after,slot*0x21c+0x1b8)[0]
   after[slot*0x21c+0x1b8:slot*0x21c+0x1ba]=before[slot*0x21c+0x1b8:slot*0x21c+0x1ba]
   assert bytes(after)==before and t.raw()==state_before,'King occupancy changed an unowned field'
   occupancy_rows.append(dict(contains=contains,inside_ticks=timer))
  for row in selection_inputs:
   t.h(t.a['count'],row['count']);t.g.mem_write(t.a['groups'],struct.pack('<8h',*row['indices']));t.w(Q,row['seed'])
   before=t.groups();result=t.run('select',(row['current'],));assert t.groups()==before
   select_rows.append(dict(hill=result,seed=t.u(Q)))
  versions[name]=dict(image_sha256=t.spec['pin'],owners=t.a,geometry=geometry_rows,
   occupancy=occupancy_rows,selection=select_rows,whole_owner_calls=t.steps,observed_instructions=len(t.executed))
 assert versions['retail']['geometry']==versions['tu11']['geometry']==versions['u5']['geometry']
 assert versions['retail']['occupancy']==versions['tu11']['occupancy']==versions['u5']['occupancy']
 assert versions['retail']['selection']==versions['tu11']['selection']==versions['u5']['selection']
 # Pin whole original function shapes, direct-call counterpart constraints and
 # relocated globals/constants, starting from the registered virtual callbacks.
 pairs=[];listings=[];directory=a.output.parent/'instructions';directory.mkdir(exist_ok=True)
 for earlier in ('retail','tu11'):
  old=sources[earlier];later=sources['u5'];todo=[(old.a[k],later.a[k])for k in ('initialize','start','build','select','contains','player')];seen=set()
  while todo:
   oe,ne=todo.pop()
   if(oe,ne)in seen:continue
   seen.add((oe,ne));_,oi=literal(old.code,oe,old.starts);_,ni=literal(later.code,ne,later.starts)
   assert complete_tokens(oi,old.image)==complete_tokens(ni,later.image),(earlier,hex(oe),hex(ne))
   pairs.append(dict(earlier=earlier,old_entry=f'{oe:08x}',u5_entry=f'{ne:08x}',instructions=len(oi),
     complete_shape_equal=True,relocated_operands=references(oi,ni,old.image,later.image)))
   for version,entry,ins in((earlier,oe,oi),('u5',ne,ni)):
    path=directory/f'{version}-{entry:08x}.asm';path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
    listings.append(dict(file=path.name,sha256=sha(path),instructions=len(ins)))
   oc=[i for i in oi if i.mnemonic=='call'and i.op_str.startswith('0x')]
   nc=[i for i in ni if i.mnemonic=='call'and i.op_str.startswith('0x')];assert len(oc)==len(nc)
   todo.extend((int(x.op_str,16),int(y.op_str,16))for x,y in zip(oc,nc))
 data=dict(schema='original-king-engine-v1',boundary=__doc__,maps=maps,raw_blobs=blobs,
  occupancy_inputs=occupancy_inputs,selection_inputs=selection_inputs,versions=versions,instruction_patches=[],supplied_returns=[])
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(fixture_sha256=sha(a.fixture),oracle_sha256=sha(__file__),complete_source_pairs=pairs,
  instruction_listings=listings,boundary=__doc__,instruction_patches=[],supplied_returns=[],
  summary=dict(disc_maps=12,disc_hills=sum(short(int.from_bytes(bytes.fromhex(v[-1]['after_groups_hex'])[:2],'little'))for v in versions['retail']['geometry'][:12]),
   synthetic_scenarios=len(maps)-12,occupancy_inputs=len(occupancy_inputs),selection_inputs=len(selection_inputs),whole_owner_calls=sum(v['whole_owner_calls']for v in versions.values())))
 a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report['summary'],indent=2))
if __name__=='__main__':main()
