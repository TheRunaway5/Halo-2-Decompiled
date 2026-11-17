"""Bind the native TU1.5 object tick children (BE850/BE610/C0F60/10DD80/C0200/1D5790/1E15D0/C0910).

The prefix is installed only into the build-local specialized copy of the
live-support unit, and only when the complete canonical body matches its pin
(state type normalized), every direct child the closure enters keeps its body
in the unit and every routed child keeps its original route (dispatch or the
required owner at its original site). Each bound owner also receives its
verification reference: the unmodified canonical body with only its child
calls routed to the verification replay (HALO2_VERIFY_OBJECT_SUBTREE). A
changed or missing body keeps its translated route.
"""
import hashlib
import re

PINS = {
    'be850': 'fcb5f63844dc005a881ebf41f10a2f67fa6c541d8deb0520c7ed5a14221687d8',
    'be610': '01388185c8a5797345587c71e9f495bd531b446f48efad065b75bbe7d32fa7cd',
    # C0F60's C100F call is a required boundary since the light callers'
    # re-emission (tools/regenerate_light_caller_boundaries_tu15.py).
    'c0f60': 'ec94382b336fb1b5ca936ef5832e656308e48ee90a0ec9fb1c855506bf924380',
    '10dd80': '310060d43344a26a10ede42ac47088350ec933e3a0e32800a6064544f9f004a9',
    'c0200': 'c41a1f41e4791ce05c006c4bdc5d1666d7e9de3b7ed96ccf05836fdfcba66d17',
    '1d5790': 'df0ff49f463e4b8a609f70a1c226b4175d8950e9c13464480b363b965546117c',
    '1e15d0': '9dcfc8b473af87a7df592e58efb0792f9f3cb2c9e03cd1cfb3f0389e98a5128c',
    'c0910': '8db4eac3d20c7bb1774f3be441a238239f05c45e634d7ec9bd1c18f700fd8686',
}
# Direct children of each closure, in the bridge argument order.
CHILDREN = {
    'be850': ('be610', '1dbbd0', '10dd80', 'c0f60', 'be850', 'c0200'),
    'be610': ('1d9f50', '1dc610', 'bb200', 'b9460', '1e1980'),
    'c0f60': ('bbd10', 'c5250', 'c4d30', 'c3750', 'c0f60'),
    '10dd80': (),
    'c0200': ('bac30', '10e1f0'),
    '1d5790': ('1e15d0',),
    '1e15d0': (),
    'c0910': ('bcb70', '134730'),
}
# Routed children: the original route text each canonical body must keep.
ROUTES = {
    'be850': ('s.required(0xbe892u,0xbf0c0u,false);', 's.required(0xbe941u,0xbff20u,false);'),
    'be610': ('{const auto target=0x1d9ec0u;s.push(0xbe664u,0xbe669u);dispatch(s,target,0xbe664u,false);}',
              '{const auto target=0x1d8c80u;s.push(0xbe718u,0xbe71du);dispatch(s,target,0xbe718u,false);}',
              '{const auto target=0x1d8d80u;s.push(0xbe72fu,0xbe734u);dispatch(s,target,0xbe72fu,false);}',
              '{const auto target=0xb9770u;s.push(0xbe74eu,0xbe753u);dispatch(s,target,0xbe74eu,false);}'),
    '10dd80': ('{const auto target=s.reg(0,4,0);s.push(0x10ddc1u,0x10ddc3u);dispatch(s,target,0x10ddc1u,false);}',),
    'c0f60': ('s.push(0xc100fu,0xc1014u);s.required(0xc100fu,0xc4d30u,false);',),
}
REQUIRED = {'be850'}
DISPATCHING = {'be610', '10dd80'}
# Children that leave the unit through the World's required owner at their
# original call site (the CALL before the return address the closure pushed)
# instead of the unit's own body. They reach the light callers' binding there
# (physics_live_world_tu15.cpp): C0910's BCB70 calls (C09EC, C0A57) run the
# attachments track's native object_function_value, and C0F60's C4D30 call
# (C100F) runs placement's native light_reconnect, by default; the unit's
# BCB70 and C4D30 run under HALO2_REFERENCE_LIGHT_CALLERS_TU15.
REQUIRED_CHILDREN = {('c0910', 'bcb70'), ('c0f60', 'c4d30')}
INCLUDE = '#include "physics/runtime/native_tu15_object_subtree.hpp"\n'
DETAIL = 'halo2::native_object_subtree_detail'
REPLAY = 'halo2::native_object_tick_detail'
REFERENCES_BEGIN = '// BEGIN native object subtree references\n'
REFERENCES_END = '// END native object subtree references\n'
FIRST_BODY = re.compile(r'^static void run_[0-9a-f]+\(WorldState &s\) \{$', re.M)


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def reference_name(name):
    return f'native_object_subtree_reference_{name}'


def prefix(name):
    lambdas = [f'[](auto &v){{{reference_name(name)}(v);}}']
    if name in REQUIRED:
        lambdas.append('[](auto &v,std::uint32_t target,std::uint32_t site){v.required(site,target,false);}')
    if name in DISPATCHING:
        lambdas.append('[](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}')
    lambdas += [f'[](auto &v){{v.required(r(v.memory,v.gp[4],4)-5u,0x{child}u,false);}}'
                if (name, child) in REQUIRED_CHILDREN else f'[](auto &v){{run_{child}(v);}}' for child in CHILDREN[name]]
    return (f'\n    // BEGIN native object subtree {name}\n'
            f'    if({DETAIL}::bridge_{name}(s,' + ',\n        '.join(lambdas) + '))return;\n'
            f'    // END native object subtree {name}')


def reference(found, name):
    """The canonical body with only its child calls routed to the replay."""
    text = found[0].replace(f'static void run_{name}({found[1]} &s) {{', f'void {reference_name(name)}(WorldState &s) {{', 1)
    text = re.sub(r'\brun_([0-9a-f]+)\(s\)', REPLAY + r'::replay(s,0x\1u)', text)
    text = re.sub(r'\bdispatch\(s,(target|0x[0-9a-f]+u),0x[0-9a-f]+u,false\)', REPLAY + r'::replay(s,\1)', text)
    text = re.sub(r'\bs\.required\(0x[0-9a-f]+u,(0x[0-9a-f]+u),false\)', REPLAY + r'::replay(s,\1)', text)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('void '):
        raise ValueError(f'Object subtree reference {name} kept an unrouted child')
    return text + '\n'


def admitted(canonical):
    """Owners whose canonical body matches its pin and whose children exist."""
    names = []
    for name, pin in PINS.items():
        found = body(canonical, name)
        if not found or digest(found) != pin:
            continue
        if not all(body(canonical, child) for child in CHILDREN[name]):
            continue
        if not all(route in found[0] for route in ROUTES.get(name, ())):
            continue
        names.append(name)
    return names


def install(canonical, output):
    """Return (output, installed entries)."""
    names = admitted(canonical)
    installed, references = [], []
    for name in names:
        current = body(output, name)
        text = prefix(name)
        if not current or text in current[0]:
            continue
        start = current[0].index('{') + 1
        output = output[:current.start()] + current[0][:start] + text + current[0][start:] + output[current.end():]
        references.append(reference(body(canonical, name), name))
        installed.append(name)
    if not installed:
        return output, []
    first = FIRST_BODY.search(output)
    if not first or REFERENCES_BEGIN in output:
        raise ValueError('Object subtree references need one unbound live-support unit')
    block = REFERENCES_BEGIN + ''.join(references) + REFERENCES_END
    output = output[:first.start()] + block + output[first.start():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, installed
