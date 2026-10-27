#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include "halo2/source_object_audio_views_tu15.hpp"
#include "halo2/source_loop_sound_request_tu15.hpp"

namespace halo2 {
// Completed native pre-view publication for an ordinary, unattached-to-player
// object lsnd. The actual authority owns the salted loop/object generations,
// BCB70 control/admission,18F9B0 location and BC330 cluster. This is independent
// of its local cameras/PAS and is not an allocated client source-world record.
// Client playback owns its own camera selection, controllers and SDL voices.
struct SourceObjectLoopEmitterTu15 {
    std::uint32_t loop=0xffffffffu,object=0xffffffffu,definition=0xffffffffu;
    std::uint16_t source_flags{};
    bool function_active{};
    std::int16_t cluster=-1;
    float maximum_distance{},stop_duration{};
    // Complete original18F9B0 output, including actual node/marker position,
    // control, gain, velocity and root location. No pointer is dereferenced
    // by a receiver; only the owned bytes are transported.
    std::array<std::byte,68> location{};
    bool operator==(const SourceObjectLoopEmitterTu15&)const=default;
};
inline constexpr std::size_t source_object_loop_emitter_capacity_tu15=1024;
// Bounded schemas preserve source generations and finite original caller
// fields before a complete authoritative publication can replace the client
// bank. An available empty bank retires old owners; unavailable legacy data
// must not be interpreted as a successful empty source publication.
void validate_source_object_loop_emitters_tu15(std::span<const SourceObjectLoopEmitterTu15>);
struct MultiplayerAudioBsp;
// Compiled TU1.5 253070/18F900/190460 selection over the real converted BSP
// and the client's actually submitted cameras. No source World, listener
// record, PAS bit or source admission result is allocated or supplied.
float native_object_loop_cluster_scale_tu15(const MultiplayerAudioBsp&,int,int);
bool native_object_loop_within_distance_tu15(Vec3,float,const SourceObjectAudioViewsTu15&);
// Original18D5A0's ordinary attached-loop intent. Missing PAS/distance views
// produce no request: the native controller retires untouched owners. An
// inactive function supplies the authored stop/fade request. The copied
// envelope is native presentation input, not a claim that a peer executed
//220C60 or admitted a source voice. Historical backing argument4 is NONE.
std::vector<SourceLoopSoundRequestTu15> native_object_loop_requests_tu15(
    std::span<const SourceObjectLoopEmitterTu15>,const MultiplayerAudioBsp&,
    const SourceObjectAudioViewsTu15&);
}
