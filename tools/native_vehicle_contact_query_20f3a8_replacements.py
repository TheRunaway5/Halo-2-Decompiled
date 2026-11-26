"""Bind the original vehicle contact load and query-record preparation stage."""

import re

from native_vehicle_wheel_offset_20e414_replacements import OWNER, digest


INCLUDE = '#include "physics/vehicles/native_tu15_vehicle_contact_query_20f3a8.hpp"\n'
ENTRIES = ('20aaa8', '20aafc', '20ab16')
EXITS = ('20ad31', '20ade0')
STAGE = re.compile(r'^L20aaa8:[\s\S]*?(?=^L20ad31:)', re.M)
RAW_PIN = '2ee2c48438315e1555b0261000f151e3cb0dbc82bed68034bda4ca5691665985'
PREPARED_PIN = 'b3d89dbc438c63d9f30ac6221f5da276ae1df004c88d2894bd62eb10cc6ed61b'
DATA = (('518444', '51df54'), ('4650f4', '46a194'), ('46504c', '46a0ec'),
        ('465030', '46a0d0'), ('46fc50', '474d10'))


def stage_of(source):
    body = OWNER.search(source)
    stage = STAGE.search(body[0]) if body else None
    if not stage:
        raise ValueError('Vehicle 20F3A8 contact query preparation missing')
    return stage, body.start()


def check_routes(canonical):
    stage, _ = stage_of(canonical)
    routes = (
        'case 0x209a50u:return 0x20e350u;', 'case 0x209a50u:run_209a50(s);break;',
        'instruction(0x20a9d1u);} if(s.flags.cf||s.flags.zf)goto L20aaa8;',
        'instruction(0x20aaa6u);} goto L20aafc;',
        'instruction(0x20a685u);} if(!s.flags.pf)goto L20ab16;',
        'case 0x20b2b0u:return 0x20fbb0u;', 'case 0x20b3b0u:return 0x20fcb0u;',
        *(f'dispatch(s,target,0x{pc}u,false);' for pc in ('20ad4c', '20ad85', '20adca')),
        *(f'L{pc}: if(s.services.instruction){{s.services.instruction(0x{pc}u);}}' for pc in EXITS),
        *(f'case 0x{old}u:return 0x{new}u;' for old, new in DATA),
    )
    if digest(stage[0]) != RAW_PIN or any(canonical.count(route) != 1 for route in routes):
        raise ValueError('Vehicle 20F3A8 contact query preparation or adjacent dispatch changed')
    if any(canonical.count(f'goto L{pc};') != 1 for pc in ENTRIES):
        raise ValueError('Vehicle 20F3A8 contact query entry predecessors changed')
    return stage[0]


def prepared_literal(canonical):
    raw = check_routes(canonical)
    literal, count = re.subn(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
                            r's.instruction_literal(\1);', raw)
    for old, new in DATA:
        literal = literal.replace(f'selected(0x{old}u,s.image)', f'(s.image==SourcePhysicsContactImage::MapPack11?0x{old}u:0x{new}u)')
    if count != 137 or digest(literal) != PREPARED_PIN:
        raise ValueError('Vehicle 20F3A8 prepared contact query changed')
    return literal


def reference(canonical):
    literal = prepared_literal(canonical)
    namespace = 'halo2::native_vehicle_contact_query_20f3a8_detail'
    for operation, helper in (
        ('_mm_add_ss', 'native_world_leaves_detail::ordered_add_ss'),
        ('_mm_mul_ss', 'native_tu15_leaves::ordered_mul_ss'),
        ('_mm_sub_ss', 'native_tu15_leaves::ordered_difference_ss'),
        ('_mm_div_ss', 'native_vehicle_wheel_response_20e6c3_detail::ordered_divide_ss'),
    ):
        literal = literal.replace(f'{operation}(', f'halo2::{helper}(')
    for site, offset, pop in (('20aad3', '54', True), ('20aae2', '58', True),
                              ('20aaf8', '5c', True), ('20ab8a', '14', False)):
        original = f'w(s.memory,s.observe,0x{site}u,(0x{offset}u+s.gp[4]),4,u(spill(s.fp.st(0))));'
        original += 's.fp.pop();' if pop else ''
        replacement = f'{namespace}::spill_reference<0x{site}u, 0x{offset}u, {str(pop).lower()}>(s);'
        if literal.count(original) != 1:
            raise ValueError(f'Vehicle contact query reference {site} spill changed')
        literal = literal.replace(original, replacement)
    for original, helper in (('s.fp.st(0)=-s.fp.st(0);', 'negate_load'),
                             ('s.fp.st(0)=std::fabs(s.fp.st(0));', 'absolute_length')):
        if literal.count(original) != 1:
            raise ValueError(f'Vehicle contact query reference {helper} changed')
        literal = literal.replace(original, f'{namespace}::{helper}(s);')
    return literal


def bridge_stage(canonical):
    stage = reference(canonical)
    sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', stage)
    for pc in sites:
        line = re.compile(rf'^([^\n]*instruction_literal\(0x{pc}u\);)', re.M)
        stage, count = line.subn(rf'L_contact_query_fallback_{pc}:\n\1', stage)
        if count != 1:
            raise ValueError(f'Unexpected vehicle contact query continuation {pc}')
    for pc in ENTRIES:
        label = f'L{pc}:'
        label_in_fallback = f'L_contact_query_fallback_{pc}:\n{label} '
        if stage.count(label_in_fallback) != 1:
            raise ValueError(f'Unexpected vehicle contact query entry {pc}')
        # A trivial uninitialized scalar can be crossed by incoming gotos.
        declare = '    std::uint32_t contact_query_entry;\n' if pc == ENTRIES[0] else ''
        stage = stage.replace(label_in_fallback, label + '\n' + declare +
            f'    contact_query_entry=0x{pc}u;\n    goto L_contact_query_dispatch;\n'
            f'L_contact_query_fallback_{pc}:\n', 1)
    stage += f'    goto L{EXITS[0]};\nL_contact_query_dispatch:\n'
    stage += ('    switch (halo2::native_vehicle_contact_query_20f3a8_detail::bridge(s,contact_query_entry)) {\n'
              '    case 0: switch (contact_query_entry) {\n')
    stage += ''.join(f'        case 0x{pc}u: goto L_contact_query_fallback_{pc};\n' for pc in ENTRIES)
    stage += '        default: throw std::logic_error("Unknown vehicle contact query entry");\n    }\n'
    stage += ''.join(f'    case 0x{pc}u: goto L_contact_query_fallback_{pc};\n' for pc in sites)
    stage += ''.join(f'    case 0x{pc}u: goto L{pc};\n' for pc in EXITS)
    stage += '    default: throw std::logic_error("Unknown vehicle contact query continuation");\n    }\n'
    return stage


def install(canonical, emitted):
    literal = prepared_literal(canonical)
    stage, offset = stage_of(emitted)
    bound = bridge_stage(canonical)
    if stage[0] == bound and emitted.count(INCLUDE) == 1:
        return emitted, ['20f3a8']
    if stage[0] != literal or INCLUDE in emitted:
        raise ValueError('Vehicle 20F3A8 emitted contact query boundaries changed')
    return INCLUDE + emitted[:offset+stage.start()] + bound + emitted[offset+stage.end():], ['20f3a8']
