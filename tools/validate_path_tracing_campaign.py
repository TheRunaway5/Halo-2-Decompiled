#!/usr/bin/env python3
"""Compare deterministic Original/Path Tracing campaign shots in owned storage.

Invoke through tools/run_test.py --scratch-root .local/test-scratch. Each shot/mode starts its own native process
with neutral input, the same caller interval, interpolation fraction and seed.
The actual source clock and camera receipts must match before image comparison.
All raw frames, traces, settings and caches are removed by the enclosing owners.
Only --report-output and explicitly requested --retain-captures PNG pairs remain.
Pixel metrics describe a comparison; they do not establish artistic parity.
"""
import argparse
import copy
import hashlib
import itertools
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import time
from types import SimpleNamespace

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
from run_campaign_skin_runtime import configuration_errors, valid_state
from run_tests import validation_environment

CAMERA_FIELDS = ('frame', 'tick', 'eye_bits', 'forward_bits', 'up_bits',
                 'projection_angle_bits', 'viewport')
STAGES = ('frame_begin', 'simulation_end', 'before_render', 'after_render')
POSE_FIELDS = ('WORLD_node_bits', 'sampled_node_bits', 'builder_rows', 'ordinary_main_units', 'definition')
POSE_DIAGNOSTIC_OWNER_LIMIT = 20
POSE_DIAGNOSTIC_UNIT_LIMIT = 64
POSE_COMPARISON = 'Exact full stream; only native unit word4 context address replaced by validated full owner'
UNIT_FIELDS = ('ordinal', 'lod', 'coverage', 'lighting_class', 'override_slot', 'shader_override')
UNIT_WORD_FIELDS = tuple(f'unit_words[{word}]' for word in range(8))


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def sequence_sha(rows):
    return hashlib.sha256(json.dumps(rows, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def semantic_main_units(units, owner):
    """Compare the native context by its validated full owner, not its address.

    SourceLiveModelBaseSelectionTu15::copy resolves unit word4 to the owner and
    validates its live generation. The main trace selects that same owner.
    This one native allocation address can differ between rendering modes;
    every other word and selection field remains part of the comparison.
    """
    if not isinstance(units, list):
        raise ValueError('Native main units must be an ordered list')
    result = []
    for unit in units:
        words = unit.get('unit_words') if isinstance(unit, dict) else None
        if not isinstance(words, list) or len(words) != 8 or any(
                type(word) is not int or not 0 <= word <= 0xffffffff for word in words):
            raise ValueError('Native main unit lacks its exact eight source words')
        if words[4] in (0, 0xffffffff):
            raise ValueError('Native main unit lacks its validated owner context')
        compared = list(words)
        compared[4] = owner
        result.append(dict(unit, unit_words=compared))
    return result


def inspect_trace(events, spec, selection_sink=None):
    """Reduce the real trace without retaining large per-actor geometry banks."""
    configurations, cameras, stages, renderers, errors = [], [], [], [], []
    pose_hash, geometry_hash = hashlib.sha256(), hashlib.sha256()
    semantic_pose_hash = hashlib.sha256()
    pose_field_hashes = {name: hashlib.sha256() for name in POSE_FIELDS}
    unit_field_hashes = {name: hashlib.sha256() for name in (*UNIT_FIELDS, *UNIT_WORD_FIELDS)}
    pose_count = geometry_count = interpolated_count = scene_pose_count = 0
    nonempty_geometry_count = interpolated_geometry_count = materialized_vertices = 0
    pending_poses = {}
    seen_poses = set()
    last_actor_frame, last_actors = None, []
    count = 0
    for count, event in enumerate(events, 1):
        if type(event.get('sequence')) is not int or event['sequence'] != count - 1:
            raise ValueError('Trace sequence is missing, duplicated or reordered')
        kind = event.get('event')
        if kind == 'configuration':
            configurations.append(event)
        elif kind == 'main_view':
            if type(event.get('frame')) is not int or type(event.get('tick')) is not int:
                raise ValueError('Camera frame/tick receipt must contain integers')
            if not valid_state(event.get('state')):
                raise ValueError('Invalid actual camera source-clock receipt')
            if event.get('main_commands_completed') is not True:
                raise ValueError('Campaign source main commands did not complete')
            for name in ('eye_bits', 'forward_bits', 'up_bits'):
                if not isinstance(event.get(name), list) or len(event[name]) != 3 or any(
                        type(v) is not int or not 0 <= v <= 0xffffffff for v in event[name]):
                    raise ValueError('Camera receipt lacks exact vector bits')
            if type(event.get('projection_angle_bits')) is not int or not isinstance(event.get('viewport'), list) or len(event['viewport']) != 4:
                raise ValueError('Camera receipt lacks projection/viewport')
            cameras.append({key: event[key] for key in CAMERA_FIELDS})
        elif kind == 'source_state':
            if not valid_state(event.get('state')):
                raise ValueError('Invalid actual source-clock receipt')
            stages.append([event.get('stage'), event.get('frame'), event['state']['source_tick']])
        elif kind == 'renderer':
            if type(event.get('frame')) is not int or event.get('submitted') is not True:
                raise ValueError('Missing actual submitted renderer frame')
            renderers.append([event.get('frame'), event.get('submitted'), event.get('width'), event.get('height')])
            if pending_poses:
                raise ValueError('Actor pose has no materialized geometry receipt')
            seen_poses.clear()
        elif kind in ('actor_pose', 'actor_geometry', 'scene_pose', 'scene_geometry'):
            frame, owner = event.get('frame'), event.get('owner')
            if type(frame) is not int or not 0 <= frame < spec['frames'] or type(owner) is not int or not 0 <= owner < 0xffffffff:
                raise ValueError('Actor receipt lacks an exact frame/full owner')
            key = (frame, owner)
            model_kind = 'scene_object' if kind.startswith('scene_') else 'actor'
            if last_actor_frame != frame:
                last_actor_frame, last_actors = frame, []
            if kind.endswith('_pose'):
                for name in ('WORLD_node_bits', 'sampled_node_bits'):
                    bank = event.get(name)
                    if not isinstance(bank, list) or not bank or any(not isinstance(node, list) or len(node) != 13 or
                            any(type(v) is not int or not 0 <= v <= 0xffffffff for v in node) for node in bank):
                        raise ValueError('Actor receipt lacks complete exact WORLD/sample node bits')
                if len(event['WORLD_node_bits']) != len(event['sampled_node_bits']) or key in seen_poses:
                    raise ValueError('Actor sampled pose count or ordering differs')
                fields = ('frame', 'owner', 'definition', 'builder_rows', 'ordinary_main_units', 'WORLD_node_bits', 'sampled_node_bits')
                if any(name not in event for name in fields):
                    raise ValueError('Actor receipt lacks native model ownership')
                digest = sequence_sha(dict(model_kind=model_kind, **{name: event[name] for name in fields}))
                pose_hash.update(bytes.fromhex(digest));pose_count += 1
                compared = {name: event[name] for name in fields}
                compared['ordinary_main_units'] = semantic_main_units(event['ordinary_main_units'], owner)
                semantic_digest = sequence_sha(dict(model_kind=model_kind, **compared))
                semantic_pose_hash.update(bytes.fromhex(semantic_digest))
                units = event['ordinary_main_units']
                for name, field_hash in unit_field_hashes.items():
                    values = [unit['unit_words'][UNIT_WORD_FIELDS.index(name)] if name in UNIT_WORD_FIELDS
                              else unit.get(name) for unit in units]
                    field_hash.update(bytes.fromhex(sequence_sha(dict(frame=frame, owner=owner,
                        model_kind=model_kind, value=values))))
                if selection_sink:
                    selection_sink(dict(frame=frame, owner=owner, model_kind=model_kind, units=units))
                field_digests = {name: sequence_sha(dict(frame=frame, owner=owner, model_kind=model_kind,
                                                       value=event[name])) for name in POSE_FIELDS}
                for name, field_digest in field_digests.items():
                    pose_field_hashes[name].update(bytes.fromhex(field_digest))
                scene_pose_count += model_kind == 'scene_object'
                interpolated_count += event['WORLD_node_bits'] != event['sampled_node_bits']
                receipt = dict(frame=frame, owner=owner, nodes=len(event['sampled_node_bits']), pose_sha256=digest,
                               model_kind=model_kind, interpolated=event['WORLD_node_bits'] != event['sampled_node_bits'],
                               pose_field_sha256=field_digests, semantic_pose_sha256=semantic_digest)
                # Retain only the last frame's small native selection records;
                # complete WORLD/sample banks remain streamed hashes. Truncated
                # diagnostics never replace the full strict comparison digest.
                if len(last_actors) < POSE_DIAGNOSTIC_OWNER_LIMIT:
                    units = event['ordinary_main_units']
                    receipt['native_selection'] = dict(definition=event['definition'], builder_rows=event['builder_rows'],
                        ordinary_main_units=copy.deepcopy(units[:POSE_DIAGNOSTIC_UNIT_LIMIT]), ordinary_main_unit_count=len(units),
                        ordinary_main_units_truncated=len(units) > POSE_DIAGNOSTIC_UNIT_LIMIT)
                seen_poses.add(key)
                pending_poses[key] = receipt
                last_actors.append(receipt)
            else:
                digest = event.get('complete_materialized_vertex_sha256')
                if key not in pending_poses or type(event.get('vertices')) is not int or event['vertices'] < 0 or not isinstance(digest, str) or len(digest) != 64 or any(c not in '0123456789abcdef' for c in digest):
                    raise ValueError('Actor geometry lacks a matching pose or complete materialized hash')
                if pending_poses[key]['model_kind'] != model_kind:
                    raise ValueError('Model geometry changed its native presentation owner kind')
                if not event['vertices'] and digest != hashlib.sha256(b'').hexdigest():
                    raise ValueError('Empty actor geometry has a nonempty materialized hash')
                geometry_hash.update(bytes.fromhex(sequence_sha({name: event[name] for name in
                    ('frame', 'owner', 'vertices', 'complete_materialized_vertex_sha256')})))
                geometry_count += 1
                receipt = pending_poses.pop(key)
                receipt.update(vertices=event['vertices'], geometry_sha256=digest)
                materialized_vertices += event['vertices']
                nonempty_geometry_count += event['vertices'] > 0
                interpolated_geometry_count += event['vertices'] > 0 and receipt['interpolated']
    if len(configurations) != 1:
        raise ValueError('Capture needs exactly one configuration receipt')
    args = SimpleNamespace(**spec, squad=None, graphics_mode='Path Tracing' if spec['mode'] == 'PathTracing' else 'Original')
    errors += configuration_errors(configurations[0], args)
    expected_stages = [['before_scenario_factories', None], ['after_scenario_factories', None], ['before_frames', 0]]
    expected_stages += [[stage, frame] for frame in range(spec['frames']) for stage in STAGES]
    if [row[:2] for row in stages] != expected_stages:
        errors.append('Actual source stages do not cover every declared frame')
    if [row['frame'] for row in cameras] != list(range(spec['frames'])):
        errors.append('Actual camera receipts do not cover every declared frame')
    if renderers != [[frame, True, spec['width'], spec['height']] for frame in range(spec['frames'])]:
        errors.append('Actual submitted renderer count or dimensions differ')
    if not stages or not any(b[2] > a[2] for a, b in zip(stages, stages[1:])):
        errors.append('Actual source clock did not advance')
    if pending_poses or pose_count != geometry_count:
        errors.append('Actor pose/geometry receipts are incomplete')
    if errors:
        raise ValueError('; '.join(errors))
    return dict(events=count, cameras=cameras, stages=stages,
                camera_sha256=sequence_sha(cameras), source_tick_sha256=sequence_sha(stages),
                actors=dict(pose_count=pose_count, geometry_count=geometry_count,
                    scene_object_pose_count=scene_pose_count, actor_pose_count=pose_count-scene_pose_count,
                    interpolated_pose_count=interpolated_count, nonempty_geometry_count=nonempty_geometry_count,
                    interpolated_geometry_count=interpolated_geometry_count, materialized_vertices=materialized_vertices,
                    pose_sha256=pose_hash.hexdigest(),
                    pose_field_sha256={name: digest.hexdigest() for name, digest in pose_field_hashes.items()},
                    unit_field_sha256={name: digest.hexdigest() for name, digest in unit_field_hashes.items()},
                    semantic_pose_sha256=semantic_pose_hash.hexdigest(),
                    pose_comparison=POSE_COMPARISON,
                    geometry_sha256=geometry_hash.hexdigest(), last_frame=last_actor_frame, last_actors=last_actors))


def compare_traces(original, traced):
    errors = []
    if original['cameras'] != traced['cameras']:
        errors.append('Original/Path Tracing source cameras or projection differ')
    if original['stages'] != traced['stages']:
        errors.append('Original/Path Tracing source tick sequence differs')
    for trace in (original, traced):
        actors = trace['actors']
        hashes = [actors.get('semantic_pose_sha256')]
        hashes += [actor.get('semantic_pose_sha256') for actor in actors['last_actors']]
        if actors.get('pose_comparison') != POSE_COMPARISON or any(
                not isinstance(value, str) or len(value) != 64 or any(c not in '0123456789abcdef' for c in value)
                for value in hashes):
            errors.append('Capture lacks the full-stream semantic pose comparison receipt')
            return errors
    def compared_actors(actors):
        # Raw dispatch hashes/addresses remain diagnostic. The semantic digest
        # still covers every record, including those outside retained details.
        result = {key: value for key, value in actors.items()
                  if key not in ('pose_sha256', 'pose_field_sha256', 'unit_field_sha256', 'last_actors')}
        result['last_actors'] = [{key: value for key, value in actor.items()
                                 if key not in ('pose_sha256', 'pose_field_sha256', 'native_selection')}
                                for actor in actors['last_actors']]
        return result
    if compared_actors(original['actors']) != compared_actors(traced['actors']):
        errors.append('Original/Path Tracing sampled model poses or materialized geometry differ')
    return errors


def selection_differences(original_path, traced_path):
    """Reduce scratch-only native selections to bounded exact early differences."""
    count, examples = 0, []
    def compared(row):
        return None if row is None else dict(row, units=semantic_main_units(row['units'], row['owner']))
    def bounded(row):
        if row is None:
            return None
        return dict(row, units=row['units'][:POSE_DIAGNOSTIC_UNIT_LIMIT],
                    unit_count=len(row['units']), units_truncated=len(row['units']) > POSE_DIAGNOSTIC_UNIT_LIMIT)
    with Path(original_path).open() as original, Path(traced_path).open() as traced:
        for left, right in itertools.zip_longest(original, traced):
            a, b = (json.loads(line) if line is not None else None for line in (left, right))
            ca, cb = compared(a), compared(b)
            if ca == cb:
                continue
            count += 1
            if len(examples) < POSE_DIAGNOSTIC_OWNER_LIMIT:
                fields = []
                if ca and cb and all(ca[key] == cb[key] for key in ('frame', 'owner', 'model_kind')):
                    for unit, (u, v) in enumerate(itertools.zip_longest(ca['units'], cb['units'])):
                        if unit >= POSE_DIAGNOSTIC_UNIT_LIMIT:
                            break
                        if u is None or v is None:
                            fields.append(dict(unit=unit, field='unit_count'))
                            continue
                        for name in sorted(set(u) | set(v)):
                            if name == 'unit_words':
                                fields += [dict(unit=unit, field=UNIT_WORD_FIELDS[word], Original=x, PathTracing=y)
                                           for word, (x, y) in enumerate(zip(u[name], v[name])) if x != y]
                            elif u.get(name) != v.get(name):
                                fields.append(dict(unit=unit, field=name, Original=u.get(name), PathTracing=v.get(name)))
                examples.append(dict(Original=bounded(a), PathTracing=bounded(b), differing_fields=fields))
    return dict(differing_records=count, retained_records=len(examples), first_differences=examples)


def motion_errors(trace, final_frame):
    actors = trace['actors']
    if actors['last_frame'] != final_frame or not any(
            actor['interpolated'] and actor['vertices'] > 0 for actor in actors['last_actors']):
        return ['Final captured frame lacks nonempty geometry with an observed interpolated model pose']
    return []


def capture_worker(spec_path, result_directory):
    """Nested run_test owner: export only compact comparison inputs to its parent."""
    from PIL import Image
    from validate_path_tracing_visual import classify_validation_diagnostics, private_preferences
    spec = json.loads(spec_path.read_text())
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    parent = Path(spec['owner']).resolve(strict=True)
    result_directory = result_directory.resolve(strict=True)
    if not scratch.is_relative_to(parent) or not result_directory.is_relative_to(parent) or result_directory == parent:
        raise ValueError('Capture result must belong to the enclosing scratch owner')
    prefs = scratch/'data/Halo2Native/Halo2/menu.cfg'
    prefs.parent.mkdir(parents=True)
    prefs.write_bytes(private_preferences(b'', spec['width'], spec['height'], spec['mode'], 'Off', 1).replace(
        b'pc_path_tracing 1 1', f"pc_path_tracing {spec['samples']} {spec['bounces']}".encode()))
    frame, trace, log = (scratch/name for name in ('frame.ppm', 'trace.jsonl', 'runtime.log'))
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith('HALO2_') and key not in ('WAYLAND_DISPLAY', '__NGX_CONF_FILE')}
    environment.update(HALO2_TEST_SCRATCH=str(scratch), SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy',
        XDG_DATA_HOME=str(scratch/'data'), XDG_CONFIG_HOME=str(scratch/'config'), XDG_CACHE_HOME=str(scratch/'cache'),
        HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch/'pipelines'), MESA_SHADER_CACHE_DIR=str(scratch/'mesa'),
        __GL_SHADER_DISK_CACHE_PATH=str(scratch/'nvidia'), HALO2_MENU_FOG_SEED=str(spec['auxiliary_seed']),
        HALO2_RENDER_TIMINGS=str(scratch/'render-timings.tsv'))
    if spec['frames'] > 600:
        environment['HALO2_REFERENCE_CAMPAIGN_CAPTURE_FRAME_LIMIT'] = str(spec['frames'])
    environment, validation = validation_environment(ROOT, environment)
    command = ['xvfb-run', '-a', '-s', f"-screen 0 {max(800,spec['width'])}x{max(600,spec['height'])}x24",
        spec['runtime'], '--campaign', '--map', spec['map'], '--shared-maps', str(Path(spec['map']).parent),
        '--frames', str(spec['frames']), '--viewport', str(spec['width']), str(spec['height']),
        '--render-size', str(spec['width']), str(spec['height']), '--graphics-mode', spec['mode'],
        '--mute', '--hidden', '--validation', '--screenshot', str(frame), '--campaign-reference-capture',
        str(spec['interval']), str(spec['fraction']), str(spec['auxiliary_seed']), str(trace)]
    report = dict(passed=False, command=command, mode=spec['mode'], frames=spec['frames'], preferences_sha256=sha(prefs),
                  validation_environment=validation, scratch=str(scratch))
    started = time.monotonic()
    try:
        with log.open('wb') as output:
            result = subprocess.run(command, cwd=Path(spec['runtime']).parent, env=environment,
                                    stdout=output, stderr=subprocess.STDOUT, timeout=spec['timeout_seconds'])
        report['exit_code'] = result.returncode
        with log.open(errors='replace') as lines:
            report.update(classify_validation_diagnostics(lines, spec['mode']))
        if result.returncode or report['validation_errors']:
            raise RuntimeError('Native capture failed or reported Vulkan validation errors')
        with log.open(errors='replace') as lines:
            path_frames = [json.loads(line.removeprefix('Path Tracing frame: ')) for line in lines
                           if line.startswith('Path Tracing frame: ')]
        report['path_tracing_frames'] = path_frames
        if spec['mode'] == 'PathTracing':
            if len(path_frames) != 1 or path_frames[0].get('completed') is not True or path_frames[0].get('views', 0) < 1 or \
                    path_frames[0].get('opaque_draws', 0) < 1 or path_frames[0].get('samples_per_pixel') != spec['samples'] or \
                    path_frames[0].get('max_bounces') != spec['bounces']:
                raise ValueError('Missing completed Path Tracing draw/quality receipt')
        elif path_frames:
            raise ValueError('Original control unexpectedly drew Path Tracing')
        selection_trace = result_directory/'native-selections.jsonl'
        with trace.open() as stream, selection_trace.open('w') as selections:
            compact = inspect_trace((json.loads(line) for line in stream), spec,
                lambda row: selections.write(json.dumps(row, separators=(',', ':'))+'\n'))
        report['native_selection_trace_bytes'] = selection_trace.stat().st_size
        report['trace_sha256'] = sha(trace)
        report['camera_sha256'] = compact['camera_sha256']
        report['source_tick_sha256'] = compact['source_tick_sha256']
        report['actors'] = compact['actors']
        report['last_camera'] = compact['cameras'][-1]
        report['trace_events'] = compact['events']
        with Image.open(frame) as image:
            if image.size != (spec['width'], spec['height']):
                raise ValueError('Framebuffer dimensions differ')
            image.convert('RGB').save(result_directory/'frame.png')
        (result_directory/'trace.json').write_text(json.dumps(compact)+'\n')
        report['passed'] = True
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        report['error'] = str(error)
        if log.exists():
            with log.open('rb') as stream:
                stream.seek(max(0, log.stat().st_size-8192))
                report['diagnostic_tail'] = stream.read().decode(errors='replace')
    finally:
        report['seconds'] = round(time.monotonic()-started, 3)
        (result_directory/'result.json').write_text(json.dumps(report, indent=2)+'\n')
    return 0 if report['passed'] else 1


def image_comparison(original_path, traced_path, target=None, comparison_passed=True):
    from PIL import Image, ImageChops, ImageDraw, ImageStat
    with Image.open(original_path) as original, Image.open(traced_path) as traced:
        original, traced = original.convert('RGB'), traced.convert('RGB')
        difference = ImageStat.Stat(ImageChops.difference(original, traced))
        result = dict(original_mean_rgb=ImageStat.Stat(original).mean,
                      traced_mean_rgb=ImageStat.Stat(traced).mean, mean_absolute_rgb=difference.mean,
                      comparison_passed=comparison_passed,
                      scope='Descriptive framebuffer differences; no aesthetic pass threshold')
        if target:
            header = 24 if comparison_passed else 42
            combined = Image.new('RGB', (original.width*2, original.height+header), (15, 17, 22))
            combined.paste(original, (0, header));combined.paste(traced, (original.width, header))
            draw = ImageDraw.Draw(combined)
            draw.text((8, 6), 'Original', fill='white')
            draw.text((original.width+8, 6), 'Path Tracing', fill='white')
            if not comparison_passed:
                draw.text((8, 24), 'FAILED synchronization checks - visual comparison only', fill=(255, 100, 100))
            combined.save(target)
            result['retained_capture'] = dict(path=str(target), bytes=target.stat().st_size, sha256=sha(target))
        return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path)
    parser.add_argument('--map', type=Path)
    parser.add_argument('--frames', '--frame', type=int, nargs='+', default=[450, 1450])
    parser.add_argument('--require-interpolated-geometry', type=int, action='append', default=[], metavar='FRAME',
                        help='Require an interpolated model with nonempty selected geometry in this final captured frame')
    parser.add_argument('--width', type=int, default=640)
    parser.add_argument('--height', type=int, default=480)
    parser.add_argument('--samples', type=int, choices=(1, 2), default=1)
    parser.add_argument('--bounces', type=int, choices=(1, 2), default=1)
    parser.add_argument('--interval', type=float, default=1/30)
    parser.add_argument('--fraction', type=float, default=.5)
    parser.add_argument('--auxiliary-seed', type=int, default=1)
    parser.add_argument('--timeout-seconds', type=float, default=300)
    parser.add_argument('--report-output', type=Path)
    parser.add_argument('--retain-captures', type=Path)
    parser.add_argument('--capture-worker', type=Path, help=argparse.SUPPRESS)
    parser.add_argument('--result-directory', type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    if args.capture_worker:
        return capture_worker(args.capture_worker, args.result_directory)
    if not args.report_output:
        parser.error('--report-output is required')
    if not all(1 <= n <= 20000 for n in args.frames) or not 1 <= args.width <= 7680 or not 1 <= args.height <= 4320:
        parser.error('Require 1..20000 frames and a bounded positive viewport')
    if any(frame not in args.frames for frame in args.require_interpolated_geometry):
        parser.error('--require-interpolated-geometry must name one of the selected --frames')
    if not math.isfinite(args.interval) or not 0 < args.interval <= .05 or not math.isfinite(args.fraction) or not 0 <= args.fraction <= 1:
        parser.error('Require interval (0,.05] and fraction [0,1]')
    if not 0 <= args.auxiliary_seed <= 0xffffffff or not math.isfinite(args.timeout_seconds) or not 0 < args.timeout_seconds <= 1800:
        parser.error('Require uint32 seed and finite timeout (0,1800]')
    selection = json.loads((ROOT/'gameplay-runtime-selection.json').read_text())
    runtime = (args.runtime or ROOT/selection['runtime']).resolve(strict=True)
    source_map = (args.map or ROOT/selection['cache']/'halo2/h2_maps_win64_dx11/00a_introduction.map').resolve(strict=True)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    filesystem = subprocess.check_output(['stat', '-f', '-c', '%T', str(scratch)], text=True).strip()
    if filesystem in ('tmpfs', 'ramfs'):
        parser.error('Campaign sound snapshots require disk-backed scratch; invoke run_test.py --scratch-root .local/test-scratch')
    output = args.report_output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    retained = args.retain_captures.resolve() if args.retain_captures else None
    if retained:
        retained.mkdir(parents=True, exist_ok=False)
    pins = {'runtime': sha(runtime), 'map': sha(source_map)}
    shader_directory = runtime.parent/'shaders'
    shader_pins = {str(p.relative_to(shader_directory)): sha(p) for p in sorted(shader_directory.rglob('*.spv'))}
    report = dict(passed=False, runtime=str(runtime), map=str(source_map), input_sha256=pins,
        configuration=dict(width=args.width, height=args.height, interval=args.interval, fraction=args.fraction,
                           auxiliary_seed=args.auxiliary_seed, samples=args.samples, bounces=args.bounces,
                           require_interpolated_geometry=args.require_interpolated_geometry),
        shader_inventory_sha256=sequence_sha(shader_pins), helper_sha256=sha(__file__),
        scope='Matched native campaign source cameras/ticks and sampled actor/cinematic scene-object poses/materialized geometry; framebuffer metrics do not establish Xbox or artistic parity',
        shots=[], retained_large_artifacts=[], scratch=str(scratch))
    def save():
        output.write_text(json.dumps(report, indent=2)+'\n')
    save()
    try:
        for frame_count in dict.fromkeys(args.frames):
            shot = dict(frames=frame_count, captures=[], passed=False)
            report['shots'].append(shot)
            roles = {}
            for mode in ('Original', 'PathTracing'):
                directory = scratch/f'{frame_count}-{mode}'
                directory.mkdir()
                spec = dict(runtime=str(runtime), map=str(source_map), frames=frame_count, width=args.width,
                    height=args.height, interval=args.interval, fraction=args.fraction, auxiliary_seed=args.auxiliary_seed,
                    samples=args.samples, bounces=args.bounces, mode=mode, timeout_seconds=args.timeout_seconds, owner=str(scratch))
                spec_path = directory/'spec.json';spec_path.write_text(json.dumps(spec))
                command = [sys.executable, '-B', str(ROOT/'tools/run_test.py'), '--scratch-root', str(directory/'owned'), '--',
                    sys.executable, '-B', str(Path(__file__).resolve()), '--capture-worker', str(spec_path), '--result-directory', str(directory)]
                result = subprocess.run(command, cwd=ROOT)
                receipt = directory/'result.json'
                capture = json.loads(receipt.read_text()) if receipt.exists() else dict(passed=False, error='Capture worker produced no receipt')
                capture['scratch_removed'] = not Path(capture.get('scratch', str(directory/'missing'))).exists()
                shot['captures'].append(capture);save()
                if result.returncode or not capture['passed'] or not capture['scratch_removed']:
                    raise RuntimeError(f'{mode} frame {frame_count} failed')
                roles[mode] = directory
            traces = [json.loads((roles[mode]/'trace.json').read_text()) for mode in ('Original', 'PathTracing')]
            shot['errors'] = compare_traces(*traces)
            if frame_count in args.require_interpolated_geometry:
                for trace in traces:
                    shot['errors'] += motion_errors(trace, frame_count-1)
            shot['comparison'] = image_comparison(roles['Original']/'frame.png', roles['PathTracing']/'frame.png',
                retained/f'frame-{frame_count}-comparison.png' if retained else None, not shot['errors'])
            if shot['comparison'].get('retained_capture'):
                report['retained_large_artifacts'].append(shot['comparison']['retained_capture'])
            if shot['errors']:
                shot['native_selection_differences'] = selection_differences(
                    roles['Original']/'native-selections.jsonl', roles['PathTracing']/'native-selections.jsonl')
                raise RuntimeError('; '.join(shot['errors']))
            shot['passed'] = True;save()
            print(f'Campaign frame {frame_count}: matched source cameras/ticks, Original/PT captured', flush=True)
        if pins != {'runtime': sha(runtime), 'map': sha(source_map)} or shader_pins != {
                str(p.relative_to(shader_directory)): sha(p) for p in sorted(shader_directory.rglob('*.spv'))}:
            raise RuntimeError('Runtime, shaders or map changed during the comparison')
        report['passed'] = True
    except (OSError, ValueError, RuntimeError) as error:
        report['error'] = str(error)
    finally:
        try:
            report['helper_final_sha256'] = sha(__file__)
        except OSError:
            report['helper_final_sha256'] = None
        if report['helper_final_sha256'] != report['helper_sha256']:
            report['passed'] = False
            report['error'] = '; '.join(filter(None, (report.get('error'), 'Capture helper changed during the comparison')))
        save()
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
