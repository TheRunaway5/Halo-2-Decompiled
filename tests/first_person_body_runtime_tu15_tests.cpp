#define SDL_MAIN_HANDLED
#include "halo2/animation_presentation.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/gameplay_fov.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <bit>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
nlohmann::json report{{"passed",false},{"stage","bootstrap"}};
void need(bool value,const char *message){if(!value)throw FormatError(message);}
bool exact(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool exact(Vec3 a,Vec3 b){return exact(a.x,b.x)&&exact(a.y,b.y)&&exact(a.z,b.z);}
Vec3 vector(const SourceObserverState &state,unsigned offset) {
  return {std::bit_cast<float>(state.at(offset/4)),std::bit_cast<float>(state.at(offset/4+1)),
      std::bit_cast<float>(state.at(offset/4+2))};
}
std::array<float,3> xyz(Vec3 v){return {v.x,v.y,v.z};}
void save(const char *path) {
  const auto bytes=report.dump(2)+"\n";need(bytes.size()<=1024*1024,"First-person render report exceeds its bounded receipt");
  std::ofstream out(path);out<<bytes;need(bool(out),"Could not write first-person render report");
}
void push(SDL_Event event){need(SDL_PushEvent(&event)==1,"Could not queue actual first-person body input");}
void verify_capture(const std::filesystem::path &path,unsigned expected_width,unsigned expected_height) {
  std::ifstream input(path,std::ios::binary);need(bool(input),"Actual body screenshot was not written");
  std::string magic;unsigned width{},height{},maximum{};input>>magic>>width>>height>>maximum;
  need(magic=="P6" && width==expected_width && height==expected_height && maximum==255,
      "Actual body screenshot lost its expected RGB PPM extent");
  const auto separator=input.get();need(separator!=EOF && std::isspace(static_cast<unsigned char>(separator)),
      "Actual body screenshot has an invalid PPM header");
  const auto pixels=std::size_t(width)*height;std::vector<unsigned char> rgb(pixels*3);
  input.read(reinterpret_cast<char *>(rgb.data()),std::streamsize(rgb.size()));
  need(input.gcount()==std::streamsize(rgb.size()) && input.peek()==EOF,"Actual body screenshot has incomplete RGB pixels");
  std::size_t nonblack{};bool nonuniform{};unsigned minimum=255,maximum_channel{};
  for(std::size_t pixel=0;pixel<pixels;++pixel) {
    const auto at=pixel*3;const unsigned channel=std::max({rgb[at],rgb[at+1],rgb[at+2]});
    nonblack+=channel>8;minimum=std::min(minimum,channel);maximum_channel=std::max(maximum_channel,channel);
    nonuniform|=rgb[at]!=rgb[0] || rgb[at+1]!=rgb[1] || rgb[at+2]!=rgb[2];
  }
  report["capture_pixels"]={{"width",width},{"height",height},{"rgb_bytes",rgb.size()},
      {"nonblack_pixels",nonblack},{"nonuniform",nonuniform},
      {"minimum_max_channel",minimum},{"maximum_max_channel",maximum_channel}};
  // This is a visible-output fixture check, not an image-parity tolerance.
  // Together with the actual source fade observation, it rejects startup
  // black, a uniform clear color and an otherwise black frame with stray pixels.
  need(nonblack>pixels/100 && nonuniform && maximum_channel>minimum+8,
      "Actual body screenshot is black or uniform; it cannot prove visible body placement");
}
}
int main(int argc,char **argv)try {
  need(argc==6,"Supply prepared MCC map, shared maps, representation2|3, standing|crouching, report");
  if(!std::getenv("DISPLAY") || !std::filesystem::is_regular_file(argv[1]))return 77;
  const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
  need(scratch && std::filesystem::path(scratch).is_absolute(),"Run first-person render check through tools/run_test.py");
  const unsigned representation=unsigned(std::stoul(argv[3]));const std::string stance=argv[4];
  need(representation==2 || representation==3,"First-person render check needs human or Elite");
  need(stance=="standing" || stance=="crouching","Unknown first-person render stance");
  report["representation"]=representation;report["stance"]=stance;
  constexpr unsigned capture_frames=32;
  report["scope"]="Actual GPU-submitted grounded down-look frames through original fade completion; final visible RGB capture, original on-foot observer and exact unit WORLD body/vertices at endpoint sampling";
  SDL_SetMainReady();
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  options.local_players=1;options.starting_weapon=5;options.mute=true;options.source_object_world=true;
  options.width=640;options.height=480;options.frames=capture_frames;options.reference_tick_cadence=true;options.reference_render_fraction=1.f;
  options.variant={"Slayer",0,{{2,0},{10,0},{89,5}}};
  options.source_object_savegame_directory=std::filesystem::path(scratch)/"first-person-body-render";
  auto profile=source_default_player_configuration();profile[0x44]=representation==3?1:0;options.selected_profiles={profile};
  report["stage"]="construct-native-world";
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  auto &world=*options.prepared_world;world.bind_source_local_views(world.local_slots);
  need(world.cache->is_mcc() && world.source_objects && world.source_objects->owns_physics() && world.match.source_engine_owned(),
      "First-person render check lost its canonical native world");
  auto &objects=*world.source_objects;const auto slot=world.local_slots.at(0);const auto &data=*world.cache->original_data();
  need(data.image==OriginalData::Image::TitleUpdate15 && data.observer_constants && data.observer_world_constants && data.death_camera_constants,
      "First-person render check lacks original TU1.5 observer owners");
  std::array<Controls,multiplayer_capacity> controls{};controls[slot].crouch=stance=="crouching";
  for(unsigned tick=0;tick<105;++tick) {
    controls[slot].pitch=-std::min(1.5f,.05f*float(tick));world.match.step(controls);
    const auto p=world.match.state().players[slot];
    report["settling_input_samples"].push_back({{"tick",tick},{"raw_pitch",controls[slot].pitch},
        {"published_pitch",p.pitch},{"grounded",p.native_grounded},{"crouching",p.native_crouching}});
  }
  const auto prepared=world.match.state().players[slot];
  need(prepared.native_grounded && !prepared.dead && prepared.source_parent==null_datum,
      "First-person render check did not settle its actual on-foot body");
  options.desktop=std::make_shared<DesktopSession>();
  options.desktop->keyboard_settings.sensitivity=3;options.desktop->keyboard_settings.invert=false;
  options.desktop->display_settings.field_of_view=default_field_of_view;
  auto *window=options.desktop->window(options.width,options.height,false);const auto window_id=SDL_GetWindowID(window);
  SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  need(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"First-person render check could not capture actual mouse input");
  SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.windowID=window_id;focus.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;push(focus);
  // Runtime initializes its PC accumulators from the published source look.
  // That processed angle can differ from the last raw PC sample. Deliver the
  // measured difference back to the known settling input, without assuming
  // that the processed angle has the same pitch sign or scale.
  constexpr float mouse_gain=.0025f;
  SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
  look.motion.xrel=int(std::lround((prepared.yaw-controls[slot].yaw)/mouse_gain));
  look.motion.yrel=-int(std::lround((controls[slot].pitch-prepared.pitch)/mouse_gain));push(look);
  report["runtime_initial_processed_look"]={prepared.yaw,prepared.pitch};
  report["settling_raw_input"]={controls[slot].yaw,controls[slot].pitch};
  report["mouse_input_units"]={look.motion.xrel,look.motion.yrel};
  if(stance=="crouching") {
    SDL_Event key{};key.type=SDL_KEYDOWN;key.key.windowID=window_id;key.key.state=SDL_PRESSED;
    key.key.keysym.scancode=SDL_SCANCODE_LSHIFT;key.key.keysym.sym=SDLK_LSHIFT;push(key);
  }
  options.screenshot=std::filesystem::path(scratch)/("body-"+std::to_string(representation)+"-"+stance+".ppm");
  if(const auto retained=std::getenv("HALO2_FIRST_PERSON_BODY_CAPTURE")) {
    need(std::filesystem::path(retained).is_absolute(),"Explicit first-person capture path must be absolute");options.screenshot=retained;
    report["explicit_capture_path"]=options.screenshot.string();
  }
  SourceObserverState observer{};DeathCameraPacket packet{};unsigned camera_calls{},input_calls{},body_calls{},node_calls{},view_calls{},submitted{};
  unsigned fade_calls{},visible_frames{};bool final_source_fade_clear{};
  std::uint32_t body_model=null_datum;std::vector<Transform> completed_body;
  std::vector<Vertex> body_vertices;std::optional<RenderModel> authored_body;
  Camera submitted_camera;Vec3 expected_position{},expected_forward{},expected_up{};bool camera_equal{},steep_down{};
  float observed_horizontal_fov{};
  SourcePhysicsEntryObserverBinding diagnostic(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &) {
    if(entering || entry!=0x157fe0u)return;
    objects.memory().read(0x4f64a4u,std::as_writable_bytes(std::span(observer)));
    objects.memory().read(0x4f5514u+0x5cu,std::as_writable_bytes(std::span(packet)));++camera_calls;
  });
  options.input_observer=[&](unsigned,std::span<const Controls> input,const MultiplayerState &state) {
    need(input.size()==1 && input[0].crouch==(stance=="crouching"),"Actual native crouch input was not delivered");
    need(std::abs(input[0].pitch-controls[slot].pitch)<=mouse_gain,
        "Actual mouse input did not restore the prepared world's raw look baseline");
    report["delivered_raw_look"]={input[0].yaw,input[0].pitch};
    const auto &p=state.players[slot];need(p.native_grounded && !p.dead && p.native_crouching==(stance=="crouching"),
        "Rendered body scenario left its actual grounded stance");++input_calls;
  };
  options.node_observer=[&](unsigned view,std::uint32_t,const MultiplayerNodeObservation &sample) {
    if(view || sample.mode || sample.owner!=prepared.source_unit || sample.stage!=MultiplayerNodeStage::first_person_completed)return;
    const auto unit_nodes=objects.world_nodes(prepared.source_unit);
    need(sample.nodes.size()==unit_nodes.size() && !unit_nodes.empty() &&
        !std::memcmp(sample.nodes.data(),unit_nodes.data(),unit_nodes.size()*sizeof(Transform)),
        "Rendered looking-down body detached from the original unit WORLD bank");
    body_model=sample.render_model;completed_body.assign(sample.nodes.begin(),sample.nodes.end());++node_calls;
  };
  options.animation_observer=[&](unsigned view,unsigned player,const AnimationRenderObservation &sample) {
    if(view || player!=slot || sample.first_person || sample.unit!=prepared.source_unit)return;
    need(body_model!=null_datum && !completed_body.empty(),"Actual body draw lacks its completed168DED owner");
    need(sample.body_nodes.size()==completed_body.size() &&
        !std::memcmp(sample.body_nodes.data(),completed_body.data(),completed_body.size()*sizeof(Transform)),
        "Endpoint rendering changed the original looking-down body bank");
    if(!authored_body)authored_body=world.cache->load_model(body_model,false);
    const auto expected=skin_model_world_nodes(*authored_body,completed_body);
    need(!expected.empty() && sample.vertices.size()==expected.size(),"Body draw lost authored mode0 geometry");
    for(unsigned n=0;n<expected.size();++n)need(exact(sample.vertices[n].position,expected[n].position),
        "Actual submitted body vertex differs from original unit WORLD skinning");
    body_vertices.assign(sample.vertices.begin(),sample.vertices.end());++body_calls;
  };
  options.view_observer=[&](unsigned view,const Camera &,float horizontal,std::int16_t) {
    need(view==0,"First-person render check gained another view");observed_horizontal_fov=horizontal;
  };
  options.view_state_observer=[&](unsigned view,const MultiplayerState &state,const Camera &actual) {
    need(view==0 && camera_calls,"Actual rendered camera did not execute the original scheduler");
    const auto &p=state.players[slot];const auto frame=objects.observer_world_frame(0,p.source_unit,p.source_flags,DeathCameraSavedClusters{});
    const auto &c=*data.observer_constants;
    source_observer_register_adopt(observer,packet,c);
    source_observer_motion(observer,source_scope_camera_dt(tick_seconds,c.scalar),c);
    source_observer_world(observer,frame,objects.observer_world_services(),c,*data.death_camera_constants,*data.observer_world_constants);
    expected_position=vector(observer,0xb8);expected_forward=vector(observer,0xd8);expected_up=vector(observer,0xe4);
    steep_down=expected_forward.z<-.8f && actual.forward.z<-.8f;
    const auto &biped=world.content.player_representations.at(representation).biped;
    const auto horizontal=gameplay_observer_field_of_view(observed_horizontal_fov,biped.camera_field_of_view,default_field_of_view);
    const auto forward=direction(p.yaw,p.pitch);
    const auto aim=source_observer_aim(forward,cross(forward,{-std::sin(p.yaw),std::cos(p.yaw),0}),
        horizontal,world.content.player_screen_offset,1.3333333730697632f,gameplay_camera_angle_scale);
    const auto projection=gameplay_camera_angles(horizontal,gameplay_field_of_view(default_field_of_view),4.f/3.f);
    const bool presentation_exact=exact(actual.forward,aim.forward) && exact(actual.up,aim.up) &&
        exact(actual.field_of_view,projection.projection_angle*57.295779513f);
    submitted_camera=actual;camera_equal=exact(actual.position,expected_position) && presentation_exact;
    report["original_observer_position"]=xyz(expected_position);report["submitted_camera_position"]=xyz(actual.position);
    report["original_observer_forward"]=xyz(expected_forward);report["submitted_camera_forward"]=xyz(actual.forward);
    report["native_pitch"]=p.pitch;report["steep_down"]=steep_down;
    report["position_difference"]=length(actual.position-expected_position);report["position_exact"]=exact(actual.position,expected_position);
    report["existing_user_fov_aim_policy_exact"]=presentation_exact;++view_calls;
  };
  options.frame_observer=[&](const MultiplayerFrameTiming &frame){submitted+=frame.submitted;};
  options.gameplay_fade_observer=[&](unsigned view,std::uint32_t tick,std::optional<float> alpha,const SourceGameplayFadeStateTu15 &state) {
    need(view==0,"Grounded body fade observation gained another view");
    final_source_fade_clear=!alpha && state.opacity_bits[0]==0 && !(state.activation_mask&1u);
    visible_frames+=final_source_fade_clear;++fade_calls;
    report["source_fade_samples"].push_back({{"tick",tick},{"alpha",alpha?nlohmann::json(*alpha):nlohmann::json(nullptr)},
        {"opacity_bits",state.opacity_bits[0]},{"activation_mask",state.activation_mask},
        {"phase_counter",state.phase_counter},{"source_fade_clear",final_source_fade_clear}});
  };
  report["stage"]="render-grounded-down-look";
  const auto result=run_multiplayer(options);
  report["input_calls"]=input_calls;report["original_scheduler_calls"]=camera_calls;report["view_calls"]=view_calls;
  report["body_calls"]=body_calls;report["body_node_calls"]=node_calls;report["submitted_frames"]=submitted;
  report["source_fade_calls"]=fade_calls;report["source_fade_clear_frames"]=visible_frames;
  report["final_source_fade_clear"]=final_source_fade_clear;
  report["body_vertices"]=body_vertices.size();report["body_render_model"]=body_model;
  unsigned visible_vertices{};
  const auto right=cross(submitted_camera.forward,submitted_camera.up);
  const auto vertical=std::tan(submitted_camera.field_of_view*.017453292519943295f*.5f);
  for(const auto &v:body_vertices) {
    const auto delta=v.position-submitted_camera.position;const auto depth=dot(delta,submitted_camera.forward);
    if(depth>.015625f && std::abs(dot(delta,submitted_camera.up))<depth*vertical &&
        std::abs(dot(delta,right))<depth*vertical*(4.f/3.f))++visible_vertices;
  }
  report["body_vertices_inside_submitted_frustum"]=visible_vertices;
  verify_capture(options.screenshot,options.width,options.height);
  need(result==0 && submitted==capture_frames && input_calls==capture_frames && view_calls==capture_frames,
      "Grounded body runtime did not submit all actual frames through startup fade");
  need(fade_calls==capture_frames && visible_frames && final_source_fade_clear,
      "Original source fade did not finish naturally before the actual body capture");
  need(steep_down,"Original/submitted observers did not reach a steep down-look in the render scenario");
  need(body_calls==capture_frames && node_calls==capture_frames && visible_vertices,
      "Steep native down-look did not submit visible looking-down body geometry");
  need(camera_equal,"Rendered camera discarded original on-foot eye placement or changed the existing user-FOV aim policy");
  report["passed"]=true;report["stage"]="complete";save(argv[5]);
  std::cout<<"PASS actual grounded down-look visible frame representation="<<representation<<" stance="<<stance
      <<" vertices="<<body_vertices.size()<<" frustum_vertices="<<visible_vertices<<" capture="<<options.screenshot<<'\n';return 0;
}catch(const std::exception &error) {
  report["error"]=error.what();try{if(argc==6)save(argv[5]);}catch(const std::exception &e){std::cerr<<e.what()<<'\n';}
  std::cerr<<"First-person body render stage "<<report["stage"]<<": "<<error.what()<<'\n';return 1;
}
