#!/usr/bin/env python3
"""Full native1D77D0 on declared source DATA, closed active-body impulse graph."""
import inspect,struct
import validate_native_agent_cleanup_children_tu15 as child
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT
original_inputs=child.base.reference_inputs
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def reference_inputs(image):
 banks,provenance=original_inputs(image);u=child.base.UI
 for p,v in[(0x52be5c,u+0x1000),(0x4ecbd0,u+0x1100),(0x4f0414,u+0x1800),(0x51df54,u+0x1c00)]:banks[p]=struct.pack('<I',v)
 for p,n in[(0x41f124,0x20),(0x41f2e0,0x68),(0x41f1e0,0x68),(0x46a0cc,4),(0x46a0d0,4),(0x46a0dc,4),(0x46a0e4,4),(0x46a16c,4),(0x46a238,4),(0x46a240,4),(0x46a454,4),(0x456914,4)]:banks[p]=image.read(p,n)
 return banks,dict(provenance,declared_source_field_controls=True,captured_1d77d0_input=False,source_authored_rows_or_after_supplied=False)
def cases():
 u=child.base.UI;rows=[]
 for pattern in range(18):
  for control in range(4):
   words=[]
   def w(at,v):words.append((at,v))
   def vec(at,values):words.extend((at+n*4,bits(value)if isinstance(value,float)else value)for n,value in enumerate(values))
   datum=0x12340002;component=0x2000+2*160;own=0xabcd0004
   w(0x100,0x41f124);w(0x130,u+0x1200);w(0x1200,0);w(0x144,u+0x400);w(0x1a0,u+0x500)
   vec(0x400,[2.,3.,4.,-.25]);vec(0x410,[0.,0.,1.,0.]);w(0x500,bits([0.,.125,2.,-3.][control]));w(0x508,bits(.5));w(0x50c,17)
   w(0x800,0);w(0x83c,bits(.75));w(0x840,0x76543210);w(0x844,0)
   w(0x1044,u+0x2000);w(0x1144,u+0x4000);w(0x1800+3*16+8,u+0xa000)
   w(0x1c00,30<<16);w(0x1c08,100+control);w(component+4,0);w(component+8,own);w(component+0x10,0 if control&1 else 100);w(component+0x70,u+0x3000)
   w(0x4000+4*12,0);w(0x4000+4*12+8,u+0x7000);w(0x7000,3);w(0x7134,0);w(0x7248,0xffffffff);w(0xa1ec,0)
   for record,body,motion in[(0x3000,0x5000,0x6000),(0x3060,0x5200,0x6200),(0x2fa0,0x5400,0x6400)]:
    vec(record+0x30,[5.,7.,11.]);w(record+0x40,u+body);w(record+0x44,2)
    w(body+8,0);w(body+0x3c,u+motion);w(body+0x44,u+0x2800);w(motion,0x41f2e0);w(motion+0x2c,bits(.25))
    vec(motion+0x40,[1.,2.,3.,0.]);vec(motion+0x50,[.5,-.75,1.25,0.]);vec(motion+0x70,[4.,-2.,8.,0.])
    vec(motion+0x80,[1.,0.,0.,0.]);vec(motion+0x90,[0.,1.,0.,0.]);vec(motion+0xa0,[0.,0.,1.,0.]);vec(motion+0xe0,[2.,3.,4.,0.])
   w(0x2828,0x100) # actual active byte at island+29
   skip=0;reversed=control&1;slot=0
   if pattern==1:w(0x844,8)
   elif pattern==2:w(0x844,0x20)
   elif pattern==3:w(0x844,0x20);skip=1
   elif pattern==4:w(0x844,0x120) # part index1
   elif pattern==5:w(0x844,0xff20) # signed part index-1
   elif pattern==6:w(0x844,0x20);w(component+4,1<<18)
   elif pattern==7:w(0x844,0x20);w(0x3044,1)
   elif pattern==8:w(0x844,0x20);vec(0x3030,[0.,0.,0.])
   elif pattern==9:w(0x602c,0)
   elif pattern==10:w(0x602c,bits(2.))
   elif pattern==11:w(0x602c,bits(-.5))
   elif pattern==12:w(0x500,[0x7fc12345,0x7f812345,0x7f800000,0xff800000][control])
   elif pattern==13:w(0x4000+4*12,1<<24);w(0xa1ec,1<<19);w(0x7248,3)
   elif pattern==14:w(0x4000+4*12,1<<24);w(0xa1ec,1<<19);w(0x7134,2)
   elif pattern==15:w(0x844,0x20);w(0x6000,0x41f1e0);w(0x622c,bits(1.));slot=1
   elif pattern==16:w(0x844,0x20);w(0x5044,0)
   elif pattern==17:
    # Declared consistent one-body/one-inactive-island World, no listeners or
    # deferred references. The genuine315CB0 kernel performs its actual wake.
    w(0x844,0x20);w(0x2828,0);w(0x2800+0x20,u+0xb000);w(0x2800+0x24,0)
    w(0x2800+0x3c,u+0xba00);w(0x2800+0x40,1);w(0xba00,u+0x5000);w(0x5008,u+0xb000);w(0x5074,0)
    w(0xb008,u+0xb900);w(0xb00c,0);w(0xb010,0x80000004);w(0xb014,u+0xb800);w(0xb018,1);w(0xb800,u+0x2800)
    w(0xb07c,0);w(0xb10c,0)
   rows.append(dict(name=f'body_pattern{pattern}_control{control}',entry=0x1d77d0,receiver=u+0x800,manager=u+0x100,arguments=[skip,reversed,datum,slot],user_words=words))
 return rows
def main():
 child.OWNERS=(0x1d77d0,0x1d7b60,0x323020,0x323040,0x31dd70,0x31edf0,0x323a10,0x186c30,0x1e1170,0x315c90,0x315cb0,0x3165d0,0x316d40,0x322660,0x324f80,0x8fdb0);child.cases=cases;child.base.reference_inputs=reference_inputs;source=inspect.getsource(child.main)
 source=source.replace("'tools/validate_native_agent_cleanup_children_tu15.py'","'tools/validate_native_agent_cleanup_children_tu15.py','include/halo2/native_tu15_contact_body_update.hpp','src/physics/native_tu15_contact_body_update.cpp','src/physics/native_tu15_activation_services.cpp','tools/validate_native_contact_body_update_tu15.py'")
 needle="'native_tu15_contact_unwind_internal.hpp'):shutil.copyfile";assert source.count(needle)==1;source=source.replace(needle,"'native_tu15_contact_unwind_internal.hpp','native_tu15_contact_body_update.cpp','native_tu15_activation_services.cpp','native_tu15_island_activation.cpp'):shutil.copyfile")
 needle="environment=dict(base.__dict__,observer_source=observer_source)";assert source.count(needle)==1
 source=source.replace(needle,"source=source.replace(' int3\\n jmp *entry',' mov ${case[\"manager\"]},%eax\\n int3\\n jmp *entry');"+needle)
 needle="(project/'driver.cpp').write_text(driver)";assert source.count(needle)==1
 replacement='''      try{const auto receiver=c.at("receiver").get<std::uint32_t>();const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        NativeTu15ContactBodyUpdateServices body_services;NativeTu15ContactBodyImpulseServices impulse_services;
        impulse_services.activate_body=[&](auto body,auto&current,auto){NativeTu15IslandActivationServices activation;native_tu15_activate_physics_body(memory,body,current,activation,observe);};
        body_services.apply_point_impulse=[&](auto body,auto impulse,auto point,auto&current,auto depth){native_tu15_apply_contact_body_point_impulse(memory,body,impulse,point,current,depth,impulse_services,observe);};
        native_tu15_update_contact_body(memory,c.at("manager"),receiver,bool(args[0]),bool(args[1]),args[2],args[3],{0x200e000u,0x200e010u},fp,0,body_services,observe);
'''
 changes=[('if(log_reads)reads.push_back','if(log_reads&&!(p>=0x2000000u&&p<0x2010000u))reads.push_back'),('memory.fault=c.value("fault",J{});','memory.banks[0x2000000u].resize(0x10000u);memory.fault=c.value("fault",J{});'),('if(!read_pass)observe=[&](const auto&event){','if(!read_pass)observe=[&](const auto&event){if(event.address>=0x2000000u&&event.address<0x2010000u)return;')]
 extra='driver=\'#include "halo2/native_tu15_contact_body_update.hpp"\\n#include "halo2/native_tu15_island_activation.hpp"\\n\'+driver;'
 extra+=''.join('assert driver.count('+repr(a)+')==1;driver=driver.replace('+repr(a)+','+repr(b)+');'for a,b in changes)
 extra+='begin=driver.index(\'      try{const auto entry=\');end=driver.index(\'      }catch(const Fault&)\',begin);driver=driver[:begin]+'+repr(replacement)+'+driver[end:];'
 source=source.replace(needle,extra+needle)
 source=source.replace('native_tu15_array_allocator.cpp native_tu15_array_growth.cpp)','native_tu15_array_allocator.cpp native_tu15_array_growth.cpp native_tu15_contact_body_update.cpp native_tu15_activation_services.cpp native_tu15_island_activation.cpp)')
 source=source.replace('target_compile_features(cleanup PRIVATE cxx_std_20)\\n','target_compile_features(cleanup PRIVATE cxx_std_20)\\ntarget_compile_options(cleanup PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\\n')
 source=source.replace("target=project/'native_tu15_agent_cleanup_children.cpp'","target=project/'native_tu15_contact_body_update.cpp'")
 begin=source.index("mutants=[('point_stride'");end=source.index('\n  for name,before,after in mutants:',begin)
 source=source[:begin]+"mutants=[('row_distance','row+0x34u,m.word(point+0xcu)','row+0x34u,m.word(point+8u)'),('inverse_mass','force=divide(force,divisor)','force=mul(force,divisor)'),('object_flag','at,flags|8u,1','at,flags|4u,1')]"+source[end:]
 source=source.replace("scope='Declared valid objects", "scope='Complete1D77D0 native semantic parent with native getters/property/mass/pointvelocity/1E1170 activebody/dynamicimpulse; declared source DATA and independentFP, actual waking-island/nativeWorld invocation remains open. Only ordinary returned private stack DATA excluded from persistent ledger, no CPU postimages; declared valid objects")
 source=source.replace('report.update(passed=', 'report.update(unexecuted_source_sites={hex(owner):sorted(set(base.complete_body(code,owner).code)-executed)for owner in OWNERS},passed=')
 environment=dict(child.__dict__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
