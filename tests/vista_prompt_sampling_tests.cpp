#include "halo2/frontend.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char *message) {if(!value)throw FormatError(message);}
bool prompt(unsigned c) {
  return (c>=0xe100 && c<=0xe111) || (c>=0xe12c && c<=0xe13c);
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)throw FormatError("Vista font directory and active MCC UI pack required");
    const std::filesystem::path pack=argv[2];
    const auto vista=std::string_view(argv[1])=="auto"?hud_menu_asset_directory(pack)/"fonts":
        std::filesystem::path(argv[1]);
    if(!std::filesystem::exists(vista/"font_table.txt") ||
        !std::filesystem::exists(pack/"prompt-source.tsv"))return 77;
    std::ifstream table(vista/"font_table.txt");std::vector<std::string> names;
    for(std::string name;std::getline(table,name);) {
      if(!name.empty() && name.back()=='\r')name.pop_back();names.push_back(name);
    }
    require(names.size()==12,"Incomplete Vista selector table");
    std::size_t count{},unchanged{},pixels{};
    for(unsigned selector=0;selector<names.size();++selector) {
      auto lower=names[selector];for(auto &c:lower)if(c>='A' && c<='Z')c+='a'-'A';
      const auto source=load_retail_font(vista/lower);
      auto sampled=source;
      apply_mcc_classic_font_sampling(sampled,load_retail_font(ui_sampled_font_path(pack,selector,lower)));
      const auto previous=sampled;
      const auto changed=apply_vista_prompt_sampling(sampled,pack,selector);
      require(changed>0,"Vista prompt overlay not selected");count+=changed;
      require(sampled.ascent==previous.ascent && sampled.descent==previous.descent &&
          sampled.leading==previous.leading && sampled.kerning==previous.kerning &&
          sampled.glyph_record_indices==previous.glyph_record_indices,
          "Prompt overlay changed font lookup or layout metrics");
      for(const auto &[c,a]:previous.glyphs) {
        const auto &b=sampled.glyphs.at(c);
        require(a.x==b.x && a.y==b.y && a.width==b.width && a.height==b.height &&
            a.advance==b.advance && a.bearing_x==b.bearing_x && a.bearing_y==b.bearing_y,
            "Prompt overlay changed logical glyph geometry");
      }
      for(const auto &[c,a]:previous.sampled_glyphs) {
        const auto &b=sampled.sampled_glyphs.at(c);
        if(prompt(c)) {
          const auto &original=source.glyphs.at(c);
          if(b.x>=previous.atlas.width)require(b.width==4*original.width && b.height==4*original.height,
              "Prompt raster is not fourfold its actual Vista source extent");
        } else {
          require(a.x==b.x && a.y==b.y && a.width==b.width && a.height==b.height,
              "Prompt overlay changed an ordinary MCC glyph sample");++unchanged;
        }
      }
      for(unsigned y=0;y<previous.atlas.height;++y) {
        const auto a=previous.atlas.pixels.begin()+std::size_t(y)*previous.atlas.width*4;
        const auto b=sampled.atlas.pixels.begin()+std::size_t(y)*sampled.atlas.width*4;
        require(std::equal(a,a+std::size_t(previous.atlas.width)*4,b),
            "Prompt overlay changed retained MCC/source atlas pixels");pixels+=previous.atlas.width;
      }
      for(unsigned c:{0xe100u,0xe101u,0xe102u,0xe103u}) {
        const auto &g=sampled.glyphs.at(c);const auto &s=sampled.sampled_glyphs.at(c);
        const auto uv=retail_font_sample_uvs(sampled,c,1,1,g.width-2,g.height-2);
        require(uv[0]==(s.x+float(s.width)/g.width)/sampled.atlas.width && uv[2]>uv[0] && uv[3]>uv[1],
            "Clipped prompt UVs do not follow high resolution rectangle");
      }
    }
    std::cout<<"PASS "<<count<<" Vista prompt bindings, "<<unchanged<<" ordinary MCC glyph samples, "
        <<pixels<<" retained atlas pixels; metrics/lookup/clipping unchanged\n";
  } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
