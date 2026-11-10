#!/usr/bin/env python3
"""Keep complete triangle/support/features owners through both generators."""
import json
import os
from pathlib import Path
import sys
import subprocess
import native_simplex_triangle_scene_replacements as scene
import native_simplex_triangle_replacements as triangle
import validate_native_polygon_features_generation_tu15 as features
from specialize_native_operands import specialize

def main():
    features.baseline.FILES+=(
        'tools/native_simplex_triangle_replacements.py',
        'tools/native_simplex_triangle_scene_replacements.py',
        'tools/generate_scene_device_constructor_tu15.py',
        'tools/generate_physics_live_support_tu15.py',
        'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp',
        'tools/validate_native_simplex_triangle_generation_tu15.py',
        'src/physics/runtime/native_tu15_simplex_triangle_source_owner.hpp',
        'src/physics/runtime/native_tu15_simplex_triangle_abi.hpp',
        'src/physics/runtime/native_tu15_simplex_helpers_handoff_internal.hpp',
        'include/halo2/native_tu15_simplex_triangle_kernel.hpp',
        'include/halo2/native_tu15_simplex_triangle_value.hpp',
        'src/physics/native_tu15_simplex_triangle.cpp',
    )
    result=features.main();path=Path(sys.argv[sys.argv.index('--report')+1]);report=json.loads(path.read_text())
    try:
        scratch=Path(os.environ['HALO2_TEST_SCRATCH']);units=[]
        for name in('world','contact'):
            source=(scratch/(name+'.cpp')).read_text()
            if triangle.replace_simplex_triangle(source)!=source or triangle.installed_entries(source)!=['30d3f0']:
                raise ValueError('Complete generated triangle owner absent')
            prepared,receipt=specialize(source)
            if receipt['native_simplex_triangle_entries']!=['30d3f0'] or triangle.installed_entries(prepared)!=['30d3f0']:
                raise ValueError('Complete prepared triangle owner absent')
            if specialize(prepared)[0]!=prepared:raise ValueError('Repeated triangle preparation changed source')
            units.append(dict(owner=name,raw_body_sha256=triangle.digest(triangle.BODY.search(source)[0]),
                prepared_body_sha256=triangle.digest(triangle.BODY.search(prepared)[0]),
                native_simplex_triangle_entries=receipt['native_simplex_triangle_entries']))
        root=features.baseline.ROOT;source_path=root/'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp';current=source_path.read_text();generated_path=scratch/'scene.cpp'
        command=[sys.executable,'-B',str(root/'tools/generate_scene_device_constructor_tu15.py'),'--output',str(generated_path),'--directory',str(scratch/'scene-generation')]
        made=subprocess.run(command,capture_output=True,text=True)
        if made.returncode:raise ValueError('Complete Scene generator failed: '+(made.stdout+made.stderr)[-5000:])
        generated=generated_path.read_text()
        if generated!=current:raise ValueError('Complete Scene triangle regeneration changed source bytes')
        if scene.replace_simplex_triangle_scene(generated)!=generated or scene.installed_entries(generated)!=['30d3f0']:raise ValueError('Complete Scene triangle default absent')
        prepared,receipt=specialize(generated)
        if receipt['native_simplex_triangle_scene_entries']!=['30d3f0'] or scene.installed_entries(prepared)!=['30d3f0']:raise ValueError('Complete Scene prepared triangle default absent')
        (scratch/'scene-prepared.cpp').write_text(prepared)
        compiler=sys.argv[sys.argv.index('--compiler')+1]if '--compiler'in sys.argv else'clang++'
        for source in(generated_path,scratch/'scene-prepared.cpp'):
            compiled=subprocess.run([compiler,'-std=c++20','-fsyntax-only','-I'+str(root/'include'),'-I'+str(root/'src'),str(source)],capture_output=True,text=True)
            if compiled.returncode:raise ValueError('Complete Scene syntax failed: '+compiled.stderr[-5000:])
        if specialize(prepared)[0]!=prepared:raise ValueError('Scene repeated preparation changed source')
        units.append(dict(owner='scene',raw_body_sha256=scene.digest(scene.BODY.search(generated)[0]),prepared_body_sha256=scene.digest(scene.BODY.search(prepared)[0]),native_simplex_triangle_scene_entries=receipt['native_simplex_triangle_scene_entries']))
        report['native_simplex_triangle']=units;report['all_existing_owner_groups_preserved']=True
    except BaseException as error:report.update(passed=False,triangle_error=repr(error));raise
    finally:path.write_text(json.dumps(report,indent=2)+'\n')
    return result
if __name__=='__main__':raise SystemExit(main())
