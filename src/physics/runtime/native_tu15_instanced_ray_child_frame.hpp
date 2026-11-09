#pragma once
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "halo2/native_tu15_instanced_ray_child_kernel.hpp"

namespace halo2::native_instanced_ray_child_detail {
template<class State,class Before>class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&scope}}{}
    std::uint32_t child(){at(0x24dec0u);return s_.gp[0]=word(s_.gp[4]+8u);}
    void test_child(std::uint32_t child){s_.flags.logic(child&child,4);}
    void null_child(){s_.gp[0]=s_.flags.logic(s_.gp[0]^s_.gp[0],4);}
    std::uint32_t scenario(){at(0x24dec8u);return s_.gp[1]=word(s_.gp[1]+12u);}
    std::uint32_t key(){at(0x24deceu);return s_.gp[1]=word(s_.gp[4]+4u);}
    void store(Field field,std::uint32_t value,const Progress&){
        const auto pc=field==Field::scenario?0x24decbu:field==Field::count?0x24ded2u:field==Field::flags?0x24ded8u:field==Field::table?0x24dedfu:0x24dee5u;
        const auto offset=field==Field::scenario?12u:field==Field::count?6u:field==Field::flags?8u:field==Field::table?0u:16u;
        const auto width=field==Field::count?2u:4u,address=s_.gp[0]+offset;at(pc);
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);Base::io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
        Base::io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);Base::io([&]{s_.observe(event);});}
    }
    void finish(bool child){pop(child?0x24dee8u:0x24deedu);s_.gp[4]+=8u;}
};
template<class State,class Before>void resolve_retained(State&s,Before&before){
    native_raw_x87_detail::Scope scope{s};Frame frame{s,before,scope};Progress data;
    const auto child=resolve(frame,data);frame.finish(child!=0u);
}
}
