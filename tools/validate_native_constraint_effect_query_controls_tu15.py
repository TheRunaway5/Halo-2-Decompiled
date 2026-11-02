#!/usr/bin/env python3
"""Physical18AF20/18B0C0 enum fallback and genuine material-parent controls."""
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Effect query qualification seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 base=Path(__file__).with_name('validate_native_model_constraint_removal_tu15.py');text=base.read_text()
 first=text.index(' units={');last=text.index('\n observer=',first)
 text=text[:first]+" units={'selector':ROOT/'src/physics/native_tu15_constraint_effect_selector.cpp','location':ROOT/'src/physics/native_tu15_constraint_effect_location.cpp'}"+text[last:]
 text=once(text,"default=r'(gemini-column[01]_[01]|elongation-crates_[12]|actual-warthog_[1-4])_[0-3]'","default=r'(gemini-column[01]_0|elongation-crates_0)_[0-3]'")
 text=once(text,"observer=ROOT/'tools/observe_native_model_constraint_removal_tu15.cpp'","observer=ROOT/'tools/observe_native_constraint_effect_query_controls_tu15.cpp'")
 text=once(text,' pins={'," files.extend([ROOT/'tools/validate_native_model_constraint_removal_tu15.py',ROOT/'include/halo2/native_tu15_constraint_effect_selector.hpp',ROOT/'include/halo2/native_tu15_constraint_effect_location.hpp',ROOT/'include/halo2/native_tu15_constraint_runtime_update.hpp',ROOT/'analysis/physics/native-constraint-effect-query-controls-20261101/report.json'])\n pins={")
 text=once(text,'(0x22d0a0,0x22e1b0,0x22e390,0xde8c0,0x22fe30,0x16e8a0)','(0x18af20,0x18b0c0,0x18ae80,0x18add0)')
 text=once(text,"  for pc in (0x22d13f,0x22d155,0x22d233,0x22e1fc):metadata[str(pc)]['stop_before']=True",'  # These authenticated queries have no external callback.')
 text=once(text,'#include "halo2/native_tu15_model_constraint_removal.hpp"\\n','#include "halo2/native_tu15_constraint_effect_selector.hpp"\\n')
 text=once(text,"text=units['removal'].read_text();needle='read(m,list_row+2u,2)-1u'",'''text=units['selector'].read_text()+"""
namespace halo2 {
std::uint32_t test_native_effect_definition_query(const SourcePhysicsContactMemory&m,std::uint32_t definition,std::uint32_t type,std::uint16_t material,bool secondary,NativeTu15ConstraintEffectSelectorStats*stats){return effect_for_definition(m,definition,type,material,secondary,stats);}
std::uint32_t test_native_effect_material_query(const SourcePhysicsContactMemory&m,std::uint16_t material,std::uint32_t type,std::uint16_t other,bool secondary,NativeTu15ConstraintEffectSelectorStats*stats){return effect_for_material(m,material,type,other,secondary,stats);}
}
"""
  (project/'selector.cpp').write_text(text)
  needle='read(m,candidate+0x14u,2)' ''')
 text=once(text,"text.replace(needle,'read(m,list_row+2u,2)-2u')","text.replace(needle,'read(m,candidate+0x12u,2)')")
 text=once(text,"first='removal.cpp'if variant=='native'else'wrong_list_count.cpp'","first='selector.cpp'if variant=='native'else'wrong_list_count.cpp'")
 text=once(text,'{first} release.cpp slots.cpp cache.cpp delete.cpp','{first} location.cpp')
 text=once(text,'{case["selection"]}', '"{ROOT}/analysis/physics/native-constraint-effect-query-controls-20261101/report.json"')
 text=once(text,"remaining=['Actual17A0F0 and22FEA0 lifetime callbacks if reached','Positive authored constraint creation','Fault/alias/mutable-observer domains','Production integration and visible multiplayer']","remaining=['Actual camera-qualified full selector positive input','Additional fallback/material-parent paths outside retained authored query tuples','Production integration and visible multiplayer','Fault/alias/mutable-observer domains']")
 text=text.replace('wrong_list_count','wrong_material_key').replace('Original22D0A0 rejects incorrect native list removal count','Original18ADD0 rejects incorrect native material-key field')
 namespace={'__file__':__file__,'__name__':'native_constraint_effect_queries_base'}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
