"""Bind the native TU1.5 effect owners (group E): the 20680 lens query reset
(lens flare unit), the C35D0 / C3300 / C2A80 per-view light fades (effect
light view unit) and the 176A40 / 252100 particle inputs (death cycle, combat
action and particle frame units; 4EFA0 direct or dispatched).

The prefix is installed only into the build-local specialized copy, and only
when every canonical body of the owner's closure matches its pin (state type
normalized), every selected() operand in those bodies is the identity under
TU1.5, and every genuine child keeps its direct body in the unit. Each bound
owner also receives its verification reference: the unmodified canonical
bodies with only the genuine child calls routed to the verification replay
(HALO2_VERIFY_EFFECT_OWNERS); inline callees keep their translated reference
copies. A changed or missing body keeps its translated route.
"""
import hashlib
import re

PINS = {
    '20680': 'd4f4712b719d603130dc0ea388c41eef772a4928af380f575e5f9088ee6de746',
    'c35d0': '69d9d3c4640a5d09d414b33d438f069a765afd9ab8d50768af25323b5e1708c5',
    'c3300': '2176815d63ceaea1f00951317b502ad1db5b614f67484ce96bfb4b10465c9977',
    'c2a80': 'bc8b68259b3bf9e2ddb30c739220084a75c7083fc867526bc9f251d877bf281d',
    '176a40': '8b565d96abc8252cb19461751d618580232a9c7af9532bebeba8d02a183558e7',
    # Direct 4EFA0 body (combat action, particle frame) or its dispatch (death cycle).
    '252100': ('2a3e64f69f7a817e6d9c1053ebb1518c92147f57a929702d590a0231da628427',
               'd5ac4ac2180567668e0d597684f0d70a36bcb2a3f1d0fb06758e745d0c9cba8f'),
}
# Owner: (closure bodies, genuine children in bridge argument order).
OWNERS = {
    '20680': (('20680',), ()),
    'c35d0': (('c35d0', 'c3300', 'c2a80'), ('16ea60',)),
    '176a40': (('176a40', '252100'), ('4efa0',)),
}
# Children that may be entered through the unit's dispatch instead of a direct
# body; their bridge lambda also receives the call site.
SITED = {'4efa0'}
INCLUDE = '#include "rendering/effects/native_tu15_effect_owners.hpp"\n'
DETAIL = 'halo2::native_effect_owner_detail'
REPLAY = 'halo2::native_object_tick_detail'
REFERENCES_BEGIN = '// BEGIN native effect owner references\n'
REFERENCES_END = '// END native effect owner references\n'
FIRST_BODY = re.compile(r'^static void run_[0-9a-f]+\(WorldState &s\) \{$', re.M)
MAPPING = re.compile(r'^std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*$', re.M)
CASE = re.compile(r'case (0x[0-9a-f]+)u:return (0x[0-9a-f]+)u;')
SELECTED = re.compile(r'\bselected\(\s*(0x[0-9a-f]+)u\s*,\s*s\.image\s*\)')


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def reference_name(name):
    return f'native_effect_reference_{name}'


def child_lambda(canonical, child):
    if child not in SITED:
        return f'[](auto &v){{run_{child}(v);}}'
    if body(canonical, child):
        return f'[](auto &v,std::uint32_t){{run_{child}(v);}}'
    return f'[](auto &v,std::uint32_t site){{dispatch(v,0x{child}u,site,false);}}'


def prefix(owner, canonical):
    lambdas = [f'[](auto &v){{{reference_name(owner)}(v);}}']
    lambdas += [child_lambda(canonical, child) for child in OWNERS[owner][1]]
    return (f'\n    // BEGIN native effect owner {owner}\n'
            f'    if({DETAIL}::bridge_{owner}(s,' + ',\n        '.join(lambdas) + '))return;\n'
            f'    // END native effect owner {owner}')


def reference(found, name, owner):
    """A canonical body: inline callees to their references, children to the replay."""
    bodies, children = OWNERS[owner]
    text = found[0].replace(f'static void run_{name}({found[1]} &s) {{', f'void {reference_name(name)}(WorldState &s) {{', 1)

    def route(match):
        callee = match[1]
        if callee in bodies:
            return f'{reference_name(callee)}(s)'
        if callee in children:
            return f'{REPLAY}::replay(s,0x{callee}u)'
        return match[0]
    text = re.sub(r'\brun_([0-9a-f]+)\(s\)', route, text)
    # A dispatched child: {const auto target=selected(0x...u,s.image);...dispatch(s,target,site,false);}
    for child in children:
        if re.search(r'const auto target=selected\(0x' + child + r'u,s\.image\);', text):
            text = re.sub(r'\bdispatch\(s,target,0x[0-9a-f]+u,false\)', f'{REPLAY}::replay(s,target)', text)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('void '):
        raise ValueError(f'Effect owner reference {name} kept an unrouted child')
    return text + '\n'


def identity_operands(canonical, found):
    mapping = MAPPING.search(canonical)
    if not mapping:
        return False
    addresses = {int(a, 16): int(b, 16) for a, b in CASE.findall(mapping[0])}
    return all(addresses.get(int(m[1], 16)) == int(m[1], 16) for m in SELECTED.finditer(found[0]))


def admitted(canonical):
    """Owners whose closure bodies match their pins and whose children exist."""
    names = []
    for owner, (bodies, children) in OWNERS.items():
        found = [body(canonical, name) for name in bodies]
        pins = [PINS[name] if isinstance(PINS[name], tuple) else (PINS[name],) for name in bodies]
        if not all(found) or any(digest(f) not in pin for f, pin in zip(found, pins)):
            continue
        if not all(identity_operands(canonical, f) for f in found):
            continue
        # Every child keeps a direct body, or (SITED) the closure body that
        # calls it dispatches it and the unit holds no body for it.
        def routed(child):
            if body(canonical, child):
                return True
            return child in SITED and any(f'const auto target=selected(0x{child}u,s.image);' in f[0] for f in found)
        if not all(routed(child) for child in children):
            continue
        names.append(owner)
    return names


# Owners installed by default: all three, rewritten to the code standard of
# docs/native_replacement/REMAINING_TRANSLATED_20261115.md (logic in
# native_tu15_effect_owners_systems.hpp, adapters in
# native_tu15_effect_owners.hpp) and re-proven (2026-11-16). Live-checked on
# the restored MCC cache (2026-11-16 evening): Armory parity in every role, and
# the MP and campaign verify sweep with 0 differences; see
# docs/native_replacement/EFFECT_OWNERS_20680_C35D0_176A40.md.
BOUND = ('20680', 'c35d0', '176a40')


def install(canonical, output):
    """Return (output, installed entries)."""
    installed, references = [], []
    for owner in admitted(canonical):
        if owner not in BOUND:
            continue
        current = body(output, owner)
        text = prefix(owner, canonical)
        if not current or text in current[0]:
            continue
        start = current[0].index('{') + 1
        output = output[:current.start()] + current[0][:start] + text + current[0][start:] + output[current.end():]
        # Callees first: each reference is declared before its caller.
        for name in reversed(OWNERS[owner][0]):
            references.append(reference(body(canonical, name), name, owner))
        installed.append(owner)
    if not installed:
        return output, []
    first = FIRST_BODY.search(output)
    if not first or REFERENCES_BEGIN in output:
        raise ValueError('Effect owner references need one unbound unit')
    block = REFERENCES_BEGIN + ''.join(references) + REFERENCES_END
    output = output[:first.start()] + block + output[first.start():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, installed
