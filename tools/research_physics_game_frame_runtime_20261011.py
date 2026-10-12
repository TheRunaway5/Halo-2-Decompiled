#!/usr/bin/env python3
"""Authenticate the complete game physics-frame owner and reached CPU owners.

The frame anchor must uniquely match its complete CFG and its already paired
world-step call. Additional bodies bind only through complete paired callers.
No gameplay state or platform result is supplied by this read-only recovery.
"""
import argparse
import collections
import copy
import csv
import json
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
import research_mp_sandbox_update_functions as base
from original_physics_file_hash import StatCheckedSha256

sha = StatCheckedSha256()
PARENT = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-game-callback-runtime-20261011/correspondence-verified/complete-report.json'
PARENT_PIN = 'e4b0a21a7859c727ba3374222465e9f425c63ba70639b13dff13eca441e2ad43'
ADDED = {0x1c7660, 0x2ef630, 0x148360, 0x148740, 0x22ecb0,
         0x327866, 0x3276fc, 0x32778e, 0x327855, 0x32781c,
         0x1c7b90, 0x27fa50, 0x1c8350, 0xbc910, 0xb8250, 0xbd6a0, 0xbbe70}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    assert sha(PARENT) == PARENT_PIN
    parent = json.loads(PARENT.read_text())
    old_path = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(old_path) == PIN and sha(base.NEW_DEFAULT) == base.NEW_PIN
    old, new = Xbe(old_path), Xbe(base.NEW_DEFAULT)
    oc, nc = _Code(old, '.text'), _Code(new, '.text')
    old_starts = {int(r['address'], 16) for r in csv.DictReader(
        (ROOT / 'analysis/map-pack-update/functions.tsv').open(), delimiter='\t')}
    mapping = {int(p, 16): q for p, q in parent['entries'].items()}
    assert not ADDED & mapping.keys()
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    md.skipdata = True
    section = new.section('.text')
    linear = list(md.disasm(new.section_bytes(section), section.va))
    starts = {i.operands[0].imm for i in linear if i.id and i.mnemonic == 'call'
              and i.operands[0].type == 2 and nc.inside(i.operands[0].imm)}
    starts |= set(mapping.values())

    def bodies(p, q):
        left = _trace(oc, p, old_starts | set(mapping) | ADDED)
        right = _trace(nc, q, starts | set(mapping.values()) | {q})
        oi = [oc.at(x) for x in sorted(left.code)]
        ni = [nc.at(x) for x in sorted(right.code)]
        assert base.full_tokens(oi, old) == base.full_tokens(ni, new), (hex(p), hex(q))
        return left, right, oi, ni

    # A prefix only locates candidates. Require full CFG equality and the
    # authenticated actual317650 callee before accepting the frame anchor.
    p = 0x1c7660
    oi = [oc.at(x) for x in sorted(_trace(oc, p, old_starts | ADDED).code)]
    prefix = [base.norm(i, old) for i in oi[:8]]
    tokens = [base.norm(i, new) if i.id else '' for i in linear]
    matches = []
    for n in range(len(linear)-len(prefix)):
        if tokens[n:n+len(prefix)] != prefix:
            continue
        q = linear[n].address
        try:
            of, nf, left, right = bodies(p, q)
        except Exception:
            continue
        calls = [(x, y) for x, y in zip(left, right)
                 if x.mnemonic == 'call' and x.operands[0].type == 2
                 and x.operands[0].imm == 0x317650]
        if len(calls) == 1 and calls[0][1].operands[0].imm == mapping[0x317650]:
            matches.append(q)
    assert len(matches) == 1, matches
    mapping[p] = matches[0]
    anchors = [dict(owner=p, later=mapping[p], complete_body_unique=True,
                    required_paired_callee=0x317650, later_callee=mapping[0x317650])]
    while not ADDED <= mapping.keys():
        count = len(mapping)
        for p, q in list(mapping.items()):
            of, nf, oi, ni = bodies(p, q)
            for x, y in zip(oi, ni):
                if x.mnemonic not in ('call', 'jmp') or x.operands[0].type != 2:
                    continue
                target, later = x.operands[0].imm, y.operands[0].imm
                if target not in ADDED:
                    continue
                assert target not in mapping or mapping[target] == later
                if target not in mapping:
                    mapping[target] = later
                    anchors.append(dict(owner=p, site=x.address, later_site=y.address,
                        target=target, later_target=later, transfer=x.mnemonic))
        assert len(mapping) > count, sorted(ADDED-mapping.keys())

    a.output.mkdir(parents=True, exist_ok=False)
    directory = a.output / 'instructions'
    directory.mkdir()
    result = copy.deepcopy(parent)
    # Authenticate every retained original listing against both actual images.
    for row in parent['routines']:
        for label, image in (('disc', old), ('later', new)):
            listing = row[label+'_listing']
            path = PARENT.parent / 'instructions' / listing['file']
            assert sha(path) == listing['sha256']
            for line in path.read_text().splitlines():
                address, encoded, *_ = line.split()
                raw = bytes.fromhex(encoded)
                assert image.read(int(address, 16), len(raw)) == raw
            (directory / listing['file']).write_bytes(path.read_bytes())
    mnemonics = collections.Counter(parent['mnemonics'])

    def bind(key, p, q):
        name = f'{p & 0xffffffff:08x}'
        assert name not in result[key] or result[key][name] == (q & 0xffffffff), (key, name)
        result[key][name] = q & 0xffffffff

    for p in sorted(ADDED):
        q = mapping[p]
        of, nf, oi, ni = bodies(p, q)
        assert not of.tables and not nf.tables, 'New dispatch tables need complete evidence'
        bind('entries', p, q)
        row = dict(disc_1_1_entry=f'{p:08x}', later_entry=f'{q:08x}',
                   instructions=len(oi), complete_paired_body=True, tables=[])
        for x, y in zip(oi, ni):
            bind('instruction_map', x.address, y.address)
            mnemonics[x.mnemonic] += 1
            assert len(x.operands) == len(y.operands)
            for u, v in zip(x.operands, y.operands):
                assert u.type == v.type and u.size == v.size
                if u.type == 2:
                    left, right = u.imm, v.imm
                elif u.type == 3:
                    left, right = u.mem.disp, v.mem.disp
                else:
                    continue
                if base.containing(old, left) and base.containing(new, right):
                    bind('address_map', left, right)
            if x.mnemonic in ('call', 'jmp') and x.operands[0].type == 2:
                target, later = x.operands[0].imm, y.operands[0].imm
                if target in mapping:
                    assert mapping[target] == later
                elif x.mnemonic == 'call' or target not in of.code:
                    key = 'outside_direct_calls' if x.mnemonic == 'call' else 'outside_tail_jumps'
                    result[key].setdefault(f'{target:08x}', []).append(dict(
                        site=f'{x.address:08x}', later_target=f'{later:08x}', owner=f'{p:08x}'))
            elif x.mnemonic == 'call':
                result['indirect_calls'].append(dict(site=f'{x.address:08x}',
                    owner=f'{p:08x}', operand=x.op_str))
        for label, ins in (('disc', oi), ('later', ni)):
            path = directory / f'{p:08x}-{label}.asm'
            path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
            row[label+'_listing'] = dict(file=path.name, sha256=sha(path))
        result['routines'].append(row)
    for key in ('outside_direct_calls', 'outside_tail_jumps'):
        result[key] = {p: rows for p, rows in result[key].items() if p not in result['entries']}
    result.update(extension_tool_sha256=sha(__file__), complete_bodies=len(mapping),
        mnemonics=dict(mnemonics), parent_complete_correspondence_sha256=PARENT_PIN,
        game_frame_anchors=anchors, newly_paired_entries=[f'{p:08x}' for p in sorted(ADDED)],
        scope=__doc__)
    path = a.output / 'complete-report.json'
    path.write_text(json.dumps(result, sort_keys=True, indent=2)+'\n')
    print(json.dumps(dict(sha256=sha(path), complete_bodies=len(mapping),
        instructions=sum(mnemonics.values()), added=len(ADDED), anchors=anchors)))


if __name__ == '__main__':
    main()
