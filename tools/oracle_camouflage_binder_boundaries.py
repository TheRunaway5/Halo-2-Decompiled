#!/usr/bin/env python3
"""Original color40 state consumption and bounded entry-to-boundary16B90.

Complete1AE70->1A170->131ED0->3F5CF0 compositions use actual five compiled
camouflage implementation ranges, finite primary/fog inputs, declared state75
alpha/RGB baselines and a backed command buffer. Actual FPCW is observed at
every FISTP. Fresh16B90 executions start at entry and stop before the first
genuine cache/graphics service; no instructions or returns are replaced.
Command emission is not GPU acceptance, rendering, or framebuffer parity.
"""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path

from oracle_camouflage_overlay import CamouflageOverlay
from oracle_damage_feedback import bits
from oracle_mp_spawn import Cache, CACHE_PIN
from oracle_multiplayer_hud_positions import PIN, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW
from disasm import listing
from inventory import read_rows
from xbe import FUNCTIONS_CSV

ROOT = Path(__file__).resolve().parents[1]
def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

OUT = ROOT / 'analysis/multiplayer/game-variants/camouflage-binder-20261011'
PRIOR = ROOT / 'analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json'
PRIOR_PIN = '152fca115d6c5e0f5a528873b2f95938446aa1b29b6607fc61ec07ff5d0887c2'
BOUNDARIES = (0x143c0, 0x137bd0, 0x3fac20, 0x3faae0, 0x3fa3d0, 0x3f8320)
WATCHED = (0x16b90, 0x16f60, 0x17000, 0x17170, 0x18900, 0x189a0, 0x1ae70,
           0x174d0, 0x18a90, 0x19be0, 0x19ca0, 0x1a170, 0x131ed0, 0x3f5cf0,
           0x1ab50, 0x17420, 0x18d70, 0x18e80, 0x3f7230, 0x1b0a0, 0x1cf50, 0x1b050)
OWNERS = dict(pass_binder=0x16b90, state_extern=0x1a170, rgb_pack=0x131ed0,
              immediate_state=0x3f5cf0, parameter_vertex=0x17000, shader_dispatch=0x1bbf0,
              bank_reset=0x16b10, draw_callback=0x4d0b0, texture_bank=0x18a90,
              texture_extern=0x19530, parameter_texture=0x16f60,
              animation=0x17420, vertex_upload=0x18e80, program_bind=0x1b050)


class BinderTrace(CamouflageOverlay):
    def __init__(self, cache):
        super().__init__(cache)
        self.watched = WATCHED
        self.stop_services = False
        self.boundary = None
        self.sequence = []
        self.fp_observations = []

    def hook(self, uc, address, size, data):
        if getattr(self, 'recording', False) and self.stop_services and address in BOUNDARIES:
            # The boundary instruction is not executed and is deliberately not
            # included in executed_original, unlike a normal instruction hook.
            assert bytes(uc.mem_read(address, size)) == self.image.read(address, size)
            self.boundary = address
            uc.emu_stop()
            return
        super().hook(uc, address, size, data)
        if not getattr(self, 'recording', False):
            return
        if address in self.watched:
            self.sequence.append(address)
        if address in (0x131efe, 0x131f0b, 0x131f1c):
            self.fp_observations.append(dict(address=address, fpcw=uc.reg_read(UC_X86_REG_FPCW)))


def object_context(o):
    pool = o.r(0x4e0300)
    o.w(pool + 0x24, 12)
    o.w(pool + 0x38, 1)
    o.w(o.unit + 0x14, 0xffffffff)
    o.g.mem_write(o.unit + 0xaa, b'\0')
    o.w(0x485a58, o.model_owner)


def program(o, layer, index):
    datum = 0xe7ec0667 if layer == 18 else 0xe7ef066a
    root = o.r(o.table + (datum & 65535) * 16 + 8)
    assert o.r(root + 28) == 1
    post = o.r(root + 32)
    assert index < o.r(post)
    return post, o.r(post + 4) + index * 306


def fog_context(o, fog):
    o.w(0x4b9f5c, fog['datum'])
    o.w(0x4b9f80, bits(fog['near']))
    o.w(0x4b9f84, bits(fog['far']))
    o.g.mem_write(0x4b9dac, struct.pack('<3f', 1., -.5, .25))


def command_context(o):
    command = o.alloc(bytes(0x10000))
    # Actual device identity is a declared backed input, corroborated by the
    # retained retail RAM snapshot. It supplies storage, not an API return.
    o.w(0x407488, 0x404fe0)
    o.w(0x404fe0, command)
    o.w(0x404fe4, command + 0xff00)
    return command


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    ap.add_argument('--prior',type=Path,default=PRIOR)
    ap.add_argument('--prior-sha256',default=PRIOR_PIN,
                    help='Explicit digest for a freshly regenerated original pass corpus')
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    assert sha(args.prior) == args.prior_sha256
    assert sha(ROOT / 'assets/disc/maps/lockout.map') == CACHE_PIN
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    o = BinderTrace(cache)
    object_context(o)
    bank = o.alloc(bytes(0x1800))
    command = command_context(o)
    o.recording = True
    source_rows = json.loads(args.prior.read_text())['cases']
    state_cases = []
    for index, source in enumerate(source_rows):
        post, impl = program(o, source['layer'], source['implementation'])
        o.g.mem_write(bank, bytes(0x1800))
        o.w(bank + 0x20, post)
        o.w(bank + 0x24, impl)
        masks = [o.r(impl + off) for off in (0x10c, 0x110, 0x104, 0x108, 0x114)]
        o.g.mem_write(bank + 0x44, struct.pack('<5I', *masks))
        o.g.mem_write(bank + 0x30, struct.pack('<5I', *masks))
        o.w(o.unit + 0x2b0, source['primary_bits'])
        fog_context(o, source['fog'])
        o.w(0x4b9ed8, source['local_user'])
        o.run(0x1ae70, registers=[(UC_X86_REG_EDI, bank)])
        color = list(struct.unpack('<3I', o.g.mem_read(bank + 0x108 + 40 * 12, 12)))
        assert color == source['color40_bits']
        packed = struct.unpack('<H', o.g.mem_read(impl + 0x11e, 2))[0]
        maps = [bytes(o.g.mem_read(o.r(post + 0x3c) + ((packed & 511) + n) * 4, 4)).hex()
                for n in range(packed >> 9)]
        for alpha, cw in itertools.product((0, 0x7f, 0xff), (0x027f, 0x067f, 0x0a7f, 0x0e7f)):
            initial = (alpha << 24) | 0x123456
            o.w(0x407498 + 75 * 4, initial)
            o.w(0x404fe0, command)
            o.g.mem_write(command, b'\xa5' * 32)
            o.sequence = []
            o.fp_observations = []
            o.prepare(argv=[impl + 0x11e], registers=[(UC_X86_REG_EBX, bank)])
            # Set after prepare, which otherwise resets CW. Observe it at the
            # original FISTPs instead of trusting a requested input label.
            o.g.reg_write(UC_X86_REG_FPCW, cw)
            o.g.emu_start(0x1a170, STOP, count=200000)
            assert o.g.reg_read(UC_X86_REG_EIP) == STOP
            assert all(row['fpcw'] == cw for row in o.fp_observations)
            length = o.r(0x404fe0) - command
            result = o.r(0x407498 + 75 * 4)
            assert result >> 24 == alpha
            assert len(o.fp_observations) == (3 if maps else 0)
            assert length == (8 if maps else 0)
            state_cases.append(dict(prior_case=index, layer=source['layer'], implementation=source['implementation'],
                                    primary_bits=source['primary_bits'], color40_bits=color,
                                    mappings=maps, initial_state75=initial, state75=result, requested_fpcw=cw,
                                    actual_fistp=o.fp_observations.copy(), command_hex=bytes(o.g.mem_read(command, length)).hex(),
                                    owner_sequence=[f'{address:08x}' for address in o.sequence]))

    prefix_cases = []
    prefix_traces = []
    for (layer, implementation), primary, fog_datum in itertools.product(
            ((18, 0), (18, 1), (18, 2), (18, 3), (19, 0)), (0, bits(.25), 0x31c00000), (0xffffffff, 7)):
        t = BinderTrace(cache)
        prefix_traces.append(t)
        object_context(t)
        for datum, tag in cache.tags.items():
            t.w(t.table + (datum & 65535) * 16, int.from_bytes(tag['group'].encode()[::-1], 'little'))
        current = t.alloc(bytes(0x1800))
        t.w(current + 8, 0xe9080783)  # original MC body shader, distinct from override argument
        t.g.mem_write(current + 0x30, struct.pack('<5I', *([0xffffffff] * 5)))
        t.g.mem_write(current + 0x1530, struct.pack('<128f', *([.125, .25, .5, 1.] * 32)))
        cmd = command_context(t)
        t.w(t.unit + 0x2b0, primary)
        fog = dict(datum=fog_datum, near=10., far=30.)
        fog_context(t, fog)
        t.w(0x4b9ed8, 0xffffffff)
        t.recording = True
        t.stop_services = True
        t.prepare([current, 0xe7e90664, 0, layer, 0, implementation, bits(1.)])
        t.g.emu_start(0x16b90, STOP, count=200000)
        assert t.boundary in BOUNDARIES
        assert t.g.reg_read(UC_X86_REG_EIP) == t.boundary
        post, impl = program(t, layer, implementation)
        assert t.r(current + 0x20) == post and t.r(current + 0x24) == impl
        boundary_input = {}
        if t.boundary == 0x143c0:
            sp = t.g.reg_read(UC_X86_REG_ESP)
            boundary_input = dict(bitmap_datum=t.g.reg_read(UC_X86_REG_EAX), bitmap_index=t.g.reg_read(UC_X86_REG_ECX) & 65535,
                                  stage=t.r(sp + 4))
        length = t.r(0x404fe0) - cmd
        prefix_cases.append(dict(layer=layer, implementation=implementation, primary_bits=primary, fog=fog,
                                 shader_argument=0xe7e90664, bank_shader_input=0xe9080783,
                                 initial_extern_dirty_masks=[0xffffffff] * 5, initial_vertex_hex=struct.pack('<128f', *([.125, .25, .5, 1.] * 32)).hex(),
                                 stop_before=t.boundary, boundary_input=boundary_input,
                                 owner_sequence=[f'{address:08x}' for address in t.sequence],
                                 scalar37_bits=t.r(current + 0x5c + 37 * 4),
                                 color40_bits=list(struct.unpack('<3I', t.g.mem_read(current + 0x108 + 40 * 12, 12))),
                                 command_hex=bytes(t.g.mem_read(cmd, length)).hex(),
                                 vertex_hex=bytes(t.g.mem_read(current + 0x1530, 512)).hex(),
                                 vertex_dirty_masks=[t.r(current + 0x1730), t.r(current + 0x1734)],
                                 state75=t.r(0x407498 + 75 * 4)))

    rows = read_rows(FUNCTIONS_CSV)
    owners = []
    for name, address in OWNERS.items():
        data = o.image.read(address, int(rows[address]['size']))
        path = args.output / f'{name}-{address:08x}.asm'
        path.write_text(f'{address:08x} indexed_bytes={len(data)} sha256={hashlib.sha256(data).hexdigest()}\n'
                        + '\n'.join(listing(data, address, rows)) + '\n')
        owners.append(dict(owner=name, address=address, indexed_bytes=len(data), listing=path.name, listing_sha256=sha(path)))
    executed = dict(o.executed)
    for trace in prefix_traces:
        executed.update(trace.executed)
    dependencies = [Path(__file__).resolve(), ROOT / 'tools/oracle_camouflage_overlay.py', ROOT / 'tools/oracle_shield_overlay.py',
                    ROOT / 'tools/oracle_damage_feedback.py', ROOT / 'tools/oracle_multiplayer_hud_positions.py',
                    ROOT / 'tools/oracle_camouflage_render_context.py', ROOT / 'tools/oracle_mp_spawn.py',
                    ROOT / 'assets/disc/maps/lockout.map', args.prior]
    report = dict(schema='original-camouflage-binder-boundaries-v1', boundary=__doc__, xbe_sha256=PIN, cache_sha256=CACHE_PIN,
                  state_cases=state_cases, pass_entry_prefixes=prefix_cases, owners=owners,
                  literal_state_slot0_hex=o.image.read(0x467020, 4).hex(),
                  immediate_state75_method=struct.unpack('<I', o.image.read(0x43e6f0 + 75 * 4, 4))[0],
                  dependencies={str(path.relative_to(ROOT)): sha(path) for path in dependencies},
                  instruction_patches=[], supplied_returns=[],
                  executed_original=[dict(address=address, bytes_hex=o.image.read(address, size).hex()) for address, size in sorted(executed.items())],
                  limits=['State cases compose whole1AE70 and1A170 using actual compiled ranges; FPCW is observed at all reached FISTPs.',
                          'State75 alpha/RGB and command-buffer storage are declared inputs, not proof of original frame defaults.',
                          '16B90 starts at entry, with actual override/original-body tags and declared dirty masks, vertex baseline, fog, device storage.',
                          'Every16B90 prefix stops before a real resource/device service; no ready cache/graphics result is supplied.',
                          'Command words are original CPU output, not accepted graphics commands, successful rendering, or target alias pixel parity.'])
    path = args.output / 'binder-report.json'
    path.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(state_cases=len(state_cases), actual_fistps=sum(len(row['actual_fistp']) for row in state_cases),
                          pass_entry_prefixes=len(prefix_cases), boundaries=sorted(set(row['stop_before'] for row in prefix_cases)),
                          executed_instructions=len(executed), report_sha256=sha(path)), indent=2))


if __name__ == '__main__':
    main()
