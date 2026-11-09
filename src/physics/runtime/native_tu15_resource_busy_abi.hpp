#pragma once
#include "halo2/native_tu15_resource_busy_kernel.hpp"
#include "physics/runtime/native_tu15_page_window_abi.hpp"

namespace halo2::native_resource_busy_detail {
inline bool reference_mode(){static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_RESOURCE_BUSY");return p&&*p;}();return value;}
struct Activity {std::uint64_t queries{},callbacks{},reference_queries{};};
inline Activity &activity(){thread_local Activity value;return value;}
template<class State,class Before=native_page_window_detail::Noop>struct QueryAccess {
    using P=NativeTu15ResourceBusyPhase;State &s;Before before{};
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    std::uint32_t read(P phase,const NativeTu15ResourceBusy &data,std::uint32_t address){
        auto &g=s.gp;switch(phase){
        case P::initial_header:at(0x4057f0u);break;
        case P::device:g[2]=s.flags.logic(g[0]&0x70000u,4);s.flags.minus(g[2],0x50000u,4);at(0x405800u);s.push(0x405800u,g[7]);at(0x405801u);break;
        case P::initial_parent:at(0x405809u);break;
        case P::initial_parent_header:s.flags.logic(g[0]&0x780000u,4);s.flags.logic(g[2],4);at(0x40581bu);break;
        case P::selected_header:
            if((data.initial_header&0x70000u)==0x50000u&&data.parent)g[1]=g[2];at(0x405825u);break;
        case P::parent:g[0]=s.flags.logic(g[0]&0x70000u,4);s.flags.minus(g[0],0x50000u,4);at(0x40583au);break;
        case P::parent_header:at(0x405841u);break;
        case P::fence:
            if((data.header&0x70000u)!=0x50000u){g[0]=s.flags.logic(g[0]&0x70000u,4);s.flags.minus(g[0],0x50000u,4);}at(0x405849u);break;
        case P::counter_owner:at(0x405850u);break;
        case P::counter:at(0x405854u);break;
        case P::submitted:at(0x405856u);break;
        default:throw std::runtime_error("Invalid native resource query read boundary");
        }
        const auto value=physics_step_detail::r(s.memory,address,4);
        switch(phase){
        case P::initial_header:g[0]=value;g[2]=g[0];break;
        case P::device:g[7]=value;break;
        case P::initial_parent:g[2]=value;s.flags.logic(g[2],4);if(value)s.flags.logic(g[0]&0x780000u,4);break;
        case P::initial_parent_header:s.flags.logic(value&0x780000u,4);break;
        case P::selected_header:g[0]=value;s.flags.logic(g[0]&0x780000u,4);break;
        case P::parent:g[0]=value;s.flags.logic(g[0],4);break;
        case P::parent_header:s.flags.logic(value&0x780000u,4);break;
        case P::fence:g[2]=value;s.flags.logic(g[2],4);break;
        case P::counter_owner:g[0]=value;break;
        case P::counter:g[6]=value;break;
        case P::submitted:g[0]=value;break;
        default:break;
        }
        return value;
    }
    void prepare(P phase,const NativeTu15ResourceBusy&){
        auto &g=s.gp;
        if(phase==P::save_counter){at(0x405853u);s.push(0x405853u,g[6]);}
        else if(phase==P::counter_compare){g[7]=g[0];g[0]=s.flags.minus(g[0],g[6],4);g[7]=s.flags.minus(g[7],g[2],4);s.flags.minus(g[7],g[0],4);at(0x405861u);g[6]=s.pop();}
    }
    void write(P,const NativeTu15ResourceBusy&,std::uint32_t address,std::uint32_t){s.gp[0]=s.flags.logic(0u,4);at(0x40586du);physics_step_detail::w(s.memory,s.observe,0x40586du,address,4,0u);}
    void complete(P phase,const NativeTu15ResourceBusy&){
        if(phase==P::busy){s.gp[0]=1u;at(0x405869u);s.gp[7]=s.pop();at(0x40586au);s.pop();}
        else{at(0x405870u);s.gp[7]=s.pop();at(0x405871u);s.pop();}
    }
};
template<class State,class Before=native_page_window_detail::Noop>bool query(State &s,Before before={}){
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().queries;NativeTu15ResourceBusy data;data.resource=s.gp[1];QueryAccess<State,Before> memory{s,before};native_tu15_resource_is_busy(memory,data);return true;
}
template<class State,class Before=native_page_window_detail::Noop>struct CallbackAccess {
    using P=NativeTu15PageBusyPhase;State &s;Before before{};
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    std::uint32_t argument(P,const NativeTu15PageBusy&){at(0x12fcb0u);s.gp[0]=physics_step_detail::r(s.memory,s.gp[4]+4u,4);return s.gp[0];}
    std::uint32_t read(P phase,const NativeTu15PageBusy &data,std::uint32_t address,unsigned width){
        auto &g=s.gp;switch(phase){
        case P::array:at(0x12fcb4u);break;case P::rows:at(0x12fcbau);break;
        case P::flags:g[0]=s.flags.logic(g[0]&0xffffu,4);g[0]*=5u;at(0x12fcc5u);break;
        case P::global_policy:s.flags.logic(s.reg(1,1,0)&2u,1);at(0x12fcd6u);break;
        case P::association:if(!(data.flags&2u))s.flags.logic(s.reg(1,1,0)&2u,1);at(0x12fce0u);break;
        case P::ready:at(0x12fcebu);break;
        default:throw std::runtime_error("Invalid native page-busy read boundary");
        }
        const auto value=physics_step_detail::r(s.memory,address,width);
        switch(phase){
        case P::array:g[1]=value;break;case P::rows:g[2]=value;break;
        case P::flags:s.set(1,value,1,0);s.flags.logic(s.reg(1,1,0)&1u,1);g[0]=g[2]+g[0]*8u;break;
        case P::global_policy:case P::ready:s.set(1,value,1,0);s.flags.logic(s.reg(1,1,0),1);break;
        case P::association:s.flags.minus(value,0xffffffffu,4);break;
        default:break;
        }
        return value;
    }
    void prepare(P,const NativeTu15PageBusy&){s.gp[1]=s.gp[0]+0x14u;at(0x12fcf5u);s.push(0x12fcf5u,0x12fcfau);}
    void complete(P phase,const NativeTu15PageBusy&){
        if(phase==P::busy){s.set(0,1u,1,0);at(0x12fd00u);s.pop();}
        else{s.set(0,s.flags.logic(0u,1),1,0);at(0x12fce8u);s.pop();}
        s.gp[4]+=4u;
    }
};
template<class State,class Call,class Before=native_page_window_detail::Noop>bool callback(State &s,Call call,Before before={}){
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().callbacks;NativeTu15PageBusy data;CallbackAccess<State,Before> memory{s,before};
    const auto resource=[&](std::uint32_t,const NativeTu15PageBusy&){
        if(!query(s,before)){++activity().reference_queries;call(s,0x4057f0u,0x12fcf5u,false);}
        s.flags.logic(s.gp[0],4);return s.gp[0]!=0u;
    };
    native_tu15_page_is_busy(memory,data,resource);return true;
}
} // namespace halo2::native_resource_busy_detail
