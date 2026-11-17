#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_barycentric_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_barycentric_detail {
using Edge=NativeTu15HullDegenerateEdge;
enum class Scalar {load,multiply,divide,subtract,compare_pop};
enum class Stack {duplicate,discard,subtract_pop,subtract_st1,multiply_st1,multiply_st2};

template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    static __m128 sub(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static std::uint32_t bits(__m128 a){std::uint32_t value;std::memcpy(&value,&a,4u);return value;}
    static __m128 y(__m128 a){return _mm_shuffle_ps(a,a,85);}
    static __m128 z(__m128 a){return _mm_shuffle_ps(a,a,170);}
    template<unsigned Y,unsigned Z> void dot_sum() {
        auto& x=s_.xmm;x[Y]=add(y(x[0]),x[0]);x[Z]=add(z(x[0]),x[Y]);
    }
    void scalar(std::uint32_t pc,std::uint32_t address,Scalar op){io_.scalar(pc,address,op);}
    void stack(Stack op){io_.stack(op);}
    void status(){auto& g=s_.gp;g[0]=(g[0]&0xffff0000u)|io_.status();}
    void test(std::uint32_t mask){s_.flags.logic((s_.gp[0]>>8u)&mask,1u);}
    void finish(std::uint32_t popped,std::uint32_t returned) {
        auto& g=s_.gp;g[4]=g[5];io_.pop(popped,5u);io_.ret(returned);
    }
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    bool measure_triangle() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.push(0x3122f0u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4u);
        g[4]=s_.flags.minus(g[4],32u,4u);
        io_.load(0x3122f9u,1u,g[5]+12u);io_.load(0x3122fcu,0u,g[5]+16u);
        io_.load_vector(0x3122ffu,3u,g[0]);io_.load_vector(0x312302u,4u,g[1]);
        io_.load(0x312305u,2u,g[5]+20u);io_.load_vector(0x312308u,5u,g[2]);
        x[1]=sub(x[4],x[3]);x[0]=mul(x[1],x[1]);dot_sum<6,7>();
        x[2]=sub(x[5],x[3]);g[0]=g[4]+8u;io_.store(0x312337u,g[0],bits(x[7]));
        x[0]=mul(x[2],x[2]);dot_sum<6,7>();g[1]=g[4]+16u;
        io_.store(0x31235bu,g[1],bits(x[7]));
        scalar(0x31235fu,g[4]+16u,Scalar::load);scalar(0x312363u,g[4]+8u,Scalar::multiply);
        x[0]=mul(x[2],x[1]);dot_sum<6,7>();g[0]=g[4]+12u;
        io_.store(0x312387u,g[0],bits(x[7]));
        scalar(0x31238bu,g[4]+12u,Scalar::load);scalar(0x31238fu,g[4]+12u,Scalar::multiply);
        stack(Stack::subtract_pop);io_.spill(0x312395u,g[4]+20u,false);
        scalar(0x312399u,0x46a0e8u,Scalar::compare_pop);status();test(0x41u);return s_.flags.zf;
    }
    void project_triangle() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.load(0x3123aau,1u,g[5]+8u);
        scalar(0x3123adu,0x46a0d0u,Scalar::load);scalar(0x3123b3u,g[4]+20u,Scalar::divide);
        io_.load_vector(0x3123b7u,0u,g[1]);x[3]=sub(x[3],x[0]);
        x[0]=mul(x[3],x[1]);x[1]=add(y(x[0]),x[0]);x[3]=mul(x[3],x[2]);
        x[4]=z(x[0]);x[0]=x[3];x[4]=add(x[4],x[1]);dot_sum<1,2>();g[0]=g[4]+28u;
        io_.store(0x3123f9u,g[0],bits(x[2]));
        io_.load(0x3123fdu,0u,g[5]+24u);g[2]=g[4]+24u;io_.store(0x312404u,g[2],bits(x[4]));
        scalar(0x312408u,g[4]+28u,Scalar::load);scalar(0x31240cu,g[4]+12u,Scalar::multiply);
        scalar(0x312410u,g[4]+24u,Scalar::load);scalar(0x312414u,g[4]+16u,Scalar::multiply);
        stack(Stack::subtract_pop);
        scalar(0x31241au,g[4]+12u,Scalar::load);scalar(0x31241eu,g[4]+24u,Scalar::multiply);
        scalar(0x312422u,g[4]+28u,Scalar::load);scalar(0x312426u,g[4]+8u,Scalar::multiply);
        stack(Stack::subtract_pop);io_.spill(0x31242cu,g[4]+28u,true);
        stack(Stack::duplicate);stack(Stack::multiply_st2);io_.spill(0x312434u,g[0],true);
        scalar(0x312436u,g[4]+20u,Scalar::load);stack(Stack::subtract_st1);
        scalar(0x31243cu,g[4]+28u,Scalar::subtract);stack(Stack::multiply_st2);io_.spill(0x312442u,g[0]+4u,true);
        stack(Stack::discard);scalar(0x312447u,g[4]+28u,Scalar::load);
        stack(Stack::multiply_st1);io_.spill(0x31244du,g[0]+8u,true);stack(Stack::discard);
        finish(0x312454u,0x312455u);
    }
    Edge select_degenerate_edge() {
        auto& g=s_.gp;auto& x=s_.xmm;
        scalar(0x312456u,g[4]+8u,Scalar::load);x[4]=sub(x[4],x[5]);
        scalar(0x31245du,g[4]+16u,Scalar::compare_pop);
        x[0]=mul(x[4],x[4]);x[6]=y(x[0]);x[7]=x[0];status();
        x[6]=add(x[6],x[0]);test(1u);x[7]=z(x[0]);g[1]=g[4]+12u;x[7]=add(x[7],x[6]);
        io_.store(0x312486u,g[1],bits(x[7]));scalar(0x31248au,g[4]+12u,Scalar::load);
        if(!s_.flags.zf) {
            scalar(0x312567u,g[4]+16u,Scalar::compare_pop);status();test(1u);
            return s_.flags.zf?Edge::bc_duplicate_ratio:Edge::ac;
        }
        scalar(0x312494u,g[4]+8u,Scalar::compare_pop);status();test(1u);
        if(!s_.flags.zf)return Edge::ab;
        scalar(0x31249fu,g[4]+12u,Scalar::load);io_.load(0x3124a3u,1u,g[5]+24u);
        scalar(0x3124a6u,0x46a0e8u,Scalar::compare_pop);io_.store(0x3124acu,g[1]+4u,0u);
        status();test(0x41u);return s_.flags.zf?Edge::bc_saved_ratio:Edge::zero;
    }
    void project_bc_saved() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.load_vector(0x3124bau,0u,g[2]);io_.load(0x3124bdu,2u,g[5]+8u);io_.load_vector(0x3124c0u,1u,g[2]);
        x[1]=sub(x[1],x[0]);x[0]=mul(x[1],x[4]);dot_sum<1,2>();g[0]=g[4]+28u;
        io_.store(0x3124e6u,g[0],bits(x[2]));
        scalar(0x3124eau,g[4]+28u,Scalar::load);scalar(0x3124eeu,g[4]+12u,Scalar::divide);
        io_.spill(0x3124f2u,g[1],false);scalar(0x3124f4u,0x46a0d0u,Scalar::load);
        stack(Stack::subtract_st1);io_.spill(0x3124fcu,g[1]+8u,true);stack(Stack::discard);
        finish(0x312503u,0x312504u);
    }
    void zero_weights() {
        auto& g=s_.gp;io_.store(0x312505u,g[1],0u);io_.store(0x31250bu,g[1]+8u,0u);
        finish(0x312514u,0x312515u);
    }
    void project_ab() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.load(0x312516u,1u,g[5]+8u);io_.load_vector(0x312519u,0u,g[1]);
        x[0]=mul(sub(x[0],x[3]),x[1]);dot_sum<1,2>();g[2]=g[4]+28u;
        io_.store(0x31253cu,g[2],bits(x[2]));
        scalar(0x312540u,g[4]+28u,Scalar::load);scalar(0x312544u,g[4]+8u,Scalar::divide);
        io_.load(0x312548u,0u,g[5]+24u);io_.store(0x31254bu,g[0]+8u,0u);
        stack(Stack::duplicate);io_.spill(0x312554u,g[0],true);
        scalar(0x312556u,0x46a0d0u,Scalar::load);stack(Stack::subtract_st1);
        io_.spill(0x31255eu,g[0]+4u,true);stack(Stack::discard);finish(0x312565u,0x312566u);
    }
    void project_bc_duplicate() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.load(0x312572u,0u,g[5]+8u);io_.load_vector(0x312575u,0u,g[0]);
        x[0]=mul(sub(x[0],x[5]),x[4]);dot_sum<1,2>();g[1]=g[4]+28u;
        io_.store(0x312598u,g[1],bits(x[2]));
        scalar(0x31259cu,g[4]+28u,Scalar::load);scalar(0x3125a0u,g[4]+12u,Scalar::divide);
        io_.load(0x3125a4u,0u,g[5]+24u);io_.store(0x3125a7u,g[0]+4u,0u);
        stack(Stack::duplicate);io_.spill(0x3125b0u,g[0],true);
        scalar(0x3125b2u,0x46a0d0u,Scalar::load);stack(Stack::subtract_st1);
        io_.spill(0x3125bau,g[0]+8u,true);stack(Stack::discard);finish(0x3125c1u,0x3125c2u);
    }
    void project_ac() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.load(0x3125c3u,2u,g[5]+8u);io_.load_vector(0x3125c6u,0u,g[2]);
        x[0]=mul(sub(x[0],x[3]),x[2]);dot_sum<1,2>();g[0]=g[4]+28u;
        io_.store(0x3125e9u,g[0],bits(x[2]));
        scalar(0x3125edu,g[4]+28u,Scalar::load);scalar(0x3125f1u,g[4]+16u,Scalar::divide);
        io_.load(0x3125f5u,0u,g[5]+24u);io_.store(0x3125f8u,g[0],0u);
        io_.spill(0x3125feu,g[0]+8u,false);
        scalar(0x312601u,0x46a0d0u,Scalar::load);stack(Stack::subtract_st1);
        io_.spill(0x312609u,g[0]+4u,true);stack(Stack::discard);finish(0x312610u,0x312611u);
    }
};

template<class State,class Before> class PhysicalIo:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>& fp_;
public:
    using Base::push;
    PhysicalIo(State& s,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);}
    void load_vector(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);if(address&15u)throw std::runtime_error("Unaligned original hull barycentric MOVAPS load");
        std::array<std::byte,16> bytes{};
        fp_.io([&]{s_.memory.read_vector(address,bytes);});std::memcpy(&s_.xmm[reg],bytes.data(),16u);
    }
    void pop(std::uint32_t pc,unsigned reg){s_.gp[reg]=Base::pop(pc);}
    void ret(std::uint32_t pc){Base::pop(pc);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){Base::store(pc,address,value,[]{});}
    void scalar(std::uint32_t pc,std::uint32_t address,Scalar op) {
        at(pc);const auto bits=word(address);
        switch(op) {
        case Scalar::load: fp_.require_capacity(1u);asm volatile("flds %0"::"m"(bits):"memory","st");break;
        case Scalar::multiply: asm volatile("fmuls %0"::"m"(bits):"memory","st");break;
        case Scalar::divide: asm volatile("fdivs %0"::"m"(bits):"memory","st");break;
        case Scalar::subtract: asm volatile("fsubs %0"::"m"(bits):"memory","st");break;
        case Scalar::compare_pop: asm volatile("fcomps %0"::"m"(bits):"memory","st");break;
        }
        fp_.publish();
    }
    void stack(Stack op) {
        switch(op) {
        case Stack::duplicate:fp_.require_capacity(1u);asm volatile("fld %%st(0)":::"memory","st");break;
        case Stack::discard:asm volatile("fstp %%st(0)":::"memory","st");break;
        case Stack::subtract_pop:
            // Authored DE E9: ST(1) - ST(0), then pop. Use the exact
            // opcode because the AT&T mnemonic reverses the named forms.
            asm volatile(".byte 0xde,0xe9":::"memory","st","st(1)");break;
        case Stack::subtract_st1:asm volatile("fsub %%st(1),%%st(0)":::"memory","st");break;
        case Stack::multiply_st1:asm volatile("fmul %%st(1),%%st(0)":::"memory","st");break;
        case Stack::multiply_st2:asm volatile("fmul %%st(2),%%st(0)":::"memory","st");break;
        }
        fp_.publish();
    }
    std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
    void spill(std::uint32_t pc,std::uint32_t address,bool pop) {
        at(pc);const auto pending=fp_.stage_single(pop);
        Base::store(pc,address,pending.bits,[&]{fp_.commit(pending);},true);
    }
};
template<class State,class Before> void run_private_owner(State& s,Before before) {
    if(!native_raw_x87_detail::supported(s)||s.fp.count>4u)
        throw std::runtime_error("Private hull barycentric projection requires four free dense masked x87 slots");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before> io{s,before,fp};
    Schedule<State,decltype(io)> view{s,io};native_tu15_solver_hull_barycentric_kernel(view);
}
} // namespace halo2::native_solver_hull_barycentric_detail
#endif
