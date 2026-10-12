#include "halo2/cache.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
std::vector<std::string> names(const std::filesystem::path &path) {
  std::ifstream file(path);std::vector<std::string> result;
  for(std::string row;std::getline(file,row);) {
    if(!row.empty() && row.back()=='\r')row.pop_back();result.push_back(row);
  }
  return result;
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)return 2;
    const std::filesystem::path maps=argv[1],pack=argv[2];
    if(!std::filesystem::exists(maps/"shared.map") || !std::filesystem::exists(pack/"manifest.json"))return 77;
    check(is_mcc_classic_hud_pack(pack),"MCC source marker missing");
    Cache cache(maps/"shared.map");std::map<std::uint32_t,UiImage> images;
    for(const auto &tag:cache.tags()) {
      if(tag.group!="bitm" || !tag.address || !tag.name.starts_with("ui\\hud\\"))continue;
      UiImage image;image.name=tag.name;
      for(unsigned i=0;i<cache.bitmap_count(tag.datum);++i)image.images.push_back(cache.load_bitmap(tag.datum,i));
      images.emplace(tag.datum,std::move(image));
    }
    const auto sampled=load_ui_asset_library(images,pack,"hud.tsv");
    check(sampled.size()==images.size() && sampled.size()==70,"Incomplete MCC equivalent HUD banks");
    unsigned pages=0;
    for(const auto &[datum,image]:images) {
      const auto &bank=sampled.at(image.name);
      check(bank.size()==image.images.size(),"MCC changed original bank count");
      pages+=bank.size();
    }
    check(pages==328,"Incomplete MCC equivalent HUD pages");
    check(sampled.at("ui\\hud\\bitmaps\\new_hud\\crosshairs\\hud_reticles")[0].width==220,
          "MCC 4x-original reticle was not imported");
    check(sampled.at("ui\\hud\\bitmaps\\hud_sweeper")[0].width==128,"MCC radar texture missing");
    const auto original_names=names(maps/"fonts/font_table.txt");
    const auto imported_names=names(pack/"fonts/font_table.txt");
    check(original_names.size()==11 && imported_names.size()==12,"Font source tables incomplete");
    unsigned glyphs=0,controls=0;
    for(unsigned selector=0;selector<original_names.size();++selector) {
      auto original=load_retail_font(maps/"fonts"/original_names[selector]);const auto before=original;
      const auto path=hud_sampled_font_path(pack,selector,original_names[selector]);
      check(path.filename()==imported_names[selector],"MCC authored font selector was ignored");
      const auto source=load_retail_font(path);
      const auto count=apply_mcc_classic_font_sampling(original,source);
      check(count>0,"No authored MCC glyphs selected");glyphs+=count;
      check(original.ascent==before.ascent && original.descent==before.descent && original.leading==before.leading &&
            original.kerning==before.kerning && original.glyph_record_indices==before.glyph_record_indices,
            "MCC font import changed original layout/lookup metrics");
      for(const auto &[c,logical]:original.glyphs) {
        const auto &old=before.glyphs.at(c);
        check(logical.x==old.x && logical.y==old.y && logical.width==old.width && logical.height==old.height &&
            logical.advance==old.advance && logical.bearing_x==old.bearing_x && logical.bearing_y==old.bearing_y,
            "MCC changed logical glyph bounds");
      }
      for(const auto &[c,sample]:original.sampled_glyphs) {
        const auto &g=source.glyphs.at(c);
        check(sample.width==g.width && sample.height==g.height,"MCC glyph raster was resized");
        for(unsigned y=0;y<g.height;++y)for(unsigned x=0;x<g.width;++x)for(unsigned channel=0;channel<4;++channel)
          check(original.atlas.pixels[((sample.y+y)*original.atlas.width+sample.x+x)*4+channel]==
              source.atlas.pixels[((g.y+y)*source.atlas.width+g.x+x)*4+channel],"MCC glyph pixels changed");
        if(c>=0xe100 && c<=0xe111)++controls;
      }
    }
    check(controls>=100,"MCC authored controller glyphs not covered");
    std::cout<<"MCC Classic: "<<sampled.size()<<" HUD banks, "<<pages<<" pages, "<<glyphs
             <<" native glyph selections, "<<controls<<" controller glyph selections; logical font metrics preserved\n";
    return 0;
  } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
