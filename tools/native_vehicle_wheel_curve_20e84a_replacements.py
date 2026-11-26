"""Bind the TU1.5 vehicle wheel data flow around its real curve call."""

import re

from native_vehicle_wheel_offset_20e414_replacements import OWNER, digest


INCLUDE = '#include "physics/vehicles/native_tu15_vehicle_wheel_curve_20e84a.hpp"\n'
CALL = '209f8d'
RETURN = '209f92'
ACCUMULATE = '209fe6'
RELOAD = '20a019'
END = '20a01f'
PRE_LABEL = 'L_wheel_curve_entry:'
POST_LABEL = 'L_wheel_curve_return:'
PRE = re.compile(r'^(?:L_wheel_curve_entry:|[^\n]*instruction(?:_literal)?\(0x209f4au\))'
                 r'[\s\S]*?(?=^[^\n]*instruction(?:_literal)?\(0x209f8du\))', re.M)
POST = re.compile(r'^(?:L_wheel_curve_return:|[^\n]*instruction(?:_literal)?\(0x209f92u\))'
                  r'[\s\S]*?(?=^L20a01f:)', re.M)
RAW_PIN = ('73775ae1167566bc897eb394a12347f981a8475367b2fb5f44313448aa57e8d5',
           'c4e633e197a183304614f1ba3824e2aa966d82ef5d88b9fc03b76eb74a185225')
PREPARED_PIN = ('2055916fd04b0c6c3494000e268703b230d2b90ca376a1e856137c71ae58fcb0',
                '72c703b3768759cf7eaaf57a29aaf7871b31ebcd5b9e6c08535c1d46517ca805')
DATA = (('465a78', '46ab30'), ('4650f4', '46a194'), ('452000', '456918'), ('465850', '46a908'),
        ('465194', '46a234'), ('465290', '46a330'), ('465030', '46a0d0'), ('46fc5c', '474d1c'))


def stages_of(source):
    body = OWNER.search(source)
    pre, post = (pattern.search(body[0]) if body else None for pattern in (PRE, POST))
    if not pre or not post:
        raise ValueError('Vehicle 20E84A wheel curve stages missing')
    return pre, post, body.start()


def literals(canonical):
    pre, post, _ = stages_of(canonical)
    routes = (
        'case 0x209a50u:return 0x20e350u;', 'case 0x209a50u:run_209a50(s);break;',
        'case 0x17e370u:return 0x17f790u;', 'case 0x209f92u:return 0x20e892u;',
        ('const auto target=selected(0x17e370u,s.image);'
         's.push(0x209f8du,selected(0x209f92u,s.image));dispatch(s,target,0x209f8du,false);'),
        'if(s.flags.cf||s.flags.zf)goto L209fe6;',
        'if(s.flags.zf)goto L20a019;',
        *(f'case 0x{old}u:return 0x{new}u;' for old, new in DATA),
    )
    if tuple(digest(stage[0]) for stage in (pre, post)) != RAW_PIN or any(canonical.count(r) != 1 for r in routes):
        raise ValueError('Vehicle 20E84A original stages or genuine curve call changed')
    stages = []
    for stage, expected_count, expected_hash in zip((pre, post), (18, 24), PREPARED_PIN, strict=True):
        literal, count = re.subn(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
                                r's.instruction_literal(\1);', stage[0])
        for old, new in DATA:
            literal = literal.replace(f'selected(0x{old}u,s.image)', f'(s.image==SourcePhysicsContactImage::MapPack11?0x{old}u:0x{new}u)')
        if count != expected_count or digest(literal) != expected_hash:
            raise ValueError('Vehicle 20E84A prepared wheel curve stage changed')
        stages.append(literal)
    return tuple(stages)


def references(canonical):
    pre, post = literals(canonical)
    spill = 'w(s.memory,s.observe,0x209f84u,(0x14u+s.gp[4]),4,u(spill(s.fp.st(0))));s.fp.pop();'
    if pre.count(spill) != 1:
        raise ValueError('Vehicle 20E84A original FSTP changed')
    pre = pre.replace(spill, 'halo2::native_vehicle_wheel_curve_20e84a_detail::store_curve_argument_reference(s);')
    for operation, helper in (
        ('_mm_add_ss', 'native_world_leaves_detail::ordered_add_ss'),
        ('_mm_mul_ss', 'native_tu15_leaves::ordered_mul_ss'),
        ('_mm_sub_ss', 'native_tu15_leaves::ordered_difference_ss'),
    ):
        post = post.replace(f'{operation}(', f'halo2::{helper}(')
    return pre, post


def prepared_reference(canonical):
    return ''.join(references(canonical))


def stage_switch(label, bridge, continuations, boundary, boundary_label):
    routes = ''.join(f'    case 0x{pc}u: goto L_wheel_curve_{pc};\n' for pc in continuations)
    return (f'{label}\n'
            f'    switch (halo2::native_vehicle_wheel_curve_20e84a_detail::{bridge}(s)) {{\n'
            '    case 0: break;\n' + routes +
            f'    case 0x{boundary}u: goto {boundary_label};\n'
            '    default: throw std::logic_error("Unknown vehicle wheel curve continuation");\n'
            '    }\n')


def bridge_stages(canonical):
    pre, post = references(canonical)
    site_regex = re.compile(r'instruction_literal\(0x([0-9a-f]+)u\)')
    pre_pcs, post_pcs = (site_regex.findall(stage) for stage in (pre, post))
    if pre_pcs[0] != '209f4a' or [post_pcs[i] for i in (0, 12, 23)] != [RETURN, ACCUMULATE, RELOAD]:
        raise ValueError('Vehicle 20E84A wheel curve entries changed')
    pre_labels = [f'L_wheel_curve_{pc}:\n' + line if i else line
                  for i, (pc, line) in enumerate(zip(pre_pcs, pre.splitlines(keepends=True), strict=True))]
    pre = (stage_switch(PRE_LABEL, 'before_curve', pre_pcs[1:], CALL, 'L_wheel_curve_call') +
           ''.join(pre_labels) + 'L_wheel_curve_call:\n')
    routes = {
        RETURN: (POST_LABEL, 'after_curve<>'),
        ACCUMULATE: (f'L{ACCUMULATE}:', 'after_curve<12>'),
        RELOAD: (f'L{RELOAD}:', 'after_curve<23>'),
    }
    output = []
    for i, (pc, line) in enumerate(zip(post_pcs, post.splitlines(keepends=True), strict=True)):
        if pc in routes:
            label, bridge = routes[pc]
            output.append(stage_switch(label, bridge, post_pcs[i+1:], END, f'L{END}'))
            line = line.removeprefix(f'{label} ')
        if i:
            output.append(f'L_wheel_curve_{pc}:\n')
        output.append(line)
    return pre, ''.join(output)


def install(canonical, emitted):
    pre, post, offset = stages_of(emitted)
    expected = literals(canonical)
    bound = bridge_stages(canonical)
    current = (pre[0], post[0])
    if current == bound and emitted.count(INCLUDE) == 1:
        return emitted, ['20e84a']
    if current != expected or INCLUDE in emitted:
        raise ValueError('Vehicle 20E84A emitted wheel curve boundaries changed')
    return (INCLUDE + emitted[:offset+pre.start()] + bound[0] +
            emitted[offset+pre.end():offset+post.start()] + bound[1] + emitted[offset+post.end():], ['20e84a'])
