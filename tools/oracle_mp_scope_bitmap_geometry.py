#!/usr/bin/env python3
"""Original bitmap producer through scope stretch/mirror draw requests.

Inherited video/user-projection/appearance inputs remain explicit. The CPU
producer executes unchanged; its void graphics consumer is excluded at the
actual request ABI without manufacturing bitmap readiness or GPU completion.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_EIP
from oracle_multiplayer_hud_positions import Retail, transplant, ROOT, PIN
from inspect_multiplayer_hud import HudCache
from oracle_smg_firing_1_1 import Xbe, _Code, _trace


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class GeometryTrace(Retail):
    def __init__(self):
        self.requests = []
        self.kernel_input = None
        self.kernel_quad = None
        self.executed = set()
        super().__init__()

    def hook(self, guest, address, size, user):
        self.executed.add(address)
        frame = guest.reg_read(UC_X86_REG_EBP)
        if address == 0x22b1b5:
            self.kernel_input = dict(
                anchor8_hex=bytes(guest.mem_read(frame+0x48, 8)).hex(),
                x_axis8_hex=bytes(guest.mem_read(frame, 8)).hex(),
                y_axis8_hex=bytes(guest.mem_read(frame+0x30, 8)).hex(),
                registration_point8_hex=bytes(guest.mem_read(frame+0x3c, 8)).hex(),
                texture_width4_hex=bytes(guest.mem_read(frame+0x38, 4)).hex(),
                texture_height4_hex=bytes(guest.mem_read(frame+8, 4)).hex(),
                sprite_bounds10_hex=bytes(guest.mem_read(frame+0x54, 16)).hex())
        elif address == 0x22b299:
            self.kernel_quad = bytes(guest.mem_read(frame+0xc, 32)).hex()
            return  # Deliberately continue beyond the older oracle's stop.
        elif address == 0x22a664:
            stack = guest.reg_read(UC_X86_REG_ESP)
            xy, uv = guest.reg_read(UC_X86_REG_ECX), guest.reg_read(UC_X86_REG_EAX)
            self.requests.append(dict(
                caller_return_site=f'{self.r(stack):08x}',
                positions20_hex=bytes(guest.mem_read(xy, 32)).hex(),
                sprite_bounds10_hex=bytes(guest.mem_read(uv, 16)).hex(),
                bitmap=f'{self.r(stack+4):08x}', image=self.r(stack+8),
                shader=f'{self.r(stack+12):08x}'))
            # Literal original RET 0Ch at22A858 establishes this void ABI.
            # No returned result, readiness, shader or device success is supplied.
            guest.reg_write(UC_X86_REG_EIP, self.r(stack))
            guest.reg_write(UC_X86_REG_ESP, stack+16)
            return
        super().hook(guest, address, size, user)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), 'Write an independent report'
    cache = HudCache(ROOT/'assets/disc/maps/lockout.map')
    trace = GeometryTrace()
    xbe = Xbe(ROOT/'assets/disc/default.xbe')
    code = _Code(xbe, '.text')
    assert xbe.read(0x22a858, 3) == bytes.fromhex('c20c00')
    definitions = {tag['name']:tag for tag in cache.tags.values() if tag['group']=='nhdt'}
    rows = []
    for definition, widget_name in [('ui\\hud\\masterchief','binocular_mask'),
                                    ('ui\\hud\\smg','weapon_background_right'),
                                    ('ui\\hud\\smg','crosshair')]:
        row = next(widget for widget in cache.hud(definitions[definition])['bitmap'] if widget['name']==widget_name)
        rows.append((row, transplant(cache, trace, row)))
    status, inputs = trace.alloc(bytes(0x400)), trace.alloc(bytes(16))
    constants = [dict(address=f'{address:08x}', raw4_hex=xbe.read(address,4).hex())
                 for address in (0x45dc00, 0x45dc0c)]
    cases = []
    for flags in (0, 1, 8, 9):
        video = trace.video(flags)
        for players in range(1, 5):
            mode, cells = trace.cells(players)
            for cell in cells:
                trace.g.mem_write(0x4b9dd0, struct.pack('<4h', *cell['outer']))
                trace.g.mem_write(0x4b9dd8, struct.pack('<4h', *cell['safe']))
                for row, widget in rows:
                    trace.requests = []
                    trace.kernel_input = trace.kernel_quad = None
                    slot = trace.run(0x13a690, argv=[row['anchor']])
                    trace.run(0x22aff1, argv=[cell['view'], status, widget, inputs])
                    assert trace.kernel_input is not None
                    count = len(trace.requests)
                    assert count == (4 if row['name']=='binocular_mask' else 1)
                    cases.append(dict(name=f"flags{flags}_players{players}_view{cell['view']}_{row['name']}",
                                      video_flags=flags, video=video, players=players, grid_mode=mode, **cell,
                                      authored_widget=row, original_layout_slot=slot,
                                      raw_widget_hex=bytes(cache.data[int(row['offset'],16):int(row['offset'],16)+0x64]).hex(),
                                      kernel_input=trace.kernel_input, original_initial_quad20_hex=trace.kernel_quad,
                                      original_draw_requests=trace.requests))
    # Explicit branch controls modify only the authored widget's flag word.
    # They are separate from the120 untouched actual authored-input cases.
    trace.video(0)
    mode,cells=trace.cells(1)
    cell=cells[0]
    trace.g.mem_write(0x4b9dd0,struct.pack('<4h',*cell['outer']))
    trace.g.mem_write(0x4b9dd8,struct.pack('<4h',*cell['safe']))
    row,widget=rows[0]
    for flags in range(32):
        trace.g.mem_write(widget+0x1e,struct.pack('<H',flags))
        trace.requests=[]
        trace.kernel_input=trace.kernel_quad=None
        slot=trace.run(0x13a690,argv=[row['anchor']])
        trace.run(0x22aff1,argv=[0,status,widget,inputs])
        assert len(trace.requests)==1+bool(flags&4)+bool(flags&8)+((flags&12)==12)
        cases.append(dict(name=f'explicit_flag_word_{flags:02x}',explicitly_modified_widget_flag_word=flags,
                          video_flags=0,players=1,grid_mode=mode,**cell,authored_widget=row,
                          original_layout_slot=slot,modified_guest_widget_hex=bytes(trace.g.mem_read(widget,0x64)).hex(),
                          kernel_input=trace.kernel_input,original_initial_quad20_hex=trace.kernel_quad,
                          original_draw_requests=trace.requests))
    output = args.output.parent
    output.mkdir(parents=True, exist_ok=True)
    listings = []
    for entry in (0x22aff1,0x22ae21,0x13a690,0x1396c7,0x22bcaf,0x2b460,0x1d6d0,0x22a664):
        routine = _trace(code, entry, set())
        instructions = [code.at(address) for address in sorted(routine.code)]
        target = output/'instructions'/f'{entry:08x}.asm'
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions))
        listings.append(dict(entry=f'{entry:08x}', file=str(target.relative_to(output)), sha256=sha(target),
                             rows=len(instructions), executed_rows=sum(i.address in trace.executed for i in instructions)))
    report = dict(schema=1, tool_sha256=sha(__file__), original_base_xbe_sha256=PIN,
                  cache_sha256=sha(ROOT/'assets/disc/maps/lockout.map'),
                  preserved_dependencies={name:sha(ROOT/name) for name in ['tools/oracle_multiplayer_hud_positions.py','tools/inspect_multiplayer_hud.py']},
                  source_constants=constants, cases=cases, instructions=listings,
                  boundaries=['Actual original Lockout bitmap/widget bytes are transplanted unchanged except declared pointer relocation. Selected mode, anchor, registration, original effect/rounding/UV flip/stretch/mirror CPU instructions execute.',
                              'The prior geometry fixture supplies NTSC/video flags, user-count/session/projection/appearance and zero widget inputs. No gameplay visibility/status producer or active local session is claimed.',
                              'Original22A664 void graphics consumer is excluded at each genuine CPU request. Its literal RET0Ch ABI is honored without supplying any EAX result, bitmap cache readiness, GPU/device/shader success or draw completion.',
                              'Thirty-two explicitly modified branch controls replace only the transplanted binocular widget flag word with0..31; these are not claimed to be authored asset values.',
                              'Captured four-quads/UVs establish CPU scope geometry, not final framebuffer coverage, texture sampler behavior, graphics clipping or Xbox hardware pixels.'])
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(args.output), cases=len(cases), scope_cases=sum(c['authored_widget']['name']=='binocular_mask' for c in cases),
                          first_scope=cases[0]['original_draw_requests'])), flush=True)


if __name__ == '__main__':
    main()
