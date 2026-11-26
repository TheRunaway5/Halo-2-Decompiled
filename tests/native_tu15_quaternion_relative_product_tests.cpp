#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/renderer/native_tu15_quaternion_relative_product.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_quaternion_relative_product_detail;

namespace original {
using namespace halo2::physics_world_runtime_detail;

std::uint32_t selected(std::uint32_t address, SourcePhysicsContactImage) { return address; }

std::uint32_t native_single_spill(long double value, std::uint16_t &status) {
    std::uint32_t result;
    std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
        : "=m"(result), "=m"(actual) : "m"(value) : "st");
    status = (status & ~0x200u) | (actual & 0x200u);
    return result;
}

#include "body.inc"
#include "reference.inc"
} // namespace original

namespace {

Case author(std::mt19937 &rng, unsigned index) {
    Case result;
    auto &memory = result.memory;
    const auto stack = 0x20020000u + std::uint32_t(rng() % 16) * 4u;
    for (auto address = stack - 0x30u; address < stack + 0x24u; address += 4u)
        memory.put(address, std::uint32_t(rng()));

    std::uint32_t first = 0x10000020u;
    std::uint32_t second = 0x10001040u;
    std::uint32_t output = 0x10002080u;
    const auto shape = index % 16;
    switch (shape) {
    case 1: output = first; break;
    case 2: output = second; break;
    case 3: output = first + 4u; break;
    case 4: output = second - 4u; break;
    case 5: second = first; break;
    case 6: output = 0x10002ff8u; break;
    case 7: first = 0x10000ff8u; second = 0x10003040u; break;
    case 12: output = stack - 0x18u; break;
    case 13: output = stack + 4u; break;
    case 14: second = first + 8u; break;
    case 15: second = first + 8u; output = first + 4u; break;
    default: break;
    }

    std::mt19937_64 fp_rng(rng());
    for (const auto address : {output, first, second}) {
        for (unsigned lane = 0; lane < 4; ++lane) {
            const auto bits = rng() % 3 == 0 ? special_float(fp_rng)
                : std::bit_cast<std::uint32_t>(float(int(rng() % 201) - 100) * 0.25f);
            memory.put(address + lane * 4u, bits);
        }
    }
    memory.put(stack, 0x11223344u);
    memory.put(stack + 4u, first);
    memory.put(stack + 8u, second);
    random_abi(rng, result, stack);
    result.abi.gp[1] = output;

    switch (shape) {
    case 6: memory.drop(output + 8u); break; // First half of the store succeeds.
    case 7: memory.drop(first + 8u); break; // First half of the load succeeds.
    case 8: memory.drop(second); break;
    case 9: memory.drop(first); break;
    case 10: memory.drop(output); break;
    case 11: result.abi.x87_depth = 8; break;
    default: break;
    }
    return result;
}

bool native(WorldState &s) {
    return owner::bridge(s, [](WorldState &state) { original::native_frame_misc_reference_2e56a0(state); },
        original::native_single_spill);
}

void reference(WorldState &s) { original::run_2e56a0(s); }

} // namespace

int main(int argc, char **argv) try {
    const unsigned cases = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, "HALO2_VERIFY_QUATERNION_RELATIVE_PRODUCT");
    std::mt19937 rng(0x2e56a0u + (census_mode ? 1u : 0u) + (mode == "verify" ? 2u : 0u));

    Totals totals;
    for (unsigned index = 0; index < cases; ++index) {
        if (!compare(index, author(rng, index), native, reference, totals)) return 1;
    }
    if (!census_mode) check_refusals(author(rng, 1), native);
    report(cases, totals, owner::coverage().load(), unsigned(owner::branch_count), mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
