#pragma once

#include "physics/runtime/native_tu15_mopp_ray_traversal.hpp"
#include <bit>
#include <cstring>

namespace halo2::native_mopp_ray_dispatch_detail {

// Only ordinary resident bytes are batched. Device, short and split ranges
// go to the same retained branch chosen by the real dispatch table.
template<class State>
bool admitted(const State &s) noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_DISPATCH");
        return (value && *value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    if(reference || s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if(!s.services.instruction)return !s.instruction_cursor;
    const auto *cursor=s.live_instruction_cursor();
    return s.literal_service_sites && cursor &&
        (cursor==s.immutable_cursor.cursor || s.has_literal_service_sites(*cursor)) &&
        !(cursor->first_trace && *cursor->first_trace) &&
        !(cursor->second_trace && *cursor->second_trace);
}

template<class State>
void retire(State &s,std::uint32_t pc) noexcept {
    if(s.instruction_cursor)*s.instruction_cursor->position=pc;
}

template<unsigned Count,class State>
bool jump_case(State &s) noexcept {
    static_assert(Count>=1 && Count<=3);
    if constexpr(std::endian::native!=std::endian::little)return false;
    if(!admitted(s))return false;
    auto &g=s.gp;
    const auto program=g[6];
    const auto operand=program+1u;
    const auto *page=s.memory.direct_page(operand,Count);
    if(!page)return false;
    std::array<std::uint8_t,Count> bytes;
    std::memcpy(bytes.data(),page->bytes+(operand&4095u),Count);
    if constexpr(Count==1) {
        g[0]=bytes[0];
        g[6]=program+2u+g[0];
        retire(s,0x30b330u);
    } else {
        // The original final ADD overwrites every arithmetic flag from the
        // earlier shifts. There are no callbacks between these register edits.
        g[2]=s.flags.plus(bytes.back(),program,4);
        if constexpr(Count==2) {
            g[1]=std::uint32_t(bytes[0])<<8u;
            g[6]=g[2]+g[1]+3u;
            retire(s,0x30b346u);
        } else {
            g[1]=bytes[1];
            g[0]=(std::uint32_t(bytes[0])<<16u)|(std::uint32_t(bytes[1])<<8u);
            g[6]=g[2]+g[0]+4u;
            retire(s,0x30b365u);
        }
    }
    return true;
}

template<std::uint32_t ReturnSite,class State>
bool ordinary_return_frame(State &s) noexcept {
    if constexpr(std::endian::native!=std::endian::little)return false;
    auto &g=s.gp;
    const auto stack=g[4],frame=g[5];
    const auto *saved=s.memory.direct_page(stack,12u);
    const auto *return_frame=s.memory.direct_page(frame,8u);
    if(!saved || !return_frame)return false;
    std::array<std::uint32_t,3> registers;
    std::array<std::uint32_t,2> returned;
    std::memcpy(registers.data(),saved->bytes+(stack&4095u),12u);
    std::memcpy(returned.data(),return_frame->bytes+(frame&4095u),8u);
    g[7]=registers[0];
    g[6]=registers[1];
    g[3]=registers[2];
    g[5]=returned[0];
    g[4]=frame+20u;
    retire(s,ReturnSite);
    return true;
}

template<class State,std::uint32_t ReturnSite=0x30b6b5u>
bool empty_case(State &s) noexcept {
    if(physics_contact_runtime_detail::reference_scalar_stack_pops() || !admitted(s))return false;
    return ordinary_return_frame<ReturnSite>(s);
}
} // namespace halo2::native_mopp_ray_dispatch_detail
