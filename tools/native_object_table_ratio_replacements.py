"""Exact CombatAction 1655F0/24B430 pair, keeping the real local child edge."""
import hashlib
import importlib.util
from pathlib import Path
import re

_spec=importlib.util.spec_from_file_location('object_table_ratio_definition_context',
    Path(__file__).with_name('native_object_definition_mode_query_replacements.py'))
definition=importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(definition)

PARENT,LEAF='1655f0','24b430'
ENTRIES=(PARENT,LEAF)
OWNER='native_object_table_ratio_owner_detail'
DETAIL='native_object_table_ratio_detail'
BASENAME='native_tu15_object_table_ratio_source_owner.hpp'
INCLUDE='#include "gameplay/objects/'+BASENAME+'"\n'
ANCHOR=definition.INCLUDE
HEADERS={entry:f'static void run_{entry}(WorldState &s) {{' for entry in ENTRIES}
BODIES={entry:re.compile(r'^'+re.escape(header)+r'.*?^\}',re.M|re.S) for entry,header in HEADERS.items()}
PINS={
    PARENT:('c0092c440050a6c0502995b959f5c613aa301b11aa8fe225fdfef51d732f6e12',
            '7d49a618cea30869c73bbd885267e1a911e07a4f1bd4636af6a7699b78acc02a',
            '077f433ce4ccb15a0f360262cf59c8690467666ea8077cfaecc00d69814764f7'),
    LEAF:('0ca760e8a63172e2eb45d2979e7ea8b2c4e637c2d4f3ba8c5a7b255449eb5d54',
          '211beb1b6884e3a1c9227215667e7cdc9e7d5047f4f3dbe7d0ad9486d19bc5f6',
          '94914e737d8098008cfe3243a501b7678609d949998d5c32973c2e8422d01563'),
}


def prefix(entry,literal):
    tag='true' if literal else 'false'
    child=', [&](auto& child){run_24b430(child);}' if entry==PARENT else ''
    bridge='bridge_parent' if entry==PARENT else 'bridge_leaf'
    return f'\n    if(halo2::{OWNER}::{bridge}<{tag}>(s{child}))return;'


def split(entry,body):
    choices=[(tag,prefix(entry,tag)) for tag in (False,True) if prefix(entry,tag) in body]
    if not choices:
        if OWNER in body or DETAIL in body:
            raise ValueError('Unknown native table-ratio owner marker')
        return body,None
    if len(choices)!=1:
        raise ValueError('Conflicting native table-ratio callback tags')
    tag,marker=choices[0];header=HEADERS[entry]
    if body.count(marker)!=1 or not body.startswith(header+marker):
        raise ValueError('Native table-ratio marker moved')
    return header+body[len(header)+len(marker):],tag


def canonical_body(body,entry):
    return split(entry,body)[0]


def authenticate(source,allow_old_tag=False):
    if 'void source_run_combat_action_tu15(' not in source:
        if any(marker in source for marker in (OWNER,DETAIL,BASENAME)):
            raise ValueError('Native table-ratio pair outside CombatAction')
        return None
    neighbor=definition.authenticate(source)
    if neighbor is None or neighbor[3] is None:
        raise ValueError('Exact prior CombatAction 165590 binding missing')
    literal=neighbor[2]
    owner_profile=2
    if not literal:
        neighbor_pin=hashlib.sha256(neighbor[1].encode()).hexdigest()
        owner_profile=next((index for index,row in enumerate(definition.PINS[:2]) if row[0]==neighbor_pin),-1)
        if owner_profile<0:
            raise ValueError('Unknown raw CombatAction table-ratio owner')
    contexts=[];tags=set()
    for entry in ENTRIES:
        matches=list(BODIES[entry].finditer(source))
        if len(matches)!=1:
            raise ValueError('Missing or ambiguous complete CombatAction '+entry)
        match=matches[0];body,tag=split(entry,match[0]);tags.add(tag)
        raw=re.findall(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\}',body)
        prepared=re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);',body)
        count=8 if entry==PARENT else 34
        if (len(raw),len(prepared))!=((0,count) if literal else (count,0)):
            raise ValueError('Unexpected table-ratio callback format: '+entry)
        digest=hashlib.sha256(body.encode()).hexdigest()
        if digest!=PINS[entry][owner_profile]:
            raise ValueError('Complete table-ratio owner body drift: '+entry)
        contexts.append((entry,match,body))
    if len(tags)!=1:
        raise ValueError('Partial or mixed table-ratio binding')
    installed=tags.pop()
    if installed is not None and installed!=literal and not allow_old_tag:
        raise ValueError('Table-ratio callback tag drift')
    count=int(installed is not None)
    if (source.count(INCLUDE)!=count or source.count(BASENAME)!=count or source.count(OWNER)!=2*count or DETAIL in source or
        source.count(ANCHOR)!=1 or (count and source.count(ANCHOR+INCLUDE)!=1)):
        raise ValueError('Partial, foreign or misplaced table-ratio binding')
    return contexts,literal,installed


def rebind(source,contexts,literal):
    for entry,match,body in sorted(contexts,key=lambda row:row[1].start(),reverse=True):
        header=HEADERS[entry]
        target=body if literal is None else header+prefix(entry,literal)+body[len(header):]
        source=source[:match.start()]+target+source[match.end():]
    return source


def replace_object_table_ratio(source):
    found=authenticate(source)
    if found is None or found[2] is not None:
        return source
    contexts,literal,_=found
    result=rebind(source,contexts,literal).replace(ANCHOR,ANCHOR+INCLUDE,1)
    installed_entries(result)
    return result


def remove_object_table_ratio(source):
    found=authenticate(source)
    if found is None or found[2] is None:
        return source
    return rebind(source,found[0],None).replace(INCLUDE,'',1)


def installed_entries(source):
    found=authenticate(source)
    if found is None or found[2] is None:
        raise ValueError('Complete CombatAction table-ratio defaults missing')
    return list(ENTRIES)


def retag_specialized(source,output):
    before=authenticate(source)
    if before is None or before[2] is None:
        return output
    contexts,literal,installed=authenticate(output,allow_old_tag=True)
    if not literal or installed!=before[2]:
        raise ValueError('Specializer moved the table-ratio binding')
    result=rebind(output,contexts,True)
    installed_entries(result)
    return result
