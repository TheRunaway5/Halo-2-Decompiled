#!/usr/bin/env python3
"""Complete1E0A60 native positive conveyor/function composition proof."""
import inspect,struct
import validate_native_part_response_tu15 as parent
ORIGINAL_CASES=parent.cases

def cases(quick=False):
 rows=ORIGINAL_CASES();u=parent.parent.parent.prop.base.UI
 positives=[r for r in rows if r['name'].startswith('positive_part_')]
 for row in positives:
  for changed_material in(False,True):
   for fallback_map in(False,True):
    extra=[(0x6018,u+0x6200),(0x6224,u+0x6400),(0x6440,u+0x6800),(0x6448,u+0x6600),(0x6450,u+0x6500),(0x6460,u+0x6480),
      (0x6480,0xffff0000 if fallback_map else 0),(0x6484,0),(0x6500,2<<9),(0x6600,4<<16),(0x6604,(5<<16)|1),
      (0x50b4,4),(0x50b8,u+0x5400)]
    for n in range(4):extra.extend([(0x5200+n*20+8,n<<16),(0x5200+n*20+16,0xabcd0001)])
    for n in range(2):extra.extend([(0x6800+n*20+8,0x3f800000),(0x6800+n*20+12,20),(0x6800+n*20+16,u+0x6900+n*32),
      (0x6900+n*32,0),(0x6904+n*32,0),(0x6908+n*32,0x3f800000)])
    basis=((1.,0.,0.,0.,1.,0.),(0.,1.,0.,0.,0.,1.),(0.,0.,1.,1.,0.,0.),(.3,.4,.5,-.25,.75,.125))
    for n,vector in enumerate(basis):
     for axis,value in enumerate(vector):extra.append((0x5400+n*24+axis*4,struct.unpack('<I',struct.pack('<f',value))[0]))
    if changed_material:extra.append((0x300c,3))
    rows.append(dict(row,name='conveyor_'+row['name']+'_'+str(changed_material)+'_'+str(fallback_map),user_words=[*row['user_words'],*extra]))
 for original in [r for r in rows if r['name']in('positive_part_1_0','positive_part_1_1','positive_part_2_0','positive_part_2_1')]:
  for count in(0,4):
   extra=[(0x734,count),(0x738,0x80000000|count)]
   for n in range(4):extra.append((0x1000+n*8,0x55550000+n))
   rows.append(dict(original,name='missing_property_'+original['name']+'_'+str(count),user_words=[*original['user_words'],*extra]))
 return rows

def main():
 from pathlib import Path
 import os
 base=parent.parent.parent.prop.base;original_inputs=base.reference_inputs
 def inputs(image):
  banks,provenance=original_inputs(image);banks=dict(banks);banks[0x4f0414]=struct.pack('<I',base.UI+0x6000)
  return banks,dict(provenance,native_function_domain='Actual13DE70 kind0 native evaluator, ordinary declared shader/material DATA; no supplied shader result')
 base.reference_inputs=inputs
 base.FILES=(*base.FILES,'include/halo2/native_tu15_function_evaluation.hpp','src/core/native_tu15_function_evaluation.cpp','tools/validate_native_part_response_conveyor_tu15.py')
 metadata=base.metadata
 def with_function_metadata(image,banks):
  if 0x13de70 not in base.OWNERS:base.OWNERS=(*base.OWNERS,0x13de70)
  return metadata(image,banks)
 base.metadata=with_function_metadata
 write=Path.write_text;scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 def staged_write(path,text,*args,**kwargs):
  if path.is_relative_to(scratch):
   if path.name=='native_tu15_array_growth_tests.cpp':
    text='#include "halo2/native_tu15_function_evaluation.hpp"\n'+text
    marker='      const auto host_sw='
    text=text.replace(marker,'      NativeTu15ShaderFunctionEvaluationServices evaluator;evaluator.evaluate=[&](auto reference,auto first,auto second,auto&state,auto depth){return native_tu15_evaluate_authored_function(memory,reference,first,second,state,depth);};\n'+marker)
    text=text.replace('growth,{},observe)', 'growth,evaluator,observe)')
   if path.name=='CMakeLists.txt'and'part_contact_response.cpp'in text:
    text=text.replace('tests/native_tu15_array_growth_tests.cpp','src/core/native_tu15_function_evaluation.cpp tests/native_tu15_array_growth_tests.cpp')
  return write(path,text,*args,**kwargs)
 Path.write_text=staged_write;parent.cases=cases
 try:
  result=parent.main()
  import sys,json,subprocess
  report_path=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';report=json.loads(report_path.read_text())
  command=report['commands'][0];project=Path(command[command.index('--source')+1]);kernel=project/'src/physics/native_tu15_part_contact_response.cpp';original=kernel.read_text();negative=[]
  for name,before,after in [('material_cap','material_count>=3u','material_count>=2u'),('hit_status','m.write(0x1e0fc3u,slot+0x44u,2u,1)','m.write(0x1e0fc3u,slot+0x44u,1u,1)'),('property_vector','?0u:slot+0x30u','?0u:0u')]:
   assert original.count(before)==1;kernel.write_text(original.replace(before,after));mutant=list(command);mutant[mutant.index('--output')+1]=str(report_path.parent/('negative-part-'+name));r=subprocess.run(mutant);assert r.returncode!=0,'Original did not reject part '+name;negative.append(name);kernel.write_text(original)
  report['part_parent_mutants_rejected']=negative;report_path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
  return result
 finally:
  Path.write_text=write
  import sys,json
  p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if p.exists():
   d=json.loads(p.read_text());d.update(scope='Declared valid complete1E0A60 normal/material/hit/property/conveyor response, native184C50/355D0/13DE70kind0, nativebodyproperty/array/pool. AllpersistentDATA+orderedoperands/preimages/stores+FP; positivevectors and second-mapfallback. Other9valid evaluator kinds and rawprivatecontrolledger remain explicit.',positive_shader_functions_qualified=True);p.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
