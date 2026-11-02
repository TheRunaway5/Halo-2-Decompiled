#!/usr/bin/env python3
"""Physical13E9A0 current DATA pointer, byte selectors/read order/RET8 proof."""
import inspect
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT

def cases(quick=False):
 u,d=base.UI,base.DATA;rows=[]
 def add(name,kind,layer,index,**extra):rows.append(dict(name=name,entry=0x13e9a0,receiver=u+0x100,
    arguments=[layer,index],user_words=[(0x104,d),(0x1000,kind)],**extra))
 for kind in range(256):add(f'kind_{kind}',kind,17,31)
 for kind in(4,5,6,7,8,10):
  for layer,index in((0,0),(1,1),(255,255),(256,257),(0xffffffff,0x80000080),(0x12345678,0xabcdef01)):
   add(f'selectors_{kind}_{layer:x}_{index:x}',kind,layer,index)
 for name,address,size,nth in(('reference',u+0x104,4,1),('first_kind',d,1,1),('current_kind',d,1,2),('stride',0x44b888,1,1)):
  add('fault_'+name,4,255,255,fault=dict(address=address,bytes=size,occurrence=nth,write=False))
 return rows

def main():
 base.OWNERS=(0x13e9a0,);base.cases=cases
 base.reference_inputs=lambda image:({0x44b884:image.read(0x44b884,256)},dict(kind='Authenticated TU1.5 readonly stride-table bytes; declared function reference/blob/selectors',expected_after_supplied=False))
 base.FILES=(*base.FILES,'include/halo2/native_tu15_function_control_point.hpp','src/core/native_tu15_function_control_point.cpp','tools/validate_native_function_control_point_tu15.py')
 original=inspect.getsource(base.original).replace(" mov ${case['receiver']},%ecx", " mov ${case['receiver']},%eax\n mov ${case['receiver']},%ecx")
 namespace=dict(base.__dict__);exec(original,namespace);base.original=namespace['original']
 source=inspect.getsource(base.main)
 source=source.replace("tests/native_tu15_array_growth_tests.cpp'", "src/core/native_tu15_function_control_point.cpp tests/native_tu15_array_growth_tests.cpp'")
 needle="        (project/'CMakeLists.txt').write_text("
 extra='''        driver=project/'tests/native_tu15_array_growth_tests.cpp';text=driver.read_text();text='#include "halo2/native_tu15_function_control_point.hpp"\\n'+text
        text=text.replace('if(entry==0x2e3250u)','if(entry==0x13e9a0u)result=native_tu15_authored_function_control_point(memory,receiver,args[0],args[1]);else if(entry==0x2e3250u)')
        text=text.replace('      check(calls==','      if(!failed&&!required)check(result==c.at("return_word"),name+" control-point DATA identity differs");\\n      check(calls==')
        driver.write_text(text)
'''
 assert source.count(needle)==1;source=source.replace(needle,extra+needle)
 begin=source.index('        changes=((');end=source.index('\n        for name,path,before,after in changes:',begin)
 source=source[:begin]+'''        changes=(('kind_lower','src/core/native_tu15_function_control_point.cpp','kind<4u','kind<3u'),('selector_width','src/core/native_tu15_function_control_point.cpp','layer&0xffu','layer'),('current_kind_read','src/core/native_tu15_function_control_point.cpp','current_kind=read(blob,1)','current_kind=kind'))
'''+source[end:]
 source=source.replace("'actual allocator request order differs'", "'actual allocator request order differs','control-point DATA identity differs','array read order differs','source array completion differs'")
 source=source.replace("remaining_explicit_owners=['default allocator149930'", "remaining_explicit_owners=['Function13DE70 remaining authored kinds and CRT/native math','default allocator149930'")
 environment=dict(base.__dict__);exec(source,environment);return environment['main']()
if __name__=='__main__':main()
