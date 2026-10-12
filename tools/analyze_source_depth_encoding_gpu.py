"""Freeze isolated historical-expression D24 attachment diagnostics.

CPU analysis only. This does not activate or amend a production shader, run
Vulkan, reproduce title interpolation, or establish physical Xbox behavior.
"""
from pathlib import Path
import hashlib
import json
import numpy as np

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer-shading/depth-encoding-gpu-prototype'
DOMAIN=1<<24


def entry(path):
    digest=hashlib.sha256()
    with path.open('rb') as f:
        while block:=f.read(1<<20): digest.update(block)
    return dict(path=str(path.relative_to(ROOT)),bytes=path.stat().st_size,sha256=digest.hexdigest())


def main():
    normal=(ROOT/'analysis/multiplayer-shading/depth-encoding-gpu-normal.log').read_bytes()
    sanitized=(ROOT/'analysis/multiplayer-shading/depth-encoding-gpu-asan.log').read_bytes()
    assert normal==sanitized
    assert b'SDK errors0' in normal and b'NVIDIA GeForce RTX 5080' in normal
    for value in (normal,sanitized,(ROOT/'analysis/multiplayer-shading/depth-encoding-gpu-asan-cpu.log').read_bytes()):
        assert b'ERROR:' not in value and b'runtime error:' not in value
    historical=entry(BASE/'historical-world.frag')
    input_reports=[]
    for build in ('build','build-asan'):
        data=json.loads((BASE/build/'shaders/inputs.json').read_text())
        native=next(x for x in data['files'] if x['path']=='shaders/world.frag')
        assert native['sha256']==historical['sha256']
        # Those records retain the actual original path used before its later
        # source-supported production correction, never relabelled as current.
        input_reports.append(data)
    assert input_reports[0]['current_expression']==input_reports[1]['current_expression']
    measured=[]
    for name in ('current','source'):
        filenames=[f'{name}-depth-low24.bin',f'{name}-emitted-float-words.bin']
        for filename in filenames:
            assert entry(BASE/'normal'/filename)['sha256']==entry(BASE/'asan'/filename)['sha256']
            assert (BASE/'normal'/filename).stat().st_size==DOMAIN*4
        depth=np.memmap(BASE/'normal'/filenames[0],dtype='<u4',mode='r')
        emitted=np.memmap(BASE/'normal'/filenames[1],dtype='<u4',mode='r')
        histogram={};count=0;first=None;last=None;examples=[]
        for start in range(0,DOMAIN,1<<20):
            end=min(start+(1<<20),DOMAIN)
            expected=np.arange(start,end,dtype=np.int64)
            delta=depth[start:end].astype(np.int64)-expected
            values,counts=np.unique(delta,return_counts=True)
            for value,n in zip(values,counts):histogram[str(int(value))]=histogram.get(str(int(value)),0)+int(n)
            changed=np.flatnonzero(delta)
            count+=len(changed)
            if len(changed):
                if first is None:first=int(start+changed[0])
                last=int(start+changed[-1])
                for i in changed[:max(0,8-len(examples))]:
                    n=int(start+i);examples.append(dict(input_integer=n,stored_integer=int(depth[n]),emitted_float_word=f'{int(emitted[n]):08X}'))
        measured.append(dict(expression=name,integers=DOMAIN,integer_mismatches=count,
            integer_delta_histogram=histogram,first_mismatch=first,last_mismatch=last,examples=examples))
    assert measured[0]['integer_mismatches']==6291455 and measured[0]['integer_delta_histogram']=={'0':10485761,'1':6291455}
    assert measured[1]['integer_mismatches']==0
    assert (BASE/'normal/measurements.tsv').read_bytes()==(BASE/'asan/measurements.tsv').read_bytes()
    controls=[]
    for line in (BASE/'normal/measurements.tsv').read_text().splitlines():
        fields=line.split('\t')
        if fields[0]=='compare':controls.append(dict(compare_op=int(fields[1]),expression=int(fields[2]),RGB_response_mask=int(fields[3]),unchanged_depth_integer=int(fields[4])))
    assert len(controls)==8
    inputs=[entry(BASE/'historical-world.frag')]
    for report in input_reports:
        for item in report['files']:
            if item['path']=='shaders/world.frag':continue
            path=ROOT/item['path']
            if item['path']=='tools/prepare_source_depth_encoding_gpu.py':
                path=BASE/'history/prepare_source_depth_encoding_gpu.py'
            actual=entry(path);assert actual['sha256']==item['sha256'],str(path)
            if actual not in inputs:inputs.append(actual)
    paths=[ROOT/p for p in (
        'tests/source_depth_encoding_gpu_tests.cpp','tests/source_depth_encoding.vert','tests/source_depth_encoding.frag',
        'tests/source_weather_plate_gpu_fixture.hpp','tools/prepare_source_depth_encoding_gpu.py',
        'tools/build_source_depth_encoding_gpu.sh','tools/analyze_source_depth_encoding_gpu.py',
        'docs/SOURCE_DEPTH_ENCODING_GPU_DIAGNOSTIC.md',
        '.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json',
        '.tools/validation/usr/lib/libVkLayer_khronos_validation.so')]
    inputs.extend(entry(p) for p in paths)
    artifacts=[]
    for directory in ('normal','asan'):
        artifacts.extend(entry(p) for p in sorted((BASE/directory).iterdir()) if p.is_file())
    for build in ('build','build-asan'):
        artifacts.append(entry(BASE/build/'source_depth_encoding_gpu_tests'))
        artifacts.extend(entry(p) for p in sorted((BASE/build/'shaders').iterdir()) if p.is_file())
    artifacts.extend(entry(ROOT/'analysis/multiplayer-shading'/p) for p in (
        'depth-encoding-gpu-build.log','depth-encoding-gpu-build-asan.log',
        'depth-encoding-gpu-normal.log','depth-encoding-gpu-asan.log','depth-encoding-gpu-asan-cpu.log'))
    report=dict(scope=__doc__,historical_production_input=historical,
        current_expression=input_reports[0]['current_expression'],source_expression=input_reports[0]['source_expression'],
        source_xemu_commit=input_reports[0]['source_xemu_commit'],
        device='NVIDIA GeForce RTX 5080',SDK_layer='1.4.363',synchronization_validation=True,
        extent=[4096,4096],single_sample_D24S8=True,emitted_float_companion='R32_UINT',
        measured=measured,current_float_words_differing_from_strict_CPU_division=8388606,
        source_float_words_differing_from_strict_CPU=0,comparators=controls,
        fractional_clamp_cases_per_expression=30,finite_input_guards=30,invalid_extent_guards=4,
        normal_ASAN_measurements_and_all_binary_outputs_identical=True,
        GPU_address_UB_exit=0,GPU_leak_detection=False,CPU_leak_detection=True,
        initialized_stencil=90,target_alpha=127,stencil_alpha_and_no_write_reference_depth_preserved=True,
        SDK_errors=0,
        boundaries=['Isolated literal-expression compiler/attachment diagnostic after admission.',
          'Not an execution of the entire production world fragment module or a native scene depth capture.',
          'No title W interpolation, source window/discard/clipping, scene history, all-map or physical Xbox claim.',
          'The common finite clamp is explicit test input preparation; N=0 and negative W do not prove original geometry admission.',
          'Current means the frozen pre-correction native expression; later production shader changes do not rewrite these measurements.',
          'Native initialized stencil 0x5A and RGBA reference color are controlled test inputs, not original scene state.'],
        inputs=inputs,artifacts=artifacts)
    (BASE/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(report=entry(BASE/'report.json'),measured=measured,comparators=controls),indent=2))


if __name__=='__main__':main()
