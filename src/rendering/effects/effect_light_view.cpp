#include "halo2/effect_light_view.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
using Record=SourceEffectLightDatumPool::Record;
float scalar(const Record& r,unsigned at) {return std::bit_cast<float>(r.at(at/4));}
Vec3 vector(const Record& r,unsigned at) {return {scalar(r,at),scalar(r,at+4),scalar(r,at+8)};}
void scalar(Record& r,unsigned at,float value) {r.at(at/4)=std::bit_cast<std::uint32_t>(value);}
EffectLightViewFades fades(const Record& r) {
    return {scalar(r,0xf8),scalar(r,0xfc),scalar(r,0x104),scalar(r,0x100),scalar(r,0x108)};
}
SourceEffectLightRuntime runtime(const Record& r,std::uint32_t id) {
    return {id,r[0x4c/4],std::uint8_t(r[0]>>16),std::int16_t(r[0x54/4]),
        std::int16_t(r[0x48/4]),std::bit_cast<std::int32_t>(r[2]),scalar(r,0x24),scalar(r,0x100),
        vector(r,0x18),vector(r,0x84),vector(r,0xac),vector(r,0xb8),
        {scalar(r,0xc4),scalar(r,0xd0),vector(r,0xd4),vector(r,0xe0),scalar(r,0xec)},scalar(r,0xc8)};
}
}
void source_effect_lights_update_view(SourceEffectLightDatumPool& pool,
    const SourceEffectLightDefinitions& definition,Vec3 camera_position) {
    for(const auto id:pool.identities()) {
        auto& r=pool.record(id);if(!(r[0]&0x20000u))continue;
        const auto f=source_effect_light_view_fades(definition(r[1]),vector(r,0x28),scalar(r,0x34),camera_position);
        scalar(r,0xf8,f.diffuse);scalar(r,0xfc,f.specular);scalar(r,0x104,f.secondary);
        scalar(r,0x100,f.overall);scalar(r,0x108,f.volume);
    }
}
SourceEffectLightView source_effect_lights_query(SourceEffectLightDatumPool& pool,
    const SourceEffectLightSpatialPool& spatial,const SourceEffectLightDefinitions& definition,
    SourceEffectLightQueryState& state,SourceEffectLightQueryKind kind,
    SourceEffectLightCollectionContext context) {
    SourceEffectLightView out;out.epoch=++state.epoch;context.visibility.epoch=out.epoch;
    std::vector<SourceEffectLightVisibilitySource> sources;
    for(const auto id:pool.identities()) {
        const auto& r=pool.record(id);
        sources.push_back({runtime(r,id),r[1]==0xffffffffu?nullptr:&definition(r[1]),r[3]});
    }
    std::vector<std::vector<std::uint32_t>> lists(spatial.heads.size());
    for(std::size_t c=0;c<lists.size();++c)lists[c]=spatial.cluster(std::int16_t(c));
    const std::span<SourceBspAdmissionRecord> storage=kind==SourceEffectLightQueryKind::Main?
        std::span<SourceBspAdmissionRecord>{state.main_queue}:std::span<SourceBspAdmissionRecord>{state.light_queue};
    SourceEffectLightVisibleQueue queue{std::uint32_t(storage.size()),0,storage};
    source_effect_light_collect_visible(sources,lists,queue,context);
    for(const auto& s:sources) {
        auto& r=pool.record(s.record.identity);r[3]=s.last_seen_epoch;
        out.records.push_back({s.record,s.definition,fades(r),r[1]});
    }
    for(unsigned i=0;i<queue.count;++i)out.visible.push_back(storage[i].id);
    return out;
}
void source_effect_lights_select(SourceEffectLightView& view,
    std::array<SourceShadowSelectionCache,4>& caches,std::int32_t local_player,
    const SourceEffectLightCandidateContext& context,std::int32_t frame,bool cinematic) {
    std::vector<SourceEffectLightCandidateSource> sources;
    for(const auto& r:view.records)sources.push_back({r.runtime,r.definition});
    view.candidates=source_effect_light_candidates(sources,view.visible,context);
    auto& cache=caches[std::clamp(local_player,0,3)];
    view.selected=source_shadow_select(cache,view.candidates,frame,3,
        std::bit_cast<float>(0x3c23d70au),cinematic);
}
}
