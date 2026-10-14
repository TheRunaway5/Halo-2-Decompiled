#!/usr/bin/env python3
"""Authenticate campaign cooperative death, respawn and camera owners.

Complete instruction shape, internal branch topology, calls and image operands
are retained by the shared matcher. A matching prefix alone is not evidence of
equivalent behavior. No gameplay policy or native expected output is supplied.
"""
import json
import sys
from pathlib import Path
import research_mp_sandbox_update_functions as matcher

matcher.ENTRIES = (0x23fca0, 0x23fe30, 0xbbda0, 0x139f90, 0x13a010,
                   0x13a0d0, 0x13a5b0, 0x14e4c0, 0x14e510, 0x1500b0,
                   0x1509c0, 0xcd5c0, 0x1ce060)

if __name__ == '__main__':
    if '--output' not in sys.argv:
        raise SystemExit('--output required')
    matcher.main()
    path = Path(sys.argv[sys.argv.index('--output') + 1])
    report = json.loads(path.read_text())
    # Short predicates have fewer instructions than the prefix matcher's
    # minimum. Obtain their entry only from complete authenticated callers,
    # then compare the complete callee independently.
    from oracle_smg_firing_1_1 import Xbe, _Code
    from generate_physics_live_support_tu15 import complete_body
    old_path = (Path(sys.argv[sys.argv.index('--old') + 1]) if '--old' in sys.argv else
                matcher.ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    new_path = Path(sys.argv[sys.argv.index('--new') + 1]) if '--new' in sys.argv else matcher.NEW_DEFAULT
    old, new = Xbe(old_path), Xbe(new_path)
    oc, nc = _Code(old, '.text'), _Code(new, '.text')
    for row in report['routines']:
        if 'later_entry' in row:
            continue
        old_entry = int(row['disc_1_1_entry'], 16)
        calls = [c for r in report['routines']
                 if r.get('normalized_instruction_and_internal_branch_topology_equal')
                 for c in r.get('paired_direct_calls', [])
                 if int(c['old_target'], 16) == old_entry]
        entries = {int(c['later_target'], 16) for c in calls}
        if len(entries) != 1:
            continue
        later_entry = entries.pop()
        oi = [oc.at(p) for p in sorted(complete_body(oc, old_entry).code)]
        ni = [nc.at(p) for p in sorted(complete_body(nc, later_entry).code)]
        equal = matcher.full_tokens(oi, old) == matcher.full_tokens(ni, new)
        row.update(later_entry=f'{later_entry:08x}', old_instructions=len(oi),
                   later_instructions=len(ni),
                   normalized_instruction_and_internal_branch_topology_equal=equal,
                   authenticated_caller_evidence=calls)
        for label, instructions, entry in (('disc', oi, old_entry), ('later', ni, later_entry)):
            listing = path.parent / 'instructions' / f'{old_entry:08x}-{label}-{entry:08x}.asm'
            listing.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'
                                       for i in instructions))
            row[label + '_listing'] = dict(file=listing.name, sha256=matcher.sha(listing))
    report['owner_selection_tool_sha256'] = matcher.sha(__file__)
    report['instruction_matcher_tool_sha256'] = report.pop('tool_sha256')
    report['complete_authenticated_owners'] = sum(
        r.get('normalized_instruction_and_internal_branch_topology_equal', False)
        for r in report['routines'])
    report['original_instructions'] = sum(r.get('old_instructions', 0) for r in report['routines'])
    report['disc_1_1_path'] = str(old_path)
    path.write_text(json.dumps(report, indent=2) + '\n')
