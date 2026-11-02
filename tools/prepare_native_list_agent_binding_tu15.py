#!/usr/bin/env python3
"""Stage only measured2F8030 with native collection/filter/convex children."""
import argparse,hashlib,json,difflib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS=['native_list_agent_factory_attempts','native_list_agent_factory_calls','native_list_agent_factory_refusals','native_list_agent_count_queries','native_list_agent_filter_queries','native_list_agent_child_factories','native_list_agent_child_returns']
def sha(b):return hashlib.sha256(b).hexdigest()
def change(s,a,b,n=1):assert s.count(a)==n,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output;out.mkdir(exist_ok=False,parents=True);stage=out/'staged';base={};pins={};patch=[]
 source=(ROOT/'src/physics/native_tu15_world_integration_residuals.cpp').read_text();start=source.index('      if(target.identity==0x2f8030u||target.identity==0x2f81f0u){');end=source.index('      if(target.identity==0x2f1cc0u',start);branch=source[start:end]
 branch=change(branch,'if(target.identity==0x2f8030u||target.identity==0x2f81f0u){','if(target.identity==0x2f8030u){\n        if(stats)++stats->native_list_agent_factory_attempts;\n        try{')
 start=branch.index('          if(target.identity==0x2f81f0u){');end=branch.index('          const NativeTu15ListContactArguments arguments{',start);branch=branch[:start]+branch[end:]
 branch=change(branch,'return create.agents.create_agent(child,a,b,context,NativeTu15ActivationContactManager{data},context_fp);','if(stats)++stats->native_list_agent_child_factories;\n          const auto result=create.agents.create_agent(child,a,b,context,NativeTu15ActivationContactManager{data},context_fp);\n          if(stats)++stats->native_list_agent_child_returns;return result;')
 branch=change(branch,'return native_tu15_collection_child_count(m,collection);','if(stats)++stats->native_list_agent_count_queries;\n          return native_tu15_collection_child_count(m,collection);')
 branch=change(branch,'          NativeTu15CollectionFilterServices filter;','          if(stats)++stats->native_list_agent_filter_queries;\n          NativeTu15CollectionFilterServices filter;')
 branch=change(branch,'          --factory_depth;return result;','          --factory_depth;if(stats)++stats->native_list_agent_factory_calls;return result;')
 branch=change(branch,'        }catch(...){--factory_depth;throw;}\n      }','        }catch(...){--factory_depth;throw;}\n        }catch(...){if(stats)++stats->native_list_agent_factory_refusals;throw;}\n      }')
 for rel in ['src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake']:
  path=a.source_root/rel
  if not path.exists()and rel=='src/physics/physics_live_world_tu15.cpp':path=a.source_root/'src/physics_live_world_tu15.cpp'
  raw=path.read_bytes();s=raw.decode();base[rel]=sha(raw)
  if rel.endswith('world_integration_residuals.cpp'):
   needle='#include "halo2/native_tu15_contact_agent_factory_input_observer.hpp"';s=change(s,needle,needle+'\n#include "halo2/native_tu15_list_contact_agent.hpp"\n#include "halo2/native_tu15_collection_filter.hpp"')
   needle='    create.agents.create_agent=[&]';s=change(s,needle,'    unsigned factory_depth{};\n'+needle)
   needle='      if(target.identity!=0x2e4eb0u)';s=change(s,needle,branch+needle)
  elif rel.endswith('native_tu15_world_integration.hpp')or rel.endswith('native_tu15_physics_frame_residuals.hpp'):
   needle='  std::uint64_t native_contact_agent_cached_cleanups{}';at=s.index(needle);s=s[:at]+'  std::uint64_t '+','.join(k+'{}'for k in FIELDS)+';\n'+s[at:]
  elif rel.endswith('physics_live_world_tu15.cpp'):
   needle='    result.native_contact_agent_point_residuals=impl_->native_physics_integrations.native_contact_agent_point_residuals;';s=change(s,needle,needle+'\n'+''.join('    result.'+k+'=impl_->native_physics_integrations.'+k+';\n'for k in FIELDS).rstrip())
  elif rel.endswith('physics_multiplayer_acceptance.cpp'):
   needle='        {"native_contact_agent_point_residuals",native_frames.native_contact_agent_point_residuals},';s=change(s,needle,needle+'\n'+''.join('        {"'+k+'",native_frames.'+k+'},\n'for k in FIELDS).rstrip(),2)
   needle='  if(std::getenv("HALO2_REQUIRE_NATIVE_CONVEX_AGENT_FACTORY")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_LIST_AGENT_FACTORY")) {
    const bool list_passed=native_frames.native_list_agent_factory_calls>0&&
      native_frames.native_list_agent_factory_attempts==native_frames.native_list_agent_factory_calls&&
      native_frames.native_list_agent_factory_refusals==0&&
      native_frames.native_list_agent_count_queries==native_frames.native_list_agent_factory_calls&&
      native_frames.native_list_agent_filter_queries>0&&native_frames.native_list_agent_child_factories>0&&
      native_frames.native_list_agent_child_factories==native_frames.native_list_agent_child_returns&&
      !native_frames.residual_owner_calls.contains(0x2f8030u)&&
      !native_frames.residual_owner_calls.contains(0x2f7d20u)&&
      !native_frames.residual_owner_calls.contains(0x1cbe70u)&&!native_frames.residual_owner_calls.contains(0x3061f0u);
    report["native_list_agent_factory_gate"]=list_passed;passed&=list_passed;
  }
''';s=change(s,needle,gate+needle)
  else:
   needle='add_library(halo2_native_object_velocity STATIC';s=change(s,needle,needle+'\n    src/physics/native_tu15_list_contact_agent.cpp\n    src/physics/native_tu15_collection_filter.cpp')
  dest=stage/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(s);pins[rel]=sha(s.encode());patch.extend(difflib.unified_diff(raw.decode().splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 for rel in ['src/physics/native_tu15_list_contact_agent.cpp','include/halo2/native_tu15_list_contact_agent.hpp','src/physics/native_tu15_collection_filter.cpp','include/halo2/native_tu15_collection_filter.hpp']:
  raw=(ROOT/rel).read_bytes();dest=stage/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw);pins[rel]=sha(raw)
 for rel in ['tests/native_tu15_world_residuals_tests.cpp','tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp']:
  raw=(a.source_root/rel).read_bytes();dest=stage/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw);pins[rel]=sha(raw)
 (out/'native-list-agent-binding.diff').write_text(''.join(patch));(out/'source-pins.json').write_text(json.dumps({'base_source_root':str(a.source_root.resolve()),'base_hashes':base,'staged_hashes':pins,'scope':'Measured2F8030 native constructor and collection count/filter/resolver with recursive native class23 convex children. No reversed/wrapped/BV factory admission. Unknown owners refuse without original CPU fallback.','production_applied':False,'component_current_input_qualified':False,'world_model_qualified':False,'live_qualified':False,'remaining':['Coherent native callback composition/live whole frame','Other native factory families','Unsupported profile/exception domains and full native-only closure']},indent=2)+'\n');print(out)
if __name__=='__main__':main()
