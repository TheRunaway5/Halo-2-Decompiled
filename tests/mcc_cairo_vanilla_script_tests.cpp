#include "mcc_cairo_raw_fixture.hpp"
#include "halo2/mcc_script_layout.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_campaign_runtime_tu15.hpp"
#include "halo2/source_startup_sound_storage_tu15.hpp"
#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <nlohmann/json.hpp>
namespace {
using namespace halo2;
using Json=nlohmann::json;
void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
void put(std::span<std::byte>b,std::size_t o,std::uint32_t v,unsigned size=4){for(unsigned i=0;i<size;++i)b[o+i]=std::byte(v>>(8*i));}
void put(SourceRuntimeMemory&m,unsigned a,unsigned v,unsigned size=4){std::array<std::byte,4>b{};put(b,0,v,size);m.write(a,std::span(b).first(size));}
unsigned get(const SourceRuntimeMemory&m,unsigned a){return Bytes(m.view(a,4)).u32(0);}
struct Scenario {
 Cache cache;std::span<const std::byte>nodes,strings,scripts;std::vector<std::byte>root;std::array<std::vector<std::byte>,3>raw_fixture;
 explicit Scenario(const char*path):cache(path){const auto*t=cache.find_tag(cache.scenario_tag());auto r=cache.read_tag_bytes(t->address,0x240);root.assign(r.begin(),r.end());nodes=bank(0x238,20);strings=bank(0x1b0,1);scripts=bank(0x1b8,40);}
 void restore_raw_fixture(const std::filesystem::path&dir){raw_fixture[0]=halo2_test::read_cairo_fixture(dir/"block_568.bin");raw_fixture[1]=halo2_test::read_cairo_fixture(dir/"block_432.bin");raw_fixture[2]=halo2_test::read_cairo_fixture(dir/"block_440.bin");root=halo2_test::read_cairo_fixture(dir/"scenario.bin");nodes=raw_fixture[0];strings=raw_fixture[1];scripts=raw_fixture[2];}
 std::span<const std::byte>bank(unsigned at,unsigned stride)const{const Bytes r(root);return r.u32(at)?cache.read_tag_bytes(r.u32(at+4),std::size_t(r.u32(at))*stride):std::span<const std::byte>{};}
 std::string name(unsigned at)const{if(at>=strings.size())return {};return Bytes(strings).string(at,strings.size()-at);}
 unsigned script(std::string_view name)const{for(unsigned at=0;at<scripts.size();at+=40)if(Bytes(scripts.subspan(at,40)).string(0,32)==name)return Bytes(scripts.subspan(at,40)).u32(36);throw FormatError("missing script");}
 Json normalized(std::span<const std::byte>data,unsigned datum,unsigned depth=0)const{
  require(depth<128,"Cairo AST recursion differs");const Bytes n(data.subspan((datum&65535u)*20,20));require(n.u16(0)==datum>>16,"Cairo AST full datum differs");const auto flags=n.u16(6),op=n.u16(2),type=n.u16(4);auto value=n.u32(16);
  if(!(flags&1)){const Bytes fn(data.subspan((value&65535u)*20,20));Json args=Json::array();std::set<unsigned>seen;for(auto next=fn.u32(8);next!=0xffffffffu;next=Bytes(data.subspan((next&65535u)*20,20)).u32(8)){require(seen.insert(next).second,"AST sibling cycle");args.push_back(normalized(data,next,depth+1));}return {{"fn",name(fn.u32(12))},{"static",bool(flags&2)},{"flags",flags},{"type",type},{"args",args}};}
  const auto text=name(n.u32(12));if(flags&4)return {{"global",text},{"type",type},{"flags",flags}};
  Json result={{"literal_type",op},{"context",type},{"flags",flags}};
  if(op==5)value&=255;else if(op==7||op==10||(op>=44&&op<=49))value&=65535;
  if(op==5||op==6||op==7||op==8){result["value"]=value;return result;}
  if(op==32||op==33||op==36){const auto*t=cache.find_tag(TagHandle{value});require(t,"AST tag identity missing");result["binding"]={{"group",t->group},{"path",t->name}};}
  else if(op==11){const auto id=cache.string_id(value);require(id==text,"AST string ID mismatch");result["binding"]={{"string_id",id}};}
  else if(op>=56&&op<=61){const auto names=bank(0x48,0x24);const Bytes b(names.subspan((value&65535u)*0x24,0x24));require(b.string(0,32)==text,"AST object identity mismatch");result["binding"]={{"object_name",text},{"object_type",b.u16(32)}};}
  else if(op==14){const Bytes b(bank(0x1e0,0x38).subspan((value&65535u)*0x38,0x38));require(b.string(4,32)==text,"AST flag identity mismatch");result["binding"]={{"cutscene_flag",text}};}
  else if(!text.empty())result["identity"]=text;else result["value"]=value;
  return result;
 }
};
std::map<std::string,unsigned>needs(const MccScriptLayout&r){std::map<std::string,unsigned>n;for(const auto&i:r.requirements)++n[i.name];return n;}
void original_free_rows(std::span<const std::byte>nodes){
 constexpr unsigned pool=0x90000000,rows=0x91000000,bits=0x92000000,stack=0x60008000;
 for(unsigned debug:{0u,8u}){
  SourceRuntimeMemory m;const auto count=unsigned(nodes.size()/20);for(const auto[a,n]:{std::pair(pool,0x100u),std::pair(rows,unsigned(nodes.size())),std::pair(bits,(count+31)/32*4),std::pair(0x60000000u,0x10000u)})m.mount(a,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,"explicit original node-pool fixture");
  m.write(rows,nodes);put(m,pool+0x24,20);put(m,pool+0x2a,debug,1);put(m,pool+0x48,bits);put(m,stack,0xfeedcafe);
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=count;abi.gp[1]=rows;abi.gp[6]=pool;abi.gp[4]=stack;abi.eflags=0x202;
  SourcePhysicsWorldRuntimeServices services;std::set<unsigned>seen;services.instruction=[&](unsigned pc){seen.insert(pc);};
  services.required_owner=[&](auto&bank,const auto&call,auto&state,const auto&observe){require(call.target==0x32a4e0,"Unverified node-pool owner");source_run_startup_sound_storage_tu15(bank,call.target,state,{0x37f,0x1f80},services,observe);};
  source_run_campaign_runtime_tu15(m,0x16e510,abi,{0x37f,0x1f80},services);
  unsigned active=0;for(unsigned i=0;i<count;++i){const bool occupied=Bytes(nodes.subspan(i*20,20)).u16(0)!=0;active+=occupied;require(bool(get(m,bits+(i/32)*4)&(1u<<(i%32)))==occupied,"Original bitmap did not recognize retired rows");if(!occupied&&debug){const Bytes free(m.view(rows+i*20,20));require(free.u16(0)==0,"Original poison lost zero salt");for(unsigned b=2;b<20;++b)require(m.view(rows+i*20+b,1)[0]==std::byte{0xba},"Retired row differs from original debug poison");}}
  require(get(m,pool+0x3c)==active&&abi.gp[4]==stack+4&&seen.contains(0x32a4e0)&&seen.contains(0x16e5a0),"Original node census, allocator or return did not run");
 }
}
}
int main(int argc,char**argv)try{
 if(argc==1 || (argc>=3 && (!*argv[1] || !*argv[2])))return 77;
 require(argc==3 || argc==4,"Pass MCC Cairo, Xbox Cairo and optional raw donor fixture directory");Scenario mcc(argv[1]),xbox(argv[2]);if(Bytes(mcc.root).u32(0x1b8)==419 || Bytes(mcc.root).u32(0x1b8)==418){const auto dir=argc==4?std::filesystem::path(argv[3]):std::filesystem::path(__FILE__).parent_path().parent_path()/".local/mcc-script-signatures-20261016/v4/raw-donor";if(!std::filesystem::exists(dir/"block_568.bin")){std::cout<<"Raw donor snapshot required after automatic Cairo conversion\n";return 77;}mcc.restore_raw_fixture(dir);}const auto donor=std::vector<std::byte>(mcc.nodes.begin(),mcc.nodes.end());
 const auto plain=project_mcc_script_layout(mcc.nodes,mcc.strings,mcc.scripts,{true});
 const auto converted=project_mcc_script_layout(mcc.cache,mcc.nodes,mcc.strings,mcc.scripts,{true});
 const auto pn=needs(plain),cn=needs(converted);require(pn.at("cinematic_start_movie")==2&&!cn.contains("cinematic_start_movie")&&!cn.contains("enable_render_light_suppressor")&&!cn.contains("disable_render_light_suppressor")&&plain.requirements.size()==converted.requirements.size()+4,"Exact Cairo authoring delta did not remove four proven requirements");
 const std::set<unsigned>retired{0xea2b06b8,0xea2c06b9,0xea2d06ba,0x81c71e53,0x81c81e54,0x81c91e55,0xf7ca1457,0xf7cb1458,0xf84f14dc,0xf85014dd};
 const std::set<unsigned>predecessors{3928491701u,2177113680u,4157084757u,4165735641u};
 unsigned changed=0;for(unsigned i=0;i<mcc.nodes.size()/20;++i){const Bytes raw(mcc.nodes.subspan(i*20,20));const unsigned datum=(unsigned(raw.u16(0))<<16)|i;for(unsigned b=0;b<20;++b){const auto a=i*20+b;if(plain.nodes[a]!=converted.nodes[a]){++changed;require(retired.contains(datum)||(predecessors.contains(datum)&&b>=8&&b<12),"Cairo delta escaped declared links/retired rows");}}if(retired.contains(datum)){const Bytes free(std::span(converted.nodes).subspan(i*20,20));require(!free.u16(0),"Retired call retained active salt");for(unsigned b=2;b<20;++b)require(converted.nodes[i*20+b]==std::byte{0xba},"Retired call differs from original free row");}}
 Json proofs=Json::array();for(const char*name:{"x02_scene_01","c01_outro_scene_01","x02_scene_06"}){const auto actual=mcc.normalized(converted.nodes,mcc.script(name)),reference=xbox.normalized(xbox.nodes,xbox.script(name));require(actual==reference,"Converted full control-flow subtree differs from original Xbox");proofs.push_back({{"script",name},{"full_normalized_ast_equal",true},{"children",actual["args"].size()}});}
 // A byte-preserving custom variant is deliberately outside this profile.
 auto modified=donor;modified[(0x6b1u)*20+12]^=std::byte{1};auto custom=project_mcc_script_layout(mcc.cache,modified,mcc.strings,mcc.scripts,{true});require(needs(custom).at("cinematic_start_movie")==1,"Custom numeric/compiler row was normalized as retail");
 auto custom_strings=std::vector<std::byte>(mcc.strings.begin(),mcc.strings.end());custom_strings[0xd22]^=std::byte{1};custom=project_mcc_script_layout(mcc.cache,donor,custom_strings,mcc.scripts,{true});require(needs(custom).at("cinematic_start_movie")==1,"Custom movie string was normalized as retail");
 auto scripts=std::vector<std::byte>(mcc.scripts.begin(),mcc.scripts.end());for(unsigned at=0;at<scripts.size();at+=40)if(Bytes(scripts).string(at,32)=="x02_scene_01")put(scripts,at+32,1,2);custom=project_mcc_script_layout(mcc.cache,donor,mcc.strings,scripts,{true});require(needs(custom).at("cinematic_start_movie")==1,"Custom script type was normalized as retail");
 modified=donor;unsigned free_index=0;while(Bytes(modified).u16(free_index*20))++free_index;const unsigned at=free_index*20;std::fill(modified.begin()+at,modified.begin()+at+20,std::byte{});put(modified,at,0x9345,2);put(modified,at+2,6,2);put(modified,at+4,6,2);put(modified,at+6,9,2);put(modified,at+8,0xea2b06b8);custom=project_mcc_script_layout(mcc.cache,modified,mcc.strings,mcc.scripts,{true});require(needs(custom).at("cinematic_start_movie")==1,"External incoming sibling was retired");
 scripts.assign(mcc.scripts.begin(),mcc.scripts.end());scripts.resize(scripts.size()+40);const auto start=scripts.size()-40;const std::string label="custom_shared_movie";std::copy(std::as_bytes(std::span(label)).begin(),std::as_bytes(std::span(label)).end(),scripts.begin()+start);put(scripts,start+32,3,2);put(scripts,start+36,0xea2b06b8);custom=project_mcc_script_layout(mcc.cache,donor,mcc.strings,scripts,{true});require(needs(custom).at("cinematic_start_movie")==1,"External script root was retired");
 bool rejected=false;try{project_mcc_script_layout(xbox.cache,xbox.nodes,xbox.strings,xbox.scripts,{true});}catch(const FormatError&){rejected=true;}require(rejected,"Xbox data was treated as an MCC donor");
 require(std::ranges::equal(donor,mcc.nodes),"Borrowed cache AST changed");original_free_rows(converted.nodes);
 std::cout<<Json({{"retail_ast_proofs",proofs},{"requirements_before",plain.requirements.size()},{"requirements_after",converted.requirements.size()},{"remaining",cn},{"changed_bytes_vs_opcode_projection",changed},{"retired_rows",retired.size()},{"custom_guards",5},{"original_node_pool_modes",2},{"full_gameplay_execution_claimed",false}}).dump(2)<<'\n';
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
