#pragma once
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>
namespace halo2 {
struct UiVariant;
// Native extension writes live outside the original112 descriptor IDs and
// never occupy unidentified bytes in the original304-byte variant buffer.
inline constexpr unsigned native_slayer_ruleset_parameter=0x80000000u;
enum class CustomSlayer : unsigned { none, fiesta, infection };
CustomSlayer custom_slayer(const UiVariant &);
std::vector<UiVariant> load_custom_slayer_variants(const std::filesystem::path &);
std::vector<UiVariant> bundled_slayer_variants();
std::uint32_t source_starting_weapon(std::uint32_t map_default,unsigned option,
    std::uint32_t excluded,std::span<const std::uint32_t,18> references,std::uint32_t &shared_seed);
std::uint64_t custom_slayer_fingerprint(std::uint64_t base,const UiVariant &);
}
