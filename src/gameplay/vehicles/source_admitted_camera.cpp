#include "halo2/source_admitted_camera.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_player_departure.hpp"
#include "halo2/source_execution_calls.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
namespace {
thread_local SourceAdmittedCameraBank *bound_bank{};
void need(bool value,const char *why){if(!value)throw FormatError(why);}
MultiplayerObjectRuntime &initial_world(MultiplayerMatch &match,MultiplayerObjectRuntime &world){
  world.require_authority_executor(true);
  need(match.source_object_runtime()==&world&&match.simulation().ticks()==0,
      "Fresh admitted camera requires its actual source tick0 world");
  return world;
}
}
SourceAdmittedCamera::SourceAdmittedCamera(MultiplayerMatch &match,MultiplayerObjectRuntime &world,
    const OriginalData &data,const AcceptedSourcePlayers &accepted,std::span<const OnlineAdmission> roster,
    unsigned physical_owner)
    :match_(match),world_(world),context_(accepted.context),owner_(physical_owner),
     seed_(initial_world(match,world).tu15_world().original_camera_seed()),camera_(world,data) {
  need(seed_.available(),"Admitted camera lost its locally produced original director seed");
  need(owner_<16&&context_.host<16&&(context_.peers&(1u<<context_.host))&&(context_.peers&(1u<<owner_))&&
      std::ranges::any_of(context_.session,[](auto value){return value!=std::byte{};})&&
      !roster.empty()&&roster.size()<=16&&roster.size()==accepted.players.size(),
      "Admitted camera has another authority context or roster");
  std::uint16_t slots{},physical{};
  for(unsigned i=0;i<roster.size();++i) {
    const auto &p=roster[i];const auto peer=online_admission_peer(p);
    need(p.slot==i&&peer<roster.size()&&online_admission_peer(roster[peer])==peer&&
        !roster[peer].guest&&accepted.players[i].slot==i&&!(slots&(1u<<i)),
        "Admitted camera lost its independent physical-seat mapping");
    slots|=std::uint16_t(1u<<i);physical|=std::uint16_t(1u<<peer);
    (void)flags(accepted.players[i]);
    if(peer==owner_)views_.push_back(accepted.players[i]);
  }
  need(physical==context_.peers&&!views_.empty()&&views_.size()<=4&&views_.front().slot==owner_,
      "Admitted camera does not own the exact accepted couch");
}
std::uint16_t SourceAdmittedCamera::flags(const SourcePlayerAdmission &p)const {
  need(world_.authority_executor_thread()&&p.slot<16&&p.player!=null_datum&&
      match_.player(p.slot).connected&&match_.player(p.slot).source_datum==p.player,
      "Admitted camera player allocation changed");
  const auto &bank=world_.memory();
  const SourceRawDatumPoolServices storage{.read=[&](auto address,auto bytes){return bank.view(address,bytes);}};
  const auto row=source_raw_datum_lookup(world_.source_player_pool(),p.player,storage);
  need(row!=0,"Admitted camera lost its full original player identity");
  SourcePlayerIdentity identity{};
  bank.read(row+offsetof(SourceDeparturePlayer,identity),std::as_writable_bytes(std::span(identity)));
  need(identity==p.identity,"Admitted camera has another original player identity");
  std::uint16_t result{};
  bank.read(row+offsetof(SourceDeparturePlayer,flags),std::as_writable_bytes(std::span(&result,1)));
  return result;
}
std::optional<NativeDeathCameraView> SourceAdmittedCamera::update(const AuthorityTransferContext &context,
    unsigned view,float seconds,bool cycle,std::optional<NativeDeathCameraView> *completed) {
  need(context==context_&&view<views_.size()&&std::isfinite(seconds)&&seconds>=0,
      "Numeric camera update has another context, view or elapsed input");
  const auto &p=views_[view];
  return camera_.update_player_impl(view,p.player,flags(p),seconds,cycle,&seed_,completed);
}
NativeVehicleCamera::Checkpoint SourceAdmittedCamera::checkpoint()const {
  for(const auto &p:views_)(void)flags(p);
  return camera_.checkpoint(source_authority_binding(context_));
}
SourceAdmittedCameraBank::SourceAdmittedCameraBank(MultiplayerMatch &match,MultiplayerObjectRuntime &world,
    const OriginalData &data,const AcceptedSourcePlayers &accepted,std::span<const OnlineAdmission> roster)
    :world_(world),context_(accepted.context) {
  need(context_.host<16&&(context_.peers&(1u<<context_.host)),
      "Admitted camera bank has no actual hosting physical owner");
  for(unsigned owner=0;owner<16;++owner)if(context_.peers&(1u<<owner))
    cameras_[owner]=std::make_unique<SourceAdmittedCamera>(match,world,data,accepted,roster,owner);
}
void SourceAdmittedCameraBank::own()const {
  need(executor_==std::this_thread::get_id()&&world_.authority_executor_thread(),
      "Admitted camera bank moved to another native executor");
}
void SourceAdmittedCameraBank::activate() {
  own();world_.require_authority_executor(true);
  need(!active_&&cameras_[context_.host]&&cameras_[context_.host]->match_.simulation().ticks()==0,
      "Numeric camera replay must start from its genuine untouched tick0 base");active_=true;
}
void SourceAdmittedCameraBank::validate(unsigned owner,unsigned view,float seconds)const {
  own();need(active_&&owner<16&&cameras_[owner]&&view<cameras_[owner]->views().size()&&
      std::isfinite(seconds)&&seconds>=0,"Numeric camera has no preinstalled admitted owner or view");
  (void)cameras_[owner]->flags(cameras_[owner]->views()[view]);
}
std::optional<NativeDeathCameraView> SourceAdmittedCameraBank::update(unsigned owner,unsigned view,float seconds,bool cycle,
    std::optional<NativeDeathCameraView> *completed) {
  validate(owner,view,seconds);
  SourceExecutionCallScope call(&world_.memory(),SourceExecutionCallKind::AdmittedCamera,[&]{
    return source_execution_arguments(SourceAdmittedCameraInput{std::uint8_t(owner),std::uint8_t(view),seconds,cycle});});
  auto &camera=*cameras_[owner];const auto result=camera.update(context_,view,seconds,cycle,completed);
  call.complete([&]{const auto &state=camera.camera_.views_[view];return source_execution_admitted_camera_result(result,
      camera.death_view(view),state.controller,state.observer,camera.camera_.random_,camera.camera_.saved_clusters_,
      state.player,state.seated,state.handing_off);});
  return result;
}
bool SourceAdmittedCameraBank::death_view(unsigned owner,unsigned view)const {
  validate(owner,view,0);return cameras_[owner]->death_view(view);
}
NativeVehicleCamera::Checkpoint SourceAdmittedCameraBank::checkpoint(unsigned owner)const {
  own();need(owner<16&&cameras_[owner],"Camera checkpoint has no admitted physical owner");
  return cameras_[owner]->checkpoint();
}
std::span<const SourcePlayerAdmission> SourceAdmittedCameraBank::views(unsigned owner)const {
  own();need(owner<16&&cameras_[owner],"Camera view roster has no admitted physical owner");return cameras_[owner]->views();
}
SourceAdmittedCameraBank::Binding::Binding(SourceAdmittedCameraBank &bank):bank_(&bank) {
  bank.own();need(bank.active_&&!bound_bank,"Numeric camera replay already has an executor binding");bound_bank=&bank;
}
SourceAdmittedCameraBank::Binding::~Binding() {
  if(bank_->executor_!=std::this_thread::get_id()||bound_bank!=bank_)std::terminate();bound_bank=nullptr;
}
SourceAdmittedCameraBank &SourceAdmittedCameraBank::bound(MultiplayerObjectRuntime &world) {
  need(bound_bank&&&bound_bank->world_==&world,"Numeric camera replay has no locally admitted executor owner");
  bound_bank->own();return *bound_bank;
}
}
