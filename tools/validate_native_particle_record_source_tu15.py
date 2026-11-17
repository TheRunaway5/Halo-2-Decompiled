#!/usr/bin/env python3
"""Compare native 1E4C0 source with the complete retained ParticleDraw owner."""
from pathlib import Path
import subprocess
import sys

import native_particle_record_replacements as binding
from specialize_native_operands import specialize
from validate_native_flight_property_query_source_tu15 import SourceProfile, digest, main, one
from validate_native_indexed_datum_gate_source_tu15 import CALLBACK, SIGN_EXTEND, WORLD
from validate_native_particle_record_frame_tu15 import corpus

ROOT=Path(__file__).resolve().parents[1]
UNIT='src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp'
GEN='tools/generate_effect_particle_draw_tu15.py'
FRAME='src/rendering/effects/native_tu15_particle_record_source_frame.hpp'
DRIVER='tests/native_tu15_particle_record_source_tests.cpp'


def canonical_source(label,text,require_binding):
    found=binding.authenticate(text)
    if found is None:
        raise ValueError('Missing ParticleDraw 1E4C0 owner '+label)
    if (found[3] is not None)!=require_binding:
        raise ValueError('1E4C0 native default installation differs from the declared mode '+label)
    if not require_binding:
        return text,None
    canonical=binding.remove_particle_record(text)
    if binding.replace_particle_record(canonical)!=text or binding.installed_entries(text)!=['1e4c0']:
        raise ValueError('Installed 1E4C0 raw default does not reverse exactly '+label)
    prepared,receipt=specialize(text)
    if (receipt.get('native_particle_record_entries')!=['1e4c0'] or specialize(prepared)[0]!=prepared or
            binding.remove_particle_record(prepared)!=specialize(canonical)[0]):
        raise ValueError('Installed 1E4C0 prepared default does not reverse exactly '+label)
    return canonical,dict(installed_source_sha256=digest(text),installed_body_sha256=digest(one(binding.BODY,text,'installed 1E4C0')),
                          prepared_installed_body_sha256=digest(one(binding.BODY,prepared,'prepared installed 1E4C0')),
                          exact_raw_prepared_inverse=True)


def prepare(scratch,require_binding=False):
    fresh=scratch/'draw.cpp'
    cmd=[sys.executable,'-B',str(ROOT/GEN),'--output',str(fresh),'--directory',str(scratch/'draw')]
    made=subprocess.run(cmd,cwd=ROOT,text=True,capture_output=True,timeout=600)
    if made.returncode:
        raise ValueError('ParticleDraw genuine generation failed: '+made.stderr[-8000:])
    current,current_binding=canonical_source('current',(ROOT/UNIT).read_text(),require_binding)
    fresh_text,fresh_binding=canonical_source('fresh',fresh.read_text(),require_binding)
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
            body=one(binding.BODY,source,'1E4C0 owner')
            if digest(body)!=binding.PINS[variant=='literal'] or len(set(CALLBACK.findall(body)))!=64:
                raise ValueError('Complete 1E4C0 callback or body drift')
            # The oracle executes the original IMUL/SHR flags, as installed.
            oracle=binding.ordered(body)
            chunks+=[oracle.replace('run_1e4c0(',variant+'_retained(',1)]
            evidence[label+'-'+variant]=dict(source_sha256=digest(source),body_sha256=digest(body),
                                             ordered_oracle_sha256=digest(oracle))
        chunks+=['}',f'static void retained({label}::WorldState& s,bool literal){{'
                 f'if(literal){label}::literal_retained(s);else {label}::raw_retained(s);}}']
    (scratch/'particle_record_source_owners.inc').write_text('\n'.join(chunks)+'\n')
    return dict(form=evidence,generation=cmd,source_model_compared=True,original_flag_oracle=True)


PROFILE=SourceProfile(UNIT,GEN,FRAME,DRIVER,prepare,corpus,'1E4C0',(
    ('ignored-direction','(s.other_flags&1024u)?0u-width:width','width'),
    ('missing-iteration-callback','--g[1];\n            if(s.services.instruction)','--g[1];\n            if(false)'),
    ('commuted-plane-sum','x[3]=add(x[3],x[2]);','x[3]=add(x[2],x[3]);'),
),extra_pins=('tools/validate_native_particle_record_source_tu15.py','tools/native_particle_record_replacements.py'))

if __name__=='__main__':
    raise SystemExit(main(PROFILE))
