"""Bind the call-free first wheel transform stage of TU1.5 vehicle motion."""

from hashlib import sha256
import re


INCLUDE = '#include "physics/vehicles/native_tu15_vehicle_wheel_offset_20e414.hpp"\n'
OWNER = re.compile(r'^static void run_209a50\(MotionState &s\) \{\n.*?\n\}(?=\n\nstatic void run_3115f0)', re.M | re.S)
STAGE = re.compile(r'^L209b14:[\s\S]*?(?=^L_wheel_cross_product_entry:|^.*instruction(?:_literal)?\(0x209b81u\))', re.M)
RAW_PIN = '0bf265c2dd2bfac1e006b132225d77a93b4622db1aaef40d9d3669f81ede09b3'
PREPARED_PIN = 'e8639545d0d66cb7ef38947ef9b6c52e469951e5ac7ee2f51c669cca202c0b40'
END = '209b81'


def digest(text):
    return sha256(text.encode()).hexdigest()


def original(canonical):
    body = OWNER.search(canonical)
    stage = STAGE.search(body[0]) if body else None
    if (not stage or digest(stage[0]) != RAW_PIN or body[0].count('goto L209b14;') != 1 or
            any(canonical.count(route) != 1 for route in (
                'case 0x209a50u:return 0x20e350u;',
                'case 0x465030u:return 0x46a0d0u;',
                'case 0x46503cu:return 0x46a0dcu;',
                'case 0x209a50u:run_209a50(s);break;',
            ))):
        raise ValueError('Vehicle 20E414 retained wheel offset or live owner changed')
    return stage[0]


def prepared_reference(canonical):
    stage, count = re.subn(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
        r's.instruction_literal(\1);', original(canonical))
    for old, new in (('465030', '46a0d0'), ('46503c', '46a0dc')):
        stage = stage.replace(f'selected(0x{old}u,s.image)',
            f'(s.image==SourcePhysicsContactImage::MapPack11?0x{old}u:0x{new}u)')
    if count != 19 or digest(stage) != PREPARED_PIN:
        raise ValueError('Vehicle 20E414 prepared wheel offset changed')
    return stage


def bridge_stage(canonical):
    stage = prepared_reference(canonical)
    entry, *sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', stage)
    if entry != '209b14':
        raise ValueError('Vehicle 20E414 wheel loop entry changed')
    routes = ''.join(f'    case 0x{pc}u: goto L_wheel_offset_{pc};\n' for pc in [*sites, END])
    prefix = ('L209b14:\n'
              '    switch (halo2::native_vehicle_wheel_offset_20e414_detail::bridge(s)) {\n'
              '    case 0: break;\n' + routes +
              '    default: throw std::logic_error("Unknown vehicle wheel offset continuation");\n'
              '    }\n')
    stage = stage.removeprefix('L209b14: ')
    for pc in sites:
        original_line = f'\ns.instruction_literal(0x{pc}u);'
        stage = stage.replace(original_line, f'\nL_wheel_offset_{pc}:{original_line}', 1)
    return prefix + stage + f'L_wheel_offset_{END}:\n'


def install(canonical, emitted):
    reference = prepared_reference(canonical)
    body = OWNER.search(emitted)
    stage = STAGE.search(body[0]) if body else None
    if not stage or body[0].count('goto L209b14;') != 1:
        raise ValueError('Vehicle 20E414 prepared wheel loop route changed')
    conversion = bridge_stage(canonical)
    if stage[0] == conversion and emitted.count(INCLUDE) == 1:
        return emitted, ['20e414']
    if stage[0] != reference or INCLUDE in emitted:
        raise ValueError('Vehicle 20E414 prepared wheel loop sites changed')
    start = body.start() + stage.start()
    end = body.start() + stage.end()
    return INCLUDE + emitted[:start] + conversion + emitted[end:], ['20e414']
