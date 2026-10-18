#pragma once
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_player_departure.hpp"
#include <atomic>
#include <bit>
#include <fstream>
#include <thread>
#include <openssl/sha.h>

namespace halo2::test {
// Cut comparison must not include fresh wall/CPU/UTC samples taken between
// the last replayed call and checkpoint capture. Native game ticks and the
// complete external-input/return transcript still execute and validate.
class CameraContinuationClock final:public PhysicsLiveClockSource {
public:
  std::uint32_t time=1000;
  std::uint64_t counter=7000000,filetime=132000000000000000ull;
  std::uint32_t milliseconds()const noexcept override{return time;}
  std::uint64_t cycles()const noexcept override{return counter;}
  std::uint64_t utc_filetime()const noexcept override{return filetime;}
};
inline void verify_native_vehicle_camera_continuation(MultiplayerPreparedWorld &world,NativeVehicleCamera &camera,
    std::array<Controls,multiplayer_capacity> controls,const MultiplayerOptions &options){
  const auto need=[](bool yes,const char *why){if(!yes)throw FormatError(why);};
  const auto rejects=[&](auto execute){bool rejected=false;try{execute();}catch(const std::exception &){rejected=true;}
    need(rejected,"Invalid camera continuation was accepted");};
  auto &objects=*world.source_objects;SourceMatchAuthority owner(world.match,objects);
  NativeVehicleCamera::AuthorityBinding binding{};binding.front()=std::byte{0x51};
  const auto admissions=owner.player_admissions(world.local_slots);
  const auto saved=owner.checkpoint(binding);const auto saved_camera=camera.checkpoint(binding);
  const auto camera_bytes=NativeVehicleCamera::encode_checkpoint(saved_camera);
  const auto decoded_camera=NativeVehicleCamera::decode_checkpoint(camera_bytes,binding,std::uint32_t(saved.match.simulation.ticks),admissions);
  need(NativeVehicleCamera::encode_checkpoint(decoded_camera)==camera_bytes,"Private camera codec is not canonical");
  for(unsigned n=0;n<camera_bytes.size();++n)rejects([&]{
    NativeVehicleCamera::decode_checkpoint(std::span(camera_bytes).first(n),binding,std::uint32_t(saved.match.simulation.ticks),admissions);});
  auto changed_bytes=camera_bytes;changed_bytes.back()^=std::byte{1};
  rejects([&]{NativeVehicleCamera::decode_checkpoint(changed_bytes,binding,std::uint32_t(saved.match.simulation.ticks),admissions);});
  changed_bytes=camera_bytes;changed_bytes.at(44+32+4+4)=std::byte{2};
  std::array<std::byte,32> byte_digest{};const auto payload=std::span<const std::byte>(changed_bytes).subspan(44);
  need(SHA256(reinterpret_cast<const unsigned char *>(payload.data()),payload.size(),reinterpret_cast<unsigned char *>(byte_digest.data())),"Cannot hash malformed camera fixture");
  std::copy(byte_digest.begin(),byte_digest.end(),changed_bytes.begin()+12);
  rejects([&]{NativeVehicleCamera::decode_checkpoint(changed_bytes,binding,std::uint32_t(saved.match.simulation.ticks),admissions);});
  auto wrong_views=admissions;wrong_views.front().identity.front()^=0x80;
  rejects([&]{NativeVehicleCamera::decode_checkpoint(camera_bytes,binding,std::uint32_t(saved.match.simulation.ticks),wrong_views);});
  const auto restore_world=[&]{auto next=owner.prepare(saved,binding,admissions);
    need(owner.commit(std::move(next)),"Camera fixture could not restore its exact coupled world");};
  using Views=std::array<std::optional<NativeDeathCameraView>,4>;
  const auto frame=[&](NativeVehicleCamera &target,unsigned phase){
    world.match.step(controls);const auto state=world.match.state();Views result{};
    for(unsigned view=0;view<world.local_slots.size();++view){const auto &p=state.players[world.local_slots[view]];
      result[view]=target.update_player(view,p.source_player,p.source_flags,tick_seconds,
          view==0&&phase%4==0&&target.death_view(view));}
    return result;
  };
  std::vector<SourceExecutionPhase> phases;std::vector<Views> expected;
  for(unsigned i=0;i<12;++i)phases.push_back(owner.capture_execution_phase([&]{expected.push_back(frame(camera,i));}));
  const auto future_camera=camera.checkpoint(binding);
  const auto expected_world=encode_source_authority(owner.checkpoint(binding));restore_world();
  const auto detached=[&]{NativeVehicleCamera temporary(objects,*world.cache->original_data());
    auto next=temporary.prepare(decoded_camera,binding);need(temporary.commit(std::move(next)),"Temporary camera import failed");
    return temporary.checkpoint(binding);}();
  NativeVehicleCamera recipient(objects,*world.cache->original_data());
  rejects([&]{recipient.prepare({},binding);});auto wrong=binding;wrong.back()=std::byte{1};
  rejects([&]{recipient.prepare(saved_camera,wrong);});rejects([&]{recipient.checkpoint({});});
  rejects([&]{recipient.prepare(future_camera,binding);});
  std::atomic<bool> wrong_thread=false;
  {std::jthread other([&]{try{recipient.checkpoint(binding);}catch(const std::exception &){wrong_thread=true;}});}
  need(wrong_thread,"Camera continuation ignored native executor ownership");
  auto prepared=recipient.prepare(saved_camera,binding);
  need(!camera.commit(std::move(prepared)),"Camera continuation committed to another owner");
  need(recipient.commit(std::move(prepared)),"Exact camera continuation failed to commit");
  {auto cancelled=recipient.prepare(saved_camera,binding);}
  auto changed_world=recipient.prepare(saved_camera,binding);world.match.step(controls);
  need(!recipient.commit(std::move(changed_world)),"Advanced native world accepted an old camera preparation");restore_world();
  auto changed_camera=recipient.prepare(saved_camera,binding);const auto stamp=objects.execution_mutations();
  recipient.update_player(0,null_datum,0,0);
  need(objects.execution_mutations()==stamp,"Empty camera-view reset unexpectedly changed native state");
  need(!recipient.commit(std::move(changed_camera)),"Changed private observer accepted an old preparation");
  const auto &bank=objects.memory();
  const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return bank.view(at,n);}};
  const auto row=source_raw_datum_lookup(objects.source_player_pool(),admissions.front().player,storage);
  const auto address=row+offsetof(SourceDeparturePlayer,identity);
  std::vector<std::byte> altered(bank.view(address,sizeof(SourcePlayerIdentity)).begin(),bank.view(address,sizeof(SourcePlayerIdentity)).end());
  altered.front()^=std::byte{0x80};objects.memory().write(address,altered);
  rejects([&]{recipient.prepare(saved_camera,binding);});restore_world();
  auto final=recipient.prepare(detached,binding);
  need(recipient.commit(std::move(final)),"Admitted camera failed after complete world repair");
  auto within_frame=recipient.prepare(saved_camera,binding);
  owner.capture_execution_phase([&]{
    rejects([&]{recipient.checkpoint(binding);});rejects([&]{recipient.prepare(saved_camera,binding);});
    need(!recipient.commit(std::move(within_frame)),"Camera continuation published inside an active source frame");});
  const auto equal=[](const auto &a,const auto &b){
    if(a.has_value()!=b.has_value())return false;if(!a)return true;
    const auto words=[](const NativeDeathCameraView &v){return std::array{
      std::bit_cast<std::uint32_t>(v.position.x),std::bit_cast<std::uint32_t>(v.position.y),std::bit_cast<std::uint32_t>(v.position.z),
      std::bit_cast<std::uint32_t>(v.forward.x),std::bit_cast<std::uint32_t>(v.forward.y),std::bit_cast<std::uint32_t>(v.forward.z),
      std::bit_cast<std::uint32_t>(v.up.x),std::bit_cast<std::uint32_t>(v.up.y),std::bit_cast<std::uint32_t>(v.up.z),
      std::bit_cast<std::uint32_t>(v.horizontal_fov),std::bit_cast<std::uint32_t>(v.aim_displacement),std::uint32_t(v.camera_mode)};};
    return words(*a)==words(*b);
  };
  for(unsigned i=0;i<phases.size();++i){Views actual;
    owner.replay_execution_phase(phases[i],[&]{actual=frame(recipient,i);});
    for(unsigned view=0;view<actual.size();++view)need(equal(actual[view],expected[i][view]),"Restored camera changed an actual observer result");}
  const auto actual_checkpoint=owner.checkpoint(binding);const auto actual_world=encode_source_authority(actual_checkpoint);
  if(actual_world!=expected_world)if(const auto *output=std::getenv("HALO2_CAMERA_CONTINUATION_OUTPUT")){
    std::filesystem::create_directories(output);
    const auto write=[&](const char *name,const auto &bytes){std::ofstream file(std::filesystem::path(output)/name,std::ios::binary);
      file.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());need(bool(file),"Cannot retain camera continuation diagnostic");};
    write("expected.bin",expected_world);write("actual.bin",actual_world);
  }
  need(actual_world==expected_world,"Camera continuation changed the complete replayed native world");
  std::exception_ptr cross_executor_error;
  {std::jthread executor([&]{try{
    auto clock=std::make_shared<CameraContinuationClock>();clock->time=900000;
    clock->counter=17000000000ull;clock->filetime+=70000000;
    MultiplayerPreparedWorld target(options,std::nullopt,clock);target.bind_source_local_views(target.local_slots);
    SourceMatchAuthority imported(target.match,*target.source_objects);
    auto native=imported.prepare(saved,binding,admissions);
    need(imported.commit(std::move(native)),"Separate camera executor could not import the complete native world");
    NativeVehicleCamera target_camera(*target.source_objects,*target.cache->original_data());
    auto next=target_camera.prepare(detached,binding);
    need(target_camera.commit(std::move(next)),"Owning camera checkpoint did not rebind to a separate native executor");
    for(unsigned i=0;i<phases.size();++i){Views actual;
      imported.replay_execution_phase(phases[i],[&]{
        target.match.step(controls);const auto state=target.match.state();
        for(unsigned view=0;view<target.local_slots.size();++view){const auto &p=state.players[target.local_slots[view]];
          actual[view]=target_camera.update_player(view,p.source_player,p.source_flags,tick_seconds,
              view==0&&i%4==0&&target_camera.death_view(view));}
      });
      for(unsigned view=0;view<actual.size();++view)need(equal(actual[view],expected[i][view]),"Separate camera executor changed an actual observer result");
    }
    need(encode_source_authority(imported.checkpoint(binding))==expected_world,"Separate camera executor changed the complete native world");
  }catch(...){cross_executor_error=std::current_exception();}});}
  if(cross_executor_error)std::rethrow_exception(cross_executor_error);
}
}
