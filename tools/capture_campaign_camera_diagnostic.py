#!/usr/bin/env python3
"""Capture the ordinary authored opening in run_test.py-owned storage.

The existing opt-in frame/pose diagnostics do not issue script commands or
advance source time. Only bounded PNG references and a compressed pose trace
are retained; the owner removes the raw frames and process log.
"""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import time

from physics_test_cache import runtime_cache, runtime_map


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--runtime', type=Path, required=True)
    ap.add_argument('--mcc-root', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--seconds', type=int, default=240)
    ap.add_argument('--width', type=int, default=960)
    ap.add_argument('--height', type=int, default=540)
    ap.add_argument('--stop-after-tick', type=int, default=5300)
    ap.add_argument('--private-display', action='store_true',
                    help='Caller owns an xvfb-run display inside this scratch owner')
    ap.add_argument('--menu-subtitles', action='store_true',
                    help='Use actual campaign-menu keys with an isolated subtitles-on preference')
    ap.add_argument('--validation', action='store_true',
                    help='Enable Vulkan validation and the repository layer when available')
    ap.add_argument('--graphics-mode', choices=('Original','pStencil','PathTracing'), default='Original')
    ap.add_argument('--launcher', type=Path,
                    help='Exercise this actual script entry point while verifying the declared runtime image')
    ap.add_argument('--launcher-profile-suffix',
                    help='The inspected launcher suffix appended to its isolated XDG directories')
    ap.add_argument('--capture-tick-range',
                    help='Retain every completed GPU frame in a bounded first:last tick window')
    ap.add_argument('--depth-diagnostics', action='store_true',
                    help='Read completed GPU D24/stencil only for the bounded capture window')
    ap.add_argument('--hologram-depth-diagnostics', action='store_true',
                    help='Retain matched native hologram/body inputs with the bounded depth readback')
    args = ap.parse_args()
    if not 1 <= args.seconds <= 600 or not 1 <= args.stop_after_tick <= 5731:
        ap.error('Use1..600seconds and1..5731opening ticks')
    if not 320 <= args.width <= 4096 or not 180 <= args.height <= 2160:
        ap.error('Use a bounded320..4096 by180..2160surface')
    capture_range = None
    if args.capture_tick_range:
        match = re.fullmatch(r'(\d+):(\d+)', args.capture_tick_range)
        if not match:
            ap.error('Capture range requires first:last')
        first, last = map(int, match.groups())
        if not 0 <= first <= last <= 5731 or last - first > 8 or args.stop_after_tick <= last:
            ap.error('Capture at most9 consecutive opening ticks and stop after the window')
        capture_range = (first, last)
    if args.depth_diagnostics and not capture_range:
        ap.error('Depth diagnostics require a bounded capture tick range')
    if args.hologram_depth_diagnostics and not args.depth_diagnostics:
        ap.error('Matched hologram inputs require the bounded GPU depth diagnostic')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.menu_subtitles and not args.private_display:
        ap.error('Menu input requires the caller-owned private Xvfb display')
    if args.launcher and not (args.private_display and args.menu_subtitles):
        ap.error('Launcher acceptance requires the owned display and actual menu input')
    if args.launcher_profile_suffix and (not args.launcher or not re.fullmatch(
            r'[A-Za-z0-9][A-Za-z0-9_-]*', args.launcher_profile_suffix)):
        ap.error('A launcher profile suffix must be one safe directory name')
    if args.private_display:
        auth = Path(os.environ.get('XAUTHORITY', '/missing')).resolve()
        if not auth.is_file() or not auth.is_relative_to(scratch):
            ap.error('Private Xvfb authority must belong to HALO2_TEST_SCRATCH')
    args.output.mkdir(parents=True, exist_ok=False)
    maps, receipt = runtime_cache(args.mcc_root)
    intro = runtime_map(maps, maps / '00a_introduction.map')
    runtime = args.runtime.resolve(strict=True)
    launcher = args.launcher.resolve(strict=True) if args.launcher else None
    env = {k: v for k, v in os.environ.items() if not k.startswith('HALO2_')}
    env.update(HALO2_TEST_SCRATCH=str(scratch), SDL_AUDIODRIVER='dummy', SDL_VIDEODRIVER='x11',
               MESA_SHADER_CACHE_DIR=str(scratch/'mesa'),
               __GL_SHADER_DISK_CACHE_PATH=str(scratch/'nvidia'),
               HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch/'pipelines'),
               HALO2_PSTENCIL_CAMPAIGN_TRACE=str(scratch / 'poses.jsonl'),
               HALO2_CAMPAIGN_HOLOGRAM_TRACE=str(scratch / 'hologram.jsonl'),
               HALO2_PSTENCIL_CAMPAIGN_CAPTURES=str(scratch / 'frames'))
    if capture_range:
        env['HALO2_PSTENCIL_CAMPAIGN_CAPTURE_TICK_RANGE'] = args.capture_tick_range
    if args.depth_diagnostics:
        env['HALO2_DEPTH_DIAGNOSTICS'] = '1'
    if args.hologram_depth_diagnostics:
        env['HALO2_HOLOGRAM_DEPTH_DIAGNOSTICS'] = '1'
    validation_layer = None
    if args.validation:
        from run_tests import validation_environment
        env, validation_layer = validation_environment(Path(__file__).resolve().parents[1], env)
    for key in ('XDG_CONFIG_HOME', 'XDG_DATA_HOME', 'XDG_CACHE_HOME'):
        env[key] = str(scratch / key.lower())
    command = [str(launcher or runtime),'--shared-maps',str(maps),'--frames','30000','--mute',
               '--viewport',str(args.width),str(args.height),'--graphics-mode',args.graphics_mode]
    if args.validation:
        command.append('--validation')
    if args.menu_subtitles:
        effective_data = Path(env['XDG_DATA_HOME'])
        if args.launcher_profile_suffix:
            effective_data /= args.launcher_profile_suffix
        prefs = effective_data / 'Halo2Native/Halo2'
        prefs.mkdir(parents=True)
        (prefs / 'menu.cfg').write_text('halo2-native-menu-v1\nactive "Camera Diagnostic"\n'
            'profile "Camera Diagnostic"\npc_display 0 0 0 3 0 0 0 10 1\n')
        command = ['stdbuf','-oL','-eL',*command,'--menu','--menu-screen','218',
                   '--menu-maps',str(maps),'--skip-intro']
    else:
        command += ['--campaign','--map',str(intro),'--hidden']
    if not args.private_display:
        command = ['xvfb-run','-a','-s','-screen 0 1280x720x24',*command]
    report = dict(command=command, runtime_sha256=hashlib.sha256(runtime.read_bytes()).hexdigest(),
                  mcc_inputs=receipt, scratch=str(scratch), seconds=args.seconds,
                  opening_map=dict(path=str(intro),bytes=intro.stat().st_size,
                      sha256=hashlib.sha256(intro.read_bytes()).hexdigest()),
                  source_commands_injected=False, retained=[], completed=False,
                  stop_after_tick=args.stop_after_tick,
                  menu_subtitles=args.menu_subtitles,menu_inputs=[],
                  validation_requested=args.validation,validation_layer=validation_layer,
                  graphics_mode=args.graphics_mode,
                  launcher=(dict(path=str(launcher),sha256=hashlib.sha256(launcher.read_bytes()).hexdigest(),
                      profile_suffix=args.launcher_profile_suffix) if launcher else None),
                  runtime_image_observed=False,
                  capture_tick_range=capture_range,
                  depth_diagnostics=args.depth_diagnostics,
                  hologram_depth_diagnostics=args.hologram_depth_diagnostics,
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output / 'capture.json').write_text(json.dumps(report, indent=2) + '\n')
    with (scratch / 'runtime.log').open('w') as log:
        process = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT)
        started = time.monotonic()
        inputs = None
        try:
            while process.poll() is None:
                if not report['runtime_image_observed']:
                    pending = [process.pid]
                    while pending:
                        pid = pending.pop()
                        try:
                            if os.path.samefile(f'/proc/{pid}/exe', runtime):
                                report['runtime_image_observed'] = True
                                report['runtime_pid'] = pid
                                break
                            pending.extend(map(int, Path(f'/proc/{pid}/task/{pid}/children').read_text().split()))
                        except OSError:
                            continue
                if args.menu_subtitles and len(report['menu_inputs']) < 2:
                    with (scratch / 'runtime.log').open('rb') as stream:
                        stream.seek(max(0, (scratch / 'runtime.log').stat().st_size - 32768))
                        observed = stream.read().decode(errors='replace')
                    expected = 'Menu ready screen=218' if not report['menu_inputs'] else 'Menu screen 12'
                    if expected in observed:
                        if inputs is None:
                            sys.path.insert(0,str(Path(__file__).resolve().parents[1] / 'tests'))
                            from native_split_screen_menu_startup import X11Input
                            inputs = X11Input()
                        inputs.key('Return')
                        report['menu_inputs'].append(dict(observation=expected,key='Return',
                            window=inputs.window,seconds=round(time.monotonic()-started,3)))
                trace = scratch / 'poses.jsonl'
                if trace.exists():
                    with trace.open('rb') as stream:
                        stream.seek(max(0, trace.stat().st_size - 8192))
                        lines = stream.read().splitlines()
                    for line in reversed(lines):
                        try:
                            row = json.loads(line)
                        except (ValueError, UnicodeError):
                            continue
                        if row['tick'] >= args.stop_after_tick:
                            report['declared_tick_reached'] = row['tick']
                        break
                if report.get('declared_tick_reached') or time.monotonic() - started >= args.seconds:
                    break
                time.sleep(.1)
            report['completed'] = process.poll() is not None
        finally:
            if inputs is not None:
                inputs.close()
            if process.poll() is None:
                # Retire only this Xvfb wrapper's owned descendants. Keep all
                # children in the outer run_test.py group for cancellation.
                def descendants(pid):
                    path = Path(f'/proc/{pid}/task/{pid}/children')
                    try:
                        children = [int(p) for p in path.read_text().split()]
                    except FileNotFoundError:
                        return []
                    return [p for child in children for p in [*descendants(child), child]]
                for pid in descendants(process.pid):
                    try:
                        os.kill(pid, signal.SIGTERM)
                    except ProcessLookupError:
                        pass
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
        report['elapsed_seconds'] = round(time.monotonic() - started, 3)
        report['returncode'] = process.returncode
    trace = scratch / 'poses.jsonl'
    if trace.exists():
        packed = lzma.compress(trace.read_bytes())
        (args.output / 'poses.jsonl.xz').write_bytes(packed)
    hologram = scratch / 'hologram.jsonl'
    if hologram.exists():
        (args.output / 'hologram.jsonl.xz').write_bytes(lzma.compress(hologram.read_bytes()))
    depth_references = []
    geometry_references = []
    for path in sorted((scratch / 'frames').glob('*.ppm')):
        target_tick = re.search(r'-target-(\d+)\.ppm$', path.name)
        temporal_tick = re.fullmatch(r'tick-(\d+)-frame-\d+\.ppm', path.name)
        retain = (capture_range and temporal_tick and capture_range[0] <= int(temporal_tick[1]) <= capture_range[1])
        retain = retain or (not capture_range and target_tick and int(target_tick[1]) in (1150, 1400, 3400, 5000, 5200))
        if not retain:
            continue
        target = args.output / (path.stem + '.png')
        subprocess.run(['magick', str(path), str(target)], check=True)
        if args.depth_diagnostics:
            for suffix in ('.depth-d24-low24.bin', '.stencil-u8.bin'):
                source = path.with_name(path.name + suffix)
                raw = source.read_bytes()
                packed = lzma.compress(raw)
                if lzma.decompress(packed) != raw:
                    raise RuntimeError('Depth reference compression changed bytes')
                retained = args.output / (path.stem + suffix + '.xz')
                retained.write_bytes(packed)
                depth_references.append(dict(frame=target.name,path=retained.name,
                    raw_bytes=len(raw),raw_sha256=hashlib.sha256(raw).hexdigest(),
                    bytes=len(packed),sha256=hashlib.sha256(packed).hexdigest()))
        if args.hologram_depth_diagnostics:
            source = path.with_name(path.name + '.hologram-depth.json')
            raw = source.read_bytes()
            if len(raw) > 8*1024*1024:
                raise RuntimeError('Matched hologram reference exceeded its declared per-frame bound')
            payload = json.loads(raw)
            if payload.get('schema') != 'H2_HOLOGRAM_DEPTH_INPUT_V1':
                raise RuntimeError('Matched hologram reference schema differs')
            packed = lzma.compress(raw)
            if lzma.decompress(packed) != raw:
                raise RuntimeError('Matched geometry compression changed bytes')
            retained = args.output / (path.stem + '.hologram-depth.json.xz')
            retained.write_bytes(packed)
            geometry_references.append(dict(frame=target.name,path=retained.name,
                raw_bytes=len(raw),raw_sha256=hashlib.sha256(raw).hexdigest(),
                bytes=len(packed),sha256=hashlib.sha256(packed).hexdigest()))
    if args.depth_diagnostics:
        (args.output / 'depth-reference.json').write_text(json.dumps(dict(
            scope='Actual completed GPU D24/stencil aspects; no original Xbox word reinterpretation',
            width=args.width,height=args.height,references=depth_references,
            matched_geometry=geometry_references),indent=2)+'\n')
    with (scratch / 'runtime.log').open('rb') as log:
        log.seek(max(0, (scratch / 'runtime.log').stat().st_size - 65536))
        (args.output / 'runtime-tail.log').write_bytes(log.read())
    report['validation_errors'] = []
    report['validation_error_count'] = 0
    with (scratch / 'runtime.log').open(errors='replace') as log:
        for line in log:
            if line.startswith('error: hologram'):
                failure = line.encode()
                report['hologram_failure_bytes'] = len(failure)
                report['hologram_failure_truncated'] = len(failure) > 65536
                (args.output / 'hologram-failure.txt').write_bytes(failure[:65536])
            if 'Validation Error' in line or 'VUID-' in line:
                report['validation_error_count'] += 1
                if len(report['validation_errors']) < 8:
                    report['validation_errors'].append(line.strip()[:2048])
    report['retained'] = [dict(path=str(p), bytes=p.stat().st_size,
        sha256=hashlib.sha256(p.read_bytes()).hexdigest())
        for p in sorted(args.output.iterdir()) if p.name != 'capture.json']
    report['retained_bytes'] = sum(row['bytes'] for row in report['retained'])
    report['retention_reason'] = 'Required before/after reference inputs for camera cuts, Earth framing and authored Regret hologram'
    report['runtime_sha256_after'] = hashlib.sha256(runtime.read_bytes()).hexdigest()
    report['passed'] = (trace.exists() and bool(report.get('declared_tick_reached')) and
                        report['returncode'] in (0, -signal.SIGTERM) and
                        report['validation_error_count'] == 0 and
                        report['runtime_image_observed'] and
                        report['runtime_sha256_after'] == report['runtime_sha256'] and
                        (not capture_range or bool(list(args.output.glob('*.png')))) and
                        (not args.depth_diagnostics or len(depth_references) == 2*len(list(args.output.glob('*.png')))) and
                        (not args.hologram_depth_diagnostics or len(geometry_references) == len(list(args.output.glob('*.png')))))
    (args.output / 'capture.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(output=str(args.output), scratch=str(scratch),
                          frames=len(list(args.output.glob('*.png'))),
                          retained_bytes=report['retained_bytes'])))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
