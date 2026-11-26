"""Bind the World sweep shape support adapter to its authenticated owner."""

import importlib.util
from pathlib import Path
import re


NAME = '2e22b0'
ENTRY = '2e5db0'
PIN = 'e6ee0f022274265f3ced0af57a9733e13d54756e348be62b33dbe03837507a76'
OPERANDS = {0x2e22cc: 0x2e5dcc, 0x465048: 0x46a0e8}
ROUTE = 'dispatch(s,target,0x2e22c9u,false);'
REFERENCE = 'native_swept_support_reference_' + NAME
INCLUDE = '#include "physics/runtime/native_tu15_swept_support_world.hpp"\n'
PREFIX = (
    '\n    // BEGIN native World swept support 2e5db0\n'
    f'    if(halo2::native_swept_support_world_detail::bridge(s,{REFERENCE},\n'
    '        [](auto &state,std::uint32_t target,std::uint32_t site){dispatch(state,target,site,false);}))return;\n'
    '    // END native World swept support 2e5db0'
)


def world_helper():
    path = Path(__file__).with_name('native_world_leaves_replacements.py')
    spec = importlib.util.spec_from_file_location('swept_world_metadata', path)
    world = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(world)
    return world


def admitted(canonical, world):
    found = world.body(canonical, NAME)
    if not found or found[1] != world.STATE or world.digest(found) != PIN:
        return False
    if found[0].count(ROUTE) != 1:
        return False
    used_operands = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
    mapping = world.mapping(canonical)
    return used_operands == set(OPERANDS) and all(mapping.get(old) == new for old, new in OPERANDS.items())


def ordered(text, world):
    return world.reference(text).replace('_mm_add_ss(', 'halo2::native_world_leaves_detail::ordered_add_ss(')


def reference(text, world):
    text = ordered(text, world)
    return text.replace(f'static void run_{NAME}(', f'void {REFERENCE}(', 1).replace(
        ROUTE, 'halo2::native_world_leaves_detail::replay_child(s,target);', 1)


def install(canonical, output):
    world = world_helper()
    if not admitted(canonical, world):
        return output, []
    current = world.body(output, NAME)
    if not current or PREFIX in current[0] or current[0].count(ROUTE) != 1:
        return output, []
    start = current[0].index('{') + 1
    original = world.body(canonical, NAME)[0]
    body = current[0][:start] + PREFIX + ordered(current[0][start:], world)
    output = output[:current.start()] + reference(original, world) + '\n\n' + body + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [ENTRY]
