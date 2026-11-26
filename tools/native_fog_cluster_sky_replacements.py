"""Bind the campaign fog cluster initialization and the sky projection gate."""
from native_fog_projected_merge_replacements import CURSOR, instruction
from native_fog_resolution_replacements import body, raw_owner


INCLUDE = '#include "campaign/native_tu15_fog_cluster_sky.hpp"\n'
NAMESPACE = 'halo2::native_fog_cluster_sky_detail'
CLUSTER_CALL = 's.push(0x131c5du,0x131c62u);run_133a70(s);'
GEOMETRY_CALL = 's.push(0x132183u,0x132188u);run_133b40(s);'
# First label, first retained instruction, fallthrough, other continuations.
STAGES = (
    ('cluster', '131c31', '131c89', 'environment', (('1320e5', 'sky', 1),)),
    ('sky', '1320e5', '132183', 'project', (
        ('132597', 'missing_sky', 1), ('13208c', 'uncovered', 2))),
)


def symbol(stage):
    return f'native_fog_cluster_sky_{stage}_reference_131ac0'


def split(owner, stage, specialized, prepared=False):
    _, first, last, _, _ = stage
    head = f'\nL{first}:'
    tail = '\n' + (f'L{last}:' if prepared else '') + instruction(last, specialized)
    if owner.count(head) != 1 or owner.count(tail) != 1:
        raise ValueError(f'131AC0 fog cluster sky boundaries changed at {first}')
    before, section = owner.split(head, 1)
    selected, after = section.split(tail, 1)
    return before + head, selected, tail + after


def reference_body(canonical, stage, direct_child=False):
    name, first, _, fallthrough, continuations = stage
    _, selected, _ = split(raw_owner(canonical), stage, False)
    for target, destination, count in continuations:
        jump = f'goto L{target};'
        if selected.count(jump) != count:
            raise ValueError(f'131AC0 fog cluster sky edges changed at {first}')
        selected = selected.replace(jump, f'return {NAMESPACE}::Next::{destination};')
    if name == 'cluster' and not direct_child:
        selected = selected.replace('run_133a70(s);', 'halo2::native_object_tick_detail::replay(s,0x133a70u);', 1)
    return (f'static {NAMESPACE}::Next {symbol(name)}(WorldState &s) {{\n'
            + selected.lstrip(' ') + f'\nreturn {NAMESPACE}::Next::{fallthrough};\n}}\n')


def install(canonical, emitted):
    original = raw_owner(canonical)
    current = body(emitted)
    if not current or any(current[0].count(route) != 1 or original.count(route) != 1
                          for route in (CLUSTER_CALL, GEOMETRY_CALL)):
        raise ValueError('131AC0 fog cluster sky requires the two original direct calls')
    prepared = all(symbol(stage[0]) in emitted for stage in STAGES)
    if not prepared and (INCLUDE in emitted or any(symbol(stage[0]) in emitted for stage in STAGES)):
        raise ValueError('Partially prepared 131AC0 fog cluster sky binding')
    owner, references = current[0], []
    for stage in STAGES:
        name, first, last, fallthrough, continuations = stage
        before, actual, after = split(owner, stage, True, prepared)
        _, raw, _ = split(original, stage, False)
        expected = CURSOR.sub(r's.instruction_literal(\1);', raw)
        child = ',[](auto &v){run_133a70(v);}' if name == 'cluster' else ''
        installed = f'\n    switch({NAMESPACE}::{name}(s,{symbol(name)}{child})) {{\n'
        for target, destination in ((last, fallthrough), *((pc, value) for pc, value, _ in continuations)):
            installed += f'        case {NAMESPACE}::Next::{destination}:goto L{target};\n'
        installed += '        default:break;\n    }\n' + expected.lstrip(' ')
        reference = reference_body(canonical, stage)
        if prepared:
            if emitted.count(reference) != 1 or actual != installed:
                raise ValueError(f'Prepared 131AC0 fog cluster sky binding changed at {first}')
        else:
            if actual != expected:
                raise ValueError(f'Specialized 131AC0 fog cluster sky source changed at {first}')
            owner = before + installed + f'\nL{last}:' + after.removeprefix('\n')
        references.append(reference)
    entries = [stage[1] for stage in STAGES]
    if prepared:
        if emitted.count(INCLUDE) != 1:
            raise ValueError('Prepared 131AC0 fog cluster sky include changed')
        return emitted, entries
    return INCLUDE + emitted[:current.start()] + ''.join(references) + '\n' + owner + emitted[current.end():], entries
