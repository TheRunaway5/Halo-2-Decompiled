#!/usr/bin/env python3
"""Natural frame admission owners with the existing applied protection adapter."""
import ctypes
import hashlib
import json
from pathlib import Path
import sys
import unicorn
from unicorn.x86_const import *
import oracle_smg_effect_tick_clock_1_1 as tick
from oracle_smg_effect_owner_1_1 import sha, save, encoded

ROOT = tick.ROOT
OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission'
PROTECTION = ROOT / 'analysis/multiplayer/integration/spawn-source/local-map-protection'
PRIOR = {**tick.FROZEN,
    'tools/oracle_smg_effect_tick_clock_1_1.py': '67b2028de06314f10872c03fd0304800521bb0bbf1b49b1caee1ccd1ab851b25',
    'docs/MP_SMG_EFFECT_TICK_CLOCK_RESEARCH.md': 'c3029384537b107b98a995b8444cb6e3dc7815bbc7570146444adb5614967014',
    'analysis/multiplayer/integration/inventory-source/smg-effect-tick-clock/measured/report.json': '11e38bb25644e158b0fe4a1c13326fe74299c5832d17b42753768455605b9fbc',
    'analysis/multiplayer/integration/inventory-source/smg-effect-tick-clock/manifest.json': 'e465905221301f22548b3cf815d539adc3ebf0e8744c3d7b882fa25bc9dfab25'}
ENTRIES = {0x12cbc0, 0x1902e0, 0x1904d0, 0x190650, 0x190690, 0x190730, 0x1907e0, 0x190890, 0x190b30, 0x190a30, 0x147dd0, 0x139640}


def load_protection():
    report = json.loads((PROTECTION / 'report.json').read_text())
    metadata = report['compiled_compatibility_access']
    assert sha(PROTECTION / 'compiled-compatibility-access.cpp') == metadata['source_sha256']
    assert sha(PROTECTION / 'compiled-compatibility-access.so') == metadata['library_sha256']
    for name, row in report['primary_sources'].items():
        assert sha(PROTECTION / 'primary-source' / name) == row['sha256'], name
    library = ctypes.CDLL(str(PROTECTION / 'compiled-compatibility-access.so'))
    callback_type = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_uint32, ctypes.c_size_t, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32))
    library.contract.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    library.contract.restype = ctypes.c_int
    library.apply.argtypes = [ctypes.c_uint32, ctypes.c_size_t, ctypes.c_uint32, callback_type]
    library.apply.restype = ctypes.c_int
    return library, callback_type, dict(compiled=metadata, primary_sources=report['primary_sources'], original_report_sha256=sha(PROTECTION / 'report.json'), scope='Read-only reuse of pinned actual compatibility permission bodies/library; original requested mask converted and Unicorn.mem_protect actually applied. No game/cache/admission result supplied; no Xbox PTE/cache parity.')


class AdmissionTrace(tick.TickTrace):
    def __init__(self, *args, **kwargs):
        self.protection_calls, self.frame_records = [], []
        self.permission_library, self.permission_callback_type, self.permission_metadata = load_protection()
        super().__init__(*args, **kwargs)

    def frame_state(self):
        game = self.u(0x4edf18)
        return dict(game_pointer=f'{game:08x}', game1200_hex=self.raw(game, 0x1200) if game else None, frame4f4860_12c0_hex=self.raw(0x4f4860, 0x12c0), arena4ed9f0_34_hex=self.raw(0x4ed9f0, 0x34), graphics4eda30_30_hex=self.raw(0x4eda30, 0x30), cpu=self.cpu())

    def invalid(self, guest, access, address, size, value, user):
        if self.life_on and self.native_clock is not None:
            self.new_invalid = dict(boundary='Unmapped actual original frame/startup dependency; no missing receiver, field, result or admission is supplied.', instruction=f'{guest.reg_read(UC_X86_REG_EIP):08x}', access=access, address=f'{address:08x}', size=size, cpu=self.cpu(), stack40_hex=self.raw(guest.reg_read(UC_X86_REG_ESP), 0x40), actual_frame_state=self.frame_state(), original_result_supplied=False)
            return False
        return super().invalid(guest, access, address, size, value, user)

    def life_stage(self, name, entry):
        if self.native_clock is None:
            return super().life_stage(name, entry)
        self.active, self.new_invalid = name, None
        first, before = len(self.events), self.life_state()
        try:
            self.run(entry)
            completion = 'complete_original_return'
            self.life_return(tick.system.life.birth.local.base.STOP)
        except (unicorn.UcError, tick.system.KernelBoundary) as error:
            if self.new_invalid is None:
                address = self.g.reg_read(UC_X86_REG_EIP)
                recent = []
                for value in self.recent:
                    instruction = self.code.at(value)
                    if instruction is not None:
                        recent.append(dict(address=f'{value:08x}', bytes_hex=instruction.bytes.hex(), mnemonic=instruction.mnemonic, operands=instruction.op_str))
                self.new_invalid = dict(boundary='Actual CPU exception while executing unchanged original frame/startup instructions; not converted into an owner result.', exception=str(error), exception_code=getattr(error, 'errno', None), instruction=f'{address:08x}', cpu=self.cpu(), stack40_hex=self.raw(self.g.reg_read(UC_X86_REG_ESP), 0x40), recent_original_instructions=recent, actual_frame_state=self.frame_state(), original_result_supplied=False)
            completion = 'blocked_original_instruction'
        row = dict(name=name, entry=f'{entry:08x}', arguments=[], before_state=before, after_state=self.life_state(), completion=completion, stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}', invalid=self.new_invalid, services=self.events[first:], after_cpu=self.cpu(), pending_original_calls=[row['id'] for _, _, row in self.life_pending])
        self.life_stages.append(row)
        return completion == 'complete_original_return'

    def hook(self, guest, address, size, user):
        if self.life_on and self.native_clock is not None:
            if address in ENTRIES:
                self.frame_records.append(dict(order=self.life_order, instruction=f'{address:08x}', state=self.frame_state(), scope='Literal original state at owning entry; no fields selected or assigned.'))
            if address == 0x800000b6:
                stack = guest.reg_read(UC_X86_REG_ESP)
                base, count, protection = (self.u(stack + 4 + index * 4) for index in range(3))
                assert base % 4096 == 0 and count > 0 and count % 4096 == 0
                converted = (ctypes.c_uint32 * 4)()
                assert self.permission_library.contract(protection, converted) == 1
                before = bytes(guest.mem_read(base, count))
                before_cpu = self.cpu()
                regions = [dict(start=f'{left:08x}', end=f'{right:08x}', permissions=permissions) for left, right, permissions in guest.mem_regions() if left < base + count and right >= base]
                assert {row['permissions'] for row in regions} == {7}
                applied, errors = [], []

                def apply(requested_base, requested_count, windows, old):
                    try:
                        assert requested_base == base and requested_count == count and windows == converted[3] == 0x40
                        guest.mem_protect(requested_base, requested_count, 7)
                        after_regions = [dict(start=f'{left:08x}', end=f'{right:08x}', permissions=permissions) for left, right, permissions in guest.mem_regions() if left < base + count and right >= base]
                        assert {row['permissions'] for row in after_regions} == {7}
                        old[0] = 0x40
                        applied.append(dict(base=f'{requested_base:08x}', count=requested_count, windows_access=f'{windows:08x}', actual_guest_access=7, previous_windows_access='00000040', regions_after=after_regions))
                        return 1
                    except Exception as error:
                        errors.append(str(error))
                        return 0

                callback = self.permission_callback_type(apply)
                actual = self.permission_library.apply(base, count, protection, callback)
                assert actual == 1 and len(applied) == 1 and not errors, (actual, applied, errors)
                after = bytes(guest.mem_read(base, count))
                assert before == after
                returned = self.u(stack)
                guest.reg_write(UC_X86_REG_ESP, stack + 16)
                guest.reg_write(UC_X86_REG_EIP, returned)
                assert self.cpu()['registers'] == before_cpu['registers']
                self.protection_calls.append(dict(order=self.life_order, instruction='800000b6', ordinal=182, original_return=f'{returned:08x}', args=[f'{base:08x}', f'{count:08x}', f'{protection:08x}'], before_cpu=before_cpu, after_cpu=self.cpu(), source_converted_words=[f'{value:08x}' for value in converted], raw_before_sha256=hashlib.sha256(before).hexdigest(), raw_after_sha256=hashlib.sha256(after).hexdigest(), regions_before=regions, applied=applied[0], frame_state=self.frame_state(), scope=self.permission_metadata['scope']))
                return
        super().hook(guest, address, size, user)


def main():
    pin = sha(__file__)
    for file, expected in PRIOR.items():
        assert sha(ROOT / file) == expected, file
    original_save = tick.save

    def capture(path, contents):
        if path.name == 'report.json':
            report = json.loads(contents)
            trace = active_trace[0]
            report.update(schema='source-smg-effect-frame-admission-v1', oracle_sha256=pin, underlying_tick_oracle_sha256=sha(tick.__file__), frame_records=trace.frame_records, actual_memory_protection=trace.protection_calls, permission_provider=trace.permission_metadata, final_frame_state=trace.frame_state())
            contents = encoded(report)
        original_save(path, contents)

    active_trace = []

    class CapturedTrace(AdmissionTrace):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            active_trace.append(self)

    # Only Python observer/output destinations are selected. The immutable
    # owning preparation, source instructions, requests and clock roots remain.
    tick.OUT, tick.TickTrace, tick.FROZEN, tick.save = OUT, CapturedTrace, PRIOR, capture
    tick.main()
    assert sha(__file__) == pin
    for file, expected in PRIOR.items():
        assert sha(ROOT / file) == expected, file


if __name__ == '__main__':
    main()
