#pragma once
#include "physics/runtime/native_tu15_mopp_ray_plane_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_branch_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_crossing_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_recursion_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_resume_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_primitive_key_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_leaf_frame.hpp"

namespace halo2::native_mopp_ray_caller_detail {
// Private composition of the qualified native DATA stages. Slab and diagnostic
// owners are explicit native operations until their complete adapters join this
// module. No production prefix admits this controller yet.
template<class State,class Before,class Dispatch,class Slab,class Diagnostic>
void traverse(State&s,Before&before,Dispatch&dispatch,Slab&slab,Diagnostic&diagnostic){
    native_raw_x87_detail::Scope scope{s};IntegerFrame frame{s,before,ScopeIo<State>{&scope}};
    auto entry=frame.begin();
    for(;;){
        if(!entry.valid_program_class){diagnostic(s,before,scope,dispatch);return;}
        frame.publish_program_class(native_mopp_aabb_abi_detail::source_class(entry.opcode));
        if(entry.opcode==0u){frame.finish();return;}
        auto program=entry.program;
        if(entry.opcode<=4u){
            ScaleFrame scale{s,before,scope};native_mopp_ray_scale_detail::scale(scale,program,entry.opcode);
        }else if(native_mopp_integer_program_detail::step(frame,program,entry.opcode)){
            // The source-owned views publish their actual current program.
        }else if(entry.opcode>=0x10u&&entry.opcode<=0x25u){
            PlaneFrame plane{s,before,scope};native_mopp_ray_plane_detail::Progress planes;
            if(!native_mopp_ray_plane_detail::prepare(plane,program,entry.opcode,planes)){
                diagnostic(s,before,scope,dispatch);return;
            }
            BranchFrame branch{s,before,scope};
            if(native_mopp_ray_branch_detail::choose(branch,program)==native_mopp_ray_branch_detail::Decision::crossing){
                CrossingFrame crossing{s,before,scope};native_mopp_ray_crossing_detail::Progress crossed;
                native_mopp_ray_crossing_detail::prepare_child(crossing,crossed);
                RecursionFrame recursion{s,before,scope};
                recursion.invoke(crossed.increasing,[&]{traverse(s,before,dispatch,slab,diagnostic);});
                ResumeFrame resume{s,before,scope};native_mopp_ray_resume_detail::Progress resumed;
                if(native_mopp_ray_resume_detail::resume(resume,crossed.increasing,resumed)){
                    frame.finish();return;
                }
            }
        }else if(entry.opcode>=0x26u&&entry.opcode<=0x2bu){
            if(slab(s,before,scope,entry.opcode,program)){frame.finish();return;}
        }else if(entry.opcode>=0x30u&&entry.opcode<=0x53u){
            PrimitiveKeyFrame keys{s,before,scope};std::uint32_t key{};
            native_mopp_primitive_key_detail::decode(keys,program,entry.opcode,key);
            LeafFrame leaf{s,before,scope,dispatch};native_mopp_ray_leaf_detail::Progress primitive;
            const auto result=native_mopp_ray_leaf_detail::visit(leaf,key,primitive);leaf.finish(result);return;
        }else{diagnostic(s,before,scope,dispatch);return;}
        entry=frame.next();
    }
}
}
