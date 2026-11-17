#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <cstdint>
#include <functional>
#include <stdexcept>

namespace halo2 {
// Per-object render lighting: the whole TU1.5 3DAF0 tree over the current
// banks. 3DAF0 returns the 84-byte lighting bank of an object (the render
// cache record's +54 bank of its root, a baked probe, or the default) and
// keeps the root's cache record (3DDC0 allocation/eviction, 3DD00 colors,
// 3DFD0 query cadence, 3E590/3E4D0 interpolation and D65C0/D5D30 conversion)
// current. All of it runs natively. The D3D90 collision/lightmap query is the
// one owner reached through a service; 13DE70 authored functions run on the
// native evaluator and use `function` only for that evaluator's refusals
// (which happen before any store, as the evaluator stores nothing).
struct NativeTu15ObjectLightingServices {
    // D3D90(EDI=object, first, sample): AL. `first` is 3DFD0's fresh-record
    // byte; the query may write the five-dword sample at `sample`.
    std::function<bool(std::uint32_t object,std::uint32_t first,std::uint32_t sample)> query;
    // 13DE70(EDI=reference, parameter, second): XMM0 bits.
    std::function<std::uint32_t(std::uint32_t reference,std::uint32_t parameter,std::uint32_t second)> function;
};
struct NativeTu15ObjectLightingStats {
    std::uint64_t calls{},default_banks{},probe_banks{},roots{},children{},uncached_children{},
        acquisitions{},allocations{},evictions{},updates{},unchanged{},frozen{},due{},near_skips{},
        queries{},query_hits{},interpolations{},rotations{},snaps{},conversions{},
        conversion_fallbacks{},color_publications{},function_refusals{};
};
// Thrown before any store when the root has no cache record (the 3DC10
// static-bank path, whose sample is an uninitialized original stack local).
struct NativeTu15ObjectLightingDecline : std::runtime_error {
    using std::runtime_error::runtime_error;
};
// Whole 3DAF0(EAX object, metric bits). Requires masked original FP
// exceptions and the original x87 control word 027F (std::invalid_argument,
// no effects). The arithmetic follows the original SSE/x87 operation order.
std::uint32_t native_tu15_object_lighting_3daf0(SourcePhysicsContactMemory&,std::uint32_t object,
    std::uint32_t metric_bits,NativeTu15PhysicsFrameFp&,const NativeTu15ObjectLightingServices&,
    NativeTu15ObjectLightingStats* = nullptr);
// Whole 3DDC0(object): the object's render-cache token, or 0xFFFFFFFF.
std::uint32_t native_tu15_object_lighting_acquire_3ddc0(SourcePhysicsContactMemory&,std::uint32_t object,
    NativeTu15PhysicsFrameFp&,NativeTu15ObjectLightingStats* = nullptr);
// Whole 3DFD0(token, object; XMM0 metric bits).
void native_tu15_object_lighting_update_3dfd0(SourcePhysicsContactMemory&,std::uint32_t token,
    std::uint32_t object,std::uint32_t metric_bits,NativeTu15PhysicsFrameFp&,
    const NativeTu15ObjectLightingServices&,NativeTu15ObjectLightingStats* = nullptr);
}
