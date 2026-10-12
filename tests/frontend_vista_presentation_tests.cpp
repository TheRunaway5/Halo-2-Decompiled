#include "halo2/frontend.hpp"
#include "halo2/frontend_canvas.hpp"
#include "halo2/frontend_frame_sampling.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void require(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
void label_prefix(const UiDraw &draw, const Mesh &mesh, const RetailFont &font,
                  const UiText &label, std::u16string_view expected) {
  std::vector<std::size_t> quads;
  const float left=320+label.bounds.left*.4f, right=320+label.bounds.right*.4f;
  const float top=240-label.bounds.top*.4f, bottom=240-label.bounds.bottom*.4f;
  for (std::size_t i=0; i<draw.vertices.size(); i+=6) {
    const auto &v=draw.vertices[i];
    if (mesh.materials.at(v.material).name=="font/conduit-12" &&
        v.position.x>=left && draw.vertices[i+4].position.x<=right &&
        v.position.y>=top && draw.vertices[i+4].position.y<=bottom)
      quads.push_back(i);
  }
  require(quads.size()==expected.size(), "Vista annotation has wrong revealed glyph count");
  for (std::size_t j=0; j<expected.size(); ++j) {
    const auto &g=font.glyphs.at(expected[j]);
    const auto &a=draw.vertices[quads[j]], &b=draw.vertices[quads[j]+4];
    const float u=(a.uv.x+b.uv.x)*.5f*font.atlas.width;
    const float v=(a.uv.y+b.uv.y)*.5f*font.atlas.height;
    require(u>=g.x && u<=g.x+g.width && v>=g.y && v<=g.y+g.height,
            "Vista annotation uses incorrect glyphs");
    require(b.position.x-a.position.x<=g.width*.12f+.0001f &&
            b.position.y-a.position.y<=g.height*.12f+.0001f,
            "Vista annotation lost original small font scale");
  }
}
}
int main(int argc, char **argv) {
  if (argc != 3 || !std::filesystem::exists(std::filesystem::path(argv[1]) / "mainmenu.map") ||
      !std::filesystem::exists(argv[2])) return 77;
  try {
    const std::filesystem::path maps = argv[1];
    Cache cache(maps / "mainmenu.map");
    auto content = cache.load_frontend();
    content.vista = load_vista_settings(argv[2]);
    Mesh mesh;
    FrontendPainter painter(content, maps / "fonts", mesh);
    const auto &pc=*content.vista;
    const auto font=load_retail_font(pc.source_fonts/"conduit-12");
    const auto &label=pc.screens.at(19).panes.at(0).texts.at(1);
    require(pc.typewriter.interval_ms==50 && pc.typewriter.cursor==0xdb,
            "Vista source typing settings changed");
    require(pc.button_keys.size()==25 && !pc.button_keys[0].empty() &&
            pc.button_keys[12].find("CANCEL")!=std::string::npos,
            "Vista footer prompt table is missing");
    MenuState state(content);
    state.open(19);
    label_prefix(painter.draw(state,5,-1,100),mesh,font,label,u"\u00db");
    painter.draw(state,5,-1,150);
    const auto first=pc.localized(pc.screens.at(19),label.string).front();
    const std::u16string prefix{char16_t(first),u'\u00db'};
    label_prefix(painter.draw(state,5,-1,150),mesh,font,label,prefix);
    label_prefix(painter.draw(state,5,-1,10000),mesh,font,label,prefix);
    state.ui_clock(10000);
    state.select(1); state.select(0);
    label_prefix(painter.draw(state,5,-1,10000),mesh,font,label,u"\u00db");
    unsigned annotations{};
    for (unsigned i=0;i<6;++i) {
      state.select(i);
      for (const auto &t:pc.screens.at(19).panes.at(i).texts)
        if (t.flags&8) ++annotations;
      for (unsigned step=0;step<150;++step)
        painter.draw(state,5,-1,10000+step*50);
    }
    require(annotations==21,"Vista icon annotation inventory changed");
    state.select(0);
    const auto hub=painter.draw(state,5,-1,20000);
    unsigned centers{},extensions{},footer_glyphs{};
    for (std::size_t i=0;i<hub.vertices.size();i+=6) {
      const auto &a=hub.vertices[i], &b=hub.vertices[i+4];
      const auto &material=mesh.materials.at(a.material);
      if (material.ui_frame_center) ++centers;
      if (material.ui_frame_extension!=UiFrameExtension::None) {
        ++extensions;
        require(std::abs(a.position.x)<.001f && std::abs(b.position.x-640)<.001f,
                "Vista frame side copies do not use the final native canvas");
      }
      if (material.name.starts_with("font/") && a.position.y>430) {
        ++footer_glyphs;
        require(a.position.x>=0 && b.position.x<=640 && b.position.y<=480,
                "Vista footer prompt escapes the authored canvas");
      }
    }
    require(centers==1 && extensions==2,"Vista frame is missing widescreen side extensions");
    require(footer_glyphs>0,"Vista footer prompt was not drawn");
    for (const auto size:std::array<std::array<unsigned,2>,6>{{
        {640,480},{1280,800},{1280,720},{1720,720},{2560,720},{1001,601}}}) {
      const auto canvas=frontend_canvas(size[0],size[1]);
      require(canvas.left>=0 && canvas.top>=0 && canvas.left+canvas.width<=size[0]+.001f,
              "Centered Vista foreground leaves its drawable");
    }
    state.select(5);
    state.accept();
    require(state.frame().screen == vista_menu(297), "About route did not reproduce the reported popup");
    const auto draw = painter.draw(state, 5.f, -1, 5000);
    require(!draw.backdrops.empty(), "Vista About popup leaves sharp Settings widgets underneath its text");
    require(draw.backdrops.back().fade==1 && draw.backdrops.back().before_vertex>0,
            "Vista About does not compose its backdrop after lower widgets");
    state.back();
    state.select(0);
    state.mouse_wheel(-99);
    require(state.frame().selected==1,"wheel magnitude incorrectly skips Vista options");
    state.mouse_wheel(1);
    state.mouse_wheel(1);
    require(state.frame().selected==5,"Vista wheel up does not share source list wrap");
    state.mouse_wheel(-1);
    require(state.frame().selected==0,"Vista wheel down does not wrap to first option");
    state.open(vista_menu(253));
    for (unsigned i=0;i<100;++i) {
      state.mouse_wheel(-1);
      require(state.frame().entries[state.frame().selected].value!=-2,
              "wheel focuses a controls section heading");
      painter.draw(state,5,-1,20000);
    }
    state.select(1);state.accept();
    require(state.capturing_binding(),"wheel binding guard test did not enter capture");
    const auto selection=state.frame().selected;
    state.mouse_wheel(-1);
    require(state.frame().selected==selection && state.capturing_binding(),
            "wheel navigation consumes a binding capture");
    std::cout << "Vista popup, frame, typing, footer prompts and wheel navigation passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
