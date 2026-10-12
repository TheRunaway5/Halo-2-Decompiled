#!/usr/bin/env python3
"""Authenticate the complete missing main heap producer, without heap captures."""
import argparse, csv, hashlib, json, struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, norm, full_tokens
WORLD = ROOT/'analysis/multiplayer/sandbox-source/physics-game-frame-runtime-20261011/correspondence-v2/complete-report.json'
WORLD_PIN = '92379da9dc9d29baea9eaca194cebdab7d69f8be319aecf6cd0b759e26e744cc'
POOL = ROOT/'analysis/multiplayer/sandbox-source/physics-pool-startup-tu15-20261012/correspondence-v1/complete-report.json'
POOL_PIN = '6d10227d9838a4e6e2e2db443df0c699bbb203a7843a2b6f756a569fd755b527'
HEAP_ENTRIES = (0x148190,0x22e4e0,0x2dd590,0x22e950,0x22eb20,0x148a10,0x148a70,0x22e1d0,0x22e380,0x22e400,0x22e420,0x22e430)
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
 assert sha(old_path)==PIN and sha(NEW_DEFAULT)==NEW_PIN
 assert sha(WORLD)==WORLD_PIN and sha(POOL)==POOL_PIN
 old,new=Xbe(old_path),Xbe(NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
 of=_trace(oc,0x1482a0,starts);oi=[oc.at(p)for p in sorted(of.code)]
 md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;md.skipdata=True
 sec=new.section('.text');linear=list(md.disasm(new.section_bytes(sec),sec.va))
 ns={i.operands[0].imm for i in linear if i.id and i.mnemonic=='call' and i.operands[0].type==2 and nc.inside(i.operands[0].imm)}
 tokens=[norm(i,new) if i.id else '' for i in linear];prefix=[norm(i,old)for i in oi[:6]];matches=[]
 for n in range(len(tokens)-len(prefix)):
  if tokens[n:n+len(prefix)]!=prefix:continue
  q=linear[n].address;nf=_trace(nc,q,ns|{q});ni=[nc.at(p)for p in sorted(nf.code)]
  if full_tokens(oi,old)==full_tokens(ni,new):matches.append((q,nf,ni))
 assert len(matches)==1,[(hex(x[0]),len(x[2]))for x in matches]
 q,nf,ni=matches[0];assert q==0x149630 and len(oi)==len(ni)==22 and not of.tables and not nf.tables
 address_map={};instruction_map={};rows=[]
 for x,y in zip(oi,ni):
  instruction_map[f'{x.address:08x}']=y.address
  assert x.mnemonic==y.mnemonic and len(x.operands)==len(y.operands)
  for u,v in zip(x.operands,y.operands):
   assert u.type==v.type and u.size==v.size
   p=z=None
   if u.type==2:p,z=u.imm&0xffffffff,v.imm&0xffffffff
   if u.type==3:p,z=u.mem.disp&0xffffffff,v.mem.disp&0xffffffff
   if p is not None and p!=z:
    assert p not in address_map or address_map[p]==z;address_map[p]=z
  rows.append(dict(old=x.address,later=y.address,old_bytes=x.bytes.hex(),later_bytes=y.bytes.hex(),mnemonic=x.mnemonic))
 assert address_map[0x33785c]==0x33b3ac and address_map[0x2d817e]==0x2dbc2e and address_map[0x480d90]==0x48605c
 pool=json.loads(POOL.read_text());assert pool['entries']['0033785c']==address_map[0x33785c]
 world=json.loads(WORLD.read_text());selected={f'{p:08x}':world['entries'][f'{p:08x}']for p in HEAP_ENTRIES}
 # Literal callable vtables and loader globals come from the selected executable.
 tables={}
 for p in (0x45e77c,0x45e7a8):
  v=world['address_map'][f'{p:08x}'];words=struct.unpack('<7I',new.read(v,28))
  tables[f'{p:08x}']=dict(later=v,hex=new.read(v,28).hex(),methods=words)
 globals={f'{p:08x}':world['address_map'][f'{p:08x}']for p in (0x480d88,0x480d8c,0x480d90,0x480d98,0x480d9c,0x480da0,0x480db4)}
 a.output.mkdir(parents=True,exist_ok=False)
 for label,ins in [('disc',oi),('later',ni)]:
  (a.output/f'001482a0-{label}.asm').write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
 report=dict(schema=1,tool_sha256=sha(__file__),original_sha256=PIN,later_original_sha256=NEW_PIN,
  world_report_sha256=WORLD_PIN,pool_report_sha256=POOL_PIN,entries={'001482a0':q},complete_bodies=1,
  instruction_map=instruction_map,address_map={f'{p:08x}':v for p,v in address_map.items()},instructions=rows,
  existing_world_entries=selected,vtables=tables,globals=globals,
  failure_owner=dict(old=0x2d817e,later=0x2dbc2e,compiled=False),
  captures_supplied=False,live_world_startup_proven=False)
 (a.output/'complete-report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(sha(a.output/'complete-report.json'))
if __name__=='__main__':main()
