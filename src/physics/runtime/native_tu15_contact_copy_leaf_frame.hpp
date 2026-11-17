#pragma once
#include "halo2/native_tu15_contact_copy_leaf_kernel.hpp"
#include "halo2/native_tu15_contact_agent_dispatch.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include "physics/runtime/native_tu15_mopp_ray_integer_frame.hpp"

namespace halo2::native_contact_copy_leaf_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class Scope>struct Io{Scope*scope;template<class Call>void operator()(Call&&call)const{scope->io(std::forward<Call>(call));}};
template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,Io<native_integer_fp_detail::Scope<State>>>{
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::io;using Base::vector_store;using Base::pop;
    bool canonical_;
    std::uint32_t site(std::uint32_t value)const{return canonical_?value:value+0x3aa0u;}
    static std::uint32_t read_site(unsigned field){
        switch(field){case 0:return 0x300106u;case 0x10:return 0x30010cu;
        case 0x20:return 0x300114u;case 0x24:return 0x30011au;
        case 0x30:return 0x300120u;case 0x40:return 0x300128u;
        case 0x50:return 0x300130u;case 0x60:return 0x300138u;
        case 0x70:return 0x300140u;case 0x74:return 0x300146u;case 0x78:return 0x30014cu;
        case 0x7c:return 0x300152u;case 0x7d:return 0x300158u;}
        throw std::logic_error("Foreign contact-copy field");
    }
    static std::uint32_t write_site(unsigned field){
        switch(field){case 0:return 0x300109u;case 0x10:return 0x300110u;
        case 0x20:return 0x300117u;case 0x24:return 0x30011du;
        case 0x30:return 0x300124u;case 0x40:return 0x30012cu;
        case 0x50:return 0x300134u;case 0x60:return 0x30013cu;
        case 0x70:return 0x300143u;case 0x74:return 0x300149u;case 0x78:return 0x30014fu;
        case 0x7c:return 0x300155u;case 0x7d:return 0x30015bu;}
        throw std::logic_error("Foreign contact-copy publication");
    }
    void publish(unsigned field,std::uint32_t value,unsigned width){
        const auto pc=site(write_site(field)),address=s_.gp[0]+field;at(pc);
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);io([&]{s_.observe(event);});}
    }
public:
    Frame(State&s,Before&before,Scope&scope,bool canonical):Base{s,before,{&scope}},canonical_(canonical){}
    void begin(){s_.gp[0]=s_.gp[1];at(site(0x300102u));s_.gp[1]=Base::word(s_.gp[4]+4u);}
    NativeTu15ContactRecordVector vector(NativeTu15ContactRecord,unsigned field){
        s_.xmm[0]=Base::vector(site(read_site(field)),s_.gp[1]+field);
        return std::bit_cast<NativeTu15ContactRecordVector>(s_.xmm[0]);
    }
    std::uint32_t word(NativeTu15ContactRecord,unsigned field){
        at(site(read_site(field)));return s_.gp[2]=Base::word(s_.gp[1]+field);
    }
    std::uint8_t byte(NativeTu15ContactRecord,unsigned field){
        at(site(read_site(field)));const auto value=Base::word(s_.gp[1]+field,1);
        const auto reg=field==0x7c?2u:1u;s_.gp[reg]=(s_.gp[reg]&0xffffff00u)|value;
        return static_cast<std::uint8_t>(value);
    }
    void vector(NativeTu15ContactRecord,unsigned field,const NativeTu15ContactRecordVector&value,NativeTu15ContactRecordPublication){
        vector_store(site(write_site(field)),s_.gp[0]+field,std::bit_cast<std::remove_cvref_t<decltype(s_.xmm[0])>>(value));
    }
    void word(NativeTu15ContactRecord,unsigned field,std::uint32_t value,NativeTu15ContactRecordPublication){publish(field,value,4);}
    void byte(NativeTu15ContactRecord,unsigned field,std::uint8_t value,NativeTu15ContactRecordPublication){publish(field,value,1);}
    void finish(){pop(site(0x30015eu));s_.gp[4]+=4u;}
};
template<class State,class Before=Noop>void run_copy_private(State&s,Before before={},bool canonical=false){
    native_integer_fp_detail::Scope scope{s};Frame frame{s,before,scope,canonical};frame.begin();
    copy(frame,NativeTu15ContactRecord{s.gp[0]},NativeTu15ContactRecord{s.gp[1]});frame.finish();
}
template<class State,class Before=Noop>void run_invalid_private(State&s,Before before={},bool canonical=false){
    native_integer_fp_detail::Scope scope{s};Io<native_integer_fp_detail::Scope<State>>io{&scope};
    s.gp[0]=native_tu15_invalid_contact_agent().identity;
    const auto pc=canonical?0x307fe5u:0x30bad5u;before(s,pc);
    if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;
    std::array<std::byte,4>word{};io([&]{s.memory.read(s.gp[4],word);});s.gp[4]+=4u;
}
} // namespace halo2::native_contact_copy_leaf_detail
#endif
