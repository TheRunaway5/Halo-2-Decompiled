#pragma once
#include "halo2/source_texture_camera_tu15.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_frustum_camera.hpp"
#include <functional>
#include <atomic>
#include <memory>
#include <optional>
#include <span>

namespace halo2 {
class Renderer;
class PhysicsLiveWorldTu15;
struct SourceTextureCameraScenePrefixTu15;
// Literal1DE30 ->1DC20 target18 allocation recipe. Logical source image
// coordinates and the native scene attachment extent are independent inputs.
constexpr std::array<std::int32_t,2> source_texture_camera_target18_extent_tu15() {return {640,480};}
// Copied only while the same World's genuine CPU-prefix is current. The
// collection camera and the submitting packet camera are distinct original
// banks; environment clamping is permitted to change only the former far.
// Resolve depth is a third owner: configured4922F4/F8, not either camera far.
struct SourceTextureCameraCompletedPrefixTu15 {
    std::array<std::byte,0x298> packet;
    std::array<std::byte,0x74> collection_camera;
    std::array<std::byte,0xc0> collection_projection;
    std::array<std::int16_t,4> copy_rectangle;
    std::array<std::uint32_t,2> copy_depth_bits;
    std::uint32_t backend_target{},render_scale_bits{};
    std::uint8_t texture_camera_view{},copy_complete{};
};
// Only the actual Renderer issues this capability after its native image
// allocation/retirement owner has completed. No native handle is narrowed or
// written into an original32-bit target record. A stale generation rejects.
class SourceTextureCameraNativeTargetsTu15 {
public:
    std::optional<unsigned> selected() const;
    bool available(unsigned target) const;
    void require_current() const;
    void require_inputs(const SourceTextureCameraInputsTu15 &) const;
private:
    friend class Renderer;
    SourceTextureCameraNativeTargetsTu15(const SourceTextureCameraInputsTu15 &input,bool target18,bool target20,
        std::weak_ptr<void> owner,std::array<std::uint64_t,2> views,std::array<const void *,2> copies,
        std::uint64_t serial,std::function<void()> current):input_(input),target18_(target18),target20_(target20),
        current_(std::move(current)),owner_(std::move(owner)),views_(views),copies_(copies),serial_(serial){}
    SourceTextureCameraInputsTu15 input_;
    bool target18_{},target20_{};
    std::function<void()> current_;
    std::weak_ptr<void> owner_;
    std::array<std::uint64_t,2> views_;
    std::array<const void *,2> copies_;
    std::uint64_t serial_{};
};
// Deferred original25800 backend work. The original compiled source owns the
// successful256D0 object/marker resolution, complete view clone/FOV mutation,
// branch order and cleanup. This receipt alone never claims scene submission,
// completed GPU copy or a rendered framebuffer.
class SourceTextureCameraFrameTu15 {
public:
    const SourceTextureCameraInputsTu15 &before()const {return before_;}
    const SourceTextureCameraInputsTu15 &after()const {return after_;}
    const std::optional<std::array<std::byte,0x118>> &scene_view()const {return view_;}
    const std::optional<std::array<std::byte,0x118>> &completed_scene_view()const {return completed_;}
    const std::optional<SourceTextureCameraCompletedPrefixTu15> &completed_scene_prefix()const {return prefix_;}
    std::optional<unsigned> copy_target()const {return copy_target_;}
    bool source_rendered_view()const {return rendered_;}
    bool suppress_ordinary_view()const {return suppress_ordinary_;}
    const SourceTextureCameraNativeTargetsTu15 &targets()const {return targets_;}
private:
    friend class Renderer;
    friend class SourceTextureCameraFrameCaptureTu15;
    friend void source_texture_camera_complete_scene_prefix_tu15(PhysicsLiveWorldTu15 &,
        SourceTextureCameraFrameTu15 &,const SourceTextureCameraScenePrefixTu15 &);
    friend SourceTextureCameraFrameTu15 source_texture_camera_frame_tu15(
        PhysicsLiveWorldTu15 &,std::span<const std::byte,0x118>,const SourceTextureCameraNativeTargetsTu15 &);
    explicit SourceTextureCameraFrameTu15(const SourceTextureCameraNativeTargetsTu15 &targets):targets_(targets){}
    SourceTextureCameraInputsTu15 before_,after_;
    SourceTextureCameraNativeTargetsTu15 targets_;
    std::optional<std::array<std::byte,0x118>> view_;
    std::optional<std::array<std::byte,0x118>> completed_;
    std::optional<SourceTextureCameraCompletedPrefixTu15> prefix_;
    const SourcePhysicsContactMemory *memory_owner_{};
    std::optional<unsigned> copy_target_;
    bool selected_{},rendered_{},suppress_ordinary_{};
    std::shared_ptr<std::atomic<unsigned>> submission_=std::make_shared<std::atomic<unsigned>>(0u);
};
// The raw29-word camera in the completed source view. Context flags remain
// caller-owned: the source camera fields replace only the corresponding
// fields in an actual ordinary submitting projection context.
struct SourceTextureCameraViewFieldsTu15 {
    Vec3 position,forward,up;
    SourceFrustumProjectionInput projection;
    float distance_scale{},near_distance{},far_distance{};
    ShaderVector oblique_near_plane{};
};
SourceTextureCameraViewFieldsTu15 source_texture_camera_view_fields_tu15(
    const SourceTextureCameraFrameTu15 &,SourceFrustumProjectionInput submitting_context);
// Exact14BA0 target0 viewport arithmetic in the original source coordinate
// space, top,left,bottom,right. Native image scaling belongs to Renderer.
// This does not supply a target/SDK return or admit direct target18 rendering.
std::array<std::int32_t,4> source_texture_camera_viewport_tu15(
    const SourceTextureCameraFrameTu15 &);
// Bind the current same-World completed2B770 CPU-prefix at its exact132B0
// backend boundary. Retain both camera banks and the configured resolve
// operands. The captured25800 request remains immutable. No scene, GPU copy
// or completed original2B770 return is supplied by this completion.
void source_texture_camera_complete_scene_prefix_tu15(PhysicsLiveWorldTu15 &,
    SourceTextureCameraFrameTu15 &,const SourceTextureCameraScenePrefixTu15 &);
// Native world service hook. It handles only the three authenticated25800
// callee sites during the synchronous frame-capture scope; every other call
// retains its ordinary required owner. It records deferred work, not success.
bool source_texture_camera_frame_call_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &);
// original_view must be the actual complete submitting0x118 source record.
// No synthetic zero-filled view or current-camera fallback is constructed.
SourceTextureCameraFrameTu15 source_texture_camera_frame_tu15(PhysicsLiveWorldTu15 &,
    std::span<const std::byte,0x118> original_view,const SourceTextureCameraNativeTargetsTu15 &);
}
