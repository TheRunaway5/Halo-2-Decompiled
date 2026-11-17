"""Exact native admission and original flag residue for the ParticleDraw 1E4C0 owner."""
import hashlib
import re

ANCHOR='#include "halo2/source_object_attachment_tu15.hpp"\n'
BASENAME='native_tu15_particle_record_source_owner.hpp'
INCLUDE=f'#include "rendering/effects/{BASENAME}"\n'
OWNER='native_particle_record_owner_detail'
DETAIL='native_particle_record_detail'
HEADER='static void run_1e4c0(WorldState &s) {'
BODY=re.compile(r'^'+re.escape(HEADER)+r'.*?^\}',re.M|re.S)
PINS={False:'c5ed15d5011718aa6815f9b0d22db6c704e6f3453823063d6952bb2d9d9b5120',
      True:'6c223a1d4b64b20f0af815518aac1aafae61ec5cf65987b237269bff64c69100'}
PARENT_CALL='s.push(0x17776cu,0x177771u);run_1e4c0(s);'
EXPORT_CASE='case 0x1e4c0u:run_1e4c0(s);break;'
# IMUL and SHR leave flags undefined; the next access publishes them, so the
# retained continuation executes the original instructions too.
ORDERED=(('s.set(0,s.flags.product(s.reg(0,4,0),0x70u),4,0);',
          f's.set(0,halo2::{DETAIL}::multiply_slot(s,s.reg(0,4,0)),4,0);'),
         ('s.set(1,s.flags.right(s.reg(1,4,0),0x2u,4,false),4,0);',
          f's.set(1,halo2::{DETAIL}::shift_count(s,s.reg(1,4,0)),4,0);'))


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def prefix(literal):
    return f'\n    if(halo2::{OWNER}::bridge<{"true" if literal else "false"}>(s))return;'


def ordered(body,*,remove=False):
    for old,new in ORDERED:
        if remove:
            old,new=new,old
        if body.count(old)!=1 or new in body:
            raise ValueError('Missing or ambiguous 1E4C0 original flag site')
        body=body.replace(old,new,1)
    return body


def split(body):
    matches=[literal for literal in (False,True) if prefix(literal) in body]
    if not matches:
        if OWNER in body or DETAIL in body:
            raise ValueError('Unknown ParticleDraw 1E4C0 binding')
        return body,None
    if len(matches)!=1:
        raise ValueError('Conflicting ParticleDraw 1E4C0 binding')
    literal=matches[0];marker=prefix(literal)
    if body.count(marker)!=1 or not body.startswith(HEADER+marker):
        raise ValueError('Moved ParticleDraw 1E4C0 binding')
    return ordered(HEADER+body[len(HEADER+marker):],remove=True),literal


def canonical_body(body):
    return split(body)[0]


def authenticate(source,*,transition=False):
    if 'void source_run_effect_particle_draw_tu15(' not in source:
        if any(name in source for name in (OWNER,DETAIL,BASENAME)):
            raise ValueError('Foreign 1E4C0 binding outside ParticleDraw')
        return None
    rows=list(BODY.finditer(source))
    if len(rows)!=1 or source.count(ANCHOR)!=1 or source.count(PARENT_CALL)!=1:
        raise ValueError('Missing exact 1E4C0 owner or genuine 17776C caller')
    match=rows[0];body,installed=split(match[0]);active=int(installed is not None)
    if (source.count(OWNER)!=active or source.count(DETAIL)!=2*active or
            source.count(BASENAME)!=active or source.count(ANCHOR+INCLUDE)!=active):
        raise ValueError('Foreign 1E4C0 source markers or moved include')
    literal='s.instruction_literal(' in body
    if sha(body)!=PINS[literal] or (installed is not None and literal!=installed and not transition):
        raise ValueError('Exact retained 1E4C0 owner or callback format changed')
    export=source.partition('void source_run_effect_particle_draw_tu15(')[2]
    if ('constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;' not in export[:600] or
            export.count(EXPORT_CASE)!=1):
        raise ValueError('ParticleDraw 1E4C0 export image or dispatch changed')
    return match,body,literal,installed


def bound_body(body,literal):
    return HEADER+prefix(literal)+ordered(body)[len(HEADER):]


def replace_particle_record(source):
    found=authenticate(source)
    if found is None or found[3] is not None:
        return source
    match,body,literal,_=found
    result=(source[:match.start()]+bound_body(body,literal)+source[match.end():]).replace(ANCHOR,ANCHOR+INCLUDE,1)
    installed_entries(result)
    return result


def remove_particle_record(source):
    found=authenticate(source)
    if found is None or found[3] is None:
        return source
    match,body,_,_=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)


def installed_entries(source):
    found=authenticate(source)
    if found is None or found[3] is None:
        raise ValueError('Missing exact ParticleDraw 1E4C0 native binding')
    return ['1e4c0']


def retag_specialized(source,output):
    prior=authenticate(source)
    after=authenticate(output,transition=True)
    if prior is None or prior[3] is None or after is None or not after[2] or after[3]!=prior[3]:
        raise ValueError('ParticleDraw 1E4C0 specialization changed its source transport')
    match,body,literal,_=after
    result=output[:match.start()]+bound_body(body,literal)+output[match.end():]
    installed_entries(result)
    return result
