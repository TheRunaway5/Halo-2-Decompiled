#include "halo2/frontend.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/frontend_model.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/object_lighting.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
void inputs(const std::filesystem::path &fixture){
 std::ifstream input(fixture);unsigned count=0;
 for(std::string line;std::getline(input,line);){
  if(line.empty()||line[0]=='#')continue;
  unsigned foreground,background,flags;std::string name,result,opacity;
  std::istringstream row(line);row>>foreground>>background>>flags>>name>>result>>opacity;
  check(bool(row),"incomplete original emblem input case");
  FrontendAppearance a;a.foreground=foreground;a.background=background;a.flags=flags;
  const auto word=std::bit_cast<std::uint32_t>(frontend_emblem_input(a,std::stoul(name,nullptr,16),std::bit_cast<float>(std::uint32_t(std::stoul(opacity,nullptr,16)))));
  check(word==std::stoul(result,nullptr,16),"emblem appearance input differs from original14CCF0/22ACE7");++count;
 }
 check(count==682,"original emblem input coverage changed");
}
void original(const Cache &cache,const FrontendContent &content,const std::filesystem::path &program){
 auto m=frontend_emblem_material(cache,0xe5560336);
 FrontendAppearance a;a.colors={8,12,9,1};a.foreground=26;a.background=9;
 update_frontend_emblem(m,a,content.appearance_palette,1);
 std::array<std::uint32_t,60> expected{};std::ifstream file(program,std::ios::binary);file.read(reinterpret_cast<char*>(expected.data()),sizeof expected);
 check(file.gcount()==sizeof expected,"incomplete retained original emblem program");
 if(m.forward_program.words!=expected){for(unsigned i=0;i<60;++i)if(m.forward_program.words[i]!=expected[i])std::cerr<<"program word "<<i<<" native="<<std::hex<<m.forward_program.words[i]<<" original="<<expected[i]<<std::dec<<'\n';}
 check(m.forward_program.words==expected,"emblem pixel program differs from retained original22A664 draw");
 check(m.ui_emblem_pass&&m.hud_pass&&m.forward_vertex==11&&m.forward_blend==1,"emblem lost exact screen_hud/normal-alpha pass binding");
 check(m.animated_textures[0].size()==32&&m.animated_textures[1].size()==64,"emblem lost original background/foreground page banks");
 check(m.texture_frames[0]==9&&m.texture_frames[1]==26,"emblem page selection differs from retained original");
 for(unsigned page=0;page<64;++page){a.foreground=page;a.background=page%32;update_frontend_emblem(m,a,content.appearance_palette,1);
  check(m.texture_frames[0]==page%32&&m.texture_frames[1]==page,"authored emblem frame overlay selects wrong page");}
 const auto baseline=m.forward_program.words;a.flags=1;update_frontend_emblem(m,a,content.appearance_palette,1);
 check(m.forward_program.words!=baseline,"X toggle did not reach original foreground overlay input");
 a.flags=0;update_frontend_emblem(m,a,content.appearance_palette,.5f);
 check(m.forward_program.words!=baseline,"widget alpha did not reach original opacity overlay");
}
void model_inputs(const std::filesystem::path &fixture){
 std::ifstream input(fixture);unsigned count=0;
 for(std::string line;std::getline(input,line);){
  if(line.empty()||line[0]=='#')continue;
  unsigned foreground,background,flags,supported,enabled;std::string name,result;
  std::istringstream row(line);row>>foreground>>background>>flags>>name>>result>>supported>>enabled;
  check(bool(row),"incomplete original model emblem input case");
  FrontendAppearance a;a.foreground=foreground;a.background=background;a.flags=flags;
  const auto value=frontend_model_emblem_input(a,std::stoul(name,nullptr,16));
  check(bool(value)==bool(supported),"model emblem function support differs from originalBCC20");
  check(std::bit_cast<std::uint32_t>(value.value_or(0))==std::stoul(result,nullptr,16),"model emblem function differs from originalBCC20/BC970");
  check(enabled==unsigned(supported),"fixture changed original model emblem enable boundary");++count;
 }
 check(count==6144,"original model emblem input coverage changed");
}
void model_materials(const Cache &cache,const FrontendContent &content,const std::filesystem::path &fixtures){
 const std::array<std::pair<std::uint32_t,const char *>,2> shaders{{
  {0xe24b00d6,"spartan"},{0xe33a018f,"elite"}}};
 for(const auto &[datum,species]:shaders){
  const auto authored=cache.load_material(datum);
  check(!authored.lods.empty(),"model emblem has no authored shader LOD");
  for(unsigned lod=0;lod<authored.lods.size();++lod){
   auto m=authored;m.selected_shader_lod=lod;
   check(prepare_dynamic_material(cache,m,{},true,0.f),"model emblem dynamic pass not prepared");
   check(m.animated_textures[0].size()==64,"model emblem lost original foreground bank");
   FrontendAppearance a;a.colors={8,12,9,1};a.foreground=26;a.background=9;
   auto update=[&](double time){
    apply_frontend_appearance(std::span<Material>(&m,1),a,content.appearance_palette);
    const auto input=[&](std::uint32_t name){return frontend_model_material_input(&a,
        cache.load_frontend_model_initial_shield_fraction(
            datum==0xe24b00d6?0xe23400bf:0xe3110154),name).value_or(0);};
    animate_frontend_model_material(m,time,input);
   };
   update(0);
   std::array<std::uint32_t,60> expected{};
   std::ifstream file(fixtures/(std::string("frontend-model-emblem-")+species+"-live-program.bin"),std::ios::binary);
   file.read(reinterpret_cast<char*>(expected.data()),sizeof expected);
   check(file.gcount()==sizeof expected,"retained live model emblem program is incomplete");
   if(m.forward_program.words!=expected){for(unsigned i=0;i<60;++i)if(m.forward_program.words[i]!=expected[i])std::cerr<<species<<" model word "<<i<<" native="<<std::hex<<m.forward_program.words[i]<<" original="<<expected[i]<<std::dec<<'\n';}
   check(m.forward_program.words==expected,"model emblem program differs from original16F32");
   check(m.texture_frames[0]==26,"model emblem did not select source foreground");
   for(unsigned page=0;page<64;++page){a.foreground=page;update(12.375);
    check(m.texture_frames[0]==page,"model emblem page function did not reach bitmap sampler");}
   const auto before=m.forward_program.words;a.flags=1;update(12.375);
   check(m.forward_program.words!=before,"model emblem toggle did not reach pixel combiner");
   const auto hidden=m.forward_program.words;a.background=31;update(99.5);
   check(m.forward_program.words==hidden&&m.texture_frames[0]==63,"model emblem added a background or clock input absent from authored pass");
  }
 }
}
void model_shield(const Cache &cache,const std::filesystem::path &fixtures){
 std::ifstream input(fixtures/"frontend-model-shield-original.tsv");unsigned count=0;
 for(std::string line;std::getline(input,line);){
  if(line.empty()||line[0]=='#')continue;
  unsigned kind;std::uint32_t supplied,expected;std::istringstream row(line);
  row>>kind>>std::hex>>supplied>>expected;check(bool(row),"incomplete original shield context");
  float result;
  if(kind==0)result=cache.load_frontend_model_initial_shield_fraction(supplied);
  else if(kind==1)result=frontend_model_initial_shield_fraction(std::bit_cast<float>(supplied));
  else if(kind==2)result=frontend_model_material_input(nullptr,std::bit_cast<float>(supplied),0x0f000559).value();
  else throw FormatError("unknown original shield case");
  check(std::bit_cast<std::uint32_t>(result)==expected,"model shield fraction differs from B7B40/D5B60/BCC20");++count;
 }
 check(count==29,"original shield initialization coverage changed");
 std::array<std::uint32_t,60> expected{};
 std::ifstream bank(fixtures/"frontend-model-elite-arms-emissive-live-program.bin",std::ios::binary);
 bank.read(reinterpret_cast<char*>(expected.data()),sizeof expected);
 check(bank.gcount()==sizeof expected,"retained original Elite layer7 program is incomplete");
 const auto authored=cache.load_material(0xe325017a);
 check(authored.lods.size()==4,"original Elite arms shader LOD coverage changed");
 const auto shield=cache.load_frontend_model_initial_shield_fraction(0xe3110154);
 const auto context=[&](std::uint32_t name){return frontend_model_material_input(nullptr,shield,name).value_or(0);};
 for(unsigned lod=0;lod<authored.lods.size();++lod){
  auto m=authored;m.selected_shader_lod=lod;
  check(prepare_dynamic_material(cache,m,{},true,0.f),"Elite arms source pass unresolved");
  auto emitter=std::find_if(m.world_overlays_after.begin(),m.world_overlays_after.end(),
      [](const auto &p){return p&&p->forward_vertex==2;});
  check(emitter!=m.world_overlays_after.end(),"Elite arms lost original layer7 child");
  check((*emitter)->forward_program.words[10]==0,"unanimated child no longer reproduces missing emissive color");
  auto peer=m;const auto shared=*emitter;
  animate_frontend_model_material(m,0,context);
  check((*emitter)->forward_program.words==expected,"Elite child program differs from actual original16F32");
  check(shared->forward_program.words[10]==0,"per-model child animation mutated shared resident pass");
  check((*emitter).get()!=shared.get(),"per-model animation retained a shared mutable child bank");
  for(double time:{12.375,99.5}){
   animate_frontend_model_material(m,time,context);
   check((*emitter)->forward_program.words==expected,"Elite emitter acquired an unauthored clock input");
  }
  animate_frontend_model_material(m,0,[](std::uint32_t){return 0.f;});
  check((*emitter)->forward_program.words[10]==0,"shield input no longer controls original emitter color");
  animate_frontend_model_material(m,0,context);
  check((*emitter)->forward_program.words==expected,"restored source shield input did not restore emitter");
 }
}
void geometry(const FrontendContent &content,const std::filesystem::path &maps){
 const auto &pane=content.screens.at(47).panes.front();
 check(pane.grids.size()==1&&pane.grids[0].skin==0xe5550335&&pane.grids[0].position.x==-271&&pane.grids[0].position.y==38,"appearance grid not read at original pane+68");
 const auto &skin=content.skin_groups.at(pane.grids[0].skin);check(skin.emblems.size()==2,"appearance group lost its two original type9 widgets");
 MenuState state(content);state.open(47);Mesh mesh;FrontendPainter painter(content,maps/"fonts",mesh);const auto draw=painter.draw(state,5);
 std::vector<std::array<float,4>> bounds;
 for(std::size_t first=0;first<draw.vertices.size();){const auto material=draw.vertices[first].material;auto end=first+1;while(end<draw.vertices.size()&&draw.vertices[end].material==material)++end;
  if(mesh.materials.at(material).ui_emblem_pass){check(end-first==6,"emblem UI quad is incomplete");float left=1e6f,top=1e6f,right=-1e6f,bottom=-1e6f;
   for(auto i=first;i<end;++i){const auto &v=draw.vertices[i];left=std::min(left,v.position.x);top=std::min(top,v.position.y);right=std::max(right,v.position.x);bottom=std::max(bottom,v.position.y);
    check(v.position.z==1&&v.color.x==1&&v.color.y==1&&v.color.z==1,"emblem immediate vertex color differs from original opaque v9");}
   bounds.push_back({left,top,right,bottom});}
  first=end;
 }
 check(bounds==std::vector<std::array<float,4>>{{49,74,177,202},{190,74,222,106}},"emblem group placement differs from original2BACBC/live22A664");
}
}
int main(int argc,char**argv){try{
 if(argc!=4)throw FormatError("emblem check needs inputs/program/maps");
 const auto maps=std::filesystem::path(argv[3]);if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2])||!std::filesystem::exists(maps/"mainmenu.map"))return 77;
 const auto fixtures=std::filesystem::path(argv[1]).parent_path();
 if(!std::filesystem::exists(fixtures/"frontend-model-emblem-inputs-original.tsv")||
    !std::filesystem::exists(fixtures/"frontend-model-emblem-spartan-live-program.bin")||
    !std::filesystem::exists(fixtures/"frontend-model-emblem-elite-live-program.bin")||
    !std::filesystem::exists(fixtures/"frontend-model-shield-original.tsv")||
    !std::filesystem::exists(fixtures/"frontend-model-elite-arms-emissive-live-program.bin"))return 77;
 inputs(argv[1]);model_inputs(fixtures/"frontend-model-emblem-inputs-original.tsv");Cache cache(maps/"mainmenu.map");const auto content=cache.load_frontend();original(cache,content,argv[2]);model_materials(cache,content,fixtures);model_shield(cache,fixtures);geometry(content,maps);
 std::cout<<"682 UI + 6144 model +29 shield original inputs, 4 live program banks, all Elite child LODs, authored64-page selection, toggles and UI quads passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
