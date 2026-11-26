"""Bind the selected-image D8571 object response flag route."""
import native_damage_update_entry_replacements as prefix
import native_damage_update_return_replacements as owner


INCLUDE = '#include "gameplay/units/native_tu15_damage_update_flag_route.hpp"\n'
REFERENCE = 'native_damage_update_flag_route_reference_d73e0'
NAMESPACE = 'halo2::native_damage_update_flag_route_detail'
OPERANDS = {0x4edf18: 0x4f3218, 0xdd5b0: 0xde500, 0xd7677: 0xd85c7}


def admitted(canonical):
    mapping = prefix.world.mapping(canonical)
    return owner.admitted(canonical) and all(mapping.get(key) == pc for key, pc in OPERANDS.items())


def reference(body, replay=True):
    first = body.index('\nLd7621:')
    last = body.index('\nLd767b:', first)
    span = body[first:last].removeprefix('\nLd7621:')
    span = span.replace('goto Ld7689;', f'return {NAMESPACE}::Next::keep_flags;')
    span = span.replace('goto Ld767b;', f'return {NAMESPACE}::Next::clear_flags;')
    if replay:
        span = span.replace('dispatch(s,target,0xd7672u,false);', 'halo2::native_object_tick_detail::replay(s,target);')
    suffix = '' if replay else '_live'
    return (f'static {NAMESPACE}::Next {REFERENCE}{suffix}(ObservedDamageState &s) {{\n'
            f'{span}\n    return {NAMESPACE}::Next::clear_flags;\n}}\n')


def install(canonical, output):
    if not admitted(canonical) or REFERENCE in output:
        return output, []
    original = owner.body(canonical)
    found = owner.body(output)
    if not original or not found:
        return output, []
    label = '\nLd7621:'
    if found[1].count(label) != 1:
        return output, []
    hook = (f'\n    switch ({NAMESPACE}::bridge(s,{REFERENCE},'
            '[](auto &v,auto target,auto site,auto tail){dispatch(v,target,site,tail);})) {\n'
            f'    case {NAMESPACE}::Next::keep_flags: goto Ld7689;\n'
            f'    case {NAMESPACE}::Next::clear_flags: goto Ld767b;\n'
            f'    case {NAMESPACE}::Next::retained: break;\n    }}\n')
    body = found[1].replace(label,label + hook,1)
    output = INCLUDE + output[:found.start(1)] + reference(original[1]) + body + output[found.end(1):]
    return output, ['d8571']
