"""Bind the TU1.5 damage update entry in the selected-observation owner."""
import re

import importlib.util
from pathlib import Path


spec = importlib.util.spec_from_file_location(
    'native_world_leaves_replacements', Path(__file__).with_name('native_world_leaves_replacements.py'))
world = importlib.util.module_from_spec(spec)
spec.loader.exec_module(world)

NAME = 'd73e0'
PIN = '998ca941170cccccc6457286bd2d7dfd992d3a0680f3ebbfe9530eca0cfc6c3a'
OWNER_INCLUDE = '#include "halo2/source_damage_delivery_runtime.hpp"\n'
INCLUDE = '#include "gameplay/units/native_tu15_damage_update_entry.hpp"\n'
REFERENCE = 'native_damage_update_reference_d73e0'
NAMESPACE = 'halo2::native_damage_update_entry_detail'
OPERANDS = {0xd740d: 0xd835d, 0xd7160: 0xd80a0, 0x4e78d0: 0x4ecbd0,
            0x4edf18: 0x4f3218, 0x4fcdbc: 0x5020bc}
CONTINUATIONS = {'d7c4f': 'finish', 'd74a0': 'response', 'd75a4': 'validate'}
MARKER = '\nLd74a0: '
SELECTOR = 'return state.selected_observation_pcs && state.image==SourcePhysicsContactImage::TitleUpdate15?tu15:disc;'


def admitted(canonical):
    found = world.body(canonical, NAME)
    if not found or found[1] != 'ObservedDamageState' or world.digest(found) != PIN:
        return False
    if OWNER_INCLUDE not in canonical or canonical.count(SELECTOR) != 1:
        return False
    stage = found[0].split(MARKER)[0]
    used = {int(pc, 16) for pc in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', stage)}
    resolved = world.mapping(canonical)
    return used | {0xd7160} == OPERANDS.keys() and all(resolved.get(key) == pc for key, pc in OPERANDS.items())


def reference(body, replay=True):
    first = body.index('{') + 1
    stage = body[first:body.index(MARKER)]
    for label, continuation in CONTINUATIONS.items():
        stage = stage.replace(f'goto L{label};', f'return {NAMESPACE}::Resume::{continuation};')
    if replay:
        stage = stage.replace('run_d7160(s);', 'halo2::native_object_tick_detail::replay(s,0xd80a0u);')
    suffix = '' if replay else '_live'
    return f'static {NAMESPACE}::Resume {REFERENCE}{suffix}(ObservedDamageState &s) {{{stage}\n}}\n'


def install(canonical, output):
    if not admitted(canonical):
        return output, []
    original = world.body(canonical, NAME)
    found = world.body(output, NAME)
    if not found or REFERENCE in output:
        return output, []
    start = found[0].index('{') + 1 + found.start()
    branches = ''.join(f'    case {NAMESPACE}::Resume::{target}: goto L{label};\n'
                       for label, target in CONTINUATIONS.items())
    hook = (f'\n    // BEGIN native damage update entry\n'
            f'    switch ({NAMESPACE}::bridge(s,{REFERENCE},[](auto &value){{run_d7160(value);}},'
            f'[&](auto key){{return selected(key,s.image);}})) {{\n{branches}'
            f'    case {NAMESPACE}::Resume::retained: break;\n    }}\n'
            f'    // END native damage update entry\n')
    prepared = output[:found.start()] + reference(original[0]) + output[found.start():start] + hook + output[start:]
    return INCLUDE + prepared, ['d8330']
