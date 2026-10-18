#include "halo2/native_tu15_renderer_startup_data.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_model_render_tu15.hpp"
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
void word(SourceRuntimeMemory&m,unsigned p,unsigned v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);}
constexpr std::array<unsigned,5>targets{0x1404e9,0x1404e9,0x1404e9,0x1404dd,0x1404dd};
}
int main(int argc,char**argv)try{
 auto memory=native_tu15_world_memory({0x60212000u,{},"isolated same factory clock"});const auto cold=memory.checkpoint();
 if(argc==2&&std::string_view(argv[1])=="--dump"){nlohmann::json j=nlohmann::json::array();for(const auto&r:cold.regions){std::vector<unsigned>b;for(auto v:r.bytes)b.push_back(std::to_integer<unsigned>(v));j.push_back({{"address",r.address},{"bytes",b},{"owner",r.owner},{"access",int(r.access)},{"page_bytes",r.page_bytes},{"committed",r.committed}});}std::cout<<j.dump()<<'\n';return 0;}
 const auto owner=std::ranges::find(cold.regions,0x1404f8u,&SourceRuntimeMemory::CheckpointRegion::address);check(owner!=cold.regions.end()&&owner->bytes.size()==20&&owner->access==SourceRuntimeMemory::Access::ReadOnly&&!owner->page_bytes&&owner->committed.empty(),"selector lacks exact immutable20B owner");check(memory.native_address(0x1404f8u,20).value>UINT32_MAX,"selector backing narrowed");for(unsigned n=0;n<5;++n)check(Bytes(memory.view(0x1404f8u,20)).u32(n*4)==targets[n],"original selector target differs");
 for(auto[p,n]:std::array<std::pair<unsigned,unsigned>,3>{{{0x1404f7u,2},{0x1404f8u,21},{0x14050bu,2}}}){bool no{};try{memory.view(p,n);}catch(const std::out_of_range&){no=true;}check(no,"selector borrowed inline padding/code");++negatives;}
 for(auto[p,n]:std::array<std::pair<unsigned,unsigned>,3>{{{0x1404f7u,2},{0x1404f8u,20},{0x14050bu,2}}}){SourceRuntimeMemory foreign;foreign.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,"foreign original selector overlap");const auto before=foreign.checkpoint();bool no{};try{mount_native_tu15_renderer_startup_data(foreign);}catch(const std::invalid_argument&){no=true;}check(no&&foreign.checkpoint()==before,"selector conflict caused partial mounting");++negatives;}
 bool read_only{};try{memory.mutable_view(0x1404f8u,20);}catch(const std::runtime_error&){read_only=true;}check(read_only,"selector table became mutable");++negatives;
 std::optional<nlohmann::json>oracle;if(argc==2){if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;std::ifstream in(argv[1]);oracle=nlohmann::json::parse(in);check(oracle->at("format")=="native_tu15_model_probe_selector_v1"&&oracle->at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&oracle->at("instruction_patches").empty()&&oracle->at("supplied_callee_returns").empty(),"foreign model selector authority");}
 unsigned ordinal{};for(unsigned selector:{0u,1u,2u,3u,4u,5u,6u,7u,0x80000000u,0xffffffffu}){
  memory.restore(cold);memory.mount(0x62000000u,std::array<std::byte,256>{},SourceRuntimeMemory::Access::ReadWrite,"declared original selector caller stack");word(memory,0x62000080u,0x63000000u);const auto before=memory.checkpoint();SourcePhysicsWorldRuntimeAbi a{};a.gp={selector,0x11223344u,0x22334455u,0x33445566u,0x62000080u,0x44556677u,0x55667788u,0x66778899u};a.eflags=0x202;std::vector<unsigned>pcs;unsigned stores{};SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){pcs.push_back(p);};source_run_model_render_tu15(memory,0x1404d0u,a,{0x27f,0x1f80},services,[&](const auto&){++stores;});check(a.gp[0]==(((selector-1)&0xffffff00u)|unsigned(selector==4||selector==5))&&a.gp[4]==0x62000084u&&!stores&&memory.checkpoint()==before,"original selector computation/RET/data changed");if(oracle){const auto&c=oracle->at("cases").at(ordinal);check(c.at("selector")==selector&&c.at("exit_gp")==a.gp&&c.at("exit_eflags")==a.eflags&&c.at("pcs")==pcs&&c.at("stores").empty(),"whole original selector GP/flags/PC/RET differs");}++ordinal;
 }
 if(argc==4){check(std::string_view(argv[1])=="--mcc","usage: model-selector-data-test [ORIGINAL_JSON | --mcc MCC_MAP SAVE_DIRECTORY]");if(!*argv[2]||!std::filesystem::exists(argv[2]))return 77;Cache cache(argv[2]);check(cache.is_mcc(),"actual donor is not MCC");std::filesystem::create_directories(argv[3]);PhysicsLiveWorldTu15 world(cache,argv[3]);check(world.memory().is_read_only(0x1404f8u,20)&&world.memory().native_address(0x1404f8u,20).value>UINT32_MAX,"normalWorld selector owner differs");for(unsigned n=0;n<5;++n)check(Bytes(world.memory().view(0x1404f8u,20)).u32(n*4)==targets[n],"normalWorld selector targets differ");auto a=world.prepare_source_call();a.gp[0]=2;world.run_actual(0x1404d0u,a);check((a.gp[0]&255u)==0,"normalWorld genuine selector failed");std::cout<<"actual_MCC_original_selector=0 ";}
 else check(argc<=2,"unknown selector test arguments");
 std::cout<<"PASS "<<checks<<" checks/"<<negatives<<" negatives; exact20B inline targets and10 whole1404D0 cases; no replacement branch/results/GPU claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
