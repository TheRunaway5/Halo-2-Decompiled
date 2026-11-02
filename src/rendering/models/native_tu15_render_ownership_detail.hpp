#pragma once
#include "halo2/native_tu15_render_object_ownership.hpp"

namespace halo2::render_ownership_detail {
// Resolution metadata belongs to the retained data layout. The ABI adapter
// uses it to preserve legacy volatile outputs without rereading mutable data.
struct Root {std::uint32_t identity = 0xffffffffu, headers{};};
struct Attachment {
    std::uint32_t marker = 0xffffffffu, definition{}, directory{}, count{}, rows{};
    std::int16_t index{};
    bool count_read{}, found{};
};
struct ViewCategory {
    std::uint32_t category = 3, row{}, callback{}, blend{}, zero{};
    std::uint8_t first_person_gate{};
    bool compared{}, unordered{};
};
enum class PriorityPath {no_object, no_view, other_camera, nonunit_root, different_unit, matching_unit};
struct Priority {
    PriorityPath path = PriorityPath::no_object;
    std::uint32_t view{}, pool{}, headers{}, header{}, body{}, view_unit{}, root_unit{};
    std::uint8_t root_kind{};
    ViewCategory camera;
    Root root;
};
struct PriorityTransport {
    std::function<void()> object_present, enter_root;
    std::function<void(std::uint32_t)> enter_camera;
    std::function<void(const ViewCategory&)> leave_camera;
    std::function<void(const Root&)> leave_root;
};
Root root(const SourcePhysicsContactMemory&, std::uint32_t);
Attachment attachment(const SourcePhysicsContactMemory&, std::uint32_t, std::int16_t,
    const std::function<void()>& before_definition = {});
ViewCategory category(const SourcePhysicsContactMemory&, std::uint32_t, NativeTu15PhysicsFrameFp&, bool live_sse = false);
Priority priority(const SourcePhysicsContactMemory&, std::uint32_t, NativeTu15PhysicsFrameFp&,
    const PriorityTransport& = {}, bool live_sse = false);
}
