#pragma once
#include "halo2/source_decorator_decals_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <memory>

namespace halo2 {
struct SourceDecoratorDecalCommandTu15 {
    unsigned ordinal{};
    std::array<std::uint32_t,6> words{};
    std::array<std::byte,24> group{};
    ShaderVector fade{}; // Actual4ECCD c53: slope,intercept,0,1.
};
struct SourceDecoratorDecalFrameTu15 {
    std::uint32_t bsp{};
    unsigned definition{}; // Original ordinary41630 owns definition0 only.
    std::array<std::uint32_t,6> command_header{};
    std::vector<std::array<std::byte,16>> cells;
    std::vector<SourceDecoratorDecalCommandTu15> commands;
    std::array<ShaderVector,192> camera_constants{};
    ShaderVector viewport{},clip_range{},depth_parameters{};
};
// Owning readback AFTER real3F820/3F960/41630. It never fills admitted cells,
// invokes a selector or creates commands. Camera/viewport/depth inputs must be
// those completed by this actual view owner; no values are defaulted here.
SourceDecoratorDecalFrameTu15 source_decorator_decal_frame_tu15(
    const SourceRuntimeMemory&,const SourceDecoratorDecalBspTu15&,
    const std::array<ShaderVector,192>& camera_constants,
    ShaderVector viewport,ShaderVector clip_range,ShaderVector depth_parameters);

struct SourceDecoratorDecalDrawTu15 {
    SourceDecoratorDecalCommandTu15 source;
    std::uint32_t bsp{};
    unsigned definition{},group{},cell{},first{},count{};
    // Aliasing ownership retains the actual immutable Cache-adapted bank.
    // The authored index resource remains reference data; floating4EBC0 uses
    // nonindexed QUADS over this canonical20B vertex range.
    std::shared_ptr<const SourceDecoratorDecalCacheTu15> cache;
    std::shared_ptr<const Material> material;
    std::array<ShaderVector,192> constants{};
    ShaderVector viewport{},clip_range{},depth_parameters{};
};
class SourceDecoratorDecalSceneTu15 {
public:
    SourceDecoratorDecalSceneTu15(const Cache&,std::size_t bsp);
    ~SourceDecoratorDecalSceneTu15();
    const SourceDecoratorDecalBspTu15& assets()const;
    std::vector<SourceDecoratorDecalDrawTu15> draw(const SourceDecoratorDecalFrameTu15&);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
