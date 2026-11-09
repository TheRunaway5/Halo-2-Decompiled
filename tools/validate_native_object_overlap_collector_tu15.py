#!/usr/bin/env python3
"""Physical1CDFA0 collector plus genuine2FAB70 sphere/capsule composition.

The additional owner is the actual terminal4615B8 collector captured in the
Coagulation failing World. Only ordinary before inputs are varied; unchanged
TU1.5 instructions produce every predicate, callback and retained byte.
"""
import copy,inspect,json,random,struct,sys
import validate_native_sphere_capsule_overlap_tu15 as base

ORIGINAL_CASES=base.cases
ORIGINAL_DRIVER=base.driver_source
ORIGINAL_METADATA=base.metadata
EXTRA_OWNERS=(0x1cdfa0,0x2e8cf0,0x31dd70,0x31edf0,0x31ede0,0xad390,0xa9520,0xab4b0)
B=base.BASE
ROOT,BODY,PROPERTIES,USER_DATA,SHAPE_TABLE=(B+n for n in(0x1000,0x1100,0x1200,0x1300,0x1400))

def metadata(image):
 result=ORIGINAL_METADATA(image);code=base._Code(image,'.text')
 for site in result['sites']:
  instruction=code.at(site['instruction'])
  if instruction.mnemonic in('push','call'):site['implicit_control_store']=instruction.mnemonic
 return result

def observer_source():
 text=base.observer_source()
 text=text.replace('user_regs_struct regs{};','J control_stores=J::array();user_regs_struct regs{};')
 old='const auto registers=gp(regs);J pending=J::array();'
 assert text.count(old)==1
 text=text.replace(old,old+'''J control=nullptr;
    if(site->second.contains("implicit_control_store")){
      const auto p=registers[4]-4u;if(retained(p,4))control=J{{"instruction",pc},{"address",p},{"bytes",4},{"kind",site->second.at("implicit_control_store")},{"sequence",count}};
    }
''')
 old='reads.push_back(J{{"instruction",pc},{"address",p},{"bytes",n}});'
 assert text.count(old)==1
 text=text.replace(old,'reads.push_back(J{{"instruction",pc},{"address",p},{"bytes",n},{"sequence",count}});')
 old='pending.push_back(J{{"instruction",pc},{"address",p},{"before",hex(memory(child.pid,p,n))},{"bytes",n}});'
 assert text.count(old)==1
 text=text.replace(old,'pending.push_back(J{{"instruction",pc},{"address",p},{"before",hex(memory(child.pid,p,n))},{"bytes",n},{"sequence",count}});')
 old='    for(auto&a:pending){a["written"]=hex(memory(child.pid,a.at("address"),a.at("bytes")));'
 assert text.count(old)==1
 text=text.replace(old,'''    if(!control.is_null()){control["written"]=hex(memory(child.pid,control.at("address"),4));control_stores.push_back(control);}
'''+old)
 old='{"timestamps",timestamps}'
 assert text.count(old)==1
 return text.replace(old,old+',{ "implicit_control_stores",control_stores}')

def qualify_private_control_afterimages(rows,output):
 witnesses=[]
 for row in rows:
  row['private_control_afterimages']=[]
  # These are exactly3128C0 arithmetic DATA cells, not a caller stack range.
  for address in range(row['closest_point']+8,row['closest_point']+16):
   semantic=[];control=[]
   for event in row['writes']:
    if event['address']<=address<event['address']+len(bytes.fromhex(event['written'])):semantic.append(event)
   for event in row.get('implicit_control_stores',[]):
    if event['address']<=address<event['address']+4:control.append(event)
   if not control:continue
   last_control=max(control,key=lambda e:e['sequence'])
   if semantic and max(e['sequence']for e in semantic)>last_control['sequence']:continue
   assert semantic,(row['name'],hex(address),'control replaced no qualified arithmetic DATA')
   last_semantic=max(semantic,key=lambda e:e['sequence'])
   assert last_semantic['instruction']in(0x312904,0x312911,0x31293d,0x312980)
   assert base.physical.STACK<=address<base.physical.SP
   assert last_control['kind']in('push','call')
   assert not any(e['sequence']>last_control['sequence']and e['address']<=address<e['address']+e['bytes']for e in row['reads']),('ordinary read after control replacement',row['name'],hex(address))
   # A void call cannot return a pointer to these dead private cells through
   # its public argument words or any preserved/nonvolatile register.
   assert address not in row['boundary_stack']
   assert address not in[row['boundary_gp'][n]for n in(3,5,6,7)]
   value=bytes.fromhex(last_semantic['written'])[address-last_semantic['address']]
   item=dict(address=address,native_semantic_byte=value,arithmetic_lastwriter=last_semantic,
       original_control_lastwriter=last_control,no_subsequent_ordinary_read=True,
       below_public_entry_sp=True,void_return_or_preserved_register_escape=False)
   row['private_control_afterimages'].append(item)
  if row['private_control_afterimages']:witnesses.append(dict(name=row['name'],bytes=row['private_control_afterimages']))
 base.save(output/'dead-control-spill-witnesses.json',witnesses)

def cases(image):
 rows=ORIGINAL_CASES(image)
 def add(name,owner=0x2e8cf0,changes=(),seed=False):
  rng=random.Random(len(rows));data=bytearray(rng.randbytes(0x2000))
  def put(p,n):struct.pack_into('<I',data,p-B,n&0xffffffff)
  for p,n in((base.SECOND+12,ROOT),(ROOT+12,0),(ROOT+0x18,1),(ROOT+0x20,BODY),
      (BODY+0x30,PROPERTIES),(BODY+0x34,2),(PROPERTIES,0x2002),(PROPERTIES+4,91),
      (PROPERTIES+8,0x2001),(PROPERTIES+12,7),(base.SECOND,base.S),
      (base.S,SHAPE_TABLE),(SHAPE_TABLE+0x14,owner),(base.S+8,USER_DATA),
      (base.COLLECTOR+12,0xffffffff)):put(p,n)
  data[USER_DATA-B+0x1e]=255
  for p,n in changes:put(p,n)
  rows.append(dict(name=('fault_seed_'if seed else'')+'collector_'+name,kind='collector',
      entry=0x1cdfa0,receiver=base.COLLECTOR,arguments=[0xdeadbeef,base.SECOND],
      first=0xdeadbeef,second=base.SECOND,collector=base.COLLECTOR,data=B+0x1800,
      closest_point=B+0x1900,local_collector=B+0x1a00,configuration=0,
      banks=[dict(address=B,before=data.hex())],before_fp=[0x27f,0x453f,0x1f80],
      stop_pc=base.physical.STOP,boundary=None,declared_boundary=None))
 for owner in EXTRA_OWNERS[1:]:
  for excluded in(0xffffffff,7,8):add(f'class_{owner:x}_excluded_{excluded:x}',owner,((base.COLLECTOR+12,excluded),))
 for kind in(0,1,2,0xffffffff):add('root_type_'+hex(kind),changes=((ROOT+0x18,kind),))
 add('no_body',changes=((ROOT+0x20,0),))
 add('direct_root',changes=((base.SECOND+12,0),(base.SECOND+0x18,1),(base.SECOND+0x20,BODY)))
 add('deep_root',changes=((base.SECOND+12,ROOT+0x80),(ROOT+0x8c,ROOT)))
 for count in(0,1,2,0xffffffff,0x80000000):add('property_count_'+hex(count),changes=((BODY+0x34,count),))
 add('property_first',changes=((PROPERTIES,0x2001),(PROPERTIES+4,9)))
 add('property_missing',changes=((PROPERTIES,0),(PROPERTIES+8,0)))
 add('property_none',changes=((PROPERTIES+12,0xffffffff),))
 add('component_zero_excluded',changes=((BODY+0x34,0),(base.COLLECTOR+12,0)))
 for value in(0,1,2,15,16):add('reserved_userdata_'+str(value),changes=((base.S+8,value),))
 add('signed_negative_userdata',changes=((base.S+8,0x80000000),))
 rows[-1]['banks'].append(dict(address=0x80000000,before=(bytes(30)+b'\xff\x00').hex()))
 add('authored_nonphantom',changes=((USER_DATA+0x1c,0x00ffffff),))
 add('authored_phantom',changes=((USER_DATA+0x1c,0x0000ffff),))
 add('polygon_ignores_userdata',0xab4b0,((base.S+8,0xffffffff),))
 add('all_writes',seed=True)
 # Compose the actual two-adapter live chain and terminal1CDFA0 through the
 # unchanged2FAB70 query, not a supplied collector result.
 for original in ORIGINAL_CASES(image):
  if not original['name'].endswith('overlap') or original.get('kind')!='reverse':continue
  for excluded in(0xffffffff,7,8):
   row=copy.deepcopy(original);row['name']='fault_seed_composed_'+original['name']+'_'+hex(excluded)
   bank=next(b for b in row['banks']if b['address']==B);data=bytearray.fromhex(bank['before']);data+=bytes(0x1000)
   def put(p,n):struct.pack_into('<I',data,p-B,n&0xffffffff)
   terminal=B+0x1600;terminal_table=B+0x1700
   put(base.COLLECTOR,0x41ca40);put(base.COLLECTOR+8,terminal)
   put(terminal,terminal_table);put(terminal+12,excluded);put(terminal_table+4,0x1cdfa0)
   for n,descriptor in enumerate((base.FIRST,base.SECOND)):
    body=BODY+n*0x80;properties=PROPERTIES+n*0x80
    for p,value in((descriptor+12,0),(descriptor+0x18,1),(descriptor+0x20,body),
        (body+0x30,properties),(body+0x34,1),(properties,0x2001),(properties+4,7+n)):
     put(p,value)
   for n,(shape,owner)in enumerate(((base.S,0x2e8cf0),(base.C,0x31ede0))):
    table=SHAPE_TABLE+n*0x40;put(shape,table);put(shape+8,0);put(table+0x14,owner)
   bank['before']=data.hex();rows.append(row)
 return rows

def driver_source():
 text=ORIGINAL_DRIVER()
 old='   if(owner.identity==0x30b960u)'
 assert text.count(old)==1
 text=text.replace(old,'   if(owner.identity==0x1cdfa0u){native_tu15_collect_object_overlap_pair(m,receiver,b,observe);return;}\n'+old)
 old='   binding.query({0xdeadbeefu},{c.at("entry")},{c.at("first")},{c.at("second")},{0xbad0bad0u},{c.at("collector")},fp);'
 assert text.count(old)==1
 text=text.replace(old,'   if(c.at("kind")=="collector")binding.collect({c.at("collector")},{0x1cdfa0u},{c.at("first")},{c.at("second")},fp);\n   else '+old.strip())
 old='   if(c.at("kind")=="reverse")'
 assert text.count(old)==1
 text=text.replace(old,'   if(c.at("kind")=="collector")native_tu15_collect_object_overlap_pair(m,{c.at("collector")},{c.at("second")},observe);\n   else '+old.strip())
 old='for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" bank differs");'
 assert text.count(old)==1
 text=text.replace(old,'''for(const auto&b:c.at("banks")){const auto expected=bytes(b.at("after"));const auto&actual=m.banks.at(b.at("address"));if(actual!=expected){J differences=J::array();for(std::size_t n=0;n<actual.size()&&differences.size()<16;++n)if(actual[n]!=expected[n])differences.push_back(J::array({b.at("address").get<std::uint32_t>()+n,std::to_integer<unsigned>(actual[n]),std::to_integer<unsigned>(expected[n])}));throw std::runtime_error(name+" bank differs "+differences.dump());}}''')
 old='const auto&actual=m.banks.at(b.at("address"));if(actual!=expected)'
 assert text.count(old)==1
 text=text.replace(old,'''const auto&actual=m.banks.at(b.at("address"));auto checked=actual;
   for(const auto&w:c.at("private_control_afterimages")){const auto address=w.at("address").get<std::uint32_t>(),first=b.at("address").get<std::uint32_t>();if(address>=first&&std::uint64_t(address-first)<checked.size()){check(std::to_integer<unsigned>(actual[address-first])==w.at("native_semantic_byte"),name+" dead DATA arithmetic value differs");checked[address-first]=expected[address-first];}}
   if(checked!=expected)''')
 text=text.replace('if(actual[n]!=expected[n])differences.push_back','if(checked[n]!=expected[n])differences.push_back')
 old='const auto name=c.at("name").get<std::string>();Memory m;'
 assert text.count(old)==1
 text=text.replace(old,'''const auto name=c.at("name").get<std::string>();
#if defined(HALO2_COLLECTOR_COMPOSED_ONLY)
  if(!name.starts_with("fault_seed_composed"))continue;
#endif
  Memory m;''')
 return text

def main():
 base.OWNERS+=EXTRA_OWNERS
 base.UNITS+=('native_tu15_activation_services.cpp','native_tu15_contact_agent_factory.cpp')
 source=inspect.getsource(base.main)
 old='all(c["boundary_gp"][4]==physical.SP+4 for c in rows)'
 assert source.count(old)==1
 source=source.replace(old,'all(c["boundary_gp"][4]==physical.SP+(12 if c["kind"]=="collector"else 4) for c in rows)')
 marker="  fixture=scratch/'original.json'"
 assert source.count(marker)==1
 source=source.replace(marker,'  qualify_private_control_afterimages(rows,a.output)\n'+marker)
 marker="  cmake='cmake_minimum_required"
 assert source.count(marker)==1
 extra='''  collector_kernel=(project/'native_tu15_contact_pair_collectors.cpp').read_text()
  pair_mutations={
   'wrong_component_key':('==0x2001u','==0x2002u'),
   'wrong_exclusion':('excluded==component','excluded!=component'),
   'opposite_authored_byte':('word(user_data+0x1eu,1)!=0xffu','word(user_data+0x1eu,1)==0xffu'),
   'missing_dirty_byte':('std::pair{0x1ce03au,8u}','std::pair{0x1ce03au,9u}')}
  for name,(before,after)in pair_mutations.items():
   assert collector_kernel.count(before)==1,(name,collector_kernel.count(before))
   (project/(name+'.cpp')).write_text(collector_kernel.replace(before,after))
  point_mutations={
   'wrong_closest_projection':('query_dot(direction,from_point)','query_dot(direction,direction)'),
   'wrong_closest_ratio':('fdivs %0','fmuls %0')}
  for name,(before,after)in point_mutations.items():
   assert source.count(before)==1,(name,source.count(before))
   (project/(name+'.cpp')).write_text(source.replace(before,after))
'''
 source=source.replace(marker,extra+marker)
 source=source.replace("for name in('direct','binding',*mutations):","for name in('direct','binding',*mutations,*pair_mutations,*point_mutations):")
 source=source.replace("name in('direct','binding')else name+'.cpp'","name in('direct','binding')or name in pair_mutations else name+'.cpp'")
 source=source.replace("' '.join(UNITS[1:])","' '.join(name+'.cpp'if n=='native_tu15_contact_pair_collectors.cpp'and name in pair_mutations else n for n in UNITS[1:])")
 source=source.replace("   if name=='binding':", "   if name in point_mutations:cmake+=f'target_compile_definitions({target} PRIVATE HALO2_COLLECTOR_COMPOSED_ONLY=1)\\n'\n   if name=='binding':")
 start=source.index("  cmake='cmake_minimum_required")
 stop=source.index("  (project/'CMakeLists.txt').write_text(cmake);",start)
 source=source[:start]+'''  cmake='cmake_minimum_required(VERSION 3.20)\\nproject(object_overlap LANGUAGES CXX)\\ninclude(CTest)\\n'
  def configure(target):
   return f'target_include_directories({target} PRIVATE include)\\ntarget_compile_features({target} PRIVATE cxx_std_20)\\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\\n'
  common=[n for n in UNITS[1:]if n!='native_tu15_contact_pair_collectors.cpp']
  for target,units in [('common',common),('kernel_native',[UNITS[0]]),('collector_native',['native_tu15_contact_pair_collectors.cpp']),('driver_direct',['driver.cpp']),('driver_binding',['driver.cpp']),('driver_composed',['driver.cpp'])]:
   cmake+=f'add_library({target} OBJECT '+ ' '.join(units)+')\\n'+configure(target)
  cmake+='target_compile_definitions(driver_binding PRIVATE HALO2_SPHERE_QUERY_BINDING=1)\\ntarget_compile_definitions(driver_composed PRIVATE HALO2_COLLECTOR_COMPOSED_ONLY=1)\\n'
  targets=[]
  for name in('direct','binding',*mutations,*pair_mutations,*point_mutations):
   target='sphere_capsule_'+name;targets.append(target)
   driver='driver_binding'if name=='binding'else'driver_composed'if name in point_mutations else'driver_direct'
   parts=[f'$<TARGET_OBJECTS:{driver}>','$<TARGET_OBJECTS:common>']
   parts+=['$<TARGET_OBJECTS:kernel_native>']if name in('direct','binding')or name in pair_mutations else[name+'.cpp']
   parts+=[name+'.cpp']if name in pair_mutations else['$<TARGET_OBJECTS:collector_native>']
   cmake+=f'add_executable({target} '+ ' '.join(parts)+')\\n'+configure(target)
   prefix=''if name in('direct','binding')else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_test(NAME {target} COMMAND {prefix}$<TARGET_FILE:{target}> "{fixture}")\\n'
''' +source[stop:]
 source=source.replace('negative_controls=list(mutations)','negative_controls=[*mutations,*pair_mutations,*point_mutations],dead_control_spill_witnesses="dead-control-spill-witnesses.json",volatile_cpu_postimage_claimed=False,unmodified_original_afterbanks=True')
 source=source.replace("'tools/observe_native_constraint_step_tu15.cpp'):","'tools/observe_native_constraint_step_tu15.cpp','tools/validate_native_object_overlap_collector_tu15.py','include/halo2/native_tu15_contact_pair_collectors.hpp'):")
 env=dict(base.__dict__,cases=cases,driver_source=driver_source,metadata=metadata,observer_source=observer_source,qualify_private_control_afterimages=qualify_private_control_afterimages,__doc__=__doc__)
 exec(source,env)
 return env['main']()

if __name__=='__main__':raise SystemExit(main())
