#!/usr/bin/env python3
"""Original-only genuine1479 PROCESS through complete RET16, for lifetime audit."""
import inspect,json,sys,hashlib
from pathlib import Path
import probe_native_moving_shape_solver_original_tu15 as graph
ROOT=Path(__file__).resolve().parents[1]
def observer_source():
 text=graph.observer_source()
 assert text.count('solver_entered&&pc==0x2f8cc2u')==1
 text=text.replace('solver_entered&&pc==0x2f8cc2u','pc==0x5000000u').replace('pc==0x30e930u)solver_entered=true','pc==0x2e5320u)solver_entered=true')
 text=text.replace('child.step();J calls=','child.step();const auto process_entry_sp=std::uint32_t(child.regs().rsp);J calls=')
 text=text.replace('if(pc==0x5000000u){result=', 'if(pc==0x5000000u){check(regs.rsp==process_entry_sp+20u,"Original PROCESS RET16 stack delta differs");result=')
 text=text.replace('{"original_return_site",0x2f8cc2u}','{"original_return_site",0x5000000u}')
 return text
def main():
 text=inspect.getsource(graph.main);namespace=dict(graph.__dict__,observer_source=observer_source,__doc__=__doc__);exec(text,namespace);rc=namespace['main']()
 output=Path(sys.argv[sys.argv.index('--output')+1]);p=output/'report.json';r=json.loads(p.read_text());r['original_only_full_process']=True;r['native_full_process_qualified']=False;r['source_ret16_delta_checked']=True;r['source_hashes']['tools/probe_native_moving_shape_process_full_original_tu15.py']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();r['scope']=__doc__;p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())
