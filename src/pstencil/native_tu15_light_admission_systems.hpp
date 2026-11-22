#pragma once

#include <cstddef>
#include <cstdint>

namespace halo2::native_light_admission {
struct LightPool {
    std::byte header[68];
    std::uint32_t records;
};
struct LightRecord {
    std::uint32_t header;
    std::uint32_t definition;
    std::byte before_cluster[64];
    std::uint16_t cluster;
    std::byte tail[198];
};
struct TagRecord {
    std::uint32_t group, identity, definition, size;
};
static_assert(offsetof(LightPool, records) == 68);
static_assert(offsetof(LightRecord, definition) == 4 && offsetof(LightRecord, cluster) == 72);
static_assert(sizeof(LightRecord) == 272 && offsetof(TagRecord, definition) == 8 && sizeof(TagRecord) == 16);

inline constexpr std::uint32_t no_light = UINT32_MAX;
inline constexpr std::uint16_t no_cluster = UINT16_MAX;
inline constexpr std::uint32_t datum_index_mask = UINT16_MAX;
inline constexpr std::uint32_t allow_without_cluster = 1u << 5;
inline constexpr std::uint32_t cinematic_view_only = 1u << 10;
inline constexpr std::uint32_t use_view_cluster = 1u << 18;
inline constexpr std::uint32_t filter_visibility = 1u << 21;
inline constexpr std::uint16_t cinematic_view = 2;

template<class Frame> void admit(Frame& frame) {
    const auto light = frame.begin();
    if(light == no_light || !frame.light_enabled(light)) {
        frame.reject();
        return;
    }

    const auto record = frame.light_record(light & datum_index_mask, sizeof(LightRecord));
    const auto definition = frame.definition(record, offsetof(LightRecord, definition),
        datum_index_mask, sizeof(TagRecord), offsetof(TagRecord, definition));
    if(frame.has_visible_shape(light)) {
        frame.build_shape(light);
        const auto flags = frame.shape_flags(definition, cinematic_view_only);
        const bool view_allows = !(flags & cinematic_view_only) || frame.in_view_mode(cinematic_view);
        frame.complete_view_gate(view_allows);
    } else {
        frame.read_view_gate();
    }

    frame.apply_visibility_gate(definition, filter_visibility);
    const auto cluster = frame.cluster(record, offsetof(LightRecord, cluster), no_cluster);
    if(cluster != no_cluster) {
        frame.finish(true);
        return;
    }
    const auto flags = frame.unclustered_flags(definition, allow_without_cluster);
    frame.finish((flags & allow_without_cluster) && frame.uses_view_cluster(flags, use_view_cluster));
}
} // namespace halo2::native_light_admission
