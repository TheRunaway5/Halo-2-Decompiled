"""Bind the 251C80 entry and forced LOD path, retaining the resolver call."""

import importlib.util
from pathlib import Path


ENTRY = '251c80'
REFERENCE = 'native_particle_location_entry_reference_' + ENTRY
INCLUDE = '#include "rendering/effects/native_tu15_particle_location_entry.hpp"\n'
RESOLVER_CALL = 'run_178070(s);'
SCAN_LABEL = 'L251d00_entry:'
PREFIX = (
    '\n    // BEGIN native particle location entry 251c80\n'
    '    using halo2::native_particle_location_entry_detail::Resume;\n'
    '    switch (halo2::native_particle_location_entry_detail::bridge(s,\n'
    f'        {REFERENCE}, [](auto &value){{run_178070(value);}})) {{\n'
    f'    case Resume::observers: goto {SCAN_LABEL[:-1]};\n'
    '    case Resume::emitters: goto L251e30;\n'
    '    case Resume::retained: break;\n'
    '    }\n'
    '    // END native particle location entry 251c80'
)


def observer_helper():
    path = Path(__file__).with_name('native_particle_observer_lod_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_location_entry_metadata', path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    return helper


def extract(owner, observer):
    first = owner.index('{') + 1
    scan = owner.index(observer.PREFIX) if observer.PREFIX in owner else owner.index(observer.ANCHOR)
    forced_begin = owner.index('L251e23: ', scan)
    forced_end = owner.index('L251e30: ', forced_begin)
    entry = owner[first:scan].removeprefix(PREFIX).removesuffix(SCAN_LABEL + '\n')
    return first, scan, entry, owner[forced_begin:forced_end]


def reference(entry, forced, replay=True):
    if entry.count(RESOLVER_CALL) != 1 or entry.count('goto L251e30;') != 1:
        raise ValueError('Particle location resolver or continuation drifted')
    entry = entry.replace('goto L251e30;', 'return Resume::emitters;', 1)
    if replay:
        entry = entry.replace(RESOLVER_CALL, 'halo2::native_world_leaves_detail::replay_child(s,0x178070u);', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    return ('static halo2::native_particle_location_entry_detail::Resume '
            f'{name}(WorldState &s) {{\n'
            '    using halo2::native_particle_location_entry_detail::Resume;\n'
            f'{entry}    return Resume::observers;\n{forced}    return Resume::emitters;\n}}\n')


def install(canonical, output):
    observer = observer_helper()
    world = observer.world_helper()
    if not observer.admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or PREFIX in current[0]:
        return output, []
    start, scan, entry, forced = extract(current[0], observer)
    if 'selected(' in entry or 'selected(' in forced or entry.count('s.instruction_literal(') != 38:
        return output, []
    if forced.count('s.instruction_literal(') != 2:
        return output, []
    owner = current[0][:start] + PREFIX + current[0][start:scan] + SCAN_LABEL + '\n' + current[0][scan:]
    output = output[:current.start()] + reference(entry, forced) + '\n' + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [ENTRY]
