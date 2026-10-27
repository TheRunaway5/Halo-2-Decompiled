#include "halo2/full_scoreboard_painter.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
static void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
int main(int argc,char **argv)try {
 need(argc==2,"Supply original scoreboard kerning controls");nlohmann::json source;std::ifstream in(argv[1]);in>>source;
 need(source.at("source_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original kerning source pin changed");
 RetailFont font;font.atlas.width=32;font.atlas.height=8;font.ascent=10;
 font.glyphs[u'A']={0,0,8,8,10,0,8};font.glyphs[u'V']={8,0,8,8,10,0,8};
 for(const auto &pair:source.at("font_stimulus").at("pairs"))font.kerning[{pair[0].get<unsigned>(),pair[1].get<unsigned>()}]=pair[2].get<int>();
 FullScoreboardPainter painter(font,1,2);FullScoreboardPaintConstants constants;
 constants.bright_rgb_threshold=std::bit_cast<std::uint32_t>(.68f);constants.pixel_scale=std::bit_cast<std::uint32_t>(255.f);
 FullScoreboardPaintFrame frame;frame.clip={0,0,100,100};frame.x87_control_word=0x27f;
 unsigned compared{};
 for(const auto &control:source.at("cases")) {
  FullScoreboardLayout layout;layout.status=FullScoreboardStatus::Ready;auto &widget=layout.title;widget.valid=true;
  widget.rectangle=widget.text_rectangle={0,0,100,100};widget.foreground={1,1,1,1};
  widget.right_justified=control.at("alignment")==2;
  for(const char c:control.at("text").get<std::string>())widget.text.push_back(char16_t(c));
  const auto out=painter.draw(layout,constants,frame);need(out.status==FullScoreboardPaintStatus::Ready,"Original text control did not draw");
  std::vector<const Vertex*> glyphs;for(std::size_t i=0;i<out.draw.vertices.size();i+=6)if(out.draw.vertices[i].material==1)glyphs.push_back(&out.draw.vertices[i]);
  need(glyphs.size()==control.at("quads").size(),"Scoreboard kerning changed glyph admission");
  for(unsigned i=0;i<glyphs.size();++i){const auto &actual=*glyphs[i];const auto &expected=control.at("quads")[i];
   if(actual.position.x!=expected.at("x").get<float>()||actual.position.y!=expected.at("y").get<float>()) {
    std::cerr<<"text="<<control.at("text")<<" alignment="<<control.at("alignment")<<" glyph="<<i<<" actual="<<actual.position.x<<","<<actual.position.y<<" original="<<expected.at("x")<<","<<expected.at("y")<<'\n';
    throw std::runtime_error("Actual scoreboard glyph origin differs from unchanged original iterator/kerning calls");
   }
   ++compared;
  }
 }
 need(compared==26,"Original asymmetric kerning controls were lost");
 std::cout<<"PASS "<<compared<<" actual original scoreboard glyph origins: asymmetric previous/current pairs, initial two-character iterator state, left and centered measurement/draw; declared font metadata only, runtime MCC artwork unchanged\n";
 return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
