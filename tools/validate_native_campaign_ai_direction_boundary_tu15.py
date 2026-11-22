#!/usr/bin/env python3
"""Check 1CA560 against physical TU1.5 at entry and on real geometry returns."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

import oracle_native_campaign_ai_area_perform_tu15 as area_reference
import validate_native_campaign_script_builtins_tu15 as reference


ROOT = Path(__file__).resolve().parents[1]
ENTRY = 0x1CA560
GEOMETRY = 0x16C620
SITE = 0x1CA7A5
MANAGER = reference.DATA
ACTOR = MANAGER + 0x100
SCENARIO = MANAGER + 0xA00
DIRECTION, UP = SCENARIO + 0x180, SCENARIO + 0x1A0
STACK = reference.SP - 0x180

NATIVE = r'''
#include "campaign/ai/native_campaign_ai_behavior_129_obstacle_probe.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace halo2::direction_probe_test {
using Json = nlohmann::json;
struct Interrupted {};

std::string hex(std::span<const std::byte> bytes) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (auto value : bytes) {
        const auto word = std::to_integer<unsigned>(value);
        result += digits[word >> 4u];
        result += digits[word & 15u];
    }
    return result;
}

class ProbeBank final : public SourcePhysicsContactMemory {
    struct Region {std::uint32_t base; std::vector<std::byte> bytes;};
    std::vector<Region> regions_;

    template<class Regions>
    static auto locate(Regions &regions, std::uint32_t address, std::size_t size) {
        for (auto &region : regions)
            if (address >= region.base && std::uint64_t(address - region.base) + size <= region.bytes.size())
                return region.bytes.data() + (address - region.base);
        throw std::runtime_error("Direction probe used an undeclared bank at " + std::to_string(address));
    }

public:
    explicit ProbeBank(const Json &row) {
        for (const auto &bank : row.at("banks")) {
            const auto data = bank.at("before").get<std::string>();
            Region region{bank.at("address"), {}};
            for (std::size_t i = 0; i < data.size(); i += 2)
                region.bytes.push_back(std::byte(std::stoul(data.substr(i, 2), nullptr, 16)));
            regions_.push_back(std::move(region));
        }
    }
    void read(std::uint32_t address, std::span<std::byte> out) const override {
        std::copy_n(locate(regions_, address, out.size()), out.size(), out.begin());
    }
    void write(std::uint32_t address, std::span<const std::byte> in) override {
        if (_mm_getcsr() != 0x5f84u) throw std::runtime_error("Source FP leaked into bank write");
        std::copy(in.begin(), in.end(), locate(regions_, address, in.size()));
    }
    Json banks() const {
        Json result = Json::array();
        for (const auto &region : regions_)
            result.push_back({{"address", region.base}, {"after", hex(region.bytes)}});
        return result;
    }
};

Json execute(const Json &row) {
    ProbeBank bank(row);
    const auto input = row.at("before_fp");
    NativeTu15PhysicsFrameFp fp{input[0], input[1], input[2]};
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp = row.at("gp");
    abi.xmm = row.at("xmm");
    abi.eflags = 0x202u;
    abi.floating_point = SourcePhysicsFactoryFloatingPoint{fp.control_word, fp.mxcsr};
    abi.x87_status = fp.status;
    NativeCampaignAiPerformServicesTu15 services;
    std::uint32_t called_site{}, called_entry{};
    std::size_t child_index{};
    services.behavior_direction_geometry = [&](auto site, auto entry, auto &state) {
        called_site = site;
        called_entry = entry;
        if (!row.contains("geometry")) throw Interrupted{};
        const auto &child = row.at("geometry").at(child_index++);
        const Json child_fp{state.floating_point->x87_control_word, state.x87_status, state.floating_point->mxcsr};
        if (site != child.at("site") || entry != child.at("entry") || Json(state.gp) != child.at("input_gp") ||
            Json(state.xmm) != child.at("input_xmm") || child_fp != child.at("input_fp"))
            throw std::runtime_error("Direction probe reached a different geometry input at " + std::to_string(site));
        for (const auto &delta : child.at("output_deltas")) {
            const auto data = delta.at("bytes").get<std::string>();
            const auto address = delta.at("address").get<std::uint32_t>();
            std::vector<std::byte> bytes;
            for (std::size_t i = 0; i < data.size(); i += 2)
                bytes.push_back(std::byte(std::stoul(data.substr(i, 2), nullptr, 16)));
            bank.write(address, bytes);
        }
        state.gp = child.at("output_gp");
        state.xmm = child.at("output_xmm");
        state.eflags = child.at("output_flags");
        const auto &output_fp = child.at("output_fp");
        state.floating_point = SourcePhysicsFactoryFloatingPoint{output_fp[0], output_fp[2]};
        state.x87_status = output_fp[1];
    };
    const auto original_host = _mm_getcsr();
    Json stores = Json::array();
    const SourcePhysicsContactRecordObserver observe = [&](const auto &store) {
        stores.push_back({store.instruction, store.address, hex(store.before), hex(store.written)});
    };
    _mm_setcsr(0x5f84u);
    try {
        behavior_129_direction::ObstacleProbe(bank, abi, fp, services, observe).run();
        if (!row.contains("geometry") || child_index != row.at("geometry").size())
            throw std::runtime_error("Native probe skipped its geometry owner");
    } catch (const Interrupted &) {}
    if (_mm_getcsr() != 0x5f84u) throw std::runtime_error("Source FP leaked into host");
    _mm_setcsr(original_host);
    return Json{{"called_site", called_site}, {"called_entry", called_entry}, {"gp", abi.gp},
                {"xmm", abi.xmm}, {"after_fp", {fp.control_word, fp.status, fp.mxcsr}},
                {"writes", stores}, {"banks", bank.banks()}};
}
}

// The selected direct-prefix cohort initializes the actor with no sector.
// Reaching 2762A0 in this cohort is an error.
namespace halo2 {
NativeCampaignAiPerformRequiredOwnerTu15::NativeCampaignAiPerformRequiredOwnerTu15(
    std::uint32_t site, std::uint32_t entry) : std::runtime_error("Unexpected missing owner"),
    site_(site), entry_(entry) {}
void native_campaign_ai_route_traversal_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t, SourcePhysicsWorldRuntimeAbi &, const NativeCampaignAiPerformServicesTu15 &,
    const SourcePhysicsContactRecordObserver &) {
    throw std::runtime_error("Direct-prefix fixture has no initial navigation sector");
}
}

int main(int argc, char **argv) try {
    if (argc != 3) return 2;
    using namespace halo2::direction_probe_test;
    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);
    Json outcomes = Json::array();
    for (const auto &row : Json::parse(input)) outcomes.push_back(execute(row));
    output << outcomes;
    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
}
'''


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def inputs(image):
    rows = []
    for mode in (0x1F80, 0x3F80, 0x5F80, 0x7F80, 0x9FC0, 0x1F84):
        for direction in ((bits(1), 0, 0), (bits(-1.25), bits(0.625), bits(0.125)),
                          (0x00000003, 0x80000001, 0), (0x7FA00001, 0x7F800000, 0x80000000)):
            bank = bytearray(0xC00)

            def put(address, value):
                at = address - MANAGER
                bank[at:at + 4] = struct.pack('<I', value)

            put(MANAGER + 0x44, ACTOR)
            put(ACTOR + 0x18, 0x87650017)
            put(ACTOR + 0x238, bits(1.25))
            put(ACTOR + 0x23C, bits(-2.5))
            put(ACTOR + 0x240, bits(3.25))
            put(ACTOR + 0x28C, 0xFFFFFFFF)
            put(UP + 8, bits(1))
            for index, value in enumerate(direction):
                put(DIRECTION + 4 * index, value)
            stack = bytearray([0xA5]) * 0x1C0
            struct.pack_into('<4I', stack, reference.SP - STACK,
                             reference.STOP, 0x93510000, DIRECTION, 0)
            banks = [(MANAGER, bank), (0x5020C8, struct.pack('<I', MANAGER)),
                     (0x4ECC18, struct.pack('<I', SCENARIO)), (0x474D10, struct.pack('<I', UP)),
                     (0x46A000, image.read(0x46A000, 0x1000)), (STACK, stack)]
            gp = [0x5A010000 + k * 0x11111 for k in range(8)]
            gp[4] = reference.SP
            rows.append(dict(id=len(rows), name=f'fp{mode:x}-dir{direction}', entry=ENTRY,
                             gp=gp, xmm=[[0x6B000000 + i * 4 + k for k in range(4)] for i in range(8)],
                             matrix=0x93510000, source=DIRECTION, stack_arguments=[0],
                             before_fp=[0x27F, 0x1800, mode], live_x87=[], strict_writes=True,
                             stop_before_entries=[GEOMETRY],
                             banks=[dict(address=addr, before=bytes(data).hex()) for addr, data in banks]))
    return rows


def differences(before, after):
    deltas = []
    for prior, current in zip(before, after, strict=True):
        assert prior['address'] == current['address']
        source_bytes, destination_bytes = (bytes.fromhex(bank['after']) for bank in (prior, current))
        for offset, (left, right) in enumerate(zip(source_bytes, destination_bytes, strict=True)):
            if left != right:
                deltas.append(dict(address=prior['address'] + offset, bytes=f'{right:02x}'))
    return deltas


def returning_cases(image, scratch, rows):
    # One genuine BSP node with two absent children. Authored navigation has
    # no current sector, and the fallback floor test rejects the height.
    geometry = 0x1003000
    bsp = bytearray(0x1000)
    struct.pack_into('<I', bsp, 4, geometry + 0x100)
    struct.pack_into('<I', bsp, 12, geometry + 0x200)
    bsp[0x100:0x108] = bytes.fromhex('0000ffffffffffff')
    struct.pack_into('<4f', bsp, 0x200, 0, 0, 1, 0)
    pointers = [(0x4ECC10, geometry), (0x4F9B50, geometry + 0x800),
                (0x4ECC30, geometry + 0xC00), (0x4ECD10, geometry + 0xE00),
                (0x4F0580, geometry + 0xF00)]
    cases = []
    for base in rows[::4]:
        row = dict(base, id=len(cases), name=f'{base["name"]}-real-geometry-return')
        stack_base = reference.SP - 0x900
        stack = bytearray([0xA5]) * 0x940
        struct.pack_into('<4I', stack, reference.SP - stack_base,
                         reference.STOP, 0x93510000, DIRECTION, 0)
        row['banks'] = [bank for bank in row['banks'] if bank['address'] != STACK]
        row['banks'] += [dict(address=stack_base, before=stack.hex()),
                         dict(address=geometry, before=bsp.hex())]
        row['banks'] += [dict(address=address, before=struct.pack('<I', value).hex())
                         for address, value in pointers]
        cases.append(row)
    metadata = area_reference.metadata(image, (ENTRY, GEOMETRY, 0x276E10, 0x16B940, 0x1E68E0, 0x1E6980))
    directory = scratch / 'full-physical'
    directory.mkdir()
    owned = reference.prepare(image, metadata, directory)
    stops = (GEOMETRY, 0x1CA7AA, 0x276E10, 0x1CA7CC)
    phases = [reference.physical(owned, [dict(row, stop_before_entries=[stop]) for row in cases])['cases']
              for stop in stops]
    completed = reference.physical(owned, [dict(row, stop_before_entries=[]) for row in cases])['cases']
    for row, final, *values in zip(cases, completed, *phases, strict=True):
        assert final['stopped_at'] == reference.STOP and final['boundary_gp'][0] & 0xFF == 0
        row['geometry'] = []
        for site, entry, before, after in zip((SITE, 0x1CA7C7), (GEOMETRY, 0x276E10), values[::2], values[1::2]):
            assert before['stopped_at'] == entry and before['completed'] and after['completed']
            row['geometry'].append(dict(site=site, entry=entry, input_gp=before['boundary_gp'],
                                        input_xmm=before['boundary_xmm'], input_fp=before['after_fp'],
                                        output_gp=after['boundary_gp'], output_xmm=after['boundary_xmm'],
                                        output_fp=after['after_fp'], output_flags=after['boundary_eflags'],
                                        output_deltas=differences(before['banks'], after['banks'])))
    return cases, completed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = reference.Xbe(reference.NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == reference.NEW_PIN
    reference.ROUTINES['direction probe'] = dict(closure_stops={0x2762A0, GEOMETRY, 0x276E10, 0x202970})
    metadata, code = reference.metadata(image, {'direction probe': ENTRY})
    assert code.at(SITE).operands[0].imm == GEOMETRY
    rows = inputs(image)
    physical = reference.physical(reference.prepare(image, metadata, scratch), rows)['cases']
    returning, physical_returns = returning_cases(image, scratch, rows)
    header = ROOT / 'src/campaign/ai/native_campaign_ai_behavior_129_obstacle_probe.hpp'
    source, binary = scratch / 'prefix.cpp', scratch / 'prefix'
    source.write_text(NATIVE)
    subprocess.run(['c++', '-std=c++20', '-O2', '-Isrc', '-Iinclude', str(source), '-o', str(binary)],
                   cwd=ROOT, check=True)
    paths = scratch / 'prefix-input.json', scratch / 'prefix-output.json'
    paths[0].write_text(json.dumps(rows + returning))
    subprocess.run([str(binary), *(str(p) for p in paths)], check=True)
    native = json.loads(paths[1].read_text())
    for row, expected, observed in zip(rows, physical, native[:len(rows)], strict=True):
        label = row['name']
        assert (expected['stopped_at'], observed['called_entry'], observed['called_site']) == (GEOMETRY, GEOMETRY, SITE), label
        assert expected['boundary_gp'] == observed['gp'], (label, expected['boundary_gp'], observed['gp'])
        assert expected['boundary_xmm'] == observed['xmm'], (label, expected['boundary_xmm'], observed['xmm'])
        assert expected['after_fp'] == observed['after_fp'], (label, expected['after_fp'], observed['after_fp'])
        # The existing physical observer records explicit memory operands.
        # Complete stack-bank equality also covers all PUSH/CALL residue.
        stores = [[item['instruction'], item['address'], item['before'], item['written']]
                  for item in expected['writes']]
        sites = {item[0] for item in stores}
        assert stores == [item for item in observed['writes'] if item[0] in sites], label
        assert expected['banks'] == observed['banks'], label
    for row, expected, observed in zip(returning, physical_returns, native[len(rows):], strict=True):
        label = row['name']
        assert expected['boundary_gp'][0] & 0xFF == observed['gp'][0] & 0xFF, label
        assert expected['boundary_gp'][3:8] == observed['gp'][3:8], label
        assert expected['after_fp'] == observed['after_fp'], label
        assert expected['banks'] == observed['banks'], label
        stores = [[item['instruction'], item['address'], item['before'], item['written']]
                  for item in expected['writes'] if ENTRY <= item['instruction'] <= 0x1CA8F4]
        sites = {item[0] for item in stores}
        assert stores == [item for item in observed['writes'] if item[0] in sites], label
    report = dict(passed=True, reference_sha256=reference.NEW_PIN, cases=len(rows),
                  entry=hex(ENTRY), stop_before=hex(GEOMETRY), source_call=hex(SITE),
                  original_instructions=sum(case['original_instructions'] for case in physical),
                  ordered_stores=sum(len(case['writes']) for case in physical),
                  matched_gp_xmm_fp_and_retained_stack=True, supplied_original_outcomes=False,
                  completed_real_geometry_cases=len(returning),
                  native_return_cases_replayed_observed_geometry=True,
                  source_sha256=hashlib.sha256(header.read_bytes()).hexdigest(), scratch=str(scratch))
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
