#!/usr/bin/env python3
"""Physical355D0/184C50 composed with genuine native13DE70 kind0/1 kernels."""
import inspect,struct
import validate_native_surface_conveyor_tu15 as parent
from oracle_smg_firing_1_1 import ROOT
ORIGINAL_CASES=parent.cases

def cases():
 rows=ORIGINAL_CASES();u=parent.base.UI
 for entry in(0x355d0,0x184c50):
  for kind,flags in((0,0),(0,0xf0),(1,1),(1,0),(11,0)):
   for channels in(1,2):
    words=[(0x518,u+0x700),(0x724,u+0x800),(0x860,u+0xa00),(0x850,u+0xb00),(0x848,u+0xc00),(0x840,u+0xd00),
     (0xa00,0),(0xa04,0),(0xb00,channels<<9),(0xc00,4<<16),(0xc04,(5<<16)|1),
     (0x1510,u+0x1600),(0x15b4,1),(0x15b8,u+0x1800),(0x1608,0),(0x1610,0xabcd0001)]
    for n in range(2):words.extend([(0xd00+n*20+8,0x3f800000),(0xd00+n*20+12,20),(0xd00+n*20+16,u+0xe00+n*32),
     (0xe00+n*32,kind|(flags<<8)),(0xe04+n*32,0),(0xe08+n*32,0x3f800000)])
    for n,bits in enumerate((0x3f800000,0,0,0,0x3f800000,0)):words.append((0x1800+n*4,bits))
    args=[0,u+0x2000,u+0x2004]if entry==0x355d0 else[]
    rows.append(dict(name=f'composed_{entry:x}_{kind}_{flags}_{channels}',entry=entry,receiver=0,arguments=args,
     incoming_eax=0xabcd0001 if entry==0x355d0 else 0,incoming_ebx=u+0x2000,user_words=words))
 return rows

def main():
 parent.cases=cases;source=inspect.getsource(parent.main)
 source=source.replace("base.OWNERS=(0x355d0,0x184c50)","base.OWNERS=(0x355d0,0x184c50,0x13de70)")
 source=source.replace("files=('include/halo2/native_tu15_surface_conveyor.hpp'", "files=('include/halo2/native_tu15_function_evaluation.hpp','src/core/native_tu15_function_evaluation.cpp','tools/validate_native_surface_conveyor_composed_tu15.py','include/halo2/native_tu15_surface_conveyor.hpp'")
 needle="shutil.copyfile(ROOT/'src/physics/native_tu15_surface_conveyor.cpp',project/'kernel.cpp')";assert source.count(needle)==1
 source=source.replace(needle,needle+";shutil.copyfile(ROOT/'src/core/native_tu15_function_evaluation.cpp',project/'function.cpp')")
 source=source.replace('add_executable(conveyor driver.cpp kernel.cpp)', 'add_executable(conveyor driver.cpp kernel.cpp function.cpp)')
 source=source.replace('driver=\'#include "halo2/native_tu15_surface_conveyor.hpp"', 'driver=\'#include "halo2/native_tu15_function_evaluation.hpp"\\n#include "halo2/native_tu15_surface_conveyor.hpp"')
 source=source.replace('auto first,auto second,auto&,auto)->std::uint32_t', 'auto first,auto second,auto&state,auto depth)->std::uint32_t')
 source=source.replace('throw Required{};};','if(c.value("required_owner",false))throw Required{};return native_tu15_evaluate_authored_function(memory,function,first,second,state,depth);};')
 source=source.replace('positive_function_math_qualified=False','positive_function_math_qualified=True')
 source=source.replace("scope='Closed source no-function returns and actual13DE70 service-entry prefixes only.","scope='Native13DE70 boundedkind0/1/null/invalid evaluator composed through355D0 and184C50; genuine positive derivatives/material vectors plus closed no-function returns and actual13DE70 service-entry prefixes.")
 source=source.replace('Positive evaluator/range/derivative/vector output remains unqualified.','Other9valid authored-function kinds remain counted/refused by the sole native evaluator; sourceprivatebefore ledger unclaimed.')
 environment=dict(parent.__dict__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
