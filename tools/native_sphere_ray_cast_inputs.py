"""Private raw/prepared copies of the genuine complete sphere source owner."""
import re
import subprocess
import sys
from native_sphere_ray_cast_replacements import (BODY,PREFIX,INCLUDE,PIN,digest,
    authenticate,replace_sphere_ray_cast,remove_sphere_ray_cast,SINGLE_SPILL)
from specialize_native_operands import specialize

UNIT='src/physics/physics_live_support_tu15_generated.cpp'
def prepare(root,scratch,require_binding=False):
    source=(root/UNIT).read_text()
    fresh=scratch/'fresh-sphere-primary.cpp'
    command=[sys.executable,'-B',str(root/'tools/generate_physics_live_support_tu15.py'),'--output',str(fresh),'--manifest',str(scratch/'fresh-sphere-manifest.json')]
    result=subprocess.run(command,capture_output=True,text=True,cwd=root,timeout=300)
    if result.returncode:raise ValueError('Full fresh sphere source generation failed '+result.stderr[-7000:])
    pieces=[];receipts=[];rejected=0
    for origin,source in(('current',source),('fresh',fresh.read_text())):
        match,body=authenticate(source);bound=replace_sphere_ray_cast(source)
        if require_binding and bound!=source:raise ValueError('Missing production sphere binding '+origin)
        assert replace_sphere_ray_cast(bound)==bound
        if PREFIX not in match[0]:assert remove_sphere_ray_cast(bound)==source
        prepared,receipt=specialize(bound)
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
        dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
        helper=SINGLE_SPILL.search(source)
        for suffix,text in(('bound',bound),('prepared',prepared)):
            own=BODY.search(text)[0];assert PREFIX in own
            instrumented=own.replace(PREFIX,PREFIX+'\n    ++literal_entries;',1)
            reference=own.replace(PREFIX,'\n    ++reference_entries;',1)
            namespace=('fresh_'if origin=='fresh'else'')+suffix
            pieces.append('namespace '+namespace+' {\ninline unsigned literal_entries{},reference_entries{};\n'+(helper[0]+'\n'if helper else'')+resolver+'\n'+instrumented+'\n'+reference.replace('run_2e8dd0(', 'retained_2e8dd0(',1)+'\n}\n')
        mutations=[source.replace(match[0],'',1),source.replace(match[0],match[0]+match[0],1),
            source.replace(match[0],match[0].replace('0xcu','0x10u',1),1),
            source.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),
            source.replace(dispatcher,dispatcher.replace('run_2e8dd0(s);','run_306220(s);',1),1),
            source.replace(dispatcher,dispatcher.replace('s.required(site,target,tail);','s.required(site,target+1u,tail);',1),1),
            bound.replace(INCLUDE,'',1),bound.replace(PREFIX,PREFIX.replace('::bridge','::wrong_bridge'),1)]
        if helper:mutations.append(source.replace(helper[0],helper[0].replace('actual&0x200u','actual&0x400u',1),1))
        for index,changed in enumerate(mutations):
            if changed in(source,bound):raise ValueError('Missing sphere authentication mutation')
            try:authenticate(changed);replace_sphere_ray_cast(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Admitted sphere context mutation '+origin+':'+str(index))
        receipts.append(dict(origin=origin,unit=UNIT,body_sha256=digest(body),resolver_sha256=digest(resolver),dispatcher_sha256=digest(dispatcher),
            raw_bound_body_sha256=digest(BODY.search(bound)[0]),prepared_body_sha256=digest(BODY.search(prepared)[0]),
            single_spill_helper_sha256=digest(helper[0])if helper else None,raw_source_sha256=digest(source),prepared_receipt_sha256=digest(str(receipt))))
    (scratch/'sphere_source_bodies.inc').write_text(''.join(pieces))
    return dict(owners=receipts,source_variants=4,authentication_mutations_rejected=rejected,production_binding_required=require_binding,
                fresh_generation_command=command,exact_fresh_delta='Whole-pinned C1/spill emission and helper; no retained source rewritten')
