#pragma once
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/original_sound_runtime_bank_tu15.hpp"

namespace halo2 {
struct MccOriginalSoundRuntimeInputTu15 {
    std::span<const OriginalSoundRuntimeBindingTu15> bindings;
    OriginalSoundRuntimeStringsTu15 strings;
};
// Immutable publication lineage, minted only by complete staged construction.
// Parent metadata remains owned and independently authenticatable. The graph
// retains the exact original payloads and physical resource-descriptor routes.
// This owner grants neither a current World epoch nor timing/device readiness.
class MccOriginalSoundRuntimeDeliveryTu15 final {
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit MccOriginalSoundRuntimeDeliveryTu15(std::unique_ptr<Impl>);
public:
    ~MccOriginalSoundRuntimeDeliveryTu15();
    const MccSoundRuntimePlan& parent()const;
    std::shared_ptr<const OriginalSoundRuntimeBankTu15> bank()const;
    std::span<const OriginalSoundRuntimeBindingTu15> bindings()const;
    bool contains(TagHandle)const;
    // Binds the complete sealed metadata, original provenance and actual
    // whole-sector bytes. Donor filesystem paths and host pointers stay local.
    const std::array<std::byte,32>& resource_identity()const;
    // Pure extension of an already authenticated resident-resource digest.
    // This grants no Cache identity, current memory epoch or readiness lease.
    std::array<std::byte,32> bind_resource_identity(const std::array<std::byte,32>&)const;
    // Original descriptors read their exact retained sectors. A validated
    // prefix request returns nullopt for its existing external resource owner;
    // a foreign descriptor, locator, flags or extent throws before delegation.
    std::optional<std::vector<std::byte>> read_cache_resource(const SourceSoundCacheReadTu15&)const;
    // Checks the entire immutable native plan and this exact lineage owner.
    // Caller copies and failed/foreign substitutions cannot mint a new seal.
    void require_plan(const MccSoundRuntimePlan&)const;
    friend MccSoundRuntimePlan stage_mcc_original_sound_runtime_delivery_tu15(
        const MccSoundRuntimeSource&,const MccSoundRuntimePlan&,
        const MccOriginalSoundRuntimeInputTu15&,std::uint32_t available_bytes);
};
// Append original-ABI tables after the complete native prefix, update every
// owning UGH root and only the named full-salt snd! targets, and rebase all
// existing chunk routes. Class banks and unrelated sound definitions stay
// intact. No caller input, Cache or live allocation is changed on failure.
MccSoundRuntimePlan stage_mcc_original_sound_runtime_delivery_tu15(
    const MccSoundRuntimeSource&,const MccSoundRuntimePlan&,
    const MccOriginalSoundRuntimeInputTu15&,std::uint32_t available_bytes);
}
