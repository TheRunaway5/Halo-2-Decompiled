#!/usr/bin/env python3
"""Authenticate185020 immutable dispatch and exhaust native AL classification."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,x):p.write_text(json.dumps(x,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');body=complete_body(code,0x185020)
  tables={0x185044:image.read(0x185044,8),0x18504c:image.read(0x18504c,24)}
  targets=[int.from_bytes(tables[0x185044][n:n+4],'little')for n in(0,4)];assert set(targets)=={0x18503c,0x18503e}
  assert set(tables[0x18504c])<={0,1}
  true_kinds={n+1 for n,index in enumerate(tables[0x18504c])if targets[index]==0x18503c}
  source=ROOT/'src/physics/native_tu15_collection_filter.cpp';text=source.read_text();start=text.index('bool shape_has_user_data(');end=text.index('\n}',start)+2;native=text[start:end]
  cases={int(n)for n in re.findall(r'case (\d+):',native)};assert cases==true_kinds and 'default:return false;'in native
  assert 'case 24:return true;'in native and native.count('return true;')==1
  values=[*range(256),0x100,0x7fffffff,0x80000000,0xfffffffe,0xffffffff]
  expected=[int(v in true_kinds)for v in values]
  # The native routine is compiled unchanged in this TU; no implementation
  # result is used to construct the authenticated original truth set.
  driver=scratch/'switch.cpp';driver.write_text('#include "'+str(source)+'"\n#include <array>\n#include <stdexcept>\nint main(){const std::array<unsigned,'+str(len(values))+'>v{'+','.join(str(v)+'u'for v in values)+'};const std::array<unsigned,'+str(len(values))+'>e{'+','.join(map(str,expected))+'};for(unsigned n=0;n<v.size();++n)if(unsigned(halo2::shape_has_user_data(v[n]))!=e[n])throw std::runtime_error("Native AL switch differs");}\n')
  compilers=[]
  for compiler in('g++','clang++'):
   binary=scratch/('switch-'+compiler.replace('+','x'));cmd=[compiler,'-std=c++20','-O2','-I'+str(ROOT/'include'),str(driver),'-o',str(binary)];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-3000:];subprocess.run([str(binary)],check=True);compilers.append(dict(compiler=compiler,command=cmd,passed=True))
  instructions=[dict(address=p,bytes=code.at(p).bytes.hex(),assembly=code.at(p).mnemonic+' '+code.at(p).op_str)for p in sorted(body.code)]
  report.update(passed=True,original_sha256=NEW_PIN,source_sha256=sha(source),native_routine_sha256=hashlib.sha256(native.encode()).hexdigest(),
   instructions=instructions,immutable_tables=[dict(address=p,bytes=raw.hex(),sha256=hashlib.sha256(raw).hexdigest())for p,raw in tables.items()],
   source_dispatch_fetch_instructions=[0x18502e,0x185035],true_kinds=sorted(true_kinds),compiled_profiles_per_compiler=len(values),compiler_results=compilers,
   complete_uint32_domain_argument='Original decrements uint32 kind, compares unsigned<=23; only1..24 index the authenticated tables. Native switch has exactly the same true singleton cases and false default for every other uint32. All256bytevalues and5wide edges compiled under both compilers.',
   scope='AL classification only; sourcecaller consumesAL. Tables are CPU instruction dispatch, not physics or callback DATA. No source opcode/table patch or native GP/PC transport.')
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
