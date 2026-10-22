#include "gameplay/vehicles/native_vehicle_camera_continuation_data.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_player_departure.hpp"
#include <algorithm>

namespace halo2 {
namespace {
void quiescent(MultiplayerObjectRuntime &runtime){
  runtime.require_authority_executor(true);
  if(source_execution_inputs(runtime.memory()))
    throw FormatError("Camera continuation requires a completed native frame");
}
std::uint32_t source_tick(MultiplayerObjectRuntime &runtime){
  const auto &bank=runtime.memory();const auto clock=Bytes(bank.view(0x51df54u,4)).u32(0);
  if(!clock||clock>UINT32_MAX-12u)throw FormatError("Camera continuation lost its native clock");
  return Bytes(bank.view(clock+8u,4)).u32(0);
}
SourcePlayerIdentity identity(MultiplayerObjectRuntime &runtime,std::uint32_t player){
  const auto &bank=runtime.memory();
  const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return bank.view(at,n);}};
  const auto row=source_raw_datum_lookup(runtime.source_player_pool(),player,storage);
  if(!row)throw FormatError("Camera continuation lost its full native player allocation");
  SourcePlayerIdentity result{};
  bank.read(row+offsetof(SourceDeparturePlayer,identity),std::as_writable_bytes(std::span(result)));
  return result;
}
}
struct NativeVehicleCamera::Prepared::Data {
  const NativeVehicleCamera *owner{};
  Checkpoint incoming,before;
  std::array<std::uint64_t,2> mutations{};
};
std::uint64_t NativeVehicleCamera::checkpoint_allocation(const Checkpoint &saved){
  return saved.data_?sizeof(Checkpoint::Data):0;
}
NativeVehicleCamera::Checkpoint NativeVehicleCamera::rebind_checkpoint(const Checkpoint &saved,
    AuthorityBinding previous,AuthorityBinding next,std::span<const SourcePlayerAdmission> views){
  if(!saved.data_||saved.data_->binding!=previous||previous==next||views.empty()||views.size()>4||
      std::ranges::all_of(next,[](auto b){return b==std::byte{};}))
    throw FormatError("Camera role transition has a different binding or view roster");
  std::uint16_t seen{};
  for(unsigned i=0;i<4;++i){const auto &v=saved.data_->views[i];
    if(i<views.size()){
      const auto &p=views[i];
      if(p.slot>=16||(seen&(1u<<p.slot))||p.player==null_datum||(p.player&65535u)!=p.slot||
          (v.player!=null_datum&&(v.player!=p.player||saved.data_->identities[i]!=p.identity)))
        throw FormatError("Camera role transition changed an independently admitted view");
      seen|=std::uint16_t(1u<<p.slot);
    }else if(v.player!=null_datum)throw FormatError("Camera role transition omitted an existing native view");
  }
  Checkpoint out;auto data=std::make_shared<Checkpoint::Data>(*saved.data_);data->binding=next;out.data_=std::move(data);return out;
}
NativeVehicleCamera::Prepared::Prepared()=default;
NativeVehicleCamera::Prepared::~Prepared()=default;
NativeVehicleCamera::Prepared::Prepared(Prepared &&)noexcept=default;
NativeVehicleCamera::Prepared &NativeVehicleCamera::Prepared::operator=(Prepared &&)noexcept=default;
NativeVehicleCamera::Checkpoint NativeVehicleCamera::checkpoint(AuthorityBinding binding)const {
  quiescent(runtime_);
  if(std::ranges::all_of(binding,[](auto b){return b==std::byte{};}))
    throw FormatError("Camera continuation requires its admitted world binding");
  auto data=std::make_shared<Checkpoint::Data>();data->binding=binding;data->tick=source_tick(runtime_);
  data->views=views_;data->clusters=saved_clusters_;data->random=random_;
  for(unsigned view=0;view<views_.size();++view)if(views_[view].player!=0xffffffffu)
    data->identities[view]=identity(runtime_,views_[view].player);
  Checkpoint result;result.data_=std::move(data);return result;
}
NativeVehicleCamera::Prepared NativeVehicleCamera::prepare(const Checkpoint &saved,AuthorityBinding binding)const {
  quiescent(runtime_);
  if(!saved.data_||saved.data_->binding!=binding||saved.data_->tick!=source_tick(runtime_))
    throw FormatError("Camera continuation has a different binding or completed native tick");
  for(unsigned view=0;view<saved.data_->views.size();++view){const auto player=saved.data_->views[view].player;
    if(player!=0xffffffffu&&saved.data_->identities[view]!=identity(runtime_,player))
      throw FormatError("Camera continuation has a different full native player identity");}
  Prepared result;result.data_=std::make_unique<Prepared::Data>();auto &out=*result.data_;
  out.owner=this;out.incoming=saved;out.before=checkpoint(binding);out.mutations=runtime_.execution_mutations();return result;
}
bool NativeVehicleCamera::commit(Prepared &&prepared)noexcept {
  if(!prepared.data_||prepared.data_->owner!=this||!runtime_.authority_executor_thread())return false;
  const auto &saved=*prepared.data_;
  try{quiescent(runtime_);if(saved.mutations!=runtime_.execution_mutations()||!saved.before.data_->matches(*this))return false;}
  catch(...){return false;}
  const auto &incoming=*saved.incoming.data_;
  views_=incoming.views;saved_clusters_=incoming.clusters;random_=incoming.random;
  prepared.data_.reset();return true;
}
}
