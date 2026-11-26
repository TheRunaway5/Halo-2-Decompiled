"""Bind the TU1.5 effect observer retirement walk in both original callers."""

import re

from native_model_marker_lookup_replacements import function, digest
from native_world_leaves_replacements import mapping


ENTRY = '17e250'
INCLUDE = '#include "rendering/effects/native_tu15_effect_observer_retire_17e250.hpp"\n'
NAMESPACE = 'halo2::native_effect_observer_retire_detail'
PROFILES = {
    'source_run_physics_live_support_tu15': (
        'b4529fcc6379a041650239eaf014190319e4f207953e79d417b879ba3f82b2cf',
        'af2e0ea04a65e9083e1ecbf77228b63a0bcf1efab64a1be76d6c3ea39b366ae5',
        'case 0x17e250u:run_17e250(s);return;',
        '[](WorldState &state) { dispatch(state,0x16ea60u,0x17e25cu,false); }',
    ),
    'source_run_combat_action_tu15': (
        '46c20b8dbc26f7ab68fe5a433c9a887c79caff083073d4a6b2cf564c7c449494',
        '80ff785061af2b59287f94eafd2dd2be6412f118e42dd54d8aef68d7fbc046c3',
        'if(target==selected(0x17e250u,s.image)){run_17e250(s);return;}',
        'run_16ea60',
    ),
}
PUSHES = ('17e250', '17e257', '17e25c', '17e284', '17e285', '17e2af')
RESUMES = (
    '17e251', '17e257', '17e258', '17e261', '17e26c', '17e26f', '17e276',
    '17e285', '17e286', '17e293', '17e2a7', '17e2ab', '17e2b4', '17e2ca',
    '17e2d1', '17e2e5', '17e2fb', '17e2fe', '17e305', '17e316', '17e317',
    '17e318', '17e319',
)


def install(canonical, prepared):
    profile = next((value for name, value in PROFILES.items() if f'void {name}(' in canonical), None)
    if profile is None:
        return prepared, []
    original_pin, prepared_pin, entry_route, first_child = profile
    try:
        original = function(canonical, 'run_17e250')
        owner = function(prepared, 'run_17e250')
    except ValueError:
        return prepared, []
    if (digest(original) != original_pin or digest(owner) != prepared_pin or
            canonical.count(entry_route) != 1 or canonical.count('case 0x17e250u:run_17e250(s);break;') != 1 or
            prepared.count(owner) != 1 or INCLUDE in prepared):
        return prepared, []
    operands = set(re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', original))
    if operands:
        resolved = mapping(canonical)
        if any(resolved.get(int(value, 16)) != int(value, 16) for value in (*operands, ENTRY)):
            return prepared, []

    body = owner
    for site in PUSHES:
        anchor = rf's\.push\((0x{site}u,[^;]+)\);'
        def precise_save(match):
            return ('if(s.image==SourcePhysicsContactImage::TitleUpdate15){'
                    f'halo2::native_two_bone_solve::push_word(s,{match[1]});'
                    f'}}else{{{match[0]}}}')
        body, count = re.subn(anchor, precise_save, body)
        if count != 1:
            raise ValueError(f'Effect observer-retirement stack write {site} changed')
    for site in RESUMES:
        literal = f's.instruction_literal(0x{site}u);'
        pattern = rf'(L{site}: )?{re.escape(literal)}'
        body, count = re.subn(pattern, lambda m: (m[1] or '') + f'L_native_observer_retire_{site}: {literal}', body)
        if count != 1:
            raise ValueError(f'Effect observer-retirement continuation {site} changed')

    hook = (f'\n    switch ({NAMESPACE}::bridge(s,{first_child},run_17e320)) {{\n'
            f'    case {NAMESPACE}::done: return;\n'
            + ''.join(f'    case 0x{pc}u: goto L_native_observer_retire_{pc};\n' for pc in RESUMES)
            + '    default: break;\n    }\n')
    opening = body.index('{') + 1
    return INCLUDE + prepared.replace(owner, body[:opening] + hook + body[opening:], 1), [ENTRY]
