#include "halo2/multiplayer_object_runtime.hpp"
#include <cstring>
#include "halo2/cache.hpp"
#include "halo2/native_threads.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/native_object_lifetimes.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_process_heap_platform.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/source_damage_runtime.hpp"
#include "halo2/death_camera_binding.hpp"
#include "halo2/source_segment_collision.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_object_render_entry.hpp"
#include "halo2/source_model_build.hpp"
#include "halo2/source_render_cache.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/native_tu15_startup_timer.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/original_data.hpp"
#include "physics_live_entries_tu15.hpp"
#include <algorithm>
#include <bit>
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
    }
    SourceModelBuildWorld w;auto &m=w.admission.memory;
    m.banks={word(bank,0x4eb114),word(bank,0x4e7918),word(bank,0x4e7914),object_pool(),
      [this](auto p,auto n){return bank.view(p,n);},
      [this](auto flags,auto p){return (call(0x12f5e0,{},flags,p)&255u)!=0;}};
    m.write=[this](auto p,auto n){return bank.mutable_view(p,n);};
    m.view=std::int32_t(view);m.render_frame=word(bank,0x4c1554);m.render_cache_pool=word(bank,0x510c24);
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
MultiplayerObjectRuntime::MultiplayerObjectRuntime(const std::filesystem::path &image,const Cache &cache,
    std::size_t bsp,const std::filesystem::path &files) {
  const auto original=cache.original_data()?cache.original_data():OriginalData::load_xbe(image);
  if(original->image==OriginalData::Image::TitleUpdate15)
    tu15_=std::make_unique<PhysicsLiveWorldTu15>(cache,image,files,bsp);
  else impl_=std::make_unique<Impl>(image,cache,bsp,files);
}
MultiplayerObjectRuntime::~MultiplayerObjectRuntime()=default;
SourceRuntimeMemory &MultiplayerObjectRuntime::memory(){return tu15_?tu15_->memory():impl_->bank;}
std::uint32_t MultiplayerObjectRuntime::object_pool()const{return tu15_?tu15_->object_pool():impl_->object_pool();}
std::uint32_t MultiplayerObjectRuntime::player_pool()const{return tu15_?tu15_->player_pool():impl_->player_pool();}
bool MultiplayerObjectRuntime::owns_physics()const{return bool(tu15_);}
std::optional<std::uint32_t> MultiplayerObjectRuntime::physical_body_count()const {
  if(!tu15_)return {};
  // Complete TU1.5 world construction publishes this counter owner; the
  // original registration/removal routines maintain its first DWORD.
  const auto owner=word(tu15_->memory(),0x52be44u);
  if(!owner)throw FormatError("TU1.5 world has no source physical-body counter");
  return word(tu15_->memory(),owner);
}
std::uint32_t MultiplayerObjectRuntime::source_clock()const {
  return word(tu15_?tu15_->memory():impl_->bank,tu15_?0x51df54u:0x518444u);
}
std::uint32_t MultiplayerObjectRuntime::object_body(std::uint32_t object)const {
  if(tu15_)return tu15_->object_body(object);
  const SourceRawDatumPoolServices storage{.read=[this](auto p,auto n){return impl_->bank.view(p,n);}};
  const auto header=source_raw_datum_lookup(impl_->object_pool(),object,storage);
  if(!header)throw FormatError("Object body requires a live full source handle");
  return word(impl_->bank,header+8u);
}
void MultiplayerObjectRuntime::deliver_command(std::uint32_t player,std::uint32_t object,
    std::span<const std::byte,0x5c>command) {
  if(!tu15_)throw FormatError("Source physics command delivery requires the TU1.5 owner");
  tu15_->deliver_command(player,object,command);
}
MultiplayerSourceMotion MultiplayerObjectRuntime::biped_motion(std::uint32_t object)const {
  if(!tu15_)throw FormatError("Source physics motion readback requires the TU1.5 owner");
  const auto body=tu15_->object_body(object);const auto &bank=tu15_->memory();
  const auto scalar=[&](std::uint32_t p){return std::bit_cast<float>(word(bank,p));};
  const auto vector=[&](std::uint32_t p){return Vec3{scalar(p),scalar(p+4),scalar(p+8)};};
  // Actual biped consumers E4A40 and E2720 own these object/unit fields.
  // Physical state is retained verbatim; it is not a guessed grounded flag.
  return {vector(body+0x64u),vector(body+0x88u),vector(body+0x70u),
      scalar(body+0x2c4u),std::to_integer<std::uint8_t>(bank.view(body+0x3dcu,1)[0])};
}
void MultiplayerObjectRuntime::construct_player(std::uint32_t player,unsigned user,
    const SourcePlayerConfiguration &configuration) {
  if(tu15_)tu15_->construct_player(player,user,configuration,native_machine_identity());
  else impl_->construct_player(player,user,configuration);
}
void MultiplayerObjectRuntime::bind_player(std::uint32_t player,std::uint32_t object) {
  if(tu15_)tu15_->bind_player(player,object);else impl_->bind_player(player,object);
}
SourcePlayerEquipmentTu15 MultiplayerObjectRuntime::initialize_player_equipment(std::uint32_t player,std::uint32_t object) {
  if(!tu15_)throw FormatError("Source equipment initialization requires the TU1.5 physics owner");
  return tu15_->initialize_player_equipment(player,object);
}
SourcePlayerEquipmentTu15 MultiplayerObjectRuntime::player_equipment(std::uint32_t object) const {
  if(!tu15_)throw FormatError("Source equipment readback requires the TU1.5 physics owner");
  return tu15_->player_equipment(object);
}
void MultiplayerObjectRuntime::kill_biped(std::uint32_t object) {
  if(!tu15_){impl_->kill_biped(object);return;}
  constexpr std::uint32_t damage=0x60040000u;
  const std::array record_args{object,damage};tu15_->call_actual(0xd8c20u,record_args);
  const std::array death_args{object,damage,1u,0u};tu15_->call_actual(0xd8d50u,death_args);
  const std::array cleanup_args{object,0xffffffffu,0u,0u,0u};tu15_->call_actual(0xccf40u,cleanup_args);
}
void MultiplayerObjectRuntime::configure_session(const SourceGameVariant &variant,
    std::uint16_t mode,std::int16_t rate,std::uint32_t seed) {
  if(tu15_){tu15_->configure_session(variant,mode,rate,seed);tu15_->initialize_map();}
  else impl_->configure_session(variant,mode,rate,seed);
}
std::uint32_t MultiplayerObjectRuntime::create_biped(std::uint32_t definition,
    const Vec3 &position,const Vec3 &forward) {
  return tu15_?tu15_->create_biped(definition,position,forward):impl_->create_biped(definition,position,forward);
}
std::uint32_t MultiplayerObjectRuntime::create_scenario_object(const ScenarioObject &placement) {
  if(!tu15_)throw FormatError("Original scene body construction requires the TU1.5 physics owner");
  return tu15_->create_scenario_object(placement);
}
std::uint32_t MultiplayerObjectRuntime::create_netgame_object(unsigned row) {
  if(!tu15_)throw FormatError("Netgame body birth requires the TU1.5 live physics world");
  return tu15_->create_netgame_object(row);
}
void MultiplayerObjectRuntime::destroy_object(std::uint32_t object) {
  if(!tu15_)throw FormatError("Original body retirement requires the TU1.5 physics owner");
  tu15_->destroy_object(object);
}
void MultiplayerObjectRuntime::step_objects(){if(tu15_)tu15_->step_objects();else impl_->step_objects();}
std::vector<Transform> MultiplayerObjectRuntime::world_nodes(std::uint32_t object)const {
  if(!tu15_)return impl_->world_nodes(object);
  const auto &bank=tu15_->memory();const auto body=tu15_->object_body(object);
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
ShadowCasterSphere MultiplayerObjectRuntime::object_sphere(std::uint32_t object)const {
  if(!tu15_)return impl_->object_sphere(object);
  const auto body=tu15_->object_body(object);const auto &bank=tu15_->memory();
  const auto scalar=[&](std::uint32_t p){return std::bit_cast<float>(word(bank,p));};
  return {{scalar(body+0x30u),scalar(body+0x34u),scalar(body+0x38u)},scalar(body+0x3cu)};
}
std::optional<DeathCameraTarget> MultiplayerObjectRuntime::camera_target(std::uint32_t object)const {
  return source_death_camera_target(tu15_?tu15_->memory():impl_->bank,object_pool(),object);
}
SourceObserverWorldServices MultiplayerObjectRuntime::observer_world_services() {
  if(tu15_) {
    const auto invoke=[this](std::uint32_t entry,std::span<const std::uint32_t> args,
                             std::uint32_t eax,std::uint32_t edx,std::uint32_t esi) {
      auto &bank=tu15_->memory();SourcePhysicsWorldRuntimeAbi a{};
      a.gp[4]=stack;a.fs_base=tls;a.eflags=2;a.floating_point=fp;
      a.gp[0]=eax;a.gp[2]=edx;a.gp[6]=esi;
      put(bank,stack,0x60300000u);
      for(unsigned i=0;i<args.size();++i)put(bank,stack+4u+4u*i,args[i]);
      tu15_->run_actual(entry,a);return a.gp[0];
    };
    SourceObserverWorldServices result;
    result.locate=[this,invoke](Vec3 position) {
      constexpr std::uint32_t input=0x60030000u,output=0x60030100u;
      auto &bank=tu15_->memory();bank.write(input,std::as_bytes(std::span(&position,1)));
      // Actual1746B9 supplies EDX point and ESI full eight-byte location.
      invoke(0x11e600u,{},0,input,output);
      return SourceObserverLocation{{word(bank,output),word(bank,output+4u)}};
    };
    result.fog_side=[this,locate=result.locate](Vec3 position) {
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
      constexpr std::uint32_t start=0x60030200u,direction=0x60030220u,hit=0x60030300u;
      auto &bank=tu15_->memory();bank.write(start,std::as_bytes(std::span(&origin,1)));
      bank.write(direction,std::as_bytes(std::span(&delta,1)));
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
    std::uint32_t current,std::uint16_t flags,const DeathCameraSavedClusters &saved)const {
  const auto &bank=tu15_?tu15_->memory():impl_->bank;
  const auto bsp=bank.view(tu15_?0x474c24u:0x46fb64u,2);
  const auto index=std::bit_cast<std::int16_t>(std::uint16_t(std::to_integer<unsigned>(bsp[0])|
      (std::to_integer<unsigned>(bsp[1])<<8)));
  return {view,index,std::bit_cast<std::int32_t>(word(bank,word(bank,tu15_?0x4ecc18u:0x4e7918u)+0x9c)),
      current,flags,saved};
}
std::uint32_t MultiplayerObjectRuntime::call(std::uint32_t entry,std::span<const std::uint32_t> args,
    std::optional<std::uint32_t> eax,std::optional<std::uint32_t> ecx,std::optional<std::uint32_t> edx) {
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
void MultiplayerObjectRuntime::preload_render_model(std::uint32_t model) {
  if(tu15_)throw FormatError("TU1.5 native model-resource adapter is unresolved");
  impl_->preload_render_model(model);
}
void MultiplayerObjectRuntime::begin_model_frame() {
  if(tu15_)throw FormatError("TU1.5 native model-builder adapter is unresolved");
  impl_->begin_model_frame();
}
SourceObjectDrawCollector MultiplayerObjectRuntime::build_world_model(std::uint32_t object,unsigned view,
    unsigned players,bool modifier,float metric,float lighting_metric) {
  if(tu15_)throw FormatError("TU1.5 native model-builder adapter is unresolved");
  return impl_->build_world_model(object,view,players,modifier,metric,lighting_metric);
}
} // namespace halo2
