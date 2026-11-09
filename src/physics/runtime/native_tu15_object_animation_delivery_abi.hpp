#pragma once
#include "halo2/native_tu15_object_animation_delivery_kernel.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_object_node_admission.hpp"

namespace halo2::native_object_animation_detail {
using P=NativeTu15ObjectAnimationPhase;using Data=NativeTu15ObjectAnimation;
enum class Mode {delivery,selection,presence};
inline bool reference_mode(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_OBJECT_ANIMATION");return p&&*p;}();return value;}
struct Activity {std::uint64_t delivery{},selection{},presence{};};
inline Activity&activity(){thread_local Activity value;return value;}
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
// Preserve the original immediate SHL's undefined arithmetic bits using the
// same real instruction. AND has already produced the exact incoming flags;
// SAHF restores its low arithmetic state and XOR explicitly supplies OF=0.
inline std::uint32_t shifted_tag_flags(std::uint32_t tag,std::uint32_t incoming){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t flags=(incoming&0xd5u)<<8u,overflow;
    asm volatile("xorl %%ecx,%%ecx; sahf; shll $4,%2; seto %%cl; lahf; movzbl %%ah,%%eax"
        :"+&a"(flags),"=&c"(overflow),"+&r"(tag)::"cc");
    return (flags&0xd5u)|((overflow&1u)<<11u);
#else
    throw std::runtime_error("Original animation tag shift flag backend unavailable");
#endif
}
template<class State,class Before=Noop>struct Access {
    State&s;Mode mode;Before before{};
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    void push(std::uint32_t pc,std::uint32_t value){at(pc);s.push(pc,value);}
    void begin(Data&d){auto&g=s.gp;
        push(0xbfb90u,g[3]);push(0xbfb91u,g[5]);at(0xbfb92u);g[5]=physics_step_detail::r(s.memory,g[4]+0x10u,4u);
        push(0xbfb96u,g[6]);push(0xbfb97u,g[7]);g[7]=g[1];s.flags.logic(g[7],4);g[6]=g[0];g[3]=g[2];
        d.manager=g[5];d.mask=g[7];d.count=g[6];d.object=g[3];
    }
    std::uint32_t read(P phase,const Data&,std::uint32_t address,unsigned width){auto&g=s.gp;std::uint32_t pc{};
        const bool delivery=mode==Mode::delivery,presence=mode==Mode::presence;
        switch(phase){
        case P::selection:pc=delivery?0xbfbbau:0xb87b0u;break;
        case P::graph_tag:pc=delivery?0xbfbc4u:presence?0xb87bau:0xb8789u;break;
        case P::animation_index:pc=delivery?0xbfbcbu:presence?0xb87beu:0xb8782u;break;
        case P::packed_id:pc=delivery?0xbfbd1u:0xb879du;break;
        case P::tag_array:pc=delivery?0xbfbd4u:0xb878bu;break;
        case P::graph:{
            const auto value=s.flags.logic(g[0]&0xffffu,4);g[0]=value;
            const auto flags=shifted_tag_flags(value,s.flags.bits());g[0]=value<<4u;s.flags.load(flags);pc=delivery?0xbfbe2u:0xb8799u;break;
        }
        case P::post_enabled:pc=0xbfc24u;break;
        case P::post_flags:pc=0xbfc2bu;break;
        case P::object_pool:pc=0xbfc31u;break;
        case P::object_rows:pc=0xbfc36u;break;
        case P::object_body:g[3]=s.flags.logic(g[3]&0xffffu,4);g[2]=g[3]*3u;pc=0xbfc42u;break;
        case P::post_offset:pc=0xbfc46u;break;
        default:throw std::runtime_error("Invalid object animation read phase");
        }
        at(pc);const auto value=physics_step_detail::r(s.memory,address,width);
        switch(phase){
        case P::selection:
            if(delivery)g[0]=value;else g[2]=value;
            g[1]=s.flags.logic(0xffffffffu,4);s.flags.minus(value,g[1],4);break;
        case P::graph_tag:
            if(!presence)g[0]=value;
            if(delivery||presence)s.flags.minus(value,g[1],4);break;
        case P::animation_index:s.flags.minus(value,0xffffu,2);break;
        case P::packed_id:if(delivery)g[1]=value;else g[0]=value;break;
        case P::tag_array:case P::graph:g[2]=value;break;
        case P::post_enabled:s.set(0,value,1,0);s.flags.logic(value,1);break;
        case P::post_flags:s.flags.logic(value&2u,1);break;
        case P::object_pool:case P::object_body:g[0]=value;break;
        case P::object_rows:g[1]=value;break;
        case P::post_offset:g[2]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));break;
        default:break;
        }
        return value;
    }
    std::uint32_t frame(P phase,const Data&,unsigned offset){
        const auto pc=phase==P::sample_output?0xbfbecu:phase==P::sample_orientation?0xbfbf0u:phase==P::pose_output?0xbfc05u:
            phase==P::pose_orientation?0xbfc09u:phase==P::notify_output?0xbfc18u:0xbfc4du;
        at(pc);const auto value=physics_step_detail::r(s.memory,s.gp[4]+offset,4u);
        s.gp[phase==P::sample_output||phase==P::pose_orientation?0:phase==P::pose_output?2:1]=value;return value;
    }
    void prepare(P phase,const Data&){auto&g=s.gp;
        switch(phase){
        case P::scan_result:
            s.flags.logic(g[0],4);if(std::bit_cast<std::int32_t>(g[0])>=0){g[0]=s.flags.increment(g[0],4,false);s.flags.minus(g[0],g[6],4);if(s.flags.sf!=s.flags.of)g[6]=g[0];}break;
        case P::count_test:s.flags.logic(g[6],4);break;
        case P::resolve_call:push(mode==Mode::delivery?0xbfbe6u:0xb87a0u,g[mode==Mode::delivery?1:0]);break;
        case P::sample_call:
            push(0xbfbf4u,g[3]);push(0xbfbf5u,0u);push(0xbfbf7u,g[0]);push(0xbfbf8u,g[7]);
            push(0xbfbf9u,0x3f800000u);push(0xbfbfeu,g[1]);push(0xbfbffu,g[5]);break;
        case P::pose_call:
            push(0xbfc0du,g[2]);push(0xbfc0eu,g[6]);push(0xbfc0fu,g[7]);push(0xbfc10u,g[5]);push(0xbfc11u,g[0]);push(0xbfc12u,g[3]);break;
        case P::notify_call:push(0xbfc1cu,g[1]);push(0xbfc1du,g[6]);push(0xbfc1eu,g[7]);break;
        case P::post_call:
            g[2]=s.flags.plus(g[2],g[0],4);push(0xbfc53u,g[2]);g[3]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(g[6]))));
            push(0xbfc57u,g[7]);g[0]=g[5]+0x64u;break;
        default:throw std::runtime_error("Invalid animation delivery preparation phase");
        }
    }
    void finish(const Data&){auto&g=s.gp;at(0xbfc60u);g[7]=s.pop();at(0xbfc61u);g[6]=s.pop();at(0xbfc62u);g[5]=s.pop();at(0xbfc63u);g[3]=s.pop();at(0xbfc64u);s.pop();g[4]+=12u;}
};
template<class State,class Call,class Before=Noop>struct Services {
    State&s;Call&call;Access<State,Before>&memory;
    void adopt(Data&d){d.manager=s.gp[5];d.count=s.gp[6];d.mask=s.gp[7];d.object=s.gp[3];}
    void invoke(Data&d,std::uint32_t owner,std::uint32_t pc,std::uint32_t next){memory.push(pc,next);call(s,owner,pc);adopt(d);}
    std::uint32_t scan(Data&d){invoke(d,0x141010u,0xbfba2u,0xbfba7u);return s.gp[0];}
    void resolve(Data&d){const bool delivery=memory.mode==Mode::delivery;invoke(d,0x1e3150u,delivery?0xbfbe7u:0xb87a1u,delivery?0xbfbecu:0xb87a6u);}
    void sample(Data&d){invoke(d,0x1d5840u,0xbfc00u,0xbfc05u);}
    void poses(Data&d){invoke(d,0xbf9a0u,0xbfc13u,0xbfc18u);}
    void notify(Data&d){invoke(d,0x10e100u,0xbfc1fu,0xbfc24u);}
    void postprocess(Data&d){invoke(d,0x1e1720u,0xbfc5bu,0xbfc60u);}
};
template<class State,class Call,class Before=Noop>void deliver_private(State&s,Call call,Before before={}){
    Access<State,Before> memory{s,Mode::delivery,before};Services<State,Call,Before> services{s,call,memory};Data data;
    native_tu15_deliver_object_animation(memory,data,services);
}
template<class State,class Call,class Before=Noop>void selection_private(State&s,Call call,Before before={}){
    Access<State,Before> memory{s,Mode::selection,before};Services<State,Call,Before> services{s,call,memory};Data data;data.manager=s.gp[1];
    s.gp[0]=s.flags.logic(0u,4);data.animation_index=memory.read(P::animation_index,data,data.manager+6u,2u);
    if(data.animation_index!=0xffffu){data.graph_tag=memory.read(P::graph_tag,data,data.manager,4u);native_tu15_resolve_object_animation(memory,data,services,true);}
    memory.at(0xb87a6u);s.pop();
}
template<class State,class Before=Noop>void presence_private(State&s,Before before={}){
    Access<State,Before> memory{s,Mode::presence,before};Data data;data.manager=s.gp[0];
    if(native_tu15_object_animation_present(memory,data)){s.gp[0]=1u;memory.at(0xb87c9u);s.pop();}
    else{s.gp[0]=s.flags.logic(0u,4);memory.at(0xb87ccu);s.pop();}
}
template<class State,class Call>bool bridge(State&s,std::uint32_t entry,Call call){
    if(entry!=0xbfb90u&&entry!=0xb8780u&&entry!=0xb87b0u)return false;
    if(reference_mode()||!native_object_node_admission_detail::admitted(s,{{0xb8780u,0xb87ccu},{0xbfb90u,0xbfc64u}}))return false;
    if(entry==0xbfb90u){++activity().delivery;deliver_private(s,call);}
    else if(entry==0xb8780u){++activity().selection;selection_private(s,call);}
    else{++activity().presence;presence_private(s);}
    return true;
}
} // namespace halo2::native_object_animation_detail
