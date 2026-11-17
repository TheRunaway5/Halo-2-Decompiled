"""Exact native admission for the ParticleDraw 252EC0 particle envelope owner."""
import hashlib
import re

ANCHOR='#include "halo2/source_projectile_effect_observer.hpp"\n'
BASENAME='native_tu15_particle_envelope_source_owner.hpp'
INCLUDE=f'#include "rendering/effects/{BASENAME}"\n'
OWNER='native_particle_envelope_owner_detail'
DETAIL='native_particle_envelope_detail'
HEADER='static void run_252ec0(WorldState &s) {'
BODY=re.compile(r'^'+re.escape(HEADER)+r'.*?^\}',re.M|re.S)
PINS={False:'6e91fc9ad50fc3a4cda502e5a3ac39e9b9ad917ef692c53ae409f03cb5f806b9',
      True:'99c7c42bfe8594c363e54265cfc45642b3e031bcefd333e10bc665fcbf45cd88'}
PARENT_CALL='s.push(0x1776bdu,0x1776c2u);run_252ec0(s);'
EXPORT_CASE='case 0x252ec0u:run_252ec0(s);break;'


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def prefix(literal):
    return f'\n    if(halo2::{OWNER}::bridge<{"true" if literal else "false"}>(s))return;'


def split(body):
    matches=[literal for literal in (False,True) if prefix(literal) in body]
    if not matches:
        if OWNER in body or DETAIL in body:
            raise ValueError('Unknown ParticleDraw 252EC0 binding')
        return body,None
    if len(matches)!=1:
        raise ValueError('Conflicting ParticleDraw 252EC0 binding')
    literal=matches[0];marker=prefix(literal)
    if body.count(marker)!=1 or not body.startswith(HEADER+marker):
        raise ValueError('Moved ParticleDraw 252EC0 binding')
    return HEADER+body[len(HEADER+marker):],literal


def canonical_body(body):
    return split(body)[0]


def authenticate(source,*,transition=False):
    if 'void source_run_effect_particle_draw_tu15(' not in source:
        if any(name in source for name in (OWNER,DETAIL,BASENAME)):
            raise ValueError('Foreign 252EC0 binding outside ParticleDraw')
        return None
    rows=list(BODY.finditer(source))
    if len(rows)!=1 or source.count(ANCHOR)!=1 or source.count(PARENT_CALL)!=1:
        raise ValueError('Missing exact 252EC0 owner or genuine 1776BD caller')
    match=rows[0];body,installed=split(match[0]);active=int(installed is not None)
    if (source.count(OWNER)!=active or DETAIL in source or
            source.count(BASENAME)!=active or source.count(ANCHOR+INCLUDE)!=active):
        raise ValueError('Foreign 252EC0 source markers or moved include')
    literal='s.instruction_literal(' in body
    if sha(body)!=PINS[literal] or (installed is not None and literal!=installed and not transition):
        raise ValueError('Exact retained 252EC0 owner or callback format changed')
    export=source.partition('void source_run_effect_particle_draw_tu15(')[2]
    if ('constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;' not in export[:600] or
            export.count(EXPORT_CASE)!=1):
        raise ValueError('ParticleDraw 252EC0 export image or dispatch changed')
    return match,body,literal,installed


def replace_particle_envelope(source):
    found=authenticate(source)
    if found is None or found[3] is not None:
        return source
    match,body,literal,_=found
    bound=HEADER+prefix(literal)+body[len(HEADER):]
    result=(source[:match.start()]+bound+source[match.end():]).replace(ANCHOR,ANCHOR+INCLUDE,1)
    installed_entries(result)
    return result


def remove_particle_envelope(source):
    found=authenticate(source)
    if found is None or found[3] is None:
        return source
    match,body,_,_=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)


def installed_entries(source):
    found=authenticate(source)
    if found is None or found[3] is None:
        raise ValueError('Missing exact ParticleDraw 252EC0 native binding')
    return ['252ec0']


def retag_specialized(source,output):
    prior=authenticate(source)
    after=authenticate(output,transition=True)
    if prior is None or prior[3] is None or after is None or not after[2] or after[3]!=prior[3]:
        raise ValueError('ParticleDraw 252EC0 specialization changed its source transport')
    match,body,literal,_=after
    result=output[:match.start()]+HEADER+prefix(literal)+body[len(HEADER):]+output[match.end():]
    installed_entries(result)
    return result
