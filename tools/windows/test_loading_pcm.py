#!/usr/bin/env python3
"""Compare real Linux/Windows loading decoders in disposable run_test scratch.

Only the authenticated MCC WEM is extracted. Audio, executables, libraries and
the private Wine prefix are removed by the enclosing run_test.py supervisor.
"""
import argparse
import array
import ctypes
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import time
import traceback

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from mcc_loading_presentation import CODEBOOK, CODEBOOK_SHA, MEDIA_SHA, V3_FRAMES, V3_PCM_SHA, _v3_pcm, read_media


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def identity(path):
    return dict(path=str(path), bytes=path.stat().st_size, sha256=sha(path))


def windows_path(path):
    return 'Z:' + str(path.resolve()).replace('/', chr(92))


def command(argv, *, env=None, timeout=120, log=None):
    started = time.monotonic()
    if log is None:
        value = subprocess.run(argv, env=env, stdin=subprocess.DEVNULL,
                               capture_output=True, text=True, errors='replace', timeout=timeout)
        output = dict(stdout=value.stdout[-8192:], stderr=value.stderr[-8192:])
    else:
        # Wine daemons must never inherit a captured pipe. Their lifetime is
        # independent of the finite client process whose result we inspect.
        with log.open('wb') as stream:
            value = subprocess.run(argv, env=env, stdin=subprocess.DEVNULL,
                                   stdout=stream, stderr=subprocess.STDOUT, timeout=timeout)
        output = dict(stdout=log.read_text(errors='replace')[-8192:])
    return dict(command=[str(p) for p in argv], exit_code=value.returncode,
                seconds=time.monotonic()-started, **output)


def freeze_windows(executable, runtime, destination):
    destination.mkdir()
    available = {p.name.lower():p for p in runtime.iterdir() if p.is_file()}
    available.update({p.name.lower():p for p in executable.parent.iterdir() if p.is_file()})
    pending, copied = [executable], {}
    while pending:
        source = pending.pop()
        if source.name.lower() in copied:
            continue
        target = destination / source.name
        shutil.copy2(source, target)
        record = identity(source)
        if sha(target) != record['sha256']:
            raise ValueError('Windows converter closure changed during freeze')
        copied[source.name.lower()] = record
        imports = subprocess.run(['objdump', '-p', str(target)], capture_output=True,
                                 text=True, check=True).stdout
        for name in re.findall(r'DLL Name:\s*(\S+)', imports):
            if name.lower() in available:
                pending.append(available[name.lower()])
    return destination / executable.name, copied


def wave_record(path):
    data = path.read_bytes()
    result = identity(path)
    result.update(header_hex=data[:44].hex(), pcm_bytes=max(0,len(data)-44),
                  pcm_sha256=hashlib.sha256(data[44:]).hexdigest())
    try:
        _v3_pcm(data)
        result['authenticated'] = True
    except Exception as error:
        result.update(authenticated=False, authentication_error=str(error))
    return result


def compare(left, right):
    a, b = left.read_bytes()[44:], right.read_bytes()[44:]
    af, bf, ai, bi = array.array('f'), array.array('f'), array.array('I'), array.array('I')
    for values, raw in ((af,a),(bf,b),(ai,a),(bi,b)):
        values.frombytes(raw)
        if sys.byteorder != 'little':
            values.byteswap()
    count, first, maximum, squared, max_index, nonfinite = 0, None, 0., 0., None, 0
    for index, (x,y,xb,yb) in enumerate(zip(af,bf,ai,bi)):
        if xb == yb:
            continue
        count += 1
        if first is None:
            first = dict(sample=index, frame=index//2, channel=index%2,
                         linux=x, windows=y, linux_bits=f'{xb:08x}', windows_bits=f'{yb:08x}')
        if not math.isfinite(x) or not math.isfinite(y):
            nonfinite += 1
            continue
        difference = abs(x-y)
        squared += difference*difference
        if difference > maximum:
            maximum, max_index = difference, index
    return dict(equal_bytes=len(a)==len(b), samples_compared=min(len(af),len(bf)),
                differing_samples=count, first_difference=first, max_absolute_difference=maximum,
                maximum_difference_sample=max_index, rms_difference=math.sqrt(squared/max(1,min(len(af),len(bf)))),
                nonfinite_differences=nonfinite)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--linux-converter', type=Path, default=Path('build/halo2_mcc_loading_audio'))
    parser.add_argument('--windows-converter', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source, package = args.source.resolve(strict=True), args.package.resolve(strict=True)
    executable = (args.windows_converter or package/'halo2_mcc_loading_audio.exe').resolve(strict=True)
    linux = args.linux_converter.resolve(strict=True)
    prefix, stage = owner/'wine', owner/'windows'
    args.report.parent.mkdir(parents=True, exist_ok=True)
    report = dict(passed=False, scratch=str(owner), retained_large_artifacts=[],
                  expected_pcm_sha256=V3_PCM_SHA, expected_frames=V3_FRAMES)
    env = os.environ | dict(WINEPREFIX=str(prefix), WINEARCH='win64', WINEDEBUG='-all',
                           WINEDLLOVERRIDES='winemenubuilder.exe=d;winewayland.drv=d',
                           DISPLAY='', WAYLAND_DISPLAY='')
    pak = source.parent/'MCC/Content/Paks/MCC-WindowsNoEditor.pak'
    before = pak.stat()
    try:
        payload, provenance = read_media(source)
        wem = owner/'loading.wem'; wem.write_bytes(payload)
        codebook = owner/'codebooks.bin'; shutil.copy2(package/CODEBOOK, codebook)
        report['wem'] = identity(wem) | dict(provenance=provenance)
        report['codebook'] = identity(codebook)
        assert report['wem']['sha256'] == MEDIA_SHA
        assert report['codebook']['sha256'] == CODEBOOK_SHA
        win, closure = freeze_windows(executable, package, stage)
        report['windows_closure'] = closure
        report['linux_converter'] = identity(linux)
        report['linux_dependencies'] = command(['ldd', str(linux)])
        report['linux_library_files'] = {}
        for line in report['linux_dependencies']['stdout'].splitlines():
            match = re.search(r'=> (/\S+)',line)
            if match:
                library_path = Path(match[1]).resolve(strict=True)
                report['linux_library_files'][library_path.name] = identity(library_path)
        report['converter_compiler_comment'] = command(['readelf','-p','.comment',str(linux)])
        report['source_hashes'] = {str(path):sha(path) for path in (
            Path(__file__).resolve(), Path(__file__).resolve().parents[1]/'mcc_loading_presentation.py',
            Path(__file__).resolve().parents[1]/'convert_mcc_loading_audio.cpp')}
        report['versions'] = {name:command(argv) for name,argv in dict(
            wine=['wine','--version'], host_compiler=['c++','--version'],
            linux_packages=['pkg-config','--modversion','vorbis','ogg']).items()}
        library = ctypes.CDLL('libvorbis.so.0'); library.vorbis_version_string.restype=ctypes.c_char_p
        report['versions']['linux_vorbis_runtime'] = library.vorbis_version_string().decode()
        lock = json.loads((Path(__file__).resolve().parent/'sdk-lock.json').read_text())
        report['versions']['windows_sdk_packages'] = [r for r in lock.get('packages',[]) if any(
            n in r.get('name','') for n in ('libvorbis','libogg','gcc','libstdc++'))]
        boot = command(['wineboot','--init'], env=env, timeout=300, log=owner/'wineboot.log')
        report['wine_boot'] = boot
        if boot['exit_code']:
            raise RuntimeError('Private Wine prefix failed to initialize')
        subprocess.run(['wineserver','-w'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=60,check=True)
        linux_wav = owner/'linux.wav'
        report['linux_run'] = command([str(linux),str(wem),str(linux_wav),str(codebook),str(V3_FRAMES)])
        if report['linux_run']['exit_code']:
            raise RuntimeError('Linux decoder failed')
        report['linux_wave'] = wave_record(linux_wav)
        report['windows_runs'] = []
        for index in range(2):
            wav = owner/f'windows-{index}.wav'
            run = command(['wine',str(win),windows_path(wem),windows_path(wav),windows_path(codebook),str(V3_FRAMES)],
                          env=env, log=owner/f'windows-{index}.log')
            report['windows_runs'].append(run)
            if run['exit_code']:
                raise RuntimeError('Windows decoder failed')
            run['wave'] = wave_record(wav)
            run['comparison_to_linux'] = compare(linux_wav,wav)
        hashes = [r['wave']['pcm_sha256'] for r in report['windows_runs']]
        report['windows_deterministic'] = len(set(hashes)) == 1
        report['passed'] = report['linux_wave']['authenticated'] and all(
            r['wave']['authenticated'] for r in report['windows_runs']) and report['windows_deterministic']
    except BaseException as error:
        report.update(error=repr(error),traceback=traceback.format_exc()[-12000:])
    finally:
        for argv in (['wineserver','-k'],['wineserver','-w']):
            try:
                subprocess.run(argv,env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=30)
            except (OSError,subprocess.TimeoutExpired):
                pass
        after = pak.stat()
        report['original_pak_metadata_unchanged'] = (before.st_dev,before.st_ino,before.st_size,before.st_mtime_ns) == (
            after.st_dev,after.st_ino,after.st_size,after.st_mtime_ns)
        report['passed'] = report['passed'] and report['original_pak_metadata_unchanged']
        args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(dict(passed=report['passed'],report=str(args.report),scratch=str(owner),
                              error=report.get('error'),
                              linux_pcm_sha256=report.get('linux_wave',{}).get('pcm_sha256'),
                              windows_pcm_sha256=[r.get('wave',{}).get('pcm_sha256') for r in report.get('windows_runs',[])])),flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
