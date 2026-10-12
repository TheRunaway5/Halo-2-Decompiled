#!/usr/bin/env python3
"""Execute whole original entry update decoding with its real allocator.

The literal owner/definition vtables, concrete allocator and King codec execute
unchanged in three XBEs. A bounded original boot suffix constructs/publishes
the allocator over declared backing. Rows, streams, prior live bank and output
storage are declared ABI inputs. No allocation pointer or service return is
supplied. Successful decoder status alone is not outer packet acceptance.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP
from oracle_king_engine import KingTrace,D,M
from oracle_game_variants import ROOT
from oracle_mp_full_scoreboard_1_5 import sha
from research_replication_allocator import setup_allocator,BOOT
from oracle_engine_entry import CONTRACTS,CONTRACT_PIN,BANK_CONTRACTS

OUT=ROOT/'analysis/multiplayer/game-variants/engine-entry-decode-20261012/complete'
BASE=ROOT/'tests/fixtures/king_wire_original.json'
BASE_PIN='6f2520b3864189d06cb3b4f80f92431ad406eaa974aab425b24911301936e891'
ALLOCATOR_TOOL=ROOT/'tools/research_replication_allocator.py'
ALLOCATOR_PIN='2fe43232461eb8df7abcb6d2cb88aa1cf31ebe5ad48fa2f830d27a265b75302a'
BANK_PIN='44c4599ff00ed27dc3124555d98ca3e6c9f4a10565c9dfaf03fa9eb2fa1d6e2d'
THIS,DEFS,LIVE,DATA,STREAM,MASK,COUNT,BLOCKS,OWNER=(D+x for x in(0x24000,0x24100,0x24200,0x24300,0x24400,0x24500,0x24504,0x24600,0x2a000))
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/engine_entry_decode_original.json');args=ap.parse_args()
 args.output.parent.mkdir(parents=True,exist_ok=True);args.fixture.parent.mkdir(parents=True,exist_ok=True)
 assert sha(CONTRACTS)==CONTRACT_PIN and sha(BASE)==BASE_PIN and sha(ALLOCATOR_TOOL)==ALLOCATOR_PIN and sha(BANK_CONTRACTS)==BANK_PIN
 contracts=json.loads(CONTRACTS.read_text());banks=json.loads(BANK_CONTRACTS.read_text());base=json.loads(BASE.read_text())
 rng=random.Random(0x8aac0);inputs=[]
 def append(row,data,mode=3,selected=4,**extra):
  n=len(inputs);s=bytearray.fromhex(row['stream_hex']);struct.pack_into('<Ii',s,0,DATA,struct.unpack_from('<i',s,4)[0]);struct.pack_into('<i',s,12,mode)
  prefix=bytearray(rng.randbytes(16));handle=(0x12340000,0x678903ff,0x90000207)[n%3]
  struct.pack_into('<Ih',prefix,0,handle,3)
  inputs.append(dict(handle=handle,prefix_hex=prefix.hex(),selected=selected,live_hex=row['bank_hex'],data_hex=data,
   stream_hex=s.hex(),secondary_pending=rng.getrandbits(32),creation_size=rng.getrandbits(32),creation_address=rng.getrandbits(32),
   initial_mask=rng.getrandbits(32),initial_count=n%3,capacity=(0,1,2,0xffffffff)[n%4],**extra))
 for row,written in zip(base['writers'],base['versions']['retail']['written']):
  append(dict(row,bank_hex=rng.randbytes(40).hex()),written['data_hex'])
 for row in base['readers']:append(row,row['data_hex'])
 # Mode4 really zeroes then invokes definition19; all ten registered selector
 # slots distinguish actual matching initialization from early cleanup.
 for selected in range(10):
  for n in range(128):
   old=base['writers'][n];append(old,base['versions']['retail']['written'][n]['data_hex'],4,selected)
 # Full stale IDs and NONE reject before any allocator or stream activity.
 for n in range(64):
  row=dict(inputs[n]);row['handle']=0xffffffff if n%2 else row['handle']^0x40000000;row['identity_control']=True;inputs.append(row)
 # Negative bank size fails real allocation, calls actual compaction, retries,
 # and returns3 without entering bank initialization/read.
 for n in range(16):
  row=dict(inputs[n]);row['state_size']=-1-n;row['allocation_failure_control']=True;inputs.append(row)
 versions={};listings=[]
 for name in ('retail','tu11','u5'):
  t=KingTrace(name);setup=setup_allocator(t,name);b=BOOT[name];wrapper=setup['wrapper'];obj=setup['object']
  owner_table=next(v for v in contracts['actual_virtual_tables']if v['image']==name and v['kind']=='entity_replication_owner')
  definition=next(v for v in banks['entity_definition_tables']if v['image']==name)
  entry=t.u(owner_table['address']+6*4);assert entry==contracts['entries'][name]['entry_update_decode']
  t.w(THIS,definition['vtable']);t.w(OWNER,owner_table['address']);t.w(OWNER+0x10,DEFS);t.w(DEFS+16,THIS)
  methods={key:t.u(t.u(obj)+slot*4)for key,slot in (('allocate',5),('info',1),('release',0),('compact',10))}
  callbacks={key:t.u(definition['vtable']+slot*4)for key,slot in (('initialize',19),('read',15))}
  definition_identifier=t.call(t.u(definition['vtable']),registers=((UC_X86_REG_ECX,THIS),))
  identities=[]
  for selected in range(10):
   engine=t.u(t.spec['table']+selected*4)
   identifier=t.call(t.u(t.u(engine)+39*4),registers=((UC_X86_REG_ECX,engine),))if engine else -1
   identities.append(dict(present=bool(engine),identifier=identifier))
  active=False;events=[];private=0;row_at=0;stride=32 if name=='retail'else 36
  def observe(g,address,size,_):
   nonlocal private
   if not active:return
   kind=next((key for key,value in {**methods,**callbacks}.items()if value==address),None)
   if not kind:return
   sp=g.reg_read(UC_X86_REG_ESP);event=dict(kind=kind,count=t.u(wrapper+4),stream_hex=bytes(g.mem_read(STREAM,52)).hex())
   if kind=='allocate':event['size']=t.u(sp+4);assert t.u(sp+8)==t.u(sp+12)==0
   elif kind=='compact':assert t.u(sp+4)==0
   elif kind in ('info','release'):
    event['private_address']=t.u(sp+4);assert event['private_address']==private
    if kind=='release':assert t.u(sp+8)==0xffffffff
    event['bank_hex']=bytes(g.mem_read(private,40)).hex()
   elif kind=='initialize':
    private=t.u(sp+16);event.update(creation_size=t.u(sp+4),creation_address=t.u(sp+8),state_size=t.u(sp+12),
      private_address=private,bank_hex=bytes(g.mem_read(private,40)).hex())
   elif kind=='read':
    private=t.u(sp+16);assert t.u(sp+4)==0 and t.u(sp+20)==STREAM
    event.update(state_size=t.u(sp+12),initial_mask=t.u(t.u(sp+8)),private_address=private,bank_hex=bytes(g.mem_read(private,40)).hex())
   events.append(event)
  t.g.hook_add(UC_HOOK_CODE,observe);results=[]
  for index,row in enumerate(inputs):
   t.w(M+0xc14,row['selected']);t.w(M+0x24,0xaabbccdd)
   row_at=OWNER+0x14+(row['handle']&1023)*stride
   raw=bytes.fromhex(row['prefix_hex'])+(struct.pack('<I',row['secondary_pending'])if name!='retail'else b'')
   raw+=struct.pack('<4I',row['creation_size'],row['creation_address'],row.get('state_size',40)&0xffffffff,LIVE)
   t.g.mem_write(row_at,raw+b'\x79'*8);t.g.mem_write(LIVE-8,b'\x74'*8+bytes.fromhex(row['live_hex'])+b'\x74'*8)
   t.g.mem_write(DATA-8,b'\x75'*8+bytes.fromhex(row['data_hex'])+b'\x75'*8)
   t.g.mem_write(STREAM,bytes.fromhex(row['stream_hex']));t.w(MASK,row['initial_mask']);t.w(COUNT,row['initial_count'])
   t.g.mem_write(BLOCKS,b'\xa5'*40);before=t.u(wrapper+4);events.clear();private=0;active=True
   try:status=t.call(entry,(row['handle'],MASK,row['capacity'],COUNT,BLOCKS,STREAM),((UC_X86_REG_ECX,OWNER),))
   except Exception as error:raise RuntimeError(f'{name} input {index}: {error}')from error
   finally:active=False
   assert bytes(t.g.mem_read(row_at,stride))==raw and bytes(t.g.mem_read(row_at+stride,8))==b'\x79'*8
   assert bytes(t.g.mem_read(LIVE-8,56))==b'\x74'*8+bytes.fromhex(row['live_hex'])+b'\x74'*8
   assert bytes(t.g.mem_read(DATA-8,96))==b'\x75'*8+bytes.fromhex(row['data_hex'])+b'\x75'*8
   bank_hex=None
   if status==0:
    record=BLOCKS+row['initial_count']*8;assert t.u(record+4)==private and t.u(record)==0xd0028
    bank_hex=bytes(t.g.mem_read(private,40)).hex()
   elif events and events[-1]['kind']=='release':bank_hex=events[-1]['bank_hex']
   else:assert status==3
   after_stream=bytes(t.g.mem_read(STREAM,52));cap=struct.unpack_from('<i',after_stream,4)[0];cursor=struct.unpack_from('<i',after_stream,16)[0]
   results.append(dict(status=status,mask=t.u(MASK),produced_count=t.u(COUNT),blocks_hex=bytes(t.g.mem_read(BLOCKS,40)).hex(),
    stream_hex=after_stream.hex(),bank_hex=bank_hex,private_address=private,entry_hex=raw.hex(),
    allocator_count_before=before,allocator_count_after=t.u(wrapper+4),events=list(events),overflow=bool(cursor>cap*8 or after_stream[20])))
  path=args.output.parent/f'{name}-executed.asm';digest=hashlib.sha256()
  with path.open('w')as output:
   for address in sorted(t.executed):
    ins=t.code.at(address);assert bytes(t.g.mem_read(address,ins.size))==bytes(ins.bytes)
    digest.update(struct.pack('<I',address)+bytes(ins.bytes));output.write(f'{address:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n')
  listings.append(dict(image=name,file=path.name,instructions=len(t.executed),sha256=sha(path)))
  versions[name]=dict(image_sha256=t.spec['pin'],entry=entry,row_stride=stride,allocator_setup=setup,
    identities=identities,definition_identifier=definition_identifier,executed_instruction_sha256=digest.hexdigest(),results=results)
  if name!='retail':
   def normalized(result):
    result=dict(result);result['entry_hex']=result['entry_hex'][:32]+result['entry_hex'][40:];return result
   assert [normalized(row)for row in results]==versions['retail']['results'],name
 fixture=dict(schema='original-engine-entry-decode-v1',boundary=__doc__,inputs=inputs,versions=versions,
  live_address=LIVE,data_address=DATA,instruction_patches=[],supplied_returns=[])
 args.fixture.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'))+'\n')
 results=versions['retail']['results'];summary=dict(images=3,inputs=len(inputs),whole_decode_calls=3*len(inputs),
  successful_decodes=3*sum(r['status']==0 for r in results),successful_overflow_decodes=3*sum(r['status']==0 and r['overflow']for r in results),
  actual_reads=3*sum(sum(e['kind']=='read'for e in r['events'])for r in results),
  initialized_banks=3*sum(sum(e['kind']=='initialize'for e in r['events'])for r in results),
  released_rejections=3*sum(sum(e['kind']=='release'for e in r['events'])for r in results),
  actual_compactions=3*sum(sum(e['kind']=='compact'for e in r['events'])for r in results))
 report=dict(schema='original-engine-entry-decode-report-v1',boundary=__doc__,summary=summary,oracle_sha256=sha(__file__),
  orchestration_sha256=sha(CONTRACTS),bank_contracts_sha256=sha(BANK_CONTRACTS),base_fixture_sha256=sha(BASE),allocator_tool_sha256=sha(ALLOCATOR_TOOL),
  fixture_sha256=sha(args.fixture),instruction_listings=listings,
  versions={name:{k:v for k,v in row.items()if k!='results'}for name,row in versions.items()},instruction_patches=[],supplied_returns=[])
 args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(summary),flush=True)
if __name__=='__main__':main()
