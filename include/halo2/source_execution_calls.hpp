#pragma once
#include "halo2/source_execution_inputs.hpp"
#include "halo2/simulation.hpp"
#include "halo2/source_execution_camera.hpp"
#include "halo2/source_execution_lens.hpp"
#include "halo2/particle_collision.hpp"
#include "halo2/source_object_loop_emitter_tu15.hpp"
#include "halo2/native_death_camera.hpp"
#include "halo2/netcode_mode.hpp"
namespace halo2 {
struct Vec3;
struct SourcePortalVisibilitySet;
class MultiplayerMatch;
struct MultiplayerState;
class MultiplayerObjectRuntime;
struct SourcePhysicsContactRuntimeAbi;
struct SourceObjectAdmission;
struct SourceDrawUnitState;
struct SourceExecutionModelInputTu15;
struct SourceBspPartCommandState;
struct SourcePresentedModelPoseTu15;
std::vector<std::byte> source_execution_arguments(std::span<const SourcePresentedModelPoseTu15>);
std::vector<std::byte> source_execution_arguments(const std::array<Controls,16> &);
// MatchStepViews: the step's controls, the producer's rewind mode and every
// slot's remote displayed view (lag-compensated hitscan).
std::vector<std::byte> source_execution_arguments(const std::array<Controls,16> &,NetcodeMode,
    const std::array<std::optional<NetcodeViewTime>,16> &);
std::vector<std::byte> source_execution_arguments(float);
std::vector<std::byte> source_execution_arguments(unsigned);
std::vector<std::byte> source_execution_arguments(std::uint32_t,std::uint32_t);
std::vector<std::byte> source_execution_arguments(unsigned,Vec3);
std::vector<std::byte> source_execution_arguments(const SourcePortalVisibilitySet &);
std::vector<std::byte> source_execution_arguments(double,std::int16_t);
std::vector<std::byte> source_execution_arguments(std::uint32_t,unsigned,unsigned,bool,float,float);
std::vector<std::byte> source_execution_arguments(unsigned,Vec3,Vec3,Vec3);
std::vector<std::byte> source_execution_arguments(std::span<const std::uint32_t>);
std::vector<std::byte> source_execution_arguments(std::uint32_t,std::span<const std::uint32_t>);
std::vector<std::byte> source_execution_arguments(std::uint32_t,std::span<const std::byte>);
std::vector<std::byte> source_execution_arguments(std::uint32_t,const SourcePhysicsContactRuntimeAbi&);
std::vector<std::byte> source_execution_result(std::uint32_t);
std::vector<std::byte> source_execution_result(const SourcePhysicsContactRuntimeAbi&);
std::vector<std::byte> source_execution_arguments(const SourceObjectAdmission&,const SourceDrawUnitState&,const SourceExecutionModelInputTu15&);
std::vector<std::byte> source_execution_model_result(const SourceObjectAdmission&,const SourceDrawUnitState&);
std::vector<std::byte> source_execution_arguments(const SourceBspPartCommandState&,const SourceObjectAdmission&,std::uint32_t);
std::vector<std::byte> source_execution_attachment_result(const SourceBspPartCommandState&);
std::vector<std::byte> source_execution_state_result(const MultiplayerState&);
std::vector<std::byte> source_execution_arguments(const SourcePlayerCameraCall&);
std::vector<std::byte> source_execution_camera_result(const DeathCameraPacket&,
    const SourceCameraControllerState&,const SourceObserverState&,std::uint32_t,std::optional<std::uint8_t>);
std::vector<std::byte> source_execution_lens_result(const SourcePhysicsContactRuntimeAbi&,const std::array<std::byte,32>&);
std::vector<std::byte> source_execution_arguments(bool,std::span<const SourceLensQueryRollback>);
void validate_source_lens_device_regions(std::span<const SourceLensDeviceRegion>);
std::vector<std::byte> source_execution_arguments(std::uint32_t,const SourcePhysicsContactRuntimeAbi&,bool,
    std::span<const SourceLensDeviceRegion>);
std::vector<std::byte> source_execution_arguments(std::uint32_t,const SourcePhysicsContactRuntimeAbi&,bool,
    const SourceLensDeviceDomains&);
std::vector<std::byte> source_execution_lens_draw_result(const SourcePhysicsContactRuntimeAbi&,
    const std::array<std::byte,32>&,const SourceLensWorldPass&);
std::vector<std::byte> source_execution_arguments(std::uint32_t,Vec3,Vec3);
std::vector<std::byte> source_execution_collision_result(const std::optional<ParticleCollisionHit>&);
std::vector<std::byte> source_execution_loop_emitters_result(std::span<const SourceObjectLoopEmitterTu15>);
struct SourceAdmittedCameraInput {
    std::uint8_t owner{},view{};
    float seconds{};
    bool cycle{};
};
std::vector<std::byte> source_execution_arguments(const SourceAdmittedCameraInput &);
std::vector<std::byte> source_execution_admitted_camera_result(const std::optional<NativeDeathCameraView> &,
    bool,const SourceCameraControllerState &,const SourceObserverState &,std::uint32_t,const DeathCameraSavedClusters &,
    std::uint32_t,bool,bool);
void validate_source_execution_calls(std::span<const SourceExecutionCall>);
void replay_source_execution_calls(MultiplayerMatch &,MultiplayerObjectRuntime &,std::span<const SourceExecutionCall>);
}
