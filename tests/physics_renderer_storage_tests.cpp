#include "halo2/physics_renderer_storage_runtime.hpp"
#include "halo2/source_runtime_image.hpp"
#include <openssl/evp.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <set>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool ok,const std::string &why){if(!ok)throw std::runtime_error(why);}
SourcePhysicsWorldRuntimeAbi abi(const J &j) {
  SourcePhysicsWorldRuntimeAbi a{};
  a.gp=j.at("gp").get<decltype(a.gp)>();a.eflags=j.at("eflags");
  a.x87_status=j.at("x87_status");
  a.floating_point=SourcePhysicsFactoryFloatingPoint{j.at("x87_control").get<std::uint16_t>(),j.at("mxcsr")};
  for(unsigned i=0;i<8;++i) {
    const auto h=j.at("xmm").at(i).get<std::string>();check(h.size()==32,"Original XMM extent");
    for(unsigned k=0;k<4;++k)a.xmm[i][k]=std::stoul(h.substr(24-8*k,8),nullptr,16);
  }
  return a;
}
void same_abi(const SourcePhysicsWorldRuntimeAbi&a,const J&j,const std::string&why) {
  const auto b=abi(j);
  if(a.gp!=b.gp) {
    std::ostringstream s;s<<why<<" GP"<<std::hex;
    for(unsigned i=0;i<8;++i)if(a.gp[i]!=b.gp[i])s<<' '<<i<<':'<<a.gp[i]<<'/'<<b.gp[i];
    throw std::runtime_error(s.str());
  }
  check((a.eflags&0xcd5u)==(b.eflags&0xcd5u),why+" defined flags");
  check(a.xmm==b.xmm && a.floating_point->x87_control_word==b.floating_point->x87_control_word &&
      a.floating_point->mxcsr==b.floating_point->mxcsr && a.x87_status==b.x87_status &&
      !a.x87_depth && !a.active_mmx,why+" retained FP state");
}
std::string digest(std::span<const std::byte> b){
  std::array<unsigned char,EVP_MAX_MD_SIZE> out;unsigned n=0;
  check(EVP_Digest(b.data(),b.size(),out.data(),&n,EVP_sha256(),nullptr)==1,"SHA256 unavailable");
  std::ostringstream s;for(unsigned i=0;i<n;++i)s<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(out[i]);return s.str();
}

std::vector<std::byte> unhex(const std::string&s){std::vector<std::byte>b;check(!(s.size()&1u),"Odd source byte extent");for(unsigned i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return b;}
void put(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b;for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);}
}
int main(int argc,char**argv){try{
 check(argc==3,"Supply unchanged TU1.5 renderer storage fixture and selected executable");
 if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
#if !defined(__linux__)
 return 77;
#endif
 std::ifstream in(argv[1]);const auto f=J::parse(in);std::size_t writes=0,final_bytes=0,instructions=0;std::set<std::uint32_t>sites;
 const auto first_entry=0x133760u;const auto stack=f.at("stack").get<std::uint32_t>();
 for(const auto &c:f.at("cases")){
  auto bank=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
  std::vector<std::byte> initial(0x10000);for(unsigned i=0;i<initial.size();++i)initial[i]=std::byte((i*13+7)&255u);
  bank.mount(stack,initial,SourceRuntimeMemory::Access::ReadWrite,"declared renderer producer caller stack");
  auto state=abi(c.at("entry_abi"));put(bank,state.gp[4],f.at("stop"));std::size_t stores=0;
  SourcePhysicsWorldRuntimeServices services{};services.instruction=[&](std::uint32_t p){std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(8)<<p;sites.insert(f.at("canonical_instruction_map").at(key.str()).get<std::uint32_t>());++instructions;};
  const auto observe=[&](const SourcePhysicsContactRecordStore&s){
   if(stack<=s.address && s.address<stack+initial.size())return;
   check(stores<c.at("stores").size(),"Unexpected source renderer store");const auto&r=c.at("stores")[stores++];
   std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(8)<<s.instruction;
   const auto before=unhex(r.at("before")),written=unhex(r.at("written"));
   check(f.at("canonical_instruction_map").at(key.str())==r.at("instruction") && s.address==r.at("address") && std::ranges::equal(s.before,before) && std::ranges::equal(s.written,written),"Actual source renderer ordered store differs");++writes;
  };
  unsigned resources=0;
  {
   SourcePhysicsPoolHostStorage host(bank,c.at("token"),[&](const SourcePhysicsPoolResourceEvent&e){
    check(resources<c.at("resources").size(),"Unexpected native renderer resource");const auto&r=c.at("resources")[resources++];
    check(e.allocation && J(e.request)==r.at("request") && J(e.result)==r.at("result"),"Actual native renderer request/result differs");
    check(digest(bank.view(e.result[1],e.result[2]))==r.at("initial_sha256").get<std::string>(),"Actual native renderer initial pages differ");
   });
   bool pending=false;try{source_run_physics_renderer_storage_with_native_pages(bank,first_entry,state,{0x27f,0x1f80},services,host,observe);}catch(const SourcePhysicsRendererAllocatorUnavailable&e){
    check(e.call.target==f.at("missing_allocator_target") && !e.call.tail,"Wrong actual renderer allocator owner");pending=true;
   }
   check(pending,"Original required renderer allocator was bypassed");same_abi(state,c.at("exit_abi"),"Original renderer allocator frontier ABI");
   check(std::ranges::equal(bank.view(state.gp[4],8),unhex(c.at("pending_stack"))),"Original pending renderer allocation frame differs");
   check(stores==c.at("stores").size() && resources==1 && host.live_allocations()==1,"Original renderer prefix counts differ");
   for(const auto&r:c.at("final_banks")){const auto p=r.at("address").get<std::uint32_t>();const auto n=r.at("bytes").get<std::size_t>();check(digest(bank.view(p,n))==r.at("sha256").get<std::string>(),"Whole retained renderer bank differs");final_bytes+=n;}
  }
  bool released=false;try{bank.view(c.at("token"),1);}catch(const std::exception&){released=true;}check(released,"Native renderer raw pages were retained after retirement");
  auto missing=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);missing.mount(stack,initial,SourceRuntimeMemory::Access::ReadWrite,"independent missing raw renderer owner caller stack");
  auto stopped=abi(c.at("entry_abi"));put(missing,stopped.gp[4],f.at("stop"));bool unavailable=false;
  try{source_run_physics_renderer_storage_runtime(missing,first_entry,stopped,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},{});}catch(const std::runtime_error&){unavailable=true;}
  check(unavailable,"Missing original raw renderer owner did not stop");same_abi(stopped,c.at("missing_raw_owner").at("abi"),"Missing raw renderer owner actual frontier ABI");
  check(std::ranges::equal(missing.view(stopped.gp[4],20),unhex(c.at("missing_raw_owner").at("stack"))),"Missing raw renderer owner actual frame differs");
 }
 check(f.at("cases").size()==12 && writes==f.at("ordered_writes") && instructions==f.at("original_instruction_count") && sites.size()==f.at("executed_sites"),"Complete renderer corpus count differs");
 std::cout<<"12 fresh TU1.5 renderer producer prefixes, "<<instructions<<" instruction observations, "<<writes<<" ordered stores and "<<final_bytes<<" final bank bytes match; process heap allocation remains required\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
