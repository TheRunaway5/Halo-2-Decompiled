#!/usr/bin/env python3
"""Execute unchanged original King packet writers/readers with real bitstreams.

The actual entity vtables, flag builders, checkpoints, rollback, word writes and
bit readers execute in retail/TU1.1/TU1.5. Buffers, padding, capacities, cursors,
mode, prior bank and flags are declared ABI inputs. Codec success is distinct
from stream overflow or packet acceptance. No native code, instruction patches,
supplied service returns, actual network delivery or live match is involved.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_ECX
from oracle_king_engine import KingTrace,D
from oracle_game_variants import ROOT
from oracle_mp_full_scoreboard_1_5 import sha

OUT=ROOT/'analysis/multiplayer/game-variants/king-wire-20261012/complete'
CONTRACTS=ROOT/'analysis/multiplayer/game-variants/king-replication-source-20261012/complete/report.json'
WIRE_CONTRACTS=ROOT/'analysis/multiplayer/game-variants/king-wire-source-20261012/complete/report.json'
DATA,BANK,STREAM,FLAGS,THIS=D+0x28000,D+0x28100,D+0x28200,D+0x28300,D+0x28400
def random_bytes(rng,n):return bytes(rng.getrandbits(8)for _ in range(n))
def stream(capacity,cursor,mode,error=0):
 values=[DATA,capacity,0xa5a5a5a5,mode,cursor,error|0x5a3c7e00,0]+[0xa5a5a5a5]*4+[0,0]
 return struct.pack('<13I',*values)
def bank(rng,n):
 first=rng.getrandbits(8);assigned=rng.getrandbits(9);flags=first&rng.getrandbits(8)
 active=flags&rng.getrandbits(8);playing=active&rng.getrandbits(8)
 words=[first,assigned,flags,playing,active]+[rng.randrange(8)if assigned&(1<<i)else 65535 for i in range(9)]
 return struct.pack('<14H2B3HhH',*words,n%4,n%2,n%32,(-1,0,1,32767)[n%4]&65535,0xa55a,
  (-1,0,1,7,14)[n%5],rng.getrandbits(16))
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/king_wire_original.json');args=ap.parse_args()
 args.output.parent.mkdir(parents=True,exist_ok=True);args.fixture.parent.mkdir(parents=True,exist_ok=True)
 contracts=json.loads(CONTRACTS.read_text());wire_contracts=json.loads(WIRE_CONTRACTS.read_text())
 rng=random.Random(0x9a3a0);writers=[];readers=[]
 for n in range(2048):
  cap=(0,1,2,3,4,5,8,12,16,32,64)[(n//128+n)%11];cursor=(n//128)%8
  writers.append(dict(bank_hex=bank(rng,n).hex(),data_hex=(random_bytes(rng,80)if n%4==0 else bytes(80)).hex(),
   stream_hex=stream(cap,cursor,1,n%2).hex(),requested=n&127,reserve=(0,1,2,7)[n//512],
   initial_written=(0,0x80000000,rng.getrandbits(32))[n%3],ignored=[0xffffffff,0x12345678,0x87654321]))
 # Checked-value diagnostics execute the actual original CRT formatter. They
 # produce a discarded stack message, then continue with the same low-bit
 # writes; no rejected value or successful return is supplied by the harness.
 invalid=((0,65535),(2,65535),(4,65535),(6,65535),(8,65535),
  (10,65535),(10,8),(10,15),(10,32767),(28,255),(30,65535),(30,32768),
  (32,32768),(32,65534),(36,32768),(36,65534),(36,15),(36,32767))
 for index,(at,value)in enumerate(invalid):
  raw=bytearray(bank(rng,2048+index))
  if at==28:raw[at]=value
  else:struct.pack_into('<H',raw,at,value)
  if at==10:struct.pack_into('<H',raw,2,int.from_bytes(raw[2:4],'little')|1)
  writers.append(dict(bank_hex=raw.hex(),data_hex=bytes(80).hex(),stream_hex=stream(64,index%8,1,index%2).hex(),
   requested=127,reserve=0,initial_written=0,ignored=[0xffffffff,0x12345678,0x87654321],checked_value_offset=at))
 for n in range(1024):
  cap=(0,1,2,3,4,5,8,12,16,32,64)[n%11]
  cursor=(0,n%8,max(0,cap*8-1),cap*8,cap*8+1)[(n//11)%5]
  readers.append(dict(bank_hex=random_bytes(rng,40).hex(),data_hex=random_bytes(rng,80).hex(),
   stream_hex=stream(cap,cursor,3,n%2).hex(),initial_read=rng.getrandbits(32)))
 versions={};all_instructions=[]
 for name in ('retail','tu11','u5'):
  t=KingTrace(name);definition=next(row for row in contracts['entity_definition_tables']if row['image']==name)
  assert t.spec['pin']==contracts['images'][name]['sha256']
  table=definition['vtable'];entries={key:t.u(table+slot*4)for key,slot in (('write',14),('read',15),('size',11))}
  entries['overflow']=wire_contracts['entries'][name]['stream_overflowed']
  assert entries['write']==contracts['entries'][name]['king_wire_write'] and entries['read']==contracts['entries'][name]['king_wire_read']
  t.w(THIS,table);name_pointer=t.call(t.u(table+4),registers=((UC_X86_REG_ECX,THIS),))
  assert t.image.read(name_pointer,20).split(b'\0')[0]==b'king-engine-globals'
  def setup(row):
   t.g.mem_write(DATA-8,b'\x73'*8+bytes.fromhex(row['data_hex'])+b'\x73'*8)
   t.g.mem_write(BANK-8,b'\x74'*8+bytes.fromhex(row['bank_hex'])+b'\x74'*8)
   t.g.mem_write(STREAM,bytes.fromhex(row['stream_hex']))
  def observe(result):
   assert bytes(t.g.mem_read(DATA-8,8))==bytes(t.g.mem_read(DATA+80,8))==b'\x73'*8
   assert bytes(t.g.mem_read(BANK-8,8))==bytes(t.g.mem_read(BANK+40,8))==b'\x74'*8
   raw=bytes(t.g.mem_read(STREAM,52));capacity,cursor,error=struct.unpack_from('<I',raw,4)[0],struct.unpack_from('<I',raw,16)[0],raw[20]
   overflow=bool(t.call(entries['overflow'],registers=((UC_X86_REG_ECX,STREAM),))&255)
   assert overflow==bool(cursor>capacity*8 or error) and bytes(t.g.mem_read(STREAM,52))==raw
   return dict(data_hex=bytes(t.g.mem_read(DATA,80)).hex(),bank_hex=bytes(t.g.mem_read(BANK,40)).hex(),
    stream_hex=raw.hex(),flags=t.u(FLAGS),returned_al=result&255,overflow=overflow)
  written=[];decoded=[];random_read=[];sizes=[]
  for n,row in enumerate(writers):
   setup(row);t.w(FLAGS,row['initial_written'])
   ignored=row['ignored'];argv=(ignored[0],row['requested'],FLAGS,ignored[1],BANK,ignored[2],STREAM,row['reserve'])
   try:result=t.call(entries['write'],argv,((UC_X86_REG_ECX,THIS),))
   except Exception:raise RuntimeError(f'{name} writer {n} {row}')from None
   after=observe(result);assert after['bank_hex']==row['bank_hex'];written.append(after)
   # Fresh read state starts at the same original bit offset. The declared
   # storage/error/padding are retained; this does not label a failed/partial
   # writer output an accepted packet.
   read_row=dict(bank_hex=('a5'*40),data_hex=after['data_hex'],stream_hex=stream(
    struct.unpack_from('<I',bytes.fromhex(row['stream_hex']),4)[0],
    struct.unpack_from('<I',bytes.fromhex(row['stream_hex']),16)[0],3,n%2).hex(),initial_read=0xffffffff)
   setup(read_row);t.w(FLAGS,read_row['initial_read'])
   result=t.call(entries['read'],(0xffffffff,FLAGS,0x12345678,BANK,STREAM),((UC_X86_REG_ECX,THIS),))
   after_read=observe(result);assert after_read['data_hex']==read_row['data_hex'];decoded.append(after_read)
  for n,row in enumerate(readers):
   setup(row);t.w(FLAGS,row['initial_read'])
   result=t.call(entries['read'],(0xffffffff,FLAGS,0x12345678,BANK,STREAM),((UC_X86_REG_ECX,THIS),))
   after=observe(result);assert after['data_hex']==row['data_hex'];random_read.append(after)
  for mask in range(128):
   t.w(FLAGS,mask);t.w(BANK,0xa5a5a5a5)
   t.call(entries['size'],(0xffffffff,FLAGS,BANK),((UC_X86_REG_ECX,THIS),))
   assert t.u(FLAGS)==mask;sizes.append(t.u(BANK))
  digest=hashlib.sha256();path=args.output.parent/f'{name}-executed.asm'
  with path.open('w')as out:
   for address in sorted(t.executed):
    ins=t.code.at(address);assert bytes(t.g.mem_read(address,ins.size))==bytes(ins.bytes)
    digest.update(struct.pack('<I',address)+bytes(ins.bytes))
    out.write(f'{address:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n')
  versions[name]=dict(image_sha256=t.spec['pin'],entries=entries,written=written,decoded=decoded,random_read=random_read,sizes=sizes,
   observed_instructions=len(t.executed),executed_instruction_sha256=digest.hexdigest())
  all_instructions.append(dict(image=name,file=path.name,sha256=sha(path),instructions=len(t.executed)))
  if name!='retail':
   for key in ('written','decoded','random_read','sizes'):assert versions[name][key]==versions['retail'][key],(name,key)
 fixture=dict(schema='original-king-wire-v1',boundary=__doc__,writers=writers,readers=readers,versions=versions,
  data_address=DATA,instruction_patches=[],supplied_returns=[])
 args.fixture.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'))+'\n')
 rows=versions['retail'];summary=dict(versions=3,writer_inputs=len(writers),roundtrip_read_inputs=len(writers),random_read_inputs=len(readers),
  size_inputs=128,whole_codec_calls=3*(len(writers)*2+len(readers)+128),
  whole_stream_query_calls=3*(len(writers)*2+len(readers)),
  checked_value_inputs=len(invalid),
  successful_zero_mask_writes=sum(r['returned_al']!=0 and r['flags']==0 for r in rows['written']),
  successful_overflow_reads=sum(r['returned_al']!=0 and r['overflow']for r in rows['decoded']+rows['random_read']))
 report=dict(schema='original-king-wire-report-v1',boundary=__doc__,summary=summary,oracle_sha256=sha(__file__),fixture_sha256=sha(args.fixture),
  static_contracts_sha256=sha(CONTRACTS),wire_contracts_sha256=sha(WIRE_CONTRACTS),instruction_listings=all_instructions,
  versions={name:{key:value for key,value in row.items()if key not in('written','decoded','random_read','sizes')}for name,row in versions.items()},
  instruction_patches=[],supplied_returns=[])
 args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(summary),flush=True)
if __name__=='__main__':main()
