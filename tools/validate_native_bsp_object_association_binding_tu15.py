#!/usr/bin/env python3
"""Qualify the same native BSP binding with real Source DATA and native sidecars."""
import argparse,hashlib,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_bsp_object_association_tu15 as graph
from build_native_tu15_constraint_before_reference import sha,save
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def driver():
    text=graph.driver_text().replace('#include "halo2/native_tu15_bsp_object_association.hpp"',
        '#include "halo2/native_tu15_bsp_object_association_binding.hpp"')
    text=text.replace('else native_tu15_initialize_bsp_object_associations(memory,scratch,observe);',
      '''else {
          std::vector<NativeTu15ContactPrivateRange>retained;
          for(const auto&bank:c.at("banks"))retained.push_back({bank.at("address"),std::uint32_t(memory.banks.at(bank.at("address")).size())});
          NativeTu15BspObjectAssociationStats stats;
          native_tu15_initialize_bound_bsp_object_associations(memory,{0x74000000u,32u},retained,observe,&stats);
          check(stats.attempts==1&&stats.completed==1&&stats.refusals==0,name+" binding activity differs");
          check(std::all_of(memory.banks.at(0x74000000u).begin(),memory.banks.at(0x74000000u).end(),[](auto b){return b==std::byte(0xa5);}),name+" native sidecar changed backing RAM");
        }''')
    # Exercise the production sidecar with two different unavailable backings.
    # The actual outer bank remains sentinel-filled; it is never an input.
    text=text.replace('std::size_t cases{},stores{}', 'std::size_t cases{},stores{}')
    marker='  check(cases>20,'
    extra=r'''
  {
    const auto&sample=fixture.at("cases")[0];
    const auto make=[&]{Memory memory;for(const auto&bank:sample.at("banks"))memory.banks.emplace(bank.at("address"),file(path.parent_path()/bank.at("before_file").get<std::string>()));return memory;};
    auto memory=make();std::vector<NativeTu15ContactPrivateRange>retained;
    for(const auto&bank:sample.at("banks"))retained.push_back({bank.at("address"),std::uint32_t(memory.banks.at(bank.at("address")).size())});
    unsigned rejected{};
    for(const auto resources:std::array{NativeTu15BspObjectAssociationResources{0x74000000u,31u},
        NativeTu15BspObjectAssociationResources{0x74000000u,64u},
        NativeTu15BspObjectAssociationResources{0x74000001u,32u},
        NativeTu15BspObjectAssociationResources{0xfffffff0u,32u},
        NativeTu15BspObjectAssociationResources{0x4000000u,32u}}){
      const auto before=memory.banks;bool failed{};
      try{native_tu15_initialize_bound_bsp_object_associations(memory,resources,retained);}
      catch(const std::runtime_error&){failed=true;}
      check(failed&&memory.banks==before,"Malformed BSP resources changed retained DATA");++rejected;
    }
    // A retained mask publication may not hide inside the iterator sidecar.
    std::array<std::byte,4>pointer{std::byte{},std::byte{},std::byte{},std::byte(0x74)};
    memory.write(0x505e7cu,pointer);const auto before=memory.banks;bool failed{};
    try{native_tu15_initialize_bound_bsp_object_associations(memory,{0x74000000u,32u},retained);}
    catch(const std::runtime_error&e){failed=std::string(e.what()).find("aliases private DATA")!=std::string::npos;}
    check(failed&&memory.banks==before,"Private BSP mask publication was concealed");++rejected;
    // An actual standalone iterator owns15 Source-written bytes, leaving its
    // padding unavailable; the shared production guard must enforce this.
    auto outer=make();const std::array owned{NativeTu15ContactPrivateRange{0x74000000u,32u}};
    NativeTu15ContactCandidatePrivateData data(outer,owned,retained);
    native_tu15_reset_bsp_object_associations(data,{0x74000000u});
    std::array<std::byte,1>padding;failed=false;
    try{data.read(0x74000005u,padding);}catch(const std::runtime_error&e){failed=std::string(e.what()).find("unprovided private DATA")!=std::string::npos;}
    check(failed,"BSP iterator padding was fabricated");++rejected;
    check(rejected==7,"BSP resource/padding guard controls missing");
  }
'''
    assert text.count(marker)==1
    return text.replace(marker,extra+marker)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    files=[*graph.STAGE.rglob('*.hpp'),graph.STAGE/'src/physics/native_tu15_bsp_object_association.cpp',ROOT/'tools/validate_native_bsp_object_association_tu15.py',Path(__file__),ROOT/'tools/validate_native_array_growth_tu15.py',ROOT/'tools/observe_native_array_growth_tu15.cpp',ROOT/'tests/native_tu15_array_growth_tests.cpp']
    pins={str(f.relative_to(ROOT)):sha(f)for f in files}
    report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],source_outcomes_supplied=False,live_integration_claimed=False)
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN;graph.base.OWNERS=graph.OWNERS
        metadata=graph.base.metadata
        def exact_metadata(*a):
            m,authority=metadata(*a);next(s for s in m['sites']if s['instruction']==0xbcf2e)['operands'][0]['read']=True;return m,authority
        graph.base.metadata=exact_metadata
        banks={p:struct.pack('<I',v)for p,v in ((0x4ecbd0,graph.POOL),(0x4ecc18,graph.SCENARIO),(0x505e7c,graph.MASK))}
        rows=graph.cases();fixture,authority=graph.base.original(Xbe(NEW_DEFAULT),banks,rows,scratch)
        save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows))
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        for f in files:
            if f.suffix=='.hpp'and graph.STAGE in f.parents:shutil.copyfile(f,project/'include/halo2'/f.name)
        shutil.copyfile(graph.STAGE/'src/physics/native_tu15_bsp_object_association.cpp',project/'native.cpp')
        (project/'driver.cpp').write_text(driver())
        (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(boundBsp LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(bound driver.cpp native.cpp)\ntarget_include_directories(bound PRIVATE include)\ntarget_compile_features(bound PRIVATE cxx_std_20)\ntarget_link_libraries(bound PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME bound_bsp COMMAND bound "'+str(fixture/'original.json')+'")\n')
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),'--target','bound','--filter','^bound_bsp$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1']
            result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(args.output/(compiler+'-console.log')).write_text(result.stdout[-32000:])
            report.setdefault('results',[]).append(dict(compiler=compiler,command=command,exit_code=result.returncode));assert result.returncode==0,result.stdout[-4000:]
        assert all(sha(ROOT/f)==v for f,v in pins.items())
        report.update(passed=True,profiles=len(rows),authority=authority,resource_and_padding_controls=7,
          same_production_private_guard=True,underlying_bank_unchanged=True,source_control_bytes_supplied=False,
          scope='Same production native BSP sidecar binding; retained DATA/events/reads/FP/return/fault/aliases, malformed resources and hidden publication refusal. Host delivery remains separate.')
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());save(args.output/'report.json',report)
if __name__=='__main__':main()
