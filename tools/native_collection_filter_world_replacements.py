"""Bind the native World collection filter to canonical 1C6C30 / TU1.5 1CBE70."""

import importlib.util
from pathlib import Path
import re


ENTRY = '1cbe70'
NAME = '1c6c30'
PIN = '9342602d312f7144be870509d17146c90088c063d9291a8e353b89d34f214b81'
FILTER_GROUPS_PIN = '12ede73ca2c426398f1012fa9bccbdd529f88531f137baaecac3d2de09b77049'
OPERANDS = {
    0x486550:0x48b860, 0x183c00:0x185020,
    0x1c6c80:0x1cbec0, 0x1c6c8d:0x1cbecd, 0x1c6c9c:0x1cbedc,
    0x1c6cab:0x1cbeeb, 0x1c6cb7:0x1cbef7, 0x1c6d13:0x1cbf53, 0x1c6d63:0x1cbfa3,
}
ROUTES = ('1c6c7d', '1c6c8a', '1c6c99', '1c6ca8', '1c6cb2', '1c6d10')
GROUP_FILTER = 'run_2e5e20(s);'
INCLUDE = '#include "physics/runtime/native_tu15_collection_filter_world.hpp"\n'
REFERENCE = 'native_collection_filter_reference_' + NAME
PREFIX = (
    '\n    // BEGIN native World collection filter 1cbe70\n'
    f'    if(halo2::native_collection_filter_world_detail::bridge(s,{REFERENCE},\n'
    '        [](auto &state,std::uint32_t target,std::uint32_t site){dispatch(state,target,site,false);},\n'
    '        [](auto &state){run_2e5e20(state);}))return;\n'
    '    // END native World collection filter 1cbe70'
)


def world_helper():
    path = Path(__file__).with_name('native_world_leaves_replacements.py')
    spec = importlib.util.spec_from_file_location('collection_filter_world_metadata', path)
    world = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(world)
    return world


def admitted(canonical, world):
    owner = world.body(canonical, NAME)
    groups = world.body(canonical, '2e5e20')
    if not owner or owner[1] != world.STATE or world.digest(owner) != PIN:
        return False
    if not groups or groups[1] != world.STATE or world.digest(groups) != FILTER_GROUPS_PIN:
        return False
    used = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', owner[0])}
    mapping = world.mapping(canonical)
    if used != set(OPERANDS) or any(mapping.get(a) != b for a, b in OPERANDS.items()):
        return False
    return mapping.get(0x2e5e20) == 0x2e98d0


def reference(original):
    body = original.replace(f'static void run_{NAME}(', f'static void {REFERENCE}(', 1)
    replay = 'halo2::native_world_leaves_detail::replay_child(s,'
    for site in ROUTES:
        call = f'dispatch(s,target,0x{site}u,false);'
        if body.count(call) != 1:
            raise ValueError(f'Collection filter reference call site {site} drifted')
        body = body.replace(call, replay + 'target);', 1)
    if body.count(GROUP_FILTER) != 1:
        raise ValueError('Collection filter group call drifted')
    return body.replace(GROUP_FILTER, replay + '0x2e98d0u);', 1)


def install(canonical, output):
    world = world_helper()
    if not admitted(canonical, world):
        return output, []
    current = world.body(output, NAME)
    if not current or PREFIX in current[0]:
        return output, []
    start = current[0].index('{') + 1
    body = current[0][:start] + PREFIX + current[0][start:]
    original = world.body(canonical, NAME)[0]
    output = output[:current.start()] + reference(original) + '\n\n' + body + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [ENTRY]
