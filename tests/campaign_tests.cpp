#include "halo2/simulation.hpp"
#include "halo2/audio.hpp"
#include "halo2/residency.hpp"
#include <iostream>
#include <algorithm>
#include <bit>
#include <sstream>
using namespace halo2;
namespace {
void check(bool result, const char *message) {
  if (!result) throw std::runtime_error(message);
}
template<class F> void rejects(F f, const char *message) {
  try { f(); } catch (const FormatError &) { return; }
  throw std::runtime_error(message);
}
struct Program {
  ScriptProgram data;
  std::uint32_t node(ScriptNode n) {
    const auto i = std::uint32_t(data.nodes.size());
    n.salt = std::uint16_t(0xe000 + i);
    data.nodes.push_back(n);
    return (std::uint32_t(n.salt) << 16) | i;
  }
  std::uint32_t scalar(int value, unsigned type = 8) {
    return node({0, std::uint16_t(type), std::uint16_t(type), 9,
                 null_datum, 0, std::uint32_t(value)});
  }
  std::uint32_t global(unsigned i, unsigned type = 8) {
    return node({0, std::uint16_t(type), std::uint16_t(type), 13,
                 null_datum, 0, i});
  }
  std::uint32_t real(float value) {
    return scalar(std::bit_cast<std::uint32_t>(value),6);
  }
  std::uint32_t text(const std::string &value) {
    const auto at=std::uint32_t(data.strings.size());
    data.strings+=value; data.strings+='\0';
    return node({0,9,9,9,null_datum,at,0});
  }
  std::uint32_t call(const std::string &name,
                     std::vector<std::uint32_t> args = {}, unsigned type = 4) {
    const auto at = std::uint32_t(data.strings.size());
    data.strings += name;
    data.strings += '\0';
    for (std::size_t i = 0; i < args.size(); ++i)
      data.nodes[args[i] & 65535].next = i + 1 < args.size() ? args[i+1] : null_datum;
    const auto fn = node({0, 0, 2, 9,
                         args.empty() ? null_datum : args[0], at, 0});
    return node({0, 0, std::uint16_t(type), 8, null_datum, at, fn});
  }
  void script(const std::string &name, std::uint32_t root, unsigned type = 0) {
    data.scripts.push_back({name, std::uint16_t(type), 4, root});
  }
  std::uint32_t invoke(const std::string &name, unsigned type) {
    const auto found = std::find_if(data.scripts.begin(), data.scripts.end(),
        [&](const auto &script) { return script.name == name; });
    if (found == data.scripts.end()) throw FormatError("missing authored helper: " + name);
    const auto index = std::uint16_t(found - data.scripts.begin());
    const auto root = call(name, {}, type);
    auto &n = data.nodes[root & 65535];
    n.flags |= 2;
    n.opcode = index;
    return root;
  }
};
void vm_checkpoint() {
  Program p;
  p.data.globals.push_back({"counter", 8, p.scalar(0)});
  p.script("main", p.call("begin", {
      p.call("set", {p.global(0), p.scalar(11)}, 8),
      p.call("sleep", {p.scalar(3)}),
      p.call("set", {p.global(0), p.scalar(29)}, 8),
      p.call("record", {p.global(0)})}));
  int calls = 0;
  ScriptVm vm(p.data, [&](const auto &, const auto &args) {
    check(args.at(0).number() == 29, "resumed nested expression result");
    ++calls;
    return ScriptValue{};
  });
  vm.start_level();
  vm.tick(1);
  const auto saved = vm.snapshot();
  vm.tick(3);
  check(calls == 0, "sleep does not expire early");
  vm.tick(4);
  check(calls == 1 && vm.globals()[0].number() == 29 && vm.running() == 0,
        "script finishes before restore");
  vm.restore(saved);
  check(vm.globals()[0].number() == 11 && vm.running() == 1,
        "checkpoint restores globals and active stack");
  vm.tick(3);
  check(calls == 1, "restore preserves absolute wake deadline");
  vm.tick(4);
  check(calls == 2 && vm.diagnostics().empty(), "checkpoint replays suspended continuation");
  auto other = p.data;
  ScriptVm unrelated(other, [](const auto &, const auto &) { return ScriptValue{}; });
  rejects([&] { unrelated.restore(saved); }, "cross-program checkpoint rejected");
  rejects([&] { vm.restore({}); }, "empty checkpoint rejected");
}
void dispatch_boundary() {
  Program p;
  p.script("first", p.call("begin", {p.call("request"), p.call("later")}));
  p.script("second", p.call("later"));
  int calls = 0;
  ScriptVm *active = nullptr;
  ScriptVm vm(p.data, [&](const auto &name, const auto &) {
    if (name == "request") {
      rejects([&] { active->snapshot(); }, "mid-call checkpoint rejected");
      active->suspend_tick();
    } else ++calls;
    return ScriptValue{};
  });
  active = &vm;
  vm.start_level();
  vm.tick(1);
  check(calls == 0, "restore request stops later calls and other threads");
  const auto saved = vm.snapshot();
  vm.tick(2);
  check(calls == 2, "finished host call is not repeated next tick");
  vm.restore(saved);
  vm.tick(2);
  check(calls == 4, "safe snapshot captures post-call continuation");
}
void poll_boundaries() {
  Program p;
  p.data.globals.push_back({"ready",5,p.scalar(0,5)});
  p.script("global_poll", p.call("begin", {
      p.call("sleep_until", {p.global(0,5),p.scalar(2,7)}), p.call("done")}));
  p.script("change", p.call("begin", {p.call("sleep",{p.scalar(1)}),
      p.call("set",{p.global(0,5),p.scalar(1,5)},5)}));
  int done = 0;
  ScriptVm vm(p.data,[&](const auto &,const auto &) { ++done;return ScriptValue{}; });
  vm.start_level();
  vm.tick(0); vm.tick(1); vm.tick(2);
  check(done == 0, "primitive global predicate sampled before poll sleep");
  vm.tick(4);
  check(done == 1, "global predicate refreshed on following poll");
  Program timeout;
  timeout.script("timeout",timeout.call("sleep_until",{
      timeout.call("must_not_run",{},5),timeout.scalar(1,7),timeout.scalar(0)}));
  ScriptVm zero(timeout.data,[&](const auto &,const auto &) {
    throw FormatError("zero-timeout predicate was evaluated");return ScriptValue{};
  });
  zero.start_level(); zero.tick(0);
  check(zero.diagnostics().empty() && !zero.running(),
        "zero timeout completes without evaluating nonliteral predicate");
}
void scene_checkpoint() {
  LevelContent c;
  c.object_names.push_back({"prop", 6, 0});
  ScenarioObject prop;
  prop.type = 6; prop.definition = 42; prop.name = 0;
  prop.position = {1, 2, 3};
  c.objects.push_back(prop);
  SceneObjects scene(c, 0);
  const auto handle = scene.resolve(0);
  scene.set_function_variable(handle,17,.2f,0,0);
  const auto saved = scene;
  scene.set_function_variable(handle,17,.9f,0,0);
  const auto replacement = scene.create(0, true);
  check(handle != replacement, "object recreated with new generation");
  scene.restore(saved);
  check(scene.resolve(0) == handle && scene.get(handle).position.x == 1,
        "checkpoint restores object transform and name generation");
  check(scene.function_variable(handle,17,0) == .2f,
        "checkpoint restores object function state alongside its generation");
  rejects([&] { scene.get(replacement); }, "post-save generation invalid after restore");
  Presentation view(c);
  auto supplied_data = std::make_shared<OriginalData>();
  for (unsigned i = 0; i < supplied_data->fade_curve.size(); ++i)
    supplied_data->fade_curve[i] = std::uint8_t(i * 255 / 1023);
  c.original_data = supplied_data;
  view.cinematic_control(true);
  view.scripted = true;
  view.player_camera_enabled = false;
  view.fade({1, 0, 0}, true, 30);
  for (int i = 0; i < 10; ++i) view.tick();
  const auto presentation = view;
  const auto alpha = view.effects().fade;
  for (int i = 0; i < 20; ++i) view.tick();
  view.cinematic_control(false);
  view.restore(presentation);
  check(view.cinematic && view.scripted && !view.player_camera_enabled &&
            view.effects().fade == alpha,
        "checkpoint restores cinematic state and transition clock");
}
LevelContent arena() {
  LevelContent c;
  c.collision = {{{-100,-100,0},{100,-100,0},{100,100,0},0,0},
                 {{-100,-100,0},{100,100,0},{-100,100,0},0,0}};
  c.player.datum = 1;
  c.player.height = 1; c.player.crouch_height = .6f;
  c.player.radius = .2f; c.player.camera_height = .8f;
  c.player.crouch_camera_height = .5f;
  c.player.vitality.health = c.player.vitality.shield = 100;
  c.movement = {2,2,2,10,1,1,1,5,2,1};
  c.cutscene_flags.push_back({"inside_cutscene", {10,20,0},0,0});
  c.object_names.push_back({"prop",6,0});
  ScenarioObject prop;
  prop.type = 6; prop.definition = 42; prop.name = 0;
  c.objects.push_back(prop);
  return c;
}
void tutorial_controls_and_checkpoint() {
  auto c=arena();Program p;
  p.data.globals.push_back({"inverted",5,p.scalar(0,5)});
  p.data.globals.push_back({"look",5,p.scalar(0,5)});
  p.script("train",p.call("begin",{
      p.call("hud_enable_training",{p.scalar(0,5)}),
      p.call("hud_set_training_text",{p.scalar(0x1000123,11)}),
      p.call("hud_show_training_text",{p.scalar(1,5)}),
      p.call("player_training_activate_crouch"),
      p.call("controller_set_look_invert",{p.scalar(1,5)}),
      p.call("set",{p.global(0,5),p.call("controller_get_look_invert",{},5)},5),
      p.call("player_action_test_reset"),p.call("player_action_test_look_up_begin")}));
  p.script("observe",p.call("set",{p.global(1,5),p.call("player_action_test_look_relative_up",{},5)},5));
  p.script("finish",p.call("begin",{p.call("player_action_test_reset"),p.call("player_action_test_look_pitch_end"),
      p.call("hud_show_training_text",{p.scalar(0,5)})}));
  c.scripts=p.data;Simulation sim(c,0,{{0,0,0},0,0});bool inverted=false;
  sim.controller_look_settings([&]{return inverted;},[&](bool v){inverted=v;});
  sim.run_script("train");sim.step({});
  check(sim.scripts().diagnostics().empty() && inverted && sim.scripts().globals()[0].truth(),
      "Tutorial inversion must reach the controller profile owner");
  check(!sim.training().enabled && sim.training().show && sim.training().text==0x1000123 && sim.training().requests==2,
      "Training text visibility, event requests and automatic-training enable are independent source fields");
  Controls input;input.yaw=.2f;input.pitch=-.3f;sim.step(input);
  check(sim.player().yaw==0 && std::abs(sim.player_pitch()-.3f)<1e-6,
      "Look-up calibration must block yaw and make pitch positive");
  input.yaw=0;input.pitch=.4f;sim.run_script("observe");sim.request_checkpoint();sim.step(input);
  check(sim.scripts().globals()[1].truth(),"Relative up must record pitch rather than yaw");
  sim.run_script("finish");sim.step(input);
  check(!sim.training().show && sim.training().suppressed==0,"Pitch end must release both source yaw restrictions");
  sim.request_revert();sim.step({});
  check(sim.training().show && sim.training().text==0x1000123 && sim.training().suppressed==0x3000,
      "Checkpoint must retain the tutorial prompt and calibration restrictions");
}
void campaign_create_containing() {
  auto c=arena();
  c.object_names[0].name="e1_crate";
  auto other=c.objects[0];other.name=1;other.placement=1;other.definition=43;
  c.objects.push_back(other);c.object_names.push_back({"E1_crate",6,1});
  c.object_names.push_back({"e1_unused",6,-1});
  Program p;
  p.script("destroy_first",p.call("object_destroy",{p.node({0,56,50,9,null_datum,0,0})}));
  p.script("destroy_second",p.call("object_destroy",{p.node({0,56,50,9,null_datum,0,1})}));
  p.script("create",p.call("object_create_containing",{p.text("e1_")}));
  p.script("all",p.call("object_create_containing",{p.text("")}));
  c.scripts=p.data;Simulation sim(c,0,{{0,0,0},0,0});
  const auto other_handle=sim.scene().resolve(1);
  sim.run_script("destroy_first");sim.step({});
  sim.run_script("create");sim.step({});
  check(sim.scripts().diagnostics().empty() && sim.scene().resolve(0)!=null_datum,
        "Campaign containing creation must recreate matching placements and skip unbound names");
  const auto handle=sim.scene().resolve(0);
  check(sim.scene().resolve(1)==other_handle && sim.scene().resolve(2)==null_datum,
        "Containing creation preserves case-sensitive selection and unbound names");
  sim.run_script("create");sim.step({});
  check(sim.scene().resolve(0)==handle,"Containing creation retains existing generations");
  sim.run_script("destroy_second");sim.step({});sim.run_script("all");sim.step({});
  check(sim.scripts().diagnostics().empty() && sim.scene().resolve(1)!=null_datum,
        "Empty containing search creates every bound scenario name");
}
void bsp_builtin_boundary() {
  auto c = arena();
  Program p;
  p.script("same_bsp",p.call("switch_bsp_by_name",{p.scalar(0,26)}));
  c.scripts = p.data;
  Simulation sim(c,0,{{0,0,0},0,0});
  sim.run_script("same_bsp"); sim.step({});
  check(sim.scripts().diagnostics().empty(),
        "compiled BSP ordinal is handled by the real streaming request service");
}
void bsp_streaming_checkpoint() {
  auto c=arena(); c.bsp_count=2;
  Program p;
  p.data.globals.push_back({"continuation",8,p.scalar(0)});
  p.script("travel",p.call("begin",{
      p.call("sleep",{p.scalar(1)}),
      p.call("switch_bsp_by_name",{p.scalar(1,26)}),
      p.call("set",{p.global(0),p.scalar(13)},8),
      p.call("sleep",{p.scalar(3)}),
      p.call("set",{p.global(0),p.scalar(29)},8)}));
  c.scripts=p.data;
  int loads=0;
  Simulation sim(c,0,{{0,0,0},0,0},{},[&](std::size_t target) {
    check(target==1,"BSP loader receives compiled ordinal"); ++loads;
    BspContent next{c.collision,c.navigation};
    for (auto &t:next.collision) t.a.z=t.b.z=t.c.z=-10;
    return next;
  });
  sim.run_script("travel"); sim.request_checkpoint(); sim.step({});
  check(sim.bsp()==0 && sim.has_checkpoint(),"save before streaming retains original BSP");
  sim.step({});
  const auto hit=sim.collision().raycast({0,0,1},{0,0,-1},20);
  check(sim.bsp()==1 && sim.bsp_revision()==1 && loads==1 && hit &&
            hit->position.z==-10 && sim.scripts().globals()[0].number()==13 &&
            sim.scripts().running()==1 && sim.scripts().diagnostics().empty(),
        "streaming replaces collision and preserves script globals and continuation");
  sim.request_revert(); sim.step({});
  const auto old=sim.collision().raycast({0,0,1},{0,0,-1},20);
  check(sim.bsp()==0 && sim.bsp_revision()==2 && old && old->position.z==0 &&
            loads==1 && sim.ticks()==1 && sim.scripts().globals()[0].number()==0,
        "cross-BSP revert restores owned geometry and VM at saved tick");
  sim.step({});
  check(sim.bsp()==1 && loads==2,"restored authored continuation replays streaming request");
  sim.request_bsp(0); sim.request_bsp(1); sim.step({});
  check(sim.bsp()==1 && loads==2,"requesting current BSP cancels pending transition");
  sim.request_bsp(0); sim.request_bsp(99); sim.request_revert(); sim.step({});
  check(sim.bsp()==0 && loads==2,"revert cancels pending invalid-overwritten BSP request");
}
void bsp_scene_lifetime() {
  auto c=arena(); c.objects.clear(); c.object_names.clear(); c.bsp_count=2;
  c.trigger_volumes.push_back({"bridge",-1,0,{1,0,0},{0,0,1},{0,0,0},{10,10,10}});
  c.bsp_transitions.push_back({0x10000,0});
  auto add=[&](int name,int source,int origin,int policy,unsigned mask,Vec3 position) {
    ScenarioObject o; o.type=6; o.definition=42; o.placement=int(c.objects.size());
    o.name=name; o.source=source; o.origin_bsp=origin; o.bsp_policy=policy;
    o.manual_bsps=mask; o.position=position; o.flags=4;
    c.objects.push_back(o);
    if (name>=0) c.object_names.push_back({"prop"+std::to_string(name),6,o.placement});
  };
  add(0,0,0,0,0,{20,20,1}); // named origin-scoped persistent object
  add(1,1,1,2,2,{20,20,1}); // named manual mask
  add(-1,1,0,1,0,{20,20,1}); // unnamed automatic scenery outside volume
  add(2,1,0,1,0,{1,1,1}); // named any-BSP object inside transition volume
  SceneObjects scene(c,0);
  const auto a=scene.resolve(0), b=scene.resolve(1), inside=scene.resolve(2);
  check(scene.get(a).visible && !scene.get(b).visible && scene.get(inside).visible,
        "initial named entries are created then hidden by BSP policy; flag4 is admitted");
  std::uint32_t automatic=null_datum;
  for (const auto &o:scene.objects()) if (o.handle!=null_datum && o.placement==2) automatic=o.handle;
  scene.switch_bsp(1);
  check(!scene.get(a).visible && scene.get(a).bsp_hidden &&
            scene.get(b).visible && scene.get(inside).visible,
        "origin/manual objects hide and reactivate; interior transition object survives");
  rejects([&]{scene.get(automatic);},"outside unnamed scenery destroys its old generation");
  scene.switch_bsp(0);
  check(scene.resolve(0)==a && scene.get(a).visible && !scene.get(b).visible,
        "returning BSP preserves persistent generation and reactivates authored object");
  scene.visible(a,false); scene.switch_bsp(1); scene.switch_bsp(0);
  check(!scene.get(a).visible && !scene.get(a).bsp_hidden && scene.get(a).script_hidden,
        "BSP reactivation preserves the independent script visibility flag");
  check(!scene.retains(0,1,{0,1,1}) && scene.retains(0,1,{1,1,1}),
        "retention uses strict trigger interior, including boundary exclusion");
}
void class_fade_and_live_mixer() {
  SoundClassFades f;
  f.set("machinery",0,.5f);
  check(f.state()[28].target==-64 && f.state()[34].target==-64 &&
            f.state()[33].target==0 && f.state()[28].current==0,
        "retail case-sensitive substring matching selects classes without changing other ambience");
  f.advance(.1f);
  check(f.state()[28].current==-12.8f,"original single-precision fade at .1 seconds");
  f.advance(.15f);
  check(std::abs(f.state()[28].current+32)<.00001f,
        "fade halfway interpolates decibels rather than linear amplitude");
  check(std::bit_cast<std::uint32_t>(sound_db_to_linear(-64))==0x3a2566d2 &&
            std::bit_cast<std::uint32_t>(sound_db_to_linear(-32))==0x3ccdc612,
        "amplitude conversion agrees with original x87 golden bits");
  const auto saved=f;
  f.set("machinery",1,.5f); f.advance(.25f);
  check(std::abs(f.state()[28].current+16)<.00001f,"interrupted fade begins from its current dB value");
  f.set("machinery",0,-1);
  check(std::abs(f.state()[28].current+16)<.00001f && f.state()[28].remaining==0,
        "negative duration waits for next positive audio clock update");
  f.advance(0); f.advance(.001f);
  check(f.state()[28].current==-64,"positive audio update snaps negative-duration target");
  SDL_setenv("SDL_AUDIODRIVER","dummy",1);
  AudioEngine audio(false);
  auto tone=std::make_shared<PcmSound>(); tone->stereo.assign(32,.5f); tone->sound_class=28;
  audio.play(tone,1,0,true);
  auto output=audio.render(4);
  check(output[0]==.5f,"live voice begins at full class gain");
  audio.class_gains(saved.gains()); output=audio.render(4);
  check(std::abs(output[0]-.5f*sound_db_to_linear(-32))<1e-7f,
        "class fade changes an already-playing voice at mix time");
  audio.stop();
  tone->sound_class=33; audio.play(tone,1,0,true); output=audio.render(4);
  check(output[0]==.5f,"unselected newly-created voice preserves actual snd class");
  audio.stop();
  tone->class_spans={{2,28},{4,33}};
  audio.play(tone); output=audio.render(4);
  check(output[0]<.02f && output[2]<.02f && output[4]==.5f && output[6]==.5f,
        "concatenated nonlooping phases retain their own sound class boundaries");
}
void campaign_configuration_checkpoint() {
  auto c=arena(); Program p;
  p.script("configure",p.call("begin",{
      p.call("objectives_finish_up_to",{p.scalar(0)}),
      p.call("objectives_show_up_to",{p.scalar(2)}),
      p.call("objectives_finish_up_to",{p.scalar(8)}),
      p.call("objectives_finish_up_to",{p.scalar(65538)}),
      p.call("game_can_use_flashlights",{p.scalar(0,5)}),
      p.call("cinematic_enable_ambience_details",{p.scalar(0,5)}),
      p.call("sound_class_set_gain",{p.text("device_machinery"),p.real(0),p.scalar(15,7)})}));
  p.script("erase",p.call("begin",{
      p.call("objectives_clear"),
      p.call("game_can_use_flashlights",{p.scalar(1,5)}),
      p.call("sound_class_set_gain",{p.text("device_machinery"),p.real(1),p.scalar(0,7)})}));
  c.scripts=p.data; Simulation sim(c,0,{{0,0,0},0,0});
  sim.run_script("configure"); sim.step({});
  check(sim.scripts().diagnostics().empty() && sim.objectives()==std::array<int,5>{2,1,1,0,0} &&
            !sim.flashlights_allowed() && !sim.ambience_details_enabled(),
        "original objective/configuration hosts update native campaign state");
  sim.advance_audio(.25f); sim.request_checkpoint(); sim.step({});
  sim.run_script("erase"); sim.step({});
  sim.advance_audio(.25f); sim.request_revert(); sim.step({});
  check(sim.objectives()==std::array<int,5>{2,1,1,0,0} && !sim.flashlights_allowed() &&
            sim.sound_classes().state()[28].current==-32 &&
            sim.sound_classes().state()[28].remaining==.25f,
        "checkpoint restores objectives, permissions and in-flight wall-clock class envelope");
}
void retail_external_long_expression() {
  auto c=arena(); Program p;
  p.script("retail_long",p.call("set",{p.global(0x83e3,8),p.scalar(2,8)},8));
  c.scripts=p.data; Simulation sim(c,0,{{0,0,0},0,0});
  sim.run_script("retail_long"); sim.step({});
  check(sim.scripts().diagnostics().empty(),
        "retail external short accepts authored long expression and converts at binding");
}
void campaign_residency_requests() {
  auto c=arena(); Program p;
  p.script("predict",p.call("begin",{
      p.call("sound_impulse_predict",{p.scalar(42,32)}),
      p.call("sound_impulse_predict",{p.scalar(-1,32)}),
      p.call("predict_model_section",{p.scalar(43,41),p.scalar(-1)}),
      p.call("predict_model_section",{p.scalar(-1,41),p.scalar(0)}),
      p.call("predict_structure_section",{p.scalar(0,26),p.scalar(2),p.scalar(0,5)}),
      p.call("predict_structure_section",{p.scalar(0,26),p.scalar(-1),p.scalar(1,5)}),
      p.call("predict_structure_section",{p.scalar(1,26),p.scalar(2),p.scalar(1,5)}),
      p.call("object_type_predict",{p.scalar(44,38)}),
      p.call("object_type_predict",{p.scalar(-1,38)})}));
  c.scripts=p.data; std::vector<Simulation::Prediction> requests;
  Simulation sim(c,0,{{0,0,0},0,0},{},{},[&](const auto &r){requests.push_back(r);});
  sim.run_script("predict"); sim.step({});
  check(sim.scripts().diagnostics().empty() && requests.size()==4 &&
            requests[0].kind==Simulation::Prediction::Sound && requests[0].datum==42 &&
            requests[1].kind==Simulation::Prediction::Model && requests[1].section==-1 &&
            requests[2].kind==Simulation::Prediction::Structure && !requests[2].instances &&
            requests[3].kind==Simulation::Prediction::Object && requests[3].datum==44,
        "prediction uses real residency service, admits object/model-all and rejects NONE/noncurrent");
}
void campaign_look_cone() {
  // Retail29F850 ->29F7A0 ->CC2B0 tests facing only. A wall or the
  // target machine's own collision must not reject the Armory's light test.
  for(const bool wall:{false,true})for(const float angle:{0.f,10.f}) {
    auto c=arena();c.objects[0].position={10,0,c.player.camera_height+.002f};
    if(wall) {
      c.collision.push_back({{5,-10,-10},{5,10,-10},{5,10,10}});
      c.collision.push_back({{5,-10,-10},{5,10,10},{5,-10,10}});
    }
    Program p;p.data.globals.push_back({"seen",5,p.scalar(0,5)});
    p.script("look",p.call("set",{p.global(0,5),p.call("objects_can_see_object",{
        p.call("players",{},31),p.node({0,56,50,9,null_datum,0,0}),p.real(angle)},5)},5));
    c.scripts=p.data;Simulation simulation(c,0,{{0,0,0},0,0});
    simulation.run_script("look");simulation.step({});
    check(simulation.scripts().diagnostics().empty() &&
        simulation.scripts().globals()[0].truth()==(angle>0),
        "Campaign facing cone added occlusion or lost the strict original angle boundary");
  }
}
void add_enemy(LevelContent &c);
void add_enemy(LevelContent &c);
void cinematic_object_cleanup() {
  auto c=arena(); add_enemy(c); Program p;
  p.script("cleanup",p.call("object_destroy_type_mask",{p.scalar(0)}));
  c.scripts=p.data; Simulation sim(c,0,{{0,0,0},0,0}); sim.place_squad("enemy");
  const auto handle=sim.scene().resolve(0);
  sim.request_checkpoint(); sim.step({});
  sim.run_script("cleanup"); sim.step({});
  check(sim.scripts().diagnostics().empty() && sim.scene().resolve(0)==null_datum &&
            !sim.actors()[0].exists && !sim.player().vitality.dead && sim.player().exists,
        "zero type mask means all roots while native player and equipment stay protected");
  sim.request_revert(); sim.step({});
  check(sim.actors()[0].exists && sim.scene().resolve(0)==handle,
        "checkpoint restores deleted cinematic objects and actor identities");
}
void add_enemy(LevelContent &c) {
  WeaponDefinition w;
  w.datum = 3; w.instantaneous = true; w.capacity = 50;
  w.reserve = 100; w.reload_rounds = 50; w.reload_time = 1;
  w.rounds_per_shot = w.minimum_rounds = w.burst = w.projectiles_per_shot = 1;
  w.rounds_per_second = 30; w.range = 50; w.damage_min = 25;
  c.weapons.emplace(3,w); c.starting_weapon = 3;
  CharacterDefinition character;
  character.datum = 2; character.biped = c.player;
  character.biped.datum = 4; character.biped.team = 3;
  character.vision_distance = 30; character.vision_angle = 1.5f;
  character.acknowledgement = .1f;
  c.characters.emplace(2,character);
  SquadDefinition squad;
  squad.name = "enemy"; squad.count = 1; squad.bsp = 0; squad.team = 3;
  squad.starts.push_back({{2,0,0},3.14159265f,0,2,3,{}});
  c.squads.push_back(squad);
}
void tutorial_actor_commands_and_text() {
  auto c=arena();add_enemy(c);c.command_points={{{{3,0,0},-1,-1}}};
  c.cinematic_titles.push_back({123,0xffffff,0,{0,0,480,640},1,9,1,4,1});
  Program p;p.data.globals.push_back({"completed",5,p.scalar(0,5)});
  p.script("walk",p.call("begin",{p.call("cs_go_to",{p.scalar(0,28)}),
      p.call("set",{p.global(0,5),p.scalar(1,5)},5),p.call("sleep_forever")}),5);
  p.script("setup",p.call("begin",{
      p.call("ai_cannot_die",{p.scalar(0,19),p.scalar(1,5)}),
      p.call("cs_run_command_script",{p.scalar(0,19),p.scalar(0,21)}),
      p.call("cinematic_subtitle",{p.scalar(123,11),p.real(2)}),
      p.call("cinematic_set_title",{p.scalar(0,16)})}));
  c.scripts=p.data;Simulation sim(c,0,{{0,0,0},0,0});sim.place_squad("enemy");
  sim.cinematic_text_services([](unsigned id){return id==123;},[](unsigned){return 2.f;});
  sim.run_script("setup");sim.step({});sim.step({});
  check(sim.scripts().diagnostics().empty() && !sim.scripts().globals()[0].truth(),"Actor command must yield until real movement completes");
  check(sim.actors()[0].source_datum!=null_datum && sim.subtitle().text==123 && sim.titles()[0].index==0,
      "Real actor handle and cinematic text owners");
  sim.request_checkpoint();sim.step({});const auto remaining=sim.subtitle().remaining;
  for(unsigned i=0;i<90;++i)sim.step({});
  check(sim.scripts().diagnostics().empty() && sim.scripts().globals()[0].truth() && sim.actors()[0].body.position.x>2.5f,
      "Actor reaches authored command point before script resumes");
  sim.request_revert();sim.step({});
  check(!sim.scripts().globals()[0].truth() && sim.subtitle().remaining==remaining && sim.actors()[0].command.goal,
      "Checkpoint restores actor wait and cinematic timer together");
  for(unsigned i=0;i<160;++i)sim.step({});
  check(sim.scripts().diagnostics().empty() && !sim.subtitle().text && sim.titles()[0].index==-1,
      "Checkpoint replay expires both source text owners");
}
void protection_flags() {
  for (bool vitality_protection : {false,true}) {
    auto c = arena(); add_enemy(c);
    Program p;
    p.script("protect",vitality_protection ?
        p.call("object_cannot_take_damage",{p.scalar(0x80000000u,31)}) :
        p.call("unit_impervious",{p.scalar(0x80000000u,31),p.scalar(1,5)}));
    p.script("unprotect",p.call("object_can_take_damage",{p.scalar(0x80000000u,31)}));
    c.scripts = p.data;
    Simulation sim(c,0,{{0,0,0},0,0}); sim.place_squad("enemy");
    sim.run_script("protect");
    for (int i=0;i<30;++i) sim.step({});
    check(sim.scripts().diagnostics().empty() && sim.shots() > 0 &&
              (vitality_protection || sim.hits() > 0),
          "damage-protection fixture has an attacking enemy");
    check(sim.player().vitality.dead != vitality_protection,
          "object damage protection grants immunity; unit impervious does not");
    if (vitality_protection) {
      sim.run_script("unprotect");
      for (int i=0;i<30;++i) sim.step({});
      check(sim.player().vitality.dead, "object_can_take_damage restores damage");
    }
  }
}
void campaign_fixed_upper_damage() {
  auto c=arena();add_enemy(c);
  auto &weapon=c.weapons.at(3);
  weapon.damage_min=0;
  weapon.damage_amount=DamageAmountDefinition{0,25,25};
  Simulation simulation(c,0,{{0,0,0},0,0});simulation.place_squad("enemy");
  for(unsigned step=0;step<30;++step)simulation.step({});
  check(simulation.shots()>0 && simulation.hits()>0 && simulation.player().vitality.dead,
      "Campaign direct hits used the lower envelope instead of the fixed authored upper amount");
}
void checkpoint_audio_and_faults() {
  auto c = arena();
  Program p;
  p.script("loop",p.call("begin", {
      p.call("sound_looping_start",{p.scalar(42,35),p.scalar(-1,50),p.scalar(1)}),
      p.call("sound_looping_set_alternate",{p.scalar(42,35),p.scalar(1,5)})}));
  p.script("discarded",p.call("begin", {
      p.call("sound_looping_stop",{p.scalar(42,35)}),
      p.call("sound_impulse_start",{p.scalar(43,32),p.scalar(-1,50),p.scalar(1)}),
      p.call("unsupported_fixture_service")}));
  c.scripts = p.data;
  Simulation sim(c,0,{{0,0,0},0,0});
  sim.run_script("loop"); sim.request_checkpoint(); sim.step({}); sim.take_sounds();
  sim.run_script("discarded"); sim.step({}); sim.take_sounds();
  check(sim.scripts().diagnostics().size() == 1, "discarded timeline fault recorded");
  sim.request_revert(); sim.step({});
  const auto restored = sim.take_sounds();
  check(restored.size() == 3 && restored[0].action == SoundEvent::Reset &&
            restored[1].action == SoundEvent::LoopStart && restored[1].datum == 42 &&
            restored[2].action == SoundEvent::LoopAlternate && restored[2].alternate,
        "checkpoint stops discarded audio and restarts saved loop and alternate");
  check(sim.scripts().diagnostics().size() == 1,
        "restoring a checkpoint cannot conceal observed script faults");
}
void player_controls_and_external_global() {
  auto c = arena(); add_enemy(c);
  Program p;
  p.data.globals.push_back({"observed_style",7,p.scalar(-1,7)});
  p.script("controls",p.call("begin",{
      p.call("player_disable_movement",{p.scalar(1,5)}),
      p.call("object_hide",{p.scalar(0,50),p.scalar(1,5)}),
      p.call("set",{p.global(0x83e3,7),p.scalar(1,7)},7),
      p.call("set",{p.global(0,7),p.global(0x83e3,7)},7)}));
  p.script("unknown_external",p.call("set",{p.global(0x8000,7),p.scalar(1,7)},7));
  c.scripts = p.data;
  Simulation sim(c,0,{{0,0,0},0,0}); sim.run_script("controls");
  Controls input; input.forward = 1; input.yaw = .5f; input.fire = true;
  for (int i=0;i<10;++i) sim.step(input);
  check(sim.scripts().diagnostics().empty() && sim.player().body.position.x == 0 &&
            sim.player().body.position.y == 0 && sim.player().yaw == .5f &&
            sim.shots() > 0 && !sim.player().visible,
        "movement disabling preserves camera and weapon input independently");
  check(sim.scripts().globals()[0].number() == 0,
        "null-backed retail letterbox global ignores write and refreshes zero");
  sim.run_script("unknown_external"); sim.step({});
  check(sim.scripts().diagnostics().size() == 1,
        "unknown external global fails explicitly");
}
std::string world_state(const Simulation &sim) {
  std::ostringstream out;
  auto scalar = [&](float value) { out << std::bit_cast<std::uint32_t>(value) << ','; };
  auto vector = [&](Vec3 v) { scalar(v.x);scalar(v.y);scalar(v.z); };
  auto unit = [&](const Unit &u) {
    vector(u.body.position); vector(u.body.velocity); vector(u.body.support_normal);
    scalar(u.yaw); scalar(u.crouch); scalar(u.awareness);
    scalar(u.vitality.body); scalar(u.vitality.shield);
    scalar(u.weapon.delay); scalar(u.weapon.reload);
    out << u.body.grounded << ',' << u.vitality.dead << ','
        << u.vitality.body_stun << ',' << u.vitality.shield_stun << ','
        << u.weapon.loaded << ',' << u.weapon.reserve << ',' << u.weapon.burst_left
        << ',' << u.weapon.last_shot << ',' << u.jump.since_jump << ','
        << u.jump.held_ticks << ',' << u.state << ',' << u.target << ',' << u.waypoint;
    for (const auto &point : u.route) vector(point);
  };
  out << sim.ticks() << ',' << sim.shots() << ',' << sim.hits() << ',';
  scalar(sim.player_pitch());
  unit(sim.player());
  for (const auto &actor : sim.actors()) unit(actor);
  // Includes active projectile positions and the current actor debug geometry.
  for (const auto &vertex : sim.debug_mesh()) vector(vertex.position);
  const auto& lifetimes=sim.object_lifetimes();const auto& pool=lifetimes.pool();
  out<<pool.cursor()<<','<<pool.high_water()<<','<<pool.count()<<','<<pool.next_salt()<<',';
  for(const auto word:pool.bitmap())out<<word<<',';
  for(const auto byte:pool.elements())out<<unsigned(byte)<<',';
  for(const auto& event:lifetimes.events()) {
    const auto& owner=event.owner;
    out<<event.ordinal<<','<<int(event.kind)<<','<<owner.handle<<','<<owner.definition<<','
       <<unsigned(owner.type)<<','<<owner.published<<','<<owner.allocation_ordinal<<','
       <<owner.publication_ordinal<<',';
  }
  const auto& bodies=sim.source_object_bodies();
  for(const auto b:bodies.arena())out<<std::to_integer<unsigned>(b)<<',';
  for(const auto b:bodies.scratch())out<<std::to_integer<unsigned>(b)<<',';
  for(const auto h:bodies.handles())out<<h<<',';
  for(const auto& object:sim.scene().objects()) {
    out<<object.handle<<','<<object.placement<<','<<object.created_tick<<',';
    vector(object.position);
  }
  return out.str();
}
void initial_geometry_ownership() {
  auto copied=arena(),transferred=copied;
  Simulation a(copied,0,{{0,0,0},0,0});
  BspContent geometry{std::move(transferred.collision),transferred.navigation};
  const auto count=geometry.collision.size();
  Simulation b(transferred,std::move(geometry),0,{{0,0,0},0,0});
  check(transferred.collision.empty() && b.collision().size()==count,
        "Transferred BSP keeps the complete collision bank in its world owner");
  Controls input;input.forward=.7f;input.right=.2f;
  for(unsigned i=0;i<90;++i) {
    if(i==20){a.request_checkpoint();b.request_checkpoint();}
    if(i==50){a.request_revert();b.request_revert();}
    input.jump=i%30==0;input.yaw=float(i)*.01f;
    a.step(input);b.step(input);
    check(world_state(a)==world_state(b),
          "Transferred BSP must preserve movement, lifetime and checkpoint replay exactly");
  }
}
void cinematic_outro_checkpoint() {
  auto content=arena();Program p;
  p.script("outro",p.call("cinematic_outro_start"));
  p.script("stop",p.call("cinematic_stop"));content.scripts=p.data;
  Simulation sim(content,0,{{0,0,0},0,0});
  check(!sim.presentation().cinematic_outro,"Cinematic outro flag starts clear");
  sim.request_checkpoint();sim.step({});
  sim.run_script("outro");sim.step({});
  check(sim.scripts().diagnostics().empty() && sim.presentation().cinematic_outro,
        "Original outro builtin must retain its independent presentation flag");
  sim.run_script("stop");sim.step({});
  check(sim.presentation().cinematic_outro,"Cinematic stop must not clear the level-owned outro flag");
  sim.request_revert();sim.step({});
  check(!sim.presentation().cinematic_outro,"Checkpoint restores the cinematic outro flag");
}
void source_body_checkpoint() {
  auto c=arena();Simulation sim(c,0,{{0,0,0},0,0});
  const auto h=sim.allocate_source_body_storage(65);
  const auto blocker=sim.allocate_source_body_storage(80);
  check(h!=DatumPool::none && blocker!=DatumPool::none,"checkpoint storage fixture allocation failed");
  sim.source_body_storage(h)[64]=std::byte{0x79};
  check(sim.append_source_body_storage(h,0,83,2),"checkpoint fixture relocation failed");
  sim.request_checkpoint();sim.step({});const auto saved=world_state(sim);
  sim.release_source_body_storage(h);sim.release_source_body_storage(blocker);
  const auto replacement=sim.allocate_source_body_storage(1124);
  check(replacement!=h && sim.source_object_bodies().contains(replacement),"discarded branch did not reuse shared storage");
  sim.request_revert();sim.step({});
  check(world_state(sim)==saved && sim.source_body_storage(h)[64]==std::byte{0x79},
      "campaign checkpoint lost persistent source heap, body bytes, handles or scratch");
  check(!sim.source_object_bodies().contains(replacement),"campaign restore retained discarded source body");
  check(sim.append_source_body_storage(h,4,17,1),"restored source body cannot grow");
}
void projectile_lifetime_cleanup() {
  auto c=arena();add_enemy(c);c.bsp_count=2;
  auto& weapon=c.weapons.at(3);weapon.instantaneous=false;weapon.projectile_speed=15;
  weapon.projectile=99;
  c.trigger_volumes.push_back({"outside projectile",-1,0,{1,0,0},{0,0,1},{0,0,0},{10,10,10}});
  c.bsp_transitions.push_back({0x10000,0});
  Program p;
  p.script("cleanup",p.call("object_destroy_type_mask",{p.scalar(1<<5)}));
  p.script("replace",p.call("object_create_anew",{p.scalar(0,56)}));
  c.scripts=p.data;
  Simulation sim(c,0,{{0,0,0},0,0},{},[&](std::size_t){return BspContent{c.collision,c.navigation};});
  const auto original=sim.scene().resolve(0);
  sim.request_checkpoint();sim.step({});
  const auto saved=world_state(sim);
  sim.run_script("replace");sim.step({});
  check(sim.scene().resolve(0)!=original,"real script did not replace the shared scene generation");
  sim.request_revert();sim.step({});sim.take_sounds();
  check(sim.scene().resolve(0)==original && world_state(sim)==saved,
      "campaign restore lost shared identity, pool bytes or native mutation order");
  const auto retained_count=sim.object_lifetimes().pool().count();
  const auto held_weapon=sim.player().weapon.source_datum;
  check(held_weapon!=null_datum && sim.object_lifetimes().contains(held_weapon),"Campaign inventory has no shared item identity");
  Controls fire;fire.fire=true;sim.step(fire);
  check(sim.object_lifetimes().pool().count()>retained_count,"native ordinary projectile did not reserve shared storage");
  sim.run_script("cleanup");sim.step({});
  check(sim.object_lifetimes().pool().count()==retained_count && sim.object_lifetimes().contains(original) && sim.object_lifetimes().contains(held_weapon),
      "type-mask projectile cleanup leaked an identity or destroyed a retained scene owner");
  for(unsigned i=0;i<30 && sim.object_lifetimes().pool().count()==retained_count;++i)sim.step(fire);
  check(sim.object_lifetimes().pool().count()>retained_count,"BSP cleanup fixture has no live native projectile");
  sim.request_bsp(1);sim.step({});
  check(sim.bsp()==1 && sim.object_lifetimes().pool().count()==retained_count && sim.object_lifetimes().contains(held_weapon),
      "BSP projectile removal leaked shared object reservations");
  check(sim.scripts().diagnostics().empty(),"shared lifetime script fixture reported a fault");
}
void combat_checkpoint_replay() {
  auto c = arena(); add_enemy(c);
  auto &weapon = c.weapons.at(3);
  weapon.instantaneous = false; weapon.projectile_speed = 15;
  weapon.projectile_gravity = .2f; weapon.damage_min = 2;
  weapon.minimum_error = .08f; weapon.projectiles_per_shot = 2;
  Simulation sim(c,0,{{0,0,0},0,0}); sim.place_squad("enemy");
  Controls input; input.forward = .6f; input.right = .2f; input.fire = true;
  for (int i=0;i<3;++i) sim.step(input);
  sim.request_checkpoint(); sim.step(input);
  const auto saved = world_state(sim);
  const auto saved_tick = sim.ticks();
  for (int i=0;i<14;++i) {
    input.jump = i < 3; input.crouch = i >= 10;
    input.pitch = float(i) * .01f;
    sim.step(input);
  }
  const auto expected = world_state(sim);
  check(sim.shots() > 4, "combat replay fixture exercises firing and RNG");
  sim.request_revert(); sim.step({}); sim.take_sounds();
  check(sim.ticks() == saved_tick && world_state(sim) == saved,
        "combat checkpoint restores player, actors, projectiles and weapon counters");
  for (int i=0;i<14;++i) {
    input.jump = i < 3; input.crouch = i >= 10;
    input.pitch = float(i) * .01f;
    sim.step(input);
  }
  check(world_state(sim) == expected,
        "restored combat replays identical movement, firing, AI and random spread");
}
void cinematic_checkpoint(bool user_skip) {
  auto c = arena();
  Program p;
  p.data.globals.push_back({"completed",8,p.scalar(0)});
  // Same save/wait/reverted branch structure as Cairo's compiled wrapper.
  const auto cutscene = p.call("begin", {
      p.call("object_teleport", {p.scalar(0,50), p.scalar(0,12)}),
      p.call("object_create_anew", {p.scalar(0,56)}),
      p.call("sleep", {p.scalar(5)})});
  p.script("mission", p.call("begin", {
      p.call("cinematic_start"),
      p.call("cinematic_skip_start_internal"),
      p.call("game_save_cinematic_skip"),
      p.call("sleep_until", {p.call("not", {p.call("game_saving",{},5)},5),
                              p.scalar(1,7)}),
      p.call("if", {p.call("not", {p.call("game_reverted",{},5)},5), cutscene}),
      p.call("cinematic_skip_stop_internal"),
      p.call("if", {p.call("not", {p.call("game_reverted",{},5)},5),
                      p.call("game_revert")}),
      p.call("cinematic_stop"),
      p.call("set", {p.global(0),p.scalar(1)},8)}));
  c.scripts = p.data;
  Simulation sim(c,0,{{0,0,0},0,0});
  sim.start_level();
  sim.step({});
  check(sim.has_checkpoint() && sim.cinematic_skippable() && sim.ticks() == 1,
        "cinematic save captured after tick at safe boundary");
  const auto saved_position = sim.player().body.position;
  const auto original = sim.scene().resolve(0);
  sim.step({});
  check(sim.player().body.position.x == 10 && sim.scene().resolve(0) != original,
        "cinematic body runs only on fresh unreverted entry");
  if (user_skip) {
    Controls skip; skip.skip_cinematic = true;
    sim.step(skip);
  } else {
    for (int i = 0; i < 5; ++i) sim.step({});
  }
  check(sim.checkpoint_restores() == 1 && sim.ticks() == 1 &&
            sim.player().body.position.x == saved_position.x &&
            sim.scene().resolve(0) == original,
        "skip and natural completion restore world and script time");
  const auto events = sim.take_sounds();
  check(events.size() == 1 && events.front().action == SoundEvent::Reset,
        "discarded timeline audio is stopped on restore");
  Controls resumed_input;
  resumed_input.skip_cinematic = user_skip;
  sim.step(resumed_input);
  check(sim.scripts().diagnostics().empty() &&
            sim.scripts().globals()[0].number() == 1 &&
            !sim.cinematic_skippable() && !sim.presentation().cinematic &&
            !sim.scripts().running() && sim.checkpoint_restores() == 1,
        "restored-tick signal continues once even when skip input stays held");
}
void retail_skip_controller(const std::filesystem::path &directory, bool skip) {
  Cache cache(directory / "01b_spacestation.map");
  auto content = cache.load_content(0);
  Program p;
  p.data = content.scripts;
  const auto body_global = unsigned(p.data.globals.size());
  p.data.globals.push_back({"test_body",8,p.scalar(0)});
  const auto completion_global = unsigned(p.data.globals.size());
  p.data.globals.push_back({"test_continuation",8,p.scalar(0)});
  p.script("test_skip_controller", p.call("begin", {
      p.call("if", {p.invoke("cinematic_skip_start",5),
                     p.call("begin", {
                         p.call("set", {p.global(body_global),p.scalar(41)},8),
                         p.call("sleep", {p.scalar(5)})})}),
      p.invoke("cinematic_skip_stop",4),
      p.call("set", {p.global(completion_global),p.scalar(1)},8)}));
  content.scripts = p.data;
  Simulation sim(content,0,cache.spawns().at(0));
  sim.run_script("test_skip_controller");
  sim.step({});
  check(sim.has_checkpoint(), "authored Cairo helper requests a real checkpoint");
  sim.step({});
  check(sim.scripts().globals()[body_global].number() == 41,
        "unchanged authored start helper permits unreverted cutscene");
  if (skip) {
    Controls input; input.skip_cinematic = true;
    sim.step(input);
  } else
    for (int i=0;i<5;++i) sim.step({});
  check(sim.checkpoint_restores() == 1 && sim.ticks() == 1,
        "authored stop helper and user skip revert to saved clock");
  sim.step({});
  check(sim.scripts().diagnostics().empty() &&
            sim.scripts().globals()[body_global].number() == 0 &&
            sim.scripts().globals()[completion_global].number() == 1 &&
            sim.checkpoint_restores() == 1 && !sim.scripts().running(),
        "unchanged Cairo helpers consume restored-tick signal and continue once");
}
void retail_player_stash(const std::filesystem::path &directory) {
  Cache cache(directory / "01b_spacestation.map");
  const auto content = cache.load_content(0);
  Simulation sim(content,0,cache.spawns().at(0));
  sim.run_script("cinematic_stash_players"); sim.step({});
  check(sim.scripts().diagnostics().empty() && !sim.player().visible &&
            sim.player().cannot_take_damage,
        "authored Cairo stash hides player and disables vitality damage");
  sim.run_script("cinematic_unstash_players"); sim.step({});
  check(sim.scripts().diagnostics().empty() && sim.player().visible &&
            !sim.player().cannot_take_damage,
        "authored Cairo unstash restores visibility and damage");
}
void retail_audio_residency(const std::filesystem::path &directory) {
  Cache cache(directory / "01b_spacestation.map"); NativeResidency assets(cache);
  for (const auto &[token,sound_class]:std::vector<std::pair<std::string,unsigned>>{
          {"x02_01_mus",38},{"x02_01_fol",49},{"x02_0010_mas",48}}) {
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t){
      return t.group=="snd!" && t.name.ends_with(token);});
    check(tag!=cache.tags().end(),"Cairo original sound fixture exists");
    const auto pcm=assets.sound(tag->datum);
    check(pcm && pcm->frames()>0 && pcm->sound_class==sound_class &&
              assets.sound(tag->datum)==pcm,
          "Cairo prediction decodes and retains real clip/class reused by live voice playback");
  }
  check(assets.sound_count()==3,"sound residency coalesces duplicate predictions");
  const auto volume=cache.load_bitmap_resource(0xe43002bbu,0);
  const auto vector=cache.load_bitmap_resource(0xe31501a0u,0);
  check(volume.type==1 && volume.depth==32 && volume.levels[0].size()==75008 &&
            volume.levels[1].size()==9472 && volume.levels[2].size()==1280 &&
            vector.format==22 && vector.levels[0].size()==43776 &&
            vector.levels[1].size()==11008 && vector.levels[2].size()==2816,
        "prediction retains all authored volume/vector bitmap LOD bytes without display decoding");
  rejects([&]{cache.load_bitmap_resource(0xe31501a0u,1);},
          "prediction validates bitmap indices");
  const auto resources=cache.structure_resources(0,0,false);
  const auto sections=cache.load_model_resources(0xf58d1355u);
  check(sections.size()>=5 && sections[0].size()==37020,
        "prediction retains all render-model section payloads beyond the display LOD");
  const auto model_bitmaps=cache.model_bitmap_resources(0xf58d1355u);
  check(!model_bitmaps.empty() && std::all_of(model_bitmaps.begin(),model_bitmaps.end(),
            [&](const auto &r){return !cache.load_bitmap_resource(r.datum,r.index).levels[0].empty();}),
        "model prediction retains every referenced material bitmap record independent of display support");
  const auto bsp_blocks=cache.load_bsp_resources(0);
  check(bsp_blocks.size()==109+114 && !bsp_blocks[109+113].empty(),
        "prediction retains every cluster and instanced-definition payload");
  check(std::any_of(resources.begin(),resources.end(),[](const auto &r) {
          return r.type==2 && r.datum==0xf72714efu;
        }),"Cairo cluster predicts Pillar of Autumn beyond its own BSP geometry");
  assets.structure(0,0,false);
  const auto models=assets.model_count(), bitmaps=assets.bitmap_count();
  check(models>0 && bitmaps>0,"structure prediction retains dependent models and original bitmaps");
  assets.structure(0,0,false);
  check(assets.model_count()==models && assets.bitmap_count()==bitmaps,
        "structure prediction coalesces repeated dependent resources");
  rejects([&]{cache.structure_resources(0,114,true);},
          "definition prediction uses definition count, not placement count");
}
}
int main(int argc, char **argv) {
  try {
    vm_checkpoint(); dispatch_boundary(); poll_boundaries(); scene_checkpoint();campaign_create_containing();
    protection_flags(); tutorial_actor_commands_and_text(); checkpoint_audio_and_faults();
    campaign_fixed_upper_damage();
    player_controls_and_external_global();tutorial_controls_and_checkpoint();
    combat_checkpoint_replay(); projectile_lifetime_cleanup(); source_body_checkpoint(); bsp_builtin_boundary();
    initial_geometry_ownership();
    cinematic_outro_checkpoint();
    bsp_streaming_checkpoint(); bsp_scene_lifetime();
    class_fade_and_live_mixer(); campaign_configuration_checkpoint();
    retail_external_long_expression();
    campaign_residency_requests();
    campaign_look_cone();
    cinematic_object_cleanup();
    cinematic_checkpoint(false); cinematic_checkpoint(true);
    if (argc > 1) {
      retail_skip_controller(argv[1],false);
      retail_skip_controller(argv[1],true);
      retail_player_stash(argv[1]);
      retail_audio_residency(argv[1]);
    }
    std::cout << "Campaign checkpoint tests passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
