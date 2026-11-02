#!/usr/bin/env python3
"""Narrow native query/Camera suffix integration over exact frozen1477 source."""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS={'attempts':'attempts','calls':'completed','refusals':'refusals','resource_refusals':'resource_refusals','unknown_owners':'unknown_owners()','bsp_calls':'query.bsp_calls','bsp_hits':'query.bsp_hits','cluster_visits':'query.cluster_visits','instanced_attempts':'instanced.attempts','instanced_calls':'instanced.completed','instanced_refusals':'instanced.refusals','mopp_calls':'instanced.traversal.completed','mopp_opcodes':'instanced.traversal.opcodes','mopp_leaves':'instanced.traversal.leaves','primitive_calls':'instanced.instance.completed','primitive_hits':'instanced.instance.hits','dynamic_calls':'dynamic.completed','dynamic_ray_calls':'dynamic.ray_calls','unprovided_reads':'instanced.scene.setup.unprovided_reads','unprovided_writes':'instanced.scene.setup.unprovided_writes'}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--base',type=Path,required=True);p.add_argument('--stage',type=Path,required=True);a=p.parse_args();base=a.base.resolve();stage=a.stage.resolve();stage.mkdir(parents=True,exist_ok=False);staged=stage/'staged';manifest=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-main-heap-look-original-v1477-20261102/required-source-layer/source-manifest.json';expected=json.loads(manifest.read_text())['source_files_sha256'];base_hashes={};hashes={};sources={};deltas=[]
 def read(name):
  q=base/name
  if not q.exists()and name.startswith('src/physics/'):q=base/'src'/Path(name).name
  key=str(q.relative_to(base));assert sha(q)==expected[key],name;base_hashes[name]=sha(q);sources[name]=q;return q.read_text()
 def write(name,s):q=staged/name;q.parent.mkdir(parents=True,exist_ok=True);q.write_text(s);hashes[name]=sha(q);deltas.append(name)
 header='include/halo2/native_tu15_look_pipeline_binding.hpp';s=read(header);needle='#include "halo2/native_tu15_camera_position.hpp"';s=s.replace(needle,needle+'\n#include "halo2/native_tu15_camera_query_binding.hpp"');s=s.replace('    NativeTu15CameraPositionScratch camera;','    NativeTu15CameraPositionScratch camera;\n    NativeTu15CameraQueryBindingScratch query;');s=s.replace('    NativeTu15LookVectorAngleStats angles;','    NativeTu15LookVectorAngleStats angles;\n    NativeTu15CameraQueryBindingActivity scene_queries;\n    std::uint64_t unsupported_fp_domains{};')
 needle='        NativeTu15CameraPositionServices camera;';assert s.count(needle)==1;s=s.replace(needle,'''        if(fields.control_word!=0x27fu){++out.unsupported_fp_domains;
            throw std::runtime_error("Native look pipeline requires its qualified default Host FP domain");}
'''+needle)
 needle='''            ++out.query_refusals;
            throw std::runtime_error("Native look pipeline requires16C620 geometric query");''';assert s.count(needle)==1;s=s.replace(needle,'''            try{return native_tu15_query_bound_camera_scene(memory,
                {call.mask,call.origin,call.displacement,call.ignore_first,call.ignore_second,call.result},
                scratch.query,current,fields.preserved_x87_depth,observe,&out.scene_queries);
            }catch(...){++out.query_refusals;throw;}''');write(header,s)
 world='src/physics/physics_live_world_tu15.cpp';s=read(world);needle='{{0x6005d000u},{0x6005d100u,{0x6005d200u}}},';assert s.count(needle)==1;s=s.replace(needle,'{{0x6005d000u},{0x6005d100u,{0x6005d200u}},{0x600a0000u}},');needle='    const auto&look_initialization=impl_->native_look_initialization;';assert s.count(needle)==1;getter='    const auto&camera_query=look_pipeline.scene_queries;\n'+''.join('    result.native_camera_query_'+k+'=camera_query.'+v+';\n'for k,v in FIELDS.items())+'    result.native_look_update_fp_refusals=look_pipeline.unsupported_fp_domains;\n';s=s.replace(needle,getter+needle);write(world,s)
 h='include/halo2/native_tu15_physics_frame_residuals.hpp';s=read(h);needle='  std::uint64_t native_look_update_attempts';at=s.index(needle);s=s[:at]+'  std::uint64_t '+','.join('native_camera_query_'+k+'{}'for k in FIELDS)+';\n  std::uint64_t native_look_update_fp_refusals{};\n'+s[at:];write(h,s)
 accept='tests/physics_multiplayer_acceptance.cpp';s=read(accept);needle='        {"native_look_update_attempts",native_frames.native_look_update_attempts},';assert s.count(needle)==2;s=s.replace(needle,''.join('        {"native_camera_query_'+k+'",native_frames.native_camera_query_'+k+'},\n'for k in FIELDS)+'        {"native_look_update_fp_refusals",native_frames.native_look_update_fp_refusals},\n'+needle)
 needle='''      native_frames.native_look_update_marker_observer_refusals==0;''';assert s.count(needle)==1;s=s.replace(needle,'''      native_frames.native_look_update_marker_observer_refusals==0&&native_frames.native_look_update_fp_refusals==0;''')
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_CAMERA_QUERY")) {
    const bool native_query=native_frames.native_camera_query_calls>0&&
      native_frames.native_camera_query_attempts==native_frames.native_camera_query_calls&&
      native_frames.native_camera_query_refusals==0&&native_frames.native_camera_query_resource_refusals==0&&
      native_frames.native_camera_query_unknown_owners==0&&native_frames.native_camera_query_instanced_refusals==0&&
      native_frames.native_camera_query_unprovided_reads==0&&native_frames.native_camera_query_unprovided_writes==0&&
      native_frames.native_look_update_query_refusals==0&&native_frames.native_look_update_fp_refusals==0;
    report["native_camera_query_gate"]=native_query;passed&=native_query;
  }
''';needle='  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_CONTROLLER")) {';assert s.count(needle)==1;s=s.replace(needle,gate+needle);write(accept,s)
 # Copy only qualified new source/headers; existing source owners are reused.
 refs=['native-camera-geometric-query-full-actual-qualified-20261102','native-camera-instanced-query-qualified-20261102'];copied=[]
 for ref in refs:
  pins=json.loads((ROOT/'analysis/physics'/ref/'source-pins.json').read_text())
  for name,digest in pins['source_hashes'].items():
   if not name.startswith(('src/','include/')):continue
   target=base/name
   if target.exists():assert sha(target)==digest,(name,'existingowner');continue
   src=ROOT/'analysis/physics'/ref/'source'/name;assert sha(src)==digest
   dst=staged/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst);hashes[name]=sha(dst);copied.append(name)
 for ref in('native-mopp-private-setup-stage-20261102','native-instanced-ray-host-callback-stage-20261102','native-camera-query-binding-stage-20261102','native-camera-position-query-suffix-stage-20261102'):
  path=ROOT/'analysis/physics'/ref;pins=json.loads((path/'source-pins.json').read_text());assert pins['qualified']
  for name,digest in pins['staged_hashes'].items():
   target=base/name
   if target.exists()and name not in base_hashes:base_hashes[name]=sha(target);sources[name]=target;deltas.append(name)
   src=path/'staged'/name;assert sha(src)==digest;dst=staged/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst);hashes[name]=sha(dst);copied.append(name)
 for name in('include/halo2/native_tu15_instanced_scene_ray_binding.hpp',):
  q=ROOT/name;dst=staged/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(q,dst);hashes[name]=sha(dst);copied.append(name)
 for name,ref in(('include/halo2/native_tu15_mopp_ray_setup.hpp','native-mopp-ray-setup-qualified-prefix-20261102'),('include/halo2/native_tu15_instanced_ray_callbacks.hpp','native-instanced-ray-callbacks-qualified-prefix-20261102')):
  proof=json.loads((ROOT/'analysis/physics'/ref/'source-pins.json').read_text());digest=proof['source_hashes'][name];q=ROOT/name;assert sha(q)==digest
  dst=staged/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(q,dst);hashes[name]=sha(dst);copied.append(name)
 cmake='cmake/PhysicsLiveWorldTu15.cmake';s=read(cmake);new=[n for n in copied if n.endswith('.cpp')and n not in s];assert len(new)==len(set(new));s+='\n# Source-authenticated native camera scene query, sole existing inverse/island owners reused.\ntarget_sources(halo2_native_pair_consumer PRIVATE\n'+''.join('    '+n+'\n'for n in new)+')\nif(MSVC)\n    set_source_files_properties(\n'+''.join('        '+n+'\n'for n in new)+'        PROPERTIES COMPILE_OPTIONS /fp:strict)\nelse()\n    set_source_files_properties(\n'+''.join('        '+n+'\n'for n in new)+'        PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")\nendif()\n';write(cmake,s)
 inherited=[]
 for name in('tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp'):
  src=base/name;assert sha(src)==expected[name];dst=staged/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst);hashes[name]=sha(dst);inherited.append(name)
 patch=''
 for name in dict.fromkeys(deltas):patch+=''.join(difflib.unified_diff(sources[name].read_text().splitlines(keepends=True),(staged/name).read_text().splitlines(keepends=True),fromfile='frozen1477/'+name,tofile='nativeQuery/'+name))
 (stage/'delta.patch').write_text(patch)
 pins=dict(qualified=False,root_reviewed=False,peer_review_pending=True,live_qualified=False,base_source_root=str(base),base_source_files={n:str(p)for n,p in sources.items()},base_hashes=base_hashes,staged_hashes=hashes,delta_files=list(dict.fromkeys(deltas)),inherited_files=inherited,fields=FIELDS,additional_field='native_look_update_fp_refusals',host_data=dict(query='[600A0000,600B0000)',parent='[6005D000,6005D228)',retained='begins60070000',descending_control='near600F0000',owner='Existing1MiB ownedHostmemory; semanticDATA only, notGP/controlmetadata'),scope='SelectedfreshHost27F/emptyx87/read-onlyobserver nativeLook→nativecamera/query, typedinputROhooks preserved; soleinverse/island/mathlibraries reused. Queryunknownowners countthrow, noCPUfallback.',remaining=['CoherentWorld/Hostcounter/gate/exception models andindependentpeer','Freshvisiblefull3multiplayer/importedMCC/actualvehiclecamera/strict','AlternateFP/secondhit camera component domains remainexplicit; canonicalRoot andallotherphysicsparents open'])
 (stage/'source-pins.json').write_text(json.dumps(pins,indent=2,sort_keys=True)+'\n');print(sha(stage/'source-pins.json'),len(hashes),len(new));return 0
if __name__=='__main__':raise SystemExit(main())
