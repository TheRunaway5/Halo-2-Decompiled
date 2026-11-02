#!/usr/bin/env python3
"""Whole221 gameplay removal through exact extended point/PART callback.

Declared coherent ordinary DATA with positive authored functions0..10; no
captured gameplay/conveyor caller or visible multiplayer claim.
"""
import copy,hashlib,inspect,json,os,struct,sys
from pathlib import Path
import validate_native_point_part_gameplay_composition_tu15 as bounded

EXTRA_OWNERS=(0x13de70,0x17f8a0,0x17f790,0x13e9a0,0x37cde8,0x37ce0c,
 0x37cfd1,0x332084,0x332099,0x3320c9,0x33216d,0x332122)
CORE=('native_tu15_function_evaluation_extended.cpp','native_tu15_function_curves.cpp','native_tu15_function_control_point.cpp')

def extended_cases(rows):
 u=bounded.parent.U;templates=[r for r in rows if r['name']in('part_prefix_0x281d40','effect_part_prefix_0x281d40')];assert len(templates)==2
 for template in templates:
  for kind in range(11):
   for flags in range(2):
    extra=[(0x6018,u+0x6200),(0x6224,u+0x6400),(0x6440,u+0x6800),
     (0x6448,u+0x6600),(0x6450,u+0x6500),(0x6460,u+0x6480),
     (0x6480,0),(0x6484,0),(0x6500,2<<9),(0x6600,4<<16),(0x6604,(5<<16)|1),
     (0x94b4,4),(0x94b8,u+0x5400)]
    for n in range(4):extra.extend([(0x9600+n*20+8,n<<16),(0x9600+n*20+16,0xabcd0001)])
    basis=((1.,0.,0.,0.,1.,0.),(0.,1.,0.,0.,0.,1.),(0.,0.,1.,1.,0.,0.),(.3,.4,.5,-.25,.75,.125))
    for n,vector in enumerate(basis):
     for axis,value in enumerate(vector):extra.append((0x5400+n*24+axis*4,struct.unpack('<I',struct.pack('<f',value))[0]))
    for n in range(2):
     blob=0xa000+n*0x400;extra.extend([(0x6800+n*20+8,0x3f800000),(0x6800+n*20+12,512),
      (0x6800+n*20+16,u+blob),(blob,kind|(flags<<8)|((3 if kind==2 else 6 if kind==3 else 0)<<16)|((5 if kind==2 else 7 if kind==3 else 0)<<24))])
     coefficients=[0x3e000000,0x3f600000,0x3f000000,0x3e800000,0x3f800000,0xbe800000,0x40000000,0x3e800000]
     extra.extend((blob+0x14+i*4,coefficients[i%8])for i in range(64))
     if kind==9:extra.extend([(blob+0x1c,0x40000000),(blob+0x28,0x40000000)])
     if kind==10:
      for layer in range(2):
       at=blob+0x14+layer*48;extra.extend([(at,0x40350d6f),(at+4,0),(at+8,0),(at+12,0),(at+16,0x3f800000),(at+20,0),(at+24,0),(at+28,0)])
    row=copy.deepcopy(template);row.update(name='whole_gameplay_function_'+str(kind)+'_'+str(flags)+'_'+template['name'],user_words=[*row['user_words'],*extra],whole_function_extended=True,declared_function_kind=kind);rows.append(row)
 return rows

def main():
 os.environ['HALO2_POINT_PART_STAGE']=os.environ['HALO2_POINT_PART_EXTENDED_STAGE']
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();getsource=inspect.getsource;write=Path.write_text
 source=getsource(bounded.main).replace('  return rows','  return extended_cases(rows)')
 source=source.replace('0x31c450,0x31c5e0)','0x31c450,0x31c5e0,*EXTRA_OWNERS)')
 old="'src/core/native_tu15_function_evaluation.cpp','include/halo2/native_tu15_part_contact_binding.hpp'"
 assert old in source
 source=source.replace(old,"*['src/core/'+n for n in CORE],'include/halo2/native_tu15_part_contact_binding_extended.hpp',*['include/halo2/'+n.replace('.cpp','.hpp')for n in CORE],'tools/validate_native_point_part_gameplay_extended_tu15.py'")
 old="  shutil.copyfile(ROOT/'src/core/native_tu15_function_evaluation.cpp',project/'function_evaluation.cpp')"
 assert old in source
 source=source.replace(old,"  for name in CORE:shutil.copyfile(ROOT/'src/core'/name,project/name)")
 source=source.replace("' '.join(native)+' function_evaluation.cpp)","' '.join(native)+' '+' '.join(CORE)+')")
 needle="  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text()"
 setup="""  banks[0x4f0414]=struct.pack('<I',U+0x6000)
  for address,size in((0x46a000,0x600),(0x4752a8,0x5000),(0x44b884,11),(0x48d4f4,4),(0x473360,8)):
   for existing in list(banks):
    if address<=existing and existing+len(banks[existing])<=address+size:
     assert banks[existing]==image.read(existing,len(banks[existing]));del banks[existing]
   banks[address]=image.read(address,size)
"""
 # The old wrapper inserts this observer line into the generated main via an
 # escaped literal. Extend its existing globals_setup string instead.
 old="  for p in(0x474d04,0x474d10):"
 assert old in source
 source=source.replace(old,"  banks[0x4f0414]=struct.pack('<I',U+0x6000)\n"+old)
 source=source.replace(" source=source.replace(needle,globals_setup+needle)"," globals_setup+=EXTRA_BANK_SETUP\n source=source.replace(needle,globals_setup+needle)")
 source=source.replace('environment=dict(parent.__dict__,STAGED_HEADER=header)','environment=dict(parent.__dict__,STAGED_HEADER=header,CORE=CORE)')
 def patched_source(function):
  text=getsource(function)
  text=text.replace("'--test-jobs','1'];commands", "'--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log'];commands")
  needle="index=all_cases.index(case);fp=[0x27f|((index%4)<<10),((index%8)<<11)|((index%4)<<8)|(index%32),0x1f80|((index%4)<<13)];case['before_fp']=fp"
  text=text.replace(needle,"index=all_cases.index(case);mode=0 if case.get('whole_function_extended')else index%4;fp=[0x27f|(mode<<10),((index%8)<<11)|((index%4)<<8)|(index%32),0x1f80|(mode<<13)];case['before_fp']=fp")
  return text
 def staged_write(path,text,*args,**kwargs):
  if path.is_relative_to(scratch)and path.name=='observer.cpp':
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
  if path.is_relative_to(scratch)and path.name=='driver.cpp':
   text=text.replace('  std::size_t cases{},stores{},read_count{},faults{},mutations{};','  std::size_t cases{},stores{},read_count{},faults{},mutations{};std::array<std::uint64_t,11>kind_profiles{},kind_calls{};')
   anchor='      if(read_pass){J expected=';assert text.count(anchor)==1
   check='''      if(c.value("whole_function_extended",false)){
        const auto kind=c.at("declared_function_kind").get<unsigned>();
        check(!failed&&!required&&result.part.function.calls>0&&result.part.function.kinds[kind]==result.part.function.calls,name+" extended point function was not executed");
        if(!read_pass){++kind_profiles[kind];kind_calls[kind]+=result.part.function.calls;}
      }
'''
   text=text.replace(anchor,check+anchor).replace('{"passed",true},{"cases",cases}','{"passed",true},{"kind_profiles",kind_profiles},{"kind_calls",kind_calls},{"cases",cases}')
  return write(path,text,*args,**kwargs)
 inspect.getsource=patched_source;Path.write_text=staged_write
 environment=dict(bounded.__dict__,CORE=CORE,EXTRA_OWNERS=EXTRA_OWNERS,extended_cases=extended_cases,
  EXTRA_BANK_SETUP=setup[setup.index('  for address,size'):],__doc__=__doc__)
 try:exec(source,environment);return environment['main']()
 finally:
  inspect.getsource=getsource;Path.write_text=write
  p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if p.exists():
   r=json.loads(p.read_text());r.update(whole_gameplay_extended_point_callback=True,scope=__doc__,source_private_raw_before_ledger_claimed=False)
   activity={}
   for compiler in('g++','clang++'):
    for log in(p.parent/compiler).rglob('LastTest.log'):
     for line in log.read_text().splitlines():
      if line.startswith('{'):
       row=json.loads(line)
       if 'kind_profiles'in row:activity[compiler]=dict(profiles=row['kind_profiles'],calls=row['kind_calls'])
   if r.get('passed'):
    assert set(activity)=={'g++','clang++'},'Missing positive native function activity output'
    assert all(v['profiles']==[4]*11 and all(v['calls'])for v in activity.values())
    r['positive_declared_function_activity']=activity
   p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':raise SystemExit(main())
