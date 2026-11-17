#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_support_pair_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
namespace halo2::native_solver_hull_support_pair_detail {
using P=NativeTu15SolverHullSupportPairPhase;
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
enum class Publication { physical,retained };
template<class State,class Call,class Before,class Checkpoint,Publication Mode>class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
public:
    using Data=NativeTu15SolverHullSupportPair<__m128>;
    static constexpr bool translation_before_second_y=false;
private:
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;Call&call_;Checkpoint&checkpoint_;
    void retired(std::uint32_t pc){
        if constexpr(Mode==Publication::retained){
            switch(pc){
            case 0x313390u:checkpoint_(s_,0x313391u);break;case 0x313399u:checkpoint_(s_,0x31339au);break;
            case 0x31339au:checkpoint_(s_,0x31339du);break;case 0x31339du:checkpoint_(s_,0x31339eu);break;
            case 0x31339eu:checkpoint_(s_,0x31339fu);break;case 0x31339fu:checkpoint_(s_,0x3133a2u);break;
            case 0x3133a4u:checkpoint_(s_,0x3133a7u);break;case 0x3133a7u:checkpoint_(s_,0x3133a9u);break;
            case 0x3133a9u:checkpoint_(s_,0x3133aau);break;case 0x3133aau:checkpoint_(s_,0x3133abu);break;
            case 0x3133abu:checkpoint_(s_,0x3133aeu);break;case 0x3133aeu:checkpoint_(s_,0x3133b2u);break;
            case 0x3133b2u:checkpoint_(s_,0x3133b9u);break;case 0x3133b9u:checkpoint_(s_,0x3133bcu);break;
            case 0x3133bcu:checkpoint_(s_,0x3133bfu);break;case 0x3133bfu:checkpoint_(s_,0x3133c3u);break;
            case 0x3133c3u:checkpoint_(s_,0x3133c6u);break;case 0x313418u:checkpoint_(s_,0x313419u);break;
            case 0x31341du:checkpoint_(s_,0x313422u);break;case 0x313422u:checkpoint_(s_,0x313424u);break;
            case 0x313424u:checkpoint_(s_,0x313425u);break;case 0x313425u:checkpoint_(s_,0x313428u);break;
            case 0x313428u:checkpoint_(s_,0x31342bu);break;case 0x31342bu:checkpoint_(s_,0x31342fu);break;
            case 0x31342fu:checkpoint_(s_,0x313433u);break;case 0x31343du:checkpoint_(s_,0x313441u);break;
            case 0x313455u:checkpoint_(s_,0x313458u);break;case 0x313461u:checkpoint_(s_,0x313465u);break;
            case 0x313465u:checkpoint_(s_,0x313466u);break;case 0x313466u:checkpoint_(s_,0x313467u);break;
            case 0x313467u:checkpoint_(s_,0x313468u);break;case 0x31346au:checkpoint_(s_,0x31346bu);break;
            default:throw std::logic_error("Unsupported support-pair source frontier");
            }
        }
    }
    void push_owned(std::uint32_t pc,std::uint32_t value){
        if constexpr(Mode==Publication::retained){at(pc);s_.gp[4]-=4u;Base::store(pc,s_.gp[4],value,[]{},true);}
        else Base::push(pc,value);
        retired(pc);
    }
    void call_return(std::uint32_t pc,std::uint32_t value){const auto address=s_.gp[4]-4u;
        if constexpr(Mode==Publication::retained){s_.gp[4]=address;Base::store(pc,address,value,[]{},true);}
        else Base::store(pc,address,value,[&]{s_.gp[4]=address;},true);
    }
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{pc,address,flags,flags|0x10000u};}
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);
        for(unsigned offset:{0u,8u}){SourcePhysicsContactRecordStore event{pc,address+offset,{},{}};
            if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}
            const auto data=std::span(bytes).subspan(offset,8);fp_.io([&]{s_.memory.write(event.address,data);});
            if(s_.observe){event.written.assign(data.begin(),data.end());fp_.io([&]{s_.observe(event);});}
        }
        retired(pc);
    }
    void adopt(Data&d){d.context=s_.gp[6];d.direction=s_.gp[3];d.point_pair=s_.gp[7];d.second_output=s_.gp[3];}
public:
    Frame(State&s,Call&call,Before&before,Checkpoint&checkpoint,native_raw_x87_detail::Scope<State>&scope):Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&scope}},fp_(scope),call_(call),checkpoint_(checkpoint){}
    void begin(Data&d){auto&g=s_.gp;push_owned(0x313390u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x14u,4);push_owned(0x313399u,g[3]);at(0x31339au);g[3]=word(g[5]+8u);retired(0x31339au);push_owned(0x31339du,g[6]);push_owned(0x31339eu,g[7]);at(0x31339fu);g[7]=word(g[5]+12u);retired(0x31339fu);g[6]=g[1];adopt(d);}
    std::uint32_t first_shape(const Data&d){at(0x3133a4u);const auto value=s_.gp[1]=word(d.context+0x40u);retired(0x3133a4u);return value;}
    std::uint32_t first_table(const Data&){at(0x3133a7u);const auto value=s_.gp[0]=word(s_.gp[1]);retired(0x3133a7u);return value;}
    void support_first(Data&d){auto&g=s_.gp;push_owned(0x3133a9u,g[7]);push_owned(0x3133aau,g[3]);at(0x3133abu);const auto target=d.first_owner=word(g[0]+0x30u);call_return(0x3133abu,0x3133aeu);fp_.child([&]{call_(s_,target,0x3133abu);retired(0x3133abu);});adopt(d);}
    std::uint32_t second_shape(Data&d){at(0x3133c3u);const auto value=s_.gp[1]=word(s_.gp[6]+0x44u);
        if constexpr(Mode==Publication::retained){d.z=s_.xmm[3];d.sign=s_.xmm[1];d.input=s_.xmm[0];d.x=s_.xmm[2];d.y=s_.xmm[5];}
        retired(0x3133c3u);return value;
    }
    __m128 vector(P phase,Data&d,std::uint32_t address){
        std::uint32_t pc{};unsigned reg{};
        switch(phase){
        case P::first_z:pc=0x3133aeu;reg=3;break;case P::sign:pc=0x3133b2u;reg=1;break;
        case P::direction:pc=0x3133b9u;reg=0;break;case P::first_x:pc=0x3133bcu;reg=2;break;case P::first_y:pc=0x3133bfu;reg=5;break;
        case P::second_local:pc=0x313428u;reg=0;break;case P::translation:pc=0x31342bu;reg=1;break;case P::second_z:pc=0x31342fu;reg=3;break;
        case P::second_y:s_.xmm[2]=d.local_z;s_.xmm[3]=d.final_z_term;pc=0x31343du;reg=2;break;
        case P::second_x:s_.xmm[3]=d.translated;s_.xmm[2]=d.final_y_term;s_.xmm[1]=d.local_x;pc=0x313455u;reg=0;break;
        }
        if constexpr(Mode==Publication::retained){
            switch(phase){
            case P::first_z:case P::second_z:address=s_.gp[6]+0x20u;break;
            case P::direction:case P::second_local:address=s_.gp[3];break;
            case P::first_x:case P::second_x:address=s_.gp[6];break;
            case P::first_y:case P::second_y:address=s_.gp[6]+0x10u;break;
            case P::translation:address=s_.gp[6]+0x30u;break;case P::sign:address=0x461960u;break;
            }
        }
        const auto value=s_.xmm[reg]=Base::vector(pc,address);
        if constexpr(Mode==Publication::retained){
            if(phase==P::second_z){d.local=s_.xmm[0];d.translation=s_.xmm[1];}
            else if(phase==P::second_y){d.final_z_term=s_.xmm[3];d.translation=s_.xmm[1];d.local=s_.xmm[0];}
            else if(phase==P::second_x){d.local_x=s_.xmm[1];d.final_y_term=s_.xmm[2];d.translated=s_.xmm[3];}
        }
        retired(pc);return value;
    }
    __m128 xor_bits(__m128 a,__m128 b){return _mm_xor_ps(a,b);}
    template<unsigned Mask>__m128 shuffle(__m128 a,__m128 b){return _mm_shuffle_ps(a,b,Mask);}
    __m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    __m128 add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    void prepare_second(Data&d){auto&g=s_.gp;
        s_.xmm[0]=d.negative;s_.xmm[1]=d.transformed;s_.xmm[2]=d.z_term;s_.xmm[3]=d.negative_x;
        s_.xmm[4]=d.z_low;s_.xmm[5]=d.y_term;s_.xmm[6]=d.negative_z;s_.xmm[7]=d.z_high;
        g[3]=g[7]+0x10u;d.second_output=g[3];push_owned(0x313418u,g[3]);g[0]=g[4]+0x14u;d.transformed_direction=g[0];
        if constexpr(Mode==Publication::retained)store_vector(0x31341du,g[4]+0x14u,s_.xmm[1]);
        else store_vector(0x31341du,g[4]+0x14u,d.transformed);
    }
    std::uint32_t second_table(const Data&){at(0x313422u);const auto value=s_.gp[2]=word(s_.gp[1]);retired(0x313422u);return value;}
    void support_second(Data&d){auto&g=s_.gp;push_owned(0x313424u,g[0]);at(0x313425u);const auto target=d.second_owner=word(g[2]+0x30u);call_return(0x313425u,0x313428u);fp_.child([&]{call_(s_,target,0x313425u);retired(0x313425u);});adopt(d);}
    void store_result(const Data&d){s_.xmm[0]=d.result;s_.xmm[1]=d.local_x;s_.xmm[2]=d.final_y_term;s_.xmm[3]=d.translated;store_vector(0x313461u,s_.gp[7]+0x20u,d.result);}
    void finish(const Data&){auto&g=s_.gp;g[7]=pop(0x313465u);retired(0x313465u);g[6]=pop(0x313466u);retired(0x313466u);g[3]=pop(0x313467u);retired(0x313467u);g[4]=g[5];g[5]=pop(0x31346au);retired(0x31346au);pop(0x31346bu);g[4]+=8u;}
};
template<Publication Mode=Publication::physical,class State,class Call,class Before=Noop,class Checkpoint=Noop>void run_private_owner(State&s,Call call,Before before={},Checkpoint checkpoint={}){native_raw_x87_detail::Scope scope{s};Frame<State,Call,Before,Checkpoint,Mode>view{s,call,before,checkpoint,scope};typename decltype(view)::Data data;native_tu15_solver_hull_support_pair_kernel(view,data);}
} // namespace halo2::native_solver_hull_support_pair_detail
#endif
