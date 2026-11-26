#pragma once
#include "physics/runtime/native_tu15_mopp_ray_traversal.hpp"
#include <bit>

namespace halo2::native_mopp_ray_scale_tail_detail {
enum class Continuation { original, write_shift, read_scale, read_property, write_property,
    read_key, write_key, set_variables, resume };

inline bool reference_scale_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SCALE_PROVIDER");
        return value && *value;
    }();
    return reference;
}

inline bool reference_scale_tail() {
    static const bool retained=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SCALE_TAIL");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return retained;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State>
const auto *admitted_cursor(const State &s) {
    const auto *position=s.live_instruction_cursor();
    if(!position || !s.literal_service_sites ||
       (position!=s.immutable_cursor.cursor && !s.has_literal_service_sites(*position)) ||
       (position->first_trace && *position->first_trace) ||
       (position->second_trace && *position->second_trace))return decltype(position){};
    return position;
}

template<class State>
bool provider_continuation(const State &s,const typename State::InstructionCursor *position) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15 || s.observe || s.active_mmx)
        return false;
    const auto *current=admitted_cursor(s);
    return current==position && (s.services.instruction ? current!=nullptr : !s.instruction_cursor);
}

template<class State,class Value>
bool read_operand(const State &s,std::uint32_t address,Value &value) {
    if(!reference_scale_provider())
        return native_raw_x87_detail::direct_read(s.memory,address,
            {reinterpret_cast<std::byte *>(&value),sizeof(Value)});
    if(const auto *page=s.memory.direct_page(address,sizeof(Value))) {
        std::memcpy(&value,page->bytes+(address&4095u),sizeof(Value));
        return true;
    }
    return false;
}
#endif

template<class State,class Spill>
Continuation scale_tail(State &s,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_scale_tail() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || !s.fp.count || s.active_mmx)return Continuation::original;
    const auto *position=admitted_cursor(s);
    if(s.services.instruction ? !position : s.instruction_cursor != nullptr)return Continuation::original;
    const auto publish=[&](std::uint32_t pc) {
        if(position)*position->position=pc;
    };
    auto &g=s.gp;
    using namespace physics_step_detail;
    const auto store=[&](std::uint32_t pc,unsigned offset,std::uint32_t value,
                         Continuation before,Continuation after)->std::optional<Continuation> {
        publish(pc);
        const auto address=g[4]+offset;
        if(s.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1})))return {};
        if(reference_scale_provider())return before;
        w(s.memory,s.observe,pc,address,4u,value);
        if(!provider_continuation(s,position))return after;
        return {};
    };
    // A refused read retains its original provider access. After a real
    // write, resume only from the next instruction with current frame state.
    std::uint32_t word;
    if(!read_operand(s,g[3]+0x10u,word))return Continuation::original;
    g[1]=s.flags.plus(word,g[0],4);
    if(const auto next=store(0x30b412u,0x50u,g[1],
        Continuation::write_shift,Continuation::read_scale))return *next;

    if(!s.fp.count || !read_operand(s,g[3]+0x14u,word))return Continuation::read_scale;
    publish(0x30b416u);
    s.fp.st(0)=mul(s.fp.st(0),ext(f(word)));
    g[6]=s.flags.plus(g[6],4u,4);
    publish(0x30b41cu);
    const auto scale=spill(s.fp.st(0),s.fp.status);
    const auto address=g[4]+0x54u;
    const bool direct=s.memory.try_callback_free_write(address,std::as_bytes(std::span{&scale,1}));
    if(!direct) {
        // The x87 spill already updated C1. Use its result for the original
        // write and leave ST(0) live until that write completes.
        w(s.memory,s.observe,0x30b41cu,address,4,scale);
    }
    s.fp.pop();
    if(!direct && (reference_scale_provider() || !provider_continuation(s,position)))
        return Continuation::read_property;

    if(!read_operand(s,g[3]+0x1cu,g[2]))return Continuation::read_property;
    if(const auto next=store(0x30b423u,0x5cu,g[2],
        Continuation::write_property,Continuation::read_key))return *next;
    if(!read_operand(s,g[3]+0x18u,g[0]))return Continuation::read_key;
    if(const auto next=store(0x30b42au,0x58u,g[0],
        Continuation::write_key,Continuation::set_variables))return *next;
    g[3]=g[4]+0x40u;
    publish(0x30b432u);
    return Continuation::resume;
#else
    (void)s;(void)spill;
    return Continuation::original;
#endif
}
} // namespace halo2::native_mopp_ray_scale_tail_detail
