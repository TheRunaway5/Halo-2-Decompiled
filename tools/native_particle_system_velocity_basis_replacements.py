"""Bind the fallback velocity basis and shared final lane in 1778D0."""

import importlib.util
from pathlib import Path


ENTRY = '1778d0'
SPAN = '177b7c'
BEGIN = 'L177b7c: '
SHARED = 'L177ccf: '
END = 'L177cd9: '
REFERENCE = 'native_particle_system_velocity_basis_reference_' + ENTRY
LANE_REFERENCE = 'native_particle_system_basis_final_lane_reference_' + ENTRY
NAMESPACE = 'halo2::native_particle_system_velocity_basis_detail'
INCLUDE = '#include "rendering/effects/native_tu15_particle_system_velocity_basis.hpp"\n'
ORDERED_MUL = 'halo2::native_tu15_leaves::ordered_mul_ss('
HOOK = (
    '\n    // BEGIN native particle system velocity basis 177b7c\n'
    f'    if ({NAMESPACE}::bridge_velocity(s,{REFERENCE}))goto L177ccf;\n'
    '    // END native particle system velocity basis 177b7c\n'
)
LANE_HOOK = (
    '\n    // BEGIN native particle system final basis lane 177ccf\n'
    f'    if ({NAMESPACE}::bridge_lane(s,{LANE_REFERENCE}))goto L177cd9;\n'
    '    // END native particle system final basis lane 177ccf\n'
)


def setup_helper():
    path = Path(__file__).with_name('native_particle_system_update_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_system_velocity_basis_metadata', path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    return helper


def extract(owner):
    begin = owner.index(BEGIN) + len(BEGIN)
    shared = owner.index(SHARED, begin)
    lane = shared + len(SHARED)
    end = owner.index(END, lane)
    return begin, shared, lane, end, owner[begin:shared].removeprefix(HOOK), owner[lane:end].removeprefix(LANE_HOOK)


def ordered(velocity):
    operation = '_mm_mul_ss('
    if velocity.count(operation) + velocity.count(ORDERED_MUL) != 9:
        raise ValueError('Particle system velocity basis MULSS sites drifted')
    return velocity.replace(operation, ORDERED_MUL)


def reference(span, lane=False, live=False):
    name = LANE_REFERENCE if lane else REFERENCE
    if live:
        name += '_live'
    return f'static void {name}(WorldState &s) {{\n' + (span if lane else ordered(span)) + '}\n'


def install(canonical, output):
    setup = setup_helper()
    world = setup.world_helper()
    if not setup.admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or HOOK in current[0] or any(label not in current[0] for label in (BEGIN, SHARED, END)):
        return output, []
    begin, shared, lane, end, velocity, tail = extract(current[0])
    if ('selected(' in velocity + tail or velocity.count('s.instruction_literal(') != 87 or
            tail.count('s.instruction_literal(') != 2):
        return output, []
    refs = reference(velocity) + '\n' + reference(tail, lane=True) + '\n'
    owner = (current[0][:begin] + HOOK + ordered(velocity) + current[0][shared:lane]
             + LANE_HOOK + tail + current[0][end:])
    output = output[:current.start()] + refs + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [SPAN]
