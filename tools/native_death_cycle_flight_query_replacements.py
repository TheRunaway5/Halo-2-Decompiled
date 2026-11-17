"""Exact E0900 overlay for the distinct TU1.5 DeathCycle source owner."""
import importlib.util
from pathlib import Path
import re

_shape_spec=importlib.util.spec_from_file_location('native_death_flight_shape',
    Path(__file__).with_name('native_flight_property_query_replacements.py'))
shape=importlib.util.module_from_spec(_shape_spec)
_shape_spec.loader.exec_module(shape)

OWNER='native_death_cycle_flight_query_owner_detail'
BASENAME='native_tu15_death_cycle_flight_query_source_owner.hpp'
INCLUDE='#include "gameplay/objects/'+BASENAME+'"\n'
ANCHOR='#include "halo2/death_cycle_runtime_tu15.hpp"\n'
HEADER=shape.HEADER
BODY=shape.BODY
ENTRY='void source_run_death_cycle_runtime_tu15('
BODY_PINS={
    (False,False):'7f6f154bf4e8adb4a0e08b489b0e92e12f0b6c102b1c97eb631cd6e0942d6e6e',
    (True,False):'e0b5ea5e4dfffe5558fef8823e3749fc8aa793f976e7a5f223bd62d74dc68dca',
    (False,True):shape.PROFILE_BODIES[False,True],
    (True,True):shape.PROFILE_BODIES[True,True],
}
DISPATCH_PINS={
    (False,False):'2ddcb4ca932da7abcdbbbf8050bead73a3a18e4a4153021913f0555281b8aa20',
    (True,False):'03799301083f53afa5eb43c5043509a12d3518626043f8cca11c6ce5e0e68ccc',
    (False,True):'1940da2e3890172390375a282ec9b1827d5a001fc02f5b85d0367efb9666ac45',
    (True,True):'1940da2e3890172390375a282ec9b1827d5a001fc02f5b85d0367efb9666ac45',
}
# Correlated complete resolver, live state, public entry and ownership.
CONTEXTS=(
    ('cc3118fc8d131fc3d8beb8e314827a9412e84288f99b72509be63ff966688b61',
     '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
     '2a991a9c2fe08cfe7b819dca11f49e352be74fd0afe781f4cfed9eadbea34292',
     '006c8d4b1ef535a74a05825002e6ceeded8498269d7140d5ab56b87b661530db'),
    ('1e70ace48573a64c67d51a9c7cde9f01c560e73795f8b089a918001d6bb1fbd6',
     '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
     '756f21822724abbf43554d0db4ee99793f788ed0ec5d74fb2424d15388f273ba',
     '269ea56cac029b21798798ae989aa9a99b4e055fdc86c959d545e6cb3b8d3ce9'),
)
PATTERNS=(r'^std::uint32_t selected\([^\n]+',
          r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;',
          r'^void source_run_death_cycle_runtime_tu15\(.*?^\}',
          r'^bool source_death_cycle_runtime_tu15_owns_entry\([^\n]+')


def prefix(literal,fresh):
    tag=lambda v:'true'if v else'false'
    return '\n    if(halo2::'+OWNER+'::bridge<'+tag(literal)+','+tag(fresh)+'>(s,native_single_spill))return;'


PREFIXES={key:prefix(*key)for key in BODY_PINS}


def one(source,pattern):
    rows=re.findall(pattern,source,re.M|re.S)
    if len(rows)!=1:raise ValueError('Missing or ambiguous complete DeathCycle E0900 context')
    return rows[0]


def canonical_body(body):
    found=[marker for marker in PREFIXES.values()if marker in body]
    if not found:
        if OWNER in body:raise ValueError('Unknown DeathCycle E0900 prefix')
        return body
    if len(found)!=1 or body.count(found[0])!=1 or not body.startswith(HEADER+found[0]):
        raise ValueError('DeathCycle E0900 bridge moved from its exact entry')
    return HEADER+body[len(HEADER)+len(found[0]):]


def authenticate(source,allow_old_tag=False):
    if ENTRY not in source:
        if OWNER in source or BASENAME in source:raise ValueError('DeathCycle E0900 owner missing')
        return None
    match=shape.match_body(source)
    body=match[0]
    prefixes=[key for key,marker in PREFIXES.items()if marker in body]
    if len(prefixes)>1:raise ValueError('Ambiguous DeathCycle E0900 entry')
    installed=prefixes[0] if prefixes else None
    body=canonical_body(body)
    literal=shape.callback_format(body)
    profiles=[key for key,pin in BODY_PINS.items()if key[0]==literal and pin==shape.digest(body)]
    if len(profiles)!=1:raise ValueError('Complete DeathCycle E0900 body/profile changed')
    profile=profiles[0];fresh=profile[1]
    if installed is not None and (installed[1]!=fresh or (installed[0]!=literal and not allow_old_tag)):
        raise ValueError('Wrong DeathCycle E0900 tag or spill policy')
    dispatch=one(source,r'^static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]+')
    if shape.digest(dispatch)!=DISPATCH_PINS[profile] or tuple(shape.digest(one(source,pattern))for pattern in PATTERNS)!=CONTEXTS[int(fresh)]:
        raise ValueError('Complete DeathCycle E0900 owner context changed')
    helper=shape.HELPER.findall(source)
    if len(helper)!=1 or shape.digest(helper[0])!=shape.HELPER_PIN:
        raise ValueError('Complete DeathCycle E0900 spill helper changed')
    namespaces=re.findall(r'^[ \t]*(?:inline[ \t]+)?namespace[^\n]*',source,re.M)
    owned=int(installed is not None)
    if namespaces!=['namespace halo2 {','namespace {'] or source.count(OWNER)!=owned or source.count(BASENAME)!=owned or source.count(INCLUDE)!=owned or shape.CORE in source or shape.OWNER in source:
        raise ValueError('Foreign or duplicate DeathCycle E0900 owner marker')
    if source.count(ANCHOR)!=1 or (owned and source.count(ANCHOR+INCLUDE)!=1):
        raise ValueError('DeathCycle E0900 include moved from its exact anchor')
    return match,body,profile,installed


def replace_death_cycle_flight_query(source):
    found=authenticate(source)
    if found is None or found[3]is not None:return source
    match,body,profile,_=found
    proposed=HEADER+PREFIXES[profile]+body[len(HEADER):]
    changed=(source[:match.start()]+proposed+source[match.end():]).replace(ANCHOR,ANCHOR+INCLUDE,1)
    authenticate(changed)
    return changed


def remove_death_cycle_flight_query(source):
    found=authenticate(source)
    if found is None or found[3]is None:return source
    match,body,_,_=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)


def installed_entries(source):
    found=authenticate(source)
    if found is None or found[3]is None:raise ValueError('Complete DeathCycle E0900 default missing')
    return ['e0900']


def retag_specialized(source,output):
    old=authenticate(source)
    if old is None or old[3]is None:return output
    match,body,profile,installed=authenticate(output,allow_old_tag=True)
    if installed!=old[3] or profile!=(True,old[2][1]):
        raise ValueError('Specializer moved or changed the DeathCycle E0900 entry')
    changed=output[:match.start()]+HEADER+PREFIXES[profile]+body[len(HEADER):]+output[match.end():]
    installed_entries(changed)
    return changed
