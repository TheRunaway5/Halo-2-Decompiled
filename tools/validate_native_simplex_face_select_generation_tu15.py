#!/usr/bin/env python3
"""Preserve full World/Contact generation and all earlier native owner groups."""
import json
import os
from pathlib import Path
import sys
import native_simplex_face_select_replacements as binding
import validate_native_simplex_triangle_generation_tu15 as triangle
from generated_routine_ownership import function_bodies
from specialize_native_operands import specialize

def main():
    triangle.features.baseline.FILES+=(
        'tools/native_simplex_face_select_replacements.py',
        'tools/validate_native_simplex_face_select_generation_tu15.py',
        'src/physics/runtime/native_tu15_simplex_face_select_source_owner.hpp',
        'src/physics/runtime/native_tu15_simplex_face_select_abi.hpp',
        'include/halo2/native_tu15_simplex_face_select_kernel.hpp',
    )
    result=triangle.main();path=Path(sys.argv[sys.argv.index('--report')+1]);report=json.loads(path.read_text())
    try:
        scratch=Path(os.environ['HALO2_TEST_SCRATCH']);units=[]
        for name in('world','contact'):
            source=(scratch/(name+'.cpp')).read_text()
            if binding.replace_simplex_face_select(source)!=source or binding.installed_entries(source)!=['30d5d0']:
                raise ValueError('Complete generated face-select owner absent: '+name)
            original=binding.remove_simplex_face_select(source)
            old={s.symbol:b for s,b in function_bodies(original)};new={s.symbol:b for s,b in function_bodies(source)}
            if set(old)!=set(new)or any(b!=new[n]for n,b in old.items()if n!='309ae0'):
                raise ValueError('Face-select overlay changed another complete routine: '+name)
            prepared,receipt=specialize(source)
            if receipt['native_simplex_face_select_entries']!=['30d5d0'] or binding.installed_entries(prepared)!=['30d5d0']:
                raise ValueError('Complete prepared face-select owner absent: '+name)
            if specialize(prepared)[0]!=prepared:raise ValueError('Repeated face-select preparation changed source')
            units.append(dict(owner=name,raw_body_sha256=binding.digest(binding.BODY.search(source)[0]),
                prepared_body_sha256=binding.digest(binding.BODY.search(prepared)[0]),
                native_simplex_face_select_entries=receipt['native_simplex_face_select_entries'],
                all_other_complete_routines_preserved=True))
        report['native_simplex_face_select']=units;report['all_existing_owner_groups_preserved']=True
    except BaseException as error:report.update(passed=False,face_select_error=repr(error));raise
    finally:path.write_text(json.dumps(report,indent=2)+'\n')
    return result
if __name__=='__main__':raise SystemExit(main())
