"""Bind the native TU1.5 device update callbacks (1179C0 machine update, 10B840 device update).

The prefix is installed only into the build-local specialized copy of the
scene device constructor unit, and only when the complete canonical body
matches its pin (state type normalized), every selected() operand of the body
is the identity under TU1.5 (the unit's own resolver), every direct child the
closure enters keeps its body in the unit and every dispatched child keeps its
original dispatch route (B7E50, BBFF0, BF050 and B8760 have no body in the
unit, so the dispatch reaches the required owner). Each bound owner also
receives its verification reference: the unmodified canonical body with only
its child calls routed to the verification replay (HALO2_VERIFY_DEVICE_UPDATE).
A changed or missing body keeps its translated route.
"""
import hashlib
import re

PINS = {
    '1179c0': '61dd675747e98ce04be4453ebe6621d15b639dc606fe431be0b54f2e8ae2fc3c',
    '10b840': '6551e54b84794e050498355a702159d6add7072d3aeacd17379600ee3d608758',
}
# Direct children of each closure, in the bridge argument order (after the
# reference and the dispatch).
CHILDREN = {
    '1179c0': ('1da750', '1ce8d0', '1cebd0', '10c5d0', 'bd080', 'bad60', '10c290', '1181f0', 'bda50', '118350'),
    '10b840': ('10cd70', '10c5d0', '1ce8d0', '10ca30', '121890', '1cebd0', '1181f0'),
}
# Dispatched children: the original route text each canonical body must keep,
# and targets that must not have a body in the unit.
DISPATCHED = {
    '1179c0': (('b7e50', 0x117bd0, 0x117bd5), ('bbff0', 0x117e11, 0x117e16), ('bf050', 0x118096, 0x11809b)),
    '10b840': (('b8760', 0x10bcf0, 0x10bcf5), ('b8760', 0x10bd6d, 0x10bd72), ('bf050', 0x10bdc7, 0x10bdcc)),
}
INCLUDE = '#include "physics/runtime/native_tu15_device_update.hpp"\n'
DETAIL = 'halo2::native_device_update_detail'
REPLAY = 'halo2::native_object_tick_detail'
REFERENCES_BEGIN = '// BEGIN native device update references\n'
REFERENCES_END = '// END native device update references\n'
FIRST_BODY = re.compile(r'^static void run_[0-9a-f]+\(WorldState &s\) \{$', re.M)
RESOLVER = re.compile(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}', re.S)


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def route(target, site, back):
    return (f'{{const auto target=selected(0x{target}u,s.image);s.push(0x{site:x}u,selected(0x{back:x}u,s.image));'
            f'dispatch(s,target,0x{site:x}u,false);}}')


def identity_operands(text, found):
    operands = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
    resolver = RESOLVER.search(text)
    if not resolver:
        return False
    mapping = {int(a, 16): int(b, 16) for a, b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;', resolver[0])}
    return all(mapping.get(value) == value for value in operands)


def reference_name(name):
    return f'native_device_update_reference_{name}'


def prefix(name):
    lambdas = [f'[](auto &v){{{reference_name(name)}(v);}}',
               '[](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}']
    lambdas += [f'[](auto &v){{run_{child}(v);}}' for child in CHILDREN[name]]
    return (f'\n    // BEGIN native device update {name}\n'
            f'    if({DETAIL}::bridge_{name}(s,' + ',\n        '.join(lambdas) + '))return;\n'
            f'    // END native device update {name}')


def reference(found, name):
    """The canonical body with only its child calls routed to the replay."""
    text = found[0].replace(f'static void run_{name}({found[1]} &s) {{', f'void {reference_name(name)}(WorldState &s) {{', 1)
    text = re.sub(r'\brun_([0-9a-f]+)\(s\)', REPLAY + r'::replay(s,0x\1u)', text)
    text = re.sub(r'\bdispatch\(s,target,0x[0-9a-f]+u,false\)', REPLAY + r'::replay(s,target)', text)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('void '):
        raise ValueError(f'Device update reference {name} kept an unrouted child')
    return text + '\n'


def admitted(canonical):
    """Owners whose canonical body matches its pin, operands and children."""
    names = []
    for name, pin in PINS.items():
        found = body(canonical, name)
        if not found or digest(found) != pin or not identity_operands(canonical, found):
            continue
        if not all(body(canonical, child) for child in CHILDREN[name]):
            continue
        if not all(route(target, site, back) in found[0] and not body(canonical, target)
                   for target, site, back in DISPATCHED[name]):
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
        raise ValueError('Device update references need one unbound scene device unit')
    block = REFERENCES_BEGIN + ''.join(references) + REFERENCES_END
    output = output[:first.start()] + block + output[first.start():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, installed
