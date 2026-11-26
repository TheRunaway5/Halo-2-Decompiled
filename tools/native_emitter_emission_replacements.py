"""Bind the shared native emission tail of the 250780 emitter update."""

import importlib.util
from pathlib import Path


ENTRY = '250780'
SPAN = '25097b'
ANCHOR = 'L25097b: '
REFERENCE = 'native_emitter_emission_reference_' + ENTRY
INCLUDE = '#include "rendering/effects/native_tu15_emitter_emission.hpp"\n'
CHILDREN = ('176a20', '250410', '250440', '178070', '250430', '250c60')
ROUTES = ''.join(f'            if constexpr (decltype(child)::value == 0x{target}u) run_{target}(value);\n'
                 for target in CHILDREN)
PREFIX = (
    '\n    // BEGIN native particle emitter emission 25097b\n'
    f'    if(halo2::native_emitter_emission_detail::bridge(s,{REFERENCE},\n'
    '        [](auto &value,auto child){\n' + ROUTES + '        }))return;\n'
    '    // END native particle emitter emission 25097b\n'
)


def transform_helper():
    path = Path(__file__).with_name('native_emitter_transform_replacements.py')
    spec = importlib.util.spec_from_file_location('emitter_emission_metadata', path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    return helper


def extract(owner):
    start = owner.index(ANCHOR) + len(ANCHOR)
    tail = owner[start:]
    if tail.startswith(PREFIX):
        tail = tail[len(PREFIX):].replace(
            'halo2::native_tu15_leaves::ordered_mul_ss(', '_mm_mul_ss(').replace(
            'halo2::native_world_leaves_detail::ordered_add_ss(', '_mm_add_ss(')
    return start, tail


def ordered(tail):
    if tail.count('_mm_mul_ss(') != 1 or tail.count('_mm_add_ss(') != 4:
        raise ValueError('Emitter emission scalar SSE sites drifted')
    return tail.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(').replace(
        '_mm_add_ss(', 'halo2::native_world_leaves_detail::ordered_add_ss(')


def reference(tail, replay=True):
    tail = ordered(tail)
    for target in CHILDREN:
        child = f'run_{target}(s);'
        if tail.count(child) != 1:
            raise ValueError(f'Emitter emission child {target} drifted')
        if replay:
            tail = tail.replace(child, f'halo2::native_world_leaves_detail::replay_child(s,0x{target}u);', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    return f'static void {name}(WorldState &s) {{\n{tail}\n'


def install(canonical, output):
    transform = transform_helper()
    world = transform.world_helper()
    if not transform.admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or PREFIX in current[0] or ANCHOR not in current[0]:
        return output, []
    start, tail = extract(current[0])
    if 'selected(' in tail or tail.count('s.instruction_literal(') != 81:
        return output, []
    owner = current[0][:start] + PREFIX + ordered(tail)
    output = output[:current.start()] + reference(tail) + '\n' + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [SPAN]
