#include "halo2/physics_game_arena_native.hpp"
#include "halo2/physics_physical_storage_runtime.hpp"
#include "physics/physics_game_arena_call_frames.hpp"
#include "halo2/native_threads.hpp"
#include "platform/native_startup_resources.hpp"
#include "halo2/source_runtime_host_storage.hpp"
#include <atomic>
#include <bit>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <mutex>
#include <sstream>
#include <thread>
#include <algorithm>
#include <limits>
#include <system_error>
#include <utility>
#include <set>
#include <optional>
#if defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
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
constexpr auto worker_stack_owner="actual host arena worker stack";
constexpr std::uint32_t first_file=0xf4000001u;
std::span<const std::byte> saved_bytes(const SourceRuntimeMemory::Checkpoint &bank,
                                     std::uint32_t address,std::size_t bytes) {
  const auto at=std::upper_bound(bank.regions.begin(),bank.regions.end(),address,
      [](auto p,const auto &region){return p<region.address;});
  require(at!=bank.regions.begin(),"Arena checkpoint lacks required source backing");
  const auto &region=*std::prev(at);const auto offset=std::uint64_t(address)-region.address;
  require(offset<=region.bytes.size() && bytes<=region.bytes.size()-offset,
          "Arena checkpoint source extent differs");
  if(region.page_bytes && bytes)
    for(auto page=offset/region.page_bytes;page<=(offset+bytes-1)/region.page_bytes;++page)
      require(region.committed[page]!=0,"Arena checkpoint source extent is uncommitted");
  return std::span(region.bytes).subspan(offset,bytes);
}
std::uint32_t saved_word(const SourceRuntimeMemory::Checkpoint &bank,std::uint32_t address) {
  const auto bytes=saved_bytes(bank,address,4);std::uint32_t value=0;
  for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
  return value;
}
void validate_arena_ownership(const SourcePhysicsGameArenaNative::OwnershipCheckpoint &saved,
                              const SourceRuntimeMemory::Checkpoint &bank) {
  SourceRuntimeMemory::validate_checkpoint(bank);
  require(saved.version==1 && saved.worker && saved.worker_stack && saved.worker_semaphore,
          "Unsupported or unfinished arena checkpoint");
  require(saved_word(bank,0x4ecc24u)==saved.worker && saved_word(bank,0x4ecc30u)==saved.worker_semaphore &&
          saved_word(bank,0x4ef1bcu)==0,"Arena checkpoint worker/queue binding differs");
  require(std::count_if(bank.regions.begin(),bank.regions.end(),
      [](const auto &region){return region.owner==worker_stack_owner;})==1,"Arena checkpoint worker-stack owner differs");
  const auto stack=std::lower_bound(bank.regions.begin(),bank.regions.end(),saved.worker_stack,
      [](const auto &region,auto address){return region.address<address;});
  require(stack!=bank.regions.end() && stack->address==saved.worker_stack && stack->owner==worker_stack_owner &&
          stack->bytes.size()==0x10000u && stack->access==SourceRuntimeMemory::Access::ReadWrite &&
          !stack->page_bytes && stack->committed.empty() &&
          std::uint64_t(saved.worker_stack)+stack->bytes.size()==saved.scratch.next,
          "Arena checkpoint scratch extent/cursor differs");
  require(saved.synchronization.objects.size()==3,"Arena checkpoint synchronization topology differs");
  std::set<std::uint32_t> handles;
  for(const auto address:{0x4ecc28u,0x4ecc2cu,0x4ecc30u}) {
    const auto handle=saved_word(bank,address);
    require(handles.insert(handle).second,"Arena checkpoint duplicates a synchronization binding");
    const auto object=std::ranges::find(saved.synchronization.objects,handle,&NativeSynchronization::ObjectCheckpoint::handle);
    require(object!=saved.synchronization.objects.end(),"Arena checkpoint lacks a bound synchronization object");
    if(address==0x4ecc30u)
      require(object->kind==NativeSynchronization::ObjectKind::Semaphore && !object->count && object->limit==150,
              "Arena checkpoint has pending worker permits");
    else require(object->kind==NativeSynchronization::ObjectKind::Mutex,"Arena checkpoint mutex binding differs");
  }
  require(saved.files.size()==2 && saved.next_file==std::uint64_t(first_file)+saved.files.size(),
          "Arena checkpoint file namespace differs");
  std::set<std::string> names;auto expected=first_file;
  for(const auto &file:saved.files) {
    require(file.alias==expected++ && (file.name=="savegame0.bin" || file.name=="savegame1.bin") &&
            names.insert(file.name).second && file.bytes.size()<=UINT32_MAX,
            "Arena checkpoint file identity or extent differs");
  }
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
  mutable std::recursive_mutex operation_gate;
  std::size_t active_calls{};
  bool staging{};
  struct RestoreBarrier {
    Impl &owner;
    explicit RestoreBarrier(Impl &owner):owner(owner) {
      require(!owner.active_calls && !owner.staging,"Arena already has an active call or staged restore");
      owner.staging=true;
    }
    ~RestoreBarrier(){owner.staging=false;}
  };
  // Lower source callbacks can reenter protect_pages during run(). Retain a
  // recursive call gate and an explicit depth; reentrant checkpoints must
  // still reject instead of mistaking same-thread lock ownership for a barrier.
  struct ActiveCall {
    Impl &owner;
    explicit ActiveCall(Impl &owner):owner(owner) {
      std::lock_guard gate(owner.operation_gate);
      require(!owner.staging,"Arena resources have a staged restore");++owner.active_calls;
    }
    ~ActiveCall(){std::lock_guard gate(owner.operation_gate);--owner.active_calls;}
  };
  struct NativeFile {
    int fd;
    std::string name;
    NativeFile(int fd,std::string name) noexcept:fd(fd),name(std::move(name)) {}
    NativeFile(NativeFile &&other) noexcept:fd(std::exchange(other.fd,-1)),name(std::move(other.name)) {}
    ~NativeFile() {
#if defined(__linux__) || defined(__APPLE__)
      if(fd>=0)::close(fd);
#endif
    }
  };
  std::map<std::uint32_t,NativeFile> files;
  std::uint32_t next_file = first_file, worker = 0,worker_semaphore=0;
  std::uint32_t worker_stack = 0;
  std::atomic<bool> stopped = false;
  std::atomic<bool> worker_joined=false;
  std::atomic<bool> stopped_at_idle=false;
  native_startup_resources::Worker native_worker;
  // Declared last: stop/join actual workers before captured resources retire.
  NativeThreads threads{{0xf5000001u,0xf5000100ull}};
  NativeThreadServices workers = threads.services();

  Impl(SourceRuntimeMemory &m, SourcePhysicsGameArenaConfiguration c,
       SourcePhysicsWorldRuntimeServices s, SourcePhysicsContactRecordObserver o)
      :shared(m),config(std::move(c)),lower(std::move(s)),observe(std::move(o)),
       physical(m),scratch(m,config.worker_stack_address) {
    require(!config.savegame_directory.empty() && config.worker_stack_address,
            "Native startup needs explicit file and private-stack owners");
#if !defined(__linux__) && !defined(__APPLE__)
    throw std::runtime_error("Native arena startup platform is unavailable");
#endif
  }
  ~Impl() {
    try { stop(); } catch (...) {}
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
    worker_joined=true;
  }
  void require_retired_worker() const {
    require(stopped,"Arena continuation requires stopped execution");
    require(worker && worker_joined && stopped_at_idle && workers.outcome(worker).state==NativeThreadState::Stopped,
            "Arena continuation requires joined source-idle worker");
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
      worker_semaphore=result.handle;finish(16,result.handle);return true;
    }
    case 0x2dc0b6: {
      require(!worker && arg(0)==0 && arg(1)==0x4000 && arg(2)==0x1235f0 &&
              arg(3)==0 && arg(4)==0 && arg(5)==0,"Original async thread request differs");
      {
        std::lock_guard lock(shared.mutex);
        worker_stack=scratch.allocate({0,0,0x10000,0x101000,4},worker_stack_owner).base;
      }
      std::promise<native_startup_resources::Worker> id;
      auto ready=id.get_future();
      const auto result=workers.create([this,&id](std::stop_token stopped) {
        try {id.set_value(native_startup_resources::current_worker());}
        catch(...) {id.set_exception(std::current_exception());throw;}
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
      native_worker=ready.get();
      finish(24,worker);return true;
    }
    case 0x2dbea6: {
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
    case 0x2e0478: {
      const auto h=arg(0),ms=arg(1);
      require(ms==0xffffffffu,"Original async wait request differs");
      const bool idle=call.instruction==0x12236au && h==worker_semaphore;
      try {
        if(stop.stop_possible() && config.schedule.before_worker_wait)
          config.schedule.before_worker_wait(h,stop);
        // A completed source job must retire its queue/free-list ownership
        // and release its mutexes before cancellation. Only the original
        // worker's idle semaphore wait is a native shutdown boundary.
        acquire_native_synchronization(sync,h,ms,idle?stop:std::stop_token{},5);
      } catch(const NativeWorkerStopped &) {
        if(stop.stop_possible())stopped_at_idle=idle;
        throw;
      }
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
#if defined(__linux__) || defined(__APPLE__)
      const auto path=config.savegame_directory/name.substr(3);
      auto retained_name=name.substr(3);
      const auto fd=::open(path.c_str(),O_RDWR|O_CREAT|O_CLOEXEC,0600);
      if(fd<0){finish(28,0xffffffffu);return true;}
      const auto alias=next_file;NativeFile opened(fd,std::move(retained_name));
      files.emplace(alias,std::move(opened));++next_file;
      finish(28,alias);return true;
#else
      throw std::runtime_error("Native source file opening unavailable");
#endif
    }
    case 0x2dc378: {
      const auto found=files.find(arg(0));
      require(found!=files.end() && arg(2) && word(shared,arg(2))==0 && arg(3)==0,
              "Reached startup seek owner unavailable");
#if defined(__linux__) || defined(__APPLE__)
      const auto offset=::lseek(found->second.fd,arg(1),SEEK_SET);
      require(offset<=UINT32_MAX,"Native source file position exceeds reached ABI");
      finish(16,offset<0?0xffffffffu:std::uint32_t(offset));return true;
#else
      throw std::runtime_error("Native source file seek unavailable");
#endif
    }
    case 0x2dc2f4: {
      const auto found=files.find(arg(0));require(found!=files.end(),"Reached startup file extent owner unavailable");
#if defined(__linux__) || defined(__APPLE__)
      const auto position=::lseek(found->second.fd,0,SEEK_CUR);
      const auto ok=position>=0 && ::ftruncate(found->second.fd,position)==0;
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
  Impl::ActiveCall operation(*impl_);
  if(entry==0x2d79aau || entry==0x2d7a1au) {
    // Physical pages outlive the joined startup worker, like protect_pages.
    std::lock_guard lock(impl_->shared.mutex);
    source_run_physics_physical_storage_with_native_pages(impl_->shared.bank,entry,abi,
        impl_->config.floating_point,impl_->lower,impl_->physical,impl_->observe);
    return;
  }
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
void SourcePhysicsGameArenaNative::stop(){
  std::unique_lock operation(impl_->operation_gate);
  require(!impl_->active_calls && !impl_->staging,"Arena cannot stop inside an active source call or restore");
  // Refuse new source execution before joining, but let the real worker finish
  // any local resource callbacks; never hold the call gate while joining it.
  impl_->stopped=true;operation.unlock();impl_->stop();
}
void SourcePhysicsGameArenaNative::protect_pages(std::uint32_t p,std::uint32_t n,std::uint32_t protection){
  Impl::ActiveCall operation(*impl_);
  std::lock_guard lock(impl_->shared.mutex);
  impl_->physical.protect(p,n,protection);
}
std::uint32_t SourcePhysicsGameArenaNative::allocate_native_pages(SourcePhysicalAllocationRequest request){
  Impl::ActiveCall operation(*impl_);
  std::lock_guard lock(impl_->shared.mutex);
  return impl_->physical.allocate(request);
}
SourcePhysicsGameArenaNative::OwnershipCheckpoint SourcePhysicsGameArenaNative::ownership_checkpoint() const {
  std::unique_lock operation(impl_->operation_gate,std::try_to_lock);
  require(operation.owns_lock() && !impl_->active_calls && !impl_->staging,"Arena checkpoint has active native callers or restore");
  std::lock_guard lock(impl_->shared.mutex);impl_->require_retired_worker();
  OwnershipCheckpoint saved{1,impl_->worker,impl_->worker_stack,impl_->worker_semaphore,impl_->next_file,
      impl_->scratch.cursor_checkpoint(),impl_->physical.ownership_checkpoint(),impl_->synchronization.checkpoint(),{}};
  for(const auto &[alias,file]:impl_->files) {
#if defined(__linux__) || defined(__APPLE__)
    struct stat before{},after{};
    if(::fstat(file.fd,&before))throw std::system_error(errno,std::generic_category(),"Capture arena file extent");
    const auto position=::lseek(file.fd,0,SEEK_CUR);
    require(S_ISREG(before.st_mode) && before.st_size>=0 && std::uint64_t(before.st_size)<=UINT32_MAX &&
            position>=0 && std::uint64_t(position)<=UINT32_MAX,"Arena file exceeds retained source ABI");
    FileCheckpoint copy{alias,std::uint32_t(position),file.name,SourceRuntimeSnapshotBytes::capture_file(file.fd)};
    if(::fstat(file.fd,&after))throw std::system_error(errno,std::generic_category(),"Recheck arena file extent");
    require(copy.bytes.size()==std::uint64_t(before.st_size) && native_startup_resources::same_file_extent_and_time(before,after) &&
        ::lseek(file.fd,0,SEEK_CUR)==position,
        "Arena file changed during checkpoint");
    saved.files.push_back(std::move(copy));
#else
    throw std::runtime_error("Native arena file checkpoint platform unavailable");
#endif
  }
  validate_arena_ownership(saved,impl_->shared.bank.checkpoint());
  return saved;
}
struct SourcePhysicsGameArenaNative::PreparedOwnership::Impl {
  SourcePhysicsGameArenaNative::Impl *owner;
  std::unique_lock<std::recursive_mutex> operation;
  std::unique_lock<std::recursive_mutex> bank_lock;
  std::optional<SourcePhysicsGameArenaNative::Impl::RestoreBarrier> barrier;
  std::optional<SourceRuntimeHostStorage::PreparedCursor> scratch;
  std::optional<SourceRuntimeHostPhysicalMemory::PreparedOwnership> physical;
  std::optional<NativeSynchronization::PreparedRestore> synchronization;
  std::map<std::uint32_t,SourcePhysicsGameArenaNative::Impl::NativeFile> files;
  std::uint32_t next_file;
  Impl(SourcePhysicsGameArenaNative::Impl &target,const OwnershipCheckpoint &saved,
       const SourceRuntimeMemory::Checkpoint &bank,SourceRuntimeMemory &replacement)
      :owner(&target),operation(target.operation_gate,std::try_to_lock),
       bank_lock(target.shared.mutex,std::defer_lock),next_file(saved.next_file) {
    require(operation.owns_lock() && !target.active_calls && !target.staging,"Arena import has active native callers or restore");
    bank_lock.lock();target.require_retired_worker();
    require(saved.worker==target.worker && saved.worker_stack==target.worker_stack &&
            saved.worker_semaphore==target.worker_semaphore,"Arena import cannot rebind a different native worker namespace");
    barrier.emplace(target);
    scratch.emplace(target.scratch.prepare_cursor(saved.scratch));
    physical.emplace(target.physical.prepare_ownership(saved.physical,bank,replacement));
    synchronization.emplace(target.synchronization.prepare_restore(saved.synchronization));
    for(const auto &saved_file:saved.files) {
#if defined(__linux__) || defined(__APPLE__)
      auto name=saved_file.name;
      const auto pattern=(target.config.savegame_directory/".halo2-arena-XXXXXX").string();
      std::vector<char> path(pattern.begin(),pattern.end());path.push_back('\0');
      const auto fd=::mkstemp(path.data());
      if(fd<0)throw std::system_error(errno,std::generic_category(),"Stage private arena file");
      SourcePhysicsGameArenaNative::Impl::NativeFile file(fd,std::move(name));
      if(::unlink(path.data()))throw std::system_error(errno,std::generic_category(),"Unlink private arena staging file");
      if(::fcntl(fd,F_SETFD,FD_CLOEXEC)<0 || ::ftruncate(fd,off_t(saved_file.bytes.size())))
        throw std::system_error(errno,std::generic_category(),"Stage arena file extent/ownership");
      // The new file was extended with actual zero bytes. Preserve zero pages
      // as holes, including when the caller's save directory is on tmpfs;
      // writing them would create an unnecessary second resident file bank.
      std::size_t pending=0;
      for(std::size_t done=0;done<saved_file.bytes.size();) {
        const auto bytes=std::span(saved_file.bytes).subspan(done,
            std::min<std::size_t>(0x1000,saved_file.bytes.size()-done));
        if(std::any_of(bytes.begin(),bytes.end(),[](auto byte){return byte!=std::byte{};})) {
          for(std::size_t written=0;written<bytes.size();) {
            const auto count=::pwrite(fd,bytes.data()+written,bytes.size()-written,off_t(done+written));
            if(count<0 && errno==EINTR)continue;
            if(count<=0)throw std::system_error(count<0?errno:EIO,std::generic_category(),"Stage arena file bytes");
            written+=std::size_t(count);
          }
          pending+=bytes.size();
          if(pending>=1024*1024) {
            if(native_startup_resources::synchronize_file(fd))throw std::system_error(errno,std::generic_category(),"Synchronize staged arena bytes");
            pending=0;
          }
        }
        done+=bytes.size();
      }
      if(::lseek(fd,saved_file.position,SEEK_SET)<0)
        throw std::system_error(errno,std::generic_category(),"Stage arena file position");
      files.emplace(saved_file.alias,std::move(file));
#else
      throw std::runtime_error("Native arena file import platform unavailable");
#endif
    }
  }
};
SourcePhysicsGameArenaNative::PreparedOwnership::PreparedOwnership()=default;
SourcePhysicsGameArenaNative::PreparedOwnership::~PreparedOwnership()=default;
SourcePhysicsGameArenaNative::PreparedOwnership::PreparedOwnership(PreparedOwnership &&) noexcept=default;
SourcePhysicsGameArenaNative::PreparedOwnership &SourcePhysicsGameArenaNative::PreparedOwnership::operator=(PreparedOwnership &&) noexcept=default;
bool SourcePhysicsGameArenaNative::PreparedOwnership::for_owner(const SourcePhysicsGameArenaNative &owner) const noexcept {
  return impl_ && impl_->owner==owner.impl_.get();
}
bool SourcePhysicsGameArenaNative::PreparedOwnership::matches_memory(const SourceRuntimeMemory &bank) const noexcept {
  return impl_ && impl_->physical->matches_memory(bank);
}
SourcePhysicsGameArenaNative::PreparedOwnership SourcePhysicsGameArenaNative::prepare_ownership(
    const OwnershipCheckpoint &saved,const SourceRuntimeMemory::Checkpoint &bank,
    SourceRuntimeMemory &replacement) const {
  validate_arena_ownership(saved,bank);
  PreparedOwnership prepared;prepared.impl_=std::make_unique<PreparedOwnership::Impl>(*impl_,saved,bank,replacement);
  return prepared;
}
bool SourcePhysicsGameArenaNative::commit_ownership(PreparedOwnership &&prepared) noexcept {
  if(!prepared.for_owner(*this) || !prepared.matches_memory(impl_->shared.bank) ||
      !prepared.impl_->scratch->for_owner(impl_->scratch) ||
      !prepared.impl_->physical->for_owner(impl_->physical) ||
      !prepared.impl_->synchronization->for_owner(impl_->synchronization))return false;
  // All resources/locks were staged before bank publication. The retired local
  // native worker and callback closures remain; no remote thread is fabricated.
  impl_->scratch.commit_cursor(std::move(*prepared.impl_->scratch));
  impl_->physical.commit_ownership(std::move(*prepared.impl_->physical));
  impl_->synchronization.commit_restore(std::move(*prepared.impl_->synchronization));
  impl_->files.swap(prepared.impl_->files);impl_->next_file=prepared.impl_->next_file;
  prepared.impl_.reset();return true;
}
} // namespace halo2
