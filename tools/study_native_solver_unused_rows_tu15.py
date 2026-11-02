#!/usr/bin/env python3
"""Original-only lifetime reads/retained-pointer stores after30DE count1 copies.

This diagnostic never provides private bytes, GP state or outcomes to a native
kernel. It watches Source-defined work identity through complete2E return.
"""
import inspect,json,sys,hashlib
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import probe_native_moving_shape_process_full_original_tu15 as full
from oracle_smg_firing_1_1 import _Code
ROOT=Path(__file__).resolve().parents[1]
GRAPH=ROOT/'analysis/physics/native-moving-shape-process-full-original1479-20261102-second/g++-actual.json'
ORIGINAL_METADATA=full.graph.actual.parent.metadata
def metadata(image,output):
 result=ORIGINAL_METADATA(image,output);code=_Code(image,'.text');sites={};registers={v:n for n,v in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
 for profile in json.loads(GRAPH.read_text())['profiles']:
  for executed in profile['executed_unique_solver_sites']:
   pc=executed['instruction'];ins=code.at(pc);assert image.read(pc,15).hex()==executed['original_bytes15'];operands=[]
   if ins.mnemonic!='lea':
    for op in ins.operands:
     if op.type!=X86_OP_MEM:continue
     segment=ins.reg_name(op.mem.segment)
     # FS references are the separately owned normal-SEH chain, outside work.
     if segment in ('fs','gs'):continue
     access=CS_AC_READ if ins.mnemonic in ('test','cmp')else CS_AC_WRITE if ins.mnemonic in ('fst','fstp','fistp')else op.access
     operands.append(dict(base=registers.get(ins.reg_name(op.mem.base),-1),index=registers.get(ins.reg_name(op.mem.index),-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites[pc]=dict(instruction=pc,operands=operands)
 result['sites']=list(sites.values());return result
def observer_source():
 text=full.observer_source()
 text=text.replace('const auto capture=J::parse(input);const auto&typed=', 'const auto capture=J::parse(input);std::ifstream metadata_file(argv[3]);const auto metadata=J::parse(metadata_file);std::map<std::uint32_t,J>operands;for(const auto&site:metadata.at("sites"))operands.emplace(site.at("instruction"),site);const auto&typed=')
 text=text.replace('unsigned instructions{};bool solver_entered{};', 'unsigned instructions{};std::uint32_t work{};bool copied{};J watched_reads=J::array(),retained_pointers=J::array(),copy_sites=J::array();bool solver_entered{};')
 anchor='   const auto opcode=child.read(pc,15);'
 insert='''   if(pc==0x30de60u){work=std::uint32_t(regs.rcx);check(!copied,"Repeated30DE lifetime requires separate owner");}
   const auto overlap=[](std::uint32_t a,unsigned n,std::uint32_t b,unsigned bytes){return std::uint64_t(a)<std::uint64_t(b)+bytes&&std::uint64_t(b)<std::uint64_t(a)+n;};
   const auto retained=[&](std::uint32_t p,unsigned n){for(const auto&r:capture.at("regions")){const auto begin=r.at("address").get<std::uint32_t>();if(p>=begin&&std::uint64_t(p-begin)+n<=r.at("size").get<std::uint32_t>())return true;}return false;};
   J pending=J::array();
   if(copied){const auto found=operands.find(pc);check(found!=operands.end(),"Original future path escaped authenticated operand census");const auto current=gp(regs);for(const auto&op:found->second.at("operands")){auto address=op.at("displacement").get<std::uint32_t>();const auto base=op.at("base").get<int>(),index=op.at("index").get<int>();if(base>=0)address+=current.at(base);if(index>=0)address+=current.at(index)*op.at("scale").get<std::uint32_t>();const auto width=op.at("bytes").get<unsigned>();if(op.at("read").get<bool>())for(auto offset:{0x30u,0x40u,0xf0u,0x100u})if(overlap(address,width,work+offset,16))watched_reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",width},{"work_offset",offset}});if(op.at("write").get<bool>()&&retained(address,width))pending.push_back(J{{"instruction",pc},{"address",address},{"bytes",width}});}}
'''
 assert text.count(anchor)==1;text=text.replace(anchor,insert+anchor)
 anchor='if(solver_entered&&call)calls.push_back'
 insert='''if(work&&pc==0x30e067u){copied=true;copy_sites=J::array({0x30e02du,0x30e036u,0x30e04eu,0x30e067u});}
   for(const auto&store:pending){const auto raw=child.read(store.at("address"),store.at("bytes"));for(unsigned offset=0;offset+4u<=raw.size();offset+=4u){std::uint32_t value{};std::memcpy(&value,raw.data()+offset,4);if(value>=work&&value-work<0x140u)retained_pointers.push_back(J{{"instruction",pc},{"address",store.at("address").get<std::uint32_t>()+offset},{"work_offset",value-work}});}}
   '''
 assert text.count(anchor)==1;text=text.replace(anchor,insert+anchor)
 text=text.replace('{"original_solver_return",result}', '{"work_identity",work},{"unprovided_copy_sites",copy_sites},{"future_reads_of_unprovided_rows",watched_reads},{"retained_stores_of_work_pointers",retained_pointers},{"tracked_until_full_process_return",copied},{"original_solver_return",result}')
 return text
def main():
 full.graph.actual.parent.metadata=metadata
 source=inspect.getsource(full.main);namespace=dict(full.__dict__,observer_source=observer_source,__doc__=__doc__);exec(source,namespace);rc=namespace['main']()
 output=Path(sys.argv[sys.argv.index('--output')+1]);path=output/'report.json';report=json.loads(path.read_text());report['source_only_future_use_audit']=True;report['no_unprovided_bytes_retained_or_supplied_to_native']=True;report['source_hashes'][str(Path(__file__).relative_to(ROOT))]=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();report['source_pc_census_sha256']=hashlib.sha256(GRAPH.read_bytes()).hexdigest();report['native_parent_qualified']=False;path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())
