#include "halo2/native_tu15_renderer_startup_data.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
unsigned checks{},negatives{};
void check(bool yes,const char*why){++checks;if(!yes)throw std::runtime_error(why);}
std::string hex(std::span<const std::byte>b){std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+="0123456789abcdef"[n>>4];s+="0123456789abcdef"[n&15];}return s;}
void word(SourceRuntimeMemory&m,unsigned p,unsigned v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);}
constexpr std::array<unsigned,18>words{0,0,0xc1200000u,0xc1200000u,0,0,0x41200000u,0,0,0,0xc1200000u,0,0,0x41200000u,0,0,0,0xc2200000u};
struct PrefixStop{};
}
int main(int argc,char**argv)try{
 auto memory=native_tu15_world_memory({0x60212000u,{},"isolated same factory clock"});const auto cold=memory.checkpoint();
 if(argc==2&&std::string_view(argv[1])=="--dump"){nlohmann::json j=nlohmann::json::array();for(const auto&r:cold.regions){std::vector<unsigned>b;for(auto v:r.bytes)b.push_back(std::to_integer<unsigned>(v));j.push_back({{"address",r.address},{"bytes",b},{"owner",r.owner},{"access",int(r.access)},{"page_bytes",r.page_bytes},{"committed",r.committed}});}std::cout<<j.dump()<<'\n';return 0;}
 const auto owner=std::ranges::find(cold.regions,0x44a7ccu,&SourceRuntimeMemory::CheckpointRegion::address);check(owner!=cold.regions.end()&&owner->bytes.size()==72&&owner->access==SourceRuntimeMemory::Access::ReadOnly&&!owner->page_bytes&&owner->committed.empty(),"probe vectors lack exact immutable72B owner");check(memory.native_address(0x44a7ccu,72).value>UINT32_MAX,"probe vector backing narrowed");for(unsigned n=0;n<18;++n)check(Bytes(memory.view(0x44a7ccu,72)).u32(n*4)==words[n],"original probe FLOAT bits differ");
 for(auto[p,n]:std::array<std::pair<unsigned,unsigned>,3>{{{0x44a7cbu,2},{0x44a7ccu,73},{0x44a813u,2}}}){bool no{};try{memory.view(p,n);}catch(const std::out_of_range&){no=true;}check(no,"probe bank borrowed neighbor bytes");++negatives;}
 for(auto[p,n]:std::array<std::pair<unsigned,unsigned>,3>{{{0x44a7cbu,2},{0x44a7ccu,72},{0x44a813u,2}}}){SourceRuntimeMemory foreign;foreign.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,"foreign original probe vector overlap");const auto before=foreign.checkpoint();bool no{};try{mount_native_tu15_renderer_startup_data(foreign);}catch(const std::invalid_argument&){no=true;}check(no&&foreign.checkpoint()==before,"probe vector overlap caused partial mounting");++negatives;}
 bool read_only{};try{memory.mutable_view(0x44a7ccu,72);}catch(const std::runtime_error&){read_only=true;}check(read_only,"authored probe bank became mutable");++negatives;
 std::optional<nlohmann::json>oracle;if(argc==2){if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;std::ifstream in(argv[1]);oracle=nlohmann::json::parse(in);check(oracle->at("format")=="native_tu15_model_probe_vectors_prefix_v1"&&oracle->at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&oracle->at("instruction_patches").empty()&&oracle->at("supplied_callee_returns").empty(),"foreign probe vector authority");}
 unsigned ordinal{};for(const auto&[flags,row]:std::array<std::pair<unsigned,unsigned>,8>{{{0,0},{1,1},{4,0},{4,1},{4,2},{4,3},{4,4},{4,5}}}){
  memory.restore(cold);memory.mount(0x61000000u,std::vector<std::byte>(0x5000),SourceRuntimeMemory::Access::ReadWrite,"declared original matched geometry and probe origin inputs");memory.mount(0x62000000u,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"declared original D4F90 caller stack");word(memory,0x4ecc14u,0x61000000u);word(memory,0x61000080u,1);word(memory,0x61000084u,0x61001000u);word(memory,0x6100101cu,0);word(memory,0x61001004u,7);word(memory,0x4ecc18u,0x61002000u);word(memory,0x61002008u,7);word(memory,0x61003000u,std::bit_cast<unsigned>(1.25f));word(memory,0x61003004u,std::bit_cast<unsigned>(-2.5f));word(memory,0x61003008u,std::bit_cast<unsigned>(3.75f));word(memory,0x62000804u,flags);word(memory,0x62000814u,0x44a7ccu+row*12);
  SourcePhysicsWorldRuntimeAbi a{};a.gp={0x11,0x22,0x33,0x61003000u,0x62000800u,0x55,0x66,0x77};a.eflags=0x202;for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)a.xmm[i][j]=0x7fa12000u+i*16+j;std::vector<unsigned>pcs;nlohmann::json stores=nlohmann::json::array();SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){if(p==0xd5090u)throw PrefixStop{};pcs.push_back(p);};bool stopped{};try{source_run_model_render_tu15(memory,0xd4f90u,a,{0x27f,0x1f80},services,[&](const auto&s){stores.push_back({{"pc",s.instruction},{"address",s.address},{"before",hex(s.before)},{"after",hex(s.written)}});});}catch(const PrefixStop&){stopped=true;}
  check(stopped&&pcs.front()==0xd4f90&&pcs.back()==0xd508a&&a.gp[6]==0x44a7ccu+row*12&&Bytes(memory.view(0x62000814u,4)).u32(0)==(flags==1?5u:1u),"original source did not select genuine row/count/ray setup");for(unsigned n=0;n<18;++n)check(Bytes(memory.view(0x44a7ccu,72)).u32(n*4)==words[n],"ray setup changed original vector bank");if(oracle){const auto&c=oracle->at("cases").at(ordinal);check(c.at("exit_gp")==a.gp&&c.at("exit_xmm")==a.xmm&&c.at("exit_eflags")==a.eflags&&c.at("pcs")==pcs&&c.at("stores")==stores&&c.at("stack_hex")==hex(memory.view(0x62000000u,4096))&&a.floating_point&&c.at("mxcsr")==a.floating_point->mxcsr,"original ray-setup GP/XMM/FP/flags/PC/stores/stack differs");}++ordinal;
 }
 if(argc==4){check(std::string_view(argv[1])=="--mcc","usage: probe-vector-data-test [ORIGINAL_JSON | --mcc MCC_MAP SAVE_DIRECTORY]");if(!*argv[2]||!std::filesystem::exists(argv[2]))return 77;Cache cache(argv[2]);check(cache.is_mcc(),"actual donor is not MCC");std::filesystem::create_directories(argv[3]);PhysicsLiveWorldTu15 world(cache,argv[3]);check(world.memory().is_read_only(0x44a7ccu,72)&&world.memory().native_address(0x44a7ccu,72).value>UINT32_MAX,"normalWorld probe bank owner differs");for(unsigned n=0;n<18;++n)check(Bytes(world.memory().view(0x44a7ccu,72)).u32(n*4)==words[n],"normalWorld probe FLOATbits differ");std::cout<<"actual_MCC_probe_vectors=6 ";}
 else check(argc<=2,"unknown probe vector arguments");
 std::cout<<"PASS "<<checks<<" checks/"<<negatives<<" negatives; exact72B immutable vectors and8 genuine pre-collision prefixes; no supplied ray hits/GPU/lighting parity claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
