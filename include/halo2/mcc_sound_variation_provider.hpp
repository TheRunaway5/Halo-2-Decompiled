#pragma once
#include "halo2/sound_variation.hpp"
#include <memory>

namespace halo2 {
class PhysicsLiveWorldTu15;
class SourceNativePublication;
class MultiplayerAudio;
class MccSoundTimingReadLease;
struct MccSoundVariationProviderAccess;
// Owned metadata from an actual converted native World. The opaque lease
// retains its source memory and epoch; a provider can outlive the World but
// rejects after teardown or any restore, including identical-byte restores.
// Cache retains the actual MCC DAT/PCM resources and must still outlive its
// World. Its address is borrowed equality only, never an ownership token.
class MccSoundVariationProvider final {
    struct NativeBindingData;
    struct NativeHeldData;
public:
    MccSoundVariationProvider(const MccSoundVariationProvider&)=default;
    MccSoundVariationProvider&operator=(const MccSoundVariationProvider&)=default;
    // Producer-thread, current-epoch check without metadata reads. Consumers
    // must call this even when returning an already cached metadata bank.
    // This check does not hold a transaction across later Audio operations;
    // the existing retained producer/restore discipline remains required.
    void require_current(const Cache& expected_owner)const;
    // Complete validation and owned-copy parsing share one retained read
    // transaction. No borrowed source span, host pointer, source voice or
    // synthetic readiness is exposed. Permutation counts admit1..32; larger counts reject.
    // Range last/forced and used bits are snapshots at this load. They are
    // not continuing aliases or selection-state/RNG writeback to the World;
    // an Audio bank's private selection state remains a separate owner.
    SoundVariationDefinition load(const Cache& expected_owner,TagHandle sound,
                                  unsigned pitch=0)const;
    // Transferable target admission: retains metadata/Cache, only a weak
    // native-scope identity. It never retains a live gate or timing mutex.
    class NativeBinding {
    public:
        NativeBinding()=default;
        // Direct creator-only native eligibility, without a timing operation.
        // Actual lease freshness is rechecked when acquiring HeldNativeBinding.
        bool current()const noexcept;
    private:
        std::shared_ptr<const NativeBindingData> data_;
        explicit NativeBinding(std::shared_ptr<const NativeBindingData>);
        friend class MccSoundVariationProvider;
    };
    class HeldNativeBinding {
    public:
        ~HeldNativeBinding();
        HeldNativeBinding(const HeldNativeBinding&)=delete;
        HeldNativeBinding(HeldNativeBinding&&)=delete;
        // No locks, allocation, callbacks or native-byte reads. Release this
        // scope after SDL, before native exclusion or outgoing owner retirement.
        bool matches(const NativeBinding&)const noexcept;
    private:
        std::unique_ptr<NativeHeldData> data_;
        explicit HeldNativeBinding(std::unique_ptr<NativeHeldData>);
        friend class MccSoundVariationProvider;
    };
    // Requires a genuine current takeover scope, the exact strong Cache owner
    // and this provider's actual native timing owner, before timing/SDL holds.
    NativeBinding native_binding(const SourceNativePublication&,
        std::shared_ptr<const Cache>)const;
    static HeldNativeBinding hold_native_binding(const NativeBinding&);
private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit MccSoundVariationProvider(std::shared_ptr<const Impl>);
    MccSoundTimingReadLease timing_lease()const;
    static MccSoundVariationProvider binding_provider(const NativeBinding&);
    friend class MultiplayerAudio;
    friend struct MccSoundVariationProviderAccess;
};
} // namespace halo2
