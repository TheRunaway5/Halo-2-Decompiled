#!/usr/bin/env python3
"""Separate read-only measurement of actual C57A0/E0C20/30BD0 fixture state.

Frozen source mains run without changes to game instructions/state. Existing
listing writes are compared and suppressed. Baseline reports must reproduce
exactly before and during measurement; new observations are separate evidence.
"""
import argparse
import hashlib
import importlib
import json
from pathlib import Path
import struct
import sys
from export_mp_pickup_outer_fixtures_1_1 import ROOT, BASE, sha, require, source_pins, json_bytes
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_XMM0, UC_X86_REG_XMM1,
    UC_X86_REG_FP0, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG, UC_X86_REG_MXCSR)

OUT = BASE / 'pickup-consumer-measured'
INSTANCES = []
NORMAL_SITES = (0x30be9, 0x30beb, 0x30bf1, 0x30bf4, 0x30c00, 0x30c02, 0x30c0f, 0x30c17, 0x30c38)
GEOMETRY_SITES = (0xc5845, 0xc59ab, 0xc5b4b, 0xc5b70, 0xc5b8d, 0xc5bb2)


class MeasurementMixin:
    def __init__(self, *args, **kwargs):
        self.measured_cases = []
        self.measured_pending = []
        self.measured_active = []
        self.measured_normal = []
        INSTANCES.append(self)
        super().__init__(*args, **kwargs)

    def mbits(self, address, count=3):
        return [f'{self.u(address + i * 4):08x}' for i in range(count)]

    def object_input(self, datum):
        pool = self.u(0x4e78d0); headers = self.u(pool + 0x44)
        header = headers + (datum & 0xffff) * 12; body = self.u(header + 8)
        return dict(datum=f'{datum:08x}', header_address=f'{header:08x}',
            header_hex=bytes(self.g.mem_read(header, 12)).hex(), header_type=self.u(header + 3, 1),
            body_address=f'{body:08x}', definition=f'{self.u(body):08x}', parent=f'{self.u(body + 0x14):08x}',
            parent_node=self.u(body + 0x18, 1), root_bits=self.mbits(body + 0x64),
            scale_bits=f'{self.u(body + 0xa0):08x}', body_prefix_hex=bytes(self.g.mem_read(body, 0x34c)).hex())

    def fpu_state(self):
        g = self.g
        registers = []
        for index in range(8):
            mantissa, exponent = g.reg_read(UC_X86_REG_FP0 + index)
            registers.append(dict(register=f'FP{index}', mantissa_hex=f'{mantissa:016x}',
                exponent_hex=f'{exponent:04x}', raw80_hex=struct.pack('<QH', mantissa, exponent).hex()))
        status = g.reg_read(UC_X86_REG_FPSW)
        return dict(control=f'{g.reg_read(UC_X86_REG_FPCW):04x}', status=f'{status:04x}',
            tag=f'{g.reg_read(UC_X86_REG_FPTAG):04x}', top=(status >> 11) & 7,
            mxcsr=f'{g.reg_read(UC_X86_REG_MXCSR):08x}', registers=registers)

    def finish_measured(self, address):
        for item in self.measured_pending[:]:
            if item['return_address'] != address:
                continue
            kind = item['kind']; row = item['row']
            if kind == 'visibility':
                row['final_al'] = self.g.reg_read(UC_X86_REG_EAX) & 255
                row['query_range'][1] = len(self.outer_queries)
                require(self.measured_active.pop() is row, 'Visibility stack differs')
            elif kind == 'capsule':
                row['bottom_bits'] = self.mbits(item['bottom'])
                row['extent_bits'] = self.mbits(item['extent'], 1)[0]
                row['radius_bits'] = self.mbits(item['radius'], 1)[0]
            elif kind == 'root':
                row['source_root_bits'] = self.mbits(item['output'])
            elif kind == 'item_point':
                row['item_point_bits'] = self.mbits(item['output'])
            else:
                row['output_bits'] = self.mbits(item['input_pointer'])
                row['return_xmm0_bits'] = f'{self.g.reg_read(UC_X86_REG_XMM0) & 0xffffffff:08x}'
                row['length_spill_bits'] = self.mbits(item['spill'], 1)[0]
                row['return_fpu_state'] = self.fpu_state()
                require(self.measured_normal.pop() is row, 'Normalizer stack differs')
            self.measured_pending.remove(item)

    def run(self, *args, **kwargs):
        super().run(*args, **kwargs)
        self.finish_measured(0x3000000)

    def hook(self, g, address, size, user):
        self.finish_measured(address)
        sp = g.reg_read(UC_X86_REG_ESP)
        if address == 0xc57a0 and hasattr(self, 'visibility_calls'):
            unit = g.reg_read(UC_X86_REG_EDI); item = g.reg_read(UC_X86_REG_ESI)
            row = dict(id=len(self.measured_cases), suite=self.measurement_suite, phase=self.active,
                source_call_index=len(self.visibility_calls), caller=f'{self.u(sp):08x}',
                unit_input=self.object_input(unit), item_input=self.object_input(item),
                query_range=[len(self.outer_queries), None], geometry=[], normalizer=None)
            self.measured_cases.append(row); self.measured_active.append(row)
            self.measured_pending.append(dict(kind='visibility', row=row, return_address=self.u(sp)))
        case = self.measured_active[-1] if self.measured_active else None
        if case and address == 0xe0c20:
            unit = g.reg_read(UC_X86_REG_EAX); state = self.object_input(unit)
            body = int(state['body_address'], 16); definition = int(state['definition'], 16)
            tag_bank = self.u(0x4eb114); definition_address = self.u(tag_bank + (definition & 0xffff) * 16 + 8)
            fields = bytes(g.mem_read(definition_address + 0x264, 16))
            row = dict(source_call_index=len(self.visibility_calls), caller=f'{self.u(sp):08x}', unit_input=state,
                current_crouch_bits=f'{self.u(body + 0x2c4):08x}', metadata_address=f'{definition_address:08x}',
                metadata_fields_hex=fields.hex(), metadata_file_offset=f'{definition_address + 0x264 - self.cache.magic:08x}')
            case['capsule'] = row
            self.measured_pending.append(dict(kind='capsule', row=row, return_address=self.u(sp),
                bottom=g.reg_read(UC_X86_REG_ECX), extent=self.u(sp + 4), radius=self.u(sp + 8)))
        if case and address == 0xbacc0 and self.u(sp) == 0xe0c5c:
            self.measured_pending.append(dict(kind='root', row=case['capsule'], return_address=self.u(sp),
                output=g.reg_read(UC_X86_REG_ESI)))
        if case and address == 0xfbfa0 and self.u(sp) == 0xc5845:
            case['item_point_input'] = self.object_input(g.reg_read(UC_X86_REG_ECX))
            self.measured_pending.append(dict(kind='item_point', row=case, return_address=self.u(sp),
                output=g.reg_read(UC_X86_REG_EAX)))
        if case and address == 0x30bd0 and self.u(sp) == 0xc5985:
            pointer = g.reg_read(UC_X86_REG_EAX)
            row = dict(caller=f'{self.u(sp):08x}', input_pointer=f'{pointer:08x}', input_bits=self.mbits(pointer),
                entry_fpu_state=self.fpu_state(), stages=[],
                world_up_pointer=f'{self.u(0x46fc50):08x}', world_up_bits=self.mbits(self.u(0x46fc50)),
                fallback_pointer=f'{self.u(0x46fc58):08x}', fallback_bits=self.mbits(self.u(0x46fc58)),
                threshold_bits=f'{self.u(0x46504c):08x}', one_bits=f'{self.u(0x465030):08x}')
            case['normalizer'] = row; self.measured_normal.append(row)
            self.measured_pending.append(dict(kind='normalizer', row=row, return_address=self.u(sp),
                input_pointer=pointer, spill=sp - 4))
        if self.measured_normal and address in NORMAL_SITES:
            row = self.measured_normal[-1]
            pending = next(p for p in self.measured_pending if p['kind'] == 'normalizer' and p['row'] is row)
            row['stages'].append(dict(instruction=f'{address:08x}', input_or_output_bits=self.mbits(pending['input_pointer']),
                spill_bits=self.mbits(pending['spill'], 1)[0], spill_defined=address not in (0x30be9, 0x30beb, 0x30bf1),
                xmm0_bits=f'{g.reg_read(UC_X86_REG_XMM0) & 0xffffffff:08x}',
                xmm1_bits=f'{g.reg_read(UC_X86_REG_XMM1) & 0xffffffff:08x}', fpu_state=self.fpu_state()))
        if case and address in GEOMETRY_SITES:
            # Observe actual source stack buffers only at the owning stages.
            vectors = {}
            if address == 0xc5845:
                locations = [('bottom', 0x34), ('top', 0x58), ('item', 0x10)]
            elif address == 0xc59ab:
                locations = [('chosen_side', 4)]
            else:
                locations = [('positive_target', 0x1c), ('negative_target', 0x28)]
            for name, offset in locations:
                vectors[name] = self.mbits(sp + offset)
            case['geometry'].append(dict(instruction=f'{address:08x}', vectors=vectors))
        super().hook(g, address, size, user)


def run_source_main(suite, output_name, measured=False):
    module_name = 'oracle_mp_pickup_visibility_1_1' if suite == 'pickup-visibility' else 'oracle_mp_pickup_retreat_1_1'
    module = importlib.import_module(module_name)
    class_name = 'VisibilityTrace' if suite == 'pickup-visibility' else 'RetreatTrace'
    original_class = getattr(module, class_name)
    fresh = BASE / suite / output_name
    original_write = Path.write_text

    def protected_write(path, data, *args, **kwargs):
        destination = path.resolve()
        encoded = data.encode(kwargs.get('encoding') or 'utf-8')
        if destination.parent == BASE / suite / 'instructions':
            require(destination.exists() and destination.read_bytes() == encoded, f'Frozen listing differs: {destination}')
            return len(data)
        require(destination == fresh, f'Unexpected source output: {destination}')
        if destination.exists():
            require(destination.read_bytes() == encoded, f'Changed repeat evidence: {destination}')
            return len(data)
        return original_write(path, data, *args, **kwargs)

    if measured:
        setattr(module, class_name, type('Measured' + class_name, (MeasurementMixin, original_class), {'measurement_suite': suite}))
    saved_argv = sys.argv; Path.write_text = protected_write
    try:
        sys.argv = [module_name, '--output', str(fresh)]
        module.main()
    finally:
        sys.argv = saved_argv; Path.write_text = original_write
        setattr(module, class_name, original_class)
    original = BASE / suite / 'report.json'
    require(fresh.read_bytes() == original.read_bytes(), f'Original fixture repeat differs: {suite}')
    return dict(suite=suite, measured=measured, original_report_sha256=sha(original),
        fresh_report=str(fresh.relative_to(ROOT)), fresh_report_sha256=sha(fresh), byte_identical=True,
        frozen_listing_writes_compared_and_suppressed=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=OUT)
    parser.add_argument('--preflight-only', action='store_true')
    args = parser.parse_args(); out = args.output_dir.resolve()
    require(ROOT / 'analysis' in out.parents, 'Evidence only under ignored analysis')
    reports = {suite: json.loads((BASE / suite / 'report.json').read_text()) for suite in ('pickup-visibility', 'pickup-retreat')}
    pins = source_pins(reports)
    for p, pin in {
        'tools/export_mp_pickup_outer_fixtures_1_1.py': '666b8b0cc79aa2dc1ede057e015a373ff6608e7db931ba46b5fca9e6579ab124',
        'tools/export_mp_pickup_consumer_fixtures_1_1.py': '1840130981199a9ed7cf7bedbfe803734c16aa180391a8379a3916f9a7e933a6',
        'analysis/multiplayer/integration/inventory-source/pickup-consumer-fixtures/manifest.json': '01bd542b423cf260e2efcc2aa72b2252c6c00216907906fc7838415272cd554c',
        'analysis/multiplayer/integration/inventory-source/pickup-native-contract/00030bd0.asm': '4ac33ed77002105df833f5446e5974aec847ed939eaae77a4679e980ed601d3a',
    }.items():
        require(sha(ROOT / p) == pin, f'Frozen gate changed: {p}'); pins[p] = pin
    preflight = [run_source_main(suite, 'consumer-preflight.json') for suite in reports]
    out.mkdir(parents=True, exist_ok=True)
    if args.preflight_only:
        (out / 'preflight.json').write_bytes(json_bytes(preflight)); print(json.dumps(preflight)); return
    baselines = [run_source_main(suite, 'consumer-measured-baseline.json', True) for suite in reports]
    cases = []; source_case_checks = 0
    for instance in INSTANCES:
        require(not instance.measured_pending and not instance.measured_active and not instance.measured_normal, 'Unfinished original observation')
        report = reports[instance.measurement_suite]
        for case in instance.measured_cases:
            original = report['calls'][case['source_call_index']]
            require(original['entry'] == '000c57a0' and case['query_range'] == original['query_range'], 'Measured call/order differs')
            require(case['final_al'] == int(original['return_eax'], 16) & 255, 'Measured final AL differs')
            capsule = case['capsule']; old_cap = report['calls'][capsule['source_call_index']]
            require(capsule['bottom_bits'] == old_cap['bottom']['bits'] and capsule['extent_bits'] == old_cap['extent_bits']
                and capsule['radius_bits'] == old_cap['radius_bits'], 'Measured capsule outputs differ')
            case['global_id'] = len(cases); cases.append(case); source_case_checks += 1
    normalizers = [c['normalizer'] for c in cases if c['normalizer'] is not None]
    require((len(cases), len(normalizers)) == (207, 187), 'Measured coverage differs')
    report = dict(source_tool_sha256=sha(Path(__file__)), source_pins=pins, preflight=preflight,
        measured_baseline=baselines, cases=cases, checked_original_case_outputs=source_case_checks,
        scope='Untouched original instructions on inherited actual allocated/equipped fixture state. Per-call crouch/header/parent and actual normalizer input/FPU/spill/output/return/current side buffers are measured, not reconstructed. Configured FPCW/MXCSR, heap/residency, supplied root/proximity, original world/Havok/current-node and Xbox reciprocal boundaries remain explicit.')
    path = out / 'report.json'; data = json_bytes(report)
    if path.exists():
        require(path.read_bytes() == data, 'Refusing changed measured evidence')
    else:
        path.write_bytes(data)
    print(json.dumps(dict(cases=len(cases), normalizers=len(normalizers), report_sha256=sha(path),
        byte_identical_source_preflight=True, byte_identical_measured_baseline=True)))


if __name__ == '__main__':
    main()
