"""Exact CombatAction 165590 childless definition query."""
import hashlib
import importlib.util
from pathlib import Path
import re

_spec=importlib.util.spec_from_file_location('definition_mode_projectile_context',
    Path(__file__).with_name('native_projectile_function_query_replacements.py'))
projectile=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(projectile)
ENTRY='165590'
HEADER='static void run_165590(WorldState &s) {'
OWNER='native_object_definition_mode_query_owner_detail'
DETAIL='native_object_definition_mode_query_detail'
BASENAME='native_tu15_object_definition_mode_query_source_owner.hpp'
INCLUDE='#include "gameplay/objects/'+BASENAME+'"\n'
ANCHOR=projectile.INCLUDE
BODY=re.compile(r'^'+re.escape(HEADER)+r'.*?^\}',re.M|re.S)
CONTEXT=(
    r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;',
    r'^std::uint32_t selected[^\n]+',
    r'^static void dispatch[^\n]+',
    r'^void source_run_combat_action_tu15\(.*?^\}',
    r'^bool source_combat_action_tu15_owns_entry[^\n]+',
)
PINS=(
    (
        'b87005071aecb83264930dfe7eb08fc7d71b4b5e258ac1db6dea42dd657ba2c7',
        '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
        '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75',
        '2f716e064af3a72690bbe766be88580a1927daa39319590d18444e94e00114ef',
        '8babe9ef20df7e057fde057724996687cced5f124bc6fc80910b5a777ad39132',
        '5ffaf3d432c95512967e86067d95492571ed9f7989aa4b897ccc5457c5de9135',
    ),
    (
        '9ced5c763857feab404f5e915e8bfae8efecded6ff648cb40bd2f7a9f9e560ab',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75',
        'f434da6ab6b8f8239fe575d6e75efb8e00f4a67a5188ac30cfff70e1f8d3a0ef',
        '8babe9ef20df7e057fde057724996687cced5f124bc6fc80910b5a777ad39132',
        '5ffaf3d432c95512967e86067d95492571ed9f7989aa4b897ccc5457c5de9135',
    ),
    (
        'aab7acafc79a6ab947cbb3b9e2cb5c366307dd15db638102e966ef8a6d196ec4',
        '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
        '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75',
        'ba92d4566306f2d5010c8f6b8276d209a573f7dfa97835e511faf7bc77644f5d',
        '8babe9ef20df7e057fde057724996687cced5f124bc6fc80910b5a777ad39132',
        '5ffaf3d432c95512967e86067d95492571ed9f7989aa4b897ccc5457c5de9135',
    ),
    (
        'aab7acafc79a6ab947cbb3b9e2cb5c366307dd15db638102e966ef8a6d196ec4',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75',
        'f434da6ab6b8f8239fe575d6e75efb8e00f4a67a5188ac30cfff70e1f8d3a0ef',
        '8babe9ef20df7e057fde057724996687cced5f124bc6fc80910b5a777ad39132',
        '5ffaf3d432c95512967e86067d95492571ed9f7989aa4b897ccc5457c5de9135',
    ),
)

def prefix(literal):
    return '\n    if(halo2::'+OWNER+'::bridge<'+('true' if literal else 'false')+'>(s))return;'

def split(body):
    matches=[(literal,prefix(literal)) for literal in (False,True) if prefix(literal) in body]
    if not matches:
        if OWNER in body or DETAIL in body:raise ValueError('Unknown native 165590 marker')
        return body,None
    if len(matches)!=1:raise ValueError('Conflicting native 165590 tags')
    literal,marker=matches[0]
    if body.count(marker)!=1 or not body.startswith(HEADER+marker):raise ValueError('Native 165590 marker moved')
    return HEADER+body[len(HEADER)+len(marker):],literal

def canonical_body(body):
    return split(body)[0]

def authenticate(source,allow_old_tag=False):
    if 'void source_run_combat_action_tu15('not in source:
        if any(v in source for v in (OWNER,DETAIL,BASENAME)):raise ValueError('Native 165590 outside CombatAction')
        return None
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Missing or ambiguous whole 165590 body')
    match=matches[0];body,installed=split(match[0])
    raw=re.findall(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\}',body)
    prepared=re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);',body)
    if (len(raw),len(prepared))not in ((23,0),(0,23)):raise ValueError('165590 needs 23 uniform source sites')
    literal=bool(prepared);values=[body]
    if installed is not None and literal!=installed and not allow_old_tag:raise ValueError('165590 callback tag drift')
    for pattern in CONTEXT:
        found=re.findall(pattern,source,re.M|re.S)
        if len(found)!=1:raise ValueError('Complete 165590 context missing or ambiguous')
        values.append(found[0])
    actual=tuple(hashlib.sha256(value.encode()).hexdigest() for value in values)
    if actual not in (PINS[2:] if literal else PINS[:2]):raise ValueError('Complete correlated 165590 context changed')
    if re.findall(r'^[ \t]*(?:inline[ \t]+)?namespace[^\n]*',source,re.M)!=['namespace halo2 {','namespace {']:
        raise ValueError('CombatAction 165590 namespace drift')
    count=int(installed is not None)
    if source.count(OWNER)!=count or source.count(BASENAME)!=count or source.count(INCLUDE)!=count or DETAIL in source:
        raise ValueError('Foreign or duplicate 165590 native marker')
    projectile.installed_entries(source)
    if source.count(ANCHOR)!=1 or(count and source.count(ANCHOR+INCLUDE)!=1):
        raise ValueError('165590 include moved across the exact Projectile owner')
    return match,body,literal,installed

def replace_object_definition_mode_query(source):
    found=authenticate(source)
    if found is None or found[3]is not None:return source
    match,body,literal,_=found
    result=source[:match.start()]+HEADER+prefix(literal)+body[len(HEADER):]+source[match.end():]
    result=result.replace(ANCHOR,ANCHOR+INCLUDE,1);installed_entries(result);return result

def remove_object_definition_mode_query(source):
    found=authenticate(source)
    if found is None or found[3]is None:return source
    match,body,_,_=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)

def installed_entries(source):
    found=authenticate(source)
    if found is None or found[3]is None:raise ValueError('Complete 165590 default missing')
    return [ENTRY]

def retag_specialized(source,output):
    before=authenticate(source)
    if before is None or before[3]is None:return output
    match,body,literal,installed=authenticate(output,allow_old_tag=True)
    if not literal or installed!=before[3]:raise ValueError('Specializer moved 165590 native marker')
    result=output[:match.start()]+HEADER+prefix(True)+body[len(HEADER):]+output[match.end():]
    installed_entries(result);return result
