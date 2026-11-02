#!/usr/bin/env python3
"""Stage the native3232E0 World service on an immutable production cohort."""
import argparse,difflib,json,re,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
FIELDS={'attempts':'attempts','calls':'completed','refusals':'refusals','unqualified_factories':'unqualified_factories',
 'body_destructor_refusals':'body_destructor_refusals','base_lock_releases':'base_lock_releases','base_lock_refusals':'base_lock_refusals',
 'source_factories':'managers.factories','constructors':'managers.constructors','collectors':'managers.collectors',
 'registrations':'managers.registrations','unregistrations':'managers.unregistrations','deferred_references':'managers.deferred_references',
 'array_growths':'managers.array_growths','body_destructions':'managers.body_destructions','protected_references':'managers.protected_references',
 'storage_releases':'managers.storage_releases','collector_destructions':'managers.collector_destructions',
 'collector_base_lock_releases':'managers.base_lock_releases'}
PREFIX='native_pair_manager_factory_'
SHARED=['include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp',
 'src/physics/native_tu15_world_integration_residuals.cpp','src/physics/physics_live_world_tu15.cpp',
 'tests/physics_multiplayer_acceptance.cpp','tests/helpers/native_world_convex_factory_prefix_tu15.hpp']
def once(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.source_root=a.source_root.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';stage.mkdir();before={};paths={}
 for relative in SHARED:
  source=a.source_root/relative
  if not source.exists()and relative=='src/physics/physics_live_world_tu15.cpp':source=a.source_root/'src/physics_live_world_tu15.cpp'
  assert source.is_file(),str(source);paths[relative]=str(source);before[relative]=source.read_text();target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
 # Unchanged closure and model helpers are explicit inherited inputs.
 inherited=['cmake/PhysicsLiveWorldTu15.cmake','tests/native_tu15_world_residuals_tests.cpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp']
 for relative in inherited:
  source=a.source_root/relative;target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
 helper='include/halo2/native_tu15_activation_manager_factory_binding.hpp';shutil.copyfile(ROOT/helper,stage/helper)
 relative=SHARED[0];s=once(before[relative],'#include <map>','#include "halo2/native_tu15_activation_manager_factory_binding.hpp"\n#include <map>');s=once(s,'  NativeTu15IslandRegroupBindingActivity island_regroup;','  NativeTu15ActivationManagerFactoryBindingActivity pair_manager_factory;\n  NativeTu15IslandRegroupBindingActivity island_regroup;');(stage/relative).write_text(s)
 relative=SHARED[1];s=once(before[relative],'  std::uint64_t native_contact_manager_base_lock_releases{};','  std::uint64_t '+','.join(PREFIX+n+'{}'for n in FIELDS)+';\n  std::uint64_t native_contact_manager_base_lock_releases{};');(stage/relative).write_text(s)
 relative=SHARED[2];s=before[relative];start=s.index('    create.agents.create_manager=[&]');end=s.index('    create.agents.classify_shape=',start)
 old=s[start:end];new='''    create.agents.create_manager=[&](auto factory,auto target,auto first,auto second,auto input,auto&current){
      (void)input; // Original3232E0 ignores its third formal.
      return native_tu15_create_bound_activation_contact_manager(m,factory,target,first,second,
        current,pair_storage,observe,stats?&stats->pair_manager_factory:nullptr);
    };
''';s=s[:start]+new+s[end:];assert 'call(target.identity,args,factory,12)'not in s;(stage/relative).write_text(s)
 relative=SHARED[3];s=before[relative];anchor='    result.native_contact_manager_base_lock_releases=impl_->native_physics_integrations.native_contact_manager_base_lock_releases;'
 getter='    const auto&pair_manager_factory=impl_->native_physics_integrations.pair_manager_factory;\n'+''.join('    result.'+PREFIX+name+'=pair_manager_factory.'+field+';\n'for name,field in FIELDS.items())
 s=once(s,anchor,getter+anchor);(stage/relative).write_text(s)
 relative=SHARED[4];s=before[relative];anchor='        {"native_contact_manager_base_lock_releases",native_frames.native_contact_manager_base_lock_releases},';assert s.count(anchor)==2
 rows=''.join('        {"'+PREFIX+n+'",native_frames.'+PREFIX+n+'},\n'for n in FIELDS);s=s.replace(anchor,rows+anchor)
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_MANAGER_FACTORY")) {
    const bool manager_factory=native_frames.native_pair_manager_factory_calls>0&&
      native_frames.native_pair_manager_factory_attempts==native_frames.native_pair_manager_factory_calls&&
      native_frames.native_pair_manager_factory_refusals==0&&native_frames.native_pair_manager_factory_unqualified_factories==0&&
      native_frames.native_pair_manager_factory_body_destructor_refusals==0&&native_frames.native_pair_manager_factory_base_lock_refusals==0&&
      native_frames.native_pair_manager_factory_source_factories==native_frames.native_pair_manager_factory_calls&&
      native_frames.native_pair_manager_factory_constructors==native_frames.native_pair_manager_factory_calls&&
      native_frames.native_pair_manager_factory_collectors==native_frames.native_pair_manager_factory_calls&&
      native_frames.native_pair_manager_factory_registrations>0&&!native_frames.residual_owner_calls.contains(0x3232e0u);
    report["native_pair_manager_factory_gate"]=manager_factory;passed&=manager_factory;
  }
''';anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_MERGE")) {';s=once(s,anchor,gate+anchor);(stage/relative).write_text(s)
 relative=SHARED[5];s=before[relative];start=s.index('        }else if(target==0x3232e0u){');end=s.index('        }else throw ',start)
 s=s[:start]+'''        }else if(target==0x3232e0u){
            throw std::runtime_error("Old World manager factory transport must not execute");
'''+s[end:]
 s=once(s,'    catch(const Published&){}','    catch(const Published&){}\n    if(published_agent)published_manager=m.get(published_agent+8u);\n    check(stats.pair_manager_factory.attempts==1&&stats.pair_manager_factory.completed==1&&stats.pair_manager_factory.refusals==0&&stats.pair_manager_factory.managers.registrations==2&&stats.pair_manager_factory.body_destructor_refusals==0&&stats.pair_manager_factory.base_lock_refusals==0,"Native World manager factory activity/lifetime differs");')
 s=once(s,'for(auto target:{0x320ef0u,0x2e4eb0u,0x2f8300u})','for(auto target:{0x320ef0u,0x2e4eb0u,0x2f8300u,0x3232e0u})')
 s=s.replace('"remaining_manager_service_executed_native_kernel",true','"manager_factory_service_fully_native_no_residual",true');(stage/relative).write_text(s)
 patch=''.join(''.join(difflib.unified_diff(before[p].splitlines(True),(stage/p).read_text().splitlines(True),fromfile='a/'+p,tofile='b/'+p))for p in SHARED);(a.output/'delta.patch').write_text(patch)
 pins=dict(qualified=False,working_tree_modified=False,production_applied=False,live_qualified=False,base_source_root=str(a.source_root),base_paths=paths,
  base_hashes={r:sha(Path(paths[r]))for r in SHARED},delta_files=SHARED,new_files=[helper],inherited_files=inherited,
  staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},fields={PREFIX+n:v for n,v in FIELDS.items()},
  direct_builder_proof='analysis/physics/native-activation-manager-factory-binding-tu15-20261102-first/report.json',
  scope='Actual typedWorld320EF0 managercallback nowuses sole native3232E0builder+nativepair_storage. Base-lock missing-registry native; body/registered-lock owners countedrefused, no executor.19Getter/twoJSON/gatefields. Samefactory/mergeprefix/default fixtures preserved; no fullruntime/liveclaim.')
 save(a.output/'source-pins.json',pins);print(dict(prepared=True,delta_files=len(SHARED),flat_fields=len(FIELDS)));return 0
if __name__=='__main__':raise SystemExit(main())
