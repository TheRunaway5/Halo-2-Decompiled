"""Bind D86F2 shield recovery and D87A6 follow-up gating."""
import native_damage_update_entry_replacements as prefix
import native_damage_update_return_replacements as owner


INCLUDE = '#include "gameplay/units/native_tu15_damage_update_shield_recovery.hpp"\n'
REFERENCE = 'native_damage_update_shield_recovery_reference_d73e0'
NAMESPACE = 'halo2::native_damage_update_shield_recovery_detail'
ENTRIES = {'d77a2': 'recover', 'd7856': 'evaluate'}
EXITS = {'d7751': 'armor', 'd774a': 'clear_pending', 'd79a9': 'countdown',
         'd787e': 'default_scale', 'd788a': 'apply_scale'}
CHILDREN = {'14f5d0': 0x1509b0, '13be58': 0x13d1c8, 'a87e0': 0xb1c30}
OPERANDS = {0x518444: 0x51df54, 0x46507c: 0x46a11c, 0x4652d0: 0x46a370,
            0xd77c3: 0xd8713, 0xd7840: 0xd8790, 0xd784c: 0xd879c,
            **{int(name,16): target for name,target in CHILDREN.items()}}


def admitted(canonical):
    mapping = prefix.world.mapping(canonical)
    return owner.admitted(canonical) and all(mapping.get(key) == pc for key, pc in OPERANDS.items())


def reference(body, replay=True):
    arms = ''.join(f'    case {NAMESPACE}::Entry::{entry}: goto L{pc};\n'
                   for pc, entry in ENTRIES.items())
    start = body.index('\nLd77a2:')
    span = body[start:body.index('\nLd787e:',start)]
    for target, outcome in EXITS.items():
        span = span.replace(f'goto L{target};', f'return {NAMESPACE}::Next::{outcome};')
    if replay:
        for name, target in CHILDREN.items():
            span = span.replace(f'run_{name}(s);', f'halo2::native_object_tick_detail::replay(s,0x{target:x}u);')
    suffix = '' if replay else '_live'
    return (f'static {NAMESPACE}::Next {REFERENCE}{suffix}(ObservedDamageState &s,{NAMESPACE}::Entry entry) {{\n'
            f'    switch (entry) {{\n{arms}    }}\n'
            f'    throw std::logic_error("Invalid damage shield recovery entry");{span}\n}}\n')


def install(canonical, output):
    if not admitted(canonical) or REFERENCE in output:
        return output, []
    original = owner.body(canonical)
    found = owner.body(output)
    if not original or not found:
        return output, []
    body = found[1]
    routes = ' else '.join(
        f'if constexpr (decltype(target)::value==0x{target:x}u) run_{name}(v);'
        for name, target in CHILDREN.items())
    for first, entry in ENTRIES.items():
        label = f'\nL{first}:'
        if body.count(label) != 1:
            return output, []
        arms = ''.join(f'    case {NAMESPACE}::Next::{outcome}: goto L{dest};\n'
                       for dest, outcome in EXITS.items())
        hook = (f'\n    switch ({NAMESPACE}::bridge(s,{NAMESPACE}::Entry::{entry},{REFERENCE},'
                f'[](auto &v,auto target) {{ {routes} }},damage_mulss)) {{\n'
                f'{arms}    case {NAMESPACE}::Next::retained: break;\n'
                '    }\n')
        body = body.replace(label, label + hook, 1)
    output = INCLUDE + output[:found.start(1)] + reference(original[1]) + body + output[found.end(1):]
    return output, ['d86f2']
