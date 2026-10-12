#!/usr/bin/env python3
"""Execute whole original live entry-bank copy/application in three XBEs.

Actual outer owner vtables, definition slot23 and matching King bank owners run
unchanged. Literal retail20/TU24 rows, definitions with their genuine vtable,
private/live storage and bounded original services are declared ABI inputs.
This does not execute allocation/decode, upstream handle admission or packet
acceptance. Stale identities, ignored block metadata and overlap controls are
owner-contract inputs, not claims about accepted live packets.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP
from oracle_king_engine import KingTrace,D,M,G,C,K,P
from oracle_game_variants import ROOT
from oracle_mp_full_scoreboard_1_5 import literal,sha

OUT=ROOT/'analysis/multiplayer/game-variants/engine-entry-20261012/typed-layout'
CONTRACTS=ROOT/'analysis/multiplayer/game-variants/king-replication-orchestration-source-20261012/complete/report.json'
CONTRACT_PIN='9713a9e33cf9b140c00268245578c7aa1037d54f9d14ce8d2ec15d1be6f536ff'
BANK_CONTRACTS=ROOT/'analysis/multiplayer/game-variants/king-replication-source-20261012/complete/report.json'
BASE=ROOT/'tests/fixtures/engine_entity_original.json'
BASE_PIN='42e16ac18ead0e7c1360e1056cc59d7a014e76d7a422963175ea729de7d42a71'
THIS,BLOCK,ARENA,DEFINITIONS,BITMAP,OWNER=D+0x24000,D+0x24100,D+0x24200,D+0x24400,D+0x24500,D+0x2a000
def random_bytes(rng,n):return bytes(rng.getrandbits(8)for _ in range(n))
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/engine_entry_original.json');args=ap.parse_args()
 args.output.parent.mkdir(parents=True,exist_ok=True);args.fixture.parent.mkdir(parents=True,exist_ok=True)
 assert sha(CONTRACTS)==CONTRACT_PIN and sha(BASE)==BASE_PIN
 contracts=json.loads(CONTRACTS.read_text());bank_contracts=json.loads(BANK_CONTRACTS.read_text());base=json.loads(BASE.read_text())
 rng=random.Random(0x8ac40);blobs={}
 def blob(raw):
  digest=hashlib.sha256(raw).hexdigest();blobs[digest]=raw.hex();return digest
 inputs=[]
 for old_index,old in enumerate(base['inputs']):
  if old['operation']!='apply':continue
  n=len(inputs);handle=(0xffffffff,0x12340000,0x678903ff,0x90000207)[n%4]
  prefix=bytearray(random_bytes(rng,16));struct.pack_into('<Ih',prefix,0,handle if n%2 else (handle^0x40000000),3)
  arena=bytearray(random_bytes(rng,192));arena[96:136]=bytes.fromhex(base['raw_blobs'][old['bank']])
  inputs.append(dict(globals=blob(bytes.fromhex(base['raw_blobs'][old['globals']])),base_input=old_index,
    handle=handle,prefix_hex=prefix.hex(),secondary_pending=rng.getrandbits(32),state_size=40,
    creation_address=rng.getrandbits(32),creation_size=rng.getrandbits(32),
    source_offset=96,live_offset=0,arena_hex=arena.hex(),mask=old['mask'],
    block_size_bits=rng.getrandbits(16),block_type_bits=rng.getrandbits(16),
    game_over_hex=old['game_over_hex'],rate=old['rate']))
 # These literal owner controls exercise every MOVSD/MOVSB remainder and
 # forward overlap without claiming a source game produced such entries.
 for n,(size,delta)in enumerate([(size,None)for size in range(65)]+[(40,d)for d in(-7,-4,-3,-1,0,1,3,4,7)]):
  row=dict(inputs[n%len(inputs)]);raw=bytearray.fromhex(blobs[row['globals']]);struct.pack_into('<I',raw,0xc14,2)
  row.update(globals=blob(raw),state_size=size,source_offset=96 if delta is None else 64,
    live_offset=0 if delta is None else 64+delta,arena_hex=random_bytes(rng,192).hex(),owner_control=True)
  inputs.append(row)
 versions={};listings=[]
 for name in ('retail','tu11','u5'):
  t=KingTrace(name);t.setup(bytes.fromhex(base['map']['rows_hex']));t.run('initialize')
  owner_table=next(v for v in contracts['actual_virtual_tables']if v['image']==name and v['kind']=='entity_replication_owner')
  entry=t.u(owner_table['address']+7*4);assert entry==contracts['entries'][name]['entry_update_apply']
  definition=next(v for v in bank_contracts['entity_definition_tables']if v['image']==name)
  t.w(THIS,definition['vtable']);t.w(OWNER,owner_table['address']);t.w(OWNER+0x10,DEFINITIONS);t.w(DEFINITIONS+4+3*4,THIS)
  definition_identifier=t.call(t.u(definition['vtable']),registers=((UC_X86_REG_ECX,THIS),))
  identities=[]
  for selected in range(10):
   obj=t.u(t.spec['table']+selected*4)
   identifier=t.call(t.u(t.u(obj)+39*4),registers=((UC_X86_REG_ECX,obj),))if obj else -1
   identities.append(dict(present=bool(obj),identifier=identifier))
  definition_entry=t.u(definition['vtable']+23*4)
  bank_entry=bank_contracts['entries'][name]['king_apply']
  services={key:bank_contracts['entries'][name][owner]for key,owner in (
   ('refresh_player_properties','refresh_player_properties'),('refresh_recording','refresh_recording'),
   ('phase_changed','phase_change'),('begin_game_over','game_over_begin'))}
  first=literal(t.code,services['refresh_recording'],t.starts)[1][0];recorder=int(first.op_str.split('[0x')[1].split(']')[0],16)
  stride=32 if name=='retail'else 36;events=[];active=False;row_at=live=0
  def hook(g,address,size,_):
   if not active:return
   kind='definition_apply'if address==definition_entry else 'bank_apply'if address==bank_entry else 'build'if address==t.a['build']else next((k for k,v in services.items()if v==address),None)
   if kind:
    event=dict(kind=kind,globals=blob(bytes(g.mem_read(M,0xc18))),row_hex=bytes(g.mem_read(row_at,stride)).hex(),
      arena_hex=bytes(g.mem_read(ARENA,192)).hex(),game_over_hex=bytes(g.mem_read(G+0x1128,8)).hex())
    if kind=='definition_apply':
     sp=g.reg_read(UC_X86_REG_ESP);assert t.u(sp+4)==row_at and t.u(sp+16)==live
     event.update(mask=t.u(sp+8),state_size=t.u(sp+12))
    if kind=='bank_apply':
     sp=g.reg_read(UC_X86_REG_ESP);assert t.u(sp+12)==live
     event.update(mask=t.u(sp+4),opaque=t.u(sp+8))
    if kind=='phase_changed':event['phase']=g.reg_read(UC_X86_REG_EAX)&255
    events.append(event)
  t.g.hook_add(UC_HOOK_CODE,hook);results=[]
  for row in inputs:
   raw=bytes.fromhex(blobs[row['globals']]);t.g.mem_write(M,raw);t.g.mem_write(G,bytes(0x1200));t.g.mem_write(G+0xc,b'\x04')
   t.g.mem_write(G+0x1128,bytes.fromhex(row['game_over_hex']));t.h(C+2,row['rate']);t.w(P+0x48,BITMAP);t.w(BITMAP,0);t.g.mem_write(recorder,b'\0')
   live=ARENA+row['live_offset'];source=ARENA+row['source_offset'];row_at=OWNER+0x14+(row['handle']&1023)*stride
   row_raw=bytes.fromhex(row['prefix_hex'])+(struct.pack('<I',row['secondary_pending'])if name!='retail'else b'')
   row_raw+=struct.pack('<4I',row['creation_size'],row['creation_address'],row['state_size'],live)
   if row['handle']&1023:t.g.mem_write(row_at-8,b'\x79'*8)
   t.g.mem_write(row_at,row_raw+b'\x79'*8);t.g.mem_write(ARENA,bytes.fromhex(row['arena_hex']))
   block_raw=struct.pack('<HHI',row['block_size_bits'],row['block_type_bits'],source);t.g.mem_write(BLOCK,block_raw)
   events.clear();active=True
   try:inner_al=t.call(entry,(row['handle'],row['mask'],0x12345678,BLOCK),((UC_X86_REG_ECX,OWNER),))&255
   finally:active=False
   assert bytes(t.g.mem_read(row_at,stride))==row_raw and bytes(t.g.mem_read(BLOCK,8))==block_raw
   if row['handle']&1023:assert bytes(t.g.mem_read(row_at-8,8))==b'\x79'*8
   assert bytes(t.g.mem_read(row_at+stride,8))==b'\x79'*8
   results.append(dict(globals=blob(bytes(t.g.mem_read(M,0xc18))),row_hex=row_raw.hex(),
    arena_hex=bytes(t.g.mem_read(ARENA,192)).hex(),game_over_hex=bytes(t.g.mem_read(G+0x1128,8)).hex(),
    observed_inner_al=inner_al,events=list(events)))
  if name!='retail':
   def normalized(result):
    result=json.loads(json.dumps(result));result['row_hex']=result['row_hex'][:32]+result['row_hex'][40:]
    for event in result['events']:event['row_hex']=event['row_hex'][:32]+event['row_hex'][40:]
    return result
   assert [normalized(row)for row in results]==versions['retail']['results'],name
  digest=hashlib.sha256();path=args.output.parent/f'{name}-executed.asm'
  with path.open('w')as output:
   for address in sorted(t.executed):
    ins=t.code.at(address);assert bytes(t.g.mem_read(address,ins.size))==bytes(ins.bytes)
    digest.update(struct.pack('<I',address)+bytes(ins.bytes));output.write(f'{address:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n')
  listings.append(dict(image=name,file=path.name,instructions=len(t.executed),sha256=sha(path)))
  versions[name]=dict(image_sha256=t.spec['pin'],entry=entry,owner_vtable=owner_table['address'],row_stride=stride,
   results=results,identities=identities,definition_identifier=definition_identifier,
   game_over_seconds_hex=t.image.read(t.spec['constants'][0],4).hex(),executed_instruction_sha256=digest.hexdigest())
 fixture=dict(schema='original-engine-entry-v1',boundary=__doc__,map=base['map'],raw_blobs=blobs,inputs=inputs,
  versions=versions,arena_address=ARENA,instruction_patches=[],supplied_returns=[])
 args.fixture.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(schema='original-engine-entry-report-v1',boundary=__doc__,oracle_sha256=sha(__file__),
  contracts_sha256=sha(CONTRACTS),base_fixture_sha256=sha(BASE),fixture_sha256=sha(args.fixture),
  instruction_listings=listings,versions={name:{k:v for k,v in row.items()if k!='results'}for name,row in versions.items()},
  summary=dict(images=3,inputs=len(inputs),whole_entry_apply_calls=3*len(inputs),owner_controls=3*sum(r.get('owner_control',False)for r in inputs),
   real_bank_calls=sum(sum(e['kind']=='bank_apply'for e in r['events'])for v in versions.values()for r in v['results']),
   identity_rejections_after_copy=sum(r['observed_inner_al']==0 for v in versions.values()for r in v['results'])),
  instruction_patches=[],supplied_returns=[])
 args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report['summary']),flush=True)
if __name__=='__main__':main()
