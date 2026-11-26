"""Bind the particle system's location walk and shared drain in Effect Frame."""

import importlib.util
from pathlib import Path


ENTRY = '1778d0'
SPAN = '177983'
PIN = '77de0abba635d34ce32f687427ec46c1f7f101fe55a04f1626e50eab60b03fd0'
BEGIN_MIDDLE = 'L177985: '
BEGIN_DRAIN = 'L177dd7: '
ENTRY_REFERENCE = 'native_particle_system_entry_reference_' + ENTRY
DRAIN_REFERENCE = 'native_particle_system_drain_reference_' + ENTRY
INCLUDE = '#include "rendering/effects/native_tu15_particle_system_update.hpp"\n'
CHILDREN = ('178290', '178070', '251a60', '13a6b0')
DRAIN = '178240'
DISPATCH = 'dispatch(s,target,0x17796cu,false);'
NAMESPACE = 'halo2::native_particle_system_update_detail'
ROUTES = ''.join(f'            if constexpr (decltype(child)::value == 0x{target}u) run_{target}(value);\n'
                 for target in CHILDREN)
ENTRY_HOOK = (
    '\n    // BEGIN native particle system setup 1778d0\n'
    f'    using {NAMESPACE}::Resume;\n'
    f'    switch ({NAMESPACE}::bridge_entry(s,{ENTRY_REFERENCE},\n'
    '        [](auto &value,auto child){\n' + ROUTES + '        },\n'
    '        [](auto &value,std::uint32_t target,std::uint32_t site){dispatch(value,target,site,false);})) {\n'
    '    case Resume::attachments: goto L177990;\n'
    '    case Resume::drain: goto L177dd7;\n'
    '    case Resume::retained: break;\n'
    '    }\n'
    '    // END native particle system setup 1778d0'
)
DRAIN_HOOK = (
    '\n    // BEGIN native particle system drain 177dd7\n'
    f'    if ({NAMESPACE}::bridge_drain(s,{DRAIN_REFERENCE},\n'
    '        [](auto &value,auto){run_178240(value);}))return;\n'
    '    // END native particle system drain 177dd7\n'
)


def world_helper():
    path = Path(__file__).with_name('native_world_leaves_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_system_update_metadata', path)
    world = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(world)
    return world


def admitted(canonical, world):
    owner = world.body(canonical, ENTRY)
    return (owner and owner[1] == 'WorldState' and 'source_run_effect_particle_frame_tu15(' in canonical
            and world.digest(owner) == PIN)


def extract(owner):
    start = owner.index('{') + 1
    end = owner.index(BEGIN_MIDDLE, start)
    drain = owner.index(BEGIN_DRAIN, end) + len(BEGIN_DRAIN)
    entry = owner[start:end].removeprefix(ENTRY_HOOK)
    tail = owner[drain:].removeprefix(DRAIN_HOOK)
    return start, end, drain, entry, tail


def replay_children(span, children, dispatch=None):
    for target in children:
        child = f'run_{target}(s);'
        if span.count(child) != 1:
            raise ValueError(f'Particle system update child {target} drifted')
        span = span.replace(child, f'halo2::native_world_leaves_detail::replay_child(s,0x{target}u);', 1)
    if dispatch:
        if span.count(dispatch) != 1:
            raise ValueError('Particle system update interface method drifted')
        span = span.replace(dispatch, 'halo2::native_world_leaves_detail::replay_child(s,target);', 1)
    return span


def reference_entry(span, replay=True):
    for label, result in (('L177990', 'attachments'), ('L177dd7', 'drain')):
        branch = f'goto {label};'
        if span.count(branch) != 1:
            raise ValueError(f'Particle system setup route {label} drifted')
        span = span.replace(branch, f'return Resume::{result};', 1)
    if replay:
        span = replay_children(span, CHILDREN, DISPATCH)
    name = ENTRY_REFERENCE if replay else ENTRY_REFERENCE + '_live'
    return (f'static {NAMESPACE}::Resume {name}(WorldState &s) {{\n'
            f'    using {NAMESPACE}::Resume;\n' + span + '}\n')


def reference_drain(span, replay=True):
    if replay:
        span = replay_children(span, (DRAIN,))
    name = DRAIN_REFERENCE if replay else DRAIN_REFERENCE + '_live'
    return f'static void {name}(WorldState &s) {{\n' + span + '\n'


def install(canonical, output):
    world = world_helper()
    if not admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or ENTRY_HOOK in current[0] or BEGIN_MIDDLE not in current[0] or BEGIN_DRAIN not in current[0]:
        return output, []
    start, _, drain, entry, tail = extract(current[0])
    if ('selected(' in entry + tail or entry.count('s.instruction_literal(') != 59 or
            tail.count('s.instruction_literal(') != 26):
        return output, []
    owner = current[0][:start] + ENTRY_HOOK + current[0][start:drain] + DRAIN_HOOK + current[0][drain:]
    output = output[:current.start()] + reference_entry(entry) + '\n' + reference_drain(tail) + '\n' + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [SPAN]
