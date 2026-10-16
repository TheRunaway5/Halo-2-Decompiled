#include "halo2/cinematic_animation.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/unit_animation_selection.hpp"
#include "halo2/frontend_model_primary.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <map>

namespace halo2 {
struct CinematicAnimationAssets::State {
  explicit State(const Cache &source):cache(source){}
  const Cache &cache;
  std::map<std::pair<std::uint32_t,std::uint32_t>,std::weak_ptr<const AnimationClip>> clips;
  std::map<std::uint32_t,RenderModel> models;
  std::map<std::uint32_t,std::optional<AnimationClip>> idles;
  struct Primary {
    FrontendModelPrimary data;
    std::vector<std::weak_ptr<const AnimationClip>> owners;
  };
  std::map<std::pair<std::uint32_t,std::uint32_t>,Primary> primaries;
};
CinematicAnimationAssets::CinematicAnimationAssets(const Cache &cache):state_(std::make_unique<State>(cache)){}
CinematicAnimationAssets::~CinematicAnimationAssets()=default;
std::shared_ptr<const AnimationClip> CinematicAnimationAssets::animation(std::uint32_t graph,std::uint32_t name) {
  auto &saved=state_->clips[{graph,name}];if(auto resident=saved.lock())return resident;
  const auto index=state_->cache.find_local_animation_index(graph,name);if(!index)return {};
  auto clip=std::make_shared<const AnimationClip>(state_->cache.load_animation(graph,*index));saved=clip;return clip;
}
UnitAnimationDefinition CinematicAnimationAssets::definition(const AnimationClip &clip)const {
  return state_->cache.load_source_unit_animation_definition(clip.source_graph,(clip.source_index<<16)|0xffffu);
}
const RenderModel &CinematicAnimationAssets::hierarchy(std::uint32_t id) {
  const auto found=state_->models.find(id);if(found!=state_->models.end())return found->second;
  return state_->models.emplace(id,state_->cache.load_model_hierarchy(id)).first->second;
}
void CinematicAnimationAssets::retire_animations() {
  for(auto at=state_->primaries.begin();at!=state_->primaries.end();) {
    std::erase_if(at->second.owners,[](const auto &owner){return owner.expired();});
    if(at->second.owners.empty())at=state_->primaries.erase(at);else ++at;
  }
}
const FrontendModelPrimary &CinematicAnimationAssets::primary(const std::shared_ptr<const AnimationClip> &clip) {
  if(!clip)throw FormatError("Cinematic primary lost its live clip owner");
  retire_animations();
  const auto key=std::pair(clip->source_graph,clip->source_index);
  auto found=state_->primaries.find(key);
  if(found==state_->primaries.end()) {
    State::Primary primary;
    primary.data=state_->cache.load_frontend_model_primary_animation(key.first,key.second);
    found=state_->primaries.emplace(key,std::move(primary)).first;
  }
  if(found->second.data.frame_count!=clip->frame_count || found->second.data.nodes!=clip->nodes)
    throw FormatError("Cinematic primary resource differs from its retained controller metadata");
  if(std::ranges::none_of(found->second.owners,[&](const auto &owner){return owner.lock()==clip;}))
    found->second.owners.push_back(clip);
  return found->second.data;
}
const AnimationClip *CinematicAnimationAssets::idle(std::uint32_t id) {
  auto found=state_->idles.find(id);
  if(found==state_->idles.end())found=state_->idles.emplace(id,state_->cache.load_initialized_object_idle(id)).first;
  return found->second?&*found->second:nullptr;
}
CinematicAnimationChannel CinematicAnimationAssets::initialized_channel(std::uint32_t id,const AnimationClip &clip)const {
  const auto object=state_->cache.load_source_tag_metadata(id);
  const Bytes object_header(object.bytes_at(object.source_address(),0x3c));
  if(object_header.u16(0)!=0 && object_header.u16(0)!=1)
    throw FormatError("Initialized attachment requires its scenery named-idle override owner");
  const auto model_id=object_header.u32(0x38);
  const auto model=state_->cache.load_source_tag_metadata(model_id);
  const Bytes model_header(model.bytes_at(model.source_address(),0x7c));
  const auto graph_id=model_header.u32(0x14);
  const auto graph=state_->cache.load_source_tag_metadata(graph_id);
  const Bytes header(graph.bytes_at(graph.source_address(),0x5c));
  if(std::int16_t(header.u16(0xc))!=std::bit_cast<std::int32_t>(model_header.u32(0x78)))
    throw FormatError("Initialized attachment animation has incompatible model/graph nodes");
  if(graph_id!=clip.source_graph)
    throw FormatError("Initialized attachment animation requires its inherited controller owner");
  if(!clip.movement.empty() || !clip.movement_yaw.empty())
    throw FormatError("Initialized attachment animation requires its physical root-motion owner");
  // The decoded clip is the native cache's actual resident resource. Linked
  // variants require their original RNG and resource selection, never NONE.
  initialized_variant(clip,graph_id,(clip.source_index<<16)|0xffffu);
  std::array<std::byte,0x90> manager{};
  for(unsigned i=0;i<4;++i)manager[0x68+i]=std::byte(graph_id>>(i*8));
  SourceAnimationGraphMemory memory;
  memory.graph_address=[&](auto datum) {
    if(datum!=graph_id)throw FormatError("Initialized attachment lost its master graph");
    return graph.source_address();
  };
  memory.read=[&](auto address,auto count){return graph.bytes_at(address,count);};
  const auto identifiers=normalize_source_animation_request(manager,
      {0x07000001u,0x07000001u,0x07000001u,0x07000001u},memory);
  return start_initialized_object_animation(clip,definition(clip),identifiers);
}
std::uint32_t CinematicAnimationAssets::initialized_variant(const AnimationClip &clip,
    std::uint32_t graph_id,std::uint32_t packed)const {
  if(graph_id!=clip.source_graph || packed!=((clip.source_index<<16)|0xffffu))
    throw FormatError("Initialized attachment lost its resident local animation");
  const auto graph=state_->cache.load_source_tag_metadata(graph_id);
  const Bytes header(graph.bytes_at(graph.source_address(),0x34));
  if(clip.source_index>=header.u32(0x2c))throw FormatError("Initialized attachment clip is outside its graph");
  const Bytes animation(graph.bytes_at(header.u32(0x30)+clip.source_index*0x6c,0x6c));
  if(animation.u16(0x30)!=0xffffu || animation.u16(0x32)!=0xffffu)
    throw FormatError("Initialized attachment requires its linked-variant RNG/resource owner");
  (void)definition(clip);
  return packed;
}
std::vector<ObjectAttachmentMarker> CinematicAnimationAssets::markers(std::uint32_t id,std::uint32_t name)const {
  if(!name || name==0xffffffffu)return {};
  const auto object=state_->cache.load_source_tag_metadata(id);
  const auto model=Bytes(object.bytes_at(object.source_address(),0x3c)).u32(0x38);
  if(model==0xffffffffu)return {};
  const auto definition=state_->cache.load_source_tag_metadata(model);
  const auto render=Bytes(definition.bytes_at(definition.source_address(),8)).u32(4);
  if(render==0xffffffffu)return {};
  return source_model_markers(state_->cache.load_source_tag_metadata(render),name);
}
namespace {
std::uint32_t get(const CinematicAnimationChannel &state,unsigned at,unsigned n=4) {
  std::uint32_t value=0;
  for(unsigned i=0;i<n;++i)value|=std::uint32_t(std::to_integer<unsigned>(state.manager.at(at+i)))<<(8*i);
  return value;
}
void put(CinematicAnimationChannel &state,unsigned at,std::uint32_t value,unsigned n=4) {
  for(unsigned i=0;i<n;++i)state.manager.at(at+i)=std::byte(value>>(8*i));
}
float mul(float a,float b){volatile float value=a*b;return value;}
float sub(float a,float b){volatile float value=a-b;return value;}
std::int32_t nearest_even(float value) {
  const float lower=std::floor(value),fraction=value-lower;
  const auto integer=std::int32_t(lower);
  return integer+int(fraction>.5f || (fraction==.5f && (integer&1)));
}
UnitAnimationServices services(CinematicAnimationChannel &state,
    const CinematicAnimationEventConsumer &consumer) {
  UnitAnimationServices result;result.tick_seconds=1.f/30.f;
  result.definition=[&state](auto graph,auto packed)->const UnitAnimationDefinition& {
    if(graph!=get(state,0) || packed!=get(state,4))
      throw FormatError("Cinematic animation definition differs from its retained channel");
    return state.definition;
  };
  result.event=[consumer](auto,float frame,const auto &event,auto,auto) {
    if(!consumer)throw FormatError("Cinematic animation event has no runtime consumer");
    consumer(frame,event);
  };
  return result;
}
}
CinematicAnimationChannel start_cinematic_animation(const AnimationClip &clip,
    UnitAnimationDefinition definition,bool unit,bool loop,bool hold,std::int16_t frame) {
  if(clip.animation_type!=0 || clip.source_graph==0xffffffffu || clip.source_index>0x7fffu ||
      clip.frame_count==0 || clip.frame_count>0x7fffu ||
      definition.frame_count!=int(clip.frame_count) || definition.loop_frame!=int(clip.loop_frame))
    throw FormatError("Cinematic custom animation requires its actual type0 graph/clip definition");
  CinematicAnimationChannel state;state.definition=std::move(definition);
  state.unit=unit;state.hold=hold;
  // 1C6470 installs local packed index (inherited=-1), rate1 and active
  // custom flags4001/4003. The other manager channels are genuinely absent.
  for(unsigned at:{32u,64u}) {put(state,at,0xffffffffu);put(state,at+4,0xffffffffu);}
  put(state,0,clip.source_graph);put(state,4,(clip.source_index<<16)|0xffffu);
  put(state,0x12,loop?0x4003u:0x4001u,2);
  put(state,0x18,std::bit_cast<std::uint32_t>(1.f));put(state,0x68,clip.source_graph);
  put(state,0x7c,unit?0x0e0000c2u:0u);
  // 1CCB80 / 11B4A0 retain the signed-short seconds/frame round trip.
  const float start=mul(mul(float(frame),1.f/30.f),30.f);
  auto source=services(state,{});
  clamp_source_unit_animation_frame(std::span(state.manager).first(32),start,source);
  return state;
}
CinematicAnimationChannel start_initialized_object_animation(const AnimationClip &clip,
    UnitAnimationDefinition definition,const std::array<std::uint32_t,4> &identifiers,
    std::array<std::byte,0x90> backing) {
  if(clip.animation_type!=0 || clip.source_graph==0xffffffffu || clip.source_index>0x7fffu ||
      !clip.frame_count || clip.frame_count>0x7fffu || definition.frame_count!=int(clip.frame_count) ||
      definition.loop_frame!=int(clip.loop_frame) || definition.authored_flags ||
      !clip.movement.empty() || !clip.movement_yaw.empty())
    throw FormatError("Initialized attachment requires its admitted type0 default controller branch");
  CinematicAnimationChannel state;state.manager=backing;state.definition=std::move(definition);state.unit=true;
  // Full constructor/reset final stores preserve the untouched root-delta tail.
  reset_source_unit_animation_channels(state.manager,0x3f800000u);
  for(unsigned at=0x60;at<0x68;++at)put(state,at,0,1);
  put(state,0x68,clip.source_graph);put(state,0x6c,2,2);put(state,0x6e,0,2);
  for(unsigned i=0;i<4;++i)put(state,0x70+i*4,identifiers[i]);
  put(state,0x80,0);
  if(!install_source_unit_animation_channel(state.manager,0,clip.source_graph,0x3f,
      (clip.source_index<<16)|0xffffu,identifiers[3],0xffffffffu,0xffffffffu,0,0x3f800000u))
    throw FormatError("Initialized attachment primary channel was not admitted");
  auto source=services(state,{});
  clamp_source_unit_animation_frame(std::span(state.manager).first(32),0,source);
  put(state,0x66,1,1);return state;
}
bool custom_cinematic_animation(const CinematicAnimationChannel &state) {
  return (get(state,0x12,2)&0x4000u)!=0;
}
void advance_initialized_object_animation(CinematicAnimationChannel &state,
    const std::function<std::uint32_t(std::uint32_t)> &resident_variant,
    const CinematicAnimationEventConsumer &consumer) {
  if(custom_cinematic_animation(state))throw FormatError("Ordinary animation received a custom controller");
  auto source=services(state,consumer);
  source.select_variant=[&](auto,std::uint32_t packed) {
    if(!resident_variant)throw FormatError("Initialized attachment resident variant owner is unavailable");
    return resident_variant(packed);
  };
  advance_source_unit_animation_transitions(state.manager);
  if(!update_source_unit_animation_manager(state.manager,0,{},{},{1,0},source))
    throw FormatError("Initialized attachment manager lost its graph");
}
CinematicAnimationAdvance advance_cinematic_animation(CinematicAnimationChannel &state,
    const CinematicAnimationEventConsumer &consumer) {
  if(state.paused)return CinematicAnimationAdvance::Held;
  auto source=services(state,consumer);
  advance_source_unit_animation_channel(std::span(state.manager).first(32),state.manager,
      {1,0},source);
  // 111650 consumes the anticipatory completion bit after channel update.
  // Cinematic-held units pause at N-1; scenery retains the raw N-.0001 clock.
  if(state.unit && !(get(state,0x12,2)&2u) && (get(state,0x11,1)&2u)) {
    if(!state.hold)return CinematicAnimationAdvance::StopCustom;
    state.paused=true;put(state,0x6c,get(state,0x6c,1)|1u,1);
    return CinematicAnimationAdvance::Held;
  }
  return CinematicAnimationAdvance::Running;
}
float cinematic_animation_frame(const CinematicAnimationChannel &state) {
  return std::bit_cast<float>(get(state,0x1c));
}
float cinematic_animation_sample_frame(const CinematicAnimationChannel &state) {
  const float frame=cinematic_animation_frame(state),last=float(state.definition.frame_count-1);
  const float index=std::clamp(std::floor(frame),0.f,last);
  const auto options=get(state,0x12,2);const float rate=std::bit_cast<float>(get(state,0x18));
  if(index==last || (!(options&0x40u) && !((options&1u) && rate!=0.f && rate!=1.f)))return index;
  const float fraction=frame-std::floor(frame);
  if(fraction<.0001f)return index;
  if(fraction>.9999f)return std::min(last,index+1.f);
  return index+fraction;
}
std::uint8_t cinematic_animation_completion(const CinematicAnimationChannel &state) {
  return std::uint8_t(get(state,0x11,1));
}
std::int16_t cinematic_animation_time(const CinematicAnimationChannel &state) {
  //11DD90 admits only the custom activity, even when an ordinary primary
  //channel is valid. Scenery10F510 queries any valid scenery channel.
  if(state.unit && get(state,0x7c)!=0x0e0000c2u)return 0;
  const float duration=mul(float(state.definition.frame_count),1.f/30.f);
  const float elapsed=mul(cinematic_animation_frame(state),1.f/30.f);
  auto ticks=nearest_even(mul(sub(duration,elapsed),30.f));
  if(!state.unit)ticks=std::bit_cast<std::int16_t>(std::uint16_t(ticks));
  return std::bit_cast<std::int16_t>(std::uint16_t(std::max(0,ticks-2)));
}
Transform cinematic_animation_anchor(const Transform &animated,
    const std::optional<Transform> &anchor) {
  return anchor?compose_transform(*anchor,animated):animated;
}
}
