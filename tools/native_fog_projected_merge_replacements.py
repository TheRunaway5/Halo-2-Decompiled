"""Bind the fog merge immediately following the real 133B40 projection call."""
import re

from native_fog_resolution_replacements import body, raw_owner


INCLUDE = '#include "campaign/native_tu15_fog_projected_merge.hpp"\n'
NAMESPACE = 'halo2::native_fog_projected_merge_detail'
REFERENCE = 'native_fog_projected_merge_reference_131ac0'
BEFORE_CALL = 's.push(0x132183u,0x132188u);run_133b40(s);'
AFTER_CALL = 's.push(0x1324afu,0x1324b4u);run_1342b0(s);'
FIRST = '132188'
PATCHY = '132492'
BLOOM = '1324d9'
CURSOR = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')


def instruction(pc, specialized):
    return f's.instruction_literal(0x{pc}u);' if specialized else f'if(s.services.instruction){{s.services.instruction(0x{pc}u);}}'


def split(owner, specialized, prepared=False):
    head = '\nL' + FIRST + ':' if prepared else '\n' + instruction(FIRST, specialized)
    tail = '\n' + ('L' + PATCHY + ':' if prepared else '') + instruction(PATCHY, specialized)
    if owner.count(head) != 1 or owner.count(tail) != 1:
        raise ValueError('131AC0 projected fog merge boundaries changed')
    before, after = owner.split(head, 1)
    selected, after = after.split(tail, 1)
    return before, head + selected, tail + after


def pinned(selected):
    for intrinsic, operation, expected in (
            ('_mm_mul_ss', 'halo2::native_tu15_leaves::ordered_mul_ss', 21),
            ('_mm_add_ss', 'halo2::native_world_leaves_detail::ordered_add_ss', 22)):
        selected, count = re.subn(rf'\b{intrinsic}\(', f'{operation}(', selected)
        if count != expected:
            raise ValueError(f'131AC0 projected fog merge {intrinsic} sites changed')
    return selected


def reference_body(canonical):
    _, selected, _ = split(raw_owner(canonical), False)
    selected, count = re.subn(r'goto L1324d9;', f'return {NAMESPACE}::Next::bloom;', selected)
    if count != 3:
        raise ValueError('131AC0 projected fog merge bloom edges changed')
    return (f'static {NAMESPACE}::Next {REFERENCE}(WorldState &s) {{'
            + pinned(selected) + f'\nreturn {NAMESPACE}::Next::patchy_fog;\n}}\n')


def install(canonical, emitted):
    original = raw_owner(canonical)
    current = body(emitted)
    if not current or any(current[0].count(route) != 1 or original.count(route) != 1 for route in (BEFORE_CALL, AFTER_CALL)):
        raise ValueError('131AC0 projected fog merge requires the original geometry and patchy fog calls')
    reference = reference_body(canonical)
    _, raw, _ = split(original, False)
    expected = CURSOR.sub(r's.instruction_literal(\1);', raw)
    installed = (f'\nL{FIRST}:\n    switch({NAMESPACE}::bridge(s,{REFERENCE})) {{\n'
                 + f'        case {NAMESPACE}::Next::patchy_fog:goto L{PATCHY};\n'
                 + f'        case {NAMESPACE}::Next::bloom:goto L{BLOOM};\n'
                 + f'        case {NAMESPACE}::Next::fallback:break;\n'
                 + '    }\n' + pinned(expected).removeprefix('\n'))
    prepared = REFERENCE in emitted
    before, actual, after = split(current[0], True, prepared)
    if not before.endswith(BEFORE_CALL):
        raise ValueError('131AC0 projected fog merge is no longer immediately after the true child')
    if prepared:
        if emitted.count(INCLUDE) != 1 or emitted.count(reference) != 1 or actual != installed:
            raise ValueError('Prepared 131AC0 projected fog merge binding changed')
        return emitted, ['132188']
    if actual != expected or INCLUDE in emitted:
        raise ValueError('Specialized 131AC0 projected fog merge body changed')
    return (INCLUDE + emitted[:current.start()] + reference + '\n' + before + installed
            + f'\nL{PATCHY}:' + after.removeprefix('\n') + emitted[current.end():], ['132188'])
