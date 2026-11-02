#!/usr/bin/env python3
"""Decode the original destruction sound inventory through a native engine.

Use tools/run_test.py. WAV files remain inside HALO2_TEST_SCRATCH; only bounded
receipts leave it. This checks default-pitch/permutation PCM, not gameplay cue
selection, device submission, spatial output or original waveform equivalence.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import wave

from mcc_content import MccCache
from weapon_test_cache import require_mcc_map


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', required=True, type=Path)
    parser.add_argument('--inventory', required=True, type=Path)
    parser.add_argument('--maps', required=True, type=Path)
    parser.add_argument('--report-output', required=True, type=Path)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.engine = args.engine.resolve(strict=True)
    args.maps = args.maps.resolve(strict=True)
    inventory = json.loads(args.inventory.read_text())
    if (not inventory.get('passed') or inventory.get('vehicle_paths') != 37 or
            inventory.get('dependency_roots') != 'vehicle model damage and section responses'):
        raise ValueError('Require the completed original vehicle-destruction inventory')
    sounds = sorted({sound for owner in inventory['owners'] for sound in owner['sounds']})
    if not sounds:
        raise ValueError('The original destruction inventory has no sound banks')
    engine_hash = sha(args.engine)
    inputs = {str(args.engine): engine_hash, str(args.inventory.resolve()): sha(args.inventory)}
    for name in ('sounds_en.dat','sounds_neutral.dat'):
        resource=args.maps/name
        inputs[str(resource)]=sha(resource)
    report = dict(passed=False, scope=__doc__, engine=str(args.engine), engine_sha256=engine_hash,
        inventory_sha256=sha(args.inventory), owned_scratch=str(scratch), cases=[],
        all_permutations_verified=False, live_audio_delivery_verified=False,
        original_waveform_equivalence_verified=False, retained_large_artifacts=[])
    args.report_output = args.report_output.resolve()
    if (args.report_output in (args.engine,args.inventory.resolve()) or
            args.report_output.is_relative_to(args.maps)):
        raise ValueError('The bounded report must not overwrite runtime/reference inputs')
    args.report_output.parent.mkdir(parents=True, exist_ok=True)

    def save():
        raw = json.dumps(report, indent=2) + '\n'
        if len(raw.encode()) > 1024 * 1024:
            raise ValueError('Destruction audio report exceeds its bounded budget')
        args.report_output.write_text(raw)

    save()
    try:
        # Resolve actual resident metadata; names alone in a scenario index
        # cannot identify a shared resource owner.
        owners = {}
        candidates = ['shared.map', 'single_player_shared.map']
        candidates += [Path(row['mcc_path']).name for row in inventory['maps']]
        for name in dict.fromkeys(candidates):
            path = require_mcc_map(args.maps / name)
            cache = MccCache(path)
            try:
                for tag in cache.tags.values():
                    if tag['group'] == 'snd!' and tag['name'] in sounds and tag['offset'] is not None:
                        owners.setdefault(tag['name'], (path, cache.u16(tag['offset'])))
            finally:
                cache.close()
            if len(owners) == len(sounds):
                break
        for index, sound in enumerate(sounds):
            row = dict(sound=sound, passed=False)
            report['cases'].append(row)
            try:
                if sound not in owners:
                    raise ValueError('Original sound has no resident MCC owner')
                path, flags = owners[sound]
                row['mcc_flags'] = flags
                if flags & 0x800:
                    raise ValueError('Original destruction bank is marked remastered-only')
                inputs.setdefault(str(path), sha(path))
                output = scratch / f'destruction-{index:02d}.wav'
                log = scratch / f'destruction-{index:02d}.log'
                command = [str(args.engine), '--map', str(path), '--shared-maps', str(args.maps),
                           '--decode-sound', sound, '--audio-output', str(output)]
                row.update(map=str(path), map_sha256=inputs[str(path)], command=command)
                with log.open('w') as stream:
                    result = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT,
                                            timeout=90, check=False).returncode
                row['returncode'] = result
                if result or not output.is_file():
                    raise ValueError('Native decoder failed: ' + log.read_text()[-4000:])
                with wave.open(str(output), 'rb') as decoded:
                    if (decoded.getnchannels(), decoded.getsampwidth(), decoded.getframerate()) != (2, 2, 48000):
                        raise ValueError('Native decoded output lost its stereo PCM format')
                    frames = decoded.getnframes()
                    peak = samples = 0
                    while data := decoded.readframes(4096):
                        for sample, in struct.iter_unpack('<h', data):
                            peak = max(peak, abs(sample))
                            samples += 1
                if not frames or samples != frames * 2 or not peak:
                    raise ValueError('Native destruction sound is empty, partial or silent')
                row.update(passed=True, classic_route_eligible=True, frames=frames, peak_s16=peak,
                           wav_sha256=sha(output), decoded_bytes=output.stat().st_size)
            except Exception as error:
                row['error'] = str(error)
            finally:
                # Remove each decoded bank promptly, including failed output.
                (scratch / f'destruction-{index:02d}.wav').unlink(missing_ok=True)
                (scratch / f'destruction-{index:02d}.log').unlink(missing_ok=True)
                save()
        report['inputs_unchanged'] = all(sha(path) == pin for path, pin in inputs.items())
        report['input_hashes'] = inputs
        report['passed'] = report['inputs_unchanged'] and all(row['passed'] for row in report['cases'])
    except Exception as error:
        report['error'] = str(error)
    finally:
        save()
    print(json.dumps(dict(passed=report['passed'], cases=len(report['cases']),
                          report=str(args.report_output))), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
