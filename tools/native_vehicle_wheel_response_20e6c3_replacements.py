"""Bind the wheel's call-free SSE response after its actual TU1.5 child."""

import re

from native_vehicle_wheel_offset_20e414_replacements import OWNER, digest


INCLUDE = '#include "physics/vehicles/native_tu15_vehicle_wheel_response_20e6c3.hpp"\n'
ENTRY = '209dc3'
END = '209f4a'
ACCUMULATE = '209fe6'
REJECTED = '20a019'
LABEL = 'L_wheel_response_entry:'
STAGE = re.compile(r'^(?:L_wheel_response_entry:|[^\n]*instruction(?:_literal)?\(0x209dc3u\))'
                   r'[\s\S]*?(?=^L_wheel_curve_entry:|^[^\n]*instruction(?:_literal)?\(0x209f4au\))', re.M)
RAW_PIN = 'd161beabd19159944c357e225b45032aa5b61a7c83b997e514757977ea463700'
PREPARED_PIN = '337f8b4e3a6b3be04f5b587e3bb68431b267152d1a9a21e50cb9510820ab7961'


def stage_of(source):
    body = OWNER.search(source)
    stage = STAGE.search(body[0]) if body else None
    if not stage:
        raise ValueError('Vehicle 20E6C3 response stage missing')
    return stage, body.start()


def prepared_literal(canonical):
    stage, _ = stage_of(canonical)
    data_mapping = [
        ('4650e0', '46a180'), ('465030', '46a0d0'), ('526254', '52be68'),
        ('518444', '51df54'), ('465048', '46a0e8'),
    ]
    routes = [
        'case 0x209a50u:return 0x20e350u;', 'case 0x209a50u:run_209a50(s);break;',
        'case 0x184250u:return 0x185670u;', 'case 0x209dc3u:return 0x20e6c3u;',
        ('const auto target=selected(0x184250u,s.image);'
         's.push(0x209dbeu,selected(0x209dc3u,s.image));dispatch(s,target,0x209dbeu,false);'),
        *(f'L{pc}: if(s.services.instruction){{s.services.instruction(0x{pc}u);}}'
          for pc in (ACCUMULATE, REJECTED)),
        *(f'case 0x{old}u:return 0x{new}u;' for old, new in data_mapping),
    ]
    if digest(stage[0]) != RAW_PIN or any(canonical.count(part) != 1 for part in routes):
        raise ValueError('Vehicle 20E6C3 source owner, return site or response exit changed')
    text, count = re.subn(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
        r's.instruction_literal(\1);', stage[0])
    for old, new in data_mapping:
        text = text.replace(f'selected(0x{old}u,s.image)', f'(s.image==SourcePhysicsContactImage::MapPack11?0x{old}u:0x{new}u)')
    if count != 95 or digest(text) != PREPARED_PIN:
        raise ValueError('Vehicle 20E6C3 prepared response changed')
    return text


def prepared_reference(canonical):
    text = prepared_literal(canonical)
    for operation, helper in (
        ('_mm_add_ss', 'native_world_leaves_detail::ordered_add_ss'),
        ('_mm_mul_ss', 'native_tu15_leaves::ordered_mul_ss'),
        ('_mm_sub_ss', 'native_tu15_leaves::ordered_difference_ss'),
        ('_mm_div_ss', 'native_vehicle_wheel_response_20e6c3_detail::ordered_divide_ss'),
    ):
        text = text.replace(f'{operation}(', f'halo2::{helper}(')
    return text


def bridge_stage(canonical):
    stage = prepared_reference(canonical)
    entry, *sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', stage)
    if entry != ENTRY:
        raise ValueError('Vehicle 20E6C3 response entry changed')
    routes = ''.join(f'    case 0x{pc}u: goto L_wheel_response_{pc};\n' for pc in [*sites, END])
    routes += ''.join(f'    case 0x{pc}u: goto L{pc};\n' for pc in (ACCUMULATE, REJECTED))
    prefix = (LABEL + '\n'
              '    switch (halo2::native_vehicle_wheel_response_20e6c3_detail::bridge(s)) {\n'
              '    case 0: break;\n' + routes +
              '    default: throw std::logic_error("Unknown vehicle wheel response continuation");\n'
              '    }\n')
    for pc in sites:
        line = re.compile(r'^([^\n]*instruction_literal\(0x' + pc + r'u\);)', re.M)
        stage, count = line.subn(f'L_wheel_response_{pc}:\n' + r'\1', stage)
        if count != 1:
            raise ValueError(f'Unexpected response continuation {pc}')
    return prefix + stage + f'L_wheel_response_{END}:\n'


def install(canonical, emitted):
    reference = prepared_literal(canonical)
    stage, offset = stage_of(emitted)
    bound = bridge_stage(canonical)
    if stage[0] == bound and emitted.count(INCLUDE) == 1:
        return emitted, ['20e6c3']
    if stage[0] != reference or INCLUDE in emitted:
        raise ValueError('Vehicle 20E6C3 emitted response boundaries changed')
    return INCLUDE + emitted[:offset+stage.start()] + bound + emitted[offset+stage.end():], ['20e6c3']
