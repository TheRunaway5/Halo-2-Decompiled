"""Bind the call-free wheel clamp and force preparation in TU1.5 motion."""

import re

from native_vehicle_wheel_offset_20e414_replacements import OWNER, digest


INCLUDE = '#include "physics/vehicles/native_tu15_vehicle_wheel_force_20e4f4.hpp"\n'
ENTRY = '209bf4'
END = '209dbe'
BYPASS = '20a01f'
LABEL = 'L_wheel_force_entry:'
STAGE = re.compile(r'^(?:L_wheel_force_entry:|[^\n]*instruction(?:_literal)?\(0x209bf4u\))'
                   r'[\s\S]*?(?=^[^\n]*instruction(?:_literal)?\(0x209dbeu\))', re.M)
RAW_PIN = 'db487addda186aba030cc500788966298f6ac24885e870f117337c9cb0fac34c'
PREPARED_PIN = 'f9375a6c54f1aafa297876187de7c53248e6038da4c6b4844c967d96b37433a0'
CALL = ('const auto target=selected(0x184250u,s.image);'
        's.push(0x209dbeu,selected(0x209dc3u,s.image));dispatch(s,target,0x209dbeu,false);')


def stage_of(source):
    body = OWNER.search(source)
    stage = STAGE.search(body[0]) if body else None
    if not stage:
        raise ValueError('Vehicle 20E4F4 force stage missing')
    return stage, body.start()


def prepared_literal(canonical):
    stage, _ = stage_of(canonical)
    mapping = [('465418', '46a4b8'), ('4650d8', '46a178'), ('526248', '52be5c')]
    routes = [
        'case 0x209a50u:return 0x20e350u;', 'case 0x209a50u:run_209a50(s);break;',
        'case 0x184250u:return 0x185670u;', 'case 0x209dc3u:return 0x20e6c3u;', CALL,
        'L20a01f: if(s.services.instruction){s.services.instruction(0x20a01fu);}',
        *(f'case 0x{old}u:return 0x{new}u;' for old, new in mapping),
    ]
    if digest(stage[0]) != RAW_PIN or any(canonical.count(part) != 1 for part in routes):
        raise ValueError('Vehicle 20E4F4 source owner or child boundary changed')
    text, count = re.subn(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
        r's.instruction_literal(\1);', stage[0])
    for old, new in mapping:
        text = text.replace(f'selected(0x{old}u,s.image)', f'(s.image==SourcePhysicsContactImage::MapPack11?0x{old}u:0x{new}u)')
    if count != 119 or digest(text) != PREPARED_PIN:
        raise ValueError('Vehicle 20E4F4 prepared force stage changed')
    return text


def prepared_reference(canonical):
    text = prepared_literal(canonical)
    for operation, helper in (
        ('_mm_add_ss', 'native_world_leaves_detail::ordered_add_ss'),
        ('_mm_mul_ss', 'native_tu15_leaves::ordered_mul_ss'),
        ('_mm_sub_ss', 'native_tu15_leaves::ordered_difference_ss'),
    ):
        text = text.replace(f'{operation}(', f'halo2::{helper}(')
    return text


def bridge_stage(canonical):
    stage = prepared_reference(canonical)
    entry, *sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', stage)
    if entry != ENTRY:
        raise ValueError('Vehicle 20E4F4 force entry changed')
    routes = ''.join(f'    case 0x{pc}u: goto L_wheel_force_{pc};\n' for pc in [*sites, END])
    routes += f'    case 0x{BYPASS}u: goto L{BYPASS};\n'
    prefix = (LABEL + '\n'
              '    switch (halo2::native_vehicle_wheel_force_20e4f4_detail::bridge(s)) {\n'
              '    case 0: break;\n' + routes +
              '    default: throw std::logic_error("Unknown vehicle wheel force continuation");\n'
              '    }\n')
    for pc in sites:
        line = re.compile(r'^([^\n]*instruction_literal\(0x' + pc + r'u\);)', re.M)
        stage, count = line.subn(f'L_wheel_force_{pc}:\n' + r'\1', stage)
        if count != 1:
            raise ValueError(f'Unexpected force continuation {pc}')
    return prefix + stage + f'L_wheel_force_{END}:\n'


def install(canonical, emitted):
    reference = prepared_literal(canonical)
    stage, offset = stage_of(emitted)
    bound = bridge_stage(canonical)
    if stage[0] == bound and emitted.count(INCLUDE) == 1:
        return emitted, ['20e4f4']
    if stage[0] != reference or INCLUDE in emitted:
        raise ValueError('Vehicle 20E4F4 emitted force boundaries changed')
    return INCLUDE + emitted[:offset+stage.start()] + bound + emitted[offset+stage.end():], ['20e4f4']
