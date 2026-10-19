#include "halo2/frontend.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/ui_dds.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void require(bool value,const char *why) {if(!value)throw FormatError(why);}
template<class F> void rejects(F &&call) {
  try {call();}catch(const FormatError &){return;}
  throw FormatError("Malformed prompt binding admitted");
}
bool control(unsigned c) {return (c>=0xe100 && c<=0xe111) || (c>=0xe12c && c<=0xe13c);}
bool action_control(unsigned c) {return (c>=0xe100 && c<=0xe10f) || (c>=0xe132 && c<=0xe135);}
bool same(const FontGlyph &a,const FontGlyph &b) {
  return a.x==b.x && a.y==b.y && a.width==b.width && a.height==b.height &&
      a.advance==b.advance && a.bearing_x==b.bearing_x && a.bearing_y==b.bearing_y;
}
void layout(const RetailFont &a,const RetailFont &b) {
  require(a.ascent==b.ascent && a.descent==b.descent && a.leading==b.leading &&
      a.file_fingerprint==b.file_fingerprint && a.glyph_record_indices==b.glyph_record_indices &&
      a.kerning==b.kerning && a.glyphs.size()==b.glyphs.size(),"Prompt overlay changed source lookup/metrics");
  for(const auto &[c,g]:a.glyphs)require(same(g,b.glyphs.at(c)),"Prompt overlay changed logical glyph geometry");
}
void pixels(const RetailFont &a,const RetailFont &b) {
  for(unsigned y=0;y<a.atlas.height;++y)
    require(std::equal(a.atlas.pixels.begin()+std::size_t(y)*a.atlas.width*4,
        a.atlas.pixels.begin()+std::size_t(y+1)*a.atlas.width*4,
        b.atlas.pixels.begin()+std::size_t(y)*b.atlas.width*4),"Prompt overlay changed retained ordinary atlas pixels");
  for(const auto &[c,s]:a.sampled_glyphs)if(!control(c)) {
    const auto &t=b.sampled_glyphs.at(c);
    require(s.x==t.x && s.y==t.y && s.width==t.width && s.height==t.height,
        "Prompt overlay changed an ordinary sampled glyph");
  }
}
void retained_state(const RetailFont& a,const RetailFont& b) {
  require(a.ascent==b.ascent && a.descent==b.descent && a.leading==b.leading &&
      a.file_fingerprint==b.file_fingerprint && a.glyph_record_indices==b.glyph_record_indices &&
      a.kerning==b.kerning && a.glyphs.size()==b.glyphs.size(),"Action metrics changed ordinary font state");
  require(a.atlas.width==b.atlas.width && a.atlas.height==b.atlas.height && a.atlas.format==b.atlas.format &&
      a.atlas.layers==b.atlas.layers && a.atlas.depth==b.atlas.depth && a.atlas.mips.empty()==b.atlas.mips.empty() &&
      a.atlas.pixels==b.atlas.pixels && a.sampled_glyphs.size()==b.sampled_glyphs.size(),
      "Action metrics changed atlas or sample ownership");
  for(const auto& [c,s]:a.sampled_glyphs) {
    const auto& t=b.sampled_glyphs.at(c);
    require(s.x==t.x && s.y==t.y && s.width==t.width && s.height==t.height,
        "Action metrics changed a sampled rectangle");
  }
}
void action_metrics(const RetailFont& before,unsigned selector,const nlohmann::json& reconstruction,bool menu=false) {
  const auto project=[&](RetailFont& font,unsigned owner) {
    return menu?apply_mcc_menu_button_metrics(font,owner):apply_mcc_action_button_metrics(font,owner);
  };
  auto projected=before;
  require(project(projected,selector)==(selector==5?0u:16u),
      "Action metric projection changed the wrong control count");
  retained_state(before,projected);
  unsigned controls{};
  for(const auto& [c,g]:before.glyphs) {
    const auto& after=projected.glyphs.at(c);
    if(!action_control(c)) {require(same(g,after),"Action metrics changed an ordinary glyph");continue;}
    ++controls;
    const auto& records=reconstruction.at("records");
    const auto source=std::find_if(records.begin(),records.end(),[&](const auto& r) {
      return r.at("selector")==selector && r.at("codepoint")==c;
    });
    require(source!=records.end() && source->at("source_record")==projected.glyph_record_indices.at(c),
        "Action metric source record differs from authored metadata");
    const auto size=source->at("source_size").template get<std::array<unsigned,2>>();
    // Advance and bearings are the independently authenticated40-row offline
    // metadata, not dimensions inferred from the MCC raster or an offset fix.
    require(after.x==g.x && after.y==g.y && after.width==size[0] && after.height==size[1] &&
        after.advance==int(size[0]) && after.bearing_x==0 && after.bearing_y==(menu?29:selector==5?22:32),
        "Action metric tuple differs from historical authored source");
    require(retail_font_sample_uvs(before,c,0,0,g.width,g.height)==
        retail_font_sample_uvs(projected,c,0,0,after.width,after.height),"Projection changed full authored sample UVs");
  }
  require(controls==20,"Action metric fixture did not cover20 controls");
  const auto once=projected;
  require(project(projected,selector)==0,"Action metric projection is not idempotent");
  retained_state(once,projected);layout(once,projected);
  const auto reject=[&](auto mutate) {
    auto candidate=before;mutate(candidate);const auto corrupted=candidate;
    rejects([&]{project(candidate,selector);});
    retained_state(corrupted,candidate);layout(corrupted,candidate);
  };
  reject([](auto& f){f.file_fingerprint^=1;});
  reject([](auto& f){f.glyph_record_indices.pop_back();});
  reject([](auto& f){f.glyph_record_indices.at(0xe135)++;});
  reject([](auto& f){f.glyph_record_indices.at('A')=f.glyph_record_indices.at(0xe135);});
  reject([](auto& f){f.glyphs.at(0xe135).advance++;});
  reject([](auto& f){f.glyphs.erase(0xe135);});
  reject([](auto& f){f.sampled_glyphs.erase(0xe135);});
  reject([](auto& f){f.sampled_glyphs.at(0xe135).x++;});
  reject([](auto& f){f.sampled_glyphs.at(0xe135).height++;});
  reject([](auto& f){f.atlas.pixels.pop_back();});
  auto wrong=before;
  rejects([&]{project(wrong,4);});retained_state(before,wrong);layout(before,wrong);
  rejects([&]{project(wrong,menu?6:selector==5?6:5);});retained_state(before,wrong);layout(before,wrong);
  if(selector!=5) {
    reject([&](auto& f){f.glyphs.at(0xe100)=once.glyphs.at(0xe100);});
  }
  if(selector==6) {
    const auto& x=projected.glyphs.at(0xe102);const auto& key=projected.glyphs.at(0xe132);
    const float scale=18.f/x.height;
    require(x.width*scale==16 && x.height*scale==18 && key.width*scale==19 &&
        key.height*scale==19 && key.bearing_y*scale==16,"Historical controller/keycap geometry not restored");
  }
  auto ordinary=before;ordinary.glyphs.at('A').bearing_x++;
  const auto ordinary_before=ordinary;
  require(project(ordinary,selector)==(selector==5?0u:16u),"Unrelated glyph suppressed projection");
  retained_state(ordinary_before,ordinary);
  for(const auto& [c,g]:ordinary_before.glyphs)if(!action_control(c))
    require(same(g,ordinary.glyphs.at(c)),"Projection mutated an unrelated custom glyph");
}
struct Owned {
  std::filesystem::path root,previous=std::filesystem::current_path();
  Owned() {
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch,"Owned test scratch required");
    root=std::filesystem::path(scratch)/"MCC custom prompts with spaces";
    require(std::filesystem::create_directory(root),"Test scratch already exists");
  }
  ~Owned(){std::filesystem::current_path(previous);std::filesystem::remove_all(root);}
};
void write(const std::filesystem::path &path,const std::string &text) {std::ofstream(path)<<text;}
Texture decoded_atlas(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  require(bool(file),"Authored atlas missing");
  std::vector<std::byte> bytes(std::size_t(file.tellg()));file.seekg(0);
  require(bool(file.read(reinterpret_cast<char*>(bytes.data()),bytes.size())),"Authored atlas truncated");
  return read_ui_dds_texture(Bytes(bytes));
}
}
int main(int argc,char **argv) {try {
  if(argc!=3)throw FormatError("Imported MCC font directory (or imported-root directory) and authored prompt bank required");
  std::filesystem::path font_directory=argv[1],bank_source=argv[2];
  if(!std::filesystem::exists(font_directory/"font_table.txt")) {
    if(!std::filesystem::is_directory(font_directory))return 77;
    bool found=false;
    for(const auto &entry:std::filesystem::directory_iterator(font_directory)) {
      const auto fonts=entry.path()/"halo2/h2_fonts";
      if(std::filesystem::exists(fonts/"font_table.txt")){font_directory=fonts;found=true;break;}
    }
    if(!found)return 77;
  }
  if(!std::filesystem::exists(bank_source/"prompts.tsv"))return 77;
  const auto source=load_gameplay_font_source(font_directory);
  require(source.domain==GameplayFontDomain::Mcc && source.names.size()==12,"Imported MCC font owner required");
  std::ifstream reconstruction_file(bank_source/"reconstruction.json");
  const auto reconstruction=nlohmann::json::parse(reconstruction_file);
  require(reconstruction.at("density")==4,"Authored density provenance differs");
  Owned scratch;
  const auto fonts=scratch.root/"cache/halo2/h2_fonts",bank=scratch.root/"cache/presentation/prompts";
  std::filesystem::create_directories(fonts);std::filesystem::create_directories(bank/"atlases");
  for(const auto &entry:std::filesystem::directory_iterator(font_directory))
    if(entry.is_regular_file())std::filesystem::create_hard_link(entry.path(),fonts/entry.path().filename());
  std::filesystem::copy_file(bank_source/"prompts.tsv",bank/"prompts.tsv");
  for(unsigned i=0;i<6;++i)std::filesystem::create_hard_link(bank_source/"atlases"/(std::to_string(i)+".dds"),bank/"atlases"/(std::to_string(i)+".dds"));
  write(bank/"prompt-source.tsv","H2_VISTA_PROMPTS_V1\n.\n");
  const auto other=scratch.root/"unrelated CWD";std::filesystem::create_directory(other);std::filesystem::current_path(other);
  require(font_prompt_asset_directory(fonts)==bank,"Direct MCC fonts did not select their separate cache prompt bank");
  require(font_prompt_asset_directory(scratch.root/"legacy/fonts",other)==other,"Legacy explicit prompt pack changed");
  std::size_t total{},retained{},different_dimensions{};
  const auto private_source=load_gameplay_font_source(fonts);
  for(unsigned selector=0;selector<12;++selector) {
    const auto before=load_retail_font(fonts/source.names[selector]);auto sampled=before;
    const auto changed=apply_vista_prompt_sampling(sampled,font_prompt_asset_directory(fonts),selector);
    require(changed>0,"MCC direct font omitted authored control samples");total+=changed;layout(before,sampled);pixels(before,sampled);
    std::ifstream rows(bank/"prompts.tsv");std::string line;std::getline(rows,line);
    while(std::getline(rows,line)) {
      std::istringstream row(line);unsigned owner,c,x,y,w,h;std::string atlas;row>>owner>>c>>x>>y>>w>>h>>atlas;
      if(owner!=selector)continue;
      const auto &g=before.glyphs.at(c);const auto &s=sampled.sampled_glyphs.at(c);
      require(s.x==before.atlas.width+1+x && s.y==y && s.width==w && s.height==h,"Sample differs from exact authored atlas rectangle");
      const auto authored=std::find_if(reconstruction.at("records").begin(),reconstruction.at("records").end(),[&](const auto &record){
        return record.at("selector")==selector && record.at("codepoint")==c;
      });
      require(authored!=reconstruction.at("records").end() && authored->at("name")==source.names[selector] &&
          authored->at("source_record")==before.glyph_record_indices.at(c),"Authored artwork has a different named codepoint owner");
      const auto original_size=authored->at("source_size").template get<std::array<unsigned,2>>();
      require(w==4*original_size[0] && h==4*original_size[1] && authored->at("raster_size")==nlohmann::json::array({w,h}) &&
          authored->at("rectangle")==nlohmann::json::array({x,y,w,h}) && authored->at("atlas")==atlas,
          "Sample differs from historical authored raster/density provenance");
      different_dimensions+=g.width!=original_size[0] || g.height!=original_size[1];
      const auto uv=retail_font_sample_uvs(sampled,c,1,1,int(g.width)-2,int(g.height)-2);
      require(uv==std::array<float,4>{(s.x+float(s.width)/g.width)/sampled.atlas.width,
          (s.y+float(s.height)/g.height)/sampled.atlas.height,
          (s.x+float(g.width-1)*s.width/g.width)/sampled.atlas.width,
          (s.y+float(g.height-1)*s.height/g.height)/sampled.atlas.height},"Prompt clipping lost source logical rectangle");
    }
    const auto first=std::find_if(reconstruction.at("records").begin(),reconstruction.at("records").end(),
        [&](const auto &record){return record.at("selector")==selector;});
    const auto authored_atlas=decoded_atlas(bank/first->at("atlas").template get<std::string>());
    for(unsigned y=0;y<authored_atlas.height;++y)
      require(std::equal(authored_atlas.pixels.begin()+std::size_t(y)*authored_atlas.width*4,
          authored_atlas.pixels.begin()+std::size_t(y+1)*authored_atlas.width*4,
          sampled.atlas.pixels.begin()+(std::size_t(y)*sampled.atlas.width+before.atlas.width+1)*4),"Prompt overlay changed authored bitmap bytes");
    retained+=before.atlas.pixels.size();
  }
  require(total==331,"Authored MCC prompt binding count differs from pinned331-row pack");
  require(different_dimensions==178,"MCC versus historical artwork dimension inventory changed");
  // Dedicated action buttons use only finite metadata, never offline fonts.
  for(const unsigned selector:{1u,5u,6u,7u}) {
    auto buttons=load_retail_font(fonts/source.names[selector]);
    const auto unsampled=buttons;
    const bool menu=selector==1 || selector==7;
    rejects([&]{if(menu)apply_mcc_menu_button_metrics(buttons,selector);else apply_mcc_action_button_metrics(buttons,selector);});
    retained_state(unsampled,buttons);layout(unsampled,buttons);
    apply_vista_prompt_sampling(buttons,bank,selector);
    action_metrics(buttons,selector,reconstruction,menu);
  }
  // Ordinary gameplay retains its already projected original logical owner.
  for(const unsigned selector:{5u,6u,8u}) {
    const auto before=load_gameplay_font(private_source,selector);auto sampled=before;
    const auto changed=apply_vista_prompt_sampling(sampled,bank,selector);require(changed>0,"Projected HUD font omitted owned controls");
    layout(before,sampled);pixels(before,sampled);
    for(const auto &[c,s]:sampled.sampled_glyphs)if(control(c) && s.x>before.atlas.width) {
      const auto index=before.glyph_record_indices.at(c);
      require(std::count(before.glyph_record_indices.begin(),before.glyph_record_indices.end(),index)==1,
          "Projected HUD font admitted fallback alias artwork");
    }
  }
  auto aliased=load_retail_font(fonts/source.names[6]);const auto before=aliased;
  aliased.glyph_record_indices[0xe100]=aliased.glyph_record_indices[0xe101];
  const auto changed=apply_vista_prompt_sampling(aliased,bank,6);
  require(changed==27 && !aliased.sampled_glyphs.contains(0xe100) && !aliased.sampled_glyphs.contains(0xe101),"Prompt sampler admitted a shared control alias");
  const auto absolute=scratch.root/"absolute legacy binding";std::filesystem::create_directory(absolute);
  write(absolute/"prompt-source.tsv","H2_VISTA_PROMPTS_V1\n"+bank.string()+"\n");
  auto legacy=before;require(apply_vista_prompt_sampling(legacy,absolute,6)==29,"Existing absolute prompt binding no longer works");
  write(bank/"prompt-source.tsv","H2_VISTA_PROMPTS_V1\n.\nextra\n");auto rejected=before;
  rejects([&]{apply_vista_prompt_sampling(rejected,bank,6);});layout(before,rejected);
  require(rejected.atlas.pixels==before.atlas.pixels,"Malformed binding mutated font before rejection");
  std::cout<<"PASS331 exact raw MCC controls;12selectors;3projected HUD owners;"<<retained
      <<" ordinary atlas bytes retained;80 guarded action/menu tuples,selector5 no-op,selectors1/6/7 projection,idempotence,atomic rejection;"
      <<"relative/absolute bindings,CWD independence,alias rejection,metrics/lookup/kerning/clippedUVs\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
