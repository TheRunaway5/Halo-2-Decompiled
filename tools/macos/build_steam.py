#!/usr/bin/env python3
"""Cross-publish the Steam metadata/content helper's two private Mac runtimes.

This may run on Linux with the pinned .NET SDK. The Mac verification step
checks native dependencies, signs the private outputs, and executes both CPU
slices. Validation always invokes this builder inside tools/run_test.py.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / 'tools/steam'
RIDS = {'arm64': 'osx-arm64', 'x86_64': 'osx-x64'}


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def prepare_upstream(work, project):
    """Acquire immutable downloader inputs and apply the narrow GUI adapter hooks."""
    inputs = json.loads((PROJECT / 'upstream.json').read_text())
    original = work / 'upstream-original'
    original.mkdir(exist_ok=True)
    patched = project / 'upstream'
    patched.mkdir(exist_ok=True)
    changes = {
        'ContentDownloader.cs': [
            ('static class ContentDownloader', 'static partial class ContentDownloader', 1),
            ('Console.WriteLine("Manifest {0} ({1})", depot.ManifestId, newManifest.CreationTime);',
             'Console.WriteLine("Manifest {0} ({1})", depot.ManifestId, newManifest.CreationTime);\n'
             '            SetupManifest(depot.DepotId, depot.ManifestId, newManifest);', 1),
            ('if (neededChunks.Count == 0)\n                {',
             'if (neededChunks.Count == 0)\n                {\n'
             '                    Halo2.SteamSetup.ContentProtocol.Reused(file.TotalSize);', 1),
            ('var sizeOnDisk = (file.TotalSize - (ulong)neededChunks.Select(x => (long)x.UncompressedLength).Sum());',
             'var sizeOnDisk = (file.TotalSize - (ulong)neededChunks.Select(x => (long)x.UncompressedLength).Sum());\n'
             '                Halo2.SteamSetup.ContentProtocol.Reused(sizeOnDisk);', 1)],
        'Steam3Session.cs': [
            ('class Steam3Session', 'partial class Steam3Session', 1),
            ('DisplayQrCode(session.ChallengeURL);',
             'Halo2.SteamSetup.ContentProtocol.Qr(session.ChallengeURL);', 2),
            ('authSession.PollingWaitForResultAsync()',
             'authSession.PollingWaitForResultAsync(abortedToken.Token)', 1)]}
    records = {}
    for name, item in inputs['files'].items():
        source = original / name
        source.parent.mkdir(parents=True, exist_ok=True)
        if not source.exists():
            with urllib.request.urlopen(item['url'], timeout=60) as response:
                source.write_bytes(response.read(2 * 1024 * 1024))
        if digest(source) != item['sha256']:
            raise RuntimeError('Changed downloader source: ' + name)
        if source.suffix != '.cs': continue
        code = source.read_text(encoding='utf-8-sig')
        for before, after, count in changes.get(source.name, []):
            if code.count(before) != count:
                raise RuntimeError('Downloader adapter anchor changed: ' + source.name + ': ' + before)
            code = code.replace(before, after)
        # Upstream is compiled with nullable annotations disabled. Keep that
        # contract confined to the unmodified third-party translation units.
        target = patched / source.name
        target.write_text('#nullable disable\n' + code)
        records[name] = dict(original_sha256=digest(source), adapted_sha256=digest(target))
    return original, records


def build(work, prefix, arches, dotnet, *, bootstrap_lock=False):
    work, prefix = work.resolve(), prefix.resolve()
    if work.is_relative_to(prefix) or prefix.is_relative_to(work):
        raise RuntimeError('Steam build work and SDK output need separate owners')
    work.mkdir(parents=True, exist_ok=True)
    project = work / 'project'
    project.mkdir(exist_ok=True)
    for path in PROJECT.iterdir():
        if path.is_file() and path.suffix in ('.cs', '.csproj', '.json'):
            shutil.copy2(path, project / path.name)
    original, upstream_records = prepare_upstream(work, project)
    if not bootstrap_lock and not (project / 'packages.lock.json').is_file():
        raise RuntimeError('The checked-in Steam package lock is required')
    environment = dict(os.environ, DOTNET_CLI_HOME=str(work / 'dotnet-user'),
        NUGET_PACKAGES=str(work / 'nuget'), DOTNET_SKIP_FIRST_TIME_EXPERIENCE='1',
        NUGET_HTTP_CACHE_PATH=str(work / 'nuget-http'), NUGET_PLUGINS_CACHE_PATH=str(work / 'nuget-plugins'),
        DOTNET_CLI_USE_MSBUILD_SERVER='0', MSBUILDDISABLENODEREUSE='1',
        DOTNET_GENERATE_ASPNET_CERTIFICATE='false', DOTNET_CLI_WORKLOAD_UPDATE_NOTIFY_DISABLE='true',
        DOTNET_NOLOGO='1', DOTNET_CLI_TELEMETRY_OPTOUT='1')
    commands = []
    def run(*arguments):
        command = [dotnet, *map(str, arguments)]
        commands.append(command)
        subprocess.run(command, cwd=project, env=environment, check=True)
    run('restore', project / 'Halo2Steam.csproj', '--use-lock-file' if bootstrap_lock else '--locked-mode')
    expected_packages = json.loads((PROJECT / 'package-inputs.json').read_text())['sha256']
    package_inputs = {}
    for archive in (work / 'nuget').rglob('*.nupkg'):
        name = archive.relative_to(work / 'nuget').as_posix()
        actual = digest(archive)
        if expected_packages.get(name) != actual:
            raise RuntimeError('Unrecognized or changed Steam runtime package: ' + name)
        package_inputs[name] = actual
    staged = work / 'staged-sdk'
    if staged.exists(): shutil.rmtree(staged)
    staged.mkdir()
    for arch in arches:
        run('publish', project / 'Halo2Steam.csproj', '-c', 'Release', '-r', RIDS[arch],
            '--self-contained', 'true', '--no-restore', '-o', staged / arch,
            '-p:DebugType=None', '-p:DebugSymbols=false', '-p:UseSharedCompilation=false')
    licenses = staged / 'licenses'
    licenses.mkdir()
    for package in (work / 'nuget').glob('*/*'):
        if not package.is_dir(): continue
        notices = [p for p in package.rglob('*') if p.is_file() and
                   (p.suffix == '.nuspec' or p.name.lower().startswith(('license', 'copying', 'thirdpartynotices', 'notice')))]
        for path in notices:
            target = licenses / package.relative_to(work / 'nuget') / path.relative_to(package)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)
    shutil.copy2(project / 'packages.lock.json', staged / 'packages.lock.json')
    shutil.copy2(PROJECT / 'content-profile.json', staged / 'content-profile.json')
    corresponding = staged / 'corresponding-source'
    shutil.copytree(original, corresponding / 'upstream')
    shutil.copytree(project, corresponding / 'helper',
                    ignore=shutil.ignore_patterns('bin', 'obj'))
    shutil.copy2(Path(__file__), corresponding / 'build_steam.py')
    shutil.copy2(original / 'LICENSE', licenses / 'DepotDownloader-GPL-2.0.txt')
    report = dict(schema='H2_MACOS_STEAM_SDK_V1', architectures=arches,
        layout='one self-contained managed/native runtime per CPU; the engine and launcher remain universal Mach-O binaries',
        framework='10.0.12', steamkit='3.4.0', commands=commands, package_inputs=package_inputs,
        downloader=json.loads((PROJECT / 'upstream.json').read_text()), upstream_adapters=upstream_records,
        source_sha256={p.name: digest(p) for p in PROJECT.iterdir() if p.is_file()},
        package_lock=json.loads((project / 'packages.lock.json').read_text()), signed=False,
        files={str(p.relative_to(staged)): dict(bytes=p.stat().st_size, sha256=digest(p))
               for p in sorted(staged.rglob('*')) if p.is_file()})
    (staged / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    (staged / 'built.stamp').write_text(digest(PROJECT / 'Halo2Steam.csproj') + '\n')
    if prefix.exists():
        if any(prefix.iterdir()) and not (prefix / 'manifest.json').is_file():
            raise RuntimeError('Refusing to replace an unrelated Steam SDK output')
        shutil.rmtree(prefix)
    staged.rename(prefix)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--arch', action='append', choices=RIDS, required=True)
    parser.add_argument('--dotnet', default='dotnet')
    parser.add_argument('--bootstrap-lock', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    build(args.work, args.prefix, list(dict.fromkeys(args.arch)), args.dotnet,
          bootstrap_lock=args.bootstrap_lock)


if __name__ == '__main__':
    main()
