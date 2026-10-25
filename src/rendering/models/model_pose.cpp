#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/model_skin_submission.hpp"
#include "halo2/model_skin_gpu_admission.hpp"
#include "halo2/model_interpolation.hpp"
#include "halo2/render_interpolation.hpp"
#include <glm/gtc/quaternion.hpp>
#include "halo2/transform.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/source_model_pose.hpp"
#include "halo2/source_model_palette.hpp"
#include "halo2/first_person_dual.hpp"
#include "halo2/frontend_model_primary.hpp"
#include <algorithm>
#include <functional>
#include <cstdlib>
#include "rendering/models/model_skin_vertices.hpp"

namespace halo2 {
std::vector<Vertex> pose_scenery_model(const Cache &cache,
                                      const RenderModel &model, float seconds) {
  if (model.animation != 0xffffffffu) {
    if (const auto idle = cache.find_animation_name(model.animation, "idle")) {
      const auto clip = cache.load_named_animation(model.animation, *idle);
      return pose_model(model, &clip, seconds);
    }
  }
  return pose_model(model, nullptr, seconds);
}
namespace {
struct SampledSkeleton {
  std::vector<Transform> nodes;
  std::vector<int> model_mapping;
};
} // namespace
BonePose sample_animation(const AnimationClip &clip, std::size_t node,
                          float frame, BonePose pose) {
  if (!clip.frame_count || node >= clip.nodes.size() ||
      clip.channels.size() != clip.nodes.size() ||
      clip.components.size() != clip.nodes.size() || !std::isfinite(frame))
    throw FormatError("invalid animation sample");
  frame = std::clamp(frame, 0.f, float(clip.frame_count - 1));
  for (unsigned channel = 0; channel < 3; ++channel) {
    if (!(clip.components[node] & (1u << channel)))
      continue;
    const auto &track = clip.channels[node][channel];
    if (track.times.empty() || track.keys.size() != track.times.size())
      throw FormatError("invalid animation sampling channel");
    const auto upper =
        std::upper_bound(track.times.begin(), track.times.end(), frame);
    const auto k = upper == track.times.begin()
                       ? 0u
                       : unsigned(upper - track.times.begin() - 1);
    const auto next = std::min(k + 1, unsigned(track.times.size() - 1));
    const float blend =
        frame <= track.times[k] || k == next
            ? 0.f
            : std::clamp((frame - track.times[k]) /
                             float(track.times[next] - track.times[k]),
                         0.f, 1.f);
    std::array<float, 4> value{};
    for (unsigned j = 0; j < 4; ++j)
      value[j] = track.keys[k][j] * (1 - blend) + track.keys[next][j] * blend;
    if (channel == 0) {
      float norm = 0;
      for (float v : value)
        norm += v * v;
      if (norm < 1e-12f || !std::isfinite(norm))
        throw FormatError("invalid sampled quaternion");
      for (auto &v : value)
        v /= std::sqrt(norm);
      pose.rotation = value;
    } else if (channel == 1)
      pose.translation = {value[0], value[1], value[2]};
    else
      pose.scale = value[0];
  }
  return pose;
}
void first_person_ammunition_overlay(std::span<BonePose> locals,const AnimationClip &clip,float frame) {
  if(clip.animation_type!=2 || locals.size()!=clip.nodes.size())
    throw FormatError("Unsupported first-person ammunition overlay contract");
  if(!std::isfinite(frame) || frame<0 || frame>=clip.frame_count || std::floor(frame)!=frame ||
      clip.channels.size()!=locals.size() || clip.components.size()!=locals.size())
    throw FormatError("Invalid first-person ammunition channel sample");
  for(std::size_t i=0;i<locals.size();++i) {
    // The actual type2 Needler overlays have translation and scale channels.
    //293200/293350 use a+(b-a)*((frame-time)*RCPSS(delta)), not weighted sums.
    // As in the existing instruction oracles, binary32 reciprocal is the
    // explicit software convention; physical Xbox RCPSS seeds are separate.
    if(clip.components[i]&1u)throw FormatError("Unrecovered ammunition rotation channel");
    for(unsigned channel=1;channel<3;++channel)if(clip.components[i]&(1u<<channel)) {
      const auto &track=clip.channels[i][channel];
      if(track.times.empty() || track.times.size()!=track.keys.size())throw FormatError("Invalid ammunition keys");
      const auto upper=std::upper_bound(track.times.begin(),track.times.end(),frame);
      const auto k=upper==track.times.begin()?0u:unsigned(upper-track.times.begin()-1);
      const auto next=std::min(k+1,unsigned(track.times.size()-1));
      const float fraction=next==k || frame==track.times[k]?0.f:
          (frame-float(track.times[k]))*(1.f/float(track.times[next]-track.times[k]));
      const auto value=[&](unsigned j){const auto a=track.keys[k][j];
        return fraction==0?a:a+(track.keys[next][j]-a)*fraction;};
      if(channel==1)locals[i].translation={value(0),value(1),value(2)};
      else locals[i].scale=value(0);
    }
  }
}
namespace {
SampledSkeleton sample_skeleton(const RenderModel &model,
                                const AnimationClip *clip, float seconds,
                                const RenderModel *companion,
                                bool companion_first = false,float authored_frame = -1.f,
                                const Transform *root = nullptr,bool secondary=false,
                                const AnimationFrameWindow *window=nullptr,
                                const AnimationClip *ammunition=nullptr,float ammunition_frame=0) {
  if (model.skin.size() != model.mesh.vertices.size() ||
      !std::isfinite(seconds) || seconds < 0)
    throw FormatError("invalid model pose request");
  const auto count = clip ? clip->nodes.size() : model.bones.size();
  std::vector<BonePose> local(count);
  std::vector<int> parents(count);
  std::vector<int> mapping(model.bones.size(), -1);
  if (clip) {
    if (!clip->frame_count || clip->loop_frame >= clip->frame_count ||
        clip->frames.size() != clip->frame_count * count ||
        clip->parents.size() != count || clip->components.size() != count)
      throw FormatError("invalid animation pose data");
    const float at = authored_frame>=0 ? authored_frame : seconds*30;
    const auto loop = clip->frame_count - clip->loop_frame;
    const float wrapped =
        at < float(clip->frame_count)
            ? at
            : float(clip->loop_frame) +
                  std::fmod(at - clip->loop_frame, float(loop));
    for (std::size_t i = 0; i < count; ++i) {
      const auto find_bind =
          [&](const RenderModel *source) -> const ModelBone * {
        //169965 maps only graph nodes carrying the left-arm flag for hand1;
        //1690FD therefore leaves all other companion bind locals at identity.
        if(secondary && source==companion) {
          if(clip->node_flags.size()!=count)throw FormatError("Missing original offhand graph node flags");
          if(!(clip->node_flags[i]&0x20u))return nullptr;
        }
        if (source)
          for (const auto &bone : source->bones)
            if (bone.name == clip->nodes[i])
              return &bone;
        return nullptr;
      };
      const ModelBone *bind = find_bind(companion_first ? companion : &model);
      if (!bind)
        bind = find_bind(companion_first ? &model : companion);
      if (bind)
        local[i] = bind->bind;
      const auto bind_pose=local[i];
      if (clip->channels.size() != count)
        throw FormatError("animation lacks sampling channels");
      if(!window)for (unsigned channel = 0; channel < 3; ++channel)
        if (clip->components[i] & (1u << channel)) {
          const auto &track = clip->channels[i][channel];
          if (track.times.empty() || track.keys.size() != track.times.size())
            throw FormatError("invalid animation sampling channel");
          const auto upper =
              std::upper_bound(track.times.begin(), track.times.end(), wrapped);
          const auto k = upper == track.times.begin()
                             ? 0u
                             : unsigned(upper - track.times.begin() - 1),
                     next = std::min(k + 1, unsigned(track.times.size() - 1));
          const float blend =
              wrapped <= track.times[k] || k == next
                  ? 0.f
                  : std::clamp((wrapped - track.times[k]) /
                                   float(track.times[next] - track.times[k]),
                               0.f, 1.f);
          std::array<float, 4> value{};
          for (unsigned j = 0; j < 4; ++j)
            value[j] =
                track.keys[k][j] * (1 - blend) + track.keys[next][j] * blend;
          if (channel == 0)
            local[i].rotation = value;
          else if (channel == 1)
            local[i].translation = {value[0], value[1], value[2]};
          else
            local[i].scale = value[0];
        }
      if(window) {
        const auto before=sample_animation(*clip,i,window->previous,bind_pose);
        const auto after=sample_animation(*clip,i,window->current,bind_pose);
        const auto quaternion=[](const auto &r){return glm::normalize(glm::quat(r[3],r[0],r[1],r[2]));};
        const auto rotation=glm::slerp(quaternion(before.rotation),quaternion(after.rotation),window->fraction);
        local[i].rotation={rotation.x,rotation.y,rotation.z,rotation.w};
        local[i].translation=render_lerp(before.translation,after.translation,window->fraction);
        local[i].scale=std::lerp(before.scale,after.scale,window->fraction);
      }
      parents[i] = clip->parents[i];
    }
    for (std::size_t i = 0; i < model.bones.size(); ++i) {
      for (std::size_t j = 0; j < count; ++j)
        if (model.bones[i].name == clip->nodes[j])
          mapping[i] = int(j);
      if (mapping[i] < 0)
        throw FormatError("animation graph lacks model bone");
    }
  } else
    for (std::size_t i = 0; i < count; ++i) {
      local[i] = model.bones[i].bind;
      parents[i] = model.bones[i].parent;
      mapping[i] = int(i);
    }
  if(ammunition) {
    if(!clip || ammunition->nodes!=clip->nodes || ammunition->parents!=clip->parents)
      throw FormatError("Ammunition overlay disagrees with the shared first-person graph");
    first_person_ammunition_overlay(local,*ammunition,ammunition_frame);
  }
  std::vector<Transform> world(count);
  if(secondary) {
    if(!clip || model.bones.empty() || !root)throw FormatError("Missing original offhand hierarchy inputs");
    const auto mirror=first_person_dual_root(*clip,model.bones.front().name);
    if(mirror.node>=0)return {source_first_person_dual_nodes(local,parents,mirror,*root),std::move(mapping)};
  }
  std::vector<unsigned> state(count);
  std::function<void(std::size_t)> resolve = [&](std::size_t i) {
    if (i >= count || state[i] == 1)
      throw FormatError("cyclic animation skeleton");
    if (state[i] == 2)
      return;
    state[i] = 1;
    world[i] = source_model_local_transform(local[i]);
    if(parents[i]<-1)throw FormatError("invalid animation skeleton parent");
    if (parents[i] >= 0) {
      resolve(std::size_t(parents[i]));
      world[i] = compose_transform(world[parents[i]],world[i]);
    } else if(root) world[i] = compose_transform(*root,world[i]);
    state[i] = 2;
  };
  for (std::size_t i = 0; i < count; ++i)
    resolve(i);
  return {std::move(world), std::move(mapping)};
}
std::vector<std::array<float,12>> skin_palette(const RenderModel &model,
                               const std::vector<Transform> &world,
                               const std::vector<int> &mapping) {
  if (model.skin.size() != model.mesh.vertices.size() ||
      mapping.size() != model.bones.size())
    throw FormatError("invalid model skin mapping");
  std::vector<Transform> model_world,inverse;
  model_world.reserve(model.bones.size());inverse.reserve(model.bones.size());
  for (std::size_t i = 0; i < model.bones.size(); ++i) {
    if(mapping[i]<0 || std::size_t(mapping[i])>=world.size())
      throw FormatError("model skin node outside actual world bank");
    model_world.push_back(world[std::size_t(mapping[i])]);
    const auto &b=model.bones[i];Transform bind;bind.scale=b.inverse_scale;
    for(unsigned c=0;c<3;++c)
      bind.basis[c]={b.inverse[c*3],b.inverse[c*3+1],b.inverse[c*3+2]};
    bind.position={b.inverse[9],b.inverse[10],b.inverse[11]};
    inverse.push_back(bind);
  }
  //1376C0 composes the original Transform bank before flattening its rows.
  auto skin=source_model_skin_palette(model_world,inverse);
  append_source_model_compounds(skin,model.compound_nodes);
  return skin;
}
void finish_skin(const RenderModel &model,std::vector<Vertex> &vertices,
                 std::span<const std::array<float,12>> skin) {
  const auto *reference=std::getenv("HALO2_REFERENCE_SCALAR_SKINNING");
  skin_model_vertices(vertices,model.skin,skin,!(reference && *reference));
  if(const auto *verify=std::getenv("HALO2_VERIFY_SIMD_SKINNING");verify && *verify) {
    auto expected=model.mesh.vertices;
    skin_model_vertices_scalar(expected,model.skin,skin);
    if(!vertices.empty() && std::memcmp(vertices.data(),expected.data(),vertices.size()*sizeof(Vertex)))
      throw FormatError("SIMD skin differs from complete original scalar posed vertices");
  }
}
std::vector<Vertex> skin_model(const RenderModel &model,
                               const std::vector<Transform> &world,
                               const std::vector<int> &mapping) {
  auto skin=skin_palette(model,world,mapping);
  auto vertices=model.mesh.vertices;
  finish_skin(model,vertices,skin);
  return vertices;
}
std::vector<Transform> model_order_nodes(const SampledSkeleton &pose) {
  std::vector<Transform> nodes;nodes.reserve(pose.model_mapping.size());
  for(const auto node:pose.model_mapping) {
    if(node<0 || std::size_t(node)>=pose.nodes.size())
      throw FormatError("model node outside actual pose bank");
    nodes.push_back(pose.nodes[std::size_t(node)]);
  }
  return nodes;
}
} // namespace
std::vector<std::array<float,12>> model_skin_world_palette(const RenderModel &model,
                                                        std::span<const Transform> nodes) {
  if(nodes.size()!=model.bones.size())throw FormatError("posed model world bank count changed");
  std::vector<int> mapping(nodes.size());
  for(std::size_t i=0;i<mapping.size();++i)mapping[i]=int(i);
  return skin_palette(model,std::vector<Transform>(nodes.begin(),nodes.end()),mapping);
}
ModelSkinWorldPose model_skin_world_pose(const RenderModel &model,
    std::span<const Transform> nodes,bool allow_gpu) {
  ModelSkinWorldPose result;
  result.palette=model_skin_world_palette(model,nodes);
  result.vertices=model.mesh.vertices;
  const auto *scalar=std::getenv("HALO2_REFERENCE_SCALAR_SKINNING");
  const auto *verify=std::getenv("HALO2_VERIFY_SIMD_SKINNING");
  result.gpu=allow_gpu && !(scalar && *scalar) && !(verify && *verify) &&
      model_skin_gpu_host_effects_admitted() &&
      model_skin_gpu_arithmetic_admitted(result.vertices,model.skin,result.palette);
  if(!result.gpu)finish_skin(model,result.vertices,result.palette);
  return result;
}
bool model_skin_gpu_requested()noexcept {
  const auto *reference=std::getenv("HALO2_REFERENCE_CPU_SKINNING");
  if(reference && *reference)return false;
  const auto *enable=std::getenv("HALO2_ENABLE_GPU_SKINNING");
  return !(enable && enable[0]=='0' && !enable[1]);
}
std::vector<Vertex> skin_model_world_nodes(const RenderModel &model,
                                          std::span<const Transform> nodes) {
  if(nodes.size()!=model.bones.size())throw FormatError("posed model world bank count changed");
  std::vector<int> mapping(nodes.size());
  for(std::size_t i=0;i<mapping.size();++i)mapping[i]=int(i);
  return skin_model(model,std::vector<Transform>(nodes.begin(),nodes.end()),mapping);
}
std::vector<Vertex> pose_model(const RenderModel &model,
                               const AnimationClip *clip, float seconds,
                               const RenderModel *companion) {
  const auto pose = sample_skeleton(model, clip, seconds, companion);
  return skin_model(model, pose.nodes, pose.model_mapping);
}
std::vector<Transform> pose_model_nodes(const RenderModel &model,const AnimationClip *clip,
                                       float seconds,const RenderModel *companion) {
  return model_order_nodes(sample_skeleton(model,clip,seconds,companion));
}
std::vector<Vertex> pose_model_world(const RenderModel &model,const Transform &root,
                                    const AnimationClip *clip,float seconds,const RenderModel *companion) {
  const auto pose=sample_skeleton(model,clip,seconds,companion,false,-1.f,&root);
  return skin_model(model,pose.nodes,pose.model_mapping);
}
std::vector<Transform> pose_model_nodes_world(const RenderModel &model,const Transform &root,
                                             const AnimationClip *clip,float seconds,const RenderModel *companion) {
  return model_order_nodes(sample_skeleton(model,clip,seconds,companion,false,-1.f,&root));
}
Transform pose_model_node(const RenderModel &model, unsigned model_node,
                          const AnimationClip *clip, float seconds,
                          const RenderModel *companion) {
  if(model_node>=model.bones.size())throw FormatError("posed model node outside skeleton");
  const auto pose=sample_skeleton(model,clip,seconds,companion);
  const auto node=std::size_t(pose.model_mapping.at(model_node));
  return pose.nodes.at(node);
}
namespace {
void validate_controller_frame(const AnimationClip *clip,float frame) {
  if(!std::isfinite(frame) || frame<0 || (clip && frame>=float(clip->frame_count)))
    throw FormatError("Retained model controller frame is outside its source clip");
  if(!clip && frame!=0)
    throw FormatError("Unanimated model has no controller frame");
}
}
std::vector<Vertex> pose_model_frame(const RenderModel &model,const AnimationClip *clip,
                                    float frame,const RenderModel *companion) {
  validate_controller_frame(clip,frame);
  const auto pose=sample_skeleton(model,clip,0,companion,false,frame);
  return skin_model(model,pose.nodes,pose.model_mapping);
}
std::vector<Transform> pose_model_nodes_frame(const RenderModel &model,const AnimationClip *clip,
                                             float frame,const RenderModel *companion) {
  validate_controller_frame(clip,frame);
  return model_order_nodes(sample_skeleton(model,clip,0,companion,false,frame));
}
std::vector<Vertex> pose_model_world_frame(const RenderModel &model,const Transform &root,
                                          const AnimationClip *clip,float frame,const RenderModel *companion) {
  validate_controller_frame(clip,frame);
  const auto pose=sample_skeleton(model,clip,0,companion,false,frame,&root);
  return skin_model(model,pose.nodes,pose.model_mapping);
}
std::vector<Transform> pose_model_nodes_world_frame(const RenderModel &model,const Transform &root,
                                                   const AnimationClip *clip,float frame,const RenderModel *companion) {
  validate_controller_frame(clip,frame);
  return model_order_nodes(sample_skeleton(model,clip,0,companion,false,frame,&root));
}
std::vector<Transform> pose_world_object_nodes_frame(const RenderModel &model,const Transform &root,
                                                   const FrontendModelPrimary &clip,float frame) {
  // BF0C0 copies mode bind locals, samples by node index, then publishes each
  // matrix using mode+48/+4C parent order. No StringID remapping occurs here.
  for(std::size_t i=0;i<model.bones.size();++i)
    if(i==0?model.bones[i].parent!=-1:
        model.bones[i].parent<0 || std::size_t(model.bones[i].parent)>=i)
      throw FormatError("World object model hierarchy differs from its source node order");
  const auto locals=sample_world_object_primary(model,clip,frame,{},frame!=std::floor(frame));
  return pose_model_locals_nodes_world(model,root,locals);
}
std::vector<Vertex> pose_model_world_between_frames(const RenderModel &model,const Transform &root,
    const AnimationClip &clip,AnimationFrameWindow window) {
  validate_render_fraction(window.fraction);
  validate_controller_frame(&clip,window.previous);validate_controller_frame(&clip,window.current);
  if(window.fraction==0)return pose_model_world_frame(model,root,&clip,window.previous);
  if(window.fraction==1 || window.previous==window.current)return pose_model_world_frame(model,root,&clip,window.current);
  const auto pose=sample_skeleton(model,&clip,0,nullptr,false,window.current,&root,false,&window);
  return skin_model(model,pose.nodes,pose.model_mapping);
}
std::vector<Transform> pose_model_nodes_world_between_frames(const RenderModel &model,const Transform &root,
    const AnimationClip &clip,AnimationFrameWindow window) {
  validate_render_fraction(window.fraction);
  validate_controller_frame(&clip,window.previous);validate_controller_frame(&clip,window.current);
  if(window.fraction==0)return pose_model_nodes_world_frame(model,root,&clip,window.previous);
  if(window.fraction==1 || window.previous==window.current)return pose_model_nodes_world_frame(model,root,&clip,window.current);
  return model_order_nodes(sample_skeleton(model,&clip,0,nullptr,false,window.current,&root,false,&window));
}
Transform pose_model_node_frame(const RenderModel &model,unsigned model_node,
                               const AnimationClip *clip,float frame,const RenderModel *companion) {
  validate_controller_frame(clip,frame);
  if(model_node>=model.bones.size())throw FormatError("posed model node outside skeleton");
  const auto pose=sample_skeleton(model,clip,0,companion,false,frame);
  const auto node=std::size_t(pose.model_mapping.at(model_node));
  return pose.nodes.at(node);
}
namespace {
SampledSkeleton model_local_skeleton(const RenderModel&model,std::span<const BonePose>locals,
                                     const Transform *root=nullptr){
  if(locals.size()!=model.bones.size())throw FormatError("model local pose count mismatch");
  SampledSkeleton result;const auto count=locals.size();
  result.nodes.resize(count);result.model_mapping.resize(count);
  std::vector<unsigned>state(count);
  std::function<void(unsigned)>resolve=[&](unsigned i){
    if(i>=count || state[i]==1)throw FormatError("cyclic model local skeleton");
    if(state[i]==2)return;
    state[i]=1;result.nodes[i]=source_model_local_transform(locals[i]);
    const auto parent=model.bones[i].parent;
    if(parent<-1)throw FormatError("invalid model local skeleton parent");
    if(parent>=0){resolve(unsigned(parent));result.nodes[i]=compose_transform(result.nodes[unsigned(parent)],result.nodes[i]);}
    else if(root)result.nodes[i]=compose_transform(*root,result.nodes[i]);
    result.model_mapping[i]=int(i);state[i]=2;
  };
  for(unsigned i=0;i<count;++i)resolve(i);
  return result;
}
}
std::vector<Vertex> pose_model_locals(const RenderModel&model,std::span<const BonePose>locals){
  const auto pose=model_local_skeleton(model,locals);return skin_model(model,pose.nodes,pose.model_mapping);
}
std::vector<Transform> pose_model_locals_nodes(const RenderModel &model,std::span<const BonePose> locals) {
  return model_order_nodes(model_local_skeleton(model,locals));
}
std::vector<Vertex> pose_model_locals_world(const RenderModel &model,const Transform &root,
                                           std::span<const BonePose> locals) {
  const auto pose=model_local_skeleton(model,locals,&root);
  return skin_model(model,pose.nodes,pose.model_mapping);
}
std::vector<Transform> pose_model_locals_nodes_world(const RenderModel &model,const Transform &root,
                                                    std::span<const BonePose> locals) {
  return model_order_nodes(model_local_skeleton(model,locals,&root));
}
Transform pose_model_locals_node(const RenderModel&model,unsigned node,std::span<const BonePose>locals){
  if(node>=model.bones.size())throw FormatError("model local pose node outside skeleton");
  const auto pose=model_local_skeleton(model,locals);return pose.nodes[node];
}
namespace {
FirstPersonPose first_person_pose(const RenderModel &weapon,
                                         const RenderModel &arms,
                                         const AnimationClip &clip,
                                         float seconds, Vec3 weapon_offset,
                                         const Transform *camera,
                                         const FirstPersonIkDefinition *ik,
                                         float aiming_weight,float authored_frame = -1.f,
                                         bool secondary=false,const Transform *primary_adjustment=nullptr,
                                         const AnimationClip *ammunition=nullptr,float ammunition_frame=0) {
  if (!std::isfinite(weapon_offset.x) || !std::isfinite(weapon_offset.y) ||
      !std::isfinite(weapon_offset.z))
    throw FormatError("invalid first person weapon offset");
  // Retail 16760C writes weapon bind channels first, then arms, before
  // sampling one shared graph. Both meshes must use the same resulting nodes.
  Transform offset;
  offset.position = weapon_offset;
  //167D9D ->1CBD50 ->1DD1C0 applies the authored offset before children.
  auto pose = sample_skeleton(weapon, &clip, seconds, &arms, true,authored_frame,&offset,secondary,nullptr,ammunition,ammunition_frame);
  std::vector<int> arms_mapping;
  for (const auto &bone : arms.bones) {
    const auto node =
        std::find(clip.nodes.begin(), clip.nodes.end(), bone.name);
    if (node == clip.nodes.end())
      throw FormatError("first person graph lacks arms bone");
    arms_mapping.push_back(int(node - clip.nodes.begin()));
  }
  auto nodes=pose.nodes;
  Transform adjustment;
  const auto camera_control =
      std::find(clip.nodes.begin(), clip.nodes.end(), 0x0e0000e0u);
  if(primary_adjustment)adjustment=*primary_adjustment;
  else if (camera_control != clip.nodes.end()) {
    adjustment = nodes[std::size_t(camera_control - clip.nodes.begin())];
    // Original 16760C subtracts the authored offset before 165F16 inverts.
    adjustment.position = adjustment.position - weapon_offset;
    adjustment = inverse_transform(adjustment);
  }
  const auto root = camera ? compose_transform(*camera, adjustment) : adjustment;
  for (auto &node : nodes) node = compose_transform(root, node);
  if (ik) {
    std::vector<Transform> arms_nodes, weapon_nodes;
    std::vector<int> parents;
    for (const auto node : arms_mapping) arms_nodes.push_back(nodes[std::size_t(node)]);
    for (const auto node : pose.model_mapping) weapon_nodes.push_back(nodes[std::size_t(node)]);
    for (const auto &bone : arms.bones) parents.push_back(bone.parent);
    apply_first_person_ik(*ik, parents, arms_nodes, weapon_nodes, aiming_weight);
    for (std::size_t i=0; i<arms_mapping.size(); ++i)
      nodes[std::size_t(arms_mapping[i])] = arms_nodes[i];
  }
  pose.nodes=std::move(nodes);
  FirstPersonPose result{skin_model(weapon, pose.nodes, pose.model_mapping),
                         skin_model(arms, pose.nodes, arms_mapping),{}, {}};
  result.camera_adjustment=adjustment;
  for(const auto node:pose.model_mapping)
    result.weapon_nodes.push_back(pose.nodes[std::size_t(node)]);
  for(const auto node:arms_mapping)
    result.arms_nodes.push_back(pose.nodes[std::size_t(node)]);
  return result;
}
} // namespace
FirstPersonPose pose_first_person_models(const RenderModel &weapon,
                                         const RenderModel &arms,
                                         const AnimationClip &clip,
                                         float seconds, Vec3 weapon_offset) {
  return first_person_pose(weapon,arms,clip,seconds,weapon_offset,nullptr,nullptr,0);
}
FirstPersonPose pose_first_person_models_world(const RenderModel &weapon,
                                               const RenderModel &arms,
                                               const AnimationClip &clip,
                                               float seconds, Vec3 weapon_offset,
                                               const Transform &camera,
                                               const FirstPersonIkDefinition &ik,
                                               float aiming_weight,bool secondary,const Transform *primary_adjustment,
                                               const AnimationClip *ammunition,float ammunition_frame) {
  return first_person_pose(weapon,arms,clip,seconds,weapon_offset,&camera,&ik,aiming_weight,-1,secondary,primary_adjustment,ammunition,ammunition_frame);
}
FirstPersonPose pose_first_person_models_world_frame(const RenderModel &weapon,
                                                     const RenderModel &arms,
                                                     const AnimationClip &clip,float frame,
                                                     Vec3 weapon_offset,const Transform &camera,
                                                     const FirstPersonIkDefinition &ik,float aiming_weight,
                                                     bool secondary,const Transform *primary_adjustment,
                                               const AnimationClip *ammunition,float ammunition_frame) {
  if(!std::isfinite(frame) || frame<0)throw FormatError("Invalid registered first-person clock frame");
  return first_person_pose(weapon,arms,clip,0,weapon_offset,&camera,&ik,aiming_weight,frame,secondary,primary_adjustment,ammunition,ammunition_frame);
}
} // namespace halo2
