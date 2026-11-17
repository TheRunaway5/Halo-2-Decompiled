"""Bind the native TU1.5 live-support leaves of group L
(docs/native_replacement/LIVE_SUPPORT_LEAVES.md).

Systems: src/physics/runtime/native_tu15_live_support_systems.hpp; boundary
adapters (kernel_*): native_tu15_live_support_leaves.hpp, run by the
shared leaf framework (native_tu15_leaves::run: TU1.5 image, literal cursor
or no callbacks, no active trace, the census switch; HALO2_REFERENCE_MATRIX_LEAVES
and HALO2_VERIFY_MATRIX_LEAVES). A leaf is installed into a specialized unit
only when its complete canonical body matches one of its pins (state type
normalized): the literal live-support form, or the identical body whose
selected() operands the unit maps to themselves. Any other body keeps its
translated route. A leaf whose native includes a child routine (16E600
performs its strncpy call, 32A4E0) also requires the child's pinned body in
the same unit. A leaf may name a kernel per pin: 32CC0C's kernel follows the
caller unit's x87 status model (C1 kept by the store helpers, or not).
A leaf with genuine children (CHILD_CALLS, 22A790) runs through
run_with_children with one route per child and a verification reference: a
lambda copy of its canonical body with the child calls routed to
replay_child (any other call refuses the binding). Canonical copies of a leaf
under another unit's names (CANONICAL, the physics world unit's 16D220) bind
when their TU1.5 form is the pinned body.
Called from spatial_owner_bindings in tools/specialize_native_operands.py.
"""
import hashlib
import re

INCLUDE = '#include "physics/runtime/native_tu15_live_support_leaves.hpp"\n'
# name -> (pins, kernel). The second pin is the selected()-operand form of
# the copies in other units. kernel is one name, or a pin -> name map.
LEAVES = {
    'c9550': (('24ef03b17fd3128c4c2070c1b8f2847dcf08969cd7410268b44468b5a1e0e5ef',
               'ac539608764a7f26783f56bb2fb6148223a757f9b81403e9677781fa1f05ed85'), 'kernel_c9550'),
    '1ec970': (('cf06a2abd49d60be9c9034296078ffc1ea8e24715dd2a1e1d245dafb79047789',
                'f369d48225ea895a5711f2e2efafee96d1636dbb2c9afab0f5b85fe8f96b1fe6'), 'kernel_1ec970'),
    '1f7f60': (('8a9f4d3ee34a7ad2d8322277683ad892868028fb0d2fecf731d0a9e093401778',), 'kernel_1f7f60'),
    '1f7e90': (('b4ef98fa0518397db184cbf76be42717eb069ea5c78a8e4627204b4d941ee0f8',), 'kernel_1f7e90'),
    '32a4e0': (('600597ef82f70f99f45f9a714b73f6aeac897a55393e2c7eb3d3a7211273f971',
                'fce0132962550111e65bcf194f7309640f1ade805dc21ed3d5ec3a268ecbc149'), 'kernel_32a4e0'),
    '16e4b0': (('65733c37da0373d56679a89487a462c233229790945d32cc937d38a82f424877',
                '4a1c0865c7b1dafbabd176af35a6e19ac028b473e5c425ea9c715fe70daaa957'), 'kernel_16e4b0'),
    '16e450': (('55da5e253a84c2677e6df46d0c1598fdb0c8e3428d1625417519f787b7ac7c53',
                '14b5c0d66219e712ca73143abbba65ecaa9a445c8ef537bc475a937df42833f4'), 'kernel_16e450'),
    '22a790': (('c1dd64b3c341832ae67ae0d983170b060fec1c0572646c77d9fdf7775e26bedd',
                'b78364a9c211f386ce6ac657e8fe494bad4664469fa513e1cbacae2db64c4d75'), 'kernel_22a790'),
    'c1e90': (('41f4be2f70f13e2b73b758bc1322b05bac9468915003f9b3384f224447c6d20f',
               'f1b544db6d3485e52fc814d89aa17dfdebad44c8b94357cc0c1f444d8f1152d5'), 'kernel_c1e90'),
    '110350': (('916164d4210f78035d62b3212f4efebfd32fb16680fc72ec1f3f83ba786374ee',), 'kernel_110350'),
    '243f20': (('9cf2fa52fc71827bce4c0dfb3a659a028b883cf23f5b49f165dbf3652085ffb7',
                'ab6e13b67ab7e3e224b4d9f9a878ef8b5d68b8a4cd17c4cc4949eccf8b2e6b21'), 'kernel_243f20'),
    # The live-support form spills without C1; the other units' form keeps
    # the hardware C1 of each x87 store.
    '32cc0c': (('f2d0eed9f8a02e38dfc7e0c94fb57c9dac1dfe681e383a58f55f4c6ea49e7c8d',
                'a986de85270b3abd08af1de2d4263b7c734a2cfc7d474e75e05955c08b7778ac',
                '27ecb2521416216d1c5774536ebfec6cce41d13d2fde473aeba92b3e80b7e265'),
               {'f2d0eed9f8a02e38dfc7e0c94fb57c9dac1dfe681e383a58f55f4c6ea49e7c8d': 'kernel_32cc0c<false>',
                 'a986de85270b3abd08af1de2d4263b7c734a2cfc7d474e75e05955c08b7778ac': 'kernel_32cc0c<true>',
                 '27ecb2521416216d1c5774536ebfec6cce41d13d2fde473aeba92b3e80b7e265': 'kernel_32cc0c<true>'}),
    '16e600': (('8c034439ab8f9e0649512eff9fbc514de005a38a1ea9c56f444e23beb1f812af',
                '8db59f55747a1b4a6ba29f1dca103b6825361517c0d2e8b022be88d75af18dad'), 'kernel_16e600'),
    '1211c0': (('261957acc000658f2f545d1e7035c823df6bd142eb6bcf3ae15a3b940b0d1b28',
                '57503aa527485d56be6a746ca663c81cbeb2e25a94c585089b11c1b400de31ed'), 'kernel_1211c0'),
    '171010': (('5f491f9ba7aca431568c671552d9fef7c3307c4680e11df2c7cdc1199a4851bd',
                '8bd6138e7a7a4525f5bf8a0deae6a55a9165277f320e763469e64593fd9bc22a'), 'kernel_171010'),
    '1f8200': (('03e703c7ff3d0c38f4aab78dab1ab2984ea2aefceb5d0300027a11f70ca20bb3',), 'kernel_1f8200'),
    # 138D90 in four units: the live-support form, and the selected()-operand
    # forms of the combat action and model render units (their FISTP spills
    # keep the hardware C1) and of the map world support unit (its spills
    # leave C1 as the FLD cleared it).
    '138d90': (('84713fac7c193b564f4209f28c1b692cee57849f0003f631c399ea68fbfacea3',
                '0ee53257b9a366d57a654dd9907b7b71dfda6afe22f93451d04b5caf5d829c47',
                '5c25d6dfa794d86ed421640dce44e8cab07d8bb44fa81195e3ada666ab4bfa31',
                '0cd2b676a43e66bdc19c19ec606935107d22e2da2abf2f67263336ea99e786f4'),
               {'84713fac7c193b564f4209f28c1b692cee57849f0003f631c399ea68fbfacea3': 'kernel_138d90<true>',
                '0ee53257b9a366d57a654dd9907b7b71dfda6afe22f93451d04b5caf5d829c47': 'kernel_138d90<true>',
                '5c25d6dfa794d86ed421640dce44e8cab07d8bb44fa81195e3ada666ab4bfa31': 'kernel_138d90<true>',
                '0cd2b676a43e66bdc19c19ec606935107d22e2da2abf2f67263336ea99e786f4': 'kernel_138d90<false>'}),
    # 179CD0: the live-support form and the combat action unit's
    # selected()-operand form (its children's routes differ).
    '179cd0': (('640d540f10e81473d55ad3bb3773f696bb18566afb0aab75f12bc0070b4d7fd6',
                '3a203df5f12810471083049fa250afff33f10cd428ee0263703bda6a0ea5e6f0'), 'kernel_179cd0'),
}
# Child routines a leaf's native performs itself: they must match too.
CHILDREN = {
    # 1211C0's dominant-axis call: the 123210 leaf (kernel_123210). Units that
    # do not hold 123210 dispatch to it by its address.
    '1211c0': {'123210': ('8728c0405f5059e10d21fbbba7ccdee7494e1cd16f0c272f2884a3357c0b43ae',)},
    '16e600': {'32a4e0': LEAVES['32a4e0'][0]},
    # 16E4B0 performs its strncpy call; 16E450 its 16E4B0 call (and so strncpy).
    '16e4b0': {'32a4e0': LEAVES['32a4e0'][0]},
    '16e450': {'16e4b0': LEAVES['16e4b0'][0], '32a4e0': LEAVES['32a4e0'][0]},
    # 1F8200 runs its region iteration (1F7E90, 1F7F60) inline.
    '1f8200': {'1f7e90': LEAVES['1f7e90'][0], '1f7f60': LEAVES['1f7f60'][0]},
}
# Leaves with genuine children (native_tu15_leaf_children.hpp): each child
# target's route in the unit, ('dispatch', call site), ('local', name), or
# ('call', call site, name) for a child whose route differs between the
# units' copies (local where the body calls run_<name>, else dispatch). The
# kernel receives one route per child, in this order; verification replays
# the binder's reference copy of the body with those calls routed to
# replay_child(). Any other call in the body refuses the binding.
CHILD_CALLS = {
    '22a790': {0x4057f0: ('dispatch', 0x22a7e6)},
    # 1F8200: the node matrix inverse (affine-inverse framework) and the BSP
    # segment traversal.
    '1f8200': {0x144090: ('dispatch', 0x1f8284), 0x1e6980: ('local', '1e6980')},
    # 138D90: the level offset by bitmap type (2D, 3D, cube) and the texel
    # address.
    '138d90': {0x138510: ('call', 0x138f24, '138510'), 0x1385d0: ('call', 0x138f17, '1385d0'),
               0x1386e0: ('call', 0x138f0e, '1386e0'), 0x138c20: ('call', 0x138f4c, '138c20')},
    # 179CD0: the effects' slot scan (the effect framework's native), group
    # E's attachment, visibility and advance owners, and the effect delete.
    '179cd0': {0x16ea60: ('call', 0x179ced, '16ea60'), 0x17c370: ('call', 0x179d18, '17c370'),
               0x17c5c0: ('call', 0x179d21, '17c5c0'), 0x17a380: ('call', 0x179d59, '17a380'),
               0x17c140: ('call', 0x179d6f, '17c140')},
}
REFERENCE = 'native_live_support_reference_'
# Canonical copies of a leaf in a unit with its own names: local name ->
# (leaf, {local child: child leaf}). A copy binds only when its TU1.5 form,
# with each world_observation_pc(disc, tu15, s) taken at tu15, each selected()
# operand mapped by the unit, and labels and children renamed to their TU1.5
# names, is the leaf's pinned body, and the same holds for each child. In the
# observation-PC form (ObservedWorldState) the prefix runs only while the
# state selects TU1.5 observation PCs. A unit whose store events report its
# canonical (disc-build) PCs while its cursor follows TU1.5 (the physics pool
# startup unit) binds in the store-site form: its TU1.5 form also maps each
# store's PC (s.push and w sites) through the unit, and the native runs with
# run_with_store_sites, reporting each store at the canonical PC that the
# binder's site map gives for the kernel's TU1.5 site.
CANONICAL = {
    '16d220': ('16e600', {'326970': '32a4e0'}),
    '16d070': ('16e450', {'16d0d0': '16e4b0', '326970': '32a4e0'}),
    '16d0d0': ('16e4b0', {'326970': '32a4e0'}),
    '326970': ('32a4e0', {}),
}
STORE_SITE = re.compile(r'(\bs\.push\(|\bw\(s\.memory,s\.observe,)(0x[0-9a-f]+)u,')
OBSERVED = 'ObservedWorldState'
OBSERVATION = re.compile(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)')
HEADER = 'std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image)'
MAPPING = re.compile(re.escape(HEADER) +
    r'\{if\(image==SourcePhysicsContactImage::MapPack11\)return p;switch\(p\)\{'
    r'(?P<cases>(?:case 0x[0-9a-f]+u:return 0x[0-9a-f]+u;)+)')
CASE = re.compile(r'case (0x[0-9a-f]+)u:return (0x[0-9a-f]+)u;')
CALL = re.compile(r'\bselected\(\s*(0x[0-9a-f]+)u\s*,\s*s\.image\s*\)')


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def marker(name):
    return f'    // BEGIN native live-support leaf {name}\n'


def resolve(text, how):
    # A 'call' route is the body's own: local when it calls run_<name>.
    if how[0] != 'call':
        return how
    return ('local', how[2]) if f'run_{how[2]}(s);' in text else ('dispatch', how[1])


def route(target, how):
    kind, value = how
    call = f'dispatch(r,0x{target:x}u,0x{value:x}u,false)' if kind == 'dispatch' else f'run_{value}(r)'
    return f'[](auto &r){{{call};}}'


def prefix(name, kernel, found=None):
    if name in CHILD_CALLS:
        # The verification reference is a lambda in the prefix: the unit
        # partitioner admits only recovered bodies at namespace scope.
        routes = ''.join(',' + route(target, resolve(found[0], how)) for target, how in CHILD_CALLS[name].items())
        return (marker(name) +
                f'    if(halo2::native_tu15_leaves::run_with_children(s,[](auto &io,auto &m){{halo2::native_tu15_leaves::{kernel}(io,m{routes});}},\n'
                f'        []({found[1]} &s) {{\n{routed(found, name)}        }},0x0u))return;\n'
                f'    // END native live-support leaf {name}\n')
    return (marker(name) +
            f'    if(halo2::native_tu15_leaves::run(s,[](auto &io,auto &m){{halo2::native_tu15_leaves::{kernel}(io,m);}},\n'
            f'        [](auto &r){{run_{name}(r);}},0x0u))return;\n'
            f'    // END native live-support leaf {name}\n')


def routed(found, name):
    """The canonical body's statements with its child calls routed to the
    replay; None when a call is not one of the declared children."""
    text = found[0]
    for target, how in CHILD_CALLS[name].items():
        kind, value = resolve(found[0], how)
        if kind == 'dispatch':
            pattern = re.compile(r'\{const auto target=(?:selected\()?0x%xu(?:,s\.image\))?;s\.push\(0x%xu,[^;]*\);'
                                 r'dispatch\(s,target,0x%xu,false\);\}' % (target, value, value))
            text, count = pattern.subn(lambda m: m[0].replace(f'dispatch(s,target,0x{value:x}u,false)',
                                                              f'halo2::native_tu15_leaves::replay_child(s,0x{target:x}u)'), text)
        else:
            count = text.count(f'run_{value}(s);')
            text = text.replace(f'run_{value}(s);', f'halo2::native_tu15_leaves::replay_child(s,0x{target:x}u);')
        if count < 1:
            return None
    if re.search(r'\bdispatch\(|\brequired\(', text):
        return None
    return text[text.index('{\n') + 2:text.rindex('}')]


def reference(found, name):
    """The routed body as a function (the validator's harness form)."""
    statements = routed(found, name)
    return None if statements is None else f'static void {REFERENCE}{name}({found[1]} &s) {{\n{statements}}}\n\n'


def identity_operands(canonical, found):
    operands = CALL.findall(found[0])
    if not operands:
        return True
    mapping = MAPPING.search(canonical)
    if not mapping:
        return False
    addresses = {int(a, 16): int(b, 16) for a, b in CASE.findall(mapping['cases'])}
    return all(addresses.get(int(operand, 16)) == int(operand, 16) for operand in operands)


def matches(canonical, name, pins):
    found = body(canonical, name)
    return bool(found) and digest(found) in pins and identity_operands(canonical, found)


def child_matches(canonical, name, child, pins):
    # A child the unit holds must match its pin; one it does not hold must
    # be the leaf's dispatched call to the child's own address.
    if body(canonical, child):
        return matches(canonical, child, pins)
    return f'selected(0x{child}u,s.image)' in body(canonical, name)[0]


def admitted(canonical, name):
    if name in CHILD_CALLS and (not body(canonical, name) or reference(body(canonical, name), name) is None):
        return False
    return matches(canonical, name, LEAVES[name][0]) and all(
        child_matches(canonical, name, child, pins) for child, pins in CHILDREN.get(name, {}).items())


def unit_mapping(canonical):
    found = MAPPING.search(canonical)
    return {int(a, 16): int(b, 16) for a, b in CASE.findall(found['cases'])} if found else {}


def tu15_form(canonical, local, leaf, children, stores=False):
    # The copy's TU1.5 body, or None when it cannot be formed. With stores,
    # each store site's literal PC is mapped by the unit as well.
    found = body(canonical, local)
    if not found:
        return None
    mapping = unit_mapping(canonical)
    text = OBSERVATION.sub(r'\1', found[0])
    if stores:
        sites = [int(x, 16) for _, x in STORE_SITE.findall(text)]
        if any(x not in mapping for x in sites):
            return None
        text = STORE_SITE.sub(lambda m: f'{m[1]}0x{mapping[int(m[2], 16)]:x}u,', text)
    operands = [int(x, 16) for x in CALL.findall(text)]
    if any(x not in mapping for x in operands):
        return None
    text = CALL.sub(lambda m: f'0x{mapping[int(m[1], 16)]:x}u', text)
    delta = int(leaf, 16) - int(local, 16)
    text = re.sub(r'\bL([0-9a-f]+)\b', lambda m: f'L{int(m[1], 16) + delta:x}', text)
    text = text.replace(f'static void run_{local}(', f'static void run_{leaf}(', 1)
    for child, child_leaf in children.items():
        text = text.replace(f'run_{child}(s)', f'run_{child_leaf}(s)')
    return text.replace(f'{found[1]} &s', 'STATE &s')


def canonical_form(canonical, local):
    # 'plain' or 'stores' (the store-site form), or None.
    leaf, children = CANONICAL[local]
    for stores in (False, True):
        forms = [tu15_form(canonical, local, leaf, children, stores)]
        forms += [tu15_form(canonical, child, child_leaf, children, stores) for child, child_leaf in children.items()]
        pins = [LEAVES[leaf][0]] + [LEAVES[child_leaf][0] for child_leaf in children.values()]
        if all(form is not None and hashlib.sha256(form.encode()).hexdigest() in pin for form, pin in zip(forms, pins)):
            return 'stores' if stores else 'plain'
    return None


def canonical_admitted(canonical, local):
    return canonical_form(canonical, local) is not None


def store_sites(canonical, local):
    # The canonical store PC of each TU1.5 store site of a copy and its
    # children, as the unit maps them.
    mapping = unit_mapping(canonical)
    sites = {}
    for name in (local, *CANONICAL[local][1]):
        for _, site in STORE_SITE.findall(body(canonical, name)[0]):
            sites[mapping[int(site, 16)]] = int(site, 16)
    return sites


def site_map(sites):
    cases = ''.join(f'case 0x{tu15:x}u:return 0x{disc:x}u;' for tu15, disc in sorted(sites.items()))
    return (f'[](std::uint32_t p)->std::uint32_t{{switch(p){{{cases}}}'
            'throw std::runtime_error("Unmapped TU1.5 leaf store site");}')


def canonical_marker(local):
    return f'    // BEGIN native live-support leaf {CANONICAL[local][0]} (canonical {local})\n'


def canonical_prefix(local, state, canonical=None):
    leaf = CANONICAL[local][0]
    kernel = f'[](auto &io,auto &m){{halo2::native_tu15_leaves::{LEAVES[leaf][1]}(io,m);}}'
    if canonical is not None and canonical_form(canonical, local) == 'stores':
        return (canonical_marker(local) +
                f'    if(halo2::native_tu15_leaves::run_with_store_sites(s,{kernel},\n'
                f'        [](auto &r){{run_{local}(r);}},{site_map(store_sites(canonical, local))}))return;\n'
                f'    // END native live-support leaf {leaf} (canonical {local})\n')
    guard = 's.selected_observation_pcs && ' if state == OBSERVED else ''
    return (canonical_marker(local) +
            f'    if({guard}halo2::native_tu15_leaves::run(s,{kernel},\n'
            f'        [](auto &r){{run_{local}(r);}},0x0u))return;\n'
            f'    // END native live-support leaf {leaf} (canonical {local})\n')


def kernel_for(canonical, name):
    kernel = LEAVES[name][1]
    return kernel if isinstance(kernel, str) else kernel[digest(body(canonical, name))]


# The systems follow the code standard (named game data, logic separate
# from the ABI adapter); False holds every leaf on its translated route.
ENABLED = True


def install(canonical, output):
    installed = []
    if not ENABLED:
        return output, installed
    for name in LEAVES:
        if not admitted(canonical, name):
            continue
        kernel = kernel_for(canonical, name)
        current = body(output, name)
        if not current or marker(name) in current[0]:
            continue
        start = current[0].index('{\n') + 2
        text = current[0][:start] + prefix(name, kernel, body(canonical, name)) + current[0][start:]
        output = output[:current.start()] + text + output[current.end():]
        installed.append(name)
    for local in CANONICAL:
        if not canonical_admitted(canonical, local):
            continue
        current = body(output, local)
        if not current or canonical_marker(local) in current[0]:
            continue
        start = current[0].index('{\n') + 2
        output = (output[:current.start()] + current[0][:start] + canonical_prefix(local, current[1], canonical) +
                  current[0][start:] + output[current.end():])
        installed.append(f'{CANONICAL[local][0]}@{local}')
    if installed and INCLUDE not in output:
        output = INCLUDE + output
    return output, installed
