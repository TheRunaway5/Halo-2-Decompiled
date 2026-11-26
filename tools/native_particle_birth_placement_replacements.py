"""Bind the particle birth property evaluation and placement continuation."""

import importlib.util
from pathlib import Path


ENTRY = '250c60'
SPAN = '250e67'
START = 'L250e67: '
END = 's.instruction_literal(0x25113au);'
CONTINUATION = 'L25113a'
REFERENCE = 'native_particle_birth_placement_reference_' + ENTRY
INCLUDE = '#include "rendering/effects/native_tu15_particle_birth_placement.hpp"\n'
DIRECT = {'176a40': 1, '252480': 1, '145b70': 2}
COLLISION = 'dispatch(s,target,0x25103bu,false);'
ORDERED = {'add': ('native_world_leaves_detail', 17), 'mul': ('native_tu15_leaves', 18)}
ROUTES = ''.join(f'            if constexpr (decltype(child)::value == 0x{target}u) run_{target}(value);\n'
                 for target in DIRECT)
ROUTES += '            if constexpr (decltype(child)::value == 0x16c620u) dispatch(value,0x16c620u,site,false);\n'
HOOK = (
    '\n    // BEGIN native particle birth placement 250e67\n'
    f'    if(halo2::native_particle_birth_placement_detail::bridge(s,{REFERENCE},\n'
    '        [](auto &value,auto child,std::uint32_t site){\n' + ROUTES +
    f'        }}))goto {CONTINUATION};\n'
    '    // END native particle birth placement 250e67\n'
)


def prefix_helper():
    path = Path(__file__).with_name('native_particle_birth_prefix_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_birth_placement_metadata', path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    return helper


def extract(owner):
    start = owner.index(START) + len(START)
    end = owner.index(END, start)
    continuation = owner.find(CONTINUATION + ': ', start, end)
    if continuation != -1:
        end = continuation
    span = owner[start:end].removeprefix(HOOK)
    return start, end, span


def ordered(span):
    for name, (namespace, count) in ORDERED.items():
        original = f'_mm_{name}_ss('
        operation = f'halo2::{namespace}::ordered_{name}_ss('
        if span.count(original) + span.count(operation) != count:
            raise ValueError(f'Particle birth placement {name}ss sites drifted')
        span = span.replace(original, operation)
    return span


def reference(span, replay=True):
    span = ordered(span)
    for target, count in DIRECT.items():
        child = f'run_{target}(s);'
        if span.count(child) != count:
            raise ValueError(f'Particle birth placement child {target} drifted')
        if replay:
            span = span.replace(child, f'halo2::native_world_leaves_detail::replay_child(s,0x{target}u);')
    if span.count(COLLISION) != 1:
        raise ValueError('Particle birth placement collision route drifted')
    if replay:
        span = span.replace(COLLISION, 'halo2::native_world_leaves_detail::replay_child(s,target);', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    return f'static void {name}(WorldState &s) {{\n{span}}}\n'


def install(canonical, output):
    prefix = prefix_helper()
    world = prefix.world_helper()
    if not prefix.admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or HOOK in current[0] or START not in current[0] or END not in current[0]:
        return output, []
    start, end, span = extract(current[0])
    if 'selected(' in span or span.count('s.instruction_literal(') != 174:
        return output, []
    owner = current[0][:start] + HOOK + ordered(span) + CONTINUATION + ': ' + current[0][end:]
    output = output[:current.start()] + reference(span) + '\n' + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [SPAN]
