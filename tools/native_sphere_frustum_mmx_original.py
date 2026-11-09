"""Full opaque MMX observations for the unchanged physical sphere classifier.

The shared runner and original .text remain unchanged. An owned i386 bridge
loads declared FSAVE input and snapshots actual full tags/eight80 payloads with
FNSAVE/FRSTOR. Probe code is outside the original instruction image; it restores
GP/EFLAGS and FP before the original resumes and supplies no source outcome.
"""
import hashlib,inspect
from pathlib import Path
import validate_native_tu15_prt_helpers as shared

PROBE=0x64000000
BRIDGE='''.org 0x100
frstor 0x64000000
int3
.org 0x120
fnsave 0x64000100
frstor 0x64000100
int3
'''
SEED='''
check(c.contains("active_mmx")&&depth==0&&top==0,"MMX oracle requires declared TOP0/depth0");
std::array<std::byte,108> declared_mmx{};
const auto input_word=[&](unsigned offset,std::uint16_t value){std::memcpy(declared_mmx.data()+offset,&value,2);};
input_word(0,fp.cwd);input_word(4,fp.swd);input_word(8,0);
for(unsigned n=0;n<8;++n){const auto raw=bytes(c.at("active_mmx").at("physical")[n]);check(raw.size()==10,"Invalid opaque MMX input");std::memcpy(declared_mmx.data()+28+n*10,raw.data(),10);}
memory(child.pid,0x64000000u,declared_mmx);
const auto declared_regs=regs;auto initialization_regs=regs;initialization_regs.rip=0x70000100u;initialization_regs.eflags&=~0x100u;
request(PTRACE_SETREGS,child.pid,nullptr,&initialization_regs);request(PTRACE_CONT,child.pid);wait();
check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"MMX declared-input bridge failed");
request(PTRACE_SETREGS,child.pid,nullptr,const_cast<user_regs_struct*>(&declared_regs));
'''
SNAPSHOT='''
unsigned mmx_snapshots{};std::optional<std::uint16_t>mmx_entry_tag;
const auto full_mmx=[&]{
 user_regs_struct original_regs{};request(PTRACE_GETREGS,child.pid,nullptr,&original_regs);
 auto probe_regs=original_regs;probe_regs.rip=0x70000120u;probe_regs.eflags&=~0x100u;
 request(PTRACE_SETREGS,child.pid,nullptr,&probe_regs);request(PTRACE_CONT,child.pid);wait();
 check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"MMX full FP observation bridge failed");
 request(PTRACE_SETREGS,child.pid,nullptr,&original_regs);
 const auto image=memory(child.pid,0x64000100u,108);std::uint16_t tag{},sw{};
 std::memcpy(&tag,image.data()+8,2);std::memcpy(&sw,image.data()+4,2);
 for(unsigned n=0;n<8;++n)check(((tag>>(n*2u))&3u)!=3u,"MMX oracle file has an empty physical register");
 if(!mmx_entry_tag)mmx_entry_tag=tag;
 check(tag==*mmx_entry_tag&&(sw&0x3800u)==0,"Original classifier changed MMX full tags/TOP");
 J physical=J::array();for(unsigned n=0;n<8;++n){const auto raw=hex(std::span(image).subspan(28+n*10,10));check(raw==c.at("active_mmx").at("physical")[n].get<std::string>(),"Original classifier changed opaque MMX80 payload");physical.push_back(raw);}
 ++mmx_snapshots;return J{{"tag_word",tag},{"physical",physical},{"known_mask",c.at("active_mmx").at("known_mask")}};
};
'''

def physical_original_mmx(*args,**kwargs):
 source=inspect.getsource(shared.physical_original)
 marker="    (directory / 'reference.s').write_text(assembly)"
 assert source.count(marker)==1
 source=source.replace(marker,'    assembly += '+repr(BRIDGE)+'\n'+marker)
 marker="    private_observer = directory / 'observer.cpp'"
 assert source.count(marker)==1
 seed_marker='request(PTRACE_SETFPREGS,child.pid,nullptr,&fp);'
 snapshot_marker='  std::vector<std::byte>stack(12);'
 trace_marker='observed["xmm"]=std::move(lanes);'
 boundary_marker='if(c.contains("stop_before_entries"))result.back()["stopped_at"]=std::uint32_t(regs.rip);'
 addition=''
 for old,new in((seed_marker,seed_marker+SEED),(snapshot_marker,SNAPSHOT+snapshot_marker),(trace_marker,trace_marker+'observed["active_mmx"]=full_mmx();'),(boundary_marker,'result.back()["active_mmx"]=full_mmx();result.back()["mmx_snapshots"]=mmx_snapshots;'+boundary_marker)):
  addition+='    if observer_text.count('+repr(old)+')!=1:raise ValueError("Opaque MMX oracle source boundary drift")\n'
  addition+='    observer_text=observer_text.replace('+repr(old)+','+repr(new)+')\n'
 source=source.replace(marker,addition+marker)
 source=source.replace('    observer_text = observer_source.read_text()', '    observer_text = observer_source.read_text().replace("#include <map>","#include <map>\\n#include <optional>")')
 namespace=dict(vars(shared));exec(compile(source,__file__,'exec'),namespace)
 proof,authority=namespace['physical_original'](*args,**kwargs)
 for row in proof['cases']:
  assert all(call['active_mmx']['tag_word']==row['active_mmx']['tag_word']for call in row['calls'])
 authority.update(mmx_observation='Actual full FSAVE tag word/TOP and all eight opaque80 payloads; no numeric classification',
                  observer_input_only_frestore=True,observation_stub_fnsave_frstor=True,
                  mmx_snapshots=sum(row['mmx_snapshots']for row in proof['cases']),
                  wrapper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 return proof,authority
