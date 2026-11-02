#!/usr/bin/env python3
"""Genuine1479 PROCESS source prefix derives six DATA formals for native314710.

The independently executed original prefix is input construction, not a native
parent claim. No original child outcome, GP or private control frame is used.
"""
import inspect,hashlib,json
from pathlib import Path
import validate_native_moving_shape_process_actual_tu15 as actual
import probe_native_moving_shape_solver_original_tu15 as graph
ROOT=Path(__file__).resolve().parents[1]
UNITS=('native_tu15_solver_hull_query.cpp','native_tu15_solver_hull_single_vertex.cpp','native_tu15_solver_hull_support.cpp','native_tu15_contact_shape.cpp')
def observer_source():
 text=(ROOT/'tools/observe_native_moving_shape_process_actual_tu15.cpp').read_text()
 return text[:text.index('int main(int argc,char**argv)')]+(ROOT/'tools/observe_native_solver_hull_query_actual_tu15.cpp').read_text()
def main():
 parent=actual.parent
 parent.OWNERS=(0x314710,0x313bb0,0x32a660,0x313560,0x313390,0x313290,0x2e8d20,0x2e7d90,0x2e5db0)
 source=inspect.getsource(parent.metadata).replace('counts[0]==274','counts[0]==53')
 source=source.replace('    assert not op.mem.segment;base,index=',"    if op.mem.segment:\n     assert ins.reg_name(op.mem.segment)=='fs';continue\n    base,index=")
 namespace=dict(parent.__dict__);exec(source,namespace);parent.metadata=namespace['metadata']
 old_metadata=parent.metadata
 def metadata(image,output):
  result=old_metadata(image,output)
  for site in result['sites']:
   if site['instruction']==0x313bf5:site['instruction_dispatch_code_fetch']=True
  raw=image.read(0x3146f8,20)
  result['instruction_dispatch_code_table']=dict(address=0x3146f8,bytes=raw.hex(),sha256=hashlib.sha256(raw).hexdigest(),count1_target=int.from_bytes(raw[4:8],'little'),scope='Source313BF5 immutable CPU jump dispatch, not physics or callback DATA')
  assert result['instruction_dispatch_code_table']['count1_target']==0x313d69
  return result
 parent.metadata=metadata
 text=inspect.getsource(actual.main)
 text=text.replace("(ROOT/'tools/observe_native_moving_shape_process_actual_tu15.cpp').read_text()","observer_source()")
 text=text.replace("text=text.replace('production_binding=True'","text=text.replace('source=inspect.getsource(base.original);source=','source=original_source_with_fs(base.original);source=');text=text.replace('production_binding=True'")
 text=text.replace("environment=dict(common.__dict__,parent=parent,__doc__=__doc__)","environment=dict(common.__dict__,parent=parent,observer_source=observer_source,original_source_with_fs=original_source_with_fs,__doc__=__doc__)")
 # Keep the actual PROCESS original entry; only the driver executes its prefix
 # until the source-authored public hull-query DATA boundary.
 text=text.replace("UNITS))","UNITS))")
 text=text.replace("'include/halo2/native_tu15_moving_shape_process.hpp'","'include/halo2/native_tu15_solver_hull_query.hpp','tools/observe_native_solver_hull_query_actual_tu15.cpp','tools/validate_native_solver_hull_query_actual_tu15.py'")
 text=text.replace("halo2/native_tu15_moving_shape_process.hpp", "halo2/native_tu15_solver_hull_query.hpp")
 text=text.replace('production_binding=False,first_actual_child_entry_only=True','production_binding=False,first_actual_child_entry_only=False,source_derived_standalone_child=True')
 text=text.replace("production_binding=False,first_actual_child_entry_only=False,source_derived_standalone_child=True)","production_binding=False,first_actual_child_entry_only=False,source_derived_standalone_child=True)")
 environment=dict(actual.__dict__,UNITS=UNITS,observer_source=observer_source,original_source_with_fs=graph.original_source_with_fs,__doc__=__doc__)
 exec(text,environment);rc=environment['main']()
 # The reused outer builder authenticates the captured PROCESS entry. Label
 # that authority separately from the child that was actually qualified.
 import sys
 output=Path(sys.argv[sys.argv.index('--output')+1]);p=output/'report.json';r=json.loads(p.read_text());r['upstream_process_sites']=r.pop('body_sites');r['upstream_process_listing_sha256']=r.pop('listing_sha256');r['qualified_child_owner']=0x314710;r['qualified_child_sites']=53;r['captured_boundary_owner']=0x2e5320;r['captured_child_invocation_claimed']=False;r['source_derived_child_formals_only']=True;p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())
