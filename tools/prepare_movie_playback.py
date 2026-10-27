#!/usr/bin/env python3
"""Prepare or stage an optional, pixel-exact HEVC intro playback cache.

Original bundled movies are never replaced. Preparation checks every decoded
YUV frame against the original before publishing the separate runtime asset.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = 'H2_LOSSLESS_MOVIE_PLAYBACK_V1'
VIDEO = 'intro.hevc.mp4'


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def verify(bank, source):
    manifest = bank / 'manifest.json'
    if manifest.is_symlink() or not manifest.is_file() or manifest.stat().st_size > 65536:
        raise ValueError('Invalid playback manifest owner/extent')
    row = json.loads(manifest.read_text())
    expected = json.loads((ROOT / 'resources/movie-manifest.json').read_text())['files']['intro_60.mp4']
    if (row.get('schema') != SCHEMA or row.get('role') != 'intro' or
            row.get('file') != VIDEO or row.get('source') != expected or
            row.get('all_yuv_frames_identical') is not True or
            row.get('copied_audio_packets') is not True or
            row.get('pixel_format') != 'yuv420p' or row.get('frame_rate') != '60000/1001'):
        raise ValueError('Playback receipt does not bind the bundled original intro')
    video = bank / VIDEO
    if (video.is_symlink() or source.stat().st_size != expected['bytes'] or
            sha(source) != expected['sha256'] or video.stat().st_size != row['bytes'] or
            sha(video) != row['sha256']):
        raise ValueError('Playback source or derived payload identity differs')
    return row


def probe(path):
    return json.loads(subprocess.check_output(
        ['ffprobe', '-v', 'error', '-show_streams', '-show_format', '-of', 'json', str(path)], text=True))


def frame_hashes(path, output):
    subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-threads', '0',
                    '-i', str(path), '-an', '-pix_fmt', 'yuv420p', '-f', 'framemd5', str(output)],
                   check=True, timeout=600)
    return [line.split(',')[-1].strip() for line in output.read_text().splitlines()
            if line and not line.startswith('#')]


def stage(bank, destination, source):
    row = verify(bank, source)
    # Ninja creates declared output directories before invoking the rule.
    # An empty directory is not a pre-existing bank and contains no owner to
    # preserve. Nonempty banks retain the strict identity/refusal behavior.
    empty_output = (destination.is_dir() and not destination.is_symlink()
                    and not any(destination.iterdir()))
    if destination.exists() and not empty_output:
        if verify(destination, source) == row:
            return
        raise ValueError('Preserve the existing playback bank; select a new output directory')
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='halo2-playback-stage-', dir=destination.parent) as temporary:
        staging = Path(temporary) / 'bank'
        staging.mkdir()
        shutil.copy2(bank / VIDEO, staging / VIDEO)
        shutil.copy2(bank / 'manifest.json', staging / 'manifest.json')
        verify(staging, source)
        if empty_output:
            # rmdir refuses a concurrent writer or replacement; never remove
            # a nonempty directory to publish these rebuildable files.
            destination.rmdir()
        staging.rename(destination)


def prepare(source, destination, encoder, report_path=None):
    if destination.exists():
        verify(destination, source)
        return
    expected = json.loads((ROOT / 'resources/movie-manifest.json').read_text())['files']['intro_60.mp4']
    if source.stat().st_size != expected['bytes'] or sha(source) != expected['sha256']:
        raise ValueError('Select the exact original intro assigned by resources/movie-manifest.json')
    parent = Path(os.environ.get('HALO2_TEST_SCRATCH', ROOT / '.local/test-scratch'))
    parent.mkdir(parents=True, exist_ok=True)
    receipt = {'source': str(source), 'attempts': []}
    with tempfile.TemporaryDirectory(prefix='halo2-lossless-movie-', dir=parent) as temporary:
        scratch = Path(temporary)
        bank = scratch / 'bank'
        bank.mkdir()
        video = bank / VIDEO
        candidates = ('hevc_nvenc', 'libx265') if encoder == 'auto' else (encoder,)
        for selected in candidates:
            video.unlink(missing_ok=True)
            command = ['ffmpeg', '-hide_banner', '-loglevel', 'error']
            if selected == 'hevc_nvenc':
                command += ['-hwaccel', 'cuda', '-hwaccel_output_format', 'cuda']
            else:
                command += ['-threads', '0']
            command += ['-i', str(source), '-map', '0:v:0', '-map', '0:a:0', '-fps_mode', 'passthrough', '-c:v', selected]
            command += (['-preset', 'p1', '-tune', 'lossless', '-profile:v', 'main']
                        if selected == 'hevc_nvenc' else ['-preset', 'ultrafast', '-x265-params', 'lossless=1:log-level=error'])
            command += ['-c:a', 'copy', '-video_track_timescale', '60000', '-movflags', '+faststart', str(video)]
            result = subprocess.run(command, capture_output=True, text=True, timeout=1800)
            receipt['attempts'].append({'command': command, 'exit': result.returncode, 'stderr': result.stderr[-4000:]})
            if result.returncode == 0:
                break
        else:
            raise RuntimeError('No available lossless encoder completed; ' + receipt['attempts'][-1]['stderr'])
        original, encoded = probe(source), probe(video)
        a = next(s for s in original['streams'] if s['codec_type'] == 'video')
        b = next(s for s in encoded['streams'] if s['codec_type'] == 'video')
        for field in ('width', 'height', 'pix_fmt', 'r_frame_rate', 'nb_frames'):
            if a[field] != b[field]:
                raise ValueError('Lossless preparation changed ' + field)
        reference = frame_hashes(source, scratch / 'original.md5')
        derived = frame_hashes(video, scratch / 'derived.md5')
        if reference != derived or len(reference) != int(a['nb_frames']):
            raise ValueError('Derived video is not pixel-exact across every original frame')
        row = {'schema': SCHEMA, 'role': 'intro', 'source': expected, 'file': VIDEO,
               'bytes': video.stat().st_size, 'sha256': sha(video),
               'source_duration': float(original['format']['duration']),
               'width': a['width'], 'height': a['height'], 'frames': len(reference),
               'frame_rate': a['r_frame_rate'], 'pixel_format': a['pix_fmt'],
               'all_yuv_frames_identical': True, 'copied_audio_packets': True,
               'ordered_frame_hash_sha256': hashlib.sha256('\n'.join(reference).encode()).hexdigest()}
        (bank / 'manifest.json').write_text(json.dumps(row, indent=2) + '\n')
        stage(bank, destination, source)
        receipt.update(passed=True, manifest=row, output=str(destination))
    if report_path:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(receipt, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'resources/movies/intro_60.mp4')
    parser.add_argument('--output', type=Path, default=ROOT / 'resources/movie-playback')
    parser.add_argument('--encoder', choices=('auto', 'hevc_nvenc', 'libx265'), default='auto')
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--stage', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if args.verify:
        verify(args.output, args.source)
    elif args.stage:
        stage(args.stage, args.output, args.source)
    else:
        prepare(args.source, args.output, args.encoder, args.report)
    print('Verified lossless intro playback bank:', args.output)


if __name__ == '__main__':
    main()
