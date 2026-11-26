"""Bind the final vehicle contact force, torque and signed projection."""

import re

from native_vehicle_wheel_offset_20e414_replacements import OWNER, digest


INCLUDE = '#include "physics/vehicles/native_tu15_vehicle_contact_output_20f6e0.hpp"\n'
ENTRY = '20ade0'
END = '20b000'
LABEL = f'L{ENTRY}:'
STAGE = re.compile(r'^L20ade0:[\s\S]*?(?=^[^\n]*instruction(?:_literal)?\(0x20aff4u\))', re.M)
RAW_PIN = 'faa574e9fb3ae88132ff567282c7bfba6fba6187aa1de9b38238a999317b2f84'
PREPARED_PIN = '9616ce7461115e1a36e551cfd871b214a5358c9749bf33e783e00490b255c717'


def stage_of(source):
    body = OWNER.search(source)
    stage = STAGE.search(body[0]) if body else None
    if not stage:
        raise ValueError('Vehicle 20F6E0 contact output missing')
    return stage, body.start()


def prepared_literal(canonical):
    stage, _ = stage_of(canonical)
    routes = (
        'case 0x209a50u:return 0x20e350u;', 'case 0x209a50u:run_209a50(s);break;',
        'case 0x465048u:return 0x46a0e8u;', 'case 0x20b3b0u:return 0x20fcb0u;',
        'case 0x20adcfu:return 0x20f6cfu;', 'case 0x20affeu:return 0x20f8feu;',
        'L20b000: if(s.services.instruction){s.services.instruction(0x20b000u);}',
        *('const auto target=selected(0x20b3b0u,s.image);'
          f's.push(0x{site}u,selected(0x{ret}u,s.image));dispatch(s,target,0x{site}u,false);'
          for site, ret in (('20adca', '20adcf'), ('20aff9', '20affe'))),
    )
    if digest(stage[0]) != RAW_PIN or any(canonical.count(route) != 1 for route in routes):
        raise ValueError('Vehicle 20F6E0 contact output or neighboring dispatch changed')
    literal, count = re.subn(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
                            r's.instruction_literal(\1);', stage[0])
    literal = literal.replace('selected(0x465048u,s.image)', '(s.image==SourcePhysicsContactImage::MapPack11?0x465048u:0x46a0e8u)')
    if count != 107 or digest(literal) != PREPARED_PIN:
        raise ValueError('Vehicle 20F6E0 prepared contact output changed')
    return literal


def prepared_reference(canonical):
    stage = prepared_literal(canonical)
    for operation, helper in (
        ('_mm_add_ss', 'native_world_leaves_detail::ordered_add_ss'),
        ('_mm_mul_ss', 'native_tu15_leaves::ordered_mul_ss'),
        ('_mm_sub_ss', 'native_tu15_leaves::ordered_difference_ss'),
    ):
        stage = stage.replace(f'{operation}(', f'halo2::{helper}(')
    spill = 'w(s.memory,s.observe,0x20afd4u,(0x14u+s.gp[4]),4,u(spill(s.fp.st(0))));s.fp.pop();'
    if stage.count(spill) != 1:
        raise ValueError('Vehicle 20F6E0 projection FSTP changed')
    return stage.replace(spill, 'halo2::native_vehicle_contact_output_20f6e0_detail::store_projection_magnitude_reference(s);')


def bridge_stage(canonical):
    stage = prepared_reference(canonical).removeprefix(f'{LABEL} ')
    entry, *sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', stage)
    if entry != ENTRY:
        raise ValueError('Vehicle 20F6E0 contact output entry changed')
    routes = ''.join(f'    case 0x{pc}u: goto L_contact_output_{pc};\n' for pc in sites)
    prefix = (LABEL + '\n'
              '    switch (halo2::native_vehicle_contact_output_20f6e0_detail::bridge(s)) {\n'
              '    case 0: break;\n' + routes +
              f'    case 0x{END}u: goto L{END};\n'
              '    default: throw std::logic_error("Unknown vehicle contact output continuation");\n'
              '    }\n')
    for pc in sites:
        line = re.compile(r'^([^\n]*instruction_literal\(0x' + pc + r'u\);)', re.M)
        stage, count = line.subn(f'L_contact_output_{pc}:\n' + r'\1', stage)
        if count != 1:
            raise ValueError(f'Unexpected contact output continuation {pc}')
    return prefix + stage


def install(canonical, emitted):
    reference = prepared_literal(canonical)
    stage, offset = stage_of(emitted)
    bound = bridge_stage(canonical)
    if stage[0] == bound and emitted.count(INCLUDE) == 1:
        return emitted, ['20f6e0']
    if stage[0] != reference or INCLUDE in emitted:
        raise ValueError('Vehicle 20F6E0 emitted contact output boundaries changed')
    return INCLUDE + emitted[:offset+stage.start()] + bound + emitted[offset+stage.end():], ['20f6e0']
