#!/usr/bin/env python3
"""Compare whole physical 30E930 with its exact retained World transport."""
import argparse
import copy
import hashlib
import json
import os
import re
import subprocess
from pathlib import Path

from generated_routine_ownership import function_bodies
import validate_native_simplex_hull_source_normal_tu15 as hull_source
import validate_native_simplex_owner_frame_tu15 as owner
import validate_native_simplex_owner_support_geometry_tu15 as support_geometry

TRANSPORTS={
    0x30e96d:('if(!handoff_native_contact_features(s,target,0x30e96du))dispatch(s,target,0x30ae7du,false)',
              'call(s,target,0x30e96du)'),
    0x30e985:('if(!handoff_native_contact_features(s,target,0x30e985u))dispatch(s,target,0x30ae95u,false)',
              'call(s,target,0x30e985u)'),
    0x30ea7b:('run_309d40(s)','call(s,0x30d830u,0x30ea7bu)'),
    0x30eacc:('if(!handoff_native_simplex_helpers(s,0x312620u,0x30eaccu))run_30eb30(s)','call(s,0x312620u,0x30eaccu)'),
    0x30eb3f:('run_309ae0(s)','call(s,0x30d5d0u,0x30eb3fu)'),
    0x30eb85:('if(!handoff_native_simplex_helpers(s,0x30d3f0u,0x30eb85u))run_309900(s)','call(s,0x30d3f0u,0x30eb85u)'),
    0x30ecb0:('run_309ae0(s)','call(s,0x30d5d0u,0x30ecb0u)'),
    0x30ece2:('if(!handoff_native_simplex_helpers(s,0x30d3f0u,0x30ece2u))run_309900(s)','call(s,0x30d3f0u,0x30ece2u)'),
    0x30eeae:('dispatch(s,target,0x30b3beu,false)','call(s,target,0x30eeaeu)'),
    0x30f312:('dispatch(s,target,0x30b822u,false)','call(s,target,0x30f312u)'),
    0x30f48e:('dispatch(s,target,0x30b99eu,false)','call(s,target,0x30f48eu)'),
    0x30f59a:('run_30a370(s)','call(s,0x30de60u,0x30f59au)'),
}
PINS={**hull_source.PINS,'source_estimate':'7c93aac13aa7930e203cbe9cd111ec6a112037e49cf659f8fd6781ba20792ec2'}


def reference(scratch):
    world=owner.seed.SOURCE.read_text()
    body=next(body for sig,body in function_bodies(world)if sig.symbol=='30ae40')
    if hashlib.sha256(body.encode()).hexdigest()!=owner.fallback.PARENT_PIN:
        raise ValueError('Complete actual simplex source body drift')
    helpers=[]
    for name,pin in PINS.items():
        suffix=r'[^\n]*\n[\s\S]*?^}' if name in ('sign_extend','source_estimate') else r'[^\n]+'
        text=re.search(r'^std::uint32_t '+name+suffix,world,re.M).group()
        if hashlib.sha256(text.encode()).hexdigest()!=pin:raise ValueError('Complete simplex source '+name+' drift')
        helpers.append(text.replace('ObservedWorldState &s','WorldState &s'))
    sites=[];seen=set();lines=[]
    for line in body.splitlines():
        pc=re.search(r'services.instruction\(world_observation_pc\(0x[0-9a-f]+u,0x([0-9a-f]+)u',line)
        if pc:
            at=int(pc[1],16)
            if at in TRANSPORTS:
                old,new=TRANSPORTS[at]
                if line.count(old)!=1 or at in seen:raise ValueError('Exact simplex source call transport drift')
                line=line.replace(old,new,1);seen.add(at)
            if at==0x30f612 or re.search(r'\b(?:r|v|w|vw)\(s.memory|s.push\(|s.pop\(',line):sites.append(at)
        lines.append(line)
    if seen!=set(TRANSPORTS):raise ValueError('Incomplete simplex source child call transport')
    source='namespace retained {\nusing namespace halo2::physics_world_runtime_detail;\n'
    source+='\n'.join(helpers)+'\ninline std::uint32_t world_observation_pc(std::uint32_t,std::uint32_t actual,WorldState&){return actual;}\n'
    source+='\n'.join(lines).replace('static void run_30ae40(ObservedWorldState &s) {',
            'template<class Call>static void original_simplex(WorldState& s,Call call) {',1)
    source+='\nconstexpr bool io(std::uint32_t pc){switch(pc){'+''.join(f'case 0x{p:x}u:'for p in sites)
    source+='return true;default:return false;}}\n}\n'
    path=scratch/'simplex_owner_retained.inc';path.write_text(source);return path


def driver(scratch,retained_projection,retained_owner,retained_hull_child):
    path=owner.driver(scratch);text=path.read_text()
    def replace(old,new):
        nonlocal text
        if text.count(old)!=1:raise ValueError('Private simplex retained proof overlay anchor drift')
        text=text.replace(old,new,1)
    hull_include='#include "simplex_owner_nested_hull.inc"\n' if retained_hull_child else ''
    replace('namespace {\nconstexpr bool parent',hull_include+'#include "simplex_owner_retained.inc"\n\nnamespace {\nconstexpr bool parent')
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
    replace('services.reciprocal_sqrt=[](std::uint32_t bits){','''services.reciprocal_sqrt=[&](std::uint32_t bits){
        if(m.kind==4u&&m.pc==m.site)throw Boundary{};''')
    candidate='            const auto value=native_simplex_owner_detail::run_private_simplex_owner(state,simplex_child,before);\n'
    if retained_projection:
        candidate='''            using Parent=native_simplex_owner_detail::Frame<WorldState,decltype(simplex_child),decltype(before)>;
            struct Projector:Parent {
                WorldState& s;decltype(before)& observe;
                Projector(WorldState& state,decltype(simplex_child)& child,decltype(before)& observer)
                    :Parent(state,child,observer),s(state),observe(observer){}
                auto project(){return native_simplex_projection_detail::run_private_retained_stage(s,observe);}
            } frame{state,simplex_child,before};
            const auto value=native_tu15_simplex_owner_kernel(frame);
'''
    if retained_owner:
        header='#include "physics/runtime/native_tu15_simplex_owner_frame.hpp"'
        replace(header,header+'\n#include "physics/runtime/native_tu15_simplex_owner_retained_frame.hpp"')
        candidate='            const auto value=native_simplex_owner_retained_detail::run_private_simplex_retained(state,simplex_child,before);\n'
    if retained_hull_child:
        replace('native_simplex_hull_owner_detail::run_private_hull_owner(next,hull_child,before);return;', '''if(original)retained_hull::original_hull(next,hull_child);
            else native_simplex_hull_retained_detail::run_private_hull_retained(next,hull_child,before);
            return;''')
    predicate='retained::io(pc)'+('||retained_hull::io(pc)'if retained_hull_child else '')
    replace('    const auto value=native_simplex_owner_detail::run_private_simplex_owner(state,simplex_child,before);\n'
            '    check(value==state.gp[0],"Full simplex result lost returned EAX");', '''    try{
        if(original){
            services.instruction=[&](std::uint32_t pc){m.pc=pc;++callbacks;if('''+predicate+''')before(state,pc);};
            retained::original_simplex(state,simplex_child);
        }else {
'''+candidate+'''            check(value==state.gp[0],"Full simplex result lost returned EAX");
        }
    }catch(const Boundary&){stopped=true;}
    check(stopped==(m.kind!=0u),"Expected retained simplex fault site did not execute");''')
    replace('check(callbacks==0u,','check((callbacks!=0u)==original,')
    replace('return J{{"abi",packet(state,static_cast<std::uint32_t>(exit))}',
            'return J{{"abi",packet(state,stopped?m.pc:static_cast<std::uint32_t>(exit))},{"stopped",stopped}')
    replace('result.push_back(run(row));','result.push_back(J{{"native",run(row,false)},{"retained",run(row,true)}});')
    path.write_text(text);return path


def failure_rows(image,rows):
    if len(rows)!=60:raise ValueError('Use the complete normal corpus for composed retained fault controls')
    support=support_geometry.inputs(image,True)[0];out=[]
    def append(row,site,kind,ordinal=0):
        copy_row=copy.deepcopy(row)
        copy_row.update(name=f'fault-{site:x}-{kind}-{ordinal}',fail_site=site,fail_kind=kind,fail_ordinal=ordinal)
        out.append(copy_row)
    for site,row in ((0x30e930,rows[0]),(0x30e96d,rows[0]),(0x30ea13,rows[0]),(0x30ee7a,rows[0]),
                     (0x30eeae,rows[0]),(0x30f312,support),(0x30f59a,rows[4]),(0x30de60,rows[4]),
                     (0x30e013,rows[4])):
        for kind in (1,2,3):append(row,site,kind)
    for site,row in ((0x30e96d,rows[0]),(0x30eeae,rows[0]),(0x30f312,support)):
        append(row,site,1,1)
    for site,row in ((0x30eec0,rows[0]),(0x30f381,support),(0x30f6dd,rows[0])):
        append(row,site,1)
    for half in (0,1):
        for kind in (1,2,3):append(rows[0],0x30f65e,kind,half)
    append(rows[0],0x30f612,4)
    return out


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True);parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--smoke',action='store_true')
    mode=parser.add_mutually_exclusive_group()
    mode.add_argument('--retained-projection',action='store_true');mode.add_argument('--retained-owner',action='store_true')
    parser.add_argument('--retained-hull-child',action='store_true')
    parser.add_argument('--faults',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    if args.retained_hull_child and not args.retained_owner:parser.error('--retained-hull-child requires --retained-owner')
    if args.faults and (not args.retained_hull_child or args.smoke):parser.error('--faults requires the full retained composition')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);root=owner.seed.ROOT
    paths={Path(__file__).resolve(),Path(owner.__file__).resolve(),Path(hull_source.__file__).resolve(),
           Path(support_geometry.__file__).resolve(),owner.FRAME,
           owner.seed.SOURCE,owner.seed.BASE_DRIVER,*owner.DEPS,*owner.hull.DEPS};pending=list(paths)
    if args.retained_owner:
        retained=owner.FRAME.with_name('native_tu15_simplex_owner_retained_frame.hpp');paths.add(retained);pending.append(retained)
    while pending:
        path=pending.pop();source=path.read_text()
        for name in re.findall(r'^#include "([^"\n]+)"',source,re.M):
            dependency=next((p for p in (path.parent/name,root/'include'/name,root/'src'/name)if p.is_file()),None)
            if dependency is not None and dependency not in paths:paths.add(dependency);pending.append(dependency)
    sha=owner.seed.sha;report=dict(passed=False,scratch_owner=str(scratch),production_bound=False,retained_large_artifacts=[],
                                   input_hashes={str(p.relative_to(root)):sha(p)for p in sorted(paths)})
    try:
        if sha(args.xbe)!=owner.seed.PIN:raise ValueError('Actual TU1.5 reference image drift')
        report['reference_sha256']=sha(reference(scratch))
        if args.retained_hull_child:
            nested=hull_source.reference(scratch,'actual-world').read_text()
            if nested.count('namespace retained {')!=1:raise ValueError('Exact nested retained namespace drift')
            child=scratch/'simplex_owner_nested_hull.inc'
            child.write_text(nested.replace('namespace retained {','namespace retained_hull {',1))
            report['nested_reference_sha256']=sha(child)
        source=driver(scratch,args.retained_projection,args.retained_owner,args.retained_hull_child)
        image=owner.seed.Xbe(args.xbe);rows=owner.inputs(image,args.smoke);normal_cases=len(rows)
        if args.faults:rows+=failure_rows(image,rows)
        binary=scratch/'simplex-retained'
        command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-Wno-ignored-attributes','-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(root/'tests'),str(source),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Simplex retained proof compile failed: '+built.stderr[-8000:])
        result=subprocess.run([str(binary)],input=json.dumps(rows),capture_output=True,text=True,timeout=90)
        if result.returncode:raise ValueError('Simplex retained proof stopped: '+result.stderr[-6000:])
        differences=[];matches=0;nested_returns=0
        counts={mode:{key:0 for key in ('packets','children','reads','writes')}for mode in ('normal','failure')}
        for row,pair in zip(rows,json.loads(result.stdout),strict=True):
            actual,expected=pair['native'],pair['retained']
            mode='failure' if 'fail_kind' in row else 'normal'
            for key in counts[mode]:counts[mode][key]+=len(expected[key])
            if mode=='normal':
                nested_returns+=sum(child['entry']==0x30de60 for child in expected['children'])
            for key in ('packets','children','abi','reads','writes','banks','stopped'):
                if expected[key]==actual[key]:continue
                a,b=actual[key],expected[key]
                if isinstance(a,list) and isinstance(b,list):
                    n=next((n for n,(x,y) in enumerate(zip(a,b))if x!=y),min(len(a),len(b)))
                    a,b=a[n:n+1],b[n:n+1]
                    if key=='banks':a,b=[dict(address=v['address'],sha256=hashlib.sha256(bytes.fromhex(v['after'])).hexdigest())for v in a],[
                                          dict(address=v['address'],sha256=hashlib.sha256(bytes.fromhex(v['after'])).hexdigest())for v in b]
                differences.append(dict(case=row['name'],field=key,native=a,retained=b));break
            else:matches+=1
        report.update(cases=len(rows),normal_cases=normal_cases,fault_cases=len(rows)-normal_cases,matches=matches,
                      difference_count=len(differences),first_difference=differences[:1],counts=counts,
                      real_nested_hull_returns=nested_returns,nested_retained_hull=args.retained_hull_child,
                      passed=not differences,physical_reference=False,retained_projection=args.retained_projection,retained_owner=args.retained_owner,
                      source_scope='Exact complete World 30E930 and optional nested 30DE60 bodies, actual observation PCs, 12 outer and 8 nested genuine dispatch seams; no production admission.')
        if args.faults and nested_returns!=10:raise ValueError('Expected ten genuine normal nested hull returns')
    except Exception as error:report.update(passed=False,error=str(error))
    report['sources_unchanged']=all(sha(root/path)==value for path,value in report['input_hashes'].items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private retained simplex input drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ('passed','error','cases','matches','difference_count')if k in report}))
    return 0 if report['passed']else 1


if __name__=='__main__':
    raise SystemExit(main())
