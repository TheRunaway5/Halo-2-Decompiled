#pragma once

#include "physics/runtime/native_tu15_mopp_ray_shared_return.hpp"
#include <bit>

namespace halo2::native_mopp_ray_opcode_resume_detail {

enum class Continuation : std::uint8_t { original, read_opcode, store_default, branch_default, classify, diagnostic };

inline bool reference_opcode_resume() noexcept {
    static const bool retained=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_OPCODE_RESUME");
        return (value && *value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return retained;
}

inline bool reference_opcode_provider() noexcept {
    static const bool retained=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_OPCODE_PROVIDER");
        return value && *value;
    }();
    return retained;
}

template<class State>
bool admits_resume(const State &s,const typename State::InstructionCursor *cursor) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15 || s.observe)return false;
    if(!s.services.instruction)return !s.instruction_cursor;
    return cursor && s.literal_service_sites &&
        (cursor==s.immutable_cursor.cursor || s.has_literal_service_sites(*cursor)) &&
        !(cursor->first_trace && *cursor->first_trace) &&
        !(cursor->second_trace && *cursor->second_trace);
}

template<bool PopFirst=false,class State>
Continuation resume(State &s) {
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    const auto *cursor=s.live_instruction_cursor();
    if(reference_opcode_resume() || !admits_resume(s,cursor))return Continuation::original;

    if constexpr(PopFirst) {
        using namespace native_mopp_ray_shared_return_detail;
        if(reference_shared_return() || s.active_mmx || !s.fp.count)return Continuation::original;
        pop_x87<1u>(s);
        if(cursor)*cursor->position=0x30ae00u;
    }
    auto &g=s.gp;
    const auto *page=s.memory.direct_page(g[6],1u);
    if(!page)return PopFirst?Continuation::read_opcode:Continuation::original;
    g[0]=std::to_integer<std::uint32_t>(page->bytes[g[6]&4095u]);
    s.flags.minus(g[0],0x6bu,4);
    constexpr std::uint32_t default_fraction=999u;
    constexpr auto store_pc=0x30ae08u;
    const auto address=g[4]+0x3cu;
    if(cursor)*cursor->position=store_pc;
    if(!s.memory.try_callback_free_write(address,std::as_bytes(std::span{&default_fraction,1}))) {
        if(reference_opcode_provider()) {
            if(cursor)*cursor->position=0x30ae05u;
            return Continuation::store_default;
        }
        physics_step_detail::w(s.memory,s.observe,store_pc,address,4u,default_fraction);
        // The real store may activate tracing or replace the instruction
        // service before the conditional branch. Never repeat that store.
        const auto *current=s.live_instruction_cursor();
        if(current!=cursor || !admits_resume(s,current))return Continuation::branch_default;
    }
    if(cursor)*cursor->position=0x30ae10u;
    return s.flags.cf||s.flags.zf ? Continuation::classify : Continuation::diagnostic;
}

} // namespace halo2::native_mopp_ray_opcode_resume_detail
