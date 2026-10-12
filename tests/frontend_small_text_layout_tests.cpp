#include "halo2/frontend.hpp"
#include "halo2/frontend_typewriter.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void require(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
std::uint32_t hex(const std::string &value) {
  require(value.size()==8, "invalid original float/color word");
  return std::uint32_t(std::stoul(value,nullptr,16));
}
struct Glyph {
  unsigned codepoint{};
  std::array<std::uint32_t,4> xy;
  std::array<int,4> uv;
  std::uint32_t color{};
};
struct Label {
  std::uint32_t string{}, scale{};
  std::array<int,4> bounds;
  int font{};
  unsigned count{};
  std::vector<Glyph> glyphs;
};
std::map<unsigned,Label> fixture(const std::filesystem::path &path) {
  std::ifstream file(path);
  require(bool(file), "missing original small-font layout fixture");
  std::map<unsigned,Label> result;
  for (std::string line; std::getline(file,line);) {
    if (line.empty() || line.front()=='#') continue;
    std::istringstream input(line);
    char type{};
    unsigned index{};
    input>>type>>index;
    if (type=='C') {
      Label label;
      std::string id, scale;
      input>>id>>label.count;
      for (auto &bound:label.bounds) input>>bound;
      input>>label.font>>scale;
      label.string=hex(id); label.scale=hex(scale);
      require(result.emplace(index,std::move(label)).second,
              "duplicate original small-font label");
    } else if (type=='G') {
      Glyph glyph;
      unsigned ordinal{};
      input>>ordinal>>glyph.codepoint;
      for (auto &xy:glyph.xy) { std::string value; input>>value; xy=hex(value); }
      for (auto &uv:glyph.uv) input>>uv;
      std::string color; input>>color; glyph.color=hex(color);
      auto &label=result.at(index);
      require(ordinal==label.glyphs.size(), "unordered original small-font glyph");
      label.glyphs.push_back(glyph);
    } else throw std::runtime_error("unknown original small-font fixture row");
    require(!input.fail(), "truncated original small-font fixture row");
    std::string extra;
    require(!(input>>extra), "trailing original small-font fixture data");
  }
  unsigned glyphs{};
  for (const auto &[index,label]:result) {
    require(index>=1 && index<=7 && label.font==1 && label.scale==0x3e99999a &&
            label.count==label.glyphs.size(), "original small-font fixture coverage changed");
    glyphs+=label.count;
  }
  require(result.size()==7 && glyphs==440, "original seven-label/440-glyph fixture missing");
  return result;
}
std::uint32_t packed(const Vertex &vertex) {
  // Source colors are byte channels represented exactly by byte/255.f in
  // the host vertex. Compare the expected channels directly below.
  return (std::uint32_t(vertex.position.z*255.f+.5f)<<24) |
         (std::uint32_t(vertex.color.x*255.f+.5f)<<16) |
         (std::uint32_t(vertex.color.y*255.f+.5f)<<8) |
          std::uint32_t(vertex.color.z*255.f+.5f);
}
void compare(const UiDraw &draw,const Mesh &mesh,const RetailFont &font,
             const Label &label,unsigned index) {
  std::vector<std::size_t> quads;
  for (std::size_t i=0; i<draw.vertices.size(); i+=6) {
    const auto &a=draw.vertices[i],&c=draw.vertices[i+4];
    if (mesh.materials.at(a.material).name=="font/conduit-12" &&
        packed(a)==label.glyphs.front().color &&
        a.position.x>=label.bounds[1] && c.position.x<=label.bounds[3] &&
        a.position.y>=label.bounds[0] && c.position.y<=label.bounds[2])
      quads.push_back(i);
  }
  if (quads.size()!=label.count)
    throw std::runtime_error("Settings label "+std::to_string(index)+
        " original glyph count "+std::to_string(label.count)+
        " differs from painter "+std::to_string(quads.size()));
  // Native triangles a,b,d,b,c,d correspond to original callback a,b,c,d.
  constexpr std::array<unsigned,6> corners{0,1,3,1,2,3};
  for (std::size_t j=0; j<quads.size(); ++j) {
    const auto &expected=label.glyphs[j];
    const auto &glyph=font.glyphs.at(expected.codepoint);
    const std::array<float,4> x{
        std::bit_cast<float>(expected.xy[0]),std::bit_cast<float>(expected.xy[2]),
        std::bit_cast<float>(expected.xy[2]),std::bit_cast<float>(expected.xy[0])};
    const std::array<float,4> y{
        std::bit_cast<float>(expected.xy[1]),std::bit_cast<float>(expected.xy[1]),
        std::bit_cast<float>(expected.xy[3]),std::bit_cast<float>(expected.xy[3])};
    const std::array<float,4> u{
        float(int(glyph.x)+expected.uv[0])/font.atlas.width,
        float(int(glyph.x)+expected.uv[2])/font.atlas.width,
        float(int(glyph.x)+expected.uv[2])/font.atlas.width,
        float(int(glyph.x)+expected.uv[0])/font.atlas.width};
    const std::array<float,4> v{
        float(int(glyph.y)+expected.uv[1])/font.atlas.height,
        float(int(glyph.y)+expected.uv[1])/font.atlas.height,
        float(int(glyph.y)+expected.uv[3])/font.atlas.height,
        float(int(glyph.y)+expected.uv[3])/font.atlas.height};
    for (unsigned k=0; k<6; ++k) {
      const auto &actual=draw.vertices.at(quads[j]+k);
      const auto corner=corners[k];
      const std::array<float,8> wanted{x[corner],y[corner],u[corner],v[corner],
          float((expected.color>>24)&255)/255.f,
          float((expected.color>>16)&255)/255.f,
          float((expected.color>>8)&255)/255.f,float(expected.color&255)/255.f};
      const std::array<float,8> got{actual.position.x,actual.position.y,
          actual.uv.x,actual.uv.y,actual.position.z,
          actual.color.x,actual.color.y,actual.color.z};
      constexpr std::array<const char *,8> field{"x","y","u","v","a","r","g","b"};
      for (unsigned f=0; f<wanted.size(); ++f)
        if (std::bit_cast<std::uint32_t>(wanted[f])!=std::bit_cast<std::uint32_t>(got[f])) {
          std::ostringstream error;
          error<<"Settings label "<<index<<" glyph "<<j<<" codepoint "
               <<expected.codepoint<<" corner "<<corner<<' '<<field[f]
               <<" original word "<<std::hex<<std::bit_cast<std::uint32_t>(wanted[f])
               <<" differs from painter "<<std::bit_cast<std::uint32_t>(got[f]);
          throw std::runtime_error(error.str());
        }
    }
  }
}
}
int main(int argc,char **argv) {
  if (argc!=3 || !std::filesystem::exists(std::filesystem::path(argv[1])/"mainmenu.map") ||
      !std::filesystem::exists(argv[2])) return 77;
  try {
    const std::filesystem::path maps=argv[1];
    const Cache cache(maps/"mainmenu.map");
    const auto content=cache.load_frontend();
    const auto expected=fixture(argv[2]);
    const auto font=load_retail_font(maps/"fonts/conduit-12");
    const auto &screen=content.screens.at(19);
    const auto &labels=screen.panes.at(0).texts;
    require(labels.size()==8, "actual Settings label inventory changed");
    std::map<unsigned,std::u16string> values;
    std::map<unsigned,FrontendTypewriterState> typing;
    for (const auto &[index,label]:expected) {
      const auto &source=labels.at(index);
      const std::array<float,4> projected{
          240-source.bounds.top,320+source.bounds.left,
          240-source.bounds.bottom,320+source.bounds.right};
      bool same_bounds=true;
      for (unsigned i=0; i<projected.size(); ++i)
        same_bounds=same_bounds && projected[i]==label.bounds[i];
      require(source.string==label.string && source.flags==9 && source.font==9 &&
              source.delay==0 && same_bounds,
              "actual Settings source metadata differs from original layout input");
      const auto value=content.localized(screen,source.string);
      require(value.size()==label.count, "original Settings full text differs from loaded string");
      for (unsigned char c:value) {
        require(c<128, "small-font fixture requires its actual ASCII labels");
        values[index].push_back(c);
      }
      frontend_typewriter_set_text(typing[index],values.at(index));
      frontend_typewriter_begin(typing[index]);
    }
    Mesh mesh;
    FrontendPainter painter(content,maps/"fonts",mesh);
    MenuState state(content);
    state.open(19);
    UiDraw draw;
    // Repeated actual draws are necessary: the original advances once per
    // eligible draw, and returns the old prefix before advancing its cursor.
    bool complete=false;
    for (std::uint32_t clock=100; clock<=10000 && !complete; clock+=50) {
      complete=true;
      for (const auto &[index,value]:values) {
        const auto displayed=frontend_typewriter_draw(typing[index],value,clock,
            content.typewriter.interval_ms,content.typewriter.cursor);
        complete=complete && displayed==value && typing[index].cursor==-1;
      }
      draw=painter.draw(state,5,-1,clock);
    }
    require(complete, "actual Settings widgets did not reach full original reveal");
    for (const auto &[index,label]:expected) compare(draw,mesh,font,label,index);
    std::cout<<"Original seven fully revealed Settings labels: 440 glyphs, exact positions, source clip UVs and packed colors\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr<<e.what()<<'\n'; return 1;
  }
}
