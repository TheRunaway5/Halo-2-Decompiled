#!/usr/bin/env python3
"""Compare private 293290 direct decoding and actual row-eight return boundaries."""
import argparse
import copy
import hashlib
import json
import os
import platform
import random
import re
import shlex
import subprocess
import sys
from pathlib import Path

import oracle_native_campaign_screen_lifecycle_tu15 as observer
from native_mopp_ray_dense_original import physical_original_dense
from native_sphere_frustum_mmx_original import physical_original_mmx
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from validate_native_codec_model_format_four_tu15 import CASES as REGIONS, TAIL as ENTRY, TAIL_PIN
from validate_native_codec_sparse_decode_tu15 import (
    COPY_RETURNS, ROOT, SOURCE, STOP, TARGETS, comparison_source, exact, fixtures, patch, sha,
)

CALLS=(0x293329,0x293467,0x29354a)
RETURNS=(0x29332f,0x29346d,0x293550)
PLANES=(0x293340,0x29347e,0x293561)
TRANSLATION=(0x293496,0x2934a3,0x2934b1)
RETURN=0x2935b6
FRAME='src/physics/runtime/native_tu15_codec_format_four_decode_frame.hpp'
DEFECTS=(
    ('advance-translation-destination','store_scalar(0x2934a3u,g[0]+16u,x[1]);','store_scalar(0x2934a3u,g[0]+20u,x[1]);'),
    ('omit-rotation-sign-reload','x[6]=_mm_xor_ps(x[6],vector(0x2933b1u,g[4]+48u));',
     'x[6]=_mm_xor_ps(x[6],vector(0x2933b1u,g[4]+64u));'),
    ('wrong-call-word','Base::store(p.call,stack_slot,p.returned,[&]{g[4]=stack_slot;},true);',
     'Base::store(p.call,stack_slot,p.call,[&]{g[4]=stack_slot;},true);'),
    ('early-call-stack','Base::store(p.call,stack_slot,p.returned,[&]{g[4]=stack_slot;},true);',
     'g[4]=stack_slot;Base::store(p.call,stack_slot,p.returned,[]{},true);'),
    ('wrong-scale-plane','const auto flags=load(p.plane_test,0x56e000u+lane*32u+g[group]*4u);',
     'const auto flags=load(p.plane_test,0x56e000u+(lane==2u?1u:lane)*32u+g[group]*4u);'),
)


def original_context(code):
    body=exact((ROOT/SOURCE).read_text(),r'static void run_293290\(WorldState &s\) \{.*?\n\}')
    expected={int(pc,16) for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',body)}
    entries=(ENTRY,*TARGETS)
    graphs={entry:_trace(code,entry,set(entries)) for entry in entries}
    if (hashlib.sha256(body.encode()).hexdigest()!=TAIL_PIN or len(expected)!=221 or set(graphs[ENTRY].code)!=expected or
        {pc for pc in expected if code.at(pc).mnemonic=='call'}!=set(CALLS) or code.at(RETURN).mnemonic!='ret'):
        raise ValueError('Exact original format-four decoder ownership changed')
    for lane,pc in enumerate(CALLS):
        op=code.at(pc)
        if op.op_str!=f'dword ptr [0x{0x510f64+4*lane:x}]' or pc+op.size!=RETURNS[lane]:
            raise ValueError('Exact original format-four row call changed')
    if any(code.at(pc).op_str.split(',')[0]!='dword ptr [eax + 0x10]' for pc in TRANSLATION):
        raise ValueError('Original format-four repeated translation destination changed')
    return entries,graphs


def prepare_observer(code,image,entries,graphs):
    prior=observer.metadata
    def metadata(ref):
        result=prior(ref)
        for site in result['sites']:
            pc=site['instruction'];op=code.at(pc).mnemonic
            if op in ('pop','ret'):
                if site['operands']:raise ValueError('Unexpected explicit decoder POP/RET operand')
                site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            if pc in PLANES:
                if op!='test' or len(site['operands'])!=1:raise ValueError('Exact decoder plane TEST changed')
                site['operands'][0].update(read=True,write=False)
        return result
    observer.ENTRIES=entries
    observer.complete_body=lambda _,entry:graphs[entry]
    observer.metadata=metadata
    trace,stores=[],[]
    for site in metadata(image)['sites']:
        pc=site['instruction'];pushed=code.at(pc).mnemonic in ('push','call')
        if site['operands'] or pushed:trace.append(dict(entry=pc,gp=True,xmm=True))
        if pc not in CALLS and (pushed or any(op['write'] for op in site['operands'])):stores.append(pc)
    return trace,stores


def inputs(image,smoke):
    rng=random.Random(ENTRY)
    rows=[row for row in fixtures(image,smoke,False,False,False) if not row['via_parent']]
    values=(0,0x80000000,1,0x7fc12345,0xffc23456,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7f812345)
    for i,row in enumerate(rows):
        row.update(id=i,name=f'format-four-decoder-{i}',entry=ENTRY)
        patch(row,0x510f7c,0x61000a00)
        row['banks'].append(dict(address=0x50e000,before=rng.randbytes(8192).hex()))
        row['banks'].sort(key=lambda bank:bank['address'])
        for lane in range(3):
            patch(row,0x56e000+lane*32,(0,0xffffffff,0xaaaaaaaa,0x55555555)[(i//8+lane)%4])
        for track in range(21):
            for lane in range(8):
                patch(row,0x50ef10+track*32+lane*4,values[(i+track+lane)%len(values)])
    return rows


def comparison(scratch):
    cpp=comparison_source(scratch)
    text=cpp.read_text()
    start=text.index('                const auto execute=[&](auto before) {')
    end=text.index('                const bool transform_stop=',start)
    text=text[:start]+'''                const auto execute=[&](auto before) {
                    halo2::native_codec_format_four_decode_detail::run_private(state,before);
                };
'''+text[end:]
    changes=(
        ('#include "physics/runtime/native_tu15_codec_model_secondary_transform_frame.hpp"',
         '#include "physics/runtime/native_tu15_codec_model_secondary_transform_frame.hpp"\n#include "'+FRAME.removeprefix('src/')+'"'),
        ('return pc==0x284da7u||pc==0x284e27u||pc==0x284ea7u;',
         'return '+'||'.join(f'pc==0x{pc:x}u' for pc in CALLS)+';'),
        ('if(memory.pc==0x284dadu||memory.pc==0x284e2du||memory.pc==0x284eadu)++returned_product;',
         'if('+'||'.join(f'memory.pc==0x{pc:x}u' for pc in PLANES)+')++returned_product;'),
        ('const bool transform_stop=row.value("before_transform_continuation",false);',
         'const auto return_stop=row.value("before_child_return",0u);'),
        ('if(use_default&&!transform_stop)','if(use_default&&!return_stop)'),
        ('if(transform_stop&&pc==0x283971u)','if(return_stop==pc)'),
        ('sparse decoder','format-four decoder'),
    )
    for old,new in changes:
        if old not in text:raise ValueError('Exact format-four decoder comparator seam changed')
        text=text.replace(old,new)
    cpp.write_text(text)
    return cpp


def prepare_rows(rows,original):
    sites=set();normal=0;returned=[];repeated=0
    for row,actual in zip(rows,original,strict=True):
        executed=set(actual['executed_sites']);sites.update(executed)
        child_stop=row.get('before_child_return',0)
        if actual['completed']!=('phase'not in row or bool(child_stop)):
            raise ValueError('Original format-four decoder completion differs')
        if child_stop:
            pc=actual['stopped_at'];lane=RETURNS.index(child_stop)
            if (pc!=child_stop or not {TARGETS[lane],COPY_RETURNS[lane]}<=executed or
                not any(w['instruction']==CALLS[lane] and w['written']==pc.to_bytes(4,'little').hex() for w in actual['writes'])):
                raise ValueError('Missing genuine format-four decoder child return')
            returned.append(dict(pc=hex(pc),target=hex(TARGETS[lane]),opaque='active_mmx'in row,
                                 gp=actual['boundary_gp']))
        elif 'phase'in row:pc=actual['boundary']['instruction']
        else:
            pc=RETURN
            if pc not in executed or actual['stopped_at']!=STOP or actual['boundary_gp'][4]!=row['gp'][4]+4:
                raise ValueError('Original format-four decoder did not retire its RET')
            normal+=1
            writes=[w for w in actual['writes'] if w['instruction']in TRANSLATION]
            if len(writes)%3:raise ValueError('Original translation group was incomplete')
            for n in range(0,len(writes),3):
                group=writes[n:n+3]
                if (tuple(w['instruction'] for w in group)!=TRANSLATION or len({w['address'] for w in group})!=1 or
                    group[0]['written']!=group[1]['before'] or group[1]['written']!=group[2]['before']):
                    raise ValueError('Original translation did not write the same component three times')
                repeated+=1
        row['expected']=dict(entry=pc,gp=actual['boundary_gp'],eflags=actual['boundary_eflags'],xmm=actual['boundary_xmm'],
                             fp=actual['after_fp'],live_x87=actual.get('boundary_live_x87',[]))
        if 'active_mmx'in actual:row['expected']['active_mmx']=actual['active_mmx']
        row.update(reads=actual['reads'],writes=actual['writes'],calls=actual['calls'])
        for packet in row['calls']:
            if packet.pop('arguments'):raise ValueError('Unexpected format-four decoder trace arguments')
        for bank,after in zip(row['banks'],actual['banks'],strict=True):
            if bank['address']!=after['address']:raise ValueError('Format-four decoder bank order changed')
            bank['after']=after['after']
    return normal,returned,repeated,sites


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append')
    for option in ('smoke','prefixes','mmx','sanitizers','semantic-mutants'):parser.add_argument('--'+option,action='store_true')
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:parser.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    source_before={path.resolve():sha(path) for tree in ('tools','src','include','tests') for path in (ROOT/tree).rglob('*')
                   if path.is_file() and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    used={ROOT/FRAME,ROOT/SOURCE,Path(__file__).resolve(),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[],
                original_xbe_sha256=NEW_PIN,original_body_sha256=TAIL_PIN,parents_composed=False,
                physical_alignment_fault_qualified=False,physical_split_write_or_fault_qualified=False,
                mutable_provider_qualified=False)
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Exact original English TU1.5 changed')
        image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');entries,graphs=original_context(code)
        trace,stores=prepare_observer(code,image,entries,graphs)
        rows=inputs(image,args.smoke);normal=len(rows);proofs=[]
        def measure(batch,label):
            folder=scratch/label;folder.mkdir();opaque='active_mmx'in batch[0]
            runner=physical_original_mmx if opaque else physical_original_dense
            result,proof=runner(image,batch,folder,entries=entries,trace_calls=trace,trace_extended=True,
                regions=REGIONS+(((0x64000000,4096),)if opaque else()))
            proofs.append(proof);return result['cases']
        original=measure(rows,'normal')
        if args.mmx:
            rng=random.Random(ENTRY^0xff);opaque=[]
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(rows[mask%normal]);row.update(id=normal+mask,name=row['name']+f'-opaque-{mask}',live_x87=[])
                row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[(rng.getrandbits(64).to_bytes(8,'little')+
                    (0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex() for n in range(8)])
                opaque.append(row)
            rows+=opaque;original+=measure(opaque,'opaque')
        if args.prefixes:
            pending=[];seen=set()
            for row,actual in zip(rows,original,strict=True):
                opaque='active_mmx'in row
                for lane,pc in enumerate(RETURNS):
                    key=(opaque,'return',pc)
                    if COPY_RETURNS[lane] not in actual['executed_sites'] or key in seen:continue
                    seen.add(key);prefix=copy.deepcopy(row)
                    prefix.update(name=row['name']+f'-returned-{pc:x}',phase='before_child_return',
                                  before_child_return=pc,stop_before_entries=[pc])
                    pending.append(prefix)
                for phase,stream in (('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(actual[stream]):
                        pc=event['instruction'];key=(opaque,phase,pc)
                        if key in seen or (phase=='before_write' and pc in CALLS):continue
                        seen.add(key);prefix=copy.deepcopy(row)
                        prefix.update(name=row['name']+f'-{phase}-{pc:x}',phase=phase,fault=dict(phase=phase,ordinal=ordinal))
                        pending.append(prefix)
            for opaque in (False,True):
                batch=[row for row in pending if ('active_mmx'in row)==opaque]
                if not batch:continue
                for i,row in enumerate(batch,len(rows)):row['id']=i
                rows+=batch;original+=measure(batch,'prefix-'+str(opaque))
        complete,returned,repeated,coverage=prepare_rows(rows,original)
        required={pc for graph in graphs.values() for pc in graph.code}
        if required!=coverage or not repeated:
            raise ValueError('Original format-four decoder coverage differs: '+str(sorted(hex(pc) for pc in required-coverage)))
        if args.prefixes and {(int(w['pc'],16),w['opaque']) for w in returned}!={
                (pc,opaque) for pc in RETURNS for opaque in ((False,True)if args.mmx else(False,))}:
            raise ValueError('Original format-four decoder is missing independent child returns')
        report.update(physical_paths=len(rows),complete_returns=complete,interruption_prefixes=len(rows)-complete,
                      opaque_paths=sum('active_mmx'in row for row in rows),genuine_child_return_stops=returned,
                      repeated_translation_groups=repeated,observed_decoder_sites=len(graphs[ENTRY].code),
                      observed_child_sites=len(required)-len(graphs[ENTRY].code),original=proofs)
        cpp=comparison(scratch);fixture=scratch/'format-four-decode.json';negative=scratch/'format-four-decode-mutants.json'
        fixture.write_text(json.dumps(dict(cases=rows,write_sites=stores),separators=(',',':')))
        negative.write_text(json.dumps(dict(cases=rows[:normal],write_sites=stores),separators=(',',':')))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                     '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(cpp),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            def run(build,probe,case_path):
                result=subprocess.run(build,capture_output=True,text=True,timeout=480)
                if result.returncode:raise ValueError('Format-four decoder compile failed: '+result.stderr[-12000:])
                return subprocess.run([str(probe),str(case_path)],capture_output=True,text=True,timeout=480)
            done=run(command,binary,fixture)
            if done.returncode:raise ValueError('Format-four decoder original comparison failed: '+(done.stdout+done.stderr)[-12000:])
            result=dict(compiler=compiler,result=json.loads(done.stdout),compiled_defects=[]);report['compilers'].append(result)
            print(f'{compiler} original format-four decoder comparison passed',flush=True)
            for dependency in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(dependency).resolve()
                if path in source_before:used.add(path)
            if args.semantic_mutants:
                original_header=(ROOT/FRAME).read_text()
                for name,old,new in DEFECTS:
                    if original_header.count(old)!=1:raise ValueError('Exact format-four decoder mutant seam changed: '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);header=overlay/FRAME
                    header.parent.mkdir(parents=True);header.write_text(original_header.replace(old,new,1));probe=overlay/'probe'
                    build=[command[0],'-I'+str(overlay/'src'),*command[1:]]
                    build[build.index('-o')+1]=str(probe);build[build.index('-MF')+1]=str(overlay/'probe.d')
                    done=run(build,probe,negative);diagnostic=done.stdout+done.stderr
                    if not done.returncode or not ('format-four-decoder-'in diagnostic or 'Indirect CALL provider pre-write ABI differs'in diagnostic):
                        raise ValueError('Format-four decoder defect survived: '+name+' '+diagnostic[-7000:])
                    result['compiled_defects'].append(dict(name=name,rejected=True,diagnostic=diagnostic[-7000:]))
                    print(f'{compiler} rejected format-four decoder defect {name}',flush=True)
        if original_context(code)[0]!=entries:raise ValueError('Exact format-four decoder source owner drifted')
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        used.update(Path(module.__file__).resolve() for module in tuple(sys.modules.values())
                    if getattr(module,'__file__',None) and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        report['source_before']={str(path.relative_to(ROOT)):source_before[path] for path in sorted(used)}
        report['source_after']={name:sha(ROOT/name) for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
