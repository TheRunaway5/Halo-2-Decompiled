#!/usr/bin/env python3
"""Physical TU1.5 1F66D0 material lookup, numeric stores and typed Host values."""
import argparse,copy,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from build_native_tu15_constraint_before_reference import sha,save
STAGE=ROOT/'analysis/physics/native-contact-material-index-stage-20261103/staged'
UNIT='src/physics/native_tu15_contact_material_index.cpp'
HEADER='include/halo2/native_tu15_contact_material_index.hpp'
SHAPE=base.UI+0x100;SLOT=base.UI+0x1000;DEFINITION=base.UI+0x2000
SCENARIO=base.UI+0x3000;MATERIAL=base.UI+0x4000;DESTINATION=base.UI+0x5000

def cases():
    rows=[]
    for datum in (0xffffffff,0,2,0xffff0002,0x1234ffff,0x80000000):
        for part in (0,1,0xffff,0x8000,0x7fff,0x8001):
            signed=part if part<0x8000 else part-0x10000
            for value in (0,0xffff,0x8000,0x1234):
                words={SHAPE+0x18:part,SHAPE+0x1c:datum,SLOT:DEFINITION,
                  DEFINITION+0x5c:(MATERIAL-signed*20-0x10)&0xffffffff,
                  SCENARIO+0x10:(MATERIAL-signed*20-8)&0xffffffff,MATERIAL:value}
                rows.append(dict(name=f'material_{datum:x}_{part:x}_{value:x}',entry=0x1f66d0,receiver=0,
                  destination=DESTINATION,arguments=[],user_words=[(p-base.UI,v)for p,v in words.items()],
                  global_inputs=[[0x55d728,value,2],[0x4f0414,(SLOT-(datum&0xffff)*16-8)&0xffffffff,4],[0x4ecc18,SCENARIO,4]]))
    for datum,part in ((0xffffffff,0xffff),(0xffffffff,1),(2,1)):
        seed=next(r for r in rows if r['name']==f'material_{datum:x}_{part:x}_1234')
        for p in (SHAPE+0x18,SHAPE+0x1c,SLOT,DEFINITION+0x5c,SCENARIO+0x10,MATERIAL,0x55d728):
            row=copy.deepcopy(seed);row.update(name=f'alias_{datum:x}_{part:x}_{p:x}',destination=p);rows.append(row)
    return rows

def driver():
    prefix=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text().split('int main(int argc,char**argv)')[0]
    return '#include "halo2/native_tu15_contact_material_index.hpp"\n'+prefix+r'''
int main(int argc,char**argv)try{
  check(argc>=2,"Supply material fixture");const bool value_mode=argc==3,mutant=argc==4;
  const auto path=std::filesystem::path(argv[1]);std::ifstream input(path);const auto fixture=J::parse(input);
  unsigned profiles{},completed{},changed{};
  for(const auto&c:fixture.at("cases"))for(bool read_pass:{false,true}){
    if(mutant&&(read_pass||c.contains("fault")))continue;
    if(value_mode&&c.contains("fault")&&c.at("fault").at("write").get<bool>())continue;
    if(!read_pass&&c.contains("fault")&&!c.at("fault").at("write").get<bool>())continue;
    Memory memory;for(const auto&b:c.at("banks"))memory.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));
    memory.fault=c.value("fault",J{});memory.log_reads=read_pass;J stores=J::array();SourcePhysicsContactRecordObserver observer;
    if(!read_pass&&!value_mode)observer=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
    const auto sw=status(),cw=control();const auto mx=_mm_getcsr();bool failed{};NativeTu15ContactShapeMaterialIndex value;
    try{if(value_mode)value=native_tu15_contact_shape_material_index_value(memory,0x4000100u);
      else native_tu15_contact_shape_material_index(memory,0x4000100u,c.at("destination"),observer);
    }catch(const Fault&){failed=true;}
    const auto name=c.at("name").get<std::string>();check(failed==c.value("declared_fault",false),name+" completion differs");
    check(sw==status()&&cw==control()&&mx==_mm_getcsr(),name+" native material leaked hardware FP");check(c.at("before_fp")==c.at("after_fp"),name+" original material unexpectedly changes FP");
    if(mutant){check(!failed,"Mutant refused");++completed;bool difference{};for(const auto&b:c.at("banks"))difference|=memory.banks.at(b.at("address"))!=file(path.parent_path()/b.at("after_file").get<std::string>());changed+=difference;continue;}
    if(value_mode){
      if(!failed){const auto&e=c.at("writes").at(0);const auto bytes=unhex(e.at("written"));check(bytes.size()==2&&value.value==(std::to_integer<unsigned>(bytes[0])|(std::to_integer<unsigned>(bytes[1])<<8u))&&value.instruction==e.at("instruction"),name+" typed material value/SourcePC differs");}
      for(const auto&b:c.at("banks"))check(memory.banks.at(b.at("address"))==file(path.parent_path()/b.at("before_file").get<std::string>()),name+" value API changed Source DATA");
    }else{
      if(!read_pass)check(stores==c.at("writes"),name+" material stores differ");
      for(const auto&b:c.at("banks"))check(memory.banks.at(b.at("address"))==file(path.parent_path()/b.at("after_file").get<std::string>()),name+" complete material bank differs");
    }
    if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));check(memory.reads==expected,name+" ordered material reads differ");}
    ++profiles;
  }
  if(mutant){check(completed>100&&changed>0,"Mutant did not change completed material DATA");std::filesystem::create_directories("evidence");std::ofstream out(std::string("evidence/")+argv[3]+".json");out<<J{{"passed",true},{"completed",completed},{"data_mismatches",changed},{"refusals",0}}.dump()<<'\n';check(bool(out),"Cannot retain material mutant evidence");}
  else check(profiles>100,"Insufficient material profiles");
  std::cout<<J{{"passed",true},{"profiles",profiles},{"value_mode",value_mode},{"mutant",mutant}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    inputs=[*STAGE.rglob('*.hpp'),STAGE/UNIT,Path(__file__),ROOT/'tools/validate_native_array_growth_tu15.py',ROOT/'tools/observe_native_array_growth_tu15.cpp',ROOT/'tests/native_tu15_array_growth_tests.cpp'];pins={str(p.relative_to(ROOT)):sha(p)for p in inputs}
    report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,source_outcomes_supplied=False,live_integration_claimed=False,retained_large_artifacts=[])
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x1f66d0,)
        source=inspect.getsource(base.original)
        source=source.replace("    for case in all_cases:\n",'''    for case in all_cases:
        for p,value,width in case['global_inputs']:
            flat[p-0x10000:p-0x10000+width]=value.to_bytes(width,'little')
        (owned/'image.bin').write_bytes(flat)
''')
        source=source.replace(" mov ${case['receiver']},%ecx\n int3"," mov ${case['receiver']},%ecx\n mov ${case['destination']},%eax\n mov ${SHAPE},%edx\n int3")
        ns=dict(base.__dict__,SHAPE=SHAPE);exec(source,ns);original=ns['original'];rows=cases()
        banks={0x55d728:bytes(2),0x4f0414:bytes(4),0x4ecc18:bytes(4)}
        fixture,authority=original(image,banks,rows,scratch);normal=list(rows)
        faults=[];seen=set()
        for sample in normal:
            for kind in ('reads','writes'):
                occurrences={}
                for event in sample[kind]:
                    n=event.get('bytes',len(event.get('written',''))//2);key=(event['address'],n);occurrences[key]=occurrences.get(key,0)+1
                    if(event['instruction'],kind)in seen:continue
                    seen.add((event['instruction'],kind));r={k:copy.deepcopy(v)for k,v in sample.items()if k in ('entry','receiver','destination','arguments','user_words','global_inputs')}
                    r.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=event['address'],bytes=n,occurrence=occurrences[key],write=kind=='writes'));faults.append(r)
        fs=scratch/'faults';fs.mkdir();ff,_=original(image,banks,faults,fs)
        for row in faults:
            assert row['declared_fault']
            for bank in row['banks']:
                for key in ('before_file','after_file'):bank[key]=str(ff/bank[key])
        rows+=faults;save(fixture/'original.json',dict(image_sha256=NEW_PIN,cases=rows))
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        for f in STAGE.rglob('*.hpp'):shutil.copyfile(f,project/'include/halo2'/f.name)
        shutil.copyfile(STAGE/UNIT,project/'native.cpp');(project/'driver.cpp').write_text(driver())
        changes=(('default','std::uint16_t(fallback)','std::uint16_t(fallback^1u)'),
          ('tag','std::uint16_t(word(memory,materials+part*20u+0x10u,2))','std::uint16_t(word(memory,materials+part*20u+0x10u,2)^1u)'),
          ('bsp','std::uint16_t(word(memory,materials+signed_half(part)*20u+8u,2))','std::uint16_t(word(memory,materials+signed_half(part)*20u+8u,2)^1u)'))
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(materialIndex LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)\n'
        targets=[('material','native.cpp','')]
        original_cpp=(project/'native.cpp').read_text()
        for name,before,after in changes:
            assert original_cpp.count(before)==1;(project/('mutant_'+name+'.cpp')).write_text(original_cpp.replace(before,after));targets.append(('material_'+name,'mutant_'+name+'.cpp',' --mutant '+name))
        for name,unit,argv in targets:
            cmake+=f'add_executable({name} $<TARGET_OBJECTS:driver> {unit})\ntarget_include_directories({name} PRIVATE include)\ntarget_compile_features({name} PRIVATE cxx_std_20)\nadd_test(NAME {name} COMMAND {name} "{fixture/"original.json"}"{argv})\n'
        cmake+=f'add_test(NAME material_value COMMAND material "{fixture/"original.json"}" --value)\nadd_custom_target(material_all DEPENDS '+' '.join(t[0]for t in targets)+')\n';(project/'CMakeLists.txt').write_text(cmake)
        compiled={str(p.relative_to(project)):sha(p)for p in project.rglob('*')if p.is_file()}
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),'--target','material_all','--filter','^material','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--retain-report','evidence/'+name+'.json']for name,_,_ in changes),[])]
            r=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(args.output/(compiler+'-console.log')).write_text(r.stdout[-48000:]);report.setdefault('results',[]).append(dict(compiler=compiler,command=command,exit_code=r.returncode));assert r.returncode==0,r.stdout[-5000:]
        assert all(sha(ROOT/p)==h for p,h in pins.items())and all(sha(project/p)==h for p,h in compiled.items())
        executed={p for row in rows for p in row['executed_sites']};code=_Code(image,'.text');body=base.complete_body(code,0x1f66d0).code
        report.update(passed=True,profiles=len(rows),full_returns=len(normal),fault_prefixes=len(faults),
          ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),
          body_coverage=dict(total=len(body),executed=len(set(body)&executed)),authority=authority,
          compiled_source_pins=compiled,reference_runner_sha256=hashlib.sha256(source.encode()).hexdigest(),
          numeric_and_typed_value_apis=True,semantic_mutants=[c[0]for c in changes],
          scope='Full30-site 1F66D0 integer material lookup, numeric2B output and same typed Host value. Exact declared DATA/FP/read/store/PC/aliases/faults; native production caller integration remains separate.')
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(args.output/'report.json',report)
if __name__=='__main__':main()
