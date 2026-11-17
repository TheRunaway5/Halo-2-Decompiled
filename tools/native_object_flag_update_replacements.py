"""Bind the native TU1.5 E8D80 object flag update (canonical run_e7e60, an
entry of the vehicle update unit: an object's flags kept or cleared by their
update functions).

The prefix is installed only into the build-local specialized copy of
physics_vehicle_update_generated.cpp, and only when the canonical body
matches its pin (state type normalized), its selected() operands (the object
array, the flags' definitions, the update call's return address) map to the
TU1.5 literals the adapter uses, and its one dispatch site (the update
function) is the expected one. The bridge receives a verification reference:
a copy of the canonical body with the dispatch call routed to the recorded
child boundary. A changed or missing body keeps its translated route.
"""
import hashlib
import re

NAME = 'e7e60'
ENTRY = 'e8d80'
PIN = '44f3fd2151aa16c4958b209642331a382b1e5b02350339bb0af9a76fc96c3624'
OPERANDS = {0x4e78d0: 0x4ecbd0, 0x46ec68: 0x473d28, 0xe7ec0: 0xe8de0}
SITES = (0xe7ebe,)
STATE = 'VehicleState'
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
REFERENCE = 'native_world_leaves_reference_e7e60'
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
    """True when the canonical body and its operands qualify."""
    found = body(canonical, NAME)
    if not found or found[1] != STATE or digest(found) != PIN:
        return False
    used = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
    resolved = mapping(canonical)
    if used != set(OPERANDS) or any(resolved.get(key) != value for key, value in OPERANDS.items()):
        return False
    return tuple(int(s, 16) for s in re.findall(r'dispatch\(s,target,0x([0-9a-f]+)u,false\)', found[0])) == SITES


def reference(found):
    """The canonical body renamed, with the dispatch routed to the replay."""
    text = found[0].replace(f'static void run_{NAME}({STATE} &s) {{', f'static void {REFERENCE}({STATE} &s) {{', 1)
    text = re.sub(r'dispatch\(s,target,0x[0-9a-f]+u,false\)', f'{DETAIL}::replay_child(s,target)', text)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('static void '):
        raise ValueError('E8D80 reference kept an unrouted child')
    return text + '\n\n'


def prefix():
    return (f'\n    // BEGIN native object flag update {ENTRY}\n'
            f'    if({DETAIL}::bridge_e8d80(s,[](auto &v){{{REFERENCE}(v);}},'
            f'[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}}))return;\n'
            f'    // END native object flag update {ENTRY}')


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
