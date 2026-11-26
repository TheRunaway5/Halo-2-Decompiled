"""Bind the call-free Original contact tangent and projected record stage."""

import re

from native_vehicle_wheel_offset_20e414_replacements import OWNER, digest


INCLUDE = '#include "physics/vehicles/native_tu15_vehicle_contact_projection_20ee92.hpp"\n'
ENTRY = '20a592'
END = '20a79a'
CONTINUE = '20a887'
SKIP = '20ab16'
LABEL = f'L{ENTRY}:'
STAGE = re.compile(r'^L20a592:[\s\S]*?(?=^L20a79a:)', re.M)
RAW_PIN = '682424829cecc709479da449a2ec471ec0c56ebe4f534f6b30ce2f54b6fdb3c3'
PREPARED_PIN = '38edeaf4dd3da8e729d8277207758cf05766d1eb11839bc61da6ebcd3482308e'
DATA = (('465030', '46a0d0'), ('465048', '46a0e8'), ('46504c', '46a0ec'))


def stage_of(source):
    body = OWNER.search(source)
    stage = STAGE.search(body[0]) if body else None
    if not stage:
        raise ValueError('Vehicle 20EE92 contact projection missing')
    return stage, body.start()


def prepared_literal(canonical):
    stage, _ = stage_of(canonical)
    routes = (
        'case 0x209a50u:return 0x20e350u;', 'case 0x209a50u:run_209a50(s);break;',
        'case 0x20a450u:return 0x20ed50u;', 'case 0x1d4a00u:return 0x1d92c0u;',
        'case 0x20a58eu:return 0x20ee8eu;',
        'instruction(0x20a3f1u);} if(s.flags.zf)goto L20a592;',
        'instruction(0x20a406u);} if(s.flags.cf||s.flags.zf)goto L20a592;',
        'const auto target=selected(0x1d4a00u,s.image);s.push(0x20a589u,selected(0x20a58eu,s.image));dispatch(s,target,0x20a589u,false);',
        'const auto target=r(s.memory,(0x18u+s.gp[2]),4);s.push(0x20a44du,selected(0x20a450u,s.image));dispatch(s,target,0x20a44du,false);',
        'L20a58e: if(s.services.instruction){s.services.instruction(0x20a58eu);} s.set(6,r(s.memory,(0x24u+s.gp[4]),4),4,0);\nL20a592:',
        *(f'L{pc}: if(s.services.instruction){{s.services.instruction(0x{pc}u);}}' for pc in (END, CONTINUE, SKIP)),
        *(f'case 0x{old}u:return 0x{new}u;' for old, new in DATA),
    )
    if digest(stage[0]) != RAW_PIN or any(canonical.count(route) != 1 for route in routes):
        raise ValueError('Vehicle 20EE92 contact projection or adjacent dispatch changed')
    literal, count = re.subn(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
                            r's.instruction_literal(\1);', stage[0])
    for old, new in DATA:
        literal = literal.replace(f'selected(0x{old}u,s.image)', f'(s.image==SourcePhysicsContactImage::MapPack11?0x{old}u:0x{new}u)')
    if count != 111 or digest(literal) != PREPARED_PIN:
        raise ValueError('Vehicle 20EE92 prepared contact projection changed')
    return literal


def prepared_reference(canonical):
    stage = prepared_literal(canonical)
    namespace = 'halo2::native_vehicle_contact_projection_20ee92_detail'
    for operation, helper in (
        ('_mm_add_ss', 'native_world_leaves_detail::ordered_add_ss'),
        ('_mm_mul_ss', 'native_tu15_leaves::ordered_mul_ss'),
        ('_mm_sub_ss', 'native_tu15_leaves::ordered_difference_ss'),
        ('_mm_div_ss', 'native_vehicle_wheel_response_20e6c3_detail::ordered_divide_ss'),
    ):
        stage = stage.replace(f'{operation}(', f'halo2::{helper}(')
    for old, helper in (
        ('w(s.memory,s.observe,0x20a62au,(0x14u+s.gp[4]),4,u(spill(s.fp.st(0))));', 'store_tangent_length_reference'),
        ('s.fp.st(0)=std::fabs(s.fp.st(0));', 'absolute_tangent_length'),
    ):
        if stage.count(old) != 1:
            raise ValueError(f'Vehicle 20EE92 reference {helper} changed')
        stage = stage.replace(old, f'{namespace}::{helper}(s);')
    return stage


def bridge_stage(canonical):
    stage = prepared_reference(canonical).removeprefix(f'{LABEL} ')
    entry, *sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', stage)
    if entry != ENTRY:
        raise ValueError('Vehicle 20EE92 contact projection entry changed')
    routes = ''.join(f'    case 0x{pc}u: goto L_contact_projection_{pc};\n' for pc in sites)
    routes += ''.join(f'    case 0x{pc}u: goto L{pc};\n' for pc in (END, CONTINUE, SKIP))
    prefix = (LABEL + '\n'
              '    switch (halo2::native_vehicle_contact_projection_20ee92_detail::bridge(s)) {\n'
              '    case 0: break;\n' + routes +
              '    default: throw std::logic_error("Unknown vehicle contact projection continuation");\n'
              '    }\n')
    for pc in sites:
        line = re.compile(r'^([^\n]*instruction_literal\(0x' + pc + r'u\);)', re.M)
        stage, count = line.subn(f'L_contact_projection_{pc}:\n' + r'\1', stage)
        if count != 1:
            raise ValueError(f'Unexpected contact projection continuation {pc}')
    return prefix + stage


def install(canonical, emitted):
    reference = prepared_literal(canonical)
    stage, offset = stage_of(emitted)
    bound = bridge_stage(canonical)
    if stage[0] == bound and emitted.count(INCLUDE) == 1:
        return emitted, ['20ee92']
    if stage[0] != reference or INCLUDE in emitted:
        raise ValueError('Vehicle 20EE92 emitted contact projection boundaries changed')
    return INCLUDE + emitted[:offset+stage.start()] + bound + emitted[offset+stage.end():], ['20ee92']
