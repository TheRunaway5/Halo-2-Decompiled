#!/usr/bin/env python3
"""Native314710 cdecl wrapper and selected313BB0, source context while alive."""
import copy,inspect,struct
from pathlib import Path
import validate_native_solver_hull_single_vertex_tu15 as parent
base=parent.base;pair=parent.pair;ROOT=Path(__file__).resolve().parents[1]
ORIGINAL_OBSERVER=pair.pair_observer_source
def observer_source():
 text=ORIGINAL_OBSERVER().replace('J support_directions=J::array();','J hull_contexts=J::array(),support_directions=J::array();')
 text=text.replace('const auto site=sites.find(pc);','if(pc==0x313bb0u)hull_contexts.push_back(hex(memory(child.pid,std::uint32_t(regs.rcx),84)));const auto site=sites.find(pc);')
 return text.replace('{"support_directions",support_directions}','{"hull_contexts",hull_contexts},{"support_directions",support_directions}')
def cases(image):
 rows=[]
 for seed in parent.cases(image):
  if seed['name'].startswith('fault_seed'):continue
  c=copy.deepcopy(seed);raw=bytearray.fromhex(c['banks'][0]['before']);context=c['receiver'];word=lambda p:struct.unpack_from('<I',raw,p-0x1000000)[0]
  c['arguments']=[word(context+0x40),word(context+0x44),context,word(context+0x48),word(context+0x50),c['arguments'][0]];c['entry']=0x314710;c['receiver']=0;c['banks'].append(dict(address=0x48c974,before=bytes(8).hex()));c['name']='query_'+str(len(rows));rows.append(c)
 for seed in [rows[0],rows[1],rows[40],rows[-1]]:
  c=copy.deepcopy(seed);next(b for b in c['banks']if b['address']==0x48c974)['before']=struct.pack('<2I',0x1002800,0x1002900).hex();c['name']='profile_'+c['name'];rows.append(c)
 for seed in[rows[0],rows[40],rows[-1]]:c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=parent.driver_source().replace('#include "halo2/native_tu15_solver_hull_single_vertex.hpp"','#include "halo2/native_tu15_solver_hull_query.hpp"');text=text.replace('fixture.at("body_sites")[0]==742','fixture.at("body_sites")[0]==53')
 old='const auto result=native_tu15_complete_single_vertex_solver_hull(m,c.at("receiver"),c.at("arguments")[0],{PRIVATE+0x200u,PRIVATE+0x204u,{PRIVATE,PRIVATE+0x10u,PRIVATE+0x30u,PRIVATE+0x60u,PRIVATE+0x20u}},fp,0,supports,filtered,&stats);'
 new='const auto&a=c.at("arguments");const auto result=native_tu15_query_solver_hull(m,{{a[0]},{a[1]},a[2],a[3],a[4],a[5]},{PRIVATE+0x300u,{PRIVATE+0x200u,PRIVATE+0x204u,{PRIVATE,PRIVATE+0x10u,PRIVATE+0x30u,PRIVATE+0x60u,PRIVATE+0x20u}}},fp,0,supports,[&](){return c.at("timestamps").at(timestamp_index++).at("value").get<std::uint64_t>();},filtered,&stats);'
 new='std::size_t timestamp_index{};'+new
 assert old in text;text=text.replace(old,new).replace('NativeTu15SolverHullSingleVertexStats stats;','NativeTu15SolverHullQueryStats stats;')
 needle='   check(failed==fault_case';assert needle in text
 text=text.replace(needle,'   if(!c.at("hull_contexts").empty()){check(c.at("hull_contexts").size()==1,name+" source context count differs");const auto&data=m.banks.at(PRIVATE);check(hex(std::span<const std::byte>(data).subspan(0x300,84))==c.at("hull_contexts")[0].get<std::string>(),name+" fullydefined context DATA differs while source lives");}\n'+needle)
 return text
def main():
 source=inspect.getsource(parent.main).replace('base.OWNERS=(0x313bb0,','base.OWNERS=(0x314710,0x313bb0,')
 source=source.replace('base.observer_source=pair.pair_observer_source','base.observer_source=observer_source')
 source=source.replace("counts[0]==742","counts[0]==53").replace("source=ROOT/'src/physics/native_tu15_solver_hull_single_vertex.cpp'","source=ROOT/'src/physics/native_tu15_solver_hull_query.cpp'")
 source=source.replace("units=('native_tu15_contact_shape.cpp','native_tu15_solver_hull_support.cpp')","units=('native_tu15_contact_shape.cpp','native_tu15_solver_hull_support.cpp','native_tu15_solver_hull_single_vertex.cpp')")
 source=source.replace("source,ROOT/'include/halo2/native_tu15_solver_hull_single_vertex.hpp'","source,ROOT/'include/halo2/native_tu15_solver_hull_query.hpp'")
 source=source.replace("actual['boundary_gp'][4]==physical.SP+8","actual['boundary_gp'][4]==physical.SP+4").replace(" and actual['boundary_gp'][1]==c['receiver']",'')
 source=source.replace('0x313bb0<=e[\'instruction\']<0x3146f5','0x314710<=e[\'instruction\']<0x3147ce')
 source=source.replace('complete_body(code,0x313bb0)','complete_body(code,0x314710)')
 source=source.replace("('wrong_decrement','current_count-1u','current_count+1u'),('wrong_result','return 0u','return 1u'),('wrong_extension','if(extend){','if(!extend){')","('wrong_shape','p+0x40u,input.first.identity','p+0x40u,input.second.identity'),('wrong_context_field','p+0x4cu,0','p+0x4cu,1'),('wrong_difference_identity','p+0x50u,input.differences','p+0x50u,input.simplex')")
 source=source.replace('scope=__doc__,full_returns=', 'scope=__doc__,external_clock_samples_only=True,full_returns=')
 environment=dict(parent.__dict__,cases=cases,driver_source=driver_source,observer_source=observer_source,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
