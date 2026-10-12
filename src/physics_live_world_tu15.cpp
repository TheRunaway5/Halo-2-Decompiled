#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/physics_phantom_world_tu15.hpp"
#include "halo2/physics_game_arena_native.hpp"
#include "halo2/physics_early_initializer_runtime.hpp"
#include "halo2/physics_renderer_storage_runtime.hpp"
#include "halo2/physics_main_heap_runtime.hpp"
#include "halo2/physics_platform_locks.hpp"
#include "halo2/source_process_heap_platform.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/native_threads.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/source_runtime_cache_tags.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/player_command.hpp"
#include "halo2/source_damage_delivery_runtime.hpp"
#include "physics_live_entries_tu15.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <mutex>
#include <sstream>
#include <thread>
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

namespace halo2 {
namespace {
constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;
constexpr SourcePhysicsFactoryFloatingPoint fp{0x27f,0x1f80};
constexpr std::uint32_t stack=0x600f0000u,tls=0x60200000u,
    parameters=0x60201000u,process_context=0x60210000u,
    notifications=0x60211000u,allocation_callback=0xf7000001u,
    native_tick_count=0x60212000u,native_thread_context=0x60213000u;
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p) {
    std::array<std::byte,4>b;m.read(p,b);std::uint32_t v=0;
    for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
    return v;
}
void put(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4>b;
    for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);
}
}
struct PhysicsLiveWorldTu15::Impl {
    const Cache&cache_owner;
    SourceRuntimeMemory bank;
    SourceRuntimeHostVirtualMemory virtual_memory;
    SourcePhysicsPoolHostStorage pool_storage;
    PhysicsPlatformLocks locks;
    NativeSynchronization events{{0xf6000001u,0xf6010000ull}};
    NativeSynchronizationServices event_services=events.services();
    std::map<std::uint32_t,std::uint32_t> lock_events;
    std::map<std::thread::id,std::uint32_t> thread_ids;
    std::mutex mutex;
    const std::thread::id main_thread=std::this_thread::get_id();
    std::uint32_t last_instruction{};
    struct Allocation {std::uint32_t payload,bytes,flags;};
    std::vector<Allocation> allocations;
    struct NativeResourceRead {std::uint32_t descriptor,destination,bytes;};
    std::map<std::uint32_t,NativeResourceRead> resource_reads;
    std::uint32_t next_resource_read=0xf8000001u;
    SourceRuntimeCacheBspImage bsp;
    std::map<std::uint32_t,std::uint32_t> bipeds;
    bool session_configured{};
    struct NativeExitRegistrations {
        std::vector<std::function<void()>> callbacks;
        ~NativeExitRegistrations() {for(auto i=callbacks.rbegin();i!=callbacks.rend();++i)(*i)();}
    } native_exits;
    // Destroy and join before destroying callbacks, locks or retained memory.
    std::unique_ptr<SourcePhysicsGameArenaNative> arena;

    Impl(const Cache&cache,const std::filesystem::path&xbe,const std::filesystem::path&files,std::size_t selected)
        :cache_owner(cache),bank(load_source_runtime_image(xbe,image,SourceRuntimeKernelTickCount{
            native_tick_count,{},"native monotonic millisecond tick-count data import"})),virtual_memory(bank,0x08000000u),
         pool_storage(bank,0x10000000u),bsp(cache.load_source_runtime_cache_bsp_image(selected)) {
        bank.mount(0x60000000u,std::vector<std::byte>(0x100000),
            SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 multiplayer main call stack");
        bank.mount(tls,std::vector<std::byte>(0x2000),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 exception chain and process caller parameters");
        put(bank,tls,0xffffffffu);
        // Retained current-thread input for the original CRT/XAPI consumers.
        // This native executor has no Xbox TLS expansion; the original CRT
        // consequently constructs its main-thread state at573840 itself.
        bank.mount(native_thread_context+0x28u,std::vector<std::byte>(4),
            SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 main-thread CRT TLS policy");
        bank.mount(native_thread_context+0x12cu,std::vector<std::byte>(4),
            SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 retained executor thread identity");
        put(bank,tls+0x28u,native_thread_context);
        put(bank,native_thread_context+0x12cu,lock_services().current_thread());
        bank.mount(process_context+0x250,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 process allocation notification pointer");
        bank.mount(notifications+0x14,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 process allocation consumer");
        put(bank,tls+0x20,process_context);put(bank,process_context+0x250,notifications);
        put(bank,notifications+0x14,allocation_callback);
        // Import156 is data, not an address in the map's virtual range. Bind
        // a retained native DWORD so the encoded ordinal cannot alias cache
        // bytes at8000009C. The original33B590 loads and RET still execute.
        if(word(bank,0x41b774u)!=native_tick_count)
            throw FormatError("TU1.5 tick-count import identity differs");

        // The native process invokes actual2DE2EA with the literal caller's
        // 2DBA60..2DBA8C inputs. Publish its returned heap at the consumed CRT
        // binding. Unrun hardware/timer/XAPI startup is not reported complete.
        put(bank,parameters,0x30);
        const std::array args{2u,0u,word(bank,0x10134),word(bank,0x10138),0u,parameters};
        auto a=frame(args);
        source_run_process_heap_with_native_platform(bank,0x2da83a,a,image,fp,
            services(),virtual_memory,locks,lock_services());
        if(!a.gp[0])throw std::runtime_error("TU1.5 native process heap construction failed");
        put(bank,0x56ee28,a.gp[0]);

        arena=std::make_unique<SourcePhysicsGameArenaNative>(bank,
            SourcePhysicsGameArenaConfiguration{files,0x60110000u,fp,{}},services());
        for(auto entry:{0x12cb70u,0x1221b0u,0x125280u}) {
            a=frame({});arena->run(entry,a);
        }
        // Main-thread source construction has exclusive access after the
        // startup worker has retired the real file jobs and is stopped/joined.
        arena->stop();
        // Actual CRT row46DA08 constructs the static Havok scratch owner.
        // Its real native exit registration is retained through bank lifetime.
        call_actual(0x381370u);
        call_actual(0x3825d0u); // Actual CRT game-engine class registration.
        initialize_game_storage();
        call_actual(0x148d40u); // Actual startup row3 constructs both RNG streams.
        call_actual(0x149050u); // Actual source clock storage owner, row17.
        call_actual(0x1f3830u); // Actual source gravity/physics constants, row25.
        call_actual(0x1f4700u); // Actual44B308 scenario-instance update bitmap owner.
        call(0x133760); // actual134B00: full renderer raw/descriptor construction.
        call(0x1c5f00); // actual1CB130: real pools/main heap/renderer overflow join.
        call_actual(0xb87f0); // Actual source object header/body storage constructors.
        call_actual(0x10e3b0u); // Actual object activation bookkeeping owner, row28.
        call_actual(0x1789f0u); // Actual body/component datum owners, row40.
        call_actual(0x14dfe0u); // Actual source multiplayer player storage, row19.
        call_actual(0x159f80u); // Actual source game-engine storage, row22.
        call_actual(0x125c90u); // Actual12054 resource-cache construction.
        call_actual(0x12ebe0u); // Actual1281D texture-cache construction.
        call_actual(0x1303c0u); // Actual12818 model-resource construction.
        call_actual(0x1183f0u); // Actual473A00 antenna datum storage.
        call_actual(0x18b5c0u); // Actual startup row44B548 object-looping-sound pool.
        initialize_object_looping_sound_storage();
        call_actual(0x168ba3u); // Actual1410AC first-person view storage owner.
        call_actual(0x168bc5u); // Actual1410C2 resets its real four-view bank.
        initialize_ai_storage_prefix();
        source_bind_runtime_cache_tags_tu15(bank,cache.load_source_runtime_cache_tag_image());
        // Original1918D9 prepares every resident collision, biped, creature,
        // physical model and vehicle definition. In particular11F02A passes
        // zero to1F2C70, resolving serialized shape references before birth.
        call_actual(0x11ef70u);
        for(const auto&tag:cache.select_tags({.group=tag_groups::biped}))bipeds.emplace(tag.datum,tag.address);
        a=frame({});a.gp[0]=bsp.mapping;
        source_run_physics_live_support_tu15(bank,0x1255c0,a,fp,services());
        if(!(a.gp[0]&255u))throw FormatError("TU1.5 source BSP publication failed");
        // Original11EA1D/11EA20 prepares the selected BSP's instanced
        // collision MOPP shapes after resource publication and before binding.
        // Their serialized vtables are not covered by the resident tag pass.
        a=frame({});a.gp[0]=bsp.bsp;run_actual(0x171120u,a);
        source_bind_runtime_cache_bsp_tu15(bank,bsp);
        call_actual(0x10e410u);
        call_actual(0x178ab0u);
        call_actual(0x1cb200u);
        call_actual(0x14e0b0u);
        call_actual(0x125eb0u); // Actual191A57 cache activation/reset.
        call_actual(0x130600u); // Actual191A89 model-resource activation/reset.
        call_actual(0x1f4760u); // Actual44B310 resets the scenario-instance bitmap.
        call_actual(0x118430u); // Actual473A04 activates/resets antenna storage.
    }
    void initialize_object_looping_sound_storage() {
        // Literal18B660..18B71C storage prefix. The original2279E0 audio
        // volume/reset and18BB50 profile consumers are separate native audio
        // composition; neither is replaced with a fabricated callee result.
        const auto pool=word(bank,0x4f9b5cu),state=word(bank,0x4f9b58u);
        if(!pool || !state)throw FormatError("TU1.5 looping-sound storage constructor failed");
        const std::array active{std::byte{1}};bank.write(pool+0x29u,active);
        auto a=frame({});a.gp[6]=pool;run_actual(0x16e600u,a);
        for(unsigned i=0;i<8;++i)put(bank,state+i*4u,0xffffffffu);
        put(bank,state+0x20u,0);put(bank,state+0x24u,0);
        for(unsigned i=0;i<0x40;++i)put(bank,state+0x28u+i*4u,0);
        for(unsigned i=0;i<0x40;++i)put(bank,state+0x128u+i*4u,0xffffffffu);
        const auto one=word(bank,0x46a0d0u);
        for(auto offset:{0x228u,0x22cu,0x230u,0x234u})put(bank,state+offset,one);
        put(bank,state+0x238u,0x1f40u);put(bank,state+0x23cu,0);
    }
    void initialize_ai_storage_prefix() {
        // Actual1CFA40..1CFAB4 allocates these PVS/player records and invokes
        // complete1D0250. Later AI subsystem callbacks remain separate owners.
        for(const auto&[size,slot]:std::array<std::pair<std::uint32_t,std::uint32_t>,2>{
            {{0x374u,0x5020a8u},{0x38u,0x5020a4u}}}) {
            const auto base=word(bank,0x4f2950u),cursor=word(bank,0x4f2954u),address=base+cursor;
            if(!base || address<base || cursor>0xffffffffu-size)
                throw FormatError("TU1.5 PVS arena allocation wraps");
            bank.mutable_view(address,size);put(bank,0x4f2954u,cursor+size);
            put(bank,parameters+0x100u,size);
            const std::array args{0x4f295cu};auto a=frame(args);
            a.gp[0]=parameters+0x100u;a.gp[7]=4u;
            source_run_physics_game_arena_runtime(bank,0x1656a0u,a,image,fp,services());
            put(bank,slot,address);
        }
        call_actual(0x1d0250u);
    }
    void initialize_game_storage() {
        // Actual13A708..13A756: native subsystem composition owns the later
        // 68 startup callbacks. Preserve this source storage prefix, including
        // the source CRC and null selected-BSP short, without publishing ready.
        const auto base=word(bank,0x4f2950u),cursor=word(bank,0x4f2954u);
        const auto game=base+cursor;
        if(!base || game<base || cursor>0xffffffffu-0x1200u)
            throw FormatError("TU1.5 game-options source arena allocation wraps");
        bank.mutable_view(game,0x1200);
        put(bank,0x4f2954u,cursor+0x1200u);
        put(bank,parameters+0x100u,0x1200u);
        const std::array checksum_args{0x4f295cu};
        auto a=frame(checksum_args);a.gp[0]=parameters+0x100u;a.gp[7]=4u;
        source_run_physics_game_arena_runtime(bank,0x1656a0u,a,image,fp,services());
        for(unsigned i=0;i<0x480;++i)put(bank,game+i*4u,0);
        put(bank,0x4f3218u,game);
        const std::array none{std::byte{0xff},std::byte{0xff}};bank.write(game+2u,none);
    }
    PhysicsPlatformLockServices lock_services() {
        const auto event=[this](std::uint32_t p) {
            std::lock_guard guard(mutex);
            const auto at=lock_events.find(p);if(at!=lock_events.end())return at->second;
            const auto created=event_services.create_semaphore(0,1);
            if(created.error)throw std::system_error(created.error);
            lock_events.emplace(p,created.handle);return created.handle;
        };
        return {
            [this] {
                std::lock_guard guard(mutex);const auto id=std::this_thread::get_id();
                const auto at=thread_ids.find(id);if(at!=thread_ids.end())return at->second;
                const auto token=std::uint32_t(thread_ids.size()+1);thread_ids.emplace(id,token);return token;
            },
            [this,event](auto p) {acquire_native_synchronization(event_services,event(p),0xffffffffu,{},5);},
            [this,event](auto p) {
                const auto r=event_services.release_semaphore(event(p),1);
                if(r.error)throw std::system_error(r.error);
            }
        };
    }
    SourcePhysicsWorldRuntimeServices services() {
        SourcePhysicsWorldRuntimeServices s;
        s.instruction=[this](auto p) {
            if(std::this_thread::get_id()==main_thread)last_instruction=p;
            if(p==0x33b595u) {
                const auto elapsed=std::chrono::steady_clock::now().time_since_epoch();
                put(bank,native_tick_count,std::uint32_t(
                    std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()));
            }
        };
#if defined(__i386__) || defined(__x86_64__)
        s.timestamp_counter=[] {return __rdtsc();};
        // Explicit host CPU estimate policy; Xbox estimate bits remain a
        // separate hardware comparison. The original Newton steps still run.
        s.reciprocal_sqrt=[](std::uint32_t bits) {
            return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(
                _mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));
        };
#endif
        s.required_owner=[this](auto&m,const auto&c,auto&a,const auto&o){required(m,c,a,o);};
        return s;
    }
    void required(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&c,
        SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&o) {
        if(&m!=&bank)throw std::runtime_error("TU1.5 live world bank owner differs");
        if(source_try_native_process_heap_memory_call(bank,c,a,image,virtual_memory))return;
        if(source_try_native_physics_lock_call(m,c,a,image,locks,lock_services()))return;
        if(pool_storage.try_call(m,c,a))return;
        if(c.instruction==0x1301b0u && c.target==0x1a3b70u && !c.tail) {
            // Actual1300B0 has allocated the original texture cache block and
            // produced its hardware descriptor. Native file I/O fills that
            // block; Vulkan presentation owns its separate GPU resources.
            const auto sp=a.gp[4],tracking=a.gp[6];
            const auto descriptor=word(m,tracking+0x10u),level=word(m,tracking+4u);
            const auto size=word(m,sp+4u),destination=word(m,sp+8u);
            const auto pool=word(m,0x4f2d24u),rows=word(m,pool+0x44u);
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x1301b5u ||
                level>=3u || tracking<rows || (tracking-rows)%0x28u ||
                (tracking-rows)/0x28u>=word(m,pool+0x38u) ||
                word(m,sp+12u)!=0 || word(m,sp+16u)!=tracking+3u || word(m,sp+20u)!=3u ||
                a.gp[1]!=word(m,descriptor+0x1cu+level*4u) ||
                size!=((word(m,descriptor+0x34u+level*4u)+511u)&~511u) ||
                destination!=word(m,descriptor+0x40u+level*4u))
                throw FormatError("TU1.5 texture resource I/O differs from its actual source cache block");
            const auto bytes=cache_owner.load_source_runtime_resource(a.gp[1],size);
            bank.write(destination,bytes);
            const auto job=next_resource_read++;
            resource_reads.emplace(job,NativeResourceRead{descriptor,destination,size});
            const std::array done{std::byte{1}};bank.write(tracking+3u,done);
            a.gp[0]=job;a.gp[4]+=28u;return;
        }
        if(c.instruction==0x126139u && c.target==0x1a3b70u && !c.tail) {
            // Explicit native file-I/O boundary of the original1260C0 cache
            // owner. It has already allocated the real cache block and job
            // tracking record. Deliver actual sector bytes, retain a native
            // completed request identity, and signal its I/O completion only
            // after the read and destination write succeed. Xbox async queue
            // scheduling is not asserted by this synchronous native policy.
            const auto sp=a.gp[4],tracking=a.gp[7];
            const auto descriptor=word(m,tracking+0xcu),size=word(m,sp+4),destination=word(m,sp+8);
            const auto resource=cache_owner.load_source_runtime_animation_image(descriptor);
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x12613eu ||
                word(m,sp+12)!=0 || word(m,sp+16)!=tracking+4u || word(m,sp+20)!=6u ||
                !std::ranges::equal(bank.view(descriptor,12),std::span(resource.descriptor).first(12)) ||
                a.gp[1]!=word(m,descriptor+8u) || size!=((resource.bytes.size()+511u)&~511u))
                throw FormatError("TU1.5 animation resource I/O differs from its actual cache descriptor");
            auto bytes=cache_owner.load_source_runtime_resource(a.gp[1],size);
            bank.write(destination,bytes);
            const auto job=next_resource_read++;
            resource_reads.emplace(job,NativeResourceRead{descriptor,destination,size});
            const std::array done{std::byte{1}};bank.write(tracking+4u,done);
            a.gp[0]=job;a.gp[4]+=28;return;
        }
        if(source_physics_live_support_tu15_owns_entry(c.target)) {
            source_run_physics_live_support_tu15(m,c.target,a,fp,services(),o);return;
        }
        const auto sp=a.gp[4];
        if(c.instruction==0x33a4d3u && c.target==0x80000080u && c.tail) {
            // NtQuerySystemTime receives the actual32BBAE local output cell.
            // Native UTC is converted to the original 100ns epoch; the real
            // CRT time conversion, rand and stream seed instructions execute.
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x32bbbcu ||
                word(m,sp+4u)!=a.gp[5]-8u)
                throw FormatError("TU1.5 native system-time caller differs");
            const auto elapsed=std::chrono::system_clock::now().time_since_epoch();
            const auto ticks=std::uint64_t(std::chrono::duration_cast<
                std::chrono::duration<std::int64_t,std::ratio<1,10000000>>>(elapsed).count())+
                116444736000000000ull;
            const auto output=word(m,sp+4u);
            put(m,output,std::uint32_t(ticks));put(m,output+4u,std::uint32_t(ticks>>32u));
            a.gp[4]+=8u;return;
        }
        if(c.instruction==0x2db4d1u && c.target==0x800000b6u && c.tail) {
            if(word(m,0x41b648u)!=c.target || !arena)
                throw FormatError("TU1.5 resource-cache physical protection import differs");
            arena->protect_pages(word(m,sp+4),word(m,sp+8),word(m,sp+12));
            a.gp[4]+=16;return;
        }
        if(c.instruction==0x381389u && c.target==0x32b0b4u && !c.tail) {
            if(word(m,sp)!=0x38138eu || word(m,sp+4)!=0x383970u)
                throw FormatError("TU1.5 Havok scratch native exit registration frame differs");
            native_exits.callbacks.push_back([this]{call_actual(0x383970u);});
            // A native C runtime registration: actual retained callback and
            // failure propagation, not a claim that the Xbox CRT table ran.
            a.gp[0]=0;a.gp[4]+=4;return;
        }
        if(c.instruction==0x1255e7u && c.target==0x125870u && !c.tail) {
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x1255ecu ||
                word(m,sp+4)!=0xffffffffu || word(m,sp+8)!=bsp.file_offset ||
                a.gp[0]!=bsp.file_offset || a.gp[2]!=bsp.region.address ||
                a.gp[6]!=((bsp.region.bytes.size()+511u)&~511u))
                throw FormatError("TU1.5 BSP I/O frame differs from the actual cache resource");
            source_deliver_runtime_cache_bsp_tu15(bank,bsp);
            a.gp[0]=(a.gp[0]&0xffffff00u)|1u;a.gp[4]+=12;return;
        }
        // Actual fresh-segment and retained-segment commitment frames in the
        // unchanged TU1.5 support bodies. This is the explicit native resource
        // ABI: source owns all free lists, headers, splitting and publication.
        struct VmFrame {std::uint32_t site,ret;int base,size;std::uint32_t type;};
        constexpr std::array vm_frames{
            VmFrame{0x2dd3d8u,0x2dd3deu,0xc,0,0x1000u},
            VmFrame{0x2ddf6fu,0x2ddf71u,-4,8,0x2000u},
            VmFrame{0x2ddf95u,0x2ddf97u,-4,8,0x2000u},
            VmFrame{0x2ddfcau,0x2ddfccu,-4,-0x10,0x1000u}};
        for(const auto&f:vm_frames)if(c.instruction==f.site && c.target==0x800000b8u) {
            const auto bp=word(m,sp+4),np=word(m,sp+12);
            const auto expected_size=f.size ? a.gp[5]+std::uint32_t(f.size) : a.gp[3];
            if(c.tail || word(m,sp)!=f.ret || bp!=a.gp[5]+std::uint32_t(f.base) ||
                np!=expected_size || word(m,sp+8)!=0 || word(m,sp+16)!=f.type || word(m,sp+20)!=4)
                throw std::runtime_error("TU1.5 live process growth resource frame differs");
            const auto result=virtual_memory.allocate({word(m,bp),0,word(m,np),f.type,4});
            put(m,bp,result.base);put(m,np,result.bytes);a.gp[0]=result.status;a.gp[4]+=24;return;
        }
        for(const auto&e:physics_live_entries_tu15::entries)if(c.target==e.actual) {
            using Module=physics_live_entries_tu15::Module;
            switch(e.module) {
            case Module::Damage:source_run_damage_delivery_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::World:source_run_physics_world_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::Pool:source_run_physics_pool_startup_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::Arena:source_run_physics_game_arena_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::MainHeap:source_run_physics_main_heap_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::Early:source_run_physics_early_initializer_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::Renderer:source_run_physics_renderer_storage_runtime(m,e.canonical,a,image,fp,services(),o);return;
            case Module::Process:
                source_run_process_heap_with_native_platform(bank,e.canonical,a,image,fp,
                    services(),virtual_memory,locks,lock_services(),o);return;
            case Module::Phantom: {
                auto control=a.floating_point.value_or(fp);
                source_run_physics_phantom_world_tu15_runtime(m,e.actual,a,control,services(),o);
                a.floating_point=control;return;
            }
            }
        }
        if(c.instruction==0x2db3b0u && c.target==allocation_callback && !c.tail) {
            if(std::this_thread::get_id()!=main_thread || word(m,sp)!=0x2dee63u || word(m,sp+4)!=10u)
                throw std::runtime_error("TU1.5 native allocation notification frame differs");
            const auto p=word(m,sp+8);allocations.push_back({word(m,p),word(m,p+4),word(m,p+8)});
            a.gp[4]+=12;return;
        }
        std::ostringstream message;message<<"TU1.5 live world owner missing at "<<std::hex
            <<c.instruction<<" -> "<<c.target<<" return "<<word(m,sp);
        throw std::runtime_error(message.str());
    }
    SourcePhysicsWorldRuntimeAbi frame(std::span<const std::uint32_t>args) {
        if(std::this_thread::get_id()!=main_thread)
            throw std::runtime_error("TU1.5 live world caller thread differs");
        SourcePhysicsWorldRuntimeAbi a{};a.eflags=2;a.fs_base=tls;a.gp[4]=stack;
        a.floating_point=fp;put(bank,stack,0x60300000u);
        if(args.size()>0x1000)throw std::runtime_error("TU1.5 live call arguments exceed native frame");
        for(unsigned i=0;i<args.size();++i)put(bank,stack+4+i*4,args[i]);return a;
    }
    void run(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi&a,
        const SourcePhysicsContactRecordObserver&o={}) {
        try {
            if(entry==0x133760u || entry==0x1360f0u || entry==0x165740u)
                source_run_physics_renderer_storage_with_native_pages(bank,entry,a,fp,services(),pool_storage,o);
            else if(entry==0x1c5f00u || entry==0x1f15b0u)
                source_run_physics_early_initializer_with_native_storage(bank,entry,a,fp,services(),pool_storage,o);
            else source_run_physics_world_runtime(bank,entry,a,image,fp,services(),o);
        }catch(const std::exception&e) {
            std::ostringstream message;message<<"TU1.5 live root "<<std::hex<<entry
                <<" at "<<last_instruction<<": "<<e.what();
            throw std::runtime_error(message.str());
        }
    }
    std::uint32_t call(std::uint32_t entry,std::span<const std::uint32_t>args={}) {
        auto a=frame(args);run(entry,a);return a.gp[0];
    }
    std::uint32_t call_actual(std::uint32_t entry,std::span<const std::uint32_t>args={}) {
        auto a=frame(args);
        run_actual(entry,a);return a.gp[0];
    }
    void run_actual(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi&a,
                    const SourcePhysicsContactRecordObserver&o={}) {
        if(std::this_thread::get_id()!=main_thread)
            throw FormatError("TU1.5 live entry requires its retained executor thread");
        try {
            required(bank,{entry,entry,false},a,o);
        }catch(const std::exception&e) {
            std::ostringstream message;message<<"TU1.5 live actual root "<<std::hex<<entry
                <<" at "<<last_instruction<<": "<<e.what();
            message<<" [gp";
            for(const auto value:a.gp)message<<' '<<value;
            message<<']';
            if(a.gp[4]>=0x60000000u && a.gp[4]<=0x600fffc0u) {
                message<<" [stack";
                for(unsigned i=0;i<8;++i)message<<' '<<word(bank,a.gp[4]+i*4u);
                message<<']';
            }
            throw std::runtime_error(message.str());
        }
    }
    std::uint32_t record(std::uint32_t pool,std::uint32_t datum) const {
        SourceRawDatumPoolServices storage{.read=[this](auto p,auto n){return bank.view(p,n);}};
        return source_raw_datum_lookup(pool,datum,storage);
    }
    std::uint32_t object_body(std::uint32_t datum) const {
        const auto header=record(word(bank,0x4ecbd0u),datum);
        if(!header)throw FormatError("TU1.5 body requires a live full source object identity");
        const auto body=word(bank,header+8u);
        if(!body)throw FormatError("TU1.5 object has no source-created body");
        return body;
    }
    void construct_player(std::uint32_t player,unsigned user,
        const SourcePlayerConfiguration&configuration,const SourceMachineIdentity&machine) {
        if(!session_configured || user>=4 || !record(word(bank,0x4f54f4u),player))
            throw FormatError("TU1.5 player construction requires an allocated source player and native local user");
        constexpr std::uint32_t creation=0x60020000u;
        std::array<std::byte,0xac> input{};
        input[2]=input[3]=std::byte{0xff};
        std::fill_n(input.begin()+4,4,std::byte{0xff});
        const auto identity=source_offline_player_identity(machine,std::int32_t(user));
        for(unsigned i=0;i<machine.size();++i)input[8u+i]=std::byte(machine[i]);
        for(unsigned i=0;i<identity.size();++i)input[0xeu+i]=std::byte(identity[i]);
        for(unsigned i=0;i<configuration.size();++i)input[0x1cu+i]=std::byte(configuration[i]);
        bank.write(creation,input);
        const std::array args{player,1u,creation};call_actual(0x14ebe0u,args);
    }
    void bind_player(std::uint32_t player,std::uint32_t object) {
        if(!record(word(bank,0x4f54f4u),player))
            throw FormatError("TU1.5 player binding requires a live full source player identity");
        if(object!=0xffffffffu)object_body(object);
        const std::array args{player};auto a=frame(args);a.gp[3]=object;
        run_actual(0x14f620u,a);
    }
    SourcePlayerEquipmentTu15 player_equipment(std::uint32_t object) const {
        const auto body=object_body(object);
        const auto bytes=bank.view(body+0x210u,0x30u);
        const auto short_at=[&](unsigned at) {
            return std::uint16_t(std::to_integer<unsigned>(bytes[at])|
                (std::to_integer<unsigned>(bytes[at+1])<<8));
        };
        SourcePlayerEquipmentTu15 equipment;
        equipment.inventory_sequence=short_at(0);
        equipment.replicated_sequence=short_at(4);
        for(unsigned i=0;i<2;++i) {
            equipment.current[i]=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bytes[2+i]));
            equipment.requested[i]=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bytes[6+i]));
            equipment.grenades[i]=std::to_integer<std::uint8_t>(bytes[0x2eu+i]);
        }
        for(unsigned i=0;i<4;++i) {
            equipment.weapons[i]=word(bank,body+0x218u+i*4u);
            equipment.ready_ticks[i]=word(bank,body+0x228u+i*4u);
            equipment.definitions[i]=equipment.weapons[i]==0xffffffffu?0xffffffffu:
                word(bank,object_body(equipment.weapons[i]));
        }
        return equipment;
    }
    SourcePlayerEquipmentTu15 initialize_player_equipment(std::uint32_t player,std::uint32_t object) {
        const auto owner=record(word(bank,0x4f54f4u),player);
        if(!session_configured || !owner || word(bank,owner+0x2cu)!=object)
            throw FormatError("TU1.5 equipment requires the bound source player and unit");
        const std::array args{object};call_actual(0xb1a70u,args);
        const std::array action{0x400u};auto a=frame(action);a.gp[0]=object;
        run_actual(0xb1c30u,a);
        a=frame({});a.gp[1]=object;run_actual(0x1613b0u,a);
        return player_equipment(object);
    }
    void deliver_command(std::uint32_t player,std::uint32_t object,
                         std::span<const std::byte,0x5c> command) {
        const auto owner=record(word(bank,0x4f54f4u),player);
        if(!session_configured || !owner || word(bank,owner+0x2cu)!=object)
            throw FormatError("TU1.5 command requires the player's actual current source unit");
        object_body(object);
        if(!source_player_command_valid(command))
            throw FormatError("Native TU1.5 delivered command fails the original validator");
        constexpr std::uint32_t input=0x60028000u;bank.write(input,command);
        const std::array args{object,input};auto a=frame(args);a.gp[0]=player;
        run_actual(0x153070u,a);
    }
    void step_objects() {
        if(!session_configured || !word(bank,0x52be48u))
            throw FormatError("TU1.5 object tick requires the configured source world");
        update_player_clusters();
        for(auto entry:{0x13b6f0u,0xb8f40u,0x1cc8d0u,0xb9090u,0xb9180u})
            call_actual(entry);
        const auto clock=word(bank,0x51df54u);
        if(!clock)throw FormatError("TU1.5 frame has no source clock owner");
        // Literal13ABD3..13ABD8 follows these phases in the original frame.
        put(bank,clock+8u,word(bank,clock+8u)+1u);
    }
    void update_player_clusters() {
        // Exact bounded actual1500C0..1501A8, before13B6F0 in the game tick.
        // The full player/input/action scheduler remains separately owned.
        // Resolve the real current/previous unit and source parent root; no
        // visibility mask, cluster choice or active-body flag is supplied.
        const auto pool=word(bank,0x4f54f4u),objects=word(bank,0x4ecbd0u),heads=word(bank,objects+0x44u);
        for(std::uint32_t i=0;i<word(bank,pool+0x38u);++i) {
            if(!(word(bank,word(bank,pool+0x48u)+(i>>5)*4u)&(1u<<(i&31))))continue;
            const auto row=word(bank,pool+0x44u)+i*word(bank,pool+0x24u);
            auto object=word(bank,row+0x2cu);
            if(object==0xffffffffu)object=word(bank,row+0x30u);
            if(object==0xffffffffu)continue;
            std::uint32_t body{};unsigned depth=0;
            do {
                if(++depth>0x800)throw FormatError("TU1.5 source player parent chain is cyclic");
                body=word(bank,heads+(object&0xffffu)*12u+8u);
                object=word(bank,body+0x14u);
            }while(object!=0xffffffffu);
            const auto cluster=bank.view(body+0x2cu,2);
            if(cluster[0]!=std::byte{0xff} || cluster[1]!=std::byte{0xff})
                bank.write(row+0x2au,cluster);
        }
    }
    void initialize_map() {
        if(word(bank,0x52be48u))throw FormatError("TU1.5 physics world is already constructed");
        for(auto entry:{0x37f110u,0x37f130u,0x37f150u,0x37f1e0u,0x37f240u,
            0x37fc70u,0x37fc90u,0x37fcb0u})call(entry);
        call_actual(0xb8a00u);
        call(0x1c5ff0u);
        if(!word(bank,0x52be48u))throw FormatError("TU1.5 new-map world was not published");
        call_actual(0xb8be0u);
    }
    std::uint32_t create_biped(std::uint32_t definition,const Vec3&position,const Vec3&forward) {
        if(!session_configured)throw FormatError("TU1.5 object birth requires its actual native session options");
        const auto tag=bipeds.find(definition);
        if(tag==bipeds.end())throw FormatError("TU1.5 biped birth requires its actual cache definition");
        constexpr std::uint32_t request=0x60010000u;
        const std::array args{definition,0xffffffffu,0u};
        auto a=frame(args);a.gp[6]=request;
        source_run_physics_live_support_tu15(bank,0xb9960u,a,fp,services());
        for(const auto&[at,v]:std::array<std::pair<unsigned,Vec3>,2>{{{0x1c,position},{0x28,forward}}}) {
            put(bank,request+at,std::bit_cast<std::uint32_t>(v.x));
            put(bank,request+at+4,std::bit_cast<std::uint32_t>(v.y));
            put(bank,request+at+8,std::bit_cast<std::uint32_t>(v.z));
        }
        const std::array birth_args{request};return call_actual(0xb9b70u,birth_args);
    }
    std::uint32_t create_netgame_object(unsigned row) {
        const auto scenario=word(bank,0x4ecc20u),engine=word(bank,0x4f63b8u);
        if(!session_configured || !word(bank,0x52be48u) || !engine ||
            row>=word(bank,scenario+0x120u) || row>=100u)
            throw FormatError("TU1.5 netgame birth requires its admitted authored row and source world");
        // Literal15D92D..15D957 admits the original four row selectors.
        // The registered engine supplies its type through the real vtable.
        const auto owner=word(bank,0x56dfa0u+word(bank,engine+0xc14u)*4u);
        auto selector=frame({});selector.gp[0]=0xffffffffu;
        if(owner) {
            selector.gp[1]=owner;
            run_actual(word(bank,word(bank,owner)),selector);
        }
        const auto engine_type=selector.gp[0];
        const std::array count{4u};selector=frame(count);
        selector.gp[2]=engine_type;
        selector.gp[7]=word(bank,scenario+0x124u)+row*0x90u+4u;
        run_actual(0x15cab0u,selector);
        if(selector.gp[0]==0xffffffffu)return 0xffffffffu;
        const std::array args{engine+0x7dcu,row};call_actual(0x15d4f0u,args);
        const auto object=word(bank,engine+0x7dcu+row*8u);
        if(object!=0xffffffffu) {
            const auto body=object_body(object);
            if(bank.view(body+0xafu,1)[0]!=std::byte(row))
                throw FormatError("TU1.5 netgame constructor did not retain its actual row identity");
        }
        return object;
    }
    std::uint32_t create_scenario_object(const ScenarioObject &placement) {
        if(!session_configured || !word(bank,0x52be48u) ||
            placement.type<0 || placement.type>12 || placement.placement<0)
            throw FormatError("TU1.5 scene birth requires its live world and authored placement identity");
        const auto short_at=[&](std::uint32_t address) {
            const auto b=bank.view(address,2);
            return std::bit_cast<std::int16_t>(std::uint16_t(std::to_integer<unsigned>(b[0])|
                (std::to_integer<unsigned>(b[1])<<8)));
        };
        // Literal D7AA0..D7B48 consumes this same source type table, scenario
        // blocks, row stride and palette. Preserve the original placement bytes.
        const auto type=word(bank,0x474b90u+std::uint32_t(placement.type)*4u);
        const auto placed_offset=short_at(type+0xau),palette_offset=short_at(type+0xcu),stride=short_at(type+0xeu);
        if(placed_offset<0 || palette_offset<0 || stride<=0)
            throw FormatError("TU1.5 source type has no authored placement/palette");
        const auto scenario=word(bank,0x4ecc20u),placed=scenario+std::uint32_t(placed_offset),
            palette=scenario+std::uint32_t(palette_offset);
        if(std::uint32_t(placement.placement)>=word(bank,placed))
            throw FormatError("TU1.5 placement ordinal escapes its source block");
        const auto input=word(bank,placed+4u)+std::uint32_t(placement.placement)*std::uint32_t(stride);
        const auto choice=short_at(input);
        if(choice<0 || std::uint32_t(choice)>=word(bank,palette) ||
            word(bank,word(bank,palette+4u)+std::uint32_t(choice)*0x28u+4u)!=placement.definition)
            throw FormatError("TU1.5 placement definition differs from its original palette");
        const std::array args{std::uint32_t(placement.type),std::uint32_t(placement.placement),palette,1u,1u};
        auto a=frame(args);a.gp[0]=input;run_actual(0xc1120u,a);return a.gp[0];
    }
    void destroy_object(std::uint32_t object) {
        object_body(object);
        // Literal15C897..15C8A2 requests complete deactivation/deletion.
        const std::array deactivate{object,1u};call_actual(0xba3e0u,deactivate);
        const std::array erase{object,1u};call_actual(0xba490u,erase);
        if(record(word(bank,0x4ecbd0u),object))
            throw FormatError("TU1.5 complete deletion did not retire its full source identity");
    }
    void configure_session(const SourceGameVariant&variant,std::uint16_t mode,
        std::int16_t rate,std::uint32_t seed) {
        if(session_configured || rate<=0 || !source_game_variant_valid(variant))
            throw FormatError("TU1.5 source session requires one admitted variant and positive clock rate");
        const auto game=word(bank,0x4f3218u);
        // Native session delivery binds the actual source game-options fields;
        // complete1490B0 consumes the delivered clock rate and resets the clock.
        put(bank,game+8u,2u);
        put(bank,game+0xcu,std::uint32_t(mode)|(std::uint32_t(std::uint16_t(rate))<<16));
        put(bank,game+0x18u,seed);
        // Literal13A8F2..13A8FB copies the admitted options seed to the
        // already constructed gameplay stream before map object creation.
        const auto random=word(bank,0x4f3cd8u);
        if(!random)throw FormatError("TU1.5 gameplay random stream was not constructed");
        put(bank,random,word(bank,game+0x18u));
        bank.write(game+0x13cu,std::as_bytes(std::span(variant.bytes)));
        auto stage=frame({});stage.gp[6]=game+8u;
        run_actual(0x191ca0u,stage);
        const std::array capacity{stage.gp[0]};
        // Actual resource-stage callbacks45F944 rows6/7 consume the source
        // selector and carve their backing from the retained physical bank.
        call_actual(0x125d40u,capacity);
        call_actual(0x130500u,capacity);
        call_actual(0x12ecf0u); // Actual191AAF carves and activates texture backing.
        call_actual(0x1490b0u);
        call_actual(0x159ff0u); // Full source engine reset consumes the delivered variant.
        session_configured=true;
    }
};
PhysicsLiveWorldTu15::PhysicsLiveWorldTu15(const Cache&cache,const std::filesystem::path&xbe,const std::filesystem::path&files,std::size_t bsp)
    :impl_(std::make_unique<Impl>(cache,xbe,files,bsp)) {}
PhysicsLiveWorldTu15::~PhysicsLiveWorldTu15()=default;
SourceRuntimeMemory&PhysicsLiveWorldTu15::memory(){return impl_->bank;}
void PhysicsLiveWorldTu15::run(std::uint32_t p,SourcePhysicsWorldRuntimeAbi&a,
    const SourcePhysicsContactRecordObserver&o){impl_->run(p,a,o);}
void PhysicsLiveWorldTu15::run_actual(std::uint32_t p,SourcePhysicsWorldRuntimeAbi&a,
    const SourcePhysicsContactRecordObserver&o){impl_->run_actual(p,a,o);}
std::uint32_t PhysicsLiveWorldTu15::call(std::uint32_t p,std::span<const std::uint32_t>a){return impl_->call(p,a);}
std::uint32_t PhysicsLiveWorldTu15::call_actual(std::uint32_t p,std::span<const std::uint32_t>a){return impl_->call_actual(p,a);}
std::uint32_t PhysicsLiveWorldTu15::world()const{return word(impl_->bank,0x52be48);}
void PhysicsLiveWorldTu15::initialize_map(){impl_->initialize_map();}
std::uint32_t PhysicsLiveWorldTu15::object_pool()const{return word(impl_->bank,0x4ecbd0u);}
std::uint32_t PhysicsLiveWorldTu15::player_pool()const{return word(impl_->bank,0x4f54f4u);}
void PhysicsLiveWorldTu15::construct_player(std::uint32_t p,unsigned u,
    const SourcePlayerConfiguration&c,const SourceMachineIdentity&m){impl_->construct_player(p,u,c,m);}
void PhysicsLiveWorldTu15::bind_player(std::uint32_t p,std::uint32_t o){impl_->bind_player(p,o);}
SourcePlayerEquipmentTu15 PhysicsLiveWorldTu15::initialize_player_equipment(std::uint32_t p,std::uint32_t o){return impl_->initialize_player_equipment(p,o);}
SourcePlayerEquipmentTu15 PhysicsLiveWorldTu15::player_equipment(std::uint32_t o)const{return impl_->player_equipment(o);}
void PhysicsLiveWorldTu15::deliver_command(std::uint32_t p,std::uint32_t o,
    std::span<const std::byte,0x5c>c){impl_->deliver_command(p,o,c);}
void PhysicsLiveWorldTu15::step_objects(){impl_->step_objects();}
std::uint32_t PhysicsLiveWorldTu15::object_body(std::uint32_t d)const{return impl_->object_body(d);}
std::uint32_t PhysicsLiveWorldTu15::create_biped(std::uint32_t d,const Vec3&p,const Vec3&f){return impl_->create_biped(d,p,f);}
std::uint32_t PhysicsLiveWorldTu15::create_scenario_object(const ScenarioObject&p){return impl_->create_scenario_object(p);}
std::uint32_t PhysicsLiveWorldTu15::create_netgame_object(unsigned r){return impl_->create_netgame_object(r);}
void PhysicsLiveWorldTu15::destroy_object(std::uint32_t object){impl_->destroy_object(object);}
void PhysicsLiveWorldTu15::configure_session(const SourceGameVariant&v,std::uint16_t m,std::int16_t r,std::uint32_t s){impl_->configure_session(v,m,r,s);}
SourcePhysicsWorldRuntimeAbi PhysicsLiveWorldTu15::step_physics(const SourcePhysicsContactRecordObserver&o) {
    if(!impl_->session_configured || !world())
        throw FormatError("TU1.5 physics frame requires its constructed world and admitted session");
    auto a=impl_->frame({});
    impl_->run(0x1c7660u,a,o);
    return a;
}
} // namespace halo2
