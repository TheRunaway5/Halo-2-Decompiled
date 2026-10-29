#include "halo2/audio.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_canvas.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/frontend_frame_texture.hpp"
#include "halo2/frontend_frame_sampling.hpp"
#include "halo2/frontend_keyboard.hpp"
#include "halo2/frontend_material.hpp"
#include "halo2/frontend_timing.hpp"
#include "halo2/math.hpp"
#include "halo2/movie.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/sky.hpp"
#include "halo2/atmosphere.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
void campaign_launch(const FrontendContent &content);
template <class F> void rejects(F f) {
  bool rejected = false;
  try {
    f();
  } catch (const FormatError &) {
    rejected = true;
  }
  check(rejected, "malformed frontend data accepted");
}
std::vector<std::byte> bytes(std::initializer_list<unsigned> values) {
  std::vector<std::byte> out;
  for (auto v : values)
    out.push_back(std::byte(v));
  return out;
}
void font_decoder() {
  const auto encoded = bytes({0xc7, 0x82, 0x42, 0x02, 0x00, 0xa1, 0x23, 0x81});
  const auto decoded = decode_font_pixels(encoded, 16);
  check(decoded[0] == 0x0fff && decoded[1] == 0xffff, "two alpha pixels");
  check(decoded[2] == 0x0fff && decoded[3] == 0xffff && decoded[5] == 0xffff,
        "opaque font run");
  check(decoded[8] == 0x0fff && decoded[9] == 0x0fff, "transparent font run");
  check(decoded[10] == 0xa123 && decoded[11] == 0x0123 && decoded[12] == 0xf123,
        "literal font color");
  rejects([&] { decode_font_pixels(bytes({0x00, 0xff}), 1); });
  rejects([&] { decode_font_pixels(bytes({0xc0}), 1); });
  rejects([&] { decode_font_pixels(bytes({0x7f}), 1); });
  rejects([&] { decode_font_pixels({}, 256 * 128 + 1); });
  std::vector<std::byte> padded(128, std::byte{0xcd});
  // A 3x2 BGRA linear image, each row starts at a 64-byte boundary.
  for (unsigned y = 0; y < 2; ++y)
    for (unsigned x = 0; x < 3; ++x) {
      const auto i = y * 64 + x * 4;
      padded[i] = std::byte(10 + y);
      padded[i + 1] = std::byte(20 + x);
      padded[i + 2] = std::byte(30 + y + x);
      padded[i + 3] = std::byte{255};
    }
  const auto texture = prepare_ui_bitmap(Bytes(padded), 3, 2, 11, 16);
  check(texture.pixels.size() == 24 && texture.pixels[12] == std::byte{31} &&
            texture.pixels[14] == std::byte{11},
        "linear bitmap pitch lost");
  padded.resize(100);
  rejects([&] { prepare_ui_bitmap(Bytes(padded), 3, 2, 11, 16); });
}
std::vector<std::byte> ui_bc3_fixture(const char *name) {
  const auto path = std::filesystem::path(__FILE__).parent_path() / "fixtures" / name;
  std::ifstream file(path, std::ios::binary);
  check(bool(file), "missing original UI BC3 fixture");
  std::vector<std::byte> result;
  for (char value; file.get(value);)
    result.push_back(std::byte(static_cast<unsigned char>(value)));
  return result;
}
void ui_bc3_decoder() {
  const auto path = std::filesystem::path(__FILE__).parent_path() / "fixtures" /
                    "frontend_ui_bc3_original.tsv";
  std::ifstream file(path);
  check(bool(file), "missing original UI BC3 endpoint fixture");
  auto unhex = [](const std::string &value) {
    check(value.size() % 2 == 0, "invalid original UI BC3 fixture hex");
    std::vector<std::byte> result;
    for (std::size_t i = 0; i < value.size(); i += 2)
      result.push_back(std::byte(std::stoul(value.substr(i, 2), nullptr, 16)));
    return result;
  };
  unsigned count = 0;
  for (std::string line; std::getline(file, line);) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream row(line);
    unsigned width{}, height{};
    std::string input, expected;
    check(bool(row >> width >> height >> input >> expected),
          "invalid original UI BC3 endpoint fixture row");
    const auto raw = unhex(input);
    const auto texture = prepare_ui_bitmap(Bytes(raw), width, height, 16, 0);
    check(texture.format == 11,
          "UI BC3 remains hardware-compressed instead of original integer-expanded texels");
    check(texture.pixels == unhex(expected),
          "UI BC3 endpoint/alpha/block expansion differs from unchanged original decoder");
    ++count;
  }
  check(count == 312, "original UI BC3 endpoint fixture count changed");
  const auto raw = ui_bc3_fixture("frontend_ui_dialog1_original.bc3");
  const auto texture = prepare_ui_bitmap(Bytes(raw), 256, 256, 16, 0);
  check(texture.pixels == ui_bc3_fixture("frontend_ui_dialog1_original.rgba"),
        "actual dialog1 UI texels differ from unchanged original decoder");
}
void background_canvas_policy() {
  struct Case { unsigned width, height; float scale, left, top, canvas_width, canvas_height; };
  const std::array cases{
      Case{640,480,1,0,0,640,480}, Case{1280,720,1.5f,160,0,960,720},
      Case{1280,800,800.f/480.f,320.f/3.f,0,3200.f/3.f,800},
      Case{1680,720,1.5f,360,0,960,720}, Case{2560,720,1.5f,800,0,960,720},
      Case{1000,600,1.25f,100,0,800,600}, Case{1280,1024,2,0,32,1280,960}};
  const auto close=[](float a,float b){return std::abs(a-b)<.001f;};
  for(const auto &c:cases) {
    const auto canvas=frontend_canvas(c.width,c.height);
    check(close(canvas.scale,c.scale) && close(canvas.left,c.left) && close(canvas.top,c.top) &&
          close(canvas.width,c.canvas_width) && close(canvas.height,c.canvas_height),
          "background expansion changes the authored foreground viewport");
    // Original content coordinates and their inverse mouse transform remain
    // identical; the player preview and keyboard are not widened.
    for(const auto point:std::array<Vec2,4>{{{49,74},{320,240},{591,423},{72,168}}}) {
      const Vec2 physical{canvas.left+point.x*canvas.scale,canvas.top+point.y*canvas.scale};
      check(close((physical.x-canvas.left)/canvas.scale,point.x) &&
            close((physical.y-canvas.top)/canvas.scale,point.y),
            "widescreen background changes content or mouse coordinates");
    }
    const auto centered=canvas.source(false),wide=canvas.source(true);
    check(wide[0]==0 && wide[2]==1 && close(wide[1],centered[1]) && close(wide[3],centered[3]),
          "background tint/blur does not span the side margins");
    check(close(wide[2]/canvas.background_logical_width(),centered[2]/640.f),
          "wide popup blur changes the authored physical tap spacing");
  }
  bool rejected=false;
  try{(void)frontend_canvas(0,480);}catch(const std::invalid_argument &){rejected=true;}
  check(rejected,"empty background drawable is admitted");
}
void frame_edge_dds_parser() {
  Texture stock{8,4,15,std::vector<std::byte>(32),{{8,4,0,32}}};
  for(unsigned i=0;i<stock.pixels.size();++i)stock.pixels[i]=std::byte(i*7);
  const auto dds=frontend_frame_dds(stock);
  const Bytes header(dds);
  check(dds.size()==160 && header.u32(0)==0x20534444 && header.u32(4)==124 &&
            header.u32(12)==4 && header.u32(16)==8 && header.u32(20)==32 &&
            header.u32(76)==32 && header.u32(84)==0x33545844 &&
            std::equal(stock.pixels.begin(),stock.pixels.end(),dds.begin()+128),
        "frame DDS export changes the original BC2 storage or header identity");
  const auto restored=read_frontend_frame_dds(header,stock);
  check(restored.width==stock.width && restored.height==stock.height && restored.format==15 &&
            restored.pixels==stock.pixels && restored.mips.size()==1 && restored.mips[0].size==32,
        "frame DDS round trip changes stock compressed bytes or mip layout");
  for(const auto change:std::array<std::pair<std::size_t,std::uint32_t>,10>{{
      {0,0},{4,123},{8,0},{12,8},{16,16},{20,16},{28,2},{84,0x35545844},
      {108,0},{112,0x200}}}) {
    auto invalid=dds;
    for(unsigned i=0;i<4;++i)invalid[change.first+i]=std::byte((change.second>>(i*8))&255);
    rejects([&]{read_frontend_frame_dds(Bytes(invalid),stock);});
  }
  auto truncated=dds;truncated.pop_back();
  rejects([&]{read_frontend_frame_dds(Bytes(truncated),stock);});
  auto extra=dds;extra.push_back(std::byte{});
  rejects([&]{read_frontend_frame_dds(Bytes(extra),stock);});
  auto invalid_stock=stock;invalid_stock.mips[0].offset=16;
  rejects([&]{frontend_frame_dds(invalid_stock);});
  invalid_stock=stock;invalid_stock.layers=6;
  rejects([&]{frontend_frame_dds(invalid_stock);});
  invalid_stock=stock;invalid_stock.format=14;
  rejects([&]{frontend_frame_dds(invalid_stock);});
}
void widescreen_background_materials(const FrontendContent &content,
                                    FrontendPainter &painter,const Mesh &mesh,
                                    const std::filesystem::path &fonts) {
  unsigned backgrounds{},foregrounds{};
  for(const auto &material:mesh.materials) {
    const bool decoration=material.name.starts_with("ui\\global_bitmaps\\track") ||
        material.name=="ui/background-dim" ||
        material.name=="ui/vista-pcr-intro-black";
    check(material.ui_widescreen_background==decoration,
          "widescreen expansion includes a foreground or omits shell decoration");
    const auto edge = material.name=="ui\\global_bitmaps\\framing_center/native-left-edge" ?
        UiFrameExtension::Left : material.name=="ui\\global_bitmaps\\framing_center/native-right-edge" ?
        UiFrameExtension::Right : UiFrameExtension::None;
    check(material.ui_frame_extension==edge,
          "frame extension admits a center/foreground material or omits an outer edge");
    check(material.ui_frame_center==
              (material.name=="ui\\global_bitmaps\\framing_center"),
          "center frame sampling admits an extension/foreground material or omits the original page");
    decoration?++backgrounds:++foregrounds;
  }
  check(backgrounds>12 && foregrounds>12,"background/foreground material coverage is empty");
  for(const auto &b:content.screens.at(5).panes.front().bitmaps) {
    const auto &name=content.images.at(b.datum).name;
    check(std::any_of(mesh.materials.begin(),mesh.materials.end(),[&](const auto &m){
            return m.name==name && (name=="ui\\global_bitmaps\\framing_center" ?
                !m.ui_widescreen_background && m.ui_frame_extension==UiFrameExtension::None :
                m.ui_widescreen_background);}),
          "shell frame or timeline remains clipped to the centered canvas");
  }
  // Renaming only the frame disables the extension policy in this control
  // painter. Its unchanged authored draw is the geometry/UV reference, also
  // during introduction; no fitted screen coordinates are used as an oracle.
  auto centered_content=content;
  for(auto &[id,image]:centered_content.images)
    if(image.name=="ui\\global_bitmaps\\framing_center") image.name+="/centered-control";
  Mesh centered_mesh;
  FrontendPainter centered_painter(centered_content,fonts,centered_mesh);
  MenuState shell(content),centered_shell(centered_content);
  shell.open(47);centered_shell.open(47);
  for(float time:std::array<float,3>{0.f,.05f,5.f}) {
    const auto extended=painter.draw(shell,time),baseline=centered_painter.draw(centered_shell,time);
    std::vector<Vertex> center,left,right,authored;
    for(const auto &v:extended.vertices) {
      const auto &m=mesh.materials.at(v.material);
      if(m.name=="ui\\global_bitmaps\\framing_center") center.push_back(v);
      if(m.ui_frame_extension==UiFrameExtension::Left) left.push_back(v);
      if(m.ui_frame_extension==UiFrameExtension::Right) right.push_back(v);
      if(m.ui_frame_extension==UiFrameExtension::None) authored.push_back(v);
    }
    check(authored.size()==baseline.vertices.size(),"frame extension changes authored widget count");
    auto attributes=[](const Vertex &v){return std::array<float,8>{
        v.position.x,v.position.y,v.position.z,v.color.x,v.color.y,v.color.z,v.uv.x,v.uv.y};};
    for(std::size_t i=0;i<authored.size();++i)
      check(attributes(authored[i])==attributes(baseline.vertices[i]),
            "frame extension stretches or moves authored center geometry/UVs/colors");
    if(center.empty()) {check(left.empty()&&right.empty(),"hidden center frame has visible edge copies");continue;}
    check(center.size()==6,"authored framing_center is not one original quad");
    const float x=center[0].position.x,width=center[1].position.x-x;
    const auto sampling=frontend_frame_sampling(x,width,center[0].uv.x,center[1].uv.x,
        mesh.materials.at(center[0].material).diffuse.width);
    for(unsigned side=0;side<2;++side) {
      const auto &edge=side?right:left;
      const float boundary=side?640.f:0.f;
      const bool touches=x<=boundary&&boundary<=x+width;
      check(edge.size()==(touches?6u:0u),"frame edge is extended before the authored art touches it");
      if(!touches)continue;
      const float u=center[0].uv.x+(boundary-x)/width*(center[1].uv.x-center[0].uv.x)+
          (side?-.5f:.5f)/mesh.materials.at(center[0].material).diffuse.width;
      const auto limit=side ? sampling.right_u : sampling.left_u;
      check(limit && *limit==u,"center frame sampling differs from its independent edge");
      for(std::size_t i=0;i<6;++i)
        check(edge[i].uv.x==u && edge[i].uv.y==center[i].uv.y &&
              edge[i].position.y==center[i].position.y && edge[i].position.z==center[i].position.z &&
              edge[i].color.x==center[i].color.x && edge[i].color.y==center[i].color.y &&
              edge[i].color.z==center[i].color.z,
              "frame side strip does not sample the nearest inside boundary texel center/alpha");
      check(edge[0].position.x==0 && edge[1].position.x==640,
            "frame edge strip cannot fill its isolated side viewport");
      if(side && x==0 && width>640)
        check(u!=center[1].uv.x,"right edge incorrectly samples the offscreen texture edge");
    }
  }
  struct TemporaryEdges {
    std::filesystem::path path=std::filesystem::temp_directory_path()/"halo2-frame-edge-dds-test";
    ~TemporaryEdges(){std::error_code error;std::filesystem::remove_all(path,error);}
  } overrides;
  std::filesystem::remove_all(overrides.path);
  const Texture *stock=nullptr;
  for(const auto &[id,image]:content.images)
    if(image.name=="ui\\global_bitmaps\\framing_center")stock=&image.images.at(0);
  check(stock && stock->format==15,"original shell frame is not the expected BC2 source page");
  // Missing directories are read-only fallback paths: constructing a painter
  // must not create assets or change either independent stock texture.
  check(frontend_frame_edge_texture(*stock,overrides.path/"left-edge.dds").pixels==stock->pixels &&
            !std::filesystem::exists(overrides.path),
        "missing frame override changes stock data or writes into game assets");
  std::filesystem::create_directories(overrides.path);
  auto left_texture=*stock;left_texture.pixels.front()^=std::byte{0x5a};
  for(const auto side:std::array<unsigned,2>{0,1}) {
    const auto data=frontend_frame_dds(side?*stock:left_texture);
    std::ofstream file(overrides.path/(side?"right-edge.dds":"left-edge.dds"),std::ios::binary);
    check(bool(file.write(reinterpret_cast<const char *>(data.data()),std::streamsize(data.size()))),
          "cannot write independent frame test fixture");
  }
  Mesh mod_mesh;
  FrontendPainter mod_painter(content,fonts,mod_mesh,overrides.path);
  unsigned center_pages{},left_pages{},right_pages{};
  for(const auto &material:mod_mesh.materials) {
    const Texture *expected=nullptr;
    if(material.name=="ui\\global_bitmaps\\framing_center") {expected=stock;++center_pages;}
    else if(material.ui_frame_extension==UiFrameExtension::Left) {expected=&left_texture;++left_pages;}
    else if(material.ui_frame_extension==UiFrameExtension::Right) {expected=stock;++right_pages;}
    if(expected)check(material.diffuse.format==expected->format && material.diffuse.width==expected->width &&
                      material.diffuse.height==expected->height && material.diffuse.pixels==expected->pixels,
                      "left/right DDS overrides share data or modify the original center page");
  }
  check(center_pages==1 && left_pages==1 && right_pages==1,
        "independent frame texture/material identities are missing");
  MenuState state(content);
  const auto title=painter.draw(state,5);
  const auto &logo=content.images.at(content.screens.at(9).panes.front().bitmaps.front().datum).name;
  check(std::any_of(title.vertices.begin(),title.vertices.end(),[&](const auto &v){
          const auto &m=mesh.materials.at(v.material);return m.name==logo && !m.ui_widescreen_background;}),
        "title logo is widened with decorative strips");
  state.accept();
  const auto profile=painter.draw(state,5);
  check(state.frame().screen==10 && profile.backdrops.size()==1 &&
            profile.backdrops.front().widescreen_background,
        "profile popup leaves side margins outside its tint/blur");
  state.accept();
  const auto keyboard=painter.draw(state,5);
  check(state.frame().screen==167 && keyboard.backdrops.size()==1 &&
            keyboard.backdrops.front().widescreen_background,
        "keyboard popup leaves side margins outside its tint/blur");
}
void attract_timing() {
  using namespace std::chrono;
  const FrontendIdleTimer::Clock::time_point origin{};
  FrontendIdleTimer timer(origin);
  check(!timer.due(origin + seconds(100)),
        "loading time starts attract before the first presented menu frame");
  timer.presented(origin);
  check(!timer.due(origin + milliseconds(74999)) &&
        timer.due(origin + milliseconds(75000)),
        "attract movie does not wait the original 75000 milliseconds");
  timer.activity(origin + seconds(70));
  check(!timer.due(origin + seconds(75)) && timer.due(origin + seconds(145)),
        "controller activity did not restart the attract countdown");
  timer.screen(19, origin + seconds(150));
  check(!timer.due(origin + seconds(250)), "attract interrupted a submenu");
  timer.screen(9, origin + seconds(250));
  timer.presented(origin + seconds(250));
  check(!timer.due(origin + seconds(251)) && timer.due(origin + seconds(325)),
        "returning to the title inherited an expired idle timer");
  // Movie playback ending establishes a fresh full interval.
  timer.activity(origin + seconds(400));
  timer.await_presentation();
  timer.presented(origin + seconds(400));
  check(!timer.due(origin + seconds(474)) && timer.due(origin + seconds(475)),
        "startup movie time counted toward title inactivity");
  bool music = frontend_music_enabled(9, false);
  timer.activity(origin);
  check(frontend_music_wanted(music, timer, origin + milliseconds(69499), 5500),
        "menu music stopped before the original pre-attract interval");
  check(!frontend_music_wanted(music, timer, origin + milliseconds(69500), 5500),
        "menu music ignored the original 5500 millisecond pre-attract silence");
  timer.activity(origin + seconds(70));
  check(resolved_frontend_channel5_screen(std::array{9u, 10u}) == 9 &&
            resolved_frontend_channel5_screen(std::array{6u, 10u}) == 6 &&
            resolved_frontend_channel5_screen(std::array{9u, 10u, 167u}) == 9 &&
            resolved_frontend_channel5_screen(std::array{6u, 10u, 167u}) == 6,
        "profile or keyboard overlay replaced the original channel-five screen");
  check(!resolved_frontend_channel5_screen(std::array{14u, 10u}) &&
            !resolved_frontend_channel5_screen(std::array{10u, 167u}) &&
            !resolved_frontend_channel5_screen(std::array{9u, 8u, 167u}),
        "unverified channel arrangements were claimed resolved");
  timer.screen(*resolved_frontend_channel5_screen(std::array{9u, 10u}),
               origin + seconds(70));
  music = frontend_music_enabled(10, music);
  check(music && frontend_music_wanted(music, timer, origin + milliseconds(139499), 5500),
        "Choose Profile stopped music on entry or before the original fade interval");
  check(!frontend_music_wanted(music, timer, origin + milliseconds(139500), 5500),
        "Choose Profile bypassed channel-five music fade at 69,500 ms");
  check(!timer.due(origin + milliseconds(144999)) && timer.due(origin + seconds(145)),
        "Choose Profile bypassed channel-five attract at 75,000 ms");
  timer.activity(origin + seconds(150));
  timer.screen(*resolved_frontend_channel5_screen(std::array{9u, 10u, 167u}),
               origin + seconds(150));
  music = frontend_music_enabled(167, music);
  check(music && frontend_music_wanted(music, timer,
                                      origin + milliseconds(219499), 5500) &&
            !frontend_music_wanted(music, timer,
                                   origin + milliseconds(219500), 5500),
        "keyboard music did not retain the channel-five fade boundary");
  check(!timer.due(origin + milliseconds(224999)) &&
            timer.due(origin + seconds(225)),
        "keyboard did not retain the channel-five attract boundary");
  check(!frontend_music_enabled(10, frontend_music_enabled(14, music)),
        "profile popup restarted music over a lobby that disabled it");
  check(!frontend_music_enabled(vista_menu(13),true) &&
        !frontend_music_enabled(vista_menu(14),true) &&
        !frontend_music_enabled(vista_menu(186),true) &&
        !frontend_music_enabled(vista_menu(247),true) &&
        frontend_music_enabled(vista_menu(6),false) &&
        frontend_music_enabled(vista_menu(19),true) &&
        !frontend_music_enabled(vista_menu(19),false),
        "native Vista namespace bypassed the original music preference cases");
  check(resolved_frontend_channel5_screen(std::array{vista_menu(9),vista_menu(10),vista_menu(167)})==9,
        "native Vista namespace changed channel-five timing ownership");
}
void choose(MenuState &menu, unsigned index, unsigned expected) {
  menu.select(index);
  menu.accept();
  check(menu.frame().screen == expected, "submenu destination mismatch");
}
void music_debounce() {
  FrontendMusicTransitions music;
  check(music.request(true, 1000) == true && music.playing() &&
            music.last_change() == 1000,
        "initial music start did not establish its debounce time");
  check(!music.request(false, 7000),
        "music changed at exactly 6000 ms after its initial start");
  check(music.request(false, 70500) == false && !music.playing(),
        "pre-attract fade did not establish the stopped music state");
  check(!music.request(true, 76001) && !music.playing(),
        "early attract skip bypassed the original music debounce");
  check(!music.request(true, 76500) &&
            music.request(true, 76501) == true && music.playing(),
        "movie restart did not wait until strictly more than 6000 ms");
  check(!music.request(true, 80000),
        "retained modal preference restarted already playing music");
}
void keyboard_default_names(const FrontendContent &content) {
  MenuState sample(content);
  for (const auto *name : {"Halo0001", "Halo0002", "Halo0003", "Halo0004"}) {
    sample.accept();
    choose(sample, 0, 167);
    check(sample.keyboard_text == name && sample.frame().selected == 40,
          "Create New Profile does not use the original first-unused generated name/DONE focus");
    sample.commit_text();
    check(sample.frame().screen == 38 && sample.profile_name() == name,
          "accepting the original default name skips profile customization");
    sample.back();
    check(sample.frame().screen == 6,
          "default-name fixture did not sign in the created profile");
    sample.back();
    choose(sample, 0, 9);
  }
}
void keyboard_geometry(const FrontendContent &content, const MenuState &menu,
                       FrontendPainter &painter, const Mesh &mesh,
                       const std::filesystem::path &maps) {
  check(menu.frame().screen == 167,
        "keyboard geometry repro did not open Create New Profile");
  check(menu.keyboard_text == "Halo0001",
        "fresh Create New Profile lacks the original generated default name");
  auto replacement = menu;
  replacement.text("A");
  check(replacement.keyboard_text == "A",
        "first typed key does not replace the originally selected default name");
  auto cursor_edit = menu;
  cursor_edit.cursor(-1);
  cursor_edit.text("A");
  check(cursor_edit.keyboard_text == "Halo000A1",
        "moving the original end cursor does not clear default-name selection");
  const auto &keys = content.screens.at(167).panes.front().buttons;
  check(keys.size() == 47 && keys.front().text.bounds.top == 7 &&
            keys.front().text.bounds.left == -257 &&
            keys.front().text.bounds.bottom == -23 &&
            keys.front().text.bounds.right == -222 &&
            std::all_of(keys.begin(), keys.end(), [](const UiButton &key) {
              return key.offset.x == 0 && key.offset.y == -5;
            }),
        "keyboard geometry fixture differs from the retail button definitions");
  std::set<std::string> key_images;
  for (const auto &key : keys)
    key_images.insert(content.images.at(key.bitmap).name);
  const auto draw = painter.draw(menu, 5);
  check(draw.backdrops.size() == 1 &&
            draw.backdrops.front().before_vertex > 0 &&
            draw.backdrops.front().before_vertex < draw.vertices.size(),
        "Profile Name omits the original retained title-channel popup boundary");
  const auto &pane_bitmaps = content.screens.at(167).panes.front().bitmaps;
  check(pane_bitmaps.size() == 2 &&
            content.images.at(pane_bitmaps[0].datum).name ==
                "ui\\screens\\game_shell\\virtual_keyboard\\field_2" &&
            content.images.at(pane_bitmaps[1].datum).name ==
                "ui\\global_bitmaps\\fs_dialog",
        "keyboard input-field fixture differs from original pane artwork");
  std::size_t field_quad = draw.vertices.size(), dialog_quad = draw.vertices.size();
  for (std::size_t i = draw.backdrops.back().before_vertex;
       i < draw.vertices.size(); i += 6) {
    const auto &name = mesh.materials.at(draw.vertices[i].material).name;
    if (name == content.images.at(pane_bitmaps[0].datum).name)
      field_quad = i;
    else if (name == content.images.at(pane_bitmaps[1].datum).name)
      dialog_quad = i;
  }
  check(dialog_quad < field_quad && field_quad < draw.vertices.size(),
        "keyboard dialog covers the authored layer1 input-field bitmap");
  std::vector<std::size_t> key_quads;
  for (std::size_t i = 0; i < draw.vertices.size(); i += 6)
    if (key_images.contains(mesh.materials.at(draw.vertices[i].material).name))
      key_quads.push_back(i);
  check(key_quads.size() == 45,
        "profile keyboard still draws hidden original keys 43/44 over inactive 45/46");
  for (std::size_t visible = 0; visible < key_quads.size(); ++visible) {
    const auto i = visible < 43 ? visible : visible + 2;
    const auto &key = keys[i];
    const auto &image = content.images.at(key.bitmap);
    const auto frame = i == menu.frame().selected ? 1u : 0u;
    UiSprite sprite;
    if (!image.sequences.empty() && !image.sequences.front().empty())
      sprite = image.sequences.front().at(frame);
    else
      sprite.image = std::min(frame, unsigned(image.images.size() - 1));
    const auto &texture = image.images.at(sprite.image);
    const float width = texture.width * (sprite.right - sprite.left);
    const float height = texture.height * (sprite.bottom - sprite.top);
    // Original button render 0x253EA8: horizontal offset adds at 0x253FCB,
    // vertical offset subtracts from the widget top at 0x253FB0.
    const float x = 320 + key.text.bounds.left + key.offset.x -
                    sprite.registration.x * width;
    const float y = 240 - (key.text.bounds.top - key.offset.y) -
                    sprite.registration.y * height;
    const auto &vertex = draw.vertices.at(key_quads[visible]);
    check(vertex.position.x == x && vertex.position.y == y,
          "keyboard key art offset differs from original 0x253EA8; labels appear too high");
  }
  const auto &key = keys.front();
  const auto font = load_retail_font(maps / "fonts/conduit-12");
  const auto &glyph = font.glyphs.at('1');
  const auto &label = draw.vertices.at(key_quads.front() + 12);
  check(mesh.materials.at(label.material).name == "font/conduit-12" &&
            label.position.y == 240 - key.text.bounds.top + font.ascent -
                                    glyph.bearing_y,
        "keyboard label no longer follows the original font ascent and bearing");
  check(label.color.x == 98.f / 255 &&
            label.color.y == 147.f / 255 &&
            label.color.z == 193.f / 255 && label.position.z == 1,
        "unfocused keyboard label has an invented brightness multiplier");
  auto digit_focus = menu;
  digit_focus.select(0);
  const auto focused_draw = painter.draw(digit_focus, 5);
  std::size_t first_key = 0;
  while (!key_images.contains(mesh.materials.at(
      focused_draw.vertices.at(first_key).material).name))
    first_key += 6;
  const auto &selected_label = focused_draw.vertices.at(first_key + 12);
  check(selected_label.color.x == 172.f / 255 &&
            selected_label.color.y == 195.f / 255 &&
            selected_label.color.z == 216.f / 255 &&
            selected_label.position.z == 1,
        "focused keyboard label does not use original wigl title RGB");
  auto blocked = menu;
  for (const auto index : {43u, 44u, 45u, 46u}) {
    blocked.select(index);
    check(blocked.frame().selected == 40,
          "profile keyboard focuses an original hidden or disabled key");
  }
}
void keyboard_input_overlay(const FrontendContent &content,
                            const MenuState &menu, FrontendPainter &painter,
                            const Mesh &mesh) {
  const auto draw = painter.draw(menu, 5, -1, 0);
  auto find_quad = [&](const UiDraw &d, const char *name) {
    std::size_t result = d.vertices.size();
    for (std::size_t i = d.backdrops.back().before_vertex;
         i < d.vertices.size(); i += 6)
      if (mesh.materials.at(d.vertices[i].material).name == name)
        result = i;
    return result;
  };
  const auto selection = find_quad(draw, "rasterizer\\default_2d");
  check(selection < draw.vertices.size(),
        "keyboard omits the original selected default-name bitmap overlay");
  const auto &sel = draw.vertices.at(selection);
  check(mesh.materials.at(sel.material).diffuse.width == 4 &&
            mesh.materials.at(sel.material).diffuse.height == 4 &&
            mesh.materials.at(sel.material).diffuse.pixels ==
                content.images.at(0xe4750255).images.at(1).pixels &&
            sel.position.x == 72 && sel.position.y == 167 &&
            draw.vertices.at(selection+4).position.x == 137 &&
            draw.vertices.at(selection+4).position.y == 189 &&
            sel.color.x == 1 && sel.color.y == 1 && sel.color.z == 1 &&
            sel.position.z == 1,
        "keyboard selected-name rectangle differs from original glyph-ink bounds");
  const auto cursor = find_quad(draw, "ui\\global_bitmaps\\global_bitmaps");
  check(cursor > selection && cursor < draw.vertices.size(),
        "keyboard omits the original bitmap caret after selected-name overlay");
  const auto &pane = content.screens.at(167).panes.front();
  check(pane.texts[0].layer == 5 && pane.texts[1].layer == 5 &&
            pane.bitmaps[0].layer == 1 && pane.bitmaps[1].layer == 0 &&
            std::all_of(pane.buttons.begin(), pane.buttons.end(),
                        [](const UiButton &key) { return key.text.layer == 2; }),
        "keyboard text/button layer metadata differs from original setup");
  const auto dialog = find_quad(draw, "ui\\global_bitmaps\\fs_dialog");
  const auto field = find_quad(draw,
      "ui\\screens\\game_shell\\virtual_keyboard\\field_2");
  auto field_text = draw.vertices.size();
  auto first_key = draw.vertices.size();
  std::set<std::string> key_images;
  for (const auto &key : pane.buttons)
    key_images.insert(content.images.at(key.bitmap).name);
  for (std::size_t i = draw.backdrops.back().before_vertex;
       i < draw.vertices.size(); i += 6) {
    const auto &v = draw.vertices[i];
    const auto &name = mesh.materials.at(v.material).name;
    if (name == "font/conduit-13" && v.position.y == 167 &&
        (v.color.x > 0 || v.color.y > 0 || v.color.z > 0))
      field_text = std::min(field_text, i);
    if (key_images.contains(name))
      first_key = std::min(first_key, i);
  }
  check(dialog < selection && cursor < field && field < first_key &&
            first_key < field_text && field_text < draw.vertices.size(),
        "keyboard selection/caret cover the original layer1 field and layer5 name text");
  const auto &caret = draw.vertices.at(cursor);
  check(mesh.materials.at(caret.material).diffuse.width == 6 &&
            mesh.materials.at(caret.material).diffuse.height == 17 &&
            mesh.materials.at(caret.material).diffuse.pixels ==
                content.images.at(0xe4d702b7).images.at(0).pixels &&
            caret.position.x == 137 && caret.position.y == 167 &&
            draw.vertices.at(cursor+4).position.x == 138 &&
            draw.vertices.at(cursor+4).position.y == 187 &&
            caret.position.z == 1,
        "keyboard bitmap caret differs from original prefix line bounds");
  const auto clocked = painter.draw(menu, 5, -1, 1000);
  const auto clocked_cursor = find_quad(clocked, "ui\\global_bitmaps\\global_bitmaps");
  check(clocked.vertices.at(clocked_cursor).position.z == 1.f / 255,
        "keyboard caret uses screen-entry time instead of original cumulative UI clock");
  auto prefix = menu;
  prefix.cursor(-1);
  const auto moved = painter.draw(prefix, 5, -1, 0);
  check(find_quad(moved, "rasterizer\\default_2d") == moved.vertices.size(),
        "moving the caret retains the originally cleared name selection");
  const auto moved_cursor = find_quad(moved, "ui\\global_bitmaps\\global_bitmaps");
  check(moved.vertices.at(moved_cursor).position.x == 128 &&
            moved.vertices.at(moved_cursor+4).position.x == 129,
        "keyboard caret does not measure the actual original seven-character prefix");
  check(frontend_ui_clock_advance(0, 1.f / 60) == 16 &&
            frontend_ui_clock_advance(0, 1.f / 30) == 33 &&
            frontend_ui_clock_advance(1000, .016999999061226845f) == 1016 &&
            frontend_ui_clock_advance(0xfffffff0u, .025f) == 9 &&
            frontend_ui_clock_advance(0, 0) == 0 &&
            frontend_ui_clock_advance(0, 10) == 10000,
        "UI clock differs from original per-frame x87 conversion or wrapping addition");
  constexpr unsigned phases[] = {0, 250, 500, 1000, 1047, 1500, 2094,
                                  0x80000000u, 0xffffffffu};
  constexpr unsigned alpha[] = {255, 221, 137, 1, 0, 101, 255, 240, 199};
  constexpr unsigned quarter[] = {64, 55, 34, 0, 0, 25, 64, 60, 50};
  for (unsigned i = 0; i < std::size(phases); ++i)
    check(frontend_keyboard_caret_alpha(phases[i], 1) == alpha[i] &&
              frontend_keyboard_caret_alpha(phases[i], .25f) == quarter[i],
          "keyboard caret alpha differs from original executed x87 corridor");
}
void keyboard_description_wrap(const FrontendContent &content,
                               const MenuState &menu, FrontendPainter &painter,
                               const Mesh &mesh,
                               const std::filesystem::path &maps) {
  const auto &definition = content.screens.at(167).panes.front().texts.at(1);
  check(definition.font == 1 && definition.bounds.left == -254 &&
            definition.bounds.right == 263 && definition.bounds.top == 149 &&
            definition.bounds.bottom == 89,
        "keyboard description fixture differs from retail authored text bounds");
  const std::array<std::string, 2> lines{
      "Use the virtual keyboard to enter a name for this profile. Profile names may be up ",
      "to 16 characters."};
  check(menu.frame().description == lines[0] + lines[1],
        "keyboard wrap regression did not use the actual localized description");
  const auto font = load_retail_font(maps / "fonts/conduit-12");
  const auto draw = painter.draw(menu, 5);
  std::vector<const Vertex *> glyphs;
  for (std::size_t i = draw.backdrops.back().before_vertex;
       i < draw.vertices.size(); i += 6) {
    const auto &v = draw.vertices[i];
    if (mesh.materials.at(v.material).name == "font/conduit-12" &&
        v.position.y >= 91 && v.position.y < 151 &&
        (v.color.x > 0 || v.color.y > 0 || v.color.z > 0))
      glyphs.push_back(&v);
  }
  check(glyphs.size() == lines[0].size() + lines[1].size(),
        "keyboard description has the wrong original line-break glyph traversal");
  std::size_t index = 0;
  for (unsigned line = 0; line < lines.size(); ++line) {
    float x = 66;
    unsigned previous = 0;
    for (const unsigned c : lines[line]) {
      const auto &g = font.glyphs.at(c);
      const auto it = font.kerning.find({previous, c});
      const float kern = it == font.kerning.end() ? 0.f : it->second;
      const auto &v = *glyphs.at(index++);
      check(v.position.x == x + g.bearing_x + kern &&
                v.position.y == 91 + line * 19 + font.ascent - g.bearing_y &&
                v.uv.x == float(g.x) / font.atlas.width &&
                v.uv.y == float(g.y) / font.atlas.height,
            "keyboard description wraps beyond the original strict-fit boundary at 'up'");
      x = g.advance + x + g.bearing_x + kern;
      previous = c;
    }
  }
}
void profile_pointer_geometry(const FrontendContent &content,
                              FrontendPainter &painter, const Mesh &mesh) {
  MenuState sample(content);
  for (const auto *name : {"Hover One", "Hover Two"}) {
    check(sample.frame().screen == 9, "profile pointer fixture missed title");
    sample.accept();
    check(sample.frame().screen == 10,
          "profile pointer fixture did not press Start on title");
    choose(sample, 0, 167);
    sample.text(name);
    sample.commit_text();
    check(sample.frame().screen == 38,
          "profile pointer fixture skipped profile customization");
    sample.back();
    check(sample.frame().screen == 6,
          "profile pointer fixture did not sign in the created profile");
    sample.back();
    choose(sample, 0, 9);
  }
  sample.accept();
  check(sample.frame().screen == 10 && sample.frame().entries.size() == 3,
        "profile pointer fixture requires Create New and two saved profiles");
  const auto &list = content.screens.at(10).panes.front().lists.front();
  const auto &skin = content.skins.at(list.skin);
  const auto &end_cap = content.images.at(skin.bitmaps.front().datum);
  check(end_cap.name == "ui\\list_skins\\profile_picker\\end_cap" &&
            end_cap.images.size() == 3,
        "profile pointer fixture lost the three authored end-cap frames");
  auto quads = [&](const UiDraw &draw, const std::string &name) {
    std::vector<std::size_t> result;
    for (std::size_t i = 0; i < draw.vertices.size(); i += 6)
      if (mesh.materials.at(draw.vertices[i].material).name == name)
        result.push_back(i);
    return result;
  };
  // Original 24ADD8 selects focused frame 0, earlier-item frame 1, and
  // later-item frame 2. Three rows with the middle focused exercise all three.
  sample.select(1);
  const auto before = painter.draw(sample, 5);
  const auto icons = quads(before, end_cap.name);
  check(icons.size() == 3, "profile pointer fixture omitted end-cap icons");
  check(before.vertices.at(icons[1]).position.y-
                before.vertices.at(icons[0]).position.y == 21 &&
            before.vertices.at(icons[2]).position.y-
                before.vertices.at(icons[1]).position.y == 21,
        "profile rows do not use the original nested-skin union height 21");
  constexpr unsigned frames[] = {1, 0, 2};
  for (unsigned row = 0; row < 3; ++row) {
    const auto frame = frames[row];
    UiSprite sprite;
    if (!end_cap.sequences.empty() && !end_cap.sequences.front().empty())
      sprite = end_cap.sequences.front().at(frame);
    else
      sprite.image = frame;
    const auto &texture = end_cap.images.at(sprite.image);
    const auto &vertex = before.vertices.at(icons[row]);
    const auto &material = mesh.materials.at(vertex.material);
    check(material.diffuse.width == texture.width &&
              material.diffuse.height == texture.height &&
              material.diffuse.pixels == texture.pixels &&
              vertex.uv.x == sprite.left && vertex.uv.y == sprite.top &&
              before.vertices.at(icons[row]+4).uv.x == sprite.right &&
              before.vertices.at(icons[row]+4).uv.y == sprite.bottom,
          "profile end-cap frames/UVs differ from original focused/before/after order");
  }
  const std::string stripe_name =
      "ui\\list_skins\\profile_picker\\item_background";
  const auto stripes_before = quads(before, stripe_name);
  check(stripes_before.size() == 1,
        "Create New Profile hover repro has no authored highlight stripe");
  const auto stripe_before = stripes_before.front();
  const auto &unfocused = before.vertices.at(stripe_before);
  check(skin.animations.at(0).keys.back().alpha == 1 &&
            skin.animations.at(2).keys.back().alpha == .3f,
        "profile pointer fixture differs from original focus widget opacity");
  // Original 24AC80 -> virtual v5 at 22E930 replaces each child's
  // animation; text/bitmap drawing reads its own scale at widget+0x64.
  check(unfocused.position.z == .3f,
        "unfocused Create New Profile ignores the authored idle opacity");
  const float pointer_x = before.vertices.at(stripe_before+4).position.x-1;
  const float pointer_y = unfocused.position.y+1;
  // The stripe's right edge extends beyond the old text-only hit rectangle.
  const auto hovered = painter.hit(pointer_x, pointer_y);
  check(hovered && *hovered == 0,
        "mouse hover misses the authored Create New Profile row extent");
  sample.select(*hovered);
  const auto focused = painter.draw(sample, 5);
  const auto stripes_focused = quads(focused, stripe_name);
  check(stripes_focused.size() == 1,
        "mouse hover hid the Create New Profile highlight stripe");
  const auto &highlight = focused.vertices.at(stripes_focused.front());
  check(highlight.position.z == 1 &&
            highlight.position.x-unfocused.position.x == 17 &&
            highlight.position.y == unfocused.position.y,
        "Create New Profile hover did not apply original focus opacity/motion");
  const auto pointer_after_focus = painter.hit(pointer_x, pointer_y);
  check(pointer_after_focus && *pointer_after_focus == 0,
        "Create New Profile focus movement loses the pointer target");
  sample.accept();
  check(sample.frame().screen == 167,
        "clicking hovered Create New Profile no longer opens Profile Name");
  sample.back();
  check(sample.frame().screen == 9,
        "Profile Name cancel restores the disposed Choose Profile channel instead of title");
  sample.accept();
  check(sample.frame().screen == 10,
        "Start after cancel no longer opens a fresh Choose Profile popup");
}
void profile_saved_row_appearance(const FrontendContent &content,
                                  FrontendPainter &painter, const Mesh &mesh,
                                  const std::filesystem::path &maps) {
  const auto fixture = std::filesystem::path(__FILE__).parent_path().parent_path() /
      "analysis/frontend/profile-regression/frontend-appearance-original.tsv";
  std::ifstream input(fixture);
  check(bool(input), "saved-profile row regression lacks original appearance fixture");
  std::array<Vec3, 18> original_names{};
  unsigned name_cases = 0;
  for (std::string line; std::getline(input, line);) {
    std::istringstream row(line);
    std::string kind;
    row >> kind;
    if (kind != "name") continue;
    unsigned index;
    std::array<std::uint32_t, 3> words;
    std::string word;
    check(bool(row >> index) && index < original_names.size(),
          "invalid original saved-profile name fixture index");
    for (auto &value : words) {
      check(bool(row >> word), "incomplete original saved-profile name color");
      value = std::uint32_t(std::stoul(word, nullptr, 16));
    }
    original_names[index] = std::bit_cast<Vec3>(words);
    ++name_cases;
  }
  check(name_cases == 18 &&
      std::bit_cast<std::array<std::uint32_t, 3>>(original_names[10]) ==
          std::array<std::uint32_t, 3>{0x3f2071c8, 0x3f37b883, 0x3f4aaaab},
      "saved-profile name fixture differs from executed 2AFEAE color10 output");
  const auto preferences = std::filesystem::temp_directory_path() /
      "halo2-frontend-saved-row-test.cfg";
  std::filesystem::remove(preferences);
  MenuState sample(content, preferences);
  constexpr std::array<const char *, 2> names{"Saved Blue", "Saved Tan"};
  constexpr std::array<unsigned, 2> primary{10, 17};
  // Literal original2B05C8 bitmap packing of palette10/17, also frozen in
  // appearance-namebar-original.json. Text uses its separate truncation path.
  constexpr std::array<std::uint32_t, 2> background_pixels{0xff416c8f, 0xffb19256};
  for (unsigned profile = 0; profile < names.size(); ++profile) {
    check(sample.frame().screen == 9, "saved-profile row fixture missed title");
    sample.accept();
    choose(sample, 0, 167);
    sample.text(names[profile]);
    sample.commit_text();
    check(sample.frame().screen == 38, "saved-profile row fixture skipped creation");
    choose(sample, 1, 47);
    choose(sample, 1, 50);
    sample.select(primary[profile]);
    sample.accept();
    sample.back();
    sample.back();
    check(sample.frame().screen == 6 &&
              sample.profile_appearance().colors[0] == int(primary[profile]),
          "saved-profile row fixture did not commit distinct profile colors");
    sample.back();
    choose(sample, 0, 9);
  }
  auto saved_bytes = [&] {
    std::ifstream file(preferences, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), {});
  };
  const auto persisted = saved_bytes();
  sample.accept();
  check(sample.frame().screen == 10 && sample.frame().entries.size() == 3,
        "saved-profile row fixture needs Create New and two profiles");
  auto verify = [&](unsigned screen, const std::string &committed,
                    const std::string &persisted_bytes) {
    check(sample.frame().screen == screen, "saved-profile row fixture has wrong screen");
    const auto &list = content.screens.at(screen).panes.front().lists.front();
    const auto &skin = content.skins.at(list.skin);
    check(skin.grids.size() == 1, "saved-profile fixture has unexpected nested groups");
    const auto &grid = skin.grids.front();
    const auto &group = content.skin_groups.at(grid.skin);
    const auto background = std::find_if(group.bitmaps.begin(), group.bitmaps.end(),
        [](const auto &b) { return b.placeholder == 0x1400069a; });
    const auto name = std::find_if(group.texts.begin(), group.texts.end(),
        [](const auto &t) { return t.string == 0x0b000698; });
    check(background != group.bitmaps.end() && name != group.texts.end(),
          "saved-profile row lacks its authored appearance children");
    // Original2BACBC/2BAFA4 profile row placement: name(28,36,8,273),
    // primary bar(26,32,7,272). The profile2 bank shortens right bounds only.
    check(grid.position.x + name->bounds.left == 36 &&
              grid.position.y + name->bounds.top == 28 &&
              grid.position.y + name->bounds.bottom == 8 &&
              grid.position.x + background->position.x == 32 &&
              grid.position.y + background->position.y == 7 &&
              (name->flags & 1),
          "saved-profile row source fixture bounds changed");
    const auto &image = content.images.at(background->datum);
    check(image.images.front().height == 19,
          "saved-profile source primary bar is no longer 19 pixels high");
    check(name->font == (screen == 10 ? 1 : 4),
          "saved-profile row source font changed");
    const auto font = load_retail_font(maps / "fonts" /
                                     (name->font == 1 ? "conduit-12" : "conduit-13"));
    const std::string material = name->font == 1 ? "font/conduit-12" : "font/conduit-13";
    const auto outer_font = load_retail_font(maps / "fonts" / "conduit-12");
    for (const unsigned selected : {1u, 2u, 0u}) {
      sample.select(selected);
      const auto draw = painter.draw(sample, 5);
      if (screen == 193) {
        const auto &pane = content.screens.at(screen).panes.front();
        check(pane.grids.size() == 1 && pane.grids.front().position.x == -269 &&
                  pane.grids.front().position.y == 35,
              "focused-profile header source group placement changed");
        const auto &header_group = content.skin_groups.at(pane.grids.front().skin);
        check(header_group.texts.size() == 1 &&
                  header_group.texts.front().string == 0x0b000698 &&
                  header_group.texts.front().font == 4 &&
                  header_group.texts.front().flags == 0 &&
                  header_group.texts.front().bounds.top == 0 &&
                  header_group.texts.front().bounds.left == 0 &&
                  header_group.texts.front().bounds.bottom == -20 &&
                  header_group.texts.front().bounds.right == 128,
              "focused-profile header source name definition changed");
        // Unchanged24A886 passes the focused record's name/appearance to
        //22F042 and hides static text(6,3) in every case. CreateNew's flags20
        // do not enable the group's0B000698 name (profile-details-original).
        constexpr float header_left = 51, header_top = 205, header_right = 179;
        const auto header_foreground = [&](const Vertex &v) {
          return mesh.materials.at(v.material).name == "font/conduit-13" &&
              v.position.x >= header_left && v.position.x < header_right &&
              v.position.y >= header_top && v.position.y < header_top + 20 &&
              (v.color.x > 0 || v.color.y > 0 || v.color.z > 0);
        };
        if (!selected) {
          check(std::none_of(draw.vertices.begin(), draw.vertices.end(), header_foreground),
                "CreateNew retains the saved-profile header contrary to original24A886");
        } else {
          const auto header_font = load_retail_font(maps / "fonts" / "conduit-13");
          const auto &value = names[selected - 1];
          float width = 0;
          unsigned previous = 0;
          for (const unsigned c : std::string(value)) {
            const auto &g = header_font.glyphs.at(c);
            const auto at = header_font.kerning.find({previous, c});
            const float kerning = at == header_font.kerning.end() ? 0.f : at->second;
            width = g.advance + g.bearing_x + kerning + width;
            previous = c;
          }
          const auto &g = header_font.glyphs.at('S');
          const float x = (header_left + header_right - width) * .5f + g.bearing_x;
          const float y = header_top + header_font.ascent - g.bearing_y;
          const auto rgb = original_names[primary[selected - 1]];
          const Vec3 packed{float(unsigned(rgb.x * 255.f)) / 255.f,
                            float(unsigned(rgb.y * 255.f)) / 255.f,
                            float(unsigned(rgb.z * 255.f)) / 255.f};
          check(std::any_of(draw.vertices.begin(), draw.vertices.end(), [&](const auto &v) {
                  return header_foreground(v) && v.position.x == x && v.position.y == y &&
                      v.position.z == 1 && v.uv.x == float(g.x) / header_font.atlas.width &&
                      v.uv.y == float(g.y) / header_font.atlas.height &&
                      v.color.x == packed.x && v.color.y == packed.y && v.color.z == packed.z;
                }), "profile header does not bind the original focused name/color/bounds");
        }
      }
      std::vector<const Vertex *> bars;
      for (std::size_t i = 0; i < draw.vertices.size(); i += 6)
        if (mesh.materials.at(draw.vertices[i].material).name == image.name)
          bars.push_back(&draw.vertices[i]);
      check(bars.size() == 2,
            "saved profile rows omit original appearance-colored backgrounds");
      for (unsigned profile = 0; profile < names.size(); ++profile) {
        const unsigned row = profile + 1;
        const bool focused = selected == row;
        const float shift = focused ? 17.f : 0.f;
        // Original focus propagation ends at opacity1 / .3. Original bitmap
        // nearest-even and text truncation both pack .3 as alpha76 here.
        const float alpha = focused ? 1.f : 76.f / 255.f;
        const float px = list.position.x + shift;
        const float py = list.position.y - float(row) * 21;
        const auto &bar = *bars[profile];
        const auto pixel = background_pixels[profile];
        check(bar.position.x == 320 + px + 32 &&
                  bar.position.y == 240 - py - 26 && bar.position.z == alpha &&
                  bar.color.x == float((pixel >> 16) & 255) / 255.f &&
                  bar.color.y == float((pixel >> 8) & 255) / 255.f &&
                  bar.color.z == float(pixel & 255) / 255.f,
              "saved-profile bar does not follow its own source palette/placement/focus");
        const auto color = original_names[primary[profile]];
        const Vec3 packed{float(unsigned(color.x * 255.f)) / 255.f,
                          float(unsigned(color.y * 255.f)) / 255.f,
                          float(unsigned(color.z * 255.f)) / 255.f};
        const float left = 320 + px + 36, top = 240 - py - 28;
        const float right = 320 + px + grid.position.x + name->bounds.right;
        float x = left;
        unsigned previous = 0;
        for (const unsigned c : std::string(names[profile])) {
          const auto &glyph = font.glyphs.at(c);
          const auto kern = font.kerning.find({previous, c});
          const float kerning = kern == font.kerning.end() ? 0.f : kern->second;
          const float raw_x = x + glyph.bearing_x + kerning;
          const float raw_y = top + font.ascent - glyph.bearing_y;
          const float gx = std::max(left, raw_x);
          const float gy = std::max(top, raw_y);
          if (glyph.width && glyph.height && gx < right && gy < top + 20)
            check(std::any_of(draw.vertices.begin(), draw.vertices.end(), [&](const auto &v) {
                    return mesh.materials.at(v.material).name == material &&
                        v.position.x == gx && v.position.y == gy && v.position.z == alpha &&
                        v.uv.x == (glyph.x + gx - raw_x) / font.atlas.width &&
                        v.uv.y == (glyph.y + gy - raw_y) / font.atlas.height &&
                        v.color.x == packed.x && v.color.y == packed.y && v.color.z == packed.z;
                  }), "saved name is not drawn in its original nested bounds/color/font");
          x = glyph.advance + x + glyph.bearing_x + kerning;
          previous = c;
        }
        const auto &first = outer_font.glyphs.at('S');
        check(std::none_of(draw.vertices.begin(), draw.vertices.end(), [&](const auto &v) {
                return mesh.materials.at(v.material).name == "font/conduit-12" &&
                    v.position.x == 320 + px + 20 + first.bearing_x &&
                    v.position.y == top + outer_font.ascent - first.bearing_y &&
                    v.uv.x == float(first.x) / outer_font.atlas.width &&
                    v.uv.y == float(first.y) / outer_font.atlas.height &&
                    (v.color.x > 0 || v.color.y > 0 || v.color.z > 0);
              }), "saved profile still draws its name in the outer Create New field");
      }
      check(sample.profile_name() == committed && saved_bytes() == persisted_bytes,
            "saved-profile row focus committed another profile or altered preferences");
    }
  };
  verify(10, "Saved Tan", persisted);
  choose(sample, 1, 6);
  choose(sample, 4, 19);
  choose(sample, 0, 193);
  // Signing into the first profile is the existing explicit accept boundary.
  const auto signed_in_bytes = saved_bytes();
  check(sample.profile_name() == "Saved Blue", "saved-row fixture did not accept first profile");
  // The same exact geometry/colors must bind each saved row on193 without
  // accepting its hover or reusing only the selected appearance bank.
  verify(193, "Saved Blue", signed_in_bytes);
  std::filesystem::remove(preferences);
}
void profile_row_emblems(const FrontendContent &content,
                         FrontendPainter &painter, const Mesh &mesh) {
  const auto fixture = std::filesystem::path(__FILE__).parent_path().parent_path() /
      "analysis/frontend/profile-row-appearance/profile-row-emblems-original.tsv";
  std::ifstream input(fixture);
  check(bool(input), "saved-row emblem regression lacks original row fixture");
  std::map<unsigned, std::array<int, 8>> placements;
  std::map<std::string, FrontendAppearance> appearances;
  std::map<std::pair<unsigned, std::string>, std::uint32_t> opacities;
  unsigned cases = 0;
  for (std::string line; std::getline(input, line);) {
    std::istringstream row(line);
    std::string kind;
    row >> kind;
    if (kind == "placement") {
      unsigned screen;
      std::array<int, 8> value{};
      row >> screen;
      for (auto &field : value) row >> field;
      check(bool(row), "incomplete original row-emblem placement");
      placements.emplace(screen, value);
    } else if (kind == "case") {
      unsigned screen, index, visible, admitted;
      std::string label, opacity, hex;
      std::array<float, 3> offset{};
      row >> screen >> label >> index >> visible >> admitted >> opacity;
      for (auto &field : offset) row >> field;
      row >> hex;
      const bool focused = label == "blue_focused" || label == "tan_focused" ||
                           label == "create_after_tan";
      check(bool(row) && visible == unsigned(index != 0) && admitted == visible &&
                offset == (focused ? std::array<float, 3>{17, 0, 0}
                                   : std::array<float, 3>{0, 0, 0}) && hex.size() == 32,
            "original row-emblem visibility/focus fixture changed");
      const auto word = std::uint32_t(std::stoul(opacity, nullptr, 16));
      check(word == (focused ? 0x3f800000u : 0x3e99999au),
            "original row-emblem opacity changed");
      opacities.emplace(std::pair{screen, label}, word);
      if (screen == 193 && (label == "blue_focused" || label == "tan_focused")) {
        std::array<unsigned char, sizeof(FrontendAppearance)> bytes{};
        for (unsigned i = 0; i < bytes.size(); ++i)
          bytes[i] = static_cast<unsigned char>(std::stoul(hex.substr(i * 2, 2), nullptr, 16));
        appearances.emplace(label, std::bit_cast<FrontendAppearance>(bytes));
      }
      ++cases;
    } else {
      check(false, "unknown original row-emblem fixture record");
    }
  }
  check(cases == 12 && appearances.size() == 2 && placements.size() == 2 &&
            placements.at(10) == std::array<int, 8>{1, 11, 0, 26, 12, -6, 44, 21} &&
            placements.at(193) == placements.at(10),
        "original row-emblem constructor/placement coverage changed");
  const std::array<FrontendAppearance, 3> expected{
      appearances.at("blue_focused"), appearances.at("tan_focused"),
      appearances.at("blue_focused")};
  const std::array<const char *, 3> names{"Saved Blue", "Saved Tan", "Saved Twin"};
  const auto preferences = std::filesystem::temp_directory_path() /
      "halo2-frontend-row-emblems-test.cfg";
  std::filesystem::remove(preferences);
  MenuState sample(content, preferences);
  for (unsigned i = 0; i < names.size(); ++i) {
    sample.accept();
    choose(sample, 0, 167);
    sample.text(names[i]);
    sample.commit_text();
    choose(sample, 1, 47);
    choose(sample, 1, 50);
    sample.select(unsigned(expected[i].colors[0]));
    sample.accept();
    choose(sample, 5, 49);
    sample.select(expected[i].foreground);
    if (expected[i].flags & 1) sample.auxiliary();
    sample.accept();
    choose(sample, 6, 233);
    sample.select(expected[i].background);
    sample.accept();
    choose(sample, 0, 52);
    sample.select(unsigned(expected[i].species));
    sample.accept();
    sample.back();
    sample.back();
    check(sample.frame().screen == 6 && sample.profile_appearance() == expected[i],
          "saved-row emblem fixture did not commit its controlled appearance");
    sample.back();
    choose(sample, 0, 9);
  }
  auto saved_bytes = [&] {
    std::ifstream file(preferences, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), {});
  };
  sample.accept();
  auto verify = [&](unsigned screen) {
    check(sample.frame().screen == screen && sample.frame().entries.size() == 4,
          "saved-row emblem fixture missed its ordinary picker route");
    const auto committed = sample.profile_name();
    const auto persisted = saved_bytes();
    const auto material_count = mesh.materials.size();
    const auto &list = content.screens.at(screen).panes.front().lists.front();
    const auto &skin = content.skins.at(list.skin);
    check(list.visible >= 4 && skin.grids.size() == 1,
          "saved-row emblem fixture lacks four visible ordinary rows");
    const auto &group = content.skin_groups.at(skin.grids.front().skin);
    check(group.emblems.size() == 1, "saved-row skin lacks its original type9 child");
    const auto &emblem = group.emblems.front();
    check(emblem.flags == 1 && emblem.layer == 0 && emblem.animation == 11 &&
              emblem.delay == 250 && emblem.shader == 0xe4e302c3 &&
              emblem.bounds.top == 6 && emblem.bounds.left == -20 &&
              emblem.bounds.bottom == -26 && emblem.bounds.right == 12,
          "saved-row emblem authored flags/layer/bounds changed");
    std::set<std::string> row_bitmaps;
    for (const auto &b : skin.bitmaps) row_bitmaps.insert(content.images.at(b.datum).name);
    for (const auto &b : group.bitmaps) row_bitmaps.insert(content.images.at(b.datum).name);
    for (const unsigned selected : {1u, 2u, 0u, 3u, 1u}) {
      sample.select(selected);
      const auto draw = painter.draw(sample, 5);
      std::vector<std::size_t> quads;
      std::size_t first_row_bitmap = draw.vertices.size();
      for (std::size_t first = 0; first < draw.vertices.size(); first += 6) {
        const auto &v = draw.vertices[first];
        const auto &m = mesh.materials.at(v.material);
        if (m.ui_emblem_pass && draw.vertices[first + 4].position.x - v.position.x == 32 &&
            draw.vertices[first + 4].position.y - v.position.y == 32)
          quads.push_back(first);
        if (row_bitmaps.contains(m.name)) first_row_bitmap = std::min(first_row_bitmap, first);
      }
      check(quads.size() == 3,
            "saved profile rows omit original bound emblems or show one for CreateNew");
      std::set<unsigned> row_materials;
      const auto &placement = placements.at(screen);
      for (unsigned i = 0; i < names.size(); ++i) {
        const auto *quad = &draw.vertices[quads[i]];
        const bool focused = selected == i + 1;
        const float x = list.position.x + (focused ? 17.f : 0.f);
        const float y = list.position.y - float(i + 1) * placement[7];
        check(quad[0].position.x == 320 + x + placement[4] &&
                  quad[0].position.y == 240 - y - placement[3] &&
                  quad[4].position.x == 320 + x + placement[6] &&
                  quad[4].position.y == 240 - y - placement[5] &&
                  quad[0].uv.x == 0 && quad[0].uv.y == 0 &&
                  quad[4].uv.x == 1 && quad[4].uv.y == 1,
              "saved-row emblem differs from original2BACBC bounds/focus/raw UVs");
        for (unsigned v = 0; v < 6; ++v)
          check(quad[v].position.z == 1 && quad[v].color.x == 1 &&
                    quad[v].color.y == 1 && quad[v].color.z == 1,
                "saved-row emblem gained immediate tint instead of shader opacity");
        const float opacity = std::bit_cast<float>(opacities.at(
            {screen, focused ? "blue_focused" : "blue_unfocused"}));
        auto expected_material = content.emblem_shaders.at(emblem.shader);
        update_frontend_emblem(expected_material, expected[i], content.appearance_palette, opacity);
        const auto &actual = mesh.materials.at(quad[0].material);
        check(actual.texture_frames == expected_material.texture_frames &&
                  actual.forward_program.words == expected_material.forward_program.words &&
                  actual.object_render_colors && expected_material.object_render_colors &&
                  std::bit_cast<std::array<std::uint32_t, 12>>(*actual.object_render_colors) ==
                      std::bit_cast<std::array<std::uint32_t, 12>>(*expected_material.object_render_colors),
              "saved-row emblem reuses another row appearance/focus program bank");
        check(row_materials.insert(quad[0].material).second,
              "saved-row emblem shares a mutable material with another row instance");
        check(quads[i] < first_row_bitmap,
              "saved-row type9 emblem is above the layer0 type8 row bitmaps");
      }
      check(mesh.materials.size() == material_count,
            "saved-row emblem allocates GPU materials after painter construction");
      check(sample.profile_name() == committed && saved_bytes() == persisted,
            "saved-row emblem focus changed committed profile or preferences");
    }
  };
  verify(10);
  choose(sample, 1, 6);
  choose(sample, 4, 19);
  choose(sample, 0, 193);
  verify(193);
  sample.back();sample.back();
  check(sample.frame().screen==6,"Lobby emblem fixture did not return to main menu");
  check(sample.controller_slot(100,true)==0,"Lobby emblem fixture lost host controller");
  choose(sample,2,30);
  for(unsigned player=1;player<3;++player) {
    check(sample.controller_slot(100+player,true)==player&&sample.frame().screen==10,"Lobby emblem fixture did not open another profile picker");
    const auto &entries=sample.frame().entries;
    const auto entry=std::find_if(entries.begin(),entries.end(),[&](const auto &e){return e.label==names[player];});
    check(entry!=entries.end(),"Lobby emblem fixture lost a saved profile");
    choose(sample,unsigned(entry-entries.begin()),30);
  }
  choose(sample,0,14);
  const auto roster=sample.local_players();
  const auto lobby_draw=painter.draw(sample,5);
  std::set<unsigned> lobby_materials;
  unsigned row=0;
  for(unsigned first=0;first<lobby_draw.vertices.size();first+=6) {
    const auto material=lobby_draw.vertices[first].material;
    const auto &m=mesh.materials.at(material);
    if(!m.ui_emblem_pass)continue;
    check(row<roster.size()&&lobby_materials.insert(material).second,"Lobby rows share a mutable player emblem material");
    auto expected_material=content.emblem_shaders.at(m.source_shader_datum);
    update_frontend_emblem(expected_material,roster[row++].appearance,content.appearance_palette,1);
    check(m.texture_frames==expected_material.texture_frames&&m.forward_program.words==expected_material.forward_program.words,"Lobby row renders another player's emblem or colors");
  }
  check(row==3,"Lobby did not draw every signed-in player's emblem");
  std::filesystem::remove(preferences);
}
void profile_header_icon(const FrontendContent &content,
                         FrontendPainter &painter, const Mesh &mesh) {
  const auto fixture = std::filesystem::path(__FILE__).parent_path().parent_path() /
      "analysis/frontend/profile-row-appearance/profile-header-group-original.tsv";
  std::ifstream input(fixture);
  check(bool(input), "profile header regression lacks unchanged original group fixture");
  std::map<unsigned, std::array<int, 6>> placements;
  std::map<std::string, std::array<unsigned char, 16>> appearances;
  unsigned cases = 0, animations = 0;
  for (std::string line; std::getline(input, line);) {
    std::istringstream row(line);
    std::string kind;
    row >> kind;
    if (kind == "placement") {
      unsigned type;
      std::array<int, 6> value{};
      row >> type;
      for (auto &field : value) row >> field;
      check(bool(row), "incomplete original header placement fixture");
      placements.emplace(type, value);
    } else if (kind == "case") {
      std::string label;
      int focus, bitmap, emblem, name, sequence;
      row >> label >> focus >> bitmap >> emblem >> name >> sequence;
      check(bool(row) && bitmap == int(focus == -1) &&
                emblem == int(focus != -1) && name == int(focus != -1) && sequence == 0,
            "original header fixture no longer distinguishes CreateNew/saved children");
      ++cases;
    } else if (kind == "appearance") {
      std::string label, hex;
      row >> label >> hex;
      check(bool(row) && hex.size() == 32, "incomplete copied original header appearance");
      std::array<unsigned char, 16> value{};
      for (unsigned i = 0; i < value.size(); ++i)
        value[i] = static_cast<unsigned char>(std::stoul(hex.substr(i * 2, 2), nullptr, 16));
      appearances.emplace(label, value);
    } else if (kind == "animation") {
      ++animations;
    } else {
      check(false, "unknown original header fixture row");
    }
  }
  check(cases == 5 && animations == 33 && placements.size() == 3 &&
            placements.at(8) == std::array<int, 6>{163, -269, 35, -141, 0, 12} &&
            placements.at(9) == std::array<int, 6>{163, -269, 35, -141, -1, 11},
        "original header geometry, constructor fallback or animation coverage changed");
  const auto &pane = content.screens.at(193).panes.front();
  check(pane.grids.size() == 1 && pane.grids.front().skin == 0xe5ef03cf &&
            pane.grids.front().position.x == -269 && pane.grids.front().position.y == 35,
        "profile header group no longer matches authored193 metadata");
  const auto &group = content.skin_groups.at(pane.grids.front().skin);
  check(group.bitmaps.size() == 1 && group.emblems.size() == 1,
        "profile header lost its authored bitmap/emblem definitions");
  const auto &icon = group.bitmaps.front();
  const auto &emblem = group.emblems.front();
  const auto &image = content.images.at(icon.datum);
  check(icon.datum == 0xe5f003d0 && icon.frame == 2 && icon.placeholder == 0x0e00069b &&
            icon.animation == 13 && icon.delay == 300 && icon.flags == 0 &&
            image.images.size() == 1 && image.images.front().width == 128 &&
            image.images.front().height == 128 && image.sequences.empty() &&
            emblem.flags == 0 && emblem.animation == 11 && emblem.delay == 250 &&
            emblem.bounds.top == 0 && emblem.bounds.left == 0 &&
            emblem.bounds.bottom == -128 && emblem.bounds.right == 128,
        "profile header asset differs from original constructor/placement inputs");
  std::array<FrontendAppearance, 2> expected_appearance;
  for (unsigned i = 0; i < expected_appearance.size(); ++i) {
    const auto &source = appearances.at(i ? "tan" : "blue");
    std::array<unsigned char, sizeof(FrontendAppearance)> bytes{};
    std::copy_n(source.begin(), bytes.size(), bytes.begin());
    expected_appearance[i] = std::bit_cast<FrontendAppearance>(bytes);
  }
  const auto preferences = std::filesystem::temp_directory_path() /
      "halo2-frontend-header-icon-test.cfg";
  std::filesystem::remove(preferences);
  MenuState sample(content, preferences);
  constexpr std::array<const char *, 2> names{"Saved Blue", "Saved Tan"};
  for (unsigned i = 0; i < names.size(); ++i) {
    sample.accept();
    choose(sample, 0, 167);
    sample.text(names[i]);
    sample.commit_text();
    choose(sample, 1, 47);
    choose(sample, 1, 50);
    sample.select(unsigned(expected_appearance[i].colors[0]));
    sample.accept();
    choose(sample, 5, 49);
    sample.select(expected_appearance[i].foreground);
    if (expected_appearance[i].flags & 1) sample.auxiliary();
    sample.accept();
    choose(sample, 6, 233);
    sample.select(expected_appearance[i].background);
    sample.accept();
    choose(sample, 0, 52);
    sample.select(unsigned(expected_appearance[i].species));
    sample.accept();
    sample.back();
    sample.back();
    check(sample.frame().screen == 6 && sample.profile_appearance() == expected_appearance[i],
          "header icon fixture did not commit its source-controlled appearance");
    sample.back();
    choose(sample, 0, 9);
  }
  sample.accept();
  choose(sample, 1, 6);
  choose(sample, 4, 19);
  choose(sample, 0, 193);
  auto saved_bytes = [&] {
    std::ifstream file(preferences, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), {});
  };
  const auto persisted = saved_bytes();
  const auto &bounds = placements.at(8);
  const std::array<float, 4> expected_bounds{float(320 + bounds[1]),
      float(240 - bounds[0]), float(320 + bounds[3]), float(240 - bounds[2])};
  for (const unsigned selected : {0u, 1u, 2u, 0u, 1u}) {
    sample.select(selected);
    const auto draw = painter.draw(sample, 5);
    std::vector<const Vertex *> icons, emblems;
    for (std::size_t first = 0; first < draw.vertices.size(); first += 6) {
      const auto &vertex = draw.vertices[first];
      const auto &material = mesh.materials.at(vertex.material);
      if (material.name == image.name) icons.push_back(&vertex);
      if (material.ui_emblem_pass &&
          draw.vertices[first + 4].position.x - vertex.position.x == 128 &&
          draw.vertices[first + 4].position.y - vertex.position.y == 128)
        emblems.push_back(&vertex);
    }
    check(icons.size() == unsigned(!selected),
          "profile header omits original CreateNew question bitmap or retains it for saved focus");
    check(emblems.size() == unsigned(bool(selected)),
          "profile header omits original saved emblem or retains it for CreateNew");
    const auto *quad = selected ? emblems.front() : icons.front();
    check(quad[0].position.x == expected_bounds[0] && quad[0].position.y == expected_bounds[1] &&
              quad[4].position.x == expected_bounds[2] && quad[4].position.y == expected_bounds[3] &&
              quad[0].uv.x == 0 && quad[0].uv.y == 0 && quad[4].uv.x == 1 && quad[4].uv.y == 1,
          "profile header icon/emblem does not match original2BACBC bounds and raw UVs");
    for (unsigned i = 0; i < 6; ++i)
      check(quad[i].position.z == 1 && quad[i].color.x == 1 &&
                quad[i].color.y == 1 && quad[i].color.z == 1,
            "profile header acquires unauthored parent opacity or immediate tint");
    if (selected) {
      const auto &appearance = expected_appearance[selected - 1];
      const auto &actual = mesh.materials.at(quad[0].material);
      // The existing independent original14CCF0/22ACE7/program fixtures
      // verify this helper. This assertion verifies the real header binds
      // the focused appearance rather than the committed Blue bank.
      auto expected = content.emblem_shaders.at(emblem.shader);
      update_frontend_emblem(expected, appearance, content.appearance_palette, 1);
      check(actual.texture_frames[0] == appearance.background &&
                actual.texture_frames[1] == appearance.foreground &&
                actual.forward_program.words == expected.forward_program.words &&
                actual.object_render_colors && expected.object_render_colors &&
                std::bit_cast<std::array<std::uint32_t, 12>>(*actual.object_render_colors) ==
                    std::bit_cast<std::array<std::uint32_t, 12>>(*expected.object_render_colors),
            "profile header emblem does not use the original focused appearance/program bank");
    } else {
      const auto &actual = mesh.materials.at(quad[0].material).diffuse;
      check(actual.width == 128 && actual.height == 128 &&
                actual.pixels == image.images.front().pixels,
            "profile question bitmap ignores original constructor image0 fallback");
    }
    check(sample.profile_name() == "Saved Blue" && saved_bytes() == persisted,
          "profile header focus changed the committed profile or preferences");
  }
  std::filesystem::remove(preferences);
}
void profile_control_details(const FrontendContent &content,
                             FrontendPainter &painter, const Mesh &mesh,
                             const std::filesystem::path &maps) {
  const auto fixture = std::filesystem::path(__FILE__).parent_path().parent_path() /
      "analysis/frontend/profile-row-appearance/profile-controls-original.tsv";
  std::ifstream input(fixture);
  check(bool(input), "profile control regression lacks original executable fixture");
  std::map<unsigned, std::array<std::uint32_t, 4>> labels;
  std::map<std::array<int, 3>, std::array<std::uint32_t, 2>> selectors;
  unsigned counts = 0, rows = 0, writes = 0, initial = 0;
  for (std::string line; std::getline(input, line);) {
    if (line.starts_with('#')) continue;
    std::istringstream row(line);
    std::string kind;
    row >> kind;
    if (kind == "count") {
      unsigned screen, count;
      check(bool(row >> screen >> count) && (screen == 40 || screen == 41) && count == 4,
            "original control constructor record count changed");
      ++counts;
    } else if (kind == "row") {
      unsigned screen;
      int index;
      std::string id;
      check(bool(row >> screen >> index >> id), "incomplete original control row fixture");
      if (index >= 0 && index < 4)
        labels[screen][unsigned(index)] = std::uint32_t(std::stoul(id, nullptr, 16));
      else
        check(id == "ffffffff", "original control invalid row selector changed");
      ++rows;
    } else if (kind == "write") {
      unsigned screen, immediate_save, stored;
      int index;
      check(bool(row >> screen >> index >> immediate_save >> stored) &&
                stored == unsigned(index >= 0 && index < 4 ? index : 0),
            "original control acceptance no longer stores the literal layout enum");
      ++writes;
    } else if (kind == "initial") {
      unsigned screen, stored, selected;
      check(bool(row >> screen >> stored >> selected) && selected == (stored < 4 ? stored : 0),
            "original control initial-focus selector changed");
      ++initial;
    } else if (kind == "details") {
      std::array<int, 3> key;
      std::array<std::uint32_t, 2> strings;
      std::string id;
      check(bool(row >> key[0] >> key[1] >> key[2]),
            "incomplete original focused control fixture");
      for (auto &value : strings) {
        check(bool(row >> id), "missing original focused control string selector");
        value = std::uint32_t(std::stoul(id, nullptr, 16));
      }
      selectors.emplace(key, strings);
    } else {
      check(false, "unknown original profile control fixture row");
    }
  }
  check(counts == 2 && rows == 14 && writes == 28 && initial == 14 && selectors.size() == 108 &&
            labels.at(40) == std::array<std::uint32_t, 4>{0x07000001,0x090003e4,0x050003e5,0x0b0003e6} &&
            labels.at(41) == std::array<std::uint32_t, 4>{0x07000001,0x080002f5,0x060002f6,0x0f0002f7},
        "original profile control fixture coverage changed");
  const auto preferences = std::filesystem::temp_directory_path() /
      "halo2-frontend-profile-controls-test.cfg";
  std::filesystem::remove(preferences);
  MenuState sample(content, preferences);
  bool admitted_controls = true;
  constexpr std::array<const char *, 2> names{"Controls Blue", "Controls Tan"};
  for (unsigned profile = 0; profile < names.size(); ++profile) {
    sample.accept();
    choose(sample, 0, 167);
    sample.text(names[profile]);
    sample.commit_text();
    choose(sample, 0, 39);
    for (const unsigned screen : {41u, 40u}) {
      choose(sample, screen == 41 ? 0 : 1, screen);
      const auto &frame = sample.frame();
      admitted_controls &= frame.entries.size() == 4;
      for (unsigned i = 0; i < std::min(4u, unsigned(frame.entries.size())); ++i)
        admitted_controls &= frame.entries[i].value == int(i) &&
            frame.entries[i].label == content.localized(content.screens.at(screen), labels.at(screen)[i]);
      sample.select(profile + 2);
      sample.accept();
      check(sample.frame().screen == 39 && sample.profile_option(screen) == int(profile + 2),
            "real control route did not store accepted profile layout");
      choose(sample, screen == 41 ? 0 : 1, screen);
      check(sample.frame().selected == profile + 2,
            "control page did not reopen at its saved original enum");
      sample.back();
    }
    sample.back();
    sample.back();
    check(sample.frame().screen == 6, "profile control fixture missed main menu");
    sample.back();
    choose(sample, 0, 9);
  }
  sample.accept();
  choose(sample, 1, 6);
  choose(sample, 4, 19);
  choose(sample, 0, 193);
  const auto saved_bytes = [&] {
    std::ifstream file(preferences, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), {});
  };
  const auto persisted = saved_bytes();
  const auto font = load_retail_font(maps / "fonts" / "conduit-12");
  const auto &screen = content.screens.at(193);
  for (const unsigned selected : {1u, 2u, 0u, 1u}) {
    sample.select(selected);
    const auto draw = painter.draw(sample, 5);
    // Original24A886 reads focused record bytes104/105. The retained
    // two-record oracle sets record7 to2/2 and record8 to3/3; CreateNew
    // explicitly selects Default without reusing the previous saved record.
    const auto expected = selectors.at({selected ? int(selected) + 6 : -1, 2, 2});
    for (unsigned field = 0; field < 2; ++field) {
      const auto &definition = screen.panes.front().texts.at(field + 2);
      check(definition.font == 1 && (definition.flags & 1) &&
                definition.bounds.left == -127 && definition.bounds.right == 26 &&
                definition.bounds.top == (field ? 110 : 149) &&
                definition.bounds.bottom == (field ? 90 : 129),
            "focused control field authored geometry changed");
      const auto value = content.localized(screen, expected[field]);
      const float left = 193, right = 346, top = field ? 130 : 91, bottom = top + 20;
      // Authored dynamic fields have packed RGB172/195/216. The static
      // HighestLevel caption begins at149 and its next glyph can enter the
      // Buttons field's bounding rectangle; its source RGB98/147/193 is
      // a separate widget. Isolate the bound field without shifting bounds.
      constexpr Vec3 dynamic_color{172.f/255.f, 195.f/255.f, 216.f/255.f};
      std::vector<const Vertex *> actual;
      for (std::size_t i = 0; i + 5 < draw.vertices.size(); i += 6) {
        const auto &v = draw.vertices[i];
        if (mesh.materials.at(v.material).name == "font/conduit-12" &&
            v.position.x >= left && v.position.x < right &&
            v.position.y >= top && v.position.y < bottom &&
            v.color.x == dynamic_color.x && v.color.y == dynamic_color.y &&
            v.color.z == dynamic_color.z)
          actual.push_back(&v);
      }
      float x = left;
      unsigned previous = 0, glyph_index = 0;
      for (const unsigned c : value) {
        const auto &glyph = font.glyphs.at(c);
        const auto at = font.kerning.find({previous, c});
        const float kerning = at == font.kerning.end() ? 0.f : at->second;
        const float gx = x + glyph.bearing_x + kerning, gy = top + font.ascent - glyph.bearing_y;
        const float clipped_x = std::max(left, gx), clipped_y = std::max(top, gy);
        if (glyph.width && glyph.height && clipped_x < right && clipped_y < bottom) {
          const bool matches = glyph_index < actual.size() &&
                    actual[glyph_index]->position.x == clipped_x &&
                    actual[glyph_index]->position.y == clipped_y &&
                    actual[glyph_index]->uv.x == (glyph.x + clipped_x - gx) / font.atlas.width &&
                    actual[glyph_index]->uv.y == (glyph.y + clipped_y - gy) / font.atlas.height;
          if (!matches) {
            std::cerr << "profile control diagnostic: focus=" << selected << " field=" << field
                      << " value=" << value << " glyph=" << glyph_index << " char=" << c
                      << " expected=" << clipped_x << ',' << clipped_y << ','
                      << (glyph.x + clipped_x - gx) / font.atlas.width << ','
                      << (glyph.y + clipped_y - gy) / font.atlas.height << " actual_count=" << actual.size();
            if (glyph_index < actual.size())
              std::cerr << " actual=" << actual[glyph_index]->position.x << ','
                        << actual[glyph_index]->position.y << ',' << actual[glyph_index]->uv.x
                        << ',' << actual[glyph_index]->uv.y;
            std::cerr << '\n';
          }
          check(matches, "profile controls do not bind original focused layout labels/bounds");
          ++glyph_index;
        }
        x = glyph.advance + x + glyph.bearing_x + kerning;
        previous = c;
      }
      check(glyph_index == actual.size(), "focused control field retains another layout's glyphs");
    }
    check(sample.profile_name() == "Controls Blue" && saved_bytes() == persisted &&
              sample.profile_option(40) == 2 && sample.profile_option(41) == 2,
          "focused controls changed the committed profile or preferences");
  }
  check(admitted_controls, "native control menu admits Jumpy or shifts original four layout values");
  std::filesystem::remove(preferences);
}
void appearance_world_filter(const FrontendContent &content,
                             FrontendPainter &painter, const Mesh &mesh) {
  MenuState menu(content);
  const bool title_filter = painter.draw(menu, 5).scene_color_filter;
  menu.accept();
  const bool title_popup_filter = painter.draw(menu, 5).scene_color_filter;
  choose(menu, 0, 167);
  menu.text("World filter");
  menu.commit_text();
  choose(menu, 1, 47);
  const auto check_filter = [&](const UiDraw &draw) {
    // Actual47 source13870 ->13B52 selectsE4380218 at full strength;
    // actual16F32 binds the same60 programwords as title event8132.
    check(draw.scene_color_filter,
          "Appearance does not request the original scenario color filter");
    for (std::size_t i = 0; i + 5 < draw.vertices.size(); i += 6) {
      const auto &v = draw.vertices[i];
      check(!(v.position.x == 0 && v.position.y == 0 &&
                  v.position.z == content.overlay.alpha &&
                  v.color.x == content.overlay.red &&
                  v.color.y == content.overlay.green &&
                  v.color.z == content.overlay.blue &&
                  mesh.materials.at(v.material).name.empty()),
            "Appearance still adds the unsupported full-screen blue quad");
    }
  };
  check_filter(painter.draw(menu, 5));
  check(title_filter,
        "title no longer requests its verified scenario color filter");
  check(title_popup_filter,
        "title popup no longer retains its lower-channel scenario filter");
  const std::array<unsigned, 7> children{52, 50, 50, 50, 50, 49, 233};
  for (unsigned row = 0; row < children.size(); ++row) {
    menu.select(row);
    menu.accept();
    check(menu.frame().screen == children[row], "world filter child route");
    check_filter(painter.draw(menu, 5));
    menu.back();
  }
  // Original2234E0/13870 owns the normal mainmenu world context. The
  // scenario's constant interpolator admits its filter independently of UI
  // screen ID; fresh6/14/23/206 captures retain those gates and bound program.
  for (unsigned id : {6u, 14u, 19u, 23u, 30u, 49u, 50u, 52u, 206u, 233u}) {
    MenuState direct(content);
    direct.open(id);
    check_filter(painter.draw(direct, 5));
  }
}
void appearance_child_composition(const FrontendContent &content,
                                  FrontendPainter &painter, const Mesh &mesh) {
  MenuState menu(content);
  menu.accept();
  choose(menu, 0, 167);
  menu.text("Composition");
  menu.commit_text();
  choose(menu, 1, 47);
  const std::array<unsigned, 7> children{52, 50, 50, 50, 50, 49, 233};
  for (unsigned row = 0; row < children.size(); ++row) {
    menu.select(row);
    const auto parent = painter.draw(menu, 5);
    check(!parent.vertices.empty() && parent.scenes.size() == 1,
          "Appearance parent composition lacks its background or model");
    check(parent.scene_color_filter,
          "Appearance parent lost its verified scenario world filter");
    menu.accept();
    check(menu.frame().screen == children[row] &&
              content.screens.at(children[row]).flags == 0x25,
          "Appearance child no longer uses its original channel policy flags");
    const auto child = painter.draw(menu, 5);
    check(child.scene_color_filter == parent.scene_color_filter,
          "Appearance child changed its retained parent's world filter");
    // The original child flag5 -> v27 -> 23029A/234F8C adds no tint or
    // blur in captured main-menu mode2. It draws after the retained47
    // collector, including the original parent list beneath its panel.
    check(child.backdrops.empty(),
          "Appearance child introduced a source-forbidden tint/blur boundary");
    check(child.vertices.size() > parent.vertices.size(),
          "Appearance child replaced its retained parent composition");
    for (std::size_t i = 0; i < parent.vertices.size(); ++i) {
      const auto &before = parent.vertices[i];
      const auto &after = child.vertices[i];
      check(length(before.position-after.position) == 0 &&
                length(before.color-after.color) == 0 &&
                before.uv.x == after.uv.x && before.uv.y == after.uv.y &&
                before.material == after.material,
            "Appearance child changed the retained background or parent widgets");
    }
    for (std::size_t i = parent.vertices.size(); i < child.vertices.size(); i += 6) {
      const auto &v = child.vertices[i];
      check(!(v.position.x == 0 && v.position.y == 0 &&
                  v.position.z == .4f && v.color.x == 0 && v.color.y == 0 &&
                  v.color.z == 0 && mesh.materials.at(v.material).name.empty()),
            "Appearance child added a guessed black dim over its parent model");
    }
    check(child.scenes.size() == parent.scenes.size() &&
              child.scenes.front().definition == parent.scenes.front().definition &&
              child.scenes.front().before_vertex == parent.scenes.front().before_vertex &&
              length(child.scenes.front().animation_offset -
                     parent.scenes.front().animation_offset) == 0,
          "Appearance child replaced or repositioned the retained preview model");
    menu.back();
    check(menu.frame().screen == 47 && menu.frame().selected == row,
          "Appearance child cancel did not retain the parent selection");
  }
}
void appearance_ui_geometry(const FrontendContent &content,
                            FrontendPainter &painter, const Mesh &mesh,
                            const std::filesystem::path &maps) {
  MenuState menu(content);
  menu.accept();
  choose(menu, 0, 167);
  menu.text("Appearance UI");
  menu.commit_text();
  choose(menu, 1, 47);
  const auto &screen = content.screens.at(47);
  const auto draw = painter.draw(menu, 5);
  const auto title_font = load_retail_font(maps / "fonts/handel_gothic-11");
  std::vector<const Vertex *> title;
  for (std::size_t i = 0; i < draw.vertices.size(); i += 6) {
    const auto &v = draw.vertices[i];
    if (mesh.materials.at(v.material).name == "font/handel_gothic-11" &&
        v.position.y < 74 && (v.color.x > 0 || v.color.y > 0 || v.color.z > 0))
      title.push_back(&v);
  }
  const std::string upper = "APPEARANCE";
  check(title.size() == upper.size(), "Appearance header glyph run missing");
  for (unsigned i = 0; i < upper.size(); ++i) {
    const auto &g = title_font.glyphs.at(unsigned(upper[i]));
    check(title[i]->uv.x == float(g.x) / title_font.atlas.width &&
              title[i]->uv.y == float(g.y) / title_font.atlas.height,
          "retail screen header did not apply the original uppercase buffer flag");
  }
  const auto name_font = load_retail_font(maps / "fonts/conduit-13");
  const auto &name_glyph = name_font.glyphs.at('A');
  check(std::any_of(draw.vertices.begin(), draw.vertices.end(), [&](const auto &v) {
          return mesh.materials.at(v.material).name == "font/conduit-13" &&
              v.position.x == 215 + name_glyph.bearing_x &&
              v.position.y == 72 + name_font.ascent - name_glyph.bearing_y &&
              v.uv.x == float(name_glyph.x) / name_font.atlas.width &&
              v.uv.y == float(name_glyph.y) / name_font.atlas.height &&
              (v.color.x > 0 || v.color.y > 0 || v.color.z > 0);
        }),
        "Appearance omits the source player-name group or changes its authored placement");
  // Frozen 2C8BF5 supplies both name and value children. Preserve authored
  // color icons from the localized strings rather than drawing new swatches.
  const auto font = load_retail_font(maps / "fonts/conduit-12");
  const auto &list = screen.panes.front().lists.front();
  const auto &skin = content.skins.at(list.skin);
  const auto &value = skin.texts.at(1);
  auto has_value = [&](const UiDraw &image, unsigned row, unsigned codepoint) {
    const auto &g = font.glyphs.at(codepoint);
    // Original 2BAFA4 frozen list bounds [34,10,6,320] yield pitch28.
    const float top = 240 - list.position.y - value.bounds.top + row * 28;
    return std::any_of(image.vertices.begin(), image.vertices.end(),
        [&](const auto &v) {
          return mesh.materials.at(v.material).name == "font/conduit-12" &&
              v.position.x >= 320 + list.position.x + value.bounds.left &&
              v.position.x < 320 + list.position.x + value.bounds.right &&
              v.position.y >= top && v.position.y < top + 20 &&
              v.uv.x == float(g.x) / font.atlas.width &&
              v.uv.y == float(g.y) / font.atlas.height &&
              (v.color.x > 0 || v.color.y > 0 || v.color.z > 0);
        });
  };
  check(has_value(draw, 0, 'S') && has_value(draw, 1, 0xe019) &&
            has_value(draw, 2, 0xe010) && has_value(draw, 3, 0xe010) &&
            has_value(draw, 4, 0xe019),
        "Appearance rows omit the original model or authored color value glyphs");
  choose(menu, 0, 52);
  const auto child = painter.draw(menu, 5);
  auto popup_geometry = [&](const UiDraw &image) {
    const auto &popup = content.screens.at(menu.frame().screen);
    const auto &pane = popup.panes.front();
    const auto &caption = pane.texts.at(0);
    const auto label = content.localized(popup, caption.string);
    check(!label.empty() && unsigned(label.front()) < 128,
          "appearance popup title fixture lacks its authored ASCII glyph");
    const auto &title_glyph = title_font.glyphs.at(unsigned(label.front()));
    check(std::any_of(image.vertices.begin(), image.vertices.end(), [&](const auto &v) {
            return mesh.materials.at(v.material).name == "font/handel_gothic-11" &&
                v.position.x == 69 + title_glyph.bearing_x &&
                v.position.y == 218 + title_font.ascent - title_glyph.bearing_y &&
                v.position.z == 1 &&
                v.uv.x == float(title_glyph.x) / title_font.atlas.width &&
                v.uv.y == float(title_glyph.y) / title_font.atlas.height &&
                (v.color.x > 0 || v.color.y > 0 || v.color.z > 0);
          }), "appearance popup title incorrectly inherits the pane animation offset");
    // Original22E9C6 adds the text widget's animation54 offset(0,0),
    // independent of its parent pane56 offset(-72,-67). These projected
    // authored bounds are title(69,218), description(69,237), footer right364.
    if (pane.texts.size() > 2) {
      const auto &description = pane.texts.at(2);
      const auto value = content.localized(popup, description.string);
      const auto &glyph = font.glyphs.at(unsigned(value.front()));
      check(std::any_of(image.vertices.begin(), image.vertices.end(), [&](const auto &v) {
              return mesh.materials.at(v.material).name == "font/conduit-12" &&
                  v.position.x == 69 + glyph.bearing_x &&
                  v.position.y == 237 + font.ascent - glyph.bearing_y &&
                  v.position.z == 1 &&
                  v.uv.x == float(glyph.x) / font.atlas.width &&
                  v.uv.y == float(glyph.y) / font.atlas.height &&
                  (v.color.x > 0 || v.color.y > 0 || v.color.z > 0);
            }), "appearance popup description incorrectly inherits the pane animation offset");
    }
    const auto &last = font.glyphs.at('L');
    check(std::any_of(image.vertices.begin(), image.vertices.end(), [&](const auto &v) {
            return mesh.materials.at(v.material).name == "font/conduit-12" &&
                v.position.x == 364 - last.advance &&
                v.position.y == 390 + font.ascent - last.bearing_y &&
                v.position.z == 1 &&
                v.uv.x == float(last.x) / font.atlas.width &&
                v.uv.y == float(last.y) / font.atlas.height &&
                (v.color.x > 0 || v.color.y > 0 || v.color.z > 0);
          }), "appearance popup footer incorrectly inherits the pane animation offset");
    bool authored_corner = false;
    for (std::size_t first = 0; first < image.vertices.size(); first += 6) {
      const auto &v = image.vertices.at(first);
      if (mesh.materials.at(v.material).name == "ui\\global_bitmaps\\ul_07" &&
          v.position.x == 45 && v.position.y == 204 && v.position.z == 1)
        authored_corner = true;
    }
    check(authored_corner,
          "appearance popup bitmap incorrectly inherits the pane animation offset");
  };
  popup_geometry(child);
  std::vector<float> markers;
  const auto &child_skin = content.skins.at(
      content.screens.at(52).panes.front().lists.front().skin);
  const auto marker = content.images.at(child_skin.bitmaps.front().datum).name;
  for (std::size_t i = 0; i < child.vertices.size(); i += 6)
    if (mesh.materials.at(child.vertices[i].material).name == marker)
      markers.push_back(child.vertices[i].position.y);
  check(markers.size() >= 2 && markers.back() - markers[markers.size()-2] == 22,
        "Player Model child row pitch ignores the original bounds union");
  menu.select(1);
  menu.accept();
  const std::array<unsigned, 4> colors{8, 12, 9, 1};
  for (unsigned i = 0; i < colors.size(); ++i) {
    choose(menu, i + 1, 50);
    popup_geometry(painter.draw(menu, 5));
    check(menu.frame().context == i && menu.frame().entries.size() == 18,
          "Appearance color slot route differs from the original four handlers");
    menu.select(colors[i]);
    menu.accept();
    check(menu.frame().screen == 47, "color choice did not return to Appearance");
  }
  choose(menu, 5, 49);
  popup_geometry(painter.draw(menu, 5));
  check(menu.frame().entries.size() == 64, "original foreground mask omitted choices");
  menu.select(26);
  menu.accept();
  choose(menu, 6, 233);
  popup_geometry(painter.draw(menu, 5));
  check(menu.frame().entries.size() == 32, "original background mask omitted choices");
  menu.select(9);
  menu.accept();
  const auto edited = painter.draw(menu, 5);
  check(has_value(edited, 0, 'C') && has_value(edited, 1, 0xe018) &&
            has_value(edited, 2, 0xe021) && has_value(edited, 3, 0xe027) &&
            has_value(edited, 4, 0xe011) && has_value(edited, 5, 'T') &&
            has_value(edited, 6, 'Q'),
        "Appearance display values do not follow the edited source species/colors/emblems");
  // Actual 2AFEAE flags3 binding followed by unchanged 2B05C8 packing.
  // Values are frozen in appearance-namebar-original.json, all18 authored
  // palette entries; unlike text, bitmap channels round to nearest-even.
  constexpr std::array<std::uint32_t, 18> original_namebar{{
      0xfffdfeff,0xff535353,0xffbe2c2c,0xfff57a1f,0xfff5d22c,0xff9fac59,
      0xff21922f,0xff235644,0xff16a0a0,0xff36747a,0xff416c8f,0xff28459b,
      0xff6a4eb6,0xff75466d,0xfffb9bc9,0xff981244,0xff664e3e,0xffb19256}};
  const auto &name_group = content.skin_groups.at(screen.panes.front().grids.front().skin);
  const auto namebar_name = content.images.at(name_group.bitmaps.front().datum).name;
  for (unsigned i = 0; i < original_namebar.size(); ++i) {
    choose(menu, 1, 50);
    menu.select(i);
    menu.accept();
    const auto image = painter.draw(menu, 5);
    const auto pixel = original_namebar[i];
    check(std::any_of(image.vertices.begin(), image.vertices.end(), [&](const auto &v) {
            return mesh.materials.at(v.material).name == namebar_name &&
                v.position.x == 212 && v.position.y == 74 &&
                v.position.z == float(pixel >> 24) / 255.f &&
                v.color.x == float((pixel >> 16) & 255) / 255.f &&
                v.color.y == float((pixel >> 8) & 255) / 255.f &&
                v.color.z == float(pixel & 255) / 255.f;
          }),
          "Appearance namebar omits original raw-primary palette tint, placement or bitmap packing");
    if (i == 8) {
      // Authored animation10: delay250ms + half of its230ms intro.
      // Original bitmap packing rounds alpha.5 to128, not text's127.
      const auto halfway = painter.draw(menu, .365f);
      check(std::any_of(halfway.vertices.begin(), halfway.vertices.end(), [&](const auto &v) {
              return mesh.materials.at(v.material).name == namebar_name &&
                  v.position.z == 128.f / 255.f &&
                  v.color.x == 22.f / 255.f &&
                  v.color.y == 160.f / 255.f &&
                  v.color.z == 160.f / 255.f;
            }), "Namebar bitmap alpha uses text truncation instead of original nearest-even");
    }
  }
  choose(menu, 1, 50);
  menu.select(10);
  menu.move(-1);
  const auto scrolled = painter.draw(menu, 5);
  const auto &color_list = content.screens.at(50).panes.front().lists.front();
  const auto &color_text = content.skins.at(color_list.skin).texts.front();
  const auto &sage = font.glyphs.at(0xe017);
  const auto color_top = 240 - color_list.position.y - color_text.bounds.top;
  check(std::any_of(scrolled.vertices.begin(), scrolled.vertices.end(), [&](const auto &v) {
          return mesh.materials.at(v.material).name == "font/conduit-12" &&
              v.position.y >= color_top && v.position.y < color_top + 29 &&
              v.uv.x == float(sage.x) / font.atlas.width &&
              v.uv.y == float(sage.y) / font.atlas.height &&
              (v.color.x > 0 || v.color.y > 0 || v.color.z > 0);
        }),
        "Up within the color window scrolls every row instead of retaining original [7,8,9,10]");
  // Frozen unchanged 24C515/24C461/24C1C5, four authored cells and18
  // original color datums (appearance-window-original.json). Expected
  // windows come from the executable oracle, not the painter's formula.
  struct WindowCase { bool select; int argument; unsigned focus, first; };
  constexpr std::array<WindowCase, 18> original{{
      {true,10,10,7}, {false,-1,9,7}, {false,-1,8,7},
      {false,-1,7,7}, {false,-1,6,6}, {false,-1,5,5},
      {false,1,6,5}, {false,1,7,5}, {false,1,8,5},
      {false,1,9,6}, {false,1,10,7}, {false,1,11,8},
      {true,0,0,0}, {false,-1,17,14}, {false,1,0,0},
      {true,6,6,3}, {true,4,4,3}, {true,1,1,0}}};
  for (const auto &expected : original) {
    if (expected.select) menu.select(unsigned(expected.argument));
    else menu.move(expected.argument);
    check(menu.frame().selected == expected.focus &&
              menu.frame().first_visible == expected.first,
          "Appearance child focus/window differs from executed original selection and movement");
    check(menu.appearance().colors[0] == int(expected.focus),
          "Color window movement lost source preview callback");
  }
}
void subtitle_string_binding(const FrontendContent &content,
                             FrontendPainter &painter, const Mesh &mesh,
                             const std::filesystem::path &maps) {
  // Unchanged1496F6/1A0180/1A0350/1A02D0 executes the ID lookup in the
  // authored English range. Locale position10 is an unrelated single-space
  // string; enum11 selects180006D8 with the original double separator.
  check(content.button_keys.at(10) == "\xee\x84\x80 SELECT  \xee\x84\x81 DONE",
        "subtitle enum11 uses locale position instead of original string ID180006D8");
  check(content.button_keys.at(4).empty(),
        "subtitle enum5 substitutes another string for the missing original ID");
  MenuState menu(content);
  menu.accept();
  choose(menu, 0, 167);
  menu.text("Subtitle");
  menu.commit_text();
  choose(menu, 1, 47);
  const auto draw = painter.draw(menu, 5);
  const auto font = load_retail_font(maps / "fonts/conduit-12");
  // Original22FF53 setup then13F0E0/13F700 callbacks with the actual font.
  // Freeze the non-space glyph positions, including both controller icons.
  constexpr std::array<std::pair<unsigned, float>, 12> original{{
      {0xe100,472}, {'S',491}, {'E',499}, {'L',507}, {'E',514},
      {'C',522}, {'T',528}, {0xe101,540}, {'D',560}, {'O',568},
      {'N',576}, {'E',585}}};
  for (const auto &[codepoint, x] : original) {
    const auto &g = font.glyphs.at(codepoint);
    check(std::any_of(draw.vertices.begin(), draw.vertices.end(), [&](const auto &v) {
            return mesh.materials.at(v.material).name == "font/conduit-12" &&
                v.position.x == x && v.position.y == 427 &&
                v.uv.x == float(g.x) / font.atlas.width &&
                v.uv.y == float(g.y) / font.atlas.height &&
                (v.color.x > 0 || v.color.y > 0 || v.color.z > 0);
          }), "Appearance footer glyph placement differs from original subtitle traversal");
  }
}
void appearance_popup_layer_order(const FrontendContent &content,
                                  FrontendPainter &painter, const Mesh &mesh,
                                  const std::filesystem::path &maps) {
  MenuState menu(content);
  menu.accept();
  choose(menu, 0, 167);
  menu.text("Popup Layers");
  menu.commit_text();
  choose(menu, 1, 47);
  const auto title_font = load_retail_font(maps / "fonts/handel_gothic-11");
  const auto font = load_retail_font(maps / "fonts/conduit-12");
  for (const auto [row, screen] : std::array<std::pair<unsigned,unsigned>,4>{{
           {0,52},{1,50},{5,49},{6,233}}}) {
    choose(menu, row, screen);
    const auto draw = painter.draw(menu, 5);
    const auto &pane = content.screens.at(screen).panes.front();
    auto bitmap_quad = [&](const std::string &name) {
      for (std::size_t first = 0; first < draw.vertices.size(); first += 6)
        if (mesh.materials.at(draw.vertices[first].material).name == name)
          return first;
      return draw.vertices.size();
    };
    auto glyph_quad = [&](const RetailFont &f, const std::string &name,
                          unsigned codepoint, float x, float top) {
      const auto &glyph = f.glyphs.at(codepoint);
      for (std::size_t first = 0; first < draw.vertices.size(); first += 6) {
        const auto &v = draw.vertices[first];
        if (mesh.materials.at(v.material).name == name &&
            v.position.x == x + glyph.bearing_x &&
            v.position.y == top + f.ascent - glyph.bearing_y &&
            v.uv.x == float(glyph.x) / f.atlas.width &&
            v.uv.y == float(glyph.y) / f.atlas.height &&
            (v.color.x > 0 || v.color.y > 0 || v.color.z > 0))
          return first;
      }
      return draw.vertices.size();
    };
    const auto caption = content.localized(content.screens.at(screen),pane.texts[0].string);
    const auto title = glyph_quad(title_font,"font/handel_gothic-11",
                                  unsigned(caption.front()),69,218);
    const auto detail = bitmap_quad(content.images.at(pane.bitmaps[4].datum).name);
    const auto panel1 = bitmap_quad("ui\\global_bitmaps\\dialog1");
    const auto panel2 = bitmap_quad("ui\\global_bitmaps\\dialog2");
    const auto ul = bitmap_quad("ui\\global_bitmaps\\ul_07");
    const auto br = bitmap_quad("ui\\global_bitmaps\\br_07");
    const auto &last = font.glyphs.at('L');
    const auto footer = glyph_quad(font,"font/conduit-12",'L',
                                   364-last.advance-last.bearing_x,390);
    // Unchanged2356D8 with literal authored metadata: all steady depths0,
    // groups(0,type8),(0,type6),(1,type8),(2,type8),(5,type6).
    // The blue panel/glow must overlay caption/shadow, then footer overlays
    // the panel. Equal-group internal qsort tie order is not asserted.
    check(detail < title && title < panel1 && title < panel2 &&
              panel1 < ul && panel2 < br && ul < footer && br < footer &&
              footer < draw.vertices.size(),
          "appearance popup panel/glow is below text instead of original mixed widget layers");
    if (pane.texts.size() > 2) {
      const auto description = content.localized(content.screens.at(screen),pane.texts[2].string);
      const auto text = glyph_quad(font,"font/conduit-12",
                                   unsigned(description.front()),69,237);
      check(detail < text && text < panel1 && text < panel2,
            "appearance popup description shadow is above its original panel/glow layer");
    }
    menu.back();
    check(menu.frame().screen == 47,"popup layer fixture did not cancel to Appearance");
  }
}
void focused_profile_preview(const FrontendContent &content,
                             FrontendPainter &painter, const Mesh &mesh) {
  const auto fixture = std::filesystem::path(__FILE__).parent_path().parent_path() /
      "analysis/frontend/profile-row-appearance/profile-preview-placeholder-original.tsv";
  std::ifstream input(fixture);
  check(bool(input), "profile preview lacks original placeholder visibility fixture");
  std::map<std::string, std::array<int, 4>> original;
  std::array<int, 5> placement{};
  for (std::string line; std::getline(input, line);) {
    if (line.empty() || line.front() == '#') continue;
    std::istringstream row(line);
    std::string kind;
    row >> kind;
    if (kind == "case") {
      std::string label;
      std::array<int, 4> value{};
      row >> label;
      for (auto &field : value) row >> field;
      check(bool(row), "incomplete original profile placeholder case");
      original.emplace(label, value);
    } else if (kind == "placement") {
      int index;
      row >> index;
      for (auto &field : placement) row >> field;
      check(bool(row) && index == 6, "original profile placeholder ordinal changed");
    } else {
      check(false, "unknown original profile placeholder fixture row");
    }
  }
  check(original.size() == 7 && placement == std::array<int, 5>{162, 55, -350, 311, 0},
        "original profile placeholder admission/constructor coverage changed");
  const auto &pane = content.screens.at(193).panes.front();
  check(pane.bitmaps.size() == 7 && pane.bitmaps[6].datum == 0xe5ee03ce &&
            content.images.at(pane.bitmaps[6].datum).name ==
                "ui\\screens\\game_shell\\profile_picker\\unknown",
        "profile unknown preview lost original runtime bitmap6 mapping");
  const auto preferences = std::filesystem::temp_directory_path() /
                           "halo2-profile-preview-test.cfg";
  std::filesystem::remove(preferences);
  MenuState sample(content, preferences);
  // Create and customize both species through the actual title/profile flow.
  for (unsigned species = 0; species < 2; ++species) {
    check(sample.frame().screen == 9, "preview fixture missed the title");
    sample.accept();
    choose(sample, 0, 167);
    sample.text(species ? "Preview Elite" : "Active Spartan");
    sample.commit_text();
    check(sample.frame().screen == 38,
          "preview fixture skipped profile customization");
    choose(sample, 1, 47);
    choose(sample, 0, 52);
    sample.select(species);
    sample.accept();
    check(sample.frame().screen == 47 &&
              sample.profile_option(52) == int(species),
          "preview fixture did not persist the authored species selection");
    sample.back();
    sample.back();
    check(sample.frame().screen == 6, "preview fixture did not sign in");
    sample.back();
    choose(sample, 0, 9);
  }
  sample.accept();
  choose(sample, 1, 6); // Commit the first profile before opening Edit Profile.
  check(sample.profile_name() == "Active Spartan" &&
            sample.profile_option(52) == 0,
        "preview fixture did not commit the Spartan profile");
  choose(sample, 4, 19);
  choose(sample, 0, 193);
  check(sample.frame().entries.size() == 3,
        "Edit Profile preview fixture needs Create New and both species");
  auto saved_bytes = [&] {
    std::ifstream file(preferences, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), {});
  };
  const auto persisted = saved_bytes();
  auto check_placeholder = [&](const std::string &label) {
    const auto expected = original.at(label);
    const auto draw = painter.draw(sample, 5);
    std::vector<const Vertex *> quads;
    for (std::size_t first = 0; first < draw.vertices.size(); first += 6)
      if (mesh.materials.at(draw.vertices[first].material).name ==
          content.images.at(pane.bitmaps[6].datum).name)
        quads.push_back(&draw.vertices[first]);
    check(quads.size() == unsigned(expected[1]),
          "profile unknown preview bitmap remains visible for saved focus or is missing for CreateNew");
    if (expected[1]) {
      const auto *quad = quads.front();
      check(quad[0].position.x == 320 + placement[1] &&
                quad[0].position.y == 240 - placement[0] &&
                quad[1].position.x == 320 + placement[3] &&
                quad[2].position.y == 240 - placement[2],
            "CreateNew unknown preview bitmap changed its original constructor bounds");
    }
    // The original visibility write targets only direct bitmap6. All six
    // authored panel/decorative bitmaps remain admitted in the source cases.
    for (unsigned index = 0; index < 6; ++index)
      check(std::any_of(draw.vertices.begin(), draw.vertices.end(), [&](const auto &v) {
              return mesh.materials.at(v.material).name ==
                  content.images.at(pane.bitmaps[index].datum).name;
            }), "profile placeholder visibility hides unrelated authored panel art");
  };
  auto preview_names = [&] {
    std::vector<std::string> names;
    const auto draw = painter.draw(sample, 5);
    for (const auto &submission : draw.scenes)
      for (const auto &object : submission.definition->objects)
        if (object.starts_with("ui_player")) names.push_back(object);
    return names;
  };
  // Frozen unchanged 24A886 -> 24A855/24A828 -> 24A7EC -> 2B10A3:
  // model type 0/1 follows the focused appearance byte; datum index NONE
  // (Create New) explicitly clears both model widgets, including cached Elite.
  sample.select(0);
  check_placeholder("create_new_initial");
  sample.select(1);
  check_placeholder("focused_spartan");
  check(preview_names() == std::vector<std::string>{"ui_player1"},
        "focused Spartan profile did not submit the original type-0 preview");
  sample.select(2);
  check_placeholder("focused_elite");
  check(preview_names() == std::vector<std::string>{"ui_player1b"},
        "Edit Profile preview follows the committed species instead of focus");
  sample.select(0);
  check_placeholder("create_new_after_elite");
  check(preview_names().empty(),
        "Create New Profile leaves a player model visible on Edit Profile");
  sample.select(1);
  check_placeholder("focused_spartan_again");
  check(preview_names() == std::vector<std::string>{"ui_player1"},
        "saved-profile preview did not return after Create New focus");
  check(sample.profile_name() == "Active Spartan" &&
            sample.profile_option(52) == 0 && saved_bytes() == persisted,
        "profile hover preview changed the committed profile or preferences");
  // Accepting retains the existing commit behavior; focus alone is read-only.
  sample.select(2);
  sample.accept();
  check(sample.frame().screen == 194 &&
            sample.profile_name() == "Preview Elite" &&
            sample.profile_option(52) == 1,
        "focused-profile preview changed Edit Profile's accept behavior");
  std::filesystem::remove(preferences);
}
void retail(const std::filesystem::path &maps) {
  Cache cache(maps / "mainmenu.map");
  const auto sky = cache.load_visible_sky(0);
  check(sky && sky->name == "scenarios\\skies\\solo\\earthcity\\main_menu" &&
        sky->scale == 40 && sky->movement == .9f,
        "menu omitted the authored visible sky/parallax transform");
  check(sky->atmospheric_fog && sky->atmospheric_fog->maximum_density == .5f &&
        sky->atmospheric_fog->start == 0 && sky->atmospheric_fog->opaque == 150 &&
        std::abs(sky->atmospheric_fog->color.x-.360784f)<1e-6f,
        "title beam is missing the authored atmospheric-fog inputs");
  const auto atmosphere=cache.load_menu_atmosphere();
  check(atmosphere.gradient.width==16 && atmosphere.gradient.height==16 &&
        atmosphere.gradient.format==11 && atmosphere.gradient.pixels.size()==1024,
        "title fog composite is missing the original rasterizer gradient");
  check(atmosphere.planar.plane==ShaderVector{0,0,1,221.8275604248047f} &&
        atmosphere.planar.maximum_density==.8f && atmosphere.planar.opaque_distance==6 &&
        atmosphere.planar.opaque_depth==6 && atmosphere.planar.eye_offset==0,
        "title beam is missing the original visible planar-fog layer");
  const auto above=menu_fog_constants(atmosphere,{0,0,240},{1,0,0});
  const auto below=menu_fog_constants(atmosphere,{0,0,210},{1,0,0});
  check(above[3][2]==.5f && above[3][3]==.8f && below[3][3]==0 && below[5][0]==.8f,
        "menu fog fails the original submerged secondary-fog transition");
  auto sky_model = cache.load_model(sky->model);
  prepare_frontend_materials(cache, sky_model.mesh);
  check(!sky_model.mesh.vertices.empty() &&
        std::all_of(sky_model.mesh.vertices.begin(), sky_model.mesh.vertices.end(),
          [&](const auto &v) { return v.material < sky_model.mesh.materials.size() &&
              sky_model.mesh.materials[v.material].forward_pass; }),
        "menu sky lost its authored shader passes");
  auto beam_fixture = cache.load_bsp(0);
  prepare_frontend_materials(cache, beam_fixture);
  const auto beam = std::find_if(beam_fixture.materials.begin(), beam_fixture.materials.end(),
      [](const auto &material) { return material.name.ends_with("\\light_long_volume_noanim"); });
  check(beam != beam_fixture.materials.end(), "retail BSP light-volume shader missing");
  const auto &beam_material = *beam;
  check(beam_material.diffuse.layers == 6,
        "BSP light volume is missing the authored vector-normalization cubemap");
  check(beam_material.vertex_program.size() == 676 &&
            beam_material.forward_vertex == 1 && beam_material.forward_blend == 3,
        "BSP light volume lost its original vertex program or additive blend");
  check(beam_material.forward_raster.color_write == 0x10101 &&
        !beam_material.forward_raster.depth_write &&
        beam_material.forward_raster.cull == 0x900 &&
        beam_material.forward_raster.alpha_test == 1 &&
        beam_material.forward_raster.alpha_function == 0x204,
        "beam ignores the original RGB-only writes, culling or alpha test");
  VertexProgramState beam_inputs;
  beam_inputs.constants[0] = {1,0,0,0};
  beam_inputs.constants[1] = {0,1,0,0};
  beam_inputs.constants[2] = {0,0,1,0};
  beam_inputs.constants[3] = {0,0,0,1};
  beam_inputs.constants[4] = {0,0,0,1};
  beam_inputs.constants[6] = {0,0,1,2};
  beam_inputs.constants[7] = {0,0,1,767.8125f};
  beam_inputs.constants[10] = {1,1,1,0};
  beam_inputs.constants[172] = {1,1,0,0};
  for (unsigned i=0;i<16;++i) {
    beam_inputs.constants[18+i] = beam_material.program_constants[1][i];
    beam_inputs.attributes[i] = beam_material.program_constants[0][i];
  }
  beam_inputs.attributes[0] = {0,0,0,1};
  beam_inputs.attributes[1] = {.25f,.5f,0,1};
  beam_inputs.attributes[2] = {0,0,1,1};
  execute_vertex_program(decode_vertex_program(beam_material.vertex_program), beam_inputs);
  check(std::abs(beam_inputs.outputs[3][3]-.15f)<1e-6f &&
            std::abs(beam_inputs.outputs[9][2]-1)<1e-6f &&
            std::abs(beam_inputs.outputs[10][0]-.25f)<1e-6f &&
            std::abs(beam_inputs.outputs[11][1]-.5f)<1e-6f,
        "beam facing opacity, reflection vector or authored UVs differ");
  const auto content = cache.load_frontend();
  const auto dialog1 = std::find_if(content.images.begin(), content.images.end(),
      [](const auto &entry) {
        return entry.second.name == "ui\\global_bitmaps\\dialog1";
      });
  check(dialog1 != content.images.end() && !dialog1->second.images.empty() &&
            dialog1->second.images.front().format == 11 &&
            dialog1->second.images.front().pixels ==
                ui_bc3_fixture("frontend_ui_dialog1_original.rgba"),
        "frontend content bypassed original UI BC3 expansion");
  check(content.music_silence_ms == 5500, "retail pre-attract silence changed");
  check(content.scene_clear &&
        std::abs(content.scene_clear->x - .137255f)<1e-6f &&
        std::abs(content.scene_clear->y - .137255f)<1e-6f,
        "menu uses the viewer clear color instead of the authored sky");
  check(content.screens.size() == 84 && content.parameters.size() == 92 &&
            content.variants.size() == 39,
        "retail frontend inventory");
  check(content.localized(content.screens.at(6), 0x0800010b) == "CAMPAIGN",
        "screen-local string resolution");
  check(content.localized(content.screens.at(6), 0x0800010b) !=
            content.localized(content.screens.at(218), 0x0800010b),
        "string lists were merged by ID");
  check(content.button_keys.at(0).find("BACK") != std::string::npos,
        "button-key table start");
  const auto font = load_retail_font(maps / "fonts/handel_gothic-11");
  check(font.glyphs.at('A').width > 0 && font.glyphs.at(0xe100).width > 0,
        "retail letters/controller glyphs");
  check(font.atlas.width == 1024 && font.atlas.height < 4096,
        "bounded retail font atlas");
  const auto music = cache.load_looping_sound(content.music);
  check(music.tracks.size() == 1, "retail menu music track");
  for (auto id : {music.tracks[0].start, music.tracks[0].loop}) {
    const auto encoded = cache.load_sound(id);
    check(encoded.codec == 4 && encoded.channels == 2,
          "retail codec stream classified as ADPCM");
    const auto pcm = decode_sound(encoded);
    check(pcm.frames() > 48000 &&
              std::any_of(pcm.stereo.begin(), pcm.stereo.end(),
                          [](float v) { return std::abs(v) > .01f; }),
          "silent/truncated menu WMA");
  }
  for (auto id : content.sounds)
    if (id != null_datum)
      check(decode_sound(cache.load_sound(id)).frames() > 0, "missing UI sound");
  const auto preferences =
      std::filesystem::temp_directory_path() / "halo2-frontend-test.cfg";
  std::filesystem::remove(preferences);
  MenuState menu(content, preferences);
  Mesh ui_mesh;
  FrontendPainter profile_painter(content, maps / "fonts", ui_mesh);
  widescreen_background_materials(content, profile_painter, ui_mesh, maps / "fonts");
  keyboard_default_names(content);
  profile_pointer_geometry(content, profile_painter, ui_mesh);
  profile_saved_row_appearance(content, profile_painter, ui_mesh, maps);
  profile_row_emblems(content, profile_painter, ui_mesh);
  profile_header_icon(content, profile_painter, ui_mesh);
  profile_control_details(content, profile_painter, ui_mesh, maps);
  focused_profile_preview(content, profile_painter, ui_mesh);
  appearance_world_filter(content, profile_painter, ui_mesh);
  appearance_child_composition(content, profile_painter, ui_mesh);
  appearance_ui_geometry(content, profile_painter, ui_mesh, maps);
  subtitle_string_binding(content, profile_painter, ui_mesh, maps);
  appearance_popup_layer_order(content, profile_painter, ui_mesh, maps);
  auto has_title_bitmap = [&](const UiDraw &draw) {
    const auto &title_image = content.images.at(
        content.screens.at(9).panes.front().bitmaps.front().datum).name;
    const auto foreground = draw.vertices.begin() +
        (draw.backdrops.empty() ? 0 : draw.backdrops.back().before_vertex);
    return std::any_of(foreground, draw.vertices.end(),
        [&](const auto &vertex) {
          return ui_mesh.materials.at(vertex.material).name == title_image;
        });
  };
  check(has_title_bitmap(profile_painter.draw(menu, 5)), "title bitmap repro has no title");
  menu.accept();
  check(menu.frame().screen == 10, "profile popup repro did not press Start");
  const auto profile_draw = profile_painter.draw(menu, 5);
  check(profile_draw.backdrops.size() == 1 &&
            profile_draw.backdrops.front().before_vertex > 0,
        "Choose Profile omits the original lower-channel title snapshot blur");
  check(std::any_of(profile_draw.vertices.begin(), profile_draw.vertices.end(),
      [&](const auto &v) { return ui_mesh.materials.at(v.material).name ==
          "ui\\list_skins\\profile_picker\\item_background"; }),
      "Create New Profile omits its authored nested highlight stripe");
  check(!has_title_bitmap(profile_painter.draw(menu, 5)),
        "title logo is still drawn behind Choose Profile after Start");
  menu.back();
  check(has_title_bitmap(profile_painter.draw(menu, 5)), "profile Back did not restore title");
  menu.back();
  check(menu.frame().screen == 9 && !menu.quit,
        "B quit the retail title screen");
  menu.accept();
  check(menu.frame().screen == 10, "title Start");
  check(menu.frame().entries.size() == 1 &&
            menu.frame().entries[0].destination == 167,
        "new installation invents a saved profile");
  choose(menu, 0, 167);
  keyboard_geometry(content, menu, profile_painter, ui_mesh, maps);
  keyboard_input_overlay(content, menu, profile_painter, ui_mesh);
  keyboard_description_wrap(content, menu, profile_painter, ui_mesh, maps);
  menu.text("Player");
  menu.commit_text();
  check(menu.frame().screen == 38, "new profile skipped retail customization");
  menu.back();
  check(menu.frame().screen == 6,
        "new profile did not sign in after customization");
  menu.back();
  check(menu.frame().screen == content.dialogs.at(0x30).screen &&
            menu.frame().entries.size() == 2 &&
            !menu.frame().description.empty(),
        "main menu skipped sign-out confirmation");
  menu.select(1);
  menu.accept();
  check(menu.frame().screen == 6, "cancel sign-out returned to wrong screen");
  choose(menu, 4, 19);
  choose(menu, 0, 193);
  choose(menu, 0, 167);
  check(menu.frame().entries.size() == 47 && menu.frame().selected == 40,
        "retail keyboard focus/keys");
  menu.select(10);
  menu.accept();
  check(menu.keyboard_text == "a", "A on a keyboard key committed the name");
  menu.select(41);
  menu.accept();
  menu.select(11);
  menu.accept();
  check(menu.keyboard_text == "aB", "keyboard shift");
  menu.select(36);
  menu.accept();
  menu.text("é");
  menu.cursor(-1);
  menu.text("Z");
  menu.cursor(1);
  menu.erase_text();
  check(menu.keyboard_text == "aZ", "UTF-8 cursor and erase");
  menu.commit_text();
  check(menu.frame().screen == 38 && menu.profile_name() == "aZ",
        "local profile creation");
  menu.back();
  check(menu.frame().screen == 193 &&
            menu.frame().entries[0].destination == 167,
        "profile list create row is not first");
  choose(menu, menu.frame().selected, 194);
  choose(menu, 0, 38);
  choose(menu, 0, 39);
  choose(menu, 2, 42);
  menu.select(7);
  menu.accept();
  menu.open(42);
  check(menu.frame().selected == 7, "saved sensitivity selection");
  menu.back();
  menu.open(47);
  for (unsigned i = 0; i < 7; ++i) {
    const auto target = menu.frame().entries.at(i).destination;
    choose(menu, i, unsigned(target));
    check(!menu.frame().entries.empty(), "appearance choices missing");
    menu.back();
  }
  menu.back();
  menu.open(38);
  menu.auxiliary();
  check(menu.frame().screen == 167, "rename keyboard");
  while (!menu.keyboard_text.empty())
    menu.erase_text();
  menu.text("Tester");
  menu.commit_text();
  check(menu.profile_name() == "Tester", "profile rename");
  MenuState loaded(content, preferences);
  check(loaded.profile_name() == "Tester", "active profile persistence");
  MenuState signed_profile(content);
  signed_profile.accept();
  choose(signed_profile, 0, 167);
  signed_profile.text("Before");
  signed_profile.commit_text();
  signed_profile.back();
  check(signed_profile.frame().screen == 6, "profile fixture failed to sign in");
  signed_profile.open(38);
  signed_profile.auxiliary();
  while (!signed_profile.keyboard_text.empty()) signed_profile.erase_text();
  signed_profile.text("After");
  signed_profile.commit_text();
  signed_profile.open(194);
  choose(signed_profile, 2, content.dialogs.at(0x57).screen);
  check(signed_profile.frame().description == content.dialogs.at(0x57).message,
        "renaming the signed-in profile removed its deletion guard");
  loaded.open(42);
  check(loaded.frame().selected == 7, "profile option persistence");
  loaded.open(19);
  choose(loaded, 0, 193);
  choose(loaded, loaded.frame().selected, 194);
  choose(loaded, 1, 167);
  while (!loaded.keyboard_text.empty())
    loaded.erase_text();
  loaded.text("Copied Profile");
  loaded.commit_text();
  check(loaded.frame().screen == 193 && loaded.profile_name() == "Copied Profile",
        "profile Copy performed a rename");
  loaded.open(42);
  check(loaded.frame().selected == 7, "profile copy discarded options");
  loaded.back();
  choose(loaded, loaded.frame().selected, 194);
  choose(loaded, 2, content.dialogs.at(0x70).screen);
  check(loaded.frame().selected == 1, "profile delete defaults to confirmation");
  choose(loaded, 0, 193);
  check(loaded.profile_name() == "Player", "profile deletion did not update list");
  menu.open(6);
  choose(menu, 3, 30);
  choose(menu, 0, 14);
  choose(menu, 1, 23);
  choose(menu, 1, 207); // Original2BB4D2: Change Map, Change Rules, Quick Options.
  std::set<unsigned> engines;
  for (unsigned engine = 0; engine < 7; ++engine) {
    choose(menu, engine, 208);
    check(!menu.frame().entries.empty(), "built-in variant engine empty");
    menu.auxiliary();
    check(menu.frame().screen == 178, "all variant editing menu");
    for (unsigned category = 0; category < 6; ++category) {
      choose(menu, category, 221);
      for (unsigned param = 0; param < menu.frame().entries.size(); ++param) {
        choose(menu, param, 223);
        check(!menu.frame().entries.empty() &&
                  !menu.frame().description.empty(),
              "parameter choices/description missing");
        menu.select(unsigned(menu.frame().entries.size() - 1));
        menu.accept();
        check(menu.frame().screen == 221, "parameter accept");
      }
      menu.back();
    }
    menu.back();
    menu.back();
  }
  for (const auto &[id, p] : content.parameters) {
    MenuState sample(content);
    sample.open(223, id);
    check(sample.frame().entries.size() == p.choices.size(),
          "variant parameter omitted");
    sample.select(unsigned(p.choices.size() - 1));
    sample.accept();
    sample.open(223, id);
    check(sample.frame().selected == p.choices.size() - 1,
          "variant value retention");
  }
  MenuState lan(content);
  auto sign_in_local = [](MenuState &state) {
    state.accept(); state.accept(); state.text("Launch Test");
    state.commit_text(); state.back();
  };
  sign_in_local(lan);choose(lan,3,30);choose(lan,0,14);
  lan.select(0);lan.accept();
  check(lan.launch && lan.launch->multiplayer && !lan.launch->campaign &&
        lan.launch->host_port==2002 && lan.launch->local_players==1 && lan.launch->variant.engine==0,
        "System Link Start Game must launch a real multiplayer match");
  MenuState split(content);sign_in_local(split);choose(split,2,30);choose(split,0,14);
  split.select(0);split.accept();
  check(split.launch && split.launch->multiplayer && split.launch->local_players==1 && !split.launch->host_port,
        "Selecting one local profile must not create a second local player");
  MenuState king(content);sign_in_local(king);choose(king,3,30);choose(king,0,14);
  choose(king,1,23);choose(king,1,207);choose(king,1,208);
  choose(king,0,14);king.select(0);king.accept();
  check(king.launch && king.launch->variant.engine==3 && king.launch->host_port==2002,
        "System Link King must reach its original multiplayer engine");
  MenuState campaign(content);
  campaign.open(6);
  choose(campaign, 0, 218);
  choose(campaign, 2, 11);
  choose(campaign, 2, 12);
  campaign.select(3);
  campaign.accept();
  check(campaign.launch && campaign.launch->map == "01b_spacestation.map" &&
            campaign.launch->campaign && campaign.launch->difficulty == 3,
        "campaign map handoff");
  campaign_launch(content);
  MenuState variants(content, preferences);
  variants.open(19);
  choose(variants, 1, 60);
  choose(variants, 0, 15);
  choose(variants, 0, 194);
  choose(variants, 1, 167);
  while (!variants.keyboard_text.empty())
    variants.erase_text();
  variants.text("Custom Slayer");
  variants.commit_text();
  MenuState saved(content, preferences);
  saved.open(19);
  choose(saved, 1, 60);
  choose(saved, 0, 15);
  check(saved.frame().entries.back().label == "Custom Slayer",
        "named variant persistence");
  choose(saved, unsigned(saved.frame().entries.size() - 1), 194);
  choose(saved, 2, content.dialogs.at(0xb).screen);
  check(saved.frame().selected == 1, "variant deletion defaults to confirmation");
  choose(saved, 0, 60);
  check(saved.profile_name() == "Tester",
        "variant deletion removed a player profile");
  choose(saved, 0, 15);
  choose(saved, 0, 194);
  choose(saved, 2, 194);
  check(saved.profile_name() == "Tester",
        "built-in variant deletion removed profile");
  Mesh mesh;
  FrontendPainter painter(content, maps / "fonts", mesh);
  std::size_t frames = 0;
  for (const auto &[id, screen] : content.screens) {
    MenuState sample(content);
    if (id != 9)
      sample.open(id);
    for (unsigned pane = 0; pane < screen.panes.size(); ++pane) {
      const auto draw = painter.draw(sample, 5, int(pane));
      check(draw.vertices.size() % 3 == 0 && !draw.vertices.empty(),
            "authored pane empty");
      for (const auto &v : draw.vertices)
        check(v.material < mesh.materials.size() &&
                  std::isfinite(v.position.x) && std::isfinite(v.position.y),
              "invalid pane geometry");
      ++frames;
    }
  }
  check(frames == 150, "authored pane coverage");
  for (const auto &[id, p] : content.parameters) {
    MenuState sample(content);
    sample.open(223, id);
    check(!painter.draw(sample, 5).vertices.empty(),
          "parameter rendering omitted");
  }
  const auto level = cache.load_content(0);
  Presentation presentation(level);
  Camera player;
  presentation.control(true, player);
  presentation.camera_point(6, 0);
  const auto camera = presentation.camera(player);
  check(camera.oriented && std::abs(camera.roll) > 0 &&
            std::abs(camera.up.z) < 1,
        "retail rolled menu camera ignored");
  const auto &point = level.camera_points[6];
  const auto basis = authored_basis(point.rotation);
  check(length(camera.forward - basis.forward) < .000001f &&
            length(camera.up - basis.up) < .000001f,
        "scenario camera uses free-look Euler order");
  const auto turn = authored_basis({1.570796327f, 0, 1.570796327f});
  check(length(turn.forward - Vec3{0, 0, 1}) < .000001f &&
            length(turn.up - Vec3{0, -1, 0}) < .000001f,
        "retail matrix axis order");
  // camera_set uses the observer's quintic curve and retains derivatives
  // when a script issues another target before the prior command finishes.
  LevelContent flight;
  flight.camera_points = {{}, {}};
  flight.camera_points[1].position = {10, 0, 0};
  Presentation flyby(flight);
  flyby.control(true, {});
  flyby.camera_point(0, 0);
  flyby.camera_point(1, 100);
  for (unsigned i = 0; i < 30; ++i)
    flyby.tick();
  // Unchanged retail routines: .0001 command flush, pre-update director
  // decrement, and thirty observer updates (duration100, x0 -> x10).
  check(std::abs(flyby.camera({}).position.x - 1.671346664f) < .002f,
        "flyby uses linear interpolation instead of retail observer");
  const float before_sampling = flyby.camera({}).position.x;
  const float midpoint = flyby.camera({}, .5f).position.x;
  check(midpoint > before_sampling &&
            flyby.camera({}).position.x == before_sampling,
        "render sampling modified the authoritative camera");
  const float previous = flyby.camera({}).position.x;
  flyby.camera_point(0, 100);
  flyby.tick();
  check(flyby.camera({}).position.x > previous,
        "overlapping camera command discarded velocity");
  for (unsigned i = 1; i < 100; ++i)
    flyby.tick();
  check(length(flyby.camera({}).position) < .000001f,
        "observer did not reach authored target");
  std::filesystem::remove(preferences);
  std::cout << "Retail menu navigation, 150 panes and all 92 parameter dialogs "
               "passed\n";
}
void campaign_launch(const FrontendContent &content) {
  auto choose = [](MenuState &state, unsigned row, unsigned screen) {
    state.select(row);
    state.accept();
    check(state.frame().screen == screen, "campaign menu route changed");
  };
  for (unsigned difficulty = 0; difficulty < 4; ++difficulty) {
    MenuState state(content);
    state.open(6);
    choose(state, 0, 218);
    choose(state, 2, 11);
    choose(state, 14, 12);
    state.select(3);
    state.accept();
    check(state.launch && state.launch->map == "08b_deltacontrol.map" &&
              state.launch->campaign && state.launch->difficulty == 3,
          "Select Level lost its selected mission or difficulty");
    state.back(); // Return from Select Level's difficulty picker.
    state.back();
    check(state.frame().screen == 218, "campaign selector did not return");
    choose(state, 0, 12);
    check(!state.launch, "New Game retained an earlier launch request");
    state.back();
    check(state.frame().screen == 218 && !state.launch,
          "cancelling New Game submitted a launch");
    choose(state, 0, 12);
    state.select(difficulty);
    state.accept();
    check(state.launch && state.launch->map == "00a_introduction.map" &&
              state.launch->campaign && !state.launch->multiplayer &&
              state.launch->difficulty == difficulty &&
              state.launch->local_players == 1 && !state.launch->online_match,
          "New Game did not request a fresh Heretic campaign at chosen difficulty");
  }
  MenuState resume(content);
  resume.open(218);
  choose(resume, 1, 8);
  check(!resume.launch && resume.frame().title == "UNAVAILABLE" &&
            resume.frame().description.find("save and resume") != std::string::npos,
        "unimplemented Resume unexpectedly submitted a campaign launch");
  std::cout << "Campaign New Game: four difficulties, retained Select Level, "
               "cancel and unavailable Resume passed\n";
}
void movies(const std::filesystem::path &maps) {
  const std::array<std::pair<const char *, unsigned>, 3> files{
      {{"intro", 556}, {"attract", 3588}, {"credits", 10375}}};
  for (auto [name, count] : files) {
    const auto path =
        maps.parent_path() / "bink" / (std::string(name) + "_60.bik");
    Movie movie(path);
    check(movie.frame_count() == count &&
              std::abs(movie.frame_rate() - 29.97) < .000001,
          "movie frame rate/count differs from retail Bink header");
    check(movie.image().width == 640 && movie.image().height == 480,
          "movie canvas differs from original");
    movie.advance(movie.duration());
    check(movie.frame_number() == count && movie.finished(movie.duration()),
          "movie truncated or timing lost");
    const auto audio = decode_movie_audio(path);
    check(std::abs(double(audio.frames()) / audio.rate - movie.duration()) <
                  .2 &&
              std::any_of(audio.stereo.begin(), audio.stereo.end(),
                          [](float v) { return std::abs(v) > .01f; }),
          "movie audio silent or out of duration");
    rejects([&] { movie.advance(-1); });
    std::cout << "Retail " << name << ": " << count << " video frames, "
              << audio.frames() << " audio frames\n";
  }
}
} // namespace
int main(int argc, char **argv) {
  try {
    font_decoder();
    ui_bc3_decoder();
    background_canvas_policy();
    frame_edge_dds_parser();
    attract_timing();
    music_debounce();
    if (argc > 2 && std::string(argv[2]) == "--campaign-only") {
      Cache cache(std::filesystem::path(argv[1]) / "mainmenu.map");
      campaign_launch(cache.load_frontend());
    } else if (argc > 2 && std::string(argv[2]) == "--movies")
      movies(argv[1]);
    else if (argc > 1)
      retail(argv[1]);
    std::cout << "Frontend tests passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
