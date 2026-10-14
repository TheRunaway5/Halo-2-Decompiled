#pragma once
#include "halo2/source_scene_effect_owner.hpp"
#include "halo2/world_effects.hpp"
#include <memory>

namespace halo2 {
struct SourceMapEffectStats {
    unsigned attachments{},prepared{},spawned{};
    std::size_t live_objects{};
    std::vector<std::string> unavailable;
};
//16B840 zeroes the new body.175FA0's repeat branch does not call178240;
//1794E0 leaves A untouched for nameNONE and BAB40(nameNONE) writes B=1.
constexpr std::array<float,2> source_map_effect_unnamed_functions(){return {0,1};}
//96 authored effe attachments across the supplied Xbox scenery/machines/props.
//Resource preparation precedes the renderer. Current WORLD nodes, permutation
//bytes, full generation and birth clock come from the actual scene producers.
class SourceMapEffects {
public:
    SourceMapEffects(const Cache &,std::span<const ScenarioObject>,NativeWorldEffects &);
    ~SourceMapEffects();
    void synchronize(std::span<const SourceSceneEffectOwner>,std::uint64_t tick);
    SourceMapEffectStats stats() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
