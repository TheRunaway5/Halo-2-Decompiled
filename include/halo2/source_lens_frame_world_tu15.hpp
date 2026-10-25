#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/native_runtime_address.hpp"

namespace halo2 {
class Cache;
class PhysicsLiveWorldTu15;
class NativeLensPresentationTu15;
class SourceRuntimeMemory;
// A borrowed CPU execution owner for one presentation call. The ordinary
// world keeps its existing stack, journaling and canonical cache ownership.
// A replica presentation owner can supply its own request bank without
// acquiring simulation authority or constructing a physical world.
class SourceLensFrameWorldTu15 {
public:
    SourceLensFrameWorldTu15(PhysicsLiveWorldTu15 &);
    SourceRuntimeMemory &memory() const;
    SourcePhysicsWorldRuntimeAbi prepare_source_call(std::span<const std::uint32_t> = {}) const;
    void run_actual(std::uint32_t,SourcePhysicsWorldRuntimeAbi &) const;
    std::uint32_t call_actual(std::uint32_t) const;
    void write_source_call_input(std::uint32_t,std::span<const std::byte>) const;
    NativeRuntimeAddress resident_cache_address(const Cache &,std::uint32_t,std::size_t) const;
private:
    friend class NativeLensPresentationTu15;
    struct Operations {
        SourceRuntimeMemory &(*memory)(void *);
        SourcePhysicsWorldRuntimeAbi (*prepare)(void *,std::span<const std::uint32_t>);
        void (*run)(void *,std::uint32_t,SourcePhysicsWorldRuntimeAbi &);
        std::uint32_t (*call)(void *,std::uint32_t);
        void (*input)(void *,std::uint32_t,std::span<const std::byte>);
        NativeRuntimeAddress (*resident)(void *,const Cache &,std::uint32_t,std::size_t);
    };
    SourceLensFrameWorldTu15(void *owner,const Operations &ops):owner_(owner),operations_(&ops){}
    void *owner_;
    const Operations *operations_;
};
}
