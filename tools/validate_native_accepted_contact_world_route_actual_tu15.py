#!/usr/bin/env python3
"""Run the genuine accepted event through exact frozen production dispatch and borrowed memory."""
import inspect,json,sys
from pathlib import Path
import validate_native_accepted_contact_moving_resource_v4_actual_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
WORLD=ROOT/'analysis/physics/native-accepted-body-contact-world-cohort1482-v4-20261103'

def main():
 assert sys.argv[1]=='--coherent-source-root';source=Path(sys.argv[2]);del sys.argv[1:3]
 sha=parent.sha;assert sha(WORLD/'source-manifest.json')=='92b0a5951bc9a1810cd2ccfe4f9f60ee390543c4a2274f9ca9ac35d8c21a2757'
 pins=json.loads((WORLD/'source-manifest.json').read_text())['source_files_sha256']
 # No substitution of semantic code: copy selected exact World source bytes and
 # borrow the same actual EventMemory class and both unchanged callback fragments.
 def exact(n):
  p=source/n;assert sha(p)==pins[n],n;return p.read_text()
 event='src/physics/native_tu15_contact_point_create.cpp'
 world=exact('src/physics/native_tu15_world_integration_residuals.cpp')
 i=world.index('    native.gameplay.body_contact=')
 w=world[i:world.index('    native.activate_island=',i)]
 moving=exact('include/halo2/native_tu15_moving_shape_process_binding.hpp')
 i=moving.index('            publication_native.creation.notify_listener=')
 n=moving[i:moving.index('            publication_native.remove_point=',i)]
 assert 'view.require_retained_extent(listener,4u)' in n and 'native_tu15_dispatch_contact_creation_gameplay_listener' in n
 def driver(text,layout):
  text=parent.relocated_driver(text,layout)
  text='#include "halo2/native_tu15_moving_shape_process_binding.hpp"\n#include "halo2/native_tu15_accepted_body_contact_world_resources.hpp"\n#include "native_tu15_contact_point_create.cpp"\n'+text
  def replace(before,after):
   nonlocal text
   assert text.count(before)==1,(before,text.count(before))
   text=text.replace(before,after,1)
  replace('const std::array<NativeTu15ContactPrivateRange,1>owned{{{p,0x8000u}}};NativeTu15ContactCandidatePrivateData view(memory,owned,{});','const std::array<NativeTu15ContactPrivateRange,1>owned{{{p,0x8000u}}};NativeTu15ContactCandidatePrivateData private_data(memory,owned,{});NativeTu15ConvexSimplexMemory view(private_data,protected_observer);')
  replace('view.write(p+0x5030u,id);view.write(p+0x5034u,fields);','EventMemory event_view(view,p+0x5030u,nullptr);event_view.write(p+0x5030u,id);event_view.write(p+0x5034u,fields);')
  replace('   NativeTu15AcceptedBodyContactBindingActivity binding;auto&parent=binding.listener;', '   struct LocalStats {NativeTu15AcceptedBodyContactWorldActivity accepted_body_contact;} local_stats;auto*stats=&local_stats;auto&binding=stats->accepted_body_contact.helper;auto&parent=binding.listener;')
  a=text.index('   try{native_tu15_notify_bound_accepted_body_contact(view,typed.at("listener"),p+0x5030u,')
  b=text.index('   memory.track=false;',a)
  text=text[:a]+'''   {NativeTu15MovingShapeNativeServices native;NativeTu15MovingShapeBindingActivity out;
   NativeTu15ContactPublicationNativeServices publication_native;
   auto&storage=memory;const auto data=p;unsigned depth=0;auto&pair_deletion_scope=scope;
   const auto&moving_observer=protected_observer;
'''+w+n+'''   try{
    NativeTu15PairDeletionNativeScopeGuard current_moving_scope(scope,&out.native_only);
    publication_native.creation.notify_listener(event_view,typed.at("listener").get<std::uint32_t>(),
        NativeTu15ContactVirtualOwner{0x281710u},p+0x5030u,fp);
   }catch(const DataFault&){failed=true;}
   check(out.gameplay.body_calls==1u&&out.gameplay.completed==unsigned(!failed)&&
     out.gameplay.refusals==unsigned(failed)&&!out.gameplay.body_rejected_early_outs&&
     out.gameplay.unqualified_owners.empty()&&out.native_only.owners.empty(),
     "Existing gameplay counters differ at exact route");
   check(stats->accepted_body_contact.attempts==1u&&stats->accepted_body_contact.completed==unsigned(!failed)&&
     stats->accepted_body_contact.refusals==unsigned(failed)&&!stats->accepted_body_contact.resource_refusals,
     "Exact World adapter counters differ");}
'''+text[b:]
  # The generated alias 'view' keeps moving's actual Simplex provider.
  # Direct guard controls intentionally test the underlying private DATA object.
  a=text.index('   if(mutation.empty()&&!failed){unsigned guards{};')
  b=text.index('    report["definition_guard_controls"]=guards;',a)
  text=text[:a]+text[a:b].replace('view.','private_data.').replace('nested(view,','nested(private_data,')+text[b:]
  return text
 origin=inspect.getsource(parent.main)
 assert origin.count("project=owner/'source'")==1;origin=origin.replace("project=owner/'source'","project=owner/'route-model-source'",1)
 # Bind the helper set and direct production files to the coherent hash table,
 # which retains the unrelated producer/records/data-array headers from f053.
 marker="        (project/'driver.cpp').write_text(driver);"
 assert origin.count(marker)==1
 origin=origin.replace(marker,'''        original_component_pins=pins.copy()
        for n in set(pins)|set(ROUTE_INPUTS):
            q=project/n;q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(COHERENT_SOURCE/n,q);assert sha(q)==COHERENT_PINS[n],n
            pins[n]=COHERENT_PINS[n]
        # Moving includes several retained helper definitions not selected by
        # this event; use the complete frozen headers and private includes.
        shutil.copytree(COHERENT_SOURCE/'include',project/'include',dirs_exist_ok=True)
        for q in (COHERENT_SOURCE/'src/physics').glob('*.hpp'):
            shutil.copyfile(q,project/'src/physics'/q.name)
'''+marker,1)
 origin=origin.replace("units=sorted(n for n in pins if n.endswith('.cpp'));assert len(units)==16", "units=sorted(n for n in pins if n.endswith('.cpp') and n!=EVENT_CPP);assert len(units)==20")
 origin=origin.replace("for n,h in pins.items():assert sha(project/n)==h and sha(frozen/n)==h,n","for n,h in pins.items():assert sha(project/n)==h and sha(COHERENT_SOURCE/n)==h,n\n        for n,h in original_component_pins.items():assert sha(frozen/n)==h,n")
 origin=origin.replace('Exact V4 component resource relocation only. No World admission or arbitrary-role alias claim.', 'Genuine V4 event through exact World callback, moving notifier, production gameplay dispatch and EventMemory. Native parent+current borrowed view, not full moving/world invocation.')
 origin=origin.replace('v4_role_checks_active=True','v4_role_checks_active=True,exact_world_dispatch=True,exact_event_memory=True,coherent_manifest_sha256=COHERENT_MANIFEST,existing_gameplay_counters_verified=True')
 env=dict(parent.__dict__,relocated_driver=driver,COHERENT_SOURCE=source,COHERENT_PINS=pins,
  COHERENT_MANIFEST=sha(WORLD/'source-manifest.json'),EVENT_CPP=event,ROUTE_INPUTS=[
  event,'src/physics/native_tu15_contact_creation_gameplay.cpp','src/physics/native_tu15_activation_agents.cpp',
  'src/physics/native_tu15_activation_payload_filter.cpp','src/physics/native_tu15_convex_simplex_single.cpp'])
 exec(origin,env);env['main']()

if __name__=='__main__':main()
