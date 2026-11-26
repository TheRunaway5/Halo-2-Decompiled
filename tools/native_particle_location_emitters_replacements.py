"""Bind the 251E30 particle location emitter continuation in Death Cycle."""

import importlib.util
from pathlib import Path


ENTRY = '251c80'
TAIL = '251e30'
REFERENCE = 'native_particle_location_emitters_reference_' + ENTRY
INCLUDE = '#include "rendering/effects/native_tu15_particle_location_emitters.hpp"\n'
ANCHOR = 'L251e30: '
ALLOCATE = 'dispatch(s,target,0x251e73u,false);'
UPDATE = 'run_250780(s);'
PREFIX = (
    '\n    // BEGIN native particle location emitter tail 251e30\n'
    f'    if(halo2::native_particle_location_emitters_detail::bridge(s,{REFERENCE},\n'
    '        [](auto &value,std::uint32_t target,std::uint32_t site){dispatch(value,target,site,false);},\n'
    '        [](auto &value){run_250780(value);}))return;\n'
    '    // END native particle location emitter tail 251e30\n'
)


def observer_helper():
    path = Path(__file__).with_name('native_particle_observer_lod_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_observer_lod_metadata', path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    return helper


def extract(owner):
    start = owner.index(ANCHOR)
    return start, owner[start + len(ANCHOR):].removeprefix(PREFIX)


def reference(tail, replay=True):
    if tail.count(ALLOCATE) != 1 or tail.count(UPDATE) != 1:
        raise ValueError('Particle location emitter child dispatch drifted')
    if replay:
        play = 'halo2::native_world_leaves_detail::replay_child(s,'
        tail = tail.replace(ALLOCATE, play + 'target);', 1).replace(UPDATE, play + '0x250780u);', 1)
    return f'static void {REFERENCE}{"" if replay else "_live"}(WorldState &s) {{\n{tail}\n'


def install(canonical, output):
    observer_lod = observer_helper()
    world = observer_lod.world_helper()
    if not observer_lod.admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or PREFIX in current[0] or ANCHOR not in current[0]:
        return output, []
    start, tail = extract(current[0])
    if 'selected(' in tail or tail.count('s.instruction_literal(') != 100:
        return output, []
    edge = start + len(ANCHOR)
    owner = current[0][:edge] + PREFIX + current[0][edge:]
    output = output[:current.start()] + reference(tail) + '\n' + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [TAIL]
