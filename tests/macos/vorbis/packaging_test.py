#!/usr/bin/env python3
"""Check the actual setup converter layout with its original SDK absent.

The optional genuine track still has to pass the unchanged shared PCM gate.
Recording its current rejection here does not certify canonical audio.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'tools/macos'))
from audit_rendering import FormatError
from bundle import bundle
from macos.loading_converter import EXECUTABLE, LIBRARIES
from mcc_loading_presentation import (CODEBOOK, CONVERTER_MANIFEST, MEDIA_SHA,
    V3_FRAMES, V3_PCM_SHA, _v3_pcm, converter_dependencies)
from package_loading_converter import stage, write_manifest


def digest(path):
    return hashlib.file_digest(path.open('rb'), 'sha256').hexdigest()


def rejects(function):
    try:
        function()
    except FormatError:
        return
    raise AssertionError('Changed native converter inputs were admitted')


def main():
    app, sdk, output = map(Path, sys.argv[1:4])
    arches, media = sys.argv[4].split(','), Path(sys.argv[5]) if len(sys.argv) > 5 else None
    sdk = sdk.resolve(strict=True)
    manifest = json.loads((sdk / 'manifest.json').read_text())
    if Path(manifest['prefix']) != sdk:
        raise RuntimeError('The test does not own its SDK')
    report = dict(schema='H2_MACOS_LOADING_CONVERTER_PACKAGE_V1', architectures=arches,
        scope='Native converter packaging and admission; canonical PCM acceptance is separate',
        runs=[], retained_large_artifacts=[])
    try:
        with tempfile.TemporaryDirectory(prefix='packaged-loading-', dir=os.environ['HALO2_TEST_SCRATCH']) as directory:
            work = Path(directory)
            packaged = work / 'Loading setup.app'
            shutil.copytree(app, packaged, symlinks=True)
            resources = packaged / 'Contents/Resources'
            executable = app / 'Contents/MacOS/halo2_mcc_loading_audio'
            report['inputs'] = stage(resources, executable, sdk, ROOT, arches)
            hidden = work / 'original-sdk'
            sdk.rename(hidden)
            try:
                bundle(packaged, arches, [], [], resource_sealers=(lambda: write_manifest(resources, arches),))
                relocated = work / 'Folder with spaces/Loading setup.app'
                relocated.parent.mkdir()
                shutil.copytree(packaged, relocated, symlinks=True)
                resources = relocated / 'Contents/Resources'
                subprocess.run(['codesign', '--verify', '--deep', '--strict', str(relocated)], check=True)
                before = {str(p.relative_to(resources)): digest(p) for p in resources.rglob('*') if p.is_file()}
                owner = converter_dependencies(resources)
                assert owner['converter']['path'] == EXECUTABLE
                report['admitted_runtime'] = owner
                if media:
                    assert digest(media) == MEDIA_SHA
                else:
                    media = work / 'invalid.wem'
                    media.write_bytes(b'invalid fixture')
                environment = {k: v for k, v in os.environ.items()
                               if not k.startswith(('LD_', 'DYLD_')) and k != 'GLIBC_TUNABLES'}
                for arch in arches:
                    wav = work / (arch + '.wav')
                    command = ['/usr/bin/arch', '-' + arch, '/usr/bin/env', 'DYLD_PRINT_LIBRARIES=1',
                               str(resources / EXECUTABLE), str(media), str(wav), str(resources / CODEBOOK), str(V3_FRAMES)]
                    result = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=120)
                    libraries = [line for line in result.stderr.splitlines() if 'libvorbis' in line or 'libogg' in line]
                    assert all(any(str(resources / name) in line for line in libraries) for name in LIBRARIES)
                    row = dict(arch=arch, command=command, exit_code=result.returncode, loaded_libraries=libraries,
                               stdout=result.stdout[-2048:])
                    if digest(media) == MEDIA_SHA:
                        assert result.returncode == 0 and wav.is_file()
                        data = wav.read_bytes()
                        row['pcm_sha256'] = hashlib.sha256(data[44:]).hexdigest()
                        try:
                            _v3_pcm(data)
                            row['canonical_pcm'] = True
                        except FormatError as error:
                            assert row['pcm_sha256'] != V3_PCM_SHA and 'PCM differs' in str(error)
                            row.update(canonical_pcm=False, pcm_gate_error=str(error))
                    else:
                        assert result.returncode != 0 and not wav.exists()
                        row['invalid_media_rejected'] = True
                    report['runs'].append(row)
                # The same admission is used by setup before a subprocess is
                # launched. Tampering must not leave a valid runtime receipt.
                path = resources / LIBRARIES[0]
                saved = path.read_bytes();path.write_bytes(saved + b'changed')
                rejects(lambda: converter_dependencies(resources));path.write_bytes(saved)
                extra = resources / 'macos/loading-converter/lib/foreign.dylib'
                extra.write_bytes(b'foreign');rejects(lambda: converter_dependencies(resources));extra.unlink()
                metadata = resources / CONVERTER_MANIFEST
                saved = metadata.read_bytes();value = json.loads(saved);value['architectures'] = [arches[0]]
                if len(arches) > 1:
                    metadata.write_text(json.dumps(value));rejects(lambda: converter_dependencies(resources))
                    metadata.write_bytes(saved)
                assert converter_dependencies(resources) == owner
                after = {str(p.relative_to(resources)): digest(p) for p in resources.rglob('*') if p.is_file()}
                assert before == after
                subprocess.run(['codesign', '--verify', '--deep', '--strict', str(relocated)], check=True)
                report.update(passed=True, sdk_absent_during_runs=True, resources_unchanged=True,
                              all_actual_codec_images_private=True)
            finally:
                hidden.rename(sdk)
            report['sdk_restored'] = sdk.is_dir()
    finally:
        output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS native loading converter closure, production admission and both CPU slices; PCM acceptance remains separately recorded')


if __name__ == '__main__':
    main()
