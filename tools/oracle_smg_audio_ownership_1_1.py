#!/usr/bin/env python3
"""Independent original unvoiced source ownership kernels and callback stop.

The original device remains unavailable. Actual rejected FP requests are passed
to a declared direct creation kernel, as in the immutable cache fixture. Neither
a sound owner pool nor a driver/voice is supplied to admit source execution.
"""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_audio_cache_1_1 import CacheTrace, SERVICES
from oracle_smg_fp_sound_1_1 import BoundaryReached
from oracle_smg_audio_pool_prefix_1_1 import FROZEN as OLDER, ROOT, PIN, CACHE_PIN, digest
from oracle_smg_firing_1_1 import SEED, STOP, UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP
from oracle_smg_constructor_equipped_1_1 import _trace

OUT = ROOT / 'analysis/multiplayer/map-pack/smg-audio-host-contract'
FROZEN = {**OLDER,
    'tools/oracle_smg_audio_pool_prefix_1_1.py': '381b1763cdffb9cbc7b73c25c983697597284d29ac7e4847bd2f092f339906db',
    'tools/audit_smg_audio_pool_prefix_1_1.py': '4456ca628d3978ed7dfe65b4975a723ba73b5c8adad87fe951aa3fc8c9b84964',
    'docs/SMG_AUDIO_POOL_PREFIX_1_1_RESEARCH.md': '02dccbdc2c817b45a292c19518441af439ccda449009a0987eaf4429068058aa',
    'analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen/suite.json': '2e2c0844a4538f2b818e95f5179b2b8f4abb8af7c9928f10885cb7dcfee2959a',
    'analysis/multiplayer/map-pack/smg-audio-pool-prefix/source-fixture.json': '26efeabc982e22a8975842267cc0a936c35e3c0d6832b49b1619f8773c4bf140',
    'analysis/multiplayer/map-pack/smg-audio-pool-prefix/audit.json': 'ba75bdc8679340a55b20813c6386bc109c9b9840e6d390275e7cbdb46e4cf6ed',
}
ENTRIES = (0x128A70, 0x128AE0, 0x1280B0, 0x12BD20, 0x21BD90,
           0x16D4C0, 0x18E2F0, 0xBBCB0, 0xD2270, 0x129710, 0x129770,
           0x18A200, 0x18A2A0)


class OwnershipTrace(CacheTrace):
    def __init__(self):
        self.ownership_enabled = False
        self.ownership_calls = []
        self.ownership_stores = []
        self.ownership_executed = set()
        self.callback_stop = None
        super().__init__()

    def hook(self, g, a, size, user):
        if self.ownership_enabled:
            if a in ENTRIES:
                sp = g.reg_read(UC_X86_REG_ESP)
                self.ownership_calls.append({'phase': self.active, 'entry': f'{a:08x}',
                    'registers': self.registers(), 'return_site': f'{self.u(sp):08x}',
                    'stack': [f'{self.u(sp+4+4*i):08x}' for i in range(3)]})
            if a == 0x18E2F5:
                assert self.u(0x4F485C) == 0 and g.reg_read(UC_X86_REG_EAX) == 0
                self.callback_stop = {'instruction': f'{a:08x}',
                    'instruction_bytes': self.image.read(a, 3).hex(),
                    'dependency': 'original owner callback dereferences absent source-owned pool 004f485c at +29',
                    'owner_pool_slot': '004f485c', 'owner_pool_pointer': '00000000',
                    'registers': self.registers(), 'result_or_owner_field_supplied': False}
                raise BoundaryReached('absent original owner callback pool')
            self.ownership_executed.add(a)
        super().hook(g, a, size, user)

    def mem_write(self, g, access, address, size, value, user):
        if self.ownership_enabled:
            self.ownership_stores.append({'phase': self.active,
                'instruction': f'{g.reg_read(UC_X86_REG_EIP):08x}',
                'address': f'{address:08x}', 'size': size,
                'before_hex': bytes(g.mem_read(address, size)).hex(),
                'after_hex': (value & ((1 << (8*size))-1)).to_bytes(size, 'little').hex()})
        super().mem_write(g, access, address, size, value, user)

    def retained(self):
        result = self.cache_state()
        result.update(shared_seed=f'{self.u(SEED):08x}', local_seed=f'{self.u(SEED+4):08x}',
            source_owner_pool_pointer=f'{self.u(0x4F485C):08x}',
            source_owner_allocator_receiver=f'{self.u(0x51841C):08x}',
            original_source_callback_table_hex=self.image.read(0x44B14C, 32).hex())
        return result

    def ownership_case(self, scenario, loaded, full_remove):
        self.setup_cache()
        creation = self.creation(scenario)
        if loaded:
            self.worker_once()
        before = self.retained()
        assert self.u(self.source_pointer+0xAC, 2) == 0xFFFF
        assert self.u(self.source_pointer+0xA8) == 0xFFFFFFFF
        assert self.u(self.source_pointer+0x14) == 0x44B14C
        assert self.u(0x4F485C) == 0
        datum = int(creation['created_original_source_datum'], 16)
        self.ownership_enabled = True
        phases = []
        if full_remove:
            self.active = 'direct_original_full_remove_reason_3_until_owner_callback'
            try:
                self.fp_action(self.active, 0x128A70, (3,), ((UC_X86_REG_EAX, datum),))
            except BoundaryReached:
                assert self.callback_stop is not None
            else:
                raise AssertionError('owner callback must remain stopped')
            phases.append({'name': self.active, 'entry': '00128a70',
                'supplied_reason': 3, 'supplied_source_datum': f'{datum:08x}',
                'completed': False, 'after': self.retained()})
        else:
            for name, entry, regs in (
                ('direct_original_source_unpin', 0x1280B0, ((UC_X86_REG_ECX, self.source_pointer),)),
                ('direct_original_source_finalizer_omitting_owner_callback', 0x128AE0, ((UC_X86_REG_EDI, datum),)),
            ):
                self.fp_action(name, entry, (), regs)
                phases.append({'name': name, 'entry': f'{entry:08x}', 'completed': True,
                    'supplied_registers': [{'register': r, 'value': f'{v:08x}'} for r, v in regs],
                    'after': self.retained()})
            if not loaded:
                self.worker_once()
                phases.append({'name': 'direct_original_async_worker_after_source_finalizer',
                    'entry': '00122360', 'one_worker_iteration_call_site_supplied': True,
                    'after': self.retained()})
        after = self.retained()
        assert before['audio_initialized'] == after['audio_initialized'] == 0
        assert before['driver_pointer'] == after['driver_pointer'] == '00000000'
        return {'name': f'{scenario}_{"loaded" if loaded else "pending"}_{"full_callback_stop" if full_remove else "unpin_finalizer_kernels"}',
            'scenario': scenario, 'initial_resource_loaded': loaded, 'creation': creation,
            'before': before, 'phases': phases, 'after': after,
            'callback_stop': self.callback_stop, 'calls': self.ownership_calls,
            'stores': self.ownership_stores, 'provider_requests': self.provider_calls,
            'supplied_world_and_animation_services': {f'{a:08x}': str(v) for a, v in self.services.items()},
            'inherited_constructor_boundaries': self.boundaries,
            'source_constructor_owner_voice_or_available_result_supplied': False,
            'complete_source_removal_claimed': False,
            'direct_finalizer_omits_owner_callback': not full_remove,
            'full_normal_scheduler_or_platform_playback_claimed': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    for path, pin in FROZEN.items():
        assert digest(ROOT/path) == pin, path
    traces = []
    rows = []
    for scenario, loaded in (('ready', False), ('reload', True)):
        for full_remove in (True, False):
            trace = OwnershipTrace()
            rows.append(trace.ownership_case(scenario, loaded, full_remove))
            traces.append(trace)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    directory = args.output.parent/'instructions'
    directory.mkdir(exist_ok=True)
    listings = []
    trace = traces[0]
    executed = set().union(*(t.ownership_executed for t in traces))
    for entry in ENTRIES:
        function = _trace(trace.code, entry, trace.starts | set(ENTRIES))
        instructions = [trace.code.at(a) for a in sorted(function.code)]
        for instruction in instructions:
            assert trace.image.read(instruction.address, len(instruction.bytes)) == instruction.bytes
        path = directory/f'{entry:08x}.asm'
        path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions))
        listings.append({'entry': f'{entry:08x}', 'file': path.name, 'sha256': digest(path),
            'instructions': len(instructions), 'executed': sum(i.address in executed for i in instructions),
            'direct_sites': [{'site': f'{a:08x}', 'target': f'{b:08x}', 'kind': kind} for a, b, kind in function.sites]})
    instructions = []
    supplied = set(trace.services) | set(SERVICES)
    for address in sorted(executed):
        if address == STOP or address in supplied:
            continue
        section = next((s for s in trace.image.sections if s.va <= address < s.va+s.vsize and s.flags & 4), None)
        assert section is not None, f'{address:08x}'
        instruction = trace.code.at(address)
        assert instruction is not None
        assert trace.image.read(address, len(instruction.bytes)) == instruction.bytes
        assert all(bytes(t.g.mem_read(address, len(instruction.bytes))) == instruction.bytes for t in traces)
        instructions.append(instruction)
    path = directory/'executed-original-ownership-slice.asm'
    path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions))
    for prior, pin in FROZEN.items():
        assert digest(ROOT/prior) == pin, prior
    report = {'source_sha256': PIN, 'cache_sha256': CACHE_PIN, 'oracle_sha256': digest(__file__),
        'frozen_prior_pins': FROZEN, 'cases': rows, 'instruction_listings': listings,
        'executed_original_listing': {'file': path.name, 'sha256': digest(path), 'instructions': len(instructions)},
        'direct_kernel_call_sites_supplied': True, 'new_source_available_or_device_result_supplied': False,
        'new_source_owner_pool_fields_supplied': False, 'complete_normal_audio_lifecycle_proven': False,
        'native_gameplay_update_applied': False}
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({'output': str(args.output), 'cases': len(rows), 'executed_original_instructions': len(instructions),
        'prior_pins': len(FROZEN), 'source_owner_callback_stops': sum(row['callback_stop'] is not None for row in rows)}))


if __name__ == '__main__':
    main()
