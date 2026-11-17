"""Exact private Primary 107140 entry overlay; its 1046C0 caller stays intact."""
import importlib.util
from pathlib import Path
import re

_spec = importlib.util.spec_from_file_location('record_ratio_flight_context',
    Path(__file__).with_name('native_flight_property_query_replacements.py'))
flight = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(flight)

ENTRY = '107140'
HEADER = 'static void run_107140(WorldState &s) {'
OWNER = 'native_object_record_ratio_owner_detail'
DETAIL = 'native_object_record_ratio_detail'
BASENAME = 'native_tu15_object_record_ratio_source_owner.hpp'
INCLUDE = '#include "gameplay/objects/'+BASENAME+'"\n'
ANCHOR = flight.INCLUDE
BODY = re.compile(r'^'+re.escape(HEADER)+r'.*?^\}',re.M|re.S)
PARENT = re.compile(r'^static void run_1046c0\(WorldState &s\) \{.*?^\}',re.M|re.S)
PINS = {
    False: ('1e4f91fcb47980f7d5fa69c31a93469f0d76bd7f21548d84d18ec7fa921c5659',
            '84a26140524e26d4dddc5eea8b3bf3e7205f8fcd161c034d49e55ef5f6fcfe9c'),
    True: ('fc583b34bd0249e4bba259cbf71eeda58493b95472e9e4514316f02b79e4ab47',
           '32e7c8d5885c51767fdc5bba6dd66cf4ed67516d133fb25ab16e0c77cad36a42'),
}
ORDERED_SHIFTS = tuple((f's.flags.shift(s.reg(0,4,0),0x{count:x}u,4)',
                        f'halo2::{DETAIL}::shift_eax_for_image<{count}u>(s,s.reg(0,4,0))')
                       for count in (4,6))


def apply_ordered_shifts(body):
    for old,new in ORDERED_SHIFTS:
        if body.count(old)!=1 or new in body:
            raise ValueError('Missing or duplicate original 107140 shift')
        body=body.replace(old,new,1)
    return body


def remove_ordered_shifts(body):
    for old,new in ORDERED_SHIFTS:
        if body.count(new)!=1 or old in body:
            raise ValueError('Missing or duplicate ordered 107140 shift')
        body=body.replace(new,old,1)
    return body


def prefix(literal):
    return '\n    if(halo2::'+OWNER+'::bridge<'+('true' if literal else 'false')+'>(s))return;'


def split(body):
    matches = [(literal,prefix(literal)) for literal in (False,True) if prefix(literal) in body]
    if not matches:
        if OWNER in body or DETAIL in body:
            raise ValueError('Unknown native 107140 marker')
        return body,None
    if len(matches)!=1:
        raise ValueError('Conflicting native 107140 tags')
    literal,marker = matches[0]
    if body.count(marker)!=1 or not body.startswith(HEADER+marker):
        raise ValueError('Native 107140 marker moved')
    return remove_ordered_shifts(HEADER+body[len(HEADER)+len(marker):]),literal


def canonical_body(body):
    return split(body)[0]


def authenticate(source,allow_old_tag=False):
    if 'void source_run_physics_live_support_tu15(' not in source:
        if any(marker in source for marker in (OWNER,DETAIL,BASENAME)):
            raise ValueError('Native 107140 outside Primary')
        return None
    matches,parents = list(BODY.finditer(source)),PARENT.findall(source)
    if len(matches)!=1 or len(parents)!=1:
        raise ValueError('Missing or ambiguous complete 107140 or 1046C0 body')
    match=matches[0];body,installed=split(match[0])
    raw = re.findall(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\}',body)
    prepared = re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);',body)
    if (len(raw),len(prepared)) not in ((36,0),(0,36)):
        raise ValueError('107140 needs exactly 36 uniform callbacks')
    literal=bool(prepared)
    if installed is not None and literal!=installed and not allow_old_tag:
        raise ValueError('107140 callback tag drift')
    parent=parents[0]
    if 'native_object_ratio_caller_owner_detail' in parent:
        caller_spec=importlib.util.spec_from_file_location('record_ratio_caller_context',
            Path(__file__).with_name('native_object_ratio_caller_replacements.py'))
        caller=importlib.util.module_from_spec(caller_spec);caller_spec.loader.exec_module(caller)
        parent=caller.canonical_parent(parent)
    if tuple(flight.digest(value) for value in (body,parent))!=PINS[literal]:
        raise ValueError('Complete 107140 body or genuine 1046C0 caller changed')
    flight.installed_entries(source)
    count=int(installed is not None)
    if (source.count(OWNER)!=count or source.count(BASENAME)!=count or source.count(INCLUDE)!=count or
        source.count(DETAIL)!=2*count):
        raise ValueError('Foreign or duplicate 107140 native marker')
    if source.count(ANCHOR)!=1 or (count and source.count(ANCHOR+INCLUDE)!=1):
        raise ValueError('107140 include moved across the exact Primary Flight owner')
    return match,body,literal,installed


def replace_object_record_ratio(source):
    found=authenticate(source)
    if found is None or found[3] is not None:
        return source
    match,body,literal,_=found
    result=(source[:match.start()]+HEADER+prefix(literal)+apply_ordered_shifts(body)[len(HEADER):]+source[match.end():])
    result=result.replace(ANCHOR,ANCHOR+INCLUDE,1)
    installed_entries(result)
    return result


def remove_object_record_ratio(source):
    found=authenticate(source)
    if found is None or found[3] is None:
        return source
    match,body,_,_=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)


def installed_entries(source):
    found=authenticate(source)
    if found is None or found[3] is None:
        raise ValueError('Complete 107140 default missing')
    return [ENTRY]


def retag_specialized(source,output):
    before=authenticate(source)
    if before is None or before[3] is None:
        return output
    match,body,literal,installed=authenticate(output,allow_old_tag=True)
    if not literal or installed!=before[3]:
        raise ValueError('Specializer moved the 107140 native marker')
    result=output[:match.start()]+HEADER+prefix(True)+apply_ordered_shifts(body)[len(HEADER):]+output[match.end():]
    installed_entries(result)
    return result
