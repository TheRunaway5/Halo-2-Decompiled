#pragma once
#include "halo2/cache.hpp"
#include "halo2/transform.hpp"
#include "halo2/source_model_palette.hpp"
namespace halo2 {
struct FirstPersonIkDefinition;
struct Transform;
struct BonePose {
  Vec3 translation;
  std::array<float, 4> rotation{0, 0, 0, 1};
  float scale = 1;
};
struct ModelBone {
  std::uint32_t name{};
  int parent = -1;
  BonePose bind;
  std::array<float, 12> inverse{};
  float inverse_scale = 1;
};
struct SkinBinding {
  std::array<std::uint16_t, 4> bones{};
  std::array<float, 4> weights{1, 0, 0, 0};
};
// Source compiled skinned VP classes3..6: activeCount is selected per part,
// while storedWeights may include the section's unused packed influences.
std::array<float, 4> source_model_skin_weights(
    std::span<const std::uint8_t> storedWeights, unsigned activeCount);
struct RenderModel {
  struct VertexOrigin {
    std::uint16_t section{}, vertex{};
    std::uint16_t part=0xffffu;
    // Literal 1CAA0/1CB20 compiled class for this section and part.
    std::uint8_t vertex_class{};
  };
  std::string name;
  Mesh mesh;
  std::vector<ModelBone> bones;
  std::vector<SourceModelCompoundNode> compound_nodes;
  std::vector<SkinBinding> skin;
  // Triangle expansion preserves each original section/part/source vertex;
  // lighting, shader selection and posing keep this authored identity.
  std::vector<VertexOrigin> vertex_origins;
  std::uint32_t checksum{};
  std::uint32_t animation = 0xffffffffu;
};
// 1CAA0 chooses transparent influences for raw part types4/5; 1CB20 maps
// section modes0/1/2 directly and mode3 active counts0..4 to classes2..6.
unsigned source_model_vertex_class(unsigned section_mode,unsigned opaque_influences,
                                   unsigned transparent_influences,unsigned part_type);
struct AnimationClip {
  std::uint32_t source_graph=0xffffffffu, source_index=0xffffffffu;
  struct FrameEvent {
    std::int16_t type{}, frame{};
  };
  struct Channel {
    std::vector<unsigned> times;
    std::vector<std::array<float, 4>> keys;
  };
  std::string name;
  unsigned frame_count{}, loop_frame{};
  std::uint8_t animation_type{}; // Literal jmad+10: type2 replaces only authored channels.
  unsigned frame_event_count{};
  std::vector<FrameEvent> frame_events;
  std::vector<std::uint32_t> nodes;
  std::vector<int> parents;
  std::vector<std::uint8_t> node_flags; // jmad skeleton node+0A, source169965.
  std::vector<BonePose> frames;
  std::vector<unsigned> components; // rotation=1, translation=2, scale=4
  std::vector<std::array<Channel, 3>> channels;
  std::uint16_t authored_flags{}; // Actual jmad animation +18 (111590).
  // Literal20E440 movement records, separate from animated node channels.
  std::vector<Vec3> movement;
  std::vector<float> movement_yaw;
};
// 16A06A selector1 / 1DE7A0: first type4 event before the clip end,
// otherwise the clip length. Conversion uses the recorded nearest-even
// clock boundary; the ordinary equipped SMG uses the integral 30 Hz result.
std::int16_t animation_ready_ticks(const AnimationClip &, std::int16_t clock_rate);
BonePose sample_animation(const AnimationClip &, std::size_t node, float frame,
                          BonePose bind = {});
// Original1690FD ->1CA950, full-weight type2 ammunition channel. Apply
// only animated components to shared graph locals, before hierarchy and IK.
void first_person_ammunition_overlay(std::span<BonePose>,const AnimationClip &,float frame);
// Optional companion supplies bind channels for animation graph nodes belonging
// to the arms when posing a weapon, or vice versa. Nodes are matched by
// StringID.
std::vector<Vertex> pose_model(const RenderModel &,
                               const AnimationClip * = nullptr,
                               float seconds = 0,
                               const RenderModel *companion = nullptr);
// Model-order posed Transform bank, before inverse bind, in model coordinates.
// This is the same bank consumed by skinning, bounds and attachments.
std::vector<Transform> pose_model_nodes(const RenderModel &,const AnimationClip * = nullptr,
                                       float seconds = 0,const RenderModel *companion = nullptr);
// Current posed model node, before inverse bind, in model coordinates.
// Object bounds and attachments use the same hierarchy as rendered vertices.
Transform pose_model_node(const RenderModel &, unsigned model_node,
                          const AnimationClip * = nullptr, float seconds = 0,
                          const RenderModel *companion = nullptr);
// A retained source controller frame, without seconds conversion or wrapping.
std::vector<Vertex> pose_model_frame(const RenderModel &,const AnimationClip *,
                                    float frame,const RenderModel *companion=nullptr);
std::vector<Transform> pose_model_nodes_frame(const RenderModel &,const AnimationClip *,
                                             float frame,const RenderModel *companion=nullptr);
Transform pose_model_node_frame(const RenderModel &,unsigned model_node,
                               const AnimationClip *,float frame,
                               const RenderModel *companion=nullptr);
// Device overlays compose model-order local channels before hierarchy,
// inverse-bind and skinning. Rendering and object bounds consume one bank.
std::vector<Vertex> pose_model_locals(const RenderModel &,std::span<const BonePose>);
std::vector<Transform> pose_model_locals_nodes(const RenderModel &,std::span<const BonePose>);
Transform pose_model_locals_node(const RenderModel &,unsigned model_node,
                                 std::span<const BonePose>);
// BD090 composes the actual object root at each hierarchy root before its
// descendants. These WORLD banks feed both bounds and1376C0 inverse binds.
std::vector<Vertex> pose_model_world(const RenderModel &,const Transform &root,
                                    const AnimationClip * = nullptr,float seconds = 0,
                                    const RenderModel *companion = nullptr);
std::vector<Transform> pose_model_nodes_world(const RenderModel &,const Transform &root,
                                             const AnimationClip * = nullptr,float seconds = 0,
                                             const RenderModel *companion = nullptr);
std::vector<Vertex> pose_model_world_frame(const RenderModel &,const Transform &root,
                                          const AnimationClip *,float frame,
                                          const RenderModel *companion = nullptr);
std::vector<Transform> pose_model_nodes_world_frame(const RenderModel &,const Transform &root,
                                                   const AnimationClip *,float frame,
                                                   const RenderModel *companion = nullptr);
// Skin an already posed model-order WORLD bank after the unit's world
// callback (e.g. held-weapon IK). Uses the existing1376C0 palette arithmetic.
std::vector<Vertex> skin_model_world_nodes(const RenderModel &,
                                          std::span<const Transform>);
std::vector<Vertex> pose_model_locals_world(const RenderModel &,const Transform &root,
                                           std::span<const BonePose>);
std::vector<Transform> pose_model_locals_nodes_world(const RenderModel &,const Transform &root,
                                                    std::span<const BonePose>);
// Retail scenery initialization starts the graph's named idle loop (10A3F0).
// Models without that animation retain their bind pose.
std::vector<Vertex> pose_scenery_model(const Cache &, const RenderModel &,
                                      float seconds = 0);
struct FirstPersonPose {
  // arms contains the complete character FP model, including hands/fingers;
  // weapon is separately owned geometry and must not receive the unit shield.
  std::vector<Vertex> weapon, arms;
  // The same model-order current WORLD nodes used to skin these vertices.
  // Effects and held-object/audio markers must consume this bank instead of
  // reconstructing a muzzle from the camera or mesh bounds.
  std::vector<Transform> weapon_nodes, arms_nodes;
  Transform camera_adjustment;
};
// One shared animation skeleton, original weapon offset, and inverse camera
// control adjustment (StringID 0x0E0000E0), as in 16760C -> 165F16.
// Returned positions are camera-local (+X forward, +Y left, +Z up).
FirstPersonPose pose_first_person_models(const RenderModel &weapon,
                                         const RenderModel &arms,
                                         const AnimationClip &, float seconds,
                                         Vec3 weapon_offset);
// 165F16 ->168644: camera composition precedes marker IK and inverse binds.
// Returned vertices are in world coordinates; the existing local API above
// remains available for deterministic asset previews.
FirstPersonPose pose_first_person_models_world(const RenderModel &weapon,
                                               const RenderModel &arms,
                                               const AnimationClip &, float seconds,
                                               Vec3 weapon_offset,
                                               const Transform &camera,
                                               const FirstPersonIkDefinition &,
                                               float aiming_weight,bool secondary=false,
                                               const Transform *primary_adjustment=nullptr,
                                               const AnimationClip *ammunition=nullptr,float ammunition_frame=0);
// Registered source clock frame, without converting through elapsed seconds.
FirstPersonPose pose_first_person_models_world_frame(const RenderModel &weapon,
                                                     const RenderModel &arms,
                                                     const AnimationClip &,float frame,
                                                     Vec3 weapon_offset,const Transform &camera,
                                                     const FirstPersonIkDefinition &,float aiming_weight,
                                                     bool secondary=false,const Transform *primary_adjustment=nullptr,
                                               const AnimationClip *ammunition=nullptr,float ammunition_frame=0);
} // namespace halo2
