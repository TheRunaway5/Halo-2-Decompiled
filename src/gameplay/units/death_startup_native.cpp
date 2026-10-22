#include "halo2/death_startup_native.hpp"
#include "halo2/native_threads.hpp"
#include "platform/native_startup_resources.hpp"
#include "halo2/source_runtime_host_storage.hpp"
#include <atomic>
#include <bit>
#include <cerrno>
#include <cstdio>
#include <future>
#include <mutex>
#include <sstream>
#include <thread>
#if defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
#include <sched.h>
#include <sys/stat.h>
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
struct SourceDeathStartupNative::Impl {
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
  SourceDeathStartupNativeConfiguration config;
  SourceDeathWorldSchedulerServices lower;
  SourcePhysicsContactRecordObserver observe;
  SourceRuntimeHostPhysicalMemory physical;
  SourceRuntimeHostStorage scratch;
  std::unique_ptr<SourceRuntimeHostStorage> static_storage;
  NativeSynchronization synchronization{{0xf3000001u,0xf3000100ull}};
  NativeSynchronizationServices sync = synchronization.services();
  std::map<std::uint32_t,int> files;
  std::uint32_t next_file = 0xf4000001u, worker = 0;
  bool cache_files_bound = false;
  std::uint32_t worker_stack = 0;
  std::atomic<bool> stopped = false;
  native_startup_resources::Worker native_worker;
  // Declared last: stop/join actual workers before captured resources retire.
  NativeThreads threads{{0xf5000001u,0xf5000100ull}};
  NativeThreadServices workers = threads.services();

  Impl(SourceRuntimeMemory &m, SourceDeathStartupNativeConfiguration c,
       SourceDeathWorldSchedulerServices s, SourcePhysicsContactRecordObserver o)
      :shared(m),config(std::move(c)),lower(std::move(s)),observe(std::move(o)),
       physical(m),scratch(m,config.worker_stack_address) {
    require(!config.savegame_directory.empty() && config.worker_stack_address,
            "Native startup needs explicit file and private-stack owners");
#if !defined(__linux__) && !defined(__APPLE__)
    throw std::runtime_error("Native death startup platform is unavailable");
#endif
  }
  ~Impl() {
    try { stop(); } catch (...) {}
#if defined(__linux__) || defined(__APPLE__)
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
  void bind_cache_files(const std::array<std::filesystem::path,4> &paths) {
    check_worker();
    require(!cache_files_bound,"Native imported cache files are already mounted");
#if defined(__linux__) || defined(__APPLE__)
    std::array<int,4> opened{-1,-1,-1,-1};
    try {
      for(unsigned i=0;i<opened.size();++i) {
        opened[i]=::open(paths[i].c_str(),O_RDONLY|O_CLOEXEC);
        if(opened[i]<0)throw std::system_error(errno,std::generic_category(),
            "Native imported cache open: "+paths[i].string());
      }
      std::lock_guard lock(shared.mutex);
      // Explicit native current-file slot3. The other three selectors are
      // literal19D8C0 operands, independent of the Xbox staging algorithm.
      constexpr std::array<std::uint32_t,4> cells{
          0x55f798u+3u*0x804u,0x5607a0u,0x55f798u,0x55ff9cu};
      for(auto cell:cells)shared.bank.mutable_view(cell,4);
      shared.bank.mutable_view(0x5627b0u,4);
      for(unsigned i=0;i<opened.size();++i) {
        const auto handle=next_file++;
        files.emplace(handle,opened[i]);opened[i]=-1;
        put(shared,cells[i],handle);
      }
      put(shared,0x5627b0u,3u);cache_files_bound=true;
    } catch(...) {
      for(auto fd:opened)if(fd>=0)::close(fd);
      throw;
    }
#else
    throw std::runtime_error("Native imported cache file mount unavailable");
#endif
  }
  SourceDeathWorldSchedulerServices services(std::stop_token stop = {}) {
    auto joined = lower;
    joined.runtime.required_owner = [this,stop](auto &m,const auto &call,auto &abi,const auto &o) {
      require(&m==&shared,"Native death startup shared bank owner differs");
      if (platform(call,abi,stop)) return;
      if (!lower.runtime.required_owner) {
        std::ostringstream message;
        message << "Required original startup platform/resource owner unavailable at "
                << std::hex << call.instruction << " -> " << call.target;
        throw std::runtime_error(message.str());
      }
      lower.runtime.required_owner(m,call,abi,o);
    };
    return joined;
  }
  bool platform(const SourceUnitAnimationRuntimeCall &call,
                SourceUnitAnimationRuntimeAbi &abi,std::stop_token stop) {
    if(stopped)throw NativeWorkerStopped();
    {
      std::lock_guard lock(shared.mutex);
      if (source_try_native_death_physical_call_1_1(shared.bank,call,abi,physical)) return true;
    }
    const auto sp=abi.gp[4];
    const auto arg=[&](unsigned i){return word(shared,sp+4+i*4);};
    const auto finish=[&](unsigned bytes,std::optional<std::uint32_t> result={}) {
      (void)word(shared,sp); // Require actual readable return backing.
      if(result) abi.gp[0]=*result;
      abi.gp[4]=sp+4+bytes;
    };
    if(!call.tail && call.instruction==0x2d84c4u && call.target==0x800000eeu) {
      require(word(shared,sp)==0x2d84cau,"Original kernel-yield return frame differs");
      // Actual no-argument NtYieldExecution boundary. Native successful
      // sched_yield maps to status0; original2D84CA..2D84D6 retains its own
      // status-to-BOOL conversion. This does not reproduce Xbox scheduling.
      if(config.schedule.before_main_yield)config.schedule.before_main_yield();
#if defined(__linux__) || defined(__APPLE__)
      if(::sched_yield()!=0)throw std::system_error(errno,std::generic_category(),"Native source yield");
#else
      throw std::runtime_error("Native source kernel-yield status owner unavailable");
#endif
      check_worker();finish(0,0);return true;
    }
    if (call.tail) return false;
    switch(call.target) {
    case 0x800000b8u: {
      if(call.instruction!=0x33786fu)return false;
      require(word(shared,sp)==0x337875u && arg(1)==0 &&
              arg(3)==0x101000u && arg(4)==4,
              "Reached static virtual allocation frame differs");
      require(config.static_allocation_address,
              "Native static virtual allocation owner unavailable");
      const auto base_cell=arg(0),size_cell=arg(2);
      std::lock_guard lock(shared.mutex);
      shared.bank.mutable_view(base_cell,4);shared.bank.mutable_view(size_cell,4);
      if(!static_storage)static_storage=std::make_unique<SourceRuntimeHostStorage>(
          shared.bank,config.static_allocation_address);
      const auto result=static_storage->allocate({word(shared,base_cell),arg(1),
          word(shared,size_cell),arg(3),arg(4)});
      put(shared,base_cell,result.base);put(shared,size_cell,result.bytes);
      finish(20,result.status);return true;
    }
    case 0x2dc83a: {
      require(arg(0)==0 && arg(1)==0 && arg(2)==0,"Original async mutex request differs");
      const auto result=sync.create_mutex(false);
      if(result.error)throw std::system_error(result.error);
      finish(12,result.handle);return true;
    }
    case 0x2dc7b9: {
      require(arg(0)==0 && arg(1)==0 && arg(2)==150 && arg(3)==0,"Original async semaphore request differs");
      const auto result=sync.create_semaphore(0,150);
      if(result.error)throw std::system_error(result.error);
      finish(16,result.handle);return true;
    }
    case 0x2d8606: {
      require(!worker && arg(0)==0 && arg(1)==0x4000 && arg(2)==0x122350 &&
              arg(3)==0 && arg(4)==0 && arg(5)==0,"Original async thread request differs");
      {
        std::lock_guard lock(shared.mutex);
        worker_stack=scratch.allocate({0,0,0x10000,0x101000,4}).base;
      }
      std::promise<native_startup_resources::Worker> id;
      auto ready=id.get_future();
      const auto result=workers.create([this,&id](std::stop_token stopped) {
        try {id.set_value(native_startup_resources::current_worker());}
        catch(...) {id.set_exception(std::current_exception());throw;}
        SourceUnitAnimationRuntimeAbi state{};
        state.eflags=2;
        // Original122350 is CALL122360 followed by an unreachable INT3.
        // This explicit native thread ABI supplies private scratch only.
        state.gp[4]=worker_stack+0xf000;
        put(shared,state.gp[4],0x60200000u);
        source_run_death_startup_1_1(shared,0x122350,state,
            config.floating_point,services(stopped),observe);
        throw std::runtime_error("Original startup thread reached INT3 after worker return");
      });
      if(result.error)throw std::system_error(result.error);
      worker=result.handle;
      native_worker=ready.get();
      finish(24,worker);return true;
    }
    case 0x2d83f6: {
      require(arg(0)==worker && arg(1)==1 && native_worker,
              "Original async priority request differs");
#if defined(__linux__) || defined(__APPLE__)
      // Named native priority policy with the real host success/failure BOOL;
      // the original12226A caller consumes no BOOL or last-error value.
      const auto result=native_startup_resources::raise_worker_priority(native_worker);
      finish(8,std::uint32_t(result));return true;
#else
      throw std::runtime_error("Native worker priority operation unavailable");
#endif
    }
    case 0x2dc9c8: {
      const auto h=arg(0),ms=arg(1);
      require(ms==0xffffffffu,"Original async wait request differs");
      if(stop.stop_possible() && config.schedule.before_worker_wait)
        config.schedule.before_worker_wait(h,stop);
      acquire_native_synchronization(sync,h,ms,stop,5);
      finish(8,0);return true;
    }
    case 0x2dc892:
      if(auto error=sync.release_mutex(arg(0)))throw std::system_error(error);
      finish(4,1);return true;
    case 0x2dc814: {
      require(arg(1)==1,"Original async semaphore count differs");
      const auto result=sync.release_semaphore(arg(0),1);
      if(result.error)throw std::system_error(result.error);
      require(result.previous_count.has_value(),"Native semaphore release has no real outcome");
      if(arg(2))put(shared,arg(2),std::uint32_t(*result.previous_count));
      finish(12,1);return true;
    }
    case 0x2d84c4:
      if(config.schedule.before_main_yield)config.schedule.before_main_yield();
      std::this_thread::yield();check_worker();finish(0);return true;
    case 0x327f10: {
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
    case 0x2d8b90: {
      require(arg(1)==0xc0000000u && !arg(2) && !arg(3) && arg(4)==4 && !arg(5) && !arg(6),
              "Reached startup CreateFile argument owner unavailable");
      const auto name=string(shared,arg(0));
      require(name=="n:\\savegame0.bin" || name=="n:\\savegame1.bin",
              "Reached source file namespace owner unavailable");
#if defined(__linux__) || defined(__APPLE__)
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
    case 0x2d88c8: {
      const auto found=files.find(arg(0));
      require(found!=files.end() && arg(2) && word(shared,arg(2))==0 && arg(3)==0,
              "Reached startup seek owner unavailable");
#if defined(__linux__) || defined(__APPLE__)
      const auto offset=::lseek(found->second,arg(1),SEEK_SET);
      require(offset<=UINT32_MAX,"Native source file position exceeds reached ABI");
      finish(16,offset<0?0xffffffffu:std::uint32_t(offset));return true;
#else
      throw std::runtime_error("Native source file seek unavailable");
#endif
    }
    case 0x2d8844: {
      const auto found=files.find(arg(0));require(found!=files.end(),"Reached startup file extent owner unavailable");
#if defined(__linux__) || defined(__APPLE__)
      const auto position=::lseek(found->second,0,SEEK_CUR);
      const auto ok=position>=0 && ::ftruncate(found->second,position)==0;
      finish(4,std::uint32_t(ok));return true;
#else
      throw std::runtime_error("Native source file extent operation unavailable");
#endif
    }
    case 0x2d8681: {
      const auto found=files.find(arg(0));
      require(found!=files.end() && arg(3) && arg(4)==0,
              "Reached synchronous ReadFile owner unavailable");
#if defined(__linux__) || defined(__APPLE__)
      const auto destination=arg(1),count=arg(2),read_cell=arg(3);
      // Validate actual retained backing before advancing the real file.
      {std::lock_guard lock(shared.mutex);
        shared.bank.mutable_view(destination,count);shared.bank.mutable_view(read_cell,4);}
      std::vector<std::byte> bytes(count);
      ssize_t n;
      do { n=::read(found->second,bytes.data(),bytes.size()); } while(n<0 && errno==EINTR);
      if(n>=0)shared.write(destination,std::span(bytes).first(std::size_t(n)));
      put(shared,read_cell,n<0?0u:std::uint32_t(n));
      finish(20,n<0?0u:1u);return true;
#else
      throw std::runtime_error("Native source file reading unavailable");
#endif
    }
    default:return false;
    }
  }
};
SourceDeathStartupNative::SourceDeathStartupNative(SourceRuntimeMemory &m,
    SourceDeathStartupNativeConfiguration c,SourceDeathWorldSchedulerServices s,
    SourcePhysicsContactRecordObserver o)
    :impl_(std::make_unique<Impl>(m,std::move(c),std::move(s),std::move(o))){}
SourceDeathStartupNative::~SourceDeathStartupNative()=default;
SourcePhysicsContactMemory &SourceDeathStartupNative::memory(){return impl_->shared;}
SourceDeathWorldSchedulerServices SourceDeathStartupNative::services(){return impl_->services();}
void SourceDeathStartupNative::bind_cache_files(const std::array<std::filesystem::path,4> &paths){impl_->bind_cache_files(paths);}
void SourceDeathStartupNative::run(std::uint32_t entry,SourceUnitAnimationRuntimeAbi &abi){
  impl_->check_worker();
  if(source_death_startup_owns_entry_1_1(entry))
    source_run_death_startup_1_1(impl_->shared,entry,abi,impl_->config.floating_point,
                               impl_->services(),impl_->observe);
  else
    source_run_death_object_phases_1_1(impl_->shared,entry,abi,impl_->config.floating_point,
                                     impl_->services(),impl_->observe);
}
void SourceDeathStartupNative::stop(){impl_->stop();}
} // namespace halo2
