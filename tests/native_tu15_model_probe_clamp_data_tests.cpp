#include "halo2/native_tu15_renderer_startup_data.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
unsigned checks{},negatives{};
void check(bool yes,const char*why){++checks;if(!yes)throw std::runtime_error(why);}
void word(SourceRuntimeMemory&m,unsigned p,unsigned v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);}
std::string hex(std::span<const std::byte>b){std::string s;for(auto x:b){auto n=std::to_integer<unsigned>(x);s+="0123456789abcdef"[n>>4];s+="0123456789abcdef"[n&15];}return s;}
constexpr std::array<std::pair<unsigned,unsigned>,3>operands{{{0x46a460,0xc47a0000},{0x46a79c,0x3f170a3d},{0x46a7a0,0x3de147ae}}};
struct Case{std::array<float,6>v;float b0{},b1{};};
constexpr std::array<Case,10>cases{{
 {{{-2048,-4096,-2048,-4096,-2048,-4096}},0,0},
 {{{-1000,-1000,-1000,-1000,-1000,-1000}},0,0},
 {{{0,0,0,0,0,0}},0,0},{{{1000,1000,1000,1000,1000,1000}},0,0},
 {{{2048,4096,2048,4096,2048,4096}},0,0},
 {{{-2048,2048,0,0,2048,-2048}},.25f,.5f},
 {{{-4096,4096,4096,-4096,8192,-8192}},.5f,.25f},
 {{{-1024,1024,0,0,1024,-1024}},1,0},
 {{{1024,-1024,0,0,-1024,1024}},0,1},
 {{{-1000,1000,1000,-1000,3000,-3000}},.5f,.5f}}};
}
int main(int argc,char**argv)try{
 auto m=native_tu15_world_memory({0x60212000u,{},"isolated same factory clock"});const auto cold=m.checkpoint();
 if(argc==2&&std::string_view(argv[1])=="--dump"){nlohmann::json j=nlohmann::json::array();for(const auto&r:cold.regions){std::vector<unsigned>b;for(auto v:r.bytes)b.push_back(std::to_integer<unsigned>(v));j.push_back({{"address",r.address},{"bytes",b},{"owner",r.owner},{"access",int(r.access)},{"page_bytes",r.page_bytes},{"committed",r.committed}});}std::cout<<j.dump()<<'\n';return 0;}
 for(const auto[p,bits]:operands){const auto r=std::ranges::find(cold.regions,p,&SourceRuntimeMemory::CheckpointRegion::address);check(r!=cold.regions.end()&&r->bytes.size()==4&&r->access==SourceRuntimeMemory::Access::ReadOnly&&!r->page_bytes&&r->committed.empty()&&m.native_address(p,4).value>UINT32_MAX,"direct scalar owner extent/access/backing changed");check(Bytes(m.view(p,4)).u32(0)==bits,"direct original scalar bits differ");bool no{};try{m.mutable_view(p,4);}catch(const std::runtime_error&){no=true;}check(no,"original rdata scalar became mutable");++negatives;
  for(const auto offset:{0u,3u}){SourceRuntimeMemory foreign;foreign.mount(p+offset,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,"foreign original scalar overlap");const auto before=foreign.checkpoint();no=false;try{mount_native_tu15_renderer_startup_data(foreign);}catch(const std::invalid_argument&){no=true;}check(no&&foreign.checkpoint()==before,"scalar overlap caused partial publication");++negatives;}
 }
 for(const auto[p,n]:std::array<std::pair<unsigned,unsigned>,4>{{{0x46a45f,2},{0x46a460,5},{0x46a79b,2},{0x46a7a0,5}}}){bool no{};try{m.view(p,n);}catch(const std::out_of_range&){no=true;}check(no,"scalar backing borrowed unauthenticated neighbor bytes");++negatives;}
 std::optional<nlohmann::json>oracle;if(argc==2){if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;std::ifstream in(argv[1]);oracle=nlohmann::json::parse(in);check(oracle->at("format")=="native_tu15_model_probe_clamp_v1"&&oracle->at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&oracle->at("instruction_patches").empty()&&oracle->at("supplied_callee_returns").empty()&&oracle->at("whole_function")==true,"foreign direct scalar original authority");}
 unsigned ordinal{};for(const auto&c:cases){m.restore(cold);m.mount(0x61000000,std::vector<std::byte>(0x6000),SourceRuntimeMemory::Access::ReadWrite,"declared original FLOAT2 interpolation inputs/output");m.mount(0x62000000,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"declared original D5300 caller stack");word(m,0x6100003c,0x61001000);word(m,0x61000024,0x61002000);const std::array<std::byte,1>type{std::byte{24}};m.write(0x61001020,type);word(m,0x61001028,0x61003000);word(m,0x6100102c,0);const std::array<std::byte,6>indices{std::byte{0},std::byte{0},std::byte{1},std::byte{0},std::byte{2},std::byte{0}};m.write(0x61002000,indices);m.write(0x61003000,std::as_bytes(std::span(c.v)));word(m,0x62000080,0x63000000);word(m,0x62000084,std::bit_cast<unsigned>(c.b0));word(m,0x62000088,std::bit_cast<unsigned>(c.b1));SourcePhysicsWorldRuntimeAbi a{};a.gp={0,0x61000000u,0x22334455u,0x61004000u,0x62000080u,0x44556677u,0x55667788u,0x61004004u};a.eflags=0x202;for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)a.xmm[i][j]=0x7fa12000u+i*16+j;
  std::vector<unsigned>pcs;nlohmann::json stores=nlohmann::json::array();SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){pcs.push_back(p);};source_run_model_render_tu15(m,0xd5300,a,{0x27f,0x1f80},services,[&](const auto&s){stores.push_back({{"pc",s.instruction},{"address",s.address},{"before",hex(s.before)},{"after",hex(s.written)}});});const auto output=Bytes(m.view(0x61004000,8));check(pcs.front()==0xd5300&&(pcs.back()==0xd54ac||pcs.back()==0xd54c3)&&a.gp[4]==0x6200008c&&std::abs(output.f32(0))<=1000.f&&std::abs(output.f32(4))<=1000.f,"complete original FLOAT2 interpolation/clamp/RET did not run");
  if(oracle){const auto&o=oracle->at("cases").at(ordinal);const auto same=o.at("exit_gp")==a.gp&&o.at("exit_xmm")==a.xmm&&o.at("exit_eflags")==a.eflags&&o.at("pcs")==pcs&&o.at("stores")==stores&&o.at("output")==hex(m.view(0x61004000,8))&&o.at("stack_hex")==hex(m.view(0x62000000,4096))&&a.floating_point&&o.at("exit_x87_control")==a.floating_point->x87_control_word&&o.at("exit_mxcsr")==a.floating_point->mxcsr;if(!same)std::cerr<<nlohmann::json{{"case",ordinal},{"gp",o.at("exit_gp")==a.gp},{"xmm",o.at("exit_xmm")==a.xmm},{"flags",a.eflags},{"expected_flags",o.at("exit_eflags")},{"mxcsr",a.floating_point?a.floating_point->mxcsr:0},{"expected_mxcsr",o.at("exit_mxcsr")},{"stores",o.at("stores")==stores},{"pcs",o.at("pcs")==pcs}}.dump()<<'\n';check(same,"whole original D5300 GP/XMM/FP/flags/PC/stores/stack/output differs");}++ordinal;
 }
 if(argc==4){check(std::string_view(argv[1])=="--mcc","usage: model-clamp-data-test [ORIGINAL_JSON | --mcc MCC_MAP SAVE_DIRECTORY]");if(!*argv[2]||!std::filesystem::exists(argv[2]))return 77;Cache cache(argv[2]);check(cache.is_mcc(),"actual donor is not MCC");std::filesystem::create_directories(argv[3]);PhysicsLiveWorldTu15 world(cache,argv[3]);for(const auto[p,bits]:operands)check(world.memory().is_read_only(p,4)&&world.memory().native_address(p,4).value>UINT32_MAX&&Bytes(world.memory().view(p,4)).u32(0)==bits,"normalWorld direct original scalar owner differs");std::cout<<"actual_MCC_direct_readonly_scalar_owners=3 ";}
 else check(argc<=2,"unknown scalar test arguments");
 std::cout<<"PASS "<<checks<<" checks/"<<negatives<<" negatives; exact12B direct operands and10 complete original FLOAT2 interpolation/clamp cases; no SHORT2/lighting/frame parity claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
