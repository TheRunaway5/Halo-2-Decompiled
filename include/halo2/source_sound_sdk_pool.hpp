#pragma once
#include <cstdint>
namespace halo2 {
struct SourceSoundSdkPoolLayout {
    std::uint32_t backing_bytes,client_offset,client_capacity,alignment;
    friend bool operator==(const SourceSoundSdkPoolLayout &,const SourceSoundSdkPoolLayout &)=default;
};
// Capacity rules recovered from the observed owned Complex4627 kernel while
// unchanged TU1.5 constructs its SDK device. This does not emulate kernel
// free lists, physical availability, metadata clobbers or failure statuses.
SourceSoundSdkPoolLayout source_sound_sdk_pool_layout(std::uint32_t requested);
}
