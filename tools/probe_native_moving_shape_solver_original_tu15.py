#!/usr/bin/env python3
"""Source-only real1479 2E5320→2F8B60→30E930 executed graph and typed methods."""
import inspect,json,sys,hashlib
from pathlib import Path
import validate_native_moving_shape_process_actual_tu15 as actual
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT
ROOT=Path(__file__).resolve().parents[1]
def original_source_with_fs(function):
 text=inspect.getsource(function)
 anchor="        assembly.append(f'''.code32"
 assert text.count(anchor)==1
 text=text.replace(anchor,"        assembly.append('.section .fs,\"aw\"\\n.zero 4096');sections.append('.fs 0x71000000:{*(.fs)}')\n"+anchor)
 text=text.replace(' cld\n fninit',''' cld
 mov $123,%eax
 mov $1,%ebx
 mov $descriptor,%ecx
 mov $16,%edx
 int $0x80
 test %eax,%eax
 jnz setup_failed
 mov $7,%eax
 mov %ax,%fs
 movl $0xffffffff,%fs:0
 fninit''')
 text=text.replace('entry: .long','''setup_failed:
 mov $1,%eax
 mov $99,%ebx
 int $0x80
descriptor:.long 0,0x71000000,0xfffff,0x51
entry: .long''')
 return text
def observer_source():
 text=(ROOT/'tools/observe_native_moving_shape_process_actual_tu15.cpp').read_text();return text[:text.index('int main(int argc,char**argv)')]+(ROOT/'tools/observe_native_moving_shape_solver_original_tu15.cpp').read_text()
def main():
 text=inspect.getsource(actual.main)
 text=text.replace("(ROOT/'tools/observe_native_moving_shape_process_actual_tu15.cpp').read_text()","observer_source()")
 text=text.replace("'tools/validate_native_moving_shape_process_actual_tu15.py'","'tools/validate_native_moving_shape_process_actual_tu15.py','tools/probe_native_moving_shape_solver_original_tu15.py','tools/observe_native_moving_shape_solver_original_tu15.cpp'")
 text=text.replace('production_binding=False,first_actual_child_entry_only=True','production_binding=False,first_actual_child_entry_only=False,original_only_solver_graph=True')
 text=text.replace('environment=dict(common.__dict__,parent=parent,__doc__=__doc__)','text=text.replace("source=inspect.getsource(base.original);source=","source=original_source_with_fs(base.original);source=");environment=dict(common.__dict__,parent=parent,observer_source=observer_source,original_source_with_fs=original_source_with_fs,__doc__=__doc__)')
 namespace=dict(actual.__dict__,observer_source=observer_source,original_source_with_fs=original_source_with_fs,__doc__=__doc__);exec(text,namespace);rc=namespace['main']();out=Path(sys.argv[sys.argv.index('--output')+1]);image=Xbe(NEW_DEFAULT);authenticated=0
 for compiler in('g++','clang++'):
  report=json.loads((out/(compiler+'-actual.json')).read_text())
  for profile in report['profiles']:
   for site in profile['executed_unique_solver_sites']:
    assert bytes.fromhex(site['original_bytes15'])==image.read(site['instruction'],15),(hex(site['instruction']),'Executed source text differs');authenticated+=1
 j=json.loads((out/'report.json').read_text());j['executed_opcode_windows_authenticated']=authenticated;j['all_executed_original_bytes_unchanged']=True;j['source_only_no_native_solver_inputs_supplied']=True;(out/'report.json').write_text(json.dumps(j,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())
