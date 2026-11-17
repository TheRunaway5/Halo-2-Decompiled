#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_append_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_simplex_direction_kernel.hpp"
namespace halo2::native_solver_hull_append_detail {
using P=NativeTu15SolverHullAppendPhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Before>
class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;
    std::uint32_t load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);return s_.gp[reg]=word(address);}
    void vector_store(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{pc,address,flags,flags|0x10000u};}
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);
        for(unsigned offset:{0u,8u}){SourcePhysicsContactRecordStore event{pc,address+offset,{},{}};
            if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}
            const auto written=std::span(bytes).subspan(offset,8);fp_.io([&]{s_.memory.write(event.address,written);});
            if(s_.observe){event.written.assign(written.begin(),written.end());fp_.io([&]{s_.observe(event);});}
        }
    }
public:
    using Vector=__m128;using Data=NativeTu15SolverHullAppend<Vector>;
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp):Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp){}
    void begin(Data&d){auto&g=s_.gp;push(0x313290u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0xcu,4);d.context=g[1];}
    std::uint32_t simplex(P phase,const Data&){const auto pc=phase==P::initial?0x313299u:phase==P::first_point?0x3132bbu:phase==P::second_point?0x3132ceu:0x3132e5u;return load(pc,phase==P::final_count?1u:2u,s_.gp[1]+0x48u);}
    std::uint32_t count(P phase,const Data&d){const auto pc=phase==P::initial?0x31329cu:phase==P::first_point?0x3132beu:0x3132d1u;return load(pc,phase==P::initial?2u:6u,d.simplex+0x80u);}
    std::uint32_t point_pair(const Data&){return load(0x3132a2u,0,s_.gp[5]+8u);}
    Vector vector(P phase,const Data&,std::uint32_t address){const auto pc=phase==P::initial?0x3132a5u:phase==P::first_point?0x3132c4u:0x3132d7u;return s_.xmm[0]=Base::vector(pc,address);}
    Vector first_point(const Data&d){return s_.xmm[1]=Base::vector(0x3132a9u,d.pair);}
    std::uint32_t differences(const Data&){push(0x3132acu,s_.gp[6]);return load(0x3132adu,6,s_.gp[1]+0x50u);}
    void difference_destination(const Data&){s_.gp[2]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,s_.gp[2]);s_.gp[2]=s_.flags.plus(s_.gp[2],s_.gp[6],4);}
    Vector subtract(Vector first,Vector second){return native_simplex_direction_numeric::subtract(first,second);}
    void store_difference(const Data&d){s_.xmm[1]=d.difference;vector_store(0x3132b8u,d.destination,d.difference);}
    void point_destination(P phase,const Data&){if(phase==P::second_point)s_.gp[6]=s_.flags.plus(s_.gp[6],4u,4);s_.gp[6]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,s_.gp[6]);}
    void store_point(P phase,const Data&d){vector_store(phase==P::first_point?0x3132cau:0x3132e1u,d.destination,d.point);}
    void increment_count(const Data&d){
        at(0x3132e8u);const auto address=d.simplex+0x80u,current=word(address);auto retired=s_.flags;const auto value=retired.increment(current,4,false);
        SourcePhysicsContactRecordStore event{0x3132e8u,address,{},{}};
        if(s_.observe){event.before.resize(4);fp_.io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};std::memcpy(bytes.data(),&value,4);fp_.io([&]{s_.memory.write(address,bytes);});
        s_.flags=retired;
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());fp_.io([&]{s_.observe(event);});}
    }
    void finish(const Data&){auto&g=s_.gp;g[6]=pop(0x3132eeu);g[4]=g[5];g[5]=pop(0x3132f1u);pop(0x3132f2u);g[4]+=4u;}
};
template<class State,class Before=Noop>
void run_private_owner(State&s,Before before={}){native_raw_x87_detail::Scope<State>fp{s};Frame<State,Before>memory{s,before,fp};typename Frame<State,Before>::Data data;native_tu15_append_solver_hull_point(memory,data);}
} // namespace halo2::native_solver_hull_append_detail
#endif
