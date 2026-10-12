#!/usr/bin/env python3
"""Capture actual scripted world impacts/grenades with Vulkan validation.

Run under a working SDL/X11 display. This verifies native fixed-tick effects
across output sizes/views; it does not supply a matched original game frame.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
INPUTS = ROOT / 'analysis/multiplayer-shading/world-impacts-20261010'


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--case', choices=['impact', 'frag', 'plasma', 'all'], default='all')
    parser.add_argument('--require-light-packets', action='store_true',
                        help='Fail if an admitted light with BSP receivers has no geometry packet; this is not a final-pixel parity check')
    args = parser.parse_args()
    native = args.native.resolve()
    environment = os.environ.copy()
    validation = ROOT / '.tools/validation/usr'
    layer = validation / 'share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json'
    library = validation / 'lib/libVkLayer_khronos_validation.so'
    if layer.is_file() and library.is_file():
        environment['VK_LAYER_PATH'] = str(layer.parent)
        environment['LD_LIBRARY_PATH'] = os.pathsep.join(filter(None, (
            str(library.parent), environment.get('LD_LIBRARY_PATH'))))
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    frozen = output / 'frozen'
    frozen.mkdir()
    shutil.copy2(native, frozen / native.name)
    shutil.copytree(native.parent / 'shaders', frozen / 'shaders')
    manifest = dict(binary_sha256=sha(native), source_frame_comparison=False,
                    require_light_packets=args.require_light_packets,
                    controlled_auxiliary_seed=0x12345678, controlled_tick_cadence=True,
                    scope=__doc__, captures=[], simulation_counts_equal={})
    manifest['validation_runtime'] = {str(path):sha(path) for path in (layer,library) if path.is_file()}
    manifest['shaders'] = {str(p.relative_to(frozen)): sha(p)
                           for p in sorted((frozen / 'shaders').rglob('*')) if p.is_file()}
    runtime_shaders = native.parent / 'shaders'
    manifest['runtime_shader_directory'] = str(runtime_shaders)
    cases = {
        'impact': ('lockout', 'impact-short.inputs.txt', 16),
        'frag': ('lockout', 'frag-view.inputs.txt', 101),
        'plasma': ('midship', 'plasma-light-view.inputs.txt', 58),
    }
    dimensions = [(640, 480, 1), (1280, 720, 1), (1920, 1080, 1), (1280, 720, 4)]
    try:
        for kind, (map_name, inputs, tick) in cases.items():
            if args.case not in ('all', kind):
                continue
            counts = []
            shutil.copy2(INPUTS / inputs, frozen / inputs)
            for width, height, views in dimensions:
                name = f'{kind}-{width}x{height}-views{views}'
                ppm, log = output / f'{name}.ppm', output / f'{name}.log'
                command = [str(frozen / native.name), '--multiplayer', '--map',
                           str(ROOT / f'assets/disc/maps/{map_name}.map'),
                           '--shared-maps', str(ROOT / 'assets/disc/maps'),
                           '--gameplay-xbe', str(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),
                           '--mp-starting-weapon', '5', '--local-players', str(views),
                           '--viewport', str(width), str(height), '--hidden', '--mute',
                           '--validation', '--mp-input-script', str(frozen / inputs),
                           '--mp-reference-render-fraction', '0', '--mp-capture-tick', str(tick),
                           '--mp-reference-auxiliary-seed', str(manifest['controlled_auxiliary_seed']),
                           '--mp-reference-tick-cadence',
                           '--screenshot', str(ppm)]
                with log.open('w') as stream:
                    result = subprocess.run(command, cwd=ROOT, env=environment,
                                            stdout=stream, stderr=subprocess.STDOUT, timeout=120)
                text = log.read_text()
                if result.returncode or not ppm.is_file():
                    raise RuntimeError(f'{name}: capture failed; see {log}')
                errors = [line for line in text.splitlines()
                          if 'Validation Error' in line or 'ERROR / SPEC' in line]
                if errors:
                    raise RuntimeError(f'{name}: Vulkan validation errors; see {log}')
                capture = next(line for line in text.splitlines() if line.startswith('Input capture tick='))
                fields = {key: int(value) for key, value in
                          re.findall(r'(impacts|detonations|decals|world_particles|world_lights|registered_lights)=(\d+)', capture)}
                counts.append(fields)
                png = output / f'{name}.png'
                with Image.open(ppm) as image:
                    image.save(png)
                record = dict(name=name, command=command, capture=capture, counts=fields,
                              png_sha256=sha(png), log_sha256=sha(log), validation_errors=len(errors))
                record['random_state'] = {key:int(value) for key,value in re.findall(
                    r'(capture_tick_cadence|capture_auxiliary_start|capture_auxiliary_final|capture_primary_final)=(\d+)',capture)}
                if record['random_state'].get('capture_auxiliary_start') != manifest['controlled_auxiliary_seed'] or not record['random_state'].get('capture_tick_cadence'):
                    raise RuntimeError(f'{name}: controlled RNG/cadence observation missing')
                record['light_views'] = [dict(view=int(view),epoch=int(epoch),visible=int(visible),selected=int(selected))
                    for view,epoch,visible,selected in re.findall(
                        r'light_view=(\d+) light_epoch=(\d+) visible_lights=(\d+) selected_lights=(\d+)',capture)]
                record['light_receivers'] = [dict(view=int(view),queries=int(queries),epoch=int(epoch),clusters=int(clusters),instances=int(instances))
                    for view,queries,epoch,clusters,instances in re.findall(
                        r'receiver_view=(\d+) receiver_queries=(\d+) receiver_epoch=(\d+) receiver_clusters=(\d+) receiver_instances=(\d+)',capture)]
                record['geometry_light_packets'] = [dict(view=int(view),raw=int(raw),packed=int(packed))
                    for view,raw,packed in re.findall(
                        r'geometry_light_view=(\d+) raw_light_packets=(\d+) packed_light_packets=(\d+)',capture)]
                if len(record['geometry_light_packets']) != views:
                    raise RuntimeError(f'{name}: geometry packet observation missing for a local view')
                receivers = {row['view']:row for row in record['light_receivers']}
                selected = {row['view']:row for row in record['light_views']}
                required = [row['view'] for row in record['geometry_light_packets']
                            if selected[row['view']]['selected'] and
                            (receivers[row['view']]['clusters'] or receivers[row['view']]['instances'])]
                if args.require_light_packets and kind in ('frag', 'plasma') and not required:
                    raise RuntimeError(f'{name}: grenade capture did not exercise an admitted light with BSP receivers')
                record['receiver_views_requiring_light_packets'] = required
                record['receiver_views_missing_light_packets'] = [row['view'] for row in record['geometry_light_packets']
                            if row['view'] in required and not (row['raw'] or row['packed'])]
                manifest['captures'].append(record)
                (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
                print(name, fields, flush=True)
                if args.require_light_packets and record['receiver_views_missing_light_packets']:
                    raise RuntimeError(f'{name}: admitted light receivers have no geometry light packets')
            # Original weather and effects share auxiliary RNG. Aspect ratio
            # and additional local views change weather coverage/history;
            # compare physical resolution only with the same aspect/views.
            # The4:3 and four-view cases remain separately retained captures.
            single_views = [row for row in manifest['captures'] if row['name'].startswith(kind+'-') and row['name'].endswith('views1')]
            same_aspect = [row for row in single_views if row['name'] in (
                f'{kind}-1280x720-views1', f'{kind}-1920x1080-views1')]
            equal = all(row['counts'] == same_aspect[0]['counts'] and
                        row['random_state'] == same_aspect[0]['random_state'] for row in same_aspect)
            manifest['simulation_counts_equal'][kind] = equal
            manifest.setdefault('equal_history_resolution_groups', {})[kind] = [row['name'] for row in same_aspect]
            manifest.setdefault('aspect_random_history', {})[kind] = 'The4:3 capture is a separate weather coverage and auxiliary RNG history; not an equal-history resolution comparison'
            manifest.setdefault('split_view_random_history', {})[kind] = 'Captured independently; extra weather views share auxiliary RNG and are not an equal-history resolution comparison'
            if not equal:
                raise RuntimeError(f'{kind}: equal-view resolution changed controlled effect counts or RNG state')
        if sha(native) != manifest['binary_sha256']:
            raise RuntimeError('Native binary changed during captures')
        actual_shaders = {str(p.relative_to(native.parent)):sha(p)
                          for p in sorted(runtime_shaders.rglob('*')) if p.is_file()}
        if actual_shaders != manifest['shaders']:
            raise RuntimeError('Runtime shaders changed during captures')
        if args.require_light_packets and not any(record['receiver_views_requiring_light_packets']
                                                  for record in manifest['captures']):
            raise RuntimeError('Light packet gate did not exercise an admitted light with BSP receivers')
    finally:
        (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')


if __name__ == '__main__':
    main()
