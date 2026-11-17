"""Bind only the exact Primary D3950 parent and preserve its required dispatch."""
import hashlib
import importlib.util
from pathlib import Path
import re

ENTRY = 'd3950'
HEADER = 'static void run_d3950(WorldState &s) {'
OWNER = 'native_node_function_progress_owner_detail'
DETAIL = 'native_node_function_progress_detail'
BASENAME = 'native_tu15_node_function_progress_source_owner.hpp'
INCLUDE = '#include "gameplay/animation/' + BASENAME + '"\n'
ANCHOR = '#include "gameplay/objects/native_tu15_object_record_ratio_source_owner.hpp"\n'
# Exact known predecessor chain, nearest first. Proofs that strip the 107140
# and then the Flight overlay remove only their own include lines.
PREDECESSORS = (
    ANCHOR,
    '#include "gameplay/objects/native_tu15_flight_property_query_source_owner.hpp"\n',
    '#include "halo2/source_effect_compiled_symbols_tu15.hpp"\n',
)
BODY = re.compile(r'^' + re.escape(HEADER) + r'.*?^\}', re.M | re.S)
CONTEXT = (
    r'^static void run_c9070\(WorldState &s\) \{.*?^\}',
    r'^std::uint32_t selected[^\n]+',
    r'^static void dispatch[^\n]+',
    r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;',
    r'^std::uint32_t sign_extend[^\n]+\{.*?^\}',
    r'^void source_run_physics_live_support_tu15\(.*?^\}',
    r'^bool source_physics_live_support_tu15_owns_entry[^\n]+',
)
PINS = {
    'current-prepared': (
        '10c5174a33925cbd8d0944b349b129911dfba8c946199c0a38b17d385ac37842',
        'b3b50f1d4f9826f000934a090b693c0fc6c047b7b56a2abd15bafeded5343a81',
        '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
        '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052',
        '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '912e109873b4775f13979a32e325106d74712a3e4a0bf43d5485d911cd01f35a',
        'f53eb05fed15f9f03fb04925d9a466a46992c9c79539232e8c54f76a96b43fab',
    ),
    'current-raw': (
        '16032edfa64985723047f8ff6ecd0d297bd3e63115c6079161ee8128c27bb607',
        'e3c60cfb8d2afd18bb4f03f3557dc2d24561155260938d8fdc40fa9f0f157ef7',
        '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
        '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
        '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '912e109873b4775f13979a32e325106d74712a3e4a0bf43d5485d911cd01f35a',
        'f53eb05fed15f9f03fb04925d9a466a46992c9c79539232e8c54f76a96b43fab',
    ),
    'fresh-prepared': (
        '10c5174a33925cbd8d0944b349b129911dfba8c946199c0a38b17d385ac37842',
        'b3b50f1d4f9826f000934a090b693c0fc6c047b7b56a2abd15bafeded5343a81',
        '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
        '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '7e227c0aa7f7b21a540fa749c52d95a75432e51a9513543be7bddc25fa58ca09',
        '5a8d30ef7f8b2b214d9e32e5e4cdb91d43c2556853b5ad03692086703cec016c',
    ),
    'fresh-raw': (
        '16032edfa64985723047f8ff6ecd0d297bd3e63115c6079161ee8128c27bb607',
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
    return ('\n    if(halo2::' + OWNER + '::bridge<' + tag + '>(s,[](auto&state,auto target,auto site,bool tail)'
            '{dispatch(state,target,site,tail);}))return;')


PREFIXES = {literal: prefix(literal) for literal in (False, True)}


def split(body):
    markers = [(literal, marker) for literal, marker in PREFIXES.items() if marker in body]
    if not markers:
        if OWNER in body or DETAIL in body:
            raise ValueError('Unknown native D3950 marker')
        return body, None
    if len(markers) != 1:
        raise ValueError('Conflicting native D3950 tags')
    literal, marker = markers[0]
    if body.count(marker) != 1 or not body.startswith(HEADER + marker):
        raise ValueError('Native D3950 marker moved from the entry')
    return HEADER + body[len(HEADER) + len(marker):], literal


def canonical_body(body):
    return split(body)[0]


def anchor(source):
    for candidate in PREDECESSORS:
        if candidate in source:
            return candidate
        if candidate.split('"')[1].rsplit('/', 1)[1] in source:
            raise ValueError('Native D3950 known predecessor include moved')
    raise ValueError('Missing native D3950 Primary include anchor')


def authenticate(source, allow_old_tag=False):
    if 'void source_run_physics_live_support_tu15(' not in source:
        if any(marker in source for marker in (OWNER, DETAIL, BASENAME)):
            raise ValueError('Native D3950 marker outside Primary')
        return None
    bodies = list(BODY.finditer(source))
    if len(bodies) != 1:
        raise ValueError('Missing or ambiguous complete Primary D3950')
    match = bodies[0]
    body, installed = split(match[0])
    raw = re.findall(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\}', body)
    prepared = re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);', body)
    if (len(raw), len(prepared)) not in ((37, 0), (0, 37)):
        raise ValueError('D3950 requires exactly 37 uniformly formatted callback sites')
    literal = bool(prepared)
    if installed is not None and installed != literal and not allow_old_tag:
        raise ValueError('Native D3950 callback tag changed')
    context = [re.findall(pattern, source, re.M | re.S) for pattern in CONTEXT]
    if any(len(parts) != 1 for parts in context):
        raise ValueError('Missing or ambiguous exact Primary D3950 context')
    caller = context[0][0]
    if 'native_object_function_query_owner_detail' in caller or 'native_object_function_query_detail' in caller:
        query_spec = importlib.util.spec_from_file_location(
            'node_function_progress_c9070_context', Path(__file__).with_name('native_object_function_query_replacements.py'))
        query = importlib.util.module_from_spec(query_spec)
        query_spec.loader.exec_module(query)
        context[0][0] = query.canonical_body(caller)
    pins = tuple(digest(part) for part in (body, *(parts[0] for parts in context)))
    if sum(expected == pins for name, expected in PINS.items() if name.endswith('-prepared') == literal) != 1:
        raise ValueError('Complete correlated Primary D3950 source changed')
    dispatcher = context[2][0]
    if 's.required(site,target,tail);' not in dispatcher or 'case 0x1cf0e0u:' in dispatcher:
        raise ValueError('Genuine D3950 external dispatch changed')
    calls = re.findall(r'\{const auto target=0x1cf0e0u;s\.push\((0xd[0-9a-f]+u),(0xd[0-9a-f]+u)\);'
                       r'dispatch\(s,target,\1,false\);\}', body)
    if calls != [('0xd3998u', '0xd399du'), ('0xd39b3u', '0xd39b8u')]:
        raise ValueError('Authentic D3950 required callsites or pushed returns changed')
    count = int(installed is not None)
    if source.count(OWNER) != count or source.count(BASENAME) != count or source.count(INCLUDE) != count or DETAIL in source:
        raise ValueError('Foreign or duplicate native D3950 marker')
    at = anchor(source)
    if source.count(at) != 1 or (count and source.count(at + INCLUDE) != 1):
        raise ValueError('Native D3950 include moved across its owning Primary anchor')
    return match, body, literal, installed


def replace_node_function_progress(source):
    found = authenticate(source)
    if found is None or found[3] is not None:
        return source
    match, body, literal, _ = found
    result = source[:match.start()] + HEADER + prefix(literal) + body[len(HEADER):] + source[match.end():]
    at = anchor(result)
    result = result.replace(at, at + INCLUDE, 1)
    installed_entries(result)
    return result


def remove_node_function_progress(source):
    found = authenticate(source)
    if found is None or found[3] is None:
        return source
    match, body, _, _ = found
    return (source[:match.start()] + body + source[match.end():]).replace(INCLUDE, '', 1)


def installed_entries(source):
    found = authenticate(source)
    if found is None or found[3] is None:
        raise ValueError('Exact Primary D3950 native prefix is missing')
    return [ENTRY]


def retag_specialized(source, output):
    before = authenticate(source)
    if before is None or before[3] is None:
        return output
    match, body, literal, installed = authenticate(output, allow_old_tag=True)
    if not literal or installed != before[3]:
        raise ValueError('Specializer moved the exact D3950 callback format')
    result = output[:match.start()] + HEADER + prefix(True) + body[len(HEADER):] + output[match.end():]
    installed_entries(result)
    return result
