#include "halo2/content.hpp"
#include "halo2/hud.hpp"
#include "halo2/ui_dds.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
using namespace halo2;
namespace halo2 {
Texture baseline_motion_sensor_composite(const MotionSensorRenderPlan &,const MotionSensorArtwork &,Vec3);
}
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
Texture read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);check(bool(file),"Missing source DDS");
  std::vector<std::byte> data(std::size_t(file.tellg()));file.seekg(0);
  check(bool(file.read(reinterpret_cast<char*>(data.data()),data.size())),"Truncated source DDS");
  return read_ui_dds_texture(Bytes(data));
}
std::filesystem::path source_path(const std::filesystem::path &trial,std::string name,unsigned frame) {
  name.erase(0,std::string("ui\\hud\\bitmaps\\").size());
  std::replace(name.begin(),name.end(),'\\','/');
  std::ostringstream page;page.width(4);page.fill('0');page<<frame;
  return trial/"sources"/name/(page.str()+".dds");
}
}
int main(int argc,char **argv) {
  try {
    check(argc==4,"Supply maps, Vista pack and reconstructed pack");
    const auto maps=std::filesystem::path(argv[1]),vista=std::filesystem::path(argv[2]),trial=std::filesystem::path(argv[3]);
    unsigned exact_pages{},exact_channels{},font_glyphs{},font_count{};
    {
      Cache shared(maps/"shared.map");std::map<std::uint32_t,UiImage> originals;
      for(const auto &tag:shared.tags())if(tag.group=="bitm" && tag.name.starts_with("ui\\hud\\")) {
        UiImage bank;bank.name=tag.name;
        const auto metadata=shared.load_source_tag_metadata(tag.datum);
        const Bytes header(metadata.bytes_at(tag.address,76));
        for(unsigned frame=0;frame<header.u32(68);++frame) {
          const Bytes page(metadata.bytes_at(header.u32(72)+frame*116,116));
          auto texture=read(source_path(trial,tag.name,frame));
          check(texture.width==page.u16(4) && texture.height==page.u16(6),"Source inventory changes original page dimensions");
          bank.images.push_back(std::move(texture));
        }
        originals.emplace(tag.datum,std::move(bank));
      }
      check(originals.size()==70,"Complete HUD inventory lost a source bank");
      const auto donor=load_ui_asset_library(originals,vista,"hud.tsv");
      const auto pack=load_ui_asset_library(originals,trial,"hud.tsv");
      check(pack.size()==originals.size(),"High-density HUD pack is missing an original bank");
      for(const auto &[datum,bank]:originals)for(unsigned frame=0;frame<bank.images.size();++frame) {
        auto source=donor.contains(bank.name)?donor.at(bank.name).at(frame):bank.images[frame];
        if(source.format>=14 && source.format<=16)source=prepare_xemu_bc_texture(source);
        const auto &target=pack.at(bank.name).at(frame);
        check(target.width%source.width==0 && target.height%source.height==0 &&
              target.width/source.width==target.height/source.height && target.width>source.width,
              "HUD page is not an integer source enlargement");
        const auto scale=target.width/source.width;
        check(target.format==11 && target.pixels.size()==std::size_t(target.width)*target.height*4,
              "Vector raster has an invalid decoded channel layout");
        // Full SVG raster identity and source shape bounds are independently
        // checked by verify_hud_4k.py; these are native texture-loader checks.
        const bool legacy=bank.name.ends_with("\\smg_meter") || bank.name.ends_with("\\battle_rifle_meter") ||
          bank.name.ends_with("\\shotgun_meter") || bank.name.ends_with("\\needler_meter");
        if(legacy)for(unsigned y=0;y<target.height;++y)for(unsigned x=0;x<target.width;++x) {
          check(target.pixels[(std::size_t(y)*target.width+x)*4+2]==source.pixels[(std::size_t(y/scale)*source.width+x/scale)*4+2],
                "Vector ammo page changes an authored blue selector label");++exact_channels;
        }
        ++exact_pages;
      }
      check(exact_pages==328,"Complete HUD pack lost an original frame");
    }
    {
      std::ifstream table(maps/"fonts/font_table.txt");std::set<std::string> names;
      for(std::string name;std::getline(table,name);) {if(!name.empty()&&name.back()=='\r')name.pop_back();names.insert(name);}
      for(const auto &name:names) {
        auto a=load_retail_font(trial/"fonts-source"/name);
        const auto b=load_retail_font(trial/"fonts"/name);
        check(a.glyphs.size()==b.glyphs.size() && a.kerning==b.kerning,"Font loses glyphs or kerning");
        auto logical=load_retail_font(maps/"fonts"/name),reference=logical;
        apply_retail_font_sampling(reference,a);
        apply_retail_font_sampling(logical,b,a); // validates original logical metrics and trace provenance
        for(const auto &[c,g]:reference.sampled_glyphs)
          check(logical.sampled_glyphs.contains(c),"Font enlargement loses admitted source glyph coverage");
        for(const auto &[c,g]:a.glyphs) {
          if(g.width && g.height)check(logical.sampled_glyphs.contains(c),"Vector font leaves an original private glyph or fallback alias at low density");
          const auto &h=b.glyphs.at(c);
          check(g.advance==h.advance && g.bearing_x==h.bearing_x && g.bearing_y==h.bearing_y,
                "Font enlargement moves an authored glyph");
          if(g.width && g.height)check(h.width>g.width && h.height>g.height,"Font glyph was not enlarged");
          ++font_glyphs;
        }
        for(const auto &[c,g]:reference.glyphs) {
          const auto &h=logical.glyphs.at(c);
          check(g.x==h.x && g.y==h.y && g.width==h.width && g.height==h.height && g.advance==h.advance &&
                g.bearing_x==h.bearing_x && g.bearing_y==h.bearing_y,"Font enlargement changes logical source metrics");
        }
        auto damaged=b;damaged.glyphs.at('A').advance+=1;
        auto unmodified=load_retail_font(maps/"fonts"/name);bool refused=false;
        try{apply_retail_font_sampling(unmodified,damaged,a);}catch(const FormatError &){refused=true;}
        check(refused,"Font donor validation admits changed layout metrics");
        ++font_count;
      }
    }
    Cache cache(maps/"lockout.map");const auto level=cache.load_content(0,true);
    unsigned positions=0,source_pages=0,alpha=0;
    for(unsigned representation:{2u,3u}) {
      const auto content=cache.load_hud(level.player_representations.at(representation).biped.datum,cache.slayer_starting_weapon());
      std::ifstream source_file(trial/"sources/smg_meter.dds",std::ios::binary|std::ios::ate);
      check(bool(source_file),"Missing independently decoded magazine source");
      std::vector<std::byte> bytes(std::size_t(source_file.tellg()));source_file.seekg(0);
      check(bool(source_file.read(reinterpret_cast<char*>(bytes.data()),bytes.size())),"Truncated magazine source");
      const auto decoded=read_ui_dds_texture(Bytes(bytes));
      const auto source=std::find_if(content.images.begin(),content.images.end(),[](const auto &entry){return entry.second.name.ends_with("\\smg_meter");});
      check(source!=content.images.end() && source->second.images[0].pixels==decoded.pixels,
            "Independent magazine resource decode differs from native Xbox decoder");
      Mesh before_mesh,after_mesh;
      MultiplayerHud before(content,maps/"fonts",before_mesh,4,"Slayer",{},vista),
                     after(content,maps/"fonts",after_mesh,4,"Slayer",{},trial);
      check(before_mesh.materials.size()==after_mesh.materials.size(),"Reconstruction changed material topology");
      MultiplayerState state;
      for(unsigned n=0;n<4;++n) {
        auto &p=state.players[n];p.connected=true;p.shield=p.health=1;p.loaded=60;p.reserve=120;
        p.source_unit=0x10000+n;p.source_player=0x20000+n;p.source_membership=n;p.position={float(n),0,0};p.velocity={2,0,0};
        if(representation==3){p.source_configuration=SourcePlayerConfiguration{};(*p.source_configuration)[0x44]=1;}
      }
      for(unsigned tick=0;tick<63;++tick) {
        state.tick=tick;
        auto plan=source_motion_sensor_render_plan({},0,0,{0,0},{0,0},tick,tick_seconds);
        plan.blips={{{8,32},1,1,{1,0,0},false},{{43,27},.5f,.75f,{1,1,0},true}};
        const auto old=baseline_motion_sensor_composite(plan,content.sensor.artwork,{.4f,.6f,.8f});
        const auto native=source_motion_sensor_composite(plan,content.sensor.artwork,{.4f,.6f,.8f});
        check(old.width==native.width && old.height==native.height && old.pixels==native.pixels,
              "Default radar differs from pre-reconstruction source implementation");
        ++source_pages;
        before.update_motion_sensor(state,0,0);after.update_motion_sensor(state,0,0);
        const auto &page=after_mesh.materials.at(*after.motion_sensor_material(0)).diffuse;
        check(page.width==256 && page.height==256,"Trial did not admit higher-density live radar");
        for(unsigned at=3;at<page.pixels.size();at+=4){check(page.pixels[at]==std::byte{},"Reconstructed radar writes destination alpha");++alpha;}
        for(unsigned pixel:{0u,255u,255u*256,256u*256-1})for(unsigned c=0;c<4;++c)
          check(page.pixels[pixel*4+c]==std::byte{},"Reconstructed radar leaks into square corners");
      }
      for(unsigned view=0;view<4;++view){
        before.update_motion_sensor(state,0,view);after.update_motion_sensor(state,0,view);
        before.animate(state,0,view,25,14400,0);after.animate(state,0,view,25,14400,0);
      }
      for(unsigned width:{640u,1280u,3840u})for(unsigned players=1;players<=4;++players)for(unsigned view=0;view<players;++view) {
        const auto height=width==640?480:width==1280?720:2160;
        const auto a=before.draw(state,0,view,players,width,height,25,14400);
        const auto b=after.draw(state,0,view,players,width,height,25,14400);
        check(a.size()==b.size(),"Reconstruction changed HUD quad count");
        for(unsigned n=0;n<a.size();++n) {
          check(a[n].position.x==b[n].position.x && a[n].position.y==b[n].position.y && a[n].position.z==b[n].position.z,
                "Reconstruction moved original HUD geometry");
          ++positions;
        }
      }
      for(unsigned n=0;n<before_mesh.materials.size();++n) {
        const auto &a=before_mesh.materials[n],&b=after_mesh.materials[n];
        check(a.name==b.name && a.forward_blend==b.forward_blend && a.forward_program.words==b.forward_program.words,
              "Reconstruction changed HUD shader or blend behavior");
      }
    }
    const auto donor=load_retail_font(vista/"fonts/conduit-13"),font=load_retail_font(trial/"fonts/conduit-13"),source_font=load_retail_font(maps/"fonts/conduit-13");
    for(unsigned c='0';c<='9';++c) {
      const auto &a=donor.glyphs.at(c),&b=font.glyphs.at(c),&logical=source_font.glyphs.at(c);
      check(b.width==a.width*3 && b.height==a.height*3 && b.advance==logical.advance && b.bearing_x==logical.bearing_x && b.bearing_y==logical.bearing_y,
            "Reconstructed digit changed source metrics or failed to increase raster density");
    }
    bool rejected=false;
    try{source_motion_sensor_composite({}, {}, {},65);}catch(const FormatError &){rejected=true;}
    check(rejected,"Unbounded radar raster dimensions admitted");
    std::cout<<positions<<" unchanged HUD vertices at SD/HD/4K across both appearances and1-4 views; "
             <<source_pages<<" byte-identical original radar pages; "<<alpha<<" zero-alpha reconstructed radar pixels; "
             <<exact_pages<<" vector raster pages / "<<exact_channels<<" retained ammo selector samples; "
             <<font_count<<" full fonts / "<<font_glyphs<<" codepoints including aliases; changed font metrics rejected\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
