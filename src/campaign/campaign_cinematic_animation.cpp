#include "halo2/simulation.hpp"
#include "halo2/math.hpp"
#include "halo2/source_model_pose.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/source_scene_physics.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <sstream>

namespace halo2 {
std::set<std::uint32_t> cinematic_lighting_definitions(const LevelContent &content) {
  std::set<std::uint32_t> result;bool computed=false;
  const auto &program=content.scripts;
  for(const auto &function:program.nodes) {
    if(!(function.flags&1) || function.type!=2 || function.string_offset>=program.strings.size())continue;
    const auto end=program.strings.find('\0',function.string_offset);
    if(end==std::string::npos || program.strings.substr(function.string_offset,end-function.string_offset)!=
        "object_uses_cinematic_lighting")continue;
    if(function.next==null_datum || (function.next&65535)>=program.nodes.size())
      throw FormatError("Cinematic lighting residency has no object expression");
    const auto &object=program.nodes[function.next&65535];
    if(object.salt!=function.next>>16)throw FormatError("Cinematic lighting residency has a stale object expression");
    if(!(object.flags&1) || (object.flags&4) || object.opcode<56 || object.opcode>61) {computed=true;continue;}
    const auto name=std::int16_t(object.value);
    if(name<0)continue;
    const auto &named=content.object_names.at(unsigned(name));
    const auto found=std::ranges::find_if(content.objects,[&](const auto &row) {
      return row.type==named.type && row.placement==named.placement;
    });
    if(found!=content.objects.end() && found->definition!=null_datum)result.insert(found->definition);
  }
  if(computed)for(const auto &object:content.objects)
    if(object.definition!=null_datum)result.insert(object.definition);
  return result;
}
const CampaignCinematicState *Simulation::cinematic_state()const {
  const auto *adapter=animation_loader_.target<CinematicAnimationLoader>();
  return adapter?adapter->state.get():nullptr;
}
CampaignCinematicState &Simulation::mutable_cinematic_state() {
  auto *adapter=animation_loader_.target<CinematicAnimationLoader>();
  if(!adapter) {
    animation_loader_=CinematicAnimationLoader{animation_loader_,{}, {}};
    adapter=animation_loader_.target<CinematicAnimationLoader>();
  }
  if(!adapter->state)adapter->state=std::make_shared<CampaignCinematicState>();
  return *adapter->state;
}
void Simulation::cinematic_animation_assets(std::shared_ptr<CinematicAnimationAssets> assets) {
  NativeAuthorityBoundary::Call active(authority_calls_);
  const auto *current=animation_loader_.target<CinematicAnimationLoader>();
  if(!assets || (current?!current->load:!animation_loader_))throw FormatError("Cinematic object animations require their cache and clip loader");
  for(const auto &[handle,animation]:object_animations_) {
    (void)handle;if(animation.channel)throw FormatError("Cannot replace cinematic asset ownership during playback");
  }
  ++authority_calls_.mutations;
  if(auto *adapter=animation_loader_.target<CinematicAnimationLoader>())adapter->assets=std::move(assets);
  else animation_loader_=CinematicAnimationLoader{animation_loader_,std::move(assets),{}};
}
bool Simulation::start_cinematic_object_animation(std::uint32_t handle,std::uint32_t graph,
    std::uint32_t name,std::uint32_t anchor,bool unit,bool loop,bool interpolate) {
  if(handle==null_datum || graph==null_datum || (!unit && !name))return false;
  if(source_campaign_) {
    auto &objects=source_campaign_->objects();
    const auto source=[&](auto object){return object==0?player_.source_datum:object;};
    const bool started=source_campaign_cinematic_animation_tu15(objects.tu15_world(),
        source(handle),graph,name,source(anchor),unit,loop,interpolate);
    publish_source_scene_physics(scene_,objects.memory(),objects.source_object_pool());
    return started;
  }
  if(interpolate)throw FormatError("Cinematic object pose-transition interpolation owner is unavailable");
  auto &object=scene_.get(handle);const auto &placement=content_.objects.at(object.placement);
  if(unit?(placement.type!=0 && placement.type!=1):placement.type!=6)
    throw FormatError("Cinematic custom-animation object has the wrong source type");
  const auto *adapter=animation_loader_.target<CinematicAnimationLoader>();
  if(!adapter || !adapter->assets)throw FormatError("Cinematic object animation cache owner is unavailable");
  auto assets=adapter->assets;
  auto clip=assets->animation(graph,name);if(!clip)return false;
  (void)assets->hierarchy(placement.definition);
  auto channel=start_cinematic_animation(*clip,assets->definition(*clip),unit,loop,presentation_.cinematic);
  ObjectAnimation animation{std::move(clip),ticks_,loop};animation.assets=std::move(assets);
  animation.channel=std::move(channel);animation.anchor=anchor;
  if(anchor!=null_datum) {
    // Relative starts detach ordinary parenting and reset canonical placement;
    // the live anchor's posed node0 is applied before the child hierarchy.
    object.position={};object.rotation={};object.solved_transform.reset();
    object.attachment_parent=null_datum;
    if(cinematic_state())mutable_cinematic_state().attachments.erase(handle);
  }
  object_animations_.insert_or_assign(handle,std::move(animation));
  refresh_cinematic_object_poses();return true;
}
void Simulation::step_cinematic_object_animations() {
  if(auto *adapter=animation_loader_.target<CinematicAnimationLoader>();adapter && adapter->state)
    std::erase_if(adapter->state->lit_objects,[&](auto handle) {
      return handle==0?!player_.exists:!object_lifetimes_.contains(handle);
    });
  const auto alive=[&](auto handle){return std::ranges::any_of(scene_.objects(),
      [&](const auto &object){return object.handle==handle;});};
  if(auto *adapter=animation_loader_.target<CinematicAnimationLoader>();adapter && adapter->state)
    std::erase_if(adapter->state->attachments,[&](const auto &row) {
      if(!alive(row.first))return true;
      const auto &object=scene_.get(row.first);
      if(object.attachment_parent!=row.second.parent)return true;
      if(!alive(row.second.parent))throw FormatError("Cinematic attachment parent retired without its child");
      return false;
    });
  std::erase_if(object_animations_,[&](const auto &row){return !alive(row.first);});
  if(auto *adapter=animation_loader_.target<CinematicAnimationLoader>();adapter && adapter->assets)
    adapter->assets->retire_animations();
  for(auto &[handle,animation]:object_animations_)if(animation.channel) {
    (void)handle;
    const auto consume=[&](float,const auto &packet) {
      const auto type=std::to_integer<unsigned>(packet[0])|(std::to_integer<unsigned>(packet[1])<<8);
      if(type!=0)throw FormatError("Cinematic animation sound/effect event has no runtime owner");
      animation.events.push_back(packet);
    };
    if(!custom_cinematic_animation(*animation.channel)) {
      advance_initialized_object_animation(*animation.channel,[&](auto packed) {
        return animation.assets->initialized_variant(*animation.clip,animation.clip->source_graph,packed);
      },consume);
      continue;
    }
    const auto result=advance_cinematic_animation(*animation.channel,consume);
    if(result==CinematicAnimationAdvance::StopCustom)
      throw FormatError("Cinematic unit completion requires its stop/idle and physical-root owner");
  }
  refresh_cinematic_object_poses();
}
void Simulation::refresh_cinematic_object_poses(std::optional<std::uint32_t> requested) {
  std::map<std::uint32_t,std::vector<Transform>> posed;std::set<std::uint32_t> visiting;
  const auto *adapter=animation_loader_.target<CinematicAnimationLoader>();
  const auto *context=adapter?adapter->state.get():nullptr;
  const auto alive=[&](auto handle){return std::ranges::any_of(scene_.objects(),
      [&](const auto &object){return object.handle==handle;});};
  const auto evaluate=[&](auto &&self,std::uint32_t handle,CinematicAnimationAssets &assets)->const std::vector<Transform>& {
    if(const auto found=posed.find(handle);found!=posed.end())return found->second;
    if(!visiting.insert(handle).second)throw FormatError("Cinematic live-anchor hierarchy is cyclic");
    const auto &object=scene_.get(handle);const auto &placement=content_.objects.at(object.placement);
    auto animation=object_animations_.find(handle);
    bool animated=animation!=object_animations_.end() && animation->second.channel.has_value();
    const CampaignCinematicState::Attachment *attachment=nullptr;
    if(context)if(const auto found=context->attachments.find(handle);found!=context->attachments.end())
      attachment=&found->second;
    const bool attachment_pose=attachment || requested==handle || (context &&
        std::ranges::any_of(context->attachments,[&](const auto &row){return row.second.parent==handle;}));
    const AnimationClip *initialized=nullptr;
    if(!animated && attachment_pose) {
      initialized=assets.idle(placement.definition);
      if(initialized && initialized->frame_count>1) {
        // The fresh constructor's resident default channel is retained before
        // the object's ordinary update. Older births need their prior owner;
        // elapsed time is not reconstructed from a guessed idle loop.
        if(object.created_tick!=ticks_)
          throw FormatError("Cinematic attachment requires its prior initialized animation owner");
        ObjectAnimation initial{std::make_shared<const AnimationClip>(*initialized),ticks_,true};
        initial.assets=adapter->assets;
        initial.channel=assets.initialized_channel(placement.definition,*initialized);
        animation=object_animations_.insert_or_assign(handle,std::move(initial)).first;
        animated=true;
      }
    }
    const bool custom=animated && custom_cinematic_animation(*animation->second.channel);
    if(!animated && !attachment && object.solved_nodes) {
      visiting.erase(handle);return posed.emplace(handle,*object.solved_nodes).first->second;
    }
    const auto basis=authored_basis(object.rotation);
    Transform root{object.scale,{basis.forward,basis.left,basis.up},object.position};
    if(object.solved_transform)root=*object.solved_transform;
    if(attachment) {
      const auto &parent=self(self,attachment->parent,assets);
      if(attachment->node>=parent.size())throw FormatError("Cinematic attachment lost its current parent node");
      root=source_object_transform(attachment->local.position,attachment->local.forward,attachment->local.up,
          object.scale,false,&parent[attachment->node],false);
    }
    const AnimationClip *clip=nullptr;float frame=0;
    if(animated) {
      const auto &state=animation->second;clip=state.clip.get();frame=cinematic_animation_sample_frame(*state.channel);
      if(custom && state.anchor!=null_datum){root={};root.scale=object.scale;}
      if(custom && state.anchor!=null_datum && alive(state.anchor)) {
        const auto &anchor=self(self,state.anchor,assets);
        if(!anchor.empty())root=cinematic_animation_anchor(root,anchor.front());
      }
    } else {
      clip=initialized?initialized:assets.idle(placement.definition);
    }
    if(attachment)scene_.get(handle).solved_transform=root;
    const auto &hierarchy=assets.hierarchy(placement.definition);
    std::vector<Transform> nodes;
    try{nodes=animated?pose_world_object_nodes_frame(hierarchy,root,assets.primary(animation->second.clip),frame):
        pose_model_nodes_world_frame(hierarchy,root,clip,frame);}
    catch(const FormatError &error) {
      std::ostringstream detail;
      detail<<"Cinematic pose object="<<std::hex<<handle<<" definition="<<placement.definition
          <<" model="<<hierarchy.name<<" clip="<<(clip?clip->name:"NONE")
          <<" frame="<<std::dec<<frame<<": "<<error.what();
      throw FormatError(detail.str());
    }
    visiting.erase(handle);return posed.emplace(handle,std::move(nodes)).first->second;
  };
  for(const auto &[handle,animation]:object_animations_)if(animation.channel && alive(handle))
    (void)evaluate(evaluate,handle,*animation.assets);
  if(adapter && adapter->assets) {
    if(context)for(const auto &[handle,attachment]:context->attachments) {
      (void)attachment;if(alive(handle))(void)evaluate(evaluate,handle,*adapter->assets);
    }
    if(requested)(void)evaluate(evaluate,*requested,*adapter->assets);
  } else if(requested || (context && !context->attachments.empty()))
    throw FormatError("Cinematic attachment cache owner is unavailable");
  for(auto &[handle,nodes]:posed) {
    const auto animation=object_animations_.find(handle);
    if((animation!=object_animations_.end() && animation->second.channel) || requested==handle ||
        (context && context->attachments.contains(handle)))
      scene_.get(handle).solved_nodes=std::move(nodes);
  }
}
void Simulation::attach_cinematic_objects(std::uint32_t parent,std::uint32_t parent_marker,
    std::uint32_t child,std::uint32_t child_marker) {
  if(parent==null_datum || child==null_datum)return;
  if(source_campaign_) {
    auto &objects=source_campaign_->objects();
    const auto source=[&](auto object){return object==0?player_.source_datum:object;};
    source_campaign_cinematic_attach_tu15(objects.tu15_world(),source(parent),parent_marker,
        source(child),child_marker);
    publish_source_scene_physics(scene_,objects.memory(),objects.source_object_pool());
    return;
  }
  if(scene_.get(child).attachment_parent!=null_datum)return; //10FFD0 already attached guard.
  const auto *adapter=animation_loader_.target<CinematicAnimationLoader>();
  if(!adapter || !adapter->assets)throw FormatError("Cinematic attachment cache owner is unavailable");
  auto assets=adapter->assets;
  for(auto ancestor=parent;ancestor!=null_datum;ancestor=scene_.get(ancestor).attachment_parent)
    if(ancestor==child)throw FormatError("Cinematic object attachment hierarchy is cyclic");
  for(const auto &object:scene_.objects())
    if(object.handle!=null_datum && object.attachment_parent==parent)
      throw FormatError("Multiple cinematic children require the original sibling-order owner");
  const auto definition=[&](auto handle) {
    const auto &placement=content_.objects.at(scene_.get(handle).placement);
    if(placement.type!=0 && placement.type!=1 && placement.type!=6)
      throw FormatError("Cinematic attachment needs its original object-type owner");
    return placement.definition;
  };
  const auto select=[&](auto handle,auto name) {
    const auto markers=assets->markers(definition(handle),name);
    if(markers.empty())return ObjectAttachmentMarker{}; //BAD60 identity/node0 fallback.
    if(markers.front().region!=-1)
      throw FormatError("Cinematic attachment marker requires its live region-permutation owner");
    return markers.front();
  };
  const auto parent_selected=select(parent,parent_marker),child_selected=select(child,child_marker);
  refresh_cinematic_object_poses(parent);refresh_cinematic_object_poses(child);
  const auto &parent_nodes=*scene_.get(parent).solved_nodes;
  const auto &child_nodes=*scene_.get(child).solved_nodes;
  if(parent_selected.node>=parent_nodes.size() || child_selected.node>=child_nodes.size())
    throw FormatError("Cinematic attachment marker has no current posed node");
  auto &object=scene_.get(child);
  const auto basis=authored_basis(object.rotation);
  const Transform current=object.solved_transform.value_or(
      Transform{object.scale,{basis.forward,basis.left,basis.up},object.position});
  const ObjectAttachmentFrame current_frame{current.position,current.basis[0],current.basis[2]};
  const auto parent_world=compose_transform(parent_nodes[parent_selected.node],parent_selected.local);
  const bool named_child=child_marker && child_marker!=null_datum;
  const auto child_frame=named_child?compose_transform(child_nodes[child_selected.node],child_selected.local):child_selected.local;
  const auto aligned=source_object_attachment_frame(current_frame,parent_world,child_frame,named_child);
  const auto local=source_object_attachment_local_frame(aligned,parent_nodes[parent_selected.node]);
  mutable_cinematic_state().attachments.insert_or_assign(child,
      CampaignCinematicState::Attachment{parent,parent_selected.node,local});
  object.attachment_parent=parent;object.position=local.position;object.solved_nodes.reset();
  refresh_cinematic_object_poses(child);
}
}
