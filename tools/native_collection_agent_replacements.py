"""Bind the native TU1.5 304350 welded collection agent (canonical run_300870)
in the physics world unit.

The prefix is installed only into the build-local specialized copy, and only
when the canonical body matches its pin (state type normalized), its selected()
operands (return addresses, the exception handler, the timer names, the
profiler stream, the weld predicate, the bias, the thread memory and the
candidate filter) map to the TU1.5 literals the adapter uses, and every child
qualifies:
  - the translated children (2F36F0 run_2efc30, 304130 run_300650, 30CD00
    run_309210, 30CF40 run_309450, and 2E3250 run_2df780 under 303EE0) exist;
  - the children the adapter runs inline match the pins their own binders
    check (303C70/303D00 local buffers, 303EE0 contact array, 303BA0 and
    2E47B0 world leaves) and 30BAD0 (run_307fe0) matches its pin and operand.
The bridge receives a verification reference: a copy of the canonical body
with each translated child and each dispatch call routed to the recorded child
boundary, and the inline children replaced by their own routed references.
A changed or missing body keeps its translated route.
"""
import hashlib
import importlib.util
from pathlib import Path
import re

NAME = '300870'
ENTRY = '304350'
PIN = '1504814ca217f8283daa51ba6df2e32f7b924c0a24bf9683408f352afde228d2'
OPERANDS = {
    0x2ef680: 0x2f3140, 0x300900: 0x3043e0, 0x300936: 0x304416, 0x30097a: 0x30445a, 0x30099d: 0x30447d,
    0x3009b5: 0x304495, 0x3009bd: 0x30449d, 0x3009cf: 0x3044af, 0x300a00: 0x3044e0, 0x300a0c: 0x3044ec,
    0x300ae6: 0x3045c6, 0x300b73: 0x304653, 0x300da4: 0x304884, 0x300e29: 0x304909, 0x300e56: 0x304936,
    0x300eea: 0x3049ca, 0x300f31: 0x304a11, 0x300f8f: 0x304a6f, 0x300fc3: 0x304aa3, 0x37b346: 0x37edf6,
    0x4188f4: 0x41c394, 0x4195dc: 0x41d05c, 0x4650e0: 0x46a180, 0x4693ac: 0x46e46c, 0x487638: 0x48c938,
    0x487674: 0x48c974, 0x487678: 0x48c978,
}
SITES = (0x30099a, 0x3009b2, 0x3009ba, 0x3009c9, 0x3009fd, 0x300b70, 0x300fc0)
# Translated children: canonical name -> TU1.5 entry (replay_child target).
CHILDREN = {'2efc30': 0x2f36f0, '300650': 0x304130, '309210': 0x30cd00, '309450': 0x30cf40}
RESERVE = '2df780'
NULL_AGENT = ('307fe0', '26d634800865bacf5588cb14bf21d287386f3252a04c980cbd16229420729523', {0x487ac4: 0x48cdc4})
STATE = 'ObservedWorldState'
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
REFERENCE = 'native_world_leaves_reference_300870'
DETAIL = 'halo2::native_world_leaves_detail'


def load(name):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def mapping(text):
    resolver = re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}', text, re.S)
    if not resolver:
        return {}
    return {int(a, 16): int(b, 16) for a, b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;', resolver[0])}


def used_operands(found):
    return {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}


def admitted(canonical):
    """True when the canonical body, its operands and all of its children qualify."""
    found = body(canonical, NAME)
    if not found or found[1] != STATE or digest(found) != PIN:
        return False
    resolved = mapping(canonical)
    if used_operands(found) != set(OPERANDS) or any(resolved.get(k) != v for k, v in OPERANDS.items()):
        return False
    if tuple(int(s, 16) for s in re.findall(r'dispatch\(s,target,0x([0-9a-f]+)u,false\)', found[0])) != SITES:
        return False
    for child in (*CHILDREN, RESERVE):
        if not body(canonical, child) or body(canonical, child)[1] != STATE:
            return False
    name, pin, operands = NULL_AGENT
    null = body(canonical, name)
    if not null or null[1] != STATE or digest(null) != pin or used_operands(null) != set(operands) or \
            any(resolved.get(k) != v for k, v in operands.items()):
        return False
    buffers, array, leaves = load('native_local_buffers_replacements'), load('native_contact_array_replacements'), \
        load('native_world_leaves_replacements')
    return all(buffers.admitted(canonical, n) for n in buffers.OWNERS) and array.admitted(canonical) and \
        {'300100', '2e0cc0'} <= set(leaves.admitted(canonical))


def reference(found):
    """The canonical body renamed, with every child routed for the replay."""
    text = found[0].replace(f'static void run_{NAME}({STATE} &s) {{', f'static void {REFERENCE}({STATE} &s) {{', 1)
    for child, entry in CHILDREN.items():
        text = text.replace(f'run_{child}(s)', f'{DETAIL}::replay_child(s,0x{entry:x}u)')
    text = re.sub(r'dispatch\(s,target,0x[0-9a-f]+u,false\)', f'{DETAIL}::replay_child(s,target)', text)
    # The inline children run as their own routed references.
    for child in ('3001d0', '300260', '300400'):
        text = text.replace(f'run_{child}(s)', f'native_world_leaves_reference_{child}(s)')
    calls = set(re.findall(r'\brun_([0-9a-f]+)\(', text))
    if calls != {'2e0cc0', '307fe0'} or re.search(r'\bdispatch\(|required\(', text) or not text.startswith('static void '):
        raise ValueError('304350 reference kept an unrouted child')
    declarations = ''.join(f'static void native_world_leaves_reference_{child}({STATE} &);\n'
                           for child in ('3001d0', '300260', '300400'))
    return declarations + text + '\n\n'


def prefix():
    lanes = ''.join(f',[](auto &v){{run_{child}(v);}}' for child in CHILDREN)
    return (f'\n    // BEGIN native collection agent {ENTRY}\n'
            f'    if({DETAIL}::bridge_304350(s,[](auto &v){{{REFERENCE}(v);}}{lanes},'
            f'[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},'
            f'[](auto &v){{run_{RESERVE}(v);}}))return;\n'
            f'    // END native collection agent {ENTRY}')


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
