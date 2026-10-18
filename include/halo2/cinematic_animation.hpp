#pragma once
#include "halo2/model.hpp"
#include "halo2/unit_animation.hpp"
#include "halo2/campaign_cinematic_state.hpp"

namespace halo2 {
struct CinematicAnimationChannel;
// The cache must outlive this read-only asset facade and its borrowers.
// Hierarchies are retained without mesh payloads; decoded clips are shared only
// while live channels own them, so scene cleanup releases their frame banks.
class CinematicAnimationAssets {
public:
  explicit CinematicAnimationAssets(const Cache &);
  ~CinematicAnimationAssets();
  // Resolve MCC emotion names to the original111080 namespace; OG IDs pass through.
  std::uint32_t unit_emotion_name_tu15(std::uint32_t)const;
  std::shared_ptr<const AnimationClip> animation(std::uint32_t graph,std::uint32_t name);
  UnitAnimationDefinition definition(const AnimationClip &)const;
  const RenderModel &hierarchy(std::uint32_t object_definition);
  const FrontendModelPrimary &primary(const std::shared_ptr<const AnimationClip> &);
  void retire_animations();
  const AnimationClip *idle(std::uint32_t object_definition);
  CinematicAnimationChannel initialized_channel(std::uint32_t object_definition,const AnimationClip &)const;
  std::uint32_t initialized_variant(const AnimationClip &,std::uint32_t graph,std::uint32_t packed)const;
  std::vector<ObjectAttachmentMarker> markers(std::uint32_t object_definition,std::uint32_t name)const;
private:
  struct State;
  std::unique_ptr<State> state_;
};
// std::function retains this typed adapter beside its existing clip callable.
// Camera loading continues through the original callable; cinematic object
// channels can also obtain the same cache's full event definitions/hierarchies.
struct CinematicAnimationLoader {
  std::function<AnimationClip(std::uint32_t,std::uint32_t)> load;
  std::shared_ptr<CinematicAnimationAssets> assets;
  std::shared_ptr<CampaignCinematicState> state;
  AnimationClip operator()(std::uint32_t graph,std::uint32_t name)const {
    if(!load)throw FormatError("Cinematic animation clip loader is unavailable");
    return load(graph,name);
  }
};
// Retained primary channel for the type0 custom animations used by the opening.
// Metadata and event consumers come from the caller's actual cache/runtime.
// Clock advancement is the shared original unit/scenery channel implementation.
struct CinematicAnimationChannel {
  std::array<std::byte,0x90> manager{};
  UnitAnimationDefinition definition;
  bool unit{},hold{},paused{};
};
enum class CinematicAnimationAdvance {Running,Held,StopCustom};
using CinematicAnimationEventConsumer =
    std::function<void(float,const std::array<std::byte,20>&)>;
CinematicAnimationChannel start_cinematic_animation(const AnimationClip &,
    UnitAnimationDefinition,bool unit,bool loop,bool hold,std::int16_t frame=0);
// Admitted local, resident, unlinked type0 default-primary branch of
//1D3180/1D3380. The cache supplies the actual selected clip and normalized
// identifiers; source-world resource/RNG/layer controllers remain separate.
CinematicAnimationChannel start_initialized_object_animation(const AnimationClip &,
    UnitAnimationDefinition,const std::array<std::uint32_t,4> &identifiers,
    std::array<std::byte,0x90> backing={});
bool custom_cinematic_animation(const CinematicAnimationChannel &);
void advance_initialized_object_animation(CinematicAnimationChannel &,
    const std::function<std::uint32_t(std::uint32_t)> &resident_variant,
    const CinematicAnimationEventConsumer & = {});
CinematicAnimationAdvance advance_cinematic_animation(CinematicAnimationChannel &,
    const CinematicAnimationEventConsumer & = {});
float cinematic_animation_frame(const CinematicAnimationChannel &);
float cinematic_animation_sample_frame(const CinematicAnimationChannel &);
std::uint8_t cinematic_animation_completion(const CinematicAnimationChannel &);
std::int16_t cinematic_animation_time(const CinematicAnimationChannel &);
// 112DC0/10A390 apply a valid live anchor's current node0 to the supplied
// animated matrix. NONE/destroyed anchors leave that matrix unchanged.
Transform cinematic_animation_anchor(const Transform &animated,
    const std::optional<Transform> &live_anchor_node0);
}
