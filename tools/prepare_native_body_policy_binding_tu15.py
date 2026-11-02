#!/usr/bin/env python3
"""Stage a native body-policy binding delta after the qualified manager delta."""
import argparse,difflib,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
from prepare_native_activation_binding_refresh_tu15 import once

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--base',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.base=a.base.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 cpp=Path('src/physics/native_tu15_object_activation_residuals.cpp');hpp=Path('include/halo2/native_tu15_object_activation_residuals.hpp');transport=Path('src/physics/native_tu15_object_velocity_transport.cpp');paths=[cpp,hpp,transport];before=[(a.base/cpp).read_text(),(a.base/hpp).read_text(),(ROOT/transport).read_text()];source,header,adapter=before
 header=once(header,'#include "halo2/native_tu15_activation_contact_manager.hpp"','#include "halo2/native_tu15_activation_contact_manager.hpp"\n#include "halo2/native_tu15_body_policy.hpp"')
 header=once(header,'    std::uint32_t filter_result{},discrete_result{};','    std::uint32_t filter_result{},discrete_result{},policy_bounds{},shape_scratch_first{},shape_scratch_bytes{};')
 header=once(header,'    NativeTu15ActivationContactManagerStats managers;','    NativeTu15ActivationContactManagerStats managers;\n    NativeTu15BodyPolicyStats body_policy;\n    std::map<std::uint32_t,std::uint64_t> native_policy_bounds_owners,unconverted_policy_bounds_owners;')
 source=once(source,'#include "halo2/native_tu15_contact_agent_factory.hpp"','#include "halo2/native_tu15_contact_agent_factory.hpp"\n#include "halo2/native_tu15_centered_shape_bounds.hpp"\n#include "halo2/native_tu15_compound_shape_bounds.hpp"\n#include "halo2/native_tu15_sphere_shape_bounds.hpp"')
 needle='    if(activity)++activity->setters;'
 source=once(source,needle,'''    require(scratch.policy_bounds&&!(scratch.policy_bounds&15u)&&scratch.shape_scratch_first&&
        !(scratch.shape_scratch_first&15u)&&scratch.shape_scratch_bytes>=64u&&
        !(scratch.shape_scratch_bytes&15u)&&std::uint64_t(scratch.shape_scratch_first)+scratch.shape_scratch_bytes<=0xffffffffu,
        "Native TU1.5 object activation requires declared aligned policy/shape DATA storage");
'''+needle)
 needle='    NativeTu15ActivationAgentServices agents;'
 source=once(source,needle,'''    NativeTu15BodyPolicyServices body_policy;body_policy.allocation=allocation;
    auto shape_cursor=scratch.shape_scratch_first+scratch.shape_scratch_bytes;
    std::function<void(NativeTu15BoundsShape,std::uint32_t,NativeTu15BoundsTransform,
        std::uint32_t,NativeTu15BoundsOutput,NativeTu15PhysicsFrameFp&)>shape_bounds;
    shape_bounds=[&](auto shape,std::uint32_t owner,auto transform,std::uint32_t margin,auto output,auto&state){
        const bool native=owner==0x306760u||owner==0x307e30u||owner==0x2e8d90u||owner==0x2e8120u||
            owner==0x306170u||owner==0x2e8870u||owner==0x2e9600u||owner==0x2e5e80u;
        if(native&&activity)++activity->native_policy_bounds_owners[owner];
        if(owner==0x306760u)native_tu15_convex_box_bounds(memory,shape,transform,margin,output,state,depth,observer);
        else if(owner==0x307e30u)native_tu15_centered_shape_bounds(memory,shape,transform,margin,output,state,depth,observer);
        else if(owner==0x2e8d90u)native_tu15_sphere_shape_bounds(memory,shape,transform,margin,output,state,depth,observer);
        else if(owner==0x2e8120u)native_tu15_endpoint_shape_bounds(memory,shape,transform,margin,output,state,depth,observer);
        else if(owner==0x306170u||owner==0x2e8870u){
            require(shape_cursor>=scratch.shape_scratch_first+64u,
                "Native TU1.5 activation bounds exhausted declared nested DATA storage");
            const auto previous=shape_cursor;shape_cursor-=64u;
            try{
                if(owner==0x306170u)native_tu15_compound_shape_bounds(memory,shape,transform,margin,output,
                    {shape_cursor},state,body_policy.bounds,observer);
                else native_tu15_transformed_shape_bounds(memory,shape,transform,[margin]{return margin;},
                    [output]{return output;},{shape_cursor},state,body_policy.bounds,observer);
            }catch(...){shape_cursor=previous;throw;}
            shape_cursor=previous;
        }else if(owner==0x2e9600u)native_tu15_forward_shape_bounds(memory,shape,transform,margin,output,state,body_policy.bounds);
        else if(owner==0x2e5e80u)native_tu15_motion_shape_bounds(memory,shape,transform,margin,output,state,body_policy.bounds,observer);
        else{
            if(activity)++activity->unconverted_policy_bounds_owners[owner];
            const std::array arguments{transform.identity,margin,output.identity};
            call(owner,arguments,shape.identity,12,state);
        }
    };
    body_policy.bounds.child_bounds=shape_bounds;
    body_policy.policy_kind=[&](auto policy,auto owner,auto&state){return call(owner.identity,{},policy,0,state);};
    body_policy.destroy_policy=[&](auto policy,auto owner,auto disposing,auto&state){
        const std::array arguments{disposing};call(owner.identity,arguments,policy,4,state);
    };
    body_policy.release_base_lock=[&](auto object,auto flags,auto&state){
        const std::array arguments{flags};call(0x2e12a0u,arguments,object,4,state);
    };
'''+needle)
 needle='''    agents.mark_body=[&](auto body,auto flags,auto&state){
        const std::array arguments{flags};call(0x31b730u,arguments,body.identity,4,state);
    };'''
 source=once(source,needle,'''    agents.mark_body=[&](auto body,auto flags,auto&state){
        native_tu15_select_body_policy(memory,body,flags,{{scratch.policy_bounds}},state,depth,
            body_policy,observer,activity?&activity->body_policy:nullptr);
    };''')
 adapter=once(adapter,'        const auto end=std::uint32_t(scratch_end),service_stack=end-0x400u;',
  '        const auto end=std::uint32_t(scratch_end),service_stack=end-0x400u;\n        const auto policy_bounds=((end-0x60u)&~15u)-32u,shape_end=(end-0x100u)&~15u;')
 adapter=once(adapter,'{end-0x20u,end-0x1cu},remaining,observed,','{end-0x20u,end-0x1cu,policy_bounds,shape_end-0x200u,0x200u},remaining,observed,')
 after=[source,header,adapter];patch=[]
 for path,old,new in zip(paths,before,after):
  target=a.output/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new);patch.extend(difflib.unified_diff(old.splitlines(keepends=True),new.splitlines(keepends=True),fromfile='a/'+str(path),tofile='b/'+str(path)))
 delta=a.output/'activation-body-policy-binding.diff';delta.write_text(''.join(patch));units=['body_policy','gameplay_policy_destruction','shape_bounds','centered_shape_bounds','compound_shape_bounds','sphere_shape_bounds','geometry'];files=[ROOT/f'src/physics/native_tu15_{name}.cpp'for name in units]+[ROOT/f'include/halo2/native_tu15_{name}.hpp'for name in units]+[ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',Path(__file__)]
 save(a.output/'source-pins.json',dict(schema='native-tu15-body-policy-binding-source-delta-v1',qualified=False,working_tree_modified=False,base_overlay=str(a.base),base_hashes={str(path):sha(a.base/path if path!=transport else ROOT/path)for path in paths},staged_hashes={str(path):sha(a.output/'staged'/path)for path in paths},source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},patch=dict(path=str(delta),sha256=sha(delta)),required_new_compile_units=['src/physics/native_tu15_body_policy.cpp','src/physics/native_tu15_gameplay_policy_destruction.cpp'],owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]));print(delta);return 0
if __name__=='__main__':raise SystemExit(main())
