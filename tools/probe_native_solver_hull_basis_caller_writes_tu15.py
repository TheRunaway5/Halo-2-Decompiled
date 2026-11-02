#!/usr/bin/env python3
"""Original-only313BB0→313790 watch of the unprovided basis W before its read."""
import argparse,copy,hashlib,inspect,json,os,struct
from pathlib import Path
import validate_native_solver_hull_single_vertex_tu15 as parent
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
def observer_source():
 text=parent.base.observer_source()
 anchor='  J writes=J::array(),reads=J::array(),estimates=J::array(),timestamps=J::array();'
 assert text.count(anchor)==1
 configure='''  const auto watched=m.at("watched_word").get<std::uint32_t>();J hardware_writers=J::array();const auto debug=[&](unsigned n,unsigned long value){request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)+n*sizeof(unsigned long)),reinterpret_cast<void*>(value));};debug(0,watched);debug(6,0);debug(7,1u|(13u<<16u));
'''
 text=text.replace(anchor,configure+anchor)
 anchor='const auto registers=gp(regs);J pending=J::array();';assert text.count(anchor)==1
 text=text.replace(anchor,'const auto watched_before=hex(memory(child.pid,watched,4));'+anchor)
 anchor='    for(auto&a:pending){';assert text.count(anchor)==1
 record='''    const auto cause=request(PTRACE_PEEKUSER,child.pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)+6*sizeof(unsigned long)));if(cause&1u)hardware_writers.push_back(J{{"instruction",pc},{"before",watched_before},{"after",hex(memory(child.pid,watched,4))}});debug(6,0);
'''
 text=text.replace(anchor,record+anchor)
 text=text.replace('{"banks",banks},{"actual_stop"','{"watched_word",watched},{"hardware_writers",hardware_writers},{"word_at_read",hex(memory(child.pid,watched,4))},{"banks",banks},{"actual_stop"')
 return text
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,original_only=True,native_admission=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN;image=Xbe(NEW_DEFAULT);base=parent.base;physical=base.physical;A=(physical.SP-4)&~15;w_address=A-0x30b4;rows=[]
  seed=parent.cases(image)[0]
  for value in (0,0x02000000,0x7f812345):
   c=copy.deepcopy(seed);raw=bytearray.fromhex(c['banks'][0]['before']);struct.pack_into('<I',raw,0x1580,2);struct.pack_into('<4I',raw,0x1800,0x3f800000,0x40000000,0x40400000,0);struct.pack_into('<4I',raw,0x1810,0xbf800000,0x3f000000,0x40800000,0);c['banks'][0]['before']=raw.hex();c['banks'].append(dict(address=w_address,before=struct.pack('<I',value).hex()));c['banks'].sort(key=lambda b:b['address']);c['stop_pc']=0x31382c;c['name']=f'parentW{value:08x}';c['declared_private_w']=value;rows.append(c)
  base.OWNERS=(0x313bb0,0x32a660,0x313790);base.BOUNDARIES=()
  meta=inspect.getsource(base.metadata).replace('counts[0]==274','counts[0]==742').replace('    assert not op.mem.segment;base,index=',"    if op.mem.segment:\n     assert ins.reg_name(op.mem.segment)=='fs';continue\n    base,index=");ns=dict(base.__dict__);exec(meta,ns);metadata=ns['metadata'](image,a.output);physical.observer_source=observer_source
  def boundary(c,observed):assert observed['actual_stop']==0x31382c;assert observed['boundary_gp'][4]+0x3c==w_address;return None
  physical.boundary=boundary;metadata['watched_word']=w_address;physical.physical(image,metadata,rows,scratch)
  proof=[]
  for c in rows:
   assert not c['hardware_writers'];assert c['word_at_read']==struct.pack('<I',c['declared_private_w']).hex();proof.append(dict(declared_w=c['declared_private_w'],word_at_read=c['word_at_read'],writers=c['hardware_writers'],executed_instructions=c['original_instructions']))
  positive=[copy.deepcopy(seed)for seed in rows];metadata['watched_word']=w_address-4;physical.physical(image,metadata,positive,scratch)
  for c in positive:assert len(c['hardware_writers'])==1 and c['hardware_writers'][0]['instruction']in(0x31381c,0x313824,0x313828)
  report.update(passed=True,source_authority_sha256=NEW_PIN,scope=__doc__,source_listing_sha256=metadata['listing_sha256'],watched_w=w_address,source_parent_entry=0x313bb0,source_read_site=0x31382c,declared_before_controls=proof,positive_xyz_controls=[dict(watched=c['watched_word'],writers=c['hardware_writers'])for c in positive],finding='This declared direct parent count2 path has NO source writer to the W lane before its MOVAPS read; an actual caller outside this path remains a separate provenance question.',genuine_multiplayer_invocation=False,no_native_source_context_supplied=True,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0
if __name__=='__main__':raise SystemExit(main())
