#include "halo2/frontend_model_pose.hpp"
#include "halo2/source_model_pose.hpp"
#include <algorithm>
namespace halo2 {
namespace {
bool selected(std::span<const std::uint32_t>mask,unsigned n){
  if(n>=256 || mask.size()<8)throw FormatError("frontend deferred mask is not original256 bits");
  return mask[n/32]&(1u<<(n%32));
}
void require_initial(std::span<const std::uint32_t>mask,unsigned n){
  if(selected(mask,n))throw FormatError("frontend weapon callback depends on deferred node");
}
}
void frontend_model_deferred_world(const RenderModel&model,std::span<Transform>world,
    std::span<const BonePose>locals,std::span<const std::uint32_t>mask){
  if(world.empty() || world.size()!=model.bones.size() || locals.size()!=world.size() || world.size()>255)
    throw FormatError("frontend deferred hierarchy bank mismatch");
  if(selected(mask,0))throw FormatError("frontend deferred root requires unresolved source update");
  for(unsigned n=1;n<world.size();++n)if(selected(mask,n)){
    const auto parent=model.bones[n].parent;
    if(parent<0 || unsigned(parent)>=n)throw FormatError("frontend source hierarchy is not parent-before-child");
    world[n]=compose_transform(world[unsigned(parent)],source_model_local_transform(locals[n]));
  }
}
ObjectAttachmentFrame frontend_model_initialize_weapon_attachment(
    const RenderModel&model,const Transform&root,const AnimationClip&startup,
    const FrontendModelPrimary&primary,const WeaponAttachmentMarkers&attachment){
  if(attachment.named_child || attachment.parent.region!=-1)
    throw FormatError("frontend source attachment requires unresolved variant state");
  const auto locals=frontend_model_locals(model,startup,0,{},&primary);
  const auto body=pose_model_locals_nodes_world(model,root,locals);
  if(attachment.parent.node>=body.size())
    throw FormatError("frontend startup attachment node outside model");
  const auto&parent=body[attachment.parent.node];
  const auto marker=compose_transform(parent,attachment.parent.local);
  const auto aligned=source_object_attachment_frame({{}, {1,0,0}, {0,0,1}},
      marker,attachment.child.local,false);
  return source_object_attachment_local_frame(aligned,parent);
}
FrontendModelPose frontend_model_pose(const RenderModel&model,const Transform&root,
    const AnimationClip&clip,float frame,std::span<const std::uint32_t>deferred,
    const FrontendModelGrip*grip,const RenderModel*weapon_model,const AnimationClip*weapon_clip,
    const WeaponAttachmentMarkers*attachment,const FrontendModelWeaponIk*ik,float weight,
    const FrontendModelPrimary*primary,const ObjectAttachmentFrame*retained){
  if(model.bones.empty() || model.bones.size()>255)throw FormatError("frontend source node count invalid");
  std::array<std::uint32_t,8> initial;
  if(deferred.size()!=initial.size())throw FormatError("frontend source deferred mask size changed");
  for(unsigned i=0;i<initial.size();++i)initial[i]=~deferred[i];
  require_initial(deferred,0);
  auto locals=frontend_model_locals(model,clip,frame,initial,primary);
  if(grip)apply_frontend_model_grip(model,locals,*grip,initial);
  FrontendModelPose out;out.body=pose_model_locals_nodes_world(model,root,locals);
  if(weapon_model){
    if(!attachment || !retained || attachment->named_child || attachment->parent.region!=-1)
      throw FormatError("frontend source attachment requires unresolved variant state");
    const auto node=attachment->parent.node;
    if(node>=out.body.size())throw FormatError("frontend source attachment node outside model");
    require_initial(deferred,node);
    const auto&parent=out.body.at(node);
    const auto weapon_root=source_object_transform(retained->position,
        retained->forward,retained->up,1,false,&parent);
    out.weapon=pose_model_nodes_world_frame(*weapon_model,weapon_root,weapon_clip,0);
    if(ik){
      for(const auto&constraint:ik->definition.constraints){
        unsigned n=constraint.arms_node;
        for(unsigned k=0;k<3;++k){require_initial(deferred,n);const auto parent_index=model.bones.at(n).parent;if(k<2){if(parent_index<0)throw FormatError("frontend IK chain lacks source parent");n=unsigned(parent_index);}}
      }
      apply_frontend_model_weapon_ik(*ik,model,out.body,out.weapon,weight);
    }
  }
  auto sampled=frontend_model_locals(model,clip,frame,deferred,primary);
  if(grip)apply_frontend_model_grip(model,sampled,*grip,deferred);
  for(unsigned n=0;n<locals.size();++n)if(selected(deferred,n))locals[n]=sampled[n];
  frontend_model_deferred_world(model,out.body,locals,deferred);
  return out;
}
}
