#include "halo2/frontend.hpp"
#include "halo2/frontend_canvas.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <sstream>

using namespace halo2;
using Action = FrontendPointerAction;
namespace {
void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
struct Target { FrontendPointerTarget hit; float x{}, y{}; };
Target find(FrontendPainter &painter, Action action, unsigned index = 0) {
  for (float y = 0; y < 480; y += 2)
    for (float x = 0; x < 640; x += 2)
      if (const auto hit = painter.pointer_hit(x, y); hit && hit->action == action && (action != Action::Entry || hit->index == index))
        return {*hit, x, y};
  throw std::runtime_error("missing mouse action " + std::to_string(unsigned(action)));
}
void click(MenuState &state, FrontendPainter &painter, Action action, unsigned index = 0) {
  painter.draw(state, 5, -1, 10000);
  const auto target = find(painter, action, index);
  const auto source = state.frame().screen;
  try { check(state.activate_pointer(target.hit), "mouse target refused its menu action"); }
  catch (const std::exception &error) { throw std::runtime_error("screen " + std::to_string(state.frame().screen) + ": " + error.what()); }
  std::cout << "click screen=" << source << " action=" << unsigned(action) << " index=" << index
            << " x=" << target.x << " y=" << target.y << " destination=" << state.frame().screen << '\n';

}
void create(MenuState &state) {
  state.accept(); state.accept(); state.text("Mouse Prompts"); state.commit_text();
  check(state.frame().screen == 38, "profile creation failed");
}
struct PromptRect {unsigned x{}, y{}, width{}, height{};};
struct PromptAtlas {unsigned width{}, height{};std::vector<PromptRect> rectangles;};
std::map<std::string, PromptAtlas> reference_prompt_atlases(const std::filesystem::path &root) {
  std::map<std::string, PromptAtlas> result;
  for (const auto *family : {"xbox", "playstation"}) {
    const auto bank = root / (std::string("prompts-") + family);
    std::ifstream manifest_file(bank / "manifest.json");
    const auto manifest = nlohmann::json::parse(manifest_file);
    std::ifstream table(bank / "prompts.tsv");
    std::string line;
    check(bool(std::getline(table, line)) && line == "H2_VISTA_PROMPT_RASTERS_V1",
        "missing controller prompt reference table");
    while (std::getline(table, line)) {
      unsigned selector{}, code{};PromptRect rectangle;std::string atlas, extra;
      std::istringstream row(line);
      check(bool(row >> selector >> code >> rectangle.x >> rectangle.y >> rectangle.width >> rectangle.height >> atlas) && !(row >> extra),
          "invalid controller prompt reference rectangle");
      const auto material = std::string("controller-prompts/") + family + "/" + atlas + "/" +
          manifest.at("files").at(atlas).at("sha256").get<std::string>();
      auto &record = result[material];
      if (!record.width) {
        std::ifstream image(bank / atlas, std::ios::binary);
        std::array<std::byte, 128> header{};
        image.read(reinterpret_cast<char *>(header.data()), header.size());
        check(bool(image), "missing controller prompt reference DDS header");
        const Bytes bytes(header);
        check(bytes.u32(0) == 0x20534444u && bytes.u32(4) == 124,
            "invalid controller prompt reference DDS header");
        record.height = bytes.u32(12);record.width = bytes.u32(16);
      }
      check(rectangle.x < record.width && rectangle.y < record.height && rectangle.width && rectangle.height &&
          rectangle.width <= record.width - rectangle.x && rectangle.height <= record.height - rectangle.y,
          "controller prompt reference rectangle exceeds DDS");
      if (code == 0xe100 || code == 0xe101 || code == 0xe102 || code == 0xe103 || code == 0xe10c || code == 0xe10d)
        record.rectangles.push_back(rectangle);
    }
    check(table.eof(), "truncated controller prompt reference table");
  }
  check(!result.empty(), "empty controller prompt reference inventory");
  return result;
}
void pc_special_button_hit(const FrontendContent& original,
                           const std::filesystem::path& source,
                           const std::filesystem::path& fonts) {
  // Pin/record: pc14-layout-ownership-original/evidence.json, PC14 pane0
  // START GAME at file60379796. This uses its real60-byte source record in
  // an already-admitted PC slot; it does not enable the unbound lobby route.
  std::ifstream input(source,std::ios::binary);input.seekg(60379796);
  std::array<std::byte,60> raw{};input.read(reinterpret_cast<char*>(raw.data()),raw.size());
  check(bool(input),"missing actual PC14 special-button record");
  const Bytes bytes(raw);
  const auto s16=[&](std::size_t at){return std::bit_cast<std::int16_t>(bytes.u16(at));};
  UiButton button;
  button.text.flags=bytes.u32(0);button.text.font=s16(10);
  button.text.color={bytes.f32(12),bytes.f32(16),bytes.f32(20),bytes.f32(24)};
  button.text.bounds={float(s16(28)),float(s16(30)),float(s16(32)),float(s16(34))};
  button.bitmap=bytes.u32(40);button.offset={float(s16(44)),float(s16(46))};
  button.text.string=bytes.u32(48);button.text.layer=s16(52);
  button.vista_hit_top_offset=s16(54);button.vista_flags=bytes.u32(56);
  check(button.text.bounds.top==480 && button.text.bounds.left==-690 &&
      button.text.bounds.bottom==440 && button.text.bounds.right==-360 &&
      button.vista_hit_top_offset==-12 && button.vista_flags==1,
      "actual PC14 button geometry pin changed");
  // A fixed zero animation owner isolates hit layout from elapsed/focus
  // animation. No original animation timing is asserted by this regression.
  auto content=original;
  const auto pc=std::make_shared<FrontendContent>(*original.vista);
  content.vista=pc;
  auto& pane=pc->screens.at(293).panes.at(0);
  pane={};pane.buttons.push_back(button);
  Mesh mesh;FrontendPainter painter(content,fonts,mesh);MenuState state(content);
  state.open(vista_menu(293));
  auto& live=pane.buttons.front();
  live.vista_hit_top_offset=0;
  const auto before=painter.draw(state,5,0,10000);
  check(!painter.hit(100,45),"unadjusted special button unexpectedly has expanded top");
  live.vista_hit_top_offset=button.vista_hit_top_offset;
  const auto after=painter.draw(state,5,0,10000);
  check(painter.hit(100,45)==0,"PC63FFF8 extra12-unit top strip is not clickable");
  check(!painter.hit(100,43) && painter.hit(100,63)==0 && !painter.hit(100,64),
      "PC special-button hit bounds moved beyond the original rectangle");
  check(before.vertices.size()==after.vertices.size() && !after.vertices.empty(),
      "PC hit-top field changed draw inventory");
  for(std::size_t i=0;i<after.vertices.size();++i)
    check(before.vertices[i].position.x==after.vertices[i].position.x &&
        before.vertices[i].position.y==after.vertices[i].position.y,
        "PC hit-top field changed authored draw positions");
}
}
int main(int argc, char **argv) try {
  if (argc != 4 || !std::filesystem::exists(std::filesystem::path(argv[1]) / "mainmenu.map") ||
      !std::filesystem::exists(argv[2]) || !std::filesystem::exists(std::filesystem::path(argv[3]) / "prompts-xbox/manifest.json")) return 77;
  const std::filesystem::path maps = argv[1];
  auto content = Cache(maps / "mainmenu.map").load_frontend();
  content.vista = load_vista_settings(argv[2]);
  Mesh mesh;
  FrontendPainter painter(content, maps / "fonts", mesh);
  MenuState title(content);
  click(title, painter, Action::Accept);
  check(title.frame().screen == 10, "title PRESS START click failed");
  MenuState guest_exit(content);
  guest_exit.open(6);
  check(guest_exit.signed_in_profile().empty() &&
      guest_exit.frame().entries.size() == 7 &&
      guest_exit.frame().entries.at(4).destination == 19 &&
      guest_exit.frame().entries.back().label == "EXIT GAME",
      "Exit Game must follow Mods as the final main-menu option");
  guest_exit.move(-1);
  check(guest_exit.frame().selected == 6 && !guest_exit.quit,
      "keyboard/controller focus does not wrap to Exit Game");
  guest_exit.move(1);
  check(guest_exit.frame().selected == 0 && !guest_exit.quit,
      "Exit Game focus does not wrap back to Campaign");
  guest_exit.move(6);
  guest_exit.accept();
  check(guest_exit.quit && !guest_exit.launch && guest_exit.frame().screen == 6,
      "Exit Game did not request shutdown from the unsigned main menu");
  MenuState profile_exit(content);
  create(profile_exit);
  profile_exit.back();
  check(profile_exit.frame().screen == 6 && !profile_exit.signed_in_profile().empty(),
      "Exit Game mouse fixture did not sign in");
  click(profile_exit, painter, Action::Entry, 6);
  check(profile_exit.quit && !profile_exit.launch && profile_exit.frame().screen == 6,
      "Exit Game click did not request shutdown from the signed-in main menu");
  MenuState state(content);
  create(state);
  state.select(1); state.accept();
  check(state.frame().screen == 47, "Appearance route failed");
  painter.draw(state, 5, -1, 10000);
  // These coordinates are independently frozen by the original subtitle
  // regression; no test-only target rectangles are used.
  for (const float x : {545.f, 563.f, 588.f}) {
    const auto done = painter.pointer_hit(x, 432);
    check(done && done->action == Action::Back, "visible B DONE icon/label cannot be clicked");
  }
  check(!painter.pointer_hit(537, 432), "gap between SELECT and DONE activates a prompt");
  const auto done = painter.pointer_hit(545, 432);
  check(state.activate_pointer(*done) && state.frame().screen == 38, "B DONE did not return to profile");
  check(!state.activate_pointer(*done), "stale mouse target activated the next menu");
  state.select(1); state.accept();
  state.select(5); state.accept();
  // Original animation54's alpha just above its 62.5ms zero key still
  // quantizes to transparent ARGB. Invisible text must not be clickable.
  painter.draw(state, .06251f, -1, 10000);
  check(!painter.pointer_hit(136, 394), "fully transparent prompt still has a mouse target");
  const auto flags = state.appearance().flags;
  click(state, painter, Action::Auxiliary);
  check(state.frame().screen == 49 && state.appearance().flags == (flags ^ 1), "X TOGGLE ICON failed");
  click(state, painter, Action::Back);
  check(state.frame().screen == 47, "inline B CANCEL failed");
  state.back(); state.back();
  check(state.frame().screen == 6, "main menu route failed");
  state.select(2); state.accept(); state.accept();
  check(state.frame().screen == 14, "lobby route failed");
  state.set_multiplayer_map_validator([](const FrontendLaunch &) { throw FormatError("Mouse Continue regression"); });
  state.select(0); state.accept();
  check(state.frame().screen == 8, "unavailable dialog route failed");
  painter.draw(state, 5, -1, 10000);
  check(!painter.pointer_hit(150, 78), "dialog admitted an underlying Start Game target");
  click(state, painter, Action::Accept);
  check(state.frame().screen == 14 && !state.launch, "A CONTINUE did not dismiss the error");

  MenuState keyboard(content);
  keyboard.accept(); keyboard.accept(); keyboard.text("Mouse");
  click(keyboard, painter, Action::Entry, 36);
  check(keyboard.keyboard_text == "Mous", "keyboard X delete failed");
  click(keyboard, painter, Action::Entry, 37);
  check(keyboard.keyboard_text == "Mous ", "keyboard Y space failed");
  keyboard.text("Input");
  click(keyboard, painter, Action::Entry, 40);
  check(keyboard.frame().screen == 38 && keyboard.profile_name() == "Mous Input", "keyboard Start DONE failed");
  MenuState cancel(content);
  cancel.accept(); cancel.accept();
  click(cancel, painter, Action::Back);
  check(cancel.frame().screen == 9, "keyboard B CANCEL failed");

  MenuState pc(content);
  create(pc);
  pc.open(vista_menu(19));
  pc.select(5); click(pc, painter, Action::Accept);
  check(pc.frame().screen == vista_menu(297), "Vista A SELECT did not open About");
  click(pc, painter, Action::Back);
  check(pc.frame().screen == vista_menu(19), "Vista About BACK failed");
  pc.open(vista_menu(253));
  painter.draw(pc, 5, -1, 10000);
  for (int y = 0; y < 480; y += 4)
    for (int x = 0; x < 640; x += 4)
      if (const auto hit = painter.pointer_hit(float(x), float(y)); hit && hit->action == Action::Entry)
        check(pc.frame().entries[hit->index].value != -2, "binding section heading is a mouse target");
  const auto binding = unsigned(pc.frame().entries[pc.frame().selected].value);
  click(pc, painter, Action::Auxiliary);
  check(pc.capturing_binding(), "native X secondary binding caption is not clickable");
  pc.bind_pc_input(pc_mouse_button(3));
  check(pc.pc_keyboard_settings().secondary[binding] == pc_mouse_button(3), "secondary caption used primary binding");
  click(pc, painter, Action::ClearBinding);
  check(pc.pc_keyboard_settings().bindings[binding] == 0 && !pc.capturing_binding(), "Vista DELETE failed");
  click(pc, painter, Action::Accept);
  check(pc.capturing_binding(), "Vista EDIT did not capture a binding");
  click(pc, painter, Action::ClearBinding);
  check(!pc.capturing_binding() && pc.frame().screen == vista_menu(253), "Backspace caption did not clear a captured binding");
  click(pc, painter, Action::Accept);
  click(pc, painter, Action::Back);
  check(!pc.capturing_binding() && pc.frame().screen == vista_menu(253), "Vista DONE did not cancel capture");
  click(pc, painter, Action::Back);
  check(pc.frame().screen == vista_menu(19), "Vista DONE did not return to Settings");

  // Verify actual submitted icon ink against independently decoded font UVs
  // across every menu pane/selection, including parameter/category contexts.
  // The cached screen13 definitions remain source data; neither namespace
  // admits that retired route as an interactive MenuState screen.
  for (const auto retired : {13u, vista_menu(13)}) {
    MenuState sample(content);
    bool rejected = false;
    try { sample.open(retired); }
    catch (const FormatError &error) {
      rejected = std::string_view(error.what()) ==
          "Retired screen13 is unavailable in native navigation";
    }
    check(rejected, "retired screen13 became an interactive navigation route");
  }
  // These newly imported map selectors retain source resources without a
  // native widget/row lifetime (frontend_vista.cpp's shell_selected owner).
  // PC210 supplies presentation to native route210, not a PC settings route.
  for (const auto imported : {17u, 206u, 210u}) {
    MenuState sample(content);
    bool rejected = false;
    try { sample.open(vista_menu(imported)); }
    catch (const FormatError &error) {
      rejected = std::string_view(error.what()) ==
          "unimplemented source Vista settings route";
    }
    check(rejected, "source layout without direct navigation became an interactive route");
  }
  std::map<std::pair<bool, std::string>, RetailFont> fonts;
  const auto prompt_atlases = reference_prompt_atlases(argv[3]);
  unsigned draws{}, icons{}, controller_icons{};
  auto verify = [&](MenuState &sample, int pane) {
    const auto draw = painter.draw(sample, 5, pane, 10000);
    const bool vista = is_vista_menu(sample.frame().screen);
    const auto first = draw.backdrops.empty() ? 0 : draw.backdrops.back().before_vertex;
    for (std::size_t i = first; i + 5 < draw.vertices.size(); i += 6) {
      const auto &a = draw.vertices[i], &b = draw.vertices[i + 4];
      const auto &material = mesh.materials.at(a.material);
      if (material.name.starts_with("controller-prompts/") && (a.color.x != 0 || a.color.y != 0 || a.color.z != 0)) {
        const auto found = prompt_atlases.find(material.name);
        check(found != prompt_atlases.end(), "rendered controller atlas lacks its admitted reference");
        const auto &atlas = found->second;
        check(material.diffuse.width == atlas.width && material.diffuse.height == atlas.height,
            "rendered controller atlas dimensions differ from source DDS");
        const float u = (a.uv.x + b.uv.x) * .5f * atlas.width;
        const float v = (a.uv.y + b.uv.y) * .5f * atlas.height;
        if (std::any_of(atlas.rectangles.begin(), atlas.rectangles.end(), [&](const auto &r) {
              return u >= r.x && u < r.x + r.width && v >= r.y && v < r.y + r.height;
            })) {
          const auto hit = painter.pointer_hit((a.position.x + b.position.x) * .5f, (a.position.y + b.position.y) * .5f);
          if (!hit) throw std::runtime_error("unclickable rendered controller prompt on screen " +
              std::to_string(sample.frame().screen) + " pane " + std::to_string(pane));
          ++icons;++controller_icons;
        }
        continue;
      }
      if (!material.name.starts_with("font/") || (a.color.x == 0 && a.color.y == 0 && a.color.z == 0)) continue;
      const auto name = material.name.substr(5);
      const auto key = std::pair{vista, name};
      if (!fonts.contains(key)) {
        auto path = (vista ? content.vista->source_fonts : maps / "fonts") / name;
        if (!std::filesystem::exists(path)) continue;
        fonts.emplace(key, load_retail_font(path));
      }
      const auto &font = fonts.at(key);
      const float u = (a.uv.x + b.uv.x) * .5f * font.atlas.width;
      const float v = (a.uv.y + b.uv.y) * .5f * font.atlas.height;
      for (const unsigned c : {0xe100u, 0xe101u, 0xe102u, 0xe103u, 0xe10cu, 0xe10du}) {
        if (!font.glyphs.contains(c)) continue;
        const auto &g = font.glyphs.at(c);
        if (!g.width || !g.height || u < g.x || u >= g.x + g.width || v < g.y || v >= g.y + g.height) continue;
        const auto hit = painter.pointer_hit((a.position.x + b.position.x) * .5f, (a.position.y + b.position.y) * .5f);
        if (!hit) throw std::runtime_error("unclickable rendered prompt on screen " +
            std::to_string(sample.frame().screen) + " pane " + std::to_string(pane) + " glyph " + std::to_string(c));
        ++icons;
      }
    }
    ++draws;
  };
  for (const bool vista : {false, true}) {
    const auto &source = vista ? *content.vista : content;
    for (const auto &[id, definition] : source.screens) {
      if (id == 13) continue;
      // PC16/185 use the retained-result factory and dedicated painter suite.
      // PC209 is retained transition data, not a MenuState navigation route.
      // PC210 is rendered through the native progress route, not build_pc.
      // PC14/23 and map selectors17/206 are imported ahead of their dynamic widget/factory binding;
      // their raw authored children do not supply an interactive live owner.
      if (vista && (id == 5 || id == 38 || id == 14 || id == 23 || id == 17 || id == 206 ||
          id == 16 || id == 185 || id == 209 || id == 210)) continue;
      MenuState sample(content);
      if (id != 9 || vista) sample.open(vista ? vista_menu(id) : id);
      for (unsigned pane = 0; pane < std::max(1u, unsigned(definition.panes.size())); ++pane)
        for (unsigned row = 0; row < std::max(1u, unsigned(sample.frame().entries.size())); ++row) {
          sample.select(row); verify(sample, int(pane));
        }
    }
  }
  for (const auto &[id, parameter] : content.parameters) {
    MenuState sample(content); sample.open(223, id);
    for (unsigned row = 0; row < sample.frame().entries.size(); ++row) { sample.select(row); verify(sample, -1); }
  }
  for (const auto &[id, category] : content.categories) {
    MenuState sample(content); sample.open(221, id);
    for (unsigned row = 0; row < sample.frame().entries.size(); ++row) { sample.select(row); verify(sample, -1); }
  }
  check(draws > 1400 && icons > 2000, "prompt coverage missed the menu inventory");
  painter.draw(state, 5, -1, 10000);
  const auto target = find(painter, Action::Back);
  for (const auto size : {std::pair{640u, 480u}, {1280u, 720u}, {1280u, 800u}, {1680u, 720u}, {2560u, 1437u}, {1001u, 601u}}) {
    const auto canvas = frontend_canvas(size.first, size.second);
    const auto hit = painter.pointer_hit(((target.x * canvas.scale + canvas.left) - canvas.left) / canvas.scale,
                                         ((target.y * canvas.scale + canvas.top) - canvas.top) / canvas.scale);
    check(hit && hit->action == target.hit.action, "aspect ratio moved a prompt target");
  }
  pc_special_button_hit(content,argv[2],maps/"fonts");
  std::cout << draws << " menu pane/selection/context draws; " << icons
            << " visible prompt glyphs clickable (" << controller_icons << " controller atlas glyphs); menu/keyboard/dialog/Vista actions passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
