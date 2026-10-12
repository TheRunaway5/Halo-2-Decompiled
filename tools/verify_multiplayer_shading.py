#!/usr/bin/env python3
"""Capture all supplied multiplayer maps through the production Slayer path.

This checks native execution, texture/shader admission, Vulkan synchronization
and resolution/layout behavior. It does not compare original Xbox framebuffers.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw

DISC = ('ascension beavercreek burial_mounds coagulation colossus cyclotron '
        'foundation headlong lockout midship waterworks zanzibar').split()
PACK = 'containment warlock deltatap turf backwash dune elongation gemini triplicate'.split()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=Path('build-mp-shading/halo2_native'))
    parser.add_argument('--maps', type=Path, default=Path('assets/disc/maps'))
    parser.add_argument('--pack', type=Path, default=Path('assets/map-pack/maps'))
    parser.add_argument('--output', type=Path, default=Path('analysis/multiplayer-shading/live'))
    parser.add_argument('--validation', action='store_true')
    parser.add_argument('--map-name', action='append', help='Restrict to named maps for repair checks')
    parser.add_argument('--players', type=int, nargs='+', default=[1, 2, 4])
    parser.add_argument('--viewport', type=int, nargs=2, default=[1280, 720],
                        help='Primary capture extent, including odd split-screen dimensions')
    parser.add_argument('--resolutions', action='store_true', help='Also capture effect maps at 480p, 1080p and 4K')
    parser.add_argument('--frames', type=int, default=3)
    parser.add_argument('--capture-tick', type=int,
                        help='Capture after an explicit native simulation tick, with no player actions')
    parser.add_argument('--starting-weapon', type=int, default=5,
                        help='Original primary-weapon option (5=SMG) for a consistent appearance fixture')
    args = parser.parse_args()
    if any(value <= 0 or value > 32767 for value in args.viewport):
        parser.error('viewport must fit the source positive signed-short rectangle')
    binary = args.binary.resolve()
    hashes = {str(binary): digest(binary)}
    hashes.update({str(p): digest(p) for p in sorted((binary.parent/'shaders').glob('*.spv'))})
    args.output.mkdir(parents=True, exist_ok=True)
    idle_script = None
    if args.capture_tick is not None:
        if not 0 < args.capture_tick <= 1000000:
            raise ValueError('capture tick outside native finite clock range')
        idle_script = (args.output/'native-idle-input.txt').resolve()
        idle_script.write_text('# Explicit no-action input for source clock capture.\n')
    paths = {name: args.maps/(name+'.map') for name in DISC}
    paths.update({name: args.pack/(name+'.map') for name in PACK})
    names = args.map_name or list(paths)
    hashes[str(Path(__file__).resolve())] = digest(Path(__file__).resolve())
    for name in names:
        path = paths[name].resolve()
        hashes[str(path)] = digest(path)
    jobs = [(name, count, *args.viewport) for name in names for count in args.players]
    if args.resolutions:
        jobs.extend((name, 1, w, h) for name in ['beavercreek', 'lockout', 'backwash', 'dune', 'gemini']
                    if name in names for w, h in [(640, 480), (1920, 1080), (3840, 2160)])
    results = []
    environment = os.environ.copy()
    environment.setdefault('DISPLAY', ':0')
    for name, count, width, height in jobs:
        stem = f'{name}-{count}p-{width}x{height}'
        capture, log = args.output/(stem+'.ppm'), args.output/(stem+'.log')
        command = [str(binary), '--multiplayer', '--map', str(paths[name].resolve()),
                   '--mp-starting-weapon', str(args.starting_weapon),
                   '--local-players', str(count), '--viewport', str(width), str(height),
                   '--hidden', '--mute', '--screenshot', str(capture.resolve())]
        if idle_script is None:
            command.extend(['--frames', str(args.frames)])
        else:
            command.extend(['--mp-input-script',str(idle_script),'--mp-capture-tick',str(args.capture_tick)])
        if args.validation:
            command.append('--validation')
        with log.open('w') as output:
            try:
                status = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT,
                                        env=environment, timeout=120).returncode
            except subprocess.TimeoutExpired:
                status = -1
        text = log.read_text()
        diagnostics = [line for line in text.splitlines() if any(marker in line for marker in
            ['Vulkan validation:', 'fallback:', 'error:', 'Scenery:', 'Machine:', 'Prop:',
             'Object lighting unresolved:', 'Machine lighting unresolved:', 'Prop lighting unresolved:'])]
        passed = status == 0 and capture.is_file() and not diagnostics
        record = {'map': name, 'players': count, 'resolution': [width, height],
                  'exit_code': status, 'passed': passed, 'diagnostics': diagnostics,
                  'capture': str(capture.resolve()), 'log': str(log.resolve())}
        if passed:
            with Image.open(capture) as frame:
                record['passed'] = frame.size == (width, height)
                record['rgb_extrema'] = frame.getextrema()
                frame.save(capture.with_suffix('.png'))
        results.append(record)
        print(f"{stem}: {'passed' if record['passed'] else 'FAILED'}", flush=True)
        (args.output/'summary.json').write_text(json.dumps({'results': results}, indent=2)+'\n')
    contact = Image.new('RGB', (4*320, ((len(names)+3)//4)*200), '#151515')
    labels = ImageDraw.Draw(contact)
    for index, name in enumerate(names):
        path = args.output/f'{name}-{args.players[0]}p-{args.viewport[0]}x{args.viewport[1]}.ppm'
        if path.is_file():
            with Image.open(path) as frame:
                frame.thumbnail((320, 180))
                contact.paste(frame, ((index%4)*320, (index//4)*200))
        labels.text(((index%4)*320+5, (index//4)*200+182), name, fill='white')
    contact.save(args.output/'all-maps.png')
    unchanged = all(path.is_file() and digest(path) == value for name, value in hashes.items()
                    for path in [Path(name)])
    executable_shader_hashes = {name: value for name, value in hashes.items()
                               if Path(name) == binary or Path(name).suffix == '.spv'}
    report = {'binary_and_shader_sha256': executable_shader_hashes,
              'capture_input_sha256': hashes, 'artifacts_unchanged': unchanged,
              'validation': args.validation, 'frames': args.frames, 'maps': names,
              'native_capture_tick': args.capture_tick,
              'primary_viewport': args.viewport,
              'native_input_script_sha256': None if idle_script is None else digest(idle_script),
              'original_starting_weapon_option': args.starting_weapon,
              'captures': len(results), 'passed': sum(r['passed'] for r in results),
              'original_frame_comparison_performed': False, 'results': results}
    routing_from = environment.get('HALO2_CAPTURE_SHADER_FROM')
    routing_to = environment.get('HALO2_CAPTURE_SHADER_TO')
    if routing_from or routing_to:
        if not routing_from or not routing_to or Path(routing_to).resolve() != binary.parent/'shaders':
            raise RuntimeError('capture shader routing does not target the recorded snapshot')
        report['capture_only_shader_file_routing'] = {
            'from': routing_from, 'to': routing_to,
            'preload_libraries': {name: digest(Path(name)) for name in
                                  environment.get('LD_PRELOAD', '').split(':') if name}}
    (args.output/'summary.json').write_text(json.dumps(report, indent=2)+'\n')
    return 0 if unchanged and all(r['passed'] for r in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
