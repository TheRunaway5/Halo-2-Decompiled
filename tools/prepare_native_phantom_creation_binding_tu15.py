#!/usr/bin/env python3
"""Freeze native31FFC0/phantom addition on an explicit qualified cohort."""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS=['native_phantom_pair_create_notifications','native_phantom_pair_create_unqualified_owners','native_phantom_pair_create_simple_adds','native_phantom_pair_create_complex_adds']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def change(s,a,b,n=1):assert s.count(a)==n,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);dst=a.output/'staged';pins={};base={};patch=[]
 paths=['src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake']
 root_world=(ROOT/paths[0]).read_text();start=root_world.index('    if(owner.identity==0x31ffc0u){',root_world.index('  pairs.create_pair='));end=root_world.index('    if(owner.identity!=0x320ef0u)',start);branch=root_world[start:end]
 for old,new in [('native_pair_consumer_phantom_notifications', 'native_phantom_pair_create_notifications'), ('native_pair_consumer_phantom_unqualified_owners', 'native_phantom_pair_create_unqualified_owners')]:branch=branch.replace(old,new)
 branch=change(branch,'native_tu15_add_phantom_overlap(m,receiver,other,flag_word,event,current,overlap,observe);return;','native_tu15_add_phantom_overlap(m,receiver,other,flag_word,event,current,overlap,observe);if(stats)++stats->native_phantom_pair_create_complex_adds;return;')
 branch=change(branch,'native_tu15_add_simple_phantom_overlap(m,receiver,other,flag_word,event,current,overlap,observe);return;','native_tu15_add_simple_phantom_overlap(m,receiver,other,flag_word,event,current,overlap,observe);if(stats)++stats->native_phantom_pair_create_simple_adds;return;')
 for n in paths:
  source=a.source_root/n
  if not source.is_file()and n=='src/physics/physics_live_world_tu15.cpp':source=a.source_root/'src/physics_live_world_tu15.cpp'
  old=source.read_text();s=old;base[n]=sha(source)
  if n.endswith('world_integration_residuals.cpp'):
   anchor='#include "halo2/native_tu15_phantom_pair_delete.hpp"';s=change(s,anchor,anchor+'\n#include "halo2/native_tu15_phantom_overlap_add.hpp"')
   anchor='    if(owner.identity!=0x320ef0u)';s=change(s,anchor,branch+anchor)
  elif n.endswith(('world_integration.hpp','physics_frame_residuals.hpp')):
   anchor='  std::uint64_t native_pair_consumer_attempts{}';s=change(s,anchor,'  std::uint64_t '+','.join(f+'{}'for f in FIELDS)+';\n'+anchor)
  elif n.endswith('physics_live_world_tu15.cpp'):
   anchor='    result.native_pair_consumer_create_calls=impl_->native_physics_integrations.native_pair_consumer_create_calls;';s=change(s,anchor,anchor+'\n'+''.join('    result.'+f+'=impl_->native_physics_integrations.'+f+';\n'for f in FIELDS).rstrip())
  elif n.endswith('.cmake'):
   assert 'src/physics/native_tu15_phantom_overlap_add.cpp'not in s;s+='\n# Native phantom addition;31FFC0 shares the sole existing pair-delete unit.\ntarget_sources(halo2_native_pair_consumer PRIVATE src/physics/native_tu15_phantom_overlap_add.cpp)\n'
  else:
   anchor='        {"native_pair_consumer_create_calls",native_frames.native_pair_consumer_create_calls},';s=change(s,anchor,anchor+'\n'+''.join('        {"'+f+'",native_frames.'+f+'},\n'for f in FIELDS).rstrip(),2)
   anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_PHANTOM_PAIR_DELETION")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_PHANTOM_PAIR_CREATION")) {
    const bool phantom=native_frames.native_phantom_pair_create_notifications>0&&
      native_frames.native_phantom_pair_create_notifications==native_frames.native_phantom_pair_create_simple_adds+native_frames.native_phantom_pair_create_complex_adds&&
      native_frames.native_phantom_pair_create_unqualified_owners==0&&native_frames.native_pair_creation_refusals==0&&
      native_frames.native_pair_creation_attempts==native_frames.native_pair_creation_calls&&
      !native_frames.residual_owner_calls.contains(0x31ffc0u)&&!native_frames.residual_owner_calls.contains(0x315300u)&&
      !native_frames.residual_owner_calls.contains(0x315b00u)&&!native_frames.residual_owner_calls.contains(0x314f80u);
    report["native_phantom_pair_creation_gate"]=phantom;passed&=phantom;
  }
''';s=change(s,anchor,gate+anchor)
  target=dst/n;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(s);pins[n]=sha(target);patch.extend(difflib.unified_diff(old.splitlines(True),s.splitlines(True),fromfile='a/'+n,tofile='b/'+n))
 modules=['src/physics/native_tu15_phantom_pair_delete.cpp','include/halo2/native_tu15_phantom_pair_delete.hpp','src/physics/native_tu15_phantom_overlap_add.cpp','include/halo2/native_tu15_phantom_overlap_add.hpp','include/halo2/native_tu15_phantom_creation_activity.hpp']
 before=(a.source_root/modules[0]).read_text();after=(ROOT/modules[0]).read_text();entry='void native_tu15_delete_phantom_pair';assert before[before.index(entry):]==after[after.index(entry):]
 for n in modules:
  f=dst/n;f.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/n,f);pins[n]=sha(f);p=a.source_root/n;base[n]=sha(p)if p.is_file()else None
 for n in ['tests/native_tu15_world_residuals_tests.cpp','tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp']:
  f=dst/n;f.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(a.source_root/n,f);pins[n]=sha(f);base[n]=sha(a.source_root/n)
 (a.output/'phantom-native-binding.diff').write_text(''.join(patch));(a.output/'source-pins.json').write_text(json.dumps(dict(qualified=False,production_applied=False,live_qualified=False,base_source_root=str(a.source_root.resolve()),base_hashes=base,staged_hashes=pins,fields=FIELDS,gate='HALO2_REQUIRE_NATIVE_PHANTOM_PAIR_CREATION',unchanged_existing_delete_function_verified=True,excluded_duplicate_prototype='src/physics/native_tu15_phantom_pair_create.cpp',scope='Sole qualified31FFC0 sharedunit +native315300/315B00/314F80 addition/array/pool. Onlyauthentic2E4D60 added-listener leaf admitted; unknownchild/listener owners count/refuse. Real16BDATA atWorldend-A0, no fabricatedallocation/callbackresults.',remaining=['Freshactualfullnativeparent/child bank+FP proof','CoherentWorld model','Full visible nativephantom/camera/lift/fatal/multiplayer and wider physics gates']),indent=2,sort_keys=True)+'\n');print(a.output)
if __name__=='__main__':main()
