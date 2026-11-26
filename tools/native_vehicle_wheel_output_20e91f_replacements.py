"""Bind the call-free TU1.5 wheel output and loop update."""

import re

from native_vehicle_wheel_offset_20e414_replacements import OWNER, digest


INCLUDE = '#include "physics/vehicles/native_tu15_vehicle_wheel_output_20e91f.hpp"\n'
ENTRY = '20a01f'
END = '20a12c'
NEXT_WHEEL = '209b14'
LABEL = f'L{ENTRY}:'
STAGE = re.compile(r'^L20a01f:[\s\S]*?(?=^[^\n]*instruction(?:_literal)?\(0x20a12cu\))', re.M)
RAW_PIN = 'b2a45adc592847ae050ab0bacc18a7348eabbdf0ac53c53784b5608b6d63f796'
PREPARED_PIN = 'e8e3ea01a5cb3a7d2c7f0d02cf047e1089cb0711d1f600c739af80ee2ab0d0d9'


def stage_of(source):
    body = OWNER.search(source)
    stage = STAGE.search(body[0]) if body else None
    if not stage:
        raise ValueError('Vehicle 20E91F output stage missing')
    return stage, body.start()


def prepared_literal(canonical):
    stage, _ = stage_of(canonical)
    routes = (
        'case 0x209a50u:return 0x20e350u;', 'case 0x209a50u:run_209a50(s);break;',
        'L209b14: if(s.services.instruction){s.services.instruction(0x209b14u);}',
        'if(s.services.instruction){s.services.instruction(0x20a12cu);} s.set(6,r(s.memory,(0x50u+s.gp[4]),4),4,0);',
        *(f'if(s.services.instruction){{s.services.instruction(0x{pc}u);}} if(s.flags.zf)goto {LABEL[:-1]};'
          for pc in ('209d67', '209d7a', '209da0', '209dae')),
    )
    if digest(stage[0]) != RAW_PIN or any(canonical.count(route) != 1 for route in routes):
        raise ValueError('Vehicle 20E91F source owner or wheel loop route changed')
    literal, count = re.subn(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
                            r's.instruction_literal(\1);', stage[0])
    if count != 57 or digest(literal) != PREPARED_PIN:
        raise ValueError('Vehicle 20E91F prepared output changed')
    return literal


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
    stage = prepared_reference(canonical).removeprefix(f'{LABEL} ')
    entry, *sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', stage)
    if entry != ENTRY:
        raise ValueError('Vehicle 20E91F output entry changed')
    routes = ''.join(f'    case 0x{pc}u: goto L_wheel_output_{pc};\n' for pc in (*sites, END))
    routes += f'    case 0x{NEXT_WHEEL}u: goto L_wheel_output_next_wheel;\n'
    prefix = (LABEL + '\n'
              '    switch (halo2::native_vehicle_wheel_output_20e91f_detail::bridge(s)) {\n'
              '    case 0: break;\n' + routes +
              '    default: throw std::logic_error("Unknown vehicle wheel output continuation");\n'
              '    }\n')
    for pc in sites:
        line = re.compile(r'^([^\n]*instruction_literal\(0x' + pc + r'u\);)', re.M)
        stage, count = line.subn(f'L_wheel_output_{pc}:\n' + r'\1', stage)
        if count != 1:
            raise ValueError(f'Unexpected wheel output continuation {pc}')
    stage = stage.replace(f'if(s.flags.sf!=s.flags.of)goto L{NEXT_WHEEL};',
                          f'if(s.flags.sf==s.flags.of)goto L_wheel_output_{END};', 1)
    return prefix + stage + f'L_wheel_output_next_wheel:\n    goto L{NEXT_WHEEL};\nL_wheel_output_{END}:\n'


def install(canonical, emitted):
    reference = prepared_literal(canonical)
    stage, offset = stage_of(emitted)
    bound = bridge_stage(canonical)
    if stage[0] == bound and emitted.count(INCLUDE) == 1:
        return emitted, ['20e91f']
    if stage[0] != reference or INCLUDE in emitted:
        raise ValueError('Vehicle 20E91F emitted output boundaries changed')
    return INCLUDE + emitted[:offset+stage.start()] + bound + emitted[offset+stage.end():], ['20e91f']
