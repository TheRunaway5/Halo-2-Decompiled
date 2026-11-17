#!/usr/bin/env python3
"""Authenticate both30DC10 copies through full Primary/477/82/Scene generation."""
import json
import os
from pathlib import Path
import subprocess
import sys
import native_simplex_boundary_replacements as binding
import validate_native_simplex_paired_generation_tu15 as parents
from generated_routine_ownership import function_bodies
from specialize_native_operands import specialize

def main():
    baseline=parents.parents.parents.triangle.features.baseline
    baseline.FILES+=(
        'tools/native_simplex_boundary_replacements.py',
        'tools/validate_native_simplex_boundary_generation_tu15.py',
        'include/halo2/native_tu15_simplex_boundary_kernel.hpp',
        'src/physics/runtime/native_tu15_simplex_boundary_frame.hpp',
        'src/physics/runtime/native_tu15_simplex_boundary_source_owner.hpp',
    )
    result=parents.main();path=Path(sys.argv[sys.argv.index('--report')+1]);report=json.loads(path.read_text())
    try:
        scratch=Path(os.environ['HALO2_TEST_SCRATCH']);root=baseline.ROOT
        before={name:binding.digest((root/name).read_text())for name in(
            'tools/generate_physics_live_support_tu15.py','tools/native_simplex_boundary_replacements.py',
            'tools/specialize_native_operands.py','src/physics/physics_live_support_tu15_generated.cpp')}
        command=[sys.executable,'-B',str(root/'tools/generate_physics_live_support_tu15.py'),
            '--output',str(scratch/'primary.cpp'),'--manifest',str(scratch/'primary.json')]
        generated=subprocess.run(command,capture_output=True,text=True)
        if generated.returncode:raise ValueError('Full Primary generation failed: '+generated.stderr[-5000:])
        units=[]
        for name in('primary','scene'):
            source=(scratch/(name+'.cpp')).read_text()
            if binding.replace_simplex_boundary(source)!=source or binding.installed_entries(source)!=['30dc10']:
                raise ValueError('Complete generated boundary owner absent: '+name)
            original=binding.remove_simplex_boundary(source)
            old={s.symbol:b for s,b in function_bodies(original)};new={s.symbol:b for s,b in function_bodies(source)}
            if set(old)!=set(new)or any(body!=new[entry]for entry,body in old.items()if entry!='30dc10'):
                raise ValueError('Boundary overlay changed another complete routine: '+name)
            prepared,receipt=specialize(source)
            if receipt['native_simplex_boundary_entries']!=['30dc10']or binding.installed_entries(prepared)!=['30dc10']or specialize(prepared)[0]!=prepared:
                raise ValueError('Complete boundary preparation/idempotency differs: '+name)
            units.append(dict(owner=name,raw_body_sha256=binding.digest(binding.BODY.search(source)[0]),
                prepared_body_sha256=binding.digest(binding.BODY.search(prepared)[0]),
                native_simplex_boundary_entries=receipt['native_simplex_boundary_entries'],all_other_complete_routines_preserved=True))
            if name=='primary':
                (scratch/'primary-prepared.cpp').write_text(prepared)
                for variant in('primary','primary-prepared'):
                    syntax=['clang++','-std=c++20','-Wno-ignored-attributes','-fsyntax-only',
                        '-I'+str(root/'include'),'-I'+str(root/'src'),str(scratch/(variant+'.cpp'))]
                    checked=subprocess.run(syntax,capture_output=True,text=True)
                    if checked.returncode:raise ValueError('Full Primary syntax failed: '+checked.stderr[-7000:])
        if any(binding.BODY.search((scratch/(name+'.cpp')).read_text())for name in('world','contact')):
            raise ValueError('Boundary missing external ownership was silently changed to a local body')
        if any(binding.digest((root/name).read_text())!=pin for name,pin in before.items()):
            raise ValueError('Boundary full Primary generation inputs drifted')
        report.update(native_simplex_boundary=units,genuine_full_primary_command=command,
            primary_raw_prepared_full_unit_syntax=True,all_existing_owner_groups_preserved=True)
    except BaseException as error:report.update(passed=False,boundary_error=repr(error));raise
    finally:path.write_text(json.dumps(report,indent=2)+'\n')
    return result
if __name__=='__main__':raise SystemExit(main())
