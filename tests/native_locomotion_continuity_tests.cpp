#define SDL_MAIN_HANDLED
#include "halo2/native_locomotion.hpp"
#include "halo2/original_data.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <new>
#include <thread>
#include <vector>

namespace {
thread_local bool watch_storage{};
thread_local std::size_t allocations{},retirements{};
void *allocate(std::size_t n) {
  if(watch_storage)++allocations;
  if(auto *p=std::malloc(n?n:1))return p;throw std::bad_alloc();
}
void retire(void *p)noexcept {if(p&&watch_storage)++retirements;std::free(p);}
}
void *operator new(std::size_t n){return allocate(n);}
void *operator new[](std::size_t n){return allocate(n);}
void operator delete(void *p)noexcept{retire(p);}
void operator delete[](void *p)noexcept{retire(p);}
void operator delete(void *p,std::size_t)noexcept{retire(p);}
void operator delete[](void *p,std::size_t)noexcept{retire(p);}

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
template<class F>void rejects(F &&f,const char *message){bool rejected=false;try{f();}catch(const FormatError&){rejected=true;}check(rejected,message);}
std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
struct Frame {
  std::uint32_t graph{},index{},activity{},clock{};
  bool operator==(const Frame &)const=default;
};
Frame frame(const NativeLocomotionFrame &f){check(f.clip!=nullptr,"Locomotion frame lost its actual decoded clip");return {f.clip->source_graph,f.clip->source_index,f.activity,bits(f.frame)};}
struct Observation {
  std::array<std::byte,0x90> manager{};
  std::array<Frame,5> samples{};
  std::array<std::uint32_t,5> facing{};
  bool operator==(const Observation &)const=default;
};
constexpr std::array<float,5> fractions{0,.25f,.5f,.75f,1};
Observation observe(const NativeLocomotion &gait) {
  Observation result;const auto before=gait.manager();check(before.size()==result.manager.size(),"Native manager extent changed");
  std::copy(before.begin(),before.end(),result.manager.begin());
  for(unsigned i=0;i<fractions.size();++i){result.samples[i]=frame(gait.sample(fractions[i]));result.facing[i]=bits(gait.facing(fractions[i]));}
  check(std::equal(gait.manager().begin(),gait.manager().end(),result.manager.begin()),"Sampling changed the discrete manager");
  check(frame(gait.frame())==result.samples.back(),"Frame and complete interpolation endpoint disagree");return result;
}
struct Foot {
  unsigned timing{},kind{},marker{};std::uint32_t clock{};
  bool operator==(const Foot &)const=default;
};
struct Input {
  float forward{},right{};bool grounded=true,crouch{};Vec3 velocity{};float yaw{};
  SourceLandingState landing{};bool has_landing{};
};
Input input(unsigned tick,const SourceLandingDefinition &definition) {
  Input value;const auto phase=tick%420;
  if(phase<90){value.forward=1;value.velocity={2,0,0};value.yaw=.02f*float(phase);}
  else if(phase<150){value.yaw=-1.8f;}
  else if(phase<210){value.right=1;value.crouch=true;value.velocity={0,2,0};value.yaw=1.4f;}
  else if(phase<226){value.forward=1;value.grounded=false;value.velocity={0,0,3};value.yaw=1.4f;}
  else if(phase<232){
    bool forced=false;begin_source_landing(value.landing,definition,(definition.soft_speed+definition.hard_speed)*.5f,30,forced);
    for(unsigned i=226;i<phase;++i)advance_source_landing(value.landing,false);
    value.has_landing=true;value.yaw=1.4f;
  }else if(phase<330){value.forward=-1;value.crouch=true;value.velocity={-2,0,0};value.yaw=-.7f;}
  else {value.yaw=1.8f;}
  return value;
}
std::vector<Foot> advance(NativeLocomotion &gait,unsigned tick,const SourceLandingDefinition &landing) {
  const auto i=input(tick,landing);std::vector<Foot> events;
  gait.tick(i.forward,i.right,i.grounded,i.crouch,i.velocity,[&](const NativeFootstepEvent &e){
    // This is also the production footstep callback's read-only access pattern.
    check(gait.frame().clip!=nullptr&&std::isfinite(gait.facing()),"Active footstep lost frame/facing observation");
    events.push_back({e.timing,e.kind,e.marker,bits(e.frame)});
  },i.has_landing?&i.landing:nullptr,&i.yaw);return events;
}
template<class F>void storage_free_commit(F &&operation,const char *message) {
  allocations=retirements=0;watch_storage=true;const bool committed=operation();watch_storage=false;
  check(committed&&allocations==0&&retirements==0,message);
}
void run_case(const std::shared_ptr<const Cache> &cache,const LevelContent &content,
    unsigned representation,std::uint32_t weapon,bool dual) {
  const auto &biped=content.player_representations.at(representation).biped;
  const auto tag=cache->load_source_tag_metadata(biped.datum);const Bytes root(tag.bytes_at(tag.source_address(),0x304));
  const auto model=cache->load_source_tag_metadata(root.u32(0x38));const Bytes model_root(model.bytes_at(model.source_address(),0x94));
  const auto graph=model_root.u32(0x14);constexpr std::array<std::uint32_t,2> random{0x12345678,0x23456789};
  auto slot=std::make_unique<NativeLocomotion>(cache,graph,weapon,random,dual);
  // Normally advanced borrowed-cache witness: no checkpoint, rewind or replacement.
  NativeLocomotion witness(*cache,graph,weapon,random,dual);
  slot->bind_facing(biped.datum,0);witness.bind_facing(biped.datum,0);
  rejects([&]{(void)witness.checkpoint();},"Borrowed cache falsely certified an owning checkpoint");
  constexpr unsigned cut=47,suffix=420;
  for(unsigned tick=0;tick<=cut;++tick){check(advance(*slot,tick,biped.landing)==advance(witness,tick,biped.landing),"Initial authored event sequence differs");check(observe(*slot)==observe(witness),"Owning constructor changed original locomotion");}
  const auto captured=observe(*slot);const auto *captured_clip=slot->frame().clip;auto checkpoint=slot->checkpoint();
  check(checkpoint.valid()&&slot->owns_checkpoint(checkpoint),"Actual locomotion checkpoint unavailable");
  NativeLocomotion::Checkpoint empty;
  check(!empty.valid()&&!slot->owns_checkpoint(empty)&&!witness.owns_checkpoint(checkpoint),"Empty/foreign checkpoint was admitted");
  rejects([&]{(void)witness.prepare_restore(checkpoint);},"Foreign same-binding adapter accepted checkpoint");
  const auto ids=cache->load_weapon_animation_identifiers(weapon);
  const auto authored=cache->load_unit_weapon_action(graph,0x06000086,dual?0x0400054bu:ids[0],ids[1],0x0a000014);
  check(captured.samples.back().graph==authored.source_graph&&captured.samples.back().index==authored.source_index,
      "Checkpoint did not retain the actual authored moving gait");
  if(representation==3){check(authored.source_graph!=graph&&Bytes(slot->manager()).u16(4)!=0xffff,"Elite inherited packed binding was not exercised");}
  check(captured.samples.front()!=captured.samples.back()&&captured.facing.front()!=captured.facing.back(),
      "Checkpoint failed to exercise real origin/interpolated channel and facing values");
  for(unsigned tick=cut+1;tick<200;++tick)(void)advance(*slot,tick,biped.landing);
  slot->bind_facing(biped.datum,-2.1f);
  const auto changed=observe(*slot);check(changed!=captured,"Fixture did not change channel/facing history");
  {auto canceled=slot->prepare_restore(checkpoint);check(slot->restore_ready(canceled),"Fresh restore was not ready");}
  check(observe(*slot)==changed,"Cancel changed live locomotion");
  auto stale=slot->prepare_restore(checkpoint);(void)advance(*slot,200,biped.landing);const auto after_tick=observe(*slot);
  check(!slot->restore_ready(stale)&&!slot->commit_restore(stale)&&observe(*slot)==after_tick,"Stale preparation altered live locomotion");
  auto restored=slot->prepare_restore(checkpoint);
  bool thread_ok=false;std::thread wrong([&]{
    bool capture=false,prepare=false,replacement=false,observation=false;
    try{(void)slot->checkpoint();}catch(const FormatError&){capture=true;}
    try{(void)slot->prepare_restore(checkpoint);}catch(const FormatError&){prepare=true;}
    try{(void)NativeLocomotion::prepare_replacement(checkpoint,slot);}catch(const FormatError&){replacement=true;}
    try{(void)slot->frame();}catch(const FormatError&){observation=true;}
    thread_ok=capture&&prepare&&replacement&&observation&&!slot->owns_checkpoint(checkpoint)&&
        !slot->restore_ready(restored)&&!slot->commit_restore(restored);
  });wrong.join();check(thread_ok,"Wrong executor admitted locomotion continuation");
  const auto displaced_manager=slot->manager();const auto displaced_values=observe(*slot);
  storage_free_commit([&]{return slot->commit_restore(restored);},"Restore allocated, retired or failed during publication");
  check(std::equal(displaced_manager.begin(),displaced_manager.end(),displaced_values.manager.begin()),
      "Restore token failed to retain displaced manager bytes at their borrowed address");
  check(!slot->commit_restore(restored)&&!slot->restore_ready(restored),"Restore token was consumed more than once");
  check(observe(*slot)==captured&&slot->frame().clip==captured_clip,"Restore reconstructed or lost exact historical state/resource identity");
  std::vector<Observation> expected;std::vector<std::vector<Foot>> expected_events;expected.reserve(suffix);expected_events.reserve(suffix);
  unsigned foot_count=0;
  for(unsigned offset=0;offset<suffix;++offset){const auto tick=cut+1+offset;
    auto actual=advance(*slot,tick,biped.landing);auto reference=advance(witness,tick,biped.landing);
    check(actual==reference,"Restored RNG/clock emitted a different subsequent footstep sequence");foot_count+=unsigned(reference.size());
    expected.push_back(observe(witness));expected_events.push_back(std::move(reference));
    check(observe(*slot)==expected.back(),"Restored manager/sample/facing diverged from uninterrupted witness");
  }
  check(foot_count>4,"Continuation did not cross actual authored footstep and clip loop boundaries");
  // Prepare another values adapter against the same monotonic bank. The actual
  // active gait callback must not permit reentry through that sibling owner.
  std::unique_ptr<NativeLocomotion> sibling;
  auto sibling_prepare=NativeLocomotion::prepare_replacement(checkpoint,sibling);
  storage_free_commit([&]{return NativeLocomotion::commit_replacement(sibling_prepare,sibling);},"Private adapter publication allocated or retired");
  auto sibling_checkpoint=sibling->checkpoint();auto sibling_restore=sibling->prepare_restore(sibling_checkpoint);
  bool busy_checked=false;for(unsigned tick=0;tick<90&&!busy_checked;++tick){
    slot->tick(1,0,true,false,{2,0,0},[&](const NativeFootstepEvent &){
      if(busy_checked)return;busy_checked=true;
      check(slot->frame().clip&&std::isfinite(slot->facing()),"Own active event observation was rejected");
      rejects([&]{(void)slot->checkpoint();},"Reentrant capture accepted a partial tick");
      rejects([&]{(void)slot->prepare_restore(checkpoint);},"Reentrant prepare accepted a partial tick");
      rejects([&]{(void)slot->sample(.5f);},"Reentrant sampling entered an active resource bank");
      rejects([&]{slot->tick(0,0,true,false,{},{});},"Reentrant tick changed an active gait");
      rejects([&]{(void)sibling->checkpoint();},"Sibling capture entered a shared active bank");
      rejects([&]{sibling->tick(0,0,true,false,{},{});},"Sibling tick entered a shared active bank");
      rejects([&]{(void)sibling->frame();},"Sibling read entered a shared active bank");
      check(!sibling->restore_ready(sibling_restore)&&!sibling->commit_restore(sibling_restore),"Sibling publication entered a shared active bank");
      check(!NativeLocomotion::replacement_ready(sibling_prepare,sibling),"Consumed replacement became ready during callback");
    });
  }check(busy_checked,"Actual footstep callback busy guard was not exercised");
  sibling.reset();sibling_checkpoint={};
  // Original slot death is not a resource reset: checkpoint alone owns the
  // original callback bank, clips, metadata, cache and complete historical values.
  slot.reset();check(checkpoint.valid()&&captured_clip->source_graph==captured.samples.back().graph,
      "Historical checkpoint lost resource ownership after original slot destruction");
  std::unique_ptr<NativeLocomotion> other_slot;
  {auto canceled=NativeLocomotion::prepare_replacement(checkpoint,slot);
    check(NativeLocomotion::replacement_ready(canceled,slot)&&!NativeLocomotion::replacement_ready(canceled,other_slot),"Replacement lost exact slot-address admission");
    check(!NativeLocomotion::commit_replacement(canceled,other_slot),"Foreign destination slot accepted replacement");}
  check(!slot&&!other_slot,"Cancel published a historical slot");
  auto replacement=NativeLocomotion::prepare_replacement(checkpoint,slot);
  storage_free_commit([&]{return NativeLocomotion::commit_replacement(replacement,slot);},"Historical absent-slot publication allocated or retired");
  check(!NativeLocomotion::commit_replacement(replacement,slot)&&!slot->owns_checkpoint(checkpoint),"Replacement reused token or inherited original owner identity");
  check(observe(*slot)==captured&&slot->frame().clip==captured_clip,"Historical replacement ran initial selection or lost exact owned bank");
  for(unsigned offset=0;offset<suffix;++offset){check(advance(*slot,cut+1+offset,biped.landing)==expected_events[offset],"Destroyed-slot replacement changed uninterrupted footstep sequence");check(observe(*slot)==expected[offset],"Destroyed-slot replacement changed exact subsequent manager/sample/facing");}
  auto stale_slot=NativeLocomotion::prepare_replacement(checkpoint,slot);
  (void)advance(*slot,cut+suffix+1,biped.landing);const auto stale_value=observe(*slot);
  check(!NativeLocomotion::replacement_ready(stale_slot,slot)&&!NativeLocomotion::commit_replacement(stale_slot,slot)&&observe(*slot)==stale_value,"Destination revision changed after preparation but published");
  auto replaced_identity=NativeLocomotion::prepare_replacement(checkpoint,slot);
  slot.reset();check(!NativeLocomotion::replacement_ready(replaced_identity,slot)&&!NativeLocomotion::commit_replacement(replaced_identity,slot),"Retired destination identity remained ready");
  auto new_slot=NativeLocomotion::prepare_replacement(checkpoint,slot);
  storage_free_commit([&]{return NativeLocomotion::commit_replacement(new_slot,slot);},"New historical slot publication allocated or retired");
  for(unsigned i=0;i<13;++i)(void)advance(*slot,cut+1+i,biped.landing);
  const auto old_value=observe(*slot);const auto old_manager=slot->manager();const auto *old_clip=slot->frame().clip;
  auto retain_old=NativeLocomotion::prepare_replacement(checkpoint,slot);
  storage_free_commit([&]{return NativeLocomotion::commit_replacement(retain_old,slot);},"Replacement destroyed displaced borrowers during publication");
  check(std::equal(old_manager.begin(),old_manager.end(),old_value.manager.begin())&&old_clip->source_graph==old_value.samples.back().graph,
      "Consumed token failed to retain displaced manager/clip borrowers");
  check(observe(*slot)==captured,"Retained replacement changed historical values");
  std::cout<<"PASS exact authored locomotion continuation: representation="<<representation<<" dual="<<dual
      <<" suffix_ticks="<<suffix<<" footstep_events="<<foot_count<<" inherited="<<(authored.source_graph!=graph)<<'\n';
}
void cache_lifetime(const char *map,const char *shared,const std::shared_ptr<const OriginalData> &data,
    std::uint32_t graph,std::uint32_t weapon,std::uint32_t biped) {
  auto cache=std::make_shared<Cache>(map,shared,data);std::weak_ptr<const Cache> weak=cache;
  auto gait=std::make_unique<NativeLocomotion>(cache,graph,weapon,std::array<std::uint32_t,2>{0x12345678,0x23456789});
  gait->bind_facing(biped,.3f);for(unsigned i=0;i<17;++i)gait->tick(1,0,true,false,{2,0,0},{});
  auto checkpoint=gait->checkpoint();const auto captured=observe(*gait);const auto *clip=gait->frame().clip;
  gait.reset();cache.reset();check(!weak.expired()&&checkpoint.valid(),"Checkpoint did not retain the actual owning Cache");
  auto prepared=NativeLocomotion::prepare_replacement(checkpoint,gait);
  storage_free_commit([&]{return NativeLocomotion::commit_replacement(prepared,gait);},"Owning-cache replacement publication allocated or retired");
  checkpoint={};check(!weak.expired()&&observe(*gait)==captured&&gait->frame().clip==clip,"Replacement lost independently retained Cache/resources");
  gait.reset();check(weak.expired(),"Continuation resource callback bank leaked its owned Cache");
  std::cout<<"PASS locomotion resource lifetime: checkpoint survives adapter/cache release; final owner retires bank\n";
}
}
int main(int argc,char **argv){try{
  check(argc==4,"Supply MAP SHARED TU15_GAMEPLAY_XBE");
  if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[3]))return 77;
  const auto data=OriginalData::load_xbe(argv[3]);auto cache=std::make_shared<Cache>(argv[1],argv[2],data);
  const auto content=cache->load_content(0,true);const auto weapon=cache->slayer_starting_weapon(5);
  check(content.player_representations.size()>3&&content.weapons.at(weapon).dual_wieldable,"Actual Spartan/Elite/dual authored fixture resources missing");
  for(unsigned representation:{2u,3u})for(bool dual:{false,true})run_case(cache,content,representation,weapon,dual);
  const auto biped=content.player_representations[2].biped.datum;
  const auto b=cache->load_source_tag_metadata(biped);const Bytes br(b.bytes_at(b.source_address(),0x304));
  const auto m=cache->load_source_tag_metadata(br.u32(0x38));const Bytes mr(m.bytes_at(m.source_address(),0x94));
  cache_lifetime(argv[1],argv[2],data,mr.u32(0x14),weapon,biped);
  std::cout<<"PASS native locomotion exact owner/slot/executor/busy/cancel/stale/once continuation; no migration readiness\n";
}catch(const std::exception &error){watch_storage=false;std::cerr<<error.what()<<'\n';return 1;}}
