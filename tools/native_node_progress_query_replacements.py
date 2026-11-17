"""Exact AIFrame 1CF0E0 owner; preserve its genuine local descriptor edge."""
import hashlib
import re

ENTRY='1cf0e0'
HEADER='static void run_1cf0e0(WorldState &s) {'
OWNER='native_node_progress_query_owner_detail'
DETAIL='native_node_progress_query_detail'
BASENAME='native_tu15_node_progress_query_source_owner.hpp'
INCLUDE='#include "gameplay/animation/'+BASENAME+'"\n'
ANCHOR='#include "physics/runtime/native_tu15_node_descriptor_lookup.hpp"\n'
BODY=re.compile(r'^'+re.escape(HEADER)+r'.*?^\}',re.M|re.S)
CONTEXT=(
    r'^static void run_1e3150\(WorldState &s\) \{.*?^\}',
    r'^std::uint32_t selected[^\n]+',
    r'^static void dispatch[^\n]+',
    r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;',
    r'^std::uint32_t sign_extend[^\n]+\{.*?^\}',
    r'^static void run_1d63c0\(WorldState &s\) \{.*?^\}',
    r'^void source_run_ai_frame_tu15\(.*?^\}',
    r'^bool source_ai_frame_tu15_owns_entry[^\n]+',
)
PINS={
    'current-prepared': (
        'c8769a3f67949317b85ce8c46b9968e2ffdec524ca22e0b6cecc40f037d66995',
        '29a0a62adb04e575d546f4d41f2f1e3a35d724f7c045aac3b07f4a4738169630',
        '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
        '06f9ce13109229ec52f698071f0abb7f403a23f29cd2efaf9e27828d4057423e',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '700e75815abe137ec536b349ee217e0bfe34b7443a058fbacaf4ccccf9dd8347',
        'b68a8359dfc33546046f9c0681947872dd65da4edabe2e91d03338f223d1c789',
        '796da27b6844970e694891694e65fd57c777fab941c9a599312de009570be603',
    ),
    'current-raw': (
        '71035e9cf81f9d7a7d92906845f1937656c3c9f8c71072df8e64988d52f1441d',
        '38a02e3e2c9b0065ff47669090830cb94c451f7bd5f7865017b61d4a6bb85e2a',
        '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
        '1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '72ce24d305d7c388340b6970ce2c18e74c437b6aa480c6bd235095df4291c852',
        'b68a8359dfc33546046f9c0681947872dd65da4edabe2e91d03338f223d1c789',
        '796da27b6844970e694891694e65fd57c777fab941c9a599312de009570be603',
    ),
    'fresh-prepared': (
        'c8769a3f67949317b85ce8c46b9968e2ffdec524ca22e0b6cecc40f037d66995',
        '29a0a62adb04e575d546f4d41f2f1e3a35d724f7c045aac3b07f4a4738169630',
        'b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8',
        '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '700e75815abe137ec536b349ee217e0bfe34b7443a058fbacaf4ccccf9dd8347',
        '379089fe73835acd3b88fe2d6e9c3f77b961546e49c4fc823d783a12956968c5',
        '796da27b6844970e694891694e65fd57c777fab941c9a599312de009570be603',
    ),
    'fresh-raw': (
        '22ef454d55c794b944ac27fcd980752b70f6941bd7732f771f1f580041939648',
        'aea5f65daba927cf4872e361b4e6f332101885bf4ee440f80f2abe9437e611a5',
        'b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8',
        '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
        '8a98b69161c9ac5ff65e7a9dcd5c0a207ce1033ccbe93677bcaf5f11abce9732',
        '379089fe73835acd3b88fe2d6e9c3f77b961546e49c4fc823d783a12956968c5',
        '796da27b6844970e694891694e65fd57c777fab941c9a599312de009570be603',
    ),
}

def prefix(literal):
    tag='true' if literal else 'false'
    return '\n    if(halo2::'+OWNER+'::bridge<'+tag+'>(s,[](auto&state){run_1e3150(state);}))return;'

PREFIXES={key:prefix(key) for key in (False,True)}

def split(body):
    present=[(key,value) for key,value in PREFIXES.items() if value in body]
    if not present:
        if OWNER in body or DETAIL in body:raise ValueError('Unknown native 1CF0E0 marker')
        return body,None
    if len(present)!=1:raise ValueError('Conflicting native 1CF0E0 formats')
    key,value=present[0]
    if body.count(value)!=1 or not body.startswith(HEADER+value):raise ValueError('Native 1CF0E0 marker moved')
    return HEADER+body[len(HEADER)+len(value):],key

def canonical_body(body):
    return split(body)[0]

def authenticate(source,allow_old_tag=False):
    if 'void source_run_ai_frame_tu15(' not in source:
        if any(marker in source for marker in (OWNER,DETAIL,BASENAME)):
            raise ValueError('Native 1CF0E0 marker outside AIFrame')
        return None
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Missing or ambiguous AIFrame 1CF0E0 body')
    match=matches[0];body,installed=split(match[0])
    raw=re.findall(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\}',body)
    literal=re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);',body)
    if (len(raw),len(literal)) not in ((34,0),(0,34)):raise ValueError('1CF0E0 needs 34 uniform callback sites')
    prepared=bool(literal)
    pieces=[body]
    for pattern in CONTEXT:
        found=re.findall(pattern,source,re.M|re.S)
        if len(found)!=1:raise ValueError('Complete AIFrame 1CF0E0 context is missing or ambiguous')
        pieces.append(found[0])
    digests=tuple(hashlib.sha256(piece.encode()).hexdigest() for piece in pieces)
    matched=[key for key,pin in PINS.items() if key.endswith('-prepared')==prepared and pin==digests]
    if len(matched)!=1:raise ValueError('Complete correlated AIFrame 1CF0E0 context changed')
    if installed is not None and prepared!=installed and not allow_old_tag:
        raise ValueError('Native 1CF0E0 callback tag changed')
    if body.count('run_1e3150(s);')!=1 or pieces[6].count('run_1cf0e0(s);')!=1:
        raise ValueError('Genuine AIFrame 1CF0E0 edges changed')
    scope=re.findall(r'^[ \t]*(?:inline[ \t]+)?namespace[^\n]*',source,re.M)
    if scope!=['namespace halo2 {','namespace {']:raise ValueError('AIFrame 1CF0E0 namespace moved')
    count=int(installed is not None)
    if source.count(OWNER)!=count or source.count(INCLUDE)!=count or source.count(BASENAME)!=count or DETAIL in source:
        raise ValueError('Foreign or duplicate AIFrame 1CF0E0 marker')
    if source.count(ANCHOR)!=1 or (count and source.count(ANCHOR+INCLUDE)!=1):
        raise ValueError('AIFrame 1CF0E0 include moved across the descriptor owner')
    return match,body,prepared,installed

def replace_node_progress_query(source):
    found=authenticate(source)
    if found is None or found[3] is not None:return source
    match,body,literal,_=found
    result=source[:match.start()]+HEADER+prefix(literal)+body[len(HEADER):]+source[match.end():]
    result=result.replace(ANCHOR,ANCHOR+INCLUDE,1)
    installed_entries(result)
    return result

def remove_node_progress_query(source):
    found=authenticate(source)
    if found is None or found[3] is None:return source
    match,body,_,_=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)

def installed_entries(source):
    found=authenticate(source)
    if found is None or found[3] is None:raise ValueError('Exact AIFrame 1CF0E0 default is missing')
    return [ENTRY]

def retag_specialized(source,output):
    before=authenticate(source)
    if before is None or before[3] is None:return output
    match,body,literal,installed=authenticate(output,allow_old_tag=True)
    if not literal or installed!=before[3]:raise ValueError('Specializer moved native AIFrame 1CF0E0')
    result=output[:match.start()]+HEADER+prefix(True)+body[len(HEADER):]+output[match.end():]
    installed_entries(result)
    return result
