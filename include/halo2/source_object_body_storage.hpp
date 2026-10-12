#pragma once
#include "halo2/native_object_lifetimes.hpp"
#include "halo2/object_body_heap.hpp"
#include "halo2/unit_action_dispatch.hpp"
#include <array>

namespace halo2 {
// Persistent native backing composed with complete BD170, BD270 and finite
// 18FCE0/18FD70/18FE60 owners. These addresses are native opaque memory tokens,
// not captured original runtime globals or host pointers. Header bytes belong
// to the caller's one NativeObjectLifetimes pool; there is no second allocator.
class SourceObjectBodyStorage {
public:
    static constexpr std::uint32_t heap_address=0x02000000;
    static constexpr std::uint32_t headers_address=0x02200000;
    static constexpr std::uint32_t scratch_address=0x02300000;
    static constexpr std::uint32_t name_address=scratch_address+16;
    // Actual B76E5/125520 capacity is 100000. Smaller explicit capacities and
    // initial bytes are controlled native allocator inputs, not retail history.
    explicit SourceObjectBodyStorage(std::uint32_t capacity=0x100000,
        std::byte initial_backing=std::byte{0xcd});
    std::uint32_t allocate(NativeObjectLifetimes&,std::int16_t base_extent,
        const UnitActionObserver& = {},const ObjectConstructionObserver& = {});
    bool append(NativeObjectLifetimes&,std::uint32_t full_handle,
        std::int16_t descriptor,std::int16_t bytes,std::uint8_t alignment,
        const UnitConstructorObserver& = {},const ObjectConstructionObserver& = {});
    // Storage retirement only. The caller supplies real preceding live-object
    // destruction services; constructor failure can retire directly. No world,
    // hierarchy, physics or registration callback is inferred here.
    void release(NativeObjectLifetimes&,std::uint32_t full_handle,
        const ObjectConstructionObserver& = {});
    bool contains(std::uint32_t full_handle) const;
    // Full physical payload is required by BD270 after relocation but before
    // its header+6 write. Logical body() is the current header extent only.
    std::span<std::byte> backing(NativeObjectLifetimes&,std::uint32_t full_handle);
    std::span<std::byte> body(NativeObjectLifetimes&,std::uint32_t full_handle);
    std::span<std::byte> memory(NativeObjectLifetimes&,std::uint32_t address,std::size_t bytes);
    UnitConstructorObjectPool objects(NativeObjectLifetimes&,
        const ObjectConstructionObserver& = {});
    std::span<const std::byte> arena() const {return arena_;}
    std::span<const std::byte> scratch() const {return scratch_;}
    std::span<const std::uint32_t> handles() const {return handles_;}
    std::uint32_t capacity() const {return capacity_;}
private:
    std::uint32_t capacity_;
    std::vector<std::byte> arena_;
    std::array<std::byte,64> scratch_{};
    std::array<std::uint32_t,2048> handles_;
    SourceObjectBodyHeapServices services(NativeObjectLifetimes&);
    std::uint32_t pointer_cell(std::uint32_t) const;
    void require_owner(const NativeObjectLifetimes&,std::uint32_t) const;
};
}
