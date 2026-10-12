#!/usr/bin/env python3
"""Actual standard flag8 definitions and explicit button/setup controls."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct

from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import ROOT, PIN

spec = importlib.util.spec_from_file_location('original_setup', ROOT / 'tools/oracle_frontend_typewriter.py')
source = importlib.util.module_from_spec(spec)
spec.loader.exec_module(source)
retail, guest = source.retail, source.guest
cache = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
button_definition = retail.alloc(bytes(60))
rows, cases = [], []
services = {}


def service(machine, pc, size, context):
    if pc == 0x253cc8:
        services[hex(pc)] = services.get(hex(pc), 0) + 1
        retail.ret(button_definition)
    elif pc == 0x22f8c7:
        services[hex(pc)] = services.get(hex(pc), 0) + 1
        retail.ret(0)  # First pane index binding is outside text setup properties.


guest.hook_add(UC_HOOK_CODE, service)


def execute(label, raw, button, literal=''):
    source.selected_string = literal
    guest.mem_write(source.widget, bytes(0x400))
    retail.w(source.widget, 0x458940)
    buffer = source.widget + (0x70 if button else 0x74)
    retail.run(0x22f532, registers=[(UC_X86_REG_ECX, buffer)])
    if button:
        guest.mem_write(button_definition, raw)
        retail.run(0x253cff, argv=[source.widget])
    else:
        guest.mem_write(source.definition, raw)
        retail.run(0x253765, argv=[source.widget, 0, source.definition])
    flags = struct.unpack_from('<I', raw)[0]
    authored_font = struct.unpack_from('<h', raw, 10)[0]
    font, scale = retail.r(buffer + 4), retail.r(buffer + 0x20)
    shadow = not (retail.read(buffer + 0x14, '<H')[0] & 4)
    rows.append(f'S {label} {int(button)} {flags} {authored_font} 3e99999a {font} {scale:08x} {int(shadow)}')
    cases.append(dict(label=label, button=button, definition_hex=raw.hex(), literal=literal,
        definition_flags=flags, authored_font=authored_font, shared_scale_word=0x3e99999a,
        runtime_font=font, scale_word=scale, shadow=shadow))


for index, at in enumerate(source.definitions):
    execute(f'actual-screen19-text{index}', bytes(cache.data[at:at + 44]), False,
        source.localized[cache.u32(at + 36)])
actual = 0
for datum, tag in cache.tags.items():
    if tag['group'] != 'wgit':
        continue
    _, screen = cache.tag(datum, 'wgit')
    screen_id = cache.u16(screen + 4)
    localized = {}
    if cache.u32(screen + 28) != 0xffffffff:
        _, unic = cache.tag(cache.u32(screen + 28), 'unic')
        localized = dict(source.locale[cache.u16(unic + 16):cache.u16(unic + 16) + cache.u16(unic + 18)])
    for pane_index, pane in enumerate(cache.block(screen + 32, 76)):
        for index, at in enumerate(cache.block(pane + 28, 44)):
            if cache.u32(at) & 8:
                execute(f'actual-screen{screen_id}-pane{pane_index}-text{index}',
                    bytes(cache.data[at:at + 44]), False, localized.get(cache.u32(at + 36), ''))
                actual += 1
assert actual == 52
for button in (False, True):
    for flags in range(16):
        raw = bytearray(60 if button else 44)
        struct.pack_into('<I', raw, 0, flags)
        struct.pack_into('<h', raw, 10, 4)
        execute(f'explicit-{int(button)}-flags{flags}', bytes(raw), button)
fixture = ROOT / 'tests/fixtures/frontend_text_setup_original.tsv'
fixture.write_text('\n'.join(rows) + '\n')
report = dict(passed=True, executable_sha256=PIN,
    cache_sha256=hashlib.sha256(cache.data).hexdigest(),
    script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    fixture=str(fixture.relative_to(ROOT)), fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
    rows=len(rows), actual_screen19=8, actual_standard_flag8=actual, explicit_controls=32,
    explicit_services=services, cases=cases,
    scope='Unchanged253765/253CFF/22CD0A and original256-unit constructor/copy. Actualwigl bytes50ms/DB/.3. Localization and parent/definition lookup, firstpane index and font readiness supplied; no font/raster/framebuffer proof.')
(ROOT / 'analysis/frontend/settings-typewriter-original/text-setup-original.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({key: report[key] for key in ('passed', 'rows', 'actual_standard_flag8', 'fixture_sha256')}, indent=2))
cache.close()
