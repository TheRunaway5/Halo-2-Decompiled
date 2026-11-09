#!/usr/bin/env python3
"""Physical3200B0 numeric/typed DATA APIs with exact aliases and fault phases."""
import argparse,copy,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from build_native_tu15_constraint_before_reference import sha,save
STAGE=ROOT/'analysis/physics/native-island-info-typed-data-stage-20261103/staged'
UNIT='src/physics/native_tu15_island_partition_metadata.cpp'
ISLAND=base.UI+0x1000;INFO=base.UI+0x2000
def cases():
    rows=[]
    values=(0,1,0xffffffff,0x80000000,0x7fffffff,0x12345678)
    for a in values:
        for b in values:
            words={ISLAND+8:a,ISLAND+12:b,ISLAND+16:0xffffffff,ISLAND+24:0x7fffffff,
              INFO:b,INFO+4:a,INFO+8:0x80000000,INFO+12:0xffffffff}
            rows.append(dict(name=f'add_{a:x}_{b:x}',entry=0x3200b0,receiver=ISLAND,
              arguments=[base.UI+0x3000,INFO],user_words=[(p-base.UI,v)for p,v in words.items()]))
    seed=rows[15]
    for offset in(-12,-8,-4,0,4,8,12,16,20,24):
        row=copy.deepcopy(seed);row.update(name='alias_'+str(offset));row['arguments'][1]=ISLAND+offset;rows.append(row)
    row=copy.deepcopy(seed);row.update(name='current_info_after_second_publication',mutations=[dict(instruction=0x3200cf,address=INFO+4,written=struct.pack('<I',0x11223344).hex())]);rows.append(row)
    return rows
def driver():
    prefix=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text().split('int main(int argc,char**argv)')[0]
    return '#include "halo2/native_tu15_island_partition_metadata.hpp"\n'+prefix+r'''
struct Info final:NativeTu15IslandConstraintInfoData {
    Memory&memory;std::uint32_t address;bool local{};std::array<std::uint32_t,4>values{};mutable std::vector<unsigned>order;
    Info(Memory&m,std::uint32_t p):memory(m),address(p){}
    std::uint32_t word(unsigned n)const override{check(n<4,"Read unprovided info/control word");order.push_back(n);if(local)return values[n];std::array<std::byte,4>b{};memory.read(address+n*4u,b);std::uint32_t v{};for(unsigned k=0;k<4;++k)v|=std::to_integer<unsigned>(b[k])<<(8u*k);return v;}
};
int main(int argc,char**argv)try{
 check(argc==2||argc==4,"Supply island info fixture");const bool mutant=argc==4;const auto path=std::filesystem::path(argv[1]);std::ifstream input(path);const auto fixture=J::parse(input);unsigned completed{},changed{},profiles{};
 for(const auto&c:fixture.at("cases"))for(unsigned api=0;api<3;++api)for(bool read_pass:{false,true}){
  if(mutant&&(api||read_pass||c.contains("fault")||c.contains("mutations")))continue;
  const auto info=c.at("arguments")[1].get<std::uint32_t>();
  if(api==2&&(info!=0x4002000u||c.contains("fault")||c.contains("mutations")))continue;
  if(read_pass&&c.contains("mutations"))continue;
  if(!read_pass&&c.contains("fault")&&!c.at("fault").at("write").get<bool>())continue;
  Memory memory;for(const auto&b:c.at("banks"))memory.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));
  Info data(memory,info);if(api==2){for(unsigned n=0;n<4;++n)data.values[n]=data.word(n);data.local=true;data.order.clear();}
  memory.reads=J::array();memory.fault=c.value("fault",J{});memory.log_reads=read_pass;J stores=J::array();std::set<unsigned>applied;
  SourcePhysicsContactRecordObserver observer;if(!read_pass)observer=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});unsigned i{};for(const auto&m:c.value("mutations",J::array())){if(m.at("instruction")==e.instruction&&applied.insert(i).second){const auto b=unhex(m.at("written"));memory.write(m.at("address"),b);}++i;}};
  const auto sw=status(),cw=control();const auto mx=_mm_getcsr();bool failed{};NativeTu15IslandPartitionMetadataStats stats;
  try{if(api)native_tu15_add_island_constraint_info_data(memory,{c.at("receiver")},data,observer,&stats);
    else native_tu15_add_island_constraint_info(memory,{c.at("receiver")},{info},observer,&stats);
  }catch(const Fault&){failed=true;}
  const auto name=c.at("name").get<std::string>();check(failed==c.value("declared_fault",false),name+" completion differs");check(sw==status()&&cw==control()&&mx==_mm_getcsr()&&c.at("before_fp")==c.at("after_fp"),name+" info FP differs");check(stats.additions==1,"Info activity differs");
  if(mutant){check(!failed,"Info mutant refused");++completed;bool difference{};for(const auto&b:c.at("banks"))difference|=memory.banks.at(b.at("address"))!=file(path.parent_path()/b.at("after_file").get<std::string>());changed+=difference;continue;}
  if(read_pass){J expected=J::array();for(const auto&r:c.at("reads")){const auto p=r.at("address").get<std::uint32_t>();if(api==2&&p>=info&&p<info+16u)continue;expected.push_back(J::array({p,r.at("bytes")}));}check(memory.reads==expected,name+" ordered info reads differ");}
  else{check(stores==c.at("writes"),name+" info SourcePC stores differ");check(applied.size()==c.value("mutations",J::array()).size(),"Info mutation phase absent");}
  for(const auto&b:c.at("banks"))check(memory.banks.at(b.at("address"))==file(path.parent_path()/b.at("after_file").get<std::string>()),name+" info complete bank differs");
  if(api&&!failed)check(data.order==std::vector<unsigned>{0,1,1,2,3},"Info typed field order/control boundary differs");++profiles;
 }
 if(mutant){check(completed>20&&changed>0,"Info mutant did not change completed DATA");std::filesystem::create_directories("evidence");std::ofstream out(std::string("evidence/")+argv[3]+".json");out<<J{{"passed",true},{"completed",completed},{"data_mismatches",changed},{"refusals",0}}.dump()<<'\n';check(bool(out),"Cannot retain info mutation");}else check(profiles>100,"Insufficient info API coverage");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    files=[*STAGE.rglob('*.hpp'),STAGE/UNIT,Path(__file__),ROOT/'tools/validate_native_array_growth_tu15.py',ROOT/'tools/observe_native_array_growth_tu15.cpp',ROOT/'tests/native_tu15_array_growth_tests.cpp'];pins={str(p.relative_to(ROOT)):sha(p)for p in files};r=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,source_outcomes_supplied=False,live_integration_claimed=False,retained_large_artifacts=[])
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x3200b0,);rows=cases();fixture,authority=base.original(image,{},rows,scratch);normal=list(rows);faults=[];seen=set()
        for sample in normal:
            if sample.get('mutations'):continue
            for kind in('reads','writes'):
                occurrences={}
                for e in sample[kind]:
                    n=e.get('bytes',len(e.get('written',''))//2);key=(e['address'],n);occurrences[key]=occurrences.get(key,0)+1
                    if(e['instruction'],kind)in seen:continue
                    seen.add((e['instruction'],kind));row={k:copy.deepcopy(sample[k])for k in('entry','receiver','arguments','user_words')};row.update(name=f'fault_{kind}_{e["instruction"]:x}',fault=dict(address=e['address'],bytes=n,occurrence=occurrences[key],write=kind=='writes'));faults.append(row)
        fs=scratch/'faults';fs.mkdir();ff,_=base.original(image,{},faults,fs)
        for row in faults:
            assert row['declared_fault']
            for b in row['banks']:
                for k in('before_file','after_file'):b[k]=str(ff/b[k])
        rows+=faults;save(fixture/'original.json',dict(image_sha256=NEW_PIN,cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        for p in STAGE.rglob('*.hpp'):shutil.copyfile(p,project/'include/halo2'/p.name)
        shutil.copyfile(STAGE/UNIT,project/'native.cpp');(project/'driver.cpp').write_text(driver());text=(project/'native.cpp').read_text()
        changes=(('maximum','std::bit_cast<std::int32_t>(a)<=std::bit_cast<std::int32_t>(b)','a<=b'),('rows','current+rows','current-rows'),('aux','old_aux+aux','old_aux-aux'))
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(typedInfo LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)\n';targets=[('info','native.cpp','')]
        for name,before,after in changes:
            assert text.count(before)==1;(project/('mutant_'+name+'.cpp')).write_text(text.replace(before,after));targets.append(('info_'+name,'mutant_'+name+'.cpp',' --mutant '+name))
        for name,unit,argv in targets:cmake+=f'add_executable({name} $<TARGET_OBJECTS:driver> {unit})\ntarget_include_directories({name} PRIVATE include)\ntarget_compile_features({name} PRIVATE cxx_std_20)\nadd_test(NAME {name} COMMAND {name} "{fixture/"original.json"}"{argv})\n'
        cmake+='add_custom_target(info_all DEPENDS '+' '.join(t[0]for t in targets)+')\n';(project/'CMakeLists.txt').write_text(cmake);compiled={str(p.relative_to(project)):sha(p)for p in project.rglob('*')if p.is_file()}
        for cc in('g++','clang++'):
            cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/cc),'--target','info_all','--filter','^info','--cmake-arg=-DCMAKE_CXX_COMPILER='+cc,'--jobs','2','--test-jobs','1',*sum((['--retain-report','evidence/'+name+'.json']for name,_,_ in changes),[])];result=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(a.output/(cc+'-console.log')).write_text(result.stdout[-48000:]);r.setdefault('results',[]).append(dict(compiler=cc,command=cmd,exit_code=result.returncode));assert result.returncode==0,result.stdout[-5000:]
        assert all(sha(ROOT/p)==h for p,h in pins.items())and all(sha(project/p)==h for p,h in compiled.items());code=_Code(image,'.text');body=base.complete_body(code,0x3200b0).code;executed={p for row in rows for p in row['executed_sites']}
        r.update(passed=True,profiles=len(rows),full_returns=len(normal),fault_prefixes=len(faults),authority=authority,body_coverage=dict(total=len(body),executed=len(set(body)&executed)),ordered_reads=sum(len(row['reads'])for row in rows),ordered_stores=sum(len(row['writes'])for row in rows),compiled_source_pins=compiled,three_apis=['numeric','deferred memory DATA','typed local4words'],control_word_10_never_provided=True,typed_word_order=[0,1,1,2,3],semantic_mutants=[c[0]for c in changes],scope='Sole27-site3200B0 algorithm reused by numeric and typed DATA; fullretained banks/events/readorder/FP/aliases/faults. Contact31F460 composition/live remain separate.')
    except BaseException as e:r['error']=str(e)or repr(e);raise
    finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',r)
if __name__=='__main__':main()
