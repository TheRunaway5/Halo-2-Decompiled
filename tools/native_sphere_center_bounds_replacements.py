"""Bind the authenticated TU1.5 sphere-center and sphere-bounds leaves."""

import hashlib
import re

from native_mopp_ray_traversal_replacements import function


INCLUDE = '#include "physics/runtime/native_tu15_sphere_center_bounds.hpp"\n'
PINS = {
    'death': {
        '2e8d70': ('51e2677af1933665d5751b08e9e843bceb4b18bbec64a2fe539921b7b347954f',
                   '213cd8b040f7ca7d726fa81b60aba701a9564bac97dd96983406bcfa83a33077'),
        '2e8d90': ('87add1598ab6b93a86d7a8e4edc450f5041b09bf49c9a8fb4456679020f97319',
                   'fff915b81498e68f520cff8f02dc7fdf7b991605166478513be5b1a1ab68a24d'),
    },
    'support': {
        '2e8d70': ('1b1f3ad554675d08a0e4f613651ce646dd8224d2c6bf8a2d4d1ec4531fda0f19',
                   '213cd8b040f7ca7d726fa81b60aba701a9564bac97dd96983406bcfa83a33077'),
    },
}
FRONTIERS = {
    '2e8d70': ('2e8d71', '2e8d73', '2e8d7c', '2e8d7f', '2e8d81', '2e8d82'),
    '2e8d90': ('2e8d91', '2e8d99', '2e8d9c', '2e8d9f', '2e8da2', '2e8da6',
               '2e8dad', '2e8dba', '2e8dbd', '2e8dc4', '2e8dc8', '2e8dca', '2e8dcb'),
}
CALLS = {'2e8d70': 'center(s)', '2e8d90': 'bounds(s,native_single_spill)'}
SPILL = re.compile(r'^std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?^\}',
                   re.M | re.S)
SPILL_PIN = 'e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1'


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def bind(prepared, entry):
    frontiers = FRONTIERS[entry]
    labels = ' '.join(f'case 0x{pc}u: goto L_sphere_leaf_{pc};' for pc in frontiers)
    hook = ('\n    { using namespace halo2::native_sphere_center_bounds_detail; '
            f'switch ({CALLS[entry]}) {{ case 0: break; case complete: return; {labels}'
            ' default: throw std::logic_error("Invalid sphere leaf continuation"); } }')
    opening = prepared.index('{') + 1
    body = prepared[:opening] + hook + prepared[opening:]
    for pc in frontiers:
        boundary = f's.instruction_literal(0x{pc}u);'
        if body.count(boundary) != 1:
            raise ValueError('Missing sphere leaf continuation ' + pc)
        body = body.replace(boundary, f'L_sphere_leaf_{pc}: ' + boundary, 1)
    if entry == '2e8d90':
        # Preserve the XBE destination operand in ADDPS for two NaN inputs.
        add = '_mm_add_ps(s.xmm[1],s.xmm[0])'
        if body.count(add) != 1:
            raise ValueError('Sphere bounds ADDPS operands changed')
        body = body.replace(add, 'halo2::native_tu15_leaves::ordered_add_ps(s.xmm[1],s.xmm[0])', 1)
    return body


def install(canonical, prepared):
    if 'void source_run_death_cycle_runtime_tu15(' in canonical:
        layout = 'death'
        for source in (canonical, prepared):
            helpers = SPILL.findall(source)
            if len(helpers) != 1 or digest(helpers[0]) != SPILL_PIN:
                raise ValueError('Sphere bounds spill helper drift')
    elif 'void source_run_physics_live_support_tu15(' in canonical:
        layout = 'support'
    else:
        raise ValueError('Unknown sphere leaf owner layout')
    if INCLUDE in prepared:
        raise ValueError('Duplicate sphere leaf binding')
    for entry, pins in PINS[layout].items():
        route = f'case 0x{entry}u:run_{entry}(s);break;'
        original = function(canonical, 'run_' + entry)
        retained = function(prepared, 'run_' + entry)
        if (canonical.count(route) != 1 or prepared.count(route) != 1 or
                (digest(original), digest(retained)) != pins or prepared.count(retained) != 1):
            raise ValueError('Sphere leaf owner or continuation drift: ' + entry)
        prepared = prepared.replace(retained, bind(retained, entry), 1)
    return INCLUDE + prepared, list(PINS[layout])
