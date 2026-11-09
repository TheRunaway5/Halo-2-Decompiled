#!/usr/bin/env python3
"""Physical31F460→3200B0 through typed native info, without control-stack words."""
import argparse,copy,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from build_native_tu15_constraint_before_reference import sha,save
STAGE=ROOT/'analysis/physics/native-created-constraint-metadata-stage-20261103/staged'
MANAGER=base.UI+0x1000;RECEIVER=base.UI+0x2000;TABLE=base.UI+0x3000
def cases():
    rows=[]
    for matched in(False,True):
        for same in(False,True):
            for a,b in((0,0),(0x3f000000,0x3fc00000),(0x80000000,0),
                        (0x7fa12345,0x7fc54321),(0xffffffff,0x80000000)):
                words={MANAGER+0x10:TABLE if matched else TABLE+1,MANAGER+0x1c:0x87651234 if same else 0,
                  MANAGER+0x24:RECEIVER,RECEIVER:TABLE,RECEIVER+8:0x80000000,
                  RECEIVER+12:0xffffffff,RECEIVER+16:0x7fffffff,RECEIVER+24:0xffffffff,TABLE+8:0x3200b0}
                rows.append(dict(name=f'scale_{matched}_{same}_{a:x}_{b:x}',entry=0x31f460,receiver=0,
                  arguments=[MANAGER,0x87651234,0xfedc4321,a,b],user_words=[(p-base.UI,v)for p,v in words.items()]))
    seed=next(r for r in rows if r['name']=='scale_True_True_3f000000_3fc00000')
    for receiver in(MANAGER,MANAGER+8,MANAGER+0x10,MANAGER+0x20):
        row=copy.deepcopy(seed);row['name']=f'alias_receiver_{receiver:x}';words=dict(row['user_words']);words[MANAGER+0x24-base.UI]=receiver;words[receiver-base.UI]=TABLE
        # Preserve the selected schema independently of alias setup. All other
        # ordinary object fields retain their actual current aliased values.
        words[MANAGER+0x10-base.UI]=TABLE;row['user_words']=list(words.items());rows.append(row)
    row=copy.deepcopy(seed);row['name']='current_receiver_after_weights';words=dict(row['user_words']);other=base.UI+0x2400
    words.update({other-base.UI:TABLE,other+8-base.UI:0x7fffffff,other+12-base.UI:4,other+16-base.UI:7,other+24-base.UI:9});row['user_words']=list(words.items());row['mutations']=[dict(instruction=0x31f4b2,address=MANAGER+0x24,written=struct.pack('<I',other).hex())];rows.append(row)
    return rows
def driver():
    prefix=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text().split('int main(int argc,char**argv)')[0]
    return '#include "halo2/native_tu15_created_constraint_metadata_binding.hpp"\n'+prefix+r'''
int main(int argc,char**argv)try{
 check(argc==2||argc==4,"Supply created metadata fixture");const bool mutant=argc==4;const auto path=std::filesystem::path(argv[1]);std::ifstream input(path);const auto fixture=J::parse(input);unsigned completed{},changed{},profiles{};
 for(const auto&c:fixture.at("cases"))for(bool read_pass:{false,true}){
  if(mutant&&(read_pass||c.contains("fault")||c.contains("mutations")))continue;
  if(read_pass&&c.contains("mutations"))continue;
  if(!read_pass&&c.contains("fault")&&!c.at("fault").at("write").get<bool>())continue;
  Memory memory;for(const auto&b:c.at("banks"))memory.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));memory.fault=c.value("fault",J{});memory.log_reads=read_pass;
  J stores=J::array();std::set<unsigned>applied;SourcePhysicsContactRecordObserver observer;
  if(!read_pass)observer=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});unsigned i{};for(const auto&m:c.value("mutations",J::array())){if(m.at("instruction")==e.instruction&&applied.insert(i).second){const auto b=unhex(m.at("written"));memory.write(m.at("address"),b);}++i;}};
  NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15ContactBodyCreationServices services;NativeTu15CreatedConstraintMetadataActivity activity;
  services.update_constraint=[&](auto&current,auto receiver,auto owner,auto,const auto&info,auto&state){check(&current==&memory&&&state==&fp,"Created metadata view/FP owner differs");native_tu15_update_created_constraint_metadata(current,receiver,owner,info,observer,&activity);};
  const auto sw=status(),cw=control();const auto mx=_mm_getcsr();bool failed{};
  try{const auto&a=c.at("arguments");native_tu15_scale_created_contact_constraint(memory,a[0],a[1],a[2],a[3],a[4],fp,services,observer);}
  catch(const Fault&){failed=true;}
  const auto name=c.at("name").get<std::string>();check(failed==c.value("declared_fault",false),name+" completion differs");check(sw==status()&&cw==control()&&mx==_mm_getcsr(),name+" created metadata hardware FP differs");check(J::array({fp.control_word,fp.status,fp.mxcsr})==c.at("after_fp"),name+" created metadata semantic FP differs");
  if(mutant){check(!failed,"Created metadata mutant refused");++completed;bool difference{};for(const auto&b:c.at("banks"))difference|=memory.banks.at(b.at("address"))!=file(path.parent_path()/b.at("after_file").get<std::string>());changed+=difference;continue;}
  if(read_pass){J expected=J::array();for(const auto&e:c.at("reads"))expected.push_back(J::array({e.at("address"),e.at("bytes")}));check(memory.reads==expected,name+" created metadata read order differs");}
  else{check(stores==c.at("writes"),name+" created metadata SourcePC stores differ");check(applied.size()==c.value("mutations",J::array()).size(),"Created metadata mutation phase absent");}
  for(const auto&b:c.at("banks"))check(memory.banks.at(b.at("address"))==file(path.parent_path()/b.at("after_file").get<std::string>()),name+" created metadata bank differs");++profiles;
 }
 if(mutant){check(completed>15&&changed>0,"Created metadata mutation did not change completed DATA");std::filesystem::create_directories("evidence");std::ofstream out(std::string("evidence/")+argv[3]+".json");out<<J{{"passed",true},{"completed",completed},{"data_mismatches",changed},{"refusals",0}}.dump()<<'\n';check(bool(out),"Cannot retain created metadata mutant");}else check(profiles>30,"Insufficient created metadata coverage");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();files=[p for p in STAGE.rglob('*')if p.is_file()]+[Path(__file__),ROOT/'tools/validate_native_array_growth_tu15.py',ROOT/'tools/observe_native_array_growth_tu15.cpp',ROOT/'tests/native_tu15_array_growth_tests.cpp'];pins={str(p.relative_to(ROOT)):sha(p)for p in files};r=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,source_outcomes_supplied=False,live_admission_claimed=False,retained_large_artifacts=[])
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x31f460,0x3200b0);banks={0x48d1b0:struct.pack('<I',TABLE),0x48cf90:struct.pack('<I',TABLE+0x100)};rows=cases();fixture,authority=base.original(image,banks,rows,scratch);normal=list(rows);faults=[];seen=set()
        for sample in normal:
            if sample.get('mutations'):continue
            for kind in('reads','writes'):
                occurrences={}
                for e in sample[kind]:
                    n=e.get('bytes',len(e.get('written',''))//2);key=(e['address'],n);occurrences[key]=occurrences.get(key,0)+1
                    if(e['instruction'],kind)in seen:continue
                    seen.add((e['instruction'],kind));row={k:copy.deepcopy(sample[k])for k in('entry','receiver','arguments','user_words')};row.update(name=f'fault_{kind}_{e["instruction"]:x}',fault=dict(address=e['address'],bytes=n,occurrence=occurrences[key],write=kind=='writes'));faults.append(row)
        fs=scratch/'faults';fs.mkdir();ff,_=base.original(image,banks,faults,fs)
        for row in faults:
            assert row['declared_fault']
            for b in row['banks']:
                for k in('before_file','after_file'):b[k]=str(ff/b[k])
        rows+=faults;save(fixture/'original.json',dict(image_sha256=NEW_PIN,cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');(project/'src').mkdir()
        for p in STAGE.rglob('*'):
            if not p.is_file():continue
            dest=project/'include/halo2'/p.name if p.suffix=='.hpp'and 'include'in p.parts else project/'src'/p.name;shutil.copyfile(p,dest)
        (project/'driver.cpp').write_text(driver());kernel=project/'src/native_tu15_contact_body_creation.cpp';text=kernel.read_text();changes=(('weights','const auto a=same?first_weight:second_weight,b=same?second_weight:first_weight;','const auto a=same?second_weight:first_weight,b=same?first_weight:second_weight;'),('parameter','{0,0,0x88u,0,a}','{0,0,0x89u,0,a}'),('schema','m.word(0x48cf90u)','m.word(0x48cf90u)^1u'))
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(createdMetadata LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(driver OBJECT driver.cpp src/native_tu15_island_partition_metadata.cpp)\ntarget_include_directories(driver PRIVATE include src)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)\n';targets=[('created','src/native_tu15_contact_body_creation.cpp','')]
        for name,before,after in changes:
            assert text.count(before)==1;(project/('mutant_'+name+'.cpp')).write_text(text.replace(before,after));targets.append(('created_'+name,'mutant_'+name+'.cpp',' --mutant '+name))
        for name,unit,argv in targets:cmake+=f'add_executable({name} $<TARGET_OBJECTS:driver> {unit})\ntarget_include_directories({name} PRIVATE include src)\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_options({name} PRIVATE -Wl,--gc-sections)\nadd_test(NAME {name} COMMAND {name} "{fixture/"original.json"}"{argv})\n'
        cmake+='add_custom_target(created_all DEPENDS '+' '.join(t[0]for t in targets)+')\n';(project/'CMakeLists.txt').write_text(cmake);compiled={str(p.relative_to(project)):sha(p)for p in project.rglob('*')if p.is_file()}
        for cc in('g++','clang++'):
            cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/cc),'--target','created_all','--filter','^created','--cmake-arg=-DCMAKE_CXX_COMPILER='+cc,'--jobs','2','--test-jobs','1',*sum((['--retain-report','evidence/'+n+'.json']for n,_,_ in changes),[])];res=subprocess.run(cmd,capture_output=True,text=True);(a.output/(cc+'-console.log')).write_text((res.stdout+res.stderr)[-48000:]);r.setdefault('results',[]).append(dict(compiler=cc,command=cmd,exit_code=res.returncode));assert res.returncode==0,(res.stdout+res.stderr)[-5000:]
        assert all(sha(ROOT/p)==h for p,h in pins.items())and all(sha(project/p)==h for p,h in compiled.items());code=_Code(image,'.text');executed={p for row in rows for p in row['executed_sites']}
        r.update(passed=True,profiles=len(rows),full_returns=len(normal),fault_prefixes=len(faults),ordered_reads=sum(len(row['reads'])for row in rows),ordered_stores=sum(len(row['writes'])for row in rows),authority=authority,body_coverage={hex(o):dict(total=len(base.complete_body(code,o).code),executed=len(set(base.complete_body(code,o).code)&executed))for o in base.OWNERS},compiled_source_pins=compiled,info_words_exposed=[0,4,8,12],return_control_word_10_never_provided=True,semantic_mutants=[c[0]for c in changes],scope='Full31F460 typed virtual3200B0 composition on declared schema/weight/alias/fault inputs, exact retained DATA/FP/read/store/SourcePC/order; no original CPU service, info control-stack image, gameplay/tuning input or live admission.')
    except BaseException as e:r['error']=str(e)or repr(e);raise
    finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',r)
if __name__=='__main__':main()
