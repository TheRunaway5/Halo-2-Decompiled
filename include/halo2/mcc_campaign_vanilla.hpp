#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace halo2 {
struct MccCampaignTagIdentity { std::string group, path; };
// Borrow stable, checked source-token views. This seam is usable while a Cache
// is being constructed: the planner never constructs a Cache or loads content.
struct MccCampaignVanillaSource {
    bool is_mcc{};
    std::string_view scenario_path;
    std::uint32_t scenario_address{};
    std::function<std::span<const std::byte>(std::uint32_t,std::size_t)> read;
    std::function<std::optional<MccCampaignTagIdentity>(std::uint32_t)> resolve_tag;
    std::function<std::string(std::uint32_t)> resolve_string_id;
};
struct MccCampaignVanillaWrite {
    // Serialized metadata token, never a host pointer. Both byte arrays own
    // their storage; the caller must validate every before image before commit.
    std::uint32_t address{};
    std::vector<std::byte> before, after;
};
struct MccCampaignVanillaPlan {
    bool matched{}, already_equivalent{};
    std::string profile, reason;
    std::vector<MccCampaignVanillaWrite> writes;
    std::size_t removed_scripts{}, retired_nodes{}, wake_patches{};
    std::size_t script_ordinal_words{}, scenario_ordinal_words{};
};
// Only fourteen authenticated retail authoring tuples are admitted. The plan
// preserves MCC builtin opcodes and literal storage except the proved original
// tutorial static-call restoration in its already-owned function-name slot.
// Rejected/custom/mixed variants produce no writes; no builtin is made a noop.
MccCampaignVanillaPlan plan_mcc_campaign_vanilla(const MccCampaignVanillaSource&);
} // namespace halo2
