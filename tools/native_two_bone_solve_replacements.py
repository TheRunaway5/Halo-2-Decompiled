"""Bind the TU1.5 two-bone solve to its original normalization child."""
import hashlib
import re

BODY = re.compile(r'^static void run_122be0\(WorldState &s\) \{.*?^\}', re.M | re.S)
PIN = '6f1d911e83d4b1d039a2b0df9214530285d2bd4538fb0153f3ade5be26609b6a'
PREPARED = '49dec4abe6e6e14166b143b76625b60e5e75301dfc55937af40f88723fd8512f'
INCLUDE = '#include "physics/runtime/native_tu15_two_bone_solve.hpp"\n'
DETAIL = 'halo2::native_two_bone_solve'
HOOK = f'\n    if({DETAIL}::bridge(s,[](auto &v){{run_30bd0(v);}}))return;\n'


def sha(body):
    return hashlib.sha256(body.encode()).hexdigest()


def retained(body):
    # Keep original SSE destinations for dual NaNs. Host x87 helpers own
    # FLD's denormal status, FSQRT rounding, and FSTP's committed C1/TOP.
    for source, destination, count in (
        ('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(', 69),
        ('_mm_add_ss(', 'halo2::native_world_leaves_detail::ordered_add_ss(', 17),
        ('s.fp.st(0)=root(s.fp.st(0));', f'{DETAIL}::x87::square_root(s);', 4),
    ):
        if body.count(source) != count:
            raise ValueError('Prepared 122BE0 operand count changed')
        body = body.replace(source, destination)
    body, loads = re.subn(
        r's\.fp\.push\(ext\(f\((r\(s\.memory,\(0x[0-9a-f]+u\+s\.gp\[4\]\),4\))\)\)\);',
        rf's.fp.push({DETAIL}::x87::load(\1));', body)
    body, stores = re.subn(
        r'w\(s\.memory,s\.observe,(0x[0-9a-f]+u),(\(0x[0-9a-f]+u\+s\.gp\[4\]\)),4,'
        r'u\(spill\(s\.fp\.st\(0\)\)\)\);s\.fp\.pop\(\);',
        rf'{DETAIL}::x87::store_pop(s,\1,\2);', body)
    body, pushes = re.subn(r'\bs\.push\((0x[0-9a-f]+u,)', rf'{DETAIL}::push_word(s,\1', body)
    transfer = ('const auto x=r(s.memory,s.gp[6],4);w(s.memory,s.observe,0x12313du,s.gp[7],4,x);'
                'const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[6]+=delta;s.gp[7]+=delta;--s.gp[1];')
    if (loads, stores, pushes) != (4, 4, 9) or body.count(transfer) != 1:
        raise ValueError('Prepared 122BE0 memory sites changed')
    return body.replace(transfer, f'{DETAIL}::copy_matrix_word(s);', 1)


def install(canonical, output):
    originals = BODY.findall(canonical)
    if (len(originals) != 1 or sha(originals[0]) != PIN or
            'void source_run_physics_live_support_tu15(' not in canonical):
        return output, []
    if (canonical.count('static void run_30bd0(WorldState &s) {') != 1 or
            canonical.count('case 0x122be0u:run_122be0(s);break;') != 1):
        raise ValueError('122BE0 original dispatcher or child disappeared')
    prepared = list(BODY.finditer(output))
    if len(prepared) != 1 or sha(prepared[0][0]) not in (PIN, PREPARED) or INCLUDE in output:
        raise ValueError('Prepared TU1.5 122BE0 body changed')
    match = prepared[0]
    body = retained(match[0])
    opening = body.index('{') + 1
    return INCLUDE + output[:match.start()] + body[:opening] + HOOK + body[opening:] + output[match.end():], ['122be0']
