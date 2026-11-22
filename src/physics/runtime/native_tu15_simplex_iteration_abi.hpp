#pragma once
#include "halo2/native_tu15_simplex_iteration_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_host_fp_io.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

namespace halo2::native_simplex_iteration_detail {
using Data=NativeTu15SimplexIteration;
using P=NativeTu15SimplexIterationPhase;
using Kind=NativeTu15SimplexReductionKind;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
struct RetainedIo {
    template<class Callback>void operator()(Callback&& callback)const{
        native_host_fp_io_detail::preserve(std::forward<Callback>(callback));
    }
};
template<class State,bool Retained>using Io=std::conditional_t<Retained,RetainedIo,native_mopp_ray_caller_detail::ScopeIo<State>>;
template<class State,class Before,bool Retained=false>class Access:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,Io<State,Retained>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,Io<State,Retained>>;
    using Scope=native_raw_x87_detail::Scope<State>;
    using Base::s_;using Base::at;using Base::word;using Base::io;
    Scope* scope_;
    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,bool positioned=false){
        if constexpr(Retained){
            if(!positioned)at(pc);
            if(!native_host_fp_io_detail::write_simplex_retained_word(s_,address,value))
                io([&]{physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);});
        }else Base::store(pc,address,value,[]{},positioned);
    }
public:
    Access(State&s,Before&before,Scope&fp)requires(!Retained):Base{s,before,Io<State,Retained>{&fp}},scope_(&fp){}
    Access(State&s,Before&before)requires(Retained):Base{s,before,Io<State,Retained>{}},scope_(nullptr){}
    std::uint32_t frame(P phase,const Data&)const{return s_.gp[4]+(phase==P::epsilon?0x14u:0x1cu);}
    std::uint32_t word_read(P phase,const Data&,std::uint32_t address){
        std::uint32_t pc;unsigned reg;
        switch(phase){
        case P::first_count:pc=0x30ea00u;reg=0;break;
        case P::epsilon:pc=0x30ea02u;reg=8;break;
        case P::second_count:pc=0x30ea06u;reg=3;break;
        case P::iteration:pc=0x30ea0au;reg=2;break;
        default:throw std::runtime_error("Invalid simplex iteration read");
        }
        at(pc);const auto value=word(address);if(reg!=8u)s_.gp[reg]=value;return value;
    }
    void require_capacity(unsigned extra){
        if constexpr(Retained){
            s_.require_x87();
            if(s_.fp.count+extra>8u)throw std::runtime_error("Retained simplex iteration exceeds declared x87 capacity");
        }else scope_->require_capacity(extra);
    }
    void load_epsilon(std::uint32_t bits){
        if constexpr(Retained)s_.fp.push(physics_step_detail::ext(physics_step_detail::f(bits)));
        else native_simplex_projection_numeric::load(bits);
    }
    void double_epsilon(){
        if constexpr(Retained)s_.fp.st(0)=physics_step_detail::add(s_.fp.st(0),s_.fp.st(0));
        else native_simplex_iteration_numeric::double_top();
    }
    void prepare_packed(const Data&d){s_.gp[0]=s_.flags.logic(d.packed,4u);}
    void store_epsilon(const Data&){
        at(0x30ea13u);
        if constexpr(Retained){
            const auto bits=physics_step_detail::u(physics_step_detail::spill(s_.fp.st(0)));
            write(0x30ea13u,s_.gp[4]+0x14u,bits,true);s_.fp.pop();
        }else{
            auto& fp_=*scope_;const auto pending=fp_.stage_single(true);
            Base::store(0x30ea13u,s_.gp[4]+0x14u,pending.bits,[&]{fp_.commit(pending);},true);
        }
    }
    void prepare_counter(const Data&){
        s_.gp[2]=s_.flags.increment(s_.gp[2],4u,false);
        s_.gp[0]=s_.flags.minus(s_.gp[0],9u,4u);s_.flags.minus(s_.gp[0],24u,4u);
    }
    void word_write(P phase,const Data&,std::uint32_t address,std::uint32_t value){
        write(phase==P::iteration_store?0x30ea1eu:0x30ea22u,address,value);
    }
    Kind classify(const Data&d){
        at(0x30ea30u);s_.gp[1]=word(0x30f70cu+d.key,1u);
        at(0x30ea37u);const auto target=word(0x30f6e0u+s_.gp[1]*4u);
        // These are geometric reduction kinds, not interpreted instructions.
        // Preserve both original table operands and reject unknown owners.
        switch(target){
        case 0x30f253u:return Kind::points;
        case 0x30f066u:return Kind::second_edge;
        case 0x30eb76u:return Kind::second_triangle;
        case 0x30eb31u:return Kind::second_face;
        case 0x30f175u:return Kind::first_edge;
        case 0x30ea8eu:return Kind::segments;
        case 0x30ea3eu:return Kind::paired_two_three;
        case 0x30ecd3u:return Kind::first_triangle;
        case 0x30ea5cu:return Kind::paired_three_two;
        case 0x30eca2u:return Kind::first_face;
        case 0x30edd0u:return Kind::degenerate;
        default:throw std::runtime_error("Required original simplex reduction owner unavailable");
        }
    }
    void complete(const Data&d){
        if(s_.instruction_cursor&&s_.instruction_cursor->position)*s_.instruction_cursor->position=d.key<=24u?0x30ea37u:0x30ea29u;
    }
};
template<class State,class Before=Noop>
Kind run_private_stage(State&s,Before before={}){
    native_raw_x87_detail::Scope<State>fp{s};Access<State,Before>memory{s,before,fp};
    Data data;data.work=s.gp[6];data.count_address=s.gp[7];
    return native_tu15_begin_simplex_iteration(memory,data);
}
template<class State,class Before=Noop>
Kind run_private_retained_stage(State&s,Before before={}){
    Access<State,Before,true>memory{s,before};
    Data data;data.work=s.gp[6];data.count_address=s.gp[7];
    return native_tu15_begin_simplex_iteration(memory,data);
}
} // namespace halo2::native_simplex_iteration_detail
#endif
