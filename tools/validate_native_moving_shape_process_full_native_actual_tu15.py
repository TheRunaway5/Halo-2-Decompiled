#!/usr/bin/env python3
"""Private whole actual moving-contact composition, native-only providers.

Any unqualified activation/listener/native service fails explicitly. This tool
does not enable a World route or supply an original callback outcome.
"""
import hashlib,inspect,json,sys,re
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_moving_shape_process_solved_actual_tu15 as prefix
import validate_native_moving_shape_process_actual_tu15 as actual
from oracle_smg_firing_1_1 import _Code
ROOT=Path(__file__).resolve().parents[1]
EXTRA=('native_tu15_contact_publication.cpp','native_tu15_contact_distance_publication.cpp','native_tu15_contact_reduction.cpp','native_tu15_contact_point_create.cpp','native_tu15_contact_point_storage.cpp','native_tu15_contact_point_ids.cpp','native_tu15_contact_creation_listeners.cpp','native_tu15_contact_constraint_registration.cpp','native_tu15_contact_collector_info.cpp','native_tu15_pair_creation_queues.cpp','native_tu15_array_reserve.cpp','native_tu15_island_partition_metadata.cpp','native_tu15_contact_body_update.cpp','native_tu15_contact_point_removal.cpp','native_tu15_gameplay_contact_removal.cpp','native_tu15_contact_world_update.cpp','native_tu15_island_constraint_detach.cpp','native_tu15_constraint_reference_release.cpp','native_tu15_part_contact_response.cpp','native_tu15_empty_part_contact.cpp','native_tu15_body_properties.cpp','native_tu15_surface_conveyor.cpp','native_tu15_island_activation.cpp','../core/native_tu15_function_evaluation_extended.cpp','../core/native_tu15_function_curves.cpp','../core/native_tu15_function_control_point.cpp')
def observer_source():
 # The common physical observer appends this fragment inside its helper
 # namespace. Keep production includes at global scope, then resume helpers.
 text=prefix.observer_source();text='}\n#include "halo2/native_tu15_contact_publication_binding.hpp"\n#include "halo2/native_tu15_pool_array_services.hpp"\n#include "halo2/native_tu15_contact_point_binding.hpp"\n#include "halo2/native_tu15_contact_creation_gameplay.hpp"\nnamespace {\n'+text
 text=text.replace('std::array<std::byte,16384>data','std::array<std::byte,32768>data').replace('<=16384;','<=32768;')
 text=text.replace('original.step();unsigned instructions{};','original.step();const auto entry_sp=original.regs().rsp;unsigned instructions{};').replace('original.regs().rip!=0x30cd00u','original.regs().rip!=0x5000000u')
 text=text.replace('J reads=J::array(),stores=J::array(),dispatch=J::array();','J reads=J::array(),stores=J::array(),dispatch=J::array(),zero_repeats=J::array();')
 marker='const auto values=gp(regs);J pending=J::array();'
 assert text.count(marker)==1
 text=text.replace(marker,marker+'if(found->second.value("repeated_string",false)&&std::uint32_t(regs.rcx)==0u){zero_repeats.push_back(pc);original.step();continue;}')
 old='const auto entry=original.regs();std::array<std::uint32_t,7>args;for(unsigned k=0;k<7;++k)args[k]=original.word(std::uint32_t(entry.rsp)+4u+k*4u);const auto expected_fp='
 new='check(original.regs().rsp==entry_sp+20u,"Original full PROCESS RET16 differs");const auto expected_fp='
 assert text.count(old)==1;text=text.replace(old,new)
 begin=text.index('    child.publish_contact=');end=text.index('    native_tu15_process_convex_contact_agent',begin)
 native='''    NativeTu15PairDeletionNativeScope scope;NativeTu15ContactPublicationNativeServices providers;
    providers.creation.allocation=[](auto&current){return native_tu15_make_pool_array_services(current);};
    providers.creation.activate_island=[](auto&,auto,auto,auto&){throw std::runtime_error("Whole moving PROCESS requires qualified native activation316D40");};
    providers.creation.removal=[](auto&)->NativeTu15ContactPointRemovalServices{throw std::runtime_error("Whole moving PROCESS requires qualified rejected-point removal");};
    providers.creation.notify_listener=[&](auto&current,auto listener,auto owner,auto event,auto&f){native_tu15_dispatch_contact_creation_gameplay_listener(current,listener,owner,event,{p+0x6000u,p+0x6004u,p+0x6008u,p+0x6010u,p+0x6020u,p+0x6030u,p+0x6034u,p+0x6038u},f,0);};
    providers.remove_point=[&](auto&current,auto manager,auto id,auto&f){auto allocation=native_tu15_make_pool_array_services(current);native_tu15_remove_bound_contact_point(current,manager,id,{p+0x5200u,p+0x5220u,p+0x5240u,p+0x5220u},f,0,allocation);};
    auto publication=native_tu15_make_contact_publication_services(view,{{p+0x5000u},{p+0x5100u,p+0x5120u,p+0x5140u}},0,providers,scope);
    child.publish_contact=[&](auto a,auto b,auto settings,auto packet,auto manager,auto records,auto count,auto&f){native_tu15_publish_contact(view,a,b,settings,packet,manager,records,count,{p+0x4000u},f,0,publication);};
    child.publish_distance=[&](auto a,auto b,auto settings,auto cutoff,auto records,auto count,auto destination,auto manager,auto&f){native_tu15_publish_contact_distances(view,a,b,settings,cutoff,records,count,destination,manager,f,0,publication.removal);};
    child.cleanup.remove_point=publication.removal.remove_point;
'''
 text=text[:begin]+native+text[end:]
 old='check(false,"Native solved PROCESS unexpectedly returned");'
 assert text.count(old)==1;text=text.replace(old,'boundary_seen=true;')
 text=text.replace('activity.refusals==1&&convex_activity.simplex_calls','activity.completed==1&&!activity.refusals&&convex_activity.completed==1&&convex_activity.simplex_calls')
 text=text.replace('{"next_owner",0x30cd00u},{"source_publication_formals",args},','{"native_full_return",true},{"zero_repeat_instructions",zero_repeats},')
 return text
def main():
 units=tuple(dict.fromkeys(prefix.UNITS+EXTRA+('native_tu15_constraint_body_slots.cpp','native_tu15_constraint_scalar_cache.cpp','native_tu15_contact_creation_gameplay.cpp','native_tu15_activation_payload_filter.cpp','native_tu15_activation_agents.cpp')))
 pending=[ROOT/'src/physics'/n for n in units]+[ROOT/'include/halo2'/n for n in ('native_tu15_contact_publication_binding.hpp','native_tu15_pool_array_services.hpp','native_tu15_contact_point_binding.hpp','native_tu15_moving_shape_process.hpp','native_tu15_convex_simplex_solver_binding.hpp')]+[Path(__file__),ROOT/'tools/observe_native_moving_shape_process_solved_actual_tu15.cpp']
 pins={}
 while pending:
  path=pending.pop().resolve();name=str(path.relative_to(ROOT))
  if name in pins:continue
  pins[name]=hashlib.sha256(path.read_bytes()).hexdigest()
  for include in re.findall(r'^#include "([^"]+)"',path.read_text(),re.M):
   child=ROOT/'include'/include if include.startswith('halo2/')else path.parent/include
   if child.is_file():pending.append(child)
 saved=actual.parent.metadata
 graph=json.loads((ROOT/'analysis/physics/native-moving-shape-process-full-original1479-20261102-second/g++-actual.json').read_text());pcs={p['instruction']for profile in graph['profiles']for p in profile['executed_unique_solver_sites']}
 def complete_metadata(image,output):
  r=saved(image,output);code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};known={s['instruction']for s in r['sites']}
  for pc in sorted(pcs-known):
   i=code.at(pc);ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea'or i.mnemonic.startswith('prefetch'):continue
    if op.mem.segment:
     segment=i.reg_name(op.mem.segment);assert segment in('fs','ds','es'),(hex(pc),segment)
     if segment=='fs':continue
    access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else CS_AC_READ if i.mnemonic in('test','cmp')else op.access
    assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),i.mnemonic,i.op_str)
    ops.append(dict(base=registers.get(i.reg_name(op.mem.base),-1),index=registers.get(i.reg_name(op.mem.index),-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   r['sites'].append(dict(instruction=pc,operands=ops,repeated_string=i.mnemonic.startswith('rep ')))
  return r
 text=inspect.getsource(prefix.main)
 begin=text.index(' original_run=subprocess.run;');end=text.index(' parent=actual.parent;',begin);text=text[:begin]+text[end:]
 text=text.replace('r=build(image,output)','r=complete_metadata_builder(build,image,output)')
 text=text.replace("'include/halo2/native_tu15_convex_simplex_solver_binding.hpp'","'include/halo2/native_tu15_convex_simplex_solver_binding.hpp','tools/validate_native_moving_shape_process_full_native_actual_tu15.py','include/halo2/native_tu15_contact_publication_binding.hpp','include/halo2/native_tu15_pool_array_services.hpp','include/halo2/native_tu15_contact_point_binding.hpp'")
 text=text.replace(" finally:subprocess.run=original_run"," finally:pass").replace("r['mutants']=mutants","r['private_whole_native_attempt']=True")
 text=text.replace('r[\'next_actual_owner\']=0x30cd00;','')
 def complete_metadata_builder(build,image,output):
  nonlocal saved;saved=build;return complete_metadata(image,output)
 env=dict(prefix.__dict__,UNITS=units,observer_source=observer_source,complete_metadata_builder=complete_metadata_builder,__doc__=__doc__);exec(text,env)
 try:return env['main']()
 finally:
  p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if p.exists():
   r=json.loads(p.read_text());unchanged=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==s for n,s in pins.items());r['input_source_sha256']=pins;r['input_sources_unchanged']=unchanged
   if not unchanged:r['passed']=False;r['source_cohort_error']='Source inputs changed during private proof'
   p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
   if not unchanged:raise RuntimeError('Source inputs changed during private proof')
if __name__=='__main__':raise SystemExit(main())
