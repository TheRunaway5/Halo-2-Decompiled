#include "halo2/source_shadow_lighting.hpp"
#include "halo2/cache.hpp"
#include <array>

namespace halo2 {
SourceShadowLightingSelection source_shadow_lighting_selection(
    std::span<const SourceShadowLightingOwner> owners,std::uint32_t identity,
    bool default_present,const std::function<std::uint32_t(std::uint32_t)>& acquire,
    std::span<const std::optional<std::uint8_t>> valid) {
    constexpr auto none=0xffffffffu;
    std::array<const SourceShadowLightingOwner*,2048> slots{};
    for(const auto& owner:owners) {
        if(owner.slot>=slots.size() || slots[owner.slot])
            throw FormatError("source shadow lighting object slot invalid or duplicated");
        slots[owner.slot]=&owner;
    }
    auto get=[&](std::uint32_t handle)->const SourceShadowLightingOwner& {
        const auto slot=handle&0xffff;
        if(handle==none || slot>=slots.size() || !slots[slot])
            throw FormatError("source shadow lighting object slot absent");
        return *slots[slot];
    };
    std::array<bool,2048> visited{};
    for(;;) {
        const auto& current=get(identity);
        if(!current.object_flags)throw FormatError("source shadow lighting object flags absent");
        if(*current.object_flags&0x01000000u) {
            if(!default_present)throw FormatError("source shadow default lighting bank absent");
            return {SourceShadowLightingStorage::default_bank,identity,none};
        }
        if(!current.parent)throw FormatError("source shadow lighting parent absent");
        if(*current.parent==none)break;
        // Literal BAF80 visits the complete parent chain without testing any
        // intermediate object's lighting-default flag. 3DC90 restarts at root.
        for(;;) {
            const auto& node=get(identity);
            const auto slot=identity&0xffff;
            if(visited[slot])throw FormatError("source shadow lighting parent cycle");
            visited[slot]=true;
            if(!node.parent)throw FormatError("source shadow lighting parent absent");
            if(*node.parent==none)break;
            identity=*node.parent;
        }
    }
    if(!acquire)throw FormatError("source shadow lighting cache producer absent");
    const auto token=acquire(identity);
    if(token==none)return {SourceShadowLightingStorage::absent,identity,none};
    const auto slot=token&0xffff;
    if(slot>=256 || slot>=valid.size() || !valid[slot])
        throw FormatError("source shadow lighting cache validity byte absent");
    return {*valid[slot]?SourceShadowLightingStorage::owner_cache:SourceShadowLightingStorage::absent,identity,token};
}
}
