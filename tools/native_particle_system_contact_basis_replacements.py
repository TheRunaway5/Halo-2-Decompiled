"""Bind the computed contact basis in the 1778D0 particle system update."""

import importlib.util
from pathlib import Path


ENTRY = '1778d0'
SPAN = '177a2f'
START = 'L177a2f: '
END = 'L177ad8: '
REFERENCE = 'native_particle_system_contact_basis_reference_' + ENTRY
INCLUDE = '#include "rendering/effects/native_tu15_particle_system_contact_basis.hpp"\n'
CHILDREN = ('30bd0', '11f9c0')
OPERATION = 'halo2::native_tu15_leaves::ordered_mul_ss('
HOOK = (
    '\n    // BEGIN native particle system computed contact basis 177a2f\n'
    f'    if (halo2::native_particle_system_contact_basis_detail::bridge(s,{REFERENCE},\n'
    '        [](auto &value,auto child){\n'
    '            if constexpr (decltype(child)::value == 0x30bd0u) run_30bd0(value);\n'
    '            if constexpr (decltype(child)::value == 0x11f9c0u) run_11f9c0(value);\n'
    '        }))goto L177ccf;\n'
    '    // END native particle system computed contact basis 177a2f\n'
)


def setup_helper():
    path = Path(__file__).with_name('native_particle_system_update_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_system_contact_basis_metadata', path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    return helper


def extract(owner):
    start = owner.index(START) + len(START)
    end = owner.index(END, start)
    return start, end, owner[start:end].removeprefix(HOOK)


def ordered(span):
    operation = '_mm_mul_ss('
    if span.count(operation) + span.count(OPERATION) != 9:
        raise ValueError('Particle system contact basis MULSS sites drifted')
    return span.replace(operation, OPERATION)


def reference(span, replay=True):
    span = ordered(span)
    if span.count('goto L177ccf;') != 1:
        raise ValueError('Particle system contact basis shared lane route drifted')
    span = span.replace('goto L177ccf;', 'return;')
    if replay:
        span = setup_helper().replay_children(span, CHILDREN)
    name = REFERENCE if replay else REFERENCE + '_live'
    return f'static void {name}(WorldState &s) {{\n' + span + '}\n'


def install(canonical, output):
    setup = setup_helper()
    world = setup.world_helper()
    if not setup.admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or HOOK in current[0] or any(label not in current[0] for label in (START, END)):
        return output, []
    start, end, span = extract(current[0])
    if 'selected(' in span or span.count('s.instruction_literal(') != 35:
        return output, []
    owner = current[0][:start] + HOOK + ordered(span) + current[0][end:]
    output = output[:current.start()] + reference(span) + '\n' + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [SPAN]
