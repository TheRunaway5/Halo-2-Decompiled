#!/usr/bin/env python3
"""Original per-view SMG particle rendering from the immutable positive owner.

The earlier explicit view/fire, failed-audio, finite-clock and resident-resource
boundaries are retained. No new camera/view/PVS/material/readiness result,
particle/pose/lifetime or graphics service is supplied by this extension.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import subprocess
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import *
import oracle_smg_particle_owner_1_1 as owner
from oracle_smg_effect_owner_1_1 import sha, save, encoded, STOP
from functions import _trace
from oracle_smg_audio_rng_startup_1_1 import RngTrace
from oracle_mp_spawn_process_heap_world import KernelBoundary

ROOT = owner.ROOT
OUT = ROOT / 'analysis/multiplayer/integration/spawn-source/smg-particle-render'
FROZEN = dict(owner.FROZEN)
FROZEN['tools/oracle_smg_particle_owner_1_1.py'] = 'f57311f72a977e9e1ce18ba461327baec9db6ca3b39b0de70a8a2f0bb8c67c43'
FROZEN['tools/oracle_smg_audio_rng_startup_1_1.py'] = 'c67ddaa11d18fb50a67eea73e213dd926b37f343dee1845d15f37ba702ef9cc6'


class RenderTrace(owner.ParticleTrace):
    ret = RngTrace.ret
    def __init__(self, *args, **kwargs):
        self.render_on = False
        self.render_calls = []
        self.render_pending = []
        self.render_reads = []
        self.render_stores = []
        self.render_points = []
        self.render_executed = set()
        self.render_order = 0
        self.render_stages = []
        self.raw_providers = []
        self.host_backings = []
        self.next_vm = 0xa600000
        self.libc = ctypes.CDLL(None, use_errno=True)
        self.libc.mmap.restype = ctypes.c_void_p
        self.libc.mmap.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int,
                                  ctypes.c_int, ctypes.c_int, ctypes.c_long]
        super().__init__(*args, **kwargs)
        static = json.loads((OUT / 'static-complete/report.json').read_text())
        mapping = static['owners']
        self.render_entries = {int(r['tu_entry'], 16) for r in mapping}
        self.render_entries.update(int(r['tu_entry'], 16) for r in static['tu_only_owning_context'])
        self.render_routines = {e: _trace(self.code, e, self.starts | self.render_entries)
                                for e in self.render_entries}
        self.render_owners = {a: e for e, routine in self.render_routines.items() for a in routine.code}
        self.render_globals = sorted({(int(q['tu_va'], 16), q['bytes']) for row in mapping
                                      for q in row['paired_absolute_operands']})
        self.g.hook_add(UC_HOOK_MEM_READ, self.render_read)

    def render_state(self):
        return dict(source_owner=self.snapshot(), globals=[dict(address=f'{a:08x}', size=n,
            raw_hex=self.raw(a, n)) for a, n in self.render_globals],
            renderer_banks=[dict(address=f'{a:08x}', raw2ae0_hex=self.raw(a, 0x2ae0))
                            for a in (0x54fad8, 0x5525b8)],
            renderer_backing=dict(global_word=self.raw(0x518434, 4),
                raw452e8_hex=self.raw(self.u(0x518434), 0x452e8) if self.u(0x518434) else None))

    def render_returns(self, address):
        matches = [q for q in self.render_pending if q[0] == address]
        if not matches:
            return
        esp = matches[-1][1]
        for ret, stack, call in reversed(matches):
            if stack != esp:
                continue
            call.update(return_order=self.render_order, return_cpu=self.cpu())
            self.render_pending.remove((ret, stack, call))
            self.render_order += 1

    def hook(self, g, address, size, user):
        if self.render_on:
            self.render_returns(address)
            self.render_executed.add(address)
            if address in self.render_entries:
                sp = g.reg_read(UC_X86_REG_ESP)
                call = dict(id=len(self.render_calls), order=self.render_order, phase=self.active,
                    entry=f'{address:08x}', caller=f'{self.u(sp):08x}', cpu=self.cpu(),
                    stack_words=[f'{self.u(sp + 4 + n * 4):08x}' for n in range(12)])
                self.render_calls.append(call)
                self.render_pending.append((self.u(sp), sp, call))
                self.render_order += 1
            if address in self.render_owners:
                ins = self.code.at(address)
                if ins.mnemonic.startswith(('j', 'f')) or ins.mnemonic == 'call' or ins.mnemonic.endswith('ss'):
                    self.render_points.append(dict(order=self.render_order, phase=self.active,
                        instruction=f'{address:08x}', owner=f'{self.render_owners[address]:08x}', cpu=self.cpu(),
                        stack80_hex=self.raw(g.reg_read(UC_X86_REG_ESP), 0x80)))
                    self.render_order += 1
        if self.render_on and address == 0x33785c and self.active == 'source_renderer_storage_constructor':
            sp = g.reg_read(UC_X86_REG_ESP)
            assert [self.u(sp + 4 + n * 4) for n in range(4)] == [0, 0x452e8, 0x101000, 4]
            self.render_executed.add(address)
            RngTrace.cache_vm_provider(self)
            return
        if self.render_on and address == 0x800000b8:
            sp = g.reg_read(UC_X86_REG_ESP)
            args = [self.u(sp + 4 + n * 4) for n in range(5)]
            self.new_invalid = dict(boundary='Unprovided additional original process-heap kernel memory request; no status or commit result supplied.',
                kernel_target='800000b8', kernel_ordinal=184,
                original_return_site=f'{self.u(sp):08x}',
                args=[f'{v:08x}' for v in args], requested_base=f'{self.u(args[0]):08x}',
                requested_size=self.u(args[2]), cpu=self.cpu(),
                heap_prefix_hex=self.raw(self.u(0x566f08), 0x650))
            raise KernelBoundary()
        super().hook(g, address, size, user)

    def render_read(self, g, access, address, size, value, user):
        ip = g.reg_read(UC_X86_REG_EIP)
        if self.render_on and ip in self.render_owners:
            self.render_reads.append(dict(order=self.render_order, phase=self.active, instruction=f'{ip:08x}',
                address=f'{address:08x}', size=size, raw_hex=self.raw(address, size)))
            self.render_order += 1

    def mem_write(self, g, access, address, size, value, user):
        if self.render_on:
            self.render_stores.append(dict(order=self.render_order, phase=self.active,
                instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}', address=f'{address:08x}', size=size,
                before_hex=self.raw(address, size), written_hex=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()))
            self.render_order += 1
        super().mem_write(g, access, address, size, value, user)

    def render_stage(self, name, entry, args=(), regs=()):
        before = self.render_state()
        first_event = len(self.events)
        ok = self.stage(name, entry, args, regs)
        if ok:
            self.render_returns(STOP)
        self.render_stages.append(dict(name=name, entry=f'{entry:08x}', args=[f'{v:08x}' for v in args],
            registers={str(k): f'{v:08x}' for k, v in regs}, before=before, after=self.render_state(),
            completion=self.player_stages[-1]['completion'], stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',
            invalid=self.new_invalid, services=self.events[first_event:]))
        return ok


def prepare(data):
    local = owner.local
    t = local.base.owner.player_source.prepare(data, trace_type=RenderTrace)
    assert local.base.owner.player_source.spawn_and_activate(t, 0, activate=False)
    old = json.loads((local.base.owner.OUT / 'report.json').read_text())
    t.item_observe = True
    for r in old['stages']:
        assert t.item_stage(r['name'], int(r['entry'], 16), tuple(int(v, 16) for v in r['arguments']),
            tuple((int(k), int(v, 16)) for k, v in r['input_registers'].items()))
    result = dict(preparation=t.producers, player_prefix_stages=t.player_stages, stages=t.item_stages,
        calls=t.item_calls, callbacks=t.item_callbacks, stores=t.item_stores, states=t.item_states,
        resources=t.resource_delivery, services=t.events, executed_addresses=[f'{v:08x}' for v in sorted(t.executed)],
        metadata=local.base.owner.metadata(t))
    for k, v in result.items():
        assert v == old[k], ('owning runtime changed', k)
    t.item_observe = False
    t.fp_resources()
    t.effect_on = t.birth_on = True
    prior = json.loads((local.OUT / 'audio-unavailable/report.json').read_text())
    for r in prior['stages']:
        if r['name'] == 'declared_local_view_request_owner':
            for e in (0x168866, 0x16a356, 0x107b50, 0x179b90):
                t.services.pop(e, None)
            t.services[0x18ad70] = (prior['audio_boundary']['scope'], -1, 24)
        if r['name'] == 'source_control_conversion':
            cmd = int(prior['declared_requests']['command']['pointer'], 16)
            assert t.raw(cmd, 0x5c) == prior['declared_requests']['command']['before5c_hex']
            t.w(cmd, 0x50000)
            t.w(cmd + 0x14, 0x3f800000)
        assert t.effect_stage(r['name'], int(r['entry'], 16), tuple(int(v, 16) for v in r['arguments']),
            tuple((int(k), int(v, 16)) for k, v in r['registers'].items()))
    t.finish_birth(STOP)
    assert not t.birth_pending
    for k, v in dict(stages=t.effect_stages, calls=t.effect_calls, states=t.effect_states,
        reads=t.effect_reads, stores=t.effect_stores, points=t.effect_points, final_state=t.snapshot()).items():
        assert v == prior[k], ('frozen local effect changed', k)
    births = json.loads((owner.OUT / 'report.json').read_text())
    for k, v in dict(calls=t.birth_calls, points=t.birth_points, reads=t.birth_reads, stores=t.birth_stores).items():
        assert v == births[k], ('frozen particle owner changed', k)
    t.effect_on = t.birth_on = False
    return t


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', required=True, type=Path)
    args = ap.parse_args()
    for path, pin in FROZEN.items():
        assert sha(ROOT / path) == pin, path
    args.output.parent.mkdir(parents=True, exist_ok=True)
    local = owner.local
    env = dict(os.environ, WINEDEBUG='-all', WINEPREFIX=str(local.base.owner.player_source.CONTEXT_OUT / 'wine-prefix'))
    with (args.output.parent / 'host-context.stderr').open('w') as stderr:
        proc = subprocess.Popen(['wine', str(local.base.owner.player_source.CONTEXT_OUT / 'generator.exe')],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=stderr, text=True, env=env)
        try:
            data = json.loads(proc.stdout.readline())
            save(args.output.parent / 'host-context.json', encoded(data))
            t = prepare(data)
            print('All immutable local effect and particle owner fields reproduced', flush=True)
            t.render_on = True
            for mode in (0, 1):
                ok = t.render_stage('source_per_view_particle_admission_' + str(mode), 0x175ca0, (mode,))
                print('mode', mode, 'ok', ok, 'stop', f'{t.g.reg_read(UC_X86_REG_EIP):08x}', flush=True)
                if not ok:
                    break
            if t.render_stage('source_renderer_storage_constructor', 0x133760):
                for mode in (0, 1):
                    assert t.render_stage('source_per_view_after_storage_' + str(mode), 0x175ca0, (mode,))
            listings = []
            for name, addresses in [('all-executed', t.render_executed),
                    ('all-stores', {int(s['instruction'], 16) for s in t.render_stores})]:
                path = args.output.parent / 'instructions' / (name + '.asm')
                # Encoded kernel targets are dependency stops, not original
                # instructions. Record them in the stage, never as listings.
                ins = [i for a in sorted(addresses) if (i := t.code.at(a)) is not None]
                for i in ins:
                    assert i.bytes == t.image.read(i.address, i.size) == bytes(t.g.mem_read(i.address, i.size))
                save(path, ''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins).encode())
                listings.append(dict(file=str(path.relative_to(args.output.parent)), rows=len(ins), sha256=sha(path)))
            report = dict(schema='source-smg-particle-render-v1', tool_sha256=sha(__file__),
                prior_complete_local_effect_and_birth_fields_equal=True, frozen_pins=FROZEN,
                declared_entry_requests='Source175CA0 mode0/1 roots from the real original render callers; owning full render-view/camera caller schedule remains separate.',
                stages=t.render_stages, calls=t.render_calls, points=t.render_points,
                reads=t.render_reads, stores=t.render_stores, instructions=listings,
                unreturned_calls=[q[2]['id'] for q in t.render_pending],
                source_requested_raw_memory=[{k: v for k, v in q.items() if k != 'raw_provider_operation'}
                                             for q in t.raw_providers],
                new_graphics_camera_view_mask_material_clock_particle_or_transform_services=[],
                new_guest_assignments=[], instruction_patches=[])
            save(args.output, encoded(report))
            save(args.output.parent / 'actual-host-storage.json', encoded(t.raw_providers))
            print('report', sha(args.output), flush=True)
        finally:
            if proc.poll() is None:
                proc.stdin.write('x\n')
                proc.stdin.flush()
            proc.stdin.close()
            try:
                proc.wait(timeout=15)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=5)
            proc.stdout.close()
        if proc.returncode:
            raise RuntimeError(('host context process', proc.returncode))


if __name__ == '__main__':
    main()
