#!/usr/bin/env python3
"""Locate complete shadow-owner bodies in the owned TU1.5 image.

Normalized instruction shape locates candidates only. Whole CFG topology,
actual direct-call operands and absolute image operands remain separate evidence.
This is not execution, live activation or matched-frame acceptance.
"""
import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, norm, full_tokens

OLD_PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
ENTRIES = (0x226d0, 0x31f50, 0x320b0, 0x32350, 0x33400, 0x335c0, 0x335e0,
           0x3d7f0, 0x3dc90, 0x3e9c0, 0x48ee0, 0x4a1e0, 0x4a7c0, 0x4ac30,
           0x4baf0, 0xc1d90, 0x165a20)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), args.output
    old_path = ROOT / 'assets/disc/default.xbe'
    assert sha(old_path) == OLD_PIN and sha(NEW_DEFAULT) == NEW_PIN
    old, new = Xbe(old_path), Xbe(NEW_DEFAULT)
    oc, nc = _Code(old, '.text'), _Code(new, '.text')
    old_starts = {int(row['address'], 16) for row in csv.DictReader(
        (ROOT / 'analysis/retail/functions.tsv').open(), delimiter='\t')}
    bodies = {}
    needles = collections.defaultdict(set)
    for entry in ENTRIES:
        trace = _trace(oc, entry, old_starts | set(ENTRIES))
        instructions = [oc.at(pc) for pc in sorted(trace.code)]
        bodies[entry] = instructions
        for width in (16, 12, 8, 4):
            if len(instructions) >= width:
                needles[width].add(tuple(norm(i, old) for i in instructions[:width]))
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.skipdata = True
    section = new.section('.text')
    linear = list(decoder.disasm(new.section_bytes(section), section.va))
    tokens = [norm(i, new) for i in linear]
    indices = {width: collections.defaultdict(list) for width in needles}
    for width, wanted in needles.items():
        for n in range(len(tokens) - width + 1):
            key = tuple(tokens[n:n + width])
            if key in wanted:
                indices[width][key].append(linear[n].address)
    mapped, rows = {}, []
    for entry, instructions in bodies.items():
        candidates, width = [], 0
        for width in (16, 12, 8, 4):
            if len(instructions) < width:
                continue
            candidates = indices[width].get(tuple(norm(i, old) for i in instructions[:width]), [])
            if len(candidates) == 1:
                break
        row = dict(earlier_disc_entry=entry, prefix_instructions=width, candidates=candidates)
        if len(candidates) == 1:
            mapped[entry] = candidates[0]
        rows.append(row)
    # Literal direct-call targets delimit complete new-image tail branches.
    new_starts = set(mapped.values())
    decoder.detail = True
    for instruction in decoder.disasm(new.section_bytes(section), section.va):
        if instruction.mnemonic == 'call' and len(instruction.operands) == 1 and instruction.operands[0].type == X86_OP_IMM:
            if nc.inside(instruction.operands[0].imm):
                new_starts.add(instruction.operands[0].imm)
    output = args.output.resolve()
    output.mkdir(parents=True)
    for row in rows:
        entry = row['earlier_disc_entry']
        if entry not in mapped:
            continue
        target = mapped[entry]
        before = bodies[entry]
        trace = _trace(nc, target, new_starts)
        after = [nc.at(pc) for pc in sorted(trace.code)]
        equal = full_tokens(before, old) == full_tokens(after, new)
        row.update(tu15_entry=target, earlier_instructions=len(before), tu15_instructions=len(after),
                   normalized_instruction_and_internal_branch_topology_equal=equal)
        for label, instructions in (('earlier', before), ('tu15', after)):
            path = output / f'{entry:08x}-{label}.asm'
            path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions))
            row[label + '_listing'] = dict(file=path.name, sha256=sha(path))
        if not equal:
            continue
        calls, references = [], []
        for a, b in zip(before, after):
            if a.mnemonic == 'call' and a.operands[0].type == X86_OP_IMM:
                calls.append(dict(earlier_site=a.address, tu15_site=b.address,
                                  earlier_target=a.operands[0].imm, tu15_target=b.operands[0].imm,
                                  mapped_target_matches=mapped.get(a.operands[0].imm) == b.operands[0].imm
                                  if a.operands[0].imm in mapped else None))
            for left, right in zip(a.operands, b.operands):
                if left.type != X86_OP_MEM or right.type != X86_OP_MEM or left.mem.base or left.mem.index:
                    continue
                p, q = left.mem.disp & 0xffffffff, right.mem.disp & 0xffffffff
                if not any(s.va <= p and p + left.size <= s.va + s.vsize for s in old.sections):
                    continue
                if not any(s.va <= q and q + right.size <= s.va + s.vsize for s in new.sections):
                    continue
                raw, changed = old.read(p, left.size), new.read(q, right.size)
                references.append(dict(earlier_address=p, tu15_address=q, size=left.size,
                                       earlier_hex=raw.hex(), tu15_hex=changed.hex(), bytes_equal=raw == changed))
        row['paired_direct_calls'] = calls
        row['paired_absolute_memory_operands'] = references
    report = dict(scope=__doc__, earlier_disc_sha256=OLD_PIN, tu15_sha256=NEW_PIN,
                  tool_sha256=sha(__file__), routines=rows, live_shadow_activation=False,
                  original_execution_verified=False, original_framebuffer_parity=False,
                  full_goal_complete=False)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(mapped=len(mapped), total=len(ENTRIES),
                         normalized_equal=sum(row.get('normalized_instruction_and_internal_branch_topology_equal', False) for row in rows),
                         output=str(output))))


if __name__ == '__main__':
    main()
