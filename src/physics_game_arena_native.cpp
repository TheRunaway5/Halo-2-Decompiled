#include "halo2/physics_game_arena_native.hpp"
#include "halo2/physics_physical_storage_runtime.hpp"
#include "physics_game_arena_call_frames.hpp"
#include "halo2/native_threads.hpp"
#include "halo2/source_runtime_host_storage.hpp"
#include <atomic>
#include <bit>
#include <cerrno>
#include <cstdio>
#include <future>
#include <mutex>
#include <sstream>
#include <thread>
#if defined(__linux__)
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory &m, std::uint32_t p) {
  std::array<std::byte,4> raw;
  m.read(p, raw);
  std::uint32_t v = 0;
  for (unsigned i = 0; i < 4; ++i) v |= std::to_integer<std::uint32_t>(raw[i]) << (8*i);
  return v;
}
void put(SourcePhysicsContactMemory &m, std::uint32_t p, std::uint32_t v) {
  std::array<std::byte,4> raw;
  for (unsigned i = 0; i < 4; ++i) raw[i] = std::byte(v >> (8*i));
  m.write(p, raw);
}
std::string string(const SourcePhysicsContactMemory &m, std::uint32_t p) {
  std::string out;
  for (unsigned i = 0; i < 4096; ++i) {
    std::array<std::byte,1> raw;
    m.read(p+i, raw);
    if (raw[0] == std::byte{}) return out;
    out.push_back(char(std::to_integer<unsigned char>(raw[0])));
  }
  throw std::runtime_error("Original startup platform string is unterminated");
}
void require(bool ok, const char *why) {
  if (!ok) throw std::invalid_argument(why);
}
}
struct SourcePhysicsGameArenaNative::Impl {
  struct SharedMemory final : SourcePhysicsContactMemory {
    SourceRuntimeMemory &bank;
    mutable std::recursive_mutex mutex;
    explicit SharedMemory(SourceRuntimeMemory &m):bank(m){}
    void read(std::uint32_t p, std::span<std::byte> b) const override {
      std::lock_guard lock(mutex); bank.read(p,b);
    }
    void write(std::uint32_t p, std::span<const std::byte> b) override {
      std::lock_guard lock(mutex); bank.write(p,b);
    }
  } shared;
  SourcePhysicsGameArenaConfiguration config;
  SourcePhysicsWorldRuntimeServices lower;
  SourcePhysicsContactRecordObserver observe;
  SourceRuntimeHostPhysicalMemory physical;
  SourceRuntimeHostStorage scratch;
  NativeSynchronization synchronization{{0xf3000001u,0xf3000100ull}};
  NativeSynchronizationServices sync = synchronization.services();
  std::map<std::uint32_t,int> files;
  std::uint32_t next_file = 0xf4000001u, worker = 0;
  std::uint32_t worker_stack = 0;
  std::atomic<bool> stopped = false;
  std::int64_t native_worker_id = 0;
  // Declared last: stop/join actual workers before captured resources retire.
  NativeThreads threads{{0xf5000001u,0xf5000100ull}};
  NativeThreadServices workers = threads.services();

  Impl(SourceRuntimeMemory &m, SourcePhysicsGameArenaConfiguration c,
       SourcePhysicsWorldRuntimeServices s, SourcePhysicsContactRecordObserver o)
      :shared(m),config(std::move(c)),lower(std::move(s)),observe(std::move(o)),
       physical(m),scratch(m,config.worker_stack_address) {
    require(!config.savegame_directory.empty() && config.worker_stack_address,
            "Native startup needs explicit file and private-stack owners");
#if !defined(__linux__)
    throw std::runtime_error("Native arena startup platform is unavailable");
#endif
  }
  ~Impl() {
    try { stop(); } catch (...) {}
#if defined(__linux__)
    for (auto [alias,fd]:files) ::close(fd);
#endif
    if (worker_stack) {
      try { shared.bank.release(worker_stack); } catch (...) {}
    }
  }
  void check_worker(bool teardown = false) {
    if (stopped && !teardown)
      throw std::runtime_error("Native source startup owner has stopped");
    if (!worker) return;
    const auto state = workers.outcome(worker);
    if (state.state == NativeThreadState::Failed) {
      if (state.failure) std::rethrow_exception(state.failure);
      throw std::runtime_error("Native source startup worker failed");
    }
    if (state.state == NativeThreadState::Returned)
      throw std::runtime_error("Original immortal source worker returned");
  }
  void stop() {
    stopped = true;
    if (!worker) return;
    if (auto e=workers.request_stop(worker)) throw std::system_error(e);
    if (auto e=workers.join(worker)) throw std::system_error(e);
    check_worker(true);
  }
  SourcePhysicsWorldRuntimeServices services(std::stop_token stop = {}) {
    auto joined = lower;
    joined.required_owner = [this,stop](auto &m,const auto &call,auto &abi,const auto &o) {
      require(&m==&shared,"Native arena startup shared bank owner differs");
      bool authenticated=false;
      for(const auto &frame:physics_game_arena_frames::frames)
        if(frame.site==call.instruction && frame.target==call.target && !call.tail &&
            word(shared,abi.gp[4])==frame.return_address)authenticated=true;
      require(authenticated,"Original TU1.5 arena resource call frame differs");
      if(call.target==0x21b920u) {
        std::lock_guard lock(shared.mutex);
        source_run_physics_physical_storage_with_native_pages(shared.bank,0x217020,abi,
            config.floating_point,lower,physical,o);return;
      }
      if (platform(call,abi,stop)) return;
      if (!lower.required_owner) {
        std::ostringstream message;
        message << "Required original startup platform/resource owner unavailable at "
                << std::hex << call.instruction << " -> " << call.target;
        throw std::runtime_error(message.str());
      }
      lower.required_owner(m,call,abi,o);
    };
    return joined;
  }
  bool platform(const SourcePhysicsContactRuntimeCall &call,
                SourcePhysicsWorldRuntimeAbi &abi,std::stop_token stop) {
    const auto sp=abi.gp[4];
    const auto arg=[&](unsigned i){return word(shared,sp+4+i*4);};
    const auto finish=[&](unsigned bytes,std::optional<std::uint32_t> result={}) {
      (void)word(shared,sp); // Require actual readable return backing.
      if(result) abi.gp[0]=*result;
      abi.gp[4]=sp+4+bytes;
    };
    if (call.tail) return false;
    switch(call.target) {
    case 0x2e02ea: {
      require(arg(0)==0 && arg(1)==0 && arg(2)==0,"Original async mutex request differs");
      const auto result=sync.create_mutex(false);
      if(result.error)throw std::system_error(result.error);
      finish(12,result.handle);return true;
    }
    case 0x2e0269: {
      require(arg(0)==0 && arg(1)==0 && arg(2)==150 && arg(3)==0,"Original async semaphore request differs");
      const auto result=sync.create_semaphore(0,150);
      if(result.error)throw std::system_error(result.error);
      finish(16,result.handle);return true;
    }
    case 0x2dc0b6: {
      require(!worker && arg(0)==0 && arg(1)==0x4000 && arg(2)==0x1235f0 &&
              arg(3)==0 && arg(4)==0 && arg(5)==0,"Original async thread request differs");
      {
        std::lock_guard lock(shared.mutex);
        worker_stack=scratch.allocate({0,0,0x10000,0x101000,4}).base;
      }
      std::promise<std::int64_t> id;
      auto ready=id.get_future();
      const auto result=workers.create([this,&id](std::stop_token stopped) {
#if defined(__linux__)
        id.set_value(::syscall(SYS_gettid));
#else
        id.set_value(0);
#endif
        SourcePhysicsWorldRuntimeAbi state{};
        state.eflags=2;
        // Original122350 is CALL122360 followed by an unreachable INT3.
        // This explicit native thread ABI supplies private scratch only.
        state.gp[4]=worker_stack+0xf000;
        put(shared,state.gp[4],0x60200000u);
        source_run_physics_game_arena_runtime(shared,0x122350,state,
            SourcePhysicsContactImage::TitleUpdate15,config.floating_point,services(stopped),observe);
        throw std::runtime_error("Original startup thread reached INT3 after worker return");
      });
      if(result.error)throw std::system_error(result.error);
      worker=result.handle;
      native_worker_id=ready.get();
      finish(24,worker);return true;
    }
    case 0x2dbea6: {
      require(arg(0)==worker && arg(1)==1 && native_worker_id,
              "Original async priority request differs");
#if defined(__linux__)
      // Named native priority policy. Actual EPERM/EACCES returns FALSE; the
      // original12226A caller consumes no BOOL or last-error value.
      const auto result=::setpriority(PRIO_PROCESS,id_t(native_worker_id),-1)==0;
      finish(8,std::uint32_t(result));return true;
#else
      throw std::runtime_error("Native worker priority operation unavailable");
#endif
    }
    case 0x2e0478: {
      const auto h=arg(0),ms=arg(1);
      require(ms==0xffffffffu,"Original async wait request differs");
      if(stop.stop_possible() && config.schedule.before_worker_wait)
        config.schedule.before_worker_wait(h,stop);
      acquire_native_synchronization(sync,h,ms,stop,5);
      finish(8,0);return true;
    }
    case 0x2e0342:
      if(auto error=sync.release_mutex(arg(0)))throw std::system_error(error);
      finish(4,1);return true;
    case 0x2e02c4: {
      require(arg(1)==1,"Original async semaphore count differs");
      const auto result=sync.release_semaphore(arg(0),1);
      if(result.error)throw std::system_error(result.error);
      require(result.previous_count.has_value(),"Native semaphore release has no real outcome");
      if(arg(2))put(shared,arg(2),std::uint32_t(*result.previous_count));
      finish(12,1);return true;
    }
    case 0x2dbf74:
      if(config.schedule.before_main_yield)config.schedule.before_main_yield();
      std::this_thread::yield();check_worker();finish(0);return true;
    case 0x32ba80: {
      const auto destination=arg(0),capacity=arg(1),arguments=arg(3);
      const auto pattern=string(shared,arg(2));
      require(pattern=="n:\\savegame%d.bin" || pattern=="cluster %s" ||
              pattern=="%s cluster" || pattern=="%s reference",
              "Reached startup CRT format owner unavailable");
      require(capacity && capacity<=4096,"Reached startup CRT extent owner unavailable");
      std::vector<char> output(capacity);
      int n;
      if(pattern.find("%s")!=std::string::npos) {
        const auto value=string(shared,word(shared,arguments));
        n=std::snprintf(output.data(),output.size(),pattern.c_str(),value.c_str());
      } else n=std::snprintf(output.data(),output.size(),pattern.c_str(),
                            std::bit_cast<std::int32_t>(word(shared,arguments)));
      require(n>=0 && std::uint32_t(n)<capacity,"Reached truncating startup CRT owner unavailable");
      shared.write(destination,std::as_bytes(std::span(output).first(std::size_t(n)+1)));
      finish(0,std::uint32_t(n));return true; // cdecl caller owns its four args.
    }
    case 0x2dc640: {
      require(arg(1)==0xc0000000u && !arg(2) && !arg(3) && arg(4)==4 && !arg(5) && !arg(6),
              "Reached startup CreateFile argument owner unavailable");
      const auto name=string(shared,arg(0));
      require(name=="n:\\savegame0.bin" || name=="n:\\savegame1.bin",
              "Reached source file namespace owner unavailable");
#if defined(__linux__)
      const auto path=config.savegame_directory/name.substr(3);
      const auto fd=::open(path.c_str(),O_RDWR|O_CREAT|O_CLOEXEC,0600);
      if(fd<0){finish(28,0xffffffffu);return true;}
      const auto alias=next_file++;
      try { files.emplace(alias,fd); } catch (...) { ::close(fd);throw; }
      finish(28,alias);return true;
#else
      throw std::runtime_error("Native source file opening unavailable");
#endif
    }
    case 0x2dc378: {
      const auto found=files.find(arg(0));
      require(found!=files.end() && arg(2) && word(shared,arg(2))==0 && arg(3)==0,
              "Reached startup seek owner unavailable");
#if defined(__linux__)
      const auto offset=::lseek(found->second,arg(1),SEEK_SET);
      require(offset<=UINT32_MAX,"Native source file position exceeds reached ABI");
      finish(16,offset<0?0xffffffffu:std::uint32_t(offset));return true;
#else
      throw std::runtime_error("Native source file seek unavailable");
#endif
    }
    case 0x2dc2f4: {
      const auto found=files.find(arg(0));require(found!=files.end(),"Reached startup file extent owner unavailable");
#if defined(__linux__)
      const auto position=::lseek(found->second,0,SEEK_CUR);
      const auto ok=position>=0 && ::ftruncate(found->second,position)==0;
      finish(4,std::uint32_t(ok));return true;
#else
      throw std::runtime_error("Native source file extent operation unavailable");
#endif
    }
    default:return false;
    }
  }
};
SourcePhysicsGameArenaNative::SourcePhysicsGameArenaNative(SourceRuntimeMemory &m,
    SourcePhysicsGameArenaConfiguration c,const SourcePhysicsWorldRuntimeServices &s,
    const SourcePhysicsContactRecordObserver &o)
    :impl_(std::make_unique<Impl>(m,std::move(c),s,o)){}
SourcePhysicsGameArenaNative::~SourcePhysicsGameArenaNative()=default;
SourcePhysicsContactMemory &SourcePhysicsGameArenaNative::memory(){return impl_->shared;}
void SourcePhysicsGameArenaNative::run(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi){
  impl_->check_worker();
  if(entry==0x12cb70u) {
    std::lock_guard lock(impl_->shared.mutex);
    source_run_physics_physical_storage_with_native_pages(impl_->shared.bank,entry,abi,
        impl_->config.floating_point,impl_->lower,impl_->physical,impl_->observe);
    return;
  }
  source_run_physics_game_arena_runtime(impl_->shared,entry,abi,SourcePhysicsContactImage::TitleUpdate15,
      impl_->config.floating_point,impl_->services(),impl_->observe);
}
void SourcePhysicsGameArenaNative::stop(){impl_->stop();}
void SourcePhysicsGameArenaNative::protect_pages(std::uint32_t p,std::uint32_t n,std::uint32_t protection){
  std::lock_guard lock(impl_->shared.mutex);
  impl_->physical.protect(p,n,protection);
}
} // namespace halo2
