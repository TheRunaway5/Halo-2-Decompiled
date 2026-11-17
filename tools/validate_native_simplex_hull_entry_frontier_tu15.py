#!/usr/bin/env python3
"""Qualify only the first retained 30DE60 PUSH and its actual 30DE61 suffix."""
import argparse
import copy
import json
import os
import re
import subprocess
from pathlib import Path

import validate_native_simplex_hull_source_normal_tu15 as source

HEADER='physics/runtime/native_tu15_simplex_hull_entry_frontier.hpp'
SUFFIX=0x30de61


def exact_replace(text,old,new):
    if text.count(old)!=1:raise ValueError('Exact retained hull frontier anchor drift: '+old[:90])
    return text.replace(old,new,1)


def reference(scratch,defect):
    path=source.reference(scratch,'actual-world');text=path.read_text()
    signature='template<class Call>static void original_hull(WorldState& s,Call call) {'
    push='if(s.services.instruction){s.services.instruction(world_observation_pc(0x30a370u,0x30de60u,s));} s.push(world_observation_pc(0x30a370u,0x30de60u,s),s.reg(5,4,0));'
    mov='if(s.services.instruction){s.services.instruction(world_observation_pc(0x30a371u,0x30de61u,s));} s.set(5,s.reg(4,4,0),4,0);'
    label='native_first_push_retired:'
    setup=signature[:-3]+',bool after_first_push=false) {\nif(after_first_push)goto native_first_push_retired;'
    if defect=='repeat-push':
        start=setup+'\n'+label+'\n'+push+'\n'+mov
    else:
        start=setup+'\n'+push+'\n'+label+'\n'+mov
    path.write_text(exact_replace(text,signature+'\n'+push+'\n'+mov,start))
    return path


def driver(scratch,defect):
    path=source.driver(scratch,True,'actual-world');text=path.read_text()
    def replace(old,new):
        nonlocal text
        text=exact_replace(text,old,new)
    replace('#include "simplex_hull_retained.inc"','#include "'+HEADER+'"\n#include "simplex_hull_retained.inc"')
    replace('mutable unsigned selected_reads{};unsigned selected_writes{};',
            '''mutable unsigned selected_reads{};unsigned selected_writes{},first_push_writes{};
        struct ReplayedPush {};
        std::function<void(unsigned)> frontier;''')
    replace('Memory::read(address,out);','''Memory::read(address,out);
            if(pc==0x30de60u&&frontier)frontier(1u);''')
    replace('Memory::write(address,data);','''if(pc==0x30de60u&&++first_push_writes!=1u)throw ReplayedPush{};
            Memory::write(address,data);
            if(pc==0x30de60u&&frontier)frontier(2u);''')
    replace('if(m.kind==3u&&e.instruction==m.site&&selected_observers++==m.ordinal)throw Boundary{};',
            '''if(m.kind==3u&&e.instruction==m.site&&selected_observers++==m.ordinal)throw Boundary{};
        if(e.instruction==0x30de60u&&m.frontier)m.frontier(3u);''')
    branch='''    try{
        if(original){
            services.instruction=[&](std::uint32_t pc){m.pc=pc;++callbacks;if(retained::io(pc))before(state,pc);};
            retained::original_hull(state,hull_child);
        }else native_simplex_hull_retained_detail::run_private_hull_retained(state,hull_child,before);
    }catch(const Boundary&){stopped=true;}'''
    candidate='''    using namespace native_simplex_hull_entry_frontier_detail;
    const unsigned provider_phase=row.value("policy_phase",0u);
    const bool provider_throws=row.value("policy_throw",false),decline=row.value("entry_decline",false);
    unsigned changes{},pure_hits{},retirements{};bool replayed{};
    Retirement result=Retirement::declined;
    const auto before_retained=[&](std::uint32_t pc){m.pc=pc;++callbacks;if(retained::io(pc))before(state,pc);};
    services.instruction=before_retained;
    const auto*const entry_type=&services.instruction.target_type();
    const auto policy=[&](const auto&s){
        return !decline&&s.services.instruction&&s.services.instruction.target_type()==*entry_type;
    };
    m.frontier=[&](unsigned phase){
        if(phase!=provider_phase||changes)return;
        ++changes;
        services.instruction=[&](std::uint32_t pc){
            before_retained(pc);
            if(pc==next_site){++pure_hits;state.gp[6]^=1u;}
        };
        if(provider_throws)throw Boundary{};
    };
    try{
        if(original)retained::original_hull(state,hull_child);
        else {
            result=first_push(state,before,policy);
            if(result!=Retirement::declined)++retirements;
            retained::original_hull(state,hull_child,retirements!=0u);
        }
    }catch(const Boundary&){stopped=true;}
    catch(const FailMemory::ReplayedPush&){stopped=true;replayed=true;}'''
    if defect=='drop-revocation':
        candidate=exact_replace(candidate,'result=first_push(state,before,policy);',
           'result=first_push(state,before,policy);if(result==Retirement::revoked)result=Retirement::retained;')
    replace(branch,candidate)
    replace('check(stopped==(m.kind!=0u),"Expected retained I/O failure site did not execute");',
            'check(stopped==(m.kind!=0u||provider_throws||replayed),"Expected first PUSH throw did not execute");')
    replace('check((callbacks!=0u)==original,','check((callbacks!=0u)==(original||decline||!stopped||replayed),')
    replace('check(events==m.writes,"Native hull search iteration observer store differs");',
            '''if(provider_phase==2u&&provider_throws){
        check(events.empty()&&m.writes.size()==1u&&m.writes[0].at("instruction")==0x30de60u,
              "Throwing first PUSH provider did not commit its single write before observer suppression");
    }else check(events==m.writes,"Native hull search iteration observer store differs");''')
    replace('{"stopped",stopped}','''{"stopped",stopped},{"policy_changes",changes},{"new_callback_pure_hits",pure_hits},
        {"entry_retirements",retirements},{"entry_result",static_cast<unsigned>(result)},{"provider_writes",m.writes},
        {"replayed_push",replayed}''')
    path.write_text(text);return path


def inputs(image):
    out=[]
    for base in source.hull.inputs(image,True)[:4]:
        for phase,throw,decline in [(0,False,False),(0,False,True),
                                   *((n,f,False)for n in (1,2,3)for f in (False,True))]:
            row=copy.deepcopy(base)
            row.update(name=f'{base["name"]}-phase{phase}-throw{int(throw)}-decline{int(decline)}',
                       policy_phase=phase,policy_throw=throw,entry_decline=decline)
            out.append(row)
    return out


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--defect',choices=['repeat-push','drop-revocation'])
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);seed=source.seed;root=seed.ROOT
    paths={Path(__file__).resolve(),Path(source.__file__).resolve(),Path(source.hull.__file__).resolve(),
           seed.SOURCE,seed.BASE_DRIVER,root/'src'/HEADER,*source.hull.DEPS}
    pending=list(paths)
    while pending:
        path=pending.pop()
        for name in re.findall(r'^#include "([^"\n]+)"',path.read_text(),re.M):
            dependency=next((p for p in (path.parent/name,root/'include'/name,root/'src'/name)if p.is_file()),None)
            if dependency is not None and dependency not in paths:paths.add(dependency);pending.append(dependency)
    hashes={str(path.relative_to(root)):seed.sha(path)for path in sorted(paths)}
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),defect=a.defect,
                input_sha256=hashes,retained_large_artifacts=[])
    try:
        if seed.sha(a.xbe)!=seed.PIN:raise ValueError('Actual TU1.5 reference image drift')
        report['reference_sha256']=seed.sha(reference(scratch,a.defect))
        unit=driver(scratch,a.defect);report['instrumented_driver_sha256']=seed.sha(unit)
        rows=inputs(seed.Xbe(a.xbe));binary=scratch/'simplex-hull-frontier'
        command=[a.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-Wno-ignored-attributes','-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(root/'tests'),str(unit),'-o',str(binary)]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Exact hull frontier compile failed: '+built.stderr[-7000:])
        result=subprocess.run([str(binary)],input=json.dumps(rows),capture_output=True,text=True,timeout=90)
        if result.returncode:raise ValueError('Exact hull frontier execution stopped: '+result.stderr[-5000:])
        differences=[];counts=dict(retained=0,revoked=0,throws=0,declined=0,pre_io_packets=0,child_entries=0)
        for row,pair in zip(rows,json.loads(result.stdout),strict=True):
            candidate,expected_source=pair['native'],pair['retained']
            compare=('replayed_push','packets','children','abi','reads','writes','provider_writes','banks','stopped','policy_changes','new_callback_pure_hits')
            mismatch=next((k for k in compare if candidate[k]!=expected_source[k]),None)
            stopped,declined,changed=row['policy_throw'],row['entry_decline'],bool(row['policy_phase'])
            expected=(0,0)if stopped or declined else (1,2 if changed else 1)
            if mismatch is None and (candidate['entry_retirements'],candidate['entry_result'])!=expected:mismatch='entry retirement decision'
            if mismatch is None and (candidate['new_callback_pure_hits']!=int(changed and not stopped)or
                                     candidate['policy_changes']!=int(changed)):mismatch='genuine new callback witness'
            if mismatch:
                differences.append(dict(case=row['name'],field=mismatch,expected_entry=list(expected),
                                        actual_entry=[candidate['entry_retirements'],candidate['entry_result']]))
            else:counts['throws'if stopped else 'declined'if declined else 'revoked'if changed else 'retained']+=1
            counts['pre_io_packets']+=len(expected_source['packets']);counts['child_entries']+=len(expected_source['children'])
        report.update(cases=len(rows),matches=len(rows)-len(differences),first_difference=differences[:1],counts=counts,
                      first_push_pc=0x30de60,exact_suffix_pc=SUFFIX)
        defect_field={'repeat-push':'replayed_push','drop-revocation':'entry retirement decision'}
        report['passed']=(bool(differences)and differences[0]['field']==defect_field[a.defect])if a.defect else not differences
        if not report['passed']:raise ValueError('Exact hull first-frontier behavior or compiled defect control failed')
    except Exception as error:report.update(passed=False,error=str(error))
    report['sources_unchanged']=hashes=={str(path.relative_to(root)):seed.sha(path)for path in paths}
    if not report['sources_unchanged']:report.update(passed=False,error='Private hull frontier input drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ('passed','error','cases','matches','first_difference')if k in report}))
    return 0 if report['passed']else 1


if __name__=='__main__':
    raise SystemExit(main())
