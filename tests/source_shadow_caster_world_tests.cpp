#include "halo2/source_shadow_plan.hpp"
#include "halo2/source_model_palette.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/vertex_program.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

namespace {
using namespace halo2;
using Outputs=std::array<ShaderVector,7>;
constexpr unsigned output_ids[]{0,3,4,9,10,11,12};
void require(bool v,const std::string&m){if(!v)throw FormatError(m);}
uint32_t bits(float f){return std::bit_cast<uint32_t>(f);}
uint32_t word(std::istream&s){std::string t;require(bool(s>>t)&&t.size()==8,"invalid captured word");size_t n;auto v=std::stoul(t,&n,16);require(n==8&&v<=0xfffffffful,"captured word overflow");return uint32_t(v);}
float real(std::istream&s){return std::bit_cast<float>(word(s));}
VertexProgram program(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);require(bool(f),"source program missing");std::string b((std::istreambuf_iterator<char>(f)),{});return decode_vertex_program({reinterpret_cast<const std::byte*>(b.data()),b.size()});}
struct Bank{unsigned classification{},section{},part{};uint32_t mode{},shader{};ShaderVector conversion{};std::array<ShaderVector,192> constants{};unsigned count{};};
struct Sample{unsigned event{},vertex{};std::array<ShaderVector,16> raw{};ShaderVector world{},uv{};Outputs expected{};};
Outputs converted(const VertexProgram&p,VertexProgramState&s,const ShaderVector&conversion){
 execute_vertex_program(p,s);Outputs out;
 for(unsigned o=0;o<7;++o)out[o]=s.outputs[output_ids[o]];
 for(unsigned a=0;a<2;++a)out[0][a]=((std::trunc(out[0][a]*16.f)/16.f)*2.f-conversion[a])/conversion[a];
 out[0][2]/=conversion[2];for(unsigned a=0;a<3;++a)out[0][a]*=out[0][3];return out;
}
void exact(const Outputs&a,const Outputs&b,const std::string&where){for(unsigned o=0;o<7;++o)for(unsigned k=0;k<4;++k)
 require(bits(a[o][k])==bits(b[o][k]),where+" output"+std::to_string(output_ids[o])+" axis"+std::to_string(k)+" differs");}
SourceShadowPlanBindings bindings(const Bank&bank,const SourceShadowMaterial&material){
 std::array<Vertex,3> dummy;for(auto&v:dummy)v.material=0;
 auto draw=source_shadow_plan_draw(0,material,dummy,{0,true});
 std::copy_n(bank.constants.begin()+18,16,draw.constants[1].begin());
 SourceShadowPlanRoot root;std::copy_n(bank.constants.begin()+31,6,root.uniforms.projection.begin());
 std::copy_n(bank.constants.begin()+40,9,root.uniforms.constants.begin());root.casters={&draw,1};
 SourceShadowPlan plan;std::copy_n(bank.constants.begin(),12,plan.camera.begin());plan.roots={&root,1};
 SourceShadowPlanMaterial resident{0,&material};
 validate_source_shadow_plan_material(material,SourceShadowPhase::Generate);
 return source_shadow_plan_bindings(plan,root,draw,resident);
}
VertexProgramState state(const SourceShadowPlanBindings&binding,ShaderVector world,ShaderVector uv){
 VertexProgramState s;const auto packed=pack_vertex_program_bank(source_shadow_plan_vertex_bank(binding));
 std::copy_n(packed.constants.begin(),49,s.constants.begin());s.constants[172]={1,1,0,0};
 for(auto&a:s.attributes)a={0,0,0,1};s.attributes[0]={world[0],world[1],world[2],1};s.attributes[1]={uv[0],uv[1],0,1};return s;
}
}
int main(){try{
 const auto root=std::filesystem::path(HALO2_SOURCE_DIR);const auto cachefile=root/"assets/disc/maps/zanzibar.map";
 if(!std::filesystem::exists(cachefile))return 77;
 std::ifstream input(root/"tests/fixtures/retail_shadow_caster_world.tsv");require(bool(input),"caster world fixture missing");
 std::map<unsigned,Bank> banks;std::vector<Sample> samples;std::map<unsigned,Outputs> full;
 std::vector<Transform> world;std::string line;
 while(std::getline(input,line)){
  if(line.empty()||line[0]=='#')continue;std::istringstream row(line);char kind;row>>kind;
  if(kind=='N'){
   const auto mode=word(row);unsigned count;row>>count;require(bool(row)&&mode==0xe9b90844&&count==33,"literal WORLD identity changed");
   world.resize(count);for(auto&t:world){std::array<float,13> w;for(auto&f:w)f=real(row);t=std::bit_cast<Transform>(w);}
  }else{
   unsigned event,index;row>>event>>index;require(bool(row),"captured event/index missing");
   if(kind=='B'){
    auto&b=banks[event];b.classification=index;b.mode=word(row);row>>b.section>>b.part;b.shader=word(row);
    for(unsigned a=0;a<3;++a)b.conversion[a]=real(row);
   }else if(kind=='C'){
    auto&b=banks.at(event);require(index==b.count&&index<192,"captured constant bank incomplete");
    for(auto&v:b.constants[index])v=real(row);++b.count;
   }else if(kind=='V'){
    Sample s;s.event=event;s.vertex=index;require(banks.at(event).count==192,"captured bank missing");
    for(auto&a:s.raw)for(auto&v:a)v=real(row);for(auto&v:s.world)v=real(row);for(auto&v:s.uv)v=real(row);
    for(auto&o:s.expected)for(auto&v:o)v=real(row);samples.push_back(s);
   }else if(kind=='S'){
    require(event==7737&&index==full.size(),"complete actual section19 output order changed");
    for(auto&o:full[index])for(auto&v:o)v=real(row);
   }else throw FormatError("unknown caster fixture record");
  }
  require(bool(row),"captured row truncated");std::string extra;require(!(row>>extra),"captured row has trailing fields");
 }
 require(banks.size()==7&&samples.size()==14&&full.size()==672&&world.size()==33,"caster coverage changed");
 Cache cache(cachefile);std::map<uint32_t,SourceShadowMaterial> materials;
 const auto class0=program(root/"tests/fixtures/retail_shadow_generate_world_class0.vp");
 for(auto&[event,b]:banks){
  const auto part=cache.load_source_transparent_model_parts(b.mode,{&b.section,1});
  require(part.sections.size()==1&&part.sections[0].parts.at(b.part).shader==b.shader,"native literal caster part identity changed");
  if(!materials.contains(b.shader)){
   auto p=prepare_shadow_material(cache,cache.load_material(b.shader),SourceShadowPhase::Generate,true,false,1.f);
   require(p.size()==1,"source caster pass cardinality changed");
   auto prepared=decode_vertex_program(p[0].material.vertex_program);
   require(prepared.count==class0.count&&prepared.instructions==class0.instructions&&p[0].material.vertex_declaration&&
    p[0].material.vertex_declaration->semantics==std::vector<uint16_t>{0,3},"prepared caster is not literal decoded WORLD class0");
   materials.emplace(b.shader,std::move(p[0]));
  }
 }
 unsigned exact_words=0,wrong_local=0,wrong_uv=0,wrong_decompression=0;
 for(const auto&s:samples){
  const auto&b=banks.at(s.event);const auto bound=bindings(b,materials.at(b.shader));
  VertexProgramState raw;raw.constants=b.constants;raw.attributes=s.raw;
  const auto local=program(root/"tests/fixtures"/("retail_shadow_generate_class"+std::to_string(b.classification)+".vp"));
  exact(converted(local,raw,b.conversion),s.expected,"original local program event"+std::to_string(s.event));
  auto posed=state(bound,s.world,s.uv);exact(converted(class0,posed,b.conversion),s.expected,"class0 world event"+std::to_string(s.event));
  exact_words+=28;
  auto bad=state(bound,s.raw[0],s.uv);wrong_local+=converted(class0,bad,b.conversion)!=s.expected;
  const unsigned uv_attribute=b.classification==1?1:b.classification==2?2:3;
  bad=state(bound,s.world,s.raw[uv_attribute]);wrong_uv+=converted(class0,bad,b.conversion)!=s.expected;
  bad=state(bound,s.world,s.uv);bad.constants[172]=b.constants[172];wrong_decompression+=converted(class0,bad,b.conversion)!=s.expected;
 }
 require(exact_words==392&&wrong_local==14&&wrong_uv==14&&wrong_decompression==14,"caster negative controls did not exercise actual source dependencies");
 // The later paused WORLD bank is accepted only for section19: all nine
 // literal node-map matrices are first compared against this actual draw.
 const auto&b=banks.at(7737);const auto selected=cache.load_source_transparent_model_parts(b.mode,{&b.section,1}).sections.at(0);
 const auto palette=source_model_skin_palette(world,cache.load_source_model_palette_definition(b.mode).inverse_binds);
 require(selected.resident_node_map==std::vector<uint8_t>{0,1,2,4,5,7,8,12,13},"actual section19 compact node map changed");
 unsigned palette_words=0;
 for(unsigned i=0;i<selected.resident_node_map.size();++i)for(unsigned k=0;k<12;++k){
  require(bits(palette.at(selected.resident_node_map[i])[k])==bits(b.constants[50+i*3+k/4][k%4]),"postinterval WORLD bank does not match actual section19 palette");++palette_words;
 }
 auto definition=cache.load_initial_model_definition(0xedf20c6c);
 require(definition.render_model==b.mode,"native owner model does not match actual raw draw identity");
 auto selection=source_deterministic_initial_model_selection(definition,0);require(selection.resolved(),"controlled source model variant unresolved");
 auto model=cache.load_model(0xedf20c6c,false,0,selection,5);require(model.bones.size()==world.size(),"native actual skin node count changed");
 const auto posed=skin_model_world_nodes(model,world);std::map<unsigned,Vertex> vertices;
 for(unsigned i=0;i<model.vertex_origins.size();++i){const auto origin=model.vertex_origins[i];if(origin.section==19&&origin.part==0){
  auto[v,inserted]=vertices.emplace(origin.vertex,posed[i]);require(inserted||std::bit_cast<std::array<uint32_t,3>>(v->second.position)==std::bit_cast<std::array<uint32_t,3>>(posed[i].position),"expanded native pose diverged on one source vertex");
 }}
 require(vertices.size()==672,"native actual part omits a retained source vertex");
 const auto bound=bindings(b,materials.at(b.shader));unsigned native_debug_words=0;
 for(const auto&s:samples)if(s.event==7737){const auto&v=vertices.at(s.vertex);
  require(std::bit_cast<std::array<uint32_t,3>>(v.position)==std::array<uint32_t,3>{bits(s.world[0]),bits(s.world[1]),bits(s.world[2])},"native pose differs from independent captured R0");
  require(bits(v.uv.x)==bits(s.uv[0])&&bits(v.uv.y)==bits(s.uv[1]),"native UV differs from independent original decompression");
  auto st=state(bound,{v.position.x,v.position.y,v.position.z,1},{v.uv.x,v.uv.y,0,1});exact(converted(class0,st,b.conversion),s.expected,"actual native section19 DebugVertex");native_debug_words+=28;
 }
 // Complete packed capture inputs are frozen independently of native model
 // decoding. Run the actual class4 prefix/suffix as a second arithmetic path.
 std::ifstream silhouette(root/"tests/fixtures/retail_shadow_silhouette.bin",std::ios::binary);require(bool(silhouette),"actual full caster input fixture missing");
 const auto u32=[&]{uint32_t value;require(bool(silhouette.read(reinterpret_cast<char*>(&value),4)),"full caster input truncated");return value;};
 const auto skip=[&](size_t n){silhouette.seekg(std::streamoff(n),std::ios::cur);require(bool(silhouette),"full caster skip failed");};
 require(u32()==0x47533248u&&u32()==1,"full caster fixture identity changed");const unsigned draw_count=u32(),texture_count=u32();
 require(draw_count==7&&texture_count==2,"full actual caster coverage changed");
 for(unsigned i=0;i<texture_count;++i){u32();u32();const auto levels=u32();for(unsigned m=0;m<levels;++m){const auto n=u32();skip(n);}}
 std::vector<std::array<ShaderVector,16>> raw_vertices;
 for(unsigned d=0;d<draw_count;++d){const auto event=u32(),indices=u32(),count=u32();u32();const auto cls=u32();
  std::array<ShaderVector,16> inline_values;std::array<ShaderVector,192> constants;ShaderVector conversion;
  require(bool(silhouette.read(reinterpret_cast<char*>(inline_values.data()),sizeof(inline_values)))&&
   bool(silhouette.read(reinterpret_cast<char*>(constants.data()),sizeof(constants)))&&
   bool(silhouette.read(reinterpret_cast<char*>(conversion.data()),sizeof(conversion))),"full actual caster bank truncated");
  if(event==7737){require(cls==4&&count==672&&constants==b.constants&&conversion==b.conversion,"full source bank is a different draw");
   raw_vertices.resize(count,inline_values);for(auto&attributes:raw_vertices)
    require(bool(silhouette.read(reinterpret_cast<char*>(attributes.data()),4*sizeof(ShaderVector))),"full raw source vertex truncated");
  }else skip(size_t(count)*4*sizeof(ShaderVector));skip(size_t(indices)*4);
 }
 require(raw_vertices.size()==672,"full actual packed source part absent");
 const auto original_class4=program(root/"tests/fixtures/retail_shadow_generate_class4.vp");
 unsigned complete_arithmetic_words=0;
 for(const auto&[index,v]:vertices){VertexProgramState raw;raw.constants=b.constants;raw.attributes=raw_vertices.at(index);
  auto native=state(bound,{v.position.x,v.position.y,v.position.z,1},{v.uv.x,v.uv.y,0,1});
  exact(converted(class0,native,b.conversion),converted(original_class4,raw,b.conversion),"complete native/raw source shader seam vertex"+std::to_string(index));complete_arithmetic_words+=28;
 }
 float max_pixel=0,max_depth=0;unsigned different_position=0,max_ulp=0,nonposition_words=0;
 for(const auto&[index,v]:vertices){auto st=state(bound,{v.position.x,v.position.y,v.position.z,1},{v.uv.x,v.uv.y,0,1});const auto out=converted(class0,st,b.conversion);const auto&expected=full.at(index);
  for(unsigned o=1;o<7;++o)for(unsigned a=0;a<4;++a){require(bits(out[o][a])==bits(expected[o][a]),"native class0 actual GPU nonposition output differs");++nonposition_words;}
  for(unsigned a=0;a<4;++a){const auto av=bits(out[0][a]),ev=bits(expected[0][a]);if(av!=ev)++different_position;
   if((av>>31)==(ev>>31))max_ulp=std::max(max_ulp,av>ev?av-ev:ev-av);
   if(a<2)max_pixel=std::max(max_pixel,std::abs(out[0][a]/out[0][3]-expected[0][a]/expected[0][3])*b.conversion[a]*.5f);
   if(a==2)max_depth=std::max(max_depth,std::abs(out[0][a]-expected[0][a]));
  }
 }
 // Actual GPU OpDot versus ordered CPU arithmetic is measured separately;
 // the two captured DebugVertex outputs above remain strict word assertions.
 std::cout<<"392 captured generation words exact across seven class0/world seams; 42 wrong-input controls responded\n";
 std::cout<<palette_words<<" actual compact-palette words and "<<native_debug_words<<" actual native DebugVertex outputs exact\n";
 std::cout<<complete_arithmetic_words<<" full native/raw original shader arithmetic words exact\n";
 std::cout<<"672 real posed vertices / "<<nonposition_words<<" actual GPU nonposition words exact; position differences "<<different_position<<", max ULP "<<max_ulp<<", max pixel "<<max_pixel<<", max depth "<<max_depth<<'\n';
 require(max_pixel<=.06251f&&max_depth<=.000008f,"actual source GPU precision residual exceeded measured source boundary");
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
