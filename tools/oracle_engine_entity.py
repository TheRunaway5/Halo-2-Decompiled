#!/usr/bin/env python3
"""Execute original registered engine-entity identity/creation dispatch in three XBEs.

Actual slot39/slot0 getters and slots19/20/22/23 execute unchanged. Matching
King calls reach real bank owners, authored hill rebuilding and bounded real
services. Other registered engines exercise identity rejection or service-free
creation. Empty player bitmap, disabled recording and mode4 are declared inputs,
not supplied service results. Absent-engine application faults are recorded as
faults after adoption. No packet acceptance, delivery or complete match claim.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from unicorn import UcError,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_king_engine import KingTrace,source_maps,D,M,G,C,K,P
from oracle_game_variants import ROOT
from oracle_mp_full_scoreboard_1_5 import literal,sha

OUT=ROOT/'analysis/multiplayer/game-variants/engine-entity-20261012/complete'
CONTRACTS=ROOT/'analysis/multiplayer/game-variants/king-replication-source-20261012/complete/report.json'
BANK,FLAGS,THIS,CREATION,BITMAP=D+0x28000,D+0x28100,D+0x28200,D+0x28300,D+0x28400
def random_bytes(rng,n):return bytes(rng.getrandbits(8)for _ in range(n))
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/engine_entity_original.json');args=ap.parse_args()
 args.output.parent.mkdir(parents=True,exist_ok=True);args.fixture.parent.mkdir(parents=True,exist_ok=True)
 contracts=json.loads(CONTRACTS.read_text());rng=random.Random(0xa45e0);blobs={}
 def blob(raw):
  digest=hashlib.sha256(raw).hexdigest();blobs[digest]=raw.hex();return digest
 inputs=[]
 def add(operation,selected,n,**extra):
  raw=bytearray(random_bytes(rng,0xc18));bank=bytearray(random_bytes(rng,40))
  struct.pack_into('<I',raw,0x24,(0xffffffff,0,0x8abc0003)[n%3]);struct.pack_into('<I',raw,0xc14,selected)
  phase=(0,2,3,4,255)[n%5];struct.pack_into('<h',raw,0x6c,(-1,phase)[n%2]);bank[28]=phase
  struct.pack_into('<h',bank,36,(-1,0,-32768)[n%3])
  inputs.append(dict(operation=operation,selected=selected,globals=blob(raw),bank=blob(bank),
    mask=(n&127)|((0,0x80000000,0xffffff80)[n%3]),opaque=(0,0xffffffff,0x12345678)[n%3],
    game_over_hex=struct.pack('<BBHi',n%2,(n//2)%2,0,-27).hex(),rate=(-32768,-1,0,30,32767)[n%5],**extra))
 for selected in range(10):
  for n in range(3):add('initialize',selected,n)
  for n in range(128):add('difference',selected,n);add('apply',selected,n)
  for n,kind in enumerate((-32768,-1,0,1,2,3,4,5,6,7,32767)):
   for prior in (0xffffffff,0x12340007):
    add('create',selected,n,creation_hex=struct.pack('<IhH',rng.getrandbits(32),kind,0xa55a).hex(),prior_entity=prior)
    inputs[-1]['mask']=0
 for n in range(128):
  add('create',4,n,creation_hex=struct.pack('<IhH',rng.getrandbits(32),3,0x5aa5).hex(),prior_entity=(0xffffffff,0x12340007)[n%2])
 for selected in (0,5,6):
  add('create_fault',selected,96,creation_hex=struct.pack('<IhH',0xabcdef09,-1,0xa55a).hex(),prior_entity=0x12340007)
 m=next(row for row in source_maps()if row['name']=='lockout');versions={};listings=[]
 for name in ('retail','tu11','u5'):
  t=KingTrace(name);t.setup(bytes.fromhex(m['rows_hex']));t.run('initialize');t.run('build',(K,0));initial_king=t.raw()
  definition=next(row for row in contracts['entity_definition_tables']if row['image']==name)
  t.w(THIS,definition['vtable']);definition_id=t.call(t.u(definition['vtable']),registers=((UC_X86_REG_ECX,THIS),))
  assert definition_id==3
  entries={key:t.u(definition['vtable']+slot*4)for key,slot in (('initialize',19),('difference',20),('apply',23),('create',22))}
  for key,owner in (('initialize','initialize_dispatch'),('difference','difference_dispatch'),('apply','apply_dispatch'),('create','creation_apply')):
   assert entries[key]==contracts['entries'][name][owner]
  identities=[]
  for selected in range(10):
   obj=t.u(t.spec['table']+selected*4)
   entry=t.u(t.u(obj)+39*4)if obj else None
   value=t.call(entry,registers=((UC_X86_REG_ECX,obj),))if obj else -1
   identities.append(dict(selected=selected,present=bool(obj),identifier=value,object=obj,getter=entry))
  assert [r['identifier']for r in identities]==[-1,1,0,2,3,-1,-1,5,4,1]
  king_object=t.u(t.spec['table']+16);king_vtable=t.u(king_object)
  callbacks={key:t.u(king_vtable+slot*4)for key,slot in (('initialize',44),('difference',45),('apply',46))}
  services={key:contracts['entries'][name][owner]for key,owner in (
   ('refresh_player_properties','refresh_player_properties'),('refresh_recording','refresh_recording'),
   ('phase_changed','phase_change'),('begin_game_over','game_over_begin'))}
  first=literal(t.code,services['refresh_recording'],t.starts)[1][0]
  recorder=int(first.op_str.split('[0x')[1].split(']')[0],16)
  events=[];stores=[];active=False
  def hook(g,address,size,_):
   if not active:return
   kind=next((f'bank_{key}'for key,value in callbacks.items()if value==address),None)
   if not kind:kind=next((key for key,value in services.items()if value==address),None)
   if address==t.a['build']:kind='build'
   if kind:
    event=dict(kind=kind,globals=blob(bytes(g.mem_read(M,0xc18))),bank=blob(bytes(g.mem_read(BANK,40))),
      game_over_hex=bytes(g.mem_read(G+0x1128,8)).hex())
    if kind.startswith('bank_'):
     sp=g.reg_read(UC_X86_REG_ESP);event['opaque']=t.u(sp+(4 if kind=='bank_initialize'else 8))
     if kind!='bank_initialize':event['mask']=t.u(t.u(sp+4))if kind=='bank_difference'else t.u(sp+4)
    if kind=='phase_changed':event['phase']=g.reg_read(UC_X86_REG_EAX)&255
    events.append(event)
  def write(g,access,address,size,value,_):
   if active and address==M+0x24:stores.append(dict(size=size,value=value&0xffffffff))
  t.g.hook_add(UC_HOOK_CODE,hook);t.g.hook_add(UC_HOOK_MEM_WRITE,write)
  results=[];faults=0
  for n,row in enumerate(inputs):
   raw=bytearray.fromhex(blobs[row['globals']]);raw[0xfc:0xfc+0x1dc]=initial_king
   struct.pack_into('<i',raw,0xfc+0x1a0,(-1,0,65535)[n%3])
   if 'prior_entity'in row:struct.pack_into('<I',raw,0x24,row['prior_entity'])
   # This completed record is the actual ABI input, shared by native checking.
   if name=='retail':row['globals']=blob(raw)
   else:assert blob(raw)==row['globals']
   t.g.mem_write(M,bytes(raw));t.g.mem_write(G,bytes(0x1200));t.g.mem_write(G+0xc,b'\x04')
   t.g.mem_write(G+0x1128,bytes.fromhex(row['game_over_hex']));t.h(C+2,row['rate'])
   t.w(P+0x48,BITMAP);t.w(BITMAP,0);t.g.mem_write(recorder,b'\0')
   t.g.mem_write(BANK-8,b'\x73'*8+bytes.fromhex(blobs[row['bank']])+b'\x73'*8);t.w(FLAGS,row['mask'])
   operation=row['operation'];fault=None
   if operation.startswith('create'):
    t.g.mem_write(CREATION,bytes.fromhex(row['creation_hex']));argv=(CREATION,0x12345678,0xffffffff,row['mask'],row['opaque'],BANK);entry=entries['create']
   else:
    argv=(0xabcdef09,FLAGS if operation=='difference'else row['mask'],row['opaque'],BANK);entry=entries[operation]
   events.clear();stores.clear();active=True
   try:returned=t.call(entry,argv,((UC_X86_REG_ECX,THIS),))&255
   except UcError as error:
    assert operation=='create_fault'and error.errno==6,(name,n,str(error))
    returned=None;fault=dict(errno=error.errno,site=t.g.reg_read(UC_X86_REG_EIP));faults+=1
   finally:active=False
   if operation!='create_fault':assert fault is None
   result=dict(globals=blob(bytes(t.g.mem_read(M,0xc18))),bank=blob(bytes(t.g.mem_read(BANK,40))),
    game_over_hex=bytes(t.g.mem_read(G+0x1128,8)).hex(),flags=t.u(FLAGS),returned_al=returned,
    events=list(events),entity_stores=list(stores),fault=fault)
   assert bytes(t.g.mem_read(BANK-8,8))==bytes(t.g.mem_read(BANK+40,8))==b'\x73'*8
   if operation in ('apply','create','create_fault'):assert result['bank']==row['bank']
   if operation.startswith('create'):assert bytes(t.g.mem_read(CREATION,8)).hex()==row['creation_hex']
   results.append(result)
  if name!='retail':
   for old,new in zip(versions['retail']['results'],results):
    assert {k:v for k,v in old.items()if k!='fault'}=={k:v for k,v in new.items()if k!='fault'},name
    assert bool(old['fault'])==bool(new['fault'])
  digest=hashlib.sha256();path=args.output.parent/f'{name}-executed.asm'
  with path.open('w')as output:
   for address in sorted(t.executed):
    ins=t.code.at(address);assert bytes(t.g.mem_read(address,ins.size))==bytes(ins.bytes)
    digest.update(struct.pack('<I',address)+bytes(ins.bytes));output.write(f'{address:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n')
  listings.append(dict(image=name,file=path.name,sha256=sha(path),instructions=len(t.executed)))
  versions[name]=dict(image_sha256=t.spec['pin'],identities=identities,definition_identifier=definition_id,entries=entries,
   results=results,game_over_seconds_hex=t.image.read(t.spec['constants'][0],4).hex(),
   completed_calls=len(results)-faults,expected_faults=faults,executed_instruction_sha256=digest.hexdigest())
 fixture=dict(schema='original-engine-entity-v1',boundary=__doc__,map=m,raw_blobs=blobs,inputs=inputs,
  versions=versions,instruction_patches=[],supplied_returns=[])
 args.fixture.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(schema='original-engine-entity-report-v1',boundary=__doc__,oracle_sha256=sha(__file__),
  contracts_sha256=sha(CONTRACTS),fixture_sha256=sha(args.fixture),instruction_listings=listings,
  summary=dict(images=3,inputs=len(inputs),completed_calls=sum(v['completed_calls']for v in versions.values()),
   expected_absent_engine_faults=sum(v['expected_faults']for v in versions.values()),
   reached_bank_calls=sum(sum(e['kind'].startswith('bank_')for e in r['events'])for v in versions.values()for r in v['results']),
   reached_shared_services=sum(sum(e['kind']in services for e in r['events'])for v in versions.values()for r in v['results'])),
  versions={name:{k:v for k,v in row.items()if k!='results'}for name,row in versions.items()},instruction_patches=[],supplied_returns=[])
 args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report['summary']),flush=True)
if __name__=='__main__':main()
