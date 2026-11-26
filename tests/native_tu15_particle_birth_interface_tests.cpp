#include "native_tu15_particle_birth_observer_fixture.hpp"
#include "rendering/effects/native_tu15_particle_birth_interface.hpp"
#include "rendering/effects/native_tu15_particle_birth_placement.hpp"
#include "rendering/effects/native_tu15_particle_birth_prefix.hpp"

using namespace frame_misc_harness;
namespace birth = halo2::native_particle_birth_interface_detail;
namespace placement = halo2::native_particle_birth_placement_detail;
namespace prefix = halo2::native_particle_birth_prefix_detail;
namespace world = halo2::native_world_leaves_detail;

namespace {
using enum world::Register;

constexpr std::uint32_t stack = 0x20004600u, frame = stack - 0xc0u;
constexpr std::uint32_t emitter = 0x11002000u, system = 0x11003000u, context = 0x11004000u;
constexpr std::uint32_t definition = 0x11005000u, random_source = 0x11006000u;
constexpr std::uint32_t objects = 0x11007000u, object_data = 0x11008000u;
constexpr std::uint32_t particles = 0x11009000u, particle_data = 0x1100a000u, particle = particle_data + 0x40u;
constexpr std::uint32_t tag_data = 0x1100b000u, attachments = 0x1100c000u, attachment_data = 0x1100d000u;
constexpr std::uint32_t attachment_owner = 0x1100e000u, authored_origin = 0x1100f000u, notify_data = 0x11010000u;
constexpr std::uint32_t vtables = 0x11011000u, notify_word = notify_data + 0x10u;
constexpr std::uint32_t particle_handle = 0x18ea0001u, attachment_handle = 0x341b0001u;
constexpr std::array interfaces{attachment_owner, birth::standard_interface, birth::alternate_interface};
constexpr std::array methods{0x10u, 0x14u, 0x18u, 0x1cu, 0x50u};

struct Access {
    std::uint32_t address;
    std::size_t width;
    bool write;
    friend bool operator==(const Access &, const Access &) = default;
};

struct FaultBank : Memory {
    explicit FaultBank(const Memory &base, unsigned fault) : Memory(base.clone()), fault(fault) {}
    unsigned fault;
    mutable std::vector<Access> accesses;

    void record(Access event) const {
        accesses.push_back(event);
        if (accesses.size() == fault) throw std::runtime_error("Injected particle birth bank fault");
    }
    void read(std::uint32_t address, std::span<std::byte> bytes) const override {
        record({address, bytes.size(), false});
        Memory::read(address, bytes);
    }
    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        record({address, bytes.size(), true});
        Memory::write(address, bytes);
    }
};

std::uint32_t method_target(unsigned interface_index, std::uint32_t method_offset) {
    return 0x300000u + 0x100u * interface_index + method_offset;
}

Case fixture(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, stack);
    test.abi.gp[eax] = context;
    test.abi.gp[ecx] = emitter;
    test.policy = index;

    auto scalar_word = [&]() -> std::uint32_t {
        constexpr std::array values{0x7fc01728u, 0xffc03546u, 0x7f801122u, 0x7f800000u, 0x00002717u, 0x80000000u};
        if (index % 7u == 5u) return values[random() % values.size()];
        return u(float(int(random() % 2001u) - 1000) / 511.0f);
    };
    auto &memory = test.memory;
    for (auto address = frame - 0x40u; address != stack + 0x60u; address += 4u)
        memory.put(address, scalar_word());
    for (auto base : {emitter, system, context, definition, object_data, particle_data, attachment_data, authored_origin})
        for (unsigned offset = 0u; offset != 0x300u; offset += 4u)
            memory.put(base + offset, scalar_word());

    memory.put(stack, 0x250a9fu);
    memory.put(stack + 4u, system);
    memory.put(stack + 8u, definition);
    memory.put(stack + 12u, random());
    memory.put(stack + 16u, (index & 0x80u) ? u(0.625f) : scalar_word());
    memory.put(system + 0x18u, (index % 3u == 0u) ? birth::invalid : 0x27200001u);
    memory.put(0x4f720cu, objects);
    memory.put(objects + 0x44u, object_data);
    memory.put(object_data + 0x192u, index % 13u == 12u ? 8u : 0u, 1);
    memory.put(0x474d04u, authored_origin);
    memory.put(prefix::location::globals::particles, particles);
    memory.put(particles + 0x44u, particle_data);
    memory.put(prefix::random_state, random_source);
    memory.put(random_source + 4u, random());
    memory.put(prefix::low_unsigned_correction, 0x4f800000u);
    memory.put(prefix::inputs::globals::word_scale, 0x37800080u);
    memory.put(prefix::random_word_range, 0x477fff00u);
    memory.put(context + 0x50u, (index & 1u) ? particle : 0u);
    memory.put(placement::one, 0x3f800000u);
    memory.put(placement::ray_plane_z, 0xbf800000u);
    memory.put(placement::ray_offset, 0x3ba3d70au);
    memory.put(definition + 0xcu, index >> 1u & 1u, 2);
    memory.put(definition + 0x16u, (index & 4u) ? 0x10u : 0u, 1);
    memory.put(definition + 0x78u, (index & 8u) ? 8u : 0u);
    memory.put(definition + 4u, 0x39b7ffffu);
    memory.put(birth::tag_instances, tag_data + 16u);
    constexpr std::array groups{birth::standard_group, birth::alternate_group, 0xdeadbeefu};
    memory.put(tag_data, groups[(index >> 2u) % groups.size()]);
    memory.put(birth::attachment_array, attachments);
    memory.put(attachments + 0x44u, attachment_data);
    memory.put(birth::notification_context, notify_data);
    memory.put(notify_word, 0u);
    for (unsigned i = 0; i < interfaces.size(); ++i) {
        const auto table = vtables + 0x100u * i;
        memory.put(interfaces[i], table);
        for (const auto offset : methods) memory.put(table + offset, method_target(i, offset));
    }
    return test;
}

void mark_child(WorldState &state, std::uint32_t target, std::uint32_t site = 0u) {
    const auto saved_return = r(state.memory, state.gp[esp], 4);
    log_call(mix(mix(entry_hash(state, target), site), saved_return));
    state.instruction_literal(target);
}

void finish_child(WorldState &state, unsigned args, unsigned control) {
    if (policy >= 48u && (policy >> 4u) % 19u == control) {
        state.gp[edx] = state.flags.logic(state.gp[eax] ^ 0xc311u, 4);
        throw std::runtime_error("Child stopped after changing the live return state");
    }
    state.pop();
    state.gp[esp] += args;
}

void run_178070(WorldState &state) {
    mark_child(state, prefix::resolve);
    state.gp[edx] = r(state.memory, state.gp[ecx] + 4u, 4);
    state.gp[eax] = definition;
    finish_child(state, 0u, 4u);
}

void run_176a40(WorldState &state) {
    mark_child(state, placement::prepare_inputs);
    const auto cache = r(state.memory, state.gp[esp] + 20u, 4) + placement::inputs;
    const auto mask = r(state.memory, state.gp[esp] + 4u, 4);
    state.gp[eax] = state.flags.logic(r(state.memory, cache, 4) | mask, 4);
    w(state.memory, state.observe, placement::prepare_inputs, cache, 4, state.gp[eax]);
    finish_child(state, 20u, 5u);
}

void run_252480(WorldState &state) {
    mark_child(state, placement::evaluate_properties);
    state.gp[eax] = r(state.memory, state.gp[esp] + 4u, 4);
    constexpr std::array lifetimes{0x3f800000u, 0x00000000u, 0x3a83126fu, 0x7fc02468u, 0xbf800000u, 0x7f800000u};
    const auto value = lifetimes[(policy >> 4u) % lifetimes.size()];
    w(state.memory, state.observe, placement::evaluate_properties,
      state.gp[edi] + offsetof(birth::ParticleDatum, inverse_lifetime), 4, value);
    finish_child(state, 8u, 6u);
}

void run_145b70(WorldState &state) {
    mark_child(state, placement::transform_vector);
    const auto source = state.gp[ecx], output = state.gp[eax];
    const std::array words{r(state.memory, source + 8u, 4), r(state.memory, source, 4), r(state.memory, source + 4u, 4)};
    for (unsigned i = 0u; i < words.size(); ++i)
        w(state.memory, state.observe, placement::transform_vector, output + 4u * i, 4, words[i]);
    finish_child(state, 0u, 7u);
}

void run_13a6b0(WorldState &state) {
    mark_child(state, birth::enum_attachments);
    state.gp[eax] = attachment_owner;
    finish_child(state, 0u, 8u);
}

void run_176e70(WorldState &state) {
    mark_child(state, birth::create_attachment);
    state.gp[eax] = (policy & 0x20u) ? birth::invalid : attachment_handle;
    finish_child(state, 16u, 9u);
}

void run_177f10(WorldState &state) {
    mark_child(state, birth::assign_attachment);
    const auto transform = r(state.memory, state.gp[esp] + 4u, 4);
    state.gp[edx] = r(state.memory, transform, 4);
    w(state.memory, state.observe, birth::assign_attachment, state.gp[esi] + 0x40u, 4, state.gp[edx]);
    finish_child(state, 4u, 10u);
}

void run_250170(WorldState &state) {
    mark_child(state, birth::notify);
    const auto value = r(state.memory, state.gp[esp] + 8u, 4);
    w(state.memory, state.observe, birth::notify, notify_word, 4, value ^ state.gp[edx]);
    finish_child(state, 8u, 11u);
}

void dispatch(WorldState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    if (tail) throw std::runtime_error("Unexpected particle birth tail-dispatch");
    mark_child(state, target, site);
    auto &gp = state.gp;
    if (site == 0x250cb0u && target == prefix::allocate) {
        gp[eax] = (policy % 11u == 10u) ? birth::invalid : particle_handle;
        if (gp[eax] != birth::invalid) w(state.memory, state.observe, target, particle, 2, gp[eax] >> 16u);
        finish_child(state, 0u, 12u);
        return;
    }
    if (site == 0x25103bu && target == placement::collision_query) {
        const auto result = r(state.memory, gp[esp] + 24u, 4);
        const auto start_position = r(state.memory, gp[esp] + 8u, 4);
        for (unsigned offset : {0x20u, 0x24u, 0x28u, 0x40u, 0x44u, 0x48u}) {
            const auto value = r(state.memory, start_position + offset % 12u, 4);
            w(state.memory, state.observe, target, result + offset, 4, value);
        }
        gp[eax] = (gp[eax] & 0xffffff00u) | ((policy >> 4u) & 1u);
        finish_child(state, 24u, 13u);
        return;
    }
    if (site == 0x251291u && target == birth::local_transform) {
        const auto value = r(state.memory, gp[ebx], 4);
        w(state.memory, state.observe, target, gp[ecx], 4, value);
        finish_child(state, 4u, 14u);
        return;
    }

    unsigned args{}, method{};
    switch (site) {
    case 0x251158u:
    case 0x25125bu:
        method = 0x18u;
        gp[eax] = (policy >> 3u) % 4u == 3u ? birth::invalid : (policy >> 3u) % 4u;
        break;
    case 0x251187u:
        method = 0x1cu;
        args = 4u;
        gp[eax] = gp[ecx];
        break;
    case 0x2511cbu:
        method = 0x50u;
        args = 4u;
        gp[eax] = r(state.memory, gp[esp] + 4u, 4);
        break;
    case 0x25126eu:
    case 0x2512a9u:
        method = 0x14u;
        gp[eax] = (policy & 0x40u) ? birth::invalid : 0x81929118u;
        break;
    case 0x2512b2u:
        method = 0x10u;
        gp[eax] = 0xabcd0021u;
        break;
    default:
        throw std::runtime_error("Unexpected particle birth dispatch site");
    }
    if (std::ranges::find(interfaces, gp[ecx]) == interfaces.end())
        throw std::runtime_error("Unexpected particle birth interface object");
    const auto index = unsigned(std::ranges::find(interfaces, gp[ecx]) - interfaces.begin());
    if (target != method_target(index, method))
        throw std::runtime_error("Unexpected particle birth interface target");
    finish_child(state, args, 15u);
}
} // namespace

namespace original {
using ::dispatch;
using ::run_13a6b0;
using ::run_145b70;
using ::run_176a40;
using ::run_176e70;
using ::run_177f10;
using ::run_178070;
using ::run_250170;
using ::run_252480;
#include "birth_combined.inc"
} // namespace original

namespace {
bool use_collector{};

struct CompleteCases {
    unsigned returned{}, notified{}, unresolved_tag{}, child_fault{};
};

Result run(FaultBank &bank, const Case &fixture, bool native) {
    Result result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &record) { result.stores.push_back(record); };
    BirthObserverFixture observed;
    auto &observer = use_collector ? observed.collector.observer() : stores;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t site) {
        if (native && site >= prefix::first && site <= birth::last)
            throw std::runtime_error("Combined native birth delivered a translated callback");
        result.cursor = site;
    };
    if (native && !census_mode) {
        auto &cursor = services.instruction_cursor;
        cursor.position = &result.cursor;
        cursor.service_sites = halo2::native_census_detail::service_sites;
        cursor.callback_type = &services.instruction.target_type();
        cursor.executor = std::this_thread::get_id();
        cursor.literal_service_sites = true;
        cursor.observation_only_stores = !use_collector;
    }
    const auto csr = _mm_getcsr();
    std::uint16_t control;
    asm volatile("fnstcw %0" : "=m"(control));
    WorldState state{bank, observer, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(fixture.abi);
    policy = fixture.policy;
    call_log = &result.calls;
    try {
        if (native) original::run_250c60(state);
        else original::translated_250c60(state);
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
    call_log = nullptr;
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    asm volatile("fnclex\n\tfldcw %0" :: "m"(control));
    _mm_setcsr(csr);
    return result;
}

unsigned compare(const Case &test, unsigned at, Totals &totals, CompleteCases &complete) {
    FaultBank left{test.memory, at}, right{test.memory, at};
    const auto mine = run(left, test, true), theirs = run(right, test, false);
    ++totals.compared;
    totals.stores += theirs.stores.size();
    totals.children += theirs.calls.size();
    if (!theirs.fault.empty()) ++totals.faults;
    if (mine.fault != theirs.fault || mine.stores != theirs.stores || left.accesses != right.accesses ||
        !left.same(right) || mine.calls != theirs.calls || (!census_mode && mine.cursor != theirs.cursor) ||
        !same_abi(mine.abi, theirs.abi) || mine.abi.fs_base != theirs.abi.fs_base || mine.mxcsr != theirs.mxcsr) {
        std::fprintf(stderr, "Case %u access %u, native '%s' translated '%s', PC %x/%x, calls %zu/%zu, accesses %zu/%zu\n",
                     test.policy, at, mine.fault.c_str(), theirs.fault.c_str(), mine.cursor, theirs.cursor,
                     mine.calls.size(), theirs.calls.size(), left.accesses.size(), right.accesses.size());
        for (unsigned i = 0; i < 8; ++i) {
            if (mine.abi.gp[i] != theirs.abi.gp[i]) std::fprintf(stderr, "GP%u %x/%x\n", i, mine.abi.gp[i], theirs.abi.gp[i]);
            if (mine.abi.xmm[i] != theirs.abi.xmm[i])
                std::fprintf(stderr, "XMM%u low %x/%x\n", i, mine.abi.xmm[i][0], theirs.abi.xmm[i][0]);
        }
        throw std::runtime_error("Combined particle birth differential mismatch");
    }
    if (!at) {
        if (theirs.fault.empty()) {
            if (theirs.abi.gp[esp] != stack + 28u)
                throw std::runtime_error("Particle birth did not consume its original caller arguments");
            ++complete.returned;
            if (right.get(notify_word)) ++complete.notified;
        } else if (theirs.fault == "Access escaped the authored fixture at 0") {
            ++complete.unresolved_tag;
        } else if (theirs.fault == "Child stopped after changing the live return state") {
            ++complete.child_fault;
        } else {
            std::fprintf(stderr, "Unexpected fault in complete case %u at %x: %s\n", test.policy,
                         theirs.cursor, theirs.fault.c_str());
            throw std::runtime_error("Particle birth fixture has an undeclared fault");
        }
    }
    return unsigned(left.accesses.size());
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x25113au);
    Totals totals;
    CompleteCases complete;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = fixture(random, index);
        const auto accesses = compare(test, 0u, totals, complete);
        if (index < 32u) {
            for (unsigned at = 1u; at <= accesses; ++at) compare(test, at, totals, complete);
        }
    }
    if (count >= 160u && (!complete.returned || !complete.notified || !complete.unresolved_tag || !complete.child_fault))
        throw std::runtime_error("Particle birth fixture missed a complete, notified, unresolved or child-fault case");
    if (!census_mode) {
        auto native_return = [](WorldState &state) {
            return birth::bridge_epilogue(state, original::native_particle_birth_epilogue_reference_250c60);
        };
        check_refusals(fixture(random, count), native_return);
        check_birth_observer_refusals(fixture(random, count), native_return);
    }
    report(count, totals, birth::summary().coverage.load(), birth::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
