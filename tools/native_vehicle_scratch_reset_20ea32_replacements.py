"""Bind the TU1.5 vehicle scratch reset after the first wheel loop."""

import re

from native_vehicle_wheel_offset_20e414_replacements import OWNER, digest


INCLUDE = '#include "physics/vehicles/native_tu15_vehicle_scratch_reset_20ea32.hpp"\n'
ENTRY = '20a132'
END = '20a164'
BYPASS = '20a17d'
LABEL = f'L{ENTRY}:'
STAGE = re.compile(r'^L20a132:[\s\S]*?(?=^[^\n]*instruction(?:_literal)?\(0x20a164u\))', re.M)
RAW_PIN = '973161fcacbc905dc76dcce351c3ef94397037b1eaf101b2f0595168da5ac0af'
PREPARED_PIN = 'f2334d6d543f308d61c1f4696f79cd24f3436d9281f52ac7467fbba2cd95d028'


def stage_of(source):
    body = OWNER.search(source)
    stage = STAGE.search(body[0]) if body else None
    if not stage:
        raise ValueError('Vehicle 20EA32 scratch reset missing')
    return stage, body.start()


def prepared_reference(canonical):
    stage, _ = stage_of(canonical)
    routes = (
        'case 0x209a50u:return 0x20e350u;', 'case 0x209a50u:run_209a50(s);break;',
        'if(s.services.instruction){s.services.instruction(0x209afeu);} if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L20a132;',
        'if(s.services.instruction){s.services.instruction(0x20a130u);} s.set(3,s.flags.logic((s.reg(3,4,0))^(s.reg(3,4,0)),4),4,0);\nL20a132:',
        'case 0x183c50u:return 0x185070u;', 'case 0x20a17du:return 0x20ea7du;',
        'const auto target=selected(0x183c50u,s.image);s.push(0x20a178u,selected(0x20a17du,s.image));dispatch(s,target,0x20a178u,false);',
        'L20a17d: if(s.services.instruction){s.services.instruction(0x20a17du);}',
    )
    if digest(stage[0]) != RAW_PIN or any(canonical.count(route) != 1 for route in routes):
        raise ValueError('Vehicle 20EA32 scratch reset or child boundary changed')
    literal, count = re.subn(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
                            r's.instruction_literal(\1);', stage[0])
    if count != 13 or digest(literal) != PREPARED_PIN:
        raise ValueError('Vehicle 20EA32 prepared scratch reset changed')
    return literal


def bridge_stage(canonical):
    stage = prepared_reference(canonical).removeprefix(f'{LABEL} ')
    entry, *sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', stage)
    if entry != ENTRY:
        raise ValueError('Vehicle 20EA32 scratch reset entry changed')
    routes = ''.join(f'    case 0x{pc}u: goto L_vehicle_scratch_reset_{pc};\n' for pc in (*sites, END))
    routes += f'    case 0x{BYPASS}u: goto L{BYPASS};\n'
    prefix = (LABEL + '\n'
              '    switch (halo2::native_vehicle_scratch_reset_20ea32_detail::bridge(s)) {\n'
              '    case 0: break;\n' + routes +
              '    default: throw std::logic_error("Unknown vehicle scratch reset continuation");\n'
              '    }\n')
    for pc in sites:
        line = re.compile(r'^([^\n]*instruction_literal\(0x' + pc + r'u\);)', re.M)
        stage, count = line.subn(f'L_vehicle_scratch_reset_{pc}:\n' + r'\1', stage)
        if count != 1:
            raise ValueError(f'Unexpected vehicle scratch reset continuation {pc}')
    return prefix + stage + f'L_vehicle_scratch_reset_{END}:\n'


def install(canonical, emitted):
    reference = prepared_reference(canonical)
    stage, offset = stage_of(emitted)
    bound = bridge_stage(canonical)
    if stage[0] == bound and emitted.count(INCLUDE) == 1:
        return emitted, ['20ea32']
    if stage[0] != reference or INCLUDE in emitted:
        raise ValueError('Vehicle 20EA32 emitted scratch reset boundaries changed')
    return INCLUDE + emitted[:offset+stage.start()] + bound + emitted[offset+stage.end():], ['20ea32']
