#include "rendering/renderer/object_motion_blur_history.hpp"
#include <iostream>

namespace {
using namespace halo2;
constexpr std::uint32_t moving_owner = 0xe0010002;
constexpr std::uint32_t still_owner = 0xe0020005;

void require(bool condition, const char* message) {
    if (!condition) throw FormatError(message);
}

std::array<Vertex, 3> triangle(float offset, std::uint32_t section = 0) {
    std::array<Vertex, 3> vertices{};
    vertices[1].source_local_position.x = 1;
    vertices[2].source_local_position.y = 1;
    for (auto& vertex : vertices) {
        vertex.position = vertex.source_local_position;
        vertex.position.x += offset;
        vertex.source_section = section;
        vertex.source_part = 0;
    }
    return vertices;
}

ModelObjectDraw draw(std::span<const Vertex> vertices, std::optional<std::uint32_t> owner) {
    ModelObjectDraw result{};
    result.vertices = vertices;
    result.source_object = owner;
    return result;
}

void check_world_history() {
    ObjectMotionBlurHistory history;
    const auto first = triangle(1), second = triangle(2), third = triangle(3), stationary = triangle(6);
    auto objects = std::array{draw(first, moving_owner), draw(stationary, still_owner)};
    require(history.update(10, false, objects).empty(), "First frame emitted object motion");
    objects = {draw(stationary, still_owner), draw(second, moving_owner)};
    const auto motion = history.update(11, false, objects);
    require(motion.size() == 3 && motion[0].current.x == 2 && motion[0].previous.x == 1 &&
        motion[1].current.x == 3 && motion[1].previous.x == 2,
        "Reordered owners lost motion, or stationary geometry entered the pass");
    require(history.update(12, false, objects).empty(), "Unchanged WORLD geometry produced camera motion");

    objects[1] = draw(third, moving_owner);
    require(history.update(14, false, objects).empty(), "A skipped frame reused stale motion history");
    objects[1] = draw(first, moving_owner);
    require(history.update(15, true, objects).empty(), "A camera cut reused stale motion history");
    objects[1] = draw(second, moving_owner);
    require(history.update(16, false, objects).size() == 3, "A camera cut failed to seed new history");
    objects[1] = draw(third, moving_owner + 0x10000u);
    require(history.update(17, false, objects).empty(), "A recycled object slot matched an older salt");

    auto altered = third;
    altered[1].source_local_position.z = 1;
    objects[1] = draw(altered, moving_owner + 0x10000u);
    require(history.update(18, false, objects).empty(), "Changed source topology reused old vertex positions");

    objects[1] = draw(first, std::nullopt);
    require(history.update(19, false, objects).empty(), "Unowned or static world geometry entered motion history");
    objects[1] = draw(second, moving_owner + 0x10000u);
    require(history.update(20, false, objects).empty(), "Reappearing geometry reused a missing owner's pose");
    const auto duplicate = std::array{draw(third, moving_owner + 0x10000u)};
    objects[1] = duplicate[0];
    require(history.update(21, false, objects, duplicate).empty(), "A duplicate owner received ambiguous motion");
    objects[1] = draw(first, moving_owner + 0x10000u);
    require(history.update(22, false, objects).empty(), "An ambiguous prior owner seeded motion history");

    auto stippled = first;
    for (auto& vertex : stippled) vertex.source_stipple_level = 8;
    objects[1] = draw(stippled, moving_owner + 0x10000u);
    require(history.update(23, false, objects).empty(), "Changed stipple pattern reused old motion");
    for (auto& vertex : stippled) vertex.position.x += 1;
    require(history.update(24, false, objects).empty(), "Partially covered geometry entered the motion pass");
}

void check_selected_sections() {
    ObjectMotionBlurHistory history;
    const auto first = triangle(1, 3), second = triangle(2, 3), third = triangle(3, 3);
    const auto stationary = triangle(7, 6);
    const std::array<std::uint32_t, 3> moving_ordinals{10, 11, 12}, stationary_ordinals{100, 101, 102};
    auto selected = [&](const auto& vertices, auto& ordinals) {
        auto result = draw(vertices, std::nullopt);
        result.source_model_selection = SourceModelDepthSelection{moving_owner, 30, 31, 1, nullptr, 0, ordinals};
        return result;
    };
    auto objects = std::array{selected(first, moving_ordinals), selected(stationary, stationary_ordinals)};
    require(history.update(0, false, objects).empty(), "Initial selected sections emitted motion");
    objects = {selected(stationary, stationary_ordinals), selected(second, moving_ordinals)};
    require(history.update(1, false, std::span(objects).first(1), std::span(objects).last(1)).size() == 3,
        "Supplemental submissions or distinct sections of one owner lost motion");
    const std::array<std::uint32_t, 3> reordered{11, 10, 12};
    objects[1] = selected(third, reordered);
    require(history.update(2, false, objects).empty(), "Different source ordinals reused previous vertices");
    objects[1] = selected(first, reordered);
    objects[1].source_model_selection->lod = 2;
    require(history.update(3, false, objects).empty(), "An LOD switch reused previous vertices");
}

void check_deferred_skin() {
    ObjectMotionBlurHistory history;
    auto vertices = triangle(0);
    auto geometry = std::make_shared<ModelSkinGeometry>();
    geometry->owner = moving_owner;
    geometry->definition = 12;
    geometry->render_model = 13;
    geometry->vertices.assign(vertices.begin(), vertices.end());
    geometry->bindings.resize(vertices.size());
    geometry->source_vertices = {23, 24, 25};
    geometry->palette.push_back({1, 0, 0, 2, 0, 1, 0, 0, 0, 0, 1, 0});
    const std::array submissions{ModelSkinSubmission{0, geometry}};
    auto object = draw(vertices, moving_owner);
    object.deferred_skins = submissions;
    const auto objects = std::span(&object, 1);
    require(history.update(5, false, objects).empty(), "New deferred skin emitted object motion");
    geometry->palette[0][3] = 4;
    const auto motion = history.update(6, false, objects);
    require(motion.size() == 3 && motion[0].previous.x == 2 && motion[0].current.x == 4 &&
        vertices[0].position.x == 0, "Deferred skin used local positions or mutated its source vertices");
    geometry->source_vertices = {25, 24, 23};
    geometry->palette[0][3] = 7;
    require(history.update(7, false, objects).empty(), "Changed deferred vertex ordinals reused the old pose");
    geometry->palette[0][3] = 10;
    geometry->owner = still_owner;
    require(history.update(8, false, objects).empty(), "Mismatched deferred owner supplied motion");
}
}

int main() try {
    check_world_history();
    check_selected_sections();
    check_deferred_skin();
    std::cout << "PASS per-object WORLD motion and source history boundaries\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
