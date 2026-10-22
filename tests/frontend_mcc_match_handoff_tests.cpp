#include "halo2/native_vista_flow.hpp"
#include "halo2/vista_announcer_events.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "rendering/native_mcc_shader_function_data.hpp"
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char* message) {if(!value)throw FormatError(message);}
}
int main(int argc,char** argv)try {
  if(argc!=2 && argc!=3)throw FormatError("Imported MCC cache root and project root required");
  std::filesystem::path root=argv[1];
  if(argc==3 && root=="auto") {
    const std::filesystem::path project=argv[2];
    std::ifstream source(project/"assets/mcc-source.tsv");
    std::string signature,key,path;std::getline(source,signature);
    if(signature!="H2_MCC_ASSET_SOURCE_V1")return 77;
    while(source>>key>>path)if(key=="cache")root=project/path;
  }
  const auto maps=argc==2?root:root/"halo2/h2_maps_win64_dx11";
  if(!std::filesystem::is_regular_file(maps/"mainmenu.map"))return 77;
  const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");
  check(scratch,"Handoff regression requires owned scratch");
  Cache cache(maps/"mainmenu.map");
  check(cache.is_mcc(),"Handoff regression requires MCC data");
  auto content=cache.load_frontend();
  content.vista=load_vista_settings(maps/"mainmenu.map");
  FrontendSession frontend;
  frontend.retain_content(std::move(content));
  auto& state=frontend.create_state(std::filesystem::path(scratch)/"handoff.cfg");
  state.accept();state.accept();state.text("MCC Match Test");state.commit_text();state.back();
  check(state.frame().screen==6,"Profile route did not reach main menu");
  state.select(2);state.accept();check(state.frame().screen==30,"Split screen route failed");
  state.accept();check(state.frame().screen==14,"Native lobby route failed");
  state.select(0);state.accept();
  check(state.launch && NativeVistaOfflineFlow::supports(*state.launch),"Start lacks supported native request");
  std::cout<<"MCC handoff selected map="<<state.launch->map<<std::endl;
  MultiplayerOptions defaults;defaults.mute=true;
  defaults.source_object_world=true;
  defaults.source_object_savegame_directory=std::filesystem::path(scratch)/"native-world";
  unsigned loads{};
  NativeVistaOfflineFlow flow(frontend,maps,defaults,[&]{++loads;});
  flow.update_frontend(state,0,[](FrontendUiSound){throw FormatError("Unexpected early countdown beep");});
  check(!state.launch,"Native flow did not consume the actual MenuState launch request");
  check(flow.countdown()==10 && !flow.launch_ready() && loads==0,"Countdown skipped preparation boundary");
  check(state.frame().screen==14 && state.signed_in_profile()=="MCC Match Test","Native flow corrupted retained frontend state");
  const auto start=std::chrono::steady_clock::now();
  while(!flow.launch_ready()) {
    const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
    check(elapsed<16000,"Real countdown failed to request native resources");
    flow.update_frontend(state,std::uint32_t(elapsed),[](FrontendUiSound){});
    if(!flow.launch_ready()) {
      check(!std::filesystem::exists(defaults.source_object_savegame_directory),"Countdown asset worker created the source world early");
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }
  check(loads==1,"Native map resources were not requested exactly once");
  check(flow.prepared_world()->source_objects!=nullptr,"MCC menu request did not prepare the native gameplay world");
  const auto world=flow.prepared_world();
  check(world->gameplay_fonts!=nullptr,"Countdown did not reuse its prepared font bank");
  const auto tables=world->cache->shader_function_tables();
  for(bool corrupt:{false,true}) {
    SourceRuntimeMemory foreign;
    if(!corrupt)foreign.mount(0x4752a9u,std::array<std::byte,1>{},
        SourceRuntimeMemory::Access::ReadWrite,"foreign partial function row");
    const auto before=foreign.checkpoint();auto input=*tables;
    if(corrupt)input.transition[2][0]^=1;
    bool rejected{};
    try{detail::mount_native_mcc_shader_functions(foreign,input);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && foreign.checkpoint()==before,"Foreign function owner caused partial publication");
  }
  for(unsigned row=0;row<20;++row) {
    const auto expected=row<12?std::span(tables->periodic[row]):std::span(tables->transition[row-12]);
    const auto address=0x4752a8u+row*1024u;
    const auto actual=world->source_objects->memory().view(address,1024);
    check(world->source_objects->memory().is_read_only(address,1024)==(row!=8) &&
        std::ranges::equal(actual,std::as_bytes(expected)),"Gameplay function samples differ from the existing material bank");
  }
  const auto row8=std::as_bytes(std::span(tables->periodic[8]));
  const std::string row8_owner="native TU1.5 original round-reset response curve eight 1024 byte samples";
  for(unsigned control=0;control<4;++control) {
    SourceRuntimeMemory foreign;std::array<std::byte,1024> bytes;
    std::ranges::copy(row8,bytes.begin());if(control==1)bytes[4]^=std::byte{1};
    const auto address=control==2?0x4752a8u:0x4772a8u;
    foreign.mount(address,std::span(bytes).first(control==3?1023u:1024u),
        SourceRuntimeMemory::Access::ReadWrite,control==0?"foreign writable row8":row8_owner);
    const auto before=foreign.checkpoint();bool rejected{};
    try{detail::mount_native_mcc_shader_functions(foreign,*tables);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && foreign.checkpoint()==before,"Invalid writable function row caused partial publication");
  }
  SourceRuntimeMemory writable;
  writable.mount(0x4772a8u,row8,SourceRuntimeMemory::Access::ReadWrite,row8_owner);
  const auto row8_native=writable.native_address(0x4772a8u,1024);
  detail::mount_native_mcc_shader_functions(writable,*tables);
  check(!writable.is_read_only(0x4772a8u,1024) &&
      writable.native_address(0x4772a8u,1024)==row8_native,"Original writable function owner was replaced or frozen");
  writable.mutable_view(0x4772a8u,1)[0]^=std::byte{1};
  const auto changed=writable.checkpoint();bool live_rejected{};
  try{detail::mount_native_mcc_shader_functions(writable,*tables);}catch(const std::invalid_argument&){live_rejected=true;}
  check(live_rejected && writable.checkpoint()==changed,"Repeated function binding reset a live original row");
  SourceRuntimeMemory boundary;
  boundary.mount(0x47a2a8u,std::array{std::byte{0x5a}},SourceRuntimeMemory::Access::ReadWrite,
      "separate placement policy control");
  detail::mount_native_mcc_shader_functions(boundary,*tables);
  const auto mounted=boundary.checkpoint();
  const auto native=boundary.native_address(0x4752a8u,1024);
  detail::mount_native_mcc_shader_functions(boundary,*tables);
  check(boundary.checkpoint()==mounted && boundary.native_address(0x4752a8u,1024)==native &&
      !boundary.is_read_only(0x47a2a8u,1) && boundary.view(0x47a2a8u,1)[0]==std::byte{0x5a},
      "Function binding changed adjacent placement state or existing native backing");
  const auto responses=load_vista_announcer_responses(*world->cache);
  check(!responses.general.empty() && !responses.slayer.empty() && !responses.king.empty(),
      "MCC scenario lost its actual announcer response banks");
  std::cout<<"MCC announcer banks="<<responses.general.size()<<','<<responses.slayer.size()<<','<<responses.king.size()<<std::endl;
  (void)flow.take_launch();
  flow.world_loaded(world->match,world->local_slots);
  world->bind_source_local_views(world->local_slots);
  const auto local=world->local_slots.at(0);
  NativeVehicleCamera camera(*world->source_objects,*world->cache->original_data());
  // Ordinary rendering also updates the observer before the first30Hz tick,
  // while the genuine selected player is still unactivated and has no unit.
  (void)camera.update_player(0,world->match.player(local).source_datum,
      world->match.player(local).source_flags,1.f/60.f);
  world->match.step({});
  check(!flow.world_update(world->match),"Fresh match requested an immediate return");
  check(flow.player_activation_count()==1 && world->match.player(local).source_current_object!=null_datum,
      "First native tick did not activate and spawn the selected player");
  check(world->match.first_person_view(0).hands[0].weapon!=null_datum,
      "First native tick did not bind the spawned weapon");
  std::array<Controls,multiplayer_capacity> controls{};controls[local].fire=true;
  for(unsigned tick=0;tick<8;++tick) {
    world->match.step(controls);
    world->source_objects->advance_effect_frame(1.f/30.f);
    check(!flow.world_update(world->match),"Firing immediately ended the native match");
  }
  std::cout<<"PASS actual MCC menu Start/countdown/native world preparation\n";
  return 0;
}catch(const std::exception& error){std::cerr<<"error: "<<error.what()<<'\n';return 1;}
