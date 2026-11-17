#!/usr/bin/env python3
"""Compare the private hull controller to the exact backed World source body."""
import argparse
import copy
import hashlib
import json
import os
import re
import subprocess
from pathlib import Path

from generated_routine_ownership import function_bodies
import validate_native_simplex_hull_owner_frame_tu15 as hull
import validate_native_simplex_hull_seed_frame_tu15 as seed

TRANSPORTS={
    0x30e013:('dispatch(s,target,0x30a523u,false)','call(s,target,0x30e013u)'),
    0x30e230:('dispatch(s,target,0x30a740u,false)','call(s,target,0x30e230u)'),
    0x30e29c:('run_309d40(s)','call(s,0x30d830u,0x30e29cu)'),
    0x30e2fd:('run_30eb30(s)','call(s,0x312620u,0x30e2fdu)'),
    0x30e370:('run_309ae0(s)','call(s,0x30d5d0u,0x30e370u)'),
    0x30e3b6:('if(!handoff_native_simplex_helpers(s,0x30d3f0u,0x30e3b6u))run_309900(s)','call(s,0x30d3f0u,0x30e3b6u)'),
    0x30e4e1:('run_309ae0(s)','call(s,0x30d5d0u,0x30e4e1u)'),
    0x30e513:('run_309900(s)','call(s,0x30d3f0u,0x30e513u)'),
}
PINS={
    'selected':'20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2',
    'canonical_branch':'e5779c8ed411532bc15334539a7814df2a584d81946b32d413bb5346d166e5c6',
    'sign_extend':'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
}
CONTACT_SOURCE=seed.SOURCE.with_name('physics_contact_runtime_generated.cpp')
CONTACT_BODY='9e3c2c64826c7587545882917bedb2b88c8300172532ed3b8b2048ecf2faa51f'
CONTACT_PINS={**PINS,
    'selected':'d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e',
    'canonical_branch':'4e328c43da59895ab6c3f14a24129e538924d7467cb173bd65cdc3373d2a703d',
}
DEFECTS={
    'atomic-push':('at(pc);fp_.io([&]{s_.push(observe_pc(pc),value);});',
                   'at(pc);const auto address=s_.gp[4]-4u;write(pc,address,4u,value);s_.gp[4]=address;'),
    'shift-auxiliary':('s_.gp[reg]=s_.flags.shift(s_.gp[reg],4u,4u);',
                       's_.gp[reg]=s_.flags.shift(s_.gp[reg],4u,4u);s_.flags.af=true;'),
}


def reference(scratch,reference_owner):
    body=seed.authored()
    if hashlib.sha256(body.encode()).hexdigest()!=seed.BODY_PIN:
        raise ValueError('Complete actual hull source body drift')
    paired_sites=set(re.findall(r'world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)',body))
    if len(paired_sites)!=725 or {int(b,16)-int(a,16)for a,b in paired_sites}!={0x3af0}:
        raise ValueError('Actual/canonical hull source site mapping drift')
    canonical=reference_owner!='actual-world'
    contact=reference_owner=='contact';world=(CONTACT_SOURCE if contact else seed.SOURCE).read_text();owners=[]
    if contact:
        normalized=re.sub(r'world_observation_pc\((0x[0-9a-f]+u),0x[0-9a-f]+u,s\)',r'\1',body)
        normalized=normalized.replace('ObservedWorldState &s','State &s',1)
        handoff='if(!handoff_native_simplex_helpers(s,0x30d3f0u,0x30e3b6u))run_309900(s)'
        if normalized.count(handoff)!=1:raise ValueError('Exact World/Contact handoff seam drift')
        body=next(body for sig,body in function_bodies(world)if sig.symbol=='30a370')
        if hashlib.sha256(body.encode()).hexdigest()!=CONTACT_BODY or body!=normalized.replace(handoff,'run_309900(s)',1):
            raise ValueError('Complete Contact hull body or exact World mapping drift')
    for name,pin in (CONTACT_PINS if contact else PINS).items():
        pattern=r'^std::uint32_t '+name+(r'[^\n]*\n[\s\S]*?^}' if name=='sign_extend' else r'[^\n]+')
        code=re.search(pattern,world,re.M).group()
        if hashlib.sha256(code.encode()).hexdigest()!=pin:raise ValueError('Complete hull source '+name+' drift')
        owners.append(code)
    sites=[];seen=set();lines=[]
    for line in body.splitlines():
        pc=re.search(r'services.instruction\(0x([0-9a-f]+)u\)'if contact else
                     r'services.instruction\(world_observation_pc\(0x[0-9a-f]+u,0x([0-9a-f]+)u',line)
        if pc:
            at=int(pc[1],16)+(0x3af0 if contact else 0)
            if at in TRANSPORTS:
                old,new=TRANSPORTS[at]
                if contact and at==0x30e3b6:old='run_309900(s)'
                if line.count(old)!=1 or at in seen:raise ValueError('Exact source call transport drift')
                line=line.replace(old,new,1);seen.add(at)
            if re.search(r'\b(?:r|v|w|vw)\(s.memory|s.push\(|s.pop\(',line):sites.append(at-(0x3af0 if canonical else 0))
        lines.append(line)
    if seen!=set(TRANSPORTS):raise ValueError('Incomplete source child call transport')
    source='namespace retained {\nusing namespace halo2::physics_world_runtime_detail;\n'
    source+='\n'.join(owners)
    source+='\ninline std::uint32_t world_observation_pc(std::uint32_t disc,std::uint32_t actual,WorldState&){return '+('disc'if canonical else 'actual')+';}\n'
    header='static void run_30a370('+('State'if contact else 'ObservedWorldState')+' &s) {'
    source+='\n'.join(lines).replace(header,
            'template<class Call>static void original_hull(WorldState& s,Call call) {',1)
    source+='\nconstexpr bool io(std::uint32_t pc){switch(pc){'+''.join(f'case 0x{p:x}u:'for p in sites)
    source+='return true;default:return false;}}\n}\n'
    path=scratch/'simplex_hull_retained.inc';path.write_text(source)
    return path


def driver(scratch,retained_view,reference_owner):
    path=hull.driver(scratch);text=path.read_text()
    def replace(old,new):
        nonlocal text
        if text.count(old)!=1:raise ValueError('Private hull retained proof overlay anchor drift')
        text=text.replace(old,new,1)
    replace('namespace {\nconstexpr bool parent','#include "simplex_hull_retained.inc"\n\nnamespace {\nconstexpr bool parent')
    canonical=reference_owner!='actual-world'
    if canonical:replace('(pc>=0x30de60u&&pc<=0x30e8d8u)','(pc>=0x30a370u&&pc<=0x30ade8u)')
    replace('J run(const J& row) {','J run(const J& row,bool original) {')
    replace('    Memory m;m.observer_preimages=true;', '''    struct FailMemory:Memory {
        std::uint32_t site{};unsigned kind{},ordinal{};mutable unsigned selected_reads{};unsigned selected_writes{};
        void read(std::uint32_t address,std::span<std::byte> out)const override{
            if(kind==1u&&pc==site&&selected_reads++==ordinal)throw Boundary{};
            Memory::read(address,out);
        }
        void write(std::uint32_t address,std::span<const std::byte> data)override{
            if(kind==2u&&pc==site&&selected_writes++==ordinal)throw Boundary{};
            Memory::write(address,data);
        }
    };
    FailMemory m;m.observer_preimages=true;
    m.site=row.value("fail_site",0u);m.kind=row.value("fail_kind",0u);m.ordinal=row.value("fail_ordinal",0u);
    unsigned selected_observers{};bool stopped{};''')
    replace('{"written",hex(e.written)}});','''{"written",hex(e.written)}});
        if(m.kind==3u&&e.instruction==m.site&&selected_observers++==m.ordinal)throw Boundary{};''')
    begin='    if(row.at("entry")==0x30de60u)native_simplex_hull_owner_detail::run_private_hull_owner(state,hull_child,before);'
    end='    m.banks.erase(0x30e908u);'
    at=text.index(begin);until=text.index(end,at)
    native=('native_simplex_hull_retained_detail::run_private_hull_retained'+('<false>'if canonical else '') if retained_view else
            'native_simplex_hull_owner_detail::run_private_hull_owner')
    text=text[:at]+'''    try{
        if(original){
            services.instruction=[&](std::uint32_t pc){m.pc=pc;++callbacks;if(retained::io(pc))before(state,pc);};
            retained::original_hull(state,hull_child);
        }else '''+native+'''(state,hull_child,before);
    }catch(const Boundary&){stopped=true;}
    check(stopped==(m.kind!=0u),"Expected retained I/O failure site did not execute");
'''+text[until:]
    if retained_view:
        header='#include "physics/runtime/native_tu15_simplex_hull_owner_frame.hpp"'
        replace(header,header+'\n#include "physics/runtime/native_tu15_simplex_hull_retained_frame.hpp"')
    replace('check(callbacks==0u,','check((callbacks!=0u)==original,')
    replace('return J{{"abi",packet(state,static_cast<std::uint32_t>(exit))}',
            'return J{{"abi",packet(state,stopped?m.pc:static_cast<std::uint32_t>(exit))},{"stopped",stopped}')
    replace('result.push_back(run(row));','result.push_back(J{{"native",run(row,false)},{"retained",run(row,true)}});')
    path.write_text(text);return path


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True);parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--retained-view',action='store_true')
    parser.add_argument('--faults',action='store_true')
    parser.add_argument('--include-normal',action='store_true')
    parser.add_argument('--defect',choices=tuple(DEFECTS))
    parser.add_argument('--reference-owner',choices=('actual-world','canonical-world','contact'),default='actual-world')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    if args.defect and (not args.retained_view or args.faults!=(args.defect=='atomic-push')):
        parser.error('Use the retained view, with provider faults only for the atomic-PUSH defect')
    if args.reference_owner!='actual-world' and (not args.retained_view or args.defect):
        parser.error('Canonical source checks require the retained view and no physical-PC defect controls')
    if args.include_normal and (not args.faults or args.defect):
        parser.error('The combined normal and fault corpus is only for baseline fault mode')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths={Path(__file__).resolve(),Path(hull.__file__).resolve(),Path(seed.__file__).resolve(),seed.BASE_DRIVER,
           seed.FRAME,seed.SOURCE,*hull.DEPS};pending=list(paths)
    retained_header=seed.FRAME.with_name('native_tu15_simplex_hull_retained_frame.hpp')
    if args.reference_owner=='contact':paths.add(CONTACT_SOURCE);pending.append(CONTACT_SOURCE)
    if args.retained_view:
        paths.add(retained_header);pending.append(retained_header)
    while pending:
        path=pending.pop();source=path.read_text()
        for name in re.findall(r'^#include "([^"\n]+)"',source,re.M):
            target=next((p for p in (path.parent/name,seed.ROOT/'include'/name,seed.ROOT/'src'/name)if p.is_file()),None)
            if target is not None and target not in paths:paths.add(target);pending.append(target)
    report['input_hashes']={str(path.relative_to(seed.ROOT)):seed.sha(path)for path in sorted(paths)}
    try:
        if seed.sha(args.xbe)!=seed.PIN:raise ValueError('Actual TU1.5 reference image drift')
        report['reference_sha256']=seed.sha(reference(scratch,args.reference_owner))
        source=driver(scratch,args.retained_view,args.reference_owner)
        image=seed.Xbe(args.xbe)
        rows=[row for row in hull.inputs(image,False)if row['entry']==seed.ENTRY]
        if args.faults:
            variants=[]
            for site,ordinals,source_row in ((0x30de60,(0,),rows[0]),(0x30de8f,(0,1),rows[0]),
                                            (0x30e013,(0,),rows[0]),(0x30e024,(0,1),rows[0])):
                for kind in (1,2,3):
                    for ordinal in ordinals:
                        row=copy.deepcopy(source_row);row.update(name=f'fault-{site:x}-{kind}-{ordinal}',fail_site=site,
                                                                  fail_kind=kind,fail_ordinal=ordinal)
                        variants.append(row)
            for site,source_row in ((0x30e1b9,rows[0]),(0x30e8d8,rows[0]),(0x30e0a0,rows[-1])):
                row=copy.deepcopy(source_row);row.update(name=f'fault-{site:x}-1-0',fail_site=site,fail_kind=1,fail_ordinal=0)
                variants.append(row)
            if args.reference_owner!='actual-world':
                for row in variants:row['fail_site']-=0x3af0
            rows=rows+variants if args.include_normal else variants
        binary=scratch/'hull-retained'
        command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-Wno-ignored-attributes','-I'+str(seed.ROOT/'include'),'-I'+str(seed.ROOT/'src'),
                 '-I'+str(seed.ROOT/'tests'),str(source),'-o',str(binary)]
        if args.defect:
            old,new=DEFECTS[args.defect];baseline=retained_header.read_text()
            if baseline.count(old)!=1:raise ValueError('Private retained view mutation seam drift')
            directory=scratch/'retained-mutation';overlay=directory/retained_header.relative_to(seed.ROOT/'src')
            overlay.parent.mkdir(parents=True);overlay.write_text(baseline.replace(old,new,1))
            command.insert(1,'-I'+str(directory));report['defect_overlay_sha256']=seed.sha(overlay)
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Hull retained proof compile failed: '+built.stderr[-8000:])
        result=subprocess.run([str(binary)],input=json.dumps(rows),capture_output=True,text=True,timeout=90)
        if result.returncode:raise ValueError('Hull retained proof stopped: '+result.stderr[-6000:])
        differences=[];matches=0
        for row,pair in zip(rows,json.loads(result.stdout),strict=True):
            actual,expected=pair['native'],pair['retained']
            for key in expected:
                if expected[key]==actual[key]:continue
                a,b=actual[key],expected[key]
                if isinstance(a,list) and isinstance(b,list):
                    n=next((n for n,(x,y) in enumerate(zip(a,b))if x!=y),min(len(a),len(b)))
                    a,b=a[n:n+1],b[n:n+1]
                    if key=='banks':a,b=[dict(address=v['address'],sha256=hashlib.sha256(bytes.fromhex(v['after'])).hexdigest())for v in a],[
                                          dict(address=v['address'],sha256=hashlib.sha256(bytes.fromhex(v['after'])).hexdigest())for v in b]
                differences.append(dict(case=row['name'],field=key,native=a,retained=b));break
            else:matches+=1
        rejected=False
        if args.defect=='atomic-push':
            rejected=any(d['case']=='fault-30de60-1-0' and d['field']=='abi' and
                         d['native']['gp'][4]==d['retained']['gp'][4]+4 for d in differences)
        if args.defect=='shift-auxiliary':
            rejected=any(d['field']=='packets' and d['native'] and d['retained'] and
                         d['native'][0]['entry']==d['retained'][0]['entry']==0x30e1b9 and
                         d['native'][0]['eflags']^d['retained'][0]['eflags']==0x10 for d in differences)
        report.update(cases=len(rows),matches=matches,difference_count=len(differences),first_difference=differences[:1],
                      passed=rejected if args.defect else not differences,compiled_defect=args.defect,defect_rejected=rejected,
                      physical_reference=False,retained_view=args.retained_view,declared_provider_faults=args.faults,
                      reference_owner=args.reference_owner,
                      normal_cases=sum('fail_site'not in row for row in rows),fault_cases=sum('fail_site'in row for row in rows),
                      source_scope='Exact complete source body and observation mode, eight genuine dispatch seams; declared failure prefixes are separate from normal calls. No policy mutation.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(seed.sha(seed.ROOT/path)==value for path,value in report['input_hashes'].items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private retained hull input drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ('passed','error','cases','matches','difference_count')if k in report}))
    return 0 if report['passed']else 1


if __name__=='__main__':
    raise SystemExit(main())
