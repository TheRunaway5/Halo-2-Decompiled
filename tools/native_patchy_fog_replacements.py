"""Install the complete campaign environment patchy-fog accumulator."""
import hashlib
import re


OWNER_PIN = 'c5c0be3fe987c5068a661ffcba2e86a6d13b9a5df2befabdd342416de6872326'
INCLUDE = '#include "campaign/native_tu15_patchy_fog.hpp"\n'
REFERENCE = 'native_patchy_fog_reference_1342b0'
ADMISSION = f'    if(halo2::native_patchy_fog_detail::bridge(s,{REFERENCE}))return;\n'
CALLS = ('s.push(0x131f9du,0x131fa2u);run_1342b0(s);',
         's.push(0x1324afu,0x1324b4u);run_1342b0(s);')
CURSOR = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')
ORDERS = {
    '_mm_mul_ss': 'halo2::native_tu15_leaves::ordered_mul_ss',
    '_mm_add_ss': 'halo2::native_world_leaves_detail::ordered_add_ss',
}


def body(text):
    return re.search(r'static void run_1342b0\(WorldState &s\) \{.*?\n\}', text, re.S)


def pinned(original):
    working = original
    for intrinsic, ordered in ORDERS.items():
        working, count = re.subn(rf'\b{intrinsic}\(', f'{ordered}(', working)
        if count != 11:
            raise ValueError(f'1342B0 {intrinsic} sites changed')
    return working


def source(canonical):
    owner = body(canonical)
    if not owner:
        raise ValueError('Patchy fog 1342B0 owner not found')
    normalized = owner[0].replace('WorldState &s', 'STATE &s')
    if hashlib.sha256(normalized.encode()).hexdigest() != OWNER_PIN:
        raise ValueError('Patchy fog 1342B0 source changed')
    if any(canonical.count(call) != 1 for call in CALLS):
        raise ValueError('Patchy fog 1342B0 caller changed')
    return owner[0]


def reference_body(canonical):
    return pinned(source(canonical)).replace('run_1342b0', REFERENCE, 1) + '\n'


def install(canonical, output):
    original = source(canonical)
    current = body(output)
    if not current or any(output.count(call) != 1 for call in CALLS):
        raise ValueError('Specialized patchy fog 1342B0 source or caller changed')
    specialized = CURSOR.sub(r's.instruction_literal(\1);', original)
    reference = reference_body(canonical)
    first_line, statements = pinned(specialized).split('\n', 1)
    installed = first_line + '\n' + ADMISSION + statements
    if REFERENCE in output:
        if output.count(INCLUDE) != 1 or output.count(reference) != 1 or current[0] != installed:
            raise ValueError('Prepared patchy fog 1342B0 binding changed')
        return output, ['1342b0']
    if current[0] != specialized or INCLUDE in output:
        raise ValueError('Specialized patchy fog 1342B0 body changed')
    return INCLUDE + output[:current.start()] + reference + '\n' + installed + output[current.end():], ['1342b0']
