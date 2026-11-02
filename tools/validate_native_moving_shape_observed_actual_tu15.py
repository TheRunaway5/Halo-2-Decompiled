#!/usr/bin/env python3
"""Same frozen moving helper: strict empty and typed retained-observer modes."""
from pathlib import Path
import hashlib,inspect,json
import validate_native_moving_shape_process_bound_actual_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/physics/native-moving-shape-process-qualified-actual1479-20261102/source'
INTERFACE=ROOT/'analysis/physics/native-simplex-retained-observation-interface-qualified-v3-20261102/source'
WRITERS=ROOT/'analysis/physics/native-moving-shape-writer-observation-stage-20261102/staged'
SAVED_OBSERVER=parent.observer_source

def observed_source():
 text=SAVED_OBSERVER();text=text.replace('#include "halo2/native_tu15_contact_creation_gameplay.hpp"','#include <xmmintrin.h>\n#include "halo2/native_tu15_contact_creation_gameplay.hpp"',1);text=text.replace('dispatch=J::array(),zero_repeats=J::array();','dispatch=J::array(),zero_repeats=J::array(),writer_events=J::array();')
 old='pending.push_back(J{{"address",p},{"bytes",n},{"before",process_hex(original.read(p,n))}})';assert text.count(old)==1;text=text.replace(old,'pending.push_back(J{{"instruction",pc},{"address",p},{"bytes",n},{"before",process_hex(original.read(p,n))}})')
 old='e.erase("bytes");stores.push_back(e);';assert text.count(old)==1;text=text.replace(old,'e.erase("bytes");writer_events.push_back(e);e.erase("instruction");stores.push_back(e);')
 old='   NativeTu15MovingShapeBindingActivity bound;';assert text.count(old)==1
 observer=r'''   J observed_events=J::array();NativeTu15RetainedStoreObserver typed_observer;
   if(!read_pass)typed_observer=NativeTu15RetainedStoreObserver{[&](const auto&event){
    observed_events.push_back(J{{"instruction",event.instruction},{"address",event.address},{"before",process_hex(event.before)},{"written",process_hex(event.written)}});
    // Instrumentation executes under its captured HOST FP environment. Its
    // math and rounding changes must never enter the native Source FP state.
    __m128 a=_mm_set_ss(1.0f),b=_mm_set_ss(3.0f);asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");
    std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));cw^=0xc00u;asm volatile("fldcw %0"::"m"(cw):"memory");_mm_setcsr(_mm_getcsr()^0x6000u);
    const double three=3.0;asm volatile("fld1;fdivl %0;fstp %%st(0)"::"m"(three):"memory","st");
   }};
'''
 text=text.replace(old,observer+old)
 old='{p,0x8000u},fp,0,{},native_scope,&bound);';assert text.count(old)==1;text=text.replace(old,'{p,0x8000u},fp,0,{},native_scope,&bound,typed_observer);')
 old='   tracked.enabled=false;';assert text.count(old)==1;text=text.replace(old,old+'if(!read_pass)check(observed_events==writer_events,"Retained SourcePC/before/written/order differs "+observed_events.dump()+" / "+writer_events.dump());')
 text=text.replace('{"native_contact_creations",bound.publication.creation.completed}', '{"retained_observer_events",observed_events.size()},{"observer_performs_host_fp_math",!read_pass},{"native_contact_creations",bound.publication.creation.completed}')
 text=text.replace('mode<8;++mode','mode<9;++mode')
 old='native_tu15_process_bound_moving_shape_contact(memory,{1},{2},{3},{4},{5},s,f,depth,{},scope,&a);';assert text.count(old)==1;text=text.replace(old,'native_tu15_process_bound_moving_shape_contact(memory,{mode==8?s.identity-0x80u:1u},{2},{3},{4},{5},s,f,depth,{},scope,&a);')
 return text

def main():
 # A single immutable union supplies every native compiler input and header.
 overrides={str(p.relative_to(BASE)):p for p in BASE.rglob('*')if p.is_file()}
 for source in (INTERFACE,WRITERS):
  for p in source.rglob('*'):
   if p.is_file():overrides[str(p.relative_to(source))]=p
 source=inspect.getsource(parent.main)
 source=source.replace('helper_pin=hashlib.sha256(helper.read_bytes()).hexdigest()',"helper_pin=hashlib.sha256(UNION['include/halo2/native_tu15_moving_shape_process_binding.hpp'].read_bytes()).hexdigest()")
 old="  effective=baseline if name=='src/physics/native_tu15_convex_simplex_solver.cpp'else header if name=='include/halo2/native_tu15_convex_simplex_solver.hpp'else original";assert source.count(old)==1;source=source.replace(old,'  effective=UNION.get(name,original)')
 marker=' while seeds:';assert source.count(marker)==1;source=source.replace(marker,' seeds.extend(ROOT/name for name in UNION)\n'+marker)
 source=source.replace('if child.is_file():seeds.append(child.resolve())','if child.is_file()or str(child.resolve().relative_to(ROOT))in UNION:seeds.append(child.resolve())')
 source=source.replace('str(Path(p).relative_to(ROOT))','str(Path(p).resolve().relative_to(ROOT))')
 source=source.replace('hashlib.sha256(helper.read_bytes()).hexdigest()==helper_pin',"hashlib.sha256(UNION['include/halo2/native_tu15_moving_shape_process_binding.hpp'].read_bytes()).hexdigest()==helper_pin")
 source=source.replace("payload['production_guard_models']==list(range(8))","payload['production_guard_models']==list(range(9))")
 old_observer=parent.observer_source;parent.observer_source=observed_source
 try:
  environment=dict(parent.__dict__,__file__=__file__,UNION=overrides);exec(source,environment);result=environment['main']()
  import sys
  path=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());report['solver_arithmetic_baseline_sha256']=report.pop('compiled_solver_baseline_sha256');report['solver_arithmetic_baseline']=report.pop('compiled_solver_baseline');report['compiled_solver_cpp_sha256']=report['compiled_source_sha256']['src/physics/native_tu15_convex_simplex_solver.cpp'];report['typed_retained_observer_qualified']=bool(report.get('passed'));path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');return result
 finally:parent.observer_source=old_observer
if __name__=='__main__':raise SystemExit(main())
