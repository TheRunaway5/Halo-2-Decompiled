#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string_view>

#include "halo2/controller_settings.hpp"
#include "halo2/multiplayer_look_authority.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>


namespace {
void runtime_need(bool value,const char *why) {if(!value)throw std::runtime_error(why);}
#if SDL_VERSION_ATLEAST(2,0,14)
struct RuntimeGamepad {
  SDL_Joystick *joystick{};
  SDL_GameController *controller{};
  SDL_JoystickID instance=-1;
  RuntimeGamepad() {
    using namespace halo2;
    runtime_need(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)==0,"Initialize SDL gamepad input owner");
    const int device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
        SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
    runtime_need(device>=0,"Attach actual SDL virtual gamepad");
    joystick=SDL_JoystickOpen(device);runtime_need(joystick,"Open actual virtual joystick");
    instance=SDL_JoystickInstanceID(joystick);
    char guid[33];SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joystick),guid,sizeof(guid));
    const auto mapping=std::string(guid)+",Native right-stick regression,a:b0,b:b1,x:b2,y:b3,"
        "leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,";
    runtime_need(SDL_GameControllerAddMapping(mapping.c_str())>=0 && SDL_IsGameController(device),
        "Map actual SDL virtual gamepad axes");
    controller=SDL_GameControllerOpen(device);runtime_need(controller,"Open actual mapped gamepad");
    for(int axis=0;axis<SDL_CONTROLLER_AXIS_MAX;++axis)
      runtime_need(SDL_JoystickSetVirtualAxis(joystick,axis,
          axis>=SDL_CONTROLLER_AXIS_TRIGGERLEFT?SDL_JOYSTICK_AXIS_MIN:0)==0,"Neutralize actual virtual gamepad axes");
    SDL_GameControllerUpdate();
  }
  void pitch(Sint16 value) {
    runtime_need(SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_RIGHTY,value)==0,
        "Deliver ordinary actual SDL right-stick input");SDL_GameControllerUpdate();
  }
  ~RuntimeGamepad() {
    if(controller)SDL_GameControllerClose(controller);
    if(joystick)SDL_JoystickClose(joystick);
    for(int device=0;device<SDL_NumJoysticks();++device)
      if(SDL_JoystickGetDeviceInstanceID(device)==instance){SDL_JoystickDetachVirtual(device);break;}
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
  }
};
#endif
int runtime_probe(const std::filesystem::path &maps,const std::filesystem::path &report_path) {
  using namespace halo2;
#if !SDL_VERSION_ATLEAST(2,0,14)
  (void)maps;(void)report_path;return 77;
#else
  if(!std::filesystem::is_regular_file(maps/"lockout.map") || !std::getenv("DISPLAY"))return 77;
  runtime_need(std::getenv("HALO2_TEST_SCRATCH"),"Run virtual-gamepad regression through tools/run_test.py");
  nlohmann::json report={{"passed",false},{"samples",nlohmann::json::array()},
      {"scope","Actual SDL virtual right stick through automatic canonical MCC World and ordinary rendered run_multiplayer. Original source look limits, processed pitch and raw input are read-only. No actor/command/pitch outcomes or captures supplied."}};
  const auto save=[&] {
    std::ofstream out(report_path);runtime_need(bool(out),"Write virtual-gamepad runtime report");out<<report.dump(2)<<'\n';
  };
  try {
    SDL_SetMainReady();SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
    MultiplayerOptions options;options.map=maps/"lockout.map";options.shared_maps=maps;
    runtime_need(Cache(options.map,maps).is_mcc(),"Virtual-gamepad runtime requires prepared MCC input");
    options.local_players=1;options.width=640;options.height=480;options.starting_weapon=5;options.mute=true;
    options.frames=135;options.reference_tick_cadence=true;
    // One ordinary simulation tick and one controller sample per actual
    // frame. Runtime4509..4511 therefore uses1/30s, below its0.1s elapsed
    // clamp. Reaching3radians needs over85 real held-stick frame samples.
    runtime_need(!options.source_object_world,"Runtime regression accidentally forces the explicit source-world flag");
    options.desktop=std::make_shared<DesktopSession>();
    options.desktop->controller_settings.sensitivity=3;options.desktop->controller_settings.invert=false;
    options.desktop->controller_settings.auto_center=false;
    auto *window=options.desktop->window(options.width,options.height,false);
    RuntimeGamepad pad;options.input_instances={pad.instance};
    SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
    pad.pitch(SDL_JOYSTICK_AXIS_MIN);
    const SourceRuntimeMemory *memory{};unsigned worlds{},samples{},submitted{},unclamped{},reverse_samples{},released_samples{};
    std::uint32_t selected_player=null_datum,selected_unit=null_datum;
    float maximum_raw=-1000.f,held_raw{},reversed_raw{},held_processed{},reversed_processed{};
    bool release_steady=true;
    options.source_world_observer=[&](const SourceRuntimeMemory &observed){memory=&observed;++worlds;};
    options.frame_observer=[&](const MultiplayerFrameTiming &frame){submitted+=frame.submitted;};
    options.input_observer=[&](std::uint32_t step,std::span<const Controls> controls,const MultiplayerState &state) {
      runtime_need(memory && worlds==1 && controls.size()==1 && step==samples,
          "Runtime input lost the automatically selected native World or actual controller sample");
      const auto &player=state.players.at(0);
      runtime_need(player.connected && !player.dead && player.source_player!=null_datum && player.source_unit!=null_datum,
          "Virtual-gamepad control lost its genuine living native actor");
      if(!samples){selected_player=player.source_player;selected_unit=player.source_unit;}
      runtime_need(player.source_player==selected_player && player.source_unit==selected_unit,
          "Virtual-gamepad runtime changed its full native player/unit generation");
      const auto word=[&](std::uint32_t address){return Bytes(memory->view(address,4)).u32(0);};
      SourceRawDatumPoolServices source_reads;
      source_reads.read=[&](std::uint32_t address,std::size_t size){return memory->view(address,size);};
      const auto row=source_raw_datum_lookup(word(0x4f54f4u),selected_player,source_reads);
      const auto object=source_raw_datum_lookup(word(0x4ecbd0u),selected_unit,source_reads);
      runtime_need(row && object && Bytes(memory->view(row,0x30u)).u32(0x2cu)==selected_unit,
          "Virtual-gamepad source actor generation is not current");
      const auto retained=0x60070000u+(selected_player&65535u)*0xa0u;
      const Bytes look(memory->view(retained,0xa0u));
      runtime_need(look.u32(0)==selected_unit && look.u32(0x94u)==selected_player,
          "Runtime source look record is not bound to the actual full player/unit");
      const auto raw=controls[0].pitch,processed=look.f32(0x14u),minimum=look.f32(0x80u),maximum=look.f32(0x84u);
      runtime_need(std::isfinite(raw) && std::isfinite(processed) && std::isfinite(minimum) && std::isfinite(maximum) &&
          minimum<maximum && processed>=minimum && processed<=maximum &&
          std::bit_cast<std::uint32_t>(raw)==look.u32(0x9cu) &&
          std::bit_cast<std::uint32_t>(player.pitch)==look.u32(0x14u),
          "Actual source controller did not own finite authored limits/processed pitch and raw input retention");
      maximum_raw=std::max(maximum_raw,raw);unclamped+=raw>3.f;
      if(step==100){held_raw=raw;held_processed=processed;pad.pitch(SDL_JOYSTICK_AXIS_MAX);}
      if(step>100 && step<=110)++reverse_samples;
      if(step==110){reversed_raw=raw;reversed_processed=processed;pad.pitch(0);}
      if(step>111){++released_samples;release_steady=release_steady &&
          std::bit_cast<std::uint32_t>(raw)==std::bit_cast<std::uint32_t>(reversed_raw);}
      report["samples"].push_back({{"step",step},{"source_tick",state.tick},{"raw_pitch",raw},
          {"retained_raw_pitch",look.f32(0x9cu)},{"processed_pitch",processed},{"source_minimum",minimum},{"source_maximum",maximum}});
      ++samples;
    };
    const auto started=std::chrono::steady_clock::now();const auto status=run_multiplayer(options);
    pad.pitch(0);
    report.update({{"runtime_status",status},{"requested_source_world",options.source_object_world},
        {"automatically_created_worlds",worlds},{"source_player",selected_player},{"source_unit",selected_unit},
        {"actual_input_samples",samples},{"submitted_frames",submitted},{"maximum_raw_pitch",maximum_raw},
        {"held_raw_pitch",held_raw},{"reversed_raw_pitch",reversed_raw},{"raw_above3_samples",unclamped},
        {"held_processed_pitch",held_processed},{"reversed_processed_pitch",reversed_processed},{"released_raw_pitch_steady",release_steady},
        {"reverse_samples",reverse_samples},{"released_samples",released_samples},
        {"wall_seconds",std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()},
        {"no_captures_retained",true}});
    runtime_need(status==0 && worlds==1 && samples>=120 && submitted==options.frames && !options.source_object_world &&
        unclamped>0 && maximum_raw>3.f && reverse_samples>=10 && released_samples>=10 && release_steady &&
        held_raw-reversed_raw>.2f && held_processed-reversed_processed>.1f,
        "Actual SDL runtime saturated automatic-native raw look or failed its held/reverse/release frame control");
    report["passed"]=true;save();std::cout<<report.dump()<<'\n';return 0;
  }catch(const std::exception &error){report["error"]=error.what();save();std::cerr<<error.what()<<'\n';return 1;}
#endif
}
int camouflage_runtime_probe(const std::filesystem::path &maps,const std::filesystem::path &report_path) {
  using namespace halo2;
  if(!std::filesystem::is_regular_file(maps/"mainmenu.map") ||
      !std::filesystem::is_regular_file(maps/"lockout.map") || !std::getenv("DISPLAY"))return 77;
  runtime_need(std::getenv("HALO2_TEST_SCRATCH"),"Run camouflage draw regression through tools/run_test.py");
  nlohmann::json report={{"passed",false},{"cases",nlohmann::json::array()},
      {"scope","Authored Fiesta variant flag21, ordinary eight-frame runtime and read-only native timer/fade/GEN plus emitted camouflage draw commands. Successful submitted frames are required. No pose, material, fade, input-command outcome or capture supplied; no pixel-parity claim."}};
  const auto save=[&] {
    std::ofstream out(report_path);runtime_need(bool(out),"Write camouflage runtime report");out<<report.dump(2)<<'\n';
  };
  try {
    Cache frontend_cache(maps/"mainmenu.map",maps);
    runtime_need(frontend_cache.is_mcc(),"Camouflage runtime requires prepared MCC input");
    const auto frontend=frontend_cache.load_frontend();
    const auto fiesta=std::ranges::find_if(frontend.bundled_variants,[](const auto &variant) {
      return custom_slayer(variant)==CustomSlayer::fiesta;
    });
    runtime_need(fiesta!=frontend.bundled_variants.end(),"Camouflage runtime lacks its authored Fiesta preset");
    for(const bool enabled:{false,true}) {
      auto &result=report["cases"].emplace_back(nlohmann::json{{"enabled",enabled},{"passed",false},
          {"samples",nlohmann::json::array()},{"draws",nlohmann::json::array()}});
      try {
        MultiplayerOptions options;options.map=maps/"lockout.map";options.shared_maps=maps;
        options.variant=*fiesta;options.variant.values.set(21,enabled?1:0);
        runtime_need(source_game_variant_valid(source_game_variant(options.variant)) &&
            source_game_variant_get(source_game_variant(options.variant),21)==int(enabled),
            "Camouflage runtime flag is not a legal original variant input");
        options.local_players=1;options.width=640;options.height=480;options.mute=true;
        options.frames=8;options.reference_tick_cadence=true;options.input_instances={-1};
        options.source_object_savegame_directory=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/
            (enabled?"camouflage-runtime-on":"camouflage-runtime-off");
        runtime_need(!options.source_object_world,"Camouflage runtime manually forced World authority");
        const SourceRuntimeMemory *memory{};unsigned worlds{},samples{},submitted{},draws{},foreground_vertices{};
        std::uint32_t player=null_datum,unit=null_datum,latest_tick{};
        std::array<std::uint32_t,3> latest_fade{};
        bool active{},inactive=true;
        const auto native_fade=[&]() {
          runtime_need(memory && player!=null_datum && unit!=null_datum,"Camouflage readback lacks a native actor");
          const auto word=[&](std::uint32_t address){return Bytes(memory->view(address,4)).u32(0);};
          SourceRawDatumPoolServices reads;
          reads.read=[&](std::uint32_t address,std::size_t size){return memory->view(address,size);};
          const auto row=source_raw_datum_lookup(word(0x4f54f4u),player,reads);
          const auto header=source_raw_datum_lookup(word(0x4ecbd0u),unit,reads);
          runtime_need(row && header && word(row+0x2cu)==unit,
              "Camouflage runtime lost its current full native player/unit generation");
          const auto body=word(header+8u);
          runtime_need(body!=0 && body!=null_datum,"Camouflage runtime has no genuine body allocation");
          const Bytes fade(memory->view(body+0x2b0u,12));
          std::array<std::uint32_t,3> bits{fade.u32(0),fade.u32(4),fade.u32(8)};
          for(const auto value:bits)runtime_need(std::isfinite(std::bit_cast<float>(value)),
              "Camouflage runtime has nonfinite native fade");
          runtime_need(fade.f32(0)>=0.f && fade.f32(0)<=1.f,"Camouflage primary is outside its native render domain");
          return std::pair{bits,std::bit_cast<std::int16_t>(Bytes(memory->view(row+0x198u,2)).u16(0))};
        };
        options.source_world_observer=[&](const SourceRuntimeMemory &observed){memory=&observed;++worlds;};
        options.input_observer=[&](std::uint32_t step,std::span<const Controls>,const MultiplayerState &state) {
          runtime_need(memory && worlds==1 && step==samples,"Camouflage runtime lost its ordinary native frame sequence");
          const auto &actor=state.players.at(0);
          runtime_need(actor.connected && !actor.dead && actor.source_player!=null_datum && actor.source_unit!=null_datum,
              "Camouflage runtime lacks its living native participant");
          if(!samples){player=actor.source_player;unit=actor.source_unit;}
          runtime_need(actor.source_player==player && actor.source_unit==unit,
              "Camouflage runtime replaced its selected full actor generation");
          const auto [bits,timer]=native_fade();
          const std::array<std::uint32_t,3> adopted{
              std::bit_cast<std::uint32_t>(actor.source_camouflage.primary),
              std::bit_cast<std::uint32_t>(actor.source_camouflage.secondary),
              std::bit_cast<std::uint32_t>(actor.source_camouflage.fade_speed)};
          runtime_need(bits==adopted,"Camouflage runtime adopted fade differs from completed native body bits");
          SourceGameVariant mechanical;
          const auto globals=Bytes(memory->view(0x4f3218u,4)).u32(0);
          memory->read(globals+0x13cu,std::as_writable_bytes(std::span(mechanical.bytes)));
          runtime_need(source_game_variant_valid(mechanical) && source_game_variant_get(mechanical,21)==int(enabled),
              "Camouflage runtime mechanical variant lost its declared flag");
          active=active || (timer>0 && std::bit_cast<float>(bits[0])>0.f);
          inactive=inactive && timer==0 && std::bit_cast<float>(bits[0])==0.f;
          latest_fade=bits;latest_tick=state.tick;
          result["samples"].push_back({{"step",step},{"source_tick",state.tick},{"timer",timer},
              {"source_player",player},{"source_unit",unit},{"native_fade_bits",bits},{"adopted_fade_bits",adopted}});
          ++samples;
        };
        options.camouflage_layer_observer=[&](unsigned view,std::uint32_t owner,unsigned layer,
            unsigned implementation,bool first_person,float primary,unsigned vertices) {
          runtime_need(samples && view==0 && owner==unit && vertices>0 && std::isfinite(primary) &&
              primary>0.f && primary<=1.f && ((layer==18 && implementation<=2) || (layer==19 && implementation==0)),
              "Emitted camouflage draw lost its actual owner/pass/geometry");
          const auto [bits,timer]=native_fade();
          runtime_need(enabled && timer>0 && bits==latest_fade &&
              std::bit_cast<std::uint32_t>(primary)==bits[0],
              "Emitted camouflage primary differs from the completed native actor fade");
          ++draws;if(first_person)foreground_vertices+=vertices;
          if(result["draws"].size()<32)result["draws"].push_back({{"source_tick",latest_tick},{"owner",owner},
              {"layer",layer},{"implementation",implementation},{"first_person",first_person},
              {"primary_bits",std::bit_cast<std::uint32_t>(primary)},{"native_primary_bits",bits[0]},
              {"vertices",vertices}});
        };
        options.frame_observer=[&](const MultiplayerFrameTiming &frame){submitted+=frame.submitted;};
        const auto status=run_multiplayer(options);
        result.update({{"authored_variant_name",fiesta->name},{"runtime_status",status},{"submitted_frames",submitted},
            {"completed_native_samples",samples},{"automatic_worlds",worlds},{"requested_source_world",options.source_object_world},
            {"source_player",player},{"source_unit",unit},{"emitted_camouflage_draws",draws},
            {"eligible_foreground_vertices",foreground_vertices},{"native_active_observed",active},{"native_inactive_all_samples",inactive}});
        runtime_need(status==0 && worlds==1 && samples==8 && submitted==8 && !options.source_object_world,
            "Camouflage runtime did not complete its ordinary eight native samples/submitted frames");
        runtime_need(enabled?(active && foreground_vertices>0):(inactive && draws==0),
            "Camouflage runtime lacks a naturally eligible foreground draw or its off control emitted camouflage");
        result["passed"]=true;
      }catch(const std::exception &error){result["error"]=error.what();}
    }
    report["passed"]=std::ranges::all_of(report["cases"],[](const auto &row){return row["passed"].template get<bool>();});
    save();std::cout<<report.dump()<<'\n';return report["passed"].get<bool>()?0:1;
  }catch(const std::exception &error){report["error"]=error.what();save();std::cerr<<error.what()<<'\n';return 1;}
}

}

int main(int argc, char **argv) {
  if(argc==4 && std::string_view(argv[1])=="--runtime")return runtime_probe(argv[2],argv[3]);
  if(argc==4 && std::string_view(argv[1])=="--camouflage-runtime")return camouflage_runtime_probe(argv[2],argv[3]);
  using namespace halo2;
  nlohmann::json rows=nlohmann::json::array();
  unsigned failures{};
  for (bool requested : {false,true}) {
    for (bool active : {false,true}) {
      const bool expected=requested || active;
      const bool source=multiplayer_source_look_authority(requested,active);
      ControllerSettings settings;
      settings.sensitivity=3;
      settings.invert=false;
      ControllerInput input;
      input.look_pitch=1;
      float yaw{},pitch{};
      // Exercise the real production accumulator with its production policy.
      // Native delivery constrains the processed angle after taking the raw
      // current-minus-previous input, so this accumulator must not saturate.
      controller_look(input,settings,5.f,yaw,pitch,!source);
      const float held=pitch;
      input.look_pitch=-1;
      controller_look(input,settings,.1f,yaw,pitch,!source);
      const bool policy_matches=source==expected;
      const bool accumulator_matches=expected ? held>3.f : held<=1.4922565f;
      const bool reverse_delta_preserved=std::abs((held-pitch)-.10471976f)<1e-6f;
      const bool passed=policy_matches && accumulator_matches && reverse_delta_preserved;
      failures+=!passed;
      rows.push_back({{"requested_source_world",requested},{"active_source_relative_look",active},
        {"expected_source_authority",expected},{"actual_source_authority",source},
        {"held_raw_pitch",held},{"reverse_raw_pitch",pitch},
        {"policy_matches",policy_matches},{"accumulator_matches",accumulator_matches},
        {"reverse_delta_preserved",reverse_delta_preserved},{"passed",passed}});
    }
  }
  const nlohmann::json report={{"scope","Shared production look policy and real controller accumulator; no native World/input/render execution"},
    {"cases",rows},{"failures",failures},{"passed",failures==0}};
  if(argc==2) {
    std::ofstream out(argv[1]);
    if(!out)throw std::runtime_error("Cannot write multiplayer look policy report");
    out<<report.dump(2)<<'\n';
  } else if(argc!=1)throw std::runtime_error("Expected optional report path");
  std::cout<<report.dump()<<'\n';
  return failures?1:0;
}
