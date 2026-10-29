#!/usr/bin/env python3
"""Validate sword fixtures against explicitly pinned, copied reference archives.

Authenticate a coherent reference once with --authenticate. Subsequent runs
require that manifest and a complete, explicit JSON case matrix. Only the
current fixture and declared archive-member overlays compile. All copies,
objects, executables and native worlds belong to run_test.py scratch; source,
headers, archives and compiler metadata in the reference remain read-only.
This is reference-cohort validation, not current-source or selected delivery
equivalence. A source/header API mismatch fails without an ABI repair.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = 'halo2-sword-reference-archive-bank-v1'
HEADER_SUFFIXES = {'.h', '.hpp', '.hxx', '.inl', '.inc'}
REPORT_LIMIT = 1024 * 1024


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def write_json(path, value):
    data = (json.dumps(value, sort_keys=True, indent=2) + '\n').encode()
    if len(data) > REPORT_LIMIT:
        raise RuntimeError('Bounded report exceeds 1MiB: ' + str(path))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def direct(path):
    path = Path(path).absolute()
    # Distribution linker/header aliases are legitimate pinned inputs; source
    # and generated reference files still require their direct file owner.
    system_alias = path.is_relative_to('/usr') or path.is_relative_to('/lib')
    if not path.is_file() or (path.is_symlink() and not system_alias):
        raise RuntimeError('Expected a direct reference file: ' + str(path))
    return path


def ar_members(path):
    """Read ordered GNU ar members, including duplicate names, without extraction."""
    with path.open('rb') as stream:
        if stream.read(8) != b'!<arch>\n':
            raise RuntimeError('Unsupported reference archive: ' + str(path))
        names = b''
        while header := stream.read(60):
            if len(header) != 60 or header[58:60] != b'`\n':
                raise RuntimeError('Malformed archive member: ' + str(path))
            name = header[:16].decode().strip()
            size = int(header[48:58])
            data = stream.read(size)
            if len(data) != size:
                raise RuntimeError('Truncated archive member: ' + str(path))
            if size & 1:
                stream.read(1)
            if name == '//':
                names = data
                continue
            if name in ('/', '/SYM64/'):
                continue
            if name.startswith('/') and name[1:].isdigit():
                offset = int(name[1:])
                name = names[offset:names.index(b'/\n', offset)].decode()
            else:
                name = name.rstrip('/')
            yield name, hashlib.sha256(data).hexdigest()


def link_tokens(build, target):
    path = direct(build / 'CMakeFiles' / (target + '.dir') / 'link.txt')
    tokens = shlex.split(path.read_text())
    return path, tokens[tokens.index('-o') + 2:]


def resolved_input(build, token):
    if token.startswith('-Wl,@'):
        return Path(token[5:])
    if token.startswith('-'):
        return None
    path = Path(token)
    path = path if path.is_absolute() else build / path
    return path if path.is_file() else None


def source_inventory(source):
    files = [source / 'CMakeLists.txt']
    for folder in ('src', 'include', 'cmake', 'data', 'shaders', 'rust'):
        files.extend(p for p in (source / folder).rglob('*') if p.is_file()
                     and 'target' not in p.relative_to(source / folder).parts)
    return {p.relative_to(source).as_posix(): digest(direct(p)) for p in sorted(files)}


def authenticate(args, owner, report):
    base = args.reference_bank.resolve(strict=True)
    source, build = base / 'source', base / 'build'
    cache = direct(build / 'CMakeCache.txt')
    home = re.search(r'^CMAKE_HOME_DIRECTORY:INTERNAL=(.*)$', cache.read_text(), re.M)
    if not home or Path(home[1]).resolve() != source.resolve():
        raise RuntimeError('Reference cache does not identify its private source bank')
    before = source_inventory(source)
    link, tokens = link_tokens(build, 'capture_source_melee_contact')
    inputs = {str(p): digest(direct(p)) for token in tokens
              if (p := resolved_input(build, token)) is not None}
    inputs[str(link)] = digest(link)
    inputs[str(cache)] = digest(cache)
    objects, archives, occurrences = 0, 0, 0
    closure_hash = hashlib.sha256()
    for token in dict.fromkeys(t for t in tokens if t.endswith('.a')):
        archive = resolved_input(build, token)
        candidates = [p for p in (build / 'CMakeFiles').glob('*/link.txt')
                      if p.parent.name == Path(token).stem.removeprefix('lib') + '.dir']
        if '/' in token:
            candidates = [p for p in archive.parent.rglob('CMakeFiles/*/link.txt')
                          if any(Path(t).name == archive.name for t in shlex.split(p.read_text())[:4])]
        if not candidates:
            # The Cargo bridge is authenticated through its declared inputs.
            if archive.name != 'libhalo2_online_protocol_bridge.a':
                raise RuntimeError('No compiled-object closure for ' + str(archive))
            make = direct(build / 'CMakeFiles/halo2_online_protocol_rust_build.dir/build.make')
            inputs[str(make)] = digest(make)
            prefix = 'online-protocol/Release/libhalo2_online_protocol_bridge.a: '
            for line in make.read_text().splitlines():
                if line.startswith(prefix):
                    dep = Path(line[len(prefix):])
                    dep = dep if dep.is_absolute() else build / dep
                    dep = direct(dep)
                    if dep.stat().st_mtime_ns > archive.stat().st_mtime_ns:
                        raise RuntimeError('Stale Rust reference archive: ' + str(dep))
                    inputs[str(dep)] = digest(dep)
            continue
        if len(candidates) != 1:
            raise RuntimeError('Ambiguous archive object closure: ' + str(archive))
        inventory = direct(candidates[0])
        cwd = inventory.parent.parent.parent
        inputs[str(inventory)] = digest(inventory)
        flags = direct(inventory.with_name('flags.make'))
        inputs[str(flags)] = digest(flags)
        makefile = direct(inventory.with_name('build.make'))
        inputs[str(makefile)] = digest(makefile)
        expected = [cwd / t for t in shlex.split(inventory.read_text()) if t.endswith('.o')]
        members = list(ar_members(archive))
        if len(expected) != len(members):
            raise RuntimeError('Reference archive member count differs: ' + str(archive))
        for obj, (name, pin) in zip(expected, members):
            obj = direct(obj)
            if obj.name != name or digest(obj) != pin or obj.stat().st_mtime_ns > archive.stat().st_mtime_ns:
                raise RuntimeError('Reference archive/object mismatch: ' + str(obj))
            closure_hash.update((str(obj) + ' ' + pin + '\n').encode())
            depfile = direct(Path(str(obj) + '.d'))
            for token in shlex.split(depfile.read_text().replace('\\\n', ' ').split(':', 1)[1]):
                dep = Path(token)
                dep = direct(dep if dep.is_absolute() else cwd / dep)
                occurrences += 1
                if dep.stat().st_mtime_ns > obj.stat().st_mtime_ns:
                    raise RuntimeError('Reference object has newer input: ' + str(dep))
                represented = dep.is_relative_to(source) and dep.relative_to(source).as_posix() in before
                if not represented:
                    inputs[str(dep)] = digest(dep)
            objects += 1
        archives += 1
    fixture_flags = direct(build / 'CMakeFiles/capture_source_melee_contact.dir/flags.make')
    inputs[str(fixture_flags)] = digest(fixture_flags)
    if source_inventory(source) != before or any(digest(p) != pin for p, pin in inputs.items()):
        raise RuntimeError('Reference changed during authentication')
    inventory_path = args.manifest.with_name(args.manifest.stem + '-sources.json')
    if args.manifest.exists() or inventory_path.exists():
        raise RuntimeError('Preserve the existing manifest; choose a fresh name')
    write_json(inventory_path, before)
    manifest = dict(schema=SCHEMA, reference_bank=str(base), source=str(source), build=str(build),
                    source_inventory=str(inventory_path.resolve()), source_inventory_sha256=digest(inventory_path),
                    inputs=inputs, link_tokens=tokens, fixture_flags=str(fixture_flags),
                    authenticated_archives=archives, authenticated_objects=objects,
                    compiler_dependency_occurrences=occurrences, object_closure_sha256=closure_hash.hexdigest(),
                    scope=__doc__)
    write_json(args.manifest, manifest)
    report.update(passed=True, status='authenticated-reference', manifest=str(args.manifest.resolve()),
                  manifest_sha256=digest(args.manifest), archives=archives, objects=objects,
                  compiler_dependency_occurrences=occurrences)


def validate_manifest(path):
    manifest = json.loads(direct(path).read_text())
    if manifest.get('schema') != SCHEMA:
        raise RuntimeError('Unsupported sword archive-bank manifest')
    inventory = direct(manifest['source_inventory'])
    if digest(inventory) != manifest['source_inventory_sha256']:
        raise RuntimeError('Reference source inventory changed')
    pins = json.loads(inventory.read_text())
    source = Path(manifest['source'])
    if source_inventory(source) != pins:
        raise RuntimeError('Reference source/header bank changed')
    for name, pin in manifest['inputs'].items():
        if digest(direct(name)) != pin:
            raise RuntimeError('Reference link/compiler input changed: ' + name)
    return manifest, pins


def cmake_quote(value):
    if ']=]' in str(value):
        raise RuntimeError('Unsupported CMake bracket delimiter in path')
    return '[=[' + str(value) + ']=]'


CASE_RUNNER = '''from pathlib import Path
import json,os,subprocess,sys,time,hashlib
binary,mapfile,specfile,name,output=sys.argv[1:];case=next(c for c in json.loads(Path(specfile).read_text())['cases'] if c['name']==name)
scratch=Path(os.environ['HALO2_TEST_SCRATCH']);environment=dict(os.environ,**case.get('environment',{}));start=time.monotonic()
values=dict(binary=binary,map=mapfile,map_dir=str(Path(mapfile).parent),world=str(scratch/'world'),mode=case['mode'],scratch=str(scratch),report=str(Path(output)/(name+'-wrapper.json')),python=sys.executable)
command=[token.format_map(values) for token in case['wrapper']] if 'wrapper' in case else [binary,mapfile,str(Path(mapfile).parent),'native-mcc',str(scratch/'world'),case['mode']]
try:
 result=subprocess.run(command,env=environment,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=case.get('timeout',180));raw=result.stdout;status=result.returncode
except subprocess.TimeoutExpired as error:
 raw=(error.stdout or b'')+b'\\nOwned sword case timed out\\n';status=124
limit=1024*1024;retained=raw if len(raw)<=limit else raw[:limit//2]+raw[-limit//2:];out=Path(output)
(out/(name+'.log')).write_bytes(retained)
receipt=dict(name=name,mode=case['mode'],command=command,exit_code=status,seconds=time.monotonic()-start,raw_bytes=len(raw),retained_bytes=len(retained),truncated=len(raw)>limit,raw_sha256=hashlib.sha256(raw).hexdigest(),retained_sha256=hashlib.sha256(retained).hexdigest())
(out/(name+'.json')).write_text(json.dumps(receipt,indent=2)+'\\n');sys.stdout.buffer.write(raw[-20000:]);sys.exit(status)
'''


def execute(args, owner, report):
    manifest_pin, cases_pin = digest(args.manifest), digest(args.cases)
    manifest, source_pins = validate_manifest(args.manifest)
    source, build = Path(manifest['source']), Path(manifest['build'])
    specification = json.loads(direct(args.cases).read_text())
    cases = specification['cases']
    names = [case['name'] for case in cases]
    if not cases or len(cases) > 128 or len(set(names)) != len(names):
        raise RuntimeError('Supply 1..128 uniquely named complete cases')
    for case in cases:
        if not re.fullmatch(r'[A-Za-z0-9_-]+', case['name']) or not isinstance(case['mode'], str):
            raise RuntimeError('Invalid case name/mode')
        if not 1 <= case.get('timeout', 180) <= 600:
            raise RuntimeError('Case timeout must be from1 through600 seconds')
        if 'wrapper' in case and (not isinstance(case['wrapper'], list) or not case['wrapper'] or
                                  any(not isinstance(token, str) for token in case['wrapper'])):
            raise RuntimeError('Case wrapper must be a nonempty argv array')
    print('Authenticated reference; copying owned inputs for', len(cases), 'declared cases', flush=True)
    bank, work = owner / 'bank', owner / 'source'
    work.mkdir()
    for relative in source_pins:
        rel = Path(relative)
        if rel.parts[0] == 'include' or (rel.parts[0] == 'src' and rel.suffix in HEADER_SUFFIXES | {'.def'}):
            dest = bank / rel
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source / rel, dest)
    copies = {}
    for token in manifest['link_tokens']:
        path = resolved_input(build, token)
        if path is None or not path.is_relative_to(Path(manifest['reference_bank'])):
            continue
        dest = bank / 'link-inputs' / path.relative_to(Path(manifest['reference_bank']))
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, dest)
        if digest(dest) != manifest['inputs'][str(path)]:
            raise RuntimeError('Reference changed while copying ' + str(path))
        copies[str(path)] = dest
    fixture = direct(args.fixture)
    fixture_pin = digest(fixture)
    shutil.copy2(fixture, work / 'capture_source_melee_contact.cpp')
    declared_pins = {}
    extra_sources = []
    for index, filename in enumerate(specification.get('extra_sources', [])):
        original = direct(filename)
        declared_pins[str(original)] = digest(original)
        copied = work / 'extra' / str(index) / original.name
        copied.parent.mkdir(parents=True)
        shutil.copy2(original, copied)
        extra_sources.append(copied)
    include_root = work / 'includes'
    for entry in specification.get('include_files', []):
        original, relative = direct(entry['source']), Path(entry['path'])
        if relative.is_absolute() or '..' in relative.parts or relative == Path('.'):
            raise RuntimeError('Copied include path must stay inside owned include root')
        copied = include_root / relative
        if copied.exists():
            raise RuntimeError('Duplicate copied include destination: ' + str(relative))
        copied.parent.mkdir(parents=True, exist_ok=True)
        declared_pins[str(original)] = digest(original)
        shutil.copy2(original, copied)
    for case in cases:
        for token in case.get('wrapper', []):
            if '{' not in token and Path(token).is_absolute() and Path(token).is_file():
                original = direct(token)
                declared_pins[str(original)] = digest(original)
    additional_link_inputs = []
    for index, filename in enumerate(specification.get('additional_link_inputs', [])):
        original = direct(filename)
        declared_pins[str(original)] = digest(original)
        copied = bank / 'additional-link-inputs' / str(index) / original.name
        copied.parent.mkdir(parents=True)
        shutil.copy2(original, copied)
        additional_link_inputs.append(copied)
    runtime_receipts = []
    for index, entry in enumerate(specification.get('runtime_files', [])):
        original, variable = direct(entry['source']), entry['environment']
        if not re.fullmatch(r'(?:H2|HALO2)_[A-Z0-9_]+', variable) or variable == 'HALO2_TEST_SCRATCH':
            raise RuntimeError('Runtime file environment must name a declared H2/HALO2 input')
        pin = digest(original)
        if pin != entry['sha256']:
            raise RuntimeError('Runtime file differs from its explicit SHA256 pin')
        copied = bank / 'runtime-inputs' / str(index) / original.name
        copied.parent.mkdir(parents=True)
        shutil.copy2(original, copied)
        if digest(copied) != pin:
            raise RuntimeError('Runtime file changed during the owned copy')
        declared_pins[str(original)] = pin
        for case in cases:
            environment = case.setdefault('environment', {})
            if variable in environment:
                raise RuntimeError('Runtime file environment is also set directly in a case')
            environment[variable] = str(copied)
        runtime_receipts.append(dict(source=str(original), sha256=pin, copied=str(copied), environment=variable))
    write_json(work / 'cases.json', specification)
    (work / 'run_case.py').write_text(CASE_RUNNER)
    lines = ['cmake_minimum_required(VERSION 3.20)', 'project(sword_archive_bank LANGUAGES CXX)',
             'include(CTest)', 'set(CMAKE_CXX_STANDARD 20)', 'set(CMAKE_CXX_STANDARD_REQUIRED ON)',
             'add_executable(capture_source_melee_contact capture_source_melee_contact.cpp ' +
             ' '.join(map(cmake_quote, extra_sources)) + ')',
             'target_include_directories(capture_source_melee_contact PRIVATE ' +
             cmake_quote(bank / 'include') + ' ' + cmake_quote(bank / 'src') + ' ' +
             cmake_quote(include_root) + ' /usr/include/SDL2)',
             'target_link_options(capture_source_melee_contact PRIVATE -Wl,--strip-debug)']
    for field, directive in (('compile_definitions', 'target_compile_definitions'),
                             ('link_options', 'target_link_options')):
        values = specification.get(field, [])
        if not isinstance(values, list) or any(not isinstance(value, str) for value in values):
            raise RuntimeError(field + ' must be a string array')
        if values:
            lines.append(directive + '(capture_source_melee_contact PRIVATE ' +
                         ' '.join(map(cmake_quote, values)) + ')')
    mapped = []
    for token in manifest['link_tokens']:
        path = resolved_input(build, token)
        if token.startswith('-Wl,-rpath,'):
            directories = []
            for directory in token.removeprefix('-Wl,-rpath,').split(':'):
                original = Path(directory)
                if original.is_relative_to(Path(manifest['reference_bank'])):
                    original = bank / 'link-inputs' / original.relative_to(Path(manifest['reference_bank']))
                directories.append(str(original))
            mapped.append('-Wl,-rpath,' + ':'.join(directories))
        elif token.startswith('-Wl,@'):
            mapped.append('-Wl,@' + str(copies[str(path)]))
        elif path is not None:
            mapped.append(str(copies.get(str(path), path)))
        else:
            mapped.append(token)
    lines.append('target_link_libraries(capture_source_melee_contact PRIVATE ' +
                 ' '.join(cmake_quote(t) for t in mapped + additional_link_inputs) + ')')
    overlay_pins = {}
    overlay_receipts = []
    declared_members = set()
    previous_overlay_target = {}
    for index, overlay in enumerate(specification.get('overlays', [])):
        archive = direct(build / overlay['archive'])
        member = overlay['member']
        if Path(member).name != member or sum(n == member for n, _ in ar_members(archive)) != 1:
            raise RuntimeError('Overlay must identify one existing archive member')
        if str(archive) not in copies:
            raise RuntimeError('Overlay archive is outside this target closure')
        if (str(archive), member) in declared_members:
            raise RuntimeError('Declare each overlay archive member once')
        declared_members.add((str(archive), member))
        if ('source' in overlay) == ('reference_source' in overlay):
            raise RuntimeError('Overlay requires exactly one source or reference_source')
        if 'reference_source' in overlay:
            relative = Path(overlay['reference_source'])
            if relative.is_absolute() or '..' in relative.parts or relative.as_posix() not in source_pins:
                raise RuntimeError('Overlay reference_source is outside the pinned bank')
            original = direct(source / relative)
        else:
            original = direct(overlay['source'])
        copied = work / ('overlay-' + str(index)) / original.name
        copied.parent.mkdir()
        # Overlay sources are edited only in this owned tree. Frozen inputs
        # may be read-only; inherit their bytes without their write denial.
        shutil.copyfile(original, copied)
        overlay_pins[str(original)] = digest(original)
        receipt = dict(archive=str(archive), member=member, source=str(original),
                       source_sha256=overlay_pins[str(original)],
                       reference_member_sha256=next(pin for name, pin in ar_members(archive) if name == member))
        if 'replace_once' in overlay:
            replacement = overlay['replace_once']
            old, new = replacement['old'], replacement['new']
            text = copied.read_text()
            if not isinstance(old, str) or not isinstance(new, str) or not old or old == new or text.count(old) != 1:
                raise RuntimeError('Overlay requires one exact, nonempty differing replacement')
            copied.write_text(text.replace(old, new, 1))
            receipt['replace_once'] = replacement
        receipt['compiled_source_sha256'] = digest(copied)
        overlay_receipts.append(receipt)
        target = 'overlay_' + str(index)
        library = archive.stem.removeprefix('lib')
        flags_path = build / 'CMakeFiles' / (library + '.dir') / 'flags.make'
        flags = flags_path.read_text()
        options = []
        for variable in ('CXX_DEFINES', 'CXX_INCLUDES', 'CXX_FLAGS'):
            match = re.search(r'^' + variable + r' = (.*)$', flags, re.M)
            if match:
                options.extend(shlex.split(match[1].replace(str(source), str(bank))))
        options.extend(overlay.get('compile_options', []))
        archive_link = build / 'CMakeFiles' / (library + '.dir') / 'link.txt'
        matching_objects = [t for t in shlex.split(archive_link.read_text())
                            if t.endswith('.o') and Path(t).name == member]
        if len(matching_objects) != 1:
            raise RuntimeError('Overlay member has no unique compiler input')
        prefix = '# Custom options: ' + matching_objects[0] + '_OPTIONS = '
        for line in flags.splitlines():
            if line.startswith(prefix):
                options.extend(line[len(prefix):].split(';'))
        member_path = '${CMAKE_CURRENT_BINARY_DIR}/members/' + str(index) + '/' + member
        lines.extend(['add_library(' + target + ' OBJECT ' + cmake_quote(copied) + ')',
                      'target_include_directories(' + target + ' PRIVATE ' + cmake_quote(include_root) + ')',
                      # CMake de-duplicates standalone options; keep repeated
                      # option/argument pairs such as -isystem PATH together.
                      'target_compile_options(' + target + ' PRIVATE ' +
                      cmake_quote('SHELL:' + shlex.join(options)) + ')',
                      'add_custom_target(apply_' + target + ' COMMAND ${CMAKE_COMMAND} -E make_directory ' +
                      '"${CMAKE_CURRENT_BINARY_DIR}/members/' + str(index) + '" COMMAND ${CMAKE_COMMAND} -E copy ' +
                      '$<TARGET_OBJECTS:' + target + '> "' + member_path + '" COMMAND /usr/bin/ar r ' +
                      cmake_quote(copies[str(archive)]) + ' "' + member_path + '" DEPENDS ' + target +
                      ' COMMAND_EXPAND_LISTS VERBATIM)',
                      'add_dependencies(capture_source_melee_contact apply_' + target + ')'])
        if str(archive) in previous_overlay_target:
            lines.append('add_dependencies(apply_' + target + ' apply_' + previous_overlay_target[str(archive)] + ')')
        previous_overlay_target[str(archive)] = target
    for case in cases:
        command = [sys.executable, ROOT / 'tools/run_test.py', '--scratch-root', owner / 'case-scratch',
                   '--', sys.executable, work / 'run_case.py']
        lines.append('add_test(NAME ' + case['name'] + ' COMMAND ' + ' '.join(map(cmake_quote, command)) +
                     ' $<TARGET_FILE:capture_source_melee_contact> ' + ' '.join(map(cmake_quote, [args.map,
                     work / 'cases.json', case['name'], args.output])) + ')')
        lines.append('set_tests_properties(' + case['name'] + ' PROPERTIES TIMEOUT ' +
                     str(case.get('timeout', 180) + 10) + ')')
    (work / 'CMakeLists.txt').write_text('\n'.join(lines) + '\n')
    report.update(manifest=str(args.manifest), manifest_sha256=manifest_pin, cases_sha256=cases_pin,
                  fixture=str(fixture), fixture_sha256=fixture_pin, overlay_sources=overlay_pins,
                  declared_input_sha256=declared_pins, overlays=overlay_receipts,
                  runtime_files=runtime_receipts, generated_cases_sha256=digest(work / 'cases.json'),
                  reference_bank=manifest['reference_bank'], declared_cases=cases,
                  copied_archive_count=sum(p.suffix == '.a' for p in copies.values()),
                  generated_cmake_sha256=digest(work / 'CMakeLists.txt'), scope=__doc__)
    write_json(args.output / 'report.json', report)
    command = [sys.executable, '-B', str(ROOT / 'tools/run_tests.py'), '--build-worker', '--source', str(work),
               '--output', str(args.output / 'cmake'), '--jobs', str(args.jobs), '--test-jobs', '1',
               '--target', 'capture_source_melee_contact']
    report['worker_command'] = command
    code = subprocess.call(command)
    report.update(passed=code == 0, status='passed' if code == 0 else 'failed', worker_exit_code=code)
    for receipt in overlay_receipts:
        original = Path(receipt['archive'])
        before, after = list(ar_members(original)), list(ar_members(copies[str(original)]))
        allowed = {member for archive, member in declared_members if archive == str(original)}
        if len(before) != len(after) or any(a[0] != b[0] or (a[0] not in allowed and a != b)
                                             for a, b in zip(before, after)):
            raise RuntimeError('Owned overlay changed an undeclared archive member')
        receipt['compiled_member_sha256'] = next(pin for name, pin in after if name == receipt['member'])
        receipt['owned_archive_sha256'] = digest(copies[str(original)])
        receipt['only_declared_members_changed'] = True
    if digest(args.manifest) != manifest_pin or digest(args.cases) != cases_pin or digest(fixture) != fixture_pin:
        raise RuntimeError('Declared validation input changed during the run')
    if any(digest(path) != pin for path, pin in overlay_pins.items()):
        raise RuntimeError('Declared overlay source changed during the run')
    if any(digest(path) != pin for path, pin in declared_pins.items()):
        raise RuntimeError('Declared extra input changed during the run')
    validate_manifest(args.manifest)
    report['reference_unchanged_after'] = True
    return code


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authenticate', action='store_true')
    parser.add_argument('--reference-bank', type=Path)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--cases', type=Path)
    parser.add_argument('--fixture', type=Path, default=ROOT / 'tools/capture_source_melee_contact.cpp')
    parser.add_argument('--map', type=Path, default=ROOT / 'assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--worker', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.authenticate and not args.reference_bank:
        parser.error('--authenticate requires --reference-bank')
    if not args.authenticate and not args.cases:
        parser.error('Validation requires a complete explicit --cases JSON matrix')
    if not 1 <= args.jobs <= 8:
        parser.error('--jobs must be from1 through8')
    for name in ('manifest', 'output', 'fixture', 'map', 'cases'):
        if value := getattr(args, name):
            setattr(args, name, value.resolve())
    if not args.output.is_relative_to(ROOT / 'analysis'):
        parser.error('--output must be an explicit small-report directory under analysis')
    args.output.mkdir(parents=True, exist_ok=True)
    if not args.worker:
        command = [sys.executable, '-B', str(ROOT / 'tools/run_test.py'), '--scratch-root',
                   str(ROOT / '.local/test-scratch'), '--', sys.executable, '-B', str(Path(__file__).resolve()),
                   '--worker', *sys.argv[1:]]
        code = subprocess.call(command)
        path = args.output / 'report.json'
        if path.is_file():
            report = json.loads(path.read_text())
            if report.get('owner'):
                report['cleanup_verified'] = not Path(report['owner']).exists()
                write_json(path, report)
        return code
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if not owner.is_relative_to(ROOT / '.local/test-scratch'):
        raise RuntimeError('Worker requires the official repository scratch owner')
    report = dict(schema='sword-owned-reference-validation-v1', passed=False, status='running',
                  owner=str(owner), tool_sha256=digest(Path(__file__)), retained_large_artifacts=[])
    code = 1
    try:
        if args.authenticate:
            authenticate(args, owner, report)
            code = 0
        else:
            code = execute(args, owner, report)
    except Exception as error:
        report.update(passed=False, status='failed', error=f'{type(error).__name__}: {error}')
        print(report['error'], file=sys.stderr)
    finally:
        report['temporary_bytes_before_cleanup'] = sum(p.stat().st_size for p in owner.rglob('*') if p.is_file())
        write_json(args.output / 'report.json', report)
    return code


if __name__ == '__main__':
    raise SystemExit(main())
