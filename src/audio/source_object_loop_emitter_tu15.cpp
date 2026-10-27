#include "halo2/source_object_loop_emitter_tu15.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/tags/binary.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <set>
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
__m128 scalar(std::uint32_t bits){return _mm_set_ss(std::bit_cast<float>(bits));}
struct Floating {
    unsigned before=_mm_getcsr();
    Floating(){_mm_setcsr(0x1f80);}
    ~Floating(){_mm_setcsr(before);}
};
#endif
}
void validate_source_object_loop_emitters_tu15(std::span<const SourceObjectLoopEmitterTu15> bank) {
    if(bank.size()>source_object_loop_emitter_capacity_tu15)
        throw FormatError("Object-loop emitter bank exceeds the original source pool");
    std::set<std::uint16_t> slots;
    for(const auto& emitter:bank) {
        const Bytes location(emitter.location);
        if(emitter.loop==none || (emitter.loop&65535u)>=source_object_loop_emitter_capacity_tu15 ||
            emitter.object==none || (emitter.object&65535u)>=2048 || emitter.definition==none ||
            !slots.insert(std::uint16_t(emitter.loop)).second || (emitter.source_flags&1u) ||
            (std::to_integer<unsigned>(emitter.location[3])&15u)!=1 || emitter.cluster< -1 ||
            std::bit_cast<std::int16_t>(location.u16(0x2c))!=emitter.cluster ||
            !std::isfinite(emitter.maximum_distance) || !std::isfinite(emitter.stop_duration) ||
            emitter.stop_duration<0 || location.f32(4)<0)
            throw FormatError("Invalid full-generation attached object-loop emitter");
        for(const auto offset:{4u,8u,12u,16u,20u,28u,32u,36u})
            if(!std::isfinite(location.f32(offset)))
                throw FormatError("Object-loop emitter has a nonfinite native spatial field");
        const auto flags=std::to_integer<unsigned>(emitter.location[0]);
        for(const auto [flag,offset]:{std::pair{4u,0x34u},{8u,0x38u},{0x40u,0x40u}})
            if((flags&flag) && !std::isfinite(location.f32(offset)))
                throw FormatError("Object-loop emitter has a nonfinite native override");
    }
}
float native_object_loop_cluster_scale_tu15(const MultiplayerAudioBsp& bsp,int first,int second) {
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    if(first<0 || second<0 || std::size_t(first)>=bsp.clusters.size() || std::size_t(second)>=bsp.clusters.size())
        throw FormatError("Native object-loop cluster is outside its actual BSP");
    if(first==second)return 0;
    if(first>second)std::swap(first,second);
    const auto low=std::size_t(first),high=std::size_t(second),count=bsp.clusters.size();
    const auto index=(count-1)*low-(low+1)*low/2+high-1;
    const auto cell=bsp.pair_distances.at(index);
    Floating fp;
    // Complete253070 scalar SSE ordering and original constant bit patterns.
    if(!bsp.encoded_distances)
        return _mm_cvtss_f32(_mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),cell&127),scalar(0x40010204u)));
    if(!cell)return 0;
    if(cell==255)return 256;
    auto value=_mm_cvtsi32_ss(_mm_setzero_ps(),cell-1);
    value=_mm_mul_ss(value,scalar(0x3b81848eu));
    value=_mm_mul_ss(value,_mm_sub_ss(_mm_set_ss(bsp.distance_upper),_mm_set_ss(bsp.distance_lower)));
    return _mm_cvtss_f32(_mm_add_ss(value,_mm_set_ss(bsp.distance_lower)));
#else
    throw FormatError("Native object-loop cluster selection requires its qualified SSE owner");
#endif
}
bool native_object_loop_within_distance_tu15(Vec3 point,float distance,const SourceObjectAudioViewsTu15& views) {
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    Floating fp;
    const auto limit=_mm_mul_ss(_mm_set_ss(distance),_mm_set_ss(distance));
    // Complete190460: MULSS/ADDSS, with JNB after COMISS. Unordered
    // comparisons have CF set and therefore cannot admit a camera.
    for(const auto& view:views)if(view) {
        const auto x=_mm_sub_ss(_mm_set_ss(view->position.x),_mm_set_ss(point.x));
        const auto y=_mm_sub_ss(_mm_set_ss(view->position.y),_mm_set_ss(point.y));
        const auto z=_mm_sub_ss(_mm_set_ss(view->position.z),_mm_set_ss(point.z));
        auto squared=_mm_mul_ss(x,x);
        squared=_mm_add_ss(squared,_mm_mul_ss(y,y));
        squared=_mm_add_ss(squared,_mm_mul_ss(z,z));
        if(_mm_comige_ss(limit,squared))return true;
    }
    return false;
#else
    throw FormatError("Native object-loop camera distance requires its qualified SSE owner");
#endif
}
std::vector<SourceLoopSoundRequestTu15> native_object_loop_requests_tu15(
    std::span<const SourceObjectLoopEmitterTu15> bank,const MultiplayerAudioBsp& bsp,
    const SourceObjectAudioViewsTu15& views) {
    validate_source_object_loop_emitters_tu15(bank);
    for(const auto& view:views)if(view) {
        if(view->cluster< -1 || (view->cluster>=0 && std::size_t(view->cluster)>=bsp.clusters.size()) ||
            !std::isfinite(view->position.x) || !std::isfinite(view->position.y) || !std::isfinite(view->position.z))
            throw FormatError("Object-loop selection lost its submitted local camera");
    }
    std::vector<SourceLoopSoundRequestTu15> requests;
    for(const auto& emitter:bank) {
        if(emitter.cluster<0)continue;
        if(std::size_t(emitter.cluster)>=bsp.clusters.size())
            throw FormatError("Published object-loop cluster is outside its matching BSP");
        //18F900 unions the authentic listener-to-source cluster scales.
        const bool audible=std::ranges::any_of(views,[&](const auto& view){
            return view && view->cluster>=0 &&
                native_object_loop_cluster_scale_tu15(bsp,emitter.cluster,view->cluster)<256.f;
        });
        if(!audible)continue;
        const Bytes location(emitter.location);
        const Vec3 point{location.f32(12),location.f32(16),location.f32(20)};
        if(!(std::to_integer<unsigned>(emitter.location[0])&2u) &&
            !native_object_loop_within_distance_tu15(point,emitter.maximum_distance,views))continue;
        const auto flags=((emitter.source_flags>>4)&1u)|((emitter.source_flags>>5)&4u);
        SourceLoopSoundRequestTu15 request{
            {emitter.definition,3u,emitter.loop,none,none,emitter.function_active?0u:2u,
                flags,std::bit_cast<std::uint32_t>(emitter.function_active?0.f:emitter.stop_duration)},
            emitter.location,emitter.object};
        requests.push_back(std::move(request));
    }
    return requests;
}
}
