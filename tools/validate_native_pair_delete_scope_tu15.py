#!/usr/bin/env python3
"""Ensure native pair-deletion owners refuse before transport and restore scope."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--formal-stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');formal=a.formal_stage/'staged/include/halo2/native_tu15_pair_handlers.hpp';shutil.copyfile(formal,project/'include/halo2/native_tu15_pair_handlers.hpp')
  cpp=r'''#include "halo2/native_tu15_pair_deletion_native_scope.hpp"
#include <cassert>
using namespace halo2;
int main(){
 NativeTu15PairDeletionNativeScope scope;NativeTu15PairDeletionNativeScopeActivity a,b;
 unsigned abi_mutations{},executor_calls{},caught{};
 const auto adapter=[&](std::uint32_t owner){scope.reject_unconverted(owner);++abi_mutations;++executor_calls;};
 const auto refused=[&](std::uint32_t owner){const auto before=abi_mutations;try{adapter(owner);assert(false);}catch(const std::runtime_error&error){++caught;assert(std::string(error.what()).find(std::to_string(owner))!=std::string::npos);}assert(abi_mutations==before&&executor_calls==before);};
 adapter(1);
 {NativeTu15PairDeletionNativeScopeGuard outer(scope,&a);refused(0x316aa0);
  {NativeTu15PairDeletionNativeScopeGuard inner(scope,&b);refused(0x149930);}
  refused(0x316aa0);
  {NativeTu15PairDeletionNativeScopeGuard inner(scope);refused(0x2e12a0);}
  refused(0x1d7530);
 }
 adapter(2);
 try{NativeTu15PairDeletionNativeScopeGuard guard(scope,&a);throw 17;}catch(int n){assert(n==17);}
 adapter(3);
 {NativeTu15PairDeletionNativeScopeGuard guard(scope);refused(0x22fea0);}
 adapter(4);
 assert(caught==6&&abi_mutations==4&&executor_calls==4);
 assert(a.refused==3&&a.owners.size()==2&&a.owners.at(0x316aa0)==2&&a.owners.at(0x1d7530)==1);
 assert(b.refused==1&&b.owners.size()==1&&b.owners.at(0x149930)==1);
}
''';(project/'scope.cpp').write_text(cpp);results=[]
  for compiler in('g++','clang++'):
   binary=project/compiler.replace('+','x');command=[compiler,'-std=c++20','-O2','-Wall','-Wextra','-Werror','-I'+str(project/'include'),str(project/'scope.cpp'),'-o',str(binary)];r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-2500:];subprocess.run([str(binary)],check=True);results.append(dict(compiler=compiler,command=command,passed=True))
  report.update(passed=True,compiler_results=results,refused_before_transport_cases=6,restored_outside_execution_cases=4,named_owner_maps=True,nested_scopes_and_exception_restore=True,source_hashes={str(p):sha(p)for p in[formal,ROOT/'include/halo2/native_tu15_pair_deletion_native_scope.hpp',ROOT/'include/halo2/native_tu15_pair_deletion_binding.hpp',Path(__file__)]},scope='Native-only guard control proof; no original physics behavior or live selection claimed')
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
