#include "halo2/original_data.hpp"
#include "halo2/full_scoreboard_assets.hpp"
#include "halo2/full_scoreboard_painter.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

int main(int argc,char **argv) {
  try {
    if(argc!=6)throw std::runtime_error("Supply frozen provider/layout reports, original map, update image and painter report");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    nlohmann::json providers,layout;std::ifstream(argv[1])>>providers;std::ifstream(argv[2])>>layout;
    if(!providers.at("instruction_patches").empty() || !layout.at("instruction_patches").empty())
      throw std::runtime_error("Original outputs must be unpatched");
    const auto original=halo2::OriginalData::load_xbe(argv[4]);
    if(!original->full_scoreboard_name_mappings || !original->full_scoreboard_palette_fallback ||
        !original->full_scoreboard_paint_constants)throw std::runtime_error("Missing verified scoreboard imports");
    const auto compare=[](std::uint64_t value,unsigned size,const std::string &bytes) {
      if(bytes.size()!=size*2)throw std::runtime_error("Wrong original literal export length");
      for(unsigned i=0;i<size;++i)
        if(((value>>(i*8))&255)!=std::stoul(bytes.substr(i*2,2),nullptr,16))
          throw std::runtime_error("Original raw imported word differs");
    };
    const auto &m=*original->full_scoreboard_name_mappings;
    if(m.tokens.size()!=providers.at("tokens").size() || m.glyphs.size()!=providers.at("private_glyphs").size())
      throw std::runtime_error("Original full mapping table truncated");
    for(unsigned i=0;i<m.tokens.size();++i) {
      const auto &expected=providers.at("tokens").at(i);
      if(m.tokens[i].glyph!=std::stoul(expected.at("glyph").get<std::string>(),nullptr,16))
        throw std::runtime_error("Wrong original sanitizer glyph");
      const auto text=expected.at("text_utf16_hex").get<std::string>();
      if(text.size()!=m.tokens[i].text.size()*4)throw std::runtime_error("Wrong original sanitizer text size");
      for(unsigned j=0;j<m.tokens[i].text.size();++j)compare(m.tokens[i].text[j],2,text.substr(j*4,4));
    }
    for(unsigned i=0;i<m.glyphs.size();++i) {
      if(original->image==halo2::OriginalData::Image::MapPack11) {
        const auto bytes=providers.at("source_table_mapping").at("TU11").at("glyph_records12_hex").at(i).get<std::string>();
        compare(m.glyphs[i].glyph,4,bytes.substr(0,8));
        compare(m.glyphs[i].original_handler,4,bytes.substr(8,8));
        continue;
      }
      const auto &expected=providers.at("private_glyphs").at(i);
      if(m.glyphs[i].glyph!=std::stoul(expected.at("glyph").get<std::string>(),nullptr,16) ||
          m.glyphs[i].original_handler!=std::stoul(expected.at("handler").get<std::string>(),nullptr,16))
        throw std::runtime_error("Wrong original private glyph callback record");
    }
    const halo2::Cache cache(argv[3],{},original);const auto assets=cache.load_full_scoreboard_assets();
    if(!assets.scenario_present || assets.palette.size()!=providers.at("palette").at("count").get<unsigned>())
      throw std::runtime_error("Actual scenario/palette import missing");
    for(unsigned i=0;i<assets.palette.size();++i) {
      const auto hex=providers.at("palette").at("records12_hex").at(i).get<std::string>();
      for(unsigned j=0;j<3;++j)compare(assets.palette[i][j],4,hex.substr(j*8,8));
    }
    const auto fallback=providers.at("palette").at("default12_hex").get<std::string>();
    for(unsigned i=0;i<3;++i)compare((*original->full_scoreboard_palette_fallback)[i],4,fallback.substr(i*8,8));
    for(const auto &label:layout.at("tag_metadata").at("labels")) {
      const auto id=label.at("string_id").get<std::string>();
      const auto &actual=id=="07000105"?assets.name_label:id=="070000fd"?assets.wins_label:assets.score_label;
      if(actual!=label.at("text").get<std::string>())throw std::runtime_error("Actual locale label differs from original file lookup");
    }
    const auto &paint=*original->full_scoreboard_paint_constants;
    const auto &words=providers.at("renderer_literal_state");
    compare(paint.bright_rgb_threshold,4,words.at("0046a438").get<std::string>());
    compare(paint.pulse_phase,4,words.at("0046a2ec").get<std::string>());
    nlohmann::json painter;std::ifstream(argv[5])>>painter;
    if(!painter.at("instruction_patches").empty())throw std::runtime_error("Original painter output must be unpatched");
    const auto &style=painter.at("style_words");
    compare(paint.pixel_scale,4,style.at("0046a148").get<std::string>());
    compare(paint.pulse_one,4,style.at("0046a0d0").get<std::string>());
    compare(paint.pulse_half,4,style.at("0046a0cc").get<std::string>());
    const auto shadow=words.at("0044aaf8").get<std::string>();
    for(unsigned i=0;i<4;++i)compare(paint.shadow[i],4,shadow.substr(i*8,8));
    std::cout<<"Original18 sanitizer tokens/131 glyph callbacks/18 palette RGBs/fallback/three locale labels/all nine painter words import exactly\n";
    std::cout<<"LIMIT profile storage ownership and native scoreboard session adapter are separate\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
