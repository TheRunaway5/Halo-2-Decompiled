#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_boundary_projection_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"

namespace halo2::native_solver_hull_boundary_projection_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    void read(std::uint32_t pc,unsigned reg,std::uint32_t address){s_.gp[reg]=io_.word(pc,address);}
    void load_vector(std::uint32_t pc,std::uint32_t address){io_.vector(pc,0u,address);}
    void write_vector(std::uint32_t pc,std::uint32_t address){io_.store_vector(pc,address,s_.xmm[0]);}
    bool compare_weight(std::uint32_t acquire,std::uint32_t compare,std::uint32_t slot,bool release_arguments) {
        auto& g=s_.gp;
        io_.load_float(acquire,g[4]+slot);io_.compare_float(compare,0x46a0e8u);
        if(release_arguments)g[4]=s_.flags.plus(g[4],0x14u,4u);
        g[0]=(g[0]&0xffff0000u)|io_.status();s_.flags.logic((g[0]>>8u)&5u,1u);
        return s_.flags.pf;
    }
    void shifted_feature_offset() {
        auto& g=s_.gp;
        auto value=g[0];auto flags=((s_.flags.bits()&0xd5u)<<8u)|((s_.flags.bits()>>11u)&1u);unsigned overflow{};
        asm volatile("addb $127,%%al;sahf;sarl $5,%2;shll $4,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+&a"(flags),"=c"(overflow),"+&r"(value)::"cc");
        g[0]=value;s_.flags.load((flags&0xd5u)|((overflow&1u)<<11u));
    }
    void feature_offset(unsigned base_register) {
        auto& g=s_.gp;g[0]=s_.flags.minus(g[0],g[base_register],4u);shifted_feature_offset();
    }
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void copy_boundary_vertices() {
        auto& g=s_.gp;
        read(0x3144a0u,0u,g[7]+0x14u);load_vector(0x3144a3u,g[0]);
        read(0x3144a6u,1u,g[6]+0x50u);write_vector(0x3144a9u,g[1]);
        read(0x3144acu,1u,g[7]+0x24u);read(0x3144afu,0u,g[6]+0x50u);load_vector(0x3144b2u,g[1]);
        g[0]=s_.flags.plus(g[0],0x10u,4u);write_vector(0x3144b8u,g[0]);
        read(0x3144bbu,1u,g[7]+0x34u);read(0x3144beu,0u,g[6]+0x50u);load_vector(0x3144c1u,g[1]);
        g[0]=s_.flags.plus(g[0],0x20u,4u);write_vector(0x3144c7u,g[0]);
    }
    void compute_weights() {
        auto& g=s_.gp;
        read(0x3144cau,0u,g[6]+0x50u);g[1]=g[4]+0x34u;io_.push(0x3144d1u,g[1]);
        g[2]=g[0]+0x20u;io_.push(0x3144d5u,g[2]);g[1]=g[0]+0x10u;io_.push(0x3144d9u,g[1]);
        io_.push(0x3144dau,g[0]);io_.push(0x3144dbu,0x48ca00u);io_.call(0x3144e0u,0x3144e5u,0x3122f0u);
    }
    bool weights_inside_boundary() {
        return compare_weight(0x3144e5u,0x3144e9u,0x48u,true)&&
               compare_weight(0x3144f9u,0x3144fdu,0x38u,false)&&
               compare_weight(0x31450au,0x31450eu,0x3cu,false);
    }
    bool advance_facet() {
        auto& g=s_.gp;
        io_.push(0x31451bu,g[7]);g[1]=g[4]+0x824u;io_.call(0x314523u,0x314528u,0x314b20u);
        g[3]=s_.flags.increment(g[3],4u,false);s_.flags.minus(g[3],10u,4u);g[7]=g[0];
        return s_.flags.sf!=s_.flags.of;
    }
    void publish_selected_features() {
        auto& g=s_.gp;
        read(0x314534u,0u,g[7]+0x14u);read(0x314537u,2u,g[4]+0x2920u);read(0x31453eu,1u,g[6]+0x48u);
        g[0]=s_.flags.minus(g[0],g[2],4u);read(0x314543u,2u,g[4]+0x4a0u);
        // The first subtraction precedes acquisition of the two source
        // arrays. Later pairs retain their separate live base reads.
        shifted_feature_offset();load_vector(0x314550u,g[0]+g[2]);write_vector(0x314554u,g[1]);
        read(0x314557u,1u,g[6]+0x48u);read(0x31455au,2u,g[4]+0x120u);
        load_vector(0x314561u,g[0]+g[2]);write_vector(0x314565u,g[1]+0x40u);

        read(0x314569u,0u,g[7]+0x24u);read(0x31456cu,3u,g[4]+0x2920u);read(0x314573u,2u,g[4]+0x4a0u);
        g[1]=s_.flags.plus(g[1],0x40u,4u);read(0x31457du,1u,g[6]+0x48u);feature_offset(3u);
        load_vector(0x314588u,g[0]+g[2]);write_vector(0x31458cu,g[1]+0x10u);
        read(0x314590u,2u,g[4]+0x120u);load_vector(0x314597u,g[0]+g[2]);
        g[1]=s_.flags.plus(g[1],0x10u,4u);read(0x31459eu,1u,g[6]+0x48u);write_vector(0x3145a1u,g[1]+0x50u);

        read(0x3145a5u,0u,g[7]+0x34u);read(0x3145a8u,3u,g[4]+0x2920u);read(0x3145afu,2u,g[4]+0x4a0u);
        g[1]=s_.flags.plus(g[1],0x50u,4u);read(0x3145b9u,1u,g[6]+0x48u);feature_offset(3u);
        load_vector(0x3145c4u,g[0]+g[2]);write_vector(0x3145c8u,g[1]+0x20u);
        read(0x3145ccu,2u,g[4]+0x120u);load_vector(0x3145d3u,g[0]+g[2]);
        g[1]=s_.flags.plus(g[1],0x20u,4u);read(0x3145dau,1u,g[6]+0x48u);write_vector(0x3145ddu,g[1]+0x60u);
        read(0x3145e1u,0u,g[6]+0x48u);io_.store(0x3145e4u,g[0]+0x80u,3u);
    }
};

template<class State,class Call,class Before> void run_private_boundary_projection(State& s,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>4u)
        throw std::runtime_error("Private hull boundary projection requires a supported actual TU1.5 frame");
    native_raw_x87_detail::Scope fp{s};native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_solver_hull_boundary_projection_kernel(view);
}
} // namespace halo2::native_solver_hull_boundary_projection_detail
#endif
