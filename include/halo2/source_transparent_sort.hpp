#pragma once
#include "halo2/math.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>

namespace halo2 {
// Literal112-byte command layout at4B6288. The callback and64-byte payload
// remain source tokens; this producer does not interpret native draw IDs.
struct SourceTransparentCommand {
    std::int32_t priority{}; //+00
    float depth{}; //+04; normally minus eye-forward distance.
    std::int32_t secondary{}; //+08
    std::uint32_t plane_present{}; //+0C; only low byte is consumed.
    std::array<float,4> plane{}; //+10, dot(normal,point)<D
    std::uint32_t callback{}; //+20
    std::array<std::uint8_t,64> payload{}; //+24..63
    Vec3 center{}; //+64..6C
};
static_assert(sizeof(SourceTransparentCommand)==0x70);
static_assert(std::is_trivially_copyable_v<SourceTransparentCommand>);
static_assert(offsetof(SourceTransparentCommand,priority)==0x00);
static_assert(offsetof(SourceTransparentCommand,depth)==0x04);
static_assert(offsetof(SourceTransparentCommand,secondary)==0x08);
static_assert(offsetof(SourceTransparentCommand,plane_present)==0x0c);
static_assert(offsetof(SourceTransparentCommand,plane)==0x10);
static_assert(offsetof(SourceTransparentCommand,callback)==0x20);
static_assert(offsetof(SourceTransparentCommand,payload)==0x24);
static_assert(offsetof(SourceTransparentCommand,center)==0x64);
struct SourceTransparentSection {
    float depth{}; //4C152C+00, actual133AB0 owner depth.
    Vec3 center{}; //+04
    std::array<float,4> rectangle{}; //+10: x0,x1,y0,y1, actual30290.
    std::uint32_t kind{}; //+20:1 grouped screen regions,3 effect override.
};
static_assert(sizeof(SourceTransparentSection)==0x24);
static_assert(offsetof(SourceTransparentSection,depth)==0x00);
static_assert(offsetof(SourceTransparentSection,center)==0x04);
static_assert(offsetof(SourceTransparentSection,rectangle)==0x10);
static_assert(offsetof(SourceTransparentSection,kind)==0x20);

// Literal1E700 predicate. The plane branches need not form a strict weak
// ordering; use the original ordinal sorter below, never std::sort.
bool source_transparent_compare(const SourceTransparentCommand &,
    const SourceTransparentCommand &,Vec3 actual_camera_position);
// Original13DF80/13E140. Ordinals start at first, as1E370 initializes them;
// equal keys retain the original algorithm's swaps, rather than stable order.
std::vector<std::uint16_t> source_transparent_sort(
    std::span<const SourceTransparentCommand>,Vec3 actual_camera_position,
    unsigned first=0);

// Append420A0 then42760, preserving existing commands and their order.
// Input is the actual populated4C152C bank, including kind0 entries. Original
// sort-section31 is the NONE sentinel and is not a populated input entry.
void source_transparent_append_groups(std::span<const SourceTransparentSection>,
    std::vector<SourceTransparentCommand> &,std::uint32_t kind3_argument,
    unsigned command_limit=1024);

struct SourceTransparentPartInput {
    std::uint32_t selected_template_layers{}; //1BF50's actual shader LOD mask.
    std::uint16_t part_type{}; //72-byte authored part+00.
    bool material_present=true;
    unsigned view_type{}; //41040 param3; ordinary main view0.
    bool grouped_context{}; //41040 param7; queue groups exclude ordinary command.
    std::uint16_t shader_flags{}; //Selected shader+16, priority bits0/1.
    std::uint32_t selected_shader=0xffffffffu,draw_token{};
    Vec3 center{},camera_position{},camera_forward{}; //45CE0/context inputs.
    float depth_bias{}; //45CE0 output+10, not a guessed object radius.
    bool plane_present{}; //45CE0 output+14, transformed authored plane if any.
    std::array<float,4> plane{};
};
// Pure ordinary41040 admission and final command producer. Resource/part
// visibility, shader LOD selection and authored center/plane are caller inputs.
std::optional<SourceTransparentCommand> source_transparent_part_command(
    const SourceTransparentPartInput &);
}
