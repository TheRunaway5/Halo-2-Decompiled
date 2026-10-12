#!/usr/bin/env python3
"""Unchanged whole original retained identity/PvP/event/machine remapping.

Full poisoned retained context, occupied live slots,12-byte player identities,
144-byte configurations,6-byte machine roster and high-water bound are declared
API inputs. Original stack probing and every reached game routine execute.
No instruction or game/platform return is replaced. This does not manufacture
native identities, execute participant capture/ranking, or render a PCR.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EDX
from oracle_result_statistics import StatisticsTrace,P,R
from oracle_mp_full_scoreboard_1_5 import ROOT,literal,complete_tokens,references,sha

OUT=ROOT/'analysis/multiplayer/game-variants/result-retention'
FIXTURE=ROOT/'tests/fixtures/result_retention_original.json'
SIZE=0xde78;PLAYERS=0x384;STATS=0xf44;EVENTS=0x4f84;MACHINES=0xdc24;H=0x1016000
OWNERS={
 'retail':dict(remap=0x196470,machines=0x199310,find=0x199250,add=0x1991f0,probe=0x320560,base=0x510cb0,prune=True),
 'tu11':dict(remap=0x197f70,machines=0x19ae10,find=0x19ad50,add=0x19acf0,probe=0x326af0,base=0x5184a0,prune=True),
 'u5':dict(remap=0x19ae70,machines=0x19dd10,find=0x19dc50,add=0x19dbf0,probe=0x32a660,base=0x51dfb0,prune=False)}
def digest(raw):return hashlib.sha256(raw).hexdigest()
def player_id(slot):return struct.pack('<III',0x12345600+slot,0x99887700+slot,0x43210000+slot)
def machine_id(slot):return struct.pack('<IH',0x87654300+slot,0xab00+slot)
def seed(number):return bytes((i*197+(i>>8)*31+number*17)&255 for i in range(SIZE))
def make_input(number):
 edits=[]
 def edit(at,raw):edits.append(dict(offset=at,hex=raw.hex()))
 edit(1,bytes([0 if number<256 else (0,1,255)[((number-256)//9)%3]]))
 for i in range(16):
  edit(PLAYERS+i*164,bytes([int((number%4!=1 or i%2==0) and (number%4!=2 or i in(0,3,15))),(-1,0,7,15,31,127,128)[(i+number)%7]&255])+player_id(i))
  present=0 if number%4==0 else int(number%4!=1 or i<8)*255
  mid=machine_id((i+number%16)%16) if number%4!=2 else machine_id(i+32)
  if number%4==3 and i==1:mid=machine_id(number%16)
  edit(MACHINES+i*11,mid+bytes([present]))
 if number&16:edit(PLAYERS+3*164+2,player_id(2))
 for i in range(1000):edit(EVENTS+i*36+1,bytes([(i+number)%256,(i*13+number)%256]))
 live=[]
 for i in range(16):
  old=(i+number%16)%16;identity=player_id(old if number%5 else old+64)
  if number&32 and i==1:identity=player_id(number%16 if number%5 else number%16+64)
  salt=(1,0x8000,0xffff)[(i+number)%3] if number%3!=1 or i%2==0 else 0
  live.append(dict(salt=salt,identity=identity.hex(),machine=(-1,i,(i+7)%16)[(i+number)%3],
   configuration=bytes((j*53+i*17+number)&255 for j in range(144)).hex()))
 return dict(seed=number,edits=edits,live=live,high_water=(0,1,4,8,15,16,32,-1)[(number//32)%8],
  roster=[machine_id(i).hex()for i in range(16)],recording=[(1,255)[number%2] if number<256 else (0,1,255)[(number-256)%3],(0,1,255)[(number//3)%3]])
def apply_context(raw,input):
 raw=bytearray(raw)
 for e in input['edits']:
  b=bytes.fromhex(e['hex']);raw[e['offset']:e['offset']+len(b)]=b
 return bytes(raw)
class RetentionTrace(StatisticsTrace):
 def __init__(self,name):
  super().__init__(name);self.o=OWNERS[name];self.g.mem_map(H,0x1000);self.w(self.s['pool']-4,H)
 def setup_input(self,input,initialize):
  if initialize:self.g.mem_write(self.o['base'],seed(input['seed']))
  for e in input['edits']:self.g.mem_write(self.o['base']+e['offset'],bytes.fromhex(e['hex']))
  self.g.mem_write(self.s['recording'],bytes(input['recording']));self.w(P+0x38,input['high_water'])
  self.g.mem_write(R,bytes(16*0x21c))
  for i,row in enumerate(input['live']):
   at=R+i*0x21c;self.g.mem_write(at,struct.pack('<H',row['salt']));self.g.mem_write(at+4,bytes.fromhex(row['identity']))
   self.g.mem_write(at+0x1a,struct.pack('<h',row['machine']));self.g.mem_write(at+0x44,bytes.fromhex(row['configuration']))
  self.g.mem_write(H+0x30,b''.join(bytes.fromhex(v)for v in input['roster']));self.calls=[]
 def execute(self,op):
  if op=='remap':self.call(self.o['remap'])
  else:self.call(self.o['machines'],(self.o['base'],))
  assert set(self.calls)<=set(self.o[k]for k in ('remap','machines','find','add','probe')),[hex(e)for e in self.calls]
  return bytes(self.g.mem_read(self.o['base'],SIZE))
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--fixture',type=Path,default=FIXTURE);a=ap.parse_args()
 a.output.parent.mkdir(parents=True,exist_ok=True);a.fixture.parent.mkdir(parents=True,exist_ok=True)
 inputs=[make_input(n)for n in range(288)];runs=[]
 for i in range(288):runs.append(dict(input=i,operation='remap',initialize=True))
 for i in range(96):runs.append(dict(input=i,operation='machines',initialize=True))
 for sequence in range(4):
  template=make_input(31+sequence*64);template['high_water']=16;template['recording']=[1,255]
  template['edits']=[e for e in template['edits'] if not PLAYERS<=e['offset']<PLAYERS+16*164]
  template['edits'].append(dict(offset=1,hex='00'))
  for i in range(16):template['edits'].append(dict(offset=PLAYERS+i*164,hex=(b'\x01\xff'+player_id(i)).hex()))
  for step in range(6):
   item=json.loads(json.dumps(template));item['edits']=template['edits'] if step==0 else []
   for i,row in enumerate(item['live']):
    row['salt']=1;row['identity']=player_id((i+step)%16).hex()
    if step==2 and i in(0,3,15):row['salt']=0
    if step==3 and i==3:row['identity']=player_id(100+sequence).hex()
    if step==4 and i==1:row['identity']=item['live'][0]['identity']
   if step==5:item['high_water']=0
   inputs.append(item);runs.append(dict(input=len(inputs)-1,operation='remap',initialize=step==0,sequence=sequence,step=step))
 versions={};sources={}
 for name in OWNERS:
  t=RetentionTrace(name);sources[name]=t;records=[]
  for run in runs:
   input=inputs[run['input']];t.setup_input(input,run['initialize']);before=bytes(t.g.mem_read(t.o['base'],SIZE))
   after=t.execute(run['operation'])
   # Retention only moves player stats, PvP, properties, event participant
   # bytes and machine records. Complete hashes retain all other raw bytes.
   for start,end in ((0,PLAYERS),(PLAYERS+16*164,STATS),(STATS+0x3aa0,EVENTS),(MACHINES+16*11,SIZE)):
    assert before[start:end]==after[start:end],(name,hex(start))
   assert all(before[EVENTS+i*36:EVENTS+i*36+1]==after[EVENTS+i*36:EVENTS+i*36+1] and
    before[EVENTS+i*36+3:EVENTS+(i+1)*36]==after[EVENTS+i*36+3:EVENTS+(i+1)*36]for i in range(1000))
   records.append(dict(before_sha256=digest(before),after_sha256=digest(after),changed_bytes=sum(x!=y for x,y in zip(before,after)),calls=[f'{e:08x}'for e in t.calls]))
  versions[name]=dict(image_sha256=t.spec['pin'],prune=t.o['prune'],owners={k:f'{t.o[k]:08x}'for k in ('remap','machines','find','add','probe','base')},records=records)
 listings=[];pairs=[];directory=a.output.parent/'instructions';directory.mkdir(exist_ok=True)
 for name,t in sources.items():
  for key in ('remap','machines','find','add','probe'):
   entry=t.o[key];_,ins=literal(t.code,entry,t.starts);p=directory/f'{name}-{entry:08x}.asm'
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(file=p.name,instructions=len(ins),sha256=sha(p)))
   if name=='u5':continue
   later=sources['tu11'] if key=='machines' else sources['u5']
   if name=='tu11' and key=='machines':continue
   _,ni=literal(later.code,later.o[key],later.starts);x,y=complete_tokens(ins,t.image),complete_tokens(ni,later.image)
   assert len(x)==len(y),(name,key)
   differences=[i for i in range(len(x))if x[i]!=y[i]];relocations=[]
   assert differences==([79,99,137,162,190]if key=='remap'else []),(name,key,differences)
   for index in differences:
    old,new=ins[index],ni[index];assert old.mnemonic==new.mnemonic=='cmp' and old.operands[0].reg==new.operands[0].reg
    oa,na=old.operands[1].imm,new.operands[1].imm;assert oa-t.o['base']==na-later.o['base']
    relocations.append(dict(index=index,old_address=f'{oa:08x}',later_address=f'{na:08x}',context_offset=f'{oa-t.o["base"]:x}'))
   known={t.o[k]:later.o[k]for k in ('remap','machines','find','add','probe')}
   for oi,li in zip(ins,ni):
    if oi.mnemonic=='call' and oi.op_str.startswith('0x') and int(oi.op_str,16)in known:assert int(li.op_str,16)==known[int(oi.op_str,16)]
   pairs.append(dict(earlier=name,later=later.name,owner=key,instructions=len(ins),explicit_context_relocations=relocations,complete_shape_equal_except_declared_relocations=True,relocated_operands=references(ins,ni,t.image,later.image)))
 t=sources['u5']
 for entry in (0x19a430,0x19a830,0x15a860,0x15a950,0x15aa80):
  _,ins=literal(t.code,entry,t.starts);p=directory/f'u5-caller-{entry:08x}.asm'
  p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(file=p.name,instructions=len(ins),sha256=sha(p),kind='static_caller'))
 data=dict(schema='original-result-retention-v1',boundary=__doc__,instruction_patches=[],supplied_returns=[],inputs=inputs,runs=runs,versions=versions)
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(**data,fixture_sha256=sha(a.fixture),oracle_sha256=sha(__file__),instruction_listings=listings,complete_source_pairs=pairs,
  dependencies={p:sha(ROOT/p)for p in ('tools/oracle_result_statistics.py','tools/oracle_game_over.py','tools/oracle_game_variants.py','tools/oracle_mp_full_scoreboard_1_5.py')})
 a.output.write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
 print(json.dumps(dict(cases=len(runs)*3,inputs=len(inputs),fixture_sha256=sha(a.fixture),pairs=len(pairs),listings=len(listings)),indent=2))
if __name__=='__main__':main()
