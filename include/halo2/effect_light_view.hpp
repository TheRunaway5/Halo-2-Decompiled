#pragma once
#include "halo2/effect_light_lifetime.hpp"
#include "halo2/effect_light_admission.hpp"

namespace halo2 {
using SourceEffectLightDefinitions=std::function<const EffectLightDefinition&(std::uint32_t)>;
// 131FF0/134980 allocate one shared main queue (64) and one light query
// queue (1). 163C80 reserves the final slot in both; counts reset per query,
// while inactive columns retain their previous contents.
enum class SourceEffectLightQueryKind {Main,Light};
struct SourceEffectLightQueryState {
    std::uint32_t epoch{}; //4E0308, incremented by every1320F0 query
    std::array<SourceBspAdmissionRecord,64> main_queue{};
    std::array<SourceBspAdmissionRecord,1> light_queue{};
};
struct SourceEffectLightViewRecord {
    SourceEffectLightRuntime runtime;
    const EffectLightDefinition* definition;
    EffectLightViewFades fades;
    std::uint32_t definition_identity=0xffffffffu; // Actual occupied record+4.
};
struct SourceEffectLightView {
    std::uint32_t epoch{};
    std::vector<SourceEffectLightViewRecord> records;
    std::vector<std::uint32_t> visible;
    std::vector<SourceShadowCandidate> candidates,selected;
};
// Whole C15A0: only occupied records with active bit1 receive C12D0 updates.
// It reads the persistent cull center/radius (+28/+34), not the render origin.
void source_effect_lights_update_view(SourceEffectLightDatumPool&,
    const SourceEffectLightDefinitions&,Vec3 camera_position);
// 1320F0's light epoch transition followed by the complete132220 light
// branches. The caller has already constructed the actual portal frustums
// and completed BSP admission. This also owns the real record+C stamps.
SourceEffectLightView source_effect_lights_query(SourceEffectLightDatumPool&,
    const SourceEffectLightSpatialPool&,const SourceEffectLightDefinitions&,
    SourceEffectLightQueryState&,SourceEffectLightQueryKind,
    SourceEffectLightCollectionContext);
// Whole31910: actual visible order, admission/coverage, and the separately
// retained local-view cache. Frame and cinematic state are explicit inputs.
void source_effect_lights_select(SourceEffectLightView&,
    std::array<SourceShadowSelectionCache,4>&,std::int32_t local_player,
    const SourceEffectLightCandidateContext&,std::int32_t frame,bool cinematic);
}
