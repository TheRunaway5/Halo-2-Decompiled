"""Exact first codec triple overlays for the two genuine TU1.5 producers."""
import hashlib
import re

QUATERNION, TRANSLATION, SCALE = 0x2963D0, 0x296410, 0x2CEC60
UNITS = (
    'src/physics/physics_live_support_tu15_generated.cpp',
    'src/rendering/models/source_model_render_tu15_generated.cpp',
)
ENTRY_GROUPS = ((QUATERNION, TRANSLATION), (QUATERNION, TRANSLATION, SCALE))
SITES = {
    QUATERNION: (0x2963D6, 0x2963D9, 0x2963E0, 0x2963E5, 0x2963ED, 0x2963F7, 0x2963F9,
                 0x2963FC, 0x2963FF, 0x296402, 0x296405, 0x296408, 0x29640B),
    TRANSLATION: (0x296416, 0x296419, 0x296420, 0x296425, 0x29642E, 0x296431, 0x296439,
                  0x29643E, 0x296441, 0x296444, 0x296447, 0x29644A),
    SCALE: (0x2CEC65, 0x2CEC68, 0x2CEC6F, 0x2CEC75, 0x2CEC7B, 0x2CEC7E, 0x2CEC83, 0x2CEC86),
}
NAMES = dict(zip(SITES, ('quaternion', 'translation', 'scale'), strict=True))
LABELS = {entry: {pc: f'native_codec_copy_resume_{pc:x}: ' for pc in sites} for entry, sites in SITES.items()}
INCLUDE = '#include "physics/runtime/native_tu15_codec_copy_source_owner.hpp"\n'
ANCHOR = '#include "halo2/source_presented_model_pose_tu15.hpp"\n'
BODY = {entry: re.compile(rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}', re.S) for entry in SITES}
CONTEXT = tuple(re.compile(pattern, re.S) for pattern in (
    r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',
    r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',
    r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',
))


def _prefix(entry, literal):
    owner = f'halo2::native_codec_copy_owner_detail::Entry::{NAMES[entry]}'
    cases = ''.join(f'case 0x{pc:x}u:goto {label[:-2]};' for pc, label in LABELS[entry].items())
    return (f'\n    switch(halo2::native_codec_copy_owner_detail::bridge<{owner},{str(literal).lower()}>(s))'
            '{case 0u:break;case 1u:return;' + cases +
            'default:throw std::logic_error("Unsupported native codec copy continuation");}')


PREFIXES = {entry: {literal: _prefix(entry, literal) for literal in (False, True)} for entry in SITES}
# Complete pinned tuples from the genuine Primary/ModelRender producer audit.
CONTEXTS = {
    # primary current raw
    ('622a95e81166444ba0a20faeabf37c58165c275df2b56a6a3b5be93c44f09bf8',
     'dbab326186a305eefe221de75b441ebbda8ee371e3967487ef8ac317b0d553f7',
     '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
     '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
     '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'): (0, False),
    # primary current prepared
    ('6ffd17d97d8c295d2f3d69f9ca92bb7cc99175d943518ffbfcdbb360f688173a',
     '58004bb8e589bf0be7d827b41f17674471afa990fabaca2d3eae79e7951498d4',
     '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
     '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052',
     '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'): (0, True),
    # primary fresh raw
    ('622a95e81166444ba0a20faeabf37c58165c275df2b56a6a3b5be93c44f09bf8',
     'dbab326186a305eefe221de75b441ebbda8ee371e3967487ef8ac317b0d553f7',
     '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
     'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849',
     '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): (0, False),
    # primary fresh prepared
    ('6ffd17d97d8c295d2f3d69f9ca92bb7cc99175d943518ffbfcdbb360f688173a',
     '58004bb8e589bf0be7d827b41f17674471afa990fabaca2d3eae79e7951498d4',
     '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
     '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851',
     '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): (0, True),
    # model current raw
    ('836317fe44b6181ac4e5f7533b834b6240843db44ac037b38f8bfbf6bc489869',
     '0cf1dfae231a0afe10af18e3490172cf2878634994f28384acdf21100982d8c3',
     'e271e4c17e2618f7c0155e787bfda13982fff799c6a1c6fd1626f637d92bcbcf',
     '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db',
     'f928af6956a3965aef767189095b9732744dcbdb209a345a3aec57baa618e0d1',
     '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): (1, False),
    # model current prepared
    ('6ffd17d97d8c295d2f3d69f9ca92bb7cc99175d943518ffbfcdbb360f688173a',
     '58004bb8e589bf0be7d827b41f17674471afa990fabaca2d3eae79e7951498d4',
     '6e8c96b6ce6521c9fde7bf146d71347a5b383ee5a1a72ceb65939ee2da62836b',
     '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db',
     'c112d857c78bf307a089c1297c9b000870c293149fd78edd9852012887dd439d',
     '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): (1, True),
    # model fresh raw
    ('622a95e81166444ba0a20faeabf37c58165c275df2b56a6a3b5be93c44f09bf8',
     'dbab326186a305eefe221de75b441ebbda8ee371e3967487ef8ac317b0d553f7',
     'e71842df6b085697b921fab83368001e5fa7ff12183b0592d3414c4fbb5bb9d8',
     '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db',
     '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604',
     '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): (1, False),
    # model fresh prepared
    ('6ffd17d97d8c295d2f3d69f9ca92bb7cc99175d943518ffbfcdbb360f688173a',
     '58004bb8e589bf0be7d827b41f17674471afa990fabaca2d3eae79e7951498d4',
     '6e8c96b6ce6521c9fde7bf146d71347a5b383ee5a1a72ceb65939ee2da62836b',
     '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db',
     '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604',
     '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): (1, True),
}


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def _callback(pc, literal):
    value = f'0x{pc:x}u'
    return f's.instruction_literal({value});' if literal else f'if(s.services.instruction){{s.services.instruction({value});}}'


def _unbound(entry, body):
    position = body.index('{') + 1
    present = [literal for literal, prefix in PREFIXES[entry].items() if body[position:].startswith(prefix)]
    if not present:
        if 'native_codec_copy_' in body:
            raise ValueError('Partial or misplaced codec copy prefix/label')
        return body, None
    if len(present) != 1:
        raise ValueError('Ambiguous codec copy prefix format')
    literal = present[0]
    prefix = PREFIXES[entry][literal]
    if body.count(prefix) != 1:
        raise ValueError('Duplicate codec copy prefix')
    result = body[:position] + body[position + len(prefix):]
    for pc, label in LABELS[entry].items():
        if result.count(label) != 1 or sum(result.count(label + _callback(pc, form)) for form in (False, True)) != 1:
            raise ValueError(f'Codec copy continuation label is missing or relocated: {pc:#x}')
        result = result.replace(label, '', 1)
    if 'native_codec_copy_' in result:
        raise ValueError('Foreign codec copy label or prefix in owner')
    return result, literal


def canonical_body(entry, body, *, allow_transition=False):
    result, literal = _unbound(entry, body)
    prepared = 's.instruction_literal(' in result
    if literal is not None and literal != prepared and not (allow_transition and prepared and not literal):
        raise ValueError('Codec copy source format tag mismatch')
    return result


def authenticate(source, *, allow_transition=False):
    matches = {entry: list(pattern.finditer(source)) for entry, pattern in BODY.items()}
    group = tuple(entry for entry, values in matches.items() if values)
    if group not in ENTRY_GROUPS or any(len(matches[entry]) != 1 for entry in group):
        raise ValueError('Codec copy owner set is absent or ambiguous')
    owners = {entry: matches[entry][0] for entry in group}
    canonical = {entry: canonical_body(entry, match[0], allow_transition=allow_transition) for entry, match in owners.items()}
    components = [pattern.findall(source) for pattern in CONTEXT]
    if any(len(values) != 1 for values in components):
        raise ValueError('Codec copy complete resolver/dispatcher/State context missing')
    context = tuple(map(digest, (*canonical.values(), *(values[0] for values in components))))
    if context not in CONTEXTS:
        raise ValueError('Codec copy complete body/resolver/dispatcher/State context drift')
    owner, prepared = CONTEXTS[context]
    if ENTRY_GROUPS[owner] != group or any(('s.instruction_literal(' in body) != prepared for body in canonical.values()):
        raise ValueError('Codec copy authenticated owner/format mismatch')
    profiles = {_unbound(entry, match[0])[1] for entry, match in owners.items()}
    if len(profiles) != 1:
        raise ValueError('Codec copy partial group or mixed format tags')
    tag = profiles.pop()
    installed = tag is not None
    if source.count(INCLUDE) != int(installed):
        raise ValueError('Codec copy include/prefix ownership drift')
    # Each exact prefix contains the bridge and the Entry type. Counting the
    # namespace as well as the complete prefix rejects extra direct/aliased
    # calls that would otherwise acquire the installed header transitively.
    if source.count('native_codec_copy_owner_detail') != (2 * len(group) if installed else 0):
        raise ValueError('Foreign codec copy bridge outside its exact owner prefix')
    expected_labels = 2 * sum(len(LABELS[entry]) for entry in group) if installed else 0
    if source.count('native_codec_copy_resume_') != expected_labels:
        raise ValueError('Foreign codec copy continuation outside its exact owner')
    if source.count(ANCHOR) != 1 or source.index(ANCHOR) > source.index('namespace halo2 {'):
        raise ValueError('Codec copy include anchor drift')
    if installed and source.count(ANCHOR + INCLUDE) != 1:
        raise ValueError('Codec copy include is misplaced')
    for entry in SITES:
        for label in LABELS[entry].values():
            if source.count(label) != int(installed and entry in group):
                raise ValueError('Foreign codec copy label outside its owner')
        for form, prefix in PREFIXES[entry].items():
            if source.count(prefix) != int(tag is form and entry in group):
                raise ValueError('Foreign codec copy prefix outside its owner')
    return owners, canonical, owner, prepared, tag


def replace_codec_copy(source):
    if not any(pattern.search(source) for pattern in BODY.values()):
        if INCLUDE in source or 'native_codec_copy_' in source:
            raise ValueError('Codec copy overlay lacks a complete producer')
        return source
    matches, canonical, _, literal, tag = authenticate(source)
    if tag is not None:
        return source
    result = source
    for entry, match in reversed(sorted(matches.items(), key=lambda item: item[1].start())):
        body = canonical[entry]
        for pc, label in LABELS[entry].items():
            callback = _callback(pc, literal)
            if body.count(callback) != 1:
                raise ValueError('Codec copy exact retained continuation is missing')
            body = body.replace(callback, label + callback, 1)
        position = body.index('{') + 1
        body = body[:position] + PREFIXES[entry][literal] + body[position:]
        result = result[:match.start()] + body + result[match.end():]
    return result.replace(ANCHOR, ANCHOR + INCLUDE, 1)


def remove_codec_copy(source):
    matches, canonical, _, _, tag = authenticate(source)
    if tag is None:
        return source
    for entry, match in reversed(sorted(matches.items(), key=lambda item: item[1].start())):
        source = source[:match.start()] + canonical[entry] + source[match.end():]
    return source.replace(INCLUDE, '', 1)


def installed_entries(source):
    matches, _, _, _, tag = authenticate(source)
    if tag is None:
        raise ValueError('Codec copy group lacks installed entry prefixes')
    return [f'{entry:x}' for entry in matches]


def rebind_prepared(source):
    matches, _, _, prepared, tag = authenticate(source, allow_transition=True)
    if not prepared or tag is None:
        raise ValueError('Codec copy retag requires the prepared installed group')
    if not tag:
        for entry in matches:
            source = source.replace(PREFIXES[entry][False], PREFIXES[entry][True], 1)
    authenticate(source)
    return source
