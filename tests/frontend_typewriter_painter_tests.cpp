#include "halo2/frontend.hpp"
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
  const float left=320+label.bounds.left, right=320+label.bounds.right;
  const float top=240-label.bounds.top, bottom=240-label.bounds.bottom;
  for (std::size_t i=0; i<draw.vertices.size(); i+=6) {
    const auto &v=draw.vertices[i];
    if (mesh.materials.at(v.material).name=="font/conduit-12" &&
        v.position.x>=left && draw.vertices[i+4].position.x<=right &&
        v.position.y>=top && draw.vertices[i+4].position.y<=bottom)
      quads.push_back(i);
  }
  if (quads.size()!=expected.size())
    throw std::runtime_error("actual Settings prefix has wrong glyph count for string " +
        std::to_string(label.string) + ": expected " + std::to_string(expected.size()) +
        ", got " + std::to_string(quads.size()));
  for (std::size_t j=0; j<expected.size(); ++j) {
    const auto &g=font.glyphs.at(expected[j]);
    const auto i=quads[j];
    const auto &a=draw.vertices[i], &b=draw.vertices[i+4];
    const float u=(a.uv.x+b.uv.x)*.5f*font.atlas.width;
    const float v=(a.uv.y+b.uv.y)*.5f*font.atlas.height;
    require(u>=g.x && u<=g.x+g.width && v>=g.y && v<=g.y+g.height,
            "Settings prefix uses the wrong original glyph atlas rectangle");
    require(b.position.x-a.position.x<=g.width*.3f+.0001f &&
            b.position.y-a.position.y<=g.height*.3f+.0001f,
            "Settings label uses enlarged authored Conduit16 geometry");
  }
}
}
int main(int argc, char **argv) {
  if (argc!=2 || !std::filesystem::exists(std::filesystem::path(argv[1])/"mainmenu.map"))
    return 77;
  try {
    const std::filesystem::path maps=argv[1];
    Cache cache(maps/"mainmenu.map");
    const auto content=cache.load_frontend();
    const auto font=load_retail_font(maps/"fonts/conduit-12");
    const auto &label=content.screens.at(19).panes.at(0).texts.at(1);
    Mesh mesh;
    FrontendPainter painter(content,maps/"fonts",mesh);
    MenuState state(content);
    state.open(19);
    // Actual source tracks have alpha0 and X1024 at intro start. Reveal
    // admission precedes both alpha and offscreen rejection in2538F5/22CD48.
    painter.draw(state,0,-1,100);
    painter.draw(state,0,-1,150);
    label_prefix(painter.draw(state,5,-1,150),mesh,font,label,u"H\u00db");
    // A long frame advances only once, and the old prefix is displayed first.
    label_prefix(painter.draw(state,5,-1,10000),mesh,font,label,u"H\u00db");
    label_prefix(painter.draw(state,5,-1,10000),mesh,font,label,u"Ha\u00db");
    // Two focus changes before another draw still recreate the actual pane.
    state.ui_clock(10000);
    state.select(1);
    state.select(0);
    label_prefix(painter.draw(state,5,-1,10000),mesh,font,label,u"\u00db");
    // A popup preserves its underlying Settings widget allocation.
    painter.draw(state,5,-1,10050);
    state.open(8);
    painter.draw(state,5,-1,10050);
    state.back();
    label_prefix(painter.draw(state,5,-1,10050),mesh,font,label,u"H\u00db");
    // A new screen allocation restarts typing even with the same definition.
    state.back();
    state.open(19);
    label_prefix(painter.draw(state,5,-1,10050),mesh,font,label,u"\u00db");

    // Screen210 pane2 has three actual250ms-delayed labels. Its online route
    // now requires login; give the unchanged definition an offline owner in
    // this isolated copy. This probes delay admission, not service routing.
    auto delayed=content;
    delayed.screens.at(19)=content.screens.at(210);
    delayed.screens.at(19).strings.insert(content.screens.at(19).strings.begin(),
                                         content.screens.at(19).strings.end());
    auto &texts=delayed.screens.at(19).panes.at(2).texts;
    require(texts.size()>1 && texts[1].flags==9 && texts[1].delay==250,
            "actual delayed label inventory changed");
    for (auto &t:texts) if (t.flags&8) t.animation=0;
    Mesh delayed_mesh;
    FrontendPainter delayed_painter(delayed,maps/"fonts",delayed_mesh);
    MenuState delayed_state(delayed);
    delayed_state.ui_clock(1000);
    delayed_state.open(19);
    // This service-populated screen has no native matchmaking feed. Supply
    // its actual pane2 through the existing diagnostic pane override.
    const auto &delayed_label=texts[1];
    label_prefix(delayed_painter.draw(delayed_state,5,2,1249),delayed_mesh,font,delayed_label,u"");
    label_prefix(delayed_painter.draw(delayed_state,5,2,1250),delayed_mesh,font,delayed_label,u"\u00db");
    label_prefix(delayed_painter.draw(delayed_state,5,2,1300),delayed_mesh,font,delayed_label,u"\u00db");
    const auto value=delayed.localized(delayed.screens.at(19),delayed_label.string);
    require(!value.empty() && static_cast<unsigned char>(value[0])<128,
            "delayed prefix probe requires actual ASCII first character");
    std::u16string prefix{static_cast<char16_t>(value[0]),u'\u00db'};
    label_prefix(delayed_painter.draw(delayed_state,5,2,1300),delayed_mesh,font,delayed_label,prefix);
    std::cout<<"Actual Settings typing, hidden intro, retained/recreated widgets and250ms delay passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr<<e.what()<<'\n'; return 1;
  }
}
