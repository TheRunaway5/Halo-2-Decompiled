#pragma once
#include "halo2/mcc_sound_vanilla_reference.hpp"
#include <memory>

namespace halo2 {
enum class MccSoundStoredTimeStatus {
    Authenticated,AlreadyEquivalent,NoOriginalReference,UnknownOwner,
    CustomOwner,EncodedResourceMismatch,InvalidNativeProjection
};
struct MccSoundStoredTimeResult {
    TagHandle sound,gestalt;
    MccSoundStoredTimeStatus status{MccSoundStoredTimeStatus::NoOriginalReference};
    std::string reason;
};
struct MccSoundStoredTimeCorrection {
    TagHandle sound,gestalt;
    std::uint32_t source_root{},native_root{};
    std::int32_t source_ms{},original_ms{};
    // Complete canonical native header guard; only its final four bytes may
    // change. The raw Cache header, DAT route and actual PCM remain separate.
    std::array<std::byte,20> native_header_before;
    std::array<std::byte,4> before,after;
};
class MccSoundStoredTimePlan final {
public:
    std::span<const MccSoundStoredTimeCorrection> corrections()const noexcept;
    std::span<const MccSoundStoredTimeResult> sounds()const noexcept;
    std::span<const MccSoundVanillaOwnerResult> owners()const noexcept;
private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit MccSoundStoredTimePlan(std::shared_ptr<const Impl>);
    friend struct MccSoundStoredTimeAccess;
};
// Immutable, staged TU1.5 snd!+0x10 values. Admit only finite complete raw
// owners, exact full-salt sound namespaces, and every owned permutation's
// authenticated MCC encoded payload and format. A known path alone never
// qualifies a correction. Unknown/custom sounds remain explicit unsupported
// results; no PCM-derived timing or native voice readiness is substituted.
// The supplied native plan must equal the complete canonical base projection
// or its complete authenticated stored-time result. Mixed changes reject.
// This helper performs no Cache/native writes and changes no plan layout.
MccSoundStoredTimePlan plan_mcc_sound_stored_times(
    const MccSoundVanillaSource&,const MccSoundRuntimePlan&);
MccSoundStoredTimePlan plan_mcc_sound_stored_times(
    const Cache&,const MccSoundRuntimePlan&,
    const std::function<std::uint32_t(std::uint32_t,std::size_t)>& relocate);
} // namespace halo2
