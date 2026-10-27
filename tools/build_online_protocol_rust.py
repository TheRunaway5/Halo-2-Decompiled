#!/usr/bin/env python3
"""Build the pinned Rust/CXX module inside the caller's CMake build tree."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import stat
import subprocess
import sys


def run(command, cwd, env):
    result = subprocess.run(command, cwd=cwd, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print(result.stdout, end='', flush=True)
    if result.returncode:
        raise RuntimeError(f'Rust build command failed with exit {result.returncode}')
    return result.stdout


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def windows_curl_options(sdk, target):
    """Bind Rust and C++ to the same selected native DLL, without vendored curl.

    curl-sys probes vcpkg (not pkg-config) on Windows and otherwise embeds its
    own curl in our static archive. Cargo's supported links override supplies
    the already selected SDK instead of modifying the pinned upstream crate.
    """
    if sdk is None or target != 'x86_64-pc-windows-gnu':
        return []
    prefix = f'target.{target}.curl.'
    values = {'rustc-link-lib': ['dylib=curl'],
              'rustc-link-search': ['native=' + str(sdk/'lib')],
              'include': str(sdk/'include')}
    result = []
    for key, value in values.items():
        result.extend(['--config', prefix + key + '=' + json.dumps(value)])
    return result


def worker_link_arguments(target, linker, flags, sdk_library=None, sdk_import=None):
    """Link the process-main worker using the same selected native SDK input."""
    result = ['-C', f'linker={linker}']
    for value in shlex.split(flags, posix='windows-msvc' not in target):
        result += ['-C', f'link-arg={value}']
    if sdk_library is not None:
        if 'windows' in target:
            if sdk_import is None:
                raise ValueError('The Windows SDK worker requires its selected import library')
            result += ['-C', f'link-arg={sdk_import}']
        else:
            suffix = '.dylib' if 'apple-darwin' in target else '.so'
            name = sdk_library.name
            if not name.startswith('lib') or not name.endswith(suffix):
                raise ValueError('Unexpected selected EOS shared-library filename')
            # Link by the actual selected filename, retaining a relocatable
            # DT_NEEDED entry even when the vendor ELF has no SONAME.
            result += ['-L', f'native={sdk_library.parent}', '-l', f'dylib={name[3:-len(suffix)]}']
    if 'apple-darwin' in target:
        result += ['-C', 'link-arg=-Wl,-rpath,@loader_path']
    elif 'linux' in target:
        result += ['-C', 'link-arg=-Wl,-rpath,$ORIGIN']
    return result


def stage_worker(slices, destination, cwd, env):
    for source in slices:
        if not source.is_file() or (os.name != 'nt' and not source.stat().st_mode & 0o111):
            raise ValueError(f'Missing executable Rust SDK worker: {source}')
    if len(slices) == 1:
        shutil.copy2(slices[0], destination)
    else:
        run(['lipo', '-create', *map(str, slices), '-output', str(destination)], cwd, env)
        destination.chmod(stat.S_IMODE(slices[0].stat().st_mode))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cargo', required=True)
    parser.add_argument('--rustc', required=True)
    parser.add_argument('--toolchain', default='1.96.0')
    parser.add_argument('--target', action='append', required=True)
    parser.add_argument('--cxx', required=True)
    parser.add_argument('--cxx-flags', default='')
    parser.add_argument('--library-name', required=True)
    parser.add_argument('--security-library-name')
    parser.add_argument('--deployment-target', default='')
    parser.add_argument('--sysroot', default='')
    parser.add_argument('--offline', action='store_true')
    parser.add_argument('--eos-sdk-root', type=Path)
    parser.add_argument('--eos-sdk-library', type=Path)
    parser.add_argument('--eos-sdk-import-library', type=Path)
    parser.add_argument('--worker-link-flags', default='')
    parser.add_argument('--worker-linker')
    parser.add_argument('--windows-sdk', type=Path)
    args = parser.parse_args()
    sdk_library = args.eos_sdk_library.resolve(strict=True) if args.eos_sdk_library else None
    sdk_import = args.eos_sdk_import_library.resolve(strict=True) if args.eos_sdk_import_library else None
    if bool(args.eos_sdk_root) != bool(sdk_library) or (sdk_import and not sdk_library):
        raise ValueError('SDK worker linking requires the actual selected SDK library')
    windows = ['windows' in target for target in args.target]
    if any(windows) != all(windows):
        raise ValueError('SDK worker targets cannot mix executable formats')
    worker_name = 'eos_sdk_worker.exe' if all(windows) else 'eos_sdk_worker'
    manifest = args.manifest.resolve(strict=True)
    output = args.output.resolve()
    if output == manifest.parent or output.is_relative_to(manifest.parent):
        raise ValueError('Cargo output must be in the CMake build tree, outside source')
    output.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(CARGO_TARGET_DIR=str(output/'cargo'), CARGO_INCREMENTAL='0',
               RUSTUP_TOOLCHAIN=args.toolchain, RUSTUP_AUTO_INSTALL='0', CXX=args.cxx)
    env.pop('HALO2_EOS_SDK_ROOT', None)
    if args.eos_sdk_root:
        env['HALO2_EOS_SDK_ROOT'] = str(args.eos_sdk_root.resolve(strict=True))
    flags = args.cxx_flags
    worker_flags = args.worker_link_flags
    if args.sysroot:
        flags += ' -isysroot ' + shlex.quote(args.sysroot)
        worker_flags += ' -isysroot ' + shlex.quote(args.sysroot)
    env['CXXFLAGS'] = flags
    windows_sdk = None
    if args.windows_sdk:
        sdk = args.windows_sdk.resolve(strict=True)
        windows_sdk = sdk
        env.update(OPENSSL_DIR=str(sdk), OPENSSL_LIB_DIR=str(sdk/'lib'),
                   OPENSSL_INCLUDE_DIR=str(sdk/'include'),
                   PKG_CONFIG_ALLOW_CROSS='1', PKG_CONFIG_SYSROOT_DIR=str(sdk.parent),
                   PKG_CONFIG_LIBDIR=os.pathsep.join(map(str, (sdk/'lib/pkgconfig', sdk/'share/pkgconfig'))),
                   PKG_CONFIG_PATH='', CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER=args.cxx)
    if args.deployment_target:
        env['MACOSX_DEPLOYMENT_TARGET'] = args.deployment_target
    version = run([args.rustc, '--version', '--verbose'], manifest.parent, env)
    if not re.search(r'^release: 1\.96\.0$', version, re.M):
        raise ValueError('ONLINE protocol requires the pinned Rust 1.96.0 toolchain')
    lock = manifest.parent/'Cargo.lock'
    before_lock = digest(lock)
    archives, native_flags, headers = [], None, None
    security_libraries = []
    workers = []
    for target in args.target:
        sdk_options = windows_curl_options(windows_sdk, target)
        command = [args.cargo, 'rustc', '--manifest-path', str(manifest), '--locked',
                   '--release', '--lib', '--target', target, *sdk_options]
        if args.offline:
            command.append('--offline')
        command += ['--', '--print=native-static-libs']
        text = run(command, manifest.parent, env)
        matches = re.findall(r'native-static-libs: (.+)', text)
        if not matches:
            raise ValueError('rustc did not report the actual native linker requirements')
        selected = shlex.split(matches[-1], posix='windows-msvc' not in target)
        if native_flags is not None and native_flags != selected:
            raise ValueError('Universal Rust slices have different native link requirements')
        native_flags = selected
        archive = output/'cargo'/target/'release'/args.library_name
        if not archive.is_file():
            raise ValueError(f'Missing compiled Rust archive: {archive}')
        archives.append(archive)
        command = [args.cargo, 'rustc', '--manifest-path', str(manifest), '--locked',
                   '--release', '--bin', 'eos_sdk_worker', '--target', target, *sdk_options]
        if args.offline:
            command.append('--offline')
        command += ['--', *worker_link_arguments(target, args.worker_linker or args.cxx, worker_flags,
                                               sdk_library, sdk_import)]
        run(command, manifest.parent, env)
        workers.append(output/'cargo'/target/'release'/worker_name)
        if args.security_library_name:
            security_manifest=manifest.parent.parent/'online_security_ffi'/'Cargo.toml'
            command=[args.cargo,'build','--manifest-path',str(security_manifest),'--locked','--release','--target',target,*sdk_options]
            if args.offline:command.append('--offline')
            run(command,security_manifest.parent,env)
            security_libraries.append(output/'cargo'/target/'release'/args.security_library_name)
        include = output/'cargo'/'cxxbridge'
        names = ['rust/cxx.h', 'halo2-online-protocol-bridge/src/lib.rs.h']
        current = {name: (include/name).read_bytes() for name in names}
        current['src/eos_sdk.hpp'] = (manifest.parent/'src/eos_sdk.hpp').read_bytes()
        if headers is not None and current != headers:
            raise ValueError('Universal Rust slices disagree on their generated interface')
        headers = current
    destination = output/args.library_name
    if len(archives) == 1:
        shutil.copyfile(archives[0], destination)
    else:
        run(['lipo', '-create', *map(str, archives), '-output', str(destination)],
            manifest.parent, env)
    if security_libraries:
        security_destination=output/args.security_library_name
        if len(security_libraries)==1:shutil.copyfile(security_libraries[0],security_destination)
        else:run(['lipo','-create',*map(str,security_libraries),'-output',str(security_destination)],manifest.parent,env)
    worker_destination = output/worker_name
    stage_worker(workers, worker_destination, manifest.parent, env)
    if sdk_library:
        shutil.copy2(sdk_library, output/sdk_library.name)
    for name, data in headers.items():
        path = output/'include'/name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    # The final C++ linker consumes rustc's actual native dependencies. Do not
    # guess a platform library list or pass an rlib to the native linker.
    # rustc reports -lcurl for its DLL import. Resolve that one library to the
    # same SDK import used by CMake; do not alter the compiler's CRT search.
    resolved_flags = list(native_flags)
    if windows_sdk:
        curl_import = windows_sdk/'lib/libcurl.dll.a'
        if not curl_import.is_file():
            raise ValueError('Missing selected Windows curl import library')
        resolved_flags = [str(curl_import) if value == '-lcurl' else value for value in native_flags]
    response = (subprocess.list2cmdline(resolved_flags) if any('windows-msvc' in t for t in args.target)
                else ' '.join(shlex.quote(value) for value in resolved_flags))
    (output/'native-libs.rsp').write_text(response+'\n')
    if digest(lock) != before_lock:
        raise ValueError('Locked Rust build changed Cargo.lock')
    report = {'scope': 'Rust static library, process-main SDK worker and generated CXX interface, not runtime acceptance',
              'rustc': version, 'targets': args.target, 'lock_sha256': before_lock,
              'cxx_version': '1.0.205', 'native_flags': native_flags,
              'resolved_native_flags': resolved_flags,
              'requested_cxx': args.cxx, 'requested_cxx_flags': env['CXXFLAGS'],
              'archive': str(destination), 'archive_sha256': digest(destination),
              'worker': str(worker_destination), 'worker_sha256': digest(worker_destination),
              'worker_link_flags': worker_flags,
              'header_sha256': {name: hashlib.sha256(data).hexdigest() for name, data in headers.items()},
              'cargo_target_dir': env['CARGO_TARGET_DIR']}
    (output/'build-report.json').write_text(json.dumps(report, indent=2)+'\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
