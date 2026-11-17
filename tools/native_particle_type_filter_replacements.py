"""Exact native admission and observed IMUL residue for the ParticleDraw 17DE60 owner."""
import hashlib
import importlib.util
from pathlib import Path
import re

_child_spec=importlib.util.spec_from_file_location('particle_type_filter_lookup_copies',
    Path(__file__).with_name('native_object_lookup_copies_replacements.py'))
child=importlib.util.module_from_spec(_child_spec)
_child_spec.loader.exec_module(child)

ANCHOR='#include "halo2/source_effect_particle_draw_tu15.hpp"\n'
BASENAME='native_tu15_particle_type_filter_source_owner.hpp'
INCLUDE=f'#include "rendering/effects/{BASENAME}"\n'
OWNER='native_particle_type_filter_owner_detail'
DETAIL='native_particle_type_filter_detail'
HEADER='static void run_17de60(WorldState &s) {'
BODY=re.compile(r'^'+re.escape(HEADER)+r'.*?^\}',re.M|re.S)
PINS={False:'075a21b83f8172c030d3740d1fd16ab999d73ce4a9befdd8d98f68097e667fed',
      True:'84fe2b3493ee37b4f083171da86055783cb06cdc327ff486e5a3f7af82451088'}
STRIDES=('0x190u','0x21cu')
PARENT_CALL='s.push(0x1771dau,0x1771dfu);run_17de60(s);'


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def prefix(literal):
    return (f'\n    if(halo2::{OWNER}::bridge<{"true" if literal else "false"}>'
            '(s,[](auto& child){run_bcdf0(child);}))return;')


def arithmetic(body,*,remove=False):
    for stride in STRIDES:
        old=f's.flags.product(s.reg(0,4,0),{stride})'
        new=f'halo2::{DETAIL}::multiply_row<true>(s,{stride})'
        if remove:
            old,new=new,old
        if body.count(old)!=1 or new in body:
            raise ValueError('Missing or ambiguous 17DE60 observed IMUL site')
        body=body.replace(old,new,1)
    return body


def split(body):
    matches=[literal for literal in (False,True) if prefix(literal) in body]
    if not matches:
        if OWNER in body or DETAIL in body:
            raise ValueError('Unknown ParticleDraw 17DE60 binding')
        return body,None
    if len(matches)!=1:
        raise ValueError('Conflicting ParticleDraw 17DE60 binding')
    literal=matches[0];marker=prefix(literal)
    if body.count(marker)!=1 or not body.startswith(HEADER+marker):
        raise ValueError('Moved ParticleDraw 17DE60 binding')
    return arithmetic(HEADER+body[len(HEADER+marker):],remove=True),literal


def canonical_body(body):
    return split(body)[0]


def authenticate(source,*,transition=False):
    if 'void source_run_effect_particle_draw_tu15(' not in source:
        if any(name in source for name in (OWNER,DETAIL,BASENAME)):
            raise ValueError('Foreign 17DE60 binding outside ParticleDraw')
        return None
    rows=list(BODY.finditer(source))
    if len(rows)!=1 or source.count(ANCHOR)!=1 or source.count(PARENT_CALL)!=1:
        raise ValueError('Missing exact 17DE60 owner or genuine 1771DA caller')
    if child.authenticate(source)[0]!='effect_particle_draw':
        raise ValueError('Missing exact ParticleDraw BCDF0 route')
    match=rows[0];body,installed=split(match[0]);active=int(installed is not None)
    if (source.count(OWNER)!=active or source.count(DETAIL)!=2*active or
            source.count(BASENAME)!=active or source.count(ANCHOR+INCLUDE)!=active):
        raise ValueError('Foreign 17DE60 source markers or moved include')
    literal='s.instruction_literal(' in body
    if sha(body)!=PINS[literal] or (installed is not None and literal!=installed and not transition):
        raise ValueError('Exact retained 17DE60 owner or callback format changed')
    export=source.partition('void source_run_effect_particle_draw_tu15(')[2]
    if ('constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;' not in export[:600] or
            export.count('case 0x17de60u:run_17de60(s);break;')!=1):
        raise ValueError('ParticleDraw 17DE60 export image or dispatch changed')
    return match,body,literal,installed


def replace_particle_type_filter(source):
    found=authenticate(source)
    if found is None or found[3] is not None:
        return source
    match,body,literal,_=found
    bound=HEADER+prefix(literal)+arithmetic(body)[len(HEADER):]
    result=(source[:match.start()]+bound+source[match.end():]).replace(ANCHOR,ANCHOR+INCLUDE,1)
    installed_entries(result)
    return result


def remove_particle_type_filter(source):
    found=authenticate(source)
    if found is None or found[3] is None:
        return source
    match,body,_,_=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)


def installed_entries(source):
    found=authenticate(source)
    if found is None or found[3] is None:
        raise ValueError('Missing exact ParticleDraw 17DE60 native binding')
    return ['17de60']


def retag_specialized(source,output):
    prior=authenticate(source)
    after=authenticate(output,transition=True)
    if prior is None or prior[3] is None or after is None or not after[2] or after[3]!=prior[3]:
        raise ValueError('ParticleDraw 17DE60 specialization changed its source transport')
    match,body,literal,_=after
    replacement=HEADER+prefix(literal)+arithmetic(body)[len(HEADER):]
    result=output[:match.start()]+replacement+output[match.end():]
    installed_entries(result)
    return result
