#if defined(HALO2_TEST_VEHICLE_MARKER_MATRIX)
#include "physics/vehicles/vehicle_marker_matrix_instruction.hpp"
#else
#include "physics/vehicles/vehicle_marker_emission_instruction.hpp"
#endif

#include <array>
#include <cstdio>
#include <vector>

using namespace halo2;
#if defined(HALO2_TEST_VEHICLE_MARKER_MATRIX)
using namespace vehicle_marker_matrix_instruction_detail;
using MarkerInstruction = VehicleMarkerMatrixInstruction;
#else
using namespace vehicle_marker_emission_instruction_detail;
using MarkerInstruction = VehicleMarkerEmissionInstruction;
#endif

void require(bool result, const char *error) {
    if (!result) throw std::runtime_error(error);
}

int main() try {
    constexpr auto earlier_pc = actual_first - 1, later_pc = actual_last + 1;
    std::vector<PhysicsInstructionRemapTu15::Binding> bindings;
    for (const auto pc : canonical_sites) bindings.push_back({pc, pc + image_delta});
    bindings.push_back({earlier_pc, earlier_pc});
    bindings.push_back({later_pc, later_pc});
    SourcePhysicsContactRuntimeServices source, selected;
    std::uint32_t cursor{};
    bool first_trace{}, second_trace{};
    std::vector<std::uint32_t> observed;
    const auto callback = [&](auto pc) {
        cursor = pc;
        observed.push_back(pc);
    };
    const auto set_literal_policy = [&] {
        static constexpr std::array instruction_services{0x38846eu, 0x21f590u, 0x33b595u};
        source.instruction = callback;
        source.instruction_cursor = {&cursor, &first_trace, &second_trace, instruction_services,
            std::this_thread::get_id(), &source.instruction.target_type(), true, false};
    };
    const auto set_mapping = [&](std::span<const PhysicsInstructionRemapTu15::Binding> current) {
        selected.instruction = PhysicsInstructionRemapTu15{&source, current};
    };
    set_literal_policy();
    set_mapping(bindings);
    std::function<void(std::uint32_t)> instruction = MarkerInstruction{selected};
    const auto *emission = instruction.target<MarkerInstruction>();
    require(emission != nullptr, "Vehicle emitter callable type was not preserved");
    std::uint32_t *position = nullptr;
    require(emission->native_position(position) && position == &cursor, "Exact marker map rejected");

    instruction(earlier_pc);
    instruction(later_pc);
    require(observed == std::vector{earlier_pc, later_pc}, "Non-emitter observation changed namespace");
    observed.clear();
    for (const auto pc : canonical_sites) instruction(pc + image_delta);
    require(observed.size() == std::size(canonical_sites), "Emitter did not forward every instruction");
    for (std::size_t i = 0; i < observed.size(); ++i)
        require(observed[i] == canonical_sites[i] + image_delta, "Emitter PC did not return to the selected image");
    std::printf("Forwarded marker PCs");
    for (const auto pc : observed) std::printf(" %x", pc);
    std::printf("\n");

    first_trace = true;
    require(!emission->native_position(position) && !position, "First active trace admitted");
    first_trace = false;
    second_trace = true;
    require(!emission->native_position(position) && !position, "Second active trace admitted");
    second_trace = false;
    source.instruction_cursor.executor = {};
    require(!emission->native_position(position), "Foreign executor admitted");
    set_literal_policy();
    source.instruction = [&](auto pc) { observed.push_back(pc); };
    require(!emission->native_position(position) && !position, "Changed callback admitted");
    set_literal_policy();

    auto corrupted = bindings;
    ++corrupted[9].value;
    set_mapping(corrupted);
    require(!emission->native_position(position), "Incorrect marker mapping admitted");
    auto missing = bindings;
    missing.erase(missing.begin() + 9);
    set_mapping(missing);
    require(!emission->native_position(position), "Missing marker site admitted");
    auto extra = bindings;
    extra.insert(extra.begin() + 1, {canonical_sites[0] + 1, actual_first + 1});
    set_mapping(extra);
    require(!emission->native_position(position), "Extra marker site admitted");
    auto duplicate = bindings;
    duplicate.insert(duplicate.begin() + std::size(canonical_sites), bindings[std::size(canonical_sites) - 1]);
    set_mapping(duplicate);
    require(!emission->native_position(position), "Duplicate final marker site admitted");

    source.instruction = {};
    set_mapping(corrupted);
    require(!emission->native_position(position), "Inexact silent marker map admitted");
    set_mapping(bindings);
    require(emission->native_position(position) && !position, "Exact silent marker map rejected");
    const auto prior = observed.size();
    instruction(actual_first);
    require(observed.size() == prior, "Silent marker remap delivered a callback");

    selected.instruction = callback;
    require(!emission->native_position(position), "Unscoped direct callback admitted");
    instruction(actual_first);
    require(observed.back() == canonical_sites[0], "Ordinary callback did not keep the original projection");
    selected.instruction = PhysicsInstructionRemapTu15{nullptr, bindings};
    require(!emission->native_position(position), "Missing underlying services admitted");
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
