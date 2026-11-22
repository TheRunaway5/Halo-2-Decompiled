#!/usr/bin/env python3
"""Check native behavior 129 direction publication against unchanged i386 TU1.5."""

import argparse
from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess

import oracle_native_campaign_ai_area_perform_tu15 as reference
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


ROOT = Path(__file__).resolve().parents[1]
ENTRY = 0x1FB770
BASE = 0x1050000
POOL, ACTOR, DIRECTION, ORIGIN = (BASE + offset for offset in (0x40, 0x440, 0x1100, 0x1200))
OUTPUT = ACTOR + 0x608
PATTERNS = (0, 0x80000000, 0x3F800000, 0xBF100023, 0x7F800000, 0xFF800000,
            0x7FA23456, 0xFFC23456, 0x7FC12345, 1, 0x807FFFFF, 0x7F7FFFFF,
            0xFF7FFFFF, 0x3F000033, 0x40456789)
HEADER = 'src/campaign/ai/native_campaign_ai_behavior_129_direction_publication.hpp'

PROBE = r'''
#include "campaign/ai/native_campaign_ai_behavior_129_direction_publication.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
using Word=std::uint32_t;
struct Interrupt {};
class ProbeMemory final : public SourcePhysicsContactMemory {
    struct Extent { Word base; std::vector<std::byte> data; };
    std::vector<Extent> extents_;
    Word stop_read_;
    std::size_t stop_write_, stores_{};

    template<class Extents>
    static auto locate(Extents &extents, Word address, std::size_t size) {
        if (_mm_getcsr()!=0x5f84u) throw std::runtime_error("Source MXCSR leaked into a bank operation");
        for (auto &extent : extents)
            if (address >= extent.base && std::uint64_t(address-extent.base)+size <= extent.data.size())
                return extent.data.data()+(address-extent.base);
        throw std::out_of_range("Missing probe bank at "+std::to_string(address));
    }

public:
    static std::string hex(std::span<const std::byte> bytes) {
        const char *digits="0123456789abcdef";
        std::string result;
        for (auto byte : bytes) {
            const auto value=std::to_integer<unsigned>(byte);
            result+=digits[value>>4];
            result+=digits[value&15];
        }
        return result;
    }
    explicit ProbeMemory(const Json &row)
        : stop_read_(row.value("stop_read",0u)), stop_write_(row.value("stop_write",100u)) {
        for (const auto &bank : row.at("banks")) {
            Extent extent{bank.at("address"),{}};
            const auto &raw=bank.at("before").get_ref<const std::string &>();
            for (std::size_t i=0;i<raw.size();i+=2)
                extent.data.push_back(std::byte(std::stoul(raw.substr(i,2),nullptr,16)));
            extents_.push_back(std::move(extent));
        }
    }
    void read(Word address, std::span<std::byte> out) const override {
        if (address==stop_read_) throw Interrupt{};
        std::copy_n(locate(extents_,address,out.size()),out.size(),out.begin());
    }
    void write(Word address, std::span<const std::byte> in) override {
        if (stores_==stop_write_) throw Interrupt{};
        std::copy(in.begin(),in.end(),locate(extents_,address,in.size()));
        ++stores_;
    }
    Json banks() const {
        auto result=Json::array();
        for (const auto &extent : extents_)
            result.push_back({{"address",extent.base},{"after",hex(extent.data)}});
        return result;
    }
};

Json execute(const Json &row) {
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp=row.at("gp");
    abi.eflags=row.at("eflags");
    abi.xmm=row.at("xmm");
    const auto &input=row.at("before_fp");
    NativeTu15PhysicsFrameFp fp{input[0],input[1],input[2]};
    abi.x87_status=fp.status;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
    ProbeMemory bank(row);
    auto writes=Json::array();
    const auto host=_mm_getcsr();
    _mm_setcsr(0x5f84u);
    bool complete=true;
    try {
        native_campaign_ai_publish_probe_direction_tu15(bank,abi,fp,[&](const auto &record) {
            writes.push_back({{"instruction",record.instruction},{"address",record.address},
                              {"before",ProbeMemory::hex(record.before)},{"written",ProbeMemory::hex(record.written)}});
        });
    } catch (const Interrupt &) { complete=false; }
    if (_mm_getcsr()!=0x5f84u) throw std::runtime_error("Source MXCSR leaked into host");
    _mm_setcsr(host);
    return Json{{"completed",complete},{"boundary_gp",abi.gp},{"boundary_xmm",abi.xmm},
                {"boundary_eflags",abi.eflags},{"after_fp",{fp.control_word,fp.status,fp.mxcsr}},
                {"writes",writes},{"banks",bank.banks()}};
}

int main(int argc,char **argv) try {
    if (argc!=3) return 2;
    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);
    auto results=Json::array();
    for (const auto &row : Json::parse(input)) results.push_back(execute(row));
    output<<results;
    return 0;
} catch (const std::exception &error) {
    std::cerr<<error.what()<<'\n';
    return 1;
}
'''


def input_case(rng, index, mode, alias=0):
    bank = bytearray(rng.randbytes(0x1400))
    slot = (0, 1, 2, 0xFFFF, 0x7FFF, 0x9A02)[index % 6]

    def put(address, *values):
        struct.pack_into('<' + 'I' * len(values), bank, address - BASE, *values)

    put(POOL + 0x44, (ACTOR - slot * 0x888) & 0xFFFFFFFF)
    put(DIRECTION, *(PATTERNS[(index + k * 2) % len(PATTERNS)] for k in range(3)))
    put(ORIGIN, *(PATTERNS[(index * 3 + k * 5) % len(PATTERNS)] for k in range(3)))
    direction, origin = ((DIRECTION, ORIGIN), (OUTPUT - 4, ORIGIN), (OUTPUT + 4, ORIGIN),
                         (DIRECTION, OUTPUT), (reference.SP - 4, OUTPUT))[alias]
    gp = [0x5A010000 + r * 0x1020304 + index for r in range(8)]
    gp[0], gp[2], gp[4], gp[6] = 0xEB620000 | slot, direction, reference.SP, origin
    scalar, duration = PATTERNS[index % len(PATTERNS)], 0x9ABD0000 | index
    stack = bytearray(rng.randbytes(128))
    struct.pack_into('<III', stack, 32, reference.STOP, scalar, duration)
    return dict(id=index, name=f'direction_{index}_mode_{mode:x}_alias_{alias}', entry=ENTRY, gp=gp,
                eflags=0x6D7, matrix=scalar, source=duration, live_x87=[], before_fp=[0x27F, 0x1800, mode],
                xmm=[[0x6B000000 + index * 11 + r * 4 + n for n in range(4)] for r in range(8)],
                banks=[dict(address=p, before=bytes(data).hex()) for p, data in
                       ((BASE, bank), (0x5020C8, struct.pack('<I', POOL)), (reference.SP - 32, stack))],
                strict_writes=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest() != NEW_PIN:
        raise ValueError('Wrong original TU1.5 executable')
    caller = _Code(image, '.text').at(0x1CA8E5)
    assert caller.mnemonic == 'call' and caller.operands[0].imm == ENTRY
    meta = reference.metadata(image, (ENTRY,))
    # Capstone lists explicit operands. The observer also needs the actual
    # implicit stack accesses to compare push, pop and return faults.
    for site in meta['sites']:
        pc = site['instruction']
        if pc in (0x1FB781, 0x1FB784, 0x1FB7D2, 0x1FB7F2, 0x1FB7F3):
            push = pc in (0x1FB781, 0x1FB784)
            site['operands'].append(dict(base=4, index=-1, scale=0, displacement=0xFFFFFFFC if push else 0,
                                         bytes=4, read=not push, write=push))
    owned = reference.prepare(image, meta, scratch)
    rng = random.Random(ENTRY)
    rows = [input_case(rng, i, mode, i % 5)
            for i, mode in enumerate((0x1F80, 0x3F80, 0x5F80, 0x7F80, 0x9FC0, 0x1F84) * 8)]
    rows.append(input_case(rng, len(rows), 0x1F80))
    complete = reference.physical(owned, rows, 'complete')
    anchor = complete[-1]
    assert len(anchor['writes']) == 10
    faults = [dict(deepcopy(anchor), id=100 + i, name=f'before_write_{i}', stop_write=i,
                   fault=dict(phase='before_write', ordinal=i)) for i in range(len(anchor['writes']))]
    for address in (DIRECTION, DIRECTION + 4, ORIGIN + 8, ORIGIN + 4, ORIGIN, reference.SP):
        ordinal = next(i for i, event in enumerate(anchor['reads']) if event['address'] == address)
        faults.append(dict(deepcopy(anchor), id=200 + len(faults), name=f'before_read_{address:x}', stop_read=address,
                           fault=dict(phase='before_read', ordinal=ordinal)))
    original = complete + reference.physical(owned, faults, 'failure-prefixes')

    source, binary = scratch / 'probe.cpp', scratch / 'probe'
    source.write_text(PROBE)
    subprocess.run(['c++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Isrc', '-Iinclude',
                    str(source), '-o', str(binary)], cwd=ROOT, check=True)
    inputs, output = scratch / 'native-input.json', scratch / 'native-output.json'
    inputs.write_text(json.dumps(original))
    subprocess.run([binary, inputs, output], check=True)
    for expected, actual in zip(original, json.loads(output.read_text()), strict=True):
        for field in ('completed', 'boundary_gp', 'boundary_xmm', 'boundary_eflags', 'after_fp'):
            assert expected[field] == actual[field], (expected['name'], field, expected[field], actual[field])
        assert [{'address': b['address'], 'after': b['after']} for b in expected['banks']] == actual['banks'], expected['name']
        stores = [{k: w[k] for k in ('instruction', 'address', 'before', 'written')} for w in expected['writes']]
        assert stores == actual['writes'], (expected['name'], stores, actual['writes'])
    result = dict(passed=True, original_sha256=NEW_PIN, original_owner=meta['owners'][hex(ENTRY)],
                  actual_call_site=caller.address, complete_cases=len(complete), fault_prefixes=len(faults),
                  native_source_sha256=reference.sha(ROOT / HEADER),
                  tool_sha256=reference.sha(Path(__file__)),
                  original_instructions=sum(row['original_instructions'] for row in original),
                  ordered_stores=sum(len(row['writes']) for row in original),
                  matching_gp_xmm_eflags_fp=True, matching_retained_and_stack_bytes=True,
                  original_instruction_patches=[], supplied_original_dependencies=[], scratch=str(scratch))
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
