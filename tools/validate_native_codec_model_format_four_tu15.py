#!/usr/bin/env python3
"""Compare the genuine AL=1 codec transform and format-four tail frontier."""
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
from pathlib import Path

import oracle_native_campaign_screen_lifecycle_tu15 as observer
import validate_native_codec_transform_publication_tu15 as publication
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from validate_native_codec_sparse_decode_tu15 import (
    CALLS, ENTRY, PARENT, PARENT_TRANSFORM_CALL, PARENT_TRANSFORM_JOIN,
    REGIONS, ROOT, SECOND_SELECTOR, SELECTOR, SELECTORS, SOURCE, STOP, TARGETS, TRANSFORM_ENTRY,
    TRANSFORM_FRAME, comparison_source, exact, fixtures, original_owners, parent_owners,
    patch, selector_owners, sha, transform_owners,
)

FIELD, JUMP, TAIL = 0x284290, 0x2842be, 0x293290
TAIL_PIN = '80f4f97d339c22c9469bae67a3b4638860be3fac5f2c9ba205981c20f51e43d2'
RETURNS = (0x283ecc, 0x283ee9, 0x283f7b)
TRANSFORM_ENTRIES = publication.ENTRIES
CASES = tuple(sorted((*REGIONS, (0x50e000,8192))))
DOUBLE_ACCESS = {*CALLS, *publication.REPEATS, *publication.RMW}
RETURN_FRAME = 'src/physics/runtime/native_tu15_codec_model_format_four_return_frame.hpp'
DEFECTS = (
    ('skip-format-four-store','if(!s_.flags.zf)store(0x283975u,0x510f30u,4u);',
     'if(s_.flags.zf)store(0x283975u,0x510f30u,4u);'),
    ('wrong-active-return','Base::push(0x28396cu,0x283971u);','Base::push(0x28396cu,0x28397fu);'),
    ('wrong-format-four-return','Base::push(0x283a37u,0x283a3cu);','Base::push(0x283a37u,0x283947u);'),
)


def inputs(image,smoke,through_return=False):
    rng=random.Random(TAIL)
    rows=fixtures(image,smoke,False,False,True)
    if not smoke:
        rows=[copy.deepcopy(row) for _ in range(8) for row in rows]
    for profile,row in enumerate(rows):
        row.update(id=profile,name=f'codec-format-four-{profile}',stop_before_entries=[] if through_return else [TAIL],
                   max_steps=3800,live_x87=[])
        row['before_fp'][1]&=~0x3800
        row['banks'].append(dict(address=0x50e000,before=rng.randbytes(8192).hex()))
        row['banks'].sort(key=lambda bank:bank['address'])
        model=row['source'];root=0x61000c80;ancestry=0x61000d00;record=0x610009c0;values=0x61000a00
        node=3;parent=4 if profile&1 else -1;publication_index=5
        row['matrix']=root
        patch(row,row['gp'][4]+4,root)
        patch(row,root+16,ancestry)
        patch(row,model+100,1);patch(row,model+104,record)
        patch(row,record,node,2);patch(row,record+2,profile%8,1)
        for lane,value in enumerate((1+profile,-2,3,8)):
            patch(row,record+4+lane*2,value,2)
        for lane,value in enumerate((0x3f800000,0xc0000000,0x3f000000)):
            patch(row,record+12+lane*4,value)
        patch(row,ancestry+node*32+8,parent,2)
        patch(row,ancestry+node*32+6,publication_index,2)
        patch(row,ancestry+4*32+8,-1,2)
        patch(row,ancestry+4*32+6,publication_index,2)
        patch(row,ancestry+publication_index*32+4,-1,2)
        for track in (node,4):
            for lane,word in enumerate((0,0,0,0x3f800000,0x3e800000,0x3f800000,0xbe800000,0x3f800000)):
                patch(row,values+track*32+lane*4,word)
        row['has_ancestry_parent']=parent>=0
        row['output_slot']=0x50ef10+publication_index*32
        if through_return:
            secondary_bitmap=0x61000800+(-32,32)[profile%2]
            for lane,mask in enumerate((0x24,0xa1,0x29)):
                patch(row,secondary_bitmap+lane*4,mask,1)
    return rows


def original_context(code,decoder=None):
    parents=parent_owners()
    selectors,tails=selector_owners()
    transforms=transform_owners()
    tail=exact((ROOT/SOURCE).read_text(),r'static void run_293290\(WorldState &s\) \{.*?\n\}')
    tail_sites={int(pc,16) for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',tail)}
    if (hashlib.sha256(tail.encode()).hexdigest()!=TAIL_PIN or len(tail_sites)!=221 or
        tails[FIELD][JUMP]!=TAIL or (code.at(JUMP).mnemonic,code.at(JUMP).op_str)!=('jmp','0x293290')):
        raise ValueError('Exact format-four tail owner changed')
    entries=(PARENT,*SELECTORS,FIELD,ENTRY,*TARGETS,*TRANSFORM_ENTRIES)
    if decoder:entries+=(TAIL,)
    starts=set(entries)|{pc for routes in tails.values() for pc in routes.values()}
    graphs={entry:_trace(code,entry,starts) for entry in entries}
    graphs[ENTRY]=original_owners(code)
    if decoder:graphs[TAIL]=decoder.original_context(code)[1][TAIL]
    if set(graphs[TRANSFORM_ENTRY].code)!=set(transforms[SOURCE]):
        raise ValueError('Format-four transform source and physical owner differ')
    return entries,graphs,(parents,selectors,tails,transforms)


def prepare_observer(code,entries,graphs,image,decoder=None):
    prior_metadata=observer.metadata
    plane_tests=(0x28382c,0x283e6b,*(decoder.PLANES if decoder else ()))
    double_access=DOUBLE_ACCESS|set(decoder.CALLS if decoder else ())
    def metadata(ref):
        result=prior_metadata(ref)
        for site in result['sites']:
            pc=site['instruction'];op=code.at(pc).mnemonic
            if op in ('pop','ret'):
                if site['operands']:raise ValueError('Unexpected explicit POP/RET operand')
                site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            if pc in plane_tests:
                if op!='test' or len(site['operands'])!=1:raise ValueError('Exact memory TEST changed')
                site['operands'][0].update(read=True,write=False)
            if pc in publication.REPEATS:
                if op!='rep movsd':raise ValueError('Exact original REP changed')
                site['operands']=[dict(base=6,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False),
                                  dict(base=7,index=-1,scale=1,displacement=0,bytes=4,read=False,write=True)]
            if pc in publication.RMW and (op!='or' or len(site['operands'])!=1 or
               not (site['operands'][0]['read'] and site['operands'][0]['write'])):
                raise ValueError('Exact original OR read-modify-write changed')
        return result
    observer.ENTRIES=entries
    observer.complete_body=lambda _,entry:graphs[entry]
    observer.metadata=metadata
    publication.ENTRIES=entries
    trace,stores=[],[]
    for site in metadata(image)['sites']:
        pc=site['instruction'];pushed=code.at(pc).mnemonic in ('push','call')
        if site['operands'] or pushed:trace.append(dict(entry=pc,gp=True,xmm=True))
        if pc not in double_access and (pushed or any(op['write'] for op in site['operands'])):stores.append(pc)
    return trace,stores


def comparison(scratch,decoder=None):
    cpp=comparison_source(scratch)
    text=cpp.read_text()
    double_access=DOUBLE_ACCESS|set(decoder.CALLS if decoder else ())
    changes=[
        ('return pc==0x284da7u||pc==0x284e27u||pc==0x284ea7u;',
         'return '+'||'.join(f'pc==0x{pc:x}u' for pc in sorted(double_access))+';'),
    ]
    if decoder:
        anchor='name+": secondary parent did not retire RET28");'
        changes.extend((
            ('#include "physics/runtime/native_tu15_codec_model_secondary_transform_frame.hpp"',
             '#include "physics/runtime/native_tu15_codec_model_secondary_transform_frame.hpp"\n#include "'+
             RETURN_FRAME.removeprefix('src/')+'"'),
            ('                        check(stop.kind==secondary::Boundary::returned&&stop.instruction==0x283a3fu,\n'
             '                              '+anchor,
             '                        check(stop.kind==secondary::Boundary::returned&&stop.instruction==0x283a3fu,\n'
             '                              '+anchor+'\n'
             '                        halo2::native_codec_model_format_four_return_detail::run_private_tail_return(state,before);'),
            ('if(use_default&&!transform_stop)', 'const auto child_stop=row.value("before_format_four_child_return",0u);\n'
             '                if(use_default&&!transform_stop&&!child_stop)'),
            ('if(transform_stop&&pc==0x283971u)', 'if((transform_stop&&pc==0x283971u)||child_stop==pc)'),
        ))
    changes.extend((
        ('stop.kind==secondary::Boundary::returned&&stop.instruction==0x283a3fu',
         'stop.kind==secondary::Boundary::tail_entry&&stop.instruction==0x293290u'),
        ('secondary parent did not retire RET28','secondary parent did not reach the original format-four tail'),
    ))
    for old,new in changes:
        if text.count(old)!=1:raise ValueError('Exact format-four comparator seam changed')
        text=text.replace(old,new,1)
    cpp.write_text(text)
    return cpp


def qualify_paths(rows,original,decoder=None):
    normal,returns,call_stops,decoder_returns,parent_resumes,coverage=0,[],[],[],[],set()
    for row,actual in zip(rows,original,strict=True):
        sites=set(actual['executed_sites']);coverage.update(sites)
        continued=row.get('before_transform_continuation',False)
        child_stop=row.get('before_format_four_child_return',0)
        if actual['completed']!=('phase' not in row or continued or bool(child_stop)) or (not decoder and TAIL in sites):
            raise ValueError('Unexpected original format-four completion or decoder entry')
        if child_stop:
            pc=actual['stopped_at'];lane=decoder.RETURNS.index(child_stop)
            if (pc!=child_stop or not {TAIL,0x283975,SECOND_SELECTOR,JUMP,decoder.TARGETS[lane],decoder.COPY_RETURNS[lane]}<=sites or
                not any(w['instruction']==decoder.CALLS[lane] and w['written']==pc.to_bytes(4,'little').hex()
                        for w in actual['writes'])):
                raise ValueError('Missing genuine format-four child return in parent')
            decoder_returns.append(dict(pc=hex(pc),target=hex(decoder.TARGETS[lane]),gp=actual['boundary_gp']))
        elif 'phase' in row and not continued:
            pc=actual['boundary']['instruction']
            if pc==SECOND_SELECTOR:
                after=row['phase']=='after_write';sp=row['gp'][4]-16
                writes=[w for w in actual['writes'] if w['instruction']==SECOND_SELECTOR]
                if (actual['boundary_gp'][4]!=sp+(0 if after else 4) or len(writes)!=int(after) or
                    (after and (writes[0]['address']!=sp or writes[0]['written']!='3c3a2800' or
                                actual['stopped_at']!=0x283570))):
                    raise ValueError('Original second selector CALL interruption retirement differs')
                call_stops.append(dict(phase=row['phase'],stack_pointer=actual['boundary_gp'][4],committed_return=after))
            if decoder and pc==0x283a3c:
                if decoder.RETURN not in sites or actual['boundary_gp'][4]!=row['gp'][4]-12:
                    raise ValueError('Original parent continuation did not consume the decoder return word')
                parent_resumes.append(dict(pc=hex(pc),actual_gp=actual['boundary_gp']))
        else:
            pc=actual['stopped_at']
            required={SELECTOR,ENTRY,PARENT_TRANSFORM_CALL,TRANSFORM_ENTRY,0x283f1b,0x284076,
                      0x283ee4,0x283f76,*RETURNS[1:]}
            if row['has_ancestry_parent']:required.update((0x283ec7,RETURNS[0]))
            if not required<=sites or ((continued or not decoder) and {0x283a3c,0x283a3f}&sites):
                raise ValueError('Original did not reach the genuine packed transform return')
            expected_calls={SELECTOR:0x283947,PARENT_TRANSFORM_CALL:PARENT_TRANSFORM_JOIN,
                            0x283ee4:RETURNS[1],0x283f76:RETURNS[2]}
            if row['has_ancestry_parent']:expected_calls[0x283ec7]=RETURNS[0]
            if continued:
                if pc!=PARENT_TRANSFORM_JOIN or actual['boundary_gp'][0]&255!=1 or 0x283975 in sites:
                    raise ValueError('Original active transform return did not carry AL=1')
                returns.append(dict(pc=hex(pc),ancestry=row['has_ancestry_parent'],actual_gp=actual['boundary_gp']))
            else:
                if not {0x283975,SECOND_SELECTOR,FIELD,JUMP}<=sites:
                    raise ValueError(f'Actual format-four tail differs in {row["name"]}: {pc:#x}')
                if decoder:
                    if (pc!=STOP or not {TAIL,decoder.RETURN,0x283a3c,0x283a3f}<=sites or
                        actual['boundary_gp'][4]!=row['gp'][4]+44):
                        raise ValueError('Original AL=1 parent did not resume after the real decoder RET')
                    if not set(decoder.CALLS)<={w['instruction'] for w in actual['writes']}:
                        raise ValueError('Original AL=1 parent did not exercise all three real decoder children')
                    pc=0x283a3f
                elif pc!=TAIL or actual['boundary_gp'][4]!=row['gp'][4]-16:
                    raise ValueError(f'Actual format-four tail differs in {row["name"]}: {pc:#x}')
                expected_calls[SECOND_SELECTOR]=0x283a3c
                if [w['address'] for w in actual['writes'] if w['instruction']==SECOND_SELECTOR]!=[row['gp'][4]-16]:
                    raise ValueError('Original format-four tail lost its caller-owned return slot')
                if not any(w['instruction']==0x283975 and w['address']==0x510f30 and w['written']=='04000000'
                           for w in actual['writes']):
                    raise ValueError('Original active transform did not publish format four')
                if sum(w['instruction']==0x283fa7 for w in actual['writes'])!=8:
                    raise ValueError('Original active transform did not publish the complete eight-word result')
                if not all(w['address']==row['output_slot']+4*i for i,w in
                           enumerate(w for w in actual['writes'] if w['instruction']==0x283fa7)):
                    raise ValueError('Original active transform output slot changed')
                normal+=1
            for call,returned in expected_calls.items():
                if [w['written'] for w in actual['writes'] if w['instruction']==call]!=[returned.to_bytes(4,'little').hex()]:
                    raise ValueError(f'Original CALL {call:x} did not push its genuine return address')
        row['expected']=dict(entry=pc,gp=actual['boundary_gp'],eflags=actual['boundary_eflags'],xmm=actual['boundary_xmm'],
                             fp=actual['after_fp'],live_x87=actual.get('boundary_live_x87',[]))
        if 'active_mmx' in actual:row['expected']['active_mmx']=actual['active_mmx']
        row.update(reads=actual['reads'],writes=actual['writes'],calls=actual['calls'])
        for packet in row['calls']:
            if packet.pop('arguments'):raise ValueError('Unexpected format-four trace arguments')
        for bank,after in zip(row['banks'],actual['banks'],strict=True):
            if bank['address']!=after['address']:raise ValueError('Format-four fixture bank order changed')
            bank['after']=after['after']
    return normal,returns,call_stops,decoder_returns,parent_resumes,coverage


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    for option in ('smoke','prefixes','sanitizers','semantic-mutants','through-return'):
        parser.add_argument('--'+option,action='store_true')
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:parser.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    source_before={path.resolve():sha(path) for tree in ('tools','src','include','tests') for path in (ROOT/tree).rglob('*')
                   if path.is_file() and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    used={ROOT/TRANSFORM_FRAME,ROOT/SOURCE,Path(__file__).resolve(),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'}
    decoder=None
    if args.through_return:
        import validate_native_codec_format_four_decode_tu15 as decoder
        used.add(ROOT/RETURN_FRAME)
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[],
                original_xbe_sha256=NEW_PIN,actual_tail_entry=hex(TAIL),actual_tail_body_sha256=TAIL_PIN,
                parent_returns=0,format_four_decoder_executed=bool(decoder),opaque_input_qualified=False,
                gp_simd_policy_mutating_provider_qualified=False)
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Exact original English TU1.5 changed')
        image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
        entries,graphs,owners=original_context(code,decoder)
        used.update(ROOT/name for name in owners[-1])
        trace,stores=prepare_observer(code,entries,graphs,image,decoder)
        rows=inputs(image,args.smoke,bool(decoder));normal=len(rows);proofs=[]
        calls=set(CALLS)|set(decoder.CALLS if decoder else ())
        double_access=DOUBLE_ACCESS|calls
        def measure(batch,label):
            folder=scratch/label;folder.mkdir()
            result,proof=publication.physical_original_full(image,batch,folder,entries=entries,regions=CASES,
                                                            trace_calls=trace,trace_extended=True)
            proofs.append(proof)
            return result['cases']
        original=measure(rows,'actual')
        if args.prefixes:
            pending=[];seen=set()
            for row,actual in zip(rows,original,strict=True):
                mode=row['has_ancestry_parent']
                if ('return',mode) not in seen:
                    seen.add(('return',mode));prefix=copy.deepcopy(row)
                    prefix.update(name=row['name']+'-before-transform-return',phase='before_transform_continuation',
                                  before_transform_continuation=True,stop_before_entries=[PARENT_TRANSFORM_JOIN])
                    pending.append(prefix)
                if decoder:
                    for lane,pc in enumerate(decoder.RETURNS):
                        signature=('returned_child',pc)
                        if signature in seen or decoder.CALLS[lane] not in actual['executed_sites']:continue
                        seen.add(signature);prefix=copy.deepcopy(row)
                        prefix.update(name=row['name']+f'-returned-child-{pc:x}',phase='before_decoder_child_return',
                                      before_format_four_child_return=pc,stop_before_entries=[pc])
                        pending.append(prefix)
                for phase,stream in (('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(actual[stream]):
                        pc=event['instruction']
                        if pc<PARENT_TRANSFORM_CALL or (phase=='before_write' and pc in double_access):continue
                        signature=(phase,pc)
                        if signature in seen:continue
                        seen.add(signature);prefix=copy.deepcopy(row)
                        prefix.update(name=row['name']+f'-{phase}-{pc:x}',phase=phase,fault=dict(phase=phase,ordinal=ordinal))
                        pending.append(prefix)
            for i,row in enumerate(pending,normal):row['id']=i
            rows+=pending;original+=measure(pending,'prefixes')
        tails,returns,call_stops,decoder_returns,parent_resumes,coverage=qualify_paths(rows,original,decoder)
        if tails!=normal or (args.prefixes and ({r['ancestry'] for r in returns}!={False,True} or
                                                {r['committed_return'] for r in call_stops}!={False,True})):
            raise ValueError('Actual format-four entry/transform-return coverage changed')
        if decoder and args.prefixes and ({int(r['pc'],16) for r in decoder_returns}!=set(decoder.RETURNS) or
                                           len(parent_resumes)!=1):
            raise ValueError('Actual format-four returned-child coverage changed')
        secondary_sites={pc for pc in graphs[PARENT].code if 0x283947<=pc<=SECOND_SELECTOR}
        if secondary_sites-coverage:raise ValueError('Missing actual nonreturning format-four continuation sites')
        report.update(physical_paths=len(rows),tail_stops=0 if decoder else tails,parent_returns=tails if decoder else 0,
                      interruption_prefixes=len(rows)-tails,
                      original_transform_return_continuations=returns,observed_parent_sites=len(coverage&set(graphs[PARENT].code)),
                      original_decoder_return_continuations=decoder_returns,
                      original_parent_resume_after_decoder=parent_resumes,
                      second_selector_call_interruptions=call_stops,observed_nonreturning_secondary_sites=len(secondary_sites),
                      observed_transform_sites=len(coverage&set(graphs[TRANSFORM_ENTRY].code)),original=proofs)
        if decoder:
            repeated=sum(w['instruction']==decoder.TRANSLATION[-1] for row in rows[:normal] for w in row['writes'])
            if not args.smoke and not repeated:raise ValueError('Original composed translation publication was not exercised')
            report.update(observed_decoder_sites=len(coverage&set(graphs[TAIL].code)),repeated_translation_groups=repeated)
        cpp=comparison(scratch,decoder)
        fixture=scratch/'format-four.json';negative=scratch/'format-four-mutants.json'
        fixture.write_text(json.dumps(dict(cases=rows,write_sites=stores),separators=(',',':')))
        negative.write_text(json.dumps(dict(cases=rows[:2],write_sites=stores),separators=(',',':')))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                     '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(cpp),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            def run(build,probe,case_path):
                result=subprocess.run(build,capture_output=True,text=True,timeout=480)
                if result.returncode:raise ValueError('Format-four compile failed: '+result.stderr[-12000:])
                return subprocess.run([str(probe),str(case_path)],capture_output=True,text=True,timeout=480)
            done=run(command,binary,fixture)
            if done.returncode:raise ValueError('Format-four original comparison failed: '+(done.stdout+done.stderr)[-12000:])
            result=dict(compiler=compiler,result=json.loads(done.stdout),compiled_defects=[]);report['compilers'].append(result)
            print(f'{compiler} original format-four comparison passed',flush=True)
            for dependency in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(dependency).resolve()
                if path in source_before:used.add(path)
            if args.semantic_mutants:
                mutations=((RETURN_FRAME,'skip-parent-epilogue','parent.finish_parent();','(void)parent;'),
                           (RETURN_FRAME,'skip-real-format-four-return',
                            'scope.child([&]{native_codec_format_four_decode_detail::run_private(state,std::ref(before));});',
                            '(void)scope;')) if decoder else tuple((TRANSFORM_FRAME,*entry) for entry in DEFECTS)
                for subject,name,old,new in mutations:
                    original_header=(ROOT/subject).read_text()
                    if original_header.count(old)!=1:raise ValueError('Exact format-four mutant seam changed: '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);header=overlay/subject
                    header.parent.mkdir(parents=True);header.write_text(original_header.replace(old,new,1));probe=overlay/'probe'
                    build=[command[0],'-I'+str(overlay/'src'),*command[1:]]
                    build[build.index('-o')+1]=str(probe);build[build.index('-MF')+1]=str(overlay/'probe.d')
                    done=run(build,probe,negative);diagnostic=done.stdout+done.stderr
                    if not done.returncode or 'codec-format-four-' not in diagnostic:
                        raise ValueError('Format-four defect survived: '+name+' '+diagnostic[-7000:])
                    result['compiled_defects'].append(dict(name=name,rejected=True,diagnostic=diagnostic[-7000:]))
                    print(f'{compiler} rejected format-four defect {name}',flush=True)
        if original_context(code,decoder)[2]!=owners:raise ValueError('Exact format-four source owners drifted')
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error);raise
    finally:
        import sys
        used.update(Path(module.__file__).resolve() for module in tuple(sys.modules.values())
                    if getattr(module,'__file__',None) and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        report['source_before']={str(path.relative_to(ROOT)):source_before[path] for path in sorted(used)}
        report['source_after']={name:sha(ROOT/name) for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
