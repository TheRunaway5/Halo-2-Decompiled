#!/usr/bin/env python3
"""Recover original map lens request owners without claiming native rendering."""
import argparse
import hashlib
import json
from pathlib import Path

from audit_rendering import Cache
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), 'Preserve earlier evidence'
    root = Path(__file__).resolve().parents[1]
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    calls = {
        0x179abe: 0x17c710, 0x179c38: 0x17c710,
        0x17cba6: 0x3e9b0, 0x17cbb3: 0x2db80,
        0x41b0a: 0x3e9b0, 0x41b13: 0x2db80,
        0x2dcb9: 0x2e210, 0x2dcc7: 0x20990,
        0x2dcf0: 0x20990, 0x2dcf7: 0x20b20,
    }
    call_rows = []
    for pc, target in calls.items():
        instruction = code.at(pc)
        assert instruction.mnemonic == 'call'
        assert instruction.operands[0].imm == target
        call_rows.append(dict(site=pc, target=target, bytes=bytes(instruction.bytes).hex()))
    entries = (0x17c710, 0x3e9b0, 0x2db80, 0x2e210, 0x20990, 0x20b20)
    bodies = []
    for entry in entries:
        body = _trace(code, entry, set(entries))
        if entry == 0x17c710:
            assert {0x17caf9, 0x17cba6, 0x17cbb3} <= set(body.code)
        rows = []
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            rows.append(dict(address=pc, bytes=bytes(instruction.bytes).hex(),
                             mnemonic=instruction.mnemonic, operands=instruction.op_str))
        bodies.append(dict(entry=entry, instructions=rows, tables=body.tables))
    inventory = json.loads((root / 'analysis/map-world-features/inventory.json').read_text())
    maps, parts, direct, seen = {}, [], [], set()
    for item in inventory['maps']:
        path = root / item['cache']
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        assert digest == item['cache_sha256']
        maps[item['map']] = digest
        cache = Cache(path)
        for placement in item['placements']:
            for attachment in placement['attachments']:
                tag = attachment['definition']
                if tag and tag['group'] == 'lens':
                    direct.append(dict(map=item['map'], definition=tag,
                                       marker=attachment['marker']))
                if not tag or tag['group'] != 'effe':
                    continue
                datum = int(tag['datum'], 16)
                _, effect = cache.tag(datum, 'effe')
                for event_index, event in enumerate(cache.block(effect + 0x14, 0x38)):
                    for part_index, part in enumerate(cache.block(event + 0x18, 0x38)):
                        if cache.u32(part + 8) != 0x6c656e73 or cache.u32(part + 0x10) == 0xffffffff:
                            continue
                        key = (item['map'], datum, event_index, part_index)
                        if key in seen:
                            continue
                        seen.add(key)
                        lens = cache.u32(part + 0x10)
                        lens_tag, lens_at = cache.tag(lens, 'lens')
                        location = cache.u16(part + 4)
                        if location >= 0x8000:
                            location -= 0x10000
                        parts.append(dict(map=item['map'], effect=tag, event=event_index,
                                          part=part_index, location=location,
                                          lens=lens, lens_name=lens_tag['name'],
                                          lens_flags=cache.u32(lens_at + 0x28)))
        cache.close()
    assert len(maps) == 21 and any(row['map'] == 'elongation' for row in parts)
    args.output.mkdir(parents=True)
    (args.output / 'bodies.json').write_text(json.dumps(bodies, indent=2) + '\n')
    for body in bodies:
        (args.output / f"{body['entry']:08x}.txt").write_text(''.join(
            f"{row['address']:08X} {row['bytes']:24} {row['mnemonic']:8} {row['operands']}\n"
            for row in body['instructions']))
    report = dict(original_sha256=NEW_PIN, maps=maps, calls=call_rows,
                  bodies=len(bodies), instructions=sum(len(body['instructions']) for body in bodies),
                  authored_lens_parts=parts, direct_lens_attachments=direct,
                  native_lens_rendering_implemented=False,
                  full_goal_complete=False,
                  boundary='Original instruction and authored metadata recovery only; no original execution, native request owner, occlusion or GPU acceptance is claimed.')
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Recovered original lens owners:', report['bodies'], 'bodies,',
          report['instructions'], 'instructions,', len(parts), 'effect parts,',
          len(direct), 'direct attachments')


if __name__ == '__main__':
    main()
