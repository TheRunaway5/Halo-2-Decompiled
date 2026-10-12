#include "halo2/frontend_model_shader.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/frontend_model_locals.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
using namespace halo2;
namespace {
void require(bool yes,const char*message){if(!yes)throw FormatError(message);}
std::vector<unsigned> sections(const RenderModel&model){
 std::vector<unsigned> out;
 for(const auto&origin:model.vertex_origins)
  if(std::find(out.begin(),out.end(),origin.section)==out.end())out.push_back(origin.section);
 std::sort(out.begin(),out.end());return out;
}
}
int main(int argc,char**argv){try{
 require(argc==3,"frontend geometry needs original fixture and mainmenu map");
 if(!std::filesystem::exists(argv[2]))return 77;Cache cache(argv[2]);
 std::ifstream input(argv[1]);require(bool(input),"original frontend geometry fixture missing");
 unsigned cases=0,resident=0;
 for(std::string line;std::getline(input,line);){if(line.empty()||line[0]=='#')continue;
  std::istringstream row(line);row>>std::hex;std::array<unsigned,3>center;for(auto&w:center)row>>w;
  unsigned distance,unit,flags,aux,type,previous;row>>distance>>unit>>flags>>aux>>type>>previous;
  SourceGeometryLodDefinition expected;expected.model_present=true;for(auto&v:expected.model_bank){unsigned w;row>>w;v=std::bit_cast<float>(w);}
  unsigned metric,level,mode,count;row>>metric>>level>>mode>>count;std::vector<unsigned>wanted(count);for(auto&v:wanted)row>>v;
  require(bool(row),"truncated original geometry fixture");std::string extra;require(!(row>>extra),"trailing original geometry fields");
  const auto definition=cache.load_source_geometry_lod_definition(unit);
  require(definition.model_bank==expected.model_bank && definition.model_present,"literal geometry LOD bank differs from source fixture");
  require(cache.load_initial_model_definition(unit).render_model==mode,"literal render model differs from captured collector");
  const Vec3 owner{std::bit_cast<float>(center[0]),std::bit_cast<float>(center[1]),std::bit_cast<float>(center[2])};
  const auto chosen=frontend_model_geometry_lod(definition,owner,flags,aux,type,std::int8_t(previous));
  require(std::bit_cast<unsigned>(chosen.distance)==distance && std::bit_cast<unsigned>(chosen.metric)==metric && unsigned(chosen.level)==level,"frontend geometry metric/level differs from unchanged original caller");++cases;
  // Only captured singleton weapon regions use this active-permutation seam.
  // Visible body force5 is proved above; their actual region bytes are separate.
  if(cases<=40 && (unit==0xe2be0120u || unit==0xe3bd01c1u)) {
   const auto model=load_frontend_model_geometry(cache,unit,0,owner,flags,aux,std::int8_t(previous));
   auto actual=sections(model);wanted.erase(std::remove(wanted.begin(),wanted.end(),255u),wanted.end());std::sort(wanted.begin(),wanted.end());
   require(actual==wanted,"frontend held weapon loaded highest geometry instead of original owner-distance sections");++resident;
  }
 }
 require(cases==56 && resident==20,"all ten owner/held original cases were not checked");
 const auto content=cache.load_content(0);unsigned objects=0,frames=0;
 float minimum=std::numeric_limits<float>::max(),maximum=0;
 for(const auto&object:content.objects) {
  if(object.name<0 || (object.definition!=0xe23400bfu && object.definition!=0xe3110154u))continue;
  const auto name=content.object_names.at(object.name).name;
  if(name.find("ui_player")!=0)continue;
  const auto body=cache.load_model(object.definition,true,object.variant);
  const auto basis=authored_basis(object.rotation);
  const auto root=source_object_transform(object.position,basis.forward,basis.up,object.scale,false);
  const auto bounds=cache.load_object_shadow_definition(object.definition);
  const auto weapon=cache.load_frontend_model_weapon(object.definition);require(bool(weapon),"source menu held weapon absent");
  const auto bank=cache.load_source_geometry_lod_definition(*weapon);
  std::vector<AnimationClip>clips;const auto startup=cache.load_initialized_object_idle(object.definition);require(bool(startup),"source startup pose absent");clips.push_back(*startup);
  for(bool ui:{false,true})if(auto clip=cache.load_frontend_model_animation(object.definition,ui)) {
   if(std::none_of(clips.begin(),clips.end(),[&](const auto&v){return v.source_graph==clip->source_graph && v.source_index==clip->source_index;}))clips.push_back(*clip);
  }
  int retained=-1;
  for(const auto&clip:clips) {
   const auto primary=cache.load_frontend_model_primary_animation(clip.source_graph,clip.source_index);
   for(unsigned frame=0;frame<clip.frame_count;++frame) {
    const auto locals=frontend_model_locals(body,clip,float(frame),{},&primary);
    const auto nodes=pose_model_locals_nodes_world(body,root,locals);
    const auto sphere=source_shadow_sphere(bounds,nodes.front(),root.scale);
    const auto chosen=frontend_model_geometry_lod(bank,sphere.center,0,0,cache.load_source_geometry_scenario_type(),retained);
    require(chosen.level==0,"menu primary phase changed retained source weapon geometry level");retained=chosen.level;
    minimum=std::min(minimum,chosen.metric);maximum=std::max(maximum,chosen.metric);++frames;
   }
  }
  ++objects;
 }
 require(objects==10 && frames>0,"all ten authored root/primary frame ranges were not checked");
 std::cout<<cases<<" exact geometry caller cases; "<<resident<<" literal resident weapon selections; "<<objects<<" authored roots / "<<frames<<" primary samples remain level0, metric range ["<<minimum<<","<<maximum<<"]\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
