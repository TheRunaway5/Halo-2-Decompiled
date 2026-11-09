#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_mopp_ray_plane_kernel.hpp"

namespace halo2::native_mopp_ray_caller_detail {
namespace plane_abi {
struct Sites {
    std::uint32_t low,high,write_low,read_low,write_high,spill_low,read_high,spill_high;
    std::array<std::array<std::uint32_t,3>,2>endpoint;
};
inline constexpr std::array<Sites,10>projected{{
    {0x30aaae,0x30aab2,0x30aab8,0x30aabe,0x30aac2,0x30aac6,0x30aaca,0x30aace,{{{0x30aad2,0x30aad5,0},{0x30aad8,0x30aadb,0}}}},
    {0x30aae3,0x30aae7,0x30aaf2,0x30aafd,0x30ab01,0x30ab05,0x30ab09,0x30ab0d,{{{0x30ab11,0x30ab14,0},{0x30ab17,0x30ab1a,0}}}},
    {0x30ab22,0x30ab26,0x30ab2c,0x30ab32,0x30ab36,0x30ab3a,0x30ab3e,0x30ab42,{{{0x30ab46,0x30ab49,0},{0x30ab4b,0x30ab4e,0}}}},
    {0x30ab56,0x30ab5a,0x30ab65,0x30ab70,0x30ab74,0x30ab78,0x30ab7c,0x30ab80,{{{0x30ab84,0x30ab86,0},{0x30ab89,0x30ab8c,0}}}},
    {0x30ab94,0x30ab98,0x30ab9e,0x30aba4,0x30aba8,0x30abac,0x30abb0,0x30abb4,{{{0x30abb8,0x30abbb,0},{0x30abbd,0x30abc0,0}}}},
    {0x30abc8,0x30abcc,0x30abd7,0x30abe2,0x30abe6,0x30abea,0x30abee,0x30abf2,{{{0x30abf6,0x30abf8,0},{0x30abfb,0x30abfe,0}}}},
    {0x30ac06,0x30ac0d,0x30ac11,0x30ac18,0x30ac1c,0x30ac20,0x30ac24,0x30ac28,{{{0x30ac2c,0x30ac2f,0x30ac32},{0x30ac34,0x30ac37,0x30ac3a}}}},
    {0x30ac42,0x30ac4c,0x30ac50,0x30ac5a,0x30ac5e,0x30ac62,0x30ac66,0x30ac6a,{{{0x30ac6e,0x30ac71,0x30ac73},{0x30ac76,0x30ac79,0x30ac7c}}}},
    {0x30ac84,0x30ac92,0x30ac8e,0x30ac96,0x30aca4,0x30aca0,0x30aca8,0x30acac,{{{0x30acb0,0x30acb2,0x30acb5},{0x30acb8,0x30acbb,0x30acbe}}}},
    {0x30acc6,0x30acd2,0x30acd6,0x30ace2,0x30ace6,0x30acea,0x30acee,0x30acf2,{{{0x30acf6,0x30acf8,0x30acfb},{0x30acfe,0x30ad01,0x30ad04}}}},
}};
inline constexpr Sites axis{0x30ad9b,0x30ad9f,0x30ada3,0x30adae,0x30adb2,0x30adb6,0x30adba,0x30adbe,{{{0x30adc2,0,0},{0x30adc5,0,0}}}};
inline constexpr Sites thin{0x30ad0c,0,0x30ad14,0x30ad1e,0,0x30ad3a,0,0x30ad2c,{{{0x30ad3e,0,0},{0x30ad41,0,0}}}};
inline constexpr Sites wide{0x30ad4a,0x30ad4e,0x30ad52,0x30ad5a,0x30ad61,0x30ad69,0x30ad6d,0x30ad78,{{{0x30ad7c,0,0},{0x30ad7f,0,0}}}};
}

// Private caller view, not a production parent. The shared DATA/numerical core
// owns the split planes. This view publishes only their retained ABI and lazy
// ordinary memory operations; numerical decisions never inspect GP or flags.
template<class State,class Before>class PlaneFrame:public IntegerFrame<State,Before,ScopeIo<State>> {
    using Base=IntegerFrame<State,Before,ScopeIo<State>>;
    using Progress=native_mopp_ray_plane_detail::Progress;
    using Family=native_mopp_ray_plane_detail::Family;
    using Read=native_mopp_ray_plane_detail::Read;
    using Write=native_mopp_ray_plane_detail::Write;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>&scope_;
    bool low_published_{},high_published_{};
    const plane_abi::Sites&sites(const Progress&d)const{
        if(d.family==Family::projected)return plane_abi::projected[d.opcode-0x13u];
        if(d.family==Family::axis)return plane_abi::axis;
        return d.family==Family::thin_axis?plane_abi::thin:plane_abi::wide;
    }
    void publish_low(const Progress&d){
        if(low_published_)return;low_published_=true;auto&g=s_.gp;
        if(d.family!=Family::projected)return;
        if(d.opcode<=0x18u){
            if(d.opcode&1u)g[2]=native_mopp_aabb_abi_detail::shifted_immediate<1>(s_,g[2]);
            else g[2]=2u*g[1]-255u;
        }else if(d.opcode==0x19u)g[2]=3u*g[0];
        else{
            g[0]=s_.flags.minus(g[0],d.opcode==0x1cu?170u:85u,4);
            if(d.opcode==0x1au)g[1]=3u*g[0];
            else if(d.opcode==0x1bu)g[0]*=3u;
            else g[2]=3u*g[0];
        }
    }
    void publish_high(const Progress&d){
        if(high_published_)return;high_published_=true;auto&g=s_.gp;
        if(d.family!=Family::projected)return;
        if(d.opcode<=0x18u){
            if(d.opcode&1u)g[0]=native_mopp_aabb_abi_detail::shifted_immediate<1>(s_,g[0]);
            else g[1]=2u*g[0]-255u;
        }else if(d.opcode==0x19u)g[0]*=3u;
        else{
            g[0]=s_.flags.minus(g[0],d.opcode==0x1cu?170u:85u,4);
            if(d.opcode==0x1au)g[2]=3u*g[0];
            else if(d.opcode==0x1bu)g[1]=3u*g[0];
            else g[0]*=3u;
        }
    }
public:
    PlaneFrame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope)
        :Base{s,before,ScopeIo<State>{&scope}},scope_(scope){}
    std::uint32_t scalar(Read kind,const Progress&d){
        auto&g=s_.gp;const auto&p=sites(d);
        switch(kind){
        case Read::low:{
            low_published_=high_published_=false;
            const unsigned reg=d.family!=Family::projected?1u:d.opcode>=0x19u?0u:(d.opcode&1u)?2u:1u;
            return Base::load(reg,p.low,d.program+1u,1);
        }
        case Read::high:
            if(d.family==Family::projected&&d.opcode>=0x19u)publish_low(d);
            return Base::load(d.family==Family::projected?0:2,p.high,d.program+2u,1);
        case Read::scratch_low:
            if(d.opcode!=0x1bu)publish_high(d);
            at(p.read_low);return word(g[4]+0x14u);
        case Read::scratch_high:at(p.read_high);return word(g[4]+0x14u);
        case Read::left_low:return Base::load(1,0x30ad56u,d.program+4u,1);
        case Read::right_low:return Base::load(2,0x30ad65u,d.program+6u,1);
        case Read::left_high:return Base::load(0,0x30ad83u,d.program+3u,1);
        case Read::right_high:return Base::load(0,0x30ad90u,d.program+5u,1);
        case Read::right_short:
            return d.family==Family::thin_axis?Base::load(2,0x30ad10u,d.program+2u,1):Base::load(0,0x30adc9u,d.program+3u,1);
        case Read::left_offset:
            if(d.family==Family::long_axis){g[0]=native_mopp_aabb_abi_detail::shifted_immediate<8>(s_,g[0]);g[0]=s_.flags.plus(g[0],g[2],4);}
            return Base::load(1,0x30add8u,g[4]+0x24u);
        case Read::thin_delta:at(0x30ad34u);return word(0x46a0d0u);
        }
        throw std::logic_error("Unknown split-plane DATA source");
    }
    void store(Write kind,std::uint32_t value,const Progress&d){
        auto&g=s_.gp;const auto&p=sites(d);std::uint32_t pc{},offset{};
        switch(kind){
        case Write::scratch_low:publish_low(d);pc=p.write_low;offset=0x14u;break;
        case Write::scratch_high:
            if(d.family==Family::long_axis)g[0]=s_.flags.plus(g[0],0xffffffddu,4);
            pc=p.write_high;offset=0x14u;break;
        case Write::axis:
            if(d.family==Family::axis)g[0]=s_.flags.plus(g[0],0xfffffff0u,4);
            else if(d.family==Family::thin_axis)g[1]=s_.flags.logic(0u,4);
            pc=d.family==Family::axis?0x30adaau:d.family==Family::thin_axis?0x30ad24u:0x30ad71u;offset=0x3cu;break;
        case Write::left_offset:
            if(d.family==Family::long_axis){g[0]=native_mopp_aabb_abi_detail::shifted_immediate<8>(s_,g[0]);g[0]=s_.flags.plus(g[0],g[1],4);}
            pc=d.family==Family::long_axis?0x30ad8cu:d.family==Family::thin_axis?0x30ad28u:0x30add0u;offset=0x24u;break;
        case Write::right_scratch:pc=d.family==Family::thin_axis?0x30ad30u:0x30addcu;offset=0x14u;break;
        default:throw std::logic_error("Non-integer split-plane DATA destination");
        }
        Base::store(pc,g[4]+offset,value,[]{});
    }
    std::uint32_t spill(Write kind,bool pop,const Progress&d){
        const auto&p=sites(d);std::uint32_t pc{},offset{};
        if(kind==Write::low_plane){if(d.opcode==0x1bu)publish_high(d);pc=p.spill_low;offset=0x20u;}
        else if(kind==Write::high_plane){pc=p.spill_high;offset=0x1cu;}
        else if(kind==Write::end_projection){pc=0x30ade0u;offset=0x18u;}
        else throw std::logic_error("Unknown split-plane spill");
        at(pc);const auto pending=scope_.stage_single(pop);
        Base::store(pc,s_.gp[4]+offset,pending.bits,[&]{scope_.commit(pending);},true);return pending.bits;
    }
    std::uint32_t endpoint(unsigned axis,unsigned term,const Progress&d){
        at(sites(d).endpoint[d.endpoint][term]);return word(s_.gp[7]+16u*d.endpoint+4u*axis);
    }
    void integer(std::uint32_t value){scope_.require_capacity(1);native_mopp_ray_detail::integer(std::bit_cast<std::int32_t>(value));}
    void floating(std::uint32_t value){scope_.require_capacity(1);native_mopp_ray_detail::push(value);}
    void add_scalar(std::uint32_t value){native_mopp_ray_detail::add(value);}
    void subtract_scalar(std::uint32_t value){native_mopp_ray_detail::subtract(value);}
    void advance(std::uint32_t,const Progress&d){
        auto&g=s_.gp;const auto count=d.family==Family::long_axis?7u:d.family==Family::thin_axis?3u:4u;
        g[6]=s_.flags.plus(g[6],count,4);
        if(d.family==Family::thin_axis)g[0]=s_.flags.plus(g[0],0xffffffe0u,4);
    }
    void before_split(){at(0x30ade4u);}
};
}
