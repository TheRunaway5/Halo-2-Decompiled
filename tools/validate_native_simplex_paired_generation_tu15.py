#!/usr/bin/env python3
"""Preserve both30D830 owners and all earlier477/82/Scene generation groups."""
import json
import os
from pathlib import Path
import sys
import native_simplex_paired_replacements as binding
import validate_native_closest_segments_generation_tu15 as parents
from generated_routine_ownership import function_bodies
from specialize_native_operands import specialize
def main():
    parents.parents.triangle.features.baseline.FILES+=(
        'tools/native_simplex_paired_replacements.py',
        'tools/validate_native_simplex_paired_generation_tu15.py',
        'include/halo2/native_tu15_simplex_paired_kernel.hpp',
        'src/physics/runtime/native_tu15_simplex_paired_frame.hpp',
        'src/physics/runtime/native_tu15_simplex_paired_source_owner.hpp',
    )
    result=parents.main();path=Path(sys.argv[sys.argv.index('--report')+1]);report=json.loads(path.read_text())
    try:
        scratch=Path(os.environ['HALO2_TEST_SCRATCH']);units=[]
        for name in('world','contact'):
            source=(scratch/(name+'.cpp')).read_text()
            if binding.replace_simplex_paired(source)!=source or binding.installed_entries(source)!=['30d830']:raise ValueError('Complete generated paired owner absent: '+name)
            original=binding.remove_simplex_paired(source);old={s.symbol:b for s,b in function_bodies(original)};new={s.symbol:b for s,b in function_bodies(source)}
            if set(old)!=set(new)or any(b!=new[n]for n,b in old.items()if n!='309d40'):raise ValueError('Paired overlay changed another complete routine: '+name)
            prepared,receipt=specialize(source)
            if receipt['native_simplex_paired_entries']!=['30d830']or binding.installed_entries(prepared)!=['30d830']or specialize(prepared)[0]!=prepared:raise ValueError('Complete paired preparation/idempotency differs: '+name)
            units.append(dict(owner=name,raw_body_sha256=binding.digest(binding.BODY.search(source)[0]),prepared_body_sha256=binding.digest(binding.BODY.search(prepared)[0]),native_simplex_paired_entries=receipt['native_simplex_paired_entries'],all_other_complete_routines_preserved=True))
        if binding.BODY.search((scratch/'scene.cpp').read_text()):raise ValueError('Previously absent paired Scene owner unexpectedly appeared')
        report['native_simplex_paired']=units;report['all_existing_owner_groups_preserved']=True
    except BaseException as error:report.update(passed=False,paired_error=repr(error));raise
    finally:path.write_text(json.dumps(report,indent=2)+'\n')
    return result
if __name__=='__main__':raise SystemExit(main())
