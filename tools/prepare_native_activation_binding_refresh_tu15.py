#!/usr/bin/env python3
"""Prepare an explicit native predicate/manager binding delta without applying it."""
import argparse,difflib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save

def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Binding source changed at '+needle[:100])
 return text.replace(needle,replacement,1)

def refreshed(source,header):
 header=once(header,'#include "halo2/native_tu15_activation_services.hpp"',
  '#include "halo2/native_tu15_activation_services.hpp"\n#include "halo2/native_tu15_activation_payload_filter.hpp"\n#include "halo2/native_tu15_activation_contact_manager.hpp"')
 header=once(header,'native_policy_type_getters{},native_array_growths{},',
  'native_policy_type_getters{},native_manager_factories{},unconverted_manager_factories{},native_array_growths{},')
 header=once(header,'    NativeTu15ActivationServiceStats services;',
  '    NativeTu15ActivationServiceStats services;\n    NativeTu15ActivationPayloadFilterStats payload_filter;\n    NativeTu15ActivationContactManagerStats managers;')
 needle='    agents.create_manager=[&](auto factory,auto owner,auto first,auto second,auto context,auto&state){\n'
 source=once(source,needle,needle+'''        if(owner.identity==0x3232e0u){
            if(activity)++activity->native_manager_factories;
            NativeTu15ActivationContactManagerServices services;services.allocation=allocation;
            services.destroy_body=[&](auto body,auto target,auto disposing,auto&fp){
                const std::array arguments{disposing};call(target.identity,arguments,body.identity,4,fp);
            };
            services.release_base_lock=[&](auto object,auto flags,auto&fp){
                const std::array arguments{flags};call(0x2e12a0u,arguments,object,4,fp);
            };
            return native_tu15_create_activation_contact_manager(memory,factory,first,second,state,
                services,observer,activity?&activity->managers:nullptr);
        }
        if(activity)++activity->unconverted_manager_factories;
''')
 needle='            services.test_filter=[&](auto filter,auto target,auto a,auto b,auto&context){\n'
 source=once(source,needle,needle+'''                if(target.identity==0x281f10u){
                    NativeTu15ActivationPayloadFilterServices query;
                    query.motion_kind=[&](auto motion,auto owner,auto&fp){
                        if(activity)++activity->unconverted_motion_type_getters;
                        return call(owner.identity,{},motion,0,fp);
                    };
                    return native_tu15_activation_payload_filter(memory,a,b,context,query,
                        activity?&activity->payload_filter:nullptr);
                }
''')
 return source,header

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 paths=[Path('src/physics/native_tu15_object_activation_residuals.cpp'),Path('include/halo2/native_tu15_object_activation_residuals.hpp')];before=[(ROOT/path).read_text()for path in paths];after=refreshed(*before);patch=[]
 for path,old,new in zip(paths,before,after):
  target=a.output/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new);patch.extend(difflib.unified_diff(old.splitlines(keepends=True),new.splitlines(keepends=True),fromfile='a/'+str(path),tofile='b/'+str(path)))
 delta=a.output/'activation-native-binding.diff';delta.write_text(''.join(patch));units=['object_motion','island_activation','activation_agents','activation_services','contact_agent_factory','array_allocator','array_growth','object_activation_residuals','object_velocity_transport','object_velocity_handoff','activation_payload_filter','activation_contact_manager'];files=[ROOT/f'src/physics/native_tu15_{name}.cpp'for name in units]+[ROOT/f'include/halo2/native_tu15_{name}.hpp'for name in units]+[ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',Path(__file__)]
 save(a.output/'source-pins.json',dict(schema='native-tu15-activation-binding-source-delta-v1',working_tree_modified=False,qualified=False,source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},staged_hashes={str(path):sha(a.output/'staged'/path)for path in paths},patch=dict(path=str(delta),sha256=sha(delta)),required_new_compile_units=['src/physics/native_tu15_activation_payload_filter.cpp','src/physics/native_tu15_activation_contact_manager.cpp'],owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]));print(delta);return 0
if __name__=='__main__':raise SystemExit(main())
