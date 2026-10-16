#pragma once
#include "halo2/native_tu15_heap_bootstrap.hpp"
#include "halo2/physics_game_arena_native.hpp"
#include <thread>
#include <vector>
#include <optional>

namespace halo2 {
// Native startup through the complete source arena constructor, including
// its real worker and savegame file jobs. Owns the heap, physical backing and
// retired worker for their full lifetime. No tag image is admitted here.
class NativeTu15ArenaBootstrap final {
public:
    struct RendererTargetProtection {
        std::uint32_t source_address{},bytes{},protection{};
        NativeRuntimeAddress native_backing{};
    };
    explicit NativeTu15ArenaBootstrap(const std::filesystem::path& savegame_directory);
    NativeTu15ArenaBootstrap(const NativeTu15ArenaBootstrap&)=delete;
    NativeTu15ArenaBootstrap& operator=(const NativeTu15ArenaBootstrap&)=delete;
    NativeTu15ArenaBootstrap(NativeTu15ArenaBootstrap&&)=delete;
    NativeTu15ArenaBootstrap& operator=(NativeTu15ArenaBootstrap&&)=delete;
    SourceRuntimeMemory& memory() noexcept{return process_.memory();}
    const SourceRuntimeMemory& memory()const noexcept{return process_.memory();}
    NativeRuntimeAddress heap()const noexcept{return process_.heap();}
    NativeRuntimeAddress arena()const noexcept{return arena_address_;}
    NativeTu15HeapBootstrap& process_heap() noexcept{return process_;}
    bool try_renderer_target_protection_call(SourcePhysicsContactMemory&,
        const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    bool try_sound_cache_protection_call(SourcePhysicsContactMemory&,
        const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    const std::vector<RendererTargetProtection>& renderer_target_protections()const noexcept{return renderer_target_protections_;}
    const std::optional<RendererTargetProtection>& sound_cache_protection()const noexcept{return sound_cache_protection_;}
private:
    NativeTu15HeapBootstrap process_;
    std::unique_ptr<SourcePhysicsGameArenaNative> arena_owner_;
    NativeRuntimeAddress arena_address_;
    std::thread::id thread_=std::this_thread::get_id();
    std::vector<RendererTargetProtection> renderer_target_protections_;
    std::optional<RendererTargetProtection> sound_cache_protection_;
};
}
