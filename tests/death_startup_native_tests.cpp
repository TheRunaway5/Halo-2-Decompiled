#include "halo2/death_startup_native.hpp"
#include "halo2/native_threads.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/source_runtime_cache_tags.hpp"
#include "halo2/cache.hpp"
#include "halo2/native_object_lifetimes.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <openssl/evp.h>
#include <zlib.h>
#include <algorithm>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>
#if defined(__linux__)
#include <unistd.h>
#endif

namespace {
using namespace halo2;
using Json=nlohmann::json;
void check(bool ok,const std::string &why){if(!ok)throw std::runtime_error(why);}
std::uint64_t integer(std::span<const std::byte> b){std::uint64_t v=0;for(unsigned i=0;i<b.size();++i)v|=std::uint64_t(std::to_integer<unsigned>(b[i]))<<(8*i);return v;}
void put(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t v){std::array<std::byte,4> b;for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);}
std::string digest(std::span<const std::byte> b){
  std::array<unsigned char,EVP_MAX_MD_SIZE> out;unsigned n=0;
  check(EVP_Digest(b.data(),b.size(),out.data(),&n,EVP_sha256(),nullptr)==1,"SHA256 unavailable");
  std::ostringstream s;for(unsigned i=0;i<n;++i)s<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(out[i]);return s.str();
}
struct PackedStores {
  gzFile file;
  explicit PackedStores(const std::filesystem::path &p):file(gzopen(p.c_str(),"rb")){check(file,"Original startup store file missing");}
  ~PackedStores(){gzclose(file);}
  std::array<std::byte,25> next(){std::array<std::byte,25> b;check(gzread(file,b.data(),b.size())==int(b.size()),"Original startup store stream exhausted");return b;}
  bool done(){char b;return gzread(file,&b,1)==0 && gzeof(file);}
};
struct TemporaryDirectory {
  std::filesystem::path path;
  TemporaryDirectory(){
#if defined(__linux__)
    std::string name=(std::filesystem::temp_directory_path()/"halo2-death-startup-XXXXXX").string();
    auto *p=::mkdtemp(name.data());check(p,"Native startup temporary file owner unavailable");path=p;
#endif
  }
  ~TemporaryDirectory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
struct Schedule {
  std::mutex mutex;
  std::condition_variable_any changed;
  unsigned idle=0,epoch=0;
  SourceDeathStartupNativeSchedule services(){
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
} // namespace
int main(int argc,char **argv){try {
  if((argc!=3 && argc!=4) || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
#if !defined(__linux__)
  return 77;
#endif
  std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
  auto bank=load_source_runtime_image_1_1(argv[2]);
  const auto stack=0x60000000u;
  bank.mount(stack,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,
             "explicit native startup main ABI scratch");
  PackedStores expected(std::filesystem::path(argv[1]).parent_path()/fixture.at("store_file").get<std::string>());
  std::size_t stores=0;
  SourcePhysicsContactRecordObserver observe=[&](const auto &s){
    if(s.address>=stack && s.address<0x60110000u)return;
    const auto raw=expected.next();const auto b=std::span(raw);
    const auto pc=integer(b.first(4)),address=integer(b.subspan(4,4)),size=integer(b.subspan(8,1));
    if(s.instruction!=pc || s.address!=address || s.before.size()!=size || s.written.size()!=size ||
       integer(s.before)!=integer(b.subspan(9,8)) || integer(s.written)!=integer(b.subspan(17,8))){
      std::ostringstream message;message<<"Original startup store "<<stores<<" differs: actual "<<std::hex
        <<s.instruction<<':'<<s.address<<" expected "<<pc<<':'<<address;throw std::runtime_error(message.str());
    }
    ++stores;
  };
  TemporaryDirectory files;Schedule schedule;
  SourceDeathStartupNative native(bank,{files.path,0x60100000u,{0x27f,0x1f80},schedule.services(),0x10000000u}, {},observe);
  for(const auto &step:fixture.at("steps")){
    if(step.value("cache_delivery_before",false)) {
      check(argc==4 && std::filesystem::is_regular_file(argv[3]),"Actual runtime map cache is required");
      Cache cache(argv[3]);const auto image=cache.load_source_runtime_cache_tag_image();
      const auto &delivery=fixture.at("cache_delivery");
      check(image.index==delivery.at("index") && image.table==delivery.at("table") && image.count==delivery.at("count") &&
            image.scenario==delivery.at("scenario") && image.globals==delivery.at("globals"),"Literal runtime cache header differs");
      for(std::size_t i=0;i<image.regions.size();++i) {
        const auto&r=image.regions[i];const auto &expected=delivery.at("regions").at(i);
        check(r.address==expected.at("address") && r.bytes.size()==expected.at("bytes") && digest(r.bytes)==expected.at("sha256").get<std::string>(),
              "Literal runtime cache bytes differ");
      }
      const auto physical_before=digest(bank.view(0x80061000,0x30e4000));
      const auto rejected_delivery=[&](auto invalid) {
        bool refused=false;try{source_bind_runtime_cache_tags_1_1(bank,invalid);}catch(const std::exception &){refused=true;}
        check(refused && digest(bank.view(0x80061000,0x30e4000))==physical_before,
              "Invalid cache delivery wrote the shared physical bank");
      };
      auto invalid=image;invalid.scenario^=0x10000u;rejected_delivery(invalid);
      invalid=image;invalid.regions[1].address=0x82d47000u;rejected_delivery(invalid);
      invalid=image;invalid.table+=16;rejected_delivery(invalid);
      source_bind_runtime_cache_tags_1_1(bank,image);
    }
    SourceUnitAnimationRuntimeAbi state{};state.eflags=2;state.gp[4]=stack+0xf000;
    put(native.memory(),state.gp[4],0x60200000u);
    if(step.contains("native_multiplayer_options")) {
      const auto &options=step.at("native_multiplayer_options");
      check(options.at("address")==stack+0x100 && options.at("kind")==2,
            "Resource startup options are not the declared native multiplayer input");
      put(bank,stack+0x100,2);
    }
    if(step.contains("registers")) {
      for(const auto &[name,value]:step.at("registers").items()) {
        if(name=="esi")state.gp[6]=value.get<std::uint32_t>();
        else if(name=="eax")state.gp[0]=value.get<std::uint32_t>();
        else throw std::runtime_error("Startup fixture names an undeclared ABI input register");
      }
    }
    const auto args=step.value("arguments",std::vector<std::uint32_t>{});
    for(std::size_t i=0;i<args.size();++i)put(bank,state.gp[4]+4+std::uint32_t(i)*4,args[i]);
    native.run(std::stoul(step.at("entry").get<std::string>(),nullptr,16),state);
    check(state.gp[4]==stack+0xf004+args.size()*4,"Complete original startup stack return differs");
    if(fixture.contains("resource_cache"))check(state.gp[0]==step.at("eax"),"Original model-resource startup return differs");
    check(stores==step.at("stores"),"Complete original startup phase store count differs");
  }
  native.stop();
  check(stores==fixture.at("stores") && expected.done(),"Complete startup store sequence differs");
  // Retired workers cannot accept another job or re-enter a source root.
  SourceUnitAnimationRuntimeAbi retired{};retired.eflags=2;retired.gp[4]=stack+0xf000;
  bool rejected=false;
  try{native.run(0x125280,retired);}catch(const std::runtime_error &){rejected=true;}
  check(rejected && stores==fixture.at("stores"),"Stopped startup owner accepted another source root");
  std::size_t final_bytes=0;
  for(const auto &row:fixture.at("final_banks")){
    const auto address=row.at("address").get<std::uint32_t>(),bytes=row.at("bytes").get<std::uint32_t>();
    check(digest(bank.view(address,bytes))==row.at("sha256").get<std::string>(),"Complete native startup bank differs at "+std::to_string(address));
    final_bytes+=bytes;
  }
  for(unsigned i=0;i<2;++i)check(std::filesystem::file_size(files.path/("savegame"+std::to_string(i)+".bin"))==0x40b000,
                              "Actual native savegame cache extent differs");
  const auto pool=fixture.at("object_pool").get<std::uint32_t>();
  check(std::to_integer<unsigned>(bank.view(pool+0x29,1)[0])==fixture.at("object_pool_valid"),
        "Original startup/map reset pool validity differs");
  if(fixture.at("object_pool_valid")==1) {
    // Native identities and direct source allocations share the actual pool,
    // including reuse salts, cursor, occupancy and the exact header payload.
    NativeObjectLifetimes lifetimes(bank,pool);
    const auto h=lifetimes.reserve(5);lifetimes.publish_native(h,0xffffffffu);
    check(lifetimes.pool().count()==1 && integer(bank.view(pool+0x3c,4))==1,
          "Native identity view allocated a parallel header pool");
    const auto elements=integer(bank.view(pool+0x44,4));
    lifetimes.header(h)[4]=0x57;
    check(bank.view(std::uint32_t(elements)+(h&65535u)*12+4,1)[0]==std::byte{0x57},
          "Native header view copied the source header bytes");
    SourceRawDatumPoolServices storage{[&](auto p,auto n){return bank.mutable_view(p,n);},
                                       [&](auto p,auto n){return bank.view(p,n);}};
    const auto direct=source_raw_datum_allocate(pool,storage);
    check((direct&65535u)==1 && direct!=h && lifetimes.pool().count()==2,
          "Direct source allocation did not consume the shared cursor/salt");
    source_raw_datum_release_unchecked(pool,direct,storage);lifetimes.release(h);
    check(lifetimes.pool().count()==0 && lifetimes.header(h).empty(),"Shared source identity retirement failed");
    const auto reused=lifetimes.reserve(0);
    check(reused!=h && (reused&65535u)==(h&65535u),"Shared source pool lost reuse salt");
    bool copy_rejected=false;try{auto copied=lifetimes;(void)copied;}catch(const std::logic_error &){copy_rejected=true;}
    check(copy_rejected,"Raw source identities aliased a value checkpoint");
    lifetimes.release(reused);
  }
  // A missing physical resource must stop after actual source startup writes.
  auto missing=load_source_runtime_image_1_1(argv[2]);
  missing.mount(stack,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"missing-owner experiment ABI");
  SourceUnitAnimationRuntimeAbi state{};state.eflags=2;state.gp[4]=stack+0xf000;put(missing,state.gp[4],0x60200000u);
  bool stopped=false;
  try{source_run_death_startup_1_1(missing,0x12cb70,state,{0x27f,0x1f80},{});}catch(const std::runtime_error &){stopped=true;}
  check(stopped && integer(missing.view(0x4ed9fc,4))==0x61000 && integer(missing.view(0x4eda10,4))==0x3145000,
        "Missing physical owner changed the original prefix/stop boundary");
  std::cout<<"Complete original physical/async/arena/object startup: "<<stores<<" ordered retained stores, "
           <<final_bytes<<" full-bank bytes; two actual files and native worker stop/join match\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
