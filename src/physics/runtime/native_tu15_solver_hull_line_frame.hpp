#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_line_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

namespace halo2::native_solver_hull_line_detail {
using P=NativeTu15SolverHullLinePhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};

// Private physical frame. Provider GP/SIMD mutation, retained publication,
// source/default admission and hardware partial vector faults are separate.
template<class State,class Call,class Before>class Frame:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;Call&call_;
    static std::uint32_t low(__m128 value){std::uint32_t bits{};std::memcpy(&bits,&value,4);return bits;}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){Base::store(pc,address,value,[]{});}
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{pc,address,flags,flags|0x10000u};}
        std::array<std::uint64_t,2>parts{};std::memcpy(parts.data(),&value,16);
        for(unsigned n=0;n<2;++n){
            const auto bytes=std::as_bytes(std::span(parts).subspan(n,1));SourcePhysicsContactRecordStore event{pc,address+n*8u,{},{}};
            if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}
            fp_.io([&]{s_.memory.write(event.address,bytes);});
            if(s_.observe){event.written.assign(bytes.begin(),bytes.end());fp_.io([&]{s_.observe(event);});}
        }
    }
    void child(std::uint32_t pc,std::uint32_t target,std::uint32_t returned){push(pc,returned);fp_.child([&]{call_(s_,target,pc);});}
public:
    using Data=NativeTu15SolverHullLine<__m128>;
    static constexpr bool first_sum_before_sign=false;
    Frame(State&s,Call&call,Before&before,native_raw_x87_detail::Scope<State>&fp):Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void first_support(Data&){
        auto&g=s_.gp;push(0x3136a0u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x78u,4);
        push(0x3136a9u,g[6]);push(0x3136aau,g[7]);g[6]=g[1];at(0x3136adu);g[7]=word(g[6]+0x50u);
        g[0]=g[4]+0x20u;push(0x3136b4u,g[0]);push(0x3136b5u,g[7]);child(0x3136b6u,0x313390u,0x3136bbu);
    }
    __m128 vector(P phase,Data&d){
        std::uint32_t pc{},address{};unsigned reg{};
        switch(phase){
        case P::first_vertex:at(0x3136bbu);s_.gp[1]=word(s_.gp[6]+0x50u);pc=0x3136beu;address=s_.gp[1];reg=1;break;
        case P::first_transform:pc=0x3136c1u;address=s_.gp[4]+0x40u;break;
        case P::first_point:pc=0x3136c6u;address=s_.gp[4]+0x20u;reg=2;break;
        case P::sign:
            s_.xmm[0]=d.squared;s_.xmm[1]=d.xy;s_.xmm[2]=d.z;s_.gp[0]=s_.gp[4]+0x50u;
            pc=0x3136edu;address=0x461960u;break;
        case P::direction:s_.xmm[2]=d.score;pc=0x3136f8u;address=s_.gp[7];reg=1;break;
        case P::second_transform:pc=0x31371cu;address=s_.gp[4]+0x70u;break;
        case P::second_point:pc=0x313724u;address=s_.gp[4]+0x50u;reg=2;break;
        case P::second_vertex:pc=0x313729u;address=s_.gp[2];reg=1;break;
        }
        return s_.xmm[reg]=Base::vector(pc,address);
    }
    __m128 subtract(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    __m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    __m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    template<unsigned Mask>__m128 shuffle(__m128 a,__m128 b){return _mm_shuffle_ps(a,b,Mask);}
    __m128 xor_bits(__m128 a,__m128 b){return _mm_xor_ps(a,b);}
    void prepare_second(Data&){auto&g=s_.gp;push(0x3136fbu,g[0]);g[1]=g[4]+0x14u;g[2]=g[4]+0xcu;push(0x313704u,g[1]);}
    void second_support(const Data&d){
        auto&g=s_.gp;s_.xmm[1]=d.negative;g[1]=g[6];store(0x31370au,g[2],low(s_.xmm[2]));
        store_vector(0x31370eu,g[4]+0x18u,s_.xmm[1]);child(0x313713u,0x313390u,0x313718u);
    }
    void load_first_score(const Data&){
        at(0x313718u);const auto bits=word(s_.gp[4]+8u);fp_.require_capacity(1u);
        asm volatile("flds %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void second_vertex_owner(const Data&){at(0x313721u);s_.gp[2]=word(s_.gp[6]+0x50u);}
    bool compare_second(const Data&d){
        s_.xmm[0]=d.squared;s_.xmm[1]=d.xy;s_.xmm[2]=d.score;s_.gp[0]=s_.gp[4]+0xcu;
        store(0x313752u,s_.gp[0],low(s_.xmm[2]));at(0x313756u);const auto bits=word(s_.gp[4]+0xcu);std::uint16_t status{};
        asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(bits):"memory","st");fp_.publish();
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|status;s_.flags.logic((s_.gp[0]>>8u)&1u,1u);return !s_.flags.zf;
    }
    void append(const Data&d){
        auto&g=s_.gp;if(d.second){g[2]=g[4]+0x50u;push(0x313777u,g[2]);g[1]=g[6];child(0x31377au,0x313290u,0x31377fu);}
        else{g[1]=g[4]+0x20u;push(0x313765u,g[1]);g[1]=g[6];child(0x313768u,0x313290u,0x31376du);}
    }
    void finish(const Data&d){auto&g=s_.gp;g[7]=pop(d.second?0x31377fu:0x31376du);g[6]=pop(d.second?0x313780u:0x31376eu);g[4]=g[5];g[5]=pop(d.second?0x313783u:0x313771u);pop(d.second?0x313784u:0x313772u);}
};
template<class State,class Call,class Before=Noop>void run_private_owner(State&s,Call call,Before before={}){
    if(!native_raw_x87_detail::supported(s)||s.fp.count>7u)throw std::runtime_error("Private hull-line requires dense masked FP with one free slot");
    native_raw_x87_detail::Scope fp{s};Frame<State,Call,Before>view{s,call,before,fp};typename decltype(view)::Data data;
    native_tu15_solver_hull_line_kernel(view,data);
}
} // namespace halo2::native_solver_hull_line_detail
#endif
