#!/usr/bin/env python3
"""Authentic captured Lockout pair through initial native World construction.

Only before-input pool/shape/component/tag bytes and pinned XBE readonly data
are imported. World containers, broadphase pair and bounds are declared model
inputs. Stop at actual3210DE publication, before processing/force/frame parity.
The source BV constructor creates its capsule query child first and leaves its
process child null. This prefix alone cannot prove phantom-enter execution.
Compile one owned driver against a frozen official build; retain no raw banks.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ARCHIVE=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/completed-inputs-retirement-00-03/actual-failed-captures.tar.zst'
ARCHIVE_PIN='1249a58f0bc1ff1e70601c4dbdf29b9bb0cc070b46e97e600d1af242f6f33525'
MEMBER='iteration-00-lower-body1-phase0/original/required-failure-inputs/'


def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()


def driver(source,require_phantom=False):
    s=(source/'tests/helpers/native_world_convex_factory_prefix_tu15.hpp').read_text()
    s=s.replace('native_world_convex_factory_prefix','native_world_lift_factory_prefix')
    old='std::array<std::uint32_t,2>{0x1007abecu,0x1007de2cu}'
    assert s.count(old)==1;s=s.replace(old,'std::array<std::uint32_t,2>{0x1007bfccu,0x1007b64cu}')
    old='std::vector<std::uint32_t>calls;std::uint32_t published_agent{},published_manager{},published_record{};'
    assert s.count(old)==1;s=s.replace(old,old+'std::uint32_t phantom_constructors{},phantom_enters{};')
    old='m.put(payload,source_word(actual_payloads[n]));m.put(payload+8u,source_word(actual_payloads[n]+8u));m.put(payload+0x20u,body);'
    assert s.count(old)==1
    s=s.replace(old,old+'''
                m.put(payload+0x1cu,source_word(actual_payloads[n]+0x1cu));
                const auto source_body=source_word(actual_payloads[n]+0x20u);
                m.put(body+0x30u,source_word(source_body+0x30u));m.put(body+0x34u,source_word(source_body+0x34u));
''')
    start=s.index('            constexpr auto context=world+0xe800u,dispatch=')
    stop=s.index('pop=20;',start)+len('pop=20;')
    s=s[:start]+'''            const auto actual_world=source_word(actual_payloads[0]-12u+8u);
            m.put(world+0xccu,source_word(actual_world+0xccu));
            m.put(world+0xd0u,source_word(actual_world+0xd0u));
            pop=20;'''+s[stop:]
    # Required global roots stay current before-input values; no property or
    # membership result is injected. Their complete heap backing is imported.
    old='auto state=initial();'
    assert s.count(old)==1
    s=s.replace(old,'''for(auto p:{0x4ecbd0u,0x4f0414u,0x52be5cu})m.put(p,source_word(p));
    auto state=initial();''')
    old='    bool reached=false;'
    assert s.count(old)==1
    s=s.replace(old,'''    J factory_inputs=J::array();
    NativeTu15ContactAgentFactoryInputObserverBinding observe_factories(m,[&](const auto&,const auto&input){
      factory_inputs.push_back(J{{"owner",input.owner},{"first",input.first},{"second",input.second}});
    });
    bool reached=false;''')
    old='[&](const SourcePhysicsContactRecordStore&e){if(wake_prefix'
    assert s.count(old)==1
    s=s.replace(old,'''[&](const SourcePhysicsContactRecordStore&e){
          if(e.instruction==0x2f7849u)++phantom_constructors;
          if(e.instruction==0x2f795au||e.instruction==0x2f799bu)++phantom_enters;
          if(wake_prefix''')
    start=s.index('    if(published_agent)published_manager=')
    s=s[:start]+'''    check(reached&&published_agent,"Authentic lift pair did not publish a native agent");
    const auto list_data=m.get(published_agent+12u);
    J children=J::array();
    for(unsigned i=0;i<m.get(published_agent+16u);++i){const auto child=m.get(list_data+i*4u);
      const auto query=m.get(child+12u);children.push_back(J{{"agent",child},{"vtable",m.get(child)},
        {"query",query},{"query_vtable",m.get(query)},{"cached_process_agent",m.get(child+16u)}});}
    check(m.get(published_agent)==0x41cce8u&&children.size()==2,
      "Captured initial list agent differs");
    for(const auto&child:children)check(child.at("vtable")==0x41ca9cu&&
      child.at("query_vtable")==0x41c370u&&child.at("cached_process_agent")==0u,
      "Captured initial BV query/process ownership differs");
    if(REQUIRE_PHANTOM&&!(phantom_constructors>0&&phantom_enters>0)){
      std::cout<<J{{"published_agent",published_agent},{"vtable",m.get(published_agent)},
        {"size",m.get(published_agent+4u,2)},{"phantom_constructors",phantom_constructors},
        {"phantom_enters",phantom_enters},{"first_shape",m.get(payload0)},
        {"second_shape",m.get(payload1)},
        {"first_class_owner",m.get(m.get(m.get(payload0))+0x14u)},
        {"second_class_owner",m.get(m.get(m.get(payload1))+0x14u)},
        {"list_children",m.get(published_agent+16u)},
        {"list_data",m.get(published_agent+12u)},
        {"list_factory_calls",stats.native_list_agent_factory_calls},
        {"list_count_queries",stats.native_list_agent_count_queries},
        {"list_filter_queries",stats.native_list_agent_filter_queries},
        {"list_child_factories",stats.native_list_agent_child_factories},
        {"factory_inputs",factory_inputs},
        {"children",children},
        {"residual_targets",calls}}.dump()<<'\\n';
      throw std::runtime_error("Captured lift pair missed its native phantom factory/enter");
    }
    for(auto pc:{0x2f78c0u,0x2f7820u,0x1f34d0u,0x1da0f0u})
      check(std::find(calls.begin(),calls.end(),pc)==calls.end(),"Lift prefix used legacy phantom transport");
    std::cout<<J{{"passed",true},{"published_agent",published_agent},
      {"vtable",m.get(published_agent)},{"size",m.get(published_agent+4u,2)},
      {"native_phantom_constructors",phantom_constructors},{"native_phantom_enters",phantom_enters},
      {"initial_bv_chain_only",true},{"phantom_factory_live_proof",false},
      {"factory_inputs",factory_inputs},{"children",children},
      {"residual_targets",calls},{"source_payloads",J::array({0x1007bfccu,0x1007b64cu})},
      {"scope","Authentic input pair; declared World/broadphase/bounds; positive native publication prefix only"}}
      .dump()<<'\\n';return 0;
}
'''
    s=s.replace('REQUIRE_PHANTOM','true'if require_phantom else'false')
    return '#include "halo2/native_tu15_contact_agent_factory_input_observer.hpp"\n#define main frozen_world_model_main\n#include "'+str(source/'tests/native_tu15_world_residuals_tests.cpp')+'"\n#undef main\n'+s+'\nint main(int argc,char**argv)try{check(argc==2,"Require owned reference");return native_world_lift_factory_prefix(argv[1]);}catch(const std::exception&e){std::cerr<<e.what()<<"\\n";return 1;}\n'


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,required=True);p.add_argument('--build',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True)
    p.add_argument('--require-phantom',action='store_true',help='Diagnostic extra gate; initial BV publication alone is expected not to reach its cached process child')
    a=p.parse_args()
    a.source=a.source.resolve(strict=True);a.build=a.build.resolve(strict=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    report=dict(passed=False,source=str(a.source),build=str(a.build),owned_scratch=str(scratch),
                scope=__doc__,outcomes_supplied=False,retained_large_artifacts=[])
    try:
        assert sha(ARCHIVE)==ARCHIVE_PIN and sha(NEW_DEFAULT)==NEW_PIN
        raw=subprocess.check_output(['tar','--zstd','-xOf',str(ARCHIVE),MEMBER+'capture.json'])
        capture=json.loads(raw);regions=capture['regions']
        reference=scratch/'reference';reference.mkdir()
        names=list(dict.fromkeys(MEMBER+r['before_file']for r in regions))
        subprocess.run(['tar','--zstd','-xf',str(ARCHIVE),'-C',str(reference),*names],check=True)
        ref=reference/MEMBER
        normalized=dict(schema='native-tu15-pair-consumer-typed-before-v1',regions=regions)
        (ref/'capture-native-before.json').write_text(json.dumps(normalized))
        image=Xbe(NEW_DEFAULT)
        section=next(s for s in image.sections if s.name=='.rdata')
        selected=[dict(address=section.va,hex=image.section_bytes(section).hex())]
        (ref/'selected-readonly.json').write_text(json.dumps(dict(original_sha256=NEW_PIN,regions=selected)))
        cpp=scratch/'driver.cpp';cpp.write_text(driver(a.source,a.require_phantom));obj=scratch/'driver.o';exe=scratch/'native-lift-prefix'
        target=a.build/'CMakeFiles/native_tu15_world_residuals_tests.dir'
        flags={}
        for line in(target/'flags.make').read_text().splitlines():
            if ' = 'in line:
                k,v=line.split(' = ',1);flags[k]=shlex.split(v)
        compile_command=['/usr/bin/c++',*flags.get('CXX_DEFINES',[]),*flags.get('CXX_INCLUDES',[]),
                         *flags.get('CXX_FLAGS',[]),'-c',str(cpp),'-o',str(obj)]
        link=shlex.split((target/'link.txt').read_text())
        before_object='CMakeFiles/native_tu15_world_residuals_tests.dir/tests/native_tu15_world_residuals_tests.cpp.o'
        assert link.count(before_object)==1
        link=[str(obj)if word==before_object else word for word in link];link[link.index('-o')+1]=str(exe)
        link=[('-Wl,--dependency-file='+str(scratch/'link.d'))if word.startswith('-Wl,--dependency-file=')else word for word in link]
        libraries={word:sha(a.build/word)for word in link if word.endswith('.a')and(a.build/word).is_file()}
        report.update(original_sha256=NEW_PIN,archive_sha256=ARCHIVE_PIN,capture_sha256=hashlib.sha256(raw).hexdigest(),
                      before_regions=len(regions),driver_sha256=sha(cpp),helper_sha256=sha(__file__),libraries=libraries,
                      compile=compile_command,link=link)
        for label,command in [('compile',compile_command),('link',link),('prefix',[str(exe),str(ref)])]:
            r=subprocess.run(command,cwd=a.build,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            report[label+'_exit']=r.returncode
            report[label+'_diagnostic_tail']=r.stdout[-6000:]
            if r.returncode:raise RuntimeError(label+' failed: '+r.stdout[-1500:])
        report['result']=json.loads(report['prefix_diagnostic_tail'].splitlines()[-1])
        report['passed']=report['result']['passed'] and all(sha(a.build/name)==pin for name,pin in libraries.items())
    except BaseException as error:
        report['error']=str(error)or repr(error);raise
    finally:
        a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report.get('result',{})))
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
