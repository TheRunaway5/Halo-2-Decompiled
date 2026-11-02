#!/usr/bin/env python3
"""Source24DF20 through returned native16C000, no supplied hit outcomes.

Declared source-coherent instance/BSP fixtures are inherited. All persistent
DATA/result fields and FP remain strict; child-private arithmetic records have
their independent in-lifetime proofs, not raw caller stack correspondence.
"""
import copy,inspect,json,struct
from pathlib import Path
import validate_native_camera_instanced_query_tu15 as parent
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
OWNERS=(0x24df20,*parent.OWNERS)
COUNTS='46,'+parent.COUNTS
BACKEND='unchanged-tu15-native-instanced-collector-composed-v1'
def metadata(image):
 text=inspect.getsource(parent.metadata).replace('165,25,13,76,82,42,262,125,30,97,107',COUNTS)
 env=dict(parent.__dict__,OWNERS=OWNERS,COUNTS=COUNTS);exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[];seeds=[c for c in parent.cases(image)if c['entry']==0x16c000 and not c['name'].startswith('fault_seed')]
 for seed in seeds:
  for option in range(3):
   c=copy.deepcopy(seed);g=c['entry_registers'];args=c['arguments'];instance=g['eax'];collector=0x100f000;body=0x100f100;data=bytearray.fromhex(c['banks'][0]['before'])
   def put(p,n):struct.pack_into('<I',data,p-0x1000000,n&0xffffffff)
   for offset,value in((4,(0x3f800000,0,0x7f812345)[option]),(8,args[0]|(0x10000000 if option==1 else 0)),(12,args[1]),(16,args[2]),(20,args[3]),(28,g['ebx'])):put(collector+offset,value)
   put(body+4,instance);stamps=bytearray(32);struct.pack_into('<I',stamps,0,0x12345678)
   c['banks'][0]['before']=data.hex();c['banks']=[b for b in c['banks']if not base.physical.STACK<=b['address']<base.physical.STACK+0x10000];c['banks'].append(dict(address=0x4f44ec,before=stamps.hex()))
   c.update(name='collector_'+str(len(rows))+'_'+seed['name']+'_option'+str(option),entry=0x24df20,receiver=collector,arguments=[body,0x100f200],entry_registers={},body=body,collector=collector)
   rows.append(c)
 for seed in(rows[0],rows[18],rows[36]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def main():
 text=inspect.getsource(parent.main)
 text=text.replace("source=ROOT/'src/physics/native_tu15_camera_instanced_query.cpp'","source=ROOT/'src/physics/native_tu15_instanced_ray_callbacks.cpp'")
 text=text.replace("'#include \"halo2/native_tu15_camera_instanced_query.hpp\"'","'#include \"halo2/native_tu15_instanced_ray_callbacks.hpp\"\\\\n#include \"halo2/native_tu15_camera_instanced_query.hpp\"'")
 text=text.replace("J::array({165,25,13,76,82,42,262,125,30,97,107})","J::array({46,165,25,13,76,82,42,262,125,30,97,107})")
 text=text.replace("('instance_stride','q.instance*0x58u','q.instance*0x54u'),('definition_filter','mask&0x40000u','mask&0x80000u'),('origin_translation','inverse+40u','inverse+44u')","('cached_fraction','current&0xffffffffu','current^0u'),('stamp_equality','m.word(stamp_address)==stamp','m.word(stamp_address)!=stamp'),('fraction_compare','keep=below_or_equal!=0','keep=below_or_equal==0')")
 # The neutral cached_fraction placeholder is removed; two semantic mutants
 # are enough for this additive composition (three core mutants pre-exist).
 text=text.replace("('cached_fraction','current&0xffffffffu','current^0u'),",'')
 text=text.replace("units=('native_tu15_camera_query_leaves.cpp'","units=('native_tu15_camera_instanced_query.cpp','native_tu15_camera_query_leaves.cpp'")
 start=text.index(" calls='''");end=text.index("\n check='''",start)
 calls='''{NativeTu15InstancedRayCallbackServices services;services.query_instance=[&](auto instance,auto mask,auto filter,auto origin,auto delta,auto output,auto&current){return native_tu15_query_camera_instance(m,{instance,mask,filter,origin,delta,output},{0x6001000u,0x6001034u,0x6001040u,0x600104cu,0x6002000u},current,0,observer);};native_tu15_collect_instanced_scene_ray(m,c.at("collector"),c.at("body"),fp,0,services,observer);}'''
 text=text[:start]+" calls="+repr(calls)+text[end:]
 start=text.index(" check='''");end=text.index("\n env=dict",start);text=text[:start]+" check=''"+text[end:]
 text=text.replace('return env[\'main\']()',"rc=env['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());code=_Code(base.Xbe(base.NEW_DEFAULT),'.text');sites=set(complete_body(code,0x24df20).code);report.update(scope=__doc__,collector_executed_sites=len(sites&parent.support.parent.EXECUTED),collector_unexecuted_sites=sorted(sites-parent.support.parent.EXECUTED),supplied_hit_result=False);base.save(path,report);return rc")
 env=dict(parent.__dict__,__file__=__file__,__doc__=__doc__,OWNERS=OWNERS,COUNTS=COUNTS,BACKEND=BACKEND,metadata=metadata,cases=cases,parent=parent,json=json,_Code=_Code,complete_body=complete_body);exec(text,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
