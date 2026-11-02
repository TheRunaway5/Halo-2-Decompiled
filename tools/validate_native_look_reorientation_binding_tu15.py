#!/usr/bin/env python3
"""SAME FP-field-only18A2A0 binding: physical whole/fault/alias profiles and guards."""
import inspect,json
from pathlib import Path
import validate_native_look_reorientation_tu15 as kernel

def main():
 source=inspect.getsource(kernel.main)
 old="(project/'driver.cpp').write_text(driver);variants={}"
 guard=r'''
 for(unsigned mode=0;mode<2;++mode){
  Memory m;NativeTu15LookReorientationBindingActivity out;
  std::uint16_t cw=0x27f,sw=0x20;std::uint32_t mx=0x1fa0;
  bool refused=false;try{native_tu15_reorient_bound_look_controller(m,0,0,
   {cw,sw,mx,mode==0?0u:1u},mode!=0,{},&out);}catch(const std::runtime_error&){refused=true;}
  check(refused&&out.attempts==1&&out.completed==0&&out.refusals==1&&out.reorientation.calls==0,
   "Unqualified reorientation guard entered native kernel");
  check(m.banks.empty()&&m.reads.empty()&&m.raw_stores.empty()&&cw==0x27f&&sw==0x20&&mx==0x1fa0,
   "Reorientation guard touched DATA/FP");
 }
'''
 adapt="""driver=driver.replace('#include \"halo2/native_tu15_look_reorientation.hpp\"','#include \"halo2/native_tu15_look_reorientation_binding.hpp\"').replace('NativeTu15LookReorientationStats stats;','NativeTu15LookReorientationBindingActivity stats;').replace('native_tu15_reorient_look_controller(m,c.at(\"receiver\"),c.at(\"forward\"),fp,0,observer,&stats);','native_tu15_reorient_bound_look_controller(m,c.at(\"receiver\"),c.at(\"forward\"),{fp.control_word,fp.status,fp.mxcsr,0},true,observer,&stats);').replace(' for(const auto&c:fixture.at(\"cases\")){',GUARD+' for(const auto&c:fixture.at(\"cases\")){').replace('   check(failed==fault_case','   check(stats.attempts==1&&stats.completed==(!fault_case&&!observer_failed)&&stats.refusals==(fault_case||observer_failed)&&stats.reorientation.calls==1&&stats.reorientation.completed==stats.completed&&stats.reorientation.refusals==stats.refusals,name+\" binding normal/error counters differ\");   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={} """
 assert old in source;source=source.replace(old,adapt)
 source=source.replace("environment=dict(base.__dict__,__file__=__file__)","environment=dict(base.__dict__,__file__=__file__,GUARD=GUARD)")
 source=source.replace("ROOT/'include/halo2/native_tu15_look_reorientation.hpp',ROOT/", "ROOT/'include/halo2/native_tu15_look_reorientation.hpp',ROOT/'include/halo2/native_tu15_look_reorientation_binding.hpp',ROOT/")
 environment=dict(kernel.__dict__,__file__=__file__,GUARD=guard);exec(source,environment);code=environment['main']()
 path=Path(kernel.base.sys.argv[kernel.base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());report.update(scope=__doc__,binding_guards=['mutable-store-observer','nonempty-live-x87'],normal_error_counter_balance=True,remaining=['Actual typed Host reorientation input/caller boundary and private production stage','Visible multiplayer reorientation/strictOriginal withactualvehiclecamera','Nonempty x87 and mutable observer refused before native DATA access']);kernel.base.save(path,report);return code

if __name__=='__main__':raise SystemExit(main())
