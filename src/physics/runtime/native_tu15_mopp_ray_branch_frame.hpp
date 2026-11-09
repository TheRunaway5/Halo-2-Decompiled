#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_mopp_ray_branch_kernel.hpp"

namespace halo2::native_mopp_ray_caller_detail {
template<class State,class Before>class BranchFrame:public IntegerFrame<State,Before,ScopeIo<State>> {
    using Base=IntegerFrame<State,Before,ScopeIo<State>>;
    using Position=native_mopp_ray_branch_detail::Comparison;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>&scope_;
public:
    BranchFrame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope)
        :Base{s,before,ScopeIo<State>{&scope}},scope_(scope){}
    std::uint32_t end_projection(bool later){at(later?0x30ae66u:0x30ade4u);return word(s_.gp[4]+0x18u);}
    native_mopp_ray_detail::Comparison compare(Position position){
        const auto pc=position==Position::end_high?0x30ade8u:position==Position::start_high?0x30adf3u:
            position==Position::start_low?0x30ae55u:0x30ae6au;
        at(pc);const auto plane=word(s_.gp[4]+(position==Position::end_high||position==Position::start_high?0x1cu:0x20u));
        return native_mopp_ray_detail::compare(plane,position==Position::end_high||position==Position::end_low);
    }
    void publish_comparison(Position position,native_mopp_ray_detail::Comparison value){
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|value.status;
        const auto mask=position==Position::end_high||position==Position::start_high?5u:0x41u;
        s_.flags.logic((value.status>>8u)&mask,1);
    }
    std::uint32_t left_offset()const{return s_.gp[1];}
    std::uint32_t right_offset(){return Base::load(2,0x30ae59u,s_.gp[4]+0x14u);}
    void advance_left(std::uint32_t){s_.gp[6]=s_.flags.plus(s_.gp[6],s_.gp[1],4);}
    void advance_right(std::uint32_t){s_.gp[6]=s_.flags.plus(s_.gp[6],s_.gp[2],4);}
    void floating(std::uint32_t value){scope_.require_capacity(1);native_mopp_ray_detail::push(value);}
    void discard(){native_mopp_ray_detail::discard();}
    void before_continuation(native_mopp_ray_branch_detail::Decision result){
        at(result==native_mopp_ray_branch_detail::Decision::continue_program?0x30ae02u:0x30ae75u);
    }
};
}
