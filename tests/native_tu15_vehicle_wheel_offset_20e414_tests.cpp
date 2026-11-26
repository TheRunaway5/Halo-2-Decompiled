#include "native_tu15_frame_misc_harness.hpp"
#if defined(HALO2_TEST_VEHICLE_CONTACT_QUERY)
#include "physics/vehicles/native_tu15_vehicle_contact_query_20f3a8.hpp"
#include <nlohmann/json.hpp>
namespace wheel_stage = halo2::native_vehicle_contact_query_20f3a8_detail;
constexpr auto reference_flag = "HALO2_TEST_REFERENCE_VEHICLE_CONTACT_QUERY";
#elif defined(HALO2_TEST_VEHICLE_CONTACT_PROJECTION)
#include "physics/vehicles/native_tu15_vehicle_contact_projection_20ee92.hpp"
namespace wheel_stage = halo2::native_vehicle_contact_projection_20ee92_detail;
constexpr auto reference_flag = "HALO2_REFERENCE_VEHICLE_CONTACT_PROJECTION_20EE92";
#elif defined(HALO2_TEST_VEHICLE_CONTACT_OUTPUT)
#include "physics/vehicles/native_tu15_vehicle_contact_output_20f6e0.hpp"
namespace wheel_stage = halo2::native_vehicle_contact_output_20f6e0_detail;
constexpr auto reference_flag = "HALO2_REFERENCE_VEHICLE_CONTACT_OUTPUT_20F6E0";
#elif defined(HALO2_TEST_VEHICLE_WHEEL_CURVE)
#include "physics/vehicles/native_tu15_vehicle_wheel_curve_20e84a.hpp"
namespace wheel_stage = halo2::native_vehicle_wheel_curve_20e84a_detail;
constexpr auto reference_flag = "HALO2_REFERENCE_VEHICLE_WHEEL_CURVE_20E84A";
#elif defined(HALO2_TEST_VEHICLE_SCRATCH_RESET)
#include "physics/vehicles/native_tu15_vehicle_scratch_reset_20ea32.hpp"
namespace wheel_stage = halo2::native_vehicle_scratch_reset_20ea32_detail;
constexpr auto reference_flag = "HALO2_REFERENCE_VEHICLE_SCRATCH_RESET_20EA32";
#elif defined(HALO2_TEST_VEHICLE_WHEEL_OUTPUT)
#include "physics/vehicles/native_tu15_vehicle_wheel_output_20e91f.hpp"
namespace wheel_stage = halo2::native_vehicle_wheel_output_20e91f_detail;
constexpr auto reference_flag = "HALO2_REFERENCE_VEHICLE_WHEEL_OUTPUT_20E91F";
#elif defined(HALO2_TEST_VEHICLE_WHEEL_RESPONSE)
#include "physics/vehicles/native_tu15_vehicle_wheel_response_20e6c3.hpp"
namespace wheel_stage = halo2::native_vehicle_wheel_response_20e6c3_detail;
constexpr auto reference_flag = "HALO2_REFERENCE_VEHICLE_WHEEL_RESPONSE_20E6C3";
#elif defined(HALO2_TEST_VEHICLE_WHEEL_FORCE)
#include "physics/vehicles/native_tu15_vehicle_wheel_force_20e4f4.hpp"
namespace wheel_stage = halo2::native_vehicle_wheel_force_20e4f4_detail;
constexpr auto reference_flag = "HALO2_REFERENCE_VEHICLE_WHEEL_FORCE_20E4F4";
#elif defined(HALO2_TEST_VEHICLE_WHEEL_CROSS_PRODUCT)
#include "physics/vehicles/native_tu15_vehicle_wheel_cross_product_20e481.hpp"
namespace wheel_stage = halo2::native_vehicle_wheel_cross_product_20e481_detail;
constexpr auto reference_flag = "HALO2_REFERENCE_VEHICLE_WHEEL_CROSS_PRODUCT_20E481";
#else
#include "physics/vehicles/native_tu15_vehicle_wheel_offset_20e414.hpp"
namespace wheel_stage = halo2::native_vehicle_wheel_offset_20e414_detail;
constexpr auto reference_flag = "HALO2_REFERENCE_VEHICLE_WHEEL_OFFSET_20E414";
#endif

using namespace frame_misc_harness;

namespace original {
using namespace halo2::physics_vehicle_motion_detail;
#include "vehicle_wheel_offset_owner.inc"
}

namespace {
using Abi = SourcePhysicsContactRuntimeAbi;
using original::MotionState;

bool identical(const Abi &a, const Abi &b) {
    return same_abi(a, b) && a.fs_base == b.fs_base && a.floating_point && b.floating_point &&
        a.floating_point->mxcsr == b.floating_point->mxcsr && !a.active_mmx && !b.active_mmx;
}

struct Access {
    char kind;
    std::uint32_t pc, address;
    std::size_t bytes;
    Abi abi;

    bool operator==(const Access &other) const {
        return kind == other.kind && pc == other.pc && address == other.address &&
            bytes == other.bytes && identical(abi, other.abi);
    }
};

struct Provider : Memory {
    std::function<Access(char, std::uint32_t, std::size_t)> snapshot;
    std::function<void()> enable_trace;
    mutable std::vector<Access> accesses;
    unsigned fault, transition;

    Provider(const Memory &original, unsigned fault_at, unsigned transition_at)
        : Memory(original.clone()), fault(fault_at), transition(transition_at) {}

    void visit(char kind, std::uint32_t address, std::size_t bytes) const {
        accesses.push_back(snapshot(kind, address, bytes));
        const unsigned n = accesses.size();
        _mm_setcsr(0x1f80u | ((n & 3u) << 13u) | (n & 0x3fu) | ((n & 2u) ? 0x8040u : 0u));
        source_load_x87_exception_flags(std::uint16_t((n * 7u) & 0x3fu));
        if (n == fault) throw std::runtime_error("Injected vehicle wheel provider fault");
        if (n == transition) enable_trace();
    }

    void read(std::uint32_t address, std::span<std::byte> out) const override {
        visit('r', address, out.size());
        Memory::read(address, out);
    }

    void write(std::uint32_t address, std::span<const std::byte> value) override {
        visit('w', address, value.size());
        Memory::write(address, value);
    }
};

enum class Policy { quiet, cursor, remap, silent_remap, trace, arbitrary, bad_map, map_pack };

struct Result {
    Abi abi;
    std::uint32_t cursor{};
    std::uint32_t continuation{};
    std::string fault;
    std::vector<std::uint32_t> trace;
    std::vector<SourcePhysicsContactRecordStore> stores;
};

Result run(Provider &memory, const Case &input, Policy policy, bool observe, bool reference) {
    using namespace wheel_stage;
    Result result;
    const bool silent = policy == Policy::silent_remap;
    const bool remapped = silent || policy == Policy::remap || policy == Policy::bad_map;
    bool trace = policy == Policy::trace;
    SourcePhysicsContactRuntimeServices services, source;
    auto &publisher = remapped ? source : services;
    const auto track = [&](auto pc) {
        result.cursor = pc;
        if (trace || policy == Policy::arbitrary) result.trace.push_back(pc);
    };
    constexpr std::array service_sites{0x38846eu, 0x21f590u, 0x33b595u};
    if (policy != Policy::quiet) {
        publisher.instruction = track;
        if (policy != Policy::arbitrary)
            publisher.instruction_cursor = {&result.cursor, &trace, nullptr, service_sites, std::this_thread::get_id(),
                &publisher.instruction.target_type(), true, true};
        if (silent) publisher.instruction = {};
    }
#ifdef HALO2_TEST_VEHICLE_WHEEL_CURVE
    const auto instruction_sites = input.policy == 0u ? std::span<const std::uint32_t>{pre_sites} : std::span<const std::uint32_t>{post_sites};
    std::vector<PhysicsInstructionRemapTu15::Binding> mapping(instruction_sites.size());
    const auto bad_offset = input.policy == 3u ? reload_offset : input.policy == 2u ? accumulation_offset : 8u;
#else
    const auto instruction_sites = std::span{sites};
    std::array<PhysicsInstructionRemapTu15::Binding, std::size(sites)> mapping;
    constexpr unsigned bad_offset = 8;
#endif
    if (remapped) {
        for (std::size_t i = 0; i < mapping.size(); ++i)
            mapping[i] = {instruction_sites[i], instruction_sites[i] + image_delta};
        if (policy == Policy::bad_map) ++mapping[bad_offset].value;
        services.instruction = PhysicsInstructionRemapTu15{&source, mapping};
    }
    SourcePhysicsContactRecordObserver observer;
    if (observe) observer = [&](const auto &event) {
        result.stores.push_back(event);
        memory.visit('o', event.address, event.written.size());
    };
    FpScope scope(*input.abi.floating_point);
    MotionState state{memory, observer, services, policy == Policy::map_pack ?
        SourcePhysicsContactImage::MapPack11 : SourcePhysicsContactImage::TitleUpdate15};
    state.load(input.abi);
    memory.snapshot = [&](auto kind, auto address, auto bytes) {
        return Access{kind, result.cursor, address, bytes, state.save()};
    };
    memory.enable_trace = [&] {
        trace = true;
        if (silent) publisher.instruction = track;
    };
    try {
#if defined(HALO2_TEST_VEHICLE_WHEEL_CURVE) || defined(HALO2_TEST_VEHICLE_CONTACT_QUERY)
        result.continuation = reference ? original::retained(state, input.policy) : original::selected(state, input.policy);
#elif defined(HALO2_TEST_VEHICLE_WHEEL_FORCE) || defined(HALO2_TEST_VEHICLE_WHEEL_RESPONSE) || \
    defined(HALO2_TEST_VEHICLE_WHEEL_OUTPUT) || defined(HALO2_TEST_VEHICLE_SCRATCH_RESET) || \
    defined(HALO2_TEST_VEHICLE_CONTACT_OUTPUT) || defined(HALO2_TEST_VEHICLE_CONTACT_PROJECTION)
        result.continuation = reference ? original::retained(state) : original::selected(state);
#else
        if (reference) original::retained(state);
        else original::selected(state);
#endif
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
    result.abi = state.save();
    memory.snapshot = {};
    memory.enable_trace = {};
    return result;
}

Case fixture(std::mt19937 &random, unsigned i
#ifdef HALO2_TEST_VEHICLE_CONTACT_QUERY
    , bool hardware = false
#endif
) {
#ifdef HALO2_TEST_VEHICLE_CONTACT_QUERY
    const auto sp = hardware ? 0x62000500u + 4u * (i % 16u) :
        i % 4 == 0 ? 0xffffff00u : 0x62000f01u + 4u * (i % 16u);
#else
    const auto sp = i % 4 == 0 ? 0xffffff00u : 0x62000f01u + 4u * (i % 16u);
#endif
    const auto wheel = i % 5 == 0 ? sp + 0xd0u : 0x65000000u + i * 8u;
    const auto origin = i % 3 == 0 ? wheel - 0x70u : 0x66000000u + i * 4u;
    const auto frame = i % 7 == 0 ? sp + 0x84u : 0x63000300u + i * 8u;
    Case input;
    random_abi(random, input, sp);
    input.abi.gp[5] = frame;
    input.abi.gp[7] = wheel;
    constexpr std::uint32_t fp_bits[]{0, 0x80000000u, 1, 0x80000001u, 0x7f800000u, 0xff800000u,
        0x7fc12345u, 0x7f812345u, 0x3f800000u, 0xbfc00000u, 0x7f7fffffu};
    auto next = [&] { return i % 2 == 0 ? fp_bits[random() % std::size(fp_bits)] : std::uint32_t(random()); };
    for (auto entry : {sp + 0x24u, sp + 0xc0u, sp + 0xc4u, sp + 0xc8u, frame + 0x40u,
                       wheel - 0x1cu, wheel - 0x18u, wheel - 0x14u, wheel - 0x10u,
                       wheel - 0xcu, wheel - 8u, origin + 0x60u, origin + 0x64u, origin + 0x68u,
                       0x46503cu, 0x465030u, 0x46a0dcu, 0x46a0d0u})
        input.memory.put(entry, next());
    input.memory.put(sp + 0x5a0u, origin);
#ifdef HALO2_TEST_VEHICLE_WHEEL_CROSS_PRODUCT
    for (auto reg : {1u, 2u, 3u, 4u}) input.abi.xmm[reg][0] = next();
    for (auto address : {sp + 0xb4u, sp + 0xb8u, sp + 0xbcu, wheel - 4u, wheel, wheel + 4u})
        input.memory.put(address, next());
    // Exercise actual stack/wheel aliasing as well as the non-aliased path.
    if (i % 4 == 1) {
        input.abi.gp[7] = sp + 0xb8u;
        for (auto displacement : {0x10u, 0xcu, 8u})
            input.memory.put(input.abi.gp[7] - displacement, next());
    }
#endif
#ifdef HALO2_TEST_VEHICLE_WHEEL_FORCE
    const auto axes = 0x68400000u + i * 32u, parameters = 0x69000000u + i * 64u;
    const auto material_owner = 0x69800000u + i * 64u, array = 0x6a000000u + i * 80u;
    const auto datum = 0x72340000u | (7u + i % 24u);
    const auto record = array + ((datum & 0xffffu) * 5u << 5u);
    const auto route = i % 5u;
    input.abi.gp[0] = origin;
    input.abi.gp[1] = axes;
    input.abi.gp[3] = parameters;
    for (auto reg : {5u, 6u}) input.abi.xmm[reg][0] = next();
    for (auto address : {frame + 0x2cu, frame + 0x30u, origin + 0x74u, origin + 0x78u, origin + 0x8cu,
                         wheel + 0x38u, parameters + 0x10u, parameters + 0x20u, axes, axes + 4u, axes + 8u,
                         0x465418u, 0x46a4b8u, 0x4650d8u, 0x46a178u})
        input.memory.put(address, next());
    input.memory.put(wheel + 0x31u, route != 0, 1);
    input.memory.put(sp + 0x50u, material_owner);
    input.memory.put(material_owner + 0xb4u, route == 1 ? 0xffffffffu : datum);
    input.memory.put(0x526248u, material_owner);
    input.memory.put(0x52be5cu, material_owner);
    input.memory.put(material_owner + 0x44u, array);
    input.memory.put(record + 4u, route == 2 ? 0u : 0x20u);
    input.memory.put(record + 0x9cu, route == 3 ? 0u : material_owner);
#endif
#ifdef HALO2_TEST_VEHICLE_WHEEL_RESPONSE
    const auto parameters = 0x69000000u + i * 64u, coefficient = 0x6b400000u + i * 4u;
    const auto clock = 0x6c000000u + i * 16u;
    const auto bits = [](float value) { return std::bit_cast<std::uint32_t>(value); };
    input.abi.gp[0] = (input.abi.gp[0] & ~255u) | (i % 3u != 0);
    input.abi.gp[3] = parameters;
    for (const auto address : {sp + 0x30u, sp + 0x3cu, sp + 0x38u, sp + 0x100u, sp + 0x104u, sp + 0x108u,
                               sp + 0x10cu, wheel - 0x2cu, wheel - 4u, wheel, wheel + 4u, wheel + 0x4cu,
                               origin + 0x6cu, parameters + 8u, parameters + 0x14u, parameters + 0x18u,
                               parameters + 0x1cu, coefficient, 0x4650e0u, 0x46a180u, 0x465030u, 0x46a0d0u})
        input.memory.put(address, next());
    if (i % 4u != 3u) {
        // Both interpolation directions and the direct tolerance path.
        const auto first = (i & 1u) ? 1.f : -1.f;
        input.memory.put(parameters + 0x18u, bits(first));
        input.memory.put(parameters + 0x1cu, bits(-first));
        input.memory.put(wheel - 0x2cu, bits(0.25f));
        input.memory.put(0x4650e0u, bits(i % 4u == 2u ? 3.f : 0.125f));
        input.memory.put(0x46a180u, bits(i % 4u == 2u ? 3.f : 0.125f));
        input.memory.put(sp + 0x30u, bits(1.f));
        input.memory.put(sp + 0x3cu, bits(1.f));
        input.memory.put(sp + 0x100u, bits((i & 1u) ? 2.f : 0.f));
        input.memory.put(sp + 0x38u, bits(2.f));
    }
    input.memory.put(wheel + 0x50u, bits(i % 3u == 2u ? 2.f : -2.f));
    input.memory.put(0x465048u, bits(0.f));
    input.memory.put(0x46a0e8u, bits(0.f));
    input.memory.put(0x526254u, coefficient);
    input.memory.put(0x52be68u, coefficient);
    input.memory.put(0x518444u, clock);
    input.memory.put(0x51df54u, clock);
#endif
#ifdef HALO2_TEST_VEHICLE_WHEEL_OUTPUT
    const auto force_output = i % 4u == 1u ? wheel + 0x14u : 0x6d000000u + i * 32u;
    const auto torque_output = i % 4u == 2u ? force_output + 4u : 0x6e000000u + i * 32u;
    for (auto address : {wheel + 8u, wheel + 0xcu, wheel + 0x10u, wheel + 0x40u, wheel + 0x44u,
                         wheel + 0x48u, force_output, force_output + 4u, force_output + 8u,
                         torque_output, torque_output + 4u, torque_output + 8u})
        input.memory.put(address, next());
    input.memory.put(sp + 0x5a4u, force_output);
    input.memory.put(sp + 0x5a8u, torque_output);
    input.memory.put(sp + 0x1cu, i % 8u == 2u ? 0x7fffffffu : 2u);
    input.memory.put(frame + 0x3cu, i % 2u == 0u ? 4u : 2u);
#endif
#ifdef HALO2_TEST_VEHICLE_SCRATCH_RESET
    for (unsigned slot = 0; slot < 16; ++slot)
        input.memory.put(sp + 0x1e4u + slot * 0x3cu, next());
    for (auto offset : {0x18u, 0x1cu, 0x2cu, 0x34u, 0x90u})
        input.memory.put(sp + offset, next());
    // Some counts share a record marker so the final read sees the narrow
    // write from the exact iteration, with the high bits preserved.
    if (i % 6u == 0u)
        input.abi.gp[5] = sp + 0x1a0u + (i % 16u) * 0x3cu;
    constexpr std::uint32_t counts[]{0u, 1u, 0x80000000u, 0x7fffffffu};
    input.memory.put(input.abi.gp[5] + 0x44u, counts[i % std::size(counts)]);
    input.abi.gp[3] = i % 8u == 7u ? 0xffffffffu : 0u;
#endif
#ifdef HALO2_TEST_VEHICLE_WHEEL_CURVE
    input.policy = i % 4u;
    const auto factor = i % 8u == 0u ? sp + 0x10u : 0x6f000000u + i * 32u;
    input.abi.gp[1] = factor;
    for (auto address : {sp - 4u, sp + 0x14u, sp + 0x2cu, sp + 0x34u, sp + 0x104u, sp + 0x108u, sp + 0x10cu,
                         factor + 4u, factor + 8u, wheel + 0x30u, wheel + 0x40u, wheel + 0x44u, wheel + 0x48u,
                         wheel + 0x50u, wheel + 0x54u, 0x465a78u, 0x46ab30u, 0x4650f4u, 0x46a194u, 0x452000u,
                         0x456918u, 0x465850u, 0x46a908u, 0x465194u, 0x46a234u, 0x465290u, 0x46a330u,
                         0x46fc5cu, 0x474d1cu})
        input.memory.put(address, next());
    for (unsigned reg = 0; reg < 5; ++reg) input.abi.xmm[reg][0] = next();
    const auto variant = (i / 4u) % 16u;
    input.memory.put(sp + 0x1cu, variant == 6u ? 0x7fffffffu : i - 23u);
    const auto divisor = (i & 4u) ? 3u : 0xfffffffdu;
    input.memory.put(frame + 0x3cu, variant == 5u ? 0u : variant == 6u ? 0xffffffffu : divisor);
    if (variant == 7u) input.abi.x87_depth = 8;
    if (variant == 8u) input.abi.x87_depth = 7;
#endif
#ifdef HALO2_TEST_VEHICLE_CONTACT_OUTPUT
    const auto force_output = i % 4u == 1u ? wheel + 0x64u : 0x6d000000u + i * 32u;
    const auto torque_output = i % 4u == 2u ? force_output + 4u : 0x6e000000u + i * 32u;
    for (auto address : {wheel + 4u, wheel + 8u, wheel + 0xcu, wheel + 0x40u, wheel + 0x44u, wheel + 0x48u,
                         wheel + 0x58u, wheel + 0x5cu, wheel + 0x60u, wheel + 0x64u, wheel + 0x68u,
                         wheel + 0x6cu, wheel + 0x94u, wheel + 0x98u, wheel + 0x9cu,
                         sp + 0x14u, sp + 0x58u, sp + 0x5cu, sp + 0xa8u, sp + 0xacu, sp + 0xb0u,
                         force_output, force_output + 4u, force_output + 8u, torque_output,
                         torque_output + 4u, torque_output + 8u, origin + 0x70u})
        input.memory.put(address, next());
    input.memory.put(sp + 0x5a4u, force_output);
    input.memory.put(sp + 0x5a8u, torque_output);
    input.memory.put(sp + 0x13u, i % 4u != 0u, 1);
    input.memory.put(wheel + 0x80u, i % 4u != 1u, 1);
    input.memory.put(0x465048u, 0);
    input.memory.put(0x46a0e8u, 0);
    input.abi.xmm[1][0] = next();
    if (i % 8u < 4u) {
        input.abi.xmm[1][0] = i % 4u == 2u ? 0x3f800000u : 0xbf800000u;
        input.memory.put(wheel + 4u, 0x3f800000u);
        input.memory.put(wheel + 8u, 0);
        input.memory.put(wheel + 0xcu, 0);
        input.memory.put(sp + 0x58u, 0);
        input.memory.put(sp + 0x5cu, 0);
    }
#endif
#ifdef HALO2_TEST_VEHICLE_CONTACT_PROJECTION
    const auto record = i % 8u == 3u ? sp + 0x84u : 0x6d000000u + i * 32u;
    input.abi.gp[3] = record;
    for (auto address : {frame, frame + 4u, frame + 8u, record, record + 4u, record + 8u,
                         wheel + 4u, wheel + 8u, wheel + 0xcu, wheel + 0x8cu, sp + 0x14u,
                         sp + 0x60u, sp + 0x64u, sp + 0x68u, sp + 0x78u, sp + 0x7cu, sp + 0x80u,
                         sp + 0x84u, sp + 0x88u, sp + 0x8cu, sp + 0xd0u, sp + 0xd4u, sp + 0xd8u})
        input.memory.put(address, next());
    input.memory.put(sp + 0x20u, frame);
    input.memory.put(sp + 0x13u, i % 4u != 1u, 1);
    for (auto address : {0x46504cu, 0x46a0ecu}) input.memory.put(address, 0x3a83126fu);
    for (auto address : {0x465030u, 0x46a0d0u}) input.memory.put(address, 0x3f800000u);
    for (auto address : {0x465048u, 0x46a0e8u}) input.memory.put(address, 0);
    if (i % 8u < 4u) {
        input.memory.put(frame, 0x3f800000u);
        input.memory.put(frame + 4u, 0);
        input.memory.put(frame + 8u, 0);
        input.memory.put(wheel + 4u, 0x3f800000u);
        input.memory.put(wheel + 8u, i % 4u == 0u ? 0u : i % 4u == 3u ? 0x7fc12345u : 0x3f800000u);
        input.memory.put(wheel + 0xcu, 0);
    }
#endif
#ifdef HALO2_TEST_VEHICLE_CONTACT_QUERY
    using namespace wheel_stage;
    constexpr std::uint32_t entries[]{loaded_force, projected_normal, unprojected_force};
    const auto source_index = (i / 8u) % 3u;
    const auto mode = i % 8u;
    const auto contact = wheel;
    const auto definition = i % 7u == 0u ? sp + 0x50u : 0x69100000u + i * 0x200u;
    const auto driving_wheel = 0x69200000u + i * 0x100u;
    const auto load_axis = i % 5u == 0u ? sp + 0x54u : 0x69300000u + i * 32u;
    const auto clock = 0x69400000u + i * 32u;
    const auto default_axis = 0x69500000u + i * 32u;
    input.policy = entries[source_index];
    input.abi.gp[0] = load_axis;
    input.abi.gp[6] = driving_wheel;
    for (auto displacement : {0x14u, 0x1cu, 0x20u, 0x30u, 0x3cu, 0x44u, 0x48u, 0x4cu, 0x50u,
                             0x54u, 0x58u, 0x5cu, 0x60u, 0x64u, 0x68u, 0x90u, 0x98u, 0xdcu,
                             0xe0u, 0xe4u, 0x11cu, 0x120u, 0x124u, 0x128u, 0x12cu, 0x130u,
                             0x134u, 0x138u, 0x13cu, 0x140u, 0x144u, 0x148u, 0x14cu, 0x150u, 0x154u, 0x158u})
        input.memory.put(sp + displacement, next());
    for (auto address : {contact + 0x34u, contact + 0x38u, contact + 0x3cu, contact + 0x80u,
                         contact + 0x84u, contact + 0x88u, contact + 0x90u, contact + 0xa8u,
                         contact + 0xb0u, frame, frame + 4u, frame + 8u, driving_wheel + 8u,
                         driving_wheel + 0xcu, driving_wheel + 0x18u, driving_wheel + 0x40u,
                         definition + 0x18u, definition + 0xb4u, load_axis, load_axis + 4u,
                         load_axis + 8u, clock + 4u, default_axis, default_axis + 4u, default_axis + 8u})
        input.memory.put(address, next());
    input.memory.put(sp + 0x20u, definition);
    input.memory.put(sp + 0x50u, definition);
    for (auto address : {0x518444u, 0x51df54u}) input.memory.put(address, clock);
    for (auto address : {0x4650f4u, 0x46a194u}) input.memory.put(address, 0x41000000u);
    for (auto address : {0x46504cu, 0x46a0ecu}) input.memory.put(address, 0x3a83126fu);
    for (auto address : {0x465030u, 0x46a0d0u}) input.memory.put(address, 0x3f800000u);
    for (auto address : {0x46fc50u, 0x474d10u}) input.memory.put(address, default_axis);

    // Author each short-circuit and both query modes. Keep random payloads in
    // the vector inputs and unused lanes to exercise ordered NaN propagation.
    input.memory.put(driving_wheel + 0xcu, mode == 0u ? 0u : 0x40000000u);
    input.memory.put(contact + 0xa8u, mode == 1u ? 0u : 0x40800000u);
    input.memory.put(definition + 0x18u, 0x3f800000u);
    input.memory.put(contact + 0x82u, mode == 6u, 1);
    input.memory.put(contact + 0x83u, mode == 7u, 1);
    if (mode == 2u || mode == 3u || mode == 4u) {
        const auto value = mode == 2u ? 0u : mode == 3u ? 0x3f800000u : 0x7fc12345u;
        const auto axis = input.policy == unprojected_force ? 0x44u : 0x60u;
        input.memory.put(sp + axis, value);
        input.memory.put(sp + axis + 4u, 0);
        input.memory.put(sp + axis + 8u, 0);
    }
    if (hardware) {
        const auto number = [&](auto address, float value) { input.memory.put(address, std::bit_cast<std::uint32_t>(value)); };
        const auto precision = (i / 24u) % 3u == 0u ? 0u : (i / 24u) % 3u == 1u ? 2u : 3u;
        const auto rounding = (i / 72u) % 4u;
        input.abi.x87_depth = 0;
        input.abi.x87_status = 0;
        input.abi.floating_point = SourcePhysicsFactoryFloatingPoint{
            std::uint16_t(0x7fu | (precision << 8u) | (rounding << 10u)), 0x1f80u | (rounding << 13u)};
        number(sp + 0x3cu, 2.0f);
        number(sp + 0x98u, 1.0f);
        number(clock + 4u, 0.5f);
        number(contact + 0x88u, 1.0f);
        number(contact + 0x90u, mode % 2u ? 2.0f : 4.0f);
        number(contact + 0x84u, 0.5f);
        number(driving_wheel + 8u, 2.0f);
        number(driving_wheel + 0x18u, 3.0f);
        number(load_axis, 0.5f);
        number(load_axis + 4u, -0.5f);
        number(load_axis + 8u, 0.25f);
        const auto axis = input.policy == unprojected_force ? 0x44u : 0x60u;
        for (auto offset : {0u, 4u, 8u}) number(sp + axis + offset, mode == 2u ? 0.0f : offset == 0u ? 1.0f : 2.0f);
        if (mode == 4u) input.memory.put(sp + axis, 0x7fc12345u);
    }
#endif
    return input;
}

unsigned compare(const Case &input, unsigned i, Policy policy, bool observe,
                 unsigned fault = 0, unsigned transition = 0) {
    Provider reference(input.memory, fault, transition), native(input.memory, fault, transition);
    const auto expected = run(reference, input, policy, observe, true);
    const auto actual = run(native, input, policy, observe, false);
    bool expect_fault = fault != 0;
    bool stack_matches = input.abi.gp[4] == actual.abi.gp[4];
#ifdef HALO2_TEST_VEHICLE_WHEEL_CURVE
    if (input.policy == 0u) {
        const auto variant = (i / 4u) % 16u;
        if (!fault && variant >= 5u && variant <= 8u) {
            expect_fault = true;
            const auto message = variant <= 6u ? "division-fault" : "x87 stack-fault";
            if (actual.fault.find(message) == std::string::npos)
                throw std::runtime_error("Wheel curve failed to preserve the expected intrinsic fault");
        }
        const auto pushed = input.abi.gp[4] - 4u == actual.abi.gp[4];
        stack_matches = expect_fault ? stack_matches || pushed : pushed;
    }
#endif
#ifdef HALO2_TEST_VEHICLE_CONTACT_OUTPUT
    if (!fault && actual.continuation != wheel_stage::advance_contact)
        throw std::runtime_error("Vehicle contact output changed the loop continuation");
    if (!fault && observe && i % 8u < 4u) {
        const auto saw_store = [&](std::uint32_t pc) {
            return std::any_of(actual.stores.begin(), actual.stores.end(),
                [&](const auto &entry) { return entry.instruction == pc; });
        };
        if (saw_store(0x20afe0u) != (i % 4u == 2u) || saw_store(0x20afedu) != (i % 4u == 3u))
            throw std::runtime_error("Vehicle contact projection did not take its authored branch");
    }
#endif
#ifdef HALO2_TEST_VEHICLE_CONTACT_PROJECTION
    if (!fault && i % 8u < 4u) {
        constexpr std::uint32_t exits[]{wheel_stage::skip_projection, wheel_stage::alternate_response,
                                       wheel_stage::continue_response, wheel_stage::continue_response};
        if (actual.continuation != exits[i % 4u])
            throw std::runtime_error("Vehicle projection selected the wrong authored continuation");
    }
#endif
    if (!identical(expected.abi, actual.abi) || expected.trace != actual.trace ||
        expected.stores != actual.stores || expected.cursor != actual.cursor ||
        expected.continuation != actual.continuation ||
        expected.fault != actual.fault || expected.fault.empty() == expect_fault ||
        reference.accesses != native.accesses || !reference.same(native) ||
        !stack_matches || input.abi.gp[5] != actual.abi.gp[5]) {
        std::fprintf(stderr, "Wheel offset case %u policy %u fault %u trace %u PC %x/%x IO %zu/%zu\n",
            i, unsigned(policy), fault, transition, expected.cursor, actual.cursor,
            reference.accesses.size(), native.accesses.size());
        if (expected.fault != actual.fault)
            std::fprintf(stderr, "Reference fault '%s', native fault '%s'\n", expected.fault.c_str(), actual.fault.c_str());
        throw std::runtime_error("Native wheel stage differs from the retained state");
    }
#ifdef HALO2_TEST_VEHICLE_CONTACT_QUERY
    if (!fault && actual.continuation != (i % 8u < 2u ? wheel_stage::skip_query : wheel_stage::query_ready))
        throw std::runtime_error("Vehicle contact query changed its authored continuation");
#endif
#ifdef HALO2_TEST_VEHICLE_WHEEL_FORCE
    if (!fault && actual.continuation != (i % 5u == 4 ? wheel_stage::call_site : wheel_stage::bypass))
        throw std::runtime_error("Wheel force case did not reach its selected child or bypass boundary");
#endif
#ifdef HALO2_TEST_VEHICLE_WHEEL_RESPONSE
    constexpr std::uint32_t exits[]{wheel_stage::rejected, wheel_stage::accumulate, wheel_stage::x87_detour};
    if (!fault && actual.continuation != exits[i % 3u])
        throw std::runtime_error("Wheel response did not reach its selected retained continuation");
#endif
#ifdef HALO2_TEST_VEHICLE_WHEEL_OUTPUT
    if (!fault && actual.continuation != (i % 2u == 0u ? wheel_stage::next_wheel : wheel_stage::after_loop))
        throw std::runtime_error("Wheel output did not select its original loop boundary");
#endif
#ifdef HALO2_TEST_VEHICLE_SCRATCH_RESET
    const auto has_records = std::bit_cast<std::int32_t>(actual.abi.gp[0]) >
                             std::bit_cast<std::int32_t>(input.abi.gp[3]);
    if (!fault && actual.continuation != (has_records ? wheel_stage::prepare_child : wheel_stage::bypass_child))
        throw std::runtime_error("Vehicle scratch reset selected the wrong count branch");
#endif
#ifdef HALO2_TEST_VEHICLE_WHEEL_CURVE
    if (!expect_fault && actual.continuation != (input.policy == 0 ? wheel_stage::call_site : wheel_stage::output))
        throw std::runtime_error("Wheel curve changed its selected genuine continuation");
#endif
    return unsigned(native.accesses.size());
}

#ifdef HALO2_TEST_VEHICLE_WHEEL_CURVE
void check_curve_spill_rounding() {
    constexpr std::uint32_t stack = 0x72000000u;
    constexpr SourcePhysicsFactoryFloatingPoint controls{0x37fu, 0x1f80u};
    FpScope scope(controls);
    Memory memory;
    memory.put(stack + 0x14u, 0);
    SourcePhysicsContactRecordObserver observe;
    SourcePhysicsContactRuntimeServices services;
    MotionState state{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    Abi entry{};
    entry.gp[4] = stack;
    entry.x87_depth = 1;
    entry.floating_point = controls;
    for (bool rounds_up : {false, true}) {
        entry.x87[0] = rounds_up ? 0x1.0000018p0L : 0x1.0000008p0L;
        entry.x87_status = std::uint16_t(0x3800u | (rounds_up ? 0u : 0x200u));
        state.load(entry);
        wheel_stage::store_curve_argument_reference(state);
        if (state.fp.count || bool(state.fp.word() & 0x200u) != rounds_up ||
            memory.get(stack + 0x14u) != (rounds_up ? 0x3f800001u : 0x3f800000u))
            throw std::runtime_error("Wheel argument FSTP did not publish the hardware rounding direction");
    }

    state.load(entry);
    struct StoreFault {};
    try {
        wheel_stage::spill_x87_float_and_pop(state, [](std::uint32_t) { throw StoreFault{}; });
        throw std::runtime_error("Wheel argument fault fixture did not fire");
    } catch (const StoreFault &) {
        if (state.fp.count != 1 || state.fp.top != 7 || (state.fp.word() & 0x200u))
            throw std::runtime_error("Wheel argument FSTP retired after a failed store");
    }
}
#endif

#ifdef HALO2_TEST_VEHICLE_CONTACT_PROJECTION
void check_projection_spill() {
    constexpr std::uint32_t stack = 0x72000000u;
    constexpr SourcePhysicsFactoryFloatingPoint controls{0x37fu, 0x1f80u};
    FpScope scope(controls);
    Memory memory;
    memory.put(stack + 0x14u, 0);
    SourcePhysicsContactRecordObserver observe;
    SourcePhysicsContactRuntimeServices services;
    MotionState state{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    Abi entry{};
    entry.gp[4] = stack;
    entry.x87_depth = 1;
    entry.x87_status = 0x3800u;
    entry.x87[0] = -0x1.0000018p0L;
    entry.floating_point = controls;
    state.load(entry);
    wheel_stage::store_tangent_length_reference(state);
    if (state.fp.count != 1 || state.fp.top != 7u || state.fp.st(0) != entry.x87[0] ||
        !(state.fp.word() & 0x200u) || memory.get(stack + 0x14u) != 0xbf800001u)
        throw std::runtime_error("Vehicle projection FST changed the retained extended value");
    wheel_stage::absolute_tangent_length(state);
    if (state.fp.st(0) != -entry.x87[0] || (state.fp.word() & 0x200u))
        throw std::runtime_error("Vehicle projection FABS changed the retained x87 condition");
}
#endif
}

#ifdef HALO2_TEST_VEHICLE_CONTACT_QUERY
nlohmann::json hardware_fixture(unsigned count) {
    using namespace wheel_stage;
    using Json = nlohmann::json;
    const auto hex = [](std::span<const std::byte> bytes) {
        constexpr char digits[] = "0123456789abcdef";
        std::string result;
        result.reserve(bytes.size() * 2u);
        for (auto byte : bytes) {
            const auto bits = std::to_integer<unsigned>(byte);
            result.push_back(digits[bits >> 4u]);
            result.push_back(digits[bits & 15u]);
        }
        return result;
    };
    const auto format_memory = [&](const Memory &memory) {
        auto banks = Json::array();
        for (const auto &[page, bytes] : memory.pages) {
            // Keep only one canonical page for each overlapping address.
            banks.push_back({{"address", page << 12u}, {"hex", hex(*bytes)}});
        }
        return banks;
    };
    const auto format_abi = [](const Abi &abi) {
        if (abi.x87_depth != 0u || abi.active_mmx)
            throw std::runtime_error("Contact query hardware fixture must preserve its empty x87 stack");
        return Json{{"gp", abi.gp}, {"eflags", abi.eflags}, {"xmm_words", abi.xmm},
            {"fs_base", abi.fs_base}, {"mxcsr", abi.floating_point->mxcsr}, {"x87_values", Json::array()},
            {"x87_control_word", abi.floating_point->x87_control_word}, {"x87_status", abi.x87_status}, {"x87_tag", 0xffffu}};
    };
    std::mt19937 random(0x20f3a8u);
    Json cases = Json::array();
    for (unsigned i = 0; i < count; ++i) {
        const auto input = fixture(random, i, true);
        auto memory = input.memory.clone();
        Json writes = Json::array();
        std::uint32_t pc{};
        SourcePhysicsContactRuntimeServices services;
        services.instruction = [&](std::uint32_t site) { pc = site; };
        constexpr std::array service_sites{0x38846eu, 0x21f590u, 0x33b595u};
        services.instruction_cursor = {&pc, nullptr, nullptr, service_sites, std::this_thread::get_id(),
            &services.instruction.target_type(), true, true};
        const SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
            writes.push_back({{"instruction", event.instruction + image_delta}, {"address", event.address},
                {"before", hex(event.before)}, {"written", hex(event.written)}});
        };
        FpScope scope(*input.abi.floating_point);
        MotionState state{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
        state.load(input.abi);
        const auto exit = bridge(state, input.policy);
        if (exit != (i % 8u < 2u ? skip_query : query_ready))
            throw std::runtime_error("Contact query native hardware profile did not reach its expected exit");
        cases.push_back({{"id", i}, {"entry", input.policy + image_delta}, {"exit_entry", exit + image_delta},
            {"return_address", 0x100f000u}, {"initial_banks", format_memory(input.memory)},
            {"final_banks", format_memory(memory)}, {"entry_abi", format_abi(input.abi)},
            {"exit", format_abi(state.save())}, {"native_writes", writes}});
    }
    return cases;
}
#endif

int main(int argc, char **argv) try {
    if (argc != 3) return 2;
#ifdef HALO2_TEST_VEHICLE_CONTACT_QUERY
    if (std::string_view{argv[2]} == "hardware") {
        std::puts(hardware_fixture(std::stoul(argv[1])).dump().c_str());
        return 0;
    }
#endif
    if (std::string_view{argv[2]} == "reference")
        setenv(reference_flag, "1", 1);
#ifdef HALO2_TEST_VEHICLE_WHEEL_CURVE
    check_curve_spill_rounding();
#endif
#ifdef HALO2_TEST_VEHICLE_CONTACT_PROJECTION
    check_projection_spill();
#endif
    const auto cases = std::stoul(argv[1]);
    std::mt19937 random(0x20e414u);
    unsigned checks = 0, faults = 0, transitions = 0;
    for (unsigned i = 0; i < cases; ++i) {
        const auto input = fixture(random, i);
        for (auto policy : {Policy::quiet, Policy::cursor, Policy::remap, Policy::silent_remap}) {
            compare(input, i, policy, policy == Policy::cursor || policy == Policy::remap);
            ++checks;
        }
    }
#ifdef HALO2_TEST_VEHICLE_CONTACT_QUERY
    constexpr unsigned fault_profiles = 24;
#else
    constexpr unsigned fault_profiles = 8;
#endif
    for (unsigned i = 0; i < fault_profiles; ++i) {
        const auto input = fixture(random, i);
        for (auto policy : {Policy::cursor, Policy::remap, Policy::silent_remap}) {
            const bool observe = policy != Policy::silent_remap;
            const auto count = compare(input, i, policy, observe);
            ++checks;
            for (unsigned n = 1; n <= count; ++n) {
                compare(input, i, policy, observe, n);
                compare(input, i, policy, observe, 0, n);
                ++faults;
                ++transitions;
                checks += 2;
            }
        }
        for (auto policy : {Policy::trace, Policy::arbitrary, Policy::bad_map, Policy::map_pack}) {
            compare(input, i, policy, true);
            ++checks;
        }
    }
    std::printf("{\"cases\":%u,\"provider_faults\":%u,\"observer_transitions\":%u}\n", checks, faults, transitions);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
