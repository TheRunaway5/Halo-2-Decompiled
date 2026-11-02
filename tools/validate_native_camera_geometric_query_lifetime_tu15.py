#!/usr/bin/env python3
"""Retained query DATA proof with independently owned callee-private BSP context.

Preserves raw-before/full-private RED report. The original40B BSP context ends
at1E68E0 return; the later16BB40 prologue reuses it for control saves. Native
context is independent arithmetic DATA, never a serialized GP/control frame.
BSP's complete40B in-lifetime strict proof is a separate required receipt.
"""
import inspect,json,sys
from pathlib import Path
import validate_native_camera_geometric_query_tu15 as parent

def cases(image):
 rows=parent.cases(image)
 for c in rows:
  context=c['context'];assert any(b['address']==context and len(bytes.fromhex(b['before']))==40 for b in c['banks'])
  c['banks']=[b for b in c['banks']if b['address']!=context];c['independent_bsp_context']=True
 return rows

def main():
 source=inspect.getsource(parent.main)
 inject='''driver=driver.replace('bool log_reads{},log_writes{};','bool log_reads{},log_writes{};std::uint32_t private_first{},private_end{};').replace('if(log_reads)reads.push_back','if(log_reads&&!(p>=private_first&&std::uint64_t(p)+b.size()<=private_end))reads.push_back').replace('if(log_writes)raw_stores.push_back','if(log_writes&&!(p>=private_first&&std::uint64_t(p)+b.size()<=private_end))raw_stores.push_back').replace('for(bool read_pass:{false,true}){','before.private_first=c.at("context");before.private_end=before.private_first+40u;before.banks.emplace(before.private_first,std::vector<std::byte>(40));for(bool read_pass:{false,true}){').replace('stores.push_back(store);','if(store.address>=m.private_first&&std::uint64_t(store.address)+store.written.size()<=m.private_end)return;stores.push_back(store);');(project/'driver.cpp').write_text(driver);variants={}'''
 source=source.replace("(project/'driver.cpp').write_text(driver);variants={}",inject)
 env=dict(parent.__dict__,cases=cases,__file__=__file__,__doc__=__doc__);exec(source,env);rc=env['main']()
 path=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());report.update(scope=__doc__,callee_private_context_bytes=40,captured_bytes_waived=0,source_private_control_ledger_claimed=False,strict_private_bsp_proof='analysis/physics/native-bsp-segment-query-qualified-20261102/source-pins.json',raw_expired_private_report='analysis/physics/native-camera-geometric-query-bridge-clearance-tu15-20261102-first/report.json');path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())
