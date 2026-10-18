#include "halo2/native_tu15_renderer_startup_data.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_shadow_frame_tu15.hpp"
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
 const auto owner=std::ranges::find(cold.regions,0x4cd3a0u,&SourceRuntimeMemory::CheckpointRegion::address);
 check(owner!=cold.regions.end()&&owner->bytes.size()==4&&owner->access==SourceRuntimeMemory::Access::ReadWrite&&!owner->page_bytes&&owner->committed.empty()&&Bytes(owner->bytes).u32(0)==0,"renderer context pointer is not its exact cold4-byte owner");
 check(memory.native_address(0x4cd3a0u,4).value>UINT32_MAX,"renderer context backing narrowed to32bits");
 for(auto[p,n]:std::array<std::pair<unsigned,unsigned>,3>{{{0x4cd39fu,1},{0x4cd3a4u,1},{0x4cd3a0u,5}}}){bool no{};try{memory.view(p,n);}catch(const std::out_of_range&){no=true;}check(no,"current context owner borrowed neighboring bytes");++negatives;}
 for(auto[p,n]:std::array<std::pair<unsigned,unsigned>,3>{{{0x51df44u,4},{0x556e18u,0x55c0},{0x4cd3a0u,4}}}){SourceRuntimeMemory foreign;foreign.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,"foreign renderer overlap");const auto before=foreign.checkpoint();bool no{};try{mount_native_tu15_renderer_startup_data(foreign);}catch(const std::invalid_argument&){no=true;}check(no&&before==foreign.checkpoint(),"renderer data conflict caused partial publication");++negatives;}
 std::optional<nlohmann::json>oracle;
 if(argc==2){if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;std::ifstream in(argv[1]);oracle=nlohmann::json::parse(in);check(oracle->at("format")=="native_tu15_current_renderer_context_prefix_v1"&&oracle->at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&oracle->at("instruction_patches").empty()&&oracle->at("supplied_callee_returns").empty(),"foreign current-context prefix authority");}
 for(unsigned fill:{0u,90u,165u}){
  memory.restore(cold);constexpr unsigned stack=0x62000800u;memory.mount(0x62000000u,std::vector<std::byte>(4096,std::byte(fill)),SourceRuntimeMemory::Access::ReadWrite,"declared original current-context prefix caller stack");word(memory,stack,0x63000000u);word(memory,0x4cd3a0u,0x556e18u);SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x11u,0x22u,0x33u,0x44u,stack,0x55u,0x66u,0x77u};abi.eflags=0x202;std::vector<unsigned>pcs;nlohmann::json stores=nlohmann::json::array();SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){if(p==0x2cb48u)throw PrefixStop{};pcs.push_back(p);};bool stop{};try{source_run_shadow_frame_tu15(memory,0x2cb10u,abi,{0x27f,0x1f80},services,[&](const auto&s){stores.push_back({{"pc",s.instruction},{"address",s.address},{"before",hex(s.before)},{"after",hex(s.written)}});});}catch(const PrefixStop&){stop=true;}
  check(stop&&pcs.front()==0x2cb10&&pcs.back()==0x2cb44&&Bytes(memory.view(0x4cd3a0u,4)).u32(0)==0x556e18u,"genuine current-context consumer prefix did not reach exact next owner");
  check(std::ranges::all_of(memory.view(0x556e18u,0x55c0),[](auto b){return b==std::byte{};}),"context prefix fabricated constructor/frame results");
  if(oracle){const auto&c=oracle->at("cases").at(fill==0?0:fill==90?1:2);check(c.at("exit_gp")==abi.gp&&c.at("exit_eflags")==abi.eflags&&c.at("pcs")==pcs&&c.at("stores")==stores&&c.at("stack_hex")==hex(memory.view(0x62000000u,4096)),"unchanged original context prefix GP/flags/PC/ordered stores/stack differs");}
 }
 if(argc==4){check(std::string_view(argv[1])=="--mcc","usage: renderer-context-test [ORIGINAL_PREFIX_JSON | --mcc MCC_MAP SAVE_DIRECTORY]");if(!*argv[2]||!std::filesystem::exists(argv[2]))return 77;Cache cache(argv[2]);check(cache.is_mcc(),"actual input is not MCC");std::filesystem::create_directories(argv[3]);PhysicsLiveWorldTu15 world(cache,argv[3]);check(Bytes(world.memory().view(0x4cd3a0u,4)).u32(0)==0,"normalWorld supplied a selected renderer context before submission");const unsigned main=0x556e18u;world.write_source_call_input(0x4cd3a0u,std::as_bytes(std::span(&main,1)));check(Bytes(world.memory().view(0x4cd3a0u,4)).u32(0)==main&&world.memory().native_address(main,0x2ae0u).value>UINT32_MAX,"normalWorld real submitting input lost exact existing context backing");std::cout<<"actual MCC World accepted exact submitting context input; ";}
 else check(argc<=2,"unknown renderer context arguments");
 std::cout<<"PASS "<<checks<<" checks/"<<negatives<<" negatives; one cold4B owner, genuine bounded2CB10 prefix; no preset current context, fullframe/GPU claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
