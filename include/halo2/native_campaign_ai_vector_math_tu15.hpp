#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <exception>
#include <optional>
#include <stdexcept>

namespace halo2 {
// One typed native extended-real quantity. It is not a CPU/register packet.
struct NativeCampaignAiPendingDistanceTu15 {std::array<std::byte,10> extended{};};
class NativeCampaignAiPendingDistanceErrorTu15 final:public std::runtime_error {
    NativeCampaignAiPendingDistanceTu15 pending_;
    std::exception_ptr cause_;
public:
    NativeCampaignAiPendingDistanceErrorTu15(NativeCampaignAiPendingDistanceTu15,std::exception_ptr);
    const NativeCampaignAiPendingDistanceTu15 &pending()const noexcept{return pending_;}
    const std::exception_ptr &cause()const noexcept{return cause_;}
};
struct NativeCampaignAiDistancePublicationTu15 {
    bool committed{};
    std::optional<std::uint32_t> written_bits;
    std::optional<NativeCampaignAiPendingDistanceTu15> pending;
    std::exception_ptr failure;
    // A rejected publication retains its actual owed extended quantity in
    // the typed exception; an observer rejecting a committed store does not.
    void rethrow_if_failed()const;
};
// Authenticated3EA20 ->266EF9 profile: compute real x87 point distance and
// publish the caller's float field. A rejected store preserves source pre-FSTP
// FP status/TOP and carries the pending value; no spill success is fabricated.
[[nodiscard]] NativeCampaignAiDistancePublicationTu15
native_campaign_ai_publish_perception_distance_tu15(SourcePhysicsContactMemory &,
    std::uint32_t first,std::uint32_t second,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver & = {});
// Complete30BD0. Returns original magnitude REAL bits (zero on the epsilon
// refusal path); vector writes and FP side effects remain lazy and ordered.
std::uint32_t native_campaign_ai_normalize_vector_tu15(SourcePhysicsContactMemory &,
    std::uint32_t vector,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
// Native private intermediate for composed algorithms. The same arithmetic
// owns normalization; only actual global constants are read through memory.
// No retained temporary address or guest/private stack is created.
std::uint32_t native_campaign_ai_normalize_vector_value_tu15(const SourcePhysicsContactMemory &,
    std::array<std::uint32_t,3> &value,NativeTu15PhysicsFrameFp &);

}
