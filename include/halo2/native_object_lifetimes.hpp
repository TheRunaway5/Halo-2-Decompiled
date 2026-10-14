#pragma once
#include "halo2/datum_pool.hpp"
#include <map>

namespace halo2 {
// Native integration bookkeeping over the recovered shared object header pool.
// These ordinals are native call order, not an Xbox event/save/network format.
enum class NativeObjectLifetimeEventKind { reserve, publish, release };
struct NativeObjectLifetime {
    std::uint32_t handle=DatumPool::none,definition=DatumPool::none;
    std::uint8_t type{};
    bool published{};
    std::uint64_t allocation_ordinal{},publication_ordinal{};
    std::uint16_t pool_rank() const {return std::uint16_t(handle);}
    bool operator==(const NativeObjectLifetime &) const = default;
};
struct NativeObjectLifetimeEvent {
    std::uint64_t ordinal{};
    NativeObjectLifetimeEventKind kind{};
    NativeObjectLifetime owner;
    bool operator==(const NativeObjectLifetimeEvent &) const = default;
};
// Original B67C0/BC280 use one 2048-element, 12-byte object header pool for all
// types. A reservation is separate from completed native category storage.
// Publication supplies identity/definition (or explicit NONE) only, not a complete source body,
// registration, hierarchy, activation, pose or successful retail birth history.
// Value copies retain allocator bytes, sidecars and event order for checkpoints.
class NativeObjectLifetimes {
public:
    explicit NativeObjectLifetimes(std::int32_t capacity=0x800);
    NativeObjectLifetimes(SourceRuntimeMemory &,std::uint32_t source_object_pool);
    NativeObjectLifetimes(SourceRuntimeMemory &,NativeRuntimeAddress object_pool);
    // Register a completed source birth without allocating another header or
    // assigning its type/definition. Both come from the actual retained body.
    void observe_source_birth(std::uint32_t full_handle);
    // Complete source deletion has already removed the body and retired the
    // header. Observe that retirement without freeing its slot a second time.
    void observe_source_retirement(std::uint32_t full_handle);
    std::uint32_t reserve(std::uint8_t type);
    // BD170 reserves a zeroed header before B8B0B supplies its type. FF is
    // an explicitly unresolved native sidecar value, never a header byte.
    std::uint32_t reserve_storage();
    void publish_native(std::uint32_t full_handle,std::uint32_t definition);
    // Actual caller-owned rollback/free boundary; does not invent unregister,
    // detach, child recursion or other original destruction services.
    void release(std::uint32_t full_handle);
    const NativeObjectLifetime& owner(std::uint32_t full_handle) const;
    bool contains(std::uint32_t full_handle) const;
    std::span<std::uint8_t> header(std::uint32_t full_handle);
    std::span<const std::uint8_t> header(std::uint32_t full_handle) const;
    std::span<std::uint8_t> source_headers() {return pool_.element_storage();}
    const DatumPool& pool() const {return pool_;}
    std::span<const NativeObjectLifetimeEvent> events() const {return events_;}
    // Source-backed continuation composes with the enclosing whole-bank
    // transaction. Owned pools still use their existing value-copy path.
    struct SourceAuthorityCheckpoint {
        std::uint32_t version=1;
        DatumPool::SourceBinding pool;
        std::map<std::uint32_t,NativeObjectLifetime> owners;
        std::vector<NativeObjectLifetimeEvent> events;
        std::uint64_t ordinal{};
        bool operator==(const SourceAuthorityCheckpoint &) const = default;
    };
    class PreparedSourceAuthority {
    public:
        ~PreparedSourceAuthority();
        PreparedSourceAuthority(PreparedSourceAuthority &&) noexcept;
        PreparedSourceAuthority &operator=(PreparedSourceAuthority &&) noexcept;
        bool for_owner(const NativeObjectLifetimes &) const noexcept;
    private:
        friend class NativeObjectLifetimes;
        PreparedSourceAuthority();
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
    SourceAuthorityCheckpoint source_authority_checkpoint() const;
    PreparedSourceAuthority prepare_source_authority(const SourceAuthorityCheckpoint &,
                                                    const SourceRuntimeMemory &replacement) const;
    // The current source bank must first receive the exact staged backing.
    // Native owner identity and caller callbacks remain local. Quiesce every
    // lifetime/pool borrower through publication and commit/cancellation.
    bool commit_source_authority(PreparedSourceAuthority &&) noexcept;
private:
    DatumPool pool_;
    std::map<std::uint32_t,NativeObjectLifetime> owners_;
    std::vector<NativeObjectLifetimeEvent> events_;
    std::uint64_t ordinal_{};
    std::uint32_t reserve_header(std::uint8_t sidecar_type,bool write_type);
    std::uint64_t record(NativeObjectLifetimeEventKind,const NativeObjectLifetime&);
};
}
