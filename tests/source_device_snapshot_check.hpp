#pragma once
#include "halo2/source_device_world.hpp"
#include <bit>
#include <cstring>

namespace halo2::test {
inline void same_device_events(std::span<const SourceDeviceWorldEvent> a,std::span<const SourceDeviceWorldEvent> b){
    if(a.size()!=b.size())throw FormatError("authoritative device event count differs");
    for(std::size_t i=0;i<a.size();++i)if(a[i].handle!=b[i].handle || a[i].tag!=b[i].tag ||
        a[i].serial!=b[i].serial || a[i].tick!=b[i].tick)throw FormatError("authoritative device event differs");
}
inline void same_device_snapshot(const SourceDeviceWorldSnapshot&a,const SourceDeviceWorldSnapshot&b){
    const auto require=[](bool value){if(!value)throw FormatError("authoritative device snapshot differs");};
    const auto scalar=[&](float x,float y){require(std::bit_cast<std::uint32_t>(x)==std::bit_cast<std::uint32_t>(y));};
    const auto group=[&](const SourceDeviceGroup&x,const SourceDeviceGroup&y){scalar(x.value,y.value);require(x.flags==y.flags);};
    require(a.tick==b.tick && a.groups.size()==b.groups.size() && a.owners.size()==b.owners.size());
    for(std::size_t i=0;i<a.groups.size();++i)group(a.groups[i],b.groups[i]);
    for(std::size_t i=0;i<a.owners.size();++i){const auto&x=a.owners[i].state;const auto&y=b.owners[i].state;
        require(a.owners[i].definition==b.owners[i].definition && x.handle==y.handle && x.placement==y.placement &&
            x.created_tick==y.created_tick && x.control_flags==y.control_flags && x.visible==y.visible && x.bsp_hidden==y.bsp_hidden);
        scalar(x.motion.position,y.motion.position);scalar(x.motion.position_velocity,y.motion.position_velocity);
        scalar(x.motion.power,y.motion.power);scalar(x.motion.power_velocity,y.motion.power_velocity);
        require(x.motion.flags==y.motion.flags && x.motion.delay_ticks==y.motion.delay_ticks);
        require(x.door.flags==y.door.flags && x.door.open_ticks==y.door.open_ticks);
        group(x.private_power,y.private_power);group(x.private_position,y.private_position);
        static_assert(sizeof(Transform)==13*sizeof(float));
        require(std::memcmp(&x.root,&y.root,sizeof(Transform))==0);
    }
}
}
