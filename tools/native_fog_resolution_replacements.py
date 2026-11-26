"""Bind the child-free terminal fog blend in the campaign environment owner."""
import hashlib
import re


INCLUDE = '#include "campaign/native_tu15_fog_resolution.hpp"\n'
RAW_PIN = '06d3dee1dd663bd7d28162440441dbb73692d765c64b3602bd846f1c9f0f5f71'
LABEL = 'L13262b:'
RAW_ENTRY = ' if(s.services.instruction){s.services.instruction(0x13262bu);}'
SPECIALIZED_ENTRY = ' s.instruction_literal(0x13262bu);'
REFERENCE = 'native_fog_resolution_reference_131ac0'
BRIDGE = f'if(halo2::native_fog_resolution_detail::bridge(s,{REFERENCE}))return;'
MULSS = 'halo2::native_tu15_leaves::ordered_mul_ss('


def body(text):
    return re.search(r'static void run_131ac0\(WorldState &s\) \{.*?\n\}', text, re.S)


def raw_owner(canonical):
    original = body(canonical)
    if not original:
        raise ValueError('Campaign environment owner 131AC0 is missing')
    normalized = original[0].replace('WorldState &s', 'STATE &s')
    if hashlib.sha256(normalized.encode()).hexdigest() != RAW_PIN:
        raise ValueError('Campaign environment owner 131AC0 source changed')
    return original[0]


def pinned_tail(tail):
    tail, count = re.subn(r'\b_mm_mul_ss\(', MULSS, tail)
    if count != 37:
        raise ValueError(f'Fog resolution 13262B scalar multiply sites changed ({count})')
    return tail


def reference_body(canonical):
    """The original range, run both standalone and through ordered replay."""
    raw = raw_owner(canonical)
    if (raw.count(LABEL + RAW_ENTRY) != 1 or raw.count('goto ' + LABEL[:-1] + ';') != 1):
        raise ValueError('Fog resolution 13262B incoming edges changed')
    tail = raw.split(LABEL, 1)[1]
    return f'static void {REFERENCE}(WorldState &s) {{\n' + pinned_tail(tail) + '\n'


def install(canonical, output):
    reference = reference_body(canonical)
    current = body(output)
    if not current:
        raise ValueError('Specialized campaign environment owner 131AC0 is missing')
    # Literal cursor is the only allowed source transformation within the
    # selected range. Leave the two predecessor paths and all children intact.
    original_tail = raw_owner(canonical).split(LABEL, 1)[1]
    expected = re.sub(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
                      r's.instruction_literal(\1);', original_tail)
    bridge_tail = '\n    ' + BRIDGE + '\n' + pinned_tail(expected)
    if BRIDGE in current[0]:
        if (output.count(INCLUDE) != 1 or output.count(reference) != 1 or
                current[0].count(LABEL + SPECIALIZED_ENTRY) != 0 or
                current[0].split(LABEL, 1)[1] != bridge_tail):
            raise ValueError('Prepared fog resolution 13262B binding changed')
        return output, ['13262b']
    if current[0].count(LABEL + SPECIALIZED_ENTRY) != 1 or INCLUDE in output or REFERENCE in output:
        raise ValueError('Specialized fog resolution 13262B entry changed')
    before, tail = current[0].split(LABEL, 1)
    if tail != expected:
        raise ValueError('Specialized fog resolution 13262B source changed')
    installed = before + LABEL + bridge_tail
    return INCLUDE + output[:current.start()] + reference + '\n' + installed + output[current.end():], ['13262b']
