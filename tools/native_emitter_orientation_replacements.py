"""Bind the optional 250780 orientation continuation before emission."""

import importlib.util
from pathlib import Path


ENTRY = '250780'
SPAN = '2508a5'
START = 's.instruction_literal(0x2508a5u);'
END = 'L25097b: '
REFERENCE = 'native_emitter_orientation_reference_' + ENTRY
INCLUDE = '#include "rendering/effects/native_tu15_emitter_orientation.hpp"\n'
DIRECT = ('2503f0', '145b70', '1c180')
DISPATCH = (('1458a0', '250923'), ('1459b0', '250937'))
ROUTES = ''.join(f'            if constexpr (decltype(child)::value == 0x{target}u) run_{target}(value);\n'
                 for target in DIRECT)
ROUTES += ''.join(f'            if constexpr (decltype(child)::value == 0x{target}u) dispatch(value,0x{target}u,site,false);\n'
                  for target, _ in DISPATCH)
PREFIX = (
    '    // BEGIN native particle emitter orientation 2508a5\n'
    f'    if(halo2::native_emitter_orientation_detail::bridge(s,{REFERENCE},\n'
    '        [](auto &value,auto child,std::uint32_t site){\n' + ROUTES + '        }))goto L25097b;\n'
    '    // END native particle emitter orientation 2508a5\n'
)


def transform_helper():
    path = Path(__file__).with_name('native_emitter_transform_replacements.py')
    spec = importlib.util.spec_from_file_location('emitter_orientation_metadata', path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    return helper


def extract(owner):
    start = owner.index(START)
    end = owner.index(END, start)
    return start, end, owner[start:end]


def ordered(span):
    operation = 'halo2::native_world_leaves_detail::ordered_add_ss('
    if span.count('_mm_add_ss(') + span.count(operation) != 3:
        raise ValueError('Emitter orientation placement ADDSS sites drifted')
    return span.replace('_mm_add_ss(', operation)


def reference(span, replay=True):
    span = ordered(span)
    for target in DIRECT:
        child = f'run_{target}(s);'
        if span.count(child) != (2 if target == '1c180' else 1):
            raise ValueError(f'Emitter orientation child {target} drifted')
        if replay:
            span = span.replace(child, f'halo2::native_world_leaves_detail::replay_child(s,0x{target}u);')
    for target, site in DISPATCH:
        child = f'dispatch(s,target,0x{site}u,false);'
        if span.count(child) != 1:
            raise ValueError(f'Emitter orientation dispatch {target} drifted')
        if replay:
            span = span.replace(child, f'halo2::native_world_leaves_detail::replay_child(s,0x{target}u);', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    return f'static void {name}(WorldState &s) {{\n{span}}}\n'


def install(canonical, output):
    transform = transform_helper()
    world = transform.world_helper()
    if not transform.admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or PREFIX in current[0] or START not in current[0] or END not in current[0]:
        return output, []
    start, end, span = extract(current[0])
    if 'selected(' in span or span.count('s.instruction_literal(') != 60:
        return output, []
    owner = current[0][:start] + PREFIX + ordered(span) + current[0][end:]
    output = output[:current.start()] + reference(span) + '\n' + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [SPAN]
