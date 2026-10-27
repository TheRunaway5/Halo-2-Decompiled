#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <memory>
namespace halo2 {
class PhysicsLiveWorldTu15;
// Owned exact CPU-prefix output at genuine2B9F0/2BBFB→132B0. A retained
// continuation is NOT a completed original2B770 scene, a GPU return or image.
struct SourceTextureCameraScenePrefixTu15 {
    std::array<std::byte,0x298> packet{};
    std::array<std::byte,0x118> completed_view{};
    std::array<std::byte,0x120> environment{};
    std::array<std::byte,0x74> raw_camera{};
    std::array<std::byte,0xc0> projection{};
    SourcePhysicsWorldRuntimeAbi original_abi{},continuation{};
    std::int32_t frame_depth_before{},frame_depth_at_boundary{},actual_local_user{};
    bool texture_camera{},backend_requested{},finished{};
    // Valid only in the same retained source bank and active native prefix
    // scope. Copies share revocation; another view serial/depth rejects too.
    void require_current(const SourcePhysicsContactMemory &)const;
private:
    SourceTextureCameraScenePrefixTu15()=default;
    const SourcePhysicsContactMemory *memory_{};
    std::uint32_t serial_{};
    std::shared_ptr<std::atomic<bool>> active_=std::make_shared<std::atomic<bool>>(false);
    friend class PhysicsLiveWorldTu15;
    friend SourceTextureCameraScenePrefixTu15 source_capture_texture_camera_scene_prefix_tu15(
        SourcePhysicsContactMemory &,SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver &);
    friend void source_publish_texture_camera_scene_prefix_tu15(SourcePhysicsContactMemory &,
        const SourceTextureCameraScenePrefixTu15 &,const SourcePhysicsContactRecordObserver &);
    friend void source_finish_texture_camera_scene_prefix_tu15(SourcePhysicsContactMemory &,
        SourceTextureCameraScenePrefixTu15 &,SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver &);
};
// Internal native CPU composition. Callers supply the real source stack/FP
// and actual native engine services.132B0 is a declared deferred boundary.
SourceTextureCameraScenePrefixTu15 source_capture_texture_camera_scene_prefix_tu15(
    SourcePhysicsContactMemory &,SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
void source_publish_texture_camera_scene_prefix_tu15(SourcePhysicsContactMemory &,
    const SourceTextureCameraScenePrefixTu15 &,const SourcePhysicsContactRecordObserver & = {});
// Whole source13C00 in the one-view domain. Its previous-frame GPU restore is
// separately deferred while its exact CPU packet/stack/projection work runs.
void source_finish_texture_camera_scene_prefix_tu15(SourcePhysicsContactMemory &,
    SourceTextureCameraScenePrefixTu15 &,SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
