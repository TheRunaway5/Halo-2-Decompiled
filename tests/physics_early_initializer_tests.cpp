#include "halo2/physics_game_arena_native.hpp"
#include "halo2/physics_early_initializer_runtime.hpp"
#include "halo2/native_threads.hpp"
#include <openssl/evp.h>
#include <condition_variable>
#include <mutex>
#include <unistd.h>
#include "halo2/source_runtime_image.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace {
using J=nlohmann::json;
using namespace halo2;
void check(bool ok,const std::string &why) { if(!ok)throw std::runtime_error(why); }
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p) {
  std::uint32_t v=0;const auto b=m.view(p,4);
  for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
void put(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t v) {
  std::array<std::byte,4>b;
  for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);
}
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
struct TemporaryDirectory {
  std::filesystem::path path;
  TemporaryDirectory(){
#if defined(__linux__)
    std::string name=(std::filesystem::temp_directory_path()/"halo2-early-initializer-XXXXXX").string();
    auto *p=::mkdtemp(name.data());check(p,"Native startup temporary file owner unavailable");path=p;
#endif
  }
  ~TemporaryDirectory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
struct Schedule {
  std::mutex mutex;
  std::condition_variable_any changed;
  unsigned idle=0,epoch=0;
  SourcePhysicsGameArenaSchedule services(){
    return {
      [this](auto token,std::stop_token stop){
        if(token!=0xf3000003u)return;
        std::unique_lock lock(mutex);++idle;changed.notify_all();
        const auto seen=epoch;
        if(!changed.wait(lock,stop,[&]{return epoch>seen;}))throw NativeWorkerStopped();
      },
      [this]{
        std::unique_lock lock(mutex);
        check(changed.wait_for(lock,std::chrono::seconds(5),[&]{return idle>0;}),"Actual worker did not reach first source wait");
        const auto target=idle+1;++epoch;changed.notify_all();
        check(changed.wait_for(lock,std::chrono::seconds(5),[&]{return idle>=target;}),"Actual worker did not retire source file job");
      }
    };
  }
};

}
int main(int argc,char **argv) {
 try {
  check(argc==3,"Supply unchanged TU1.5 early initializer fixture and selected executable");
  if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
#if !defined(__linux__)
  return 77;
#endif
  std::ifstream input(argv[1]);const auto fixture=J::parse(input);
  const auto dir=std::filesystem::path(argv[1]).parent_path();
  std::ifstream expected(dir/fixture.at("store_file").get<std::string>(),std::ios::binary);
  check(bool(expected),"Missing original initializer store stream");
  auto bank=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
  const auto stack=0x60000000u;
  bank.mount(stack,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"declared main initializer caller stack");
  std::size_t stores=0,final_bytes=0;
  const auto observe=[&](const SourcePhysicsContactRecordStore &s) {
   if(stack<=s.address && s.address<0x60110000u)return;
   std::array<std::byte,25> raw;check(bool(expected.read(reinterpret_cast<char *>(raw.data()),raw.size())),"Original initializer store stream exhausted");
   const auto integer=[](std::span<const std::byte>b){std::uint64_t v=0;for(unsigned i=0;i<b.size();++i)v|=std::uint64_t(std::to_integer<unsigned>(b[i]))<<(8*i);return v;};
   const auto b=std::span(raw);std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(8)<<s.instruction;
   if(fixture.at("canonical_instruction_map").at(key.str())!=integer(b.first(4)) || s.address!=integer(b.subspan(4,4)) ||
      s.before.size()!=integer(b.subspan(8,1)) || integer(s.before)!=integer(b.subspan(9,8)) || integer(s.written)!=integer(b.subspan(17,8))) {
    std::ostringstream error;error<<"Original initializer store "<<stores<<" differs at "<<std::hex<<s.instruction<<':'<<s.address
      <<" expected "<<integer(b.first(4))<<':'<<integer(b.subspan(4,4));
    throw std::runtime_error(error.str());
   }
   ++stores;
  };
  TemporaryDirectory files;Schedule schedule;std::vector<SourcePhysicsPoolResourceEvent> resources;
  {
   SourcePhysicsGameArenaNative arena(bank,{files.path,0x60100000u,{0x27f,0x1f80},schedule.services()}, {},observe);
   {
    SourcePhysicsPoolHostStorage host(bank,0x10000000u,[&](const auto &e){resources.push_back(e);});
    for(const auto &step:fixture.at("steps")) {
     const auto entry=std::stoul(step.at("entry").get<std::string>(),nullptr,16);
     if(entry==0x1c5f00u)arena.stop(); // Retire idle async bank user before synchronous construction.
     auto state=abi(step.at("entry_abi"));put(bank,state.gp[4],0x60200000u);unsigned index=0;
     for(const auto &arg:step.at("arguments"))put(bank,state.gp[4]+4+4*index++,arg);
     bool missing=false;
     try {
      if(entry==0x1c5f00u || entry==0x1f15b0u)
       source_run_physics_early_initializer_with_native_storage(bank,entry,state,{0x27f,0x1f80},{},host,observe);
      else arena.run(entry,state);
     }catch(const SourcePhysicsEarlyRendererUnavailable &e){
      check(e.call.instruction==0x1c5f4fu && e.call.target==fixture.at("missing_renderer_target") && !e.call.tail,"Different required renderer frontier");
      check(word(bank,state.gp[4])==fixture.at("missing_renderer_return"),"Actual renderer CALL return frame differs");missing=true;
     }
     check(missing==step.at("pending_renderer").get<bool>(),"Initializer completion/frontier differs");
     same_abi(state,step.at("exit_abi"),"Complete TU1.5 initializer phase ABI");
     check(stores==step.at("stores"),"Complete TU1.5 initializer phase write count");
    }
    check(stores==fixture.at("stores"),"Complete original initializer write count");
    char extra;check(!expected.read(&extra,1) && expected.eof(),"Unused original initializer store suffix");
    arena.stop();
    for(const auto &region:fixture.at("final_banks")) {
     const auto p=region.at("address").get<std::uint32_t>();const auto n=region.at("bytes").get<std::size_t>();
     check(digest(bank.view(p,n))==region.at("sha256").get<std::string>(),"Complete TU1.5 initializer bank differs at "+std::to_string(p));final_bytes+=n;
    }
    check(host.live_allocations()==4 && resources.size()==4,"Fresh impact/component/main storage count differs");
    unsigned i=0;for(const auto &event:fixture.at("services"))if(event.at("kind")=="virtual_allocate") {
     check(i<resources.size() && resources[i].allocation && J(resources[i].request)==event.at("arguments") && J(resources[i].result)==event.at("result"),"Actual native initializer page request/result differs");++i;
    }
    check(i==resources.size(),"Unused native initializer resource result");
    for(unsigned i=0;i<2;++i)check(std::filesystem::file_size(files.path/("savegame"+std::to_string(i)+".bin"))==fixture.at("cache_files")[i].at("bytes"),"Actual original savegame file extent differs");
   }
   for(const auto &e:resources){bool released=false;try{bank.view(e.result[1],1);}catch(const std::exception&){released=true;}check(released,"Synchronous initializer retained retired pool/main pages");}
  }
  bool released=false;try{bank.view(0x80061000,1);}catch(const std::exception&){released=true;}
  check(released,"Initializer startup retained retired physical pages");
  // A second fresh native startup verifies the first missing join, without
  // importing any arena/pool bank or contaminating the positive run.
  {
   auto fresh=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
   fresh.mount(stack,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"independent missing initializer owner stack");
   TemporaryDirectory negative_files;Schedule negative_schedule;
   SourcePhysicsGameArenaNative arena(fresh,{negative_files.path,0x60100000u,{0x27f,0x1f80},negative_schedule.services()});
   for(unsigned i=0;i<3;++i){const auto &step=fixture.at("steps")[i];auto state=abi(step.at("entry_abi"));put(fresh,state.gp[4],0x60200000u);arena.run(std::stoul(step.at("entry").get<std::string>(),nullptr,16),state);}
   arena.stop();auto state=abi(fixture.at("steps")[3].at("entry_abi"));put(fresh,state.gp[4],0x60200000u);
   bool stopped=false;try{source_run_physics_early_initializer_runtime(fresh,0x1c5f00,state,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},{});}catch(const std::runtime_error&){stopped=true;}
   check(stopped,"Missing original CRC owner did not stop");bool found=false;
   for(const auto &row:fixture.at("early_frontiers"))if(row.at("target")==0x166a80 && row.at("return_address")==0x1cb15f){same_abi(state,row.at("abi"),"Missing initializer CRC owner actual frontier");found=true;break;}
   check(found,"Original missing CRC frontier absent");
  }
  check(stores==1049789 && fixture.at("steps").size()==5,"Complete selected initializer corpus");
  std::cout<<"5 TU1.5 startup/initializer phases, "<<stores<<" ordered writes, "<<final_bytes<<" final bank bytes and four actual pool/main allocations match; original renderer owner remains required\n";
 } catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
