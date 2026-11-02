#!/usr/bin/env python3
"""Direct native31F660 proof on actual04 initialized collector DATA.

The source append only exposes the existing semantic collector destructor;
private original ELF/ABI construction belongs to the enclosing scratch owner.
No expected capture bank or saved GP/SIMD value is consumed.
"""
import argparse,bisect,hashlib,inspect,json,os,subprocess,sys,tarfile
from pathlib import Path
import validate_native_agent_cleanup_children_tu15 as child
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ARCHIVE=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-phantom-owner-original-v1466-20261101/iteration-04-native-inactive-island-cleanup-vehicle-camera/original/required-failure-reference/actual-failed-capture.tar.zst'
PIN='5526e7976d5619f908a2a5d9fbb7b9831a6f4dacf12e6ea7998e8e01d76ad11c'
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 assert os.environ.get('HALO2_TEST_SCRATCH')and sha(ARCHIVE)==PIN and sha(NEW_DEFAULT)==NEW_PIN
 capture=json.loads((ARCHIVE.parent/'manifests/capture.json').read_text());wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(ARCHIVE)],stdout=subprocess.PIPE)
 with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
  for member in tar:
   key=Path(member.name).name
   if key in wanted:r=wanted[key];raw=tar.extractfile(member).read();assert len(raw)==r['size'];banks[r['address']]=raw
 assert stream.wait()==0 and len(banks)==len(capture['regions']);starts=sorted(banks)
 def get(p,n=4):
  i=bisect.bisect_right(starts,p)-1
  if i<0:return None
  first=starts[i];raw=banks[first];return raw[p-first:p-first+n]if p+n<=first+len(raw)else None
 def word(p):return int.from_bytes(get(p),'little')
 assert word(0x10082380)==0x41f124 and word(0x10082390)==0x41f478
 child.OWNERS=(0x31f660,0x325af0,0x315d10,0x316dc0,0xb4150,0x283450,0x2e3250,0x2e2e30,0x149da0,0x149e00,0x233250,0x233420)
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 for owner in child.OWNERS:
  for pc in complete_body(code,owner).code:
   i=code.at(pc);actual=get(pc,i.size)
   if actual is not None:assert actual==i.bytes,(hex(pc),'Source bytes differ')
 provenance=dict(archive=str(ARCHIVE.relative_to(ROOT)),archive_sha256=PIN,actual_before_banks=len(banks),actual_before_bytes=sum(map(len,banks.values())),kind='Actual initialized04 heap collector; declared direct leafECX and independent ordinary offlineFP/ABI, not captured31F660entry',expected_after_consumed=False,source_code_bytes_authenticated=True)
 child.base.reference_inputs=lambda image:(banks,provenance)
 child.cases=lambda:[dict(name=f'actual04_collector_controls_{n}',entry=0x31f660,receiver=0x10082390,arguments=[],user_words=[])for n in range(4)]
 source=inspect.getsource(child.main)
 needle="'tools/validate_native_agent_cleanup_children_tu15.py'";assert source.count(needle)==1;source=source.replace(needle,needle+",'tools/validate_native_activation_collector_actual_tu15.py','include/halo2/native_tu15_activation_collector_cleanup.hpp','src/physics/native_tu15_activation_contact_manager.cpp','src/physics/native_tu15_island_activation.cpp','analysis/physics/native-activation-collector-export-20261101/append.cpp.inc'")
 needle="'native_tu15_contact_unwind_internal.hpp'):shutil.copyfile";assert source.count(needle)==1;source=source.replace(needle,"'native_tu15_contact_unwind_internal.hpp','native_tu15_activation_contact_manager.cpp','native_tu15_island_activation.cpp'):shutil.copyfile")
 needle="  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();";assert source.count(needle)==1;source=source.replace(needle,"  activation=project/'native_tu15_activation_contact_manager.cpp';activation.write_text(activation.read_text()+(ROOT/'analysis/physics/native-activation-collector-export-20261101/append.cpp.inc').read_text())\n"+needle)
 needle="driver='#include \"halo2/native_tu15_agent_cleanup_children.hpp\"\\n'+driver";assert source.count(needle)==1;source=source.replace(needle,"driver='#include \"halo2/native_tu15_activation_collector_cleanup.hpp\"\\n#include \"halo2/native_tu15_agent_cleanup_children.hpp\"\\n'+driver")
 needle='NativeTu15AgentCleanupChildrenServices services;services.allocation=growth;';assert source.count(needle)==1;source=source.replace(needle,needle+'''NativeTu15ActivationContactManagerServices collector_services;collector_services.allocation=growth;NativeTu15ActivationContactManagerStats collector_stats;
      collector_services.destroy_body=[](auto,auto,auto,auto&){throw std::runtime_error("Unqualified actual body destructor");};collector_services.release_base_lock=[](auto,auto,auto&){throw std::runtime_error("Unqualified actual base-lock release");};''')
 needle='if(entry==0x2f82d0u)native_tu15_cleanup_cached_contact_agent';assert source.count(needle)==1;source=source.replace(needle,'if(entry==0x31f660u)native_tu15_destroy_activation_collector(memory,receiver,fp,collector_services,observe,&collector_stats);\\n        else '+needle)
 source=source.replace('native_tu15_array_allocator.cpp native_tu15_array_growth.cpp)','native_tu15_array_allocator.cpp native_tu15_array_growth.cpp native_tu15_activation_contact_manager.cpp native_tu15_island_activation.cpp)')
 needle="(project/'driver.cpp').write_text(driver)";assert source.count(needle)==1;source=source.replace(needle,"driver=driver.replace('check(cases>20,','check(cases>0,');"+needle)
 source=source.replace("target=project/'native_tu15_agent_cleanup_children.cpp'","target=project/'native_tu15_activation_contact_manager.cpp'")
 begin=source.index("mutants=[('point_stride'");end=source.index('\n  for name,before,after in mutants:',begin);source=source[:begin]+'''mutants=[('collector_vtable','0x41f410u','0x41f411u'),('body_reference','word(body+6u,2)-1u','word(body+6u,2)-2u'),('listener_removal','word(0x315d3eu,entries+index*4u,0);','word(0x315d3eu,entries+index*4u,1);')]'''+source[end:]
 assertion='      check(collector_stats.collector_destructions==1u&&collector_stats.unregistrations==2u&&collector_stats.deferred_references==2u, name+" positive actual collector lifetime not reached");\n'
 source=source.replace("driver=driver.replace('check(cases>20,','check(cases>0,');", "driver=driver.replace('check(cases>20,','check(cases>0,').replace('      check(calls==',"+repr(assertion+"      check(calls==")+");")
 source=source.replace("scope='Declared valid objects", "scope='Direct actual04 collector DATA; every captured bank and ordered FP/read/write compared; borrowed arrays and genuine current body listener/refcount lifetime; no expectedafterbytes or captured31F660ABI; declared valid objects")
 env=dict(child.__dict__);env['main']=None;exec(source,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
