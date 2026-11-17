#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_context_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

namespace halo2::native_solver_hull_context_detail {
using Data=NativeTu15SolverHullContext;
using P=NativeTu15SolverHullContextPhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
// Complete314710 context/profile wrapper. Its real313BB0 owner is transported
// through Call; this private frame does not assert that descendant is native.
template<class State,class Call,class Before>
class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;Call&call_;
    std::uint32_t load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);return s_.gp[reg]=word(address);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){Base::store(pc,address,value,[]{});}
public:
    Frame(State&s,Call&call,Before&before,native_raw_x87_detail::Scope<State>&fp)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void begin(Data&){auto&g=s_.gp;push(0x314710u,g[5]);g[5]=g[4];
        g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x6cu,4);}
    std::uint32_t profile_cursor(Data&,bool ending){
        const auto result=load(ending?0x31479bu:0x314719u,1,0x48c974u);
        if(ending)s_.gp[6]=s_.gp[0];return result;
    }
    std::uint32_t profile_limit(Data&,bool ending){at(ending?0x3147a3u:0x31471fu);return word(0x48c978u);}
    void profile_ready(Data&d,bool ending,std::uint32_t cursor,std::uint32_t limit){
        s_.flags.minus(cursor,limit,4);if(!ending){push(0x314725u,s_.gp[6]);d.context=s_.gp[4]+0x10u;}
    }
    void profile_tag(Data&,bool ending,std::uint32_t cursor,std::uint32_t tag){store(ending?0x3147abu:0x314728u,cursor,tag);}
    std::uint64_t timestamp(Data&,bool ending){
        at(ending?0x3147b1u:0x31472eu);
        if(!s_.services.timestamp_counter)throw std::runtime_error("Required original hull-context RDTSC owner unavailable");
        std::uint64_t value{};fp_.io([&]{value=s_.services.timestamp_counter();});
        s_.gp[0]=std::uint32_t(value);s_.gp[2]=std::uint32_t(value>>32u);return value;
    }
    void stage_timestamp(Data&,bool ending,std::uint32_t value){store(ending?0x3147b3u:0x314730u,s_.gp[4]+0xcu,value);}
    std::uint32_t reload_timestamp(Data&,bool ending){return load(ending?0x3147b7u:0x314734u,0,s_.gp[4]+0xcu);}
    void profile_low(Data&,bool ending,std::uint32_t cursor,std::uint32_t value){store(ending?0x3147bbu:0x314738u,cursor+4u,value);}
    void advance_profile(Data&,bool ending,std::uint32_t cursor){
        s_.gp[1]=s_.flags.plus(cursor,0xcu,4);store(ending?0x3147c1u:0x31473eu,0x48c974u,s_.gp[1]);
    }
    std::uint32_t relative_transform(Data&){return load(0x314744u,0,s_.gp[5]+0x10u);}
    std::uint32_t first_shape(Data&){return load(0x31474au,1,s_.gp[5]+8u);}
    std::uint32_t second_shape(Data&){return load(0x31474du,2,s_.gp[5]+0xcu);}
    std::uint32_t differences(Data&){return load(0x314766u,1,s_.gp[5]+0x18u);}
    std::uint32_t simplex(Data&){return load(0x314772u,0,s_.gp[5]+0x14u);}
    std::uint32_t output(Data&){return load(0x314779u,2,s_.gp[5]+0x1cu);}
    __m128 vector(P phase,const Data&,std::uint32_t address){
        const auto pc=phase==P::basis_first?0x314747u:phase==P::basis_second?0x314755u:phase==P::basis_third?0x31475eu:0x31476eu;
        return s_.xmm[0]=Base::vector(pc,address);
    }
    void store_vector(P phase,const Data&,__m128 value){
        const auto pc=phase==P::basis_first?0x314750u:phase==P::basis_second?0x314759u:phase==P::basis_third?0x314769u:0x314785u;
        const auto offset=phase==P::basis_first?0x10u:phase==P::basis_second?0x20u:phase==P::basis_third?0x30u:0x44u;
        at(pc);const auto address=s_.gp[4]+offset;
        if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{pc,address,flags,flags|0x10000u};}
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);
        for(unsigned offset:{0u,8u}){
            SourcePhysicsContactRecordStore event{pc,address+offset,{},{}};
            if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}
            const auto written=std::span(bytes).subspan(offset,8);fp_.io([&]{s_.memory.write(event.address,written);});
            if(s_.observe){event.written.assign(written.begin(),written.end());fp_.io([&]{s_.observe(event);});}
        }
    }
    void store_word(P phase,const Data&,std::uint32_t value){
        const auto pc=phase==P::first_shape?0x314762u:phase==P::second_shape?0x314775u:phase==P::differences?0x31477cu:phase==P::simplex?0x31478au:0x31478eu;
        const auto offset=phase==P::first_shape?0x50u:phase==P::second_shape?0x54u:phase==P::differences?0x60u:phase==P::simplex?0x5cu:0x60u;
        store(pc,s_.gp[4]+offset,value);
    }
    void prepare_child(const Data&d){push(0x314780u,d.output);s_.gp[1]=s_.gp[4]+0x14u;}
    std::uint32_t hull_child(const Data&){
        push(0x314796u,0x31479bu);fp_.child([&]{call_(s_,0x313bb0u,0x314796u);});return s_.gp[0];
    }
    std::uint32_t finish(const Data&){auto&g=s_.gp;g[0]=g[6];g[6]=pop(0x3147c9u);g[4]=g[5];g[5]=pop(0x3147ccu);pop(0x3147cdu);return g[0];}
};
template<class State,class Call,class Before=Noop>
std::uint32_t run_private_owner(State&s,Call call,Before before={}){
    native_raw_x87_detail::Scope<State>fp{s};Frame<State,Call,Before>memory{s,call,before,fp};Data data;
    return native_tu15_build_solver_hull_context(memory,data);
}
template<class State,class Before=Noop>
std::uint32_t run_private_continuation(State&s,Before before={}){
    const auto unreachable=[](auto&,std::uint32_t,std::uint32_t){throw std::runtime_error("Unexpected child in hull-context continuation");};
    native_raw_x87_detail::Scope<State>fp{s};auto call=unreachable;
    Frame<State,decltype(call),Before>memory{s,call,before,fp};Data data;
    return native_tu15_finish_solver_hull_context(memory,data);
}
} // namespace halo2::native_solver_hull_context_detail
#endif
