"""Bind the native TU1.5 303EE0 contact record append (canonical run_300400)
in the physics world unit.

The prefix is installed only into the build-local specialized copy, and only
when the canonical body matches its pin (state type normalized), its two
selected() return-address operands map to the TU1.5 literals the adapter
pushes, and its 2E3250 child (run_2df780) exists. The bridge receives a
verification reference: a copy of the canonical body with its 2E3250 call
routed to the recorded child boundary (303BA0 stays the translated body,
entered with the native route bypassed). A changed or missing body keeps its
translated route.
"""
import hashlib
import re

NAME = '300400'
ENTRY = '303ee0'
PIN = 'a8fe5a4bf3c4434962e1a6a2f208cc121ca8e8a62753e206f4b817958d35c7bf'
# selected() operands: the two CALL return addresses, canonical -> TU1.5.
OPERANDS = {0x300439: 0x303f19, 0x300456: 0x303f36}
CHILD = '2df780'       # canonical name of 2E3250, hkArrayUtil::_reserveMore
CHILD_ENTRY = '2e3250'
COPY = '300100'        # 303BA0, the record copy (stays translated in the reference)
STATE = 'ObservedWorldState'
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
REFERENCE = 'native_world_leaves_reference_300400'
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
    """True when the canonical body, its operands and its children qualify."""
    found = body(canonical, NAME)
    if not found or found[1] != STATE or digest(found) != PIN:
        return False
    used = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
    resolved = mapping(canonical)
    if used != set(OPERANDS) or any(resolved.get(key) != value for key, value in OPERANDS.items()):
        return False
    return all(body(canonical, child) and body(canonical, child)[1] == STATE for child in (CHILD, COPY))


def reference(found):
    """The canonical body renamed, with only its 2E3250 call routed to the replay."""
    text = found[0].replace(f'static void run_{NAME}({STATE} &s) {{', f'static void {REFERENCE}({STATE} &s) {{', 1)
    text = text.replace(f'run_{CHILD}(s)', f'{DETAIL}::replay_child(s,0x{CHILD_ENTRY}u)')
    calls = set(re.findall(r'\brun_([0-9a-f]+)\(', text))
    if calls != {COPY} or re.search(r'\bdispatch\(|required\(', text) or not text.startswith('static void '):
        raise ValueError('303EE0 reference kept an unrouted child')
    return text + '\n\n'


def prefix():
    return (f'\n    // BEGIN native contact array {ENTRY}\n'
            f'    if({DETAIL}::bridge_303ee0(s,[](auto &v){{{REFERENCE}(v);}},[](auto &v){{run_{CHILD}(v);}}))return;\n'
            f'    // END native contact array {ENTRY}')


def install(canonical, output):
    """Return (output, installed TU1.5 entries)."""
    if not admitted(canonical):
        return output, []
    current = body(output, NAME)
    text = prefix()
    if not current or text in current[0]:
        return output, []
    start = current[0].index('{') + 1
    replaced = reference(body(canonical, NAME)) + current[0][:start] + text + current[0][start:]
    output = output[:current.start()] + replaced + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [ENTRY]
