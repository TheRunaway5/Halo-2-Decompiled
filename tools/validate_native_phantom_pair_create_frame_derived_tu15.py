#!/usr/bin/env python3
"""Derive genuine31FFC0 entries by executing an actual BEFORE-only world frame.

The derived current-DATA states are original-execution witnesses, not captured
native31FFC0 ABIs. Qualify unchanged native leaf composition on those states.
"""
import argparse,hashlib,inspect,json,os,shutil,subprocess,sys,tarfile,types
from pathlib import Path
import replay_live_phantom_hardware_tu15 as replay
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],captured_native_leaf_abi=False,supplied_original_outcomes=False,after_banks_consumed=False)
 try:
  assert sha(a.archive)==a.archive_sha256
  manifest=a.archive.parent/'manifests/capture.json';capture=json.loads(manifest.read_text());assert capture['entry']==0x1cc8d0 and capture['comparison_contract']=='native_tu15_void_world_frame_v1'
  bankdir=scratch/'actual-frame-before';bankdir.mkdir();save(bankdir/'capture.json',capture);wanted={r['before_file']:r for r in capture['regions']};found=set();stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=Path(member.name).name
    if name in wanted:
     raw=tar.extractfile(member).read();assert len(raw)==wanted[name]['size'];(bankdir/name).write_bytes(raw);found.add(name)
  assert stream.wait()==0 and found==set(wanted)
  source=inspect.getsource(replay.main);prefix=source[source.index(' a.capture='):source.index(" tracer=ROOT/")];source_a=types.SimpleNamespace(capture=bankdir,output=scratch/'original-frame',native_frame_semantics=True,component_inputs=False,mcc_symbol_reference=False,mcc_input_views=False,recorded_platform_time=False,return_breakpoints=True)
  environment=dict(replay.__dict__,a=source_a);exec('\n'.join(line[1:]if line.startswith(' ')else line for line in prefix.splitlines()),environment)
  bridge=(ROOT/'tools/observe_native_contact_producer_tu15.cpp').read_text().split('struct Memory:SourcePhysicsContactMemory')[0]
  bridge=bridge.replace('"Diagnostic producer left its genuine original boundary"','"Diagnostic producer left genuine boundary at "+std::to_string(r.rip)+" signal="+std::to_string(WSTOPSIG(status))')
  driver=bridge+'''
}
int main(int argc,char**argv){J result{{"passed",false},{"entries",J::array()}};try{
 check(argc==4,"Supply originalELF/capture/output");std::ifstream input(argv[2]);const auto capture=J::parse(input);const std::filesystem::path output=argv[3];std::filesystem::create_directories(output);
 Child child;child.pid=fork();check(child.pid>=0,"fork failed");if(!child.pid){int fd=open("/dev/null",O_WRONLY);dup2(fd,1);close(fd);if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}child.wait();request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);child.wait();check(WSTOPSIG(child.status)==SIGTRAP,"Original BEFORE bridge did not stop");
 for(unsigned ordinal=0;ordinal<16;++ordinal){const auto pc=child.go_any({0x31ffc0u,0x1ccacau});if(pc==0x1ccacau){result["world_return_reached"]=true;break;}const auto regs=child.regs();const auto fp=child.fp();check(fp.ftw==0,"Phantom entry requires genuine empty incoming x87 domain");const auto pair=child.word(std::uint32_t(regs.rsp)+4u),handler=std::uint32_t(regs.rcx);const auto path=output/("entry-"+std::to_string(ordinal));std::filesystem::create_directories(path);J regions=J::array();
 for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>(),size=r.at("size").get<std::uint32_t>();const auto raw=child.read(address,size);const auto name="before-"+std::to_string(address)+".bin";std::ofstream out(path/name,std::ios::binary);out.write(reinterpret_cast<const char*>(raw.data()),raw.size());regions.push_back(J{{"address",address},{"size",size},{"before_file",name}});}
 J typed{{"owner",0x31ffc0u},{"handler",handler},{"pair",pair},{"preserved_x87_depth",0},{"floating_point",J{{"x87_control_word",fp.cwd},{"x87_status",fp.swd},{"mxcsr",fp.mxcsr}}}};
 J derived{{"input",typed},{"regions",regions},{"guest_abi_fabricated",false},{"physics_result_supplied",false},{"captured_native_leaf_abi",false},{"provenance","Typed original31FFC0 entry reached by executing genuine fullframe BEFORE, before any selectedleaf instruction"}};std::ofstream out(path/"capture-native-before.json");out<<derived.dump(2)<<'\\n';out.close();result["entries"].push_back(J{{"ordinal",ordinal},{"input",typed}});std::cout<<"GENUINE ORIGINAL PHANTOM ENTRY "<<typed.dump()<<std::endl;child.go(child.word(std::uint32_t(regs.rsp)));
 }
 check(result.value("world_return_reached",false),"Original callback budget did not close");check(!result["entries"].empty(),"Actualframe selected no phantomcreate");result["passed"]=true;
 }catch(const std::exception&e){result["error"]=e.what();}std::ofstream report(std::filesystem::path(argv[3])/"report.json");report<<result.dump(2)<<'\\n';std::cout<<result.dump(2)<<std::endl;return result["passed"].get<bool>()?0:1;}
'''
  driver='#include <filesystem>\n'+driver;(scratch/'derive.cpp').write_text(driver);command=['g++','-std=c++20','-O2','-I'+str(ROOT/'include'),str(scratch/'derive.cpp'),'-o',str(scratch/'derive')];r=subprocess.run(command,capture_output=True,text=True);(a.output/'derive-build.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:]
  derived=scratch/'derived';r=subprocess.run([str(scratch/'derive'),str(source_a.output/'original-hardware'),str(bankdir/'capture.json'),str(derived)]);census=json.loads((derived/'report.json').read_text());save(a.output/'original-entry-census.json',census);assert r.returncode==0,census;results=[]
  for entry in census['entries']:
   folder=derived/('entry-'+str(entry['ordinal']));archive=folder/'actual-native-before.tar.zst';subprocess.run(['tar','--zstd','-cf',str(archive),'-C',str(folder),*['./'+name for name in sorted(x.name for x in folder.iterdir()if x.is_file())]],check=True)
   command=[sys.executable,str(ROOT/'tools/run_test.py'),'--',sys.executable,str(ROOT/'tools/validate_native_phantom_pair_create_actual_tu15.py'),'--archive',str(archive),'--archive-sha256',sha(archive),'--output',str(a.output/('entry-'+str(entry['ordinal'])))];r=subprocess.run(command);results.append(dict(ordinal=entry['ordinal'],exit_code=r.returncode));assert r.returncode==0,'Native phantom composition differs on genuine original frame-derived entry'
  report.update(passed=True,archive_sha256=a.archive_sha256,original_entry_census=census,results=results,source_hashes={name:sha(ROOT/name)for name in('tools/validate_native_phantom_pair_create_frame_derived_tu15.py','tools/validate_native_phantom_pair_create_actual_tu15.py','tools/replay_live_phantom_hardware_tu15.py')},scope=__doc__)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
