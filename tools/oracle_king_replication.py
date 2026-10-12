#!/usr/bin/env python3
"""Execute registered original King/shared engine-bank slots41..46 in three XBEs.

Original registration, bank initialization/differences, shared application,
hill rebuilds and reached services run without patches or supplied returns.
Declared empty player pools and disabled recording exercise their real gates.
Phase1 world initialization is deliberately excluded; phase3 uses the original
mode4/no-map-reset gate. This is not network serialization, native-session
delivery, an occupied-world callback test or complete King match evidence.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX
from oracle_king_engine import KingTrace,source_maps,synthetic_maps,D,M,G,C,K,P
from oracle_mp_full_scoreboard_1_5 import literal,sha
from oracle_game_variants import ROOT

OUT=ROOT/'analysis/multiplayer/game-variants/king-replication-20261012/extended'
BANK,FLAGS,BITMAP=D+0x28000,D+0x28100,D+0x28200
SLOTS=dict(shared_initialize=41,shared_difference=42,shared_apply=43,
 king_initialize=44,king_difference=45,king_apply=46)

def signed16(v):return struct.unpack('<h',struct.pack('<H',v&65535))[0]
def signed32(v):return v if v<0x80000000 else v-0x100000000
def random_bytes(rng,n):return bytes(rng.getrandbits(8)for _ in range(n))
def shared(raw):
 return dict(words=list(struct.unpack_from('<14H',raw,6)),phase=struct.unpack_from('<h',raw,0x6c)[0],
  round=struct.unpack_from('<H',raw,0x6e)[0],clock_seconds=struct.unpack_from('<H',raw,0xe0)[0],game_over=raw[0xc08])
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/king_replication_original.json')
 args=ap.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True);args.fixture.parent.mkdir(parents=True,exist_ok=True)
 rng=random.Random(0x2c7940);maps=source_maps()+synthetic_maps();blobs={};inputs=[]
 def blob(raw):
  digest=hashlib.sha256(raw).hexdigest();blobs[digest]=raw.hex();return digest
 for ignored in (0,0xffffffff,0x12345678):
  inputs.append(dict(operation='initialize',bank=blob(random_bytes(rng,40)),ignored=ignored))
 # Full masks, equality patterns, raw-byte normalization, signed/narrowed
 # values and a second difference against the actual first output.
 for n in range(1024):
  raw=bytearray(random_bytes(rng,0xc18));bank=bytearray(random_bytes(rng,40));pattern=n//256
  phase=(-32768,-1,0,1,2,3,4,255,256,32767)[n%10]
  struct.pack_into('<h',raw,0x6c,phase);raw[0xc08]=(0,1,2,128,255)[n%5]
  hill=(-2147483648,-65537,-32769,-32768,-1,0,1,7,32767,32768,65535,65536,2147483647)[n%13]
  struct.pack_into('<i',raw,0xfc+0x1a0,hill)
  if pattern&1:
   bank[:28]=raw[6:34];bank[28]=phase&255;bank[29]=int(raw[0xc08]!=0)
   bank[30:32]=raw[0x6e:0x70];bank[32:34]=raw[0xe0:0xe2]
  if pattern&2:bank[36:38]=struct.pack('<H',hill&65535);bank[38:40]=raw[0xfc+0x1a8:0xfc+0x1aa]
  # Force each shared word mismatch in isolation as well as random banks.
  if pattern==3:bank[(n%14)*2]^=0x80
  mask=(n&255)|((0,0x80000000,0xffffff00,0x12345600)[pattern])
  inputs.append(dict(operation='difference',globals=blob(raw),bank=blob(bank),requested=mask,
    ignored=(0,0xffffffff)[n%2],repeat=True))
 # Shared slot42 ORs rather than replaces the caller's changed mask.
 for n in range(128):
  inputs.append(dict(operation='shared_difference',globals=inputs[3+n]['globals'],bank=inputs[3+n]['bank'],
    requested=n,initial_changed=rng.getrandbits(32)))
 # Literal scenario inputs and original-generated geometry states are produced
 # afresh per image, never taken from a native implementation.
 variants={};sources={};listings=[];totals={}
 for name in ('retail','tu11','u5'):
  t=KingTrace(name);sources[name]=t;obj=t.u(t.spec['table']+16);vtable=t.u(obj)
  entries={key:t.u(vtable+slot*4)for key,slot in SLOTS.items()}
  targets=[int(i.op_str,16)for i in literal(t.code,entries['shared_apply'],t.starts)[1]
    if i.mnemonic=='call'and i.op_str.startswith('0x')]
  assert len(targets)==4;services=dict(zip(('refresh_player_properties','refresh_recording','phase_changed','begin_game_over'),targets))
  first=literal(t.code,services['refresh_recording'],t.starts)[1][0]
  assert first.mnemonic=='mov'and first.op_str.startswith('al, byte ptr [0x')
  recorder=int(first.op_str.split('[0x')[1].split(']')[0],16)
  events=[];active=False
  def hook(g,a,n,_):
   if not active:return
   if a in services.values() or a==t.a['build']:
    kind=next((k for k,v in services.items()if v==a),'build')
    event=dict(kind=kind,shared=shared(bytes(g.mem_read(M,0xc18))),king=blob(t.raw()),
      game_over_hex=bytes(g.mem_read(G+0x1128,8)).hex())
    if kind=='phase_changed':event['phase']=g.reg_read(UC_X86_REG_EAX)&255
    events.append(event)
  t.g.hook_add(UC_HOOK_CODE,hook)
  def invoke(key,argv):
   nonlocal active
   events.clear();active=True
   try:return t.call(entries[key],argv,((UC_X86_REG_ECX,obj),))
   finally:active=False
  def bank_bytes():return bytes(t.g.mem_read(BANK,40))
  rows=[];calls=0
  for row in inputs:
   t.g.mem_write(BANK-8,b'\xa7'*8+bytes.fromhex(blobs[row['bank']])+b'\xa7'*8)
   kind=row['operation']
   if kind=='initialize':
    invoke('king_initialize',(row['ignored'],BANK));calls+=1
    rows.append(dict(bank=blob(bank_bytes())))
   else:
    raw=bytes.fromhex(blobs[row['globals']]);t.g.mem_write(M,raw)
    mask=row.get('initial_changed',row['requested']);t.w(FLAGS,mask)
    argv=(row['requested'],FLAGS,BANK)if kind=='shared_difference'else(FLAGS,row['ignored'],BANK)
    invoke('shared_difference'if kind=='shared_difference'else'king_difference',argv);calls+=1
    result=dict(bank=blob(bank_bytes()),changed=t.u(FLAGS))
    assert bytes(t.g.mem_read(M,0xc18))==raw and not events
    if row.get('repeat'):
     t.w(FLAGS,row['requested']);invoke('king_difference',(FLAGS,row['ignored'],BANK));calls+=1
     result.update(repeated_bank=blob(bank_bytes()),repeated_changed=t.u(FLAGS))
     assert bytes(t.g.mem_read(M,0xc18))==raw and not events
    rows.append(result)
   assert bytes(t.g.mem_read(BANK-8,8))==bytes(t.g.mem_read(BANK+40,8))==b'\xa7'*8
  apply_inputs=[];apply_rows=[];hill_count=0
  for map_index,m in enumerate(maps):
   t.setup(bytes.fromhex(m['rows_hex']));t.run('initialize')
   count=signed16(int.from_bytes(t.groups()[:2],'little'))
   hills=list(struct.unpack('<8h',t.groups()[2:]))[:count]
   if map_index<12:hill_count+=len(hills)
   # A prior valid shape also exercises the retained-geometry path for later
   # partial groups and clears only marker_count when NONE arrives.
   t.run('build',(K,0));initial=bytearray(t.raw())
   for index,hill in enumerate(hills or [0]):
    masks=(0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,127,0xffffffff)
    if index==0:masks+=tuple(range(32))
    for mask_index,mask in enumerate(masks):
     raw=bytearray(b'\xa5'*0xc18);raw[0xfc:0xfc+0x1dc]=initial
     struct.pack_into('<i',raw,0xfc+0x1a0,(hill,-1,65535)[mask_index%3])
     phase=(0,2,3,4,255)[mask_index%5];struct.pack_into('<h',raw,0x6c,(-1,phase)[mask_index%2])
     bank=bytearray(random_bytes(rng,40))if name=='retail'else bytearray.fromhex(blobs[variants['retail']['apply_inputs'][len(apply_inputs)]['bank']])
     bank[28]=phase;bank[29]=(0,1,255)[mask_index%3]
     struct.pack_into('<h',bank,36,(hill,-1,signed16(0x8000))[mask_index%3])
     # No invalid geometry pointer is supplied: all marker rows are authored
     # inputs, including synthetic empty/partial groups covered by build.
     t.g.mem_write(M,bytes(raw));t.g.mem_write(G,bytes(0x1200));t.g.mem_write(G+0xc,b'\x04')
     t.g.mem_write(G+0x1128,bytes([mask_index%2,(mask_index//2)%2,0,0])+struct.pack('<i',-27))
     t.h(C+2,(-32768,-1,0,1,30,32767)[mask_index%6])
     t.w(P+0x48,BITMAP);t.w(BITMAP,0);t.g.mem_write(recorder,b'\0')
     t.g.mem_write(BANK-8,b'\xa7'*8+bank+b'\xa7'*8)
     inp=dict(map=map_index,globals=blob(raw),bank=blob(bank),mask=mask,ignored=0xffffffff,
       raw_mode=4,game_over_hex=bytes(t.g.mem_read(G+0x1128,8)).hex(),rate=signed16(int.from_bytes(t.g.mem_read(C+2,2),'little')))
     value=invoke('king_apply',(mask,inp['ignored'],BANK));calls+=1
     after=bytes(t.g.mem_read(M,0xc18));apply_rows.append(dict(globals=blob(after),
       game_over_hex=bytes(t.g.mem_read(G+0x1128,8)).hex(),returned_al=value&255,events=list(events)))
     apply_inputs.append(inp)
     assert bank_bytes()==bytes(bank)
     assert bytes(t.g.mem_read(BANK-8,8))==bytes(t.g.mem_read(BANK+40,8))==b'\xa7'*8
  if name!='retail':
   assert rows==variants['retail']['results'] and apply_inputs==variants['retail']['apply_inputs']
   assert apply_rows==variants['retail']['apply_results'],name
  # Every executed original instruction is checked after execution, including
  # constructor, real callbacks and the complete reached spline helper chain.
  digest=hashlib.sha256()
  for address in sorted(t.executed):
   ins=t.code.at(address);actual=bytes(t.g.mem_read(address,ins.size));expected=bytes(ins.bytes)
   assert actual==expected,(name,hex(address));digest.update(struct.pack('<I',address)+actual)
  for key,entry in {**entries,**services}.items():
   _,instructions=literal(t.code,entry,t.starts);path=args.output.parent/f'{name}-{key}-{entry:08x}.asm'
   path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in instructions))
   listings.append(dict(image=name,owner=key,entry=entry,instructions=len(instructions),file=path.name,sha256=sha(path)))
  variants[name]=dict(image_sha256=t.spec['pin'],entries=entries,services=services,recorder_gate=recorder,
   game_over_seconds_hex=t.image.read(t.spec['constants'][0],4).hex(),
   results=rows,apply_inputs=apply_inputs,apply_results=apply_rows,whole_bank_calls=calls,
   observed_instructions=len(t.executed),executed_instruction_sha256=digest.hexdigest())
  totals[name]=dict(whole_bank_calls=calls,apply_calls=len(apply_rows),disc_hills=hill_count,observed_instructions=len(t.executed))
 fixture=dict(schema='original-king-replication-v1',boundary=__doc__,maps=maps,raw_blobs=blobs,inputs=inputs,
  versions=variants,instruction_patches=[],supplied_returns=[])
 args.fixture.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(schema='original-king-replication-report-v1',boundary=__doc__,oracle_sha256=sha(__file__),
  fixture_sha256=sha(args.fixture),versions={name:{k:v for k,v in row.items()if k not in('results','apply_inputs','apply_results')}for name,row in variants.items()},
  summary=dict(versions=3,initialization_inputs=3,difference_inputs=1024,shared_difference_inputs=128,
    apply_inputs=len(variants['retail']['apply_inputs']),whole_bank_calls=sum(v['whole_bank_calls']for v in variants.values()),
    disc_maps=12,disc_hills=57,synthetic_maps=len(maps)-12),instruction_listings=listings,
  instruction_patches=[],supplied_returns=[])
 args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report['summary']),flush=True)
if __name__=='__main__':main()
