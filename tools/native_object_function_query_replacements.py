"""Authenticate the Primary C9070 overlay and its exact four genuine calls."""
import hashlib
import re

ENTRY = 'c9070'
HEADER = 'static void run_c9070(WorldState &s) {'
OWNER = 'native_object_function_query_owner_detail'
DETAIL = 'native_object_function_query_detail'
BASENAME = 'native_tu15_object_function_query_source_owner.hpp'
INCLUDE = '#include "gameplay/objects/' + BASENAME + '"\n'
ANCHOR = '#include "gameplay/animation/native_tu15_node_function_progress_source_owner.hpp"\n'
BODY = re.compile(r'^' + re.escape(HEADER) + r'.*?^\}', re.M | re.S)
CONTEXT = (
    r'^std::uint32_t selected[^\n]+',
    r'^static void dispatch[^\n]+',
    r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;',
    r'^std::uint32_t sign_extend[^\n]+\{.*?^\}',
    r'^void source_run_physics_live_support_tu15\(.*?^\}',
    r'^bool source_physics_live_support_tu15_owns_entry[^\n]+',
)
PINS = {
    'current-prepared': (
        'b3b50f1d4f9826f000934a090b693c0fc6c047b7b56a2abd15bafeded5343a81',
        '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
        '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052',
        '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '912e109873b4775f13979a32e325106d74712a3e4a0bf43d5485d911cd01f35a',
        'f53eb05fed15f9f03fb04925d9a466a46992c9c79539232e8c54f76a96b43fab',
    ),
    'current-raw': (
        'e3c60cfb8d2afd18bb4f03f3557dc2d24561155260938d8fdc40fa9f0f157ef7',
        '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
        '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
        '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '912e109873b4775f13979a32e325106d74712a3e4a0bf43d5485d911cd01f35a',
        'f53eb05fed15f9f03fb04925d9a466a46992c9c79539232e8c54f76a96b43fab',
    ),
    'fresh-prepared': (
        'b3b50f1d4f9826f000934a090b693c0fc6c047b7b56a2abd15bafeded5343a81',
        '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
        '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '7e227c0aa7f7b21a540fa749c52d95a75432e51a9513543be7bddc25fa58ca09',
        '5a8d30ef7f8b2b214d9e32e5e4cdb91d43c2556853b5ad03692086703cec016c',
    ),
    'fresh-raw': (
        'e3c60cfb8d2afd18bb4f03f3557dc2d24561155260938d8fdc40fa9f0f157ef7',
        '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
        'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '7e227c0aa7f7b21a540fa749c52d95a75432e51a9513543be7bddc25fa58ca09',
        '5a8d30ef7f8b2b214d9e32e5e4cdb91d43c2556853b5ad03692086703cec016c',
    ),
}


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def prefix(literal):
    tag = 'true' if literal else 'false'
    return ('\n    if(halo2::' + OWNER + '::bridge<' + tag + '>(s,halo2::' + OWNER + '::Children{'
            '[](auto&state){dispatch(state,0xd3750u,0xc9101u,false);},'
            '[](auto&state){run_d3950(state);},[](auto&state){run_1046c0(state);}}))return;')


PREFIXES = {literal: prefix(literal) for literal in (False, True)}


def split(body):
    markers = [(literal, marker) for literal, marker in PREFIXES.items() if marker in body]
    if not markers:
        if OWNER in body or DETAIL in body:
            raise ValueError('Unknown native C9070 marker')
        return body, None
    if len(markers) != 1:
        raise ValueError('Conflicting native C9070 formats')
    literal, marker = markers[0]
    if body.count(marker) != 1 or not body.startswith(HEADER + marker):
        raise ValueError('Native C9070 marker moved from the entry')
    return HEADER + body[len(HEADER) + len(marker):], literal


def canonical_body(body):
    return split(body)[0]


def authenticate(source, allow_old_tag=False):
    if 'void source_run_physics_live_support_tu15(' not in source:
        if any(marker in source for marker in (OWNER, DETAIL, BASENAME)):
            raise ValueError('Native C9070 marker outside Primary')
        return None
    bodies = list(BODY.finditer(source))
    if len(bodies) != 1:
        raise ValueError('Missing or ambiguous complete Primary C9070')
    match = bodies[0]
    body, installed = split(match[0])
    raw = re.findall(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\}', body)
    prepared = re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);', body)
    if (len(raw), len(prepared)) not in ((144, 0), (0, 144)):
        raise ValueError('C9070 requires 144 uniformly formatted source sites')
    literal = bool(prepared)
    if installed is not None and installed != literal and not allow_old_tag:
        raise ValueError('Native C9070 callback format changed')
    context = [re.findall(pattern, source, re.M | re.S) for pattern in CONTEXT]
    if any(len(parts) != 1 for parts in context):
        raise ValueError('Missing or ambiguous exact Primary C9070 context')
    pins = tuple(digest(part) for part in (body, *(parts[0] for parts in context)))
    if sum(expected == pins for name, expected in PINS.items() if name.endswith('-prepared') == literal) != 1:
        raise ValueError('Complete correlated Primary C9070 source changed')
    calls = ('s.push(0xc9101u,0xc9106u);dispatch(s,target,0xc9101u,false);',
             's.push(0xc9108u,0xc910du);run_d3950(s);',
             's.push(0xc9186u,0xc918bu);run_d3950(s);',
             's.push(0xc92a8u,0xc92adu);run_1046c0(s);')
    if any(body.count(call) != 1 for call in calls) or body.count('target=0xd3750u;') != 1:
        raise ValueError('Authentic C9070 required/local call sites or pushed returns changed')
    if body.count('source_object_function_compiled_symbol_tu15(s.memory,') != 16:
        raise ValueError('C9070 lost an actual bank symbol operation')
    count = int(installed is not None)
    if (source.count(OWNER) != count * 2 or source.count(BASENAME) != count or
        source.count(INCLUDE) != count or DETAIL in source):
        raise ValueError('Foreign or duplicate native C9070 marker')
    if source.count(ANCHOR) != 1 or (count and source.count(ANCHOR + INCLUDE) != 1):
        raise ValueError('Native C9070 include moved across the exact D3950 anchor')
    return match, body, literal, installed


def replace_object_function_query(source):
    found = authenticate(source)
    if found is None or found[3] is not None:
        return source
    match, body, literal, _ = found
    result = source[:match.start()] + HEADER + prefix(literal) + body[len(HEADER):] + source[match.end():]
    result = result.replace(ANCHOR, ANCHOR + INCLUDE, 1)
    installed_entries(result)
    return result


def remove_object_function_query(source):
    found = authenticate(source)
    if found is None or found[3] is None:
        return source
    match, body, _, _ = found
    return (source[:match.start()] + body + source[match.end():]).replace(INCLUDE, '', 1)


def installed_entries(source):
    found = authenticate(source)
    if found is None or found[3] is None:
        raise ValueError('Exact Primary C9070 prefix is missing')
    return [ENTRY]


def retag_specialized(source, output):
    before = authenticate(source)
    if before is None or before[3] is None:
        return output
    match, body, literal, installed = authenticate(output, allow_old_tag=True)
    if not literal or installed != before[3]:
        raise ValueError('Specializer moved the exact C9070 callback format')
    result = output[:match.start()] + HEADER + prefix(True) + body[len(HEADER):] + output[match.end():]
    installed_entries(result)
    return result
