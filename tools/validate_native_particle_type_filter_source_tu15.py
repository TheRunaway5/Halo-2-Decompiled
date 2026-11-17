#!/usr/bin/env python3
"""Compare private 17DE60 source with complete real ParticleDraw ownership."""
from pathlib import Path
import re
import subprocess
import sys

import native_particle_type_filter_replacements as binding
from native_object_lookup_copies_replacements import installed_entries
from specialize_native_operands import specialize
from validate_native_flight_property_query_source_tu15 import SourceProfile, digest, main, one
from validate_native_indexed_datum_gate_source_tu15 import CALLBACK, SIGN_EXTEND, WORLD
from validate_native_particle_type_filter_frame_tu15 import corpus

ROOT=Path(__file__).resolve().parents[1]
UNIT='src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp'
GEN='tools/generate_effect_particle_draw_tu15.py'
FRAME='src/rendering/effects/native_tu15_particle_type_filter_source_frame.hpp'
DRIVER='tests/native_tu15_particle_type_filter_source_tests.cpp'
BODY=re.compile(r'^static void run_17de60\(WorldState &s\) \{.*?^\}',re.M|re.S)
CHILD=re.compile(r'^static void run_bcdf0\(WorldState &s\) \{.*?^\}',re.M|re.S)
PIN='075a21b83f8172c030d3740d1fd16ab999d73ce4a9befdd8d98f68097e667fed'
CALL='s.push(0x17de7du,0x17de82u);run_bcdf0(s);'


def canonical_source(label,text,require_binding):
    found=binding.authenticate(text)
    if found is None:
        raise ValueError('Missing ParticleDraw 17DE60 owner '+label)
    if (found[3] is not None)!=require_binding:
        raise ValueError('17DE60 native default installation differs from the declared mode '+label)
    if not require_binding:
        return text,None
    canonical=binding.remove_particle_type_filter(text)
    if binding.replace_particle_type_filter(canonical)!=text or binding.installed_entries(text)!=['17de60']:
        raise ValueError('Installed 17DE60 raw default does not reverse exactly '+label)
    prepared,receipt=specialize(text)
    if (receipt.get('native_particle_type_filter_entries')!=['17de60'] or specialize(prepared)[0]!=prepared or
            binding.remove_particle_type_filter(prepared)!=specialize(canonical)[0]):
        raise ValueError('Installed 17DE60 prepared default does not reverse exactly '+label)
    return canonical,dict(installed_source_sha256=digest(text),installed_body_sha256=digest(one(BODY,text,'installed 17DE60')),
                          prepared_installed_body_sha256=digest(one(BODY,prepared,'prepared installed 17DE60')),exact_raw_prepared_inverse=True)


def prepare(scratch,require_binding=False):
    fresh=scratch/'draw.cpp'
    cmd=[sys.executable,'-B',str(ROOT/GEN),'--output',str(fresh),'--directory',str(scratch/'draw')]
    made=subprocess.run(cmd,cwd=ROOT,text=True,capture_output=True,timeout=600)
    if made.returncode:
        raise ValueError('ParticleDraw genuine generation failed: '+made.stderr[-8000:])
    current,current_binding=canonical_source('current',(ROOT/UNIT).read_text(),require_binding)
    fresh_text,fresh_binding=canonical_source('fresh',fresh.read_text(),require_binding)
    expected=one(BODY,current,'17DE60 current')
    if digest(expected)!=PIN:
        raise ValueError('Current whole 17DE60 body changed')
    chunks=[];evidence={}
    if require_binding:
        evidence['installed']=dict(current=current_binding,fresh=fresh_binding)
    for label,raw in (('current',current),('fresh',fresh_text)):
        state=one(WORLD,raw,'ParticleDraw WorldState')
        extend=one(SIGN_EXTEND,raw,'ParticleDraw sign_extend')
        chunks+=['namespace '+label+' {','using namespace halo2::physics_world_runtime_detail;',state,extend]
        prepared=specialize(raw)[0]
        if one(WORLD,prepared,'prepared ParticleDraw WorldState')!=state:
            raise ValueError('ParticleDraw prepared state changed')
        for variant,source in (('raw',raw),('literal',prepared)):
            body=one(BODY,source,'17DE60 owner')
            literal=variant=='literal'
            canonical=re.sub(r's\.instruction_literal\(([^)]+)\);',r'if(s.services.instruction){s.services.instruction(\1);}',body)
            if canonical!=expected or body.count(CALL)!=1 or len(set(CALLBACK.findall(body)))!=27:
                raise ValueError('Complete 17DE60 callback or child edge drift')
            child=one(CHILD,source,'actual local BCDF0')
            if 'bcdf0' not in installed_entries(source) or 'native_object_lookup_copies_detail::bridge' not in child:
                raise ValueError('Actual BCDF0 independent native child changed')
            chunks+=[child.replace('run_bcdf0(',variant+'_child(',1)]
            body=body.replace('run_bcdf0(s)',variant+'_child(s)',1)
            ordered=body
            for stride in ('0x190u','0x21cu'):
                old=f's.flags.product(s.reg(0,4,0),{stride})'
                new=f'halo2::native_particle_type_filter_detail::multiply_row<true>(s,{stride})'
                if ordered.count(old)!=1:
                    raise ValueError('17DE60 original IMUL site drift')
                ordered=ordered.replace(old,new,1)
            chunks+=[body.replace('run_17de60(',variant+'_modeled(',1),
                     ordered.replace('run_17de60(',variant+'_ordered(',1)]
            evidence[label+'-'+variant]=dict(source_sha256=digest(source),parent_sha256=digest(body),
                genuine_local_child_sha256=digest(child),ordered_parent_sha256=digest(ordered))
        chunks+=['}',f'static void lookup_child({label}::WorldState& s,bool literal){{'
                 f'if(literal){label}::literal_child(s);else {label}::raw_child(s);}}',
                 f'static void retained({label}::WorldState& s,bool literal,bool ordered){{'
                 f'if(literal){{if(ordered){label}::literal_ordered(s);else {label}::literal_modeled(s);}}'
                 f'else if(ordered){label}::raw_ordered(s);else {label}::raw_modeled(s);}}']
    (scratch/'particle_type_filter_source_owners.inc').write_text('\n'.join(chunks)+'\n')
    return dict(form=evidence,generation=cmd,actual_child='BCDF0 same ParticleDraw owner',
                source_model_compared=True,original_ordered_integer_residue_compared=True)


PROFILE=SourceProfile(UNIT,GEN,FRAME,DRIVER,prepare,corpus,'17DE60',(
    ('wrong-type-mask','s.push(0x17de79u,3u)','s.push(0x17de79u,1u)'),
    ('wrong-return-pc','s.push(0x17de7du,0x17de82u)','s.push(0x17de7du,0x17de83u)'),
    ('wrong-type-stride','multiply_row<Ordered>(s,0x21cu)','multiply_row<Ordered>(s,0x218u)'),
),extra_pins=('tools/validate_native_particle_type_filter_source_tu15.py','tools/native_particle_type_filter_replacements.py'))

if __name__=='__main__':
    raise SystemExit(main(PROFILE))
