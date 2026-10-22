#!/usr/bin/env python3
"""Freeze explicit native depth stages without rewriting old results."""
import hashlib
import json
import re
from pathlib import Path

import numpy as np
from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer-shading/world-d24-native-migration'
PROJECT = OUT / 'project'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def record(path):
    return {'bytes': path.stat().st_size, 'sha256': digest(path)}


def write(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n')


def aspect_report(directory):
    results = []
    for color in sorted(directory.glob('*.ppm')):
        with Image.open(color) as frame:
            width, height = frame.size
        depth_path = Path(str(color) + '.depth-d24-low24.bin')
        stencil_path = Path(str(color) + '.stencil-u8.bin')
        depth = np.fromfile(depth_path, dtype='<u4') & 0xffffff
        stencil = np.fromfile(stencil_path, dtype=np.uint8)
        assert len(depth) == len(stencil) == width * height, str(color)
        values, counts = np.unique(stencil, return_counts=True)
        assert len(values) == 1 and values[0] == 0, 'Unexpected native stencil writer'
        results.append({'color': str(color.relative_to(ROOT)),
                        'extent': [width, height], 'texels': len(depth),
                        'actual_depth_range': [int(depth.min()), int(depth.max())],
                        'stencil_histogram': {str(int(v)): int(c) for v, c in zip(values, counts)},
                        'aspect_sha256': {str(p.relative_to(ROOT)): digest(p)
                                          for p in (depth_path, stencil_path)}})
    assert results, str(directory)
    report = {'scope': 'Actual native aspect copies, not original stencil ownership.',
              'depth_DWORD_upper_padding_ignored': True,
              'images': len(results), 'texels': sum(r['texels'] for r in results),
              'results': results}
    write(directory / 'depth-aspect-analysis.json', report)
    return report


def main():
    baseline = json.loads((OUT / 'baseline-source-manifest.json').read_text())['files']
    expected = dict(baseline)
    amendments = json.loads((OUT / 'baseline-compile-amendments.json').read_text())
    for name, values in amendments['files'].items():
        expected[name] = {'bytes': (PROJECT / name).stat().st_size,
                          'sha256': values['compiled_sha256']}
    renderer = json.loads((OUT / 'renderer-migration-source.json').read_text())
    expected['src/rendering/renderer/renderer.cpp'] = {'bytes': (PROJECT / 'src/rendering/renderer/renderer.cpp').stat().st_size,
                                    'sha256': renderer['after_sha256']}
    encoded = json.loads((OUT / 'source-encoding-amendments.json').read_text())['files']
    expected.update(encoded)
    synchronized = json.loads((OUT / 'source-depth-sync-amendment.json').read_text())
    expected.update(synchronized['files'])
    mismatches = [name for name, value in expected.items()
                  if record(PROJECT / name) != value]
    assert not mismatches, mismatches
    assert len(expected) == 4551

    # The retained pre-encoding files let historical stages be inspected after
    # the current private project advances. No old capture manifest is edited.
    historical = {
        'src/assets/original_data.cpp': OUT / 'source-amendments/original_data-before-name-fix.cpp',
        'src/rendering/renderer/renderer.cpp': OUT / 'source-amendments/renderer-before-d24.cpp',
        'CMakeLists.txt': OUT / 'source-amendments/CMakeLists-before-encoding.txt',
        'shaders/world.frag': OUT / 'source-amendments/world.frag.before-encoding-fix',
        'shaders/light_volume.frag': OUT / 'source-amendments/light_volume.frag.before-encoding-fix',
    }
    for name, path in historical.items():
        assert record(path) == baseline[name], (name, str(path))
    resource_renderer = OUT / 'source-amendments/renderer-before-load-read-mask.cpp'
    assert digest(resource_renderer) == renderer['after_sha256'] == synchronized['before_sha256']

    native_stages = {
        'D32_baseline': OUT / 'build-baseline/halo2_native-d32-baseline',
        'D24_resource_before_encoding': OUT / 'build-baseline/halo2_native-d24-resource-before-encoding',
        'D24_source_encoding_before_load_dependency': OUT / 'build-source-encoding/halo2_native-before-load-read-mask',
        'D24_source_encoding_and_load_dependency': OUT / 'build-source-encoding/halo2_native',
    }
    report = {'scope': 'Coherent private native source and execution checks. No all-map original equality claim.',
              'source_files_verified': len(expected),
              'current_source': expected,
              'historical_baseline_locations': {
                  name: {'path': str(path.relative_to(ROOT)), **record(path)}
                  for name, path in historical.items()},
              'resource_renderer_before_load_dependency': record(resource_renderer),
              'native_stages': {name: {'path': str(path.relative_to(ROOT)), **record(path)}
                                for name, path in native_stages.items()},
              'compiled_source_encoding': {
                  str(path.relative_to(ROOT)): record(path)
                  for path in sorted((OUT / 'build-source-encoding').glob('*.a'))
                  + sorted((OUT / 'build-source-encoding/shaders').glob('*.spv'))
                  + [OUT / 'build-source-encoding/multiplayer_resize_tests',
                     OUT / 'build-source-encoding/source_scenery_gpu_tests']},
              'shader_source_scope': 'Only W-to-D24 encoding changed; existing reconstruction/admission and source stencil authors are not activated.',
              'original_reference_boundary': 'Retained camera/player fixture does not restore complete original simulation, clocks, visibility, HUD or history. Native capture tick also does not fix host-accumulator interpolation or fractional shader time; sampled RGB deltas are not isolated causal effects of the resource/encoding/dependency changes.'}

    for dirname in ('all21-source-depth-1080', 'all21-source-depth-odd3'):
        directory = OUT / dirname
        summary = json.loads((directory / 'summary.json').read_text())
        assert summary['captures'] == summary['passed'] == 21
        assert summary['artifacts_unchanged'] and summary['validation']
        assert not any(r['diagnostics'] for r in summary['results'])
        report[dirname] = {'summary': record(directory / 'summary.json'),
                           'aspects': aspect_report(directory)}

    stages = {}
    for name, dirname in (('D32', 'retained-d32-baseline'),
                           ('D24_resource', 'retained-d24'),
                           ('D24_source_encoding_before_load_dependency', 'retained-source-encoding'),
                           ('D24_source_encoding_and_load_dependency', 'retained-source-depth')):
        provenance = json.loads((OUT / dirname / 'provenance.json').read_text())
        assert provenance['artifacts_unchanged'] and not provenance['validation_diagnostics']
        assert provenance['native_capture_tick'] == 6
        stages[name] = provenance['diagnostic_image_regions']
    before = np.asarray(Image.open(OUT / 'retained-d24/native.ppm'), dtype=np.int16)
    after = np.asarray(Image.open(OUT / 'retained-source-depth/native.ppm'), dtype=np.int16)
    difference = np.abs(after - before)
    depth_before = np.fromfile(OUT / 'retained-d24/native.ppm.depth-d24-low24.bin', dtype='<u4') & 0xffffff
    depth_after = np.fromfile(OUT / 'retained-source-depth/native.ppm.depth-d24-low24.bin', dtype='<u4') & 0xffffff
    report['retained'] = {'original_region_diagnostics': stages,
                          'resource_vs_encoding_RGB_pixels_differing': int(np.count_nonzero(np.any(difference, axis=2))),
                          'resource_vs_encoding_maximum_RGB_difference': int(difference.max()),
                          'resource_vs_encoding_absolute_RGB_sum': int(difference.sum()),
                          'resource_vs_encoding_depth_texels_differing': int(np.count_nonzero(depth_before != depth_after)),
                          'source_encoding_aspects': aspect_report(OUT / 'retained-source-depth')}
    old_native = OUT / 'retained-source-encoding/native.ppm'
    new_native = OUT / 'retained-source-depth/native.ppm'
    before_sync = np.asarray(Image.open(old_native), dtype=np.int16)
    sync_difference = np.abs(before_sync - after)
    old_depth = np.fromfile(Path(str(old_native) + '.depth-d24-low24.bin'), dtype='<u4') & 0xffffff
    assert np.array_equal(old_depth, depth_after), 'LOAD dependency changed retained depth'
    assert Path(str(old_native) + '.stencil-u8.bin').read_bytes() == Path(str(new_native) + '.stencil-u8.bin').read_bytes()
    report['load_dependency_retained_appearance'] = {
        'RGB_pixels_differing': int(np.count_nonzero(np.any(sync_difference, axis=2))),
        'RGB_maximum_difference': int(sync_difference.max()),
        'RGB_absolute_difference_sum': int(sync_difference.sum()),
        'D24_identical': True, 'S8_identical': True,
        'causal_RGB_attribution_established': False,
        'clock_boundary': 'Same native integer tick; fractional host interpolation and shader clock are not captured/restored.'}
    historical_summary = OUT / 'all21-source-encoding-1080/summary.json'
    old_summary = json.loads(historical_summary.read_text())
    assert old_summary['captures'] == old_summary['passed'] == 21 and old_summary['artifacts_unchanged']
    report['prior_before_load_dependency_all21_1080'] = record(historical_summary)
    focused_log = OUT / 'ctest-source-depth-graphics.log'
    text = focused_log.read_text()
    assert re.search(r'^100% tests passed(?:, 0 tests failed)? out of 2$', text, re.M)
    assert 'Skipped' not in text
    report['focused_checks'] = {'log': record(focused_log), 'passed': 2,
                                'checks': ['36 persistent resize cases', 'scenery lighting, LOD, reuse and other-view isolation']}
    resize_directory = OUT / 'build-source-encoding/resize-test'
    resize = aspect_report(resize_directory)
    assert resize['images'] == 36
    report['focused_checks']['resize_aspects'] = resize
    focused_artifacts = [path for directory in (resize_directory,
                        OUT / 'build-source-encoding/source-scenery-gpu-captures')
                        for path in directory.rglob('*') if path.is_file()]
    report['focused_checks']['artifacts'] = {
        str(path.relative_to(ROOT)): record(path) for path in sorted(focused_artifacts)}
    write(OUT / 'native-depth-migration-report.json', report)
    artifacts = [path for path in OUT.rglob('*') if path.is_file()
                 and not any(part in ('project', 'build-baseline', 'build-source-encoding')
                             for part in path.relative_to(OUT).parts)
                 and path.name not in ('native-depth-migration-artifact-manifest.json',
                                       'freeze-native-depth.log')]
    artifacts += [ROOT / 'tools/freeze_native_depth_migration.py',
                  ROOT / 'docs/SOURCE_WORLD_D24_NATIVE_MIGRATION.md']
    write(OUT / 'native-depth-migration-artifact-manifest.json', {
        'files': {str(path.relative_to(ROOT)): record(path) for path in sorted(artifacts)}})
    print(json.dumps({'source_files': len(expected), 'map_captures': 42,
                      'focused_checks': 2, 'retained': report['retained'],
                      'manifest_sha256': digest(OUT / 'native-depth-migration-artifact-manifest.json')}))


if __name__ == '__main__':
    main()
