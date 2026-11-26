"""Bind TU1.5's vehicle-math x87 classification and original tail dispatch."""

from hashlib import sha256
import re


INCLUDE = '#include "physics/vehicles/native_tu15_vehicle_x87_dispatch_331ecb.hpp"\n'
BODY = re.compile(r'^static void run_32e35b\(MathState &s\) \{.*?^\}', re.M | re.S)
CALLER = re.compile(r'^static void run_3799bc\(MathState &s\) \{.*?^\}', re.M | re.S)
PIN = '3d84e39704685bef35ebd357a761bf957c57b01a8849122dc4a97465a60bba7c'
PREPARED_PIN = '9b67c25dca49e7263c43062e0bb6b19266614e27dc22cfdeb9ce4d8df8b3171a'
CALLER_PIN = '946d2027e20302934800563c7ab3f3ef11e4482a5ffd01fe100046a8140bbfb9'
ROUTES = {
    'choose_control': '32e35f', 'alter_saved_control': '32e368',
    'load_control': '32e37d', 'classify_first': '32e383',
    'save_first_status': '32e390', 'clear_error': '32e397',
    'swap_inputs': '32e39e', 'classify_second': '32e3a6',
    'restore_inputs': '32e3af', 'decode_high_status': '32e3b7',
    'keep_high_class': '32e3c2', 'combine_classes': '32e3cf',
}
TAIL = 'dispatch(s,r(s.memory,(0x0u+s.gp[3]),4),0x32e3e5u,true);return;'


def digest(source):
    return sha256(source.encode()).hexdigest()


def original(canonical):
    copies = BODY.findall(canonical)
    caller = CALLER.search(canonical)
    if (len(copies) != 1 or digest(copies[0]) != PIN or not caller or digest(caller[0]) != CALLER_PIN or
            any(canonical.count(text) != 1 for text in (
                'case 0x32e35bu:return 0x331ecbu;',
                'case 0x469bf4u:return 0x46ecb4u;',
                'case 0x3799dcu:return 0x37d48cu;',
                'case 0x32e35bu:run_32e35b(s);break;',
                'if(target==selected(0x32e35bu,s.image)){run_32e35b(s);return;}',
            ))):
        raise ValueError('Vehicle math 331ECB source owner or caller changed')
    if copies[0].count(TAIL) != 1:
        raise ValueError('Vehicle math 331ECB lost its genuine dynamic tail dispatch')
    return copies[0]


def prepared_reference(canonical):
    body = original(canonical)
    body = body.replace('selected(0x469bf4u,s.image)',
                        '(s.image==SourcePhysicsContactImage::MapPack11?0x469bf4u:0x46ecb4u)')
    body, count = re.subn(
        r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
        r's.instruction_literal(\1);', body)
    if count != 45 or digest(body) != PREPARED_PIN:
        raise ValueError('Vehicle math 331ECB retained observation sites changed')
    return body


def install(canonical, emitted):
    reference = prepared_reference(canonical)
    current = BODY.search(emitted)
    if not current or emitted.count(
            'if(target==(s.image==SourcePhysicsContactImage::MapPack11?0x32e35bu:0x331ecbu)){run_32e35b(s);return;}') != 1:
        raise ValueError('Vehicle math 331ECB prepared dispatcher changed')

    body = reference
    for pc in ROUTES.values():
        literal = f'\ns.instruction_literal(0x{pc}u);'
        if body.count(literal) != 1:
            raise ValueError(f'Vehicle math 331ECB continuation {pc} changed')
        body = body.replace(literal, f'\nL_native_{pc}:{literal}', 1)
    namespace = 'halo2::native_vehicle_x87_dispatch_331ecb_detail'
    routes = ''.join(f'        case Continuation::{name}: goto L_native_{pc};\n' for name, pc in ROUTES.items())
    bridge = (f'\n    {{\n        using namespace {namespace};\n        std::uint32_t target;\n'
              '        switch (classify(s, target)) {\n'
              '        case Continuation::original: break;\n'
              '        case Continuation::dispatch: dispatch(s,target,0x32e3e5u,true); return;\n'
              f'{routes}        }}\n    }}')
    start = body.index('{') + 1
    body = body[:start] + bridge + body[start:]
    if current[0] == body and emitted.count(INCLUDE) == 1:
        return emitted, ['331ecb']
    if current[0] != reference or INCLUDE in emitted:
        raise ValueError('Vehicle math 331ECB prepared body changed')
    return INCLUDE + emitted[:current.start()] + body + emitted[current.end():], ['331ecb']
