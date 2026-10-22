#include "halo2/native_tu15_camera_packet_data.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_vehicle_director_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#ifdef HALO2_TEST_RETAINED_CAMERA_DIRECTOR
extern "C" bool halo2_frame15_retained_director_owns(std::uint32_t);
extern "C" void halo2_frame15_retained_director_run(halo2::SourcePhysicsContactMemory&,std::uint32_t,
 halo2::SourcePhysicsWorldRuntimeAbi&,halo2::SourcePhysicsFactoryFloatingPoint,
 const halo2::SourcePhysicsWorldRuntimeServices&,const halo2::SourcePhysicsContactRecordObserver&);
#endif
using namespace halo2;
namespace {
unsigned checks{},negatives{};
void check(bool ok,const char*why){++checks;if(!ok)throw std::runtime_error(why);}
void put(SourceRuntimeMemory&m,unsigned p,unsigned value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8));m.write(p,b);}
std::string hex(std::span<const std::byte>b){std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+="0123456789abcdef"[n>>4];s+="0123456789abcdef"[n&15];}return s;}
nlohmann::json execute(SourceRuntimeMemory&m,unsigned entry,SourcePhysicsWorldRuntimeAbi&a,bool retained){
 std::vector<unsigned>trace;nlohmann::json stores=nlohmann::json::array();SourcePhysicsWorldRuntimeServices service;service.instruction=[&](auto p){trace.push_back(p);};service.required_owner=[](auto&,const auto&,auto&,const auto&){throw FormatError("retained camera branch needs next outside owner");};const auto observer=[&](const auto&s){stores.push_back({{"pc",s.instruction},{"address",s.address},{"before",hex(s.before)},{"after",hex(s.written)}});};std::string error;
 try{
#ifdef HALO2_TEST_RETAINED_CAMERA_DIRECTOR
 if(retained)halo2_frame15_retained_director_run(m,entry,a,{0x27f,0x1f80},service,observer);else
#else
 (void)retained;
#endif
 source_run_vehicle_director_tu15(m,entry,a,{0x27f,0x1f80},service,observer);
}catch(const std::exception&e){error=e.what();}
 return {{"exit_gp",a.gp},{"exit_xmm",a.xmm},{"exit_eflags",a.eflags},{"exit_x87_depth",a.x87_depth},{"exit_x87_status",a.x87_status},{"exit_fp",a.floating_point?nlohmann::json::array({a.floating_point->x87_control_word,a.floating_point->mxcsr}):nlohmann::json()},{"pcs",trace},{"stores",stores},{"error",error}};
}
}
int main(int argc,char**argv)try{
 check(argc<=2,"usage: camera-packet-data-test [ORIGINAL_FIXTURE|--dump]");auto m=native_tu15_world_memory({0x60212000u,{},"isolated same factory clock"});const auto cold=m.checkpoint();
 if(argc==2&&std::string_view(argv[1])=="--dump"){nlohmann::json j=nlohmann::json::array();for(const auto&r:cold.regions){std::vector<unsigned>bytes;for(auto b:r.bytes)bytes.push_back(std::to_integer<unsigned>(b));j.push_back({{"address",r.address},{"bytes",bytes},{"owner",r.owner},{"access",int(r.access)},{"page_bytes",r.page_bytes},{"committed",r.committed}});}std::cout<<j.dump()<<'\n';return 0;}
 for(const auto[p,bits]:std::array<std::pair<unsigned,unsigned>,3>{{{0x47da8c,0x3f800000},{0x56d324,0},{0x56d328,0}}}){const auto r=std::ranges::find(cold.regions,p,&SourceRuntimeMemory::CheckpointRegion::address);check(r!=cold.regions.end()&&r->bytes.size()==4&&r->access==SourceRuntimeMemory::Access::ReadWrite&&!r->page_bytes&&r->committed.empty()&&Bytes(m.view(p,4)).u32(0)==bits&&m.native_address(p,4).value>UINT32_MAX,"camera packet owner lost exact writable loader state/native backing");bool bound{};try{m.validate_mutable_view(p,5);}catch(const std::out_of_range&){bound=true;}check(bound,"camera packet word borrowed neighboring data");++negatives;SourceRuntimeMemory foreign;foreign.mount(p,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,"foreign camera packet overlap");const auto prior=foreign.checkpoint();bool reject{};try{mount_native_tu15_camera_packet_data(foreign);}catch(const std::invalid_argument&){reject=true;}check(reject&&foreign.checkpoint()==prior,"camera data conflict caused partial publication");++negatives;}
 for(const auto[p,bits]:std::array<std::pair<unsigned,unsigned>,3>{{{0x46a6f4,0x3ef1463b},{0x46a6f8,0x3f20d97a},{0x46a75c,0x3ea8f5c3}}}) {
  check(m.is_read_only(p,4)&&Bytes(m.view(p,4)).u32(0)==bits,
      "Absent-unit camera lost its authenticated readonly operand");
  SourceRuntimeMemory foreign;foreign.mount(p+1,std::array<std::byte,1>{},
      SourceRuntimeMemory::Access::ReadWrite,"foreign partial camera constant overlap");
  const auto prior=foreign.checkpoint();bool reject{};
  try{mount_native_tu15_camera_packet_data(foreign);}catch(const std::invalid_argument&){reject=true;}
  check(reject&&foreign.checkpoint()==prior,"Camera constant overlap caused partial publication");++negatives;
 }
 constexpr unsigned stack=0x62000000u;std::optional<nlohmann::json>oracle;if(argc==2){if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;std::ifstream f(argv[1]);oracle=nlohmann::json::parse(f);check(oracle->at("format")=="native_tu15_camera_packet_limit_initializers_v1"&&oracle->at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&oracle->at("instruction_patches").empty()&&oracle->at("supplied_callee_returns").empty(),"foreign or patched camera limit authority");}
 unsigned cases{};for(auto entry:{0x382360u,0x382380u})for(unsigned fill:{0u,90u,165u}){m.restore(cold);m.mount(stack,std::array<std::byte,0x100>{},SourceRuntimeMemory::Access::ReadWrite,"declared original camera CRT caller stack");const auto target=entry==0x382360u?0x56d324u:0x56d328u;put(m,target,fill*0x01010101u);SourcePhysicsWorldRuntimeAbi a{};a.gp={0x11u,0x22u,0x33u,0x44u,stack+0x80u,0x55u,0x66u,0x77u};a.eflags=0x202u;for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)a.xmm[i][j]=0x7fa12000u+i*16+j;put(m,a.gp[4],0x63000000u);const auto before=a;const auto result=execute(m,entry,a,false);check(result.at("error")==""&&result.at("pcs")==std::array<unsigned,3>{entry,entry+8,entry+16}&&result.at("stores").size()==1,"complete camera CRT did not execute genuine3PC/1store");check(a.gp[4]==before.gp[4]+4&&Bytes(m.view(target,4)).u32(0)==Bytes(m.view(entry==0x382360u?0x46a164u:0x46a5f8u,4)).u32(0)&&Bytes(m.view(0x47da8c,4)).u32(0)==0x3f800000u,"camera CRT fabricated operands/multiplier/return");if(oracle){const auto&c=oracle->at("cases").at(cases);check(c.at("entry_gp")==before.gp&&c.at("entry_xmm")==before.xmm&&c.at("exit_gp")==a.gp&&c.at("exit_xmm")==a.xmm&&c.at("exit_eflags")==a.eflags&&c.at("pcs")==result.at("pcs")&&c.at("stores")==result.at("stores")&&a.floating_point&&c.at("exit_x87_control")==a.floating_point->x87_control_word&&c.at("exit_mxcsr")==a.floating_point->mxcsr&&a.x87_depth==0&&a.x87_status==0,"whole3instruction camera CRT GP/XMM/FP/PC/store/RET differential differs");for(const auto&post:c.at("postimages"))check(hex(m.view(post.at("address"),4))==post.at("raw_hex").get<std::string>(),"camera CRT complete3word postimage differs");}++cases;}
 unsigned forwarded{};
#ifdef HALO2_TEST_RETAINED_CAMERA_DIRECTOR
 // Private retained-object linkage control. Every old entry forwards directly;
 // compare its complete observed prefix, exception, ABI and bank mutation.
 constexpr std::array<unsigned,19>old_entries{0xbcee0,0xcad60,0x106160,0x157fe0,0x158290,0x158450,0x158790,0x158900,0x171ff0,0x189f20,0x245060,0x245400,0x246cf0,0x246e90,0x246f30,0x247020,0x247090,0x247140,0x37cde8};
 for(auto entry:old_entries){check(halo2_frame15_retained_director_owns(entry)&&source_vehicle_director_tu15_owns_entry(entry),"private supplement dropped old entry");SourceRuntimeMemory a,b;a.restore(cold);b.restore(cold);for(auto*p:{&a,&b}){p->mount(stack,std::array<std::byte,4096>{},SourceRuntimeMemory::Access::ReadWrite,"declared retained camera forwarding caller stack");put(*p,stack+0x800,0x63000000);}SourcePhysicsWorldRuntimeAbi aa{},bb{};aa.gp[4]=bb.gp[4]=stack+0x800;aa.eflags=bb.eflags=0x202;const auto direct=execute(a,entry,aa,true),forward=execute(b,entry,bb,false);check(direct==forward&&a.checkpoint()==b.checkpoint(),"private supplement changed retained entry instructions/stores/ABI/exception prefix");}
 check(!halo2_frame15_retained_director_owns(0x382360)&&!halo2_frame15_retained_director_owns(0x382380)&&source_vehicle_director_tu15_owns_entry(0x382360)&&source_vehicle_director_tu15_owns_entry(0x382380),"private supplement domain is not exactly2newCRTentries");
 forwarded=19;
#endif
 std::cout<<"PASS "<<checks<<" checks/"<<negatives<<" negatives;3exactwritablewords,6whole3PC/1store CRTcases and"<<forwarded<<"byte-retained entry forwarding controls; no completed GPU/frame parity claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
