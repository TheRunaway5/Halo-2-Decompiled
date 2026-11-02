#pragma once
#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <memory>
#include <nlohmann/json.hpp>

// Actual E3 native observer -> source BSP selection -> Vulkan uploads/draws.
// Actor/script/whole-level render acceptance remains a separate gate.
class E3CameraGpuConsumer {
public:
    E3CameraGpuConsumer(const halo2::Cache&,halo2::PhysicsLiveWorldTu15&);
    ~E3CameraGpuConsumer();
    void sample(unsigned authored_root,unsigned frame);
    nlohmann::json finish();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
