#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_face_refine_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

namespace halo2::native_simplex_face_refine_detail {
using Data=NativeTu15SimplexFaceRefine;
using P=NativeTu15SimplexFaceRefinePhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Before>class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::word;using Base::at;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;
    std::uint16_t comparison(std::uint32_t load_pc,std::uint32_t compare_pc,std::uint32_t address){
        at(load_pc);const auto value=word(address);fp_.require_capacity(1u);native_simplex_projection_numeric::load(value);
        at(compare_pc);const auto zero=word(0x46a0e8u);std::uint16_t status;
        asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(zero):"memory","st");
        s_.set(0,status,2,0);return status;
    }
    void publish(__m128 a,__m128 b,__m128 c,__m128 d,__m128 e,__m128 f,__m128 g){auto&x=s_.xmm;x[0]=a;x[1]=b;x[2]=c;x[3]=d;x[4]=e;x[5]=f;x[6]=g;}
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp):Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp){}
    void begin(Data&d){
        auto&g=s_.gp;d.vertices=g[7];push(0x30d1a0u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x18u,4);
        push(0x30d1a9u,g[3]);at(0x30d1aau);d.scores=g[3]=word(g[5]+12u);push(0x30d1adu,g[6]);g[6]=s_.flags.logic(g[6]^g[6],4);
        Base::store(0x30d1b0u,g[4]+12u,g[7],[]{});
    }
    std::uint32_t index(P phase,const Data&,std::uint32_t address){
        const bool second=phase==P::second_index;at(second?0x30d1c7u:0x30d1c0u);return s_.gp[second?2u:1u]=std::uint32_t(std::int32_t(std::int8_t(word(address,1))));
    }
    bool nonnegative(const Data&d,bool second){
        const auto status=comparison(second?0x30d1e2u:0x30d1ceu,second?0x30d1e5u:0x30d1d1u,d.scores+(second?d.second:d.first)*4u);
        s_.flags.logic((status>>8u)&1u,1);return !(status&0x100u);
    }
    std::uint32_t cursor(const Data&){at(0x30d1f6u);return s_.gp[0]=word(s_.gp[4]+12u);}
    __m128 vector_read(P phase,const Data&d,std::uint32_t address){
        std::uint32_t pc;unsigned reg;
        switch(phase){
        case P::origin:pc=0x30d1fau;reg=0;break;
        case P::apex:pc=0x30d1fdu;reg=2;break;
        case P::first_vertex:pc=0x30d206u;reg=3;s_.gp[0]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,d.first);break;
        case P::second_vertex:pc=0x30d215u;reg=4;s_.xmm[3]=d.first_direction;s_.xmm[2]=d.apex_delta;s_.gp[0]=native_mopp_aabb_abi_detail::shifted_immediate<4>(s_,d.second);break;
        case P::point:pc=0x30d21cu;reg=1;break;
        default:throw std::runtime_error("Invalid face-refine vector phase");
        }
        const auto value=Base::vector(pc,address);s_.xmm[reg]=value;return value;
    }
    std::uint32_t point_argument(const Data&){at(0x30d219u);return s_.gp[0]=word(s_.gp[5]+8u);}
    void score_write(P phase,const Data&d,std::uint32_t bits){
        std::uint32_t pc,offset;
        switch(phase){
        case P::along_first:pc=0x30d25eu;offset=0x14u;publish(d.second_xy,d.second_products,d.apex_delta,d.first_direction,d.second_direction,d.second_sum,d.first_sum);break;
        case P::along_second:pc=0x30d277u;offset=0x1cu;s_.xmm[0]=d.apex_first_products;s_.xmm[1]=d.apex_first_xy;break;
        case P::apex_first:pc=0x30d28du;offset=0x18u;s_.xmm[2]=d.apex_second_products;s_.xmm[3]=d.apex_first_sum;break;
        case P::apex_second:pc=0x30d2abu;offset=0x10u;s_.xmm[0]=d.apex_second_xy;s_.xmm[1]=d.apex_second_sum;break;
        default:throw std::runtime_error("Invalid face-refine score publication");
        }
        s_.gp[0]=s_.gp[4]+offset;Base::store(pc,s_.gp[0],bits,[]{});
    }
    bool compare_products(const Data&){
        namespace X=native_simplex_projection_numeric;
        at(0x30d2afu);const auto a=word(s_.gp[4]+0x10u);fp_.require_capacity(2u);X::load(a);
        at(0x30d2b3u);X::multiply(word(s_.gp[4]+0x14u));
        at(0x30d2b7u);X::load(word(s_.gp[4]+0x18u));
        at(0x30d2bbu);X::multiply(word(s_.gp[4]+0x1cu));
        const auto status=X::compare_pop_two();s_.set(0,status,2,0);s_.flags.logic((status>>8u)&0x41u,1);return !(status&0x4100u);
    }
    void invalidate(P phase,const Data&d){const bool second=phase==P::invalidate_second;Base::store(second?0x30d2c8u:0x30d2d1u,d.scores+(second?d.second:d.first)*4u,0xbf800000u,[]{});}
    void advance(const Data&){
        auto&g=s_.gp;at(0x30d2d8u);g[1]=word(g[4]+12u);g[6]=s_.flags.increment(g[6],4,false);g[1]=s_.flags.plus(g[1],16u,4);s_.flags.minus(g[6],3u,4);Base::store(0x30d2e3u,g[4]+12u,g[1],[]{});
    }
    bool positive(const Data&d,unsigned face){
        constexpr std::array loads{0x30d2edu,0x30d304u,0x30d31fu},compares{0x30d2efu,0x30d307u,0x30d322u};
        const auto status=comparison(loads[face],compares[face],d.scores+face*4u);s_.flags.logic((status>>8u)&0x41u,1);return !(status&0x4100u);
    }
    void finish(std::uint32_t face){
        auto&g=s_.gp;
        if(face==0xffffffffu){g[6]=pop(0x30d33au);g[0]=s_.flags.logic(g[0]|0xffffffffu,4);g[3]=pop(0x30d33eu);g[4]=g[5];g[5]=pop(0x30d341u);pop(0x30d342u);return;}
        if(!face)g[0]=s_.flags.logic(g[0]^g[0],4);else g[0]=face;
        constexpr std::array si{0x30d2feu,0x30d319u,0x30d334u},bx{0x30d2ffu,0x30d31au,0x30d335u},bp{0x30d302u,0x30d31du,0x30d338u},ret{0x30d303u,0x30d31eu,0x30d339u};
        g[6]=pop(si[face]);g[3]=pop(bx[face]);g[4]=g[5];g[5]=pop(bp[face]);pop(ret[face]);
    }
};
template<class State,class Before=Noop>
std::uint32_t run_private_owner(State&s,Before before={}){
    native_raw_x87_detail::Scope<State>fp{s};Frame<State,Before>memory{s,before,fp};Data data;
    return native_tu15_refine_simplex_faces(memory,data);
}
} // namespace halo2::native_simplex_face_refine_detail
#endif
