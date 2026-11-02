#!/usr/bin/env python3
"""Physical318DE0 with native temporary/reserve/allocator closure on real records."""
from pathlib import Path
from validate_native_island_lifetime_actual_before_tu15 import once
def main():
 base=Path(__file__).with_name('validate_native_body_constraint_actual_before_tu15.py');text=base.read_text()
 first=text.index(' units={');end=text.index('\n production_parent=',first)
 text=text[:first]+''' units={name:ROOT/path for name,path in {
  'parent':'src/physics/native_tu15_island_contact_merge.cpp',
  'lifetime':'src/physics/native_tu15_island_lifetime.cpp',
  'allocator':'src/physics/native_tu15_array_allocator.cpp',
  'growth':'src/physics/native_tu15_array_growth.cpp',
  'reserve':'src/physics/native_tu15_array_reserve.cpp',
  'arena':'src/physics/native_tu15_stack_arena.cpp'}.items()}'''+text[end:]
 first=text.index(' names=[');end=text.index('\n files=',first)
 text=text[:first]+" names=['island_contact_merge','island_lifetime','island_merge','island_activation','array_allocator','array_growth','array_reserve','stack_arena','physics_frame']"+text[end:]
 text=text.replace("ROOT/'tools/observe_native_body_constraint_actual_before_tu15.cpp'","ROOT/'tools/observe_native_island_contact_merge_actual_before_tu15.cpp'")
 text=once(text,"files=[production_parent,*units.values()","files=[ROOT/'tools/validate_native_body_constraint_actual_before_tu15.py',ROOT/'tools/validate_native_island_lifetime_actual_before_tu15.py',production_parent,*units.values()")
 first=text.index('  owners=[');end=text.index('\n  for owner in owners:',first)
 text=text[:first]+'  owners=[0x318de0,0x317a40,0x317ad0]'+text[end:]
 needle="metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands,repeat_count=i.mnemonic in('rep stosd','rep stosb'))"
 text=once(text,needle,needle+"\n    if i.mnemonic=='call'and(i.operands[0].type!=2 or i.operands[0].imm==0x2e31e0):metadata[str(pc)].update(service_arguments=1 if pc==0x317a8f else 2 if pc==0x317b1c else 0 if pc in[0x318f76,0x318f7d]else 3,ecx_consumed=i.operands[0].type!=2,result_consumed=False)")
 headers='#include "halo2/native_tu15_island_contact_merge.hpp"\n#include "halo2/native_tu15_array_allocator.hpp"\n#include "halo2/native_tu15_stack_arena.hpp"\n'
 text=once(text,'driver.write_text(','driver.write_text('+repr(headers)+'+')
 text=once(text,"parent=units['parent'].read_text();needle='word(component+4u)&0x800u';assert parent.count(needle)==1","parent=units['parent'].read_text();needle='store(sites[n],destination+n*4u,read(source+n*4u))';assert parent.count(needle)==1")
 text=once(text,"parent.replace(needle,'word(component+4u)&0x400u')","parent.replace(needle,'store(sites[n],destination+n*4u,read(source+n*4u)^(sites[n]==0x318f20u?4u:0u))')")
 text=text.replace('wrong_flag','wrong_first_record').replace('Original22D460 rejects wrong native positive-loop gate','Original318DE0 rejects wrong native record publication')
 text=once(text,"remaining=['Actual22D460 invocation boundary and live positive native parent activation','Saturated capacity/removal and other model rows','Fault/alias/mutable-observer domains']","remaining=['Positive duplicate-agent/manager cleanup branch','Coupled native3190D0 World binding and live activation','Arena slow/fault/unwind/alias domains beyond actual input']")
 namespace={'__file__':__file__,'__name__':'native_island_contact_merge_actual_before_base'}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
