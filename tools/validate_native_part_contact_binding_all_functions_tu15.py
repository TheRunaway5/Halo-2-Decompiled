#!/usr/bin/env python3
"""Whole PART/material/shader composition with declared all11 function kinds."""
import inspect,json,os,struct,sys
from pathlib import Path
import validate_native_part_contact_binding_extended_tu15 as compatibility

def main():
 conveyor=compatibility.parent.parent;base=conveyor.parent.parent.parent.prop.base
 original_cases=conveyor.cases;write=Path.write_text;getsource=inspect.getsource
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 base.FILES=(*base.FILES,'tools/validate_native_part_contact_binding_all_functions_tu15.py')
 def cases(quick=False):
  rows=original_cases();templates=[r for r in rows if r['name']in('conveyor_positive_part_1_0_False_False','conveyor_positive_part_2_1_True_True')]
  assert len(templates)==2
  for template in templates:
   for kind in range(11):
    for flags in range(2):
     extra=[]
     for n in range(2):
      blob=0x8000+n*0x400;extra.extend([(0x6800+n*20+12,512),(0x6800+n*20+16,base.UI+blob),
       (blob,kind|(flags<<8)|((3 if kind==2 else 6 if kind==3 else 0)<<16)|((5 if kind==2 else 7 if kind==3 else 0)<<24))])
      coefficients=[0x3e000000,0x3f600000,0x3f000000,0x3e800000,0x3f800000,0xbe800000,0x40000000,0x3e800000]
      extra.extend((blob+0x14+i*4,coefficients[i%8])for i in range(64))
      if kind==9:extra.extend([(blob+0x1c,0x40000000),(blob+0x28,0x40000000)])
      if kind==10:
       for layer in range(2):
        at=blob+0x14+layer*48
        extra.extend([(at,0x40350d6f),(at+4,0),(at+8,0),(at+12,0),(at+16,0x3f800000),(at+20,0),(at+24,0),(at+28,0)])
     rows.append(dict(template,name='extended_function_'+str(kind)+'_'+str(flags)+'_'+template['name'],user_words=[*template['user_words'],*extra],whole_function_extended=True,declared_function_kind=kind))
  return rows
 conveyor.cases=cases
 original_inputs=base.reference_inputs
 def inputs(image):
  banks,proof=original_inputs(image);banks=dict(banks)
  for address,size in((0x46a000,0x600),(0x4752a8,0x5000),(0x44b884,11),(0x48d4f4,4),(0x473360,8)):
   for existing in list(banks):
    if address<=existing and existing+len(banks[existing])<=address+size:
     assert banks[existing]==image.read(existing,len(banks[existing]))
     del banks[existing]
   banks[address]=image.read(address,size)
  return banks,dict(proof,declared_extended_functions='All11 source-format function kinds through real PART caller; readonly original curve tables/constants and normal CRT mode, no supplied results')
 base.reference_inputs=inputs
 metadata=base.metadata
 extra_owners=(0x17f8a0,0x17f790,0x13e9a0,0x37cde8,0x37ce0c,0x37cfd1,0x332084,0x332099,0x3320c9,0x33216d,0x332122)
 def all_metadata(image,banks):
  base.OWNERS=tuple(dict.fromkeys((*base.OWNERS,*extra_owners)))
  return metadata(image,banks)
 base.metadata=all_metadata
 def source(function):
  text=getsource(function)
  text=text.replace("'--jobs','2'","'--jobs','1'")
  text=text.replace("'--test-jobs','1'","'--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log'")
  text=text.replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(Path(os.environ['HALO2_TEST_SCRATCH'])/'allfunctions-original-observer.cpp')")
  # Outer parent fixture layers add their own scalar constants after inputs().
  # Coalesce those identical readonly bytes at the final original builder.
  anchor="    owned=scratch/'original';owned.mkdir();"
  if anchor in text:
   merge="    for readonly in list(banks):\n        if 0x46a000<=readonly and readonly+len(banks[readonly])<=0x46a600:\n            assert banks[readonly]==image.read(readonly,len(banks[readonly]));del banks[readonly]\n    banks[0x46a000]=image.read(0x46a000,0x600)\n"
   text=text.replace(anchor,merge+anchor)
  needle="index=all_cases.index(case);fp=[0x27f|((index%4)<<10),((index%8)<<11)|((index%4)<<8)|(index%32),0x1f80|((index%4)<<13)];case['before_fp']=fp"
  if needle in text:
   text=text.replace(needle,"index=all_cases.index(case);mode=0 if case.get('whole_function_extended')else index%4;fp=[0x27f|(mode<<10),((index%8)<<11)|((index%4)<<8)|(index%32),0x1f80|(mode<<13)];case['before_fp']=fp")
  return text
 inspect.getsource=source
 def staged_write(path,text,*args,**kwargs):
  if path.is_relative_to(scratch)and path.name=='native_tu15_array_growth_tests.cpp':
   text=text.replace('"Unbacked native array extent"','"Unbacked native array extent at "+std::to_string(p)+" width "+std::to_string(n)')
   text=text.replace('  std::size_t cases{},stores{},read_count{},faults{},mutations{};','  std::size_t cases{},stores{},read_count{},faults{},mutations{};std::array<std::uint64_t,11>declared_kind_calls{},declared_kind_profiles{};')
   anchor='      if(read_pass){J expected='
   assert text.count(anchor)==1
   check='''      if(c.value("whole_function_extended",false)){
        const auto kind=c.at("declared_function_kind").get<unsigned>();
        check(!failed&&!required&&part_binding_activity.function.calls>0&&part_binding_activity.function.kinds[kind]==part_binding_activity.function.calls,name+" declared extended function kind was not executed");
        if(!read_pass){++declared_kind_profiles[kind];declared_kind_calls[kind]+=part_binding_activity.function.calls;}
      }
'''
   text=text.replace(anchor,check+anchor)
   text=text.replace('{"passed",true},{"cases",cases}', '{"passed",true},{"declared_kind_profiles",declared_kind_profiles},{"declared_kind_calls",declared_kind_calls},{"cases",cases}')
  # Execute the unchanged complete CRT child to its real return using a
  # hardware breakpoint. Its independently qualified internal DATA accesses
  # are opaque here, while the caller/curve reads and full FP/banks remain.
  if path.is_relative_to(scratch)and'const bool repeated_empty='in text and'PTRACE_SINGLESTEP'in text:
   text=text.replace('check(site!=sites.end(),"Original array escaped authenticated complete bodies");','if(site==sites.end())throw std::runtime_error("Original array escaped authenticated complete bodies at "+std::to_string(pc));')
   needle='    const bool repeated_empty='
   assert text.count(needle)==1
   skip='''    if(pc==0x17f8e9u||pc==0x17f7c5u){
      const auto returned=pc+5u;const auto debug_offset=offsetof(struct user,u_debugreg);
      request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(debug_offset),reinterpret_cast<void*>(std::uintptr_t(returned)));
      request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(debug_offset+7*sizeof(unsigned long)),reinterpret_cast<void*>(1));
      request(PTRACE_CONT,child.pid);wait();request(PTRACE_GETREGS,child.pid,nullptr,&regs);
      check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP&&std::uint32_t(regs.rip)==returned,"Real original curve CRT did not return");
      request(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(debug_offset+7*sizeof(unsigned long)),nullptr);continue;
    }
'''
   text='#include <cstddef>\n'+text.replace(needle,skip+needle)
  return write(path,text,*args,**kwargs)
 staged_write(scratch/'allfunctions-original-observer.cpp',(base.ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text())
 Path.write_text=staged_write
 try:return compatibility.main()
 finally:
  Path.write_text=write;inspect.getsource=getsource
  path=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if path.exists():
   d=json.loads(path.read_text());d.update(all_eleven_declared_parent_functions=True,
    scope='Existing217 plus44declared all11kind PART/material/shader/source-caller/depth0 cases, using extended productionbuilder and genuineoriginalcurve/powchildren. Originalcompletefinite fmod CRT child executes unmodified to authenticreturn and is opaque in composedreadledger; separate640curve proofs qualify its actualmath/FP. Allpersistentbanks/stores/parentreads/FP remaincompared. Explicit normal-CW pow policy, no actualMCC/live function-kind claim.')
   activity={}
   for compiler in('native-gcc','native-clang'):
    logs=[path.parent/compiler/'tests.log',*(path.parent/compiler).rglob('LastTest.log')]
    for log in logs:
     if not log.exists():continue
     for line in log.read_text().splitlines():
      if line.startswith('{'):
       row=json.loads(line)
       if 'declared_kind_profiles'in row:activity[compiler]=dict(profiles=row['declared_kind_profiles'],calls=row['declared_kind_calls'])
   if d.get('passed'):
    if set(activity)!={'native-gcc','native-clang'}:
     d.update(passed=False,error='Required bounded positive activity output was not retained');path.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
    assert set(activity)=={'native-gcc','native-clang'}
    assert all(row['profiles']==[4]*11 and all(row['calls'])for row in activity.values())
    d['positive_declared_function_kind_activity']=activity
   path.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':raise SystemExit(main())
