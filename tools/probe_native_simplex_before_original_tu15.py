#!/usr/bin/env python3
"""Execute unchanged original30E930 on a genuine typed native BEFORE input.

This diagnoses the source continuation only. No original result is supplied to
the native kernel. Raw banks/compiler products stay in owned scratch; retain
only source-site/call counts, small semantic outputs and immutable authority.
"""
import argparse,hashlib,inspect,json,os,struct,subprocess,tarfile
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,scope=__doc__,owned_scratch=str(scratch),retained_large_artifacts=[],native_result_supplied=False)
 try:
  receipt=json.loads((a.reference/'retention.json').read_text());archive=a.reference/'actual-native-before.tar.zst';capture=a.reference/'capture-native-before.json';assert sha(archive)==receipt['archive_sha256']and sha(capture)==receipt['capture_manifest_sha256']and sha(NEW_DEFAULT)==NEW_PIN
  d=json.loads(capture.read_text());assert d['schema']=='native-tu15-convex-simplex-typed-before-v1'and not d['physics_result_supplied']and not d['guest_abi_fabricated'];typed=d['input'];assert typed['owner']==0x30e930 and typed['preserved_x87_depth']==0
  wanted={r['before_file']:r for r in d['regions']};banks={};process=subprocess.Popen(['zstd','-dc',str(archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=process.stdout,mode='r|')as stream:
   for member in stream:
    name=Path(member.name).name
    if name in wanted:
     row=wanted[name];raw=stream.extractfile(member).read();assert len(raw)==row['size'];banks[row['address']]=raw
  assert process.wait()==0 and len(banks)==len(wanted);merged={};last=None
  for address,data in sorted(banks.items()):
   if last is not None and last+len(merged[last])==address:merged[last]+=data
   else:merged[address]=data;last=address
  image=Xbe(NEW_DEFAULT);section=next(s for s in image.sections if s.name=='.text');(scratch/'text.bin').write_bytes(image.section_bytes(section))
  source=inspect.getsource(base.original);source=source[:source.index('        active=dict(m')]+'        return owned/"reference"\n';source=source.replace("case['before_fp']=fp","fp=case['before_fp_input'];case['before_fp']=fp")
  fp=typed['floating_point'];case=dict(entry=0x30e930,receiver=typed['work'],arguments=[typed[n]for n in('first','second','relative_transform','support_cache','plane')],before_fp_input=[fp['x87_control_word'],fp['x87_status'],fp['mxcsr']]);environment=dict(base.__dict__,metadata=lambda i,b:({},{}));exec(source,environment);exe=environment['original'](image,merged,[case],scratch)
  bridge=(ROOT/'tools/observe_native_contact_producer_tu15.cpp').read_text();bridge=bridge[:bridge.index('struct Memory:SourcePhysicsContactMemory')]
  driver='#include "halo2/native_tu15_convex_simplex_solver.hpp"\n'+bridge+'''}
int main(int argc,char**argv){J result{{"passed",false}};try{
 check(argc==5,"Supply original ELF/text/request/output");std::ifstream input(argv[3]);const auto request_data=J::parse(input);
 std::ifstream text_file(argv[2],std::ios::binary);std::vector<char>code((std::istreambuf_iterator<char>(text_file)),{});
 Child child;child.pid=fork();check(child.pid>=0,"Cannot start original simplex");if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
 child.wait();request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);child.wait();check(WSTOPSIG(child.status)==SIGTRAP,"Original simplex setup failed");child.step();
 const auto first=request_data.at("text_first").get<std::uint32_t>();const auto entry=child.regs();const auto sp=std::uint32_t(entry.rsp),stop=child.word(sp);
 std::vector<std::uint32_t>path;std::map<std::uint32_t,unsigned>counts;
 while(child.regs().rip!=stop){const auto pc=std::uint32_t(child.regs().rip);check(path.size()<100000u,"Original simplex exceeded diagnostic step bound");
  check(pc>=first&&std::uint64_t(pc-first)+15u<=code.size(),"Original simplex requires an owner outside authenticated text at"+std::to_string(pc));
  const auto bytes=child.read(pc,15);for(unsigned n=0;n<15;++n)check(bytes[n]==std::byte(std::uint8_t(code[pc-first+n])),"Original simplex instruction bytes changed");
  path.push_back(pc);++counts[pc];child.step();
 }
 check(child.regs().rsp==sp+24u,"Original30E930 RET20 differs");const auto&typed=request_data.at("input");
 const auto hex=[](const std::vector<std::byte>&data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:data){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;};
 const auto fp=child.fp();result={{"passed",true},{"instructions",path.size()},{"pc_counts",counts},{"path",path},{"return_word",std::uint32_t(child.regs().rax)},
  {"cache_after",hex(child.read(typed.at("support_cache"),12))},{"plane_after",hex(child.read(typed.at("plane"),16))},
  {"work_counts_after",hex(child.read(typed.at("work"),12))},{"after_fp",J::array({fp.cwd,fp.swd,fp.mxcsr})},{"all_executed_instruction_bytes_authenticated",true}};
 }catch(const std::exception&e){result["error"]=e.what();std::cerr<<e.what()<<'\\n';}std::ofstream output(argv[4]);output<<result.dump(2)<<'\\n';return result.at("passed").get<bool>()?0:1;}
'''
  (scratch/'driver.cpp').write_text(driver);request=scratch/'request.json';request.write_text(json.dumps(dict(text_first=section.va,input=typed)))
  command=['c++','-std=c++20','-O2','-I'+str(ROOT/'include'),str(scratch/'driver.cpp'),'-o',str(scratch/'probe')];r=subprocess.run(command,capture_output=True,text=True);(a.output/'build.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-3000:]
  result=a.output/'original.json';r=subprocess.run([str(scratch/'probe'),str(exe),str(scratch/'text.bin'),str(request),str(result)],capture_output=True,text=True);(a.output/'probe.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-3000:]
  observed=json.loads(result.read_text());path=observed.pop('path');code=_Code(image,'.text');owner=complete_body(code,0x30e930);calls=[]
  for pc in dict.fromkeys(path):
   ins=code.at(pc)
   if ins.mnemonic=='call':calls.append(dict(pc=hex(pc),assembly=ins.op_str,count=path.count(pc),actual_targets=sorted({path[n+1]for n,p in enumerate(path[:-1])if p==pc})))
  observed['calls']=calls;observed['solver_sites_executed']=len(set(path)&set(owner.code));result.write_text(json.dumps(observed,indent=2)+'\n')
  listing=''.join(f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(set(path)));(a.output/'executed-original.txt').write_text(listing)
  report.update(passed=True,original_sha256=NEW_PIN,archive_sha256=receipt['archive_sha256'],input=typed,all_instruction_bytes_unchanged=True,source_instructions=observed['instructions'],solver_sites_executed=observed['solver_sites_executed'],return_word=observed['return_word'],result_sha256=sha(result),listing_sha256=hashlib.sha256(listing.encode()).hexdigest(),capture_gp_abi_consumed=False,tool_sha256=sha(__file__))
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
