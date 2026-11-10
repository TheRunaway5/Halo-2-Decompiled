#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_closest_segments_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
namespace halo2::native_closest_segments_detail {
using Data=NativeTu15ClosestSegments;using P=NativeTu15ClosestSegmentsPhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<bool Actual,class State,class Before>class Frame:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::word;native_raw_x87_detail::Scope<State>&fp_;
    static constexpr std::uint32_t site(std::uint32_t pc){return Actual?pc:pc-0x3af0u;}
    void at(std::uint32_t pc){Base::at(site(pc));}
    void push(std::uint32_t pc,std::uint32_t value){Base::push(site(pc),value);}
    std::uint32_t pop(std::uint32_t pc){return Base::pop(site(pc));}
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp):Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp){}
    void begin(Data&d){auto&g=s_.gp;push(0x312620u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x28u,4);push(0x312629u,g[6]);push(0x31262au,g[7]);d.scratch=g[4];}
    void require_capacity(unsigned n){fp_.require_capacity(n);}
    std::uint32_t argument(P phase,const Data&d){
        std::uint32_t pc,offset;unsigned reg;
        switch(phase){
        case P::first_direction_argument:pc=0x31262bu;offset=0xcu;reg=1;break;
        case P::first_point_argument:pc=0x312631u;offset=8u;reg=0;break;
        case P::second_point_argument:pc=0x312637u;offset=0x10u;reg=2;break;
        case P::second_direction_argument:pc=0x312640u;offset=0x14u;reg=6;s_.xmm[2]=d.delta;break;
        case P::output_argument:pc=0x312839u;offset=0x18u;reg=0;break;
        default:throw std::runtime_error("Invalid closest-segment argument acquisition");
        }
        at(pc);const auto value=word(s_.gp[5]+offset);s_.gp[reg]=value;return value;
    }
    __m128 vector_read(P phase,const Data&d,std::uint32_t address){
        std::uint32_t pc;unsigned reg;
        switch(phase){
        case P::first_direction:pc=0x31262eu;reg=1;break;
        case P::first_point:pc=0x312634u;reg=4;break;
        case P::second_point:pc=0x31263au;reg=2;break;
        case P::second_direction:pc=0x312643u;reg=3;break;
        case P::fresh_second_direction:pc=0x312864u;reg=1;break;
        case P::fresh_second_point:pc=0x31286eu;reg=0;s_.xmm[2]=d.fraction;break;
        case P::fresh_separation:pc=0x31287eu;reg=0;break;
        default:throw std::runtime_error("Invalid closest-segment vector acquisition");
        }
        const auto value=Base::vector(site(pc),address);s_.xmm[reg]=value;return value;
    }
    void vector_write(P phase,const Data&d,std::uint32_t address,__m128 value){
        const auto pc=phase==P::first_output?0x312861u:phase==P::separation_output?0x31287au:0u;
        if(!pc)throw std::runtime_error("Invalid closest-segment vector publication");
        if(phase==P::separation_output){s_.xmm[0]=d.point_on_second;s_.xmm[2]=d.offset;s_.xmm[4]=d.separation;}
        at(pc);if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{site(pc),address,flags,flags|0x10000u};}
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);
        for(unsigned offset:{0u,8u}){SourcePhysicsContactRecordStore event{site(pc),address+offset,{},{}};if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}const auto data=std::span(bytes).subspan(offset,8);fp_.io([&]{s_.memory.write(event.address,data);});if(s_.observe){event.written.assign(data.begin(),data.end());fp_.io([&]{s_.observe(event);});}}
    }
    void word_write(P phase,const Data&d,std::uint32_t value){
        auto&g=s_.gp;auto&x=s_.xmm;std::uint32_t pc,address;
        switch(phase){
        case P::cross_store:pc=0x312666u;address=g[0]=d.scratch+0x14u;x[0]=d.product;x[5]=d.xy;x[6]=d.cross;break;
        case P::along_second_store:pc=0x3126a6u;address=g[0]=d.scratch+0x28u;x[0]=d.along_second_product;x[2]=d.along_second_xy;x[5]=d.along_second;x[6]=d.along_first;break;
        case P::along_first_store:pc=0x3126dau;address=g[1]=d.scratch+0x2cu;x[0]=d.length_second_product;x[2]=d.length_second_xy;x[3]=d.length_second_product;x[5]=d.length_first;break;
        case P::length_second_store:pc=0x3126eeu;address=g[0]=d.scratch+0x18u;x[2]=d.length_second_xy;x[3]=d.length_second;break;
        case P::length_first_store:pc=0x3126fau;address=g[1]=d.scratch+0x1cu;break;
        case P::second_upper_store:pc=0x3127b2u;address=d.scratch+0xcu;break;
        case P::second_lower_store:pc=0x3127d2u;address=d.scratch+0xcu;break;
        case P::second_ratio_copy:pc=0x312857u;address=d.scratch+0x2cu;x[2]=d.offset;x[4]=d.point_on_first;break;
        case P::distance_store:pc=0x31289fu;address=g[2]=d.scratch+0x2cu;x[0]=d.distance_product;x[1]=d.distance_xy;x[2]=d.distance;break;
        case P::distance_output:pc=0x3128a8u;address=d.output+0x20u;g[7]=pop(0x3128a7u);break;
        default:throw std::runtime_error("Invalid closest-segment word publication");
        }
        Base::store(site(pc),address,value,[]{});
    }
    std::uint32_t scalar_read(P phase,const Data&d){
        std::uint32_t pc,offset;std::uint32_t address{};
        switch(phase){
        case P::length_second_load:pc=0x3126f2u;offset=0x18u;break;
        case P::length_first_load:pc=0x3126feu;offset=0x1cu;break;
        case P::cross_first:pc=0x312706u;offset=0x14u;break;
        case P::cross_second:pc=0x31270au;offset=0x14u;break;
        case P::product_load:pc=0x312712u;offset=0x24u;break;
        case P::cross_square_load:pc=0x312716u;offset=0x20u;break;
        case P::numerator_length_second:pc=0x312720u;offset=0x18u;break;
        case P::numerator_along_first:pc=0x312724u;offset=0x2cu;break;
        case P::numerator_along_second:pc=0x312728u;offset=0x28u;break;
        case P::numerator_cross:pc=0x31272cu;offset=0x14u;break;
        case P::scaled_numerator:pc=0x312736u;offset=0xcu;break;
        case P::determinant_first:pc=0x31273au;offset=0xcu;break;
        case P::determinant_second:pc=0x31273eu;offset=0xcu;break;
        case P::numerator_test:pc=0x31274bu;offset=0x10u;break;
        case P::zero_test:pc=0x31274fu;address=0x46a0e8u;break;
        case P::zero_fraction:pc=0x31275cu;address=0x46a0e8u;break;
        case P::parallel_cross_square:pc=0x312769u;offset=0x20u;break;
        case P::parallel_product:pc=0x31276du;offset=0x24u;break;
        case P::parallel_epsilon:pc=0x312771u;address=0x46a25cu;break;
        case P::parallel_determinant:pc=0x312777u;offset=0xcu;break;
        case P::numerator_fraction:pc=0x312782u;offset=0x10u;break;
        case P::fraction_denominator:pc=0x312788u;offset=0xcu;break;
        case P::one_fraction:pc=0x31278eu;address=0x46a0d0u;break;
        case P::second_cross:pc=0x312799u;offset=0x14u;break;
        case P::second_offset:pc=0x31279fu;offset=0x28u;break;
        case P::second_upper:pc=0x3127a7u;offset=0x18u;break;
        case P::second_test:pc=0x3127c1u;offset=0xcu;break;
        case P::second_zero:pc=0x3127c5u;address=0x46a0e8u;break;
        case P::recompute_cross:pc=0x3127e1u;offset=0x14u;break;
        case P::recompute_fraction:pc=0x3127e5u;offset=0xcu;break;
        case P::recompute_offset:pc=0x3127e9u;offset=0x2cu;break;
        case P::recompute_zero:pc=0x3127f1u;address=0x46a0e8u;break;
        case P::recompute_zero_fraction:pc=0x3127feu;address=0x46a0e8u;break;
        case P::recompute_test:pc=0x312809u;offset=0x10u;break;
        case P::recompute_upper:pc=0x31280du;offset=0x1cu;break;
        case P::recompute_one_fraction:pc=0x312818u;address=0x46a0d0u;break;
        case P::recompute_numerator:pc=0x312823u;offset=0x10u;break;
        case P::recompute_denominator:pc=0x312827u;offset=0x1cu;break;
        case P::second_divide_numerator:pc=0x31282du;offset=0xcu;break;
        case P::second_divide_denominator:pc=0x312831u;offset=0x18u;break;
        case P::first_ratio:pc=0x312840u;offset=0x2cu;break;
        case P::second_ratio:pc=0x312846u;offset=0xcu;break;
        case P::second_ratio_reload:pc=0x31285bu;offset=0x2cu;break;
        case P::distance_reload:pc=0x3128a3u;offset=0x2cu;break;
        default:throw std::runtime_error("Invalid closest-segment scalar acquisition");
        }
        at(pc);const auto value=word(address?address:d.scratch+offset);
        if(phase==P::first_ratio||phase==P::second_ratio_reload){s_.xmm[0]=_mm_setzero_ps();std::memcpy(&s_.xmm[0],&value,4);}
        if(phase==P::second_ratio)s_.gp[7]=value;
        if(phase==P::distance_reload)s_.gp[2]=value;
        return value;
    }
    void scalar_store(P phase,const Data&d,bool pop_value){
        std::uint32_t pc,offset;
        switch(phase){
        case P::product_store:pc=0x312702u;offset=0x24u;break;
        case P::cross_square_store:pc=0x31270eu;offset=0x20u;break;
        case P::determinant_store:pc=0x31271cu;offset=0xcu;break;
        case P::numerator_store:pc=0x312732u;offset=0x10u;break;
        case P::second_fraction_store:pc=0x3127a3u;offset=0xcu;break;
        case P::recompute_store:pc=0x3127edu;offset=0x10u;break;
        case P::second_ratio_store:pc=0x312835u;offset=0xcu;break;
        case P::first_ratio_store:pc=0x31283cu;offset=0x2cu;break;
        default:throw std::runtime_error("Invalid closest-segment x87 publication");
        }
        at(pc);const auto pending=fp_.stage_single(pop_value);Base::store(site(pc),d.scratch+offset,pending.bits,[&]{fp_.commit(pending);},true);
    }
    void relation(const Data&d,std::uint32_t mask){s_.set(0,d.relation,2,0);s_.flags.logic((d.relation>>8u)&mask,1);}
    void mask(const Data&d,NativeTu15ClosestSegmentsMask update){
        using M=NativeTu15ClosestSegmentsMask;
        if(update==M::clear||update==M::add_first_lower||update==M::add_first_upper)s_.gp[1]=s_.flags.logic(d.mask,4);
        else s_.gp[1]=d.mask;
    }
    void complete(const Data&){auto&g=s_.gp;g[0]=g[1];g[6]=pop(0x3128adu);g[4]=g[5];g[5]=pop(0x3128b0u);pop(0x3128b1u);}
};
template<bool Actual=true,class State,class Before=Noop>
std::uint32_t run_private_owner(State&s,Before before={}){native_raw_x87_detail::Scope<State>fp{s};Frame<Actual,State,Before>memory{s,before,fp};Data data;return native_tu15_closest_segments(memory,data);}
} // namespace halo2::native_closest_segments_detail
#endif
