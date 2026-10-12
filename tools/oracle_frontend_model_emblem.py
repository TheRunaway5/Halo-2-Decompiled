#!/usr/bin/env python3
"""Retain the original biped emblem function boundary and authored shader banks.

The model path is BCC20 -> BC970, rather than UI 22ACE7 -> 14CCF0. Execute
those original routines unchanged with a supplied object header and the actual
main-menu scenario type. Only host addresses and valid object storage are supplied.
"""
from pathlib import Path
import hashlib, json, struct, sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from oracle_multiplayer_hud_positions import Retail, PIN
from audit_rendering import Cache, file_sha
from unicorn.x86_const import UC_X86_REG_EAX
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/frontend/customization'
c = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
_, scenario = c.tag(c.scenario, 'scnr')
scenario_type = c.u16(scenario + 0x10)
assert scenario_type == 2
r = Retail()
table = r.alloc(bytes(12)); objects = r.alloc(bytes(0x50))
obj = r.alloc(bytes(0x340)); scnr = r.alloc(bytes(0x30))
r.w(objects + 0x44, table); r.w(table + 8, obj)
r.w(0x4e0300, objects); r.w(0x4e0350, scnr)
r.g.mem_write(scnr + 0x10, struct.pack('<H', scenario_type))
value = r.alloc(bytes(4)); enabled = r.alloc(bytes(4))
names = [0x0D0005FF, 0x170005FA, 0x170005FB, 0x180005FC,
         0x1D0005FD, 0x1D0005FE, 0, 0x0C0005CB]
rows = []
cases = [(f, f % 32, 0) for f in range(256)]
cases += [(26, b, 0) for b in range(256)]
cases += [(26, 9, flags) for flags in range(256)]
for foreground, background, flags in cases:
    r.g.mem_write(obj + 0x24, bytes([foreground, background, flags, 0]))
    for name in names:
        r.run(0xBCC20, argv=[0, name, value, enabled])
        supported = r.g.reg_read(UC_X86_REG_EAX) & 255
        rows.append(f'{foreground} {background} {flags} {name:08x} '
                    f'{r.r(value):08x} {supported} {r.r(enabled) & 255}')
(OUT / 'frontend-model-emblem-inputs-original.tsv').write_text(
    '# unchanged BCC20 -> BC970; pinned XBE ' + PIN + '\n'
    '# foreground background flags input result_word supported enabled\n'
    + '\n'.join(rows) + '\n')
shaders = []
for datum in [0xE24B00D6, 0xE33A018F]:
    key = c.shader(datum); _, definition = c.tag(datum, 'shad')
    banks = []
    for pp in c.block(definition + 32, 124):
        overlays = []
        for p in c.block(pp + 60, 20):
            count = c.u32(p + 12)
            address = c.pointer(c.u32(p + 16), c.magic, count)
            overlays.append(dict(input=f'{c.u32(p):08x}', range=f'{c.u32(p+4):08x}',
                period_word=f'{c.u32(p+8):08x}', curve_hex=c.data[address:address+count].hex()))
        banks.append(dict(overlays=overlays))
    shaders.append(dict(datum=key, shader=c.shaders[key], banks=banks))
programs = []
for species, datum in [('spartan', 'e24b00d6'), ('elite', 'e33a018f')]:
    path = OUT / f'original-preview-shader-banks-{species}.json'
    live = json.loads(path.read_text())
    match = [row for row in live['rows'] if row['shader'] == datum]
    assert len(match) == 1
    program = bytes.fromhex(match[0]['pixel_program_hex'])
    assert len(program) == 240
    filename = f'frontend-model-emblem-{species}-live-program.bin'
    (OUT / filename).write_bytes(program)
    appearance_source = path.name
    appearance = match[0].get('appearance_hex')
    if appearance is None:
        appearance_source = 'original-preview-render.json'
        appearances = {row['appearance_hex'] for row in
            json.loads((OUT / appearance_source).read_text())['rows']}
        assert len(appearances) == 1
        appearance = appearances.pop()
    programs.append(dict(species=species, shader=datum, source=path.name,
        program=filename, sha256=hashlib.sha256(program).hexdigest(),
        appearance=appearance, appearance_source=appearance_source))
report = dict(xbe_sha256=PIN, mainmenu_sha256=file_sha(c.path),
    method='unchanged BCC20 -> BC970 with actual mainmenu scnr+10 type',
    scenario_type=scenario_type, cases=len(rows),
    copy_chain='2B10A3 appearance+5 -> 2B102F type0 object+24',
    authored_shaders=shaders, live_programs=programs,
    limitations='Retained live program and widget appearance are not a synchronized framebuffer comparison.')
(OUT / 'frontend-model-emblem-original.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(dict(cases=len(rows), scenario_type=scenario_type,
    shaders=[s['datum'] for s in shaders], programs=len(programs))))
