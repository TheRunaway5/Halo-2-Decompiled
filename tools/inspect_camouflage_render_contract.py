#!/usr/bin/env python3
"""Preserve complete static original render owners and compiled constants.

This is source inspection, not execution of collectors, render context,
shader binding, override queues, shadows or GPU submission.
"""
import json
import struct
from oracle_camouflage_render_context import ROOT, OUT, Context, HudCache, literal, sha


def main():
    t = Context('retail', HudCache(ROOT / 'assets/disc/maps/lockout.map'))
    rows = []
    owners = dict(geometry_selector=0x4bcc0, collector=0x133ab0, draw_binder=0x16b90,
                  pixel_extern=0x19ca0, shader_animation=0x17420, ultimate_root=0xbaf80, shadow_owner=0x3d7f0)
    directory = OUT / 'render-contract'
    directory.mkdir(exist_ok=True)
    for name, entry in owners.items():
        _, instructions = literal(t.code, entry, t.starts)
        filename = f'{name}-{entry:08x}.asm'
        p = directory / filename
        p.write_text('\n'.join(f'{i.address:08x}  {i.bytes.hex():<22} {i.mnemonic} {i.op_str}' for i in instructions) + '\n')
        rows.append(dict(owner=name, entry=entry, instructions=len(instructions), listing=filename, listing_sha256=sha(p),
                         source=[dict(address=i.address, bytes_hex=i.bytes.hex(), mnemonic=i.mnemonic, operands=i.op_str) for i in instructions]))
    constants = [dict(address=at, bytes_hex=t.image.read(at, 4).hex(), value=struct.unpack('<f', t.image.read(at, 4))[0])
                 for at in (0x45dbd8, 0x45e0dc, 0x45dbc0)]
    report = dict(schema='original-camouflage-static-render-contract-v1', boundary=__doc__, image_sha256=t.spec['pin'],
                  oracle_sha256=sha(__file__), base_oracle_sha256=sha(ROOT / 'tools/oracle_camouflage_render_context.py'), owners=rows,
                  constants=constants, execution_claim=False, instruction_patches=[], supplied_returns=[],
                  limits=['Complete static CFG listings only. Existing separate source-oracle evidence is cited by the research note.',
                          'No constructed camouflage collector or successful shader-override/render/GPU execution in this corpus.'])
    p = directory / 'report.json'
    p.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(report_sha256=sha(p), owner_instruction_counts={r['owner']: r['instructions'] for r in rows}, constants=constants), indent=2))


if __name__ == '__main__':
    main()
