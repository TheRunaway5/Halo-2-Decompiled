#include "halo2/source_admitted_camera.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/source_replay_batch.hpp"
#include "helpers/native_vehicle_camera_continuation.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <limits>
#include <iostream>
using namespace halo2;
namespace {
nlohmann::json result={{"passed",false},{"stage","bootstrap"}};
void need(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void rejects(F &&f){bool rejected{};try{f();}catch(const std::exception &){rejected=true;}
  need(rejected,"Unadmitted numeric camera input was accepted");}
std::string hash(std::span<const std::byte> bytes){std::array<unsigned char,32> digest{};
  need(SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),digest.data()),"Camera oracle hash failed");
  constexpr char hex[]="0123456789abcdef";std::string out;for(auto value:digest){out+=hex[value>>4];out+=hex[value&15];}return out;}
void save_report(){
  const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Numeric camera report requires owned scratch");
  const auto bytes=result.dump(2)+"\n";need(bytes.size()<=1024*1024,"Numeric camera report exceeds its bounded receipt");
  const auto write=[&](const std::filesystem::path &path){std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);out<<bytes;need(bool(out),"Could not retain numeric-camera source oracle receipt");};
  write(std::filesystem::path(scratch)/"source-admitted-camera.json");
  if(const auto destination=std::getenv("H2_SOURCE_ADMITTED_CAMERA_REPORT")){
    const std::filesystem::path path=destination;need(path.is_absolute(),"Explicit numeric camera receipt path must be absolute");write(path);
  }
}
}
int main(int argc,char **argv)try {
  need(argc==2,"Supply admitted MCC map");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Numeric camera oracle requires owned scratch");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=options.map.parent_path();options.source_object_world=true;
  result["stage"]="construct-source-tick0";
  options.local_players=4;options.variant={"Slayer",0,{}};options.variant.values.set(10,0);options.variant.values.set(2,0);
  options.variant.values.set(89,5);
  options.source_object_savegame_directory=std::filesystem::path(scratch)/"admitted-camera";
  std::filesystem::create_directories(options.source_object_savegame_directory);
  // Own the actual native map/Match constructors directly. The offline
  // prepared-world convenience constructor adds a genuine birth tick; its
  // deferral option requires a separate complete pre-join FFA policy. Neither
  // that policy nor a relabeled/restored clock belongs in this camera proof.
  auto clock=std::make_shared<test::CameraContinuationClock>();
  const auto cache=std::make_shared<const Cache>(options.map,options.shared_maps);
  auto content=cache->load_content(0,true,source_game_variant(options.variant));
  auto objects=std::make_unique<MultiplayerObjectRuntime>(*cache,0,options.source_object_savegame_directory,clock);
  auto rules=multiplayer_source_rules(options.variant,true);rules.weapon=null_datum;
  objects->configure_session(source_game_variant(options.variant),1,30,rules.random_seed);
  auto match=std::make_unique<MultiplayerMatch>(content,0,cache->spawns(),rules,MultiplayerKingServices{},
      [clock]{return std::uint32_t(clock->utc_filetime()/10000000ull);},objects.get());
  match->configure_weapon_pickups();
  if(cache->original_data()->melee_constants)match->configure_melee_animation(cache,*cache->original_data()->melee_constants);
  if(cache->original_data()->grenade_constants)match->configure_grenades(cache,*cache->original_data()->grenade_constants);
  const auto fp=cache->load_first_person_binding(cache->slayer_starting_weapon(5),2);
  match->configure_replicated_first_person(fp);
  std::vector<unsigned> locals;
  for(unsigned slot=0;slot<4;++slot){need(match->join("Player "+std::to_string(slot+1))==slot,
      "Original camera source player allocation changed its actual slot");locals.push_back(slot);}
  match->configure_local_views(locals,fp);
  struct WorldView {std::shared_ptr<const Cache> cache;std::unique_ptr<MultiplayerObjectRuntime> &source_objects;
      MultiplayerMatch &match;std::vector<unsigned> local_slots;};
  WorldView world{cache,objects,*match,std::move(locals)};
  need(world.cache->is_mcc()&&world.match.source_engine_owned()&&world.local_slots.size()==4,
      "Numeric camera oracle lacks its actual four-seat MCC source world");
  need(world.match.simulation().ticks()==0,"Numeric camera seed was already advanced by an actual source birth tick");
  SourceMatchAuthority authority(world.match,*world.source_objects);
  AuthorityTransferContext context{};context.session[0]=std::byte{0x71};context.peers=1;context.host=0;
  const auto binding=source_authority_binding(context);
  AcceptedSourcePlayers accepted;accepted.context=context;accepted.players=authority.player_admissions(world.local_slots);
  std::vector<OnlineAdmission> roster;
  for(unsigned i=0;i<4;++i){OnlineAdmission p;p.slot=i;p.peer=0;p.guest=i>0;p.name=world.match.player(i).name;roster.push_back(p);}
  auto bad=accepted;bad.players[0].identity[0]^=0x80;
  rejects([&]{SourceAdmittedCamera camera(world.match,*world.source_objects,*world.cache->original_data(),bad,roster,0);});
  auto bad_roster=roster;bad_roster[3].peer=3;
  rejects([&]{SourceAdmittedCamera camera(world.match,*world.source_objects,*world.cache->original_data(),accepted,bad_roster,0);});
  for(const auto invalid:std::array<AuthorityTransferContext,3>{AuthorityTransferContext{},
      AuthorityTransferContext{context.session,0,0,0,0},AuthorityTransferContext{context.session,0,0,1,16}}){
    auto missing_host=accepted;missing_host.context=invalid;
    rejects([&]{SourceAdmittedCameraBank camera(world.match,*world.source_objects,*world.cache->original_data(),missing_host,roster);});
  }
  const auto seed=authority.checkpoint(binding);
  const auto locally_initialized=world.source_objects->tu15_world().original_camera_seed();
  need(locally_initialized.available(),"Original local157EB0 constructor did not publish its opaque seed");
  result["stage"]="capture-uninterrupted-original-camera";
  NativeVehicleCamera original(*world.source_objects,*world.cache->original_data());
  std::array<Controls,multiplayer_capacity> controls{};
  std::vector<SourceExecutionPhase> phases;
  std::vector<std::vector<std::byte>> cameras;
  const auto update_original=[&](unsigned frame){
    if(frame)world.match.step(controls);
    const auto state=world.match.state();
    for(unsigned view=0;view<4;++view){const auto &p=state.players[view];
      (void)original.update_player(view,p.source_player,p.source_flags,tick_seconds,view==0&&frame%4==0);}
  };
  // The old caller resolves its default inside the lower wrapper; the typed
  // owner resolves the same original default from its local opaque seed. Prove
  // their first original execution/world/camera output exactly before replaying
  // subsequent calls whose complete argument/return transcripts are identical.
  update_original(0);
  cameras.push_back(NativeVehicleCamera::encode_checkpoint(original.checkpoint(binding)));
  const auto expected_initial=encode_source_authority(authority.checkpoint(binding));
  for(unsigned frame=1;frame<=12;++frame){
    phases.push_back(authority.capture_execution_phase([&]{update_original(frame);}));
    cameras.push_back(NativeVehicleCamera::encode_checkpoint(original.checkpoint(binding)));
  }
  const auto expected=encode_source_authority(authority.checkpoint(binding));
  result["stage"]="restore-native-tick0-and-reconstruct-camera";
  auto prepared=authority.prepare(seed,binding,accepted.players);need(authority.commit(std::move(prepared)),"Could not restore source tick0 seed");
  need(world.source_objects->tu15_world().original_camera_seed()==locally_initialized,
      "Imported world replaced the locally initialized original camera seed");
  SourceAdmittedCamera numeric(world.match,*world.source_objects,*world.cache->original_data(),accepted,roster,0);
  auto wrong=context;wrong.session[1]=std::byte{1};
  const auto before=authority.execution_boundary();
  rejects([&]{numeric.update(wrong,0,tick_seconds,false);});rejects([&]{numeric.update(context,4,tick_seconds,false);});
  for(float seconds:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
    rejects([&]{numeric.update(context,0,seconds,false);});
  need(authority.execution_boundary()==before,"Rejected numeric camera input mutated its native world");
  (void)world.match.state();
  for(unsigned view=0;view<4;++view)(void)numeric.update(context,view,tick_seconds,view==0);
  need(NativeVehicleCamera::encode_checkpoint(numeric.checkpoint())==cameras[0]&&
      encode_source_authority(authority.checkpoint(binding))==expected_initial,
      "Local157EB0 seed changed the first complete actual camera/world result");
  for(unsigned index=0;index<phases.size();++index){
    const auto frame=index+1;
    result["stage"]="numeric-replay-"+std::to_string(frame);
    authority.replay_execution_phase(phases[index],[&]{
      world.match.step(controls);
      (void)world.match.state(); // Same actual original publication call order.
      for(unsigned view=0;view<4;++view)(void)numeric.update(context,view,tick_seconds,view==0&&frame%4==0);
    });
    const auto actual_camera=NativeVehicleCamera::encode_checkpoint(numeric.checkpoint());
    result["camera_phase"]=frame;result["camera_expected_sha256"]=hash(cameras[frame]);result["camera_actual_sha256"]=hash(actual_camera);
    if(actual_camera!=cameras[frame])result["camera_first_difference"]=std::mismatch(actual_camera.begin(),actual_camera.end(),cameras[frame].begin(),cameras[frame].end()).first-actual_camera.begin();
    need(actual_camera==cameras[frame],
        "Locally reconstructed admitted camera differs from uninterrupted original owner");
  }
  const auto actual=encode_source_authority(authority.checkpoint(binding));
  result["stage"]="compare-complete-native-world";result["world_expected_sha256"]=hash(expected);result["world_actual_sha256"]=hash(actual);
  need(actual==expected,"Numeric camera reconstruction changed the complete actual post-step source world");
  rejects([&]{SourceAdmittedCamera camera(world.match,*world.source_objects,*world.cache->original_data(),accepted,roster,0);});
  result["stage"]="forged-imported-controller-cannot-reseed-camera";
  auto forged=seed;bool altered{};constexpr std::uint32_t callback=0x4f5514u+0xcu;
  for(auto &region:forged.world.world.bank.regions)if(callback>=region.address&&std::uint64_t(callback)+4<=std::uint64_t(region.address)+region.bytes.size()){
    const auto offset=callback-region.address;constexpr std::uint32_t unowned=0xf000deadu;
    for(unsigned i=0;i<4;++i)region.bytes[offset+i]=std::byte(unowned>>(i*8));altered=true;break;
  }
  need(altered,"Forged-default control has no actual original director record");
  bool imported{};
  try{auto incoming=authority.prepare(forged,binding,accepted.players);imported=authority.commit(std::move(incoming));}
  catch(const std::exception &){/* A stricter original-world admission also fails closed. */}
  need(world.source_objects->tu15_world().original_camera_seed()==locally_initialized,
      "Incoming controller words overwrote the locally generated opaque seed");
  if(imported){
    SourceAdmittedCamera isolated(world.match,*world.source_objects,*world.cache->original_data(),accepted,roster,0);
    (void)world.match.state();
    for(unsigned view=0;view<4;++view)(void)isolated.update(context,view,tick_seconds,view==0);
    need(NativeVehicleCamera::encode_checkpoint(isolated.checkpoint())==cameras[0],
        "Numeric camera selected a dispatcher from imported controller words");
  }
  result["stage"]="capture-numeric-journal-from-untouched-base";
  auto restore=authority.prepare(seed,binding,accepted.players);
  need(authority.commit(std::move(restore)),"Numeric journal could not restore its actual native tick0 base");
  SourceAdmittedCameraBank producer(world.match,*world.source_objects,*world.cache->original_data(),accepted,roster);
  rejects([&]{producer.update(0,0,tick_seconds,false);});producer.activate();
  std::vector<SourceExecutionPhase> typed;
  std::vector<std::vector<std::byte>> typed_cameras;
  for(unsigned frame=0;frame<=12;++frame){
    typed.push_back(authority.capture_execution_phase([&]{
      if(frame)world.match.step(controls);(void)world.match.state();
      for(unsigned view=0;view<4;++view)(void)producer.update(0,view,tick_seconds,view==0&&frame%4==0);
    }));
    typed_cameras.push_back(NativeVehicleCamera::encode_checkpoint(producer.checkpoint(0)));
    need(typed_cameras.back()==cameras[frame],"Numeric journal changed actual original camera output");
    need(std::ranges::count(typed.back().calls,SourceExecutionCallKind::AdmittedCamera,&SourceExecutionCall::kind)==4&&
        std::ranges::none_of(typed.back().calls,[](const auto &call){return call.kind==SourceExecutionCallKind::PlayerCamera;}),
        "Numeric camera leaked the raw controller/observer operation into its journal");
    validate_source_network_replay_calls(typed.back().calls);
  }
  need(encode_source_authority(authority.checkpoint(binding))==expected,
      "Numeric journal producer differs from the complete uninterrupted native world");
  restore=authority.prepare(seed,binding,accepted.players);
  need(authority.commit(std::move(restore)),"Numeric recipient could not restore its true native tick0 base");
  SourceAdmittedCameraBank recipient(world.match,*world.source_objects,*world.cache->original_data(),accepted,roster);recipient.activate();
  const auto fresh_boundary=authority.execution_boundary();
  rejects([&]{replay_source_execution_calls(world.match,*world.source_objects,typed[1].calls);});
  need(authority.execution_boundary()==fresh_boundary,"Missing numeric camera binding mutated the native world");
  auto bad_owner=typed[1];
  auto selected=std::ranges::find(bad_owner.calls,SourceExecutionCallKind::AdmittedCamera,&SourceExecutionCall::kind);
  need(selected!=bad_owner.calls.end(),"Numeric journal has no actual admitted camera call");
  selected->arguments=source_execution_arguments(SourceAdmittedCameraInput{1,0,tick_seconds,false});
  {SourceAdmittedCameraBank::Binding installed(recipient);
    rejects([&]{replay_source_execution_calls(world.match,*world.source_objects,bad_owner.calls);});}
  need(authority.execution_boundary()==fresh_boundary,"Unadmitted physical camera owner mutated the native world");
  for(unsigned frame=0;frame<typed.size();++frame){
    result["stage"]="numeric-generic-replay-"+std::to_string(frame);
    {SourceAdmittedCameraBank::Binding installed(recipient);authority.replay_execution_phase(typed[frame]);}
    need(NativeVehicleCamera::encode_checkpoint(recipient.checkpoint(0))==typed_cameras[frame],
        "Generic numeric replay changed an actual complete original camera checkpoint");
  }
  need(encode_source_authority(authority.checkpoint(binding))==expected,
      "Generic numeric replay changed the complete uninterrupted native world");
  result={{"passed",true},{"stage","complete"},{"source_tick0_seed",true},{"local_views",4},{"numeric_updates",52},
      {"native_frames",12},{"original_camera_bytes_exact_each_phase",true},{"complete_post_step_world_exact",true},
      {"world_sha256",hash(actual)},{"initial_camera_sha256",hash(cameras[0])},{"final_camera_sha256",hash(cameras.back())},
      {"local_seed_survived_import",true},{"forged_controller_import",imported?"admitted_and_ignored_by_numeric_camera":"rejected_by_world_admission"},
      {"network_enabled",false},{"raw25_policy_changed",false},
      {"typed_numeric_journal",true},{"generic_replay_world_exact",true},{"generic_replay_camera_exact_each_phase",true},
      {"scope","Locally reconstructed original157FE0 camera owner from independently checked source player identities, source tick0 world and numeric view/seconds/cycle inputs. Exact original call operands/returns/clocks replayed and complete native world/camera bytes compared. Not a network payload, arbitrary-origin seal or online host-loss qualification."}};
  save_report();
  std::cout<<"PASS fresh admitted numeric camera reconstructs52 actual source updates and exact complete12-frame native world\n";return 0;
}catch(const std::exception &error){result["error"]=error.what();try{save_report();}catch(const std::exception &report_error){std::cerr<<report_error.what()<<'\n';}
  std::cerr<<"Numeric camera oracle stage "<<result["stage"]<<": "<<error.what()<<'\n';return 1;}
