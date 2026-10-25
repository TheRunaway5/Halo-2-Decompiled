#pragma once
#include "physics/vehicles/physics_vehicle_runtime_internal.hpp"
#include "halo2/source_vehicle_native_rates.hpp"
#include "physics/runtime/physics_instruction_remap_tu15.hpp"
#include <atomic>

namespace halo2::physics_vehicle_runtime_detail {
inline std::atomic<std::uint64_t> rate_acceleration_calls{},rate_approach_calls{},rate_cursor_calls{};

// All 94 original instruction sites in the two complete scalar bodies. A
// mapped callback must retain every branch's original lookup/fault behavior.
inline constexpr auto rate_instruction_sites=std::to_array<std::uint32_t>({
    0x208d80u,0x208d86u,0x208d89u,0x208d8cu,0x208d8du,0x208d90u,0x208d96u,0x208d9au,
    0x208d9du,0x208d9fu,0x208da1u,0x208da4u,0x208da8u,0x208dacu,0x208dafu,0x208db3u,
    0x208db7u,0x208dbdu,0x208dbfu,0x208dc2u,0x208dc6u,0x208dc9u,0x208dcbu,0x208dcfu,
    0x208dd3u,0x208dd5u,0x208dd9u,0x208ddcu,0x208ddfu,0x208de1u,0x208de7u,0x208de9u,
    0x208dedu,0x208df5u,0x208dfbu,0x208dffu,0x208e02u,0x208e04u,0x208e08u,0x208e0cu,
    0x208e0fu,0x208e13u,0x208e16u,0x208e18u,0x208e1cu,0x208e1eu,0x208e21u,0x208e23u,
    0x208e27u,0x208e2du,0x208e31u,0x208e33u,0x208e37u,0x208e3bu,0x208e43u,0x208e49u,
    0x208e4du,0x208e52u,0x208e56u,0x208e59u,0x208e5bu,0x208e5fu,0x208e63u,0x208e70u,
    0x208e74u,0x208e77u,0x208e78u,0x208e7au,0x208e7cu,0x208e80u,0x208e83u,0x208e89u,
    0x208e8au,0x208e8bu,0x208e90u,0x208e95u,0x208e98u,0x208e9au,0x208e9du,0x208e9fu,
    0x208ea3u,0x208ea4u,0x208ea8u,0x208ea9u,0x208eaeu,0x208eb2u,0x208eb5u,0x208eb7u,
    0x208ebbu,0x208ebdu,0x208ebeu,0x208ec1u,0x208ec3u,0x208ec4u});

template<class Select>
bool vehicle_rate_native_eligible(const VehicleState &s,const Select &select,bool approach) {
    if(!s.services.instruction)return true;
    auto *cursor=s.instruction_cursor;
    if(!s.literal_service_sites || !cursor) {
        const auto *mapped=s.services.instruction.target<PhysicsInstructionRemapTu15>();
        if(!mapped || !mapped->covers(rate_instruction_sites,
            s.image==SourcePhysicsContactImage::TitleUpdate15?0x4900u:0u))return false;
        cursor=mapped->live_cursor();
    }
    if(!cursor || s.observe ||
       (cursor->first_trace && *cursor->first_trace) ||
       (cursor->second_trace && *cursor->second_trace))return false;
    // These whole routines contain none of the three live service PCs. Only
    // ordinary fully backed ranges may skip intermediate read-only cursors.
    // No view is retained across a source write. Store/fault and final cursors
    // are published explicitly; full observers retain the literal path.
    const auto stack=s.gp[4];
    if(stack<(approach?16u:0u) || stack>0xffffffffu-11u ||
       s.gp[1]>0xfffffffcu || s.gp[2]>0xfffffff0u)return false;
    return s.memory.try_read_view(stack-(approach?16u:0u),approach?28u:12u) &&
        s.memory.try_read_view(s.gp[1],4) && s.memory.try_read_view(s.gp[2],16) &&
        s.memory.try_read_view(select(0x465030u,s.image),4);
}

inline void vehicle_rate_native_site(VehicleState &s,std::uint32_t site) {
    if(s.services.instruction)s.instruction_literal(site);
}
inline void vehicle_rate_native_write(VehicleState &s,std::uint32_t site,
    std::uint32_t address,unsigned size,std::uint64_t value) {
    vehicle_rate_native_site(s,site);w(s.memory,s.observe,site,address,size,value);
}
inline void vehicle_rate_native_push(VehicleState &s,std::uint32_t site,std::uint32_t value) {
    vehicle_rate_native_site(s,site);s.push(site,value);
}

// Whole scalar rate/target kernels. The game uses these structured native
// branches. The generated instruction paths remain available for original
// instruction-trace comparisons. Stack stores and volatile ABI outputs are
// retained because enclosing original controllers can observe them.
template<class Select>
void vehicle_rate_accelerate_native(VehicleState &s, const Select &select) {
    rate_acceleration_calls.fetch_add(1,std::memory_order_relaxed);
    if(s.services.instruction)rate_cursor_calls.fetch_add(1,std::memory_order_relaxed);
    const auto load = [&](std::uint32_t p) { return _mm_set_ss(f(r(s.memory,p,4))); };
    const auto put = [&](std::uint32_t site,std::uint32_t p,__m128 value) {
        vehicle_rate_native_write(s,site,p,4,u(_mm_cvtss_f32(value)));
    };
    const auto value=s.gp[1],rates=s.gp[2],arguments=s.gp[4];
    auto direction=load(arguments+4);
    auto zero=_mm_setzero_ps();
    s.xmm[1]=direction;s.xmm[0]=zero;
    s.flags.scalar_compare(direction,zero,true);
    s.set(0,s.flags.bits()|2u,1,8);
    s.flags.logic(s.reg(0,1,8)&0x44u,1);
    if(s.flags.pf) {
        s.fp.push(ext(f(r(s.memory,arguments+4,4))));
        s.flags.scalar_compare(direction,zero,false);
        const bool increasing=!(s.flags.cf||s.flags.zf);
        s.fp.st(0)=std::fabs(s.fp.st(0));s.fp.push(s.fp.st(0));
        s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,rates+8,4))));
        s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,arguments+8,4))));
        vehicle_rate_native_write(s,0x208da8u,arguments+4,4,u(spill(s.fp.st(0))));s.fp.pop();
        s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,rates+12,4))));
        s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,arguments+8,4))));
        vehicle_rate_native_write(s,0x208db3u,arguments+8,4,u(spill(s.fp.st(0))));s.fp.pop();
        auto deceleration=load(arguments+8);s.xmm[1]=deceleration;
        if(increasing) {
            auto reverse_step=_mm_sub_ss(zero,deceleration);s.xmm[2]=reverse_step;
            s.flags.scalar_compare(reverse_step,load(value),false);
            if(!s.flags.cf) {
                s.xmm[0]=load(value);s.xmm[0]=_mm_add_ss(s.xmm[0],deceleration);
            } else {
                auto current=load(value);s.xmm[2]=current;
                s.flags.scalar_compare(current,zero,false);s.xmm[0]=current;
                if(!s.flags.cf) s.xmm[0]=_mm_add_ss(current,load(arguments+4));
                else {
                    s.xmm[0]=_mm_div_ss(s.xmm[0],deceleration);
                    s.xmm[0]=_mm_add_ss(s.xmm[0],load(select(0x465030u,s.image)));
                    s.xmm[0]=_mm_mul_ss(s.xmm[0],load(arguments+4));
                }
            }
            put(0x208dfbu,value,s.xmm[0]);
            s.flags.scalar_compare(s.xmm[0],load(rates),false);
            if(!(s.flags.cf||s.flags.zf)) {
                s.xmm[0]=load(rates);put(0x208e08u,value,s.xmm[0]);
                vehicle_rate_native_site(s,0x208e0cu);s.pop();s.gp[4]+=8;return;
            }
            s.xmm[0]=load(value);
        } else {
            auto current=load(value);s.xmm[2]=current;
            s.flags.scalar_compare(current,deceleration,false);
            if(!s.flags.cf) {
                s.xmm[2]=_mm_sub_ss(current,deceleration);put(0x208e49u,value,s.xmm[2]);
            } else {
                s.flags.scalar_compare(zero,load(value),false);
                if(!s.flags.cf) {
                    s.xmm[1]=load(value);s.xmm[1]=_mm_sub_ss(s.xmm[1],load(arguments+4));
                    put(0x208e2du,value,s.xmm[1]);
                } else {
                    s.xmm[2]=load(value);s.xmm[2]=_mm_div_ss(s.xmm[2],deceleration);
                    s.xmm[2]=_mm_sub_ss(s.xmm[2],load(select(0x465030u,s.image)));
                    s.xmm[2]=_mm_mul_ss(s.xmm[2],load(arguments+4));
                    put(0x208e49u,value,s.xmm[2]);
                }
            }
            s.xmm[0]=_mm_sub_ss(zero,load(rates+4));s.xmm[1]=load(value);
            s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
            if(!(s.flags.cf||s.flags.zf)) s.xmm[0]=load(value);
        }
        put(0x208e5fu,value,s.xmm[0]);
    }
    vehicle_rate_native_site(s,0x208e63u);s.pop();s.gp[4]+=8;
}

template<class Select>
void vehicle_rate_approach_native(VehicleState &s,const Select &select) {
    rate_approach_calls.fetch_add(1,std::memory_order_relaxed);
    if(s.services.instruction)rate_cursor_calls.fetch_add(1,std::memory_order_relaxed);
    const auto load = [&](std::uint32_t p) { return _mm_set_ss(f(r(s.memory,p,4))); };
    s.xmm[0]=load(s.gp[1]);s.flags.scalar_compare(s.xmm[0],s.xmm[3],false);
    vehicle_rate_native_push(s,0x208e77u,s.gp[3]);s.set(3,1u,1,0);
    bool clamp=false;
    if(!(s.flags.cf||s.flags.zf)) {
        s.gp[0]=r(s.memory,s.gp[4]+12,4);
        s.xmm[0]=_mm_setzero_ps();s.xmm[0]=_mm_sub_ss(s.xmm[0],load(s.gp[4]+8));
        vehicle_rate_native_push(s,0x208e89u,s.gp[0]);vehicle_rate_native_push(s,0x208e8au,s.gp[1]);
        vehicle_rate_native_write(s,0x208e8bu,s.gp[4],4,u(_mm_cvtss_f32(s.xmm[0])));
        vehicle_rate_native_push(s,0x208e90u,select(0x208e95u,s.image));vehicle_rate_accelerate_native(s,select);
        s.flags.scalar_compare(s.xmm[3],load(s.gp[1]),false);clamp=true;
    } else {
        s.flags.scalar_compare(s.xmm[3],load(s.gp[1]),false);
        if(!(s.flags.cf||s.flags.zf)) {
            s.gp[0]=r(s.memory,s.gp[4]+12,4);vehicle_rate_native_push(s,0x208ea3u,s.gp[0]);
            s.gp[0]=r(s.memory,s.gp[4]+12,4);vehicle_rate_native_push(s,0x208ea8u,s.gp[0]);
            vehicle_rate_native_push(s,0x208ea9u,select(0x208eaeu,s.image));vehicle_rate_accelerate_native(s,select);
            s.xmm[0]=load(s.gp[1]);s.flags.scalar_compare(s.xmm[0],s.xmm[3],false);clamp=true;
        }
    }
    const bool failed=clamp && s.flags.cf;
    if(failed) s.set(0,s.flags.logic(0,1),1,0);
    else {
        if(clamp) vehicle_rate_native_write(s,0x208eb7u,s.gp[1],4,u(_mm_cvtss_f32(s.xmm[3])));
        s.set(0,s.reg(3,1,0),1,0);
    }
    vehicle_rate_native_site(s,failed?0x208ec3u:0x208ebdu);s.gp[3]=s.pop();
    vehicle_rate_native_site(s,failed?0x208ec4u:0x208ebeu);s.pop();s.gp[4]+=8;
}
} // namespace halo2::physics_vehicle_runtime_detail
