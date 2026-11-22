#pragma once
#include "halo2/physics_contact_runtime.hpp"

namespace halo2::native_campaign_ai_workspace_clear {
inline constexpr std::uint32_t site=0x1c5993u;
inline constexpr std::array<std::uint32_t,0x1d6> zero_words{};
inline constexpr std::uint32_t bytes=zero_words.size()*sizeof(std::uint32_t);
inline constexpr std::uint32_t hash_site=0x27b2ddu,hash_offset=0x120b6u;
inline constexpr auto empty_hash=[] {
    std::array<std::uint32_t,0x800> words{};
    words.fill(0xffffffffu);
    return words;
}();
inline constexpr std::uint32_t hash_bytes=empty_hash.size()*sizeof(std::uint32_t);

inline bool owns_path_hash_workspace(const SourcePhysicsContactMemory &memory,std::uint32_t workspace) {
    constexpr std::uint32_t lease_bytes=0x22974u;
    if(!workspace || std::uint64_t(workspace)+lease_bytes>0x100000000ull)return false;
    const auto initialized=memory.try_read_view(0x52be58u,1);
    if(!initialized || (*initialized)[0]!=std::byte{1})return false;
    const auto backing=memory.try_read_view(0x51df44u,4);
    const auto descriptors=memory.try_read_view(0x50208cu,24);
    if(!backing || !descriptors)return false;
    const auto word=[](std::span<const std::byte> data,std::size_t offset) {
        std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(data[offset+n])<<(8u*n);
        return value;
    };
    const auto arena=word(*backing,0);
    if(!arena)return false;
    // 1D2D90 publishes two consecutive leases at 50208C. An inactive
    // descriptor or a changed backing/size keeps the original store path.
    for(unsigned n=0;n<2;++n) {
        const auto offset=n*12u;
        if(std::uint64_t(arena)+n*lease_bytes==workspace &&
           (*descriptors)[offset]==std::byte{1} &&
           word(*descriptors,offset+4u)==workspace &&
           word(*descriptors,offset+8u)==lease_bytes)
            return true;
    }
    return false;
}

// The bound body has already delivered the instruction at site. Leave the
// original loop in charge of observer delivery, direction and fault prefixes
// unless the live World can commit the entire forward clear.
template<class State>
bool try_clear(State &state) {
    const auto &clear=state.services.clear_ai_search_workspace;
    auto &gp=state.gp;
    if(!clear || state.observe || state.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       (state.other_flags&0x400u) || gp[0]!=0 || gp[1]!=zero_words.size() ||
       gp[7]!=gp[4]+0x20u)
        return false;
    if(!clear(state.services,state.memory,site,gp[7]))return false;
    gp[7]+=bytes;
    gp[1]=0;
    return true;
}
template<class State>
bool try_clear_path_hash(State &state) {
    const auto &clear=state.services.clear_ai_search_workspace;
    auto &gp=state.gp;
    if(!clear || state.observe || state.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       (state.other_flags&0x400u) || gp[0]!=0xffffffffu || gp[1]!=empty_hash.size() ||
       gp[7]!=gp[6]+hash_offset)
        return false;
    if(!clear(state.services,state.memory,hash_site,gp[6]))return false;
    gp[7]+=hash_bytes;
    gp[1]=0;
    return true;
}
} // namespace halo2::native_campaign_ai_workspace_clear
