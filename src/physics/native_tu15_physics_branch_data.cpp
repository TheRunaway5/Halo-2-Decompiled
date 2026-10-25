#include "halo2/native_tu15_physics_branch_data.hpp"
#include <array>
#include <sstream>
#include <stdexcept>

namespace halo2 {
namespace {
struct BranchTable {
    std::uint32_t address;
    std::span<const std::byte> bytes;
};
#include "physics/native_tu15_physics_branch_data.inc"
#include "physics/native_tu15_physics_callable_data.inc"
#include "physics/native_tu15_physics_literal_data.inc"
#include "physics/native_tu15_light_portal_literal_data.inc"
#include "physics/native_tu15_fade_curve_data.inc"
#include "campaign/ai/native_tu15_ai_literal_data.inc"
#include "gameplay/world/native_tu15_teleporter_query_data.inc"
#include "gameplay/weapons/native_tu15_weapon_damage_data.inc"
#include "physics/native_tu15_collision_query_data.inc"
constexpr std::array<std::span<const BranchTable>,9> table_sets{
    branch_tables,callable_tables,literal_tables,ai_literal_tables,teleporter_query_tables,
    weapon_damage_tables,collision_query_tables,light_portal_literal_tables,fade_curve_bank_tables};
bool has_owner(const SourceRuntimeMemory& memory,std::uint32_t address) {
    // A zero-length view still resolves the region owner, including reserved
    // pages. The validation pass below must reject an uncommitted owner before
    // any new table is mounted, rather than treating that owner as a hole.
    try {(void)memory.view(address,0);return true;}
    catch(const std::out_of_range&) {return false;}
}
}
void mount_native_tu15_physics_branch_data(SourceRuntimeMemory& memory) {
    // Validate every shared byte before publishing any missing owner. Several
    // tables already belong to exact, separately declared source consumers.
    for(const auto tables:table_sets)for(const auto& table:tables)for(std::size_t i=0;i<table.bytes.size();++i) {
        const auto address=table.address+std::uint32_t(i);
        if(has_owner(memory,address) &&
            (!memory.is_read_only(address,1) || memory.view(address,1).front()!=table.bytes[i])) {
            std::ostringstream message;
            message<<"Native TU1.5 physics branch owner conflicts at0x"<<std::hex<<address;
            throw std::invalid_argument(message.str());
        }
    }
    for(const auto tables:table_sets)for(const auto& table:tables) {
        std::size_t i{};
        while(i<table.bytes.size()) {
            if(has_owner(memory,table.address+std::uint32_t(i))){++i;continue;}
            const auto begin=i++;
            while(i<table.bytes.size() && !has_owner(memory,table.address+std::uint32_t(i)))++i;
            memory.mount(table.address+std::uint32_t(begin),table.bytes.subspan(begin,i-begin),
                SourceRuntimeMemory::Access::ReadOnly,
                "native TU1.5 authored physics immutable dispatch data");
        }
    }
}
}
