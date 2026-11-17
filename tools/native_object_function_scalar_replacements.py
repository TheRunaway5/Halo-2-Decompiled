"""Private BCB70 parent overlay and four original TU1.5 shift spellings."""
import hashlib
import importlib.util
from pathlib import Path
import re

def load_sibling(name):
    spec = importlib.util.spec_from_file_location('object_scalar_'+name,Path(__file__).with_name(name+'.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


query = load_sibling('native_object_function_query_replacements')
type_query = load_sibling('native_object_type_query_replacements')

ENTRY = 'bcb70'
HEADER = 'static void run_bcb70(WorldState &s) {'
OWNER = 'native_object_function_scalar_owner_detail'
FRAME = 'native_object_function_scalar_detail'
ORDERED = 'native_object_function_scalar_ordered_detail'
BASENAME = 'native_tu15_object_function_scalar_source_owner.hpp'
INCLUDE = '#include "gameplay/objects/' + BASENAME + '"\n'
ANCHOR = query.INCLUDE
BODY = re.compile(r'^' + re.escape(HEADER) + r'.*?^\}', re.M | re.S)
SHIFTS = ((0xbcb96,6,2,True),(0xbcbb0,0,9,False),(0xbcc35,2,4,True),(0xbcc88,2,26,False))
DIRECT = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')
LITERAL = re.compile(r's\.instruction_literal\((0x[0-9a-f]+u)\);')
PINS = {
    'current-prepared': (
        '2cab22addbfec455dbbe19a65d9fab2b39aeee9f40faf04fd0e03ea9810e8024',
        '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
        '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052',
        '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '912e109873b4775f13979a32e325106d74712a3e4a0bf43d5485d911cd01f35a',
        'f53eb05fed15f9f03fb04925d9a466a46992c9c79539232e8c54f76a96b43fab',
        'f5c054225e32792e1bd4adac67cb060537c05aafd2892783e4b9ba416b11653c',
        '472c16d5a241ba2db3c8d0e7e85b014abc756b7f0539d2480b09f1617b3d33d7',
        '95f2a8fcb2f697c1fcfd4bb0cbfc6e55654d91452225db04e680c358eede4270',
        '8afbac9eee3a4d165e211d8711dabe43f03e811f55029b4e31f0549cba8f2463',
    ),
    'current-raw': (
        '97f18741ab0d02659a55f24833a51870f8e5d1898b75e3622842b5479e382e52',
        '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
        '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
        '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '912e109873b4775f13979a32e325106d74712a3e4a0bf43d5485d911cd01f35a',
        'f53eb05fed15f9f03fb04925d9a466a46992c9c79539232e8c54f76a96b43fab',
        'a81b06fe9b74d0ae5fb55b517e4ba8bede7a6290b36e489a11f13f73edd64b8f',
        '74c67910b5a70133e8231f707160f9b42b169d219b75af861d50088383305c0e',
        'a2f1ff4e780537a669cb84b4369ece02a6e36a8b57f581d82165b9581da30e20',
        '564384dc525a1475875aade4fab0723c5d8fa5b10c7222dc86759bf2bf241c3f',
    ),
    'fresh-prepared': (
        '2cab22addbfec455dbbe19a65d9fab2b39aeee9f40faf04fd0e03ea9810e8024',
        '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
        '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '7e227c0aa7f7b21a540fa749c52d95a75432e51a9513543be7bddc25fa58ca09',
        '5a8d30ef7f8b2b214d9e32e5e4cdb91d43c2556853b5ad03692086703cec016c',
        'f5c054225e32792e1bd4adac67cb060537c05aafd2892783e4b9ba416b11653c',
        '472c16d5a241ba2db3c8d0e7e85b014abc756b7f0539d2480b09f1617b3d33d7',
        '6e19a808ef9f301f469da11c6426b86ce51733e8ec76dfaad2b9ac36ec759fea',
        '8afbac9eee3a4d165e211d8711dabe43f03e811f55029b4e31f0549cba8f2463',
    ),
    'fresh-raw': (
        '97f18741ab0d02659a55f24833a51870f8e5d1898b75e3622842b5479e382e52',
        '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
        'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '7e227c0aa7f7b21a540fa749c52d95a75432e51a9513543be7bddc25fa58ca09',
        '5a8d30ef7f8b2b214d9e32e5e4cdb91d43c2556853b5ad03692086703cec016c',
        'a81b06fe9b74d0ae5fb55b517e4ba8bede7a6290b36e489a11f13f73edd64b8f',
        '74c67910b5a70133e8231f707160f9b42b169d219b75af861d50088383305c0e',
        '1e51bf36a3b0744ae6c6aeab2c38ec47bc1232c371831fc2b95956925cf1956d',
        '564384dc525a1475875aade4fab0723c5d8fa5b10c7222dc86759bf2bf241c3f',
    ),
}


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def prefix(literal):
    tag = 'true' if literal else 'false'
    return ('\n    if(halo2::'+OWNER+'::bridge<'+tag+'>(s,halo2::'+OWNER+'::Children{'
            '[](auto&state){run_10fb70(state);},[](auto&state){run_10de40(state);},'
            '[](auto&state){run_bc8f0(state);},[](auto&state){run_bcb70(state);}}))return;')


def format_of(body):
    counts = len(DIRECT.findall(body)),len(LITERAL.findall(body))
    if counts not in ((138,0),(0,138)):
        raise ValueError('BCB70 needs exactly 138 uniform callback sites')
    return counts[1] != 0


def ordered_shifts(body, literal, *, reverse=False):
    for pc, reg, count, left in SHIFTS:
        operand = f's.reg({reg},4,0)'
        old = (f's.flags.shift({operand},0x{count:x}u,4)' if left else
               f's.flags.right({operand},0x{count:x}u,4,false)')
        new = f'halo2::{ORDERED}::shift_for_image<{count}u,{str(left).lower()}>(s,{operand})'
        callback = (f's.instruction_literal(0x{pc:x}u);' if literal else
                    f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}')
        if body.count(callback) != 1:
            raise ValueError(f'BCB70 original shift site moved at {pc:x}')
        start = body.index(callback) + len(callback)
        stop = body.index('\n',start)
        line = body[start:stop]
        source,target = (new,old) if reverse else (old,new)
        if line.count(source)!=1 or target in line:
            raise ValueError(f'BCB70 shift expression changed at {pc:x}')
        body = body[:start]+line.replace(source,target,1)+body[stop:]
    return body


def exact_match(pattern, source):
    matches = list(pattern.finditer(source))
    if len(matches)!=1:
        raise ValueError('Missing or ambiguous complete BCB70 context: '+pattern.pattern)
    return matches[0]


def complete_body(source, entry):
    pattern = re.compile(rf'^static void run_{entry}\(WorldState &s\) \{{.*?^\}}',re.M|re.S)
    return exact_match(pattern,source)[0]


def split_body(body, *, transitional=False):
    tagged = [(value,prefix(value)) for value in (False,True) if prefix(value) in body]
    if len(tagged)>1:
        raise ValueError('Conflicting BCB70 source-format tags')
    installed = bool(tagged)
    if installed:
        value,marker = tagged[0]
        if body.count(marker)!=1 or not body.startswith(HEADER+marker):
            raise ValueError('BCB70 overlay moved from its function entry')
        body = HEADER+body[len(HEADER)+len(marker):]
    literal = format_of(body)
    if installed and value!=literal and not (transitional and literal and not value):
        raise ValueError('BCB70 callback format and tag differ')
    if installed:
        body = ordered_shifts(body,literal,reverse=True)
    return body,literal,installed


def canonical_body(body):
    return split_body(body)[0]


def authenticate(source, *, transitional=False):
    markers = (OWNER,FRAME,ORDERED,BASENAME)
    if 'void source_run_physics_live_support_tu15(' not in source:
        if any(marker in source for marker in markers):
            raise ValueError('BCB70 overlay outside its actual Primary owner')
        return None
    match = exact_match(BODY,source)
    body,literal,installed = split_body(match[0],transitional=transitional)
    query_context = query.authenticate(source)
    type_context = type_query.authenticate(source)
    if query_context is None or type_context is None:
        raise ValueError('Missing genuine inherited callback authority')
    query_bound = query_context[3] is not None
    type_bound = type_query.entry_prefix(type_context[0]['body']) is not None
    anchor = ANCHOR if query_bound else query.ANCHOR
    owned = int(installed)
    if (source.count(OWNER)!=2*owned or FRAME in source or source.count(ORDERED)!=4*owned or
        source.count(BASENAME)!=owned or source.count(INCLUDE)!=owned or source.count(anchor)!=1 or
        (installed and source.count(anchor+INCLUDE)!=1)):
        raise ValueError('Foreign, incomplete or misplaced BCB70 overlay')
    context = tuple(exact_match(re.compile(pattern,re.M|re.S),source)[0] for pattern in query.CONTEXT)
    children = tuple(complete_body(source,entry) for entry in ('10fb70','10de40','bc8f0','c9070'))
    pins = tuple(digest(value) for value in (body,*context,*children))
    identities = []
    for name, measured in PINS.items():
        if name.endswith('prepared') != literal:
            continue
        expected = list(measured)
        # The children independently remove their own overlays for whole-source
        # regeneration checks. Admit only those exact authenticated bodies.
        if not type_bound:
            expected[-3] = type_query.BODY_PINS[literal]
        if not query_bound:
            expected[-1] = query.PINS[name][0]
        if tuple(expected)==pins:
            identities.append(name)
    if len(identities)!=1:
        raise ValueError('Complete BCB70 body/resolver/dispatch/state/children context changed')
    return match,body,literal,installed,identities[0]


def replace_object_function_scalar(source):
    found = authenticate(source)
    if found is None or found[3]:
        return source
    match,body,literal,_,_ = found
    ordered = ordered_shifts(body,literal)
    result = source[:match.start()]+HEADER+prefix(literal)+ordered[len(HEADER):]+source[match.end():]
    anchor = ANCHOR if ANCHOR in source else query.ANCHOR
    result = result.replace(anchor,anchor+INCLUDE,1)
    installed_entries(result)
    return result


def remove_object_function_scalar(source):
    found = authenticate(source)
    if found is None or not found[3]:
        return source
    match,body,_,_,_ = found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)


def installed_entries(source):
    found = authenticate(source)
    if found is None or not found[3]:
        raise ValueError('Missing exact installed BCB70 overlay')
    return [ENTRY]


def retag_specialized(original, output):
    before = authenticate(original)
    after = authenticate(output,transitional=True)
    if (before is None or after is None or not before[3] or not after[3] or before[2] or not after[2] or
        not after[0][0].startswith(HEADER+prefix(False)) or
        before[4].replace('-raw','-prepared')!=after[4]):
        raise ValueError('BCB70 specialization lost exact tagged owner')
    match=after[0]
    fixed=output[:match.start()]+match[0].replace(prefix(False),prefix(True),1)+output[match.end():]
    installed_entries(fixed)
    return fixed
