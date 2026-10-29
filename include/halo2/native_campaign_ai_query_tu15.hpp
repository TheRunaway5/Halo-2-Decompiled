#pragma once
#include "halo2/physics_contact_record.hpp"
#include <optional>
#include <string_view>

namespace halo2 {
struct NativeCampaignAiQueryFp {std::uint32_t mxcsr=0x1f80u;};
struct NativeCampaignAiLivingCountResult {
  std::uint16_t value{};
  NativeCampaignAiQueryFp floating_point;
};
enum class NativeCampaignAiQuery { living_count,swarm_count,nonswarm_count,strength,fighting_count,living_fraction };
struct NativeCampaignAiQueryBuiltin {
  NativeCampaignAiQuery operation;
  std::uint16_t return_type;
};
std::optional<NativeCampaignAiQueryBuiltin> native_campaign_ai_query_builtin_tu15(std::string_view);
struct NativeCampaignAiQueryResult {
  std::uint32_t value{}; // SHORT bits for counts; REAL bits for strength/fraction.
  NativeCampaignAiQueryFp floating_point;
};
// TU1.5 ai_living_count: typed mode0 semantics of27DC40 and its packed AI
// reference/group iterators. Borrow the actual published source banks.
// Squad/group queries use signed squad+0xA counters; an existing direct actor
// counts once. Preserve source reads/FP effects and the evaluator's SHORT bits.
// This kernel never invokes executable code, a lifted body or a host AI model.
std::uint16_t native_campaign_ai_living_count_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t packed_reference,
    NativeCampaignAiQueryFp &);
// The count modes, full signed32 living fraction and weighted strength share27DC40's
// traversal. The mode is an ordinary typed operation, never an executable PC.
std::uint32_t native_campaign_ai_query_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t packed_reference,NativeCampaignAiQuery,NativeCampaignAiQueryFp &);
// Resolve an actor or authored starting-location reference through27C880's
// native datum/member lookup; a missing starting-location match is null.
std::uint32_t native_campaign_ai_actor_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t packed_reference);
//27DDA0 reads maintained squad/group combat counters. Actor references use
//1EA090's activity/awareness/age gates and1AFBD0's inclusive behavior search.
std::uint16_t native_campaign_ai_fighting_count_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t packed_reference);
}
