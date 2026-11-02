#!/usr/bin/env python3
"""Prepare a native1496F0 frame seam with explicit unsupported child owners.

Only complete no-object-service returns are currently qualified. Empty
services preserve the genuine first-child refusal; no old parent is replayed.
"""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS={'native_memory_pressure_attempts':'calls','native_memory_pressure_level_queries':'level_queries','native_memory_pressure_pool_queries':'pool_queries','native_memory_pressure_reentrant_skips':'reentrant_skips','native_memory_pressure_unbound_services':'unbound_services'}
EXTRA=['native_memory_pressure_completions','native_memory_pressure_refusals']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def change(s,a,b,n=1):assert s.count(a)==n,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);dst=a.output/'staged';base={};pins={};patch=[]
 paths=['src/physics/native_tu15_physics_frame_residuals.cpp','include/halo2/native_tu15_physics_frame_residuals.hpp','tests/physics_multiplayer_acceptance.cpp','tests/native_tu15_physics_frame_tests.cpp','cmake/PhysicsLiveWorldTu15.cmake']
 for n in paths:
  f=a.source_root/n;old=f.read_text();s=old;base[n]=sha(f)
  if n.endswith('physics_frame_residuals.cpp'):
   s=change(s,'#include "halo2/native_tu15_physics_frame_residuals.hpp"','#include "halo2/native_tu15_physics_frame_residuals.hpp"\n#include "halo2/native_tu15_memory_pressure.hpp"')
   before='  s.drain_commands=[&](auto&f){call(0x1496f0u,{},{},{},0,f);};'
   after='''  s.drain_commands=[&](auto&f){
    NativeTu15MemoryPressureStats activity;
    const auto aggregate=[&]{if(stats){
'''+''.join('      stats->'+field+'+=activity.'+member+';\n'for field,member in FIELDS.items())+'''    }};
    try{
      // Object trimming/removal/cleanup children remain deliberately empty.
      // A selected source child counts/refuses at its actual boundary.
      native_tu15_collect_memory_pressure(m,f,{},observe,&activity);
    }catch(...){aggregate();if(stats)++stats->native_memory_pressure_refusals;throw;}
    aggregate();if(stats)++stats->native_memory_pressure_completions;
  };''';s=change(s,before,after)
  elif n.endswith('physics_frame_residuals.hpp'):
   anchor='  std::uint64_t frame_calls{},residual_calls{};';s=change(s,anchor,anchor+'\n  std::uint64_t '+','.join(f+'{}'for f in(*FIELDS,*EXTRA))+';')
  elif n.endswith('physics_multiplayer_acceptance.cpp'):
   anchor='        {"native_object_publication_attempts",native_frames.native_object_publication_attempts},';s=change(s,anchor,''.join('        {"'+field+'",native_frames.'+field+'},\n'for field in(*FIELDS,*EXTRA))+anchor,2)
   anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_OBJECT_PUBLICATION")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_MEMORY_PRESSURE")) {
    const bool memory=native_frames.native_memory_pressure_completions>0&&
      native_frames.native_memory_pressure_attempts==native_frames.native_memory_pressure_completions&&
      native_frames.native_memory_pressure_refusals==0&&native_frames.native_memory_pressure_unbound_services==0&&
      !native_frames.residual_owner_calls.contains(0x1496f0u)&&!native_frames.residual_owner_calls.contains(0x149ad0u)&&
      !native_frames.residual_owner_calls.contains(0x2335b0u);
    report["native_memory_pressure_gate"]=memory;passed&=memory;
  }
''';s=change(s,anchor,gate+anchor)
  elif n.endswith('.cmake'):
   token='src/physics/native_tu15_physics_frame_residuals.cpp';assert s.count(token)>=1;s=s.replace(token,token+' src/physics/native_tu15_memory_pressure.cpp')
  else:
   s=change(s,'0x2e2990,0x2e2910,0x1496f0,0x31b1a0,0x282c10,0x1cce00,0x1496f0,0x2e2b30,destructor','0x2e2990,0x2e2910,0x31b1a0,0x282c10,0x1cce00,0x2e2b30,destructor')
   anchor='  check(stats.frame_calls==1&&stats.residual_calls==calls.size()&&publications==2,"Residual native/mixed counters");';s=change(s,anchor,anchor+'\n  check(stats.native_memory_pressure_attempts==2&&stats.native_memory_pressure_completions==2&&stats.native_memory_pressure_level_queries==2&&stats.native_memory_pressure_refusals==0&&stats.native_memory_pressure_unbound_services==0&&!stats.residual_owner_calls.contains(0x1496f0u),"Frame did not select native memory-pressure owners");')
   s=change(s,'check(entry==0x1496f0,"Residual failure boundary");','check(entry==0x31b1a0,"Residual failure boundary");')
  t=dst/n;t.parent.mkdir(parents=True,exist_ok=True);t.write_text(s);pins[n]=sha(t);patch.extend(difflib.unified_diff(old.splitlines(True),s.splitlines(True),fromfile='a/'+n,tofile='b/'+n))
 for n in ['src/physics/native_tu15_memory_pressure.cpp','include/halo2/native_tu15_memory_pressure.hpp']:
  t=dst/n;t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/n,t);pins[n]=sha(t);f=a.source_root/n;base[n]=sha(f)if f.exists()else None
 (a.output/'native-memory-pressure-frame.diff').write_text(''.join(patch));(a.output/'source-pins.json').write_text(json.dumps(dict(qualified=False,production_applied=False,live_qualified=False,base_source_root=str(a.source_root.resolve()),base_hashes=base,staged_hashes=pins,fields=[*FIELDS,*EXTRA],gate='HALO2_REQUIRE_NATIVE_MEMORY_PRESSURE',scope='Native1496F0/149AD0/2335B0 frame selection for qualified no-object-service returns. Selected unconverted object/cleanup children count/refuse with empty services, never replay parent. No GP/privatecontrol reconstruction. Native leaf restore/batch also implemented but not reached by production empty-service profile.',remaining=['Actual production Frame model/source/compiler qualification','Actual live nativeGC/no oldowner and full original comparisons','Returned positive object-service collection loops and unmasked SSE exceptions']),indent=2,sort_keys=True)+'\n');print(a.output)
if __name__=='__main__':main()
