"""Exercise production shader custom commands through actual CMake Makefiles."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]

CHECK=r'''
import hashlib,json,os,subprocess,sys,time
from pathlib import Path
build,source=map(Path,sys.argv[1:])
def calls():
    rows=[json.loads(line) for line in (source/'invocations.jsonl').read_text().splitlines()]
    return {name:sum(any(Path(arg).name==tool for arg in row) and
                    ('--header-only' in row if kind=='header' else
                     '--header-only' not in row and '--list-outputs' not in row)
                    for row in rows)
            for name,tool,kind in [('world','compile_native_world_kernels.py','full'),
                                   ('fragment','compile_native_fragment_kernels.py','full'),
                                   ('header','compile_native_fragment_kernels.py','header')]}
headers=[build/'native-world-kernel-catalog/native_world_kernels.generated.hpp',
         build/'native-world-kernel-catalog/native_position_kernels.generated.hpp',
         build/'native-fragment-catalog/native_fragment_kernels.generated.hpp']
manifests=[build/'native-world-kernel-catalog/compiled.json',build/'native-fragment-catalog/compiled.json']
outputs=manifests+sorted((build/'shaders').rglob('*.spv'))
def pins(paths):return {str(p.relative_to(build)):{'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
    'inode':p.stat().st_ino,'mtime_ns':p.stat().st_mtime_ns} for p in paths}
initial=calls();before=pins(headers+outputs)
assert initial=={'world':1,'fragment':1,'header':1},initial
time.sleep(.02)
dependency=source/'tools/native_world_fragment_kernel.py';changed=time.time_ns();os.utime(dependency,ns=(changed,changed))
command=['cmake','--build',str(build),'--parallel','2','--target','native_world_kernels','native_fragment_kernels']
subprocess.run(command,check=True);revalidated=calls();after=pins(headers+outputs)
subprocess.run(command,check=True);idle=calls();final=pins(headers+outputs)
report={'schema':'HALO2_NATIVE_SHADER_CMAKE_COMPLETION_V1','passed':False,
        'initial_calls':initial,'revalidated_calls':revalidated,'idle_calls':idle,
        'touched_dependency':str(dependency),'dependency_mtime_ns':changed,
        'before':before,'after':after,'final':final,
        'scope':'Actual production CMake world command block and fragment module with reduced authenticated source registries; real GLSL compiler/optimizer/validator; no game or assets build'}
def save():(build/'completion-report.json').write_text(json.dumps(report,indent=2)+'\n')
save()
assert revalidated=={'world':2,'fragment':2,'header':2},revalidated
assert idle==revalidated,('Repeated compilation after identical revalidation',revalidated,idle)
assert before.keys()==after.keys()==final.keys()
for name,old in before.items():
    assert old['sha256']==after[name]['sha256'],('Revalidation changed bytes',name)
    assert after[name]['mtime_ns']>changed,('Declared output remains older than dependency',name)
    assert after[name]==final[name],('Idle target rewrote output',name)
for p in headers:
    name=str(p.relative_to(build));assert before[name]['inode']==after[name]['inode'],('Unchanged header was replaced',name)
report['passed']=True;save()
'''


class NativeShaderBuildCompletion(unittest.TestCase):
    def test_actual_cmake_revalidation_finishes_and_repeated_targets_are_idle(self):
        self.assertTrue(shutil.which('cmake') and shutil.which('make'))
        work=Path(os.environ['HALO2_TEST_SCRATCH'])/'shader-cmake-completion';work.mkdir()
        source=work/'source';source.mkdir()
        shutil.copytree(ROOT/'shaders',source/'shaders')
        (source/'tools').mkdir();(source/'cmake').mkdir()
        tools=('compile_native_world_kernels.py','compile_native_fragment_kernels.py',
               'native_world_vertex_kernel.py','native_world_fragment_kernel.py',
               'native_cpu_vertex_kernel.py','native_combiner_contract.py','audit_native_renderer_kernels.py')
        for name in tools:shutil.copy2(ROOT/'tools'/name,source/'tools'/name)
        cmake=ROOT/'CMakeLists.txt';original=cmake.read_text()
        start=original.index('set(NATIVE_WORLD_KERNEL_REGISTRY ')
        end=original.index('include(cmake/NativeFragmentKernels.cmake)',start)
        block=original[start:end]
        (source/'cmake/world.cmake').write_text(block)
        shutil.copy2(ROOT/'cmake/NativeFragmentKernels.cmake',source/'cmake/NativeFragmentKernels.cmake')
        world_path=ROOT/'shaders/native-programs/world/programs.json';world=json.loads(world_path.read_bytes())
        world['entries']=world['entries'][:1]
        (source/'shaders/native-programs/world/programs.json').write_text(json.dumps(world))
        fragment_source=source/'shaders/native-programs/fragments';shutil.rmtree(fragment_source);fragment_source.mkdir()
        rows=[]
        for name,family in [('effects.json','hud'),('atmosphere.json','fog')]:
            registry=json.loads((ROOT/'shaders/native-programs/fragments'/name).read_bytes())
            rows.append(next(row for row in registry['entries'] if row['family']==family))
        (fragment_source/'programs.json').write_text(json.dumps({'schema':registry['schema'],'entries':rows}))
        python=source/'python-driver'
        python.write_text('#!'+sys.executable+'\nimport json,subprocess,sys\nfrom pathlib import Path\n'
            'with (Path(__file__).parent/"invocations.jsonl").open("a") as stream:stream.write(json.dumps(sys.argv[1:])+"\\n")\n'
            'raise SystemExit(subprocess.run(['+repr(sys.executable)+',*sys.argv[1:]]).returncode)\n')
        python.chmod(0o700)
        (source/'src/rendering').mkdir(parents=True)
        shutil.copy2(ROOT/'src/rendering/native_fragment_kernel.cpp',source/'src/rendering/native_fragment_kernel.cpp')
        (source/'dummy.cpp').write_text('int completion_fixture;\n')
        (source/'check.py').write_text(CHECK)
        project='''cmake_minimum_required(VERSION 3.20)
project(native_shader_completion LANGUAGES CXX)
enable_testing()
set(Python3_EXECUTABLE "@SOURCE@/python-driver")
find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)
find_program(SPIRV_OPTIMIZER spirv-opt REQUIRED)
find_program(SPIRV_VALIDATOR spirv-val REQUIRED)
set(NATIVE_SHADER_SOURCE_DIR "@SOURCE@/shaders/native-programs")
add_custom_target(halo2_shaders)
add_library(halo2_cache STATIC EXCLUDE_FROM_ALL dummy.cpp)
include(cmake/world.cmake)
include(cmake/NativeFragmentKernels.cmake)
add_test(NAME native_shader_completion COMMAND "@PYTHON@" "@SOURCE@/check.py" "@BINARY@" "@SOURCE@")
'''.replace('@SOURCE@',source.as_posix()).replace('@BINARY@','$'+'{CMAKE_BINARY_DIR}').replace('@PYTHON@',sys.executable)
        (source/'CMakeLists.txt').write_text(project)
        output=work/'reports'
        command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(source),
            '--output',str(output),'--scratch-root',str(work/'scratch'),
            '--target','native_world_kernels','--target','native_fragment_kernels',
            '--filter','^native_shader_completion$','--cmake-arg=-GUnix Makefiles',
            '--retain-report','completion-report.json']
        result=subprocess.run(command,capture_output=True,text=True)
        report_path=output/'evidence/completion-report.json'
        detail=json.loads(report_path.read_bytes()) if report_path.exists() else {}
        record={'command':command,'exit_code':result.returncode,'detail':detail,
            'source_pins':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
                for p in [cmake,ROOT/'cmake/NativeFragmentKernels.cmake',*(ROOT/'tools'/name for name in tools)]},
            'actual_world_cmake_block_sha256':hashlib.sha256(block.encode()).hexdigest(),
            'world_registry_source_sha256':hashlib.sha256(world_path.read_bytes()).hexdigest(),
            'reduced_world_entries':len(world['entries']),'reduced_fragment_entries':len(rows)}
        build_report=json.loads((output/'report.json').read_bytes()) if (output/'report.json').exists() else {}
        record['private_build_removed']=not Path(build_report.get('build','/nonexistent')).exists()
        if destination:=os.environ.get('HALO2_NATIVE_SHADER_COMPLETION_REPORT'):
            Path(destination).write_text(json.dumps(record,indent=2)+'\n')
        self.assertEqual(result.returncode,0,result.stdout[-12000:]+result.stderr[-2000:])
        self.assertTrue(detail.get('passed'),detail)
        self.assertTrue(record['private_build_removed'])


if __name__=='__main__':unittest.main()
