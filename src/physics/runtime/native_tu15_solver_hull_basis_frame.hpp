#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_basis_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

namespace halo2::native_solver_hull_basis_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};

// Private physical-publication view. Each I/O publishes the escaped SIMD
// state and the logical x87 stack. Child dispatch consumes the real call ABI
// and leaves the actual resumed registers in State. This adapter does not
// select retained-source publication or qualify MMIO/partial vector faults.
template<class State,class Call,class Before>class Frame:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;
    Call&call_;

    static std::uint32_t low(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
    static __m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
    static __m128 sub(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 sub_scalar(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 mul_scalar(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 estimate(__m128 a,__m128 b){asm volatile("rsqrtss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    template<unsigned mask>static __m128 shuffle(__m128 value){return _mm_shuffle_ps(value,value,mask);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){Base::store(pc,address,value,[]{});}
    void vector_store(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{pc,address,flags,flags|0x10000u};}
        std::array<std::byte,16>data{};std::memcpy(data.data(),&value,16);
        for(unsigned offset:{0u,8u}){
            const auto bytes=std::span(data).subspan(offset,8);SourcePhysicsContactRecordStore event{pc,address+offset,{},{}};
            if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}
            fp_.io([&]{s_.memory.write(event.address,bytes);});
            if(s_.observe){event.written.assign(bytes.begin(),bytes.end());fp_.io([&]{s_.observe(event);});}
        }
    }
    std::uint32_t load(std::uint32_t pc,std::uint32_t address){at(pc);return word(address);}
    void load_float(std::uint32_t pc,std::uint32_t address){
        const auto bits=load(pc,address);fp_.require_capacity(1u);
        asm volatile("flds %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void compare_stack(){asm volatile("fcomp %%st(1)":::"memory","st");fp_.publish();}
    void drop(){asm volatile("fstp %%st(0)":::"memory","st");fp_.publish();}
    void absolute(){asm volatile("fabs":::"memory","st");fp_.publish();}
    void save_float(std::uint32_t pc,std::uint32_t address,bool pop_value){
        at(pc);const auto pending=fp_.stage_single(pop_value);
        Base::store(pc,address,pending.bits,[&]{fp_.commit(pending);},true);
    }
    void status_to_ax(){std::uint16_t status{};asm volatile("fnstsw %0":"=a"(status));s_.set(0,status,2,0);}
    void child(std::uint32_t pc,std::uint32_t target,std::uint32_t returned){push(pc,returned);fp_.child([&]{call_(s_,target,pc);});}

public:
    Frame(State&s,Call&call,Before&before,native_raw_x87_detail::Scope<State>&fp):
        Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void open(){
        auto&g=s_.gp;push(0x313790u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);
        g[4]=s_.flags.minus(g[4],0xb8u,4);push(0x31379cu,g[6]);g[6]=g[1];
    }
    void measure_axes(){
        auto&g=s_.gp;auto&x=s_.xmm;
        g[0]=load(0x31379fu,g[6]+0x50u);
        x[1]=Base::vector(0x3137a2u,g[0]+0x10u);x[0]=Base::vector(0x3137a6u,g[0]);
        x[0]=sub(x[0],x[1]);vector_store(0x3137acu,g[4]+0xcu,x[0]);
        load_float(0x3137b1u,g[4]+0xcu);absolute();g[1]=s_.flags.logic(0,4);
        load_float(0x3137b9u,g[4]+0x10u);absolute();push(0x3137bfu,g[7]);
        save_float(0x3137c0u,g[4]+0x2cu,false);
        load_float(0x3137c4u,g[4]+0x18u);g[2]=1u;absolute();g[7]=2u;
        save_float(0x3137d4u,g[4]+0xcu,true);
    }
    bool first_axis_kept(){compare_stack();status_to_ax();s_.flags.logic((s_.gp[0]>>8u)&5u,1);return s_.flags.pf;}
    void promote_y(){
        drop();auto&g=s_.gp;g[2]=s_.flags.logic(0,4);load_float(0x3137e5u,g[4]+0x2cu);g[1]=1u;
    }
    bool middle_axis_kept(){
        load_float(0x3137eeu,s_.gp[4]+0xcu);compare_stack();status_to_ax();drop();
        s_.flags.logic((s_.gp[0]>>8u)&5u,1);return s_.flags.pf;
    }
    void promote_z(){s_.gp[7]=s_.gp[1];s_.gp[1]=2u;}
    void build_perpendiculars(){
        auto&g=s_.gp;auto&x=s_.xmm;
        load_float(0x313804u,g[4]+0x10u+g[2]*4u);g[0]=load(0x313808u,g[4]+0x10u+g[7]*4u);
        x[3]=x[0];asm volatile("fchs":::"memory","st");fp_.publish();
        x[3]=shuffle<210>(x[3]);x[4]=shuffle<201>(x[0]);
        store(0x31381cu,g[4]+0x30u+g[1]*4u,0u);store(0x313824u,g[4]+0x30u+g[2]*4u,g[0]);
        save_float(0x313828u,g[4]+0x30u+g[7]*4u,true);
        x[1]=Base::vector(0x31382cu,g[4]+0x30u);
        x[2]=shuffle<201>(x[1]);x[3]=mul(x[3],x[2]);x[2]=shuffle<210>(x[1]);
        x[0]=mul(x[4],x[2]);x[2]=sub(x[0],x[3]);
    }
    void normalize_first(){
        auto&g=s_.gp;auto&x=s_.xmm;
        x[0]=mul(x[1],x[1]);x[3]=add_scalar(shuffle<85>(x[0]),x[0]);x[4]=shuffle<170>(x[0]);
        g[2]=load(0x313866u,g[6]+0x50u);
        x[0]=add_scalar(x[4],x[3]);vector_store(0x313870u,g[4]+0x10u,x[0]);
        x[3]=estimate(x[3],x[0]);store(0x313879u,g[4]+0x10u,low(x[3]));
        x[5]=Base::vector(0x31387fu,g[4]+0x10u);x[0]=mul_scalar(mul_scalar(x[0],x[5]),x[5]);
        store(0x31388cu,g[4]+0xcu,0x40400000u);x[3]=scalar(load(0x313894u,g[4]+0xcu));
        x[6]=sub_scalar(x[3],x[0]);store(0x3138a1u,g[4]+0xcu,0x3f000000u);
        x[4]=scalar(load(0x3138a9u,g[4]+0xcu));x[0]=mul_scalar(mul_scalar(x[4],x[5]),x[6]);
        x[5]=mul(shuffle<0>(x[0]),x[1]);
    }
    void normalize_second_and_evaluate_first(){
        auto&g=s_.gp;auto&x=s_.xmm;
        x[0]=mul(x[2],x[2]);x[1]=add_scalar(shuffle<85>(x[0]),x[0]);
        vector_store(0x3138d5u,g[4]+0x30u,x[5]);
        x[5]=shuffle<170>(x[0]);x[0]=add_scalar(x[5],x[1]);
        vector_store(0x3138e8u,g[4]+0x10u,x[0]);
        x[1]=estimate(x[1],x[0]);store(0x3138f1u,g[4]+0x10u,low(x[1]));
        x[1]=Base::vector(0x3138f7u,g[4]+0x10u);
        x[0]=mul_scalar(mul_scalar(x[0],x[1]),x[1]);x[3]=sub_scalar(x[3],x[0]);
        g[1]=g[4]+0x40u;push(0x31390cu,g[1]);
        x[4]=mul_scalar(mul_scalar(x[4],x[1]),x[3]);x[0]=x[4];
        push(0x313918u,g[2]);g[0]=g[4]+0x38u;x[0]=mul(shuffle<0>(x[0]),x[2]);
        push(0x313924u,g[0]);g[1]=g[6];vector_store(0x313927u,g[4]+0x1cu,x[0]);
        child(0x31392cu,0x313470u,0x313931u);
    }
    void evaluate_second(){
        auto&g=s_.gp;g[2]=load(0x313931u,g[6]+0x50u);
        g[1]=g[4]+0x80u;push(0x31393bu,g[1]);push(0x31393cu,g[2]);g[0]=g[4]+0x18u;
        push(0x313941u,g[0]);g[1]=g[6];child(0x313944u,0x313470u,0x313949u);
    }
    bool select_second(){
        auto&g=s_.gp;load_float(0x313949u,g[4]+0x70u);const auto score=load(0x31394du,g[4]+0xb0u);
        asm volatile("fcomps %0"::"m"(score):"memory","st");fp_.publish();status_to_ax();
        s_.flags.logic((g[0]>>8u)&0x41u,1);return !s_.flags.zf;
    }
    void append(bool second){
        auto&g=s_.gp;
        if(second){g[2]=g[4]+0x80u;push(0x313974u,g[2]);g[1]=g[6];child(0x313977u,0x313290u,0x31397cu);}
        else{g[1]=g[4]+0x40u;push(0x31395fu,g[1]);g[1]=g[6];child(0x313962u,0x313290u,0x313967u);}
    }
    void finish(bool second){
        auto&g=s_.gp;g[7]=pop(second?0x31397cu:0x313967u);g[6]=pop(second?0x31397du:0x313968u);
        g[4]=g[5];g[5]=pop(second?0x313980u:0x31396bu);pop(second?0x313981u:0x31396cu);
    }
};

template<class State,class Call,class Before=Noop>void run_private_owner(State&s,Call call,Before before={}){
    if(!native_raw_x87_detail::supported(s)||s.fp.count>5u)throw std::runtime_error("Private hull basis requires dense masked FP with three free slots");
    native_raw_x87_detail::Scope fp{s};Frame<State,Call,Before>view{s,call,before,fp};
    native_tu15_solver_hull_basis_kernel(view);
}
} // namespace halo2::native_solver_hull_basis_detail
#endif
