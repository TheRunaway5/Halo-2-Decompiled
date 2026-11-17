"""Exact Primary 314870 raw/prepared source overlay and continuation ownership."""

import hashlib
import re

ENTRY = '314870'
BODY = re.compile(r'static void run_314870\(WorldState &s\) \{.*?\n\}', re.S)
STATE = re.compile(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;', re.S)
RESOLVER = re.compile(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}')
DISPATCHER = re.compile(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}')
INCLUDE = '#include "physics/runtime/native_tu15_solver_hull_clear_source_owner.hpp"\n'
INCLUDE_ANCHOR = '#include "halo2/source_gameplay_fade_phase_tu15.hpp"\n'
OWNER_MARKER = 'native_solver_hull_clear_owner_detail'
RESUME_MARKER = 'native_hull_clear_resume_'
FRONTIERS = (
    (0x314870, 0x314876), (0x314876, 0x314877), (0x31487d, 0x31487e),
    (0x314890, 0x314896), (0x314896, 0x31489a), (0x31489a, 0x3148a0),
    (0x3148a8, 0x3148ae), (0x3148c0, 0x3148c6), (0x3148c6, 0x3148ca),
    (0x3148ca, 0x3148d0), (0x3148d8, 0x3148d9), (0x3148d9, 0x3148da),
)
LABELS = {successor: RESUME_MARKER + f'{successor:x}' for _, successor in FRONTIERS}
RAW_CONTEXTS = (
    ('41b826852497e630c1b0060a1828b0672fd79fbe7446db1aff0a3a630cc5d4cb', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0', '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'),
    ('41b826852497e630c1b0060a1828b0672fd79fbe7446db1aff0a3a630cc5d4cb', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'),
)
PREPARED_CONTEXTS = (
    ('db1b2077c227efbc8b5c2fba783529fd0b73a53168dbe47621aab8ed8d0a277c', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052', '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'),
    ('db1b2077c227efbc8b5c2fba783529fd0b73a53168dbe47621aab8ed8d0a277c', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'),
)


def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()


def prefix(prepared):
    mode = 'true' if prepared else 'false'
    choices = ''.join(f'case 0x{pc:x}u:goto {label};' for pc, label in LABELS.items())
    return (f'\n    switch(halo2::{OWNER_MARKER}::bridge<{mode}>(s)){{case 0u:break;case 1u:return;' +
            choices + 'default:throw std::logic_error("Unsupported native hull-clear continuation");}')


PREFIXES = {prepared: prefix(prepared) for prepared in (False, True)}


def instruction(pc, prepared):
    if prepared:
        return f's.instruction_literal(0x{pc:x}u);'
    return f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}'


def overlay(body, prepared):
    for pc, label in LABELS.items():
        needle = instruction(pc, prepared)
        if body.count(needle) != 1:
            raise ValueError('Hull-clear continuation instruction drift')
        body = body.replace(needle, label + ': ' + needle, 1)
    at = body.index('{') + 1
    return body[:at] + PREFIXES[prepared] + body[at:]


def split_body(body, intermediate=False):
    keys = [key for key, value in PREFIXES.items() if value in body]
    if not keys:
        if OWNER_MARKER in body or RESUME_MARKER in body:
            raise ValueError('Partial hull-clear overlay')
        return body, None
    if len(keys) != 1:
        raise ValueError('Ambiguous hull-clear format')
    key = keys[0]
    value = PREFIXES[key]
    at = body.index('{') + 1
    if not body.startswith(value, at) or body.count(value) != 1:
        raise ValueError('Hull-clear prefix moved from authored opening')
    result = body[:at] + body[at + len(value):]
    prepared = intermediate or key
    for pc, label in LABELS.items():
        marker = label + ': '
        if result.count(marker + instruction(pc, prepared)) != 1:
            raise ValueError('Hull-clear label moved from authored instruction')
        result = result.replace(marker, '', 1)
    if overlay(result, prepared).replace(PREFIXES[prepared], value, 1) != body:
        raise ValueError('Hull-clear exact overlay reconstruction drift')
    return result, key


def canonical_body(body):
    return split_body(body)[0]


def unique(regex, source, name):
    matches = list(regex.finditer(source))
    if len(matches) != 1:
        raise ValueError('Missing or ambiguous hull-clear ' + name)
    return matches[0]


def context(source):
    return tuple(unique(regex, source, name) for regex, name in
                 ((BODY, 'owner'), (RESOLVER, 'resolver'), (DISPATCHER, 'dispatcher'), (STATE, 'LiveState')))


def validate(source, raw=False, intermediate=False, prepared_only=False):
    match, resolver, dispatcher, state = context(source)
    body, key = split_body(match[0], intermediate)
    if (raw and key is True) or (prepared_only and key is False):
        raise ValueError('Wrong hull-clear source format')
    contexts = PREPARED_CONTEXTS if intermediate or key is True or prepared_only else RAW_CONTEXTS
    if tuple(digest(value) for value in (body, resolver[0], dispatcher[0], state[0])) not in contexts:
        raise ValueError('Complete hull-clear body/resolver/dispatcher/LiveState drift')
    installed = key is not None
    if source.count(INCLUDE) != int(installed) or (installed and source.count(INCLUDE + INCLUDE_ANCHOR) != 1):
        raise ValueError('Incomplete hull-clear include ownership')
    if source.count(OWNER_MARKER) != int(installed) or source.count(RESUME_MARKER) != 2 * len(LABELS) * int(installed):
        raise ValueError('Foreign hull-clear owner or continuation marker')
    return match, body, key


def authenticate(source):
    if not BODY.search(source):
        if any(marker in source for marker in (INCLUDE, OWNER_MARKER, RESUME_MARKER)):
            raise ValueError('Missing actual hull-clear owner')
        return None
    return validate(source, raw=True)


def replace_profile(source, prepared):
    found = validate(source, prepared_only=True) if prepared else authenticate(source)
    if found is None:
        return source
    match, body, key = found
    if key is not None:
        return source
    if source.count(INCLUDE_ANCHOR) != 1:
        raise ValueError('Hull-clear include anchor drift')
    replaced = source[:match.start()] + overlay(body, prepared) + source[match.end():]
    return replaced.replace(INCLUDE_ANCHOR, INCLUDE + INCLUDE_ANCHOR, 1)


def replace_solver_hull_clear(source):
    return replace_profile(source, False)


def replace_solver_hull_clear_prepared(source):
    return replace_profile(source, True)


def remove_solver_hull_clear(source):
    found = validate(source) if PREFIXES[True] in source else authenticate(source)
    if found is None:
        return source
    match, body, _ = found
    return (source[:match.start()] + body + source[match.end():]).replace(INCLUDE, '', 1)


def rebind_prepared(source):
    match, body, key = validate(source, intermediate=True)
    if key is None:
        raise ValueError('Missing hull-clear prepared output prefix')
    return source[:match.start()] + overlay(body, True) + source[match.end():]


def installed_entries(source):
    _, _, key = validate(source)
    if key is None:
        raise ValueError('Hull-clear owner is not installed')
    return [ENTRY]
