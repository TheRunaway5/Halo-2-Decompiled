#!/usr/bin/env python3
"""Keep all three312620 owners through whole477/82/Scene regeneration."""
import json
import os
from pathlib import Path
import sys
import native_closest_segments_replacements as binding
import validate_native_simplex_face_select_generation_tu15 as parents
from generated_routine_ownership import function_bodies
from specialize_native_operands import specialize
def main():
    parents.triangle.features.baseline.FILES+=(
        'tools/native_closest_segments_replacements.py',
        'tools/validate_native_closest_segments_generation_tu15.py',
        'include/halo2/native_tu15_closest_segments_kernel.hpp',
        'src/physics/runtime/native_tu15_closest_segments_frame.hpp',
        'src/physics/runtime/native_tu15_closest_segments_source_owner.hpp',
        'src/physics/runtime/native_tu15_simplex_helpers_handoff_internal.hpp',
    )
    result=parents.main();path=Path(sys.argv[sys.argv.index('--report')+1]);report=json.loads(path.read_text())
    try:
        scratch=Path(os.environ['HALO2_TEST_SCRATCH']);units=[]
        for name in('world','contact','scene'):
            source=(scratch/(name+'.cpp')).read_text()
            if binding.replace_closest_segments(source)!=source or binding.installed_entries(source)!=['312620']:raise ValueError('Complete generated closest-segments owner absent: '+name)
            original=binding.remove_closest_segments(source);old={s.symbol:b for s,b in function_bodies(original)};new={s.symbol:b for s,b in function_bodies(source)};target='312620'if name=='scene'else'30eb30'
            if set(old)!=set(new)or any(b!=new[n]for n,b in old.items()if n!=target):raise ValueError('Closest overlay changed another complete routine: '+name)
            prepared,receipt=specialize(source);group='native_closest_segments_scene_entries'if name=='scene'else'native_closest_segments_entries'
            if receipt[group]!=['312620']or binding.installed_entries(prepared)!=['312620']or specialize(prepared)[0]!=prepared:raise ValueError('Complete closest preparation/idempotency differs: '+name)
            units.append(dict(owner=name,raw_body_sha256=binding.digest(binding.BODY.search(source)[0]),prepared_body_sha256=binding.digest(binding.BODY.search(prepared)[0]),receipt_group=group,entries=receipt[group],all_other_complete_routines_preserved=True))
        root=parents.triangle.features.baseline.ROOT;handoff=(root/'src/physics/runtime/native_tu15_simplex_helpers_handoff_internal.hpp').read_text()
        hook='#if defined(HALO2_NATIVE_CLOSEST_SEGMENTS_SOURCE_OWNER)\n    if(native_closest_segments_owner_detail::defer_world_owner(s,target,tu15_instruction))return false;\n#endif\n'
        if handoff.count(hook)!=1 or handoff.index(hook)>handoff.index('    auto actual=s.save();'):raise ValueError('Closest World handoff no longer defers before transport/I/O')
        report['native_closest_segments']=units;report['world_deferral_before_scalar_transport']=True;report['all_existing_owner_groups_preserved']=True
    except BaseException as error:report.update(passed=False,closest_error=repr(error));raise
    finally:path.write_text(json.dumps(report,indent=2)+'\n')
    return result
if __name__=='__main__':raise SystemExit(main())
