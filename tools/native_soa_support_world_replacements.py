"""Authenticate the observed World TU1.5 3077F0 SoA support leaf.

The authenticated body uses the observed World state and a seven-target
support dispatch table. The Contact unit carries a distinct body.
"""

import importlib.util
from pathlib import Path
import re


PIN = 'ade6eb5bf5a499943b696791f5e3d19243c05db5cd29208776278e87757dc850'
OPERANDS = {0x303988: 0x307988, 0x3039a4: 0x3079a4}
BRANCHES = {site + 0x4000: site for site in (
    0x3038b0, 0x3038bf, 0x3038dd, 0x3038e6, 0x303904, 0x30391c, 0x303925,
)}
SOURCE_POLICY = 'return state.selected_observation_pcs && state.image==SourcePhysicsContactImage::TitleUpdate15?tu15:disc;'
X87_READ = re.compile(r'ext\(f\((r\(s\.memory,[^,\n]*,4\))\)\)')
INCLUDE = '#include "physics/runtime/native_tu15_soa_support_world.hpp"\n'
PREFIX = (
    '\n    // BEGIN native World SoA support 3077f0\n'
    '    if(halo2::native_soa_support_world_detail::bridge(s,\n'
    '        [](auto &state){run_3037f0(state);},\n'
    '        [&](std::uint32_t target){return canonical_branch(target,s.image);}))return;\n'
    '    // END native World SoA support 3077f0'
)


def install(canonical, output):
    if SOURCE_POLICY not in canonical:
        return output, []

    filename = Path(__file__).with_name('native_world_leaves_replacements.py')
    spec = importlib.util.spec_from_file_location('world_soa_metadata', filename)
    world = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(world)

    found = world.body(canonical, '3037f0')
    if not found or found[1] != world.STATE or world.digest(found) != PIN:
        return output, []
    mapping = world.mapping(canonical)
    if any(mapping.get(old) != new for old, new in OPERANDS.items()):
        return output, []
    policy = re.search(r'^std::uint32_t canonical_branch\(.*$', canonical, re.M)
    if not policy:
        return output, []
    branches = {int(old, 16): int(new, 16) for old, new in
                re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;', policy[0])}
    if any(branches.get(actual) != disc for actual, disc in BRANCHES.items()):
        return output, []

    current = world.body(output, '3037f0')
    if not current or PREFIX in current[0]:
        return output, []
    start = current[0].index('{') + 1
    retained, count = X87_READ.subn(
        r'halo2::native_soa_support_world_detail::load_operand(\1)', world.reference(current[0][start:]))
    if count != 16:
        raise ValueError('World 3077F0 retained x87 operand count drift')
    output = output[:current.start()] + current[0][:start] + PREFIX + retained + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, ['3077f0']
