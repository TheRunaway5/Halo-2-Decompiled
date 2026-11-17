"""Bind the native TU1.5 2F33B0 key sort (hkAlgorithm::quickSortRecursive over
unsigned 32-bit keys; canonical run_2ef8f0) in the physics world unit.

The prefix is installed only into the build-local specialized copy, and only
when the canonical body matches its pin (state type normalized), its one
selected() operand (the recursion's return address) maps to the TU1.5 literal
the adapter pushes, and its only call is its own recursion. The native runs
the recursion inline. The bridge receives a verification reference: a renamed
copy of the canonical body, whose recursive call re-enters the bound body with
the native route bypassed, so the whole translated recursion is replayed. A
changed or missing body keeps its translated route.
"""
import hashlib
import re

NAME = '2ef8f0'
ENTRY = '2f33b0'
PIN = '94f2643c4bb693f4fe681e2b7ed0370854a0bbc43302d79619f7feddd2db7530'
OPERANDS = {0x2ef954: 0x2f3414}
STATE = 'ObservedWorldState'
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
REFERENCE = 'native_world_leaves_reference_2ef8f0'
DETAIL = 'halo2::native_world_leaves_detail'


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def mapping(text):
    resolver = re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}', text, re.S)
    if not resolver:
        return {}
    return {int(a, 16): int(b, 16) for a, b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;', resolver[0])}


def admitted(canonical):
    """True when the canonical body and its operand qualify."""
    found = body(canonical, NAME)
    if not found or found[1] != STATE or digest(found) != PIN:
        return False
    used = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
    resolved = mapping(canonical)
    if used != set(OPERANDS) or any(resolved.get(key) != value for key, value in OPERANDS.items()):
        return False
    return set(re.findall(r'\brun_([0-9a-f]+)\(', found[0][found[0].index('{'):])) == {NAME} and 'dispatch(' not in found[0]


def reference(found):
    """The canonical body renamed; its recursion re-enters the bound body."""
    text = found[0].replace(f'static void run_{NAME}({STATE} &s) {{', f'static void {REFERENCE}({STATE} &s) {{', 1)
    if set(re.findall(r'\brun_([0-9a-f]+)\(', text)) != {NAME} or re.search(r'\bdispatch\(|required\(', text) or \
            not text.startswith('static void '):
        raise ValueError('2F33B0 reference kept an unexpected child')
    return text + '\n\n'


def prefix():
    return (f'\n    // BEGIN native key sort {ENTRY}\n'
            f'    if({DETAIL}::bridge_2f33b0(s,[](auto &v){{{REFERENCE}(v);}}))return;\n'
            f'    // END native key sort {ENTRY}')


def install(canonical, output):
    """Return (output, installed TU1.5 entries)."""
    if not admitted(canonical):
        return output, []
    current = body(output, NAME)
    text = prefix()
    if not current or text in current[0]:
        return output, []
    start = current[0].index('{') + 1
    # The reference's recursion calls run_2ef8f0, so it is declared first.
    replaced = (f'static void run_{NAME}({STATE} &);\n' + reference(body(canonical, NAME)) +
                current[0][:start] + text + current[0][start:])
    output = output[:current.start()] + replaced + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [ENTRY]
