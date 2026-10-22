#!/usr/bin/env python3
"""Compare genuine MCC PCM, then relocate its signed dependency check bundle."""
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools'))
from mcc_loading_presentation import CODEBOOK_SHA, MEDIA_SHA, V3_FRAMES, V3_PCM_SHA, _v3_pcm


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    app, sdk, media, codebooks = (Path(value).resolve(strict=True) for value in sys.argv[1:5])
    report_path, arches = Path(sys.argv[5]), sys.argv[6:]
    manifest = json.loads((sdk / 'manifest.json').read_text())
    if manifest.get('schema') != 'H2_MACOS_VORBIS_SDK_V1' or Path(manifest['prefix']) != sdk:
        raise RuntimeError('The test does not own the supplied dependency SDK')
    if digest(media) != MEDIA_SHA or digest(codebooks) != CODEBOOK_SHA:
        raise RuntimeError('The real loading input or retained codebook checksum differs')
    report = dict(schema='H2_MACOS_LOADING_AUDIO_CHECK_V1', media_sha256=MEDIA_SHA,
        codebooks_sha256=CODEBOOK_SHA, expected_pcm_sha256=V3_PCM_SHA, frames=V3_FRAMES,
        runs=[], retained_large_artifacts=[])
    errors = []
    try:
        with tempfile.TemporaryDirectory(prefix='loading-audio-', dir=os.environ['HALO2_TEST_SCRATCH']) as directory:
            work = Path(directory)
            relocated = work / 'Folder with spaces' / app.name
            shutil.copytree(app, relocated, symlinks=True)
            for label in ('Ogg', 'Vorbis'):
                if (relocated / f'Contents/Resources/licenses/{label}.txt').read_bytes() != (sdk / f'share/licenses/{label}.txt').read_bytes():
                    raise RuntimeError('The bundled dependency license differs: ' + label)
            if (relocated / 'Contents/Resources/licenses/ww2ogg.txt').read_bytes() != (ROOT / 'external/ww2ogg/COPYING').read_bytes():
                raise RuntimeError('The bundled ww2ogg license differs')
            hidden = work / 'original-sdk'
            sdklibraries = sdk / 'lib'
            for current, mode in ((app, 'original'), (relocated, 'relocated-sdk-absent')):
                info = plistlib.loads((current / 'Contents/Info.plist').read_bytes())
                executable = current / 'Contents/MacOS' / info['CFBundleExecutable']
                if current == relocated:
                    sdk.rename(hidden)
                try:
                    subprocess.run(['codesign', '--verify', '--deep', '--strict', str(current)], check=True)
                    outputs = {}
                    for arch in arches:
                        output = work / (mode + '-' + arch + '.wav')
                        # arch is protected by SIP and clears inherited DYLD_*
                        # variables. Set tracing in env after arch has selected
                        # the slice, immediately before the owned executable.
                        command = ['/usr/bin/arch', '-' + arch, '/usr/bin/env', 'DYLD_PRINT_LIBRARIES=1',
                                   str(executable), str(media), str(output), str(codebooks), str(V3_FRAMES)]
                        environment = {k: v for k, v in os.environ.items() if not k.startswith('DYLD_')}
                        result = subprocess.run(command, env=environment, capture_output=True, text=True)
                        row = dict(mode=mode, arch=arch, command=command, exit=result.returncode,
                            stdout=result.stdout[-4096:])
                        loaded = [line for line in result.stderr.splitlines() if 'libvorbis' in line or 'libogg' in line]
                        row['loaded_libraries'] = loaded
                        if result.returncode:
                            row['error'] = result.stderr[-4096:]
                            errors.append(row['error'])
                        else:
                            data = output.read_bytes()
                            outputs[arch] = data
                            row.update(wav_bytes=len(data), pcm_sha256=hashlib.sha256(data[44:]).hexdigest())
                            try:
                                _v3_pcm(data)
                                row['canonical_pcm'] = True
                            except Exception as error:
                                row.update(canonical_pcm=False, error=str(error))
                                errors.append(str(error))
                            if not all(any(str(current / ('Contents/Frameworks/' + name)) in line for line in loaded)
                                       for name in ('libogg.0.dylib', 'libvorbis.0.dylib')):
                                errors.append('Both codec libraries must load from the actual check bundle')
                        report['runs'].append(row)
                    if len(outputs) == 2:
                        left, right = outputs.values()
                        report[mode + '_slice_comparison'] = dict(byte_exact=left == right,
                            differing_samples=sum(a != b for a, b in zip(struct.iter_unpack('<I', left[44:]), struct.iter_unpack('<I', right[44:]))))
                finally:
                    if current == relocated:
                        hidden.rename(sdk)
            report['sdk_restored'] = sdklibraries.is_dir()
            report['passed'] = not errors
            report['errors'] = errors
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n')
    if errors:
        raise RuntimeError('; '.join(dict.fromkeys(errors)))
    print('PASS genuine loading track: both slices match every canonical PCM byte; signed relocated closure and licenses verified with SDK absent')


if __name__ == '__main__':
    main()
