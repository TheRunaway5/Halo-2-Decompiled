#!/usr/bin/env python3
"""Compare current water state with unchanged authenticated TU1.5 instructions.

Only GPU setter calls are intercepted. Compiler products and emulator inputs
belong to the enclosing tools/run_test.py scratch directory.
"""
import argparse
from collections import Counter
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import subprocess

from audit_rendering import Cache
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_MXCSR, UC_X86_REG_FPCW,
)

ROOT = Path(__file__).resolve().parents[1]
PROBE = r'''
#include "halo2/water.hpp"
#include <iomanip>
#include <iostream>
int main() {
  using namespace halo2;
  std::cout << std::setprecision(9);
  WaterDefinition definition;
  while (std::cin >> definition.parameters[0]) {
    for (unsigned i=1;i<43;++i) std::cin >> definition.parameters[i];
    Vec3 eye,forward;float fov,aspect,anamorphic;
    std::cin >> eye.x >> eye.y >> eye.z >> forward.x >> forward.y >> forward.z
             >> fov >> aspect >> anamorphic;
    if (!std::cin) return 2;
    const auto frame=water_frame(definition,eye,forward,fov,aspect,anamorphic!=0);
    for(const auto &v:frame.constants)for(float f:v)std::cout<<f<<' ';
    for(const auto &v:frame.bump_matrices)for(float f:v)std::cout<<f<<' ';
    for(const auto &v:frame.local_to_world)for(float f:v)std::cout<<f<<' ';
    for(const auto &v:frame.corners)std::cout<<v.x<<' '<<v.y<<' '<<v.z<<' ';
    std::cout<<frame.horizon_argb<<'\n';
  }
}
'''


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report-output', type=Path, required=True)
    parser.add_argument('--xbe', type=Path, default=NEW_DEFAULT)
    parser.add_argument('--water-source', type=Path,
                        default=ROOT/'src/rendering/world/water.cpp')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(args.xbe)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    probe = scratch/'water-state-probe.cpp'
    binary = scratch/'water-state-probe'
    probe.write_text(PROBE)
    command = ['c++', '-std=c++20', '-O2', '-ffp-contract=off',
               '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
               '-I'+str(ROOT/'include'), str(args.water_source),
               str(probe), '-o', str(binary)]
    subprocess.run(command, check=True)
    emulator = Uc(UC_ARCH_X86, UC_MODE_32)
    emulator.mem_map(0x10000, 0x600000)
    for section in image.sections:
        if section.va + len(image.section_bytes(section)) <= 0x610000:
            emulator.mem_write(section.va, image.section_bytes(section))
    state, context, stack, stop = 0x1000000, 0x1001000, 0x200ff00, 0x3000000
    emulator.mem_map(state, 0x10000)
    emulator.mem_map(0x2000000, 0x10000)
    emulator.mem_map(stop, 0x1000)
    constants, matrices = {}, {}
    factor = 0

    def gpu_setter(machine, address, size, user):
        nonlocal factor
        pop = 0
        sp = machine.reg_read(UC_X86_REG_ESP)
        if address == 0x3fff10:
            component, value = struct.unpack('<II', machine.mem_read(sp+4, 8))
            matrices[(machine.reg_read(UC_X86_REG_EAX), component)] = \
                struct.unpack('<f', struct.pack('<I', value))[0]
            pop = 8
        elif address == 0x4012b0:
            constants[machine.reg_read(UC_X86_REG_ECX)] = list(struct.unpack(
                '<4f', machine.mem_read(machine.reg_read(UC_X86_REG_EDX), 16)))
        elif address == 0x3ff450:
            factor = struct.unpack('<I', machine.mem_read(sp+4, 4))[0]
            pop = 4
        else:
            return
        ret = struct.unpack('<I', machine.mem_read(sp, 4))[0]
        machine.reg_write(UC_X86_REG_ESP, sp+4+pop)
        machine.reg_write(UC_X86_REG_EIP, ret)

    emulator.hook_add(UC_HOOK_CODE, gpu_setter)

    def invoke(address):
        emulator.mem_write(stack, struct.pack('<I', stop))
        emulator.reg_write(UC_X86_REG_ESP, stack)
        emulator.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        emulator.reg_write(UC_X86_REG_FPCW, 0x027f)
        emulator.reg_write(UC_X86_REG_ESI, state)
        emulator.reg_write(UC_X86_REG_EAX, state)
        emulator.emu_start(address, stop, count=100000)
        assert emulator.reg_read(UC_X86_REG_EIP) == stop

    fixtures, expected = [], []
    names = ['beavercreek', 'coagulation', 'cyclotron', 'foundation', 'headlong',
             'waterworks', 'zanzibar', 'warlock', 'deltatap', 'backwash', 'dune', 'gemini']
    # The last five rows exercise the actual active-camera-context branch,
    # including each individual rejection of its panorama suppression gate.
    views = [(640, 480, 1.2, 0, None), (1920, 1080, 1.56, 0, None),
             (3840, 2160, 2.4, 1, None),
             (1920, 1080, 1.56, 1, (1, 0, 3)),
             (1920, 1080, 1.56, 1, (0, 0, 3)),
             (1920, 1080, 1.56, 1, (1, -1, 3)),
             (1920, 1080, 1.56, 1, (1, 0, 2)),
             (1920, 1080, 1.56, 0, (1, 0, 3))]
    for name in names:
        cache = Cache(ROOT/('assets/disc/maps' if name in names[:7]
                           else 'assets/map-pack/maps')/(name+'.map'))
        try:
            _, scenario = cache.tag(cache.scenario, 'scnr')
            bsp = cache.block(scenario+528, 68)[0]
            start, memory = cache.u32(bsp), cache.u32(bsp+8)
            record = cache.block(start+16+0x214, 172, memory-start)[0]
            parameters = [0.]*43
            parameters[13:] = struct.unpack_from('<30f', cache.data, record+52)
            cameras = [(3,-2,5,.8,.6,-.2), (3,-2,parameters[33],1,0,0),
                       (-6,11,-3,-.5,.25,.8), (0,0,-18,0,1,-1)]
            for camera in cameras:
                for width, height, fov, wide, camera_context in views:
                    for depth in (False, True):
                        authored = parameters.copy()
                        if depth:
                            authored[40:42] = [1., 100.]
                        eye, forward = list(map(f32, camera[:3])), list(map(f32, camera[3:]))
                        fov, aspect = f32(fov), f32(width/height)
                        emulator.mem_write(state, struct.pack('<43f', *authored))
                        emulator.mem_write(0x4c65cc, struct.pack('<3f', *forward))
                        emulator.mem_write(0x491e38, struct.pack('<3f', *eye))
                        emulator.mem_write(0x491e68, struct.pack('<4h', 0,0,height,width))
                        emulator.mem_write(0x491e60, struct.pack('<f', fov))
                        emulator.mem_write(0x4922e2, bytes([wide]))
                        emulator.mem_write(0x4f3218, struct.pack('<I', context if camera_context else 0))
                        effective_wide = wide
                        if camera_context:
                            active, index, kind = camera_context
                            emulator.mem_write(context, bytes(12))
                            emulator.mem_write(context+1, bytes([active]))
                            emulator.mem_write(context+2, struct.pack('<h', index))
                            emulator.mem_write(context+8, struct.pack('<I', kind))
                            effective_wide = bool(wide and not (active and index != -1 and kind == 3))
                        constants.clear()
                        matrices.clear()
                        factor = 0
                        emulator.mem_write(0x4ce328, bytes(48))
                        emulator.mem_write(0x4ce358, bytes(48))
                        invoke(0x3c640)
                        invoke(0x3cff0)
                        invoke(0x3ca80)
                        result = sum((constants[x] for x in range(21,26)), [])+[0.]*4
                        result += sum(([matrices[(stage,x)] for x in range(22,26)]
                                       for stage in range(1,4)), [])
                        result += list(struct.unpack('<12f', emulator.mem_read(0x4ce358,48)))
                        result += list(struct.unpack('<12f', emulator.mem_read(0x4ce328,48)))+[factor]
                        fixtures.append(dict(map=name, context=camera_context,
                            raw_wide=wide, effective_wide=effective_wide,
                            values=authored+eye+forward+[fov,aspect,float(effective_wide)]))
                        expected.append(result)
        finally:
            cache.close()
    inputs = ''.join(' '.join(format(v,'.9g') for v in row['values'])+'\n' for row in fixtures)
    native = subprocess.run([str(binary)], input=inputs, text=True, capture_output=True, check=True)
    actual = [[float(v) for v in line.split()] for line in native.stdout.splitlines()]
    assert len(actual) == len(expected)
    suppressed = [(fixture, reference) for fixture, reference in zip(fixtures, expected)
                  if fixture['raw_wide'] and not fixture['effective_wide']]
    unqualified_inputs = ''.join(' '.join(format(v,'.9g') for v in
        (fixture['values'][:-1]+[float(fixture['raw_wide'])]))+'\n'
        for fixture, _ in suppressed)
    unqualified_native = subprocess.run([str(binary)], input=unqualified_inputs,
        text=True, capture_output=True, check=True)
    unqualified_actual = [[float(v) for v in line.split()]
                          for line in unqualified_native.stdout.splitlines()]
    context_differences = []
    assert len(unqualified_actual) == len(suppressed)
    for (fixture, reference), result in zip(suppressed, unqualified_actual):
        for component, (original, current) in enumerate(zip(reference, result)):
            if component < 60:
                original, current = f32(original), f32(current)
            if abs(original-current) > max(2e-6,abs(original)*2e-6):
                context_differences.append(dict(map=fixture['map'],component=component,
                    source=original,native_unqualified_wide=current))
    assert context_differences, 'Camera-context cases do not distinguish the actual gate'
    maximum, bit_exact, checked, errors = 0., 0, 0, []
    zero_sign_mismatches, zero_sign_components = [], Counter()
    for fixture, reference, result in zip(fixtures, expected, actual):
        assert len(reference) == len(result) == 61
        for component, (original, current) in enumerate(zip(reference, result)):
            if component < 60:
                original, current = f32(original), f32(current)
                bit_exact += struct.pack('<f',original) == struct.pack('<f',current)
                if original == current == 0 and struct.pack('<f',original) != struct.pack('<f',current):
                    zero_sign_components[component] += 1
                    zero_sign_mismatches.append(dict(map=fixture['map'],context=fixture['context'],
                        component=component,source_bits=struct.pack('<f',original).hex(),
                        native_bits=struct.pack('<f',current).hex(),
                        eye=fixture['values'][43:46],forward=fixture['values'][46:49],
                        fov=fixture['values'][49],aspect=fixture['values'][50],
                        effective_wide=fixture['effective_wide']))
            else:
                bit_exact += original == current
            checked += 1
            difference = abs(original-current)
            maximum = max(maximum, difference)
            if not math.isfinite(current) or difference > max(2e-6, abs(original)*2e-6):
                errors.append(dict(map=fixture['map'],context=fixture['context'],
                    component=component,source=original,native=current,difference=difference))
    report = dict(schema='water-state-tu15-original-v1', xbe_sha256=NEW_PIN,
        water_cpp_path=str(args.water_source),
        water_cpp_sha256=hashlib.sha256(args.water_source.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        probe_cpp_sha256=hashlib.sha256(PROBE.encode()).hexdigest(),
        source_entries=['0003C640','0003CFF0','0003CA80','001349E0'],
        fixture_count=len(fixtures),components=checked,bit_exact_components=bit_exact,
        maximum_absolute_difference=maximum,errors=errors[:20],
        passed=not errors and bit_exact==checked,
        numeric_passed=not errors,bit_exact_passed=bit_exact==checked,
        zero_sign_mismatch_count=len(zero_sign_mismatches),
        zero_sign_mismatch_components=dict(sorted(zero_sign_components.items())),
        zero_sign_mismatch_examples=zero_sign_mismatches[:24],
        zero_sign_mismatch_component_examples=[next(row for row in zero_sign_mismatches
            if row['component']==component)for component in sorted(zero_sign_components)],
        context_gate_cases=sum(row['context'] is not None for row in fixtures),
        widescreen_suppression_cases=sum(bool(row['raw_wide'] and not row['effective_wide']) for row in fixtures),
        unqualified_widescreen_differing_components=len(context_differences),
        unqualified_widescreen_examples=context_differences[:8],
        effective_widescreen_input='Native helper takes the source camera-context-qualified wide flag; caller ownership is outside this comparison.',
        command=command, scratch=str(scratch), retained_large_artifacts=[],
        supplied_services=['GPU SetTextureStageState3FFF10','GPU SetVertexShaderConstant14012B0','GPU SetRenderState_FogColor3FF450'],
        scope='Original authored water constants/bump state/camera basis/color packing with actual TU1.5 math and declared camera-context inputs; GPU setter writes intercepted. No GPU sampling, live ownership, shader output or framebuffer parity claim.')
    args.report_output.parent.mkdir(parents=True,exist_ok=True)
    args.report_output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('fixture_count','components','bit_exact_components',
          'maximum_absolute_difference','context_gate_cases','widescreen_suppression_cases','passed')}))
    assert not errors, errors[:3]
    assert bit_exact == checked, dict(bit_exact=bit_exact,checked=checked,
                                     zero_sign_mismatches=len(zero_sign_mismatches))


if __name__ == '__main__':
    main()
