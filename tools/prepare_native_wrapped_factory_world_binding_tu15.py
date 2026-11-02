#!/usr/bin/env python3
"""Private bounded2F1CC0 World composition on an immutable source cohort."""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS={'native_wrapped_factory_attempts':'attempts','native_wrapped_factory_calls':'completed','native_wrapped_factory_refusals':'refusals','native_wrapped_factory_wrapped_entries':'wrapped_calls','native_wrapped_factory_transformed_entries':'transformed_calls','native_wrapped_factory_convex_entries':'convex_calls','native_wrapped_factory_class_queries':'class_queries','native_wrapped_factory_unqualified_owners':'unqualified_owners'}
def sha(raw):return hashlib.sha256(raw).hexdigest()
def change(s,a,b,n=1):assert s.count(a)==n,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);p.add_argument('--proof',type=Path,default=ROOT/'analysis/physics/native-wrapped-factory-actual1472-coherent1473-20261102-seventh/report.json');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';oldpins={};patch=[]
 proof=a.proof.resolve();receipt=json.loads(proof.read_text());assert receipt['passed']and receipt['production_native_only_closure']
 promoted=('src/physics/native_tu15_contact_shape.cpp','include/halo2/native_tu15_contact_shape.hpp')
 for rel,pin in receipt['frozen_source_hashes'].items():
  if rel=='include/halo2/native_tu15_wrapped_factory_binding.hpp'or rel in promoted:continue
  assert sha((a.source_root/rel).read_bytes())==pin,('Qualified dependency differs from frozen1473',rel)
 branch='''      if(target.identity==0x2f1cc0u){
        NativeTu15WrappedFactoryBindingActivity activity;
        const auto collect_wrapped=[&]{if(stats){
'''+''.join('          stats->'+f+'+=activity.'+v+';\n'for f,v in FIELDS.items())+'''        }};
        try{
          // Three closed native constructors retain independent512B DATA
          // slots in [end-F00,end-900). This is above merge DATA's end-F80,
          // below manager/high pair packets, and below no live list parent.
          // No selected dependency in this helper can run a residual CPU.
          if(factory_depth||scratch.bytes<0x2800u){
            ++activity.attempts;++activity.refusals;
            throw std::runtime_error("Native wrapped factory requires qualified standalone retained DATA lifetime");
          }
          const NativeTu15WrappedContactArguments arguments{
            [&]{return first.identity;},[&]{return second.identity;},
            [&]{return settings.identity;},[&]{return manager.identity;}};
          const auto result=native_tu15_create_bound_wrapped_contact_factory(m,target,
            arguments,{std::uint32_t(end)-0xf00u,0x600u},current,state.x87_depth,
            pair_storage,observe,&activity);
          collect_wrapped();return result;
        }catch(...){collect_wrapped();
          try{throw;}catch(const std::exception&error){
            throw std::runtime_error(std::string(error.what())+" outer_factory="+std::to_string(target.identity)+
              " first="+std::to_string(first.identity)+" second="+std::to_string(second.identity)+
              " context="+std::to_string(settings.identity)+" manager="+std::to_string(manager.identity)+
              " depth="+std::to_string(state.x87_depth));
          }catch(...){throw;}
        }
      }
'''
 files=('src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake')
 for rel in files:
  source=a.source_root/rel
  if not source.exists()and rel.endswith('physics_live_world_tu15.cpp'):source=a.source_root/'src/physics_live_world_tu15.cpp'
  raw=source.read_bytes();s=raw.decode();oldpins[rel]=sha(raw)
  if rel.endswith('world_integration_residuals.cpp'):
   anchor='#include "halo2/native_tu15_wrapped_contact_agent.hpp"';s=change(s,anchor,anchor+'\n#include "halo2/native_tu15_wrapped_factory_binding.hpp"')
   anchor='      if(target.identity==0x2f7190u){';s=change(s,anchor,branch+anchor)
  elif rel.endswith(('native_tu15_world_integration.hpp','native_tu15_physics_frame_residuals.hpp')):
   anchor='  std::uint64_t native_transformed_agent_factory_attempts{}';i=s.index(anchor);s=s[:i]+'  std::uint64_t '+','.join(f+'{}'for f in FIELDS)+';\n'+s[i:]
  elif rel.endswith('physics_live_world_tu15.cpp'):
   anchor='    result.native_transformed_agent_factory_refusals=impl_->native_physics_integrations.native_transformed_agent_factory_refusals;';s=change(s,anchor,anchor+'\n'+'\n'.join('    result.'+f+'=impl_->native_physics_integrations.'+f+';'for f in FIELDS))
  elif rel.endswith('physics_multiplayer_acceptance.cpp'):
   anchor='        {"native_transformed_agent_factory_refusals",native_frames.native_transformed_agent_factory_refusals},';s=change(s,anchor,anchor+'\n'+'\n'.join('        {"'+f+'",native_frames.'+f+'},'for f in FIELDS),2)
   anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_CONVEX_AGENT_FACTORY")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_WRAPPED_AGENT_FACTORY")) {
    const bool wrapped=native_frames.native_wrapped_factory_calls>0&&
      native_frames.native_wrapped_factory_attempts==native_frames.native_wrapped_factory_calls&&
      native_frames.native_wrapped_factory_refusals==0&&native_frames.native_wrapped_factory_unqualified_owners==0&&
      native_frames.native_wrapped_factory_wrapped_entries>0&&native_frames.native_wrapped_factory_transformed_entries>0&&
      native_frames.native_wrapped_factory_convex_entries>0&&native_frames.native_wrapped_factory_class_queries>0&&
      native_frames.native_wrapped_factory_calls==native_frames.native_wrapped_factory_wrapped_entries+
        native_frames.native_wrapped_factory_transformed_entries+native_frames.native_wrapped_factory_convex_entries&&
      native_frames.native_pair_creation_refusals==0&&
      !native_frames.residual_owner_calls.contains(0x2f1cc0u)&&!native_frames.residual_owner_calls.contains(0x2f0df0u)&&
      !native_frames.residual_owner_calls.contains(0x2f7190u)&&!native_frames.residual_owner_calls.contains(0x2e44d0u)&&
      !native_frames.residual_owner_calls.contains(0x2e4eb0u)&&!native_frames.residual_owner_calls.contains(0x2f8300u);
    report["native_wrapped_agent_factory_gate"]=wrapped;passed&=wrapped;
  }
''';s=change(s,anchor,gate+anchor)
  else:
   assert 'src/physics/native_tu15_wrapped_contact_agent.cpp'in s
   # The immutable cohort already compiles this qualified unit; no duplicate.
  dst=stage/rel;dst.parent.mkdir(parents=True,exist_ok=True);dst.write_text(s);patch+=difflib.unified_diff(raw.decode().splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)
 for rel in promoted:
  raw=(a.source_root/rel).read_bytes();after=(proof.parent/'source'/rel).read_bytes();assert sha(after)==receipt['frozen_source_hashes'][rel];oldpins[rel]=sha(raw);dst=stage/rel;dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(after);patch+=difflib.unified_diff(raw.decode().splitlines(True),after.decode().splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)
 for rel in ('tests/native_tu15_world_residuals_tests.cpp','tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp'):
  dst=stage/rel;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(a.source_root/rel,dst)
 rel='include/halo2/native_tu15_wrapped_factory_binding.hpp';dst=stage/rel;shutil.copyfile(proof.parent/'source'/rel,dst)
 pins={str(p.relative_to(stage)):sha(p.read_bytes())for p in stage.rglob('*')if p.is_file()};(a.output/'native-wrapped-factory-binding.diff').write_text(''.join(patch));(a.output/'source-pins.json').write_text(json.dumps(dict(base_source_root=str(a.source_root.resolve()),base_hashes=oldpins,staged_hashes=pins,fields=FIELDS,component_proof=str(proof.relative_to(ROOT)),component_proof_sha256=sha(proof.read_bytes()),production_applied=False,live_qualified=False,scope='Only actualqualified2F1CC0→native7190/2E4EB0 standalone closedpipeline; three512BDATA slots disjointfromlive merge/highpair/lifecycle packets. Existing7190/List/Wake/Spatial/Queues source retained;75F0 remainsunadmitted. New8family fields countallrecursiveAPI entries; existingdirectfactory counters retaintheir prior scope. Unsupportedlifetime/depth/owner counts+throws, outerrealtypedtuple retained.'),indent=2,sort_keys=True)+'\n');print(a.output)
if __name__=='__main__':raise SystemExit(main())
