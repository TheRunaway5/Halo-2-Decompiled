#pragma once
#include "halo2/native_tu15_simplex_support_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/native_tu15_simplex_candidate_abi.hpp"

namespace halo2::native_simplex_support_detail {
using Data=NativeTu15SimplexSupport;
using P=NativeTu15SimplexSupportPhase;
using E=NativeTu15SimplexSupportEntry;
using N=NativeTu15SimplexSupportNext;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Support,class Before>class Access:public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;
    native_raw_x87_detail::Scope<State>&fp_;Support&support_;std::uint32_t last_branch_{};
    static bool second(const Data&d){return d.entry==E::second;}
    static bool late(const Data&d){return d.entry==E::last_first;}
    static std::uint32_t first_pc(const Data&d,std::uint32_t early){return early+(late(d)?0x5e0u:0u);}
    void vector_store(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u)throw native_mopp_ray_caller_detail::AlignedLoadFault{pc,address,s_.other_flags|s_.flags.bits(),s_.other_flags|s_.flags.bits()|0x10000u};
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);
        for(unsigned offset:{0u,8u}){
            SourcePhysicsContactRecordStore event{pc,address+offset,{},{}};
            if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}
            const auto written=std::span(bytes).subspan(offset,8);fp_.io([&]{s_.memory.write(event.address,written);});
            if(s_.observe){event.written.assign(written.begin(),written.end());fp_.io([&]{s_.observe(event);});}
        }
    }
public:
    Access(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp,Support&support)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),support_(support){}
    std::uint32_t word_read(P p,const Data&d){
        auto&g=s_.gp;std::uint32_t pc{},address{};unsigned reg=8,width=4;
        switch(p){
        case P::parity:pc=0x30f458u;address=g[4]+0x4cu;reg=0;break;
        case P::cache:pc=second(d)?0x30f274u:first_pc(d,0x30ee84u);address=g[5]+0x14u;reg=second(d)?0:1;break;
        case P::limit:pc=second(d)?0x30f277u:first_pc(d,0x30ee87u);address=d.cache+(second(d)?11u:10u);reg=second(d)?1:2;width=1;break;
        case P::count:pc=second(d)?0x30f27bu:first_pc(d,0x30ee8bu);address=second(d)?g[7]:g[6];reg=second(d)?2:0;break;
        case P::progress:pc=second(d)?0x30f285u:first_pc(d,0x30ee95u);address=g[6]+0x10u;break;
        case P::shape:pc=second(d)?0x30f29du:first_pc(d,0x30ee9fu);address=g[5]+(second(d)?12u:8u);reg=1;break;
        case P::table:pc=second(d)?0x30f2b9u:first_pc(d,0x30eea2u);address=d.shape;reg=second(d)?0:2;break;
        case P::owner:pc=second(d)?0x30f312u:first_pc(d,0x30eeaeu);address=d.table+0x30u;break;
        case P::matrix:pc=d.entry==E::transform_second?0x30f40eu:0x30f28fu;address=g[5]+16u;reg=0;break;
        default:throw std::runtime_error("Invalid simplex support scalar input");
        }
        if(p==P::table&&second(d)){s_.xmm[4]=d.z_low;s_.xmm[7]=d.z_high;}
        at(pc);auto value=word(address,width);if(width==1u)value=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
        if(reg!=8u)g[reg]=value;
        if(p==P::parity){s_.flags.logic(value&value,4);last_branch_=0x30f45eu;}
        return value;
    }
    bool at_limit(const Data&d){s_.flags.minus(d.count,d.limit,4);last_branch_=second(d)?0x30f27fu:first_pc(d,0x30ee8fu);return s_.flags.zf;}
    bool progress_blocks(std::uint32_t value,const Data&d){s_.flags.minus(value,second(d)?0xffffffffu:1u,4);last_branch_=second(d)?0x30f289u:first_pc(d,0x30ee99u);return s_.flags.zf;}
    __m128 vector_read(P p,const Data&d){
        auto&x=s_.xmm;std::uint32_t pc{},address{};unsigned reg{};
        switch(p){
        case P::inverse_z:pc=0x30f292u;address=d.matrix+32u;reg=3;break;
        case P::inverse_x:pc=0x30f296u;address=d.matrix;reg=2;break;
        case P::inverse_y:pc=0x30f299u;address=d.matrix+16u;reg=1;break;
        case P::inverse_sign:pc=0x30f2a0u;address=0x461960u;reg=5;break;
        case P::inverse_y_reload:x[4]=d.z_low;x[7]=d.z_high;pc=0x30f2b5u;address=d.matrix+16u;reg=3;break;
        case P::axis:x[0]=d.xy_rows;pc=0x30f2c2u;address=s_.gp[3];reg=1;break;
        case P::transformed_point:pc=0x30f40bu;address=s_.gp[2];reg=0;break;
        case P::translation:pc=0x30f411u;address=d.matrix+48u;reg=1;break;
        case P::transform_z:pc=0x30f415u;address=d.matrix+32u;reg=3;break;
        case P::transform_y:x[2]=d.point_z;x[3]=d.z_term;pc=0x30f423u;address=d.matrix+16u;reg=2;break;
        case P::transform_x:x[1]=d.point_x;x[2]=d.y_term;x[3]=d.zt;pc=0x30f43bu;address=d.matrix;reg=0;break;
        default:throw std::runtime_error("Invalid simplex support vector input");
        }
        return x[reg]=Base::vector(pc,address);
    }
    void prepare_first_output(Data&d){auto&g=s_.gp;g[0]=s_.flags.plus(g[0],2u,4);g[0]=native_simplex_candidate_detail::shift4(s_,g[0]);d.output=g[0]=s_.flags.plus(g[0],g[6],4);}
    void prepare_second_index(const Data&){s_.xmm[2]=_mm_shuffle_ps(s_.xmm[2],s_.xmm[3],0xee);s_.xmm[6]=native_simplex_direction_numeric::shuffle<0xaa>(s_.xmm[1]);s_.gp[2]=s_.flags.plus(s_.gp[2],14u,4);}
    void scale_second_index(const Data&d){s_.xmm[2]=d.z_products;s_.xmm[6]=d.xy_rows;s_.xmm[3]=native_simplex_direction_numeric::shuffle<0x55>(d.axis);s_.gp[2]=native_simplex_candidate_detail::shift4(s_,s_.gp[2]);}
    void locate_second_output(Data&d){s_.xmm[6]=d.y_products;s_.xmm[0]=_mm_shuffle_ps(d.xy_rows,d.z_low,0x88);d.output=s_.gp[2]=s_.flags.plus(s_.gp[2],s_.gp[6],4);}
    void push_output(const Data&d){if(second(d)){s_.xmm[0]=d.x_products;s_.xmm[3]=native_simplex_direction_numeric::shuffle<0>(d.axis);}push(second(d)?0x30f2feu:first_pc(d,0x30eeacu),d.output);}
    void prepare_direction_address(const Data&d){s_.xmm[0]=d.sum_xy;s_.gp[2]=s_.gp[4]+0x74u;}
    void push_direction(const Data&d){if(second(d))s_.xmm[0]=d.direction;push(second(d)?0x30f30cu:first_pc(d,0x30eeadu),second(d)?s_.gp[2]:s_.gp[3]);}
    void store_direction(const Data&d){vector_store(0x30f30du,s_.gp[4]+0x78u,d.direction);}
    void invoke_support(const Data&d){const auto pc=second(d)?0x30f312u:first_pc(d,0x30eeaeu);const auto address=s_.gp[4]-4u;Base::store(pc,address,second(d)?0x30f315u:first_pc(d,0x30eeb1u),[&]{s_.gp[4]=address;},true);fp_.child([&]{support_(s_,d.owner,pc);});}
    void prepare_transform(const Data&){s_.gp[1]=s_.flags.plus(s_.gp[1],10u,4);}
    void store_transformed(const Data&d){auto&x=s_.xmm;x[0]=d.transformed;x[1]=d.point_x;x[2]=d.y_term;x[3]=d.zt;s_.gp[1]=native_simplex_candidate_detail::shift4(s_,s_.gp[1]);vector_store(0x30f44du,s_.gp[1]+s_.gp[6],d.transformed);}
    void increment_transformed_count(const Data&){at(0x30f451u);const auto value=word(s_.gp[7]);const auto next=s_.flags.increment(value,4,false);Base::store(0x30f451u,s_.gp[7],next,[]{},true);}
    void complete(N next,const Data&){if(s_.instruction_cursor&&s_.instruction_cursor->position){if(next==N::iteration)*s_.instruction_cursor->position=0x30f453u;else if(next!=N::first_candidate&&next!=N::second_candidate)*s_.instruction_cursor->position=last_branch_;}}
};
template<class State,class Support,class Before=Noop>
N run_private_stage(State&s,E entry,Support support,Before before={}){
    native_raw_x87_detail::Scope<State>fp{s};Access<State,Support,Before>memory{s,before,fp,support};
    Data data;data.entry=entry;data.work=s.gp[6];data.count_address=s.gp[7];
    return native_tu15_simplex_support(memory,data);
}
} // namespace halo2::native_simplex_support_detail
#endif
