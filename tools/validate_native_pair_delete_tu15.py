#!/usr/bin/env python3
"""Public320260 on genuine cached pairs and original-generated pending DATA."""
import inspect,os
from pathlib import Path
import validate_native_pair_create_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
def main():
 assert os.environ.get('HALO2_TEST_SCRATCH');text=inspect.getsource(parent.main)
 if os.environ.get('HALO2_NATIVE_PAIR_DELETE_FORMAL_STAGE'):
  stage=Path(os.environ['HALO2_NATIVE_PAIR_DELETE_FORMAL_STAGE']).resolve()
  text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_pair_handlers.cpp',project/'native.cpp')", "shutil.copyfile(Path("+repr(str(stage/'staged/src/physics/native_tu15_pair_handlers.cpp'))+"),project/'native.cpp');shutil.copyfile(Path("+repr(str(stage/'staged/include/halo2/native_tu15_pair_handlers.hpp'))+"),project/'include/halo2/native_tu15_pair_handlers.hpp')")
  text=text.replace("'tools/validate_native_pair_create_tu15.py'", "'tools/validate_native_pair_create_tu15.py',"+repr(str(stage/'staged/src/physics/native_tu15_pair_handlers.cpp'))+','+repr(str(stage/'staged/include/halo2/native_tu15_pair_handlers.hpp')))
 text=text.replace("'tools/validate_native_pair_create_tu15.py'","'tools/validate_native_pair_create_tu15.py','tools/validate_native_pair_delete_tu15.py'")
 text=text.replace('complete_body(code,0x320ef0)','complete_body(code,0x320260)')
 begin=text.index('callback_args={');end=text.index('\n  for pc ',begin)
 text=text[:begin]+'callback_args={0x32034b:0,0x320352:0,0x3204b9:0,0x3204c1:0}'+text[end:]
 begin=text.index("  original_kernel=(project/'native.cpp')");end=text.index('\n  report.update(',begin)
 negatives='''  original_kernel=(project/'native.cpp').read_text();negative=[]
  for name,needle,replacement in [('wrong_delete_flag','m.word(0x32027bu,handler+0xcu,1,1)','m.word(0x32027bu,handler+0xcu,0,1)'),('wrong_cached_advance','m.word(0x320355u,handler+0x80u,source+16u)','m.word(0x320355u,handler+0x80u,source+8u)')]:
   assert original_kernel.count(needle)==1;(project/'native.cpp').write_text(original_kernel.replace(needle,replacement));label='negative-'+name;cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','pair_create','--filter','^child0_poison0$','--cmake-arg=-DCMAKE_CXX_COMPILER='+a.compilers[0],'--jobs','1','--test-jobs','1','--retain-report','child0_poison0.json'];r=subprocess.run(cmd);observed=json.loads((a.output/label/'evidence/child0_poison0.json').read_text());assert r.returncode!=0 and not observed.get('error')and not observed['passed'];negative.append(dict(name=name,rejected=True,command=cmd));(project/'native.cpp').write_text(original_kernel)
''';text=text[:begin]+negatives+text[end:]
 text=text.replace('for poison in range(14):','for poison in range(18):')
 text=text.replace('source_entry=0x320ef0','source_entry=0x320260')
 text=text.replace("scope='Two genuine","scope='Declared public320260 formal boundary from genuine actual02 cached-pair DATA; pending positive prepared by actual original320DA0, no fabricated rows/results; two genuine")
 needle="(project/'driver.cpp').write_text(driver)"
 transform='''
  begin=driver.index('if(variant==1){');end=driver.index('const auto changed_cursor=',begin)
  driver=driver.replace('handler=std::uint32_t(entry.rcx),formal=', 'original_handler=std::uint32_t(entry.rcx),formal=')
  driver=driver.replace('const auto variant=poison/2u;', 'auto handler=original_handler;const auto variant=poison/2u;')
  begin=driver.index('if(variant==1){');end=driver.index('const auto changed_cursor=',begin)
  driver=driver[:begin]+''' + repr('''
 if(variant==0||variant==2||variant==3||variant==4||variant==6){auto current=original.word(handler+0x80u);check(current<original.word(handler+0x84u),"Actual cached delete profile requires existing current record");if(variant==4||variant==6)current=original.word(handler+0x84u)-16u;put(pair,original.word(current));put(pair+4u,original.word(current+4u));}
 if(variant==2||variant==5){
   const auto body=original.word(original.word(pair+(variant==5?4u:0u))-0x10u+0x20u),island=original.word(body+0x44u);
   J preparation;Bridge prepare{original,sp-0x3000u,before_fp,preparation};NativeTu15PhysicsFrameFp state{before_fp.cwd,before_fp.swd,before_fp.mxcsr};
   prepare.call(0x320da0u,island,{pair},0x321053u,state);
   check(original.word(island+0x78u)>0,"Actual320DA0 did not generate a pending pair");put(handler+0x80u,original.word(handler+0x84u));
   report["ordinary_original_pending_preparation"]=preparation;
 }
 if(variant==3||variant==4)put(handler+0x80u,original.word(handler+0x84u));
 if(variant==7||variant==8){
   const auto owner=original_handler-0x1cu,world=original.word(original_handler+4u),count=original.word(owner+0x6cu),rows=original.word(owner+0x68u);
   check(count>0,"Other-island removal requires actual current contact records");const auto record=rows+(variant==8?count-1u:0u)*16u;
   const auto first_handle=original.word(record),second_handle=original.word(record+4u);
   const auto first=original.word(original.word(first_handle-0x10u+0x20u)+0x44u),second=original.word(original.word(second_handle-0x10u+0x20u)+0x44u);
   check(first==owner||second==owner,"Actual persistent contact owner does not match either physical body island");
   if(first==owner){const auto active=original.word(world+8u),active_count=original.word(world+0xcu);std::uint32_t other{};for(unsigned n=0;n<active_count;++n){const auto candidate=original.word(active+n*4u);if(candidate!=owner){other=candidate;break;}}check(other!=0,"Other-island handler requires an actual second island");handler=other+0x1cu;}
   else handler=first+0x1cu;
   put(pair,first_handle);put(pair+4u,second_handle);const auto empty=original.word(original_handler+0x84u);
   put(handler+0x80u,empty);put(handler+0x84u,empty);put(handler+0x88u,original.word(original_handler+0x88u));
 }
 auto declared_entry=entry;declared_entry.rip=0x320260u;declared_entry.rcx=handler;request(PTRACE_SETREGS,original.pid,nullptr,&declared_entry);
''')+'''+driver[end:]
  driver=driver.replace('auto start=entry;','auto start=entry;start.rip=0x320260u;start.rcx=handler;')
  begin=driver.index(' NativeTu15PairHandlerServices services;');end=driver.index('NativeTu15PhysicsFrameFp fp{before_fp.cwd',begin)
  driver=driver[:begin]+''' + repr(''' NativeTu15PairHandlerServices services;
 services.destroy_agent=[&](auto agent,auto owner,auto&fp){call(owner.identity,agent.identity,{},0x32034eu,fp);};
 services.agents.release_manager=[&](auto manager,auto owner,auto&fp){call(owner.identity,manager.identity,{},0x320355u,fp);};
 ''')+'''+driver[end:]
  driver=driver.replace('native_tu15_create_pair(memory,handler,pair,{formal},fp,services,','native_tu15_delete_pair(memory,handler,pair,fp,services,')
  driver='#include <set>\\n'+driver
  driver=driver.replace('unsigned count{};','unsigned count{};std::set<std::uint32_t>executed;')
  driver=driver.replace('check(++count<10000','executed.insert(pc);check(++count<10000')
  driver=driver.replace('{"actual_original_child_entry",true}','{"declared_public320260_entry",true},{"captured320260_caller_ABI",false},{"genuine_existing_pair_DATA",true},{"original_generated_pending_rows",variant==2||variant==5},{"executed_sites",executed}')
  '''
 text=text.replace(needle,transform+needle)
 if os.environ.get('HALO2_NATIVE_PAIR_DELETE_FORMAL_STAGE'):
  text=text.replace("'native_tu15_delete_pair(memory,handler,pair,fp,services,'", "'native_tu15_delete_pair(memory,handler,pair,{formal},fp,services,'")
 env=dict(parent.__dict__);env['main']=None;exec(text,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
