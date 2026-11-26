"""Bind the remaining campaign fog paths and require exact 131AC0 coverage."""
from collections import Counter
import hashlib
import re

import native_fog_accumulation_replacements as accumulation
import native_fog_cluster_finish_replacements as finish
import native_fog_cluster_sky_replacements as cluster_sky
import native_fog_projected_merge_replacements as merge
import native_fog_resolution_replacements as resolution

from native_fog_projected_merge_replacements import CURSOR, instruction


INCLUDE = '#include "campaign/native_tu15_fog_remaining.hpp"\n'
NAMESPACE = 'halo2::native_fog_remaining_detail'
LITERALS = re.compile(r's\.services\.instruction\(0x([0-9a-f]+)u\);')
DISPATCH = re.compile(r'^static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}', re.M)
DISPATCH_PIN = '1517f03e7cb2193572df9f624025386390ba7927c2cfac3fa1358a2edd8e8911'
PROBE = '{const auto target=0x32a660u;s.push(0x131ac5u,0x131acau);dispatch(s,target,0x131ac5u,false);}'
CALLS = {
    'setup': (('131af8', '131afd', '131470'), ('131b11', '131b16', '131610')),
    'environment_patch': (('131f9d', '131fa2', '1342b0'),),
    'projection_call': (('132183', '132188', '133b40'),),
    'sky_patch': (('1324af', '1324b4', '1342b0'),),
}
STAGES = (
    ('setup', '131ac0', '131c31', 'cluster', (('132628', 'empty', 1),)),
    ('environment', '131c89', '131d38', 'weight', (('132597', 'missing_sky', 1), ('131d38', 'weight', 1))),
    ('environment_patch', '131f80', '131fc7', 'environment_bloom', ()),
    ('projection_call', '132183', '132188', 'projected_merge', ()),
    ('sky_patch', '132492', '1324d9', 'sky_bloom', ()),
    ('sky_bloom', '1324d9', '132597', None, (('132084', 'recorded', 1),)),
    ('missing_sky', '132597', '132628', None, (('13208c', 'uncovered', 1),)),
    ('empty', '132628', '13262b', 'resolve', ()),
)


def check_dispatch(unit):
    entries = DISPATCH.findall(unit)
    if len(entries) != 1 or hashlib.sha256(entries[0].encode()).hexdigest() != DISPATCH_PIN:
        raise ValueError('131AC0 original stack probe dispatcher changed')
    if 'case 0x32a660u:' in entries[0] or 'default:s.required(site,target,tail);return;' not in entries[0]:
        raise ValueError('131AC0 original stack probe no longer uses its required-owner route')


def symbol(stage):
    return f'native_fog_remaining_{stage}_reference_131ac0'


def boundary(owner, pc, specialized):
    label = f'\nL{pc}:'
    if owner.count(label) == 1:
        return label
    marker = '\n' + instruction(pc, specialized)
    if owner.count(marker) != 1:
        raise ValueError(f'131AC0 remaining fog boundary changed at {pc}')
    return marker


def split(owner, stage, specialized):
    _, first, last, _, _ = stage
    head = boundary(owner, first, specialized)
    tail = boundary(owner, last, specialized)
    before, rest = owner.split(head, 1)
    selected, after = rest.split(tail, 1)
    if head.endswith(':'):
        return before + head, selected.lstrip(' '), tail + after
    return before + '\n', head[1:] + selected, tail + after


def pinned(selected, name):
    if name not in ('sky_bloom', 'missing_sky'):
        return selected
    for intrinsic, operation, expected in (
            ('_mm_mul_ss', 'halo2::native_tu15_leaves::ordered_mul_ss', 5 if name == 'sky_bloom' else 0),
            ('_mm_add_ss', 'halo2::native_world_leaves_detail::ordered_add_ss', 5)):
        selected, count = re.subn(rf'\b{intrinsic}\(', f'{operation}(', selected)
        if count != expected:
            raise ValueError(f'131AC0 remaining fog ordered {intrinsic} sites changed in {name}')
    return selected


def call_site(site, continuation, target):
    return f's.push(0x{site}u,0x{continuation}u);run_{target}(s);'


def reference_body(canonical, stage, direct_child=False):
    name, first, _, fallthrough, continuations = stage
    _, selected, _ = split(resolution.raw_owner(canonical), stage, False)
    for target, destination, count in continuations:
        jump = f'goto L{target};'
        if selected.count(jump) != count:
            raise ValueError(f'131AC0 remaining fog edges changed at {first}')
        selected = selected.replace(jump, f'return {NAMESPACE}::Next::{destination};')
    for site, continuation, target in CALLS.get(name, ()):
        if selected.count(call_site(site, continuation, target)) != 1:
            raise ValueError(f'131AC0 original direct child changed at {site}')
        if not direct_child:
            selected = selected.replace(f'run_{target}(s);', f'halo2::native_object_tick_detail::replay(s,0x{target}u);')
    if name == 'setup':
        if selected.count(PROBE) != 1:
            raise ValueError('131AC0 original stack probe call changed')
        if not direct_child:
            selected = selected.replace('dispatch(s,target,0x131ac5u,false);',
                                        'halo2::native_object_tick_detail::replay(s,target);')
    final = f'\nreturn {NAMESPACE}::Next::{fallthrough};' if fallthrough else ''
    return (f'static {NAMESPACE}::Next {symbol(name)}(WorldState &s) {{\n'
            + pinned(selected, name) + final + '\n}\n')


def coverage(canonical):
    older = [module.reference_body(canonical) for module in (accumulation, finish, merge, resolution)]
    older.extend(cluster_sky.reference_body(canonical, stage) for stage in cluster_sky.STAGES)
    references = [reference_body(canonical, stage) for stage in STAGES]
    original = Counter(LITERALS.findall(resolution.raw_owner(canonical)))
    covered = Counter(pc for reference in older + references for pc in LITERALS.findall(reference))
    if (sum(original.values()) != 786 or original['131bea'] != 2 or
            sum(1 for count in original.values() if count > 1) != 1 or covered != original):
        raise ValueError('131AC0 native references do not partition all 786 original literal sites')
    return older, references


def child_route(name):
    if name == 'setup':
        return (',[](auto &v,std::uint32_t target,std::uint32_t pc){\n'
                '        switch(target){case 0x131470u:run_131470(v);return;case 0x131610u:run_131610(v);return;\n'
                '        default:dispatch(v,target,pc,false);}}')
    calls = CALLS.get(name, ())
    return f',[](auto &v,std::uint32_t,std::uint32_t){{run_{calls[0][2]}(v);}}' if calls else ''


def install(canonical, emitted):
    check_dispatch(canonical)
    check_dispatch(emitted)
    current = resolution.body(emitted)
    original = resolution.raw_owner(canonical)
    older, references = coverage(canonical)
    if not current or any(emitted.count(reference) != 1 for reference in older):
        raise ValueError('131AC0 whole owner requires all authenticated native fog stages')
    owners = [symbol(stage[0]) in emitted for stage in STAGES]
    prepared = all(owners)
    if not prepared and (INCLUDE in emitted or any(owners)):
        raise ValueError('Partially prepared 131AC0 remaining fog binding')
    owner = current[0]
    for stage, reference in zip(STAGES, references):
        name, first, last, fallthrough, continuations = stage
        before, actual, after = split(owner, stage, True)
        _, raw, _ = split(original, stage, False)
        expected = CURSOR.sub(r's.instruction_literal(\1);', raw)
        routes = {pc: destination for pc, destination, _ in continuations}
        if fallthrough:
            routes[last] = fallthrough
        if not before.endswith(':'):
            before += f'L{first}:'
        bridge = (f'\n    switch({NAMESPACE}::bridge<{NAMESPACE}::Entry::{name}>('
                  f's,{symbol(name)}{child_route(name)})) {{\n'
                  + ''.join(f'        case {NAMESPACE}::Next::{dest}:goto L{pc};\n' for pc, dest in routes.items())
                  + '        default:break;\n    }\n')
        installed = bridge + pinned(expected, name)
        if prepared:
            if emitted.count(reference) != 1 or actual != installed:
                raise ValueError(f'Prepared 131AC0 remaining fog binding changed at {first}')
        else:
            if actual != expected:
                raise ValueError(f'Specialized 131AC0 remaining fog code changed at {first}')
            owner = before + installed + after
    if prepared:
        if emitted.count(INCLUDE) != 1:
            raise ValueError('Prepared 131AC0 remaining fog include changed')
        return emitted, ['131ac0']
    return INCLUDE + emitted[:current.start()] + ''.join(references) + '\n' + owner + emitted[current.end():], ['131ac0']
