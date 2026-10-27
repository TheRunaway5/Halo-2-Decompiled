#!/usr/bin/env python3
"""Bounded genuine owner GP/private-stack input audit, not parity acceptance.

Runs under run_test.py. Uses the completed physical path inventory; adds only
read/write observation metadata and a neutral-context raw-difference inventory.
Original opcodes, data, kernel, results and strict prior reports remain intact.
"""
import argparse,difflib,hashlib,json,os,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM,X86_OP_REG
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import validate_native_island_agent_hardware_composition_tu15 as base

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def audit_metadata(image,pcs):
 code=_Code(image,'.text');aliases={}
 for index,full in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi')):
  aliases[full]=(index,0xffffffff);aliases[full[1:]]=(index,0xffff)
 for index,prefix in enumerate(('a','c','d','b')):aliases[prefix+'l']=(index,0xff);aliases[prefix+'h']=(index,0xff00)
 rows=[]
 from generate_physics_live_support_tu15 import complete_body
 pcs=set(pcs)
 for entry in(0x321d20,0x321a70,0x321180,0x321220,0x320510):pcs.update(complete_body(code,entry).code)
 for pc in sorted(pcs):
  i=code.at(pc);reads,writes=i.regs_access();rr=[i.reg_name(r)for r in reads];rw=[i.reg_name(r)for r in writes]
  if i.mnemonic in('xor','sub','pxor','xorps','xorpd')and len(i.operands)==2 and i.operands[0].type==X86_OP_REG and i.operands[1].type==X86_OP_REG and i.operands[0].reg==i.operands[1].reg:rr=[r for r in rr if r!=i.reg_name(i.operands[0].reg)]
  operands=[]
  for op in i.operands:
   if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
   if op.mem.segment and i.reg_name(op.mem.segment)=='fs':continue
   if op.mem.segment:assert i.reg_name(op.mem.segment)in ('cs','ds','es','ss')
   b,idx=i.reg_name(op.mem.base),i.reg_name(op.mem.index);access=CS_AC_WRITE if i.mnemonic.startswith(('fst','fist','fnst','fsave','fnsave'))else op.access
   assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),i.mnemonic,i.op_str)
   operands.append(dict(base=aliases[b][0]if b else-1,index=aliases[idx][0]if idx else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
  rows.append(dict(instruction=pc,mnemonic=i.mnemonic,assembly=i.op_str,gp_reads=[aliases[r]for r in rr if r in aliases],gp_writes=[aliases[r]for r in rw if r in aliases],other_reads=[r for r in rr if r not in aliases],operands=operands))
 return rows

INJECT=r'''
struct InputAudit {
 std::map<std::uint32_t,J>prior_writes;std::array<std::uint32_t,8>written{};std::map<std::uint32_t,J>gp;std::set<std::uint32_t>stack_written;std::map<std::uint32_t,J>stack;std::set<std::string>other;std::uint32_t entry_sp{};std::size_t missing{};
 void instruction(Child&child,const J&i,const user_regs_struct&r){
  const auto pc=std::uint32_t(r.rip);const std::array<std::uint32_t,8>values={std::uint32_t(r.rax),std::uint32_t(r.rcx),std::uint32_t(r.rdx),std::uint32_t(r.rbx),std::uint32_t(r.rsp),std::uint32_t(r.rbp),std::uint32_t(r.rsi),std::uint32_t(r.rdi)};
  for(const auto&a:i.at("gp_reads")){const auto reg=a[0].get<unsigned>(),mask=a[1].get<std::uint32_t>()&~written[reg];if(mask){if(!gp.contains(reg))gp[reg]=J{{"register",reg},{"first_instruction",pc},{"first_assembly",i.at("mnemonic").get<std::string>()+" "+i.at("assembly").get<std::string>()},{"read_mask",mask},{"initial_value",values[reg]}};else gp[reg]["read_mask"]=gp[reg]["read_mask"].get<std::uint32_t>()|mask;}}
  for(const auto&name:i.at("other_reads"))other.insert(name.get<std::string>());
  std::vector<std::pair<std::uint32_t,std::size_t>>write_ranges;
  const auto access=[&](std::uint32_t p,std::size_t n,bool read,bool write){if(std::uint64_t(p)+n>SP+0x100u||p<SP-0x4000u)return;if(read){std::vector<std::byte>raw(n);child.read(p,raw);for(std::size_t b=0;b<n;++b){const auto address=p+b;if(!stack_written.contains(address)&&!stack.contains(address))stack[address]=J{{"address",address},{"relative_to_entry_sp",std::int64_t(address)-entry_sp},{"first_instruction",pc},{"first_assembly",i.at("mnemonic").get<std::string>()+" "+i.at("assembly").get<std::string>()},{"value",std::to_integer<unsigned>(raw[b])},{"prior_write_before_owner_entry",prior_writes.contains(address)?prior_writes.at(address):J(nullptr)},{"copy_destination",i.at("mnemonic").get<std::string>().find("movs")!=std::string::npos&&p==std::uint32_t(r.rsi)?J(std::uint32_t(r.rdi)+b):J(nullptr)}};}}if(write)write_ranges.emplace_back(p,n);};
  for(const auto&a:i.at("operands")){const auto p=operand(a,r);std::size_t n=a.at("bytes");const auto name=i.at("mnemonic").get<std::string>();if(name.starts_with("rep ")){n*=std::uint32_t(r.rcx);check(!(r.eflags&0x400),"Backward string audit needs its explicit owner");}access(p,n,a.at("read"),a.at("write"));}
  const auto name=i.at("mnemonic").get<std::string>();if(name=="push"||name=="pushfd"||name=="call")access(std::uint32_t(r.rsp)-4,4,false,true);else if(name=="pop"||name=="popfd"||name=="ret")access(std::uint32_t(r.rsp),4,true,false);else if(name=="leave")access(std::uint32_t(r.rbp),4,true,false);
  for(const auto&[p,n]:write_ranges)for(std::size_t b=0;b<n;++b)stack_written.insert(p+b);for(const auto&a:i.at("gp_writes"))written[a[0].get<unsigned>()]|=a[1].get<std::uint32_t>();
 }
 J finish(std::uint32_t caller,std::uint32_t owner){J gs=J::array(),ss=J::array();for(const auto&[r,j]:gp)gs.push_back(j);for(const auto&[p,j]:stack)ss.push_back(j);return J{{"caller",caller},{"owner",owner},{"entry_sp",entry_sp},{"initial_gp_read_before_write",gs},{"private_bytes_read_before_write",ss},{"register_reads_including_defined_internal_values",other},{"unqualified_observed_instructions",missing}};}
};
'''
LIFETIME='\nstruct Lifetime {\n const std::map<std::uint32_t,J>&sites;std::map<std::uint32_t,J>history;J selected_accesses=J::array(),copies=J::array(),gates=J::array();bool selected_gate{};std::vector<std::pair<std::uint32_t,std::size_t>>pending;std::uint32_t pc{};J info;std::size_t missing{};\n explicit Lifetime(const std::map<std::uint32_t,J>&s):sites(s){}\n void before(Child&child){const auto r=child.regs();pc=std::uint32_t(r.rip);pending.clear();if(pc==0x304858u)selected_gate=std::uint32_t(r.rsi+r.rax)==0x10082500u;else if(pc==0x3049a0u)selected_gate=std::uint32_t(r.rax)==0x10082500u;auto it=sites.find(pc);if(it==sites.end()){++missing;info=nullptr;return;}info=it->second;\n  const auto write=[&](std::uint32_t p,std::size_t n){if(p>=SP-0x4000u&&std::uint64_t(p)+n<=SP+0x100u)pending.emplace_back(p,n);};\n  for(const auto&a:info.at("operands")){const auto address=operand(a,r);const auto n=a.at("bytes").get<std::size_t>();if(info.at("mnemonic").get<std::string>().starts_with("rep ")&&!std::uint32_t(r.rcx))continue;if(a.at("write").get<bool>())write(address,n);if(address<0x10082580u&&std::uint64_t(address)+n>0x1008257cu){std::vector<std::byte>raw(n);child.read(address,raw);J values=J::array();for(auto b:raw)values.push_back(std::to_integer<unsigned>(b));selected_accesses.push_back(J{{"instruction",pc},{"assembly",info.at("mnemonic").get<std::string>()+" "+info.at("assembly").get<std::string>()},{"address",address},{"bytes",n},{"read",a.at("read")},{"write",a.at("write")},{"before",values}});}}\n  const auto name=info.at("mnemonic").get<std::string>();if(name=="push"||name=="pushfd"||name=="call")write(std::uint32_t(r.rsp)-4,4);\n  if(pc==0x2e2e30u){std::array<std::byte,16>raw{};child.read(r.rsp,raw);std::array<std::uint32_t,4>words{};std::memcpy(words.data(),raw.data(),16);if(words[3]&&words[3]<=0x4000u&&words[2]>=SP-0x4000u&&words[2]+words[3]<=SP+0x100u){std::vector<std::byte>source(words[3]);child.read(words[2],source);J values=J::array(),last=J::array();for(auto b:source)values.push_back(std::to_integer<unsigned>(b));for(unsigned record=0;record+0x80u<=words[3];record+=0x80u)for(unsigned n=record+0x7c;n<record+0x80u;++n)last.push_back(J{{"offset",n},{"last_source_write",history.contains(words[2]+n)?history.at(words[2]+n):J(nullptr)}});copies.push_back(J{{"copy_entry",pc},{"actual_caller_return",words[0]},{"source",words[2]},{"destination",words[1]},{"bytes",words[3]},{"source_bytes",values},{"source_tail_write_history",last}});}}\n }\n void after(Child&child){if(info.is_null())return;if(selected_gate&&(pc==0x304862u||pc==0x3049a5u))gates.push_back(J{{"instruction",pc},{"taken_next",std::uint32_t(child.regs().rip)},{"record",0x10082500u}});for(const auto&[p,n]:pending){std::vector<std::byte>raw(n);child.read(p,raw);for(std::size_t b=0;b<n;++b)history[p+b]=J{{"instruction",pc},{"assembly",info.at("mnemonic").get<std::string>()+" "+info.at("assembly").get<std::string>()},{"value",std::to_integer<unsigned>(raw[b])}};}}\n};\n'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--inventory',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();inventory=json.loads(a.inventory.read_text());assert inventory['passed']and inventory['observed_owner_pcs'];assert sha(NEW_DEFAULT)==NEW_PIN
 template=ROOT/'tests/native_tu15_island_agent_hardware_composition_tests.cpp';text=template.read_text();text=text.replace('\n}\nint main', '\n'+INJECT+LIFETIME+'\n}\nint main',1);text=text.replace('std::set<std::uint32_t>owner_pcs;', 'std::set<std::uint32_t>owner_pcs;std::map<std::uint32_t,J>audit_sites;for(const auto&i:meta.at("audit_sites"))audit_sites.emplace(i.at("instruction"),i);J input_audits=J::array();J neutral_raw_differences=J::array();J lifetimes=J::array();')
 text=text.replace('const auto ret=args[0];', 'const auto ret=args[0];InputAudit audit;audit.entry_sp=std::uint32_t(callee.rsp);audit.prior_writes=lifetime.history;')
 text=text.replace('bool observe=true;', 'bool observe=true;Lifetime lifetime(audit_sites);')
 text=text.replace('owner_pcs.insert(std::uint32_t(reference.regs().rip));check', 'const auto state=reference.regs();const auto observed_pc=std::uint32_t(state.rip);owner_pcs.insert(observed_pc);const auto decoded=audit_sites.find(observed_pc);if(decoded==audit_sites.end())++audit.missing;else audit.instruction(reference,decoded->second,state);check')
 text=text.replace('reference.wait(SIGTRAP);}continue;', 'reference.wait(SIGTRAP);}input_audits.push_back(audit.finish(pc,std::uint32_t(callee.rip)));continue;')
 text=text.replace('reference.request(PTRACE_SINGLESTEP);','lifetime.before(reference);reference.request(PTRACE_SINGLESTEP);')
 text=text.replace('reference.wait(SIGTRAP);','reference.wait(SIGTRAP);lifetime.after(reference);')
 text=text.replace('for(auto&s:pending){reference.read', 'lifetime.after(reference);for(auto&s:pending){reference.read')
 text=text.replace('results.push_back(J{{', 'lifetimes.push_back(J{{"case",name},{"private_source_copies",lifetime.copies},{"selected_record_accesses",lifetime.selected_accesses},{"selected_validity_gates",lifetime.gates},{"unqualified_original_sites",lifetime.missing}});results.push_back(J{{')
 # Neutral transport is measured, not accepted. Complete strict authenticated
 # context proof remains in its existing report, with no changes or waivers.
 text=text.replace('dependencies.enter(owner,receiver,args,call.at("stack"),state,call);dependencies.put(native);','dependencies.enter(owner,receiver,args,call.at("stack"),state);')
 old='for(const auto&[p,r]:actual.regions)if(r.data!=native.regions.at(p).data){std::size_t n{};while(n<r.data.size()&&r.data[n]==native.regions.at(p).data[n])++n;throw std::runtime_error(name+" full original persistent bank differs at "+std::to_string(p+n));}'
 new='J differences=J::array();for(const auto&[p,r]:actual.regions){const auto&bank=native.regions.at(p).data;for(std::size_t n=0;n<r.data.size();++n)if(r.data[n]!=bank[n])differences.push_back(J{{"address",p+n},{"original",std::to_integer<unsigned>(r.data[n])},{"neutral",std::to_integer<unsigned>(bank[n])}});}neutral_raw_differences.push_back(J{{"case",name},{"raw_equal",differences.empty()},{"bytes",differences}});'
 assert text.count(old)==1;text=text.replace(old,new);text=text.replace('{"passed",true},{"cases",results}', '{"passed",false},{"diagnostic_completed",true},{"neutral_context_raw_comparison_passed",false},{"caller_input_audits",input_audits},{"source_lifetimes",lifetimes},{"neutral_raw_differences",neutral_raw_differences},{"cases",results}')
 generated=scratch/'audit.cpp';generated.write_text(text);(a.output/'test-adaptation.diff').write_text(''.join(difflib.unified_diff(template.read_text().splitlines(True),text.splitlines(True),fromfile=str(template.relative_to(ROOT)),tofile='owned-private-input-audit.cpp')))
 # Reuse the official lifecycle/ELF setup; the generated worker adds metadata
 # and a bounded subset of cases. Both compilers still run CMake+CTest.
 source=(ROOT/'tools/validate_native_island_agent_hardware_composition_tu15.py').read_text();source=source.replace("test=ROOT/'tests/native_tu15_island_agent_hardware_composition_tests.cpp'",f'test=Path({str(generated)!r})');source=source.replace("str(f.relative_to(ROOT))", "str(f.relative_to(ROOT)if f.is_relative_to(ROOT)else f)")
 source=source.replace("'--target','native_agent_counter_mutation',",'');source=source.replace("'--filter','^native_agent_hardware'", "'--filter','^native_agent_hardware$'")
 source=source.replace("meta=metadata(image);meta.update", "meta=metadata(image);meta['audit_sites']=json.loads(Path("+repr(str(scratch/'audit-sites.json'))+").read_text());cases=[c for c in cases if c['name']in ('Gemini_0_321a70_rank_None','Gemini_0_321a70_rank_1','Gemini_1_321a70_rank_None','Gemini_1_321a70_rank_1')];meta.update")
 (scratch/'audit-sites.json').write_text(json.dumps(audit_metadata(Xbe(NEW_DEFAULT),inventory['observed_owner_pcs'])));worker=scratch/'worker.py';worker.write_text(source);command=[sys.executable,str(worker),'--output',str(a.output/'execution')];environment=dict(os.environ,PYTHONPATH=str(ROOT/'tools'));r=subprocess.run(command,env=environment);report=dict(diagnostic_completed=r.returncode==0,parity_acceptance=False,neutral_context_raw_gate_passed=False,command=command,original_sha256=NEW_PIN,inventory=str(a.inventory),inventory_sha256=sha(a.inventory),source_hashes={str(template.relative_to(ROOT)):sha(template),str(Path(__file__).relative_to(ROOT)):sha(__file__)},generated_audit_sha256=sha(generated),generated_worker_sha256=sha(worker),metadata_sha256=sha(scratch/'audit-sites.json'),input_instruction_sites=len(inventory['observed_owner_pcs']),owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return r.returncode
if __name__=='__main__':raise SystemExit(main())
