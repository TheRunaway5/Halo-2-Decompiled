#!/usr/bin/env python3
"""Native16C620 static branch fullreturns; true instanced callback separately actual-qualified."""
import copy,inspect
import validate_native_bsp_segment_query_tu15 as bsp
import validate_native_contact_pressure_tu15 as base
ROOT=bsp.ROOT;BACKEND='unchanged-tu15-native-camera-geometric-query-v1'
OWNERS=(0x16c620,0x16b940,*bsp.OWNERS,0x16bb40,0x1f7e00,0x1f8040,0x14cdb0,0x11e600)
def metadata(image):
 text=inspect.getsource(bsp.metadata).replace('assert counts==[42,262,125,30,97];','assert counts==[560,29,42,262,125,30,97,45,42,19,39,27];');env=dict(bsp.__dict__,OWNERS=OWNERS);exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[];seeds=bsp.cases(image)
 for flags in range(32):
  for seed_index in(256,261,266,271,276):
   c=copy.deepcopy(seeds[seed_index]);data=bytearray.fromhex(c['banks'][0]['before'])
   def put(p,n,width=4):data[p-0x1000000:p-0x1000000+width]=(n&((1<<(8*width))-1)).to_bytes(width,'little')
   G=0x1004600;R=0x1004800;T=0x1004300;LR=0x1004500
   for p,n in((G+16,T),(G+48,LR),(G+0xa0,0x1004100),(T+8,7),(LR,0),(LR+8,0)):put(p,n)
   for i in range(2):
    for off,n,width in((0,0,2),(4,0,1),(5,0,1),(6,0,2)):put(0x1001800+i*8+off,n,width)
   mask=(flags<<23)|1
   c.update(name=f'camera_query_{flags}_{seed_index}',entry=0x16c620,receiver=0,entry_registers={},arguments=[mask,bsp.O,bsp.V,0xffffffff,0xffffffff,R],context=base.physical.SP-0x490,bsp_output=base.physical.SP-0x41c,iterator=base.physical.SP-0x438,payload=base.physical.SP-0x43c)
   c['banks'][0]['before']=data.hex();c['banks']=[b for b in c['banks']if b['address']<base.physical.STACK or b['address']>=base.physical.STACK+0x10000];c['banks']+= [dict(address=c['context'],before=bytes(40).hex()),dict(address=c['bsp_output'],before=bytes(0x41c).hex()),dict(address=0x4ecc18,before=G.to_bytes(4,'little').hex()),dict(address=0x4ecc10,before=bsp.D.to_bytes(4,'little').hex()),dict(address=0x4f9b50,before=(0x1003300).to_bytes(4,'little').hex()),dict(address=0x474c24,before='0000'),dict(address=0x45695c,before=image.read(0x45695c,4).hex()),dict(address=0x48a0b0,before='0000')];rows.append(c)
 # Bridge and retainedlocation cases start from coherent original inputs;
 # no callback result, hitfraction or after-state is supplied.
 for bridge_byte in(0xff,0,0x80,0x81):
  for normal in(0x3f800000,0xbf800000):
   for clearance in(False,True):
    c=copy.deepcopy(rows[45]);data=bytearray.fromhex(c['banks'][0]['before']);G=0x1004600;B=0x1004380
    def put(p,n,width=4):data[p-0x1000000:p-0x1000000+width]=(n&((1<<(8*width))-1)).to_bytes(width,'little')
    put(G+0x68,B);put(0x1004170,bridge_byte,1)
    for n in range(2):
     for off,value,width in((2,3,2),(4,normal,4),(8,0,4),(12,0,4),(16,0,4)):put(B+n*24+off,value,width)
    c['banks'][0]['before']=data.hex();c['arguments'][0]=(9<<23)|2|(0x20000000 if clearance else 0);c['name']=f'bridge_{bridge_byte}_{normal}_{clearance}'
    c['banks']+=[dict(address=0x4f3ce1,before='00'),dict(address=0x4f3ce4,before='0000000000000000'),dict(address=0x46a0ec,before=image.read(0x46a0ec,4).hex()),dict(address=0x4ecc0c,before=bsp.D.to_bytes(4,'little').hex()),dict(address=0x46a38c,before=image.read(0x46a38c,4).hex()),dict(address=0x46a388,before=image.read(0x46a388,4).hex())];rows.append(c)
 for index in(0,5,10,15,20):
  c=copy.deepcopy(rows[index]);c['arguments'][0]|=0x20000000;c['name']='clearance_'+c['name'];c['banks']+=[dict(address=0x4ecc0c,before=bsp.D.to_bytes(4,'little').hex()),dict(address=0x46a38c,before=image.read(0x46a38c,4).hex()),dict(address=0x46a388,before=image.read(0x46a388,4).hex())];rows.append(c)
 for index in(0,5,80,166,176):c=copy.deepcopy(rows[index]);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows

def main():
 base.physical.observer_source=bsp.support.parent.observer_source;base.physical.physical=bsp.support.parent.ORIGINAL_PHYSICAL;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("if e['instruction']in seen:continue","if e['instruction']in seen or(category=='writes'and e['instruction']==0x1e6c81):continue")
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_camera_geometric_query.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_camera_geometric_query.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({560,29,42,262,125,30,97,45,42,19,39,27})').replace('auto services=native_tu15_contact_pressure_native_services();','NativeTu15CameraGeometricQueryServices services;').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('initial_hit','0x16c672u,q.result,0','0x16c672u,q.result,1'),('position_axis','0x16ccb1u,q.result+12u','0x16ccb1u,q.result+8u'),('bsp_admission','if(bsp_hit&&(mask&1u))','if(bsp_hit&&!(mask&1u))')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('');[shutil.copyfile(ROOT/'src/physics'/name,project/name)for name in('native_tu15_bsp_segment_query.cpp','native_tu15_bsp_surface_query.cpp','native_tu15_camera_query_leaves.cpp','native_tu15_camera_query_object_context.cpp','native_tu15_constraint_effect_location.cpp','native_tu15_joint_constraint_numeric_internal.hpp')]")
 text=text.replace("+'.cpp types.cpp)","+'.cpp types.cpp native_tu15_bsp_segment_query.cpp native_tu15_bsp_surface_query.cpp native_tu15_camera_query_leaves.cpp native_tu15_camera_query_object_context.cpp native_tu15_constraint_effect_location.cpp)")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_camera_geometric_query.hpp',ROOT/'src/physics/native_tu15_bsp_segment_query.cpp'")
 calls='''{const auto a=c.at("arguments");result=native_tu15_query_camera_geometry(m,{a[0],a[1],a[2],a[3],a[4],a[5]},{c.at("bsp_output"),c.at("context"),c.at("iterator"),c.at("payload")},fp,0,services,observer);}'''
 check='''if(!fault_case&&!observer_failed)check((result&255u)==(c.at("boundary_gp")[0].get<std::uint32_t>()&255u),name+" camera geometric bool return differs");'''
 env=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check);exec(text,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
