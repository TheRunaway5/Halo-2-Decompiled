#pragma once
#include "halo2/native_tu15_heap_bootstrap.hpp"
#include "halo2/physics_game_arena_native.hpp"

namespace halo2 {
// Native startup through the complete source arena constructor, including
// its real worker and savegame file jobs. Owns the heap, physical backing and
// retired worker for their full lifetime. No tag image is admitted here.
class NativeTu15ArenaBootstrap final {
public:
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
private:
    NativeTu15HeapBootstrap process_;
    std::unique_ptr<SourcePhysicsGameArenaNative> arena_owner_;
    NativeRuntimeAddress arena_address_;
};
}
