#pragma once
#include "halo2/source_scene_effect_owner.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/source_world_pose.hpp"
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
    struct Impl;
public:
    SourceMapEffects(const Cache &,std::span<const ScenarioObject>,NativeWorldEffects &);
    ~SourceMapEffects();
    void synchronize(std::span<const SourceSceneEffectOwner>,std::uint64_t tick,
        std::span<const SourceSceneRoot> source_roots={});
    SourceMapEffectStats stats() const;
    class Checkpoint {
    public:
        std::optional<std::uint64_t> tick()const noexcept;
    private:
        friend class SourceMapEffects;
        struct Data;std::shared_ptr<const Data> data_;
    };
    class PreparedRestore {
    public:
        ~PreparedRestore();
        PreparedRestore(PreparedRestore &&)noexcept;
        PreparedRestore &operator=(PreparedRestore &&)noexcept;
    private:
        friend class SourceMapEffects;
        PreparedRestore();std::unique_ptr<Impl> replacement_;
        std::shared_ptr<const int> owner_;
    };
    // Local completed synchronization only. Restore with the matching world
    // effect checkpoint; Cache and the stable NativeWorldEffects owner survive.
    Checkpoint checkpoint()const;
    PreparedRestore prepare_restore(const Checkpoint &)const;
    bool restore_ready(const PreparedRestore &)const noexcept;
    bool commit_restore(PreparedRestore &&)noexcept;
private:
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<const int> owner_=std::make_shared<const int>(0);
};
} // namespace halo2
