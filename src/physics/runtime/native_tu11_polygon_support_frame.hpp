#pragma once
#include "physics/runtime/native_tu15_polygon_support_frame.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)

namespace halo2::native_polygon_support_legacy_detail {
// MapPack1.1 has the same support geometry and caller layout, with its own
// authenticated bound constant and canonical instruction namespace.
template<class State,class Before>class Frame:public native_polygon_support_detail::Frame<State,Before,false> {
    using Base=native_polygon_support_detail::Frame<State,Before,false>;
    State&s_;Before&before_;native_raw_x87_detail::Scope<State>&scope_;
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&scope):Base{s,before,scope},s_(s),before_(before),scope_(scope){}
    native_polygon_support_detail::Vector initial_best(){
        constexpr auto pc=0x1f1d2au;
        if(s_.instruction_cursor&&s_.instruction_cursor->position)*s_.instruction_cursor->position=pc;
        scope_.io([&]{before_(s_,pc);});std::array<std::byte,4>bytes{};
        scope_.io([&]{s_.memory.read(0x447020u,bytes);});std::uint32_t bits{};std::memcpy(&bits,bytes.data(),4);
        const native_polygon_support_detail::Vector value{bits,0,0,0};s_.xmm[1]=std::bit_cast<__m128>(value);return value;
    }
};
template<class State,class Before>void support_retained(State&s,Before&before){
    native_raw_x87_detail::Scope scope{s};Frame frame{s,before,scope};native_polygon_support_detail::Progress data;
    native_polygon_support_detail::support(frame,data);
}
template<class State>bool admitted(const State&s){
    if(native_polygon_support_detail::reference()||s.image!=SourcePhysicsContactImage::MapPack11||!native_raw_x87_detail::supported(s))return false;
    if(!s.services.instruction&&(s.instruction_cursor||s.observe))return false;
    if(s.services.instruction){
        const auto*p=s.instruction_cursor;
        if(!s.literal_service_sites||!p||p!=&s.services.instruction_cursor||!p->position||!p->callback_type||
           s.services.instruction.target_type()!=*p->callback_type||p->executor!=std::this_thread::get_id()||
           !p->literal_service_sites||p->service_sites.size()!=3||p->service_sites[0]!=0x38846eu||
           p->service_sites[1]!=0x21f590u||p->service_sites[2]!=0x33b595u)return false;
        if((p->first_trace&&*p->first_trace)||(p->second_trace&&*p->second_trace))return false;
    }
    if(s.observe&&(!s.instruction_cursor||!s.instruction_cursor->observation_only_stores))return false;
    return true;
}
template<class State>bool bridge(State&s){
    if(!admitted(s))return false;native_polygon_support_detail::Noop before;native_polygon_support_legacy_detail::support_retained(s,before);return true;
}
template<class State>__m128 ordered_product(State&,__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
template<class State>__m128 ordered_sum(State&,__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
template<class State>std::uint32_t ordered_shift(State&s,std::uint32_t value){return native_mopp_aabb_abi_detail::shifted_immediate<4>(s,value);}
template<class State>void ordered_store(State&s,std::uint32_t pc,std::uint32_t address,__m128 value){
    if(!native_raw_x87_detail::supported(s)){physics_step_detail::vw(s.memory,s.observe,pc,address,value);return;}
    native_raw_x87_detail::Scope scope{s};native_polygon_support_detail::Noop before;Frame frame{s,before,scope};frame.complete_vector_store(pc,address,value);
}
}
#else
namespace halo2::native_polygon_support_legacy_detail {
template<class State>bool bridge(State&){return false;}
template<class State>__m128 ordered_product(State&s,__m128 left,__m128 right){return native_polygon_support_detail::ordered_product(s,left,right);}
template<class State>__m128 ordered_sum(State&s,__m128 left,__m128 right){return native_polygon_support_detail::ordered_sum(s,left,right);}
template<class State>std::uint32_t ordered_shift(State&s,std::uint32_t value){return native_polygon_support_detail::ordered_shift(s,value);}
template<class State>void ordered_store(State&s,std::uint32_t pc,std::uint32_t address,__m128 value){native_polygon_support_detail::ordered_store(s,pc,address,value);}
}
#endif
