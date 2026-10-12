#pragma once
#include "halo2/source_bsp_ranges.hpp"
#include "halo2/source_shader_lod.hpp"
#include <functional>

namespace halo2 {
// Literal24-byte3B9D0 record. The last two bytes are retained storage, not
// fields initialized by the source append. Callback/pointer words are tokens.
struct SourcePartCommand {std::array<std::uint32_t,6> words{};};
static_assert(sizeof(SourcePartCommand)==24);
struct SourcePartCommandBank {
    std::array<std::uint32_t,6> header{}; // capacity,count,layers,retained×2,data
    std::vector<SourcePartCommand> storage;
};
struct SourceBspCommandMaterial {
    std::uint32_t shader=0xffffffffu;
    std::uint8_t visibility_index=255; // Literal material+1C;255 means NONE.
};
struct SourceBspCommandGeometry {
    bool resident{}; // Actual current geometry+50 nonnull;449E0 DL0.
    std::vector<std::array<std::byte,72>> parts;
};
struct SourceBspCommandShader {
    SourceShaderLodDefinition lod;
    std::vector<std::uint32_t> layers; // Actual10-byte template LOD+2.
};
struct SourceBspPartCommandWorld {
    std::span<const SourceBspCommandGeometry> clusters,definitions;
    std::span<const std::int16_t> instance_definitions;
    std::span<const SourceBspCommandMaterial> materials;
    std::span<const std::byte> material_visibility; // Actual4ED280 allocation.
    std::int16_t bsp_selector{}; // Actual4686C4 signed-short selector.
    std::function<const SourceBspCommandShader&(std::uint32_t)> shader;
};
struct SourceBspPartCommandState {
    std::array<SourcePartCommandBank,3> banks; // Actual4C1A48,+18,+30.
    std::array<SourceCullDrawUnit,32> special_units{}; // Secondary4C62F8.
    std::int16_t special_count{}; // Actual selected4C66FC bank.
    // Observed source entry order; pointer-valued returns are not interpreted.
    std::vector<std::uint32_t> calls;
    std::array<std::uint32_t,2> main_object_counts{}; // Actual4B6280/+4.
};
struct SourceBspPartCommandContext {
    std::uint32_t mode{},flag_mask=0xffffffffu,group=31;
    std::uint32_t main_prefix{},draw_count{},mask_arena_address{};
    SourceShaderLodView lod_view;
    float player_multiplier{}; // Actual4670E4.
    bool view_modifier{}; // Actual4BA019.
    std::span<const std::uint8_t> coverage,override_slots;
    std::span<const std::uint32_t> overrides;
};
// Exact184000 bit lookup, including unaligned instance offsets and NONE.
// This reads the actual visibility allocation; it does not create its owner.
bool source_bsp_material_visible(std::span<const std::byte>,std::int16_t bsp,
    std::int32_t instance,std::int32_t material_index);
// Whole183EC0 initializes4201 bytes of the persistent4204-byte allocation.
// The three trailing bytes survive; this is a map activation, not a view reset.
void source_bsp_material_visibility_initialize(std::span<std::byte>);
//40E90's exact bank-reset and selector corridor, before41C80. Storage,
// header+10 and the selected light's special-unit bank remain untouched.
std::uint32_t source_part_commands_begin(SourceBspPartCommandState&,
    std::uint32_t mode,std::uint32_t selector);
// Project whole44AC0→45CE0/460D0→41040/40E30/3B9D0 onto the secondary
// command/special-unit banks. Existing commands and untouched bytes survive.
// Main mode0, dynamic model geometry and dynamic MTPR/prt3 shader services
// require their actual owners; this BSP-only entry rejects consumed branches.
//40E90 resets/41C80 object queues and final opaque sorting are separate owners.
void source_bsp_append_part_commands(SourceBspPartCommandState&,
    std::span<const SourceCullDrawUnit>,const SourceBspRangeArena&,
    const SourceBspPartCommandWorld&,const SourceBspPartCommandContext&);
// Whole secondary40E90 orchestration.41C80 is an explicit current object-queue
// owner, invoked after reset and before BSP commands; absence is not empty.
// Its callback may append to the same banks. Main mode's additional owners
// are not supplied by this entry.
void source_bsp_build_part_commands(SourceBspPartCommandState&,
    std::span<const SourceCullDrawUnit>,const SourceBspRangeArena&,
    const SourceBspPartCommandWorld&,const SourceBspPartCommandContext&,
    std::uint32_t selector,
    const std::function<void(SourceBspPartCommandState&,std::uint32_t)>& objects);
}
