"""Bind the TU1.5 weapon status frame, magazine loop and final status check."""

import re

from native_model_marker_lookup_replacements import function, digest
from native_world_leaves_replacements import mapping


ORIGINAL_PIN = '3289250c6d3286de2cf957ecd417028e3e1a846e2a3ef86b470d1824bf140482'
PREPARED_PIN = 'dce6c8dc0e38385e30994403d87982e609c9b60e61fe522196bd6c44a8877883'
INCLUDE = '#include "gameplay/weapons/native_tu15_weapon_hud_status_1055f0.hpp"\n'
NAMESPACE = 'halo2::native_weapon_hud_status_detail'
IMAGE = 's.image==SourcePhysicsContactImage::TitleUpdate15'
STACK_WRITES = ('105602', '105603', '10560b', '105634', '105736', '105738', '10573d')
PHASES = (
    ('1055f0', 'frame.begin()', (
        '1055fd', '105603', '105604', '10560b', '10560c', '105614', '105617', '10561c', '105623', '105625',
        '105632', '105635', '10563c', '10563e', '105644', '105647', '10564d', '105650', '105657', '105663',
        '105668', '10566e', '105675', '10567b', '105683', '10568a', '10568e', '105694', '10569c',
        '1056a0', '1056a4', '1056a8',
    ), ('1056b0', '105774')),
    ('1056b0', 'frame.magazines([](WorldState &v) { dispatch(v,0x1059c0u,0x10573du,false); })', (
        '1056b6', '1056bc', '1056bf', '1056ca', '1056ce', '1056db', '1056e2', '1056ef',
        '1056f6', '1056ff', '105703', '10570d', '105713', '105716', '10571a', '105722',
        '10572b', '105732', '105736', '105738', '105739', '10573d', '105742', '105746',
        '10574a', '10574e', '105752', '105757', '10575b', '105761', '105765',
    ), ('105774',)),
    ('1058e6', 'frame.finish()', (
        '1058ec', '1058ef', '1058f3', '1058f5', '1058fb', '105908', '10590e', '105918', '10591e',
        '10592b', '105932', '105938', '105939', '10593a', '10593b', '10593c',
    ), ()),
)
REPEAT = ('while(s.gp[1]){w(s.memory,s.observe,0x10563cu,s.gp[7],4,s.gp[0]);'
          'const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;--s.gp[1];'
          'if(s.services.instruction)s.services.instruction(0x10563cu);}')
REPEAT_TU15 = (f'while(s.gp[1]){{{NAMESPACE}::clear_word(s);'
               'if(s.gp[1])s.instruction_literal(0x10563cu);}')


def bind_phase(body, entry, method, resumes, transfers, bridge_name='bridge'):
    for pc in resumes:
        literal = f's.instruction_literal(0x{pc}u);'
        pattern = rf'(?m)^(L{pc}: )?{re.escape(literal)}'
        body, count = re.subn(pattern, lambda m: (m[1] or '') + f'L_native_weapon_status_{pc}: {literal}', body)
        if count != 1:
            raise ValueError(f'Original weapon status continuation {pc} changed')
    label = '' if entry == '1055f0' else f'L{entry}: '
    anchor = f'{label}s.instruction_literal(0x{entry}u);'
    if body.count(anchor) != 1:
        raise ValueError(f'Original weapon status entry {entry} changed')
    cases = ''.join(f'    case 0x{pc}u: goto L_native_weapon_status_{pc};\n' for pc in resumes)
    cases += ''.join(f'    case 0x{pc}u: goto L{pc};\n' for pc in transfers)
    hook = (f'{label}\n'
            f'    switch ({NAMESPACE}::{bridge_name}(s,[](auto frame) {{ return {method}; }})) {{\n'
            f'    case {NAMESPACE}::done: return;\n'
            + cases + '    default: break;\n    }\n'
            f's.instruction_literal(0x{entry}u);')
    return body.replace(anchor, hook, 1)


def install(canonical, prepared):
    if 'void source_run_weapon_hud_tu15(' not in canonical or INCLUDE in prepared:
        return prepared, False
    try:
        original = function(canonical, 'run_1055f0')
        owner = function(prepared, 'run_1055f0')
    except ValueError:
        return prepared, False
    if (digest(original) != ORIGINAL_PIN or digest(owner) != PREPARED_PIN or prepared.count(owner) != 1 or
            canonical.count('case 0x1055f0u:run_1055f0(s);break;') != 1 or
            canonical.count('if(target==selected(0x1055f0u,s.image)){run_1055f0(s);return;}') != 1):
        return prepared, False
    operands = {int(x, 16) for x in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', original)}
    resolved = mapping(canonical)
    if any(resolved.get(x) != x for x in operands | {0x1055f0}):
        return prepared, False

    body = owner
    if body.count(REPEAT) != 1:
        raise ValueError('Original weapon status repeat store changed')
    body = body.replace(REPEAT, f'if({IMAGE}){{{REPEAT_TU15}}}else{{{REPEAT}}}', 1)
    for pc in STACK_WRITES:
        pattern = rf's\.push\((0x{pc}u,[^;]+)\);'
        def push(m):
            return f'if({IMAGE}){{halo2::native_two_bone_solve::push_word(s,{m[1]});}}else{{{m[0]}}}'
        body, count = re.subn(pattern, push, body)
        if count != 1:
            raise ValueError(f'Original weapon status stack write {pc} changed')
    for phase in PHASES:
        body = bind_phase(body, *phase)
    return INCLUDE + prepared.replace(owner, body, 1), True
