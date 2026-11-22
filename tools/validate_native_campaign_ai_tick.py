#!/usr/bin/env python3
"""Check the AI scheduler against its retained body with the same child ABI."""

import argparse
import os
from pathlib import Path
import re
import subprocess

from native_campaign_ai_tick_replacements import BODIES, INCLUDE, canonical_body, replace_campaign_ai_tick
from native_source_prefixes import authenticated_prefixes
from split_native_translation_unit import split


ROOT = Path(__file__).resolve().parents[1]
PROBE = r'''
#include "campaign/ai/native_campaign_ai_tick.hpp"
#include "campaign/ai/native_campaign_ai_behavior_candidate.hpp"
#include "campaign/ai/native_campaign_ai_behavior_search.hpp"
#include <iostream>
#include <map>
#include <tuple>
#include <vector>

namespace halo2 {
using namespace physics_world_runtime_detail;
struct Memory : SourcePhysicsContactMemory {
    std::map<std::uint32_t, std::byte> bytes;
    mutable std::vector<std::tuple<bool, std::uint32_t, std::size_t>> io;
    unsigned stop{};

    void read(std::uint32_t at, std::span<std::byte> dest) const override {
        io.emplace_back(false, at, dest.size());
        if (io.size() == stop) throw std::runtime_error("read stop");
        for (auto &byte : dest) {
            const auto it = bytes.find(at++);
            byte = it == bytes.end() ? std::byte{} : it->second;
        }
    }
    void write(std::uint32_t at, std::span<const std::byte> input) override {
        io.emplace_back(true, at, input.size());
        if (io.size() == stop) throw std::runtime_error("write stop");
        for (const auto byte : input) bytes[at++] = byte;
    }
    void input(std::uint32_t at, std::uint32_t value, unsigned width) {
        for (unsigned i = 0; i < width; ++i) bytes[at++] = std::byte(value >> (i * 8));
    }
};

std::uint32_t selected(std::uint32_t value, SourcePhysicsContactImage) { return value; }
SIGN_EXTEND_IMPL

template<unsigned Target>
void child(WorldState &state) {
    state.instruction_literal(Target);
    const auto resume = state.pop();
    state.gp[0] ^= Target ^ resume;
    state.gp[1] += Target;
    state.gp[2] -= Target;
    if constexpr (Target == 0x1d0470u || Target == 0x29d540u || Target == 0x1eb4f0u) {
        const auto owner = Target == 0x1d0470u ? 0x9000u : 0xa000u;
        w(state.memory, state.observe, Target, 0x5020a8u, 4, owner);
    }
}

constexpr auto run_1d0470 = child<0x1d0470u>;
constexpr auto run_209f40 = child<0x209f40u>;
constexpr auto run_272220 = child<0x272220u>;
constexpr auto run_1eb200 = child<0x1eb200u>;
constexpr auto run_217e00 = child<0x217e00u>;
constexpr auto run_29d540 = child<0x29d540u>;

void dispatch(WorldState &state, unsigned target, unsigned pc, bool tail) {
    if (tail) std::abort();
    if (target == 0x1eb4f0u && pc == 0x1d06d3u) {
        child<0x1eb4f0u>(state);
        return;
    }
    if ((target != 0x11111u && target != 0x22222u) || (pc != 0x1b0128u && pc != 0x1b0195u)) std::abort();
    state.instruction_literal(target);
    const auto resume = state.pop();
    const auto actor = state.pop();
    if (actor != 0x12340001u) std::abort();
    if (pc == 0x1b0128u) {
        if (resume != 0x1b012bu || state.pop() != 0x8a00u) std::abort();
        state.gp[0] = target == 0x11111u ? 30u : 0x1234ffffu;
    } else {
        if (resume != 0x1b0197u) std::abort();
        state.gp[0] = 0xcafe0002u;
    }
    state.gp[1] = 0xf0f0u;
    state.gp[2] = 0x919191u;
}

REFERENCE_IMPL

auto run(bool native, unsigned enabled, unsigned mode, unsigned requested, unsigned active,
         unsigned counter, unsigned stop) {
    Memory memory;
    memory.input(0x5020a8u, 0x8000u, 4);
    memory.input(0x4f3218u, 0x7000u, 4);
    memory.input(0x8000u, requested, 1);
    memory.input(0x8001u, enabled, 1);
    memory.input(0x8002u, active, 1);
    memory.input(0x700cu, mode, 1);
    memory.input(0x9022u, counter, 2);
    memory.input(0x5000u, 0xabfedcu, 4);
    memory.stop = stop;
    std::vector<std::uint32_t> instructions;
    std::vector<SourcePhysicsContactRecordStore> stores;
    SourcePhysicsContactRuntimeServices services;
    services.instruction = [&](auto pc) { instructions.push_back(pc); };
    const SourcePhysicsContactRecordObserver observe = [&](const auto &store) { stores.push_back(store); };
    WorldState state(memory, observe, services, SourcePhysicsContactImage::TitleUpdate15);
    state.gp = {0x11ff87, 0x22ea29, 0x348e78, 0x83f2, 0x5000, 0xf001, 0xaabb22, 0xddeff6};
    state.flags.load(0x8d5u);
    bool failed = false;
    try {
        if (native) {
            if (!native_campaign_ai_tick::bridge(state, run_1d0470,
                {run_209f40, run_272220, run_1eb200, run_217e00, run_29d540},
                [](WorldState &work) { dispatch(work, 0x1eb4f0u, 0x1d06d3u, false); })) {
                std::abort();
            }
        } else {
            run_1d0670(state);
        }
    } catch (const std::runtime_error &) {
        failed = true;
    }
    return std::tuple{memory.bytes, memory.io, state.gp, state.flags.bits(), instructions, stores, failed};
}

CANDIDATE_IMPL

struct CandidateCase {
    std::int16_t id = 17;
    std::int16_t cadence = -1;
    std::uint32_t timestamp = 10;
    std::uint32_t minimum_elapsed = 0x3e800000u; // 0.25 seconds
    std::uint32_t comparison_tick = 50;
    std::uint32_t kind = 1;
    std::uint32_t callback = 0x11111u;
    unsigned barred_primary = 0;
    unsigned barred_child = 0;
};

auto run_candidate(bool native, const CandidateCase &test, unsigned stop) {
    Memory memory;
    memory.input(0x5000u, 0x1234abcd, 4);
    memory.input(0x5004u, 0x8000u, 4); // Candidate record
    memory.input(0x5008u, 0x8100u, 4); // Selected behavior
    memory.input(0x500cu, 0x8200u, 4); // Score
    memory.input(0x5010u, 0x8300u, 4); // Comparison clock
    memory.input(0x5014u, 0x8a00u, 4); // Selector callback context
    memory.input(0x8000u, std::uint16_t(test.id), 2);
    memory.input(0x8004u, std::uint16_t(test.cadence), 2);
    memory.input(0x8008u, test.minimum_elapsed, 4);
    memory.input(0x8100u, 0xabcd, 2);
    memory.input(0x8200u, 0xdefa, 2);
    memory.input(0x8308u, test.comparison_tick, 4);
    memory.input(0x5020c8u, 0x6000u, 4);
    memory.input(0x6044u, 0xa000u, 4);
    memory.input(0x51df54u, 0xb000u, 4);
    memory.input(0xb004u, 0x3dcccccd, 4); // 0.1 seconds/tick
    memory.input(0xb008u, 50, 4);
    for (unsigned index = 0; index < 14; ++index) memory.input(0xaa70u + index * 4u, test.timestamp, 4);
    memory.input(0x47b908u, 23, 4);
    memory.input(0x4fafc0u, 0x0024u, 4);
    std::array<std::uint32_t, 5> enabled{};
    const auto descriptor = [&](unsigned id, std::uint32_t address, unsigned barred) {
        memory.input(0x47b478u + id * 4u, address, 4);
        memory.input(address + 4u, barred == 2 ? 0x0004u : 0x0024u, 4);
        memory.input(address + 8u, barred == 1 ? 23 : 22, 4);
        if (barred != 3) enabled[id / 32] |= 1u << (id % 32);
    };
    if (test.id >= 0 && test.id < 131) descriptor(test.id, 0x7000u, test.barred_primary);
    descriptor(30u, 0x7100u, test.barred_child);
    for (unsigned index = 0; index < enabled.size(); ++index)
        memory.input(0x56ae84u + index * 4u, enabled[index], 4);
    memory.input(0x7002u, test.kind, 2);
    memory.input(0x7010u, test.callback, 4);
    memory.stop = stop;

    std::vector<std::uint32_t> instructions;
    std::vector<SourcePhysicsContactRecordStore> stores;
    SourcePhysicsContactRuntimeServices services;
    services.instruction = [&](auto pc) { instructions.push_back(pc); };
    const SourcePhysicsContactRecordObserver observe = [&](const auto &store) { stores.push_back(store); };
    WorldState state(memory, observe, services, SourcePhysicsContactImage::TitleUpdate15);
    state.gp = {0x12340001u, 0x22ea29, 0x348e78, 0x83f2, 0x5000, 0xf001, 0xaabb22, 0xddeff6};
    state.xmm[0] = _mm_castsi128_ps(_mm_set_epi32(1, 2, 3, 4));
    state.flags.load(0x8d5u);
    bool failed = false;
    try {
        if (native) {
            if (!native_campaign_ai_behavior_candidate::bridge(state, dispatch)) std::abort();
        } else {
            run_1b0020(state);
        }
    } catch (const std::runtime_error &) {
        failed = true;
    }
    std::array<std::uint32_t, 4> simd{};
    std::memcpy(simd.data(), &state.xmm[0], sizeof(simd));
    return std::tuple{memory.bytes, memory.io, state.gp, state.flags.bits(), instructions, stores, failed, simd};
}

void probe_rank_child(WorldState &state) {
    const auto resume = state.pop();
    const auto record = state.pop();
    const auto behavior_out = state.pop();
    const auto score_out = state.pop();
    const auto context = state.pop();
    const auto clock = state.pop();
    if ((resume != 0x1b0264u && resume != 0x1b02f2u && resume != 0x1b032eu) ||
        state.gp[0] != 0x12340001u || context != 0x8a00u || clock != 0x8300u ||
        behavior_out != 0x4ff8u || score_out != 0x4ff4u) std::abort();
    state.instruction_literal(0x1b0020u);
    const auto id = r(state.memory, record, 2);
    auto score = r(state.memory, record + 6u, 2);
    w(state.memory, state.observe, 0x1b0037u, behavior_out, 2, id);
    w(state.memory, state.observe, 0x1b003cu, score_out, 2, score);
    state.gp[0] = 0x11fb0ff0u;
    state.gp[1] = 0xf0f0u;
    state.gp[2] = 0x919191u;
}

SEARCH_IMPL

struct SearchCase {
    std::int16_t begin = 0;
    std::int16_t end = 2;
    std::int16_t current = -1;
    bool priority = true;
    bool skip_first = false;
    bool accepts_regular = true;
    unsigned linked_variant = 1;
    std::array<std::uint16_t, 4> scores{0, 1, 2, 3};
};

auto run_search(bool native, const SearchCase &test, unsigned stop) {
    Memory memory;
    const std::array<std::uint32_t, 12> stack{0x1234abcdu, 0x12340001u, 0x8400u,
        std::uint32_t(test.begin), std::uint32_t(test.end), 0x8a00u, 0x8300u,
        std::uint32_t(test.current), unsigned(test.priority), unsigned(test.skip_first), 0x8100u, 0x8200u};
    for (std::size_t index = 0; index < stack.size(); ++index) memory.input(0x5000u + index * 4u, stack[index], 4);
    memory.input(0x47c1a4u, 0x1234ffffu, 4);
    memory.input(0x8410u, 0x7000u, 4);
    memory.input(0x8400u, 0x7000u, 4);
    memory.input(0x8404u, 0x7300u, 4);
    const std::array<std::uint32_t, 4> records{0x7000u, 0x7100u, 0x7200u, 0x7300u};
    for (std::size_t index = 0; index < records.size(); ++index) {
        memory.input(records[index], 11u + index, 2);
        memory.input(records[index] + 2u, unsigned(test.accepts_regular), 1);
        memory.input(records[index] + 6u, test.scores[index], 2);
    }
    if (test.linked_variant) {
        memory.input(0x700cu, 0x7100u, 4);
        memory.input(0x7110u, test.linked_variant == 2 ? 2 : test.linked_variant == 4 ? 1 : 0, 2);
        if (test.linked_variant != 4) {
            memory.input(0x710cu, 0x7200u, 4);
            memory.input(0x7210u, test.linked_variant == 3 ? 1 : 2, 2);
        }
    }
    memory.stop = stop;
    std::vector<std::uint32_t> instructions;
    std::vector<SourcePhysicsContactRecordStore> stores;
    SourcePhysicsContactRuntimeServices services;
    services.instruction = [&](auto pc) { instructions.push_back(pc); };
    const SourcePhysicsContactRecordObserver observe = [&](const auto &store) { stores.push_back(store); };
    WorldState state(memory, observe, services, SourcePhysicsContactImage::TitleUpdate15);
    state.gp = {0x11ff87, 0x22ea29, 0x348e78, 0x83f2, 0x5000, 0xf001, 0xaabb22, 0xddeff6};
    state.flags.load(0x8d5u);
    bool failed = false;
    try {
        if (native) {
            if (!native_campaign_ai_behavior_search::bridge(state, probe_rank_child)) std::abort();
        } else {
            run_1b01e0(state);
        }
    } catch (const std::runtime_error &) {
        failed = true;
    }
    return std::tuple{memory.bytes, memory.io, state.gp, state.flags.bits(), instructions, stores, failed};
}
} // namespace halo2

int main(int argc, char **) {
    const bool suppress_native_reports = argc == 2;
    const auto filter_reports = [&](auto &reference, unsigned first, unsigned last) {
        if (suppress_native_reports) std::erase_if(std::get<4>(reference), [&](unsigned pc) {
            return first <= pc && pc <= last;
        });
    };
    unsigned cases = 0;
    for (const auto enabled : {0u, 1u, 255u})
    for (const auto mode : {0u, 4u, 255u})
    for (const auto request : {0u, 1u})
    for (const auto active : {0u, 1u})
    for (const auto counter : {0u, 1u, 2u, 0x7fffu, 0x8000u, 0xffffu}) {
        const auto expected = halo2::run(false, enabled, mode, request, active, counter, 0);
        if (std::get<6>(expected)) return 1;
        for (unsigned stop = 0; stop <= std::get<1>(expected).size(); ++stop) {
            const auto native = halo2::run(true, enabled, mode, request, active, counter, stop);
            auto reference = stop ? halo2::run(false, enabled, mode, request, active, counter, stop) : expected;
            filter_reports(reference, 0x1d0670u, 0x1d06e1u);
            if (native != reference || std::get<6>(native) != bool(stop)) {
                std::cerr << "AI scheduler mismatch " << enabled << ' ' << mode << ' ' << request
                          << ' ' << active << ' ' << counter << ' ' << stop << '\n';
                return 1;
            }
            ++cases;
        }
    }
    std::cout << cases << " native-vs-translated scheduler cases passed\n";
    std::vector<halo2::CandidateCase> candidates;
    for (const auto id : {-1, 0, 17, 130, 131})
    for (unsigned primary = 0; primary < 4; ++primary)
        candidates.push_back({.id = std::int16_t(id), .barred_primary = primary});
    for (const auto cadence : {-3, -2, -1, 0, 13, 14})
    for (const auto tick : {49u, 50u})
    for (const auto timestamp : {0xffffffffu, 10u, 50u})
    for (const auto minimum : {0u, 0x3f800000u, 0x7fc00000u})
        candidates.push_back({.cadence = std::int16_t(cadence), .timestamp = timestamp,
                              .minimum_elapsed = minimum, .comparison_tick = tick});
    for (unsigned child = 0; child < 4; ++child)
    for (const auto callback : {0x11111u, 0x22222u})
        candidates.push_back({.cadence = 0, .kind = 0, .callback = callback, .barred_child = child});
    candidates.push_back({.callback = 0});
    std::size_t candidate_cases = 0;
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        const auto &test = candidates[index];
        const auto expected = halo2::run_candidate(false, test, 0);
        if (std::get<6>(expected)) return 1;
        for (unsigned stop = 0; stop <= std::get<1>(expected).size(); ++stop) {
            const auto native = halo2::run_candidate(true, test, stop);
            auto reference = stop ? halo2::run_candidate(false, test, stop) : expected;
            filter_reports(reference, 0x1b0020u, 0x1b01cfu);
            if (native != reference || std::get<6>(native) != bool(stop)) {
                std::cerr << "AI behavior candidate mismatch in case " << index << " at I/O " << stop << '\n';
                return 1;
            }
            ++candidate_cases;
        }
    }
    std::cout << candidate_cases << " native-vs-translated behavior candidate cases passed\n";
    std::vector<halo2::SearchCase> searches{{.begin = 2, .end = 2}, {.begin = 1, .end = 0}};
    static constexpr std::array<std::array<std::uint16_t, 4>, 8> scores{{
        {0, 0, 0, 0}, {0, 1, 2, 3}, {1, 0, 0, 0}, {2, 1, 0, 0},
        {0, 2, 0, 0}, {0, 1, 1, 0}, {1, 1, 1, 1}, {0xffffu, 1, 2, 0xffffu},
    }};
    for (const auto current : {-1, 11, 14, 22})
    for (const bool priority : {false, true})
    for (const bool skip : {false, true})
    for (const bool accepts : {false, true})
    for (unsigned kind = 0; kind < 5; ++kind)
    for (auto ranking : scores)
        searches.push_back({.current = std::int16_t(current), .priority = priority, .skip_first = skip,
                            .accepts_regular = accepts, .linked_variant = kind, .scores = ranking});
    std::size_t search_cases = 0;
    for (std::size_t index = 0; index < searches.size(); ++index) {
        const auto &test = searches[index];
        const auto expected = halo2::run_search(false, test, 0);
        if (std::get<6>(expected)) return 1;
        for (unsigned stop = 0; stop <= std::get<1>(expected).size(); ++stop) {
            const auto native = halo2::run_search(true, test, stop);
            auto reference = stop ? halo2::run_search(false, test, stop) : expected;
            filter_reports(reference, 0x1b01e0u, 0x1b0381u);
            if (native != reference || std::get<6>(native) != bool(stop)) {
                std::cerr << "AI behavior search mismatch in case " << index << " at I/O " << stop << '\n';
                return 1;
            }
            ++search_cases;
        }
    }
    std::cout << search_cases << " native-vs-translated behavior search cases passed\n";
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='c++')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    print(f'Owned scratch {scratch}', flush=True)
    source = ROOT / 'src/campaign/ai/source_ai_frame_tu15_generated.cpp'
    text = source.read_text()
    unbound = text.replace(INCLUDE, '', 1)
    for entry, pattern in BODIES.items():
        unbound = pattern.sub(lambda match: canonical_body(match[0], entry), unbound)
    if replace_campaign_ai_tick(text) != text or replace_campaign_ai_tick(unbound) != text:
        raise ValueError('AI scheduler source prefix failed reconstruction')
    evidence = authenticated_prefixes(ROOT, source, text)
    if any(entry not in evidence or 'reason' in evidence[entry] for entry in BODIES):
        raise ValueError('AI scheduler source prefix failed inventory admission')
    report = split(source, scratch / 'split', 12)
    if not report['body_bytes_unchanged']:
        raise ValueError('AI scheduler source failed production partitioning')
    unit = scratch / 'probe.cpp'
    sign_extend = re.search(r'^std::uint32_t sign_extend\(.*?\n\}', text, re.M | re.S)[0]
    search = canonical_body(BODIES['1b01e0'].search(text)[0], '1b01e0')
    unit.write_text(PROBE.replace('REFERENCE_IMPL', canonical_body(BODIES['1d0670'].search(text)[0]))
                    .replace('CANDIDATE_IMPL', canonical_body(BODIES['1b0020'].search(text)[0], '1b0020'))
                    .replace('SEARCH_IMPL', search.replace('run_1b0020(s);', 'probe_rank_child(s);'))
                    .replace('SIGN_EXTEND_IMPL', sign_extend))
    executable = scratch / 'ai-scheduler-test'
    subprocess.run([args.cxx, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off',
                    '-Iinclude', '-Isrc', str(unit), '-o', str(executable)], cwd=ROOT, check=True)
    environment = os.environ.copy()
    environment.pop('HALO2_REFERENCE_CAMPAIGN_AI_TICK', None)
    environment.pop('HALO2_REFERENCE_CAMPAIGN_AI_BEHAVIOR_CANDIDATE', None)
    environment.pop('HALO2_REFERENCE_CAMPAIGN_AI_BEHAVIOR_SEARCH', None)
    environment.pop('HALO2_CENSUS_NATIVE_BRIDGES', None)
    environment.pop('HALO2_CENSUS_COUNT_NATIVE_REPORTS', None)
    subprocess.run([str(executable)], env=environment, check=True)
    environment['HALO2_CENSUS_NATIVE_BRIDGES'] = '1'
    subprocess.run([str(executable), '--expect-native-census'], env=environment, check=True)
    environment['HALO2_CENSUS_COUNT_NATIVE_REPORTS'] = '1'
    subprocess.run([str(executable)], env=environment, check=True)
    print(f"Pinned source reconstructed and {report['body_count']} bodies partitioned")


if __name__ == '__main__':
    main()
