#!/usr/bin/env python3
"""Compile/audit both production shiny families, then compare native GPU arithmetic."""
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from compile_native_fragment_kernels import entries, source_for
from native_world_fragment_kernel import lower_combiner_program
from audit_native_renderer_kernels import audit_spirv


def main():
    compiler, optimizer, validator = (shutil.which(name) for name in ('glslangValidator', 'spirv-opt', 'spirv-val'))
    if not all((compiler, optimizer, validator)):
        raise RuntimeError('Native kernel toolchain missing')
    registry = ROOT / 'shaders/native-programs/fragments/pstencil-shiny.json'
    rows = entries([registry])
    if len(rows) != 128:
        raise RuntimeError('Incomplete tangent/normal shiny kernel identity coverage')
    with tempfile.TemporaryDirectory(dir=os.environ['HALO2_TEST_SCRATCH']) as directory:
        work = Path(directory)
        def compile_one(row):
            name = row['family'] + '-' + row['key']
            source, raw, binary = (work / (name + suffix) for suffix in ('.frag', '.raw.spv', '.spv'))
            source.write_text(source_for(ROOT / 'shaders', row))
            commands = ([compiler, '-V', '--target-env', 'vulkan1.3', str(source), '-o', str(raw)],
                        [optimizer, '--preserve-interface', '-O', str(raw), '-o', str(binary)],
                        [validator, '--target-env', 'vulkan1.3', str(binary)])
            for command in commands:
                subprocess.run(command, check=True, capture_output=True)
            if not audit_spirv(binary.read_bytes())['passed']:
                raise RuntimeError('Production shiny kernel reads an instruction bank')
        with ThreadPoolExecutor(max_workers=4) as pool:
            list(pool.map(compile_one, rows))
        unique = {row['key']: row for row in rows}
        modules = []
        for key, row in sorted(unique.items()):
            # Use the identical production combiner lowering with pre-sampled
            # inputs. Packed colors stay dynamic in the normal 15-vector ABI.
            lowered = lower_combiner_program(row['controls'])
            source = '''#version 450
layout(local_size_x=1) in;
struct Fixture {uvec4 program[15]; vec4 samples[4]; vec4 facing; vec4 grazing;};
layout(set=0,binding=0,std430) readonly buffer Inputs {Fixture fixtures[];};
layout(set=0,binding=1,std430) writeonly buffer Outputs {vec4 results[];};
uint fixtureIndex;
#define material fixtures[fixtureIndex]
''' + lowered + '''
void main() {
    fixtureIndex=gl_GlobalInvocationID.x;
    vec4 samples[4];
    for(uint i=0u;i<4u;++i)samples[i]=material.samples[i];
    results[fixtureIndex]=runCombiners(samples,material.facing,material.grazing);
}
'''
            path = work / (key + '.comp')
            binary = work / (key + '.comp.spv')
            path.write_text(source)
            subprocess.run([compiler, '-V', '--target-env', 'vulkan1.3', str(path), '-o', str(binary)],
                           check=True, capture_output=True)
            subprocess.run([optimizer, '--preserve-interface', '-O', str(binary), '-o', str(binary)],
                           check=True, capture_output=True)
            subprocess.run([validator, '--target-env', 'vulkan1.3', str(binary)], check=True, capture_output=True)
            if not audit_spirv(binary.read_bytes())['passed']:
                raise RuntimeError('Numerical probe retained instruction reads')
            modules.append(dict(controls=row['controls'], path=str(binary)))
        manifest = work / 'modules.json'
        manifest.write_text(json.dumps(modules))
        subprocess.run([sys.argv[1], str(manifest)], check=True)
        print(f'{len(rows)} production Vulkan modules compiled and audited without instruction-bank reads')


if __name__ == '__main__':
    main()
