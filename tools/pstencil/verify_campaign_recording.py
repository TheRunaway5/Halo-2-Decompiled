#!/usr/bin/env python3
"""Inspect an actual opening recording without altering its pixels or audio.

Nonzero audio and native completion are bounded checks. They do not establish
dialogue balance, synchronization, original presentation parity, or smooth
native frame pacing. Retain the uncut recording alongside this receipt.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import subprocess

import numpy as np


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    directory = args.directory.resolve()
    output = directory / 'recording-verification.json'
    if output.exists():
        raise RuntimeError('Refusing to replace retained verification')
    video = directory / 'opening-uncut.mkv'
    capture = json.loads((directory / 'result.json').read_text())
    media = json.loads(subprocess.check_output([
        'ffprobe', '-v', 'error', '-show_streams', '-show_format',
        '-of', 'json', str(video)], text=True))
    command = ['ffmpeg', '-v', 'error', '-nostdin', '-i', str(video),
               '-map', '0:a:0', '-ac', '2', '-ar', '48000', '-f', 'f32le', '-']
    total = nonzero = active = seconds = 0
    peak = square_sum = 0.0
    windows = []
    with subprocess.Popen(command, stdout=subprocess.PIPE) as decoder:
        while raw := decoder.stdout.read(48000 * 2 * 4):
            samples = np.frombuffer(raw, dtype='<f4').astype(np.float64)
            if not np.isfinite(samples).all():
                raise RuntimeError('Recorded audio contains nonfinite samples')
            count = int(np.count_nonzero(samples))
            maximum = float(np.max(np.abs(samples)))
            total += len(samples)
            nonzero += count
            square_sum += float(np.dot(samples, samples))
            peak = max(peak, maximum)
            active += maximum > 1e-5
            windows.append(dict(second=seconds, samples=len(samples),
                                nonzero_samples=count, peak=maximum))
            seconds += 1
        if decoder.wait():
            raise RuntimeError('Cannot decode the recorded audio')
    log = (directory / 'native.log').read_text(errors='replace')
    states = [json.loads(line) for line in
              (directory / 'frames.jsonl').read_text().splitlines() if line.strip()]
    with (directory / 'renderer-timings.tsv').open() as stream:
        timings = list(csv.DictReader(stream, delimiter='\t'))
    pacing = {}
    for key in ('interval_ms', 'renderer_ms', 'pack_ms', 'present_ms',
                'geometry_plan_ms', 'geometry_bind_ms'):
        values = [float(row[key]) for row in timings if row.get(key) is not None]
        if key == 'interval_ms':
            values = values[1:]  # The first sample has no previous frame.
        if values:
            pacing[key] = dict(median=float(np.median(values)),
                               mean=float(np.mean(values)),
                               p95=float(np.percentile(values, 95)), max=max(values))
    faults = [int(value) for value in re.findall(r'\bscript_faults=(\d+)', log)]
    result = dict(
        scope=__doc__, capture=capture, video_sha256=digest(video), media=media,
        native=dict(trace_frames=len(states), timing_frames=len(timings),
                    last_trace=states[-1] if states else None,
                    reported_script_faults=max(faults) if faults else None,
                    script_fault_messages=len(re.findall(r'\bHS:', log)),
                    vulkan_validation_messages=log.count('VUID-')),
        audio=dict(samples=total, nonzero_samples=nonzero, peak=peak,
                   rms=(square_sum / total) ** .5 if total else 0,
                   active_one_second_windows=active, windows=windows),
        pacing=pacing, full_presentation_verified=False)
    output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(receipt=str(output), native=result['native'],
                         audio={k: v for k, v in result['audio'].items() if k != 'windows'},
                         pacing=pacing), indent=2))
    if capture.get('native_status') != 0 or not total or not nonzero or \
       result['native']['reported_script_faults'] != 0 or \
       result['native']['script_fault_messages'] or result['native']['vulkan_validation_messages']:
        raise SystemExit('The retained recording did not pass the bounded completion/audio checks')


if __name__ == '__main__':
    main()
