#!/usr/bin/env python3
"""Reproduce a live provider policy change at the original hull's first pure site."""
import argparse
import json
import os
import subprocess
from pathlib import Path

import validate_native_simplex_hull_source_normal_tu15 as source


def driver(scratch):
    path=source.driver(scratch,True,'actual-world');text=path.read_text()
    def replace(old,new):
        nonlocal text
        if text.count(old)!=1:raise ValueError('Exact hull live-policy witness anchor drift')
        text=text.replace(old,new,1)
    replace('mutable unsigned selected_reads{};unsigned selected_writes{};',
            'mutable unsigned selected_reads{};unsigned selected_writes{};std::function<void()> after_first_push;')
    replace('Memory::write(address,data);','''Memory::write(address,data);
            if(pc==0x30de60u&&after_first_push)after_first_push();''')
    replace('    try{\n        if(original){','''    unsigned pure_hits{};
    m.after_first_push=[&]{
        auto previous=std::move(services.instruction);
        services.instruction=[&,previous=std::move(previous)](std::uint32_t pc){
            if(previous)previous(pc);
            if(pc==0x30de61u){++pure_hits;state.gp[6]^=1u;}
        };
    };
    try{
        if(original){''')
    replace('{"stopped",stopped}','{"stopped",stopped},{"new_callback_pure_hits",pure_hits}')
    path.write_text(text);return path


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);seed=source.seed;root=seed.ROOT
    retained=root/'src/physics/runtime/native_tu15_simplex_hull_retained_frame.hpp'
    pinned=(Path(__file__).resolve(),Path(source.__file__).resolve(),Path(source.hull.__file__).resolve(),
            seed.SOURCE,seed.BASE_DRIVER,retained)
    hashes={str(path.relative_to(root)):seed.sha(path)for path in pinned}
    report=dict(passed=False,policy_qualified=False,production_bound=False,scratch_owner=str(scratch),
                retained_large_artifacts=[],input_sha256=hashes)
    try:
        if seed.sha(a.xbe)!=seed.PIN:raise ValueError('Actual TU1.5 reference image drift')
        report['reference_sha256']=seed.sha(source.reference(scratch,'actual-world'))
        unit=driver(scratch);report['instrumented_driver_sha256']=seed.sha(unit)
        row=source.hull.inputs(seed.Xbe(a.xbe),True)[0];binary=scratch/'policy-witness'
        command=[a.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-Wno-ignored-attributes','-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(root/'tests'),str(unit),'-o',str(binary)]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Hull policy witness compile failed: '+built.stderr[-7000:])
        result=subprocess.run([str(binary)],input=json.dumps([row]),capture_output=True,text=True,timeout=50)
        if result.returncode:raise ValueError('Hull policy witness stopped: '+result.stderr[-5000:])
        pair=json.loads(result.stdout)[0];native,reference=pair['native'],pair['retained']
        report.update(publish_site='30DE60',first_pure_site='30DE61',native_callback_hits=native['new_callback_pure_hits'],
                      exact_source_callback_hits=reference['new_callback_pure_hits'],native_final_gp6=native['abi']['gp'][6],
                      exact_source_final_gp6=reference['abi']['gp'][6],normal_results=native['abi']['gp'][0]==reference['abi']['gp'][0])
        report['passed']=(not native['stopped']and not reference['stopped']and report['normal_results']and report['native_callback_hits']==0
                          and report['exact_source_callback_hits']==1
                          and report['native_final_gp6']^report['exact_source_final_gp6']==1)
        if not report['passed']:raise ValueError('Live provider change did not reproduce expected pure-site difference')
    except Exception as error:report.update(passed=False,error=str(error))
    report['sources_unchanged']=hashes=={str(path.relative_to(root)):seed.sha(path)for path in pinned}
    if not report['sources_unchanged']:report.update(passed=False,error='Live hull policy witness source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in ('passed','error','native_callback_hits','exact_source_callback_hits')if key in report}))
    return 0 if report['passed']else 1


if __name__=='__main__':
    raise SystemExit(main())
