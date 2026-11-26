"""Bind the hot observer LOD scan inside the 251C80 death-cycle owner."""

import importlib.util
from pathlib import Path
import re


PIN = '7d0f1a2c368b7ef6ed54c8bb93d11c5cbe54c684924e299e0427e16197b19042'
ENTRY = '251c80'
SCAN = '251d00'
RESUME = '251e30'
REFERENCE = 'native_particle_observer_lod_reference_' + ENTRY
ANCHOR = 'L251d00: '
PREFIX = ('    // BEGIN native particle observer LOD 251d00\n'
          f'    if(halo2::native_particle_observer_lod_detail::bridge(s,{REFERENCE}))goto L{RESUME};\n'
          '    // END native particle observer LOD 251d00\n')
INCLUDE = '#include "rendering/effects/native_tu15_particle_observer_lod.hpp"\n'


def world_helper():
    path = Path(__file__).with_name('native_world_leaves_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_observer_lod_metadata', path)
    world = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(world)
    return world


def extract(owner):
    start = owner.index(ANCHOR)
    end = owner.index('L251e23: ', start)
    scan = owner[start:end]
    if scan.count(f'goto L{RESUME};') != 1 or not scan.endswith(f'goto L{RESUME};\n'):
        raise ValueError('Particle observer LOD continuation boundary drifted')
    return start, end, scan


def reference(scan):
    body = scan.replace(f'goto L{RESUME};', 'return;', 1)
    return f'static void {REFERENCE}(WorldState &s) {{\n{body}}}\n'


def admitted(canonical, world):
    body = world.body(canonical, ENTRY)
    return body and body[1] == 'WorldState' and world.digest(body) == PIN


def install(canonical, output):
    world = world_helper()
    if not admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or PREFIX in current[0]:
        return output, []
    start, _, scan = extract(current[0])
    if 'selected(' in scan or scan.count('s.instruction_literal(') != 94:
        return output, []
    branches = set(re.findall(r'goto (L[0-9a-f]+);', scan))
    labels = set(re.findall(r'^(L[0-9a-f]+):', scan, re.M))
    if branches - labels != {f'L{RESUME}'}:
        return output, []
    owner = current[0][:start] + PREFIX + current[0][start:]
    output = output[:current.start()] + reference(scan) + '\n' + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [SCAN]
