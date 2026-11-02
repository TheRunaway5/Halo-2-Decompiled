#!/usr/bin/env python3
"""Stage the qualified native manager deferred-array growth domain and gate."""
import argparse,difflib,hashlib,json
from pathlib import Path
FIELD='native_contact_manager_deferred_growths'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def change(s,a,b,n=1):assert s.count(a)==n,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);dst=a.output/'staged';pins={};base={};patch=[]
 names=['src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp']
 for n in names:
  source=a.source_root/n
  if not source.exists()and n=='src/physics/physics_live_world_tu15.cpp':source=a.source_root/'src/physics_live_world_tu15.cpp'
  old=source.read_text();s=old;base[n]=sha(source)
  if n.endswith('world_integration_residuals.cpp'):
   before='''      for(const auto current_world:worlds){
        const auto count=word(m,current_world+0x118u),capacity=word(m,current_world+0x11cu)&0x7fffffffu;
        const auto needed=worlds[0]==worlds[1]?2u:1u;
        if(std::uint64_t(count)+needed>capacity){if(stats)++stats->native_contact_manager_cleanup_refusals;
          throw std::runtime_error("Native TU1.5 contact manager cleanup requires qualified deferred-list capacity");}
      }'''
   after='''      for(const auto current_world:worlds){
        auto count=word(m,current_world+0x118u),capacity=word(m,current_world+0x11cu)&0x7fffffffu;
        const auto needed=worlds[0]==worlds[1]?2u:1u;
        // Predict only the bounded native pool request domain. Actual316DC0
        // rereads/grows/appends at its source phases; this preflight writes no
        // header/pointer/count and supplies no allocation result.
        bool qualified=count<=capacity;
        for(unsigned n=0;qualified&&n<needed;++n){
          if(count==capacity){
            const auto next=count?std::uint64_t(count)*2u:1u;
            qualified=next*4u<=0x2000u;
            if(qualified)capacity=std::uint32_t(next);
          }
          ++count;
        }
        if(!qualified){if(stats)++stats->native_contact_manager_cleanup_refusals;
          throw std::runtime_error("Native TU1.5 contact manager cleanup requires qualified deferred-list growth");}
      }''';s=change(s,before,after)
   s=change(s,'      catch(...){if(stats)++stats->native_contact_manager_cleanup_refusals;throw;}','      catch(...){if(stats){++stats->native_contact_manager_cleanup_refusals;stats->'+FIELD+'+=activity.array_growths;}throw;}')
   s=change(s,'      if(stats){++stats->native_contact_manager_cleanups;','      if(stats){++stats->native_contact_manager_cleanups;\n        stats->'+FIELD+'+=activity.array_growths;')
  elif n.endswith(('world_integration.hpp','physics_frame_residuals.hpp')):
   anchor='  std::uint64_t native_contact_manager_cleanups{}';s=change(s,anchor,'  std::uint64_t '+FIELD+'{};\n'+anchor)
  elif n.endswith('physics_live_world_tu15.cpp'):
   anchor='    result.native_contact_manager_cleanups=impl_->native_physics_integrations.native_contact_manager_cleanups;';s=change(s,anchor,anchor+'\n    result.'+FIELD+'=impl_->native_physics_integrations.'+FIELD+';')
  else:
   anchor='        {"native_contact_manager_cleanups",native_frames.native_contact_manager_cleanups},';s=change(s,anchor,anchor+'\n        {"'+FIELD+'",native_frames.'+FIELD+'},',2)
   anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_DEFERRED_GROWTH")) {
    const bool growth=native_frames.native_contact_manager_deferred_growths>0&&
      native_frames.native_contact_manager_cleanup_refusals==0&&native_frames.native_contact_manager_service_residuals==0&&
      native_frames.native_contact_listener_deferred_references>=native_frames.native_contact_manager_deferred_growths&&
      native_frames.native_contact_listener_unregistrations>=native_frames.native_contact_manager_deferred_growths&&
      !native_frames.residual_owner_calls.contains(0x316dc0u)&&!native_frames.residual_owner_calls.contains(0x2e3250u)&&
      !native_frames.residual_owner_calls.contains(0x2e3270u);
    report["native_contact_manager_deferred_growth_gate"]=growth;passed&=growth;
  }
''';s=change(s,anchor,gate+anchor)
  target=dst/n;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(s);pins[n]=sha(target);patch.extend(difflib.unified_diff(old.splitlines(True),s.splitlines(True),fromfile='a/'+n,tofile='b/'+n))
 (a.output/'native-manager-deferred-growth.diff').write_text(''.join(patch));(a.output/'source-pins.json').write_text(json.dumps(dict(qualified=False,production_applied=False,live_qualified=False,base_source_root=str(a.source_root.resolve()),base_hashes=base,staged_hashes=pins,fields=[FIELD],gate='HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_DEFERRED_GROWTH',scope='Bounded real native316DC0/2E3250/2E3270 growth admission only; preserve actual source-phase allocator/header/body-reference ordering. Source modules unchanged; no capacity preallocation or supplied results.',remaining=['Actual full-manager growth proof hash','Coherent World model/counter transport','Visible full multiplayer fatal-player/queues and all gates']),indent=2,sort_keys=True)+'\n');print(a.output)
if __name__=='__main__':main()
