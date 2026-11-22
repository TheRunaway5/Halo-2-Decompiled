"""Install TU1.5's native island queries only over their pinned source bodies."""

import hashlib
import re


INCLUDE = '#include "physics/runtime/native_tu15_phantom_island.hpp"\n'
OWNERS = (
    ('1d9ec0', 'MathState', 'f6fc9b9edcf9a197eef7ebe6b73ff28936f847a4d5f2f80bf3413e18b7df5dcd',
     '1d9ec0', 'query_component(s)'),
    ('315c90', 'MathState', '8095b7916042d8935e5c58ac733256c98c1c402d6824b47aea3137f672ce0fa3',
     '315c90', 'query_body(s)'),
    ('3121a0', 'ObservedWorldState', '592b21c958502eeabdf79b373c77b2c42b679a314ca8d5a7a2c859cd806f642c',
     '315c90', 'query_body(s, s.selected_observation_pcs ? 0u : 0x3af0u)'),
    ('315c90', 'WorldState', '38bd0ef9b09825d898bf78eb1351d6f68aa6dea53d276ae98f92474b9f02e5ce',
     '315c90', 'query_body(s)'),
)


def install(canonical, output):
    installed = []
    for name, state, expected, entry, call in OWNERS:
        pattern = re.compile(rf'^(?:static )?void run_{name}\({state} &s\) \{{.*?^\}}', re.M | re.S)
        original = pattern.search(canonical)
        current = pattern.search(output)
        prefix = f'    if (halo2::native_phantom_island_detail::{call}) return;\n'
        if not original or not current:
            continue
        authenticated = original[0].replace(prefix, '', 1)
        if hashlib.sha256(authenticated.encode()).hexdigest() != expected:
            continue
        if prefix not in current[0]:
            start = current.start() + current[0].index('{\n') + 2
            output = output[:start] + prefix + output[start:]
        installed.append(entry)
    if installed and INCLUDE not in output:
        output = INCLUDE + output
    return output, installed
