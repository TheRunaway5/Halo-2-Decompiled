#!/usr/bin/env python3
"""Prove the GPU denoiser regressions reject the former failure mechanisms.

CTest/tools/run_test.py own all temporary SPIR-V modules and compiler outputs.
Only the bounded JSON receipt is retained by an explicit --report path.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gpu', type=Path, required=True)
    parser.add_argument('--shaders', type=Path, required=True)
    parser.add_argument('--compiler', required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']) / 'denoiser-negative-controls'
    scratch.mkdir()
    source = ROOT / 'shaders/path_tracing/lighting_filter.glsl'
    controls = (
        ('foreground_camera_reflection', '--foreground-reflection-turn', 'lighting_filter.glsl',
         'vec3 reflected=reflect(incoming,normal);',
         'vec3 reflected=reflect(incoming,normal); if(camera.eyePosition.w==2.0)reflected=traceHistoryViewBasis()*reflected;',
         'Denoiser retained a foreground reflection trail after a rigid camera turn'),
        ('stale_reflected_object', '--reflected-object-history', 'lighting_filter.glsl',
         'bool traceChangedReflection(vec3 position,vec3 normal,vec3 incoming,float roughness,float pixelFootprint) {',
         'bool traceChangedReflection(vec3 position,vec3 normal,vec3 incoming,float roughness,float pixelFootprint) { return false;',
         'Denoiser retained a trail from moving reflected geometry'),
        ('global_reflection_reset', '--unrelated-reflection-motion', 'lighting_filter.glsl',
         'bool traceChangedReflection(vec3 position,vec3 normal,vec3 incoming,float roughness,float pixelFootprint) {',
         'bool traceChangedReflection(vec3 position,vec3 normal,vec3 incoming,float roughness,float pixelFootprint) { return tracePrevious.changedOccluderMin.w!=0.0;',
         'Unrelated off-axis geometry animation discarded converged specular history'),
        ('stale_reflection', '--reflection-history', 'lighting_filter.glsl',
         'specularCompatible=!viewDependent || traceReflectionCompatible(previousReflected,reflected,roughness);',
         'specularCompatible=true;',
         'Denoiser smeared a sharp reflected highlight'),
        ('stale_shadow', '--multiple-light-shadow-disocclusion', 'lighting_filter.glsl',
         'bool traceChangedShadow(vec3 position,vec3 normal) {',
         'bool traceChangedShadow(vec3 position,vec3 normal) { return false;',
         'RT lighting kept stale shadow visibility'),
        ('animated_light_reset', '--animated-light-noise', 'lighting_filter.glsl',
         'history=0u;count=0u;specularCompatible=false;',
         'history=0u;count=0u;specularCompatible=false; if(tracePrevious.changedLightMin.w!=0.0)return false;',
         'Unrelated offscreen geometry animation discarded denoised receiver lighting'),
        ('uniform_light_bank', '--moving-light-sampling', 'scene.glsl',
         'if(traceScene.counts.z<=64u) {', 'if(false) {',
         'Moving unequal lights or inactive bank entries introduced avoidable direct-light sampling grain'),
        ('stale_material_tint', '--reflection-tint-history', 'lighting_filter.glsl',
         '(tracePrevious.words[candidate+12u]>>5u)==traceReflectionFingerprint(material)', 'true',
         'Denoiser retained stale authored reflection tint at unchanged roughness'),
    )
    source_hashes = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                     for path in source.parent.iterdir() if path.suffix in ('.glsl', '.frag', '.vert', '.geom')}
    report = {'passed': False, 'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
              'source_hashes': source_hashes,
              'gpu_sha256': hashlib.sha256(args.gpu.read_bytes()).hexdigest(),
              'baseline_shader_hashes': {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                                        for path in (args.shaders / 'path_tracing').glob('*.spv')},
              'controls': [], 'retained_large_artifacts': []}
    try:
        for name, option, filename, old, replacement, expected in controls:
            text = (source.parent / filename).read_text()
            if text.count(old) != 1:
                raise ValueError(f'{name}: expected one production shader mutation site')
            variant = scratch / name
            include = variant / 'include/path_tracing'
            include.mkdir(parents=True)
            (include / filename).write_text(text.replace(old, replacement))
            shaders = variant / 'shaders/path_tracing'
            shaders.mkdir(parents=True)
            for path in (args.shaders / 'path_tracing').glob('*.spv'):
                shutil.copyfile(path, shaders / path.name)
            for target, defines in (('world.frag.spv', []),
                                    ('lighting.frag.spv', ['-DH2_PATH_TRACING_LIGHTING_WRITE=1'])):
                command = [args.compiler, '-V', f'-I{include.parent}', f'-I{ROOT / "shaders"}',
                           *defines, str(ROOT / 'shaders/path_tracing/world.frag'), '-o', str(shaders / target)]
                result = subprocess.run(command, capture_output=True, text=True, timeout=30)
                if result.returncode:
                    raise RuntimeError(result.stdout[-3000:] + result.stderr[-3000:])
            result = subprocess.run([str(args.gpu), str(shaders.parent), option],
                                    capture_output=True, text=True, timeout=90)
            output = result.stdout + result.stderr
            rejected = result.returncode != 0 and expected in output
            report['controls'].append({'name': name, 'exit_code': result.returncode,
                                       'rejected_by_regression': rejected, 'output': output[-6000:]})
            print(f'{name}: exit={result.returncode}, rejected={rejected}', flush=True)
            if not rejected:
                raise RuntimeError(f'{name}: mutation was not caught by its precise GPU regression\n{output[-6000:]}')
        if any(hashlib.sha256((ROOT / path).read_bytes()).hexdigest() != digest
               for path, digest in source_hashes.items()):
            raise RuntimeError('Production shader inputs changed during denoiser negative controls')
        report['passed'] = True
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
