#!/usr/bin/env python3
"""Execute unchanged original30E930 on a genuine typed native BEFORE input.

This diagnoses the source continuation only. No original result is supplied to
the native kernel. Raw banks/compiler products stay in owned scratch; retain
only source-site/call counts, small semantic outputs and immutable authority.
"""
import argparse,hashlib,inspect,json,os,struct,subprocess,tarfile,shutil
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',action='store_true',help='Also compare the native kernel on an independently loaded BEFORE, without supplying any original result');p.add_argument('--qualify',action='store_true',help='Compare ordered retained operands/stores, four declared rounding controls, GCC/Clang and rejected semantic mutants');a=p.parse_args();assert not a.qualify or a.native;a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,scope=__doc__,owned_scratch=str(scratch),retained_large_artifacts=[],native_result_supplied=False)
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
  if a.native:
   driver='#include "halo2/native_tu15_convex_simplex_solver_binding.hpp"\n'+driver
   old=' const auto fp=child.fp();result='
   assert driver.count(old)==1
   native=r'''
 const auto expected_fp=child.fp();
 Child native;native.pid=fork();check(native.pid>=0,"Cannot start native BEFORE backing");if(!native.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
 native.wait();request(PTRACE_SETOPTIONS,native.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,native.pid);native.wait();check(WSTOPSIG(native.status)==SIGTRAP,"Native BEFORE setup failed");
 struct View:halo2::SourcePhysicsContactMemory {
  Child&process;std::array<std::byte,4096>private_data{};static constexpr std::uint32_t first=0x6000000u;
  explicit View(Child&p):process(p){private_data.fill(std::byte{0xa5});}
  bool owned(std::uint32_t p,std::size_t n)const{return p>=first&&std::uint64_t(p-first)+n<=private_data.size();}
  void read(std::uint32_t p,std::span<std::byte>b)const override{if(owned(p,b.size()))std::copy_n(private_data.begin()+p-first,b.size(),b.begin());else{const auto source=process.read(p,b.size());std::copy(source.begin(),source.end(),b.begin());}}
  void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
  void write(std::uint32_t p,std::span<const std::byte>b)override{if(owned(p,b.size()))std::copy(b.begin(),b.end(),private_data.begin()+p-first);else process.write(p,b);}
 }storage(native);
 halo2::NativeTu15ConvexSimplexMemory guarded(storage);const auto&controls=typed.at("floating_point");
 halo2::NativeTu15PhysicsFrameFp state{controls.at("x87_control_word"),controls.at("x87_status"),controls.at("mxcsr")};
 constexpr auto q=0x6000000u;const halo2::NativeTu15ContactShapeScratch child_shape{q+0x900u,q+0x910u,nullptr};
 halo2::NativeTu15ConvexSimplexBindingActivity activity;
 auto bound=halo2::native_tu15_make_bound_convex_contact_services(guarded,
  {{q+0xe00u,q+0xe04u,q+0xe08u,q+0xe10u,q+0xe20u,{q+0xa00u,q+0xb00u,q+0xb40u,{q+0x400u,{q+0x200u,q+0x204u,{q,q+0x10u,q+0x30u,q+0x60u,q+0x20u}}}}},{q+0x800u,q+0x810u,&child_shape}},0,{},&activity);
 const auto native_result=bound.solve_simplex(typed.at("work"),{typed.at("first")},{typed.at("second")},typed.at("relative_transform"),typed.at("support_cache"),typed.at("plane"),state);
 std::uint64_t compared{},differences{};J first_differences=J::array();
 for(const auto&region:request_data.at("regions")){const auto address=region.at("address").get<std::uint32_t>(),bytes=region.at("size").get<std::uint32_t>();const auto expected=child.read(address,bytes),actual=native.read(address,bytes);compared+=bytes;for(unsigned i=0;i<bytes;++i)if(expected[i]!=actual[i]){++differences;if(first_differences.size()<16)first_differences.push_back(J{{"address",address+i},{"source",std::to_integer<unsigned>(expected[i])},{"native",std::to_integer<unsigned>(actual[i])}});}}
 result["native_comparison"]={{"return_word",native_result},{"compared_bytes",compared},{"different_bytes",differences},{"first_differences",first_differences},{"fp",J::array({state.control_word,state.status,state.mxcsr})},{"expected_fp",J::array({expected_fp.cwd,expected_fp.swd,expected_fp.mxcsr})},{"native_solver_calls",activity.completed}};
 check(native_result==std::uint32_t(child.regs().rax),"Native projection result differs");
 check(differences==0,"Native projection retained banks differ: "+first_differences.dump());
 check(state.control_word==expected_fp.cwd&&state.status==expected_fp.swd&&state.mxcsr==expected_fp.mxcsr,"Native projection FP differs");
 check(activity.completed==1&&activity.refusals==0&&guarded.refused_reads()==0,"Native projection ownership differs");
 const auto comparison=result.at("native_comparison");
'''
   # A local class cannot own a static data member; use the literal in its
   # address predicate while retaining the ordinary private DATA identity.
   native=native.replace('static constexpr std::uint32_t first=0x6000000u;','').replace('p>=first&&std::uint64_t(p-first)','p>=0x6000000u&&std::uint64_t(p-0x6000000u)').replace('p-first','p-0x6000000u')
   driver=driver.replace(old,native+old)
   old='{"all_executed_instruction_bytes_authenticated",true}};'
   assert driver.count(old)==1;driver=driver.replace(old,'{"all_executed_instruction_bytes_authenticated",true},{"native_comparison",comparison}};')
  if a.qualify:
   # Metadata identifies original operands; execution still runs and
   # authenticates the unchanged source instructions, never an emulator.
   saved_owners=base.OWNERS
   try:
    base.OWNERS=(0x30e930,0x3079c0,0x2e8d40,0x3077f0)
    operands,authority=base.metadata(image,merged)
   finally:base.OWNERS=saved_owners
   marker=' const auto first=request_data.at("text_first").get<std::uint32_t>();'
   assert driver.count(marker)==1
   tracking=r'''
 const auto&input_fp=request_data.at("input").at("floating_point");auto initial_fp=child.fp();
 initial_fp.cwd=input_fp.at("x87_control_word");initial_fp.swd=input_fp.at("x87_status");initial_fp.mxcsr=input_fp.at("mxcsr");
 request(PTRACE_SETFPREGS,child.pid,nullptr,&initial_fp);
 const auto captured=[&](std::uint32_t p,std::size_t n){return std::any_of(request_data.at("regions").begin(),request_data.at("regions").end(),[&](const J&r){const auto a=r.at("address").get<std::uint32_t>(),z=r.at("size").get<std::uint32_t>();return p>=a&&std::uint64_t(p-a)+n<=z;});};
 std::map<std::uint32_t,J>sites;for(const auto&s:request_data.at("metadata").at("sites"))sites.emplace(s.at("instruction"),s);
 const auto bytes_hex=[](const auto&data){constexpr char d[]="0123456789abcdef";std::string out;for(auto b:data){auto n=std::to_integer<unsigned>(b);out+=d[n>>4];out+=d[n&15];}return out;};
 J ordered_reads=J::array(),ordered_stores=J::array();
 for(const auto&t:request_data.at("dispatch_tables"))check(bytes_hex(child.read(t.at("address"),t.at("bytes").get<std::string>().size()/2u))==t.at("bytes").get<std::string>(),"Original projection dispatch table differs from authenticated XBE");
'''
   driver=driver.replace(marker,tracking+marker)
   marker='  path.push_back(pc);++counts[pc];child.step();'
   assert driver.count(marker)==1
   tracking=r'''
  const auto site=sites.find(pc);check(site!=sites.end(),"Original projection left its owner closure");const auto r=child.regs();const std::array<std::uint32_t,8>values{std::uint32_t(r.rax),std::uint32_t(r.rcx),std::uint32_t(r.rdx),std::uint32_t(r.rbx),std::uint32_t(r.rsp),std::uint32_t(r.rbp),std::uint32_t(r.rsi),std::uint32_t(r.rdi)};J pending=J::array();
  for(const auto&op:site->second.at("operands")){
   auto p=op.at("displacement").get<std::uint32_t>();const auto base=op.at("base").get<int>(),index=op.at("index").get<int>();if(base>=0)p+=values.at(base);if(index>=0)p+=values.at(index)*op.at("scale").get<unsigned>();const auto n=op.at("bytes").get<unsigned>();
   if(pc==0x30ea30u||pc==0x30ea37u||pc==0x3078a2u||pc==0x3078a9u)continue; // authenticated instruction dispatch tables
   if(pc==0x2e8d66u&&values[1]==0u)continue; // REP with ECX=0 performs no DATA access
   if(captured(p,n)){
    if(op.at("read").get<bool>())ordered_reads.push_back(J::array({p,n}));
    if(op.at("write").get<bool>())pending.push_back(J{{"address",p},{"before",bytes_hex(child.read(p,n))},{"size",n}});
   }
  }
  path.push_back(pc);++counts[pc];child.step();for(auto row:pending){row["written"]=bytes_hex(child.read(row.at("address"),row.at("size")));row.erase("size");ordered_stores.push_back(row);}
'''
   driver=driver.replace(marker,tracking)
   driver=driver.replace('  Child&process;std::array<std::byte,4096>private_data{};', '  Child&process;const std::function<bool(std::uint32_t,std::size_t)>&captured;mutable J reads=J::array();J stores=J::array();std::array<std::byte,4096>private_data{};')
   driver=driver.replace('explicit View(Child&p):process(p)', 'explicit View(Child&p,const std::function<bool(std::uint32_t,std::size_t)>&c):process(p),captured(c)')
   driver=driver.replace('std::copy(source.begin(),source.end(),b.begin());}}', 'std::copy(source.begin(),source.end(),b.begin());if(captured(p,b.size()))reads.push_back(J::array({p,b.size()}));}}')
   marker='else process.write(p,b);}'
   assert driver.count(marker)==1
   driver=driver.replace(marker,'else{if(captured(p,b.size())){const auto old=process.read(p,b.size());const auto hex=[](const auto&d){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:d){auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;};stores.push_back(J{{"address",p},{"before",hex(old)},{"written",hex(b)}});}process.write(p,b);}}')
   driver=driver.replace(' }storage(native);',' };const std::function<bool(std::uint32_t,std::size_t)>retained=captured;View storage(native,retained);')
   marker=' const auto comparison=result.at("native_comparison");'
   assert driver.count(marker)==1
   driver=driver.replace(marker,r'''
 const auto first_difference=[](const J&a,const J&b){J out=J::object();for(std::size_t n=0;n<std::max(a.size(),b.size());++n)if(n>=a.size()||n>=b.size()||a[n]!=b[n]){out={{"index",n},{"native",n<a.size()?a[n]:J()},{"original",n<b.size()?b[n]:J()}};break;}return out;};
 result["native_comparison"]["ordered_reads"]=storage.reads.size();result["native_comparison"]["ordered_stores"]=storage.stores.size();
 result["native_comparison"]["read_difference"]=first_difference(storage.reads,ordered_reads);result["native_comparison"]["store_difference"]=first_difference(storage.stores,ordered_stores);
 check(storage.reads==ordered_reads,"Native projection ordered reads differ: "+first_difference(storage.reads,ordered_reads).dump());
 check(storage.stores==ordered_stores,"Native projection ordered stores differ: "+first_difference(storage.stores,ordered_stores).dump());
 const auto comparison=result.at("native_comparison");
''')
  (scratch/'driver.cpp').write_text(driver);request=scratch/'request.json';request.write_text(json.dumps(dict(text_first=section.va,input=typed,regions=[dict(address=p,size=len(raw))for p,raw in merged.items()])))
  if a.qualify:
   request_data=json.loads(request.read_text());request_data['metadata']=operands;request_data['dispatch_tables']=[dict(address=p,bytes=image.read(p,n).hex())for p,n in((0x30f6e0,44),(0x30f70c,25),(0x307988,28),(0x3079a4,15))];request.write_text(json.dumps(request_data));report['source_authority']=authority
  units=[]
  if a.native:
   import validate_native_convex_simplex_solver_actual_tu15 as core
   units=[str(ROOT/'src/physics'/n)for n in dict.fromkeys(core.UNITS)]
  primary=None;profiles=[];negative=[]
  report['source_hashes']={str(Path(n).relative_to(ROOT)):sha(n)for n in units}
  for compiler in (('g++','clang++')if a.qualify else('c++',)):
   assert shutil.which(compiler),compiler
   executable=scratch/('probe-'+compiler)
   command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),str(scratch/'driver.cpp'),*units,'-o',str(executable)]
   r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-3000:]
   for rounding in (range(4)if a.qualify else(None,)):
    if rounding is not None:
     controls=dict(typed['floating_point']);controls['x87_control_word']=(controls['x87_control_word']&~0xc00)|(rounding<<10);controls['mxcsr']=(controls['mxcsr']&~0x6000)|(rounding<<13);request_data['input']={**typed,'floating_point':controls};request.write_text(json.dumps(request_data))
    label=compiler+('-rounding-'+str(rounding)if rounding is not None else'')
    result=a.output/(label+'.json');r=subprocess.run([str(executable),str(exe),str(scratch/'text.bin'),str(request),str(result)],capture_output=True,text=True,timeout=120);(a.output/(label+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-3000:]
    current=json.loads(result.read_text());profiles.append(dict(compiler=compiler,declared_rounding_control=rounding,result_sha256=sha(result),comparison=current.get('native_comparison')))
    if primary is None:primary=result
   if a.qualify:
    request_data['input']=typed;request.write_text(json.dumps(request_data));kernel=ROOT/'src/physics/native_tu15_convex_simplex_solver.cpp';text=kernel.read_text()
    mutations={
     'zero_epsilon':('0x2edbe6ffu','0u'),
     'wrong_edge_axis':('subtract(outer_a,outer_b)','add(outer_a,outer_b)'),
     'wrong_iteration_parity':('std::uint32_t iteration{},result{}','std::uint32_t iteration{1u},result{}'),
     'wrong_simplex_growth':('input.work,current+1u);return true;','input.work,current+2u);return true;'),
    }
    for name,(old,new)in mutations.items():
     assert text.count(old)==1,name;mutant=scratch/(name+'.cpp');mutant.write_text(text.replace(old,new));target=scratch/(compiler+'-'+name);changed=[str(mutant)if n==str(kernel)else str(target)if n==str(executable)else n for n in command];changed.insert(1,'-I'+str(ROOT/'src/physics'))
     b=subprocess.run(changed,capture_output=True,text=True);(a.output/(compiler+'-'+name+'-build.log')).write_text(b.stdout+b.stderr);assert b.returncode==0,b.stderr[-3000:]
     result=a.output/(compiler+'-'+name+'.json');r=subprocess.run([str(target),str(exe),str(scratch/'text.bin'),str(request),str(result)],capture_output=True,text=True,timeout=120);(a.output/(compiler+'-'+name+'.log')).write_text(r.stdout+r.stderr);assert r.returncode!=0 and result.exists()and not json.loads(result.read_text())['passed'],name+' survived'
     negative.append(dict(compiler=compiler,mutant=name,rejected=True,result_sha256=sha(result)))
  for name,pin in report['source_hashes'].items():assert sha(ROOT/name)==pin,'Source changed during qualification: '+name
  result=a.output/'original.json';shutil.copyfile(primary,result)
  observed=json.loads(result.read_text());path=observed.pop('path');code=_Code(image,'.text');owner=complete_body(code,0x30e930);calls=[]
  for pc in dict.fromkeys(path):
   ins=code.at(pc)
   if ins.mnemonic=='call':calls.append(dict(pc=hex(pc),assembly=ins.op_str,count=path.count(pc),actual_targets=sorted({path[n+1]for n,p in enumerate(path[:-1])if p==pc})))
  observed['calls']=calls;observed['solver_sites_executed']=len(set(path)&set(owner.code));result.write_text(json.dumps(observed,indent=2)+'\n')
  listing=''.join(f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(set(path)));(a.output/'executed-original.txt').write_text(listing)
  report.update(passed=True,native_compared=a.native,original_sha256=NEW_PIN,archive_sha256=receipt['archive_sha256'],input=typed,all_instruction_bytes_unchanged=True,source_instructions=observed['instructions'],solver_sites_executed=observed['solver_sites_executed'],return_word=observed['return_word'],result_sha256=sha(result),listing_sha256=hashlib.sha256(listing.encode()).hexdigest(),capture_gp_abi_consumed=False,tool_sha256=sha(__file__),strict_qualification=a.qualify,profiles=profiles,mutants=negative,native_full_1020_site_domain=False)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
