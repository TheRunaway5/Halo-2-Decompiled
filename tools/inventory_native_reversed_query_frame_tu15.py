#!/usr/bin/env python3
"""Read actual2F1A50/2F0F10 entries while executing an unchanged TU1.5 frame.

This is an offline original-entry census, not a native implementation proof.
It consumes only protected BEFORE banks and retains only small typed reports.
"""
import argparse, hashlib, inspect, json, os, subprocess
from pathlib import Path
import validate_native_phantom_pair_create_frame_derived_tu15 as frame

ROOT=frame.ROOT
def sha(p): return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j): p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--archive',type=Path,required=True)
 p.add_argument('--archive-sha256',required=True)
 p.add_argument('--output',type=Path,required=True)
 a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],native_kernel_qualified=False,supplied_original_outcomes=False)
 try:
  # Reuse exactly the existing BEFORE-only full-frame ELF preparation. No
  # AFTER bank is extracted or used as an input to original execution.
  source=inspect.getsource(frame.main)
  begin=source.index('  assert sha(a.archive)')
  end=source.index('  bridge=')
  environment=dict(frame.__dict__,a=a,scratch=scratch)
  exec('\n'.join(line[2:] if line.startswith('  ') else line for line in source[begin:end].splitlines()),environment)
  bridge=(ROOT/'tools/observe_native_contact_producer_tu15.cpp').read_text().split('struct Memory:SourcePhysicsContactMemory')[0]
  driver=bridge+r'''
J fp_words(const user_fpregs_struct&f){return J::array({f.cwd,f.swd,f.mxcsr});}
}
int main(int argc,char**argv){J result{{"passed",false},{"entries",J::array()}};try{
 check(argc==3,"Supply originalELF/report");Child child;child.pid=fork();check(child.pid>=0,"fork failed");
 if(!child.pid){const int fd=open("/dev/null",O_WRONLY);dup2(fd,1);close(fd);if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
 child.wait();request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);child.wait();check(WSTOPSIG(child.status)==SIGTRAP,"Original BEFORE bridge did not stop");
 for(unsigned ordinal=0;ordinal<128;++ordinal){
  const auto pc=child.go_any({0x2f1a50u,0x1ccacau});if(pc==0x1ccacau){result["world_return_reached"]=true;break;}
  const auto entry=child.regs();const auto sp=std::uint32_t(entry.rsp),agent=std::uint32_t(entry.rcx),output=child.word(sp+16u),header=child.word(output),before_count=child.word(header+4u),return_pc=child.word(sp);
  J row{{"ordinal",ordinal},{"owner",pc},{"agent",agent},{"arguments",J::array({child.word(sp+4u),child.word(sp+8u),child.word(sp+12u),output})},{"return_pc",return_pc},{"before_header",header},{"before_count",before_count},{"before_fp",fp_words(child.fp())}};
  child.go(0x2f0f10u);const auto nested=child.regs();const auto nested_sp=std::uint32_t(nested.rsp),query=child.word(agent+12u),query_table=child.word(query),query_owner=child.word(query_table+4u);
  const auto query_child=child.word(query+12u),query_child_table=child.word(query_child);
  row["child"]=J{{"owner",0x2f0f10u},{"agent",std::uint32_t(nested.rcx)},{"arguments",J::array({child.word(nested_sp+4u),child.word(nested_sp+8u),child.word(nested_sp+12u),child.word(nested_sp+16u)})},{"query_receiver",query},{"query_table",query_table},{"query_owner",query_owner},{"query_child",query_child},{"query_child_table",query_child_table},{"query_child_owner",child.word(query_child_table+4u)},{"cached_agent_before",child.word(agent+16u)}};
  child.go(0x2f0fafu);row["query_result_flag"]=std::to_integer<unsigned>(child.read(nested_sp-32u,1)[0]);row["query_return_fp"]=fp_words(child.fp());
  const auto cached=child.word(agent+16u);row["cached_process_owner_after_query"]=cached?child.word(child.word(cached)+16u):0u;
  child.go(return_pc);const auto after=child.regs();const auto after_header=child.word(output);
  row["after_header"]=after_header;row["after_count"]=child.word(after_header+4u);row["after_backing"]=child.word(after_header);row["cached_agent_after"]=child.word(agent+16u);row["after_fp"]=fp_words(child.fp());row["after_sp"]=std::uint32_t(after.rsp);
  check(std::uint32_t(after.rsp)==sp+20u,"Original2F1A50 RET16 differs");result["entries"].push_back(row);
 }
 check(result.value("world_return_reached",false),"Original owner census budget did not close");result["passed"]=true;
 }catch(const std::exception&e){result["error"]=e.what();}
 std::ofstream report(argv[2]);report<<result.dump(2)<<'\n';std::cout<<result.dump(2)<<std::endl;return result["passed"].get<bool>()?0:1;}
'''
  (scratch/'census.cpp').write_text(driver)
  command=['g++','-std=c++20','-O2','-I'+str(ROOT/'include'),str(scratch/'census.cpp'),'-o',str(scratch/'census')]
  r=subprocess.run(command,capture_output=True,text=True);(a.output/'build.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:]
  census=a.output/'original-entry-census.json'
  r=subprocess.run([str(scratch/'census'),str(environment['source_a'].output/'original-hardware'),str(census)])
  actual=json.loads(census.read_text());assert r.returncode==0,actual
  report.update(passed=True,archive_sha256=a.archive_sha256,original_entry_census=actual,after_banks_consumed=False,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__)},scope=__doc__)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file())
  save(a.output/'report.json',report)
 return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
