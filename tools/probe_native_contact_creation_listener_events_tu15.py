#!/usr/bin/env python3
"""Original-only initialized contact-event census on genuine1479 BEFORE.

No padding, GP image or event/callee outcome is supplied to native code.
"""
import hashlib,inspect,json,sys
from pathlib import Path
import probe_native_moving_shape_process_full_original_tu15 as full
ROOT=Path(__file__).resolve().parents[1]
def observer_source():
 text=full.observer_source()
 text=text.replace('unsigned instructions{};bool solver_entered{};','unsigned instructions{};J events=J::array();bool solver_entered{};')
 anchor='   const auto opcode=child.read(pc,15);'
 inserted='''   if(pc==0x2e4b60u||pc==0x281710u||pc==0x281890u){const auto event=child.word(std::uint32_t(regs.rsp)+4u);J fields=J::object();for(auto offset:{4u,8u,12u,16u,20u,24u,28u,32u})fields[std::to_string(offset)]=child.word(event+offset);fields["id"]=child.word(event)&0xffffu;J roots=J::array();for(auto offset:{8u,12u}){auto payload=child.word(event+offset);for(unsigned n=0;n<64;++n){const auto parent=child.word(payload+12u);if(!parent)break;payload=parent;check(n<63,"Cyclic event payload");}const auto kind=child.word(payload+24u),body=child.word(payload+32u);roots.push_back(J{{"payload",payload},{"kind",kind},{"body",body},{"body_material_byte",body?child.word(body+0x48u)&255u:0}});}events.push_back(J{{"owner",pc},{"receiver",std::uint32_t(regs.rcx)},{"event",event},{"initialized_fields",fields},{"payload_roots",roots},{"current_fp",fp_words(child.fp())}});}
'''
 assert text.count(anchor)==1;text=text.replace(anchor,inserted+anchor)
 text=text.replace('{"original_solver_return",result}', '{"initialized_listener_events",events},{"original_solver_return",result}')
 return text
def main():
 text=inspect.getsource(full.main);env=dict(full.__dict__,observer_source=observer_source,__doc__=__doc__);exec(text,env);rc=env['main']()
 p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';j=json.loads(p.read_text());j['original_only_initialized_event_census']=True;j['native_event_input_supplied']=False;j['padding_extracted']=False;j['source_hashes'][str(Path(__file__).relative_to(ROOT))]=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())
