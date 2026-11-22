#!/usr/bin/env python3
"""Capture native audio scheduling and callback receipts on the authored Armory route."""

import argparse
from collections import defaultdict
import json
import os
from pathlib import Path
import re
import subprocess

from physics_test_cache import runtime_cache, runtime_map
from test_campaign_armory_cinematic import CONTROLS, CONTROLS_SHA256
from test_campaign_new_game_runtime import ARMORY, parse_campaign_lines, sha
from validate_weapon_runtime_tu15 import private_display


ROOT = Path(__file__).resolve().parents[1]
WAYPOINT_TICKS = 12000
ARMORY_SHA256 = '37022bb3b9afa554a8566a065111fcf095a5b23f073b8922b935d1ceb47b4af1'
# Original and MCC scenario syntax both use these opening and tram sequences.
# These are the salted tag datums in the authenticated prepared MCC Armory.
OPENING = (3877111202, 3877176739, 3877242276, 3877307813, 3877373350)
TRAM = (3879142849, 3879470534, 3879536071, 3879601608, 3879667145, 3879732682, 3879798219)
MUSIC = 3879208386
MUSIC_PATH = r'scenarios\solo\01a_tutorial\01a_music\01a_01'
AMBIENT_PCM = 3880191441
FRAMES_PER_TICK = 1600
TRACE_LIMIT = 4096
TRACE_ROWS = {
    'Script audio packet: ': 'packets',
    'Script voice: ': 'voice_positions',
    'Script audio stop: ': 'explicit_stops',
    'Script audio retired: ': 'retirements',
    'Script audio loop: ': 'loop_events',
    'Script audio ambience: ': 'ambient_events',
    'Script audio clock: ': 'script_clocks',
}


def fields(text):
    return {
        key: int(value) if value.lstrip('-').isdigit() else value
        for key, value in re.findall(r'(\w+)=([^ ]+)', text)
    }


def observe(lines):
    source = parse_campaign_lines(lines)
    rows = defaultdict(list)
    for line_no, line in enumerate(lines, 1):
        for prefix, name in TRACE_ROWS.items():
            if line.startswith(prefix):
                rows[name].append(dict(line=line_no, **fields(line[len(prefix):])))
                break
        else:
            if line.startswith(('Looping sound: ', 'Script: ', 'Script call: ', 'SDL audio driver: ',
                                'Audio mixed_frames=', 'Authored scripted audio mixed_frames=',
                                'Rendered ', 'Campaign live input complete: ')):
                rows['markers'].append(dict(line=line_no, text=line))
            if line.startswith(('Audio fault: ', 'Audio layout: ', 'Scripted audio layout: ',
                                'Audio unavailable: ', 'Campaign ambience: ',
                                'Script audio retirement observation unavailable', 'error: ')):
                rows['errors'].append(dict(line=line_no, text=line[:1000]))
    ending = next((row for row in reversed(source['source_status'])
                   if row['scenario'] == ARMORY and row['level_finished']), None)
    excess = {name: len(items) for name, items in rows.items() if len(items) > TRACE_LIMIT}
    bounded = {name: items[:TRACE_LIMIT] for name, items in rows.items()}
    return dict(**source, **bounded, armory_ending=ending, trace_limit_exceeded=excess)


def identity(row):
    return row['sound'], row['owner'], row['voice']


def cue_sequence(report, sounds):
    selected = [row for row in report.get('packets', []) if row['sound'] in sounds]
    ordered = tuple(row['sound'] for row in selected) == sounds
    return selected if ordered else []


def check_cues(report, cues):
    positions, retirements = defaultdict(list), defaultdict(list)
    for row in report.get('voice_positions', []):
        positions[identity(row)].append(row)
    for row in report.get('retirements', []):
        retirements[identity(row)].append(row)
    milestones = []
    for packet in cues:
        first = next(iter(positions[identity(packet)]), {})
        retired = next(iter(retirements[identity(packet)]), {})
        first_frame = first.get('first_device_frame')
        first_presentation = retired.get('first_presentation_frame')
        volume = first.get('volume_hundredths_db')
        onset_ticks = first.get('tick', -1) - packet['tick']
        milestones.append(dict(
            sound=packet['sound'], tick=packet['tick'], owner=packet['owner'],
            onset_ticks=onset_ticks, first_device_frame=first_frame,
            first_presentation_frame=first_presentation,
            volume_hundredths_db=volume,
            source_frames=retired.get('source_frames'), output_frames=retired.get('output_frames'),
            packet_frames=packet['packet_frames'],
            complete=(packet['packet_frames'] > 0 and 0 <= onset_ticks <= 4
                      and isinstance(volume, int) and volume > -10000
                      and isinstance(first_frame, int) and first_frame == retired.get('first_device_frame')
                      and isinstance(first_presentation, int)
                      and retired.get('accounted') == 1 and retired.get('status') == 0
                      and retired.get('packet_frames') == packet['packet_frames']
                      and float(retired.get('source_frames', -1)) == packet['packet_frames']
                      and retired.get('output_frames', 0) > 0),
        ))
    return milestones


def qualifies_voice_group(cues):
    return (bool(cues) and all(row['complete'] and row['owner'] != 0xffffffff for row in cues)
            and len({row['owner'] for row in cues}) == 1
            and all(a['first_presentation_frame'] < b['first_presentation_frame']
                    for a, b in zip(cues, cues[1:])))


def selected_clock(report, packet):
    operands = ('tick', 'sound', 'pitch_range', 'permutation')
    return next((row for row in report.get('script_clocks', [])
                 if all(row.get(key) == packet[key] for key in operands)), {})


def impulse_playback(report):
    retirements = {identity(row): row for row in report.get('retirements', [])}
    stops = {(row['sound'], row['voice']): row for row in report.get('explicit_stops', [])}
    packets = report.get('packets', [])
    consumed = 0
    truncated, unretired = [], []
    for packet in packets:
        retirement = retirements.get(identity(packet))
        if retirement is None:
            unretired.append(dict(tick=packet['tick'], sound=packet['sound'], voice=packet['voice']))
            continue
        if retirement.get('output_frames', 0) > 0 and isinstance(retirement.get('first_device_frame'), int):
            consumed += 1
        if float(retirement.get('source_frames', 0)) < packet['packet_frames']:
            stop = stops.get((packet['sound'], packet['voice']), {})
            truncated.append(dict(tick=packet['tick'], sound=packet['sound'], voice=packet['voice'],
                                  packet_frames=packet['packet_frames'], source_frames=retirement.get('source_frames'),
                                  status=retirement.get('status'), stop_tick=stop.get('tick'),
                                  reason='authored impulse stop' if stop else 'unattributed audio retirement'))
    return dict(packets=len(packets), consumed=consumed, truncated=truncated, unretired=unretired)


def qualification(report):
    opening, tram = (cue_sequence(report, sounds) for sounds in (OPENING, TRAM))
    opening_cues, tram_cues = (check_cues(report, cues) for cues in (opening, tram))
    source = report.get('source_status', [])
    phase_entries = [fields(row['text'].removeprefix('Script call: '))
                     for row in report.get('markers', []) if row['text'].startswith('Script call: ')]
    look_entries = [row['tick'] for row in phase_entries if row['name'] == 'training_look']
    tram_entries = [row['tick'] for row in phase_entries if row['name'] == 'training_tram']
    timing_deltas = [second['tick'] - first['tick'] - first['packet_frames'] // FRAMES_PER_TICK
                     for first, second in zip(opening, opening[1:])]
    opening_clocks = [selected_clock(report, row).get('remaining_ticks') for row in opening[:-1]]
    clock_deltas = [second['tick'] - first['tick'] - duration
                    for first, second, duration in zip(opening, opening[1:], opening_clocks)
                    if duration is not None]
    ambient = [row for row in report.get('ambient_events', []) if row.get('sound') == AMBIENT_PCM
               and row.get('bsp') == 0 and row.get('tick', 99) < 28 and row.get('packet_frames', 0) > 0]
    ambient_ids = {identity(row) for row in ambient}
    ambient_consumed = [row for row in report.get('retirements', [])
                        if identity(row) in ambient_ids and row.get('loop') == 1
                        and isinstance(row.get('first_device_frame'), int)
                        and row.get('output_frames', 0) > 0 and float(row.get('source_frames', -1)) > 0]
    legacy_music = [row for row in report.get('markers', [])
                    if row['text'] == f'Looping sound: {MUSIC_PATH} tracks=1']
    music = [row for row in report.get('loop_events', []) if row.get('sound') == MUSIC]
    music_ordered = [row.get('action') for row in music] == ['start', 'stop']
    # training_tram starts the score 30 ticks before l01_0010_jon's language
    # duration elapses and stops it after the full l01_0050_jon duration.
    pcm_music_start = tram[0]['tick'] + max(0, tram[0]['packet_frames'] // FRAMES_PER_TICK - 30) if tram else None
    pcm_music_stop = tram[-2]['tick'] + tram[-2]['packet_frames'] // FRAMES_PER_TICK if tram else None
    music_clock_start = selected_clock(report, tram[0]).get('remaining_ticks') if tram else None
    music_clock_stop = selected_clock(report, tram[-2]).get('remaining_ticks') if tram else None
    music_start = tram[0]['tick'] + max(0, music_clock_start - 30) if music_clock_start is not None else None
    music_stop = tram[-2]['tick'] + music_clock_stop if music_clock_stop is not None else None
    terminal_log = [row['text'] for row in report.get('markers', [])] + report.get('log_tail', [])
    completion = next((match for line in reversed(terminal_log)
                       if (match := re.fullmatch(r'Campaign live input complete: steps=(\d+) tick=(\d+) '
                                                 r'frames=(\d+) level_finished=1', line))), None)
    rendered = next((int(match[1]) for line in terminal_log
                     if (match := re.fullmatch(r'Rendered (\d+) frames', line))), None)
    finished_at = (report.get('armory_ending') or {}).get('tick')
    impulses = impulse_playback(report)
    checks = {
        'full_armory_route': bool(report.get('completed_probe') and len(report.get('initial_scenarios', [])) == 1
                                 and report['initial_scenarios'][0]['scenario'] == ARMORY
                                 and not report.get('natural_next_scenarios') and completion
                                 and int(completion[1]) == int(completion[2]) == finished_at
                                 and int(completion[3]) > 0
                                 and (rendered is None or rendered == int(completion[3]))),
        'no_content_or_audio_faults': not (report.get('errors') or report.get('script_faults')
                                           or report.get('trace_limit_exceeded')
                                           or any(report.get('content_error_counts', []))
                                           or any(row['script_faults'] for row in source)),
        'opening_voice_order_and_callback_playback': qualifies_voice_group(opening_cues),
        'opening_authored_inter_cue_timing': (bool(opening) and look_entries == [opening[0]['tick']]
                                             and clock_deltas == [15, 0, 30, 0]),
        'tram_voice_order_and_callback_playback': qualifies_voice_group(tram_cues),
        'tram_authored_voice_spacing': (bool(tram) and len(tram_entries) == 1 and tram_entries[0] <= tram[0]['tick']
                                        and all(second['tick'] - first['tick']
                                                >= first['packet_frames'] // FRAMES_PER_TICK
                                                for first, second in zip(tram, tram[1:]))),
        'tram_score_authored_start_and_stop': (music_ordered and bool(tram)
                                              and music_clock_start == 96 and music_clock_stop == 57
                                              and music[0]['tick'] == music_start and music[1]['tick'] == music_stop
                                              and tram[0]['line'] < music[0]['line'] < tram[1]['line']
                                              and tram[-2]['line'] < music[1]['line'] < tram[-1]['line']),
        'tram_score_callback_playback': (music_ordered and music[0].get('tracks') == 1
                                        and float(music[1].get('source_frames', -1)) > 0),
        'armory_ambient_callback_playback': bool(ambient_consumed),
    }
    return dict(checks=checks, passed=all(checks.values()), training_dialogue=opening_cues,
                training_extra_wait_ticks_pcm_control=timing_deltas, training_extra_wait_ticks_native=clock_deltas,
                tram_dialogue=tram_cues, music_trace=music, music_expected_ticks=[music_start, music_stop],
                music_expected_ticks_pcm_control=[pcm_music_start, pcm_music_stop], impulse_playback=impulses,
                rendered_frames=int(completion[3]) if completion else None,
                legacy_music_start=legacy_music, ambient_voice_consumption=ambient_consumed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path)
    parser.add_argument('--mcc-root', type=Path)
    parser.add_argument('--input-report', type=Path, help='Check a previously collected runtime report')
    parser.add_argument('--report-output', type=Path, required=True)
    parser.add_argument('--graphics-mode', default='Original', choices=('Original', 'pStencil'))
    parser.add_argument('--timeout', type=int, default=540)
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Run through tools/run_test.py')
    if args.input_report:
        if args.runtime or args.mcc_root:
            parser.error('Use --input-report or --runtime and --mcc-root')
        report = json.loads(args.input_report.read_text())
        return write_report(report, args.report_output)
    if not args.runtime or not args.mcc_root:
        parser.error('Require --runtime and --mcc-root for a live replay')
    if not 1 <= args.timeout <= 1200 or sha(CONTROLS) != CONTROLS_SHA256:
        parser.error('Require the pinned Armory controls and a bounded timeout of 1..1200 seconds')
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    runtime = args.runtime.resolve(strict=True)
    maps, _ = runtime_cache(args.mcc_root)
    armory = runtime_map(maps, maps / '01a_tutorial.map')
    if sha(armory) != ARMORY_SHA256:
        parser.error('The authenticated Armory audio fixture changed')
    log = owner / 'runtime.log'
    environment = {key: value for key, value in os.environ.items() if not key.startswith('HALO2_')
                   and key not in ('DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY', 'VK_INSTANCE_LAYERS',
                                   'XDG_ACTIVATION_TOKEN')}
    settings = dict(HALO2_TEST_SCRATCH=str(owner), HALO2_PIPELINE_CACHE_DIRECTORY=str(owner / 'pipelines'),
                    HALO2_PSTENCIL_CAPTURE_PRESENT_MODE='immediate', HALO2_SCRIPT_CALL_TRACE='1',
                    HALO2_SCRIPT_AUDIO_RETIRE_TRACE='1', SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy')
    environment.update(settings)
    for key in ('XDG_CONFIG_HOME', 'XDG_DATA_HOME', 'XDG_STATE_HOME', 'XDG_CACHE_HOME'):
        environment[key] = str(owner / key.lower())
    command = ['stdbuf', '-oL', '-eL', str(runtime), '--campaign', '--map', str(armory), '--shared-maps',
               str(maps), '--hidden', '--graphics-mode', args.graphics_mode, '--viewport', '720', '405',
               '--render-size', '720', '405', '--campaign-live-input-script', str(CONTROLS), str(WAYPOINT_TICKS)]
    runtime_digest = sha(runtime)
    probe_digest = sha(__file__)
    print(f'Watching owned Armory audio trace {log}', flush=True)
    with private_display(owner) as display, log.open('wb') as output:
        environment['DISPLAY'] = display
        try:
            status = subprocess.run(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT,
                                    env=environment, timeout=args.timeout).returncode
        except subprocess.TimeoutExpired:
            status = 124
    lines = log.read_text(errors='replace').splitlines()
    report = observe(lines)
    report.update(runtime=str(runtime), runtime_sha256=runtime_digest, runtime_unchanged=sha(runtime) == runtime_digest,
                  command=command, environment=settings, controls_sha256=CONTROLS_SHA256, armory_sha256=ARMORY_SHA256,
                  probe_sha256=probe_digest,
                  exit_code=status, scratch=str(owner), retained_large_artifacts=[], log_line_count=len(lines),
                  log_tail=lines[-20:])
    report['completed_probe'] = bool(status == 0 and report['armory_ending'] and report.get('packets')
                                     and report.get('retirements'))
    return write_report(report, args.report_output)


def write_report(report, report_output):
    report['qualification'] = qualification(report)
    report_output.parent.mkdir(parents=True, exist_ok=True)
    report_output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'Wrote Armory audio receipt {report_output}', flush=True)
    return 0 if report['qualification']['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
