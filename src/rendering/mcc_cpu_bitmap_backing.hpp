#pragma once
#include "rendering/mcc_cpu_bitmap_owner.hpp"
#include <memory>
namespace halo2::private_cpu_bitmap {
// Producer-thread CPU resource ownership. World must outlive this owner and
// serialize it with its completed executor. Filtered lifecycle methods require
// the enclosing complete-source/no-continuation gate. No GPU residency claim.
class Backing final {
    struct Impl;
    struct State;
    struct Rows;
    struct Frame;
public:
    class SparseRows final {
    public:
        unsigned base() const noexcept;
        unsigned count() const noexcept;
    private:
        friend class Backing;
        std::shared_ptr<Rows> rows_;
        std::weak_ptr<State> state_;
    };
    class RetainedFrame final {
    public:
        unsigned row() const noexcept;
        unsigned encoded() const noexcept;
        unsigned header() const noexcept;
        NativeRuntimeAddress native_encoded() const noexcept;
        NativeRuntimeAddress native_header() const noexcept;
        // Rejects stale/foreign/replaced aliases and changed actual bytes.
        void validate(const SourceRuntimeMemory&) const;
    private:
        friend class Backing;
        std::shared_ptr<Frame> frame_;
        std::weak_ptr<State> state_;
    };
    explicit Backing(SourceRuntimeMemory&);
    ~Backing();
    Backing(const Backing&)=delete;
    Backing& operator=(const Backing&)=delete;
    SparseRows reserve_sparse_rows(unsigned count);
    // Owner is already authenticated against the actual Cache by World. This
    // stages real encoded pages/header, runs delivery(), and commits ONLY its
    // common24/+34/+50/+54/+70 cells. Other row bytes stay uncommitted.
    RetainedFrame stage(const Owner&, const SparseRows&);
    // Revalidate all owned cells/payload/header, recompute genuine clock+2,
    // then write only the existing +70 cell; commit() cannot refresh pages.
    Delivery refresh(const Owner&, const RetainedFrame&);
    // A failed sparse commit cannot be uncommitted. Poisoning permanently
    // rejects continuation; the enclosing World must propagate the failure.
    bool poisoned() const noexcept;
    // Capture/validate the complete actual live
    // bank and omit only this owner's proved derived regions in the COPY.
    // World must check all canonical source identities and quiescence first.
    SourceRuntimeMemory::Checkpoint checkpoint_without_resources() const;
    // Same complete read-only-set comparison as the general Memory method,
    // against authenticated filtered LIVE storage. Never ignores foreign RO.
    bool matches_read_only_checkpoint_without_resources(
        const SourceRuntimeMemory::Checkpoint&) const noexcept;
    // The enclosing successful swap already invalidated old
    // ownership; reset remains noexcept and never releases foreign backings.
    bool swap_invalidated() const noexcept;
    // Restore-only immutable-domain admission: unusable mutable postimages
    // may be discarded, but exact allocation/backing/topology must remain.
    bool matches_read_only_checkpoint_for_restore(
        const SourceRuntimeMemory::Checkpoint&) const noexcept;
private:
    SourceRuntimeMemory::Checkpoint checkpoint_for_restore() const;
    std::unique_ptr<Impl> impl_;
};
}
