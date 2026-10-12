"""Freeze only the new retained sky stencil prototype and verify stable inputs."""
from pathlib import Path
import hashlib
import json
import sys

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer-shading/source-sky-stencil-prototype'
STAGES=ROOT/'analysis/multiplayer-shading/source-sky-stencil-stage-reference'


def entry(path):
    raw=path.read_bytes()
    return dict(path=str(path.relative_to(ROOT)),bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())


def sources():
    paths=[ROOT/x for x in (
        'assets/disc/default.xbe',
        'analysis/multiplayer-shading/original-xemu/original_capture_20.rdc',
        'include/halo2/source_sky_stencil.hpp','src/source_sky_stencil.cpp',
        'include/halo2/source_weather_depth_alias.hpp','tests/source_weather_plate_gpu_fixture.hpp',
        'include/halo2/cache.hpp','include/halo2/source_weather_plate_render.hpp',
        'include/halo2/vertex_program.hpp','include/halo2/render_program.hpp',
        'include/halo2/source_vertex_declaration.hpp','include/halo2/shader_animation.hpp',
        'tests/source_sky_stencil_gpu_fixture.hpp','tests/source_sky_stencil_tests.cpp',
        'tools/build_source_sky_stencil_prototype.sh','tools/export_source_sky_stencil_stage.py',
        'tools/export_source_sky_stencil_coverage.py','tools/inspect_source_sky_stencil_stage.py',
        'tools/freeze_source_sky_stencil_proof.py',
        'analysis/multiplayer-shading/appearance-residual-replay/replay.py',
        'analysis/multiplayer-shading/sky-stencil-chronology/manifest.json',
        'analysis/multiplayer-shading/weather-connected-scene-prototype/inputs-before-validation.json',
        'analysis/multiplayer-shading/weather-depth-alias-prototype/inputs-before-final-validation.json',
        'analysis/multiplayer-shading/weather-pipeline-cmake-integration/manifest.json')]
    paths+=list((ROOT/'shaders').glob('source_sky_stencil*'))
    paths+=list(STAGES.glob('*'))
    paths+=list((OUT/'reference').glob('*'))
    paths+=list(OUT.glob('*.fixture'))+[OUT/'source-admission.json']
    paths+=list((ROOT/'analysis/multiplayer-shading/original20-sky-stencil-analysis').glob('*-packed-d24s8.bin'))
    for event in (8097,8102,8110):
        old=ROOT/'analysis/multiplayer-shading'/('appearance-residual-replay/source' if event!=8110 else 'original20-moon-stencil')
        paths += [old/f'{event}-{name}' for name in ('pipeline.json','fragment.spvasm','vertex.spvasm','fragment-block0.bin','postvs.bin','indices.bin')]
    pinned=ROOT/'analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained/hw/xbox/nv2a/pgraph/glsl'
    paths += [pinned/name for name in ('geom.c','psh.c','vsh.c','common.c')]
    for build in ('build-normal','build-asan'):
        paths += [OUT/build/'source_sky_stencil_tests']+list((OUT/build/'shaders').glob('*.spv'))
    return sorted(set(paths))


def snapshot():
    records=[entry(p) for p in sources()]
    (OUT/'inputs-before-native-validation.json').write_text(json.dumps(dict(scope='New isolated retained sky stencil source/stage/compile inputs; previous frozen proofs are references only.',files=records),indent=2)+'\n')
    print(f'Frozen {len(records)} stable inputs before native validation.')


def freeze():
    snapshot=json.loads((OUT/'inputs-before-native-validation.json').read_text())
    for record in snapshot['files']:
        assert entry(ROOT/record['path'])==record, record['path']
    normal=json.loads((OUT/'native-normal/report.json').read_text())
    sanitized=json.loads((OUT/'native-asan/report.json').read_text())
    assert normal==sanitized and normal['validation_errors']==0
    assert len(normal['metrics'])==17
    assert all(x['depth_differences']==x['stencil_differences']==0 for x in normal['metrics'])
    for n in (OUT/'native-normal').glob('*-packed-d24s8.bin'):
        assert n.read_bytes()==(OUT/'native-asan'/n.name).read_bytes()
    records=[entry(p) for p in sorted(OUT.rglob('*')) if p.is_file() and p.name!='manifest.json']
    records += [entry(ROOT/'docs/SOURCE_SKY_STENCIL_PROTOTYPE.md')]
    report=dict(scope='Exact retained original20 sky stencil endpoints on this native Vulkan diagnostic; no authored native vertices, sky color, world history or live activation.',
        stable_input_count=len(snapshot['files']),stable_inputs_unchanged=True,
        native_normal_and_ASAN_readbacks_equal=True,SDK_synchronization_validation_errors=0,
        cpu_ASAN_leak_detection=True,graphics_ASAN_address_UB=True,graphics_ASAN_leak_detection=False,
        graphics_leak_boundary='External graphics teardown allocations were observed in earlier independent controls. This run disables graphics leak detection and does not claim leak-clean graphics teardown.',
        metrics=normal['metrics'],files=records)
    (OUT/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Frozen {len(records)} new artifacts; {len(snapshot["files"])} stable inputs unchanged.')


if __name__=='__main__':
    {'snapshot':snapshot,'freeze':freeze}[sys.argv[1]]()
