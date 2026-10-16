#include "halo2/physics_internal_phantom_observation.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/scripts.hpp"
#include "halo2/source_projectile_effect_observer.hpp"
#include "halo2/source_execution_inputs.hpp"
#include <cstring>
#include "halo2/cache.hpp"
#include "halo2/native_threads.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/native_object_lifetimes.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_process_heap_platform.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/source_damage_runtime.hpp"
#include "halo2/unit_damage.hpp"
#include "halo2/death_camera_binding.hpp"
#include "halo2/source_segment_collision.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_scene_physics.hpp"
#include "halo2/source_object_render_entry.hpp"
#include "halo2/source_object_admission_memory.hpp"
#include "halo2/source_model_build.hpp"
#include "halo2/source_render_cache.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/native_tu15_startup_timer.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/original_data.hpp"
#include "physics_live_entries_tu15.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <mutex>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstring>
#if defined(__linux__)
#include <ifaddrs.h>
#include <net/if.h>
#include <netpacket/packet.h>
#endif
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory &m,std::uint32_t p) {
  std::array<std::byte,4>b;m.read(p,b);std::uint32_t v=0;
  for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
  return v;
}
void put(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t v) {
  std::array<std::byte,4>b;for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);
}
std::uint32_t word(const SourceRuntimeMemory &m,NativeRuntimeAddress p) {
  const auto b=m.view(p,4);std::uint32_t v=0;
  for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
  return v;
}
void put(SourceRuntimeMemory &m,NativeRuntimeAddress p,std::uint32_t v) {
  std::array<std::byte,4>b;for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);
}
constexpr SourcePhysicsFactoryFloatingPoint fp{0x27f,0x1f80};
constexpr std::uint32_t stack=0x600f0000u,tls=0x60200000u,parameters=tls+0x1000u;
constexpr std::uint32_t process_context=0x60210000u,notifications=0x60211000u,
    allocation_callback=0xf7000001u;
constexpr std::uint32_t native_tick_count=0x60212000u,native_thread_record=0x60214000u;
SourceRuntimeKernelTickCount native_tick_input() {
  SourceRuntimeKernelTickCount input{native_tick_count,{},"native main-thread monotonic millisecond tick-count data"};
  const auto value=std::uint32_t(native_tu15_timer_clock().interrupt_time()/10000u);
  for(unsigned i=0;i<4;++i)input.bytes[i]=std::byte(value>>(8*i));
  return input;
}
SourceMachineIdentity native_machine_identity() {
#if defined(__linux__)
  ifaddrs *interfaces=nullptr;
  if(::getifaddrs(&interfaces)!=0)
    throw std::system_error(errno,std::generic_category(),"Native source machine interface enumeration");
  struct Owner {ifaddrs *p;~Owner(){::freeifaddrs(p);}} owner{interfaces};
  for(auto *i=interfaces;i;i=i->ifa_next) {
    if(!i->ifa_addr || i->ifa_addr->sa_family!=AF_PACKET ||
        !(i->ifa_flags&IFF_UP) || (i->ifa_flags&IFF_LOOPBACK))continue;
    const auto *address=reinterpret_cast<const sockaddr_ll *>(i->ifa_addr);
    if(address->sll_halen!=6)continue;
    SourceMachineIdentity result;
    std::copy_n(address->sll_addr,result.size(),result.begin());return result;
  }
#endif
  throw FormatError("Native source player identity requires an up six-byte hardware interface");
}
}
struct MultiplayerObjectRuntime::Impl {
  SourceRuntimeMemory bank;
  NativeTu15TimerClock clock=native_tu15_timer_clock();
  const Cache &cache_owner;
  SourceRuntimeHostVirtualMemory virtual_memory;
  PhysicsPlatformLocks locks;
  NativeSynchronization events{{0xf6000001u,0xf6010000ull}};
  NativeSynchronizationServices event_services=events.services();
  std::mutex event_mutex;
  std::map<std::uint32_t,std::uint32_t> lock_events;
  std::map<std::thread::id,std::uint32_t> thread_ids;
  const std::thread::id main_thread=std::this_thread::get_id();
  struct Allocation {std::uint32_t payload,requested_bytes,flags;};
  std::vector<Allocation> allocations;
  std::uint32_t last_instruction{};
  std::map<std::uint32_t,std::uint32_t> bipeds;
  std::map<std::uint32_t,bool> fixed_bipeds;
  bool session_configured{};
  std::optional<SourceMachineIdentity> machine_identity;
  std::uint32_t next_animation_resource=0x61000000u;
  std::map<std::uint32_t,std::uint32_t> animation_resources;
  std::map<std::pair<unsigned,std::uint32_t>,SourceObjectDrawCollector> model_collectors;
  std::uint32_t next_model_collector=0x62000000u;
  static constexpr std::uint32_t model_scratch=0x60400000u;
  SourceRuntimeCacheBspImage bsp;
  // Last owner: stop/join its worker before releasing callbacks or banks.
  std::unique_ptr<SourceDeathStartupNative> startup;

  Impl(const std::filesystem::path &image,const Cache &cache,std::size_t selected,
       const std::filesystem::path &files)
      :bank(load_source_runtime_image(image,SourcePhysicsContactImage::MapPack11,native_tick_input())),cache_owner(cache),virtual_memory(bank,0x08000000u),
       bsp(cache.load_source_runtime_cache_bsp_image(selected)) {
    bank.mount(0x60000000u,std::vector<std::byte>(0x100000),SourceRuntimeMemory::Access::ReadWrite,
               "native multiplayer main call stack");
    bank.mount(tls,std::vector<std::byte>(0x2000),SourceRuntimeMemory::Access::ReadWrite,
               "native multiplayer exception-chain and caller parameters");
    bank.mount(model_scratch,std::vector<std::byte>(0x4000),SourceRuntimeMemory::Access::ReadWrite,
               "native presentation model-builder caller scratch");
    put(bank,tls,0xffffffffu);
    // Explicit native process-notification ABI. The source allocator's FS20 /
    // +250 / +14 protocol reaches an actual retained host consumer. No absent
    // Xbox callback or zeroed KTHREAD/KPCR is assumed. Only the three consumed
    // pointer cells are bound; unrelated kernel structure bytes stay absent.
    bank.mount(process_context+0x250,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,
               "native process allocation notification pointer");
    bank.mount(notifications+0x14,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,
               "native process allocation notification callback binding");
    put(bank,tls+0x20,process_context);put(bank,process_context+0x250,notifications);
    put(bank,notifications+0x14,allocation_callback);
    // Native process startup uses the complete original heap constructor with
    // literal2D7FB0..2D7FDE inputs. XAPI hardware/timer boot is not run or
    // reported successful. This explicit host binding publishes its actual
    // returned heap for the original CRT consumers in this native process.
    put(bank,parameters,0x30);
    SourceUnitAnimationRuntimeAbi abi{};abi.eflags=2;abi.fs_base=tls;abi.gp[4]=stack;
    put(bank,stack,0x60300000u);
    const std::array heap_args{2u,0u,word(bank,0x10134),word(bank,0x10138),0u,parameters};
    for(unsigned i=0;i<heap_args.size();++i)put(bank,stack+4+i*4,heap_args[i]);
    SourcePhysicsContactRuntimeServices process;
    process.required_owner=[&](auto&m,const auto&call,auto&a,const auto&o) {
      SourceUnitAnimationRuntimeAbi full{};static_cast<SourcePhysicsContactRuntimeAbi&>(full)=a;
      required(m,call,full,o);a=static_cast<SourcePhysicsContactRuntimeAbi&>(full);
    };
    source_run_process_heap_with_native_platform(bank,0x2da83a,abi,SourcePhysicsContactImage::MapPack11,
        fp,process,virtual_memory,locks,lock_services());
    if(!abi.gp[0])throw FormatError("Native multiplayer original process heap construction failed");
    put(bank,0x566f08,abi.gp[0]);
    startup=std::make_unique<SourceDeathStartupNative>(bank,
        SourceDeathStartupNativeConfiguration{files,0x60110000u,fp,{},0x10000000u},services());
    startup->bind_cache_files({cache.path(),cache.shared_directory()/"mainmenu.map",
        cache.shared_directory()/"shared.map",cache.shared_directory()/"single_player_shared.map"});
    for(auto entry:{0x12cb70u,0x1221b0u,0x125280u})call(entry);
    call(0x12f020); // Actual static allocator constructs the model-resource pools/cache.
    // Isolated original render-cache constructor over this world's actual
    // arena allocator. This does not assert complete renderer startup.
    call(0x3d260);
    initialize_game_storage();
    for(auto entry:{0x147cc0u,0x14cc00u,0x158ba0u,0x1677c3u,0xb76b0u,0x1eef40u,0x10d490u,0x1efe10u})call(entry);
    initialize_ai_storage_prefix();
    call(0x1c5f00);
    source_bind_runtime_cache_tags_1_1(bank,cache.load_source_runtime_cache_tag_image());
    for(const auto &tag:cache.select_tags({.group=tag_groups::biped}))bipeds.emplace(tag.datum,tag.address);
    for(auto entry:{0x14ccd0u,0x1677e5u,0xb78c0u,0x1eefe0u,0x10d4f0u,0x1efe70u,0x1c5fd0u})call(entry);
    call(0x124290,{},bsp.mapping); // Complete original I/O/tag publication.
    source_bind_runtime_cache_bsp_1_1(bank,bsp);
    initialize_native_thread_tls();
    call(0x32c7a6); // Complete original CRT thread construction and callback registration.
    call(0x1479b0); // Complete original shared/local RNG construction, using real clock/CRT owners.
    call(0x3274e4);
    // Actual original CRT table ordering, retaining each registration body.
    constexpr std::array registrations{0x37f110u,0x37f130u,0x37f150u,0x37f1e0u,
        0x37f240u,0x37fc70u,0x37fc90u,0x37fcb0u};
    unsigned registered=0;
    for(auto p=0x468630u;p<0x468cb8u;p+=4) {
      const auto target=word(bank,p);
      if(std::find(registrations.begin(),registrations.end(),target)!=registrations.end()) {
        call(target);++registered;
      }
    }
    if(registered!=registrations.size())throw FormatError("Original Havok CRT registration table differs");
    call(0x37d8c0);
    call(0x1775d0);call(0x177690);
    call(0x1c5ff0); // Full new-map world constructor, with genuine empty membership.
    call(0xb7aa0); // Complete object-map activation, before any native placement.
    call(0x2b520); // Original render-cache activation/reset in the same bank.
    call(0x12f260); // Original model-resource queue activation in the same bank.
  }
  PhysicsPlatformLockServices lock_services() {
    const auto event=[this](std::uint32_t p) {
      std::lock_guard guard(event_mutex);
      auto at=lock_events.find(p);if(at!=lock_events.end())return at->second;
      const auto created=event_services.create_semaphore(0,1);
      if(created.error)throw std::system_error(created.error);
      lock_events.emplace(p,created.handle);return created.handle;
    };
    return {
      [this] {
        std::lock_guard guard(event_mutex);const auto id=std::this_thread::get_id();
        auto at=thread_ids.find(id);if(at!=thread_ids.end())return at->second;
        const auto token=std::uint32_t(thread_ids.size()+1);thread_ids.emplace(id,token);return token;
      },
      [this,event](auto p) {
        acquire_native_synchronization(event_services,event(p),0xffffffffu,{},5);
      },
      [this,event](auto p) {
        const auto r=event_services.release_semaphore(event(p),1);if(r.error)throw std::system_error(r.error);
      }
    };
  }
  SourceDeathWorldSchedulerServices services() {
    SourceDeathWorldSchedulerServices s;
    s.runtime.instruction=[this](auto p){
      if(std::this_thread::get_id()==main_thread)last_instruction=p;
      if(p==0x337a45u)put(bank,native_tick_count,std::uint32_t(clock.interrupt_time()/10000u));
    };
#if defined(__i386__) || defined(__x86_64__)
    s.timestamp_counter=[] {return __rdtsc();};
    s.runtime.estimate=[](auto kind,std::uint32_t bits) {
      const auto input=_mm_set_ss(std::bit_cast<float>(bits));
      const auto result=kind==SourceUnitAnimationEstimate::Reciprocal?_mm_rcp_ss(input):_mm_rsqrt_ss(input);
      return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(result));
    };
#endif
    s.runtime.required_owner=[this](auto&m,const auto&call,auto&a,const auto&o){required(m,call,a,o);};
    s.runtime.resource_delivery=[this](auto&m,const auto&call,auto&a,const auto&) {
      if(call.instruction!=0x1e121bu || call.target!=0x124f30u || call.tail ||
          word(m,a.gp[4])!=0x1e1220u || std::this_thread::get_id()!=main_thread)
        throw FormatError("Native animation byte delivery caller differs");
      const auto descriptor=a.gp[0];
      auto found=animation_resources.find(descriptor);
      if(found==animation_resources.end()) {
        auto resource=cache_owner.load_source_runtime_animation_image(descriptor);
        std::array<std::byte,12> current;m.read(descriptor,current);
        if(!std::equal(current.begin(),current.end(),resource.descriptor.begin()))
          throw FormatError("Source animation descriptor differs from its actual cache bytes");
        const auto size=(std::uint64_t(resource.bytes.size())+4095)&~4095ull;
        if(size>0x70000000ull-next_animation_resource)
          throw FormatError("Native animation resource address space exhausted");
        const auto resident=next_animation_resource;
        bank.mount(resident,resource.bytes,SourceRuntimeMemory::Access::ReadOnly,
                   "actual native animation cache resource bytes");
        next_animation_resource+=std::uint32_t(size);
        found=animation_resources.emplace(descriptor,resident).first;
      }
      (void)word(m,a.gp[4]+4); // Original named request argument is retained.
      a.gp[0]=found->second;a.gp[4]+=8;
    };
    return s;
  }
  void required(SourcePhysicsContactMemory &m,const SourceUnitAnimationRuntimeCall &request,
      SourceUnitAnimationRuntimeAbi &a,const SourcePhysicsContactRecordObserver &o) {
    if(source_damage_owns_entry_1_1(request.target)) {
      source_run_damage_1_1(m,request.target,a,a.floating_point.value_or(fp),services().runtime,o);
      return;
    }
    if(source_try_native_process_heap_memory_call(bank,request,a,SourcePhysicsContactImage::MapPack11,virtual_memory))return;
    if(source_try_native_physics_lock_call(m,request,a,SourcePhysicsContactImage::MapPack11,locks,lock_services()))return;
    const auto sp=a.gp[4];
    if(request.tail && request.instruction==0x336983u && request.target==0x80000080u) {
      if(word(m,sp)!=0x32804cu)throw FormatError("Original system-time thunk caller differs");
      const auto output=word(m,sp+4);const auto value=clock.system_time();
      put(m,output,std::uint32_t(value));put(m,output+4,std::uint32_t(value>>32u));
      a.gp[4]+=8;return;
    }
    if(request.instruction==0x2db3b0u && request.target==allocation_callback && !request.tail) {
      if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x2db3b3u || word(m,sp+4)!=10u)
        throw FormatError("Native allocation notification caller/thread differs");
      const auto record=word(m,sp+8);
      allocations.push_back({word(m,record),word(m,record+4),word(m,record+8)});
      a.gp[4]+=12;return;
    }
    if(request.instruction==0x32ebd3u && request.target==0x80000123u && !request.tail) {
      if(word(m,sp)!=0x32ebd9u)throw FormatError("Original CRT lock initializer return differs");
      locks.initialize(m,word(m,sp+4));a.gp[4]+=8;return;
    }
    const bool thread_enter=request.instruction==0x2d84f0u && request.target==0x80000115u;
    const bool thread_leave=request.instruction==0x2d8528u && request.target==0x80000126u;
    const bool crt_enter=(request.instruction==0x32d146u && request.target==0x80000115u)||thread_enter;
    const bool crt_leave=(request.instruction==0x32d079u && request.target==0x80000126u)||thread_leave;
    if((crt_enter || crt_leave) && !request.tail) {
      const auto returned=thread_enter?0x2d84f6u:thread_leave?0x2d852eu:crt_enter?0x32d14cu:0x32d07fu;
      if(word(m,sp)!=returned || ((thread_enter||thread_leave)&&word(m,sp+4)!=0x468da0u))
        throw FormatError("Original CRT lock caller differs");
      if(crt_enter)locks.enter(m,word(m,sp+4),lock_services());
      else locks.leave(m,word(m,sp+4),lock_services());
      a.gp[4]+=8;return;
    }
    if(request.instruction==0x1242b1u && request.target==0x1244b0u && !request.tail) {
      if(word(m,sp)!=0x1242b6u || word(m,sp+4)!=0xffffffffu || word(m,sp+8)!=bsp.file_offset ||
          a.gp[0]!=bsp.file_offset || a.gp[2]!=bsp.region.address ||
          a.gp[6]!=((bsp.region.bytes.size()+511u)&~511u))
        throw FormatError("Original BSP I/O request differs from real cache resource");
      source_deliver_runtime_cache_bsp_1_1(bank,bsp);
      a.gp[0]=(a.gp[0]&0xffffff00u)|1u;a.gp[4]+=12;return;
    }
    if(source_death_startup_owns_entry_1_1(request.target)) {
      source_run_death_startup_1_1(m,request.target,a,fp,services(),o);return;
    }
    if(source_death_object_phases_owns_entry_1_1(request.target)) {
      source_run_death_object_phases_1_1(m,request.target,a,fp,services(),o);return;
    }
    std::ostringstream message;message<<"Native multiplayer source owner missing at "<<std::hex
        <<request.instruction<<" -> "<<request.target;
    throw std::runtime_error(message.str());
  }
  std::uint32_t call(std::uint32_t entry,std::span<const std::uint32_t> args={},
      std::optional<std::uint32_t> eax={},std::optional<std::uint32_t> ecx={},
      std::optional<std::uint32_t> edx={},std::optional<std::uint32_t> esi={},
      std::optional<std::uint32_t> edi={},std::optional<std::uint32_t> ebx={}) {
    if(std::this_thread::get_id()!=main_thread)throw FormatError("Native source object world belongs to its main thread");
    SourceUnitAnimationRuntimeAbi a{};a.eflags=2;a.fs_base=tls;a.gp[4]=stack;
    put(bank,stack,0x60300000u);for(unsigned i=0;i<args.size();++i)put(bank,stack+4+i*4,args[i]);
    if(eax)a.gp[0]=*eax;
    if(ecx)a.gp[1]=*ecx;
    if(edx)a.gp[2]=*edx;
    if(esi)a.gp[6]=*esi;
    if(edi)a.gp[7]=*edi;
    if(ebx)a.gp[3]=*ebx;
    try {
      if(source_death_startup_owns_entry_1_1(entry))startup->run(entry,a);
      else source_run_death_object_phases_1_1(startup->memory(),entry,a,fp,startup->services());
    } catch(const std::exception &e) {
      std::ostringstream message;message<<"Native multiplayer source root "<<std::hex<<entry
          <<" at "<<last_instruction<<": "<<e.what();
      throw std::runtime_error(message.str());
    }
    return a.gp[0];
  }
  void initialize_native_thread_tls() {
    // Explicit native main-thread TLS ABI derived from the authenticated XBE
    // descriptor and complete2D856E bootstrap stores. This owns host inputs;
    // it does not claim original title/thread-create startup was executed.
    constexpr std::uint32_t descriptor=0x418070u,stack_end=0x60100000u;
    const auto first=word(bank,descriptor),last=word(bank,descriptor+4),index=word(bank,descriptor+8);
    if(first || last || index!=0x487620u || word(bank,descriptor+12) ||
        word(bank,descriptor+16)!=0x90u || word(bank,descriptor+20))
      throw FormatError("Authenticated TU1.1 native TLS descriptor differs");
    const auto bytes=((last-first+word(bank,descriptor+16)+15u)&~15u)+4u;
    const auto block=stack_end-bytes;
    bank.mutable_view(block,bytes);
    if(word(bank,index) || word(bank,0x566ef8u))
      throw FormatError("Original title TLS index already has a different owner");
    bank.mount(native_thread_record+0x28,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,
               "native main-thread TLS allocation pointer ABI");
    bank.mount(native_thread_record+0x12c,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,
               "native main-thread opaque identity ABI");
    put(bank,tls+4,stack_end);put(bank,tls+0x28,native_thread_record);
    bank.mutable_view(tls+0x24,1)[0]=std::byte{0}; // Native user-thread execution level.
    put(bank,native_thread_record+0x28,block);
    put(bank,native_thread_record+0x12c,lock_services().current_thread());
    put(bank,index,0u-bytes/4u);put(bank,0x566ef8u,bytes);
    put(bank,block,block+4);
    // The existing owned native stack supplies the descriptor's zero-fill;
    // validate it instead of installing a CRT record or an RNG seed.
    const auto zero=bank.view(block+4,bytes-4);
    if(std::any_of(zero.begin(),zero.end(),[](auto b){return b!=std::byte{};}))
      throw FormatError("Native TLS zero-fill overlaps live caller storage");
  }
  void initialize_game_storage() {
    // Bounded original139398..1393EC storage prefix, after complete125280.
    // The following68 subsystem callbacks are owned by native composition;
    // this does not report complete139390 or publish map-ready flags.
    const auto cursor=word(bank,0x4ed654),base=word(bank,0x4ed650);
    const auto game=base+cursor;
    if(!base || game<base || cursor>0xffffffffu-0x1200u)
      throw FormatError("Original game-options arena allocation wraps");
    bank.mutable_view(game,0x1200);
    put(bank,0x4ed654,cursor+0x1200); //1393AB
    put(bank,parameters+0x100,0x1200);
    const std::array checksum_args{0x4ed65cu};
    call(0x1656a0,checksum_args,parameters+0x100,{},{},{},4);
    for(unsigned i=0;i<0x480;++i)put(bank,game+i*4,0); //1393D9 REP STOSD
    put(bank,0x4edf18,game); //1393E0
    const std::array<std::byte,2> none{std::byte{0xff},std::byte{0xff}};
    bank.write(game+2,none); //1393E6
  }
  void initialize_ai_storage_prefix() {
    // Bounded1CB180..1CB1F4: allocate the two real AI/PVS records, then
    // execute complete1CB990. Later AI subsystem constructors are excluded;
    // no whole AI startup/readiness is claimed for a native human-only match.
    for(const auto &[size,slot]:std::array<std::pair<std::uint32_t,std::uint32_t>,2>{
        {{0x374u,0x4fcda8u},{0x38u,0x4fcda4u}}}) {
      const auto cursor=word(bank,0x4ed654),base=word(bank,0x4ed650),address=base+cursor;
      if(!base || address<base || cursor>0xffffffffu-size)
        throw FormatError("Original AI/PVS arena allocation wraps");
      bank.mutable_view(address,size);
      put(bank,0x4ed654,cursor+size);
      put(bank,parameters+0x100,size);
      const std::array checksum_args{0x4ed65cu};
      call(0x1656a0,checksum_args,parameters+0x100,{},{},{},4);
      put(bank,slot,address);
    }
    call(0x1cb990);
  }
  void configure_session(const SourceGameVariant &variant,std::uint16_t mode,
      std::int16_t rate,std::uint32_t seed) {
    if(session_configured || word(bank,object_pool()+0x3c))
      throw FormatError("Native source session must be bound once before object birth");
    if(rate<=0 || !source_game_variant_valid(variant))
      throw FormatError("Native source session requires an admitted variant and positive clock rate");
    const auto game=word(bank,0x4edf18);
    // Native multiplayer options delivery:139540 copies its caller's packet
    // to game+8. These actual session fields are inputs, not original stores
    // attributed to that packet's title/lobby producer.
    const auto packet=model_scratch+0x1000u;
    const auto previous=bank.view(game+8,0x1118);
    bank.write(packet,previous);
    put(bank,packet,2);
    put(bank,packet+4,std::uint32_t(mode)|(std::uint32_t(std::uint16_t(rate))<<16));
    put(bank,packet+0x10,seed);
    bank.write(packet+0x134,std::as_bytes(std::span(variant.bytes)));
    const std::array options_args{packet};call(0x139540,options_args);
    // Isolated original resource-budget callback.190930 passes1908E0's
    // result to12F160 through45AD78+3C; execute that same selector on these
    // real options, then reserve from this world's current physical bank.
    // This does not assert the remaining190930 lifecycle callbacks ran.
    const auto resource_stage=call(0x1908e0,{},{},{},{},game+8);
    const std::array resource_args{resource_stage};call(0x12f160,resource_args);
    call(0x147d20);
    session_configured=true;
  }
  std::uint32_t object_pool()const{return word(bank,0x4e78d0);}
  std::uint32_t player_pool()const{return word(bank,0x4f01f4);}
  void preload_render_model(std::uint32_t model) {
    if(!session_configured)throw FormatError("Native model loading requires delivered session options");
    const auto mode=word(bank,word(bank,0x4eb114)+(model&0xffffu)*16+8);
    const auto count=word(bank,mode+0x24),sections=word(bank,mode+0x28);
    const std::array args{0u};call(0x1703d0,args,model);
    for(unsigned i=0;i<count;++i) {
      const auto header=sections+i*0x5cu+0x38u;
      if(std::bit_cast<std::int32_t>(word(bank,header+8))<=0)continue;
      call(0x12f5e0,{},2u,header);
      const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
      while(!(call(0x12f5e0,{},0u,header)&255u)) {
        if(std::chrono::steady_clock::now()>=deadline)
          throw FormatError("Original model-resource worker did not finish native map loading");
        std::this_thread::yield();
      }
    }
  }
  void begin_model_frame() {
    auto frame=word(bank,0x4c1554),cursor=word(bank,0x48f038);
    source_render_frame_begin(frame,cursor);
    put(bank,0x4c1554,frame);put(bank,0x48f038,cursor);
  }
  SourceObjectDrawCollector build_world_model(std::uint32_t owner,unsigned view,
      unsigned players,bool modifier,float metric,float lighting_metric) {
    if(view>=4)throw FormatError("Native model view exceeds original four views");
    // Full-handle validation belongs to this live world, before the original
    // builder's direct-low16 object lookup.
    DatumPool objects(bank,object_pool());
    if(owner==0xffffffffu || objects.lookup(owner).empty())
      throw FormatError("Native model builder received a stale object handle");
    const auto policy=source_geometry_lod_policy(players);
    put(bank,0x4c1570,view);put(bank,0x4c153c,std::uint32_t(policy.ordinary_maximum));
    bank.mutable_view(0x4c1539,1)[0]=std::byte(modifier);
    auto [at,inserted]=model_collectors.try_emplace({view,owner});auto &collector=at->second;
    if(inserted) {
      collector.address=next_model_collector;next_model_collector+=0x1000;
      bank.mount(collector.address,std::vector<std::byte>(0x1000),SourceRuntimeMemory::Access::ReadWrite,
                 "native per-view model collector caller storage");
      std::memcpy(collector.bytes.data(),&owner,sizeof(owner));
      put(bank,collector.address,owner);
    }
    SourceModelBuildWorld w;auto &m=w.admission.memory;
    m.banks={word(bank,0x4eb114),word(bank,0x4e7918),word(bank,0x4e7914),object_pool(),
      [this](auto p,auto n){return bank.view(p,n);},
      [this](auto flags,auto p){return (call(0x12f5e0,{},flags,p)&255u)!=0;}};
    m.write=[this](auto p,auto n){return bank.mutable_view(p,n);};
    m.view=std::int32_t(view);m.render_frame=word(bank,0x4c1554);m.render_cache_pool=word(bank,0x510c24);
    m.context_callback=0xbf7a0u;
    w.admission.first_person_owner=word(bank,0x4c90f0);
    w.admission.ordinary_maximum=policy.ordinary_maximum;w.admission.view_modifier=modifier;
    w.admission.rasterizer=word(bank,0x48cfa0);
    w.admission.scalar=[this](auto object,auto function) {
      const auto output=model_scratch+0x3f00;const std::array args{object,function,output};
      const auto result=call(0xbba30,args);
      return std::pair{(result&255u)!=0,std::bit_cast<float>(word(bank,output))};
    };
    w.nodes.resource_cache_present=word(bank,0x4eda64)!=0;
    w.nodes.deferred_nodes=[this](auto object,auto selected,auto nodes) {
      const std::array args{object,selected,nodes};return call(0xbe500,args);
    };
    w.cached_models=0x4c90f8;w.arena_cursor=0x48f038;w.arena=0x48f03c;w.scratch=model_scratch;
    w.lighting=[this](auto object,float metric) {
      const std::array args{std::bit_cast<std::uint32_t>(metric)};
      const auto address=call(0x3daf0,args,object);std::array<std::uint32_t,21> result;
      const auto bytes=bank.view(address,sizeof(result));std::memcpy(result.data(),bytes.data(),sizeof(result));
      return result;
    };
    source_object_build_collector(collector,{owner,std::bit_cast<std::uint32_t>(metric),
        std::bit_cast<std::uint32_t>(lighting_metric),0u,0u,{},1u},w);
    return collector;
  }
  void construct_player(std::uint32_t player,unsigned user,const SourcePlayerConfiguration &configuration) {
    if(!session_configured || user>=4)throw FormatError("Native source player birth requires its local session/user");
    SourceRawDatumPoolServices storage{.read=[this](auto p,auto n){return bank.view(p,n);}};
    if(!source_raw_datum_lookup(player_pool(),player,storage))
      throw FormatError("Native source player birth requires its actual allocated full identity");
    if(!machine_identity)machine_identity=native_machine_identity();
    const auto identity=source_offline_player_identity(*machine_identity,std::int32_t(user));
    constexpr std::uint32_t creation=0x60020000u;
    std::array<std::byte,0xac> input{};
    // Original creation+2/+4 are source-view/controller bindings. Native SDL
    // presentation owns these separately; no Xbox input user is fabricated.
    input[2]=input[3]=std::byte{0xff};
    std::fill_n(input.begin()+4,4,std::byte{0xff});
    for(unsigned i=0;i<machine_identity->size();++i)input[8+i]=std::byte((*machine_identity)[i]);
    for(unsigned i=0;i<identity.size();++i)input[0xeu+i]=std::byte(identity[i]);
    for(unsigned i=0;i<configuration.size();++i)input[0x1c+i]=std::byte(configuration[i]);
    bank.write(creation,input);
    const std::array args{player,1u,creation};call(0x14d800,args);
  }
  void bind_player(std::uint32_t player,std::uint32_t object) {
    const std::array args{player};call(0x14e240,args,{},{},{},{},{},object);
  }
  void kill_biped(std::uint32_t object) {
    constexpr std::uint32_t damage=0x60040000u;
    const std::array record_args{object,damage};call(0xd7cd0,record_args);
    const std::array death_args{object,damage,1u,0u};call(0xd7e00,death_args);
    const std::array cleanup_args{object,0xffffffffu,0u,0u,0u};call(0xcbdc0,cleanup_args);
  }
  void update_player_clusters() {
    // Exact bounded14ECCC..14ED0C early cluster loop. Command construction
    // before it and input delivery after it remain separately owned. The
    // selected cluster comes only from the actual current/previous parent root.
    const auto pool=player_pool(),heads=word(bank,object_pool()+0x44);
    for(std::uint32_t i=0;i<word(bank,pool+0x38);++i) {
      if(!(word(bank,word(bank,pool+0x48)+(i>>5)*4)&(1u<<(i&31))))continue;
      const auto row=word(bank,pool+0x44)+i*word(bank,pool+0x24);
      auto object=word(bank,row+0x2c);
      if(object==0xffffffffu)object=word(bank,row+0x30);
      if(object==0xffffffffu)continue;
      std::uint32_t body{};unsigned depth=0;
      do {
        if(++depth>0x800)throw FormatError("Source player cluster parent chain is cyclic");
        body=word(bank,heads+(object&0xffffu)*12+8);
        object=word(bank,body+0x14);
      }while(object!=0xffffffffu);
      const auto cluster=bank.view(body+0x2c,2);
      if(cluster[0]!=std::byte{0xff} || cluster[1]!=std::byte{0xff})
        bank.write(row+0x2a,cluster); //14EDC4, preserves prior cluster on NONE.
    }
  }
  void step_objects() {
    if(!session_configured || std::this_thread::get_id()!=main_thread)
      throw FormatError("Native source object tick requires its configured main-thread session");
    update_player_clusters();
    source_advance_death_object_clock_1_1(bank);
    for(const auto entry:{0x13a380u,0xb7e00u,0x1c7660u,0xb7f50u,0xb8040u})call(entry);
  }
  std::vector<Transform> world_nodes(std::uint32_t datum) const {
    const SourceRawDatumPoolServices storage{
      .read=[this](auto p,auto n){return bank.view(p,n);}};
    const auto header=source_raw_datum_lookup(object_pool(),datum,storage);
    if(!header)throw FormatError("Current WORLD nodes require a live full source object handle");
    const auto body=word(bank,header+8);
    if(!body)throw FormatError("Current WORLD nodes require the actual source object body");
    const auto short_at=[this](std::uint32_t p) {
      const auto b=bank.view(p,2);
      const auto value=std::uint16_t(std::to_integer<unsigned>(b[0])|
          (std::to_integer<unsigned>(b[1])<<8));
      return std::bit_cast<std::int16_t>(value);
    };
    const auto bytes=short_at(body+0x114),offset=short_at(body+0x116);
    if(bytes<=0 || bytes%0x34 || offset<0)
      throw FormatError("Actual source object has no complete current WORLD node bank");
    const auto first=body+std::uint32_t(offset);
    bank.view(first,std::size_t(bytes));
    std::vector<Transform> result(std::size_t(bytes)/0x34);
    const auto scalar=[this](std::uint32_t p){return std::bit_cast<float>(word(bank,p));};
    const auto vector=[&](std::uint32_t p){return Vec3{scalar(p),scalar(p+4),scalar(p+8)};};
    for(std::size_t i=0;i<result.size();++i) {
      const auto p=first+std::uint32_t(i)*0x34;
      result[i]={scalar(p),{vector(p+4),vector(p+16),vector(p+28)},vector(p+40)};
    }
    return result;
  }
  ShadowCasterSphere object_sphere(std::uint32_t datum) const {
    const SourceRawDatumPoolServices storage{
      .read=[this](auto p,auto n){return bank.view(p,n);}};
    const auto header=source_raw_datum_lookup(object_pool(),datum,storage);
    if(!header)throw FormatError("Current owner sphere requires a live full source object handle");
    const auto body=word(bank,header+8);
    if(!body)throw FormatError("Current owner sphere requires the actual source object body");
    const auto scalar=[this](std::uint32_t p){return std::bit_cast<float>(word(bank,p));};
    return {{scalar(body+0x30),scalar(body+0x34),scalar(body+0x38)},scalar(body+0x3c)};
  }
  SourceObserverLocation locate(Vec3 position) {
    constexpr std::uint32_t input=0x60030000u,output=0x60030100u;
    bank.write(input,std::as_bytes(std::span(&position,1)));
    call(0x11d4a0,{},{},{},input,output);
    return {{word(bank,output),word(bank,output+4)}};
  }
  bool fog_side(Vec3 position) {
    // Literal1732D9..173366 fog classifier over the same actual BSP/location
    // used by the complete four-probe camera obstruction owner.
    const auto location=locate(position);
    const auto cluster=location.cluster();
    if(cluster==-1)return false;
    const auto sbsp=word(bank,0x4e7918);
    const auto row=word(bank,sbsp+0xa0)+std::uint32_t(std::int32_t(cluster))*0xb0u;
    const auto index=std::to_integer<unsigned>(bank.view(row+0x70,1)[0]);
    if(index==0xffu)return false;
    const auto plane=word(bank,sbsp+0x68)+(index&0x7fu)*24u;
    const auto field=bank.view(plane+2,2);
    if(field[0]==std::byte{0xff} && field[1]==std::byte{0xff})return false;
    if(!(index&0x80u))return true;
    const auto scalar=[&](auto p){return std::bit_cast<float>(word(bank,p));};
    volatile float z=scalar(plane+0xc)*position.z,y=scalar(plane+8)*position.y;
    volatile float zy=z+y,x=position.x*scalar(plane+4),dot=zy+x;
    volatile float distance=dot-scalar(plane+0x10);
    return 0.f>distance;
  }
  SourceObserverWorldServices observer_world_services() {
    SourceObserverWorldServices result;
    result.locate=[this](Vec3 p){return locate(p);};
    result.fog_side=[this](Vec3 p){return fog_side(p);};
    result.request_visibility=[this](std::int32_t cluster) {
      call(0x3f4f0,{},std::bit_cast<std::uint32_t>(cluster));
    };
    result.collision=source_death_camera_collision_1_1(bank,
        {0x60030200u,0x60030220u,0x60030300u,0x600e0000u,tls},fp,startup->services().runtime);
    return result;
  }
  std::uint32_t create_biped(std::uint32_t definition,const Vec3 &position,const Vec3 &forward) {
    if(!session_configured)throw FormatError("Actual native source session options are required before biped birth");
    const auto tag=bipeds.find(definition);
    if(tag==bipeds.end())throw FormatError("Native biped creation requires an actual cache bipd datum");
    if(!fixed_bipeds[definition]) {
      // The bipd arm of11DE40 dispatches this complete serialized-shape owner.
      call(0x1c5cf0,{}, {},{},tag->second+0x264);
      fixed_bipeds[definition]=true;
    }
    constexpr std::uint32_t request=0x60010000u;
    const std::array args{definition,0xffffffffu,0u};
    call(0xb8820,args,{},{},{},request);
    for(const auto &[at,v]:std::array<std::pair<unsigned,Vec3>,2>{{{0x1c,position},{0x28,forward}}}) {
      put(bank,request+at,std::bit_cast<std::uint32_t>(v.x));
      put(bank,request+at+4,std::bit_cast<std::uint32_t>(v.y));
      put(bank,request+at+8,std::bit_cast<std::uint32_t>(v.z));
    }
    const std::array birth_args{request};
    return call(0xb8a30,birth_args);
  }
};
struct MultiplayerObjectRuntime::Continuation {
  SourceRuntimeMemory &bank;
  const std::thread::id executor=std::this_thread::get_id();
  const bool retained_executor;
  mutable std::recursive_mutex gate;
  std::size_t calls{};
  std::unique_ptr<SourceProjectileEffectObserver> projectile_effects;
  bool staging{},fault{};
  std::shared_ptr<std::atomic<std::uint64_t>> writes=std::make_shared<std::atomic<std::uint64_t>>(0);
  std::uint64_t mutating_calls{};
  Continuation(SourceRuntimeMemory &bank,bool retained):bank(bank),retained_executor(retained) {
    const auto serial=writes;
    source_runtime_register_write_observer(bank,this,[serial](auto,auto){serial->fetch_add(1,std::memory_order_relaxed);});
    try{source_runtime_register_topology_observer(bank,this,[serial](auto,auto) noexcept {serial->fetch_add(1,std::memory_order_relaxed);});}
    catch(...){source_runtime_unregister_write_observer(bank,this);throw;}
  }
  ~Continuation(){source_bind_internal_phantom_observer(bank,{});source_runtime_unregister_topology_observer(bank,this);source_runtime_unregister_write_observer(bank,this);}
  void require_executor() const {
    if(!retained_executor || std::this_thread::get_id()!=executor || staging)
      throw FormatError("Native object continuation requires its available TU1.5 executor");
  }
  struct PublicCall {
    Continuation &owner;
    const int exceptions=std::uncaught_exceptions();
    const bool mutating;
    std::uint64_t writes{};
    PublicCall(Continuation &owner,bool mutating=true):owner(owner),mutating(mutating) {
      std::lock_guard lock(owner.gate);
      if(owner.staging || (mutating && owner.retained_executor && std::this_thread::get_id()!=owner.executor))
        throw FormatError("Native object call requires its available retained executor");
      writes=owner.writes->load(std::memory_order_relaxed);++owner.calls;
      if(mutating)++owner.mutating_calls;
    }
    ~PublicCall() {
      std::lock_guard lock(owner.gate);
      if(mutating && std::uncaught_exceptions()>exceptions && owner.writes->load(std::memory_order_relaxed)!=writes)
        owner.fault=true;
      --owner.calls;
    }
  };
};
MultiplayerObjectRuntime::MultiplayerObjectRuntime(const std::filesystem::path &image,const Cache &cache,
    std::size_t bsp,const std::filesystem::path &files):MultiplayerObjectRuntime(image,cache,bsp,files,{}) {}
MultiplayerObjectRuntime::MultiplayerObjectRuntime(const std::filesystem::path &image,const Cache &cache,
    std::size_t bsp,const std::filesystem::path &files,std::shared_ptr<PhysicsLiveClockSource> clocks) {
  const auto original=cache.original_data()?cache.original_data():OriginalData::load_xbe(image);
  if(original->image==OriginalData::Image::TitleUpdate15) {
    if(clocks)tu15_=std::make_unique<PhysicsLiveWorldTu15>(cache,image,files,bsp,std::move(clocks));
    else tu15_=std::make_unique<PhysicsLiveWorldTu15>(cache,image,files,bsp);
  } else {
    if(clocks)throw FormatError("Native object clock provider requires its TU1.5 physics owner");
    impl_=std::make_unique<Impl>(image,cache,bsp,files);
  }
  continuation_=std::make_unique<Continuation>(tu15_?tu15_->memory():impl_->bank,bool(tu15_));
}
MultiplayerObjectRuntime::~MultiplayerObjectRuntime()=default;
PhysicsLiveWorldTu15 &MultiplayerObjectRuntime::tu15_world() {
  if(!tu15_)throw FormatError("Campaign requires its actual TU1.5 world owner");
  return *tu15_;
}
SourceRuntimeMemory &MultiplayerObjectRuntime::memory(){Continuation::PublicCall active(*continuation_,false);return tu15_?tu15_->memory():impl_->bank;}
std::vector<SourceAttachedEffect> MultiplayerObjectRuntime::attached_effects(std::uint32_t object) const {
  Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Completed map effect controls require the TU1.5 source owner");
  const auto &bank=tu15_->memory();const auto effects=word(bank,0x4f720cu);
  return source_attached_effects_tu15(bank,tu15_->object_pool(),bank.native_address(effects,0x4c),object);
}
std::uint32_t MultiplayerObjectRuntime::source_object_pool()const{Continuation::PublicCall active(*continuation_,false);return tu15_?tu15_->source_object_pool():impl_->object_pool();}
std::uint32_t MultiplayerObjectRuntime::source_player_pool()const{Continuation::PublicCall active(*continuation_,false);return tu15_?tu15_->source_player_pool():impl_->player_pool();}
NativeRuntimeAddress MultiplayerObjectRuntime::object_pool()const {Continuation::PublicCall active(*continuation_,false);
  const auto& bank=tu15_?tu15_->memory():impl_->bank;return bank.native_address(source_object_pool(),0x4c);
}
NativeRuntimeAddress MultiplayerObjectRuntime::player_pool()const {Continuation::PublicCall active(*continuation_,false);
  const auto& bank=tu15_?tu15_->memory():impl_->bank;return bank.native_address(source_player_pool(),0x4c);
}
NativeRuntimeAddress MultiplayerObjectRuntime::object_body(std::uint32_t object)const {Continuation::PublicCall active(*continuation_,false);
  const auto& bank=tu15_?tu15_->memory():impl_->bank;return bank.native_address(source_object_body(object),4);
}
bool MultiplayerObjectRuntime::owns_physics()const{Continuation::PublicCall active(*continuation_,false);return bool(tu15_);}
std::optional<std::uint32_t> MultiplayerObjectRuntime::physical_body_count()const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)return {};
  // Complete TU1.5 world construction publishes this counter owner; the
  // original registration/removal routines maintain its first DWORD.
  const auto owner=word(tu15_->memory(),0x52be44u);
  if(!owner)throw FormatError("TU1.5 world has no source physical-body counter");
  return word(tu15_->memory(),owner);
}
NativeRuntimeAddress MultiplayerObjectRuntime::source_clock()const {Continuation::PublicCall active(*continuation_,false);
  const auto& bank=tu15_?tu15_->memory():impl_->bank;
  return bank.native_address(word(bank,tu15_?0x51df54u:0x518444u),4);
}
std::uint32_t MultiplayerObjectRuntime::source_object_body(std::uint32_t object)const {Continuation::PublicCall active(*continuation_,false);
  if(tu15_)return tu15_->source_object_body(object);
  const SourceRawDatumPoolServices storage{.read=[this](auto p,auto n){return impl_->bank.view(p,n);}};
  const auto header=source_raw_datum_lookup(impl_->object_pool(),object,storage);
  if(!header)throw FormatError("Object body requires a live full source handle");
  return word(impl_->bank,header+8u);
}
void MultiplayerObjectRuntime::deliver_command(std::uint32_t player,std::uint32_t object,
    std::span<const std::byte,0x5c>command) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Source physics command delivery requires the TU1.5 owner");
  tu15_->deliver_command(player,object,command);
}
void MultiplayerObjectRuntime::prepare_player_look(std::uint32_t player,std::uint32_t object,
    std::span<std::byte,0x5c>command) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Source player look requires the TU1.5 owner");
  tu15_->prepare_player_look(player,object,command);
}
std::optional<Vec2> MultiplayerObjectRuntime::player_look(std::uint32_t player,std::uint32_t object) const {
  Continuation::PublicCall active(*continuation_,false);
  return tu15_?tu15_->player_look(player,object):std::optional<Vec2>{};
}
std::uint32_t MultiplayerObjectRuntime::object_definition(std::uint32_t object)const {Continuation::PublicCall active(*continuation_,false);
  return word(tu15_?tu15_->memory():impl_->bank,object_body(object));
}
std::uint32_t MultiplayerObjectRuntime::object_parent(std::uint32_t object)const {Continuation::PublicCall active(*continuation_,false);
  return word(tu15_?tu15_->memory():impl_->bank,object_body(object)+0x14u);
}
Transform MultiplayerObjectRuntime::object_transform(std::uint32_t object)const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Source physics transform readback requires the TU1.5 owner");
  return source_scene_physics_transform(tu15_->memory(),tu15_->source_object_pool(),object);
}
Vec3 MultiplayerObjectRuntime::object_linear_velocity(std::uint32_t object) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Object velocity presentation requires its TU1.5 physics owner");
  constexpr std::uint32_t output=0x60048000u;
  // BC200: EAX is the object, ECX the linear output, and the sole stack
  // argument is the optional angular output (RET 4). Preserve that register
  // ABI when entering the complete original routine.
  auto& bank=memory();put(bank,stack,0x60300000u);put(bank,stack+4u,0u);
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=object;abi.gp[1]=output;
  abi.gp[4]=stack;abi.fs_base=tls;abi.eflags=2;
  abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
  tu15_->run_actual(0xbc200u,abi);
  Vec3 result;memory().read(output,std::as_writable_bytes(std::span(&result,1)));return result;
}
ScriptValue MultiplayerObjectRuntime::script_device(const std::string &name,const std::vector<ScriptValue> &args) {
  Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Device script delivery requires the actual TU1.5 world");
  const bool group_get=name=="device_group_get",group_set=name=="device_group_set";
  const bool group_immediate=name=="device_group_set_immediate";
  const bool get=name=="device_get_position" || name=="device_get_power";
  const bool position=name=="device_set_position",power=name=="device_set_power";
  const bool position_immediate=name=="device_set_position_immediate";
  if(!group_get && !group_set && !group_immediate && !get && !position && !power && !position_immediate)
    throw FormatError("Unsupported original device script API");
  const unsigned arity=group_set?3u:(get || group_get)?1u:2u;
  if(args.size()!=arity)throw FormatError("Invalid original device script arity");
  auto &bank=tu15_->memory();
  const auto invoke=[&](std::uint32_t entry,std::span<const std::uint32_t> arguments,
      std::uint32_t object=null_datum,std::uint32_t eax=0u,std::uint32_t scalar=0u) {
    put(bank,stack,0x60300000u);
    for(unsigned n=0;n<arguments.size();++n)put(bank,stack+4u+n*4u,arguments[n]);
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp[0]=eax;abi.gp[4]=stack;abi.gp[6]=object;abi.gp[7]=object;
    abi.xmm[0][0]=scalar;abi.fs_base=tls;abi.eflags=2;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    tu15_->run_actual(entry,abi);return abi.gp[0];
  };
  if(group_get || group_set || group_immediate) {
    // Whole10C180 authenticates the authored ordinal and returns the actual
    // full group generation. Native script values retain authored ordinals.
    const auto ordinal=args[group_set?1u:0u].bits;
    const auto group=invoke(0x10c180u,{},null_datum,ordinal&0xffffu);
    if(group_get) {
      if(group==null_datum)return {6,0,{}};
      SourceRawDatumPoolServices storage{.read=[&](auto at,auto count){return bank.view(at,count);}};
      const auto row=source_raw_datum_lookup(word(bank,0x4ecbf8u),group,storage);
      if(!row)throw FormatError("Original device script getter lost its full group generation");
      return {6,word(bank,row+4u),{}};
    }
    const auto value=std::bit_cast<std::uint32_t>(float(args.back().number()));
    if(group_immediate) {invoke(0x10c4e0u,std::array{group},null_datum,0u,value);return {};}
    return ScriptValue::numeric((invoke(0x10c290u,std::array{group,value})&255u)!=0u,5);
  }
  const auto object=args[0].bits;
  if(object!=null_datum) {
    const auto body=object_body(object);
    const auto type=std::to_integer<unsigned>(bank.view(body+0xaau,1)[0]);
    if(type<7u || type>9u)throw FormatError("Device script requires an actual full DEVI object");
    if(get)return {6,word(bank,body+(name=="device_get_power"?0x134u:0x140u)),{}};
  }else if(get)return {6,0,{}};
  const auto value=std::bit_cast<std::uint32_t>(float(args[1].number()));
  if(position_immediate) {invoke(0x10c420u,{},object,0u,value);return {};}
  const auto result=invoke(power?0x10c240u:0x10c1f0u,std::array{value},object);
  return position?ScriptValue::numeric((result&255u)!=0u,5):ScriptValue{};
}
SourceVehicleSeatTarget MultiplayerObjectRuntime::vehicle_seat_target(std::uint32_t unit,std::uint32_t vehicle) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Live vehicle seat query requires its TU1.5 owner");
  (void)object_body(unit);(void)object_body(vehicle);
  constexpr std::uint32_t selected=0x6004a000u,seat=selected+4u;
  const std::array arguments{unit,vehicle,selected,seat};
  const auto kind=tu15_->call_actual(0xcb3d0u,arguments)&0xffffu;
  if(!kind)return {};
  if(kind>3u)throw FormatError("Original TU1.5 seat selector returned an invalid action kind");
  const auto object=word(memory(),selected);
  const auto index=std::bit_cast<std::int16_t>(std::uint16_t(word(memory(),seat)));
  if(object==0xffffffffu || index<0)throw FormatError("Original TU1.5 seat selector lost its admitted owner");
  return {std::int16_t(kind),index,object,object_transform(object).position};
}
MultiplayerSourceMotion MultiplayerObjectRuntime::biped_motion(std::uint32_t object)const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Source physics motion readback requires the TU1.5 owner");
  const auto body=tu15_->source_object_body(object);const auto &bank=tu15_->memory();
  const auto scalar=[&](std::uint32_t p){return std::bit_cast<float>(word(bank,p));};
  const auto vector=[&](std::uint32_t p){return Vec3{scalar(p),scalar(p+4),scalar(p+8)};};
  const auto root=object_transform(object);
  // Actual biped consumers E4A40 and E2720 own these object/unit fields.
  // Physical state is retained verbatim; it is not a guessed grounded flag.
  return {root.position,vector(body+0x88u),root.basis[0],
      scalar(body+0x2c4u),std::to_integer<std::uint8_t>(bank.view(body+0x3dcu,1)[0]),
      word(bank,body+0x14u),std::bit_cast<std::int16_t>(std::uint16_t(word(bank,body+0x1fcu)&0xffffu)),
      word(bank,body+0x134u),std::to_integer<std::uint8_t>(bank.view(body+0x10au,1)[0]),
      std::uint16_t(word(bank,body+0x348u)&0xffffu)};
}
void MultiplayerObjectRuntime::construct_player(std::uint32_t player,unsigned user,
    const SourcePlayerConfiguration &configuration) {Continuation::PublicCall active(*continuation_);
  if(tu15_)tu15_->construct_player(player,user,configuration,native_machine_identity());
  else impl_->construct_player(player,user,configuration);
}
void MultiplayerObjectRuntime::set_local_player(std::uint32_t player,std::int16_t view) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Original local-view composition requires the TU1.5 source world");
  tu15_->set_local_player(player,view);
}
void MultiplayerObjectRuntime::observe_sound_submissions(SourceSoundSubmitObserverTu15 observer) {
  Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Original sound submit observation requires TU1.5 ownership");
  tu15_->observe_sound_submissions(std::move(observer));
}
void MultiplayerObjectRuntime::observe_first_person_phase(SourceFirstPersonPhaseObserverTu15 observer) {
  Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("First-person phase diagnostics require the TU1.5 world");
  tu15_->observe_first_person_phase(std::move(observer));
}
std::vector<SourceFirstPersonModelTu15> MultiplayerObjectRuntime::prepare_first_person_models(unsigned view,
    Vec3 p,Vec3 f,Vec3 u,const SourceFirstPersonPhaseObserverTu15 &observer) {
  Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Source first-person draw preparation requires TU1.5 ownership");
  return tu15_->prepare_first_person_models(view,p,f,u,observer);
}
FirstPersonViewBinding MultiplayerObjectRuntime::first_person_view(unsigned view) const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Original first-person publication requires the TU1.5 source world");
  return tu15_->first_person_view(view);
}
void MultiplayerObjectRuntime::publish_native_observer_position(unsigned view,Vec3 position) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Native source observer publication requires the TU1.5 source world");
  tu15_->publish_native_observer_position(view,position);
}
MultiplayerSourceVitality MultiplayerObjectRuntime::object_vitality(std::uint32_t object)const {Continuation::PublicCall active(*continuation_,false);
  const auto body=object_body(object);const auto &bank=tu15_?tu15_->memory():impl_->bank;
  const auto scalar=[&](unsigned offset){return std::bit_cast<float>(word(bank,body+offset));};
  const auto timer=[&](unsigned offset){return std::bit_cast<std::int8_t>(
      std::to_integer<std::uint8_t>(bank.view(body+offset,1)[0]));};
  // Original D5DE0/BCC20 normalized vitality and recent-damage fields.
  // EC/F0 are fractions; E4/E8 are the separate authored maxima.
  return {scalar(0xec),scalar(0xf0),{{scalar(0xf4),scalar(0xfc),timer(0x108)},
      {scalar(0xf8),scalar(0x100),timer(0x109)}}};
}
void MultiplayerObjectRuntime::deliver_vitality(std::uint32_t object,const MultiplayerSourceVitality &state) {Continuation::PublicCall active(*continuation_);
  const auto body=object_body(object);auto &bank=memory();
  const std::array<std::pair<unsigned,float>,6> fields{{
      {0xec,state.body},{0xf0,state.shield},{0xf4,state.feedback.shield.visible},
      {0xf8,state.feedback.body.visible},{0xfc,state.feedback.shield.accumulated},
      {0x100,state.feedback.body.accumulated}}};
  for(const auto &[offset,value]:fields) {
    (void)offset;
    if(!std::isfinite(value))throw FormatError("Native vitality delivery contains a non-finite source fraction");
  }
  for(const auto &[offset,value]:fields)
    put(bank,body+offset,std::bit_cast<std::uint32_t>(value));
  const std::array timers{std::byte(std::bit_cast<std::uint8_t>(state.feedback.shield.ticks)),
      std::byte(std::bit_cast<std::uint8_t>(state.feedback.body.ticks))};
  bank.write(body+0x108u,timers);
}
void MultiplayerObjectRuntime::publish_roster_team(unsigned user,std::int8_t team) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Native team roster requires the TU1.5 live world");
  tu15_->publish_roster_team(user,team);
}
void MultiplayerObjectRuntime::bind_player(std::uint32_t player,std::uint32_t object) {Continuation::PublicCall active(*continuation_);
  if(tu15_)tu15_->bind_player(player,object);else impl_->bind_player(player,object);
}
bool MultiplayerObjectRuntime::depart_player(std::uint32_t player) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Retained ordinary departure requires the actual TU1.5 world");
  return tu15_->depart_player(player);
}
void MultiplayerObjectRuntime::deliver_direct_impact(std::uint32_t object,std::uint32_t damage,
    const DamageOwner &owner,const UnitDamageResult &result,Vec3 direction,Vec3 position) {Continuation::PublicCall active(*continuation_);
  deliver_damage_reaction(object,damage,owner,result,
      {DamageImpactContext::Kind::Direct,direction,position});
}
void MultiplayerObjectRuntime::deliver_damage_reaction(std::uint32_t object,std::uint32_t damage,
    const DamageOwner &owner,const UnitDamageResult &result,const DamageImpactContext &impact) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Damage reaction delivery requires the TU1.5 world");
  const auto body=object_body(object);auto &bank=memory();
  for(float value:{impact.direction.x,impact.direction.y,impact.direction.z,
      impact.position.x,impact.position.y,impact.position.z,impact.scale})
    if(!std::isfinite(value))throw FormatError("Damage impact contains a non-finite input");
  constexpr std::uint32_t report=0x60041000u,event=0x60042000u,accumulator=0x60043000u;
  constexpr std::uint32_t frame=0x600e0000u,caller=0x600e1000u,radial=0x600e2000u;
  const auto abi_for=[&](std::uint32_t stack) {
    put(bank,stack,0xf0000000u);
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.fs_base=tls;
    abi.eflags=2;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    return abi;
  };
  // Complete D7C60 produces request defaults from the selected image/current
  // BSP, including material/sentinels and all three scalar-one fields.
  auto request=abi_for(caller);request.gp[2]=event;put(bank,caller+4u,damage);
  tu15_->run_actual(0xd8bb0u,request);
  put(bank,event+8u,owner.player);put(bank,event+0xcu,owner.unit);
  const std::array designator{std::byte(std::uint16_t(owner.designator)&255u),
      std::byte(std::uint16_t(owner.designator)>>8)};bank.write(event+0x10u,designator);
  put(bank,event+0x54u,std::bit_cast<std::uint32_t>(impact.scale));
  const auto vector=[&](std::uint32_t destination,Vec3 value) {
    put(bank,destination,std::bit_cast<std::uint32_t>(value.x));
    put(bank,destination+4u,std::bit_cast<std::uint32_t>(value.y));
    put(bank,destination+8u,std::bit_cast<std::uint32_t>(value.z));
  };
  vector(event+0x30u,impact.position);vector(event+0x3cu,impact.direction);
  const auto tags=word(bank,0x4f0414u);
  if(impact.source_definition!=null_datum) {
    const auto projectile=word(bank,tags+(impact.source_definition&65535u)*16u+8u);
    const std::array reporting{std::byte(word(bank,projectile+0x128u)&63u)};
    bank.write(event+0x84u,reporting);
  }
  if(impact.kind==DamageImpactContext::Kind::Direct)
    put(bank,event+4u,word(bank,event+4u)|8u); // FEB40 direct request.
  if(impact.kind==DamageImpactContext::Kind::Melee) {
    // D10F0 intentionally uses attacker query bounds and source aim, with
    // event bit0 selecting +48. The contact point is not this request origin.
    const auto attacker=object_body(owner.unit);
    const auto slot=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bank.view(attacker+0x212u,1)[0]));
    if(slot>=0 && slot<4) {
      const auto weapon=object_body(word(bank,attacker+0x218u+unsigned(slot)*4u));
      const auto definition=word(bank,tags+(word(bank,weapon)&65535u)*16u+8u);
      const std::array reporting{std::byte(word(bank,definition+0x1fcu)&63u)};
      bank.write(event+0x84u,reporting); // D0830 -> D10F0 hit packet.
    }
    bank.write(event+0x30u,bank.view(attacker+0x30u,12));
    bank.write(event+0x3cu,bank.view(attacker+0x168u,12));
    bank.write(event+0x48u,bank.view(attacker+0x168u,12));
    put(bank,event+4u,word(bank,event+4u)|1u);
  } else if(impact.kind==DamageImpactContext::Kind::Radius) {
    // Authenticated D9A30 geometry block calls the actual closest-point owner,
    // then reads the retained physical root and produces both directions.
    // Native candidate/admission/falloff remain the area's existing owner.
    put(bank,event+4u,word(bank,event+4u)|1u);
    auto geometry=abi_for(radial);geometry.gp[5]=event;
    put(bank,radial+0x14u,bank.source_address(body,4));put(bank,radial+0x6cu,object);
    tu15_->run_actual(0xd9ab2u,geometry);
    if(geometry.gp[4]!=radial)throw FormatError("Radius geometry lost its source caller frame");
  }
  // DA2B0's fatal transition precedes reaction selection and physical delivery.
  // Cleanup remains later, after native attribution, with the player attached.
  if((result.report_flags&1u) && !(word(bank,body+0x10au)&4u)) {
    constexpr std::uint32_t attribution=0x60040000u;
    const std::array record{object,attribution};tu15_->call_actual(0xd8c20u,record);
    const std::array fatal{object,attribution,1u,0u};tu15_->call_actual(0xd8d50u,fatal);
  }
  const std::array<std::byte,0x58> report_clear{};bank.write(report,report_clear);
  const std::array<std::byte,0x34> accumulator_clear{};bank.write(accumulator,accumulator_clear);
  put(bank,accumulator+8u,0xffffffffu);put(bank,accumulator+0x10u,0xffffffffu);
  put(bank,accumulator+0x20u,std::bit_cast<std::uint32_t>(result.shield));
  put(bank,accumulator+0x28u,std::bit_cast<std::uint32_t>(result.body));
  put(bank,accumulator+0x30u,0x200u|(result.report_flags&1u)|((result.report_flags&2u)<<2));
  // Complete DAA90 owns report flags/direction/scales and calls CB1D0 itself.
  auto selection=abi_for(caller);selection.gp[0]=report;selection.gp[6]=event;
  selection.gp[7]=accumulator;put(bank,caller+4u,object);
  tu15_->run_actual(0xdb9e0u,selection);
  if(result.report_flags&1u) {
    const auto value=[&](unsigned offset){return std::bit_cast<float>(word(bank,report+offset));};
    death_reports_.insert_or_assign(object,DeathReport{
        std::bit_cast<std::int16_t>(std::uint16_t(word(bank,report+0x40u))),
        {value(0x18),value(0x1c),value(0x20)},(word(bank,report+4u)>>10)&1u});
  }
  auto vitality=object_vitality(object);
  report_damage_feedback(vitality.feedback,result.body,result.shield,result.report_flags,
      (word(bank,body+0x10au)&0x800u)!=0);
  deliver_vitality(object,vitality);
  const auto definition=word(bank,tags+(object_definition(object)&65535u)*16u+8u);
  const auto damage_definition=word(bank,tags+(damage&65535u)*16u+8u);
  put(bank,frame+0x10u,damage_definition);put(bank,frame+0x14u,definition);
  put(bank,frame+0x4cu,object);
  auto physical=abi_for(frame);physical.gp[5]=bank.source_address(body,4);physical.gp[7]=report;
  tu15_->run_actual(0xdbd64u,physical);
  if(physical.gp[4]!=frame)throw FormatError("Damage physical block lost its source caller frame");
  const auto reaction=word(bank,report+0x50u);
  if(std::bit_cast<std::int32_t>(reaction)>0) {
    const std::array response{object,reaction,0xffffu};
    call(0xcac00u,response,{},report+0x18u);
  }
}
SourcePlayerEquipmentTu15 MultiplayerObjectRuntime::initialize_player_equipment(std::uint32_t player,std::uint32_t object) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Source equipment initialization requires the TU1.5 physics owner");
  return tu15_->initialize_player_equipment(player,object);
}
SourcePlayerEquipmentTu15 MultiplayerObjectRuntime::player_equipment(std::uint32_t object) const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Source equipment readback requires the TU1.5 physics owner");
  return tu15_->player_equipment(object);
}
void MultiplayerObjectRuntime::kill_biped(std::uint32_t object) {Continuation::PublicCall active(*continuation_);
  if(!tu15_){impl_->kill_biped(object);return;}
  const auto body=object_body(object);
  // A source falling/kill request may already have completed CCF40 during
  // the world pass. Its detached physical corpse must not be cleaned twice.
  if(word(memory(),body+0x13cu)==null_datum &&
      memory().view(body+0x3dcu,1)[0]==std::byte{3}) {
    death_reports_.erase(object);return;
  }
  if(!(word(memory(),object_body(object)+0x10au)&4u)) {
    constexpr std::uint32_t damage=0x60040000u;
    const std::array record_args{object,damage};tu15_->call_actual(0xd8c20u,record_args);
    const std::array death_args{object,damage,1u,0u};tu15_->call_actual(0xd8d50u,death_args);
  }
  std::array cleanup_args{object,0xffffffffu,0u,0u,0u};
  if(const auto found=death_reports_.find(object);found!=death_reports_.end()) {
    constexpr std::uint32_t direction=0x60044000u;
    memory().write(direction,std::as_bytes(std::span(&found->second.direction,1)));
    cleanup_args[1]=std::uint32_t(std::int32_t(found->second.damage_class));
    cleanup_args[2]=direction;cleanup_args[4]=found->second.force;
  }
  tu15_->call_actual(0xccf40u,cleanup_args);death_reports_.erase(object);
}
void MultiplayerObjectRuntime::configure_session(const SourceGameVariant &variant,
    std::uint16_t mode,std::int16_t rate,std::uint32_t seed) {Continuation::PublicCall active(*continuation_);
  if(tu15_){tu15_->configure_session(variant,mode,rate,seed);tu15_->initialize_map();}
  else impl_->configure_session(variant,mode,rate,seed);
  death_reports_.clear();
}
std::uint32_t MultiplayerObjectRuntime::create_biped(std::uint32_t definition,
    const Vec3 &position,const Vec3 &forward,std::uint32_t player) {
  if(tu15_)return tu15_->create_biped(definition,position,forward,player);
  if(player!=0xffffffffu)throw FormatError("Player creation context requires the TU1.5 live world");
  return impl_->create_biped(definition,position,forward);
}
std::uint32_t MultiplayerObjectRuntime::create_scenario_object(const ScenarioObject &placement,SourceScenarioBirth birth) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Original scene body construction requires the TU1.5 physics owner");
  return tu15_->create_scenario_object(placement,birth);
}
std::uint32_t MultiplayerObjectRuntime::create_netgame_object(unsigned row) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Netgame body birth requires the TU1.5 live physics world");
  return tu15_->create_netgame_object(row);
}
std::uint32_t MultiplayerObjectRuntime::netgame_object(unsigned row)const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Netgame lookup requires the TU1.5 live physics world");
  return tu15_->netgame_object(row);
}
std::uint32_t MultiplayerObjectRuntime::child_object(std::uint32_t parent,std::uint32_t definition)const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Model child lookup requires the TU1.5 live physics world");
  return tu15_->child_object(parent,definition);
}
void MultiplayerObjectRuntime::destroy_object(std::uint32_t object) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Original body retirement requires the TU1.5 physics owner");
  tu15_->destroy_object(object);
  death_reports_.erase(object);
}
void MultiplayerObjectRuntime::set_unit_controller_active(std::uint32_t unit,bool requested){Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Unit controller admission requires the TU1.5 source world");
  tu15_->set_unit_controller_active(unit,requested);
}
void MultiplayerObjectRuntime::deliver_unit_input(std::uint32_t unit,Vec3 throttle,Vec3 facing){Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Unit input requires the TU1.5 source world");
  tu15_->deliver_unit_input(unit,throttle,facing);
}
void MultiplayerObjectRuntime::step_objects(){Continuation::PublicCall active(*continuation_);
  if(tu15_) {
    tu15_->step_objects();
    const auto &bank=tu15_->memory();const auto pool=tu15_->source_object_pool();
    const SourceRawDatumPoolServices storage{.read=[&](auto address,auto bytes){return bank.view(address,bytes);}};
    std::erase_if(death_reports_,[&](const auto &entry){return !source_raw_datum_lookup(pool,entry.first,storage);});
  } else impl_->step_objects();
}
SourceWeaponEffectPresentation MultiplayerObjectRuntime::request_weapon_effect(const WeaponEffectInput &input) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Original weapon effect allocation requires the TU1.5 world");
  return tu15_->request_weapon_effect(input);
}
std::uint32_t MultiplayerObjectRuntime::weapon_hud_target_flags(std::uint32_t unit) {
  Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Weapon HUD targeting requires the TU1.5 source owner");
  return tu15_->weapon_hud_target_flags(unit);
}
WeaponHudTarget MultiplayerObjectRuntime::weapon_hud_target(std::uint32_t weapon) {
  Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Weapon HUD locking requires the TU1.5 source owner");
  return tu15_->weapon_hud_target(weapon);
}
VehicleWeaponHudStatus MultiplayerObjectRuntime::vehicle_weapon_hud(std::uint32_t unit) {
  Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Vehicle weapon HUD requires the retained TU1.5 world");
  return tu15_->vehicle_weapon_hud(unit);
}
void MultiplayerObjectRuntime::populate_weapon_targeting(std::uint32_t player,std::uint32_t object,
    std::span<std::byte,0x5c> command) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Original live weapon targeting requires the TU1.5 world");
  tu15_->populate_weapon_targeting(player,object,command);
}
std::optional<ParticleCollisionHit> MultiplayerObjectRuntime::particle_collision(std::uint32_t flags,Vec3 origin,Vec3 delta) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Original particle collision requires the TU1.5 world");
  return tu15_->particle_collision(flags,origin,delta);
}
NativeGrenadeAction MultiplayerObjectRuntime::grenade_action(std::uint32_t unit)const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Grenade action readback requires the TU1.5 world");
  const auto body=tu15_->source_object_body(unit);const auto &bank=tu15_->memory();
  const auto offset=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,body+0x346u)));
  if(offset<0)throw FormatError("Source unit has no retained action component");
  NativeGrenadeAction result;bank.read(body+std::uint32_t(offset),result.source);
  if(!(result.active_actions()&(1u<<22)))return result;
  //116720 tests both retained secondary channels, starting at extension+9C.
  //Publish the source-selected graph/packed clip/frame rather than selecting
  //another throw or advancing a parallel animation timer.
  const auto extension=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,body+0x33eu)));
  if(extension<0)return result;
  for(const auto relative:{0x9cu,0xbcu}) {
    const auto channel=body+std::uint32_t(extension)+relative;
    const auto graph=word(bank,channel),packed=word(bank,channel+4u),activity=word(bank,channel+8u);
    if(graph==null_datum || (packed>>16)==0xffffu ||
        (activity!=0x0d000021u && activity!=0x1000006cu))continue;
    auto resolved=graph;
    const auto inherited=std::bit_cast<std::int16_t>(std::uint16_t(packed));
    if(inherited!=-1) {
      const auto metadata=word(bank,word(bank,0x4f0414u)+(graph&65535u)*16u+8u);
      if(inherited<0 || std::uint32_t(inherited)>=word(bank,metadata+0x4cu))
        throw FormatError("Source grenade animation has an invalid inherited graph");
      resolved=word(bank,word(bank,metadata+0x50u)+std::uint32_t(inherited)*32u+4u);
    }
    result.graph=resolved;result.clip=packed>>16;result.activity=activity;
    result.frame=std::bit_cast<float>(word(bank,channel+0x1cu));result.animation_active=true;
    return result;
  }
  return result;
}
std::int8_t MultiplayerObjectRuntime::current_grenade(std::uint32_t unit)const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Grenade selection readback requires the TU1.5 world");
  return std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(
      tu15_->memory().view(tu15_->source_object_body(unit)+0x23cu,1)[0]));
}
NativeMeleeAction MultiplayerObjectRuntime::melee_action(std::uint32_t unit)const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Melee action readback requires the TU1.5 world");
  const auto body=tu15_->source_object_body(unit);const auto &bank=tu15_->memory();
  const auto offset=std::bit_cast<std::int16_t>(std::uint16_t(word(bank,body+0x346u)));
  if(offset<0)throw FormatError("Source unit has no retained action component");
  const auto action=body+std::uint32_t(offset);NativeMeleeAction result;
  if(!(word(bank,action+4u)&(1u<<27)))return result;
  // Literal EC450/EC620 fields, retained by EBFA0 and its source clocks.
  const auto fields=bank.view(action+0x30u,6);
  result.activity=word(bank,action+0x2cu);
  result.modifiers=std::to_integer<std::uint8_t>(fields[0]);
  result.speed_modifier=std::to_integer<std::uint8_t>(fields[1]);
  result.progress=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(fields[2]));
  result.impact=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(fields[3]));
  result.recovery=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(fields[4]));
  result.duration=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(fields[5]));return result;
}
std::vector<MeleeImpact> MultiplayerObjectRuntime::take_melee_impacts() {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Source melee readback requires the TU1.5 world");
  return tu15_->take_melee_impacts();
}
std::vector<UnitDamageEffectEvent> MultiplayerObjectRuntime::take_unit_damage_effects() {Continuation::PublicCall active(*continuation_);
  if(!tu15_)return {};
  return tu15_->take_unit_damage_effects();
}
std::vector<SourceProjectileDetonationTu15> MultiplayerObjectRuntime::take_projectile_detonations() {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Source explosion readback requires the TU1.5 world");
  return tu15_->take_projectile_detonations();
}
void MultiplayerObjectRuntime::observe_body_velocity_inputs(SourceBodyVelocityInputObserverTu15 observer){Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Body velocity input observation requires its TU1.5 live world");
  tu15_->observe_body_velocity_inputs(std::move(observer));
}
void MultiplayerObjectRuntime::observe_physics_stores(SourcePhysicsStoreObserverTu15 observer){Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Physics store observation requires its TU1.5 live world");
  tu15_->observe_physics_stores(std::move(observer));
}
void MultiplayerObjectRuntime::observe_phantom_phase(SourcePhantomPhaseObserverTu15 observer){Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Phantom phase observation requires its TU1.5 live world");
  auto *memory=&tu15_->memory();
  source_bind_internal_phantom_observer(*memory,observer?
      SourceInternalPhantomObserver{[observer,memory](bool entering,const auto &abi){observer(entering,abi,*memory);}}:
      SourceInternalPhantomObserver{});
  tu15_->observe_phantom_phase(std::move(observer));
}
void MultiplayerObjectRuntime::observe_body_impulses(std::function<void(const SourceBodyImpulseTu15 &)> observer) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Point-impulse observation requires the TU1.5 world");
  tu15_->observe_body_impulses(std::move(observer));
}
void MultiplayerObjectRuntime::observe_weapon_effect_phase(SourceFirstPersonPhaseObserverTu15 observer) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Scheduled effect observation requires the TU1.5 source owner");
  tu15_->observe_weapon_effect_phase(std::move(observer));
}
void MultiplayerObjectRuntime::observe_weapon_effects(std::function<void(const SourceWeaponEffectEventTu15 &)> observer) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Source weapon-effect observation requires the TU1.5 source owner");
  tu15_->observe_weapon_effects(std::move(observer));
}
void MultiplayerObjectRuntime::observe_projectile_effects(std::function<void(const SourceProjectileDetonationTu15 &)> observer) {
  Continuation::PublicCall active(*continuation_);
  std::lock_guard lock(continuation_->gate);
  if(!tu15_)throw FormatError("Projectile contact observation requires its actual TU1.5 owner");
  if(auto *inputs=source_execution_inputs(continuation_->bank))inputs->invalidate_recording();
  if(observer && continuation_->projectile_effects)throw FormatError("Source world already has its projectile journal owner");
  if(observer)continuation_->projectile_effects=std::make_unique<SourceProjectileEffectObserver>(continuation_->bank,std::move(observer));
  else continuation_->projectile_effects.reset();
}
bool MultiplayerObjectRuntime::has_projectile_effect_observer() const {
  // Binding metadata stays readable while Match validates a staged world.
  std::lock_guard lock(continuation_->gate);return bool(continuation_->projectile_effects);
}
void MultiplayerObjectRuntime::retire_projectile_effects() noexcept {
  std::lock_guard lock(continuation_->gate);continuation_->projectile_effects.reset();
}
void MultiplayerObjectRuntime::observe_effect_damage(SourceEffectDamageObserverTu15 observer) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Effect damage observation requires the TU1.5 source owner");
  tu15_->observe_effect_damage(std::move(observer));
}
std::vector<NativeGrenadeProjectile> MultiplayerObjectRuntime::grenade_projectiles()const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Grenade body readback requires the TU1.5 world");
  const auto &bank=tu15_->memory();const auto pool=tu15_->source_object_pool();
  const auto globals=word(bank,0x4ecc1cu),count=word(bank,globals+0x100u),rows=word(bank,globals+0x104u);
  if(count>2)throw FormatError("Source grenade presentation exceeds the authored native grenade bank");
  const auto scalar=[&](auto p){return std::bit_cast<float>(word(bank,p));};
  const auto short_at=[&](auto p){return std::bit_cast<std::int16_t>(std::uint16_t(word(bank,p)));};
  std::vector<NativeGrenadeProjectile> result;
  const auto high=word(bank,pool+0x38u),headers=word(bank,pool+0x44u),stride=word(bank,pool+0x24u);
  if(stride!=12 || high>word(bank,pool+0x20u))throw FormatError("Source projectile header pool layout changed");
  for(unsigned i=0;i<high;++i) {
    if(!(word(bank,word(bank,pool+0x48u)+(i>>5)*4u)&(1u<<(i&31u))))continue;
    const auto header=headers+i*stride;
    if(bank.view(header+3u,1)[0]!=std::byte{5})continue;
    // A native projectile may reserve a shared full handle without creating
    //an original body. It retains its native motion/presentation owner; an
    //uncommitted header is not a completed source grenade to adopt.
    if(!word(bank,header+8u))continue;
    const auto datum=((word(bank,header)&65535u)<<16)|i,body=tu15_->source_object_body(datum),definition=word(bank,body);
    unsigned type=0;for(;type<count;++type)if(word(bank,rows+type*0x2cu+0x28u)==definition)break;
    if(type==count)continue;
    const auto root=object_transform(datum);NativeGrenadeProjectile p;
    p.datum=datum;p.definition=definition;p.type=std::int16_t(type);p.parent=word(bank,body+0x14u);
    // B9960 request+68/+6C/+70 -> B9B70 body+C4/+C8/+C2.
    p.player=word(bank,body+0xc4u);p.unit=word(bank,body+0xc8u);p.designator=short_at(body+0xc2u);
    p.position=root.position;p.forward=root.basis[0];p.up=root.basis[2];
    p.velocity={scalar(body+0x88u),scalar(body+0x8cu),scalar(body+0x90u)};
    // Literal FD200 constructor and projectile timer/material consumers.
    p.flags=word(bank,body+0x12cu);p.state=short_at(body+0x130u);p.age=short_at(body+0x1aau);
    p.timer=scalar(body+0x158u);p.timer_step=scalar(body+0x15cu);
    p.arming=scalar(body+0x160u);p.arming_step=scalar(body+0x164u);p.traveled=scalar(body+0x168u);
    result.push_back(p);
  }
  return result;
}
std::vector<SourceLiveObjectTu15> MultiplayerObjectRuntime::live_objects()const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Live object readback requires the TU1.5 world");
  return source_live_objects_tu15(tu15_->memory(),tu15_->object_pool());
}
SourceModelRegionBank MultiplayerObjectRuntime::object_model_regions(std::uint32_t object)const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)throw FormatError("Live model regions require the TU1.5 world");
  return source_live_model_regions_tu15(tu15_->memory(),tu15_->object_pool(),object);
}
std::vector<Transform> MultiplayerObjectRuntime::world_nodes(std::uint32_t object)const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)return impl_->world_nodes(object);
  const auto &bank=tu15_->memory();const auto body=tu15_->source_object_body(object);
  const auto short_at=[&](std::uint32_t p) {
    const auto b=bank.view(p,2);
    return std::bit_cast<std::int16_t>(std::uint16_t(std::to_integer<unsigned>(b[0])|
        (std::to_integer<unsigned>(b[1])<<8)));
  };
  const auto bytes=short_at(body+0x114u),offset=short_at(body+0x116u);
  if(bytes<=0 || bytes%0x34 || offset<0)
    throw FormatError("TU1.5 object has no complete current WORLD node bank");
  const auto first=body+std::uint32_t(offset);bank.view(first,std::size_t(bytes));
  const auto scalar=[&](std::uint32_t p){return std::bit_cast<float>(word(bank,p));};
  const auto vector=[&](std::uint32_t p){return Vec3{scalar(p),scalar(p+4),scalar(p+8)};};
  std::vector<Transform> result(std::size_t(bytes)/0x34);
  for(std::size_t i=0;i<result.size();++i) {
    const auto p=first+std::uint32_t(i)*0x34;
    result[i]={scalar(p),{vector(p+4),vector(p+16),vector(p+28)},vector(p+40)};
  }
  return result;
}
ShadowCasterSphere MultiplayerObjectRuntime::object_sphere(std::uint32_t object)const {Continuation::PublicCall active(*continuation_,false);
  if(!tu15_)return impl_->object_sphere(object);
  const auto body=tu15_->source_object_body(object);const auto &bank=tu15_->memory();
  const auto scalar=[&](std::uint32_t p){return std::bit_cast<float>(word(bank,p));};
  return {{scalar(body+0x30u),scalar(body+0x34u),scalar(body+0x38u)},scalar(body+0x3cu)};
}
std::optional<DeathCameraTarget> MultiplayerObjectRuntime::camera_target(std::uint32_t object)const {Continuation::PublicCall active(*continuation_,false);
  return source_death_camera_target(tu15_?tu15_->memory():impl_->bank,object_pool(),object);
}
DeathCameraPacket MultiplayerObjectRuntime::player_camera_packet(std::uint32_t player,float seconds,
    SourceCameraControllerState &controller,SourceObserverState &observer,std::uint32_t &random,
    bool cycle,std::uint8_t *mode) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Player camera scheduler requires the TU1.5 owner");
  return tu15_->player_camera_packet(player,seconds,controller,observer,random,cycle,mode);
}
DeathCameraPacket MultiplayerObjectRuntime::unit_camera_packet(std::uint32_t unit,float seconds,
    SourceCameraControllerState &controller,SourceObserverState &observer,std::uint8_t *camera_mode) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Unit camera scheduler requires the TU1.5 owner");
  return tu15_->unit_camera_packet(unit,seconds,controller,observer,camera_mode);
}
SourceObserverWorldServices MultiplayerObjectRuntime::observer_world_services() {Continuation::PublicCall active(*continuation_,false);
  if(tu15_) {
    const auto invoke=[this](std::uint32_t entry,std::span<const std::uint32_t> args,
                             std::uint32_t eax,std::uint32_t edx,std::uint32_t esi) {
      Continuation::PublicCall active(*continuation_);
      auto a=tu15_->prepare_source_call(args);
      a.gp[0]=eax;a.gp[2]=edx;a.gp[6]=esi;
      tu15_->run_actual(entry,a);return a.gp[0];
    };
    SourceObserverWorldServices result;
    result.locate=[this,invoke](Vec3 position) {
      Continuation::PublicCall active(*continuation_);
      constexpr std::uint32_t input=0x60030000u,output=0x60030100u;
      auto &bank=tu15_->memory();tu15_->write_source_call_input(input,std::as_bytes(std::span(&position,1)));
      // Actual1746B9 supplies EDX point and ESI full eight-byte location.
      invoke(0x11e600u,{},0,input,output);
      return SourceObserverLocation{{word(bank,output),word(bank,output+4u)}};
    };
    result.fog_side=[this,locate=result.locate](Vec3 position) {
      Continuation::PublicCall active(*continuation_);
      // Literal TU1.5 1746BE..174749, preserving scalar SSE operation order.
      const auto cluster=locate(position).cluster();if(cluster==-1)return false;
      const auto &bank=tu15_->memory();const auto sbsp=word(bank,0x4ecc18u);
      const auto row=word(bank,sbsp+0xa0u)+std::uint32_t(std::int32_t(cluster))*0xb0u;
      const auto index=std::to_integer<unsigned>(bank.view(row+0x70u,1)[0]);
      if(index==0xffu)return false;
      const auto plane=word(bank,sbsp+0x68u)+(index&0x7fu)*24u;
      const auto field=bank.view(plane+2u,2);
      if(field[0]==std::byte{255} && field[1]==std::byte{255})return false;
      if(!(index&0x80u))return true;
      const auto scalar=[&](auto p){return std::bit_cast<float>(word(bank,p));};
      volatile float z=scalar(plane+0xcu)*position.z,y=scalar(plane+8u)*position.y;
      volatile float zy=z+y,x=position.x*scalar(plane+4u),dot=zy+x;
      volatile float distance=dot-scalar(plane+0x10u);
      return 0.f>distance;
    };
    result.request_visibility=[invoke](std::int32_t cluster) {
      // Actual174654 retains EAX cluster when calling the visibility owner.
      invoke(0x3f4f0u,{},std::bit_cast<std::uint32_t>(cluster),0,0);
    };
    result.collision=[this,invoke](std::uint32_t flags,Vec3 origin,Vec3 delta)->std::optional<float> {
      Continuation::PublicCall active(*continuation_);
      constexpr std::uint32_t start=0x60030200u,direction=0x60030220u,hit=0x60030300u;
      auto &bank=tu15_->memory();tu15_->write_source_call_input(start,std::as_bytes(std::span(&origin,1)));
      tu15_->write_source_call_input(direction,std::as_bytes(std::span(&delta,1)));
      // Actual174933/174A44 call complete16C620 with the original exclusions
      // NONE and its complete5C result. Only the original miss returns null.
      const std::array args{flags,start,direction,0xffffffffu,0xffffffffu,hit};
      if(!(invoke(0x16c620u,args,0,0,0)&255u))return {};
      return std::bit_cast<float>(word(bank,hit+4u));
    };
    return result;
  }
  return impl_->observer_world_services();
}
SourceObserverWorldFrame MultiplayerObjectRuntime::observer_world_frame(std::int32_t view,
    std::uint32_t current,std::uint16_t flags,const DeathCameraSavedClusters &saved)const {Continuation::PublicCall active(*continuation_,false);
  const auto &bank=tu15_?tu15_->memory():impl_->bank;
  const auto bsp=bank.view(tu15_?0x474c24u:0x46fb64u,2);
  const auto index=std::bit_cast<std::int16_t>(std::uint16_t(std::to_integer<unsigned>(bsp[0])|
      (std::to_integer<unsigned>(bsp[1])<<8)));
  return {view,index,std::bit_cast<std::int32_t>(word(bank,word(bank,tu15_?0x4ecc18u:0x4e7918u)+0x9c)),
      current,flags,saved};
}
std::uint32_t MultiplayerObjectRuntime::call(std::uint32_t entry,std::span<const std::uint32_t> args,
    std::optional<std::uint32_t> eax,std::optional<std::uint32_t> ecx,std::optional<std::uint32_t> edx) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)return impl_->call(entry,args,eax,ecx,edx);
  SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=stack;a.fs_base=tls;a.eflags=2;a.floating_point=fp;
  put(tu15_->memory(),stack,0x60300000u);
  for(unsigned i=0;i<args.size();++i)put(tu15_->memory(),stack+4u+4u*i,args[i]);
  if(eax)a.gp[0]=*eax;if(ecx)a.gp[1]=*ecx;if(edx)a.gp[2]=*edx;
  for(const auto &binding:physics_live_entries_tu15::entries)if(binding.canonical==entry) {
    tu15_->run_actual(binding.actual,a);return a.gp[0];
  }
  throw FormatError("Multiplayer TU1.5 correspondence entry is unavailable");
}
void MultiplayerObjectRuntime::preload_render_model(std::uint32_t model) {Continuation::PublicCall active(*continuation_);
  if(tu15_){tu15_->preload_render_model(model);return;}
  impl_->preload_render_model(model);
}
void MultiplayerObjectRuntime::begin_model_frame() {Continuation::PublicCall active(*continuation_);
  if(tu15_){tu15_->begin_model_frame();return;}
  impl_->begin_model_frame();
}
void MultiplayerObjectRuntime::render_model_view(const std::function<void(PhysicsLiveWorldTu15&)>& draw) {
  Continuation::PublicCall active(*continuation_);
  if(!tu15_ || !draw)throw FormatError("Live model view requires its TU1.5 world and completed draw consumer");
  tu15_->begin_model_view();
  draw(*tu15_);
}
void MultiplayerObjectRuntime::completed_main_camera_observer(SourceCompletedMainCameraObserverTu15 observer) {
  std::unique_lock lock(continuation_->gate,std::try_to_lock);continuation_->require_executor();
  if(!lock.owns_lock() || continuation_->calls || !tu15_ || (continuation_->fault && observer))
    throw FormatError("Main camera observer requires its completed actual TU1.5 owner");
  Continuation::PublicCall active(*continuation_,false);
  tu15_->completed_main_camera_observer(std::move(observer));
}
void MultiplayerObjectRuntime::inspect_render_memory(const SourceRenderMemoryInspectorTu15& inspect) const {
  std::unique_lock lock(continuation_->gate,std::try_to_lock);continuation_->require_executor();
  if(!lock.owns_lock() || continuation_->calls || continuation_->fault || !tu15_ || !inspect)
    throw FormatError("Render memory inspection requires its completed actual TU1.5 owner");
  Continuation::PublicCall active(*continuation_,false);
  tu15_->inspect_render_memory(inspect);
}
std::optional<std::vector<Transform>> MultiplayerObjectRuntime::rendered_world_nodes(std::uint32_t object)const {
  Continuation::PublicCall active(*continuation_,false);
  return tu15_?tu15_->rendered_world_nodes(object):std::nullopt;
}
SourceObjectAdmission MultiplayerObjectRuntime::admit_render_objects(const SourcePortalVisibilitySet& visibility,
    const SourceObjectAdmissionInput& input,std::span<const std::array<std::uint32_t,4>> prior)const {Continuation::PublicCall active(*continuation_,false);
  if(tu15_)return tu15_->admit_render_objects(visibility,input,prior);
  throw FormatError("Live object admission requires authenticated TU1.5 spatial owners");
}
SourceObjectDrawCollector MultiplayerObjectRuntime::build_world_model(std::uint32_t object,unsigned view,
    unsigned players,bool modifier,float metric,float lighting_metric) {Continuation::PublicCall active(*continuation_);
  if(tu15_)return tu15_->build_world_model(object,view,players,modifier,metric,lighting_metric);
  return impl_->build_world_model(object,view,players,modifier,metric,lighting_metric);
}

bool MultiplayerObjectRuntime::authority_executor_thread() const noexcept {
  return continuation_->retained_executor&&std::this_thread::get_id()==continuation_->executor;
}
void MultiplayerObjectRuntime::require_authority_executor(bool qualified) const {
  auto &continuation=*continuation_;
  std::unique_lock lock(continuation.gate,std::try_to_lock);
  if(!lock.owns_lock())throw FormatError("Native object authority executor is busy");
  continuation.require_executor();
  if(continuation.calls)throw FormatError("Native object authority executor has an incomplete call");
  if(qualified){
    if(continuation.fault)throw FormatError("Native object authority executor has an incomplete mutation");
    tu15_->require_execution_boundary();
  }
}
std::array<std::uint64_t,2> MultiplayerObjectRuntime::execution_mutations()const {
  require_authority_executor(true);
  return {continuation_->writes->load(std::memory_order_relaxed),continuation_->mutating_calls};
}
MultiplayerObjectRuntime::AuthorityCheckpoint MultiplayerObjectRuntime::authority_checkpoint() const {
  auto &continuation=*continuation_;
  std::unique_lock lock(continuation.gate,std::try_to_lock);continuation.require_executor();
  if(!lock.owns_lock() || continuation.calls || continuation.fault)
    throw FormatError("Native object authority export requires completed source and bridge calls");
  AuthorityCheckpoint saved;saved.world=tu15_->authority_checkpoint();saved.death_reports=death_reports_;
  return saved;
}
struct MultiplayerObjectRuntime::PreparedAuthority::Impl {
  MultiplayerObjectRuntime &owner;
  std::unique_lock<std::recursive_mutex> gate;
  struct Barrier {
    Continuation &owner;
    explicit Barrier(Continuation &owner):owner(owner){owner.staging=true;}
    ~Barrier(){owner.staging=false;}
  };
  std::optional<Barrier> barrier;
  std::optional<PhysicsLiveWorldTu15::PreparedAuthority> world;
  std::map<std::uint32_t,RetainedDeathReport> reports;
  Impl(MultiplayerObjectRuntime &owner,const AuthorityCheckpoint &saved)
      :owner(owner),gate(owner.continuation_->gate,std::try_to_lock) {
    owner.continuation_->require_executor();
    if(!gate.owns_lock() || owner.continuation_->calls || saved.version!=1)
      throw FormatError("Native object authority import requires a completed TU1.5 owner");
    barrier.emplace(*owner.continuation_);
    world.emplace(owner.tu15_->prepare_authority(saved.world));
    const auto &memory=world->replacement_memory();
    // Same constructor-owned TU1.5 object pool as source_object_pool().
    const auto pool=word(memory,0x4ecbd0u);
    const SourceRawDatumPoolServices storage{.read=[&](auto address,auto bytes){return memory.view(address,bytes);}};
    if(saved.death_reports.size()>word(memory,pool+0x20u))
      throw FormatError("Native pending damage reports exceed the original object pool");
    for(const auto &[object,report]:saved.death_reports) {
      const auto header=source_raw_datum_lookup(pool,object,storage);
      if(!header || std::to_integer<unsigned>(memory.view(header+3u,1)[0])>1 || report.force>1 ||
          !std::isfinite(report.direction.x) || !std::isfinite(report.direction.y) || !std::isfinite(report.direction.z))
        throw FormatError("Native pending damage report has a stale or invalid source unit");
      const auto body=word(memory,header+8u);
      if(!body || !(word(memory,body+0x10au)&4u))
        throw FormatError("Native pending damage report lost its actual source fatal transition");
    }
    reports=saved.death_reports;
  }
};
MultiplayerObjectRuntime::PreparedAuthority::PreparedAuthority()=default;
MultiplayerObjectRuntime::PreparedAuthority::~PreparedAuthority()=default;
MultiplayerObjectRuntime::PreparedAuthority::PreparedAuthority(PreparedAuthority &&) noexcept=default;
MultiplayerObjectRuntime::PreparedAuthority &MultiplayerObjectRuntime::PreparedAuthority::operator=(PreparedAuthority &&) noexcept=default;
bool MultiplayerObjectRuntime::PreparedAuthority::for_owner(const MultiplayerObjectRuntime &owner) const noexcept {
  return impl_ && &impl_->owner==&owner;
}
const SourceRuntimeMemory &MultiplayerObjectRuntime::PreparedAuthority::replacement_memory() const {
  if(!impl_)throw FormatError("Native object authority preparation has no replacement bank");
  return impl_->world->replacement_memory();
}
MultiplayerObjectRuntime::PreparedAuthority MultiplayerObjectRuntime::prepare_authority(const AuthorityCheckpoint &saved) {
  PreparedAuthority prepared;prepared.impl_=std::make_unique<PreparedAuthority::Impl>(*this,saved);return prepared;
}
bool MultiplayerObjectRuntime::commit_authority(PreparedAuthority &&prepared) noexcept {
  if(!prepared.for_owner(*this) || std::this_thread::get_id()!=continuation_->executor)return false;
  auto &state=*prepared.impl_;
  if(!tu15_->commit_authority(std::move(*state.world)))return false;
  death_reports_.swap(state.reports);continuation_->fault=false;prepared.impl_.reset();return true;
}
void MultiplayerObjectRuntime::publish_render_visibility(const SourcePortalVisibilitySet& v) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Particle visibility requires its actual TU1.5 world");
  tu15_->publish_render_visibility(v);
}
void MultiplayerObjectRuntime::advance_effect_particles(float seconds,const SourceFirstPersonPhaseObserverTu15& observer) {Continuation::PublicCall active(*continuation_);
  if(!tu15_)throw FormatError("Particle frames require their actual TU1.5 world");
  tu15_->advance_effect_particles(seconds,observer);
}
} // namespace halo2
