#pragma once

#include "physics/runtime/native_tu15_mopp_ray_dispatch.hpp"

namespace halo2::native_mopp_ray_dispatch_detail {
inline bool reference_dispatch_tables() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_DISPATCH_TABLES");
        return value&&*value;
    }();
    return reference;
}

template<class State>
bool read_dispatch_tables(State &s,std::uint32_t &target) noexcept {
    if constexpr(std::endian::native!=std::endian::little)return false;
    if(reference_dispatch_tables() || !admitted(s))return false;
    constexpr std::uint32_t index_table=0x30b738u, target_table=0x30b6b8u;
    constexpr auto page_base=target_table&~4095u;
    static_assert((target_table+255u*4u+3u)>>12u==page_base>>12u);
    const auto index_address=index_table+s.gp[0];
    if((index_address>>12u)!=(page_base>>12u))return false;

    std::uint32_t index;
    if(const auto *page=s.memory.direct_page(page_base,4096u)) {
        index=std::to_integer<std::uint32_t>(page->bytes[index_address&4095u]);
        std::memcpy(&target,page->bytes+(target_table&4095u)+index*4u,4u);
    } else {
        // World and AI publish the byte and dword tables as separate
        // immutable regions. Borrow the actual operands at their original
        // widths. Neither borrow can run a provider callback on refusal.
        const auto opcode=s.memory.try_read_view(index_address,1u);
        if(!opcode)return false;
        index=std::to_integer<std::uint32_t>((*opcode)[0]);
        const auto branch=s.memory.try_read_view(target_table+index*4u,4u);
        if(!branch)return false;
        std::memcpy(&target,branch->data(),4u);
    }
    // Both operands are ordinary, so a refused second view can leave
    // ECX and the cursor untouched for the genuine two-read continuation.
    s.gp[1]=index;
    retire(s,0x30aaa7u);
    return true;
}
} // namespace halo2::native_mopp_ray_dispatch_detail
