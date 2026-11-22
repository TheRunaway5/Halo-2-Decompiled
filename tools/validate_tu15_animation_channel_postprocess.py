#!/usr/bin/env python3
"""Compare the factory's channel-2 post-process with unchanged i386 TU1.5."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

import validate_native_campaign_script_builtins_tu15 as reference


ROOT = Path(__file__).resolve().parents[1]
ENTRY = 0x1D63C0
MANAGER, ORIENTATION, TAGS, GRAPH, CLIP, INHERITANCE, CHILD_GRAPH = (
    reference.DATA + n * 0x200 for n in range(1, 8)
)
SOURCES = (
    'src/gameplay/objects/native_object_factory_nodebuild_tu15.cpp',
    'src/gameplay/animation/native_tu15_animation_channel_postprocess.hpp',
)

PROBE = r'''
#include "gameplay/objects/native_object_factory_nodebuild_tu15.cpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace halo2::native_object_factory_tu15 {
using Json = nlohmann::json;
struct Interrupted {};

class ProbeBank final : public SourcePhysicsContactMemory {
    struct Region {std::uint32_t base; std::vector<std::byte> bytes;};
    std::vector<Region> regions_;
    std::uint32_t stop_read_{};
    std::size_t stop_write_{};

    static std::vector<std::byte> decode(const std::string &data) {
        std::vector<std::byte> bytes;
        for (std::size_t i = 0; i < data.size(); i += 2)
            bytes.push_back(std::byte(std::stoul(data.substr(i, 2), nullptr, 16)));
        return bytes;
    }
    static std::string encode(std::span<const std::byte> bytes) {
        static constexpr char digits[] = "0123456789abcdef";
        std::string result;
        for (auto value : bytes) {
            const auto word = std::to_integer<unsigned>(value);
            result += digits[word >> 4u];
            result += digits[word & 15u];
        }
        return result;
    }
    template<class Regions>
    static auto locate(Regions &regions, std::uint32_t address, std::size_t size) {
        for (auto &region : regions)
            if (address >= region.base && std::uint64_t(address - region.base) + size <= region.bytes.size())
                return region.bytes.data() + (address - region.base);
        throw std::runtime_error("Native probe used an undeclared bank at " + std::to_string(address));
    }

public:
    Json writes = Json::array();

    explicit ProbeBank(const Json &row)
        : stop_read_(row.value("stop_read", 0u)), stop_write_(row.value("stop_write", 99u)) {
        for (const auto &bank : row.at("banks"))
            regions_.push_back({bank.at("address"), decode(bank.at("before"))});
    }
    void read(std::uint32_t address, std::span<std::byte> out) const override {
        if (address == stop_read_) throw Interrupted{};
        std::copy_n(locate(regions_, address, out.size()), out.size(), out.begin());
    }
    void write(std::uint32_t address, std::span<const std::byte> in) override {
        if (_mm_getcsr() != 0x5f84u) throw std::runtime_error("Source FP leaked into bank write");
        if (writes.size() == stop_write_) throw Interrupted{};
        auto *destination = locate(regions_, address, in.size());
        writes.push_back({address, encode({destination, in.size()}), encode(in)});
        std::copy(in.begin(), in.end(), destination);
    }
    Json banks() const {
        Json result = Json::array();
        for (const auto &region : regions_)
            result.push_back({{"address", region.base}, {"after", encode(region.bytes)}});
        return result;
    }
};

Json execute(const Json &row) {
    ProbeBank bank(row);
    const auto input = row.at("before_fp");
    NodeRebuildFp fp{input[0], input[1], input[2]};
    const auto original_host = _mm_getcsr();
    _mm_setcsr(0x5f84u);
    bool complete = true;
    try {
        Frame frame(bank, fp);
        postprocess_channel(frame, row.at("gp").at(3), row.at("matrix"), row.at("gp").at(7));
    } catch (const Interrupted &) {
        complete = false;
    }
    if (_mm_getcsr() != 0x5f84u) throw std::runtime_error("Source FP leaked into host");
    _mm_setcsr(original_host);
    return Json{{"completed", complete}, {"after_fp", {fp.control_word, fp.status, fp.mxcsr}},
                {"writes", bank.writes}, {"banks", bank.banks()}};
}
}

int main(int argc, char **argv) try {
    if (argc != 3) return 2;
    using namespace halo2::native_object_factory_tu15;
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


def make_cases(image):
    rows = []

    def add(name, *, frames=3, time=0x3F800000, weight=0x3F000000, mode=0x1F80,
            channel_flags=2, tag=0x12340001, index=0, inherited=False, requests=False,
            offset=None, alias=False):
        data = bytearray(0x1000)

        def put(address, value, width=4):
            start = address - reference.DATA
            data[start:start + width] = value.to_bytes(width, 'little')

        output = MANAGER + 0x78 if alias else ORIENTATION
        put(MANAGER + 0x6E, channel_flags, 1)
        put(MANAGER + 0x40, tag)
        put(MANAGER + 0x44, (index << 16) | (0 if inherited else 0xFFFF))
        put(MANAGER + 0x5C, time)
        put(TAGS + 0x18, GRAPH)
        put(TAGS + 0x28, CHILD_GRAPH)
        put(CHILD_GRAPH + 0x30, CLIP)
        put(GRAPH + 0x30, CLIP)
        put(GRAPH + 0x4C, 1)
        put(GRAPH + 0x50, INHERITANCE)
        put(INHERITANCE + 4, 0x56780002)
        put(CLIP + 0x14, frames & 0xFFFF, 2)
        put(CLIP + 0x30, 0xFFFFFFFF)
        for i, bits in enumerate((0xBF800001, 0x3F912345, 1)):
            put(output + 0x10 + i * 4, bits)
        for i, bits in enumerate(offset or (0x3F812345, 0xBF000006, 0x80000001)):
            put(MANAGER + 0x84 + i * 4, bits)
        stack_start = reference.SP - 64
        banks = [(reference.DATA, data), (0x4F0414, struct.pack('<I', TAGS)),
                 (0x46A0D0, image.read(0x46A0D0, 4)), (0x51DF20, bytes((requests, requests))),
                 (stack_start, bytes(96))]
        gp = [0x5A010000 + slot * 101 for slot in range(8)]
        gp[3], gp[4], gp[7] = MANAGER, reference.SP, output
        rows.append(dict(id=len(rows), name=name, entry=ENTRY, gp=gp, matrix=weight, source=0,
                         before_fp=[0x27F, 0x1800, mode], live_x87=[], strict_writes=True,
                         banks=[dict(address=p, before=bytes(raw).hex()) for p, raw in banks]))

    add('post-process disabled', channel_flags=0)
    add('no graph tag', tag=0xFFFFFFFF)
    add('no animation index', index=0xFFFF)
    add('inherited descriptor', inherited=True)
    add('native animation request owner', requests=True)
    add('orientation aliases the manager', alias=True)
    for mode in (0x1F80, 0x3F80, 0x5F80, 0x7F80, 0x9FC0, 0x1F84):
        for name, frames, time, weight in (
            ('ordinary fraction', 3, 0x3F800000, 0x3F000000),
            ('zero denominator', 0, 0x3F800000, 0x3F000000),
            ('negative denominator', -8, 0x3F800000, 0x3F000000),
            ('lower clamp', 3, 0xBF000000, 0x3F000000),
            ('upper clamp', 3, 0x41800000, 0x3F000000),
            ('negative zero', 3, 0x80000000, 0x3F000000),
            ('denormal fraction', 2, 0x00000003, 0x3F800000),
            ('NaN fraction and weight', 3, 0x7FA00123, 0xFFC00456),
            ('zero scaled by infinity', 0, 0x00000000, 0x7F800000),
        ):
            add(f'{name} [{mode:04x}]', frames=frames, time=time, weight=weight, mode=mode)
    return rows


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = reference.Xbe(reference.NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest() != reference.NEW_PIN:
        raise ValueError('Wrong original TU1.5 executable')
    reference.ROUTINES['channel post-process'] = dict(closure_stops={0x126200})
    meta, code = reference.metadata(image, {'channel post-process': ENTRY})
    caller = code.at(0x1D5897)
    assert caller.mnemonic == 'call' and caller.operands[0].imm == ENTRY
    owned = reference.prepare(image, meta, scratch)
    rows = make_cases(image)
    original = reference.physical(owned, rows)['cases']
    full_index = next(i for i, row in enumerate(rows) if row['name'] == 'ordinary fraction [1f80]')
    full = rows[full_index]
    cases = original[full_index]
    faults = []
    write_ordinal = 0
    for ordinal, store in enumerate(cases['writes']):
        if ORIENTATION + 0x10 <= store['address'] <= ORIENTATION + 0x18:
            faults.append(dict(full, id=len(rows) + len(faults), name=f'prior to root store {write_ordinal}',
                               stop_write=write_ordinal, fault=dict(phase='before_write', ordinal=ordinal)))
            write_ordinal += 1
    assert write_ordinal == 3
    read_address = ORIENTATION + 0x14
    read_ordinal = next(i for i, read in enumerate(cases['reads']) if read['address'] == read_address)
    faults.append(dict(full, id=len(rows) + len(faults), name='prior to root Y read',
                       stop_read=read_address, fault=dict(phase='before_read', ordinal=read_ordinal)))
    original += reference.physical(owned, faults)['cases']
    rows += faults

    source, binary = scratch / 'probe.cpp', scratch / 'probe'
    source.write_text(PROBE)
    subprocess.run(['c++', '-std=c++20', '-O2', '-ffunction-sections', '-fdata-sections',
                    '-Isrc', '-Iinclude', str(source), '-Wl,--gc-sections', '-o', str(binary)],
                   cwd=ROOT, check=True)
    inputs, output = scratch / 'native-input.json', scratch / 'native-output.json'
    inputs.write_text(json.dumps(rows))
    subprocess.run([str(binary), str(inputs), str(output)], check=True)
    native = json.loads(output.read_text())
    stores = 0
    for row, expected, observed in zip(rows, original, native, strict=True):
        stack_start = reference.SP - 64
        expected_writes = [[w['address'], w['before'], w['written']] for w in expected['writes']
                           if not stack_start <= w['address'] < stack_start + 96]
        expected_banks = [b for b in expected['banks'] if b['address'] != stack_start]
        observed_banks = [b for b in observed['banks'] if b['address'] != stack_start]
        assert expected['completed'] == observed['completed'], row['name']
        assert expected['after_fp'] == observed['after_fp'], (row['name'], expected['after_fp'], observed['after_fp'])
        assert expected_writes == observed['writes'], (row['name'], expected_writes, observed['writes'])
        assert expected_banks == observed_banks, row['name']
        stores += len(expected_writes)
    report = dict(passed=True, original_sha256=reference.NEW_PIN, source_entry=ENTRY,
                  actual_call_site=caller.address, cases=len(rows), ordered_nonstack_stores=stores,
                  rejected_read_and_write_prefixes=len(faults),
                  original_instructions=sum(row['original_instructions'] for row in original),
                  matching_fp_and_nonstack_bytes=True, original_instruction_patches=[],
                  supplied_original_dependencies=[], scratch=str(scratch),
                  source_sha256={p: digest(ROOT / p) for p in SOURCES},
                  tool_sha256=digest(Path(__file__)))
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
