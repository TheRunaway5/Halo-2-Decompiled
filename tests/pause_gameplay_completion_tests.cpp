#include "halo2/campaign_text.hpp"
#include "halo2/rumble.hpp"
#include "halo2/training_controls.hpp"
#include "halo2/renderer.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw FormatError(message);
}
std::vector<std::byte> constant(float value) {
  std::vector<std::byte> b(24);
  b[0] = std::byte{1};
  std::memcpy(b.data() + 4, &value, 4);
  std::memcpy(b.data() + 8, &value, 4);
  return b;
}
int calls{};
RumbleOutput observed;
int SDLCALL rumble(void *, Uint16 l, Uint16 r) {
  ++calls;
  observed = {l, r};
  return 0;
}
void rumble_output() {
  ShaderFunctionTables tables;
  RumblePlayer player;
  auto definition = std::make_shared<RumbleDefinition>();
  definition->push_back({0, {{{.5f, constant(.5f)}, {1.f, constant(.25f)}}}});
  player.play(definition, true, 1);
  auto out = player.evaluate(tables);
  check(out.left == 32768 && out.right == 16384,
        "Authored two-motor amplitudes");
  player.advance(.5f);
  out = player.evaluate(tables);
  check(!out.left && out.right == 16384, "Independent motor lifetimes");
  player.advance(.5f);
  check(player.evaluate(tables).right == 0, "Expired motor stops");
  player.clear();
  player.play(definition, false, 1);
  check(!player.evaluate(tables).left,
        "Shield selector skips unmatched response type");
  player.play(definition, true, 1);
  player.advance(.01f);
  player.play(definition, true, 1);
  check(player.evaluate(tables).left == 65535,
        "Distinct active source slots add and clamp");
  check(SDL_Init(SDL_INIT_GAMECONTROLLER) == 0, SDL_GetError());
  SDL_VirtualJoystickDesc desc{};
  desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
  desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
  desc.naxes = SDL_CONTROLLER_AXIS_MAX;
  desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
  desc.name = "Pause rumble test";
  desc.Rumble = rumble;
  const auto device = SDL_JoystickAttachVirtualEx(&desc);
  check(device >= 0, SDL_GetError());
  auto *pad = SDL_GameControllerOpen(device);
  check(pad, SDL_GetError());
  check(send_controller_rumble(pad, {32123, 12345}, true, false, true) == 0,
        SDL_GetError());
  check(calls && observed.left == 32123 && observed.right == 12345,
        "Actual SDL motor transport");
  send_controller_rumble(pad, {32123, 12345}, false, false, true);
  check(!observed.left && !observed.right,
        "Disabled preference stops both motors");
  send_controller_rumble(pad, {32123, 12345}, true, false, true);
  send_controller_rumble(pad, {32123, 12345}, true, true, true);
  check(!observed.left && !observed.right, "Pause stops both motors");
  send_controller_rumble(pad, {32123, 12345}, true, false, true);
  send_controller_rumble(pad, {32123, 12345}, true, false, false);
  check(!observed.left && !observed.right, "Focus loss stops both motors");
  SDL_GameControllerClose(pad);
  SDL_JoystickDetachVirtual(device);
  SDL_Quit();
}
void text(const std::filesystem::path &map,
          const std::filesystem::path &vista,const std::filesystem::path &captures={}) {
  Cache cache(map);
  auto content = load_campaign_text(cache, map);
  unsigned effects=0,active=0;
  const auto tables=cache.shader_function_tables();
  for(const auto &tag:cache.select_tags({.group=TagGroup::from_string("jpt!"),.require_address=true})) {
    const auto definition=std::make_shared<const RumbleDefinition>(load_rumble_definition(cache,tag.datum));
    for(unsigned i=0;i<definition->size();++i) {
      ++effects;RumblePlayer player;player.play(definition,i,1);
      for(unsigned tick=0;tick<30;++tick) {
        const auto motors=player.evaluate(*tables);active+=bool(motors.left || motors.right);player.advance(1.f/30.f);
      }
    }
  }
  check(effects && active,"Actual authored damage curves produce motor output");
  check(content.training.contains(0x100018b9),
        "Actual authored look-up training string missing");
  check(content.subtitles.empty(),
        "Tutorial has no authored cinematic subtitle table");
  content.subtitles.emplace(123, "Cinematic subtitle");
  Mesh mesh;
  CampaignTextRenderer renderer(content, cache.shared_directory() / "fonts",
                                vista, mesh);
  PlayerTrainingState training;
  CampaignSubtitle subtitle;
  check(renderer.draw(training, subtitle, 1, 1280, 720, 0).empty(),
        "Inactive text emits nothing");
  training.text = 0x100018b9;
  training.show = true;
  const auto draw = renderer.draw(training, subtitle, 1, 1280, 720, 0);
  check(!draw.empty(), "Source training panel and text render");
  for (const auto &v : draw)
    check(std::isfinite(v.position.x) && std::isfinite(v.position.y) &&
              v.material < mesh.materials.size(),
          "Valid training vertices and materials");
  training.enabled = false;
  check(renderer.draw(training, subtitle, 1, 1280, 720, 0).empty(),
        "Training enable gates rendering");
  subtitle.start(123, 2);
  check(subtitle.alpha() == 1, "Initial subtitle opacity");
  check(!renderer.draw(training, subtitle, 1, 1280, 720, 1).empty(),
        "On renders cinematic text");
  check(renderer.draw(training, subtitle, 2, 1280, 720, 1).empty(),
        "Off hides cinematic text");
  check(renderer.draw(training, subtitle, 0, 1280, 720, 1).empty(),
        "Automatic hides same-language lines");
  check(campaign_subtitles_visible(0, 0, 1),
        "Automatic enables translated cinematic text");
  subtitle.advance(2.75f);
  check(std::abs(subtitle.alpha() - std::pow(.5f, 1.9f)) < 1e-6,
        "Source subtitle fade curve");
  subtitle.advance(.75f);
  check(!subtitle.text && !subtitle.alpha(),
        "Subtitle expires after trailing fade");
  ControllerSettings pad;
  PcKeyboardSettings keys;
  check(training_control_text("\xee\x91\x97", pad, keys, true) ==
            "\xee\x84\x91",
        "Standard look stick token");
  pad.sticks = 1;
  check(training_control_text("\xee\x91\x97", pad, keys, true) ==
            "\xee\x84\x90",
        "Southpaw look stick token");
  check(training_control_text("\xee\x91\x97", pad, keys, false) == "Mouse",
        "Keyboard look prompt");
  keys.bindings[unsigned(PcAction::Jump)] = SDL_SCANCODE_J;
  check(training_control_text("\xee\x91\x86", pad, keys, false) == "J",
        "Live keyboard remap reaches training text");
  if(!captures.empty()) {
    std::filesystem::create_directories(captures);
    check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto *window=SDL_CreateWindow("Campaign training and subtitles",SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,1280,720,SDL_WINDOW_VULKAN);
    check(window,SDL_GetError());
    mesh.vertices.resize(3);
    {
      Renderer gpu(window,mesh,true);gpu.scene_clear_color({.06f,.08f,.12f});Camera camera;
      const auto capture=[&](const char *name,const std::vector<Vertex> &vertices) {
        bool drawn=false;for(unsigned retry=0;retry<4 && !drawn;++retry)
          drawn=gpu.draw(camera,captures/(std::string(name)+".ppm"),{},false,{},{},vertices);
        check(drawn,"Campaign text Vulkan capture failed");
      };
      subtitle={};training.enabled=true;
      capture("training-keyboard",renderer.draw(training,subtitle,1,1280,720,0,
          [&](const std::string &s){return training_control_text(s,pad,keys,false);}));
      capture("training-gamepad",renderer.draw(training,subtitle,1,1280,720,0,
          [&](const std::string &s){return training_control_text(s,pad,keys,true);}));
      training.enabled=false;subtitle.start(123,2);
      capture("subtitles-on",renderer.draw(training,subtitle,1,1280,720,1));
      capture("subtitles-off",renderer.draw(training,subtitle,2,1280,720,1));
      std::array<CampaignTitle,4> titles{};titles[0]={0,60};subtitle={};
      capture("tutorial-title",renderer.draw(training,subtitle,1,1280,720,0,{},titles));
    }
    SDL_DestroyWindow(window);SDL_Quit();
  }
}
} // namespace
int main(int argc, char **argv) {
  try {
    rumble_output();
    if (argc == 3 || argc==4)
      text(argv[1], argv[2],argc==4?std::filesystem::path(argv[3]):std::filesystem::path{});
    std::cout << "Pause gameplay text and rumble tests passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
