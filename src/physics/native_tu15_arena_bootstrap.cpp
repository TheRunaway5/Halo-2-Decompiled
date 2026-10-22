#include "halo2/native_tu15_arena_bootstrap.hpp"
#include "halo2/native_tu15_arena_data.hpp"
#include <condition_variable>
#include <mutex>

namespace halo2 {
namespace {
void put(SourcePhysicsContactMemory& memory,std::uint32_t address,std::uint32_t value) {
    std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));memory.write(address,bytes);
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    std::uint32_t value=0;const auto bytes=memory.view(address,4);
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
}
struct WorkerIdleBarrier {
    std::mutex mutex;
    std::condition_variable changed;
    unsigned waits=0;
    void before_wait(std::uint32_t handle) {
        if(handle!=0xf3000003u)return; // This owner's actual semaphore namespace.
        std::lock_guard lock(mutex);++waits;changed.notify_all();
    }
    void finish_startup() {
        std::unique_lock lock(mutex);
        // Initial idle wait, then one after each of the two source jobs for
        // each savegame file. A completion byte precedes free-list recycling;
        // wait for the source worker itself to finish before stopping it.
        if(!changed.wait_for(lock,std::chrono::seconds(5),[&]{return waits>=5;}))
            throw std::runtime_error("Native arena worker did not retire its four startup file jobs");
    }
};
}
NativeTu15ArenaBootstrap::NativeTu15ArenaBootstrap(const std::filesystem::path& directory) {
    if(directory.empty() || !std::filesystem::is_directory(directory))
        throw std::invalid_argument("Native arena startup needs an existing private savegame directory");
    auto& bank=process_.memory();mount_native_tu15_arena_data(bank);
    const auto idle=std::make_shared<WorkerIdleBarrier>();
    arena_owner_=std::make_unique<SourcePhysicsGameArenaNative>(bank,
        SourcePhysicsGameArenaConfiguration{directory,0x60110000u,{0x27f,0x1f80},
            {[idle](std::uint32_t handle,std::stop_token){idle->before_wait(handle);},{}}});
    constexpr std::uint32_t sp=0x600f0000u;
    for(const auto entry:{0x12cb70u,0x1221b0u,0x125280u}) {
        put(arena_owner_->memory(),sp,0x60300000u);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=sp;abi.eflags=2;abi.fs_base=0x60200000u;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        arena_owner_->run(entry,abi);
        if(abi.gp[4]!=sp+4)throw std::runtime_error("Native arena startup did not retire its source caller frame");
    }
    // Match the current world startup barrier: the complete source file jobs
    // have returned before normal constructors get exclusive bank ownership.
    idle->finish_startup();
    arena_owner_->stop();
    const auto address=word(bank,0x4f2950u);
    if(!address)throw std::runtime_error("Native arena constructor returned no owned storage");
    arena_address_=bank.native_address(address,0x3fe000u);
}
bool NativeTu15ArenaBootstrap::try_renderer_target_protection_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x800000b6u)return false;
    const auto sp=abi.gp[4];
    if(&bank!=&memory()||std::this_thread::get_id()!=thread_||!call.tail||call.instruction!=0x2db4d1u||
       word(memory(),0x41b648)!=call.target||word(memory(),sp)!=0x1db4bu||word(memory(),sp+12)!=0x404u)
        throw std::invalid_argument("Renderer physical protection has a foreign bank/site/frame");
    const auto address=word(memory(),sp+4),bytes=word(memory(),sp+8);
    const auto backing=memory().native_address(address,bytes);
    arena_owner_->protect_pages(address,bytes,0x404);
    renderer_target_protections_.push_back({address,bytes,0x404,backing});
    abi.gp[4]+=16; // Native kernel void stdcall ABI; source words/results stay owned by source code.
    return true;
}
bool NativeTu15ArenaBootstrap::try_sound_cache_protection_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x800000b6u)return false;
    const auto sp=abi.gp[4];
    // Source21EFE0 reserves this cache from its physical partition and reaches
    // the same genuine XPhysicalProtect wrapper as renderer target storage.
    // Its authored site, return, footprint and protection are distinct.
    if(&bank!=&memory()||std::this_thread::get_id()!=thread_||!call.tail||call.instruction!=0x2db4d1u||
       word(memory(),0x41b648)!=call.target||word(memory(),sp)!=0x21f0a3u||
       word(memory(),sp+8)!=0x300000u||word(memory(),sp+12)!=4u)
        throw std::invalid_argument("Sound cache physical protection has a foreign bank/site/frame");
    const auto address=word(memory(),sp+4);
    const auto backing=memory().native_address(address,0x300000);
    arena_owner_->protect_pages(address,0x300000,4);
    sound_cache_protection_=RendererTargetProtection{address,0x300000,4,backing};
    abi.gp[4]+=16; // Actual kernel void stdcall; no game result is supplied.
    return true;
}
}
