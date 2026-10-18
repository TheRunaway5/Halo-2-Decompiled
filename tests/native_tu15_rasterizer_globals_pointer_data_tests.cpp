#include "halo2/native_tu15_renderer_startup_data.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_campaign_activation_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
unsigned checks{},negatives{};
void check(bool yes,const char*why){++checks;if(!yes)throw std::runtime_error(why);}
std::string hex(std::span<const std::byte>b){std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+="0123456789abcdef"[n>>4];s+="0123456789abcdef"[n&15];}return s;}
void word(SourceRuntimeMemory&m,unsigned p,unsigned v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);}
struct PrefixStop{};
}
int main(int argc,char**argv)try{
 auto memory=native_tu15_world_memory({0x60212000u,{},"isolated same factory clock"});const auto cold=memory.checkpoint();
 if(argc==2&&std::string_view(argv[1])=="--dump"){nlohmann::json j=nlohmann::json::array();for(const auto&r:cold.regions){std::vector<unsigned>b;for(auto v:r.bytes)b.push_back(std::to_integer<unsigned>(v));j.push_back({{"address",r.address},{"bytes",b},{"owner",r.owner},{"access",int(r.access)},{"page_bytes",r.page_bytes},{"committed",r.committed}});}std::cout<<j.dump()<<'\n';return 0;}
 const auto owner=std::ranges::find(cold.regions,0x4922a0u,&SourceRuntimeMemory::CheckpointRegion::address);
 check(owner!=cold.regions.end()&&owner->bytes.size()==4&&owner->access==SourceRuntimeMemory::Access::ReadWrite&&!owner->page_bytes&&owner->committed.empty()&&Bytes(owner->bytes).u32(0)==0,"rasterizer pointer lacks exact loader-zero DWORD");
 check(memory.native_address(0x4922a0u,4).value>UINT32_MAX,"rasterizer pointer backing narrowed");
 for(auto[p,n]:std::array<std::pair<unsigned,unsigned>,3>{{{0x49229fu,1},{0x4922a4u,1},{0x4922a0u,5}}}){bool no{};try{memory.view(p,n);}catch(const std::out_of_range&){no=true;}check(no,"rasterizer pointer borrowed a neighbor");++negatives;}
 for(auto[p,n]:std::array<std::pair<unsigned,unsigned>,3>{{{0x49229fu,2},{0x4922a0u,4},{0x4922a3u,2}}}){SourceRuntimeMemory foreign;foreign.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,"foreign rasterizer pointer overlap");const auto before=foreign.checkpoint();bool no{};try{mount_native_tu15_renderer_startup_data(foreign);}catch(const std::invalid_argument&){no=true;}check(no&&foreign.checkpoint()==before,"rasterizer overlap caused partial mounting");++negatives;}
 std::optional<nlohmann::json>oracle;
 if(argc==2){if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;std::ifstream in(argv[1]);oracle=nlohmann::json::parse(in);check(oracle->at("format")=="native_tu15_rasterizer_globals_pointer_prefix_v1"&&oracle->at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&oracle->at("instruction_patches").empty()&&oracle->at("supplied_callee_returns").empty(),"foreign rasterizer prefix authority");}
 unsigned ordinal{};
 for(const auto& [count,pointer,fill]:std::array<std::tuple<unsigned,unsigned,unsigned>,3>{{{0,0xdeadbeefu,0},{1,0x61000800u,90},{2,0x61000900u,165}}}){
  memory.restore(cold);constexpr unsigned stack=0x62000800u;memory.mount(0x62000000u,std::vector<std::byte>(4096,std::byte(fill)),SourceRuntimeMemory::Access::ReadWrite,"declared original rasterizer producer caller stack");memory.mount(0x61000000u,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"declared original matg descriptor input");word(memory,0x4ecc1cu,0x61000000u);word(memory,0x61000108u,count);word(memory,0x6100010cu,pointer);word(memory,0x4922a0u,0xa5a5a5a5u);
  SourcePhysicsWorldRuntimeAbi a{};a.gp={0x11,0x22,0x33,0x44,stack,0x55,0x66,0x77};a.eflags=0x202;std::vector<unsigned>pcs;nlohmann::json stores=nlohmann::json::array();SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){if(p==0x22596u)throw PrefixStop{};pcs.push_back(p);};bool stopped{};
  try{source_run_campaign_activation_tu15(memory,0x22570u,a,{0x27f,0x1f80},services,[&](const auto&s){stores.push_back({{"pc",s.instruction},{"address",s.address},{"before",hex(s.before)},{"after",hex(s.written)}});});}catch(const PrefixStop&){stopped=true;}
  check(stopped&&stores.size()==3&&Bytes(memory.view(0x4922a0u,4)).u32(0)==(count?pointer:0),"original prefix did not choose actual descriptor/empty branch");
  if(oracle){const auto&c=oracle->at("cases").at(ordinal);check(c.at("exit_gp")==a.gp&&c.at("exit_eflags")==a.eflags&&c.at("pcs")==pcs&&c.at("stores")==stores&&c.at("stack_hex")==hex(memory.view(0x62000000u,4096)),"original rasterizer prefix GP/flags/PC/stores/stack differs");}++ordinal;
 }
 if(argc==4){check(std::string_view(argv[1])=="--mcc","usage: rasterizer-pointer-test [ORIGINAL_PREFIX_JSON | --mcc MCC_MAP SAVE_DIRECTORY]");if(!*argv[2]||!std::filesystem::exists(argv[2]))return 77;Cache cache(argv[2]);check(cache.is_mcc(),"actual donor is not MCC");std::filesystem::create_directories(argv[3]);PhysicsLiveWorldTu15 world(cache,argv[3]);auto&bank=world.memory();check(Bytes(bank.view(0x4922a0u,4)).u32(0)==0,"normalWorld fabricated rasterizer publication before producer");const auto*g=cache.find_tag(cache.globals_tag(),tag_groups::globals);check(g!=nullptr,"actual matg absent");const auto raw=Bytes(cache.read_tag_bytes(g->address+0x108u,8));const auto runtime=Bytes(bank.view(Bytes(bank.view(0x4ecc1cu,4)).u32(0)+0x108u,8));check(raw.u32(0)==1&&runtime.u32(0)==1,"actual rasterizer descriptor not singular");const auto selected=runtime.u32(4);check(bank.native_address(selected,0x108u).value==world.resident_cache_address(cache,raw.u32(4),0x108u).value,"rasterizer pointer is not exact retained canonical relocation");const auto shader=TagHandle{Bytes(bank.view(selected+0xccu,8)).u32(4)};check(cache.find_tag(shader,TagGroup{"shad"})!=nullptr,"actual rasterizer global shader full-salt owner absent");
  const auto a_before=world.prepare_source_call();auto a=a_before;const auto stack_before=std::vector<std::byte>(bank.view(a.gp[4]-8,8).begin(),bank.view(a.gp[4]-8,8).end());SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){if(p==0x22596u)throw PrefixStop{};};unsigned stores{};bool stopped{};try{source_run_campaign_activation_tu15(bank,0x22570u,a,{0x27f,0x1f80},services,[&](const auto&){++stores;});}catch(const PrefixStop&){stopped=true;}check(stopped&&stores==3&&Bytes(bank.view(0x4922a0u,4)).u32(0)==selected,"actual retained matg prefix did not select genuine rasterizer pointer");word(bank,0x4922a0u,0);bank.write(a_before.gp[4]-8,stack_before);std::cout<<"actual_MCC_rasterizer_source="<<raw.u32(4)<<" canonical="<<selected<<" shader="<<shader.value<<" normalWorld_cold=0 bounded_prefix_restored=1 ";
 }
 else check(argc<=2,"unknown rasterizer pointer test arguments");
 std::cout<<"PASS "<<checks<<" checks/"<<negatives<<" negatives; exact loader-zero storage and bounded22570 prefix; fullnewmap/graphics/camo parity unresolved\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
