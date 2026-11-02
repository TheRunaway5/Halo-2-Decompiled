#!/usr/bin/env python3
"""Read-only CD4A0 entry study over actual1477 camera-query BEFORE DATA.

Object identity is read from the actual local control record. Destination and
incoming FP are explicitly declared independent formal inputs, not a captured
CD4A0 ABI. Every query executes original16C620 to its real return; no query
outcome, flags, pose, control record or camera result is supplied.
"""
import argparse,bisect,hashlib,inspect,json,lzma,os,struct,subprocess,tarfile
from pathlib import Path
import validate_native_array_growth_tu15 as base
from observe_native_camera_query_mopp_entry_tu15 import reference_original
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',type=Path,required=True);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--declared-heights',action='store_true');p.add_argument('--seek-second-hit',action='store_true');a=p.parse_args();assert not a.seek_second_hit or a.declared_heights;a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),original_execution_only=True,native_qualified=False,captured_CD4A0_ABI=False,supplied_query_outcomes=False,source_physics_writes_by_harness=False,retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN and sha(a.archive)==a.archive_sha256
  capture=json.loads(a.archive.with_name('capture-native-before.json').read_text());assert capture['schema']=='native-tu15-camera-query-typed-before-v1'and not capture['physics_result_supplied']
  hashes=json.loads(lzma.decompress(a.archive.with_name('all-file-hashes.json.xz').read_bytes()));wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=str(Path(member.name));name=name[2:]if name.startswith('./')else name
    if name not in wanted:continue
    r=wanted[name];raw=tar.extractfile(member).read();proof=hashes.get('./'+name,hashes.get(name));assert len(raw)==r['size']and hashlib.sha256(raw).hexdigest()==proof['sha256'];banks[r['address']]=raw
  assert stream.wait()==0 and len(banks)==len(wanted);starts=sorted(banks)
  def read(p,n):
   i=bisect.bisect_right(starts,p)-1;assert i>=0 and p+n<=starts[i]+len(banks[starts[i]]),(hex(p),n);return banks[starts[i]][p-starts[i]:p-starts[i]+n]
  def word(p,n=4):return int.from_bytes(read(p,n),'little')
  controls=word(0x4f9b54);objects=word(0x4ecbd0);headers=word(objects+0x44);entries=[]
  for slot in range(4):
   record=controls+slot*0x94;datum=word(record+0x14);row=dict(control_index=slot,control_record=record,object=datum)
   if datum not in(0,0xffffffff):
    header=headers+(datum&0xffff)*12;unit=word(header+8);row.update(header=header,unit=unit,parent=word(unit+0x14),seat=word(unit+0x1fc,2),kind=word(unit+0xaa,1),special_byte=word(unit+0x10a,1),definition=word(unit),unit_forward_hex=read(unit+0x168,12).hex(),unit_position_hex=read(unit+0x64,12).hex())
    row['ordinary_on_foot']=row['parent']==0xffffffff and row['kind']==0 and not(row['special_byte']&4)
   entries.append(row)
  report.update(actual_control_census=entries,captured_before_bytes=sum(map(len,banks.values())),archive_sha256=a.archive_sha256);selected=next((r for r in entries if r.get('ordinary_on_foot')),None);assert selected,'No actual ordinary on-foot control object; do not fabricate entry'
  code=_Code(Xbe(NEW_DEFAULT),'.text');body=complete_body(code,0xcd4a0);sites=[]
  for pc in sorted(body.code):
   ins=code.at(pc);sites.append(dict(pc=pc,size=ins.size,mnemonic=ins.mnemonic,target=int(ins.op_str,16)if ins.mnemonic=='call'and ins.op_str.startswith('0x')else None))
  assert len(sites)==287
  meta=scratch/'study.json';controls=[dict(rounding_mode=n)for n in range(4)]
  if a.declared_heights:
   def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
   heights=(.1,.25,.5,2.)if a.seek_second_hit else(0.,.005,.01,.02,.04,.08,.15,.3,.6,1.,2.,4.)
   controls=[dict(rounding_mode=0,declared_height_bits=bits(h),declared_height=h,declared_blend_bits=bits(0.))for h in heights]
  tags=word(0x4f0414);tag=word(tags+(selected['definition']&0xffff)*16+8)
  report.update(offline_declared_height_controls=a.declared_heights,source_physics_writes_by_harness=a.declared_heights,live_gameplay_writes=False,tag_height_fields=dict(tag=tag,height218_bits=word(tag+0x218),height21c_bits=word(tag+0x21c),blend2c4_bits=word(selected['unit']+0x2c4)))
  save(meta,dict(sites=sites,destination=base.UI+0x2000,object=selected['object'],controls=controls,unit=selected['unit'],definition=tag,stop_after_first_query_hit=a.declared_heights,seek_second_hit=a.seek_second_hit))
  fp=capture['input']['floating_point'];case=dict(entry=0xcd4a0,receiver=selected['object'],arguments=[],user_words=[],before_fp_input=[fp['x87_control_word'],fp['x87_status'],fp['mxcsr']]);original=reference_original();original=original[:original.index('        active=dict(m')]+"        return owned/'reference'\n";original=original.replace("case['before_fp']=fp","fp=case['before_fp_input'];case['before_fp']=fp").replace(' mov ${case[\'receiver\']},%ecx\n int3',' mov ${case[\'receiver\']},%ecx\n mov $'+str(base.UI+0x2000)+',%eax\n int3')
  env=dict(base.__dict__,metadata=lambda i,b:({},{}));exec(original,env);exe=env['original'](Xbe(NEW_DEFAULT),banks,[case],scratch)
  prefix=(ROOT/'tools/observe_native_contact_producer_tu15.cpp').read_text().split('struct Memory:SourcePhysicsContactMemory')[0];prefix=prefix.replace('#include "halo2/native_tu15_contact_producer.hpp"','#include <cstdint>\n#include <span>\n#include <vector>').replace('using namespace halo2;','')
  driver=prefix+DRIVER;(scratch/'driver.cpp').write_text(driver);command=['c++','-std=c++20','-O2',str(scratch/'driver.cpp'),'-o',str(scratch/'study')];r=subprocess.run(command,capture_output=True,text=True);(a.output/'compiler.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4096:]
  r=subprocess.run([str(scratch/'study'),str(exe),str(meta),str(a.output/'original-entry-study.json')],capture_output=True,text=True,timeout=120);(a.output/'execution.log').write_text(r.stdout+r.stderr);assert r.returncode==0,(r.stdout,r.stderr);study=json.loads((a.output/'original-entry-study.json').read_text());assert study['passed']
  report.update(passed=True,selected_object=selected,declared_incoming_fp=case['before_fp_input'],declared_destination=base.UI+0x2000,query_counts=[len(v['queries'])for v in study['profiles']],positive_query_profiles=sum(any(q['actual_return_al']for q in v['queries'])for v in study['profiles']),post_query_parent_sites=sorted(set(pc for r in study['profiles']for pc in r['executed_parent_sites']if pc>=0xcd664 and pc<=0xcd7d4)),source_hashes={str(Path(__file__).resolve().relative_to(ROOT)):sha(Path(__file__)), 'tools/observe_native_contact_producer_tu15.cpp':sha(ROOT/'tools/observe_native_contact_producer_tu15.cpp')},remaining='Declared original entry study only. A native whole camera/Host proof must independently compute its queries and compare retained DATA/FP; this study provides no outcomes to it.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
DRIVER=r'''
J words(const user_fpregs_struct&f){return J::array({f.cwd,f.swd,f.mxcsr});}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto n:b){auto v=std::to_integer<unsigned>(n);s+=d[v>>4];s+=d[v&15];}return s;}
}
int main(int argc,char**argv){J report{{"passed",false},{"profiles",J::array()}};try{
 check(argc==4,"Supply original ELF, study metadata and report");std::ifstream f(argv[2]);const auto input=J::parse(f);std::map<std::uint32_t,J>sites;for(const auto&r:input.at("sites"))sites.emplace(r.at("pc"),r);
 for(const auto&control:input.at("controls")){const auto mode=control.at("rounding_mode").get<unsigned>();
  Child child;child.pid=fork();check(child.pid>=0,"Cannot fork original camera study");if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}child.wait();request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);child.wait();check(WSTOPSIG(child.status)==SIGTRAP,"Original declared caller setup failed");auto fp=child.fp();fp.cwd=(fp.cwd&~0xc00u)|(mode<<10);fp.mxcsr=(fp.mxcsr&~0x6000u)|(mode<<13);request(PTRACE_SETFPREGS,child.pid,nullptr,&fp);
  if(control.contains("declared_height_bits")){const auto put=[&](std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));child.write(p,b);};const auto definition=input.at("definition").get<std::uint32_t>();put(definition+0x218u,control.at("declared_height_bits"));put(definition+0x21cu,control.at("declared_height_bits"));put(input.at("unit").get<std::uint32_t>()+0x2c4u,control.at("declared_blend_bits"));}
  child.go(0xcd4a0u);const auto incoming=words(child.fp());J executed=J::array(),queries=J::array();bool complete{};
  for(unsigned step=0;step<2048;++step){const auto r=child.regs();const auto pc=std::uint32_t(r.rip);if(pc==0x05000000u){complete=true;break;}const auto at=sites.find(pc);check(at!=sites.end(),"Original camera escaped parent outside a genuine completed child");executed.push_back(pc);const auto&i=at->second;
   if(i.at("mnemonic")=="call"){
    const auto next=pc+i.at("size").get<unsigned>();if(!i.at("target").is_null()&&i.at("target")==0x16c620u){child.go(0x16c620u);const auto s=child.regs();J args=J::array();for(unsigned k=1;k<=6;++k)args.push_back(child.word(std::uint32_t(s.rsp)+k*4u));const auto before=words(child.fp());child.go(next);const auto result=args.at(5).get<std::uint32_t>();queries.push_back(J{{"call_site",pc},{"actual_source_arguments",args},{"before_fp",before},{"after_fp",words(child.fp())},{"actual_return_al",std::uint32_t(child.regs().rax)&255u},{"actual_result92_hex",hex(child.read(result,92))}});
    }else child.go(next);
   }else child.step();
  }
  check(complete&&std::uint32_t(child.regs().rsp)==0x0200f004u,"Original CD4A0 complete RET0 boundary not reached");report["profiles"].push_back(J{{"rounding_mode",mode},{"declared_control",control},{"declared_object",input.at("object")},{"declared_destination",input.at("destination")},{"incoming_fp",incoming},{"after_fp",words(child.fp())},{"position12_hex",hex(child.read(input.at("destination"),12))},{"queries",queries},{"executed_parent_sites",executed}});bool hit{};if(input.at("seek_second_hit"))hit=queries.size()>1&&queries.at(1).at("actual_return_al").get<unsigned>()!=0u;else for(const auto&q:queries)hit|=q.at("actual_return_al").get<unsigned>()!=0u;if(hit&&input.at("stop_after_first_query_hit"))break;
 }
 report["passed"]=true;
}catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';return report.at("passed").get<bool>()?0:1;}
'''
if __name__=='__main__':raise SystemExit(main())
