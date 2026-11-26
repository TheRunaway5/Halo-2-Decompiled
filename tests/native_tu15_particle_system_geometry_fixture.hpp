#pragma once

#include "native_tu15_particle_birth_observer_fixture.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace frame_misc_harness::geometry_fixture {
namespace world = halo2::native_world_leaves_detail;

inline constexpr std::uint32_t frame = 0x20008a00u, particle = 0x15002000u;
inline bool use_collector{};

struct Access {
    std::uint32_t address;
    std::size_t width;
    bool write;
    friend bool operator==(const Access &, const Access &) = default;
};
struct FaultBank : Memory {
    FaultBank(const Memory &base, unsigned fault) : Memory(base.clone()), fault(fault) {}
    unsigned fault;
    mutable std::vector<Access> accesses;

    void record(Access event) const {
        accesses.push_back(event);
        if (accesses.size() == fault) throw std::runtime_error("Injected particle geometry bank fault");
    }
    void read(std::uint32_t address, std::span<std::byte> output) const override {
        record({address, output.size(), false});
        Memory::read(address, output);
    }
    void write(std::uint32_t address, std::span<const std::byte> input) override {
        record({address, input.size(), true});
        Memory::write(address, input);
    }
};

inline Case make(std::mt19937 &random, unsigned index, std::uint32_t velocity) {
    Case test;
    random_abi(random, test, frame);
    test.abi.gp[world::ebx] = index % 11u ? particle : frame + 0x20u;
    test.policy = index;
    auto &memory = test.memory;
    for (unsigned offset = 0u; offset < 0xf0u; offset += 4u) memory.put(frame - 0x40u + offset, random());
    constexpr std::array vector_words{0u, 0x80000000u, 0x3f800000u, 0xff800000u, 0x7fc09713u, 0xffc08133u,
                                    0x7f804225u, 0x0000000fu, 0x80800000u, 0x7f7fffffu, 0x38800000u};
    for (unsigned axis = 0u; axis != 3u; ++axis) {
        const float value = float(int(random() % 4001u) - 2000) / 113.0f;
        const auto word = index % 4u == 1u ? vector_words[random() % vector_words.size()] : u(value);
        memory.put(test.abi.gp[world::ebx] + velocity + 4u * axis, word);
    }
    memory.put(0x46a0dcu, index % 7u ? 0xbf800000u : 0x7fc029ca);
    memory.put(0x46a0d0u, 0x3f800000u);
    memory.put(0x46a0ecu, 0x38d1b717u);
    return test;
}

template<class Native, class Literal, class Contains>
Result execute_geometry(FaultBank &bank, const Case &test, bool native, Native kernel, Literal source, Contains contains) {
    Result result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &store) { result.stores.push_back(store); };
    BirthObserverFixture observed;
    const auto &observer = use_collector ? observed.collector.observer() : stores;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t site) {
        if (native && contains(site))
            throw std::runtime_error("Native particle geometry delivered a translated site");
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
    state.load(test.abi);
    policy = test.policy;
    call_log = &result.calls;
    try {
        if (native) {
            if (!kernel(state)) throw std::runtime_error("Native particle geometry declined");
        } else source(state);
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

template<class Native, class Literal, class Contains>
unsigned compare_geometry(const Case &test, unsigned at, Totals &totals, Native kernel, Literal source, Contains contains) {
    FaultBank left{test.memory, at}, right{test.memory, at};
    const auto mine = execute_geometry(left, test, true, kernel, source, contains);
    const auto theirs = execute_geometry(right, test, false, kernel, source, contains);
    ++totals.compared;
    totals.children += theirs.calls.size();
    totals.stores += theirs.stores.size();
    if (!theirs.fault.empty()) ++totals.faults;
    if (mine.fault != theirs.fault || mine.stores != theirs.stores || left.accesses != right.accesses ||
        !left.same(right) || mine.calls != theirs.calls || (!census_mode && mine.cursor != theirs.cursor) ||
        !same_abi(mine.abi, theirs.abi) || mine.abi.fs_base != theirs.abi.fs_base || mine.mxcsr != theirs.mxcsr) {
        std::fprintf(stderr, "Case %u access %u, native '%s' translated '%s', PC %x/%x, ABI flags %x/%x x87 %x/%x MXCSR %x/%x\n",
            test.policy, at, mine.fault.c_str(), theirs.fault.c_str(), mine.cursor, theirs.cursor,
            mine.abi.eflags, theirs.abi.eflags, mine.abi.x87_status, theirs.abi.x87_status, mine.mxcsr, theirs.mxcsr);
        for (unsigned i = 0; i < 8; ++i) {
            if (mine.abi.gp[i] != theirs.abi.gp[i])
                std::fprintf(stderr, "GP%u %x/%x\n", i, mine.abi.gp[i], theirs.abi.gp[i]);
            if (mine.abi.xmm[i] != theirs.abi.xmm[i])
                std::fprintf(stderr, "XMM%u low %x/%x\n", i, mine.abi.xmm[i][0], theirs.abi.xmm[i][0]);
        }
        throw std::runtime_error("Particle system geometry differential mismatch");
    }
    return unsigned(right.accesses.size());
}
} // namespace frame_misc_harness::geometry_fixture
