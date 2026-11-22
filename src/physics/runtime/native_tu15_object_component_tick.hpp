#pragma once

#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>

namespace halo2::native_object_component_tick {

template<class State, class Dispatch>
class Frame {
    enum Register : unsigned { eax, ecx, edx, ebx, esp, ebp, esi, edi };
    using Word = std::uint32_t;
    struct SpillSites { Word load, multiply, store; };
    static constexpr Word component_flags=4, component_object=8;
    static constexpr Word component_rows=0x70, component_count=0x74;
    static constexpr Word body_motion=0x3c, body_fixed=0x40;
    static constexpr Word motion_method=0x18, motion_row=0x40;
    static constexpr Word static_vector=0x474d04;

    State &state_;
    Dispatch &dispatch_;
    void (*apply_vehicle_force_)(State &);
    std::uint32_t *cursor_;

    Word &reg(Register name) { return state_.gp[name]; }
    Word stack(Word offset) { return reg(esp)+offset; }
    static std::int32_t signed_word(Word value) { return std::bit_cast<std::int32_t>(value); }
    void at(Word pc) {
        if (cursor_) *cursor_=pc;
        else if (!native_census_detail::skip_report(state_.services,pc)) state_.instruction_literal(pc);
    }
    Word load(Word pc,Word address,unsigned bytes=4) {
        at(pc);
        return physics_step_detail::r(state_.memory,address,bytes);
    }
    void store(Word pc,Word address,Word value) {
        at(pc);
        physics_step_detail::w(state_.memory,state_.observe,pc,address,4,value);
    }
    void push(Word pc,Register name) {
        at(pc);
        state_.push(pc,reg(name));
    }
    void pop(Word pc,Register name) {
        at(pc);
        reg(name)=state_.pop();
    }
    void call(Word pc,Word resume,Word entry) {
        at(pc);
        state_.push(pc,resume);
        dispatch_(state_,entry,pc,false);
    }
    void call_motion_type(Word pc,Word resume,Register table) {
        const auto target=load(pc,reg(table)+motion_method);
        state_.push(pc,resume);
        dispatch_(state_,target,pc,false);
    }
    __m128 load_scalar(Word pc,Word address) {
        return _mm_set_ss(physics_step_detail::f(load(pc,address)));
    }
    void store_scalar(Word pc,Word address,__m128 value) {
        store(pc,address,physics_step_detail::u(_mm_cvtss_f32(value)));
    }
    void ret() {
        at(0x1d7251u);
        reg(esp)=state_.flags.plus(reg(esp),0x30u,4);
        at(0x1d7254u);
        state_.pop();
    }
    bool component_enabled() {
        reg(eax)=load(0x1d6fb0u,reg(edi)+component_flags);
        at(0x1d6fb3u);
        reg(eax)=state_.flags.right(reg(eax),5,4,false);
        at(0x1d6fb6u);
        reg(esp)=state_.flags.minus(reg(esp),0x30u,4);
        at(0x1d6fb9u);
        state_.flags.logic(reg(eax)&1u,1);
        at(0x1d6fbbu);
        return !state_.flags.zf;
    }
    bool has_active_body() {
        push(0x1d6fc1u,esi);
        at(0x1d6fc2u);
        reg(esi)=reg(edi);
        call(0x1d6fc4u,0x1d6fc9u,0x1d9ec0u);
        at(0x1d6fc9u);
        state_.flags.logic(reg(eax)&0xffu,1);
        at(0x1d6fcbu);
        return !state_.flags.zf;
    }
    void publish_object_and_acceleration() {
        auto &x=state_.xmm;
        reg(ecx)=load(0x1d6fd1u,0x51df54u);
        at(0x1d6fd7u);
        x[0]=_mm_setzero_ps();
        reg(edx)=load(0x1d6fdau,0x52be68u);
        x[1]=load_scalar(0x1d6fe0u,reg(ecx)+4u);
        x[1]=_mm_mul_ss(x[1],load_scalar(0x1d6fe5u,reg(edx)));
        reg(eax)=load(0x1d6fe9u,reg(edi)+component_object);
        store_scalar(0x1d6fecu,stack(0x28u),x[0]);
        store_scalar(0x1d6ff2u,stack(0x2cu),x[0]);
        push(0x1d6ff8u,ebx);
        at(0x1d6ff9u);
        x[0]=_mm_sub_ss(x[0],x[1]);
        store_scalar(0x1d6ffdu,stack(0x34u),x[0]);
        call(0x1d7003u,0x1d7008u,0xbda50u);
    }
    void vehicle_force() {
        reg(eax)=load(0x1d7008u,reg(edi)+component_flags);
        at(0x1d700bu);
        reg(eax)=state_.flags.right(reg(eax),2,4,false);
        at(0x1d700eu);
        state_.flags.logic(reg(eax)&1u,1);
        at(0x1d7010u);
        if (!state_.flags.zf) return;

        reg(eax)=load(0x1d7012u,reg(edi)+component_object);
        reg(edx)=load(0x1d7015u,0x4ecbd0u);
        reg(edx)=load(0x1d701bu,reg(edx)+0x44u);
        at(0x1d701eu);
        reg(ecx)=reg(eax);
        at(0x1d7020u);
        reg(ecx)=state_.flags.logic(reg(ecx)&0xffffu,4);
        at(0x1d7026u);
        reg(ecx)*=3u;
        state_.set(ecx,load(0x1d7029u,reg(edx)+reg(ecx)*4u+3u,1),1);
        at(0x1d702du);
        reg(ebx)=1;
        at(0x1d7032u);
        reg(ebx)=state_.flags.shift(reg(ebx),reg(ecx)&0xffu,4);
        at(0x1d7034u);
        state_.flags.logic(reg(ebx)&2u,1);
        at(0x1d7037u);
        if (state_.flags.zf) return;

        at(0x1d7039u);
        reg(esi)=stack(0x2cu);
        at(0x1d703du);
        state_.push(0x1d703du,0x1d7042u);
        apply_vehicle_force_(state_);
    }

    bool body_accepts_clamp() {
        reg(eax)=load(0x1d7052u,reg(edi)+component_rows);
        reg(ecx)=load(0x1d7055u,reg(eax)+reg(ebx)+motion_row);
        reg(ecx)=load(0x1d7059u,reg(ecx)+body_motion);
        reg(edx)=load(0x1d705cu,reg(ecx));
        call_motion_type(0x1d705eu,0x1d7061u,edx);
        at(0x1d7061u);
        state_.flags.minus(reg(eax),6,4);
        at(0x1d7064u);
        return !state_.flags.zf;
    }
    void load_linear_velocity() {
        auto &x=state_.xmm;
        reg(eax)=load(0x1d706au,reg(edi)+component_rows);
        reg(ecx)=load(0x1d706du,reg(eax)+reg(ebx)+motion_row);
        state_.set(edx,load(0x1d7071u,reg(ecx)+body_fixed,1),1);
        at(0x1d7074u);
        state_.flags.logic(reg(edx)&0xffu,1);
        reg(esi)=load(0x1d7076u,static_vector);
        at(0x1d707cu);
        if (state_.flags.zf) {
            reg(eax)=load(0x1d707eu,reg(ecx)+body_motion);
            x[2]=load_scalar(0x1d7081u,reg(eax)+0x40u);
            x[1]=load_scalar(0x1d7086u,reg(eax)+0x44u);
            x[0]=load_scalar(0x1d708bu,reg(eax)+0x48u);
            at(0x1d7090u);
            reg(eax)=state_.flags.plus(reg(eax),0x40u,4);
            store_scalar(0x1d7093u,stack(0x18u),x[2]);
            store_scalar(0x1d7099u,stack(0x1cu),x[1]);
            store_scalar(0x1d709fu,stack(0x20u),x[0]);
            at(0x1d70a5u);
        } else {
            reg(eax)=load(0x1d70a7u,reg(esi));
            store(0x1d70a9u,stack(0x18u),reg(eax));
            reg(eax)=load(0x1d70adu,reg(esi)+4u);
            x[2]=load_scalar(0x1d70b0u,stack(0x18u));
            store(0x1d70b6u,stack(0x1cu),reg(eax));
            reg(eax)=load(0x1d70bau,reg(esi)+8u);
            x[1]=load_scalar(0x1d70bdu,stack(0x1cu));
            reg(esi)=load(0x1d70c3u,static_vector);
            store(0x1d70c9u,stack(0x20u),reg(eax));
            x[0]=load_scalar(0x1d70cdu,stack(0x20u));
        }
    }
    void load_angular_velocity() {
        auto &x=state_.xmm;
        at(0x1d70d3u);
        state_.flags.logic(reg(edx)&0xffu,1);
        at(0x1d70d5u);
        if (state_.flags.zf) {
            reg(eax)=load(0x1d70d7u,reg(ecx)+body_motion);
            x[3]=load_scalar(0x1d70dau,reg(eax)+0x50u);
            at(0x1d70dfu);
            reg(eax)=state_.flags.plus(reg(eax),0x50u,4);
            store_scalar(0x1d70e2u,stack(0x24u),x[3]);
            x[3]=load_scalar(0x1d70e8u,reg(eax)+4u);
            store_scalar(0x1d70edu,stack(0x28u),x[3]);
            x[3]=load_scalar(0x1d70f3u,reg(eax)+8u);
            store_scalar(0x1d70f8u,stack(0x2cu),x[3]);
            at(0x1d70feu);
        } else {
            reg(ecx)=load(0x1d7100u,reg(esi));
            reg(edx)=load(0x1d7102u,reg(esi)+4u);
            reg(eax)=load(0x1d7105u,reg(esi)+8u);
            store(0x1d7108u,stack(0x24u),reg(ecx));
            store(0x1d710cu,stack(0x28u),reg(edx));
            store(0x1d7110u,stack(0x2cu),reg(eax));
        }
    }
    void spill_velocity(Word first,const std::array<SpillSites,3> &sites) {
        using namespace physics_step_detail;
        auto &fp=state_.fp;
        for (const auto &site:sites) {
            fp.push(ext(f(load(site.load,stack(first)))));
            at(site.multiply);
            fp.st(0)=mul(fp.st(0),fp.st(1));
            store(site.store,stack(first),u(spill(fp.st(0))));
            fp.pop();
            first+=4;
        }
    }
    void divide_limit(Word pc,Word limit) {
        using namespace physics_step_detail;
        const auto value=ext(f(load(pc,limit)));
        state_.fp.st(0)=physics_solver_detail::divide(value,state_.fp.st(0));
    }
    void clamp_linear_velocity() {
        using namespace physics_step_detail;
        auto &x=state_.xmm;
        auto &fp=state_.fp;
        at(0x1d7114u); x[3]=x[1];
        at(0x1d7117u); x[3]=_mm_mul_ss(x[3],x[1]);
        at(0x1d711bu); x[0]=_mm_mul_ss(x[0],x[0]);
        at(0x1d711fu); x[1]=x[2];
        at(0x1d7122u); x[0]=_mm_add_ss(x[0],x[3]);
        at(0x1d7126u); x[1]=_mm_mul_ss(x[1],x[2]);
        at(0x1d712au); x[0]=_mm_add_ss(x[0],x[1]);
        state_.flags.scalar_compare(x[0],load_scalar(0x1d712eu,0x46a348u),false);
        store_scalar(0x1d7135u,stack(0x10u),x[0]);
        at(0x1d713bu);
        if (state_.flags.cf || state_.flags.zf) return;

        fp.push(ext(f(load(0x1d713du,stack(0x10u)))));
        push(0x1d7141u,edi);
        at(0x1d7142u); fp.st(0)=physics_solver_detail::root(fp.st(0));
        at(0x1d7144u); reg(eax)=stack(0x1cu);
        at(0x1d7148u); reg(ecx)=reg(ebp);
        divide_limit(0x1d714au,0x4500b8u);
        spill_velocity(0x1cu,{{{0x1d7150u,0x1d7154u,0x1d7156u},
                              {0x1d715au,0x1d715eu,0x1d7160u},
                              {0x1d7164u,0x1d7168u,0x1d716au}}});
        at(0x1d716eu); fp.pop();
        call(0x1d7170u,0x1d7175u,0x1d9080u);
    }
    void clamp_angular_velocity() {
        using namespace physics_step_detail;
        auto &x=state_.xmm;
        auto &fp=state_.fp;
        x[1]=load_scalar(0x1d7175u,stack(0x28u));
        x[0]=load_scalar(0x1d717bu,stack(0x2cu));
        at(0x1d7181u); x[2]=x[1];
        at(0x1d7184u); x[2]=_mm_mul_ss(x[2],x[1]);
        x[1]=load_scalar(0x1d7188u,stack(0x24u));
        at(0x1d718eu); x[0]=_mm_mul_ss(x[0],x[0]);
        at(0x1d7192u); x[0]=_mm_add_ss(x[0],x[2]);
        at(0x1d7196u); x[2]=x[1];
        at(0x1d7199u); x[2]=_mm_mul_ss(x[2],x[1]);
        at(0x1d719du); x[0]=_mm_add_ss(x[0],x[2]);
        state_.flags.scalar_compare(x[0],load_scalar(0x1d71a1u,0x46a814u),false);
        store_scalar(0x1d71a8u,stack(0x14u),x[0]);
        at(0x1d71aeu);
        if (state_.flags.cf || state_.flags.zf) return;

        fp.push(ext(f(load(0x1d71b0u,stack(0x14u)))));
        push(0x1d71b4u,edi);
        at(0x1d71b5u); fp.st(0)=physics_solver_detail::root(fp.st(0));
        at(0x1d71b7u); reg(ecx)=stack(0x28u);
        at(0x1d71bbu); reg(eax)=reg(ebp);
        divide_limit(0x1d71bdu,0x46a1a4u);
        spill_velocity(0x28u,{{{0x1d71c3u,0x1d71c7u,0x1d71c9u},
                              {0x1d71cdu,0x1d71d1u,0x1d71d3u},
                              {0x1d71d7u,0x1d71dbu,0x1d71ddu}}});
        at(0x1d71e1u); fp.pop();
        call(0x1d71e3u,0x1d71e8u,0x1d9100u);
    }

    bool publish_body_and_check_island() {
        push(0x1d71e8u,edi);
        at(0x1d71e9u); reg(eax)=reg(ebp);
        call(0x1d71ebu,0x1d71f0u,0x1d7260u);
        reg(ecx)=load(0x1d71f0u,reg(edi)+component_rows);
        reg(ecx)=load(0x1d71f3u,reg(ecx)+reg(ebx)+motion_row);
        at(0x1d71f7u); reg(edx)=stack(0xfu);
        push(0x1d71fbu,edx);
        call(0x1d71fcu,0x1d7201u,0x315c90u);
        const auto active=load(0x1d7201u,reg(eax),1);
        state_.flags.minus(active,0,1);
        at(0x1d7204u);
        return !state_.flags.zf;
    }
    void apply_acceleration() {
        reg(eax)=load(0x1d7206u,reg(edi)+component_flags);
        at(0x1d7209u); reg(eax)=state_.flags.right(reg(eax),2,4,false);
        at(0x1d720cu); state_.flags.logic(reg(eax)&1u,1);
        at(0x1d720eu);
        if (!state_.flags.zf) return;

        reg(ecx)=load(0x1d7210u,reg(edi)+component_rows);
        reg(edx)=load(0x1d7213u,reg(ecx)+reg(ebx)+motion_row);
        reg(ecx)=load(0x1d7217u,reg(edx)+body_motion);
        reg(eax)=load(0x1d721au,reg(ecx));
        call_motion_type(0x1d721cu,0x1d721fu,eax);
        at(0x1d721fu); state_.flags.minus(reg(eax),6,4);
        at(0x1d7222u);
        if (state_.flags.zf) return;

        reg(ecx)=load(0x1d7224u,reg(edi)+component_rows);
        reg(edx)=load(0x1d7227u,reg(ecx)+reg(ebx)+motion_row);
        state_.set(eax,load(0x1d722bu,reg(edx)+body_fixed,1),1);
        at(0x1d722eu); state_.flags.logic(reg(eax)&0xffu,1);
        at(0x1d7230u);
        if (!state_.flags.zf) return;

        at(0x1d7232u); reg(edx)=stack(0x30u);
        at(0x1d7236u); reg(ecx)=reg(ebp);
        at(0x1d7238u); reg(esi)=reg(edi);
        call(0x1d723au,0x1d723fu,0x1d92c0u);
    }
    bool advance_body() {
        reg(eax)=load(0x1d723fu,reg(edi)+component_count);
        at(0x1d7242u); reg(ebp)=state_.flags.increment(reg(ebp),4,false);
        at(0x1d7243u); reg(ebx)=state_.flags.plus(reg(ebx),0x60u,4);
        at(0x1d7246u); state_.flags.minus(reg(ebp),reg(eax),4);
        at(0x1d7248u);
        return state_.flags.sf!=state_.flags.of;
    }
    void update_bodies() {
        reg(eax)=load(0x1d7042u,reg(edi)+component_count);
        push(0x1d7045u,ebp);
        at(0x1d7046u); reg(ebp)=state_.flags.logic(0,4);
        at(0x1d7048u); state_.flags.logic(reg(eax),4);
        at(0x1d704au);
        if (signed_word(reg(eax))>0) {
            at(0x1d7050u); reg(ebx)=state_.flags.logic(0,4);
            do {
                if (body_accepts_clamp()) {
                    load_linear_velocity();
                    load_angular_velocity();
                    clamp_linear_velocity();
                    clamp_angular_velocity();
                }
                // These child calls can republish the component's current
                // body rows, flags and count before the next decision.
                if (publish_body_and_check_island()) apply_acceleration();
            } while (advance_body());
        }
        pop(0x1d724eu,ebp);
        pop(0x1d724fu,ebx);
    }

public:
    Frame(State &state,Dispatch &dispatch,void (*apply_vehicle_force)(State &),std::uint32_t *cursor)
        :state_(state),dispatch_(dispatch),apply_vehicle_force_(apply_vehicle_force),cursor_(cursor) {}

    void update() {
        if (component_enabled()) {
            if (has_active_body()) {
                publish_object_and_acceleration();
                vehicle_force();
                update_bodies();
            }
            pop(0x1d7250u,esi);
        }
        ret();
    }
};

inline bool reference_requested() {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_OBJECT_COMPONENT_TICK");
        return value && *value && *value!='0';
    }();
    return reference;
}

template<class State,class Dispatch>
bool bridge(State &state,Dispatch dispatch,void (*apply_vehicle_force)(State &)) {
    if (state.image!=SourcePhysicsContactImage::TitleUpdate15 || reference_requested()) return false;
    // The World certifies that child calls restore its quiet cursor policy.
    // Observer-bearing or trace calls retain the ordinary per-PC delivery.
    const auto *policy=state.live_instruction_cursor();
    const bool quiet=policy && policy->literal_service_sites && state.literal_service_sites &&
        std::ranges::equal(policy->service_sites,native_census_detail::service_sites) &&
        !state.observe && !(policy->first_trace && *policy->first_trace) &&
        !(policy->second_trace && *policy->second_trace) && state.services.required_owner &&
        native_required_owner_policy_detail::preserves_instruction_policy(state.services);
    Frame frame(state,dispatch,apply_vehicle_force,quiet?policy->position:nullptr);
    frame.update();
    return true;
}

} // namespace halo2::native_object_component_tick
