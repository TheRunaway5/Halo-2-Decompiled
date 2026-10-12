#include "halo2/frontend_model_pose.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <map>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void require(bool p,const char*m){if(!p)throw FormatError(m);}
std::uint32_t word(std::istream&s){std::string x;require(bool(s>>x),"incomplete original IK fixture");return std::uint32_t(std::stoul(x,nullptr,16));}
std::vector<Transform> bank(std::istream&s,unsigned n){std::vector<Transform> v(n);for(auto&t:v){std::array<std::uint32_t,13>w;for(auto&f:w)f=word(s);t=std::bit_cast<Transform>(w);}return v;}
FirstPersonAimBlend blend(std::string s){std::array<std::uint8_t,4>b;for(unsigned i=0;i<4;++i)b[i]=std::uint8_t(std::stoul(s.substr(i*2,2),nullptr,16));return std::bit_cast<FirstPersonAimBlend>(b);}
}
int main(int argc,char**argv){try {
 require(argc==3,"IK test needs fixture directory and maps");
 const std::filesystem::path dir=argv[1],maps=argv[2];
 if(!std::filesystem::exists(dir/"model-deferred-world-original.tsv") || !std::filesystem::exists(dir/"model-held-ik-original.tsv") || !std::filesystem::exists(dir/"model-aim-initialization-original.tsv") || !std::filesystem::exists(dir/"model-attachment-initialization-original.tsv") || !std::filesystem::exists(maps/"mainmenu.map"))return 77;
 Cache cache(maps/"mainmenu.map");std::ifstream input(dir/"model-held-ik-original.tsv");unsigned cases=0,errors=0;float max_error=0;
 for(std::string line;std::getline(input,line);){if(line.empty()||line[0]=='#')continue;std::istringstream s(line);const auto unit=word(s),mode=word(s),weight=word(s);unsigned n,m;require(bool(s>>n>>m),"incomplete original bank counts");auto body=bank(s,n);const auto weapon=bank(s,m),expected=bank(s,n);const auto model=cache.load_model(unit,false);const auto ik=cache.load_frontend_model_weapon_ik(unit,mode);apply_frontend_model_weapon_ik(ik,model,body,weapon,std::bit_cast<float>(weight));
  for(unsigned i=0;i<n;++i){const auto a=std::bit_cast<std::array<std::uint32_t,13>>(body[i]),b=std::bit_cast<std::array<std::uint32_t,13>>(expected[i]);for(unsigned j=0;j<13;++j)if(a[j]!=b[j]){++errors;max_error=std::max(max_error,std::abs(std::bit_cast<float>(a[j])-std::bit_cast<float>(b[j])));}}
  ++cases;
 }
 require(cases==60,"original menu IK fixture cases missing");
 std::cout<<cases<<" original IK cases; "<<errors<<" differing words, max="<<max_error<<'\n';
 std::ifstream aims(dir/"model-aim-initialization-original.tsv");std::map<std::uint32_t,std::array<FirstPersonAimBlend,2>>states;unsigned ticks=0,advances=0,selections=0;std::map<std::uint32_t,bool>pending;
 for(std::string line;std::getline(aims,line);){if(line.empty()||line[0]=='#')continue;std::istringstream s(line);const auto unit=word(s);std::string kind,a,b;unsigned tick;require(bool(s>>kind>>tick>>a>>b),"incomplete original aim fixture");const auto expected0=blend(a),expected1=blend(b);auto&state=states[unit];
  if(kind=="initialize"){state={};continue;}
  if(kind=="tick-before"){
   require(std::bit_cast<std::uint32_t>(state[0])==std::bit_cast<std::uint32_t>(expected0)&&std::bit_cast<std::uint32_t>(state[1])==std::bit_cast<std::uint32_t>(expected1),"native before-tick aim state differs from original");
   pending[unit]=true;++ticks;continue;
  }
  if(kind=="world-before" && pending[unit]){source_aim_blend_advance(state[0]);source_aim_blend_advance(state[1]);pending[unit]=false;++advances;}
  if(kind=="world-after"){source_aim_blend_select(state[0],true,30);source_aim_blend_select(state[1],false,30);++selections;}
  require(std::bit_cast<std::uint32_t>(state[0])==std::bit_cast<std::uint32_t>(expected0)&&std::bit_cast<std::uint32_t>(state[1])==std::bit_cast<std::uint32_t>(expected1),"native world aim selection differs from original");
 }
 require(ticks==19 && advances==16 && selections==18,"original aim initialization trace incomplete");
 std::cout<<ticks<<" source tick entries / "<<advances<<" captured advances preserve aim slot words\n";
 std::ifstream deferred(dir/"model-deferred-world-original.tsv");unsigned hierarchy_cases=0,hierarchy_errors=0;
 for(std::string line;std::getline(deferred,line);){if(line.empty()||line[0]=='#')continue;std::istringstream s(line);const auto unit=word(s);unsigned n;require(bool(s>>n),"missing source hierarchy count");std::array<std::uint32_t,8>mask;for(auto&w:mask)w=word(s);auto world=bank(s,n);std::vector<BonePose>locals(n);
  for(auto&pose:locals){std::array<float,8>f;for(auto&x:f)x=std::bit_cast<float>(word(s));pose.rotation={f[0],f[1],f[2],f[3]};pose.translation={f[4],f[5],f[6]};pose.scale=f[7];}
  const auto expected=bank(s,n);const auto model=cache.load_model(unit,false);
  frontend_model_deferred_world(model,world,locals,mask);
  for(unsigned i=0;i<n;++i){const auto a=std::bit_cast<std::array<std::uint32_t,13>>(world[i]),b=std::bit_cast<std::array<std::uint32_t,13>>(expected[i]);for(unsigned j=0;j<13;++j)hierarchy_errors+=a[j]!=b[j];}
  ++hierarchy_cases;
 }
 require(hierarchy_cases==2,"source visible hierarchy fixtures missing");
 std::cout<<hierarchy_cases<<" visible source hierarchy cases; "<<hierarchy_errors<<" differing words\n";
 require(hierarchy_errors==0,"native deferred hierarchy differs from original draw producer");

 const auto level=cache.load_content(0);std::ifstream attachments(dir/"model-attachment-initialization-original.tsv");unsigned attachment_cases=0,hand_errors=0,local_errors=0;
 for(std::string line;std::getline(attachments,line);){if(line.empty()||line[0]=='#')continue;std::istringstream s(line);std::string name;require(bool(s>>name),"original startup model name missing");const auto unit=word(s),weapon=word(s);unsigned index,node;require(bool(s>>index>>node),"original startup attachment identifiers missing");const auto expected_hand=bank(s,1).front();std::array<std::uint32_t,9>expected_local;for(auto&w:expected_local)w=word(s);
  const ScenarioObject*object=nullptr;for(const auto&candidate:level.objects)if(candidate.name>=0 &&level.object_names.at(candidate.name).name==name){require(!object,"duplicate original startup scenario name");object=&candidate;}require(object &&object->definition==unit,"original startup scenario identity differs");
  const auto rotation=authored_basis(object->rotation);const auto root=source_object_transform(object->position,rotation.forward,rotation.up,object->scale,false);
  const auto model=cache.load_model(unit,false);const auto startup=cache.load_initialized_object_idle(unit);require(startup &&startup->source_index==index,"original pre-equipment startup animation differs");const auto primary=cache.load_frontend_model_primary_animation(startup->source_graph,startup->source_index);
  const auto locals=frontend_model_locals(model,*startup,0,{},&primary);const auto body=pose_model_locals_nodes_world(model,root,locals);require(node<body.size(),"original startup hand node outside model");const auto actual_hand=std::bit_cast<std::array<std::uint32_t,13>>(body[node]),hand=std::bit_cast<std::array<std::uint32_t,13>>(expected_hand);for(unsigned j=0;j<13;++j)hand_errors+=actual_hand[j]!=hand[j];
  const auto markers=select_weapon_attachment(cache.load_weapon_attachment(unit,weapon,0),{},{});require(markers.parent.node==node,"original startup marker node differs");const auto retained=frontend_model_initialize_weapon_attachment(model,root,*startup,primary,markers);const auto actual_local=std::bit_cast<std::array<std::uint32_t,9>>(retained);for(unsigned j=0;j<9;++j)local_errors+=actual_local[j]!=expected_local[j];++attachment_cases;
 }
 require(attachment_cases==10,"all original startup attachment cases required");
 std::cout<<attachment_cases*13<<" startup hand words / "<<attachment_cases*9<<" retained local words; "<<hand_errors<<" / "<<local_errors<<" differences\n";
 require(hand_errors==0 &&local_errors==0,"native attachment initialization differs from original constructor");

 return errors?2:0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
