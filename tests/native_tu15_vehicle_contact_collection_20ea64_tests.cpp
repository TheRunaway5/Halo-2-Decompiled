#include "native_tu15_frame_misc_harness.hpp"
#include "physics/vehicles/native_tu15_vehicle_contact_collection_20ea64.hpp"
#include <nlohmann/json.hpp>

namespace collection = halo2::native_vehicle_contact_collection_20ea64_detail;
namespace original {
using halo2::SourcePhysicsContactImage;
using namespace halo2::physics_vehicle_motion_detail;
#include "vehicle_contact_collection_original.inc"
}

namespace {
using namespace frame_misc_harness;
using collection::MotionState;
using Abi = SourcePhysicsContactRuntimeAbi;

constexpr std::array entries{collection::prepare_collection, collection::begin_collection,
    collection::next_wheel, collection::apply_contact};

Case fixture(std::mt19937 &random, unsigned i, bool hardware) {
    using namespace collection;
    Case input;
    auto &memory = input.memory;
    const auto entry = entries[i / 8u % entries.size()];
    const auto mode = i % 8u;
    const auto stack = 0x62000400u + 4u * (i % 16u);
    const auto definition = 0x63000200u;
    const auto vehicle = 0x64000000u;
    const auto assets = 0x65000000u;
    const auto metadata = 0x66000000u;
    const auto record = 0x68000200u;
    const auto index = mode % 4u;
    const auto wheel = vehicle + 0xb10u + 0xd8u * index;
    const auto offset = mode == 7u ? stack + 0x44u : wheel + 0x4cu;
    const auto clock = 0x69000000u;
    random_abi(random, input, stack);
    input.policy = entry;
    input.abi.gp[3] = entry == apply_contact ? offset : index;
    input.abi.gp[5] = definition;
    input.abi.gp[6] = assets;
    input.abi.gp[7] = wheel;

    constexpr std::uint32_t edge[]{0x7fc12345u, 0xffc54321u, 0x7f812345u, 0x7f800000u, 0xff800000u,
        0x80000000u, 0u, 1u, 0x807fffffu, 0x3f800000u, 0xbf800000u};
    const auto bits = [&] {
        return hardware ? std::bit_cast<std::uint32_t>(float(int(random() % 21u) - 10) / 4.0f) :
            i % 2u ? std::uint32_t(random()) : edge[random() % std::size(edge)];
    };
    for (auto displacement : {0x13u, 0x14u, 0x1bu, 0x1cu, 0x20u, 0x24u, 0x34u, 0x44u, 0x48u, 0x4cu,
                             0x54u, 0x58u, 0x5cu, 0x94u, 0xb4u, 0xb8u, 0xbcu, 0xc0u, 0xc4u, 0xc8u,
                             0x1e0u, 0x5a0u}) memory.put(stack + displacement, bits());
    for (unsigned a = 1; a <= 4; ++a) memory.put(stack - 4u * a, bits());
    for (auto delta : {0x34u, 0x38u, 0x3cu, 0x40u, 0x44u, 0x48u, 0x4cu, 0x50u, 0x54u, 0x70u, 0x74u,
                      0x78u, 0x7cu, 0x84u, 0x88u, 0x94u, 0x98u, 0x9cu, 0xa0u, 0xa4u, 0xa8u, 0xb3u})
        memory.put(wheel + delta, bits());
    for (auto address : {definition + 0x18u, definition + 0xcu, assets + 0xb4u, vehicle + 0x60u,
                         vehicle + 0x64u, vehicle + 0x68u, vehicle + 0x6cu, metadata + index * 0x4cu + 4u,
                         record, record + 4u, record + 8u, offset, offset + 4u, offset + 8u, clock})
        memory.put(address, bits());
    memory.put(stack + 0x14u, record);
    memory.put(stack + 0x1cu, index);
    memory.put(stack + 0x20u, definition);
    memory.put(stack + 0x5a0u, vehicle);
    memory.put(definition + 8u, mode == 6u ? 0u : 0x3f400000u);
    memory.put(definition + 0x44u, mode == 0u ? index : 4u);
    memory.put(definition + 0x48u, metadata);
    memory.put(wheel + 0xb3u, mode == 1u || mode == 7u ? 1u : 0u, 1u);
    memory.put(wheel + 0x70u, mode & 1u, 1u);
    memory.put(wheel + 0x84u, mode <= 1u ? 0u : 0x3f800000u);
    memory.put(record + 0x24u, (mode & 4u) ? 0xffffffffu : 32u);
    for (auto address : {0x46fc44u, 0x474d04u, 0x46fc50u, 0x474d10u}) memory.put(address, record);
    for (auto address : {0x526254u, 0x52be68u}) memory.put(address, clock);
    for (auto address : {0x46504cu, 0x46a0ecu}) memory.put(address, 0x3727c5acu);
    if (hardware) {
        const auto rounding = i / 32u % 4u;
        input.abi.x87_depth = 0;
        input.abi.x87_status = 0;
        input.abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x27fu, 0x1f80u | (rounding << 13u)};
    }
    return input;
}

bool same_registers(const Abi &a, const Abi &b) {
    return same_abi(a, b) && a.fs_base == b.fs_base && a.floating_point->mxcsr == b.floating_point->mxcsr;
}

struct Access {
    char kind;
    std::uint32_t pc, address;
    std::size_t width;
    Abi before;
    bool operator==(const Access &other) const {
        return kind == other.kind && pc == other.pc && address == other.address && width == other.width &&
            same_registers(before, other.before);
    }
};

struct Provider : Memory {
    mutable std::vector<Access> accesses;
    std::function<Access(char, std::uint32_t, std::size_t)> snapshot;
    std::function<void()> enable_trace;
    unsigned fault{}, transition{};
    bool hardware{};

    explicit Provider(const Memory &input, unsigned reject = 0, unsigned enable = 0)
        : Memory(input.clone()), fault(reject), transition(enable) {}

    void visit(char kind, std::uint32_t address, std::size_t width) const {
        accesses.push_back(snapshot(kind, address, width));
        const auto counter = unsigned(accesses.size());
        if (!hardware) {
            _mm_setcsr(0x1f80u | ((counter & 3u) << 13u) | (counter & 0x3fu) | ((counter & 2u) ? 0x8040u : 0u));
            source_load_x87_exception_flags(std::uint16_t((counter * 7u) & 0x3fu));
        }
        if (counter == fault) throw std::runtime_error("Wheel contact provider rejection");
        if (counter == transition) enable_trace();
    }

    void read(std::uint32_t address, std::span<std::byte> out) const override {
        visit('r', address, out.size());
        Memory::read(address, out);
    }
    void write(std::uint32_t address, std::span<const std::byte> in) override {
        visit('w', address, in.size());
        Memory::write(address, in);
    }
};

enum class Policy { quiet, cursor, remap, silent_remap, trace, bad_map, disc, hardware };

struct Result {
    Abi abi;
    std::uint32_t cursor{}, continuation{};
    std::string fault;
    std::vector<std::uint32_t> trace;
    std::vector<SourcePhysicsContactRecordStore> stores;
};

std::span<const std::uint32_t> sites_for(std::uint32_t entry) {
    using namespace collection;
    if (entry == prepare_collection) return arguments_sites;
    if (entry == apply_contact) return contact_sites;
    return wheel_sites;
}

Result execute(Provider &memory, const Case &input, Policy policy, bool reference) {
    using namespace collection;
    Result result;
    const bool silent = policy == Policy::silent_remap;
    const bool remapped = policy == Policy::remap || policy == Policy::bad_map || silent;
    bool tracing = policy == Policy::trace;
    SourcePhysicsContactRuntimeServices services, source;
    auto &publisher = remapped ? source : services;
    const auto track = [&](auto pc) {
        result.cursor = pc;
        if (tracing) result.trace.push_back(pc);
    };
    constexpr std::array service_sites{0x38846eu, 0x21f590u, 0x33b595u};
    if (policy != Policy::quiet) {
        publisher.instruction = track;
        publisher.instruction_cursor = {&result.cursor, &tracing, nullptr, service_sites, std::this_thread::get_id(),
            &publisher.instruction.target_type(), true, true};
        if (silent) publisher.instruction = {};
    }
    std::vector<PhysicsInstructionRemapTu15::Binding> mapping;
    if (remapped) {
        for (auto pc : sites_for(input.policy)) mapping.push_back({pc, pc + image_delta});
        if (policy == Policy::bad_map) ++mapping[mapping.size() / 2u].value;
        services.instruction = PhysicsInstructionRemapTu15{&source, mapping};
    }
    SourcePhysicsContactRecordObserver observe;
    if (policy != Policy::quiet && !silent) observe = [&](const auto &event) {
        result.stores.push_back(event);
        memory.visit('o', event.address, event.written.size());
    };
    memory.hardware = policy == Policy::hardware;
    FpScope scope(*input.abi.floating_point);
    MotionState state{memory, observe, services, policy == Policy::disc ?
        SourcePhysicsContactImage::MapPack11 : SourcePhysicsContactImage::TitleUpdate15};
    state.load(input.abi);
    memory.snapshot = [&](auto kind, auto address, auto width) {
        return Access{kind, result.cursor, address, width, state.save()};
    };
    memory.enable_trace = [&] {
        tracing = true;
        if (silent) publisher.instruction = track;
    };
    try {
        if (memory.hardware) result.continuation = bridge(state, input.policy);
        else if (reference) result.continuation = original::retained(state, input.policy);
        else result.continuation = original::selected(state, input.policy);
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
    result.abi = state.save();
    return result;
}

unsigned compare(const Case &input, Policy policy, unsigned fault = 0, unsigned transition = 0) {
    Provider reference(input.memory, fault, transition), native(input.memory, fault, transition);
    const auto expected = execute(reference, input, policy, true);
    const auto actual = execute(native, input, policy, false);
    if (!same_registers(expected.abi, actual.abi) || expected.continuation != actual.continuation ||
        expected.fault != actual.fault || actual.fault.empty() != (fault == 0) || expected.cursor != actual.cursor ||
        expected.trace != actual.trace || expected.stores != actual.stores || reference.accesses != native.accesses ||
        !reference.same(native)) {
        std::fprintf(stderr, "Contact collection entry %x policy %u fault %u trace %u owner %x/%x IO %zu/%zu messages '%s' / '%s'\n",
            input.policy, unsigned(policy), fault, transition, expected.continuation, actual.continuation,
            reference.accesses.size(), native.accesses.size(), expected.fault.c_str(), actual.fault.c_str());
        throw std::runtime_error("Contact collection differs from retained original state");
    }
    return unsigned(native.accesses.size());
}

nlohmann::json hardware_fixture(unsigned count) {
    using Json = nlohmann::json;
    using namespace collection;
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
    const auto banks = [&](const Memory &memory) {
        auto result = Json::array();
        for (const auto &[page, bytes] : memory.pages) result.push_back({{"address", page << 12u}, {"hex", hex(*bytes)}});
        return result;
    };
    const auto abi = [](const Abi &value) {
        if (value.x87_depth || value.active_mmx) throw std::runtime_error("Contact fixture expects an empty x87 stack");
        return Json{{"gp", value.gp}, {"eflags", value.eflags}, {"xmm_words", value.xmm},
            {"fs_base", value.fs_base}, {"mxcsr", value.floating_point->mxcsr}, {"x87_values", Json::array()},
            {"x87_control_word", value.floating_point->x87_control_word}, {"x87_status", value.x87_status}, {"x87_tag", 0xffffu}};
    };
    std::mt19937 random(0x20ea64u);
    Json cases = Json::array();
    for (unsigned i = 0; i < count; ++i) {
        const auto input = fixture(random, i, true);
        Provider memory(input.memory);
        const auto result = execute(memory, input, Policy::hardware, false);
        if (!result.continuation || !result.fault.empty()) throw std::runtime_error("Contact hardware fixture did not complete natively");
        auto writes = Json::array();
        for (const auto &event : result.stores) writes.push_back({{"instruction", event.instruction + image_delta},
            {"address", event.address}, {"before", hex(event.before)}, {"written", hex(event.written)}});
        cases.push_back({{"id", i}, {"entry", input.policy + image_delta}, {"exit_entry", result.continuation + image_delta},
            {"return_address", 0x100f000u}, {"initial_banks", banks(input.memory)}, {"final_banks", banks(memory)},
            {"entry_abi", abi(input.abi)}, {"exit", abi(result.abi)}, {"native_writes", writes}});
    }
    return cases;
}
}

int main(int argc, char **argv) try {
    if (argc != 3) return 2;
    const auto count = std::stoul(argv[1]);
    if (std::string_view(argv[2]) == "hardware") {
        std::puts(hardware_fixture(count).dump().c_str());
        return 0;
    }
    std::mt19937 random(0x20ea64u);
    unsigned compared{}, prefixes{};
    for (unsigned i = 0; i < count; ++i) {
        const auto input = fixture(random, i, false);
        for (auto policy : {Policy::quiet, Policy::cursor, Policy::remap, Policy::silent_remap}) {
            compare(input, policy);
            ++compared;
        }
    }
    for (unsigned i = 0; i < 32; ++i) {
        const auto input = fixture(random, i, false);
        for (auto policy : {Policy::cursor, Policy::remap, Policy::silent_remap}) {
            const auto accesses = compare(input, policy);
            ++compared;
            for (unsigned n = 1; n <= accesses; ++n) {
                compare(input, policy, n);
                compare(input, policy, 0, n);
                ++prefixes;
                compared += 2;
            }
        }
        for (auto policy : {Policy::trace, Policy::bad_map, Policy::disc}) {
            compare(input, policy);
            ++compared;
        }
    }
    std::printf("{\"cases\":%u,\"provider_faults\":%u,\"trace_resumes\":%u}\n", compared, prefixes, prefixes);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
