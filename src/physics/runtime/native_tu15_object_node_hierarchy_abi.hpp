#pragma once
#include "halo2/native_tu15_object_node_hierarchy_kernel.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_object_node_hierarchy_detail {
// Internal retained BF53C entry. The complete BF0C0 controller supplies its
// admission policy and transports the real child owners.
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Before=Noop>struct Access {
    using P=NativeTu15ObjectNodeHierarchyPhase;State &s;Before before{};
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    std::uint32_t frame_address(std::uint32_t offset)const{return s.gp[4]+offset;}
    std::uint32_t read(P phase,const NativeTu15ObjectNodeHierarchy &data,std::uint32_t address,unsigned width){
        auto &g=s.gp;
        switch(phase){
        case P::initial_count:at(0xbf53cu);break;
        case P::matrices:
            s.flags.logic(g[0],4);g[6]=1u;s.flags.minus(g[0],g[6],4);at(0xbf554u);break;
        case P::orientations:at(0xbf558u);break;
        case P::model:at(0xbf567u);break;
        case P::records:at(0xbf56bu);break;
        case P::mask:
            g[1]=s.flags.logic(g[6]&31u,4);g[0]=s.flags.shift(1u,g[1]&255u,4);
            g[1]=s.flags.right(g[6],5u,4,true);g[7]=s.flags.plus(g[7],g[3],4);at(0xbf581u);break;
        case P::parent:at(0xbf58eu);break;
        case P::matrix_root:at(0xbf592u);break;
        case P::next_model:at(0xbf5a1u);break;
        case P::next_count:at(0xbf5a5u);break;
        default:throw std::runtime_error("Invalid native object hierarchy read phase");
        }
        const auto value=physics_step_detail::r(s.memory,address,width);
        switch(phase){
        case P::initial_count:case P::matrix_root:case P::next_model:g[0]=value;break;
        case P::matrices:g[2]=value;break;
        case P::orientations:g[5]=value;break;
        case P::model:case P::next_count:g[1]=value;break;
        case P::records:g[7]=value;break;
        case P::mask:s.flags.logic(value&g[0],4);break;
        case P::parent:g[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));break;
        default:break;
        }
        (void)data;return value;
    }
    void prepare(P phase,const NativeTu15ObjectNodeHierarchy&){
        auto &g=s.gp;
        if(phase==P::begin){g[2]=s.flags.plus(g[2],0x34u,4);g[3]=0x60u;g[5]=s.flags.plus(g[5],0x20u,4);}
        else if(phase==P::advance){g[6]=s.flags.increment(g[6],4,false);g[3]=s.flags.plus(g[3],0x60u,4);
            g[5]=s.flags.plus(g[5],0x20u,4);g[2]=s.flags.plus(g[2],0x34u,4);s.flags.minus(g[6],g[1],4);}
    }
    void complete(P phase,const NativeTu15ObjectNodeHierarchy&){
        auto &g=s.gp;
        if(phase==P::finish_empty){s.flags.logic(g[0],4);if(std::bit_cast<std::int32_t>(g[0])>0){g[6]=1u;s.flags.minus(g[0],g[6],4);}}
        const auto first=phase==P::finish_empty?0xbf625u:0xbf5b6u;
        at(first);g[7]=s.pop();at(first+1u);g[6]=s.pop();at(first+2u);g[5]=s.pop();at(first+3u);g[3]=s.pop();
        g[4]=s.flags.plus(g[4],0xe8u,4);at(phase==P::finish_empty?0xbf62fu:0xbf5c0u);s.pop();g[4]+=4u;
    }
};
template<class State,class Call,class Before=Noop>
void run_private_stage(State &s,Call call,Before before={}){
    using P=NativeTu15ObjectNodeHierarchyPhase;
    Access<State,Before> access{s,before};
    struct Services {
        State &s;Call &call;Access<State,Before> &access;
        NativeTu15ObjectNodeHierarchyResume resumed()const{return {s.gp[6],s.gp[3],s.gp[5],s.gp[2],s.gp[7]};}
        NativeTu15ObjectNodeHierarchyResume local(const NativeTu15ObjectNodeHierarchy&){
            access.at(0xbf587u);s.push(0xbf587u,s.gp[5]);access.at(0xbf588u);s.push(0xbf588u,s.gp[2]);
            access.at(0xbf589u);s.push(0xbf589u,0xbf58eu);call(s,0x144cf0u);return resumed();
        }
        NativeTu15ObjectNodeHierarchyResume product(std::uint32_t,const NativeTu15ObjectNodeHierarchy&){
            s.gp[1]=s.flags.product(s.gp[1],0x34u);access.at(0xbf599u);s.push(0xbf599u,s.gp[2]);
            s.gp[1]=s.flags.plus(s.gp[1],s.gp[0],4);access.at(0xbf59cu);s.push(0xbf59cu,0xbf5a1u);call(s,0x145560u);return resumed();
        }
    } services{s,call,access};
    NativeTu15ObjectNodeHierarchy data;data.model=s.gp[5];
    native_tu15_publish_object_node_hierarchy(access,data,services);
}
} // namespace halo2::native_object_node_hierarchy_detail
