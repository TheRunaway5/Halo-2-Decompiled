#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_compact_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"

namespace halo2::native_simplex_hull_compact_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    static constexpr std::uint32_t absolute_mask=0x461950u,zero=0x48ca00u;
    void acquire(std::uint32_t pc,unsigned reg,std::uint32_t address){s_.gp[reg]=io_.word(pc,address);}
    void relative(std::uint32_t pc,unsigned reg,std::uint32_t offset){io_.vector(pc,reg,s_.gp[6]+offset);}
    static void subtract(__m128& a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");}
    static void multiply(__m128& a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");}
    static void add(__m128& a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");}
    bool distinct(unsigned lane,unsigned reg) {
        auto& x=s_.xmm;asm volatile("cmpps $1,%1,%0":"+x"(x[lane]):"x"(x[1]):"memory");
        s_.gp[reg]=static_cast<std::uint32_t>(_mm_movemask_ps(x[lane]));s_.flags.logic(s_.gp[reg]&7u,1u);
        return !s_.flags.zf;
    }
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void compact_first_shape() {
        auto& g=s_.gp;auto& x=s_.xmm;
        relative(0x30e0a3u,4u,0x30u);relative(0x30e0a7u,3u,0x40u);x[0]=x[4];subtract(x[0],x[3]);
        io_.store(0x30e0b1u,g[7],3u);io_.store(0x30e0b7u,g[6],3u);io_.vector(0x30e0bdu,5u,absolute_mask);
        x[1]=x[0];io_.vector(0x30e0c7u,0u,zero);x[1]=_mm_and_ps(x[1],x[5]);x[2]=x[0];
        bool different=distinct(2u,0u);
        if(different){relative(0x30e0dfu,1u,0x20u);subtract(x[1],x[3]);x[1]=_mm_and_ps(x[1],x[5]);x[2]=x[0];different=distinct(2u,0u);}
        if(!different){io_.store(0x30e0f7u,g[6],2u);io_.vector(0x30e0fdu,0u,zero);}
        relative(0x30e104u,1u,0x20u);subtract(x[1],x[4]);x[1]=_mm_and_ps(x[1],io_.mask(0x30e10bu,absolute_mask));
        x[2]=x[0];if(distinct(2u,0u))return;
        acquire(0x30e120u,0u,g[6]);g[0]=s_.flags.increment(g[0],4u,true);io_.store(0x30e123u,g[6],g[0]);
        g[0]=s_.flags.plus(g[0],2u,4u);io_.shift_index(0u);io_.vector(0x30e12bu,0u,g[0]+g[6]);
        g[0]=s_.flags.plus(g[0],g[6],4u);io_.store_vector(0x30e131u,g[6]+0x20u,x[0]);io_.vector(0x30e135u,0u,zero);
    }
    void compact_second_shape() {
        auto& g=s_.gp;auto& x=s_.xmm;
        relative(0x30e13cu,4u,0xf0u);relative(0x30e143u,3u,0x100u);io_.vector(0x30e14au,5u,absolute_mask);
        x[1]=x[4];subtract(x[1],x[3]);x[1]=_mm_and_ps(x[1],x[5]);x[2]=x[0];bool different=distinct(2u,0u);
        if(different){relative(0x30e168u,1u,0xe0u);subtract(x[1],x[3]);x[1]=_mm_and_ps(x[1],x[5]);x[2]=x[0];different=distinct(2u,0u);}
        if(!different){io_.decrement(0x30e183u,g[7]);io_.vector(0x30e185u,0u,zero);}
        acquire(0x30e18cu,0u,g[7]);s_.flags.minus(g[0],2u,4u);if(s_.flags.sf!=s_.flags.of)return;
        relative(0x30e193u,1u,0xe0u);subtract(x[1],x[4]);x[1]=_mm_and_ps(x[1],io_.mask(0x30e19du,absolute_mask));
        if(distinct(0u,1u))return;
        g[0]=s_.flags.increment(g[0],4u,true);io_.store(0x30e1b1u,g[7],g[0]);
        g[0]=s_.flags.plus(g[0],0xeu,4u);io_.shift_index(0u);io_.vector(0x30e1b9u,0u,g[0]+g[6]);
        g[0]=s_.flags.plus(g[0],g[6],4u);io_.store_vector(0x30e1bfu,g[6]+0xe0u,x[0]);
    }
    bool begin_transform() {
        auto& g=s_.gp;
        acquire(0x30e1c6u,1u,g[7]);g[1]=s_.flags.increment(g[1],4u,true);g[0]=g[6]+0xe0u;io_.store(0x30e1cfu,g[4]+0x18u,g[1]);
        if(s_.flags.sf)return false;
        acquire(0x30e1d5u,2u,g[4]+0x18u);g[1]=s_.flags.minus(g[6]+0xa0u,g[0],4u);g[2]=s_.flags.increment(g[2],4u,false);
        return true;
    }
    void transform_row() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.vector(0x30e1e2u,0u,g[0]);io_.vector(0x30e1e5u,1u,g[3]);io_.vector(0x30e1e8u,3u,g[3]+0x10u);
        x[2]=_mm_shuffle_ps(x[0],x[0],0);multiply(x[1],x[2]);x[2]=_mm_shuffle_ps(x[0],x[0],0x55);multiply(x[3],x[2]);
        x[2]=_mm_shuffle_ps(x[0],x[0],0xaa);io_.vector(0x30e207u,0u,g[3]+0x20u);multiply(x[0],x[2]);
        io_.vector(0x30e20eu,2u,g[3]+0x30u);add(x[0],x[2]);add(x[1],x[3]);add(x[1],x[0]);
        io_.store_vector(0x30e21bu,g[1]+g[0],x[1]);
    }
    bool next_row() {
        auto& g=s_.gp;g[0]=s_.flags.plus(g[0],0x10u,4u);g[2]=s_.flags.increment(g[2],4u,true);return !s_.flags.zf;
    }
    void reduce_boundary_if_needed() {
        auto& g=s_.gp;acquire(0x30e225u,2u,g[7]);g[2]=s_.flags.plus(g[2],io_.word(0x30e227u,g[6]),4u);
        s_.flags.minus(g[2],4u,4u);if(s_.flags.zf||s_.flags.sf!=s_.flags.of)return;
        g[1]=g[6];io_.call(0x30e230u,0x30e235u,0x30dc10u);
    }
};
template<class State,class Before,class Call> class PhysicalIo:
    public native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,Call> {
    using Base=native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,Call>;
    using Foundation=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    State& s_;
public:
    PhysicalIo(State& s,Before& before,native_raw_x87_detail::Scope<State>& fp,Call& call)
        :Base{s,before,fp,call},s_(s){}
    __m128 mask(std::uint32_t pc,std::uint32_t address) {
        Foundation::at(pc);if(address&15u)throw std::runtime_error("Unaligned original simplex ANDPS source");
        std::array<std::byte,16> bytes{};Foundation::io([&]{s_.memory.read_vector(address,bytes);});
        __m128 value;std::memcpy(&value,bytes.data(),16u);return value;
    }
};
template<class State,class Call,class Before> void run_private_hull_compact(State& s,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>6u)
        throw std::runtime_error("Private simplex hull compaction requires supported actual TU1.5 with free x87 slots");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_simplex_hull_compact_kernel(view);
}
} // namespace halo2::native_simplex_hull_compact_detail
#endif
