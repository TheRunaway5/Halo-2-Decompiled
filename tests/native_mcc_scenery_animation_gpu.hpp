#pragma once
#include "halo2/source_campaign_model.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include <memory>

// Test-only consumer of the actual native collector, authored materials and
// retained camera/light state. Captures live only in the enclosing test owner.
class SceneryAnimationGpuConsumer {
public:
    SceneryAnimationGpuConsumer(const halo2::Cache&,const halo2::LevelContent&,
        const halo2::ScenarioObject&,halo2::MultiplayerObjectRuntime&,
        std::uint32_t object,unsigned motion_tick,halo2::SourceCampaignModel&);
    ~SceneryAnimationGpuConsumer();
    void sample(const halo2::SourceObjectDrawCollector&,std::span<const halo2::Transform>);
    void finish();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
