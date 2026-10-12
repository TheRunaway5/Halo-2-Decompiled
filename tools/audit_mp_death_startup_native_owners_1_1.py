#!/usr/bin/env python3
"""Authenticate complete native startup coverage against original execution.

The declared native resource, cache I/O and thread/scheduling contracts remain
separate from original game instruction coverage. No captured game bank or
native expected result is used. This does not establish live Match admission,
world construction, corpse rendering, death-camera wiring or Xbox execution.
"""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN,CACHE_PIN

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for name in ('report','oracle','bindings','output'):ap.add_argument('--'+name,type=Path,required=True)
 a=ap.parse_args();j=json.loads(a.report.read_text());b=json.loads(a.bindings.read_text())
 assert j['source_sha256']==PIN and j['oracle_sha256']==sha(a.oracle)
 assert not j['instruction_patches'] and not j['supplied_gameplay_results']
 assert b['tool_sha256']==sha(ROOT/'tools/generate_death_world_runtime_bindings_1_1.py')
 assert b['generated_sha256']==sha(ROOT/'src/death_world_runtime_entries.hpp')
 native=set()
 for owner in b['owners'].values():
  assert owner['sha256']==sha(ROOT/owner['source'])
  native.update(int(p,16)for p in owner['complete_entries'])
 image_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(image_path)==PIN
 image=Xbe(image_path);code=_Code(image,'.text')
 listing=a.report.parent/'executed-original.asm';assert sha(listing)==j['instruction_listing_sha256']
 executed=set()
 for row in listing.read_text().splitlines():
  pc,raw,*_=row.split();pc=int(pc,16);raw=bytes.fromhex(raw)
  assert image.read(pc,len(raw))==raw;executed.add(pc)
 assert len(executed)==j['executed_original_instructions']
 stores=a.report.parent/j['store_file'];assert sha(stores)==j['store_sha256'] and stores.stat().st_size==25*j['stores']
 covered=set()
 for entry in native:covered.update(_trace(code,entry,native).code)
 missing=sorted(executed-covered);assert not missing,[hex(p)for p in missing]
 assert all(int(step['entry'],16)in native for step in j['steps'])
 cache=None
 if 'cache_delivery'in j:
  path=ROOT/'assets/disc/maps/lockout.map';assert j['cache_sha256']==sha(path)==CACHE_PIN
  raw=path.read_bytes();u=lambda p:int.from_bytes(raw[p:p+4],'little')
  ix=u(16);n=u(20);size=u(24);magic=u(ix)-(ix+32);table=u(ix+8)-magic
  address=u(table+8);regions=[(ix+magic,raw[ix:ix+n]),(address,raw[ix+n:ix+n+size])]
  expected=dict(index=ix+magic,table=u(ix+8),count=u(ix+24),scenario=u(ix+12),globals=u(ix+16),
      regions=[dict(address=p,bytes=len(v),sha256=hashlib.sha256(v).hexdigest())for p,v in regions])
  assert j['cache_delivery']==expected;cache=expected
 result=dict(tool_sha256=sha(Path(__file__)),source_sha256=PIN,report_sha256=sha(a.report),
   oracle_sha256=sha(a.oracle),bindings_sha256=sha(a.bindings),original_roots=len(j['steps']),
   original_retained_stores=j['stores'],original_executed_instructions=len(executed),
   complete_native_entries=len(native),uncovered_executed_instructions=[],
   cache_delivery=cache,original_pool_valid=j['object_pool_valid'],
   captured_gameplay_results_used=False,live_match_proof=False,boundaries=__doc__)
 a.output.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({k:result[k]for k in ('original_roots','original_retained_stores','original_executed_instructions','uncovered_executed_instructions')}))
if __name__=='__main__':main()
