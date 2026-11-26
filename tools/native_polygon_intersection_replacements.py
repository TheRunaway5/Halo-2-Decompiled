"""Bind the full TU1.5 polygon ray owner and preserve its original children."""
import hashlib
import re

BODY = re.compile(r'^static void run_1f6220\(WorldState &s\) \{.*?^\}', re.M | re.S)
PIN = 'eddc2f0dbad6bea3248b8f9092b8c63b773eafda1a901dde402ce9a06e46ca64'
PREPARED = 'ce9c153fa9943d8e9e64450b1eb91f73d6d03cfa89b73a9ec0f617d8442866f0'
HEADER = 'physics/runtime/native_tu15_polygon_intersection.hpp'
INCLUDE = f'#include "{HEADER}"\n'
DETAIL = 'halo2::native_polygon_intersection'
HOOK = (f'\n    if({DETAIL}::bridge(s,[](auto &v){{run_123210(v);}},'
        '[](auto &v){run_243510(v);}))return;\n')


def sha(body):
    return hashlib.sha256(body.encode()).hexdigest()


def retained(body):
    replacements = (
        ('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(', 19),
        ('_mm_add_ss(', 'halo2::native_world_leaves_detail::ordered_add_ss(', 7),
        ('s.fp.st(0)=root(s.fp.st(0));', f'{DETAIL}::x87::square_root(s);', 1),
        ('s.fp.st(0)=std::fabs(s.fp.st(0));', 's.fp.st(0)=std::fabs(s.fp.st(0));s.fp.status&=~0x200u;', 1),
        ('s.compare_and_pop(s.fp.st(1));', f'{DETAIL}::x87::compare_minimum(s);', 1),
        ('s.fp.st(0)=s.fp.st(0);s.fp.pop();', 's.fp.status&=~0x200u;s.fp.pop();', 1),
        ('s.fp.st(1)=add(s.fp.st(1),s.fp.st(0));s.fp.pop();', f'{DETAIL}::x87::sum_components(s);', 2),
        ('w(s.memory,s.observe,0x1f6338u,(0x4u+s.gp[4]),4,u(spill(s.fp.st(0))));',
         f'{DETAIL}::x87::spill_length(s,0x4u+s.gp[4]);', 1),
    )
    for source, destination, count in replacements:
        if body.count(source) != count:
            raise ValueError(f'Prepared 1F6220 operand changed: {source}')
        body = body.replace(source, destination)
    address = r'(r\(s\.memory,\(0x[0-9a-f]+u(?:\+s\.gp\[4\])?\),4\))'
    body, loads = re.subn(r's\.fp\.push\(ext\(f\('+address+r'\)\)\);',
                          rf's.fp.push({DETAIL}::x87::load_single(\1));', body)
    body, squares = re.subn(r's\.fp\.st\(0\)=mul\(s\.fp\.st\(0\),ext\(f\('+address+r'\)\)\);',
                            rf'{DETAIL}::x87::square_component(s,\1);', body)
    body, pushes = re.subn(r'\bs\.push\((0x[0-9a-f]+u,)', r'halo2::native_two_bone_solve::push_word(s,\1', body)
    if (loads, squares, pushes) != (4, 3, 8):
        raise ValueError(f'Prepared 1F6220 memory sites changed: {(loads, squares, pushes)}')
    return body


def bind_body(body):
    opening = body.index('{') + 1
    body = retained(body)
    return body[:opening] + HOOK + body[opening:]


def install(canonical, output):
    original = list(BODY.finditer(canonical))
    if 'void source_run_physics_live_support_tu15(' not in canonical or not original:
        return output, []
    if len(original) != 1 or sha(original[0][0]) != PIN:
        raise ValueError('Canonical 1F6220 owner changed')
    for child in ('123210', '243510'):
        if canonical.count(f'static void run_{child}(WorldState &s) {{') != 1:
            raise ValueError(f'1F6220 child {child} disappeared')
    if canonical.count('case 0x1f6220u:run_1f6220(s);break;') != 1:
        raise ValueError('Canonical 1F6220 dispatcher changed')
    prepared = list(BODY.finditer(output))
    if len(prepared) != 1 or sha(prepared[0][0]) != PREPARED or INCLUDE in output:
        raise ValueError('Prepared 1F6220 owner changed')
    match = prepared[0]
    return INCLUDE + output[:match.start()] + bind_body(match[0]) + output[match.end():], ['1f6220']
