"""Optional actual-subscription smoke. All map/runtime/capture copies are owned.

Run through tools/run_test.py. Retain only the explicit small JSON report; the
original Workshop subscription and prepared MCC cache remain read-only inputs.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import struct
import sys
import time
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import steam_workshop as workshop


def compressed_owner_check(item, scenario, base, scratch):
    from mcc_content import MccCache, decode_cache, texture_resource
    source = item / 'maps' / (scenario + '.map')
    decoded = decode_cache(source.read_bytes())
    chunk_size = 1024 * 1024
    count = (len(decoded) - 896 + chunk_size - 1) // chunk_size
    header = bytearray(decoded[:896])
    struct.pack_into('<H', header, 28, struct.unpack_from('<H', header, 28)[0] | 1)
    struct.pack_into('<4I', header, 776, chunk_size, 0, 896, count)
    table, blocks = bytearray(count * 8), []
    address = 896 + len(table)
    for index in range(count):
        raw = decoded[896 + index * chunk_size:896 + (index + 1) * chunk_size]
        packed = zlib.compress(raw, level=1)
        stored = packed if len(packed) < len(raw) else raw
        struct.pack_into('<ii', table, index * 8, len(stored) if stored is packed else -len(stored), address)
        blocks.append(stored)
        address += len(stored)
    private_item = scratch / 'compressed-input' / item.name
    workshop.save(private_item / 'ModInfo.json', workshop.metadata(item, 'ModInfo.json'))
    workshop.save(private_item / 'CampaignInfo.json', dict(CampaignMaps=['campaign/' + scenario + '.json']))
    workshop.save(private_item / 'campaign' / (scenario + '.json'),
                  workshop.metadata(item, 'campaign/' + scenario + '.json'))
    target = private_item / 'maps' / (scenario + '.map')
    target.parent.mkdir()
    with target.open('wb') as output:
        output.write(header)
        output.write(table)
        for block in blocks:
            output.write(block)
    del decoded, blocks
    encoded_pin = workshop.pin(target)
    prepared = workshop.prepare_cached(private_item, scenario, base, scratch / 'compressed-prepared')
    assert workshop.prepare_cached(private_item, scenario, base, scratch / 'compressed-prepared') == prepared
    result = MccCache(Path(prepared['map']))
    try:
        assert not result.u16(28) & 1, 'embedded resource owner remained an encoded chunk container'
        partition = None
        for tag in result.tags.values():
            if tag['group'] == 'bitm' and tag['offset'] is not None:
                for frame in result.block(tag['offset'] + 68, 168):
                    size = result.u32(frame + 80)
                    if size:
                        partition = result.u32(frame + 56), size
                        break
            if partition:
                break
        assert partition, 'actual mission supplied no bitmap partition'
        with source.open('rb') as original, (Path(prepared['shared_maps']) / 'textures.dat').open('rb') as staged:
            assert texture_resource(original, *partition) == texture_resource(staged, *partition), \
                'compressed preparation changed embedded bitmap bytes'
    finally:
        result.close()
    assert workshop.pin(target) == encoded_pin, 'compressed source fixture was modified'
    return dict(passed=True, encoded_source=encoded_pin,
                prepared=workshop.pin(Path(prepared['map'])), bitmap_bytes_unchanged=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', required=True, type=Path)
    parser.add_argument('--base-cache', required=True, type=Path)
    parser.add_argument('--item', type=Path, default=Path.home() /
                        '.local/share/Steam/steamapps/workshop/content/976730/3360515088')
    parser.add_argument('--scenario', default='earthcity_e3')
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--render', action='store_true')
    parser.add_argument('--wait-runtime', type=int, default=0)
    parser.add_argument('--ready-marker', type=Path)
    parser.add_argument('--inspect-seconds', type=int, default=0)
    parser.add_argument('--test-compressed', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    report = dict(schema='H2_WORKSHOP_RUNTIME_SMOKE_V1', passed=False, checks=[])
    try:
        deadline = time.monotonic() + args.wait_runtime
        while args.wait_runtime:
            if args.runtime.is_file() and os.access(args.runtime,os.X_OK) and (args.ready_marker is None or args.ready_marker.is_file()):
                before = args.runtime.stat()
                time.sleep(1)
                after = args.runtime.stat()
                if (before.st_size,before.st_mtime_ns,before.st_mode) == (after.st_size,after.st_mtime_ns,after.st_mode):
                    break
            if time.monotonic() >= deadline:
                raise TimeoutError('native runtime did not finish linking within the declared wait')
            time.sleep(.5)
        report['catalog'] = workshop.scan()
        report['source_before'] = workshop.pin(args.item / 'maps' / (args.scenario + '.map'))
        # The engine test build may finish/clean while this check runs. Retain
        # an owned executable/shader snapshot inside this test's scratch only.
        runtime = scratch / 'runtime/halo2_native'
        runtime.parent.mkdir()
        shutil.copy2(args.runtime, runtime)
        shutil.copytree(args.runtime.parent / 'shaders', runtime.parent / 'shaders')
        for library in args.runtime.parent.glob('*.so*'):
            if library.is_file():
                shutil.copy2(library, runtime.parent / library.name)
        for name in ('eos_sdk_worker', 'eos-client.env'):
            if (args.runtime.parent / name).is_file():
                shutil.copy2(args.runtime.parent / name, runtime.parent / name)
        for name in ('data', 'upscalers', 'custom-assets'):
            if (args.runtime.parent / name).is_dir():
                shutil.copytree(args.runtime.parent / name, runtime.parent / name)
        report['runtime'] = workshop.pin(runtime)
        prepared = workshop.prepare_cached(args.item, args.scenario, args.base_cache, scratch / 'prepared')
        reused = workshop.prepare_cached(args.item, args.scenario, args.base_cache, scratch / 'prepared')
        assert prepared == reused, 'second launch changed the immutable prepared owner'
        report['prepared'] = prepared
        report['source_after'] = workshop.pin(args.item / 'maps' / (args.scenario + '.map'))
        assert report['source_before'] == report['source_after'], 'preparation changed the subscription'
        if args.test_compressed:
            report['compressed_owner'] = compressed_owner_check(args.item, args.scenario, args.base_cache, scratch)
        common = ['--map', prepared['map'], '--shared-maps', prepared['shared_maps'], '--mute']
        commands = [('campaign_tick', [str(runtime), *common, '--campaign', '--simulate', '1'])]
        if args.render:
            commands.append(('mods_menu', [str(runtime), '--menu-maps',
                str(args.base_cache.resolve() / 'halo2/h2_maps_win64_dx11'), '--menu-screen', '6',
                '--menu-action', 'select:5', '--menu-action', 'accept', '--menu-ui-ms', '20000',
                '--skip-intro', '--menu-wait-intro-clear', '--frames', '4', '--hidden', '--viewport', '960', '720', '--mute',
                '--screenshot', str(scratch / 'mods-menu.ppm')]))
            commands.append(('map_preview', [str(runtime), *common, '--mcc-preview',
                '--prepared-mcc-cache-root', prepared['prepared_root'], '--frames', '4',
                '--graphics-mode', 'Original', '--hidden', '--viewport', '640', '480',
                '--screenshot', str(scratch / 'e3-preview.ppm')]))
            commands.append(('original_render', [str(runtime), *common, '--campaign', '--frames', '4',
                '--graphics-mode', 'Original', '--hidden', '--viewport', '640', '480',
                '--screenshot', str(scratch / 'e3.ppm')]))
        for name, command in commands:
            started = time.monotonic()
            try:
                result = subprocess.run(command, capture_output=True, text=True, timeout=180)
                row = dict(name=name, command=command, exit_code=result.returncode,
                           seconds=time.monotonic() - started,
                           stdout=result.stdout[-24000:], stderr=result.stderr[-16000:])
            except subprocess.TimeoutExpired as error:
                row = dict(name=name, command=command, exit_code=None,
                           seconds=time.monotonic() - started, error='180 second timeout',
                           stdout=str(error.stdout)[-16000:], stderr=str(error.stderr)[-8000:])
            report['checks'].append(row)
            if name=='mods_menu' and row['exit_code']==0:
                from qualify_frontend_runtime import visible_menu_capture
                row['visible_menu']=visible_menu_capture(scratch/'mods-menu.ppm')
                # An Unavailable dialog also renders nonuniform pixels and
                # exits successfully. Require the actual final Mods route.
                prefix='Menu capture route screen='
                row['menu_routes']=[line[len(prefix):] for line in row['stdout'].splitlines()
                                    if line.startswith(prefix)]
                row['mods_route_reached']=bool(row['menu_routes'] and row['menu_routes'][-1]=='60')
            print(name + ': exit=' + str(row['exit_code']) + ' ' + row.get('stderr', '')[-500:], flush=True)
        report['passed'] = all(row['exit_code'] == 0 and row.get('mods_route_reached',True)
                               for row in report['checks'])
        if (scratch / 'e3.ppm').is_file():
            report['frame'] = workshop.pin(scratch / 'e3.ppm')
        if (scratch / 'e3-preview.ppm').is_file():
            report['preview_frame'] = workshop.pin(scratch / 'e3-preview.ppm')
            if args.inspect_seconds:
                from PIL import Image
                for name in ('e3-preview', 'mods-menu'):
                    if (scratch / (name + '.ppm')).is_file():
                        Image.open(scratch / (name + '.ppm')).save(scratch / (name + '.png'))
                        print('Temporary inspection frame: ' + str(scratch / (name + '.png')), flush=True)
                time.sleep(min(60, args.inspect_seconds))
        return 0 if report['passed'] else 1
    except Exception as error:
        report['error'] = repr(error)
        raise
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    raise SystemExit(main())
