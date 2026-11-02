#!/usr/bin/env python3
"""Physical native temporary and owned-island lifetime with native allocators."""
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Island lifetime seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 base=Path(__file__).with_name('validate_native_body_constraint_actual_before_tu15.py');text=base.read_text()
 first=text.index(' units={');end=text.index('\n production_parent=',first)
 text=text[:first]+''' units={name:ROOT/path for name,path in {
  'parent':'src/physics/native_tu15_island_lifetime.cpp',
  'allocator':'src/physics/native_tu15_array_allocator.cpp',
  'growth':'src/physics/native_tu15_array_growth.cpp',
  'arena':'src/physics/native_tu15_stack_arena.cpp'}.items()}'''+text[end:]
 first=text.index(" names=[");end=text.index('\n files=',first)
 text=text[:first]+" names=['island_lifetime','island_merge','island_activation','array_allocator','array_growth','stack_arena','physics_frame']"+text[end:]
 text=text.replace("ROOT/'tools/observe_native_body_constraint_actual_before_tu15.cpp'","ROOT/'tools/observe_native_island_lifetime_actual_before_tu15.cpp'")
 text=once(text,"files=[production_parent,*units.values()","files=[ROOT/'tools/validate_native_body_constraint_actual_before_tu15.py',production_parent,*units.values()")
 first=text.index('  owners=[');end=text.index('\n  for owner in owners:',first)
 text=text[:first]+'  owners=[0x322100,0x3213a0,0x317a40,0x317ad0]'+text[end:]
 needle="metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands,repeat_count=i.mnemonic in('rep stosd','rep stosb'))"
 text=once(text,needle,needle+"\n    if i.mnemonic=='call'and i.operands[0].type!=2:metadata[str(pc)].update(service_arguments=1 if pc==0x317a8f else 2 if pc==0x317b1c else 3,ecx_consumed=True,result_consumed=False)")
 headers='#include "halo2/native_tu15_island_lifetime.hpp"\n#include "halo2/native_tu15_array_allocator.hpp"\n#include "halo2/native_tu15_stack_arena.hpp"\n'
 text=once(text,'driver.write_text(','driver.write_text('+repr(headers)+'+')
 text=once(text,"parent=units['parent'].read_text();needle='word(component+4u)&0x800u';assert parent.count(needle)==1","parent=units['parent'].read_text();needle='store(m,o,0x3213c0u,p,0x41f080u)';assert parent.count(needle)==1")
 text=once(text,"parent.replace(needle,'word(component+4u)&0x400u')","parent.replace(needle,'store(m,o,0x3213c0u,p,0x41f084u)')")
 text=text.replace('wrong_flag','wrong_initial_table').replace('Original22D460 rejects wrong native positive-loop gate','Original3213A0 rejects wrong native initial island table')
 text=once(text,"remaining=['Actual22D460 invocation boundary and live positive native parent activation','Saturated capacity/removal and other model rows','Fault/alias/mutable-observer domains']","remaining=['Coupled native3190D0 and318DE0 parent composition','Additional arena slow/owned-temp/fault/unwind profiles','Production World selection and visible multiplayer']")
 namespace={'__file__':__file__,'__name__':'native_island_lifetime_actual_before_base'}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
