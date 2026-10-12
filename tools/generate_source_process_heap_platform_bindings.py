#!/usr/bin/env python3
"""Authenticate constructor memory/initializer callsites in both owned images."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--manifest', type=Path, required=True)
    args = ap.parse_args()
    report_path = ROOT/'analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-reserve-commit/paired-complete/report.json'
    assert sha(report_path) == 'c5d4935da54f4e053868608d46ea2588248189f9952d6aebe9e75abaa9ca1e63'
    pair = next(r for r in json.loads(report_path.read_text())['owners'] if r['old_entry'] == '002da83a')
    assert pair['full_normalized_instruction_internal_branch_topology_equal']
    paths = [report_path.parent/r['file'] for r in pair['listings']]
    for path, row in zip(paths, pair['listings']): assert sha(path) == row['sha256']
    rows = [[(int(line.split()[0],16), bytes.fromhex(line.split()[1]))
             for line in path.read_text().splitlines()] for path in paths]
    original = ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(original) == PIN and sha(NEW_DEFAULT) == NEW_PIN
    images = [Xbe(original), Xbe(NEW_DEFAULT)]
    for listing, image in zip(rows, images):
        for site, raw in listing: assert image.read(site,len(raw)) == raw
    declarations = ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h'
    assert sha(declarations) == '988e679c3e042bb9f0c3836a77d3aee24117e83683da9cf839ba2a84c7fe32fb'
    assert re.search(r'XBAPI VOID NTAPI RtlInitializeCriticalSection\s*\(\s*IN PRTL_CRITICAL_SECTION CriticalSection\s*\);', declarations.read_text())
    mapping = {p:q for (p,_),(q,_) in zip(*rows)}
    codes = [_Code(image,'.text') for image in images]
    bindings = []
    for p, target, cleanup in ((0x2daa75,0x800000b8,24),(0x2daaae,0x800000b8,24),
                               (0x2dabb6,0x80000123,8)):
        q = mapping[p]; ins = [codes[0].at(p), codes[1].at(q)]
        for instruction, image in zip(ins, images):
            assert instruction.mnemonic == 'call' and instruction.operands[0].type == 3
            m = instruction.operands[0].mem; assert not m.base and not m.index
            assert int.from_bytes(image.read(m.disp,4),'little') == target
        assert mapping[p+ins[0].size] == q+ins[1].size
        bindings.append(dict(instruction=p, target=target, return11=p+ins[0].size,
            return15=q+ins[1].size, import11=ins[0].operands[0].mem.disp,
            import15=ins[1].operands[0].mem.disp, stack_bytes=cleanup,
            bytes11=ins[0].bytes.hex(), bytes15=ins[1].bytes.hex()))
    def binding(row): return '{'+','.join(f'0x{row[k]:x}u' for k in ('instruction','target','return11','return15'))+'}'
    header = '''// Generated from byte-authenticated complete original constructor listings.
// Native resource/void ABI; original Xbox kernel clobbers remain unproved.
#pragma once
#include "physics_platform_call_bindings.hpp"
namespace halo2::physics_platform_call_detail {
inline constexpr std::array<Binding,2> heap_memory_bindings{{
'''+',\n'.join('    '+binding(row) for row in bindings[:2])+'''
}};
inline constexpr Binding heap_initializer_binding'''+binding(bindings[2])+''';
} // namespace halo2::physics_platform_call_detail
'''
    args.output.parent.mkdir(parents=True,exist_ok=True); args.output.write_text(header)
    manifest = dict(schema=1, generator_sha256=sha(Path(__file__)),
        correspondence_sha256=sha(report_path), original_sha256=PIN, later_original_sha256=NEW_PIN,
        declaration_sha256=sha(declarations), bindings=bindings, header_sha256=sha(args.output),
        original_kernel_clobbers_proven=False)
    args.manifest.parent.mkdir(parents=True,exist_ok=True)
    args.manifest.write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(bindings=len(bindings), manifest_sha256=sha(args.manifest))))
if __name__ == '__main__': main()
