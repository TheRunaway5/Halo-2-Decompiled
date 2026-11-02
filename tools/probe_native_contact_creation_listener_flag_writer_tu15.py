#!/usr/bin/env python3
"""Original-only event-flag transition writer; no native input/outcome use."""
import inspect,json,hashlib,sys
from pathlib import Path
import probe_native_contact_creation_listener_events_tu15 as events
def observer_source():
 text=events.observer_source()
 text=text.replace('J events=J::array();bool solver_entered{};', 'J events=J::array(),flag_changes=J::array();std::uint32_t current_event{};bool solver_entered{};')
 text=text.replace('const auto event=child.word(std::uint32_t(regs.rsp)+4u);J fields=', 'const auto event=child.word(std::uint32_t(regs.rsp)+4u);current_event=event;J fields=')
 text=text.replace('fields["id"]=child.word(event)&0xffffu;', 'const auto id_bytes=child.read(event,2);fields["id"]=std::to_integer<unsigned>(id_bytes[0])|(std::to_integer<unsigned>(id_bytes[1])<<8u);')
 anchor='   const auto opcode=child.read(pc,15);'
 assert text.count(anchor)==1;text=text.replace(anchor,'   const auto previous_flag=current_event?child.word(current_event+0x20u):0u;\n'+anchor)
 anchor='if(solver_entered&&call)calls.push_back'
 add='''if(current_event){const auto flag=child.word(current_event+0x20u);if(flag!=previous_flag)flag_changes.push_back(J{{"instruction",pc},{"bytes15",process_hex(opcode)},{"event",current_event},{"before",previous_flag},{"after",flag}});}
   '''
 assert text.count(anchor)==1;text=text.replace(anchor,add+anchor)
 text=text.replace('{"initialized_listener_events",events}', '{"event_flag_changes",flag_changes},{"initialized_listener_events",events}')
 return text
def main():
 # The reused command builder requires one unit. This pure module is linked
 # but never called; this driver still executes only the original program.
 events.full.graph.actual.UNITS=('native_tu15_solver_hull_polytope.cpp',)
 text=inspect.getsource(events.main);env=dict(events.__dict__,observer_source=observer_source,__doc__=__doc__);exec(text,env);rc=env['main']()
 p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';j=json.loads(p.read_text());j['source_only_event_flag_writer_watch']=True;j['new_tool_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())
