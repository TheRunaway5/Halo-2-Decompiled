"""Exact Primary E0900 native entry with distinct current/fresh spill policies."""
import hashlib
import importlib.util
import re
from pathlib import Path

_parent_spec=importlib.util.spec_from_file_location('native_flight_parent_context',
    Path(__file__).with_name('native_object_type_query_replacements.py'))
parent=importlib.util.module_from_spec(_parent_spec)
_parent_spec.loader.exec_module(parent)

HEADER='static void run_e0900(WorldState &s) {'
BODY=re.compile(r'^'+re.escape(HEADER)+r'.*?^\}',re.M|re.S)
HELPER=re.compile(r'^std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?^\}',re.M|re.S)
BASENAME='native_tu15_flight_property_query_source_owner.hpp'
INCLUDE='#include "gameplay/objects/'+BASENAME+'"\n'
ANCHOR='#include "halo2/source_effect_compiled_symbols_tu15.hpp"\n'
OWNER='native_flight_property_query_owner_detail'
CORE='native_flight_property_query_detail'
BRIDGE='halo2::'+OWNER+'::bridge'
PROFILE_BODIES={
    (False,False):'8439613527b310d55ffdb2a7ce3e6f7abc0379f0ff8a0ed945343a23662f44c7',
    (False,True):'63a4d0a9996e2c8671d16f9bbbe24d2527b1a1c06d52b91e4231ba17f14a790f',
    (True,False):'afec8f038faa58df44caf71ad42dbf0a656ba01e63eba8cd8f2b10f784171f7d',
    (True,True):'79c00ea11e6d50eaf04a8f858cac23de28ac41bd48b26e37803b21857f328d91',
}
PROFILE_DISPATCHES={
    (False,False):'5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
    (False,True):'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849',
    (True,False):'37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052',
    (True,True):'8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851',
}
HELPER_PIN='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1'


def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()


def prefix(literal,fresh):
    tag=lambda v:'true' if v else 'false'
    spill='native_single_spill' if fresh else 'nullptr'
    return '\n    if('+BRIDGE+'<'+tag(literal)+','+tag(fresh)+'>(s,'+spill+'))return;'


PREFIXES={key:prefix(*key)for key in PROFILE_BODIES}


def match_body(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Missing or ambiguous complete Primary E0900 body')
    return matches[0]


def split_body(body):
    owned=[(key,value)for key,value in PREFIXES.items()if value in body]
    if not owned:
        if OWNER in body or CORE in body:raise ValueError('Unknown E0900 native body')
        return body,None
    if len(owned)!=1 or body.count(owned[0][1])!=1 or not body.startswith(HEADER+owned[0][1]):
        raise ValueError('E0900 native bridge moved from its exact entry')
    key,value=owned[0]
    return HEADER+body[len(HEADER)+len(value):],key


def canonical_body(body):
    return split_body(body)[0]


def callback_format(body):
    raw=re.findall(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\}',body)
    literal=re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);',body)
    if (len(raw),len(literal))==(76,0):return False
    if (len(raw),len(literal))==(0,76):return True
    raise ValueError('E0900 needs exactly 76 uniform callback sites')


def authenticate(source,allow_old_tag=False):
    if 'void source_run_physics_live_support_tu15('not in source:
        if any(marker in source for marker in (OWNER,CORE,BASENAME)):raise ValueError('E0900 native bridge lacks its Primary owner')
        return None
    match=match_body(source)
    base,installed=split_body(match[0])
    literal=callback_format(base)
    profiles=[key for key,pin in PROFILE_BODIES.items()if key[0]==literal and pin==digest(base)]
    if len(profiles)!=1:raise ValueError('Complete E0900 retained source profile changed')
    profile=profiles[0]
    if installed is not None and (installed[1]!=profile[1] or (installed[0]!=literal and not allow_old_tag)):
        raise ValueError('Wrong E0900 callback format or spill policy')
    context,_=parent.authenticate(source)
    if digest(context['dispatcher'])!=PROFILE_DISPATCHES[profile]:
        raise ValueError('Complete E0900 dispatcher does not match its spill profile')
    helpers=HELPER.findall(source)
    if profile[1] and (len(helpers)!=1 or digest(helpers[0])!=HELPER_PIN):
        raise ValueError('Complete E0900 fresh spill helper changed')
    if not profile[1] and helpers:raise ValueError('Unexpected E0900 current spill helper')
    namespaces=re.findall(r'^[ \t]*(?:inline[ \t]+)?namespace[^\n]*',source,re.M)
    if namespaces!=['namespace halo2 {','namespace {']:
        raise ValueError('E0900 namespace ownership changed')
    owned=int(installed is not None)
    if source.count(INCLUDE)!=owned or source.count(BASENAME)!=owned or source.count(OWNER)!=owned or CORE in source:
        raise ValueError('Foreign, partial or duplicate E0900 binding')
    if source.count(ANCHOR)!=1 or (owned and source.count(ANCHOR+INCLUDE)!=1):
        raise ValueError('E0900 include moved from its exact anchor')
    return match,base,profile,installed


def replace_flight_property_query(source):
    found=authenticate(source)
    if found is None:return source
    match,base,profile,installed=found
    if installed is not None:return source
    proposed=HEADER+PREFIXES[profile]+base[len(HEADER):]
    changed=(source[:match.start()]+proposed+source[match.end():]).replace(ANCHOR,ANCHOR+INCLUDE,1)
    authenticate(changed)
    return changed


def remove_flight_property_query(source):
    found=authenticate(source)
    if found is None:return source
    match,base,_,installed=found
    if installed is None:return source
    return (source[:match.start()]+base+source[match.end():]).replace(INCLUDE,'',1)


def installed_entries(source):
    found=authenticate(source)
    if found is None or found[3] is None:raise ValueError('Complete Primary E0900 native default missing')
    return ['e0900']


def retag_specialized(source,output):
    before=authenticate(source)
    if before is None or before[3] is None:return output
    match,base,profile,installed=authenticate(output,allow_old_tag=True)
    if installed!=before[3] or profile!=(True,before[2][1]):
        raise ValueError('Specializer moved or changed the exact E0900 binding')
    proposed=HEADER+PREFIXES[profile]+base[len(HEADER):]
    changed=output[:match.start()]+proposed+output[match.end():]
    installed_entries(changed)
    return changed
