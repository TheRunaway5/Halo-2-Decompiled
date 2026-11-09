#pragma once
#include "halo2/native_tu15_simplex_projection_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

namespace halo2::native_simplex_projection_detail {
using Data=NativeTu15SimplexProjection;
using P=NativeTu15SimplexProjectionPhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Before>class Access:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>&fp_;
public:
    Access(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp){}
    void begin(const Data&){s_.flags.logic(s_.gp[0],4u);}
    __m128 vector_read(P phase,const Data&d,std::uint32_t address){
        std::uint32_t pc;unsigned reg;
        switch(phase){
        case P::axis:pc=0x30edd7u;reg=0u;break;
        case P::first:
            s_.gp[3]=d.work+0x120u;s_.xmm[0]=d.squares;s_.xmm[1]=d.length_xy;s_.xmm[2]=d.length_z;
            pc=0x30edf9u;reg=0u;break;
        case P::fresh_axis:s_.xmm[2]=d.length;pc=0x30ee01u;reg=1u;break;
        case P::second:pc=0x30ee0cu;reg=2u;break;
        default:throw std::runtime_error("Invalid simplex projection vector phase");
        }
        s_.xmm[reg]=Base::vector(pc,address);return s_.xmm[reg];
    }
    void word_write(P phase,const Data&d,std::uint32_t value){
        std::uint32_t pc,address;
        switch(phase){
        case P::squared_length:
            s_.gp[1]=s_.gp[4]+0x24u;s_.xmm[2]=d.length;pc=0x30ee08u;address=s_.gp[1];break;
        case P::projection:
            s_.gp[2]=s_.gp[4]+0x18u;s_.xmm[0]=d.products;s_.xmm[1]=d.projection_xy;s_.xmm[2]=d.projection;
            pc=0x30ee36u;address=s_.gp[2];break;
        case P::parity:s_.gp[0]=s_.flags.logic(d.parity,4u);pc=0x30ee76u;address=s_.gp[4]+0x4cu;break;
        default:throw std::runtime_error("Invalid simplex projection scalar store");
        }
        Base::store(pc,address,value,[]{});
    }
    std::uint32_t scalar_read(P phase,const Data&){
        std::uint32_t pc,address;
        switch(phase){
        case P::projection_load:pc=0x30ee40u;address=s_.gp[4]+0x18u;break;
        case P::projection_square:pc=0x30ee44u;address=s_.gp[4]+0x18u;break;
        case P::projection_cube:pc=0x30ee48u;address=s_.gp[4]+0x18u;break;
        case P::epsilon_load:pc=0x30ee4cu;address=s_.gp[4]+0x14u;break;
        case P::threshold_scale:pc=0x30ee50u;address=0x41d658u;break;
        case P::threshold_length:pc=0x30ee56u;address=s_.gp[4]+0x24u;break;
        case P::length_load:pc=0x30ee67u;address=s_.gp[4]+0x24u;break;
        case P::iteration:pc=0x30ee6bu;address=s_.gp[4]+0x1cu;break;
        case P::tolerance_scale:pc=0x30ee6fu;address=s_.gp[4]+0x14u;break;
        default:throw std::runtime_error("Invalid simplex projection scalar read");
        }
        at(pc);const auto value=word(address);if(phase==P::iteration)s_.gp[0]=value;return value;
    }
    void require_capacity(unsigned extra){fp_.require_capacity(extra);}
    void relation(const Data&d){s_.set(0u,d.relation,2u,0u);s_.flags.logic((s_.reg(0u,1u,8u))&1u,1u);}
    void store_tolerance(const Data&){
        // CPU-write prestate and completed observer retirement are deliberately
        // distinct from the old modeled w(...);fp.pop() callback ordering.
        at(0x30ee7au);const auto pending=fp_.stage_single(true);
        Base::store(0x30ee7au,s_.gp[4]+0x10u,pending.bits,[&]{fp_.commit(pending);},true);
    }
    void complete(const Data&d){
        const auto pc=d.reducer_status?0x30ee3au:!(d.relation&0x100u)?0x30ee61u:0x30ee7eu;
        if(s_.instruction_cursor&&s_.instruction_cursor->position)*s_.instruction_cursor->position=pc;
    }
};
template<class State,class Before=Noop>
NativeTu15SimplexProjectionNext run_private_stage(State&s,Before before={}){
    native_raw_x87_detail::Scope<State>fp{s};Access<State,Before>memory{s,before,fp};
    Data data;data.work=s.gp[6];data.reducer_status=s.gp[0];
    return native_tu15_project_simplex_direction(memory,data);
}
} // namespace halo2::native_simplex_projection_detail
#endif
