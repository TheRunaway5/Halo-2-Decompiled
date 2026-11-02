#!/usr/bin/env python3
"""Whole30CD00 with real native322D20 creation and cached-point reduction."""
import copy,inspect,struct
import validate_native_contact_point_create_tu15 as component
from oracle_smg_firing_1_1 import ROOT
U=component.base.UI
RECORDS,COUNT,CONTACT,SETTINGS,T1,T2=(U+p for p in(0xb000,0xb400,0xd000,0x1200,0xc000,0xc100))
UNITS=('native_tu15_contact_publication.cpp',*component.UNITS,
       'native_tu15_contact_reduction.cpp','native_tu15_geometry.cpp','native_tu15_contact_point_removal.cpp')
def cases():
 seed=next(r for r in component.cases()if r['name']=='create_1_0_0.25');rows=[]
 for count in (0,1,3,4):
  for mode in ('new','matched','coincident','limit'):
   if not count and mode=='matched':continue
   row=copy.deepcopy(seed);data=dict(row['user_words']);p=component.MANAGER+16
   def put(address,value):data[address-U]=value&0xffffffff
   properties=251 if mode=='limit'else count
   put(p+0x94,properties);put(p+0x98,0x80000000|max(properties,8));put(p+0x38,count);put(p+0x24,count)
   put(component.IDS,0x03020100);put(COUNT,count);put(component.CONTEXT+12,SETTINGS);put(SETTINGS+4,component.bits(.5))
   for payload,transform in ((component.FIRST,T1),(component.SECOND,T2)):
    put(payload+8,transform)
    for axis in range(4):
     for lane in range(4):put(transform+32+axis*16+lane*4,component.bits(float(axis==lane)))
   for n in range(count):
    position=(0.,0.,0.,.5)if mode=='coincident'else ((0.,0.,0.,.5),(1.,0.,0.,.5),(0.,1.,0.,.5),(0.,0.,1.,.5))[n]
    for offset,vector in ((0,position),(16,(0.,0.,0.,0.)),(32,(1.,0.,0.,0.))):
     for lane,v in enumerate(vector):put(RECORDS+n*48+offset+lane*4,component.bits(v))
    put(RECORDS+n*48+28,0x3f000000|n)
   position=(0.,0.,0.,0.)if mode in ('matched','coincident')else(99.,.25,.125,0.)
   normal=(-1.,0.,0.,0.)if mode=='coincident'else(1.,0.,0.,0.)
   for offset,vector in ((16,position),(48,(.5,.25,.125,0.)),(64,normal)):
    for lane,v in enumerate(vector):put(CONTACT+offset+lane*4,component.bits(v))
   put(CONTACT+32,component.bits(.125));put(CONTACT+36,component.bits(.5))
   row.update(name=f'publish_create_{count}_{mode}',entry=0x30cd00,receiver=0,arguments=[component.FIRST,component.SECOND,component.CONTEXT,CONTACT,component.MANAGER,RECORDS,COUNT],user_words=list(data.items()));rows.append(row)
 return rows
def main():
 text=inspect.getsource(component.main)
 text=text.replace('base.OWNERS=(0x322d20,','base.OWNERS=(0x30cd00,0x30caa0,0x2e4690,0x322f70,0x325d90,0x3224d0,0x31fee0,0x320100,0x322d20,')
 text=text.replace('(0x46a16c,4))','(0x46a16c,4),(0x41c500,16),(0x46a560,4),(0x46a0e8,4))')
 text=text.replace('base.SP+20','base.SP+4')
 text=text.replace("('create_0_2_0.25','create_4_2_-0.5')","('publish_create_4_new','publish_create_0_new')")
 text=text.replace("not 0x322d20<=event['instruction']<=0x322f62","not 0x30cd00<=event['instruction']<=0x30cf31")
 text=text.replace('#include "halo2/native_tu15_contact_point_create.hpp"','#include "halo2/native_tu15_contact_publication.hpp"\\n#include "halo2/native_tu15_contact_point_removal.hpp"\\n#include "halo2/native_tu15_contact_point_create.hpp"')
 text=text.replace("'NativeTu15ContactPointCreateStats activity;'","'NativeTu15ContactPublicationStats activity;'")
 old='''      try{const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();result=native_tu15_create_contact_point(memory,c.at("receiver"),{args[0]},{args[1]},{args[2]},args[3],{PRIVATE},fp,0,services,{},&activity);'''
 new='''      NativeTu15ContactPublicationServices publication;
      publication.create_point=[&](auto manager,auto owner,auto first,auto second,auto context,auto packet,auto&f){if(owner.identity!=0x322d20u)throw Required{};return native_tu15_create_contact_point(memory,manager,first,second,context,packet,{PRIVATE+0x200u},f,0,services);};
      publication.removal.remove_point=[&](auto manager,auto owner,auto id,auto&f){if(owner.identity!=0x322f70u)throw Required{};NativeTu15ContactPointRemovalServices removal;removal.collector_removed=[&](auto island,auto target,auto,auto info,auto&){if(target.identity!=0x320100u)throw Required{};native_tu15_remove_island_constraint_info(memory,{island},{info});};native_tu15_remove_contact_point(memory,manager,id,{PRIVATE+0x300u,PRIVATE+0x320u,PRIVATE+0x340u},f,removal);};
      try{const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();native_tu15_publish_contact(memory,{args[0]},{args[1]},{args[2]},{args[3]},args[4],args[5],args[6],{PRIVATE},fp,0,publication);
'''
 assert text.count(old)==1;text=text.replace(old,new)
 text=text.replace('if(!failed&&!required)check(result==(c.at("return_word").get<std::uint32_t>()&0xffffu),name+" created contact ID differs");','')
 text=text.replace('check(activity.unprovided_event_reads==0,name+" native event read unprovided bytes");','')
 text=text.replace("(project/'driver.cpp').write_text(driver);source=",'''driver=driver.replace('"Unbacked native array extent"','"Unbacked native array extent address="+std::to_string(p)+" bytes="+std::to_string(n)').replace('catch(const Required&){required=true;}','catch(const Required&){required=true;}catch(const std::exception&e){throw std::runtime_error(name+" "+e.what());}')
  (project/'driver.cpp').write_text(driver);source=''')
 text=text.replace("driver=driver.replace('    }\\n    ++cases;faults+=',guard_model+'    }\\n    ++cases;faults+=')",'driver=driver')
 text=text.replace("(('wrong_material','properties+8u);','properties+12u);'),('wrong_relative','subtract(second_velocity,first_velocity)','subtract(first_velocity,second_velocity)'),('wrong_limit','initial_count)>250','initial_count)>252'))","(('wrong_match','if(odd(compare_top_pop()))','if(!odd(compare_top_pop()))'),('wrong_weight','m.word(0x30cdf9u,target+0xcu,weight);','m.word(0x30cdf9u,target+0xcu,weight^1u);'),('wrong_preserved_id','push(m.word(target+0x1cu));','push(0x3f000000u);'))")
 text=text.replace('if 0x322d20<=pc<=0x322f62','if 0x30cd00<=pc<=0x30cf31').replace("0x322d20).code)-sites","0x30cd00).code)-sites")
 text=text.replace("'include/halo2/native_tu15_contact_point_create.hpp',*[","'include/halo2/native_tu15_contact_publication.hpp',*[")
 text=text.replace("guard_models=['raw observer refuses before any DATA read/store','event padding2byte guard','current-body field before genuine world writer guard','normal initialized event projection excludes padding'],",'')
 text=text.replace("scope='Declared coherent manager", "scope='Native30CD00→realnative322D20 and actual322F70/30CAA0 reduction; no returned point outcome supplied. Declared coherent manager")
 env=dict(component.__dict__,__file__=__file__,UNITS=UNITS,cases=cases);exec(text,env);return env['main']()
if __name__=='__main__':main()
