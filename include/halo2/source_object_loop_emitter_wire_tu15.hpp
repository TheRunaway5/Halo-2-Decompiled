#pragma once
#include "halo2/source_object_loop_emitter_tu15.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <optional>
#include <vector>

namespace halo2 {
enum class SourceObjectLoopEmitterWireAnchorTu15 : unsigned { frame,emitter };
inline constexpr std::size_t source_object_loop_emitter_wire_bytes_tu15=93;
// This is a pre-view source bank, independent of the authority's listeners.
// Absence is unavailable; a present empty bank retires previous emitters.
// Writer supplies word(value,width), real(float), anchor(kind,full_identity).
template<class Writer>
void encode_source_object_loop_emitters_wire_tu15(Writer &out,
    const std::optional<std::vector<SourceObjectLoopEmitterTu15>> &emitters) {
    if(emitters)validate_source_object_loop_emitters_tu15(*emitters);
    out.anchor(SourceObjectLoopEmitterWireAnchorTu15::frame,0xffffffffu);
    out.word(emitters.has_value(),1);if(!emitters)return;
    out.word(emitters->size(),2);
    for(const auto &emitter:*emitters) {
        out.anchor(SourceObjectLoopEmitterWireAnchorTu15::emitter,emitter.loop);
        out.word(emitter.loop,4);out.word(emitter.object,4);out.word(emitter.definition,4);
        out.word(emitter.source_flags,2);out.word(emitter.function_active,1);
        out.word(std::bit_cast<std::uint16_t>(emitter.cluster),2);
        out.real(emitter.maximum_distance);out.real(emitter.stop_duration);
        for(auto value:emitter.location)out.word(std::to_integer<unsigned char>(value),1);
    }
}
// Reader supplies word(width), finite real(), remaining(). Bounds are checked
// before reserving a local bank; the caller adopts it only after validation.
template<class Reader>
std::optional<std::vector<SourceObjectLoopEmitterTu15>> decode_source_object_loop_emitters_wire_tu15(Reader &in) {
    const auto present=in.word(1);
    if(present>1)throw FormatError("Invalid native object-loop emitter presence");
    if(!present)return {};
    const auto count=in.word(2);
    if(count>source_object_loop_emitter_capacity_tu15 ||
        count*source_object_loop_emitter_wire_bytes_tu15>in.remaining())
        throw FormatError("Native object-loop emitter bank exceeds source capacity or snapshot extent");
    std::vector<SourceObjectLoopEmitterTu15> emitters;emitters.reserve(count);
    for(std::size_t n=0;n<count;++n) {
        SourceObjectLoopEmitterTu15 emitter;
        emitter.loop=in.word(4);emitter.object=in.word(4);emitter.definition=in.word(4);
        emitter.source_flags=in.word(2);
        const auto active=in.word(1);
        if(active>1)throw FormatError("Invalid native object-loop function admission");
        emitter.function_active=active;
        emitter.cluster=std::bit_cast<std::int16_t>(std::uint16_t(in.word(2)));
        emitter.maximum_distance=in.real();emitter.stop_duration=in.real();
        for(auto &value:emitter.location)value=std::byte(in.word(1));
        emitters.push_back(emitter);
    }
    validate_source_object_loop_emitters_tu15(emitters);return emitters;
}
}
