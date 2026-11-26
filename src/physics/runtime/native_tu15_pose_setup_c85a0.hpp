#pragma once

#include "physics/runtime/native_tu15_two_bone_solve.hpp"

namespace halo2::native_pose_setup_c85a0 {

using namespace native_object_tick_detail::boundary;
using physics_step_detail::r;
using physics_step_detail::w;
using native_tu15_leaves::ordered_mul_ss;

enum class Resume { retained_entry, returned, authored_axis, blend };

template<class State> bool selected(const State &state) {
    return state.image == SourcePhysicsContactImage::TitleUpdate15;
}

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_POSE_SETUP_C85A0");
        return value && *value;
    }();
    return enabled;
}

template<class State> struct Frame {
    State &s;
    decltype(State::gp) &g = s.gp;
    decltype(State::flags) &flags = s.flags;
    decltype(State::xmm) &x = s.xmm;
    const typename State::InstructionCursor *cursor = s.immutable_cursor.cursor;

    bool quiet() const {
        return cursor && !(cursor->first_trace && *cursor->first_trace) &&
            !(cursor->second_trace && *cursor->second_trace);
    }
    void native_site(std::uint32_t pc) {
        if(!quiet()) s.instruction_literal(pc);
    }
    void access(std::uint32_t pc) {
        if(quiet()) *cursor->position = pc;
        else s.instruction_literal(pc);
    }
    template<class Address> std::uint32_t read(std::uint32_t pc, Address address, unsigned size = 4) {
        access(pc);
        return r(s.memory,address(),size);
    }
    template<class Address, class Value>
    void store(std::uint32_t pc, Address address, Value value, unsigned size = 4) {
        access(pc);
        w(s.memory,s.observe,pc,address(),size,value());
    }
    template<class Value> void push(std::uint32_t pc, Value value) {
        access(pc);
        native_two_bone_solve::push_word(s,pc,value());
    }
    auto stack(std::uint32_t byte_offset) { return [this,byte_offset]{return g[esp]+byte_offset;}; }
    auto field(unsigned reg, std::uint32_t byte_offset = 0u) { return [this,reg,byte_offset]{return g[reg]+byte_offset;}; }
    auto constant(std::uint32_t value) { return [value]{return value;}; }
    auto scalar_bits(unsigned reg) { return [this,reg]{return physics_step_detail::u(_mm_cvtss_f32(x[reg]));}; }

    bool resolve_pose() {
        constexpr std::uint32_t object_datums = 0x4ecbd0u, model_datums = 0x4f0414u;
        native_site(0xc85a0u);
        g[esp] = flags.minus(g[esp],0x7cu,4);
        g[ecx] = read(0xc85a3u,stack(0x80u));
        g[edx] = read(0xc85aau,constant(object_datums));
        g[edx] = read(0xc85b0u,field(edx,0x44u));
        native_site(0xc85b3u);
        g[eax] = g[ecx];
        native_site(0xc85b5u);
        g[eax] = flags.logic(g[eax] & 0xffffu,4);
        native_site(0xc85bau);
        g[eax] *= 3u;
        push(0xc85bdu,field(ebp));
        g[ebp] = read(0xc85beu,[&]{return g[edx] + g[eax]*4u + 8u;});
        g[eax] = read(0xc85c2u,field(ebp));
        g[edx] = read(0xc85c5u,constant(model_datums));
        native_site(0xc85cbu);
        g[eax] = flags.logic(g[eax] & 0xffffu,4);
        native_site(0xc85d0u);
        g[eax] = flags.shift(g[eax],4u,4);
        push(0xc85d3u,field(esi));
        g[esi] = read(0xc85d4u,[&]{return g[eax] + g[edx] + 8u;});
        g[edx] = read(0xc85d8u,field(esi,0xbcu));
        native_site(0xc85deu);
        s.set(eax,flags.logic(0u,1),1,0);
        native_site(0xc85e0u);
        g[edx] = flags.right(g[edx],10u,4,false);
        native_site(0xc85e3u);
        flags.logic(g[edx] & 1u,1);
        store(0xc85e6u,stack(0x1cu),field(esi));
        store(0xc85eau,stack(0x0bu),[&]{return g[eax] & 0xffu;},1);
        native_site(0xc85eeu);
        if(!flags.zf) return false;

        s.set(edx,read(0xc85f4u,field(ebp,0x10au),1),1,0);
        native_site(0xc85fau);
        s.set(edx,flags.right(g[edx] & 0xffu,2u,1,false),1,0);
        native_site(0xc85fdu);
        flags.logic(g[edx] & 1u,1);
        native_site(0xc8600u);
        return flags.zf;
    }

    void reject_pose() {
        access(0xc8c14u);
        g[esi] = s.pop();
        access(0xc8c15u);
        g[ebp] = s.pop();
        native_site(0xc8c16u);
        g[esp] = flags.plus(g[esp],0x7cu,4);
        access(0xc8c19u);
        s.pop();
        g[esp] += 4u;
    }

    void save_initial_positions() {
        push(0xc8606u,field(ebx));
        native_site(0xc8607u);
        g[ebx] = g[ebp] + 0x168u;
        native_site(0xc860du);
        g[eax] = g[ebx];
        g[edx] = read(0xc860fu,field(eax));
        store(0xc8611u,stack(0x24u),field(edx));
        g[edx] = read(0xc8615u,field(eax,4u));
        g[eax] = read(0xc8618u,field(eax,8u));
        store(0xc861bu,stack(0x2cu),field(eax));
        native_site(0xc861fu);
        g[eax] = g[ebp] + 0x18cu;
        store(0xc8625u,stack(0x28u),field(edx));
        g[edx] = read(0xc8629u,field(eax));
        store(0xc862bu,stack(0x48u),field(edx));
        g[edx] = read(0xc862fu,field(eax,4u));
        g[eax] = read(0xc8632u,field(eax,8u));
        store(0xc8635u,stack(0x50u),field(eax));
        const auto scale_mode = read(0xc8639u,field(ebp,0x1bcu),1);
        flags.minus(scale_mode,1u,1);
        store(0xc8640u,stack(0x4cu),field(edx));
    }

    bool first_axis_can_reset() {
        using physics_step_detail::f;
        native_site(0xc8644u);
        if(flags.zf) {
            x[2] = _mm_set_ss(f(read(0xc8646u,field(esi,0x14cu))));
            store(0xc864eu,stack(0x18u),scalar_bits(2));
            native_site(0xc8654u);
        } else {
            x[0] = _mm_set_ss(f(read(0xc8656u,constant(0x46a0d0u))));
            store(0xc865eu,stack(0x18u),scalar_bits(0));
            native_site(0xc8664u);
            x[2] = x[0];
        }
        x[0] = _mm_set_ss(f(read(0xc8667u,field(esi,0x144u))));
        x[1] = _mm_set_ss(f(read(0xc866fu,field(esi,0x148u))));
        push(0xc8677u,field(edi));
        const auto offset = std::uint16_t(read(0xc8678u,field(ebp,0x33eu),2));
        g[edi] = std::uint32_t(std::bit_cast<std::int16_t>(offset));
        native_site(0xc867fu);
        x[0] = ordered_mul_ss(x[0],x[2]);
        native_site(0xc8683u);
        g[edi] = flags.plus(g[edi],g[ebp],4);
        native_site(0xc8685u);
        x[1] = ordered_mul_ss(x[1],x[2]);
        native_site(0xc8689u);
        x[2] = _mm_setzero_ps();
        native_site(0xc868cu);
        flags.scalar_compare(x[0],x[2],true);
        native_site(0xc868fu);
        s.set(eax,flags.bits() | 2u,1,8);
        native_site(0xc8690u);
        flags.logic((g[eax] >> 8u) & 0x44u,1);
        store(0xc8693u,stack(0x20u),field(edi));
        store(0xc8697u,stack(0x14u),scalar_bits(0));
        store(0xc869du,stack(0x18u),scalar_bits(1));
        native_site(0xc86a3u);
        if(flags.pf) return false;
        native_site(0xc86a5u);
        flags.scalar_compare(x[1],x[2],true);
        native_site(0xc86a8u);
        s.set(eax,flags.bits() | 2u,1,8);
        native_site(0xc86a9u);
        flags.logic((g[eax] >> 8u) & 0x44u,1);
        native_site(0xc86acu);
        return !flags.pf;
    }

    template<class Available, class Update>
    void reset_first_axis(Available &available, Update &update) {
        native_site(0xc86aeu);
        g[edx] = g[ebp] + 0x15cu;
        g[edi] = read(0xc86b4u,field(edx));
        native_site(0xc86b6u);
        g[eax] = g[ebx];
        store(0xc86b8u,field(eax),field(edi));
        g[edi] = read(0xc86bau,field(edx,4u));
        g[edx] = read(0xc86bdu,field(edx,8u));
        store(0xc86c0u,field(eax,4u),field(edi));
        store(0xc86c3u,field(eax,8u),field(edx));
        push(0xc86c6u,constant(0xc86cbu));
        available(s);
        native_site(0xc86cbu);
        flags.logic(g[eax] & 0xffu,1);
        native_site(0xc86cdu);
        if(!flags.zf) {
            g[eax] = read(0xc86cfu,stack(0x90u));
            push(0xc86d6u,constant(0));
            push(0xc86d8u,field(ebx));
            push(0xc86d9u,constant(0xc86deu));
            update(s);
            native_site(0xc86deu);
            x[2] = _mm_setzero_ps();
        }
        g[edi] = read(0xc86e1u,constant(0x474d04u));
        native_site(0xc86e7u);
        g[ecx] = g[edi];
        g[edx] = read(0xc86e9u,field(ecx));
        native_site(0xc86ebu);
        g[eax] = g[ebp] + 0x174u;
        store(0xc86f1u,field(eax),field(edx));
        g[edx] = read(0xc86f3u,field(ecx,4u));
        store(0xc86f6u,field(eax,4u),field(edx));
        g[ecx] = read(0xc86f9u,field(ecx,8u));
        store(0xc86fcu,field(eax,8u),field(ecx));
        native_site(0xc86ffu);
    }
};

template<class State, class Available, class Update>
Resume run(State &s,Available available,Update update) {
    if(!selected(s) || reference()) return Resume::retained_entry;
    Frame<State> frame{s};
    if(!frame.resolve_pose()) {
        frame.reject_pose();
        return Resume::returned;
    }
    frame.save_initial_positions();
    if(!frame.first_axis_can_reset()) return Resume::authored_axis;
    frame.reset_first_axis(available,update);
    return Resume::blend;
}

} // namespace halo2::native_pose_setup_c85a0
