#!/usr/bin/env python3
"""Native313390 source-ordered pair geometry with real sphere/capsule supports.

Only parent read/write/observer prefixes are admitted here; child fault domains
are retained in their separately qualified shape modules. No support result is
supplied. The16B transformed direction has an independent native DATA lifetime.
"""
import copy,inspect,struct
from pathlib import Path
import validate_native_solver_hull_append_tu15 as parent
base=parent.base;ROOT=Path(__file__).resolve().parents[1]
ORIGINAL_OBSERVER=base.observer_source
def pair_observer_source():
 text=ORIGINAL_OBSERVER()
 text=text.replace('J executed=J::array();','J support_directions=J::array();J executed=J::array();')
 text=text.replace('const auto site=sites.find(pc);','if(pc==0x313425u){const auto pointer=memory(child.pid,std::uint32_t(regs.rsp),4);std::uint32_t direction{};std::memcpy(&direction,pointer.data(),4);support_directions.push_back(hex(memory(child.pid,direction,16)));}const auto site=sites.find(pc);')
 return text.replace('{"executed_sites",executed}', '{"support_directions",support_directions},{"executed_sites",executed}')
def cases(image):
 rows=[]
 def make(kinds,edge=0x3f000001,rounding=0,alias=0):
  lo=0x1000000;data=bytearray((n*11+len(rows)*3)&255 for n in range(0x3000));context=lo+0x100;first=lo+0x400;second=lo+0x500;tableA=lo+0x600;tableB=lo+0x700;direction=lo+0x800;pair=lo+0x900
  if alias==1:direction=pair
  if alias==2:pair=context
  def put(p,n):struct.pack_into('<I',data,p-lo,n)
  put(context+0x40,first);put(context+0x44,second);put(first,tableA);put(second,tableB);put(tableA+0x30,kinds[0]);put(tableB+0x30,kinds[1])
  for at,words in[(direction,(0x3f800001,0xbe000001,edge,edge)),(first+0x10,(0x3f800000,edge,0xbf000001,edge)),(first+0x20,(0xbe800001,0x3f000000,edge,0x3f000000)),(second+0x10,(0x3e800001,edge,0x3f800000,edge)),(second+0x20,(0x3f000001,0xbf800000,edge,0xbf000000)),(context,(0x3f800001,0x3e000001,0xbe800001,edge)),(context+16,(0xbe000001,0x3f800001,0x3d800001,edge)),(context+32,(0x3e800001,0xbe000001,0x3f800001,edge)),(context+48,(0x3f800001,0xc0000001,0x40400001,edge))]:
   for n,value in enumerate(words):put(at+n*4,value)
  rows.append(dict(name='pair_'+str(len(rows))+'_alias'+str(alias),entry=0x313390,receiver=context,arguments=[direction,pair],relative=0,banks=[dict(address=lo,before=data.hex()),dict(address=0x461960,before=image.read(0x461960,16).hex()),dict(address=0x46a0e8,before=image.read(0x46a0e8,4).hex())],before_fp=[0x27f|(rounding<<10),((len(rows)%8)<<11)|0x453f,0x1f80|(rounding<<13)],stop_pc=base.physical.STOP,boundary=None))
 for kinds in[(0x2e8d20,0x2e8d20),(0x2e7d90,0x2e8d20),(0x2e8d20,0x2e7d90),(0x2e7d90,0x2e7d90)]:
  for edge in [0x3f000001,0,0x80000000,1,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345]:
   for rounding in range(4):make(kinds,edge,rounding)
 for alias in [1,2]:make((0x2e7d90,0x2e7d90),alias=alias)
 make((0x2e8d20,0x2e8d20));c=rows[-1];raw=bytearray.fromhex(c['banks'][0]['before']);
 for p,words in [(0x1000100,(0x60ad78ec,0xe0ad78ec,0x3f800000,0)),(0x1000110,(0,0x3f800000,0,0)),(0x1000120,(0,0,0x3f800000,0)),(c['arguments'][0],(0xbf800000,0xbf800000,0xbf800000,0))]:
  for n,value in enumerate(words):struct.pack_into('<I',raw,p-0x1000000+n*4,value)
 c['banks'][0]['before']=raw.hex();c['name']='private_direction_cancellation'
 for seed in(rows[0],rows[72],rows[-1]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_moving_shape_process.hpp"','#include "halo2/native_tu15_solver_hull_support.hpp"');text=text.replace('fixture.at("body_sites")[0]==274','fixture.at("body_sites")[0]==75')
 text=text.replace('struct Fault{};','constexpr std::uint32_t PRIVATE=0x6000000u;bool native_private(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=0x1000;}struct Fault{};').replace('if(log_reads)reads.push_back','if(log_reads&&!native_private(p,b.size()))reads.push_back').replace('if(log_writes)raw_stores.push_back','if(log_writes&&!native_private(p,b.size()))raw_stores.push_back');text=text.replace('const auto fault=c.value("fault",J{});','before.banks.emplace(PRIVATE,std::vector<std::byte>(0x1000,std::byte{0xa5}));const auto fault=c.value("fault",J{});')
 begin=text.index('   services.classify_shape=');end=text.index('   const auto cw=',begin);body='''   NativeTu15SolverHullSupportServices supports;
   SourcePhysicsContactRecordObserver filtered;if(observer)filtered=[&](const auto&e){if(!native_private(e.address,e.written.size()))observer(e);};
   unsigned support_calls{};supports.support=[&](auto shape,auto owner,auto direction,auto output,auto&state){if(++support_calls==2){const auto region=m.at(direction,16);const auto data=std::span<const std::byte>(m.banks.at(region->first)).subspan(direction-region->first,16);check(c.at("support_directions").size()==1&&hex(data)==c.at("support_directions")[0].get<std::string>(),name+" transformed direction differs at actual child lifetime");}native_tu15_contact_shape_support(m,shape,owner,direction,output,{PRIVATE+0x100,PRIVATE+0x110,nullptr},state,0,filtered);};
''';text=text[:begin]+body+text[end:];old='native_tu15_process_moving_shape_contact(m,{c.at("receiver")},{c.at("arguments")[0]},{c.at("arguments")[1]},{c.at("arguments")[2]},c.at("arguments")[3],{c.at("relative")},fp,0,services,observer,&stats);';new='native_tu15_solver_hull_support_pair(m,c.at("receiver"),c.at("arguments")[0],c.at("arguments")[1],PRIVATE,fp,0,supports,filtered,&stats);';assert old in text;return text.replace(old,new).replace('NativeTu15MovingShapeProcessServices services;','').replace('NativeTu15MovingShapeProcessStats stats;','NativeTu15SolverHullSupportStats stats;')
def main():
 base.observer_source=pair_observer_source
 source=inspect.getsource(parent.main).replace('base.OWNERS=(0x313290,)','base.OWNERS=(0x313390,0x2e8d20,0x2e7d90)');source=source.replace("'counts==[32]'","'counts[0]==75'").replace("'complete_body(code,0x313290)'","'complete_body(code,0x313390)'")
 begin=source.index("source=source[:begin]+'''");end=source.index("+source[end:]",begin)+len('+source[end:]');source=source[:begin]+'''source=source[:begin]+"  for name,needle,replacement in(('wrong_transpose_order','add(add(xterm,yterm),zterm)','add(add(zterm,yterm),xterm)'),('wrong_final_translation','add(zterm,translation)','add(zterm,zterm)'),('wrong_second_output','point_pair+0x10u);','point_pair+0x20u);')):"+source[end:]'''+source[end:]
 source=source.replace("source=source.replace('negative_controls=list(variants),','negative_controls=list(variants),explicit_oracle_gaps=[dict(instruction=0x3132e8,phase=\"RMW write-fault: physical preinstruction stop omits indivisible count read; no matching prefix claimed\")],')","source=source.replace('negative_controls=list(variants),','negative_controls=list(variants),private_direction_in_lifetime_compared=True,')")
 source=source.replace('"units=()"','"units=(\'native_tu15_contact_shape.cpp\',)"')
 source=source.replace(" source=source.replace('complete_body",''' source=source.replace("      if e['instruction']in seen:continue","      if not(0x313390<=e['instruction']<0x313470):continue\\n      if e['instruction']in seen:continue").replace("for n,e in enumerate(seed['writes']):","for n,e in enumerate(seed['writes']):\\n     if not(0x313390<=e['instruction']<0x313470):continue")
 source=source.replace('complete_body''')
 environment=dict(parent.__dict__,cases=cases,driver_source=driver_source,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
