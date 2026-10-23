#!/usr/bin/env python3
"""Whole original biped camouflage selection and bounded collector execution.

Real retail Lockout Spartan/Elite/matg records, explicit backed object/header,
distance/first-person/clock-free geometry inputs drive full4BCC0 including
B9CE0/3D3E0/4C450/4C0D0. A declared composition copies the resulting camouflage
fields to accepted-model/empty-section collectors, then runs full133AB0 and
full133F70 to return. Original clone/suppression/counter writes are observed.
This does not produce a resident draw or supply a successful rendering service.
"""
import argparse
import collections
import hashlib
import itertools
import json
import struct
from pathlib import Path

from oracle_shield_overlay import OverlayOracle
from oracle_multiplayer_hud_positions import ROOT, STOP, PIN, Xbe
from oracle_mp_spawn import Cache, CACHE_PIN
from oracle_damage_feedback import bits
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDI

OUT = ROOT / 'analysis/multiplayer/game-variants/camouflage-overlay-20261011'
HANDLE = 0x12340000

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class CamouflageOverlay(OverlayOracle):
    def hook(self, uc, address, size, data):
        if not getattr(self, 'recording', False):
            return
        assert bytes(uc.mem_read(address, size)) == self.image.read(address, size), hex(address)
        self.executed[address] = size
        if address in self.watched:
            self.entries[address] += 1
        sp = uc.reg_read(UC_X86_REG_ESP)
        if address == 0x133ab0:
            at = self.r(sp + 4)
            self.collector_calls.append(dict(collector_is_root=at == self.collector, guard=self.r(sp + 12), child_argument=self.r(sp + 8),
                                             fields=self.fields(at), raw_hex=bytes(uc.mem_read(at, 0x1e4)).hex()))
        if address == 0x133f70:
            at = uc.reg_read(UC_X86_REG_EAX)
            self.subpart_calls.append(dict(collector_is_root=at == self.collector, fields=self.fields(at), child_argument=self.r(sp + 4)))

    def __init__(self, cache):
        super().__init__(cache)
        self.image = Xbe(ROOT / 'assets/disc/default.xbe')
        self.executed = {}
        self.entries = collections.Counter()
        self.watched = (0x4bcc0, 0xb9ce0, 0x3d3e0, 0x4c450, 0x4c0d0, 0x133ab0, 0x133f70, 0x2cbf0)
        matg = next(t for t in cache.tags.values() if t['group'] == 'matg')['offset']
        count, pointer = struct.unpack_from('<II', cache.data, matg + 0x108)
        assert count == 1
        self.rasterizer_pointer = pointer
        self.w(0x485a80, pointer)
        self.global_shader = self.r(pointer + 0xd0)
        self.collector = self.alloc(bytes(0x1e4))
        self.render_context = self.alloc(bytes(0x2a80))
        self.model_owner = self.alloc(struct.pack('<I', HANDLE))
        self.counter = self.alloc(bytes(4))
        self.collector_calls = []
        self.subpart_calls = []
        self.g.mem_write(self.headers, struct.pack('<HBBHHI', 0x1234, 0, 0, 0xffff, 0x400, self.unit))

    def fields(self, at):
        return {name: self.r(at + offset) for name, offset in
                (('flags', 0x170), ('shader', 0x178), ('primary_bits', 0x188), ('counter_id', 0x1c0), ('counter_kind', 0x1c4),
                 ('draw_flags', 0x1ac), ('override_shader', 0x1d0), ('scalar_discriminator', 0x1d4),
                 ('camouflage_bits', 0x1d8), ('overlay_bits', 0x1dc), ('depth_bits', 0x1e0))}

    def case(self, row):
        self.g.mem_write(self.unit, bytes(0x400))
        self.w(self.unit, row['definition'])
        self.w(self.unit + 0x14, 0xffffffff)
        self.w(self.unit + 0x10, 0xffffffff)
        self.w(self.unit + 0xf0, bits(1.))
        self.g.mem_write(self.unit + 0xb1, b'\xff')
        self.w(self.unit + 0x134, row['activation_flags'])
        self.w(self.unit + 0x2b0, row['primary_bits'])
        self.w(0x4c1bd0, HANDLE if row['first_person_owner'] else 0xffffffff)
        self.g.mem_write(0x4c1bd4, struct.pack('<h', 1))
        self.w(0x4ba01c, 5)
        self.g.mem_write(self.context, b'\xa5' * 0x1e4)
        self.g.mem_write(self.output, b'\xa5' * 8)
        original_unit = bytes(self.g.mem_read(self.unit, 0x400))
        before_counts = dict(self.entries)
        self.recording = True
        self.run(0x4bcc0, argv=[HANDLE, bits(10.), row['caller_mode'], self.output, row['force_world'], self.output + 4],
                 registers=[(UC_X86_REG_EDI, self.context)])
        assert self.g.reg_read(UC_X86_REG_EIP) == STOP
        accepted = struct.unpack('<h', struct.pack('<H', self.g.reg_read(UC_X86_REG_EAX) & 0xffff))[0]
        selector = dict(accepted_models=accepted, first_person=self.read(self.output, '<B')[0], geometry_lod=self.read(self.output + 4, '<B')[0],
                        flags=self.r(self.context + 0x16c), shader=self.r(self.context + 0x174), primary_bits=self.r(self.context + 0x184),
                        raw_geometry_context_hex=bytes(self.g.mem_read(self.context, 0x1e0)).hex())
        assert accepted == 1
        # Declared input composition, not whole4B5D0 geometry construction:
        # one accepted model has no selected drawable sections. The model's
        # backed identity word supports actual133F70's camo kind check.
        self.g.mem_write(self.collector, bytes(0x1e4))
        self.w(self.collector, HANDLE)
        self.g.mem_write(self.collector + 0x166, struct.pack('<h', accepted))
        self.w(self.collector + 0x170, selector['flags'])
        self.w(self.collector + 0x178, selector['shader'])
        self.w(self.collector + 0x188, selector['primary_bits'])
        self.w(self.collector + 0x48, self.model_owner)
        self.w(self.collector + 0x1c8, self.counter)
        self.w(self.counter, 0)
        self.collector_calls = []
        self.subpart_calls = []
        self.run(0x133ab0, argv=[self.collector, 0, 0, 0], registers=[(UC_X86_REG_ECX, self.render_context)])
        self.recording = False
        assert bytes(self.g.mem_read(self.unit, 0x400)) == original_unit
        return dict(selector=selector, collectors=self.collector_calls.copy(), subpart_calls=self.subpart_calls.copy(),
                    root_after=self.fields(self.collector), counter_after=self.r(self.counter),
                    entered={f'{at:08x}': self.entries[at] - before_counts.get(at, 0) for at in self.watched})


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT / 'whole-owners.json')
    a = ap.parse_args()
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    assert sha(ROOT / 'assets/disc/maps/lockout.map') == CACHE_PIN
    t = CamouflageOverlay(cache)
    primary = [bits(x) for x in (-.1, -0., 0., .25, .75, 1.)] + [0x31c00000]
    rows = [dict(definition=d, primary_bits=p, activation_flags=f, first_person_owner=o, force_world=w, caller_mode=m)
            for d, p, f, o, w, m in itertools.product((0xe9f2086d, 0xea1b0896), primary, (0, 8), (False, True), (False, True), (0, 1))]
    results = [t.case(row) for row in rows]
    assert not t.entries[0x2cbf0]
    a.output.parent.mkdir(parents=True, exist_ok=True)
    report = dict(schema='original-camouflage-whole-overlay-v1', boundary=__doc__, xbe_sha256=PIN, cache_sha256=CACHE_PIN,
                  oracle_sha256=sha(__file__), dependencies={str(p.relative_to(ROOT)): sha(p) for p in
                      (ROOT / 'tools/oracle_shield_overlay.py', ROOT / 'tools/oracle_damage_feedback.py', ROOT / 'tools/oracle_multiplayer_hud_positions.py')},
                  rasterizer_pointer=t.rasterizer_pointer, global_shader=t.global_shader, inputs=rows, results=results,
                  entries={f'{at:08x}': t.entries[at] for at in t.watched}, instruction_patches=[], supplied_returns=[],
                  executed_instructions=[dict(address=at, bytes_hex=t.image.read(at, size).hex()) for at, size in sorted(t.executed.items())],
                  limits=['Declared current unit model-variant byte B1=FF; geometry history4BA050=-1, distance10 and cap5 are explicit inputs.',
                          'Original Lockout definition/model/shield metadata is unchanged. Activation flags/amount/first-person/caller-mode are declared inputs.',
                          'Collector input has one accepted model and zero drawable sections; source4B5D0 construction/resource readiness is not inferred.',
                          'Actual clone/suppression/full133F70 return executes; no2CBF0 call, ready geometry, framebuffer, refraction or GPU draw is claimed.'])
    a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(cases=len(rows), entries=report['entries'], sha256=sha(a.output)), indent=2))


if __name__ == '__main__':
    main()
