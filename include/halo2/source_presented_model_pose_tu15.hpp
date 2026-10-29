#pragma once
#include "halo2/transform.hpp"
#include "halo2/source_shadow.hpp"
#include <memory>
#include <map>
#include <set>
#include <span>

namespace halo2 {
class PhysicsLiveWorldTu15;
class SourcePhysicsContactMemory;
inline constexpr std::size_t source_presented_model_owner_capacity_tu15=2048;
inline constexpr std::size_t source_presented_model_node_capacity_tu15=256;
bool source_has_presented_model_pose_tu15(const SourcePhysicsContactMemory&)noexcept;
// A scope admitted after a palette was already built must precede the real
// next native model-frame entry. Never make an old palette appear fresh by
// changing its authentic cache token or stamp.
void source_validate_presented_model_frame_tu15(const SourcePhysicsContactMemory&,
    std::uint32_t object,std::uint32_t render_frame);
// Existing completed WORLD interpolation/prediction supplied by the actual
// submitting renderer. These inputs change presentation only, never animation
// selection, native object allocation, section choice or simulation authority.
struct SourcePresentedModelPoseTu15 {
    std::uint32_t object=~0u,definition=~0u,render_model=~0u;
    Transform root;
    ShadowCasterSphere sphere;
    std::vector<Transform> nodes;
};
void validate_source_presented_model_poses_tu15(std::span<const SourcePresentedModelPoseTu15>);
// Private low-level input lease. Native kernel libraries own the registry
// and temporary node-bank use without depending on the gameplay World layer.
// Only the World adoption scope below can publish a binding.
class SourcePresentedModelInputBindingTu15 {
public:
    ~SourcePresentedModelInputBindingTu15();
private:
    friend class SourcePresentedModelPoseScopeTu15;
    SourcePresentedModelInputBindingTu15(SourcePhysicsContactMemory&,
        const std::map<std::uint32_t,SourcePresentedModelPoseTu15>&,
        std::uint32_t frame,std::set<std::uint32_t> already_built);
    struct Impl;std::unique_ptr<Impl> impl_;
};
// Holds real source root/spatial publications through a synchronous render
// callback and restores their exact original stores afterward. Native model
// and shadow histories produced by that callback remain owned by the world.
class SourcePresentedModelPoseScopeTu15 {
public:
    SourcePresentedModelPoseScopeTu15(PhysicsLiveWorldTu15 &,
        std::span<const SourcePresentedModelPoseTu15>);
    ~SourcePresentedModelPoseScopeTu15()noexcept(false);
    SourcePresentedModelPoseScopeTu15(const SourcePresentedModelPoseScopeTu15 &)=delete;
    SourcePresentedModelPoseScopeTu15 &operator=(const SourcePresentedModelPoseScopeTu15 &)=delete;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
// Native137800 input lifetime. BF640 always executes first. The sampled WORLD
// bytes occupy its actual returned source bank only while original palette,
// compound and region-map instructions consume them; no return is replaced.
class SourcePresentedModelNodeUseTu15 {
public:
    explicit SourcePresentedModelNodeUseTu15(SourcePhysicsContactMemory &);
    ~SourcePresentedModelNodeUseTu15();
    void publish(std::uint32_t object,std::uint32_t model,std::uint32_t nodes,
        unsigned count,bool first_person=false);
    SourcePresentedModelNodeUseTu15(const SourcePresentedModelNodeUseTu15 &)=delete;
private:
    SourcePhysicsContactMemory &memory_;
    std::uint32_t address_{};std::vector<std::byte> before_;
};
}
