#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_initialize_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
namespace halo2::native_solver_hull_initialize_detail {
using P=NativeTu15SolverHullInitializePhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
// Private complete89-site physical-publication candidate. No admission or
// default binding. Provider GP/SIMD mutation and partial vector writes remain
// outside this initial view until independently compared with the owner.
template<class State,class Call,class Before>class Frame:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;Call&call_;
    static std::uint32_t low(__m128 value){std::uint32_t bits{};std::memcpy(&bits,&value,4);return bits;}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){Base::store(pc,address,value,[]{});}
    void vector_store(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{pc,address,flags,flags|0x10000u};}
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);
        for(unsigned offset:{0u,8u}){SourcePhysicsContactRecordStore event{pc,address+offset,{},{}};
            if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}
            const auto part=std::span(bytes).subspan(offset,8);fp_.io([&]{s_.memory.write(event.address,part);});
            if(s_.observe){event.written.assign(part.begin(),part.end());fp_.io([&]{s_.observe(event);});}
        }
    }
    void load_float(std::uint32_t pc,std::uint32_t address){at(pc);const auto bits=word(address);fp_.require_capacity(1u);asm volatile("flds %0"::"m"(bits):"memory","st");fp_.publish();}
    bool compare_threshold(std::uint32_t pc,std::uint32_t address){
        at(pc);const auto bits=word(address);std::uint16_t status{};
        asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(bits):"memory","st");fp_.publish();
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|status;s_.flags.logic((s_.gp[0]>>8u)&0x41u,1);return s_.flags.pf;
    }
    void call(std::uint32_t pc,std::uint32_t target,std::uint32_t returned){push(pc,returned);fp_.child([&]{call_(s_,target,pc);});}
public:
    using Data=NativeTu15SolverHullInitialize<__m128>;
    Frame(State&s,Call&call,Before&before,native_raw_x87_detail::Scope<State>&fp):Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void begin(Data&d){auto&g=s_.gp;push(0x313560u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x5cu,4);d.context=g[1];}
    __m128 vector(P phase,Data&){
        std::uint32_t pc{},address{};unsigned reg{};
        switch(phase){
        case P::sign:pc=0x313569u;address=0x461960u;reg=1;break;
        case P::translation:push(0x313570u,s_.gp[6]);s_.gp[6]=s_.gp[1];pc=0x313573u;address=s_.gp[6]+0x30u;break;
        case P::difference:pc=0x3135f1u;address=s_.gp[6];break;
        case P::fresh_difference:pc=0x313626u;address=s_.gp[6];break;
        case P::rounded_length:pc=0x313657u;address=s_.gp[4]+0x20u;break;
        case P::direction:pc=0x313668u;address=s_.gp[4]+0x10u;break;
        case P::point:pc=0x313673u;address=s_.gp[4]+0x30u;break;
        }
        return s_.xmm[reg]=Base::vector(pc,address);
    }
    __m128 xor_bits(__m128 a,__m128 b){return _mm_xor_ps(a,b);}
    __m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    __m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    template<unsigned Mask>__m128 shuffle(__m128 a,__m128 b){return _mm_shuffle_ps(a,b,Mask);}
    __m128 square_root(__m128 a){asm volatile("sqrtss %0,%0":"+x"(a)::"memory");return a;}
    void save_direction(const Data&d){s_.xmm[0]=d.direction;vector_store(0x31357au,s_.gp[4]+0x10u,s_.xmm[0]);}
    bool initial_length_above_or_unordered(const Data&d){
        s_.xmm[0]=d.product;s_.xmm[1]=d.xy;s_.xmm[2]=d.length;s_.gp[0]=s_.gp[4]+0xcu;
        store(0x31359cu,s_.gp[0],low(s_.xmm[2]));load_float(0x3135a0u,s_.gp[4]+0xcu);return compare_threshold(0x3135a4u,0x46a180u);
    }
    void default_direction(const Data&){store(0x3135b1u,s_.gp[4]+0x10u,0u);store(0x3135b9u,s_.gp[4]+0x14u,0x3f800000u);store(0x3135c1u,s_.gp[4]+0x18u,0u);store(0x3135c9u,s_.gp[4]+0x1cu,0u);}
    void support(const Data&){auto&g=s_.gp;g[1]=g[4]+0x30u;push(0x3135d5u,g[1]);g[2]=g[4]+0x14u;push(0x3135dau,g[2]);g[1]=g[6];call(0x3135ddu,0x313390u,0x3135e2u);}
    void append(const Data&){auto&g=s_.gp;g[0]=g[4]+0x30u;push(0x3135e6u,g[0]);g[1]=g[6];call(0x3135e9u,0x313290u,0x3135eeu);}
    std::uint32_t differences_owner(const Data&){at(0x3135eeu);return s_.gp[6]=word(s_.gp[6]+0x50u);}
    bool difference_above_or_unordered(const Data&d){
        s_.xmm[0]=d.product;s_.xmm[1]=d.xy;s_.xmm[2]=d.length;s_.gp[1]=s_.gp[4]+0xcu;
        store(0x313611u,s_.gp[1],low(s_.xmm[2]));load_float(0x313615u,s_.gp[4]+0xcu);return compare_threshold(0x313619u,0x46a454u);
    }
    std::uint32_t output_owner(const Data&){at(0x313629u);return s_.gp[0]=word(s_.gp[5]+0xcu);}
    void save_square(const Data&d){s_.xmm[0]=d.square;s_.xmm[1]=d.squared_xy;s_.xmm[2]=d.squared_z;vector_store(0x313648u,s_.gp[4]+0x20u,s_.xmm[0]);}
    void save_root(const Data&d){s_.xmm[0]=d.root;store(0x313651u,s_.gp[4]+0x20u,low(s_.xmm[0]));}
    void save_and_load_length(const Data&){s_.gp[2]=s_.gp[4]+0xcu;store(0x313660u,s_.gp[2],low(s_.xmm[0]));load_float(0x313664u,s_.gp[4]+0xcu);}
    void output_length(const Data&){at(0x31366du);const auto pending=fp_.stage_single(true);Base::store(0x31366du,s_.gp[0]+0x20u,pending.bits,[&]{fp_.commit(pending);},true);}
    void output_direction(const Data&){vector_store(0x313670u,s_.gp[0],s_.xmm[0]);}
    void output_point(const Data&){vector_store(0x313678u,s_.gp[0]+0x10u,s_.xmm[0]);}
    void output_weight(const Data&){store(0x31367cu,s_.gp[0]+0x24u,0x3f000000u);}
    void output_flag(const Data&d){
        at(d.produced_plane?0x313683u:0x313690u);s_.gp[0]=word(s_.gp[5]+8u);const auto pc=d.produced_plane?0x313686u:0x313693u;at(pc);
        SourcePhysicsContactRecordStore event{pc,s_.gp[0],{},{}};if(s_.observe){event.before.resize(1);fp_.io([&]{s_.memory.read(event.address,event.before);});}
        const std::array bytes{std::byte(d.produced_plane?1u:0u)};fp_.io([&]{s_.memory.write(event.address,bytes);});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());fp_.io([&]{s_.observe(event);});}
    }
    void finish(const Data&d){auto&g=s_.gp;g[6]=pop(d.produced_plane?0x313689u:0x313696u);g[4]=g[5];g[5]=pop(d.produced_plane?0x31368cu:0x313699u);pop(d.produced_plane?0x31368du:0x31369au);g[4]+=8u;}
};
template<class State,class Call,class Before=Noop>void run_private_owner(State&s,Call call,Before before={}){
    if(!native_raw_x87_detail::supported(s)||s.fp.count>7u)throw std::runtime_error("Private hull-initialize requires dense masked FP with one free slot");
    native_raw_x87_detail::Scope fp{s};Frame<State,Call,Before>view{s,call,before,fp};typename decltype(view)::Data data;
    native_tu15_solver_hull_initialize_kernel(view,data);
}
} // namespace halo2::native_solver_hull_initialize_detail
#endif
