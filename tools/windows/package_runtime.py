#!/usr/bin/env python3
"""Stage the native Windows engine, its Vulkan shaders and exact imported DLL closure.

Run on the Linux cross-build host. Game assets are supplied separately from the
user's MCC installation. This is a local engine package, not a published update.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from stage_native_renderer_shaders import stage_production_shaders

SYSTEM_DLLS = frozenset('''advapi32 avicap32 avrt bcrypt cabinet cfgmgr32 comctl32
comdlg32 crypt32 cryptbase cryptsp bcryptprimitives d2d1 d3d11 d3d12 d3d9 dbghelp dnsapi dsound dwmapi
 dxgi dwrite gdi32 gdiplus hid imm32 iphlpapi kernel32 kernelbase mf mfplat mfreadwrite
mfuuid mpr msimg32 msvcrt mswsock ncrypt netapi32 normaliz ntdll ole32 oleaut32 opengl32
powrprof propsys psapi rpcrt4 secur32 setupapi shell32 shlwapi ucrtbase user32
userenv usp10 uxtheme version vulkan-1 win32u windowscodecs winhttp wininet winmm
winnsi winspool wintrust wldap32 ws2_32 wsock32 wtsapi32'''.split())
VC_RUNTIME_DLLS = frozenset({'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll'})
VC_RUNTIME_URL = 'https://aka.ms/vc14/vc_redist.x64.exe'
EXECUTABLES = {'halo2_native': 'halo2_native.exe', 'halo2_launcher': 'Halo2-Launcher.exe',
               'halo2_title_launcher': 'Halo2-Title-Launcher.exe',
               'halo2_onboarding': 'onboarding/Halo2-Launcher.exe',
               'halo2_mcc_loading_audio': 'halo2_mcc_loading_audio.exe',
               'eos_sdk_worker': 'eos_sdk_worker.exe'}


def system_dll(name):
    stem = name.lower().removesuffix('.dll')
    return stem in SYSTEM_DLLS or stem.startswith(('api-ms-win-', 'ext-ms-win-'))


def imported_dlls(binary, objdump):
    result = subprocess.run([objdump, '-p', str(binary)], check=True,
                            capture_output=True, text=True, errors='replace')
    if 'file format pei-x86-64' not in result.stdout:
        raise RuntimeError('Expected native Windows x86_64 PE image: ' + str(binary))
    return sorted(set(re.findall(r'^\s*DLL Name:\s*(\S+)\s*$', result.stdout, re.MULTILINE)), key=str.lower)


def dll_closure(executables, sdk, *, objdump='x86_64-w64-mingw32-objdump',
                compiler='x86_64-w64-mingw32-g++', additional_libraries=()):
    executables = list(executables)
    available = {}
    for path in sorted((sdk / 'bin').glob('*')):
        if path.is_file() and path.suffix.lower() == '.dll':
            available[path.name.lower()] = path.resolve()
    search = subprocess.run([compiler, '-print-search-dirs'], check=True, capture_output=True, text=True)
    runtime_directories = []
    for line in search.stdout.splitlines():
        if line.startswith(('programs: =', 'libraries: =', 'install: ')):
            value = line.split(': ', 1)[1].removeprefix('=')
            runtime_directories.extend(Path(part).resolve() for part in value.split(os.pathsep) if part)
    # Match the toolchain that compiled this executable; newer GCC runtimes
    # remain compatible with the SDK's older MinGW dependencies.
    for name in ('libstdc++-6.dll', 'libgcc_s_seh-1.dll', 'libwinpthread-1.dll'):
        candidate = next((directory / name for directory in runtime_directories
                          if (directory / name).is_file()), None)
        if candidate is not None:
            available[name] = candidate.resolve()
    for path in [*executables, *additional_libraries]:
        if path.suffix.lower() == '.dll':
            available[path.name.lower()] = path
    dependencies, system = {}, set()
    pending, seen = list(executables), set()
    while pending:
        binary = pending.pop()
        if binary in seen:
            continue
        seen.add(binary)
        for name in imported_dlls(binary, objdump):
            key = name.lower()
            if system_dll(name) or key in VC_RUNTIME_DLLS:
                system.add(key)
                continue
            if key in dependencies:
                continue
            path = available.get(key)
            if path is None:
                result = subprocess.run([compiler, '-print-file-name=' + name], check=True,
                                        capture_output=True, text=True)
                candidate = Path(result.stdout.strip())
                if candidate.is_absolute() and candidate.is_file():
                    path = candidate.resolve()
            if path is None:
                path = next((directory / name for directory in runtime_directories
                             if (directory / name).is_file()), None)
            if path is None:
                raise RuntimeError(f'Missing Windows runtime dependency {name} imported by {binary}')
            dependencies[key] = path
            pending.append(path)
    return dependencies, sorted(system)


def _stamp(path):
    info = path.stat()
    return info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns


def _digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def _copy(source, output, relative):
    if source.is_symlink() or not source.is_file():
        raise RuntimeError('Package input must be an owned regular file: ' + str(source))
    target = output / relative
    if target.is_symlink() or any(parent.is_symlink() for parent in target.parents):
        raise RuntimeError('Refusing package output through a symlink: ' + str(target))
    target.parent.mkdir(parents=True, exist_ok=True)
    before, digest = _stamp(source), _digest(source)
    if not target.is_file() or _digest(target) != digest:
        descriptor, name = tempfile.mkstemp(prefix='.windows-package-', dir=target.parent)
        try:
            with os.fdopen(descriptor, 'wb') as destination, source.open('rb') as stream:
                shutil.copyfileobj(stream, destination)
            if _stamp(source) != before or _digest(Path(name)) != digest:
                raise RuntimeError('Package input changed while copying: ' + str(source))
            os.replace(name, target)
        finally:
            Path(name).unlink(missing_ok=True)
    return {'bytes': before[2], 'sha256': digest, 'source': str(source)}


def package(root, build, sdk, output=None):
    root, build, sdk = (Path(path).resolve() for path in (root, build, sdk))
    output = Path(output or root / 'dist/builds/windows-x86_64').absolute()
    if output == root or output in root.parents or output == build or output in build.parents:
        raise RuntimeError('Windows package needs its own output directory')
    binaries = {relative: build / relative for relative in EXECUTABLES.values()
                if (build / relative).is_file()}
    if 'halo2_native.exe' not in binaries:
        raise RuntimeError('Build the full halo2_native Windows target before packaging')
    # EOS and the Rust security adapter are loaded at runtime, so their
    # dependencies are roots even when no PE import names them directly.
    dynamic = {path.name: path for path in build.glob('*.dll') if path.is_file()}
    configuration = 'Release'
    cache = build / 'CMakeCache.txt'
    if cache.is_file():
        for line in cache.read_text(errors='replace').splitlines():
            if line.startswith('CMAKE_BUILD_TYPE:STRING='):
                configuration = line.split('=', 1)[1]
    security = build / 'online-protocol' / configuration / 'halo2_online_security_ffi.dll'
    if security.is_file():
        dynamic[security.name] = security
    images = list(binaries.values()) + list(dynamic.values())
    dlls, system = dll_closure(images, sdk)
    inputs = {**binaries, **dynamic, **{path.name: path for path in dlls.values()}}
    executable_dlls = {}
    for relative, executable in binaries.items():
        directory = Path(relative).parent
        adjacent, _ = dll_closure([executable], sdk,
            additional_libraries=[*dynamic.values(), *dlls.values()])
        if directory != Path('.'):
            for dependency in adjacent.values():
                inputs[(directory / dependency.name).as_posix()] = dependency
            executable_dlls[relative] = sorted((directory / path.name).as_posix()
                                              for path in adjacent.values())
        else:
            executable_dlls[relative] = sorted(path.name for path in adjacent.values())
    if (build / 'eos-client.env').is_file():
        inputs['eos-client.env'] = build / 'eos-client.env' 
    # Ordinary developer builds emit the inventories and SPIR-V, not an
    # admitted package receipt. Reuse the production stager in a private owner;
    # it authenticates every declared module and rejects runtime interpreters.
    scratch_parent = Path(os.environ.get('HALO2_TEST_SCRATCH', root / '.local/test-scratch'))
    scratch_parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='windows-shaders-', dir=scratch_parent) as temporary:
        shaders = Path(temporary) / 'shaders'
        shader_report = stage_production_shaders(build, shaders)
        return _publish_package(root, build, sdk, output, inputs, system, executable_dlls,
                                shaders, shader_report)


def _publish_package(root, build, sdk, output, inputs, system, executable_dlls,
                     shaders, shader_report):
    for directory, prefix, suffixes in ((shaders, 'shaders', {'.spv', '.json'}),
                                        (root / 'data/tags', 'data/tags', None),
                                        (sdk / 'share/licenses', 'licenses', None)):
        if directory.is_dir():
            for path in sorted(directory.rglob('*')):
                if path.is_file() and (suffixes is None or path.suffix in suffixes):
                    inputs[str(Path(prefix) / path.relative_to(directory))] = path
    # The wrapper sets the working directory for small runtime tag catalogs.
    output.mkdir(parents=True, exist_ok=True)
    records = {relative: _copy(source, output, relative) for relative, source in sorted(inputs.items())}
    for relative, row in records.items():
        if relative == 'shaders/production-shaders.json':
            row.pop('source')
            row.update(generated=True, source_build=str(build))
        elif relative.startswith('shaders/'):
            row['source'] = str(build / relative)
    command = '@echo off\r\ncd /d "%~dp0"\r\n"%~dp0halo2_native.exe" %*\r\n'
    command_bytes = command.encode('ascii')
    (output / 'Run-Halo2.cmd').write_bytes(command_bytes)
    records['Run-Halo2.cmd'] = {'bytes': len(command_bytes),
                                 'sha256': hashlib.sha256(command_bytes).hexdigest(),
                                 'generated': True}
    report = {'schema': 'H2_WINDOWS_NATIVE_PACKAGE_V1', 'platform': 'windows-x86_64',
              'graphics': 'Vulkan', 'runtime': 'halo2_native.exe', 'packaged_release': False,
              'scope': 'native Windows engine and Vulkan shaders; separate user-owned MCC cache required',
              'system_dlls': [name for name in system if name not in VC_RUNTIME_DLLS],
              'prerequisites': ([{'name': 'Microsoft Visual C++ v14 x64 Redistributable',
                                  'url': VC_RUNTIME_URL,
                                  'required_dlls': [name for name in system if name in VC_RUNTIME_DLLS]}]
                                if VC_RUNTIME_DLLS.intersection(system) else []),
              'files': records, 'executable_dlls': executable_dlls,
              'shader_package': {'profile': shader_report['profile'],
                                 'module_count': shader_report['module_count'],
                                 'inventory_sha256': shader_report['inventory_sha256'],
                                 'manifest_sha256': records['shaders/production-shaders.json']['sha256']},
              'total_bytes': sum(row['bytes'] for row in records.values())}
    descriptor, name = tempfile.mkstemp(prefix='.windows-package-', dir=output)
    try:
        with os.fdopen(descriptor, 'w') as stream:
            json.dump(report, stream, indent=2, sort_keys=True)
            stream.write('\n')
        os.replace(name, output / 'windows-package.json')
    finally:
        Path(name).unlink(missing_ok=True)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = package(args.root, args.build_dir, args.sdk, args.output)
    print(json.dumps({'platform': result['platform'], 'files': len(result['files']),
                      'bytes': result['total_bytes'],
                      'output': str(args.output or args.root / 'dist/builds/windows-x86_64')}))


if __name__ == '__main__':
    main()
