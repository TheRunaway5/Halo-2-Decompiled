#!/usr/bin/env python3
"""Measure Original-mode 4K campaign and multiplayer through the live renderer."""

import argparse
import csv
import hashlib
import heapq
import json
import math
import os
from pathlib import Path
import re
import statistics
import subprocess
import sys
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
RENDER = (3840, 2160)
ARMORY_ROUTE_TAPE_SHA256 = 'b764a7bb0ba0a8170f5326d1124535ca9a7aba6a8d43db84dc6abb805524c193'
UPRISING_ROUTE_TAPE_SHA256 = '1b818626e5a6c097e46a6d9d5588d49bba433a4a2c762190280aa1734df672ab'
UPRISING_ROUTE_STEPS = 1500
SCENARIOS = {'armory': ('campaign', '01a_tutorial'),
             'cairo': ('campaign', '01b_spacestation'),
             'outskirts': ('campaign', '03a_oldmombasa'),
             'uprising': ('campaign', '08a_deltacliffs'),
             'multiplayer': ('multiplayer', 'coagulation')}
CPU_METRICS = ('interval_ms', 'renderer_ms', 'fence_ms', 'acquire_ms', 'submit_ms', 'present_ms',
               'commands_ms', 'pack_ms', 'geometry_plan_ms', 'geometry_bind_ms', 'world_native_prepare_ms',
               'world_storage_upload_ms', 'world_native_light_prepared_draws', 'vertices')
HOST_METRICS = ('host_input_ms', 'host_common_ms', 'host_pre_render_ms', 'host_texture_camera_prepare_ms',
                'host_view0_objects_ms', 'host_view0_body_ms', 'host_view0_camera_lighting_ms',
                'host_common_thread_cpu_ms')
NATIVE_VIEW_PHASES = ('model_collection', 'main_cull', 'main_commands')
NATIVE_HOST_METRICS = tuple(f'host_view0_{phase}_{metric}' for phase in NATIVE_VIEW_PHASES for metric in ('ms', 'calls'))
MULTIPLAYER_TICK_PHASES = ('match_step', 'state_projection', 'post_step')
GPU_METRICS = ('gpu_total_ms', 'gpu_compute_ms', 'gpu_scene_ms', 'gpu_present_copy_ms')
LIGHT_PHASES = ('collection', 'native_selection', 'point_receivers', 'point_overlays',
                'projected_receiver', 'projected_geometry', 'projected_setup', 'projected_casters',
                'projected_layer_selection', 'projected_shadow_setup', 'projected_packets', 'projected_material')
PROJECTED_NATIVE_PHASES = ('projected_visibility_query', 'projected_receiver_classify',
                           'projected_bsp_units', 'projected_model_units', 'projected_cull',
                           'projected_commands', 'projected_snapshot')
PROJECTED_VISIBILITY_PHASES = ('projected_visibility_native', 'projected_visibility_view_setup',
                               'projected_visibility_queue_reset', 'projected_visibility_portals',
                               'projected_visibility_auxiliary', 'projected_visibility_context_reuse',
                               'projected_visibility_projection', 'projected_visibility_bsp_admission',
                               'projected_visibility_cluster_contents')
PROJECTED_CLUSTER_PHASES = ('projected_visibility_cluster_traversal', 'projected_visibility_cluster_fog',
                            'projected_visibility_object_admission', 'projected_visibility_object_geometry',
                            'projected_visibility_cluster_other_children')
PROJECTED_CLUSTER_WORK = ('projected_visibility_native_object_admissions', 'projected_visibility_stamped_scans',
                          'projected_visibility_stamped_bulk_scans', 'projected_visibility_stamped_scalar_probes',
                          'projected_visibility_stamped_bulk_probes')
LIGHT_COUNTS = ('light_records', 'selected_lights', 'raw_packets', 'point_casters',
                'projected_lights', 'projected_draws')
LEGACY_LIGHT_METRICS = (*LIGHT_COUNTS, 'total_ns',
                        *(f'{phase}_{suffix}' for phase in LIGHT_PHASES for suffix in ('ns', 'calls')))
NATIVE_LIGHT_METRICS = (*LEGACY_LIGHT_METRICS,
                        *(f'{phase}_{suffix}' for phase in PROJECTED_NATIVE_PHASES for suffix in ('ns', 'calls')))
VISIBILITY_LIGHT_METRICS = (*NATIVE_LIGHT_METRICS,
                            *(f'{phase}_{suffix}' for phase in PROJECTED_VISIBILITY_PHASES for suffix in ('ns', 'calls')))
LIGHT_METRICS = (*VISIBILITY_LIGHT_METRICS,
                 *(f'{phase}_{suffix}' for phase in PROJECTED_CLUSTER_PHASES for suffix in ('ns', 'calls')),
                 *PROJECTED_CLUSTER_WORK)
LEGACY_LIGHT_COLUMNS = ('source_frame', 'bsp', 'mode', 'width', 'height', 'cinematic', *LEGACY_LIGHT_METRICS)
NATIVE_LIGHT_COLUMNS = (*LEGACY_LIGHT_COLUMNS, *NATIVE_LIGHT_METRICS[len(LEGACY_LIGHT_METRICS):])
VISIBILITY_LIGHT_COLUMNS = (*NATIVE_LIGHT_COLUMNS, *VISIBILITY_LIGHT_METRICS[len(NATIVE_LIGHT_METRICS):])
LIGHT_COLUMNS = (*VISIBILITY_LIGHT_COLUMNS, *LIGHT_METRICS[len(VISIBILITY_LIGHT_METRICS):])
MAX_LIGHT_TIMING_BYTES = 64 * 1024 * 1024
MAX_LIGHT_TIMING_ROWS = 100_000


def sha256(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def samples(path, count, columns=None):
    with path.open(newline='') as stream:
        reader = csv.DictReader(stream, delimiter='\t')
        rows = list(reader) if columns is None else [
            {key: row[key] for key in columns if key in row} for row in reader]
    if not rows or (count is not None and len(rows) != count) or any(
            int(row['frame']) != index for index, row in enumerate(rows)):
        raise ValueError(f'{path.name} expected {count} consecutive completed frames, found {len(rows)}')
    return rows


def dimensions(rows, columns, expected):
    actual = sorted({tuple(int(row[column]) for column in columns) for row in rows})
    if actual != [expected]:
        raise ValueError(f'{", ".join(columns)} expected {expected}, recorded {actual}')


def summarize(rows, keys):
    result = {}
    for key in keys:
        values = sorted(float(row[key]) for row in rows)
        if any(not math.isfinite(value) or value < 0 for value in values):
            raise ValueError(f'Invalid timing data for {key}')
        result[key] = {'mean': statistics.fmean(values), 'median': statistics.median(values),
                       'p95': values[math.ceil(.95 * len(values)) - 1],
                       'p99': values[math.ceil(.99 * len(values)) - 1], 'max': values[-1]}
    return result


def measure_intervals(rows, args):
    intervals = summarize(rows, ('interval_ms',))['interval_ms']
    fps = 1000 / intervals['mean']
    passes_fps = fps >= args.min_fps
    passes_p95 = intervals['p95'] <= args.max_p95_ms
    passes_p99 = args.max_p99_ms is None or intervals['p99'] <= args.max_p99_ms
    return dict(frames=len(rows), interval_ms=intervals, fps=fps, passes_fps=passes_fps,
                passes_p95=passes_p95, passes_p99=passes_p99, passed=passes_fps and passes_p95 and passes_p99)


def full_route_budget_bins(rows, budget_ms):
    # Attribute slow intervals to a bounded portion of the authored timeline.
    # The individual intervals still determine the full route's score.
    width_ticks = 250
    bins = {}
    for row in rows:
        tick = int(row['host_tick_end'])
        bins.setdefault(tick // width_ticks, []).append(row)
    result = []
    for ordinal, group in sorted(bins.items()):
        slow = [row for row in group if float(row['interval_ms']) > budget_ms]
        advancing = sum(int(row['host_tick_end']) > int(row['host_tick_begin']) for row in slow)
        timing = host_timings(slow, ('host_common_thread_cpu_ms', 'host_pre_render_ms')) if slow else {}
        result.append(dict(first_tick=ordinal * width_ticks, last_tick=(ordinal + 1) * width_ticks - 1,
                           frames=len(group), over_budget=len(slow), over_budget_with_tick=advancing,
                           interval_ms_p95=summarize(group, ('interval_ms',))['interval_ms']['p95'],
                           late_host_p95_ms={key: value['p95'] for key, value in timing.items()}))
    return result


def host_timings(rows, keys=HOST_METRICS):
    return summarize(rows, (key for key in keys if all(row.get(key) not in (None, '') for row in rows)))


def native_view_timings(rows):
    observed = []
    for phase in NATIVE_VIEW_PHASES:
        duration, calls = f'host_view0_{phase}_ms', f'host_view0_{phase}_calls'
        if (all(row.get(calls) not in (None, '') for row in rows) and
                any(int(row[calls]) for row in rows)):
            observed.extend((duration, calls))
    return host_timings(rows, observed)


def summarize_light_rows(rows):
    summary = {'rows': len(rows)}
    if rows:
        metrics = (key for key in LIGHT_METRICS if key in rows[0])
        for key, stats in summarize(rows, metrics).items():
            if key.endswith('_ns'):
                summary[f'{key[:-3]}_ms'] = {name: value / 1_000_000 for name, value in stats.items()}
            else:
                summary[key] = stats
        if 'projected_visibility_native_calls' in rows[0]:
            calls = sum(row['projected_visibility_query_calls'] for row in rows)
            native = sum(row['projected_visibility_native_calls'] for row in rows)
            summary['projected_visibility_coverage'] = {
                'query_calls': calls, 'native_calls': native, 'unsubdivided_calls': calls - native}
        if 'projected_visibility_cluster_traversal_calls' in rows[0]:
            cluster_calls = sum(row['projected_visibility_cluster_contents_calls'] for row in rows)
            cluster_native = sum(row['projected_visibility_cluster_traversal_calls'] for row in rows)
            object_calls = sum(row['projected_visibility_object_admission_calls'] for row in rows)
            object_native = sum(row['projected_visibility_native_object_admissions'] for row in rows)
            summary['projected_cluster_coverage'] = {
                'cluster_calls': cluster_calls, 'native_cluster_calls': cluster_native,
                'unsubdivided_cluster_calls': cluster_calls - cluster_native,
                'object_calls_in_native_clusters': object_calls, 'native_object_calls': object_native,
                'unsubdivided_object_calls': object_calls - object_native}
    return summary


def light_phase_rows(path, cpu, surface):
    coverage = {'renderer_frames': len(cpu)}
    result = {'expected_mode': 'Original', 'expected_surface': list(surface), 'coverage': coverage}
    try:
        if path.stat().st_size > MAX_LIGHT_TIMING_BYTES:
            raise ValueError('Lighting timing trace exceeds the diagnostic size limit')
        rows = []
        with path.open(newline='') as stream:
            reader = csv.reader(stream, delimiter='\t', strict=True)
            columns = tuple(next(reader, ()))
            if columns not in (LIGHT_COLUMNS, VISIBILITY_LIGHT_COLUMNS, NATIVE_LIGHT_COLUMNS, LEGACY_LIGHT_COLUMNS):
                raise ValueError('Lighting timing trace has an unexpected header')
            for line, fields in enumerate(reader, start=2):
                if len(rows) >= MAX_LIGHT_TIMING_ROWS:
                    raise ValueError('Lighting timing trace exceeds the diagnostic row limit')
                if len(fields) != len(columns) or fields[2] not in ('Original', 'pStencil', 'Path Tracing'):
                    raise ValueError(f'Invalid lighting timing row {line}')
                row = {}
                for key, field in zip(columns, fields):
                    if key == 'mode':
                        row[key] = field
                    else:
                        if len(field) > 20 or not field.isascii() or not field.isdecimal():
                            raise ValueError(f'Invalid lighting timing value for {key} on row {line}')
                        row[key] = int(field)
                if (columns in (LIGHT_COLUMNS, VISIBILITY_LIGHT_COLUMNS) and
                        row['projected_visibility_native_calls'] > row['projected_visibility_query_calls']):
                    raise ValueError(f'Lighting timing native query coverage exceeds root calls on row {line}')
                if columns == LIGHT_COLUMNS:
                    object_native = row['projected_visibility_native_object_admissions']
                    scans = row['projected_visibility_stamped_scans']
                    bulk_scans = row['projected_visibility_stamped_bulk_scans']
                    if (row['projected_visibility_cluster_traversal_calls'] >
                            row['projected_visibility_cluster_contents_calls'] or
                            not row['projected_visibility_object_geometry_calls'] <= object_native <=
                            row['projected_visibility_object_admission_calls'] or
                            not bulk_scans <= scans <= 3 * object_native or
                            row['projected_visibility_stamped_scalar_probes'] < scans - bulk_scans or
                            row['projected_visibility_stamped_bulk_probes'] < bulk_scans):
                        raise ValueError(f'Lighting timing cluster work coverage exceeds parent on row {line}')
                rows.append(row)
    except FileNotFoundError:
        return result | {'status': 'unavailable', 'reason': 'Runtime emitted no lighting timing trace'}, None
    except (OSError, ValueError, csv.Error) as error:
        return result | {'status': 'invalid', 'reason': str(error)}, None

    original = [row for row in rows if row['mode'] == 'Original']
    primary = [row for row in original if (row['width'], row['height']) == surface]
    source_frames = [row['source_frame'] for row in primary]
    unique_frames = set(source_frames)
    host_frames = sum(row.get('host_timings_present') == '1' and row.get('host_frame') == row['frame'] for row in cpu)
    coverage.update(trace_rows=len(rows), primary_shape_rows=len(primary), other_mode_rows=len(rows) - len(original),
                    other_original_surface_rows=len(original) - len(primary),
                    distinct_primary_source_frames=len(unique_frames), duplicate_primary_rows=len(primary) - len(unique_frames),
                    missing_primary_source_frames=sum(frame not in unique_frames for frame in range(1, len(cpu) + 1)),
                    out_of_range_primary_rows=sum(not 1 <= frame <= len(cpu) for frame in source_frames),
                    renderer_frames_with_host_identity=host_frames)
    result['aggregate_scope'] = 'original_matching_surface' if primary else 'all_captured_rows'
    result['aggregate'] = summarize_light_rows(primary or rows)

    # The ordinary lighting caller writes frames+1 and the actual drawable
    # dimensions. Require the whole sequence, since an aborted submission or
    # an additional matching view can otherwise shift a window's attribution.
    source_sequence_matches = len(primary) == len(cpu) and all(
        frame == index for index, frame in enumerate(source_frames, start=1))
    coverage['primary_source_sequence_matches'] = source_sequence_matches
    if source_sequence_matches and len(cpu) == host_frames:
        return result | {'status': 'aligned', 'aligned_frames': len(primary)}, primary
    return result | {'status': 'aggregate_only', 'aligned_frames': 0,
                     'reason': 'Primary lighting and renderer host identities did not match one uninterrupted sequence'}, None


def intervals_by_tick_count(rows, *, multiplayer=False):
    by_tick_count = {}
    for row in rows:
        ticks = int(row['host_tick_end']) - int(row['host_tick_begin'])
        by_tick_count.setdefault(ticks, []).append(row)
    results = {}
    for ticks, group in sorted(by_tick_count.items()):
        phases = ['host_common_ms', 'host_common_thread_cpu_ms']
        if multiplayer:
            # The multiplayer input interval also contains the match tick.
            # Keep its child timings grouped by tick count so render-only
            # frames cannot hide expensive ticks in an all-frame percentile.
            phases.append('host_input_ms')
            for phase in MULTIPLAYER_TICK_PHASES:
                calls = f'host_{phase}_calls'
                if all(row.get(calls) not in (None, '') for row in group) and any(int(row[calls]) for row in group):
                    phases.extend((f'host_{phase}_ms', calls))
        results[ticks] = {'frames': len(group), **summarize(group, ('interval_ms',))['interval_ms'],
                          **host_timings(group, phases)}
    return results


def desktop_screensaver_active():
    try:
        status = subprocess.run(['gdbus', 'call', '--session', '--dest', 'org.freedesktop.ScreenSaver',
                                 '--object-path', '/org/freedesktop/ScreenSaver',
                                 '--method', 'org.freedesktop.ScreenSaver.GetActive'],
                                capture_output=True, text=True, timeout=1, check=False)
    except (OSError, subprocess.TimeoutExpired):
        return None
    if status.returncode == 0:
        return {'(true,)': True, '(false,)': False}.get(status.stdout.strip())
    return None


def observe_route(log, event_ticks, required_dialogue):
    delivered, dialogue, complete = [], [], None
    with log.open(errors='replace') as stream:
        for raw in stream:
            line = raw.rstrip()
            if line.startswith('Script: '):
                dialogue.append(line.removeprefix('Script: '))
            if match := re.fullmatch(r'Campaign live input: input_tick=(\d+) tick=(\d+) position=(\S+)', line):
                delivered.append(dict(input_tick=int(match[1]), tick=int(match[2]),
                                      position=match[3]))
            if match := re.fullmatch(
                    r'Campaign live input complete: steps=(\d+) tick=(\d+) frames=(\d+) level_finished=([01])', line):
                complete = dict(steps=int(match[1]), tick=int(match[2]), frames=int(match[3]),
                                level_finished=match[4] == '1')
    steps = complete['steps'] if complete else 0
    expected = [tick for tick in event_ticks if tick < steps]
    milestones = {text: next((line for line in dialogue if text in line), None)
                  for text in required_dialogue}
    return dict(completion=complete, delivered_events=delivered, milestones=milestones,
                required_events_match=complete is not None and [row['input_tick'] for row in delivered] == expected)


def route_interval_tail(cpu, gpu, indices, limit=12):
    current_phases = ('host_common_ms', 'host_common_thread_cpu_ms', 'host_pre_render_ms',
                      'host_view0_camera_lighting_ms', 'host_texture_camera_prepare_ms')
    previous_phases = ('renderer_ms', 'fence_ms', 'acquire_ms', 'submit_ms', 'present_ms',
                       'commands_ms', 'geometry_plan_ms', 'geometry_bind_ms')
    tail = []
    for index in heapq.nlargest(limit, indices, key=lambda i: float(cpu[i]['interval_ms'])):
        row, previous = cpu[index], cpu[index-1]
        # The interval ends at this draw's start. The preceding draw's
        # waits belong to this interval, as do this frame's host preparation.
        tail.append(dict(frame=int(row['frame']), interval_ms=float(row['interval_ms']),
                         tick_begin=int(row['host_tick_begin']), tick_end=int(row['host_tick_end']),
                         host={key: float(row[key]) for key in current_phases if row.get(key) not in (None, '')},
                         preceding_draw={key: float(previous[key]) for key in previous_phases},
                         preceding_gpu={key: float(gpu[index-1][key]) for key in GPU_METRICS}))
    return tail


def uprising_measurement_window(route, ticks, rewinds, yaw):
    completed = route['completion']
    events = {event['input_tick']: event for event in route['delivered_events']}
    first, start, end = (events[step] for step in (1040, 1100, 1340))
    offset = completed['steps'] - 1 - completed['tick']
    if (completed['steps'] != UPRISING_ROUTE_STEPS or len(rewinds) != 1 or
            ticks[rewinds[0]] >= first['tick'] or ticks[rewinds[0] - 1] <= end['tick'] or
            any(event['input_tick'] - event['tick'] != offset for event in events.values())):
        raise ValueError('Uprising timing did not follow the authored checkpoint rewind and encounter timeline')
    before, after = ([float(value) for value in event['position'].split(',')] for event in (start, end))
    if len(before) != 3 or len(after) != 3 or not all(math.isfinite(value) for value in before + after):
        raise ValueError('Invalid Uprising Controls positions in the encounter')
    route['encounter_advance'] = (after[0] - before[0]) * math.cos(yaw) + (after[1] - before[1]) * math.sin(yaw)
    if route['encounter_advance'] < 5:
        raise ValueError('Uprising Controls did not advance through the encounter trigger')
    route['checkpoint_rewind'] = {'frame': rewinds[0], 'before': ticks[rewinds[0] - 1], 'after': ticks[rewinds[0]]}
    return [(start['tick'], end['tick'])]


def route_measurement_rows(cpu, gpu, args, route, scenario, lighting=None):
    completed = route['completion']
    if not completed or not route['required_events_match'] or not all(route['milestones'].values()):
        raise ValueError(f'{scenario} did not complete its replay or observed dialogue requirements')
    if len(cpu) != completed['frames']:
        raise ValueError(f'{scenario} completion frame count differs from renderer timing')
    if len(cpu) <= args.warmup:
        raise ValueError(f'{scenario} stopped before it finished timing warmup')
    if any(row.get('host_timings_present') != '1' for row in cpu):
        raise ValueError(f'{scenario} route requires actual source ticks in every timing row')
    ticks = [int(row['host_tick_end']) for row in cpu]
    if ticks[-1] != completed['tick']:
        raise ValueError(f'{scenario} completion tick differs from renderer timing')
    rewinds = [index for index, tick in enumerate(ticks[1:], 1) if tick < ticks[index-1]]
    if scenario == 'uprising':
        windows = uprising_measurement_window(route, ticks, rewinds, args.uprising_yaw)
        first_index = max(args.warmup, rewinds[0] + 1)
    else:
        if rewinds:
            raise ValueError('Armory timing did not follow one continuous source timeline')
        windows = args.armory_route_window
        first_index = args.warmup
        if args.armory_require_completion:
            if not completed['level_finished']:
                raise ValueError('Armory replay did not finish the authored mission')
            windows = [*windows, (ticks[first_index - 1], ticks[-1])]
            route['full_route_after_warmup'] = True
    selected = []
    route['windows'] = []
    for start, end in windows:
        # Count intervals bounded by two actual draw starts in this tick window.
        indices = [index for index in range(first_index, len(cpu))
                   if start <= ticks[index-1] <= ticks[index] <= end]
        window = dict(start_tick=start, end_tick=end, reached=ticks[first_index-1] <= start and ticks[-1] >= end)
        route['windows'].append(window)
        if not window['reached'] or not indices:
            raise ValueError(f'{scenario} timing did not reach the complete source tick window {start}..{end}')
        window_cpu = [cpu[index] for index in indices]
        window.update(measure_intervals(window_cpu, args))
        window['host'] = host_timings(window_cpu)
        window['host'].update(native_view_timings(window_cpu))
        window['gpu'] = summarize([gpu[index] for index in indices], GPU_METRICS)
        window['interval_by_tick_count'] = intervals_by_tick_count(window_cpu)
        window['slowest_intervals'] = route_interval_tail(cpu, gpu, indices)
        if lighting is not None:
            window['light_phases'] = summarize_light_rows([lighting[index] for index in indices])
        selected.extend(indices)
    return sorted(set(selected))


def benchmark(args, scratch, role, runtime, overrides, scenario, repetition, map_path):
    directory = scratch / f'{role}-{scenario}-{repetition}'
    directory.mkdir()
    for name in ('config', 'data', 'state', 'cache'):
        (directory / name).mkdir()
    cpu_path, gpu_path, log = [directory / name for name in ('cpu.tsv', 'gpu.tsv', 'runtime.log')]
    mode, _ = SCENARIOS[scenario]
    light_path = directory / 'light-phases.tsv' if args.light_phases and mode == 'campaign' else None
    route = args.routes.get(scenario)
    command = [str(runtime), f'--{mode}', '--map', str(map_path), '--shared-maps', str(args.maps),
               '--graphics-mode', 'Original', '--mute',
               '--viewport', *map(str, args.viewport), '--render-size', *map(str, args.render_size)]
    if route:
        command += ['--campaign-live-input-script', str(route['tape']), str(route['steps'])]
    else:
        command += ['--frames', str(args.frames)]
    if mode == 'multiplayer':
        command += ['--local-players', '1', '--mp-starting-weapon', '5']
    environment = {key: value for key, value in os.environ.items() if not key.startswith('HALO2_') and
                   key not in ('LD_PRELOAD', 'VK_INSTANCE_LAYERS', 'XDG_ACTIVATION_TOKEN', 'DESKTOP_STARTUP_ID')}
    environment.update(
        TMPDIR=str(directory), TMP=str(directory), TEMP=str(directory),
        HALO2_TEST_SCRATCH=str(directory), HALO2_RENDER_TIMINGS=str(cpu_path),
        HALO2_RENDER_GPU_TIMINGS=str(gpu_path), HALO2_PIPELINE_CACHE_DIRECTORY=str(directory / 'vulkan-cache'),
        XDG_CONFIG_HOME=str(directory / 'config'), XDG_DATA_HOME=str(directory / 'data'),
        XDG_STATE_HOME=str(directory / 'state'), XDG_CACHE_HOME=str(directory / 'cache'),
        MESA_SHADER_CACHE_DIR=str(directory / 'mesa-cache'), __GL_SHADER_DISK_CACHE_PATH=str(directory / 'nvidia-cache'),
        SDL_VIDEODRIVER='x11' if args.display == 'xvfb' else args.display, SDL_AUDIODRIVER='dummy',
        SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN='1', SDL_WINDOW_ACTIVATE_WHEN_SHOWN='0',
        SDL_WINDOW_ACTIVATE_WHEN_RAISED='0', SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT='0x0000/0x0000')
    if args.display == 'xvfb':
        environment['HALO2_PSTENCIL_CAPTURE_PRESENT_MODE'] = 'immediate'
    environment.update(overrides)
    if light_path is not None:
        environment['HALO2_CAMPAIGN_LIGHT_TIMINGS'] = str(light_path)

    result = {'scenario': scenario, 'repetition': repetition, 'role': role, 'command': command,
              'load_before': os.getloadavg(), 'passed': False}
    if args.light_phases:
        result['light_phases'] = {'status': 'not_applicable' if light_path is None else 'unavailable'}
    if args.display != 'xvfb':
        result['desktop_screensaver_active_before'] = desktop_screensaver_active()
    started = time.monotonic()
    try:
        with log.open('wb') as output:
            process = subprocess.run(command, cwd=ROOT, env=environment, stdout=output,
                                     stderr=subprocess.STDOUT, timeout=args.timeout)
        if route:
            result['route'] = observe_route(log, route['event_ticks'], route['dialogue'])
        if process.returncode:
            raise ValueError(f'Runtime exited {process.returncode}')
        device = None
        display_modes = []
        gpu_prefix = 'Multiplayer GPU: ' if mode == 'multiplayer' else 'Vulkan device: '
        with log.open(errors='replace') as stream:
            for line in stream:
                if device is None and line.startswith(gpu_prefix):
                    device = line.removeprefix(gpu_prefix).partition(' split_views=')[0].strip()
                if line.startswith('Campaign timing display: '):
                    mode = json.loads(line.removeprefix('Campaign timing display: '))
                    frame = mode.pop('frame')
                    if not display_modes or mode != display_modes[-1]['mode']:
                        display_modes.append({'frame': frame, 'mode': mode})
        if not re.search(args.gpu_pattern, device or ''):
            raise ValueError(f'Expected Vulkan GPU {args.gpu_pattern!r}, got {device!r}')
        result['gpu_device'] = device
        if display_modes:
            result['sdl_display_modes'] = display_modes
        cpu_columns = ('frame', 'host_timings_present', 'host_frame', 'host_tick_begin', 'host_tick_end',
                       'render_width', 'render_height', 'present_width', 'present_height', *CPU_METRICS, *HOST_METRICS,
                       *NATIVE_HOST_METRICS)
        cpu = samples(cpu_path, None if route else args.frames, cpu_columns if route else None)
        gpu = samples(gpu_path, len(cpu))
        dimensions(cpu, ('render_width', 'render_height'), tuple(args.render_size))
        dimensions(cpu, ('present_width', 'present_height'), tuple(args.viewport))
        dimensions(gpu, ('render_width', 'render_height', 'views'), (*args.render_size, 1))
        for row in gpu:
            total, *phases = [float(row[key]) for key in GPU_METRICS]
            if not math.isclose(total, sum(phases), rel_tol=1e-8, abs_tol=1e-6):
                raise ValueError('GPU timestamps do not compose')
        lighting = None
        if light_path is not None:
            result['light_phases'], lighting = light_phase_rows(light_path, cpu, tuple(args.viewport))
        indices = (route_measurement_rows(cpu, gpu, args, result['route'], scenario, lighting) if route else
                   range(args.warmup, len(cpu)))
        if lighting is not None:
            result['light_phases']['measurement'] = summarize_light_rows([lighting[index] for index in indices])
        cpu_rows = [cpu[index] for index in indices]
        if route and result['route'].get('full_route_after_warmup'):
            result['route']['frame_budget_by_source_tick'] = full_route_budget_bins(cpu_rows, args.max_p95_ms)
        cpu = summarize(cpu_rows, CPU_METRICS)
        host_rows = [row for row in cpu_rows if row.get('host_timings_present') == '1']
        if host_rows:
            result['host'] = host_timings(host_rows)
            result['host'].update(native_view_timings(host_rows))
            result['host_samples'] = len(host_rows)
            by_tick_count = intervals_by_tick_count(host_rows, multiplayer=scenario == 'multiplayer')
            result['interval_by_tick_count'] = by_tick_count
            result['host_ticks_advanced'] = sum(ticks * group['frames'] for ticks, group in by_tick_count.items())
        gpu = summarize([gpu[index] for index in indices], GPU_METRICS)
        measurement = measure_intervals(cpu_rows, args)
        gates = result['route']['windows'] if route else [measurement]
        result.update(cpu=cpu, gpu=gpu, sampled_frames=len(cpu_rows), fps=measurement['fps'])
        for gate in ('passes_fps', 'passes_p95', 'passes_p99', 'passed'):
            result[gate] = all(window[gate] for window in gates)
        print(f"{role} {scenario} #{repetition}: {result['fps']:.2f} FPS, "
              f"{cpu['interval_ms']['p95']:.3f} ms p95, {cpu['interval_ms']['p99']:.3f} ms p99", flush=True)
    except (OSError, ValueError, KeyError, ZeroDivisionError, subprocess.TimeoutExpired) as error:
        result['error'] = str(error)
        if log.exists():
            if route and 'route' not in result:
                result['route'] = observe_route(log, route['event_ticks'], route['dialogue'])
            with log.open('rb') as stream:
                stream.seek(max(0, log.stat().st_size - 6000))
                result['log_tail'] = stream.read().decode(errors='replace')
        print(f'{role} {scenario} #{repetition}: {error}', flush=True)
    result.update(elapsed_seconds=time.monotonic() - started, load_after=os.getloadavg())
    if args.display != 'xvfb':
        result['desktop_screensaver_active_after'] = desktop_screensaver_active()
    return result


def prepare_route(receipt_path, tape_sha256, map_sha256, tape_path, steps, dialogue):
    receipt_bytes = receipt_path.read_bytes()
    receipt = json.loads(receipt_bytes)
    events = receipt['input_schedule']
    payload = (json.dumps(events, indent=2) + '\n').encode()
    if (hashlib.sha256(payload).hexdigest() != tape_sha256 or receipt['input_sha256'] != tape_sha256 or
            receipt['map_sha256'] != map_sha256):
        raise ValueError('Retained Controls or map SHA-256 mismatch')
    tape_path.write_bytes(payload)
    route = dict(tape=tape_path, steps=steps, event_ticks=sorted(event['tick'] for event in events), dialogue=dialogue)
    summary = dict(receipt=str(receipt_path.resolve()), receipt_sha256=hashlib.sha256(receipt_bytes).hexdigest(),
                   tape_sha256=tape_sha256, ticks=steps, required_dialogue=dialogue)
    return route, summary, events


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, help='Default is the launcher selection')
    parser.add_argument('--candidate', type=Path, help='Interleave baseline and candidate in alternate order')
    parser.add_argument('--reference-env', metavar='NAME=VALUE', action='append', default=[],
                        help='Set a HALO2_REFERENCE_* variable for the baseline. Without --candidate, compare the same executable.')
    parser.add_argument('--maps', type=Path, help='Default is the launcher-selected prepared MCC map directory')
    parser.add_argument('--scenario', choices=SCENARIOS, action='append')
    parser.add_argument('--armory-repetitions', type=int, default=3)
    parser.add_argument('--repetitions', type=int, default=1)
    parser.add_argument('--frames', type=int, default=180)
    parser.add_argument('--warmup', type=int, default=60)
    parser.add_argument('--viewport', nargs=2, type=int, default=(1280, 720))
    parser.add_argument('--render-size', nargs=2, type=int, default=RENDER)
    parser.add_argument('--display', choices=('xvfb', 'wayland', 'x11'), default='xvfb')
    parser.add_argument('--min-fps', type=float, default=60)
    parser.add_argument('--max-p95-ms', type=float, default=16.7)
    parser.add_argument('--max-p99-ms', type=float, help='Also gate on the observed frame-interval p99')
    parser.add_argument('--timeout', type=float, default=240)
    parser.add_argument('--gpu-pattern', default=r'RTX 5080')
    parser.add_argument('--armory-route-report', type=Path, help='Replay the pinned ordinary Controls from a retained Armory route report')
    parser.add_argument('--armory-route-ticks', type=int, help='Stop after this many live route ticks, default 12000')
    parser.add_argument('--armory-route-window', type=int, nargs=2, metavar=('BEGIN', 'END'), action='append', default=[],
                        help='Measure and gate this inclusive source tick window, repeatable')
    parser.add_argument('--armory-route-require', metavar='DIALOGUE', action='append', default=[],
                        help='Require this substring in actually executed authored dialogue, repeatable')
    parser.add_argument('--armory-require-completion', action='store_true',
                        help='Require authored Armory completion and also gate all frames after warmup')
    parser.add_argument('--uprising-route-report', type=Path,
                        help='Replay the pinned Uprising Controls through the first post-intro encounter')
    parser.add_argument('--light-phases', action='store_true', help='Collect diagnostic campaign lighting phase timings')
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    if not 0 < args.warmup < args.frames or min(args.armory_repetitions, args.repetitions) < 1:
        parser.error('Require frames greater than positive warmup and positive repetitions')
    if any(not 64 <= size <= 4096 for size in (*args.viewport, *args.render_size)):
        parser.error('Viewport and render dimensions must be 64..4096')
    budgets = (args.timeout, args.min_fps, args.max_p95_ms, args.max_p99_ms)
    if any(not math.isfinite(value) or value <= 0 for value in budgets if value is not None):
        parser.error('Timeout and performance budgets must be finite positive values')
    if args.armory_route_report:
        if args.armory_route_ticks is None:
            args.armory_route_ticks = 12000
        if ((args.scenario and 'armory' not in args.scenario) or not args.armory_route_window or
                not args.armory_route_require or not all(text.strip() for text in args.armory_route_require) or
                not all(0 <= start < end <= args.armory_route_ticks <= 12000
                        for start, end in args.armory_route_window)):
            parser.error('An Armory route requires an Armory run, inclusive tick windows within 0..12000 and authored dialogue')
    elif (args.armory_route_ticks is not None or args.armory_route_window or args.armory_route_require or
          args.armory_require_completion):
        parser.error('Armory route settings require --armory-route-report')
    if args.uprising_route_report and (not args.scenario or 'uprising' not in args.scenario):
        parser.error('An Uprising route requires --scenario uprising')
    try:
        re.compile(args.gpu_pattern)
    except re.error as error:
        parser.error(f'Invalid GPU device pattern: {error}')
    reference_env = {}
    for entry in args.reference_env:
        name, separator, value = entry.partition('=')
        if (not separator or not re.fullmatch(r'HALO2_REFERENCE_[A-Z0-9_]+', name) or
                not value or name in reference_env):
            parser.error(f'Use a distinct HALO2_REFERENCE_* name and nonempty value: {entry!r}')
        reference_env[name] = value
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.report_output.exists():
        parser.error(f'Report already exists: {args.report_output}')
    if args.display == 'xvfb' and not os.environ.get('HALO2_PERF_PRIVATE_XVFB'):
        environment = os.environ.copy()
        environment.pop('DISPLAY', None)
        environment.pop('XAUTHORITY', None)
        environment['HALO2_PERF_PRIVATE_XVFB'] = '1'
        width, height = args.viewport
        command = ['xvfb-run', '-a', '-e', str(scratch / 'xvfb.log'), '-s',
                   f'-screen 0 {width + 64}x{height + 64}x24 -nolisten tcp',
                   sys.executable, str(Path(__file__).resolve()), *sys.argv[1:]]
        os.execvpe(command[0], command, environment)

    scratch = Path(tempfile.mkdtemp(prefix='campaign-4k-', dir=scratch))
    selection_path = ROOT / 'gameplay-runtime-selection.json'
    selection = json.loads(selection_path.read_text())
    runtime = (args.runtime or ROOT / selection['runtime']).resolve(strict=True)
    args.maps = (args.maps or ROOT / selection['cache'] / 'halo2/h2_maps_win64_dx11').resolve(strict=True)
    runtimes = [('baseline', runtime, reference_env)]
    if args.candidate or reference_env:
        runtimes.append(('candidate', args.candidate.resolve(strict=True) if args.candidate else runtime, {}))
    runtime_pins = {path: sha256(path) for path in dict.fromkeys(path for _, path, _ in runtimes)}
    runtime_info = {role: {'path': str(path), 'sha256': runtime_pins[path], 'environment': overrides}
                    for role, path, overrides in runtimes}
    if not args.runtime and runtime_info['baseline']['sha256'] != selection['runtime_sha256']:
        parser.error('Launcher runtime differs from its selected hash')
    scenarios = list(dict.fromkeys(args.scenario or ('armory', 'cairo', 'multiplayer')))
    maps = {scenario: args.maps / f'{SCENARIOS[scenario][1]}.map' for scenario in scenarios}
    report = {'render': args.render_size, 'viewport': args.viewport, 'graphics': 'Original', 'display': args.display,
              'presentation': 'immediate diagnostic' if args.display == 'xvfb' else 'ordinary desktop FIFO',
              'diagnostic': args.light_phases or args.display == 'xvfb' or tuple(args.viewport) != tuple(args.render_size)
                            or bool(reference_env),
              'benchmark_tool_sha256': sha256(Path(__file__).resolve()),
              'selection_sha256': sha256(selection_path), 'runtimes': runtime_info, 'scratch': str(scratch),
              'maps': {scene: {'path': str(path), 'sha256': sha256(path)} for scene, path in maps.items()},
              'min_fps': args.min_fps, 'max_p95_ms': args.max_p95_ms, 'max_p99_ms': args.max_p99_ms, 'runs': []}
    if args.light_phases:
        report['light_phase_capture'] = {'projected_material_nested_in': 'projected_packets'}
    args.routes = {}
    for scenario, receipt, tape_hash, steps, dialogue in (
            ('armory', args.armory_route_report, ARMORY_ROUTE_TAPE_SHA256, args.armory_route_ticks, args.armory_route_require),
            ('uprising', args.uprising_route_report, UPRISING_ROUTE_TAPE_SHA256, UPRISING_ROUTE_STEPS,
             ['e1_main', 'new objective set: defend yourself from the brutes'])):
        if receipt is None:
            continue
        try:
            route, summary, events = prepare_route(receipt, tape_hash, report['maps'][scenario]['sha256'],
                                                   scratch / f'{scenario}-controls.json', steps, dialogue)
        except (OSError, KeyError, TypeError, ValueError) as error:
            parser.error(f'Invalid {scenario} route receipt: {error}')
        if scenario == 'armory':
            summary['windows'] = args.armory_route_window
            summary['require_completion'] = args.armory_require_completion
        else:
            args.uprising_yaw = events[0]['yaw']
            summary['window_inputs'] = [1100, 1340]
        args.routes[scenario] = route
        report[f'{scenario}_route'] = summary
    for scenario in scenarios:
        repetitions = args.armory_repetitions if scenario == 'armory' else args.repetitions
        for repeat in range(1, repetitions + 1):
            order = runtimes if repeat % 2 else reversed(runtimes)
            for role, path, overrides in order:
                report['runs'].append(benchmark(args, scratch, role, path, overrides, scenario, repeat, maps[scenario]))
    judged = 'candidate' if len(runtimes) > 1 else 'baseline'
    report['passed'] = all(run['passed'] for run in report['runs'] if run['role'] == judged)
    report['judged_role'] = judged
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    with args.report_output.open('x') as stream:
        json.dump(report, stream, indent=2, allow_nan=False)
        stream.write('\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
