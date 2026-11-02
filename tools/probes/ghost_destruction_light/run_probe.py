#!/usr/bin/env python3
"""Compile, link and run the standalone Ghost-destruction light probe.

Run ONLY under the project's owned-scratch lifecycle, e.g.

  python3 tools/run_test.py -- python3 -I \
      .claude/worktrees/ghost-preload-probe/tools/probes/ghost_destruction_light/run_probe.py \
      --main .local/test-scratch/zanzibar-glass-terminal/halo2-test-01m4g764 \
      --report-dir build/test-results/zanzibar-glass-terminal-20261101/claude-ghost-destruction-light-probe

Everything generated (objects, two linked probes, Xvfb/XDG/pipeline caches,
runtime logs, screenshots) stays under $HALO2_TEST_SCRATCH and is removed by
run_test.py. Only small JSON/text receipts are written to --report-dir.

The MAIN frozen build is read-only input: its acceptance link.txt/flags.make,
static archives and its acceptance-compiled multiplayer_runtime.cpp.o. The
probe object replaces tests/physics_multiplayer_acceptance.cpp.o. Variant
'old' links MAIN's libhalo2_systems.a as-is (OLD NativeWorldEffects light
catalog). Variant 'fixed' additionally links an object compiled in scratch from
MAIN/world-effects-final-catalog.cpp with halo2_systems' exact flags, which
supersedes the archive member (traced with -Wl,-y). MAIN is never relinked.
"""
import argparse, concurrent.futures, hashlib, json, os, re, shlex, subprocess, sys, time
from pathlib import Path

ROOT = Path('/home/eric/Developer/Halo-2-Decompiled')
HERE = Path(__file__).resolve().parent
PROBE = HERE / 'ghost_destruction_light_probe.cpp'
LIGHT_DEFINITIONS = '_ZNK5halo218NativeWorldEffects17light_definitionsEv'
KEYS = ('boundary:', 'not admitted', 'large-bound', 'light receiver', 'probe_', 'error',
        'world effect', 'plasma', 'exception', 'terminate', 'vk_error', 'segmentation')
COUNT_KEYS = ('selected model light pass was not admitted during preload',
              'Selected BSP light material was not admitted during preload',
              'BSP effect light material boundary', 'large-bound', 'Native light receiver',
              'pStencil direct light boundary', 'World effect boundary')


def sha(path):
    with open(path, 'rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def flags(path):
    values = {}
    for line in Path(path).read_text().splitlines():
        m = re.match(r'(CXX_DEFINES|CXX_INCLUDES|CXX_FLAGS) = (.*)', line)
        if m:
            values[m[1]] = shlex.split(m[2])
    return values['CXX_DEFINES'] + values['CXX_INCLUDES'] + values['CXX_FLAGS']


def run(command, log, cwd=None, env=None, timeout=None):
    started = time.monotonic()
    with open(log, 'w') as stream:
        try:
            r = subprocess.run(command, cwd=cwd, env=env, stdout=stream, stderr=subprocess.STDOUT, timeout=timeout)
            code = r.returncode
        except subprocess.TimeoutExpired:
            code = 'timeout'
    return {'command': [str(c) for c in command], 'cwd': str(cwd) if cwd else None, 'exit': code,
            'seconds': round(time.monotonic() - started, 1)}


def tail(path, size=4000):
    text = Path(path).read_text(errors='replace') if Path(path).exists() else ''
    return text[-size:]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--main', type=Path, required=True)
    ap.add_argument('--report-dir', type=Path, required=True)
    ap.add_argument('--variants', default='old,fixed')
    ap.add_argument('--frames', default='120')
    ap.add_argument('--timeout', type=int, default=900)
    ap.add_argument('--cwd', type=Path, default=ROOT, help='runtime working directory (Codex candidates used ROOT)')
    args = ap.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    main_dir = args.main.resolve()
    build, source = main_dir / 'build', main_dir / 'source'
    report_dir = args.report_dir.resolve()
    report_dir.mkdir(parents=True, exist_ok=True)
    cache = ROOT / 'assets/imported/mcc-zanzibar-tu15-20261101-v9'
    maps = cache / 'halo2/h2_maps_win64_dx11'
    acceptance = build / 'CMakeFiles/physics_multiplayer_acceptance.dir'
    runtime_obj = acceptance / 'src/gameplay/multiplayer/multiplayer_runtime.cpp.o'
    fixed_src = main_dir / 'world-effects-final-catalog.cpp'
    old_src = source / 'src/rendering/effects/world_effects.cpp'
    obj, bins = scratch / 'obj', scratch / 'bin'
    obj.mkdir(); bins.mkdir()
    summary = {'scratch': str(scratch), 'main': str(main_dir), 'runtime_cwd': str(args.cwd),
               'variants': {}, 'pins': {}, 'compile': {}, 'link': {}}

    link_tokens = shlex.split((acceptance / 'link.txt').read_text())
    archives = sorted({t for t in link_tokens if t.endswith('.a')})
    archive_paths = [Path(a) if os.path.isabs(a) else build / a for a in archives]

    def archive_pins():
        return {str(p.relative_to(build)) if p.is_relative_to(build) else str(p): sha(p) for p in archive_paths}

    pins = {'probe_cpp': sha(PROBE), 'run_probe_py': sha(Path(__file__)),
            'main_acceptance_link_txt': sha(acceptance / 'link.txt'),
            'main_acceptance_flags_make': sha(acceptance / 'flags.make'),
            'main_systems_flags_make': sha(build / 'CMakeFiles/halo2_systems.dir/flags.make'),
            'main_acceptance_multiplayer_runtime_o': sha(runtime_obj),
            'main_old_world_effects_cpp': sha(old_src), 'main_final_catalog_world_effects_cpp': sha(fixed_src),
            'zanzibar_map': sha(maps / 'zanzibar.map'), 'shared_map': sha(maps / 'shared.map'),
            'compiler': subprocess.run(['/usr/bin/c++', '--version'], capture_output=True, text=True).stdout.splitlines()[0]}
    before_archives = archive_pins()
    pins['archives_before_link'] = before_archives
    summary['pins'] = pins

    acc_flags = flags(acceptance / 'flags.make')
    sys_flags = flags(build / 'CMakeFiles/halo2_systems.dir/flags.make') + ['-fno-fast-math', '-ffp-contract=off']
    compiles = {
        'probe': ['/usr/bin/c++', *acc_flags, f'-I{source / "tests"}', '-c', str(PROBE), '-o', str(obj / 'probe.o')],
        'fixed_world_effects': ['/usr/bin/c++', *sys_flags, '-iquote', str(old_src.parent), '-c', str(fixed_src),
                                '-o', str(obj / 'world_effects_fixed.o')],
        'old_world_effects_check': ['/usr/bin/c++', *sys_flags, '-c', str(old_src), '-o', str(obj / 'world_effects_old.o')],
    }
    with concurrent.futures.ThreadPoolExecutor(3) as pool:
        futures = {k: pool.submit(run, c, obj / f'{k}.log') for k, c in compiles.items()}
        for k, f in futures.items():
            summary['compile'][k] = f.result()
            if summary['compile'][k]['exit'] != 0:
                summary['compile'][k]['log_tail'] = tail(obj / f'{k}.log')
    (report_dir / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    if any(v['exit'] != 0 for v in summary['compile'].values()):
        print(json.dumps({k: v['exit'] for k, v in summary['compile'].items()}))
        return 2

    # Prove the archive member is the OLD catalog object.
    members = subprocess.run(['ar', 't', str(build / 'libhalo2_systems.a')], capture_output=True, text=True).stdout.split()
    member = subprocess.run(['ar', 'p', str(build / 'libhalo2_systems.a'), 'world_effects.cpp.o'], capture_output=True).stdout
    (obj / 'archive_world_effects.o').write_bytes(member)
    def sym_size(path):
        out = subprocess.run(['nm', '-S', str(path)], capture_output=True, text=True).stdout
        for line in out.splitlines():
            if line.endswith(' ' + LIGHT_DEFINITIONS):
                return int(line.split()[1], 16)
        return None
    summary['archive_member_check'] = {
        'member_count': members.count('world_effects.cpp.o'),
        'archive_member_sha256': hashlib.sha256(member).hexdigest(),
        'fresh_old_compile_sha256': sha(obj / 'world_effects_old.o'),
        'fresh_fixed_compile_sha256': sha(obj / 'world_effects_fixed.o'),
        'archive_member_equals_fresh_old': member == (obj / 'world_effects_old.o').read_bytes(),
        'light_definitions_size': {'archive': sym_size(obj / 'archive_world_effects.o'),
                                   'old': sym_size(obj / 'world_effects_old.o'),
                                   'fixed': sym_size(obj / 'world_effects_fixed.o')}}

    variants = [v for v in args.variants.split(',') if v]
    for variant in variants:
        out = bins / f'probe-{variant}'
        tokens = []
        skip = False
        for t in link_tokens:
            if t.startswith('-Wl,--dependency-file='):
                continue
            if skip:
                tokens.append(str(out)); skip = False; continue
            if t == '-o':
                tokens.append(t); skip = True; continue
            if t == 'CMakeFiles/physics_multiplayer_acceptance.dir/tests/physics_multiplayer_acceptance.cpp.o':
                tokens.append(str(obj / 'probe.o')); continue
            if t == 'CMakeFiles/physics_multiplayer_acceptance.dir/src/gameplay/multiplayer/multiplayer_runtime.cpp.o':
                tokens.append(str(runtime_obj))
                if variant == 'fixed':
                    tokens.append(str(obj / 'world_effects_fixed.o'))
                continue
            tokens.append(t)
        tokens.insert(1, f'-Wl,-y,{LIGHT_DEFINITIONS}')
        result = run(tokens, scratch / f'link-{variant}.log', cwd=build)
        trace = [l for l in Path(scratch / f'link-{variant}.log').read_text(errors='replace').splitlines() if LIGHT_DEFINITIONS in l]
        result['light_definitions_trace'] = trace[:20]
        if result['exit'] != 0:
            result['log_tail'] = tail(scratch / f'link-{variant}.log')
        else:
            result['binary_sha256'] = sha(out)
        summary['link'][variant] = result
    after_archives = archive_pins()
    summary['pins']['archives_unchanged_during_link'] = after_archives == before_archives
    (report_dir / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    if any(v['exit'] != 0 for v in summary['link'].values()) or after_archives != before_archives:
        print('link failed or MAIN archives changed during link')
        return 3

    for variant in variants:
        work = scratch / f'run-{variant}'
        work.mkdir()
        env = os.environ.copy()
        for key, name in [('XDG_DATA_HOME', 'data'), ('XDG_CONFIG_HOME', 'config'), ('XDG_CACHE_HOME', 'cache'),
                          ('HALO2_PIPELINE_CACHE_DIRECTORY', 'pipelines'), ('MESA_SHADER_CACHE_DIR', 'mesa'),
                          ('__GL_SHADER_DISK_CACHE_PATH', 'nvidia')]:
            (work / name).mkdir()
            env[key] = str(work / name)
        env['SDL_VIDEODRIVER'] = 'x11'
        env['HALO2_PROBE_FRAMES'] = args.frames
        command = ['xvfb-run', '-a', '-s', '-screen 0 1280x720x24 -nolisten tcp', str(bins / f'probe-{variant}'),
                   str(maps / 'zanzibar.map'), str(maps), str(work / 'out'), str(work / 'report.json')]
        result = run(command, work / 'runtime.log', cwd=args.cwd, env=env, timeout=args.timeout)
        result['environment'] = {'SDL_VIDEODRIVER': 'x11', 'HALO2_PROBE_FRAMES': args.frames,
                                 'XDG/pipeline caches': 'fresh under scratch'}
        report = json.loads((work / 'report.json').read_text()) if (work / 'report.json').exists() else None
        text = (work / 'runtime.log').read_text(errors='replace') if (work / 'runtime.log').exists() else ''
        matched = [l[:1500] for l in text.splitlines() if any(k in l.lower() for k in KEYS)]
        excerpt = matched if len(matched) <= 300 else matched[:150] + [f'... {len(matched) - 300} matching lines omitted ...'] + matched[-150:]
        (report_dir / f'{variant}-log-excerpt.txt').write_text('\n'.join(excerpt) + '\n')
        result['log_counts'] = {k: text.count(k) for k in COUNT_KEYS}
        result['bsp_material_gap_lights'] = sorted({hex(int(m)) for m in re.findall(r'BSP effect light material boundary: (\d+)/', text)})
        result['log_bytes'] = len(text.encode())
        if report is not None:
            (report_dir / f'{variant}-report.json').write_text(json.dumps(report, indent=2) + '\n')
        result['log_tail'] = text[-3000:]
        result['verdict'] = report.get('verdict') if report else None
        result['probe_error'] = report.get('probe_error') if report else None
        result['run_error'] = (report or {}).get('render_phase', {}).get('run_error')
        result['log_has_exact_key'] = ('selected model light pass was not admitted during preload' in text and
                                       'light=3887483049 ' in text and 'known_light=0' in text)
        summary['variants'][variant] = result
        (report_dir / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
        print(json.dumps({'variant': variant, 'exit': result['exit'], 'seconds': result['seconds'],
                          'verdict': result['verdict'], 'probe_error': result['probe_error'],
                          'run_error': (result['run_error'] or '')[:400]}), flush=True)
    red = summary['variants'].get('old', {}).get('verdict') or {}
    green = summary['variants'].get('fixed', {}).get('verdict') or {}
    ok = (('old' not in variants or red.get('red_exact_plasma_preload_fatal')) and
          ('fixed' not in variants or green.get('green_no_throw_with_plasma_prepared_draws')))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
