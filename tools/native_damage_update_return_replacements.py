"""Bind the three TU1.5 damage update return epilogues."""
import hashlib
import re

import native_damage_update_entry_replacements as prefix


INCLUDE = '#include "gameplay/units/native_tu15_damage_update_return.hpp"\n'
REFERENCE = 'native_damage_update_return_reference_d73e0'
NAMESPACE = 'halo2::native_damage_update_return_detail'
ENTRIES = {'d7c08': 'updated', 'd7c14': 'notify', 'd7c4f': 'unchanged'}
PIN = 'f9109e48ec7acba15191bef6c1ff4d7b9a953f2d20a88161ddfae898d7e134b1'
NOTIFY_TARGET = 0xdb0a0
CONTINUATION = 0xd8b8d
END = '\nthrow std::runtime_error("Original solver fell outside authenticated CFG");'


def body(text):
    return re.search(r'(static void run_d73e0\(ObservedDamageState &s\) \{.*?\n\})'
                     r'\n\nstatic void run_d7cd0\(ObservedDamageState &s\)', text, re.S)


def admitted(canonical):
    if not prefix.admitted(canonical):
        return False
    found = body(canonical)
    if not found or hashlib.sha256(found[1].encode()).hexdigest() != PIN:
        return False
    mapping = prefix.world.mapping(canonical)
    return mapping.get(0xda150) == NOTIFY_TARGET and mapping.get(0xd7c3d) == CONTINUATION


def reference(body, replay=True):
    first = body.index('\nLd7c08:')
    source = body[first:body.index(END, first)]
    if replay:
        source = source.replace('run_da150(s);', f'halo2::native_object_tick_detail::replay(s,0x{NOTIFY_TARGET:x}u);')
    suffix = '' if replay else '_live'
    arms = ''.join(f'    case {NAMESPACE}::Entry::{entry}: goto L{pc};\n'
                   for pc, entry in ENTRIES.items())
    return (f'static void {REFERENCE}{suffix}(ObservedDamageState &s,{NAMESPACE}::Entry entry) {{\n'
            f'    switch (entry) {{\n{arms}    case {NAMESPACE}::Entry::count: break;\n    }}\n'
            f'    throw std::logic_error("Invalid damage return entry");{source}\n}}\n')


def install(canonical, output):
    if not admitted(canonical) or REFERENCE in output:
        return output, []
    original = body(canonical)
    found = body(output)
    if not found or prefix.REFERENCE not in output:
        return output, []
    emitted_body = found[1]
    for pc, entry in ENTRIES.items():
        label = f'\nL{pc}:'
        if emitted_body.count(label) != 1:
            return output, []
        hook = (f'\n    if ({NAMESPACE}::bridge(s,{NAMESPACE}::Entry::{entry},{REFERENCE},'
                '[](auto &value){run_da150(value);})) return;\n')
        emitted_body = emitted_body.replace(label, label + hook, 1)
    output = INCLUDE + output[:found.start(1)] + reference(original[1]) + emitted_body + output[found.end(1):]
    return output, ['d8b58']
