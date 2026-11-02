"""Promotion policy tests. Synthetic recorders establish no gameplay parity."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import build_launcher_engine as engine
import native_renderer_shaders as native_shaders
import stage_native_renderer_shaders as native_staging
import custom_assets
from native_variant_test_support import variant_fixture
from custom_ui_test_support import custom_ui_fixture, empty_custom_map_fixture
from test_controller_prompt_assets import controller_prompt_fixture


class EosConfigurationSelectionTests(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory()
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name)
        self.local = self.root / '.local/eos-setup/client.env'
        self.local.parent.mkdir(parents=True)
        self.local.write_text('synthetic private build input')
        self.runtime = self.root / 'bin/engine-builds/previous/halo2_native'
        self.runtime.parent.mkdir(parents=True)
        self.prior = {'runtime': self.runtime.relative_to(self.root).as_posix()}

    def test_ordinary_rebuild_keeps_the_selected_online_configuration(self):
        packaged = self.runtime.with_name('eos-client.env')
        packaged.write_text('synthetic packaged game input')
        self.assertEqual(engine.eos_client_configuration(self.root, None, self.prior), packaged)

    def test_local_online_setup_is_used_when_a_previous_build_lost_configuration(self):
        self.assertEqual(engine.eos_client_configuration(self.root, None, self.prior), self.local)
        self.assertEqual(engine.eos_client_configuration(self.root, None, None), self.local)

    def test_explicit_configuration_wins_and_unconfigured_development_stays_available(self):
        explicit = self.root / 'explicit.env'
        explicit.write_text('synthetic explicit game input')
        self.assertEqual(engine.eos_client_configuration(self.root, explicit, self.prior), explicit)
        self.local.unlink()
        self.assertIsNone(engine.eos_client_configuration(self.root, None, self.prior))


class EnginePromotionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Actual small Vulkan modules keep the publication fixture compatible
        # with the production stager and reader. They prove admission policy,
        # not the rendered behavior of a game shader.
        compiler = shutil.which('glslangValidator')
        if not compiler:
            raise unittest.SkipTest('Vulkan shader compiler unavailable')
        scratch = Path(os.environ['HALO2_TEST_SCRATCH']) / 'engine-admission-modules'
        scratch.mkdir()
        cls.modules = {}
        sources = {
            'vert': '#version 450\nvoid main(){gl_Position=vec4(0,0,0,1);}',
            'geom': '#version 450\nlayout(points)in;layout(points,max_vertices=1)out;void main(){gl_Position=gl_in[0].gl_Position;EmitVertex();EndPrimitive();}',
            'frag': '#version 450\nlayout(location=0)out vec4 c;void main(){c=vec4(1);}',
            'comp': '#version 450\nlayout(local_size_x=1)in;void main(){}',
        }
        for stage, source in sources.items():
            path = scratch / ('module.' + stage)
            path.write_text(source)
            target = path.with_name(path.name + '.spv')
            subprocess.run([compiler, '-V', str(path), '-o', str(target)],
                           check=True, stdout=subprocess.DEVNULL)
            cls.modules[stage] = target.read_bytes()

    def setUp(self):
        self.owner = tempfile.TemporaryDirectory()
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name) / 'checkout'
        self.root.mkdir()
        variant_fixture(self.root)
        custom_ui_fixture(self.root)
        empty_custom_map_fixture(self.root)
        controller_prompt_fixture(self.root)
        asset_pins = custom_assets.inventory(self.root)
        self.addCleanup(lambda: self.assertEqual(custom_assets.inventory(self.root), asset_pins,
            'Engine promotion must preserve the original custom asset sources'))
        (self.root / 'CMakeLists.txt').write_text('Synthetic policy fixture')
        (self.root / 'tests').mkdir()
        (self.root / 'cmake').mkdir()
        (self.root / 'cmake/NativeRendererProductionShaders.cmake').write_text(
            'set(native_renderer_fixed_shaders\n' +
            '\n'.join(sorted(native_shaders.AOT_FIXED_FILES)) + '\n)\n')
        for name in native_shaders.AOT_FIXED_FILES:
            origin = self.root / 'shaders' / native_staging.ALIASES.get(name, name[:-4])
            origin.parent.mkdir(parents=True, exist_ok=True)
            origin.write_text('Synthetic source owner for ' + name)
        self.cache = self.root / 'assets/imported/mcc-test'
        self.maps = self.cache / 'halo2/h2_maps_win64_dx11'
        self.maps.mkdir(parents=True)
        (self.maps / 'lockout.map').write_bytes(b'synthetic map')
        (self.maps / 'coagulation.map').write_bytes(b'synthetic frag control map')
        (self.maps / 'midship.map').write_bytes(b'synthetic plasma control map')
        (self.maps / 'zanzibar.map').write_bytes(b'synthetic receiver control map')
        for name in engine.SMG_WARTHOG_SHARED:
            (self.maps / name).write_bytes(('Synthetic shared policy input: ' + name).encode())
        self.smg_input = self.root / engine.SMG_WARTHOG_INPUT
        self.smg_input.parent.mkdir(parents=True)
        self.smg_input.write_text('# Synthetic promotion policy schedule; no gameplay claim\n' +
            '\n'.join(f'{tick} 0 {int(100 <= tick < 400)} {int(tick == 410)} {int(tick == 80)} 0 0 move 0 0'
                      for tick in range(1399)) + '\n')
        self.smg_metadata = self.root / engine.SMG_WARTHOG_METADATA
        self.smg_metadata.write_text(json.dumps(dict(capture_tick=1400,
            map_sha256=engine.digest(self.maps / 'coagulation.map'),
            input_script_sha256=engine.digest(self.smg_input),
            scope='Synthetic promotion policy metadata; no native input or gameplay qualification')))
        # The audio regression now requires mainmenu.map as a real shared input.
        # Keep this synthetic promotion fixture independent of frontend captures;
        # dedicated publication controls below replace this recorder explicitly.
        frontend = patch('qualify_frontend_runtime.qualify', side_effect=lambda root, runtime, *_:
            dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime),
                 scope='Synthetic frontend policy recorder; no rendered qualification'))
        frontend.start()
        self.addCleanup(frontend.stop)
        self.scratch = Path(self.owner.name) / 'scratch'
        self.scratch.mkdir()
        self.runtime = self.scratch / 'recorder'
        self.args = argparse.Namespace(root=self.root, output=self.root / 'reports', jobs=1, timeout=1)
        self.selection = self.root / 'gameplay-runtime-selection.json'
        self.selection.write_text('existing selection retained verbatim')
        self.original_selection = self.selection.read_bytes()
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            (self.root / name).write_text('previous launcher')
        self.launcher_modes = {name: (self.root / name).stat().st_mode & 0o777
                               for name in ('Halo2-Launcher', 'Halo2-Title-Launcher')}

    def record(self, text):
        self.runtime.write_text('#!' + sys.executable + '\n' + text)
        self.runtime.chmod(0o755)

    def test_exit_success_without_four_submitted_frames_is_rejected(self):
        for text in ('pass', 'print("Multiplayer frames=0 ticks=4")',
                     'print("Multiplayer frames=3 ticks=4")'):
            self.record(text)
            result = engine.smoke(self.runtime, self.maps, 'lockout.map', 1, self.scratch, 1)
            self.assertFalse(result['passed'])

    def test_nonzero_exit_and_timeout_never_qualify_even_with_a_frame_line(self):
        for text, timeout in (('print("Multiplayer frames=4 ticks=4"); raise SystemExit(1)', 1),
                              ('import time; time.sleep(5)', .05)):
            self.record(text)
            self.assertFalse(engine.smoke(self.runtime, self.maps, 'lockout.map', 1,
                                          self.scratch, timeout)['passed'])

    def test_frame_submission_also_requires_advancing_state_and_all_local_players(self):
        for ticks, players, expected in ((0, 4, False), (4, 1, False), (4, 4, True)):
            self.record(f'print("Multiplayer frames=4 ticks={ticks} players=4 connected=1 peak_players={players}")')
            self.assertEqual(engine.smoke(self.runtime, self.maps, 'lockout.map', 4,
                                          self.scratch, 1)['passed'], expected)

    def test_smoke_uses_original_graphics_and_owned_preferences(self):
        observation = self.scratch / 'smoke-observation.json'
        self.record('import json,os,sys\nfrom pathlib import Path\n'
                    f'Path({str(observation)!r}).write_text(json.dumps(dict(args=sys.argv[1:],'
                    'data=os.environ.get("XDG_DATA_HOME"),config=os.environ.get("XDG_CONFIG_HOME"),'
                    'cache=os.environ.get("XDG_CACHE_HOME"),'
                    'halo2={k:v for k,v in os.environ.items() if k.startswith("HALO2_")},'
                    'sdk=os.environ.get("EOS_POLICY_TEST"))))\n'
                    'print("Multiplayer frames=4 ticks=4 players=1 connected=1 peak_players=1")')
        with patch.dict(os.environ, XDG_DATA_HOME='/foreign-user-data', XDG_CONFIG_HOME='/foreign-user-config',
                        HALO2_NATIVE_DYNAMIC_OBJECT_POSE_CACHE='1', HALO2_ENABLE_GPU_SKINNING='0',
                        HALO2_NATIVE_SKIN_ADMISSION_CACHE='0', HALO2_NATIVE_BANDED_CULL='0',
                        HALO2_GPU_OBJECT_SKINS='1', HALO2_MODEL_SKIN_DIAGNOSTICS='foreign-path',
                        HALO2_KEEP_TEST_ARTIFACTS='1', HALO2_TEST_SCRATCH='/foreign-owner',
                        EOS_POLICY_TEST='preserved-sdk-setting'):
            self.assertTrue(engine.smoke(self.runtime, self.maps, 'lockout.map', 1,
                                         self.scratch, 1)['passed'])
        observed = json.loads(observation.read_text())
        self.assertEqual(observed['args'][observed['args'].index('--graphics-mode') + 1], 'Original')
        preferences = Path(observed['data']).parent
        self.assertEqual(preferences.parent, self.scratch / 'smoke-preferences')
        self.assertRegex(preferences.name, '^[0-9a-f]{32}$')
        for key, suffix in (('data', 'data'), ('config', 'config'), ('cache', 'cache')):
            self.assertEqual(observed[key], str(preferences / suffix))
        self.assertEqual(observed['halo2'], dict(HALO2_TEST_SCRATCH=str(self.scratch),
            HALO2_PIPELINE_CACHE_DIRECTORY=str(self.scratch / 'vulkan-cache')))
        self.assertEqual(observed['sdk'], 'preserved-sdk-setting')

    def gate_controls(self):
        self.args.renderer_4k_reference_control = ['HALO2_REFERENCE_COPY_FP_GRAPH_METADATA=1',
            'HALO2_REFERENCE_BODY_POSE_JOURNAL=1', 'HALO2_REFERENCE_DYNAMIC_OBJECT_POSE_CACHE=1']
        return engine.renderer_reference_controls(self.args.renderer_4k_reference_control)

    def test_packaged_4k_gate_runs_on_the_exact_published_runtime(self):
        controls = self.gate_controls()
        observed = []
        def gate(source, runtime, maps, output, scratch, reference):
            self.assertEqual(source, self.scratch / 'source')
            self.assertEqual(maps, self.maps); self.assertEqual(scratch, self.scratch)
            self.assertEqual(reference, controls)
            self.assertTrue((runtime.parent / 'shaders/production-shaders.json').is_file())
            self.assertEqual(self.selection.read_bytes(), self.original_selection)
            observed.append(runtime)
            return dict(passed=True, runtime=str(runtime), runtime_sha256=engine.digest(runtime),
                        reference_controls=reference, parity_report='synthetic passing policy fixture')
        with patch.object(engine, 'renderer_4k_qualification', side_effect=gate):
            self.attempt(dict(passed=True))
        record = json.loads(self.selection.read_text())
        self.assertEqual(observed, [self.root / record['runtime']])
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertTrue(report['renderer_4k']['passed'])
        self.assertEqual(report['renderer_4k']['runtime_sha256'], record['runtime_sha256'])
        self.assertTrue(json.loads((observed[0].parent / 'qualification.json').read_text())['renderer_4k']['passed'])

    def test_failed_packaged_4k_gate_cannot_be_accepted_as_a_smoke_exception(self):
        self.gate_controls(); self.args.accept_smoke_failure = ['lockout.map:1', 'coagulation.map:4']
        with patch.object(engine, 'renderer_4k_qualification', return_value=dict(passed=False, error='required parity failed')):
            with self.assertRaisesRegex(RuntimeError, '4K frame qualification failed'):
                self.attempt(dict(passed=True))
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        for name in self.launcher_modes:
            self.assertEqual((self.root / name).read_text(), 'previous launcher')
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])

    def test_packaged_4k_gate_keeps_runtime_source_and_publication_guards(self):
        for mutation in ('runtime', 'source', 'selection'):
            with self.subTest(mutation=mutation):
                self.gate_controls()
                self.selection.write_bytes(self.original_selection)
                def gate(source, runtime, *_):
                    if mutation == 'runtime': runtime.write_bytes(b'foreign runtime after frame qualification')
                    elif mutation == 'source': (source / 'CMakeLists.txt').write_text('changed frozen cohort')
                    else: self.selection.write_text('foreign selection during supplemental qualification')
                    return dict(passed=True)
                with patch.object(engine, 'renderer_4k_qualification', side_effect=gate):
                    with self.assertRaises(RuntimeError):
                        self.attempt(dict(passed=True))
                expected = b'foreign selection during supplemental qualification' if mutation == 'selection' else self.original_selection
                self.assertEqual(self.selection.read_bytes(), expected)
                self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])
                # The test worker freezes a new source owner for each attempt.
                shutil.rmtree(self.scratch / 'source')

    def test_renderer_reference_controls_reject_ambiguous_or_nonreference_assignments(self):
        for assignments in (['PATH=x'], ['HALO2_REFERENCE_TEST'], ['HALO2_REFERENCE_TEST='],
                            ['HALO2_REFERENCE_TEST=1,OTHER=2'], ['HALO2_REFERENCE_TEST=1@runtime'],
                            ['HALO2_REFERENCE_TEST=1\n'], ['HALO2_REFERENCE_TEST=1', 'HALO2_REFERENCE_TEST=2']):
            with self.subTest(assignments=assignments), self.assertRaises(ValueError):
                engine.renderer_reference_controls(assignments)

    def test_renderer_4k_report_policy_rechecks_complete_cases_repeats_and_pins(self):
        source = self.scratch / 'gate-source'
        helper = source / 'tools/perf_4k60/parity.py'
        helper.parent.mkdir(parents=True); helper.write_text('Synthetic parity-report policy recorder')
        runtime = self.scratch / 'gate-runtime/halo2_native'
        runtime.parent.mkdir(); (self.maps / 'coagulation.map').write_bytes(b'synthetic coagulation')
        controls = {'HALO2_REFERENCE_BODY_POSE_JOURNAL': '1'}
        def report():
            cases = []
            for index, case in enumerate(engine.RENDERER_4K_CASES):
                roles = ['reference', 'reference-repeat', 'candidate', 'candidate-repeat'] if not index else ['reference', 'candidate']
                cases.append(dict(case=case, passed=True, captures_valid=True, identical=True,
                                  reference_deterministic=True, candidate_deterministic=True,
                                  runs=[dict(role=role, valid=True, exit_code=0, sha256='a' * 64,
                                             bytes=3840 * 2160 * 3 + 17) for role in roles]))
            return dict(passed=True, reference=str(runtime), candidate=str(runtime),
                        reference_env=controls, candidate_env={}, maps=str(self.maps), map='coagulation',
                        map_path=str(self.maps / 'coagulation.map'), map_sha256=engine.digest(self.maps / 'coagulation.map'),
                        native_size=[3840, 2160], reference_sha256=engine.digest(runtime),
                        candidate_sha256=engine.digest(runtime), helper_sha256=engine.digest(helper), cases=cases)
        for mode in ('pass', 'missing', 'failed', 'wrong_runtime', 'wrong_helper', 'wrong_map',
                     'missing_repeat', 'invalid_capture', 'failed_capture', 'different_hash',
                     'wrong_size', 'partial_cases', 'runtime_changed', 'malformed_cases',
                     'malformed_runs', 'oversized_report'):
            with self.subTest(mode=mode):
                runtime.write_bytes(b'same exact packaged candidate')
                output = self.scratch / ('gate-output-' + mode)
                def execute(command, **kwargs):
                    self.assertEqual(kwargs['cwd'], runtime.parent)
                    self.assertEqual(kwargs['env']['H2_REPEAT_CANDIDATE'], '1')
                    self.assertEqual(kwargs['env']['H2_MAP'], 'coagulation')
                    self.assertEqual({key: value for key, value in kwargs['env'].items()
                                      if key.startswith('HALO2_')},
                                     {'HALO2_TEST_SCRATCH': str(self.scratch)})
                    self.assertEqual(kwargs['env']['EOS_POLICY_TEST'], 'preserved-sdk-setting')
                    self.assertEqual(command[command.index(str(helper)) + 2],
                                     'HALO2_REFERENCE_BODY_POSE_JOURNAL=1@' + str(runtime))
                    data = report()
                    if mode == 'failed': data['passed'] = False
                    elif mode == 'wrong_runtime': data['candidate'] = '/foreign/runtime'
                    elif mode == 'wrong_helper': data['helper_sha256'] = 'b' * 64
                    elif mode == 'wrong_map': data['map_sha256'] = 'b' * 64
                    elif mode == 'missing_repeat': data['cases'][0]['runs'].pop()
                    elif mode == 'invalid_capture': data['cases'][1]['runs'][1]['valid'] = False
                    elif mode == 'failed_capture': data['cases'][1]['runs'][1]['exit_code'] = 1
                    elif mode == 'different_hash': data['cases'][1]['runs'][1]['sha256'] = 'b' * 64
                    elif mode == 'wrong_size': data['cases'][1]['runs'][1]['bytes'] = 1280 * 720 * 3 + 16
                    elif mode == 'partial_cases': data['cases'].pop()
                    elif mode == 'malformed_cases': data['cases'] = [1]
                    elif mode == 'malformed_runs': data['cases'][1]['runs'] = [1]
                    if mode != 'missing': (output / 'parity.json').write_text(json.dumps(data))
                    if mode == 'oversized_report': (output / 'parity.json').write_bytes(b'x' * (1024 * 1024 + 1))
                    if mode == 'runtime_changed': runtime.write_bytes(b'foreign candidate rewrite during captures')
                    return subprocess.CompletedProcess(command, 0)
                with patch.dict(os.environ, HALO2_REFERENCE_BODY_POSE_JOURNAL='inherited must not affect candidate',
                                HALO2_NATIVE_DYNAMIC_OBJECT_POSE_CACHE='1', HALO2_ENABLE_GPU_SKINNING='0',
                                HALO2_NATIVE_SKIN_ADMISSION_CACHE='0', HALO2_NATIVE_BANDED_CULL='0',
                                HALO2_GPU_OBJECT_SKINS='1', HALO2_RENDER_GPU_TIMINGS='/foreign-output',
                                HALO2_KEEP_TEST_ARTIFACTS='1', HALO2_TEST_SCRATCH='/foreign-owner',
                                EOS_POLICY_TEST='preserved-sdk-setting'), \
                     patch.object(engine.subprocess, 'run', side_effect=execute):
                    result = engine.renderer_4k_qualification(source, runtime, self.maps, output,
                                                              self.scratch, controls)
                self.assertEqual(result['passed'], mode == 'pass', result)
                self.assertEqual(json.loads((output / 'report.json').read_text())['passed'], mode == 'pass')
                if mode == 'oversized_report':
                    self.assertFalse((output / 'parity.json').exists())

    def build(self, command, **kwargs):
        build = self.scratch / 'build'
        build.mkdir(exist_ok=True)
        self.assertIn('--cmake-arg=-DHALO2_STAGE_CUSTOM_ASSETS=ON', command)
        self.assertIn('--cmake-arg=-DHALO2_CUSTOM_ASSET_ROOT=' + str(self.root), command)
        self.assertIn('--cmake-arg=-DHALO2_TEST_ASSET_SOURCE_ROOT=' + str(self.root), command)
        self.assertIn('frontend_online_tests', command)
        self.assertIn('loop_audio_locations_tu15_tests', command)
        self.assertIn('mcc_resident_reflection_template_tests', command)
        test_cache = Path(next(value.split('=', 2)[2] for value in command
            if value.startswith('--cmake-arg=-DHALO2_MCC_PARTY_UI_TEST_CACHE=')))
        test_map = Path(next(value.split('=', 2)[2] for value in command
            if value.startswith('--cmake-arg=-DHALO2_MCC_MULTIPLAYER_ENGINE_MAP=')))
        self.assertEqual(test_map.parent, test_cache/'halo2/h2_maps_win64_dx11')
        if not getattr(self, 'omit_build_custom_assets', False):
            custom_assets.install(self.root, build / 'custom-assets')
        for name in ('halo2_native', 'Halo2-Launcher', 'Halo2-Title-Launcher', 'eos_sdk_worker'):
            (build / name).write_text('synthetic new binary')
            (build / name).chmod(0o755)
        shader_directory = next(v.split('=', 2)[2] for v in command
                                if v.startswith('--cmake-arg=-DHALO2_RUNTIME_SHADER_DIR='))
        generated = ['native_world/v-' + '1' * 64 + '.vert.spv',
                     'native_world/f-' + '2' * 64 + '.frag.spv',
                     'native-fragments/hud/' + '3' * 64 + '.frag.spv']
        (build / 'halo2_native').write_bytes(b'Synthetic admission input: ' + shader_directory.encode() +
            b'\0' + b'\0'.join(name.encode() for name in generated) + b'\0')
        (build / 'shaders').mkdir(exist_ok=True)
        names = sorted(native_shaders.AOT_FIXED_FILES | set(generated))
        rows = []
        for name in names:
            target = build / 'shaders' / name
            target.parent.mkdir(parents=True, exist_ok=True)
            payload = self.modules[name.rsplit('.', 2)[-2]]
            target.write_bytes(payload)
            if name in generated:
                rows.append(dict(path=name, bytes=len(payload), sha256=hashlib.sha256(payload).hexdigest()))
        (build / 'CMakeCache.txt').write_text(
            'CMAKE_HOME_DIRECTORY:INTERNAL=' + str(self.scratch / 'source') + '\n')
        (build / 'production-shaders.txt').write_text('\n'.join(names) + '\n')
        for name, manifest in {
            'native-world-kernel-catalog/compiled.json': dict(
                schema='HALO2_NATIVE_WORLD_KERNEL_COMPILED_V1', runtime_instruction_interpreter=False,
                stages=[row for row in rows if row['path'].startswith('native_world/')]),
            'native-fragment-catalog/compiled.json': dict(
                schema='HALO2_COMPILED_NATIVE_FRAGMENT_KERNELS_V1', passed=True,
                modules=[row for row in rows if row['path'].startswith('native-fragments/')]),
        }.items():
            target = build / name
            target.parent.mkdir(exist_ok=True)
            target.write_text(json.dumps(manifest))
        (build / 'engine-targets.txt').write_text('halo2_native|halo2_systems||src/app/main.cpp\n'
                                                'halo2_systems|||src/engine.cpp\n')
        output=Path(command[command.index('--output')+1]);output.mkdir(parents=True,exist_ok=True)
        build_only='--build-only' in command
        (output/'report.json').write_text(json.dumps(dict(
            passed=not build_only,status='built' if build_only else 'passed',
            validation_not_run=build_only,
            steps=[dict(name=name,exit_code=0) for name in
                   (('configure','build') if build_only else ('configure','build','tests'))])))
        if not build_only:
            test_maps = Path(next(value.split('=', 2)[2] for value in command
                if value.startswith('--cmake-arg=-DHALO2_MCC_MULTIPLAYER_ENGINE_MAP='))).parent
            evidence=output/'evidence/multiplayer-menu-startup.json'
            evidence.parent.mkdir(exist_ok=True)
            evidence.write_text(json.dumps(dict(passed=True,frames=1,
                runtime_sha256=engine.digest(build/'halo2_native'),
                scope='Synthetic promotion-policy fixture; no gameplay claim')))
            (evidence.parent/'friends-footer-anchor.json').write_text(json.dumps(dict(
                passed=True,cases=[dict(passed=True,device=index//13,resize_case=index%13)
                                   for index in range(39)],rendered_cases=0,
                scope='Synthetic Friends footer promotion-policy fixture; no rendered or gameplay claim')))
            (evidence.parent/'loop-audio-locations.json').write_text(json.dumps(dict(
                passed=True,cases=22,comparisons=44,mode3_cases=6,mode2_cases=16,
                original_xbe_sha256='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd',
                scope='Synthetic loop-location promotion-policy fixture; no original-call or gameplay claim')))
            smg_log = evidence.parent/'smg-warthog-audio.log'
            smg_log.write_text('Synthetic native SMG audio promotion-policy log; no gameplay claim\n')
            source = self.scratch/'source'
            reference = engine.smg_warthog_reference(source, test_maps)
            tick = reference['capture_tick']
            pins = [dict(path=str(item.resolve()), bytes=item.stat().st_size, sha256=engine.digest(item))
                for item in [build/'halo2_native', source/engine.SMG_WARTHOG_INPUT,
                    test_maps/'coagulation.map', *(test_maps/name for name in engine.SMG_WARTHOG_SHARED),
                    source/engine.SMG_WARTHOG_METADATA]]
            native = dict(schema='HALO2_NATIVE_LOOP_AUDIO_LOCATION_V1',
                collector_schema='HALO2_SOURCE_LOOP_AUDIO_LOCATION_TU15_V1', passed=True, qualified=True,
                run_result=0, map=str((test_maps/'coagulation.map').resolve()), graphics_mode='Original',
                runtime_cache_source='MCC', mute=False, requested_capture_tick=tick, final_tick=tick,
                captured_tick=True, input_samples=1399, submitted_frames=1399,
                native_projectile_publications=180, shadow_generate_draws=20, shadow_apply_draws=25,
                target=dict(owner=0xe7890009,definition=0xe89a000a,precondition_observed=True,precondition_tick=91,
                    body_before_smg=.5,minimum_body_after_smg=.2,smg_damage_observed=True,
                    first_smg_command_tick=401,first_smg_damage_tick=402,post_smg_vitality_samples=20),
                precondition=dict(observed=True,grenade_commands=1,body_before=1.0,body_after=.5,
                    grenade_projectiles=[dict(projectile=0xeabc0010,definition=0xebcd0011,type=0,
                        player=0xe1230001,unit=0xe1230002,released=True)],
                    grenade_detonations=[dict(projectile=0xeabc0010,definition=0xebcd0011,
                        player=0xe1230001,unit=0xe1230002,serial=1,tick=90)]),
                player=dict(source_player=0xe1230001,source_unit=0xe1230002,source_weapon=0xe1230003,
                    weapon_definition=0xe2340004,expected_smg_definition=0xe2340004,
                    rounds_consumed=180,shot_transitions=180,reload_commands=2,reload_transfers=2,
                    reload_rounds=120,identity_changes=0,post_pin_non_smg_inputs=0,input_errors=0,input_read_failures=0),
                audio=dict(packets=190,smg_packets=180,reload_packets=2,invalid_packets=0),
                mode3=dict(observer_bound=True,baseline_recorded=True,baseline_frozen=True,
                    entries=2,returns=2,validated_entries=2,successful_returns=2,post_fire_successful_returns=2,
                    new_generation_successful_returns=2,target_owned_post_fire_successful_returns=2,
                    preexisting_target_owned_successful_returns=0,precondition_target_owned_successful_returns=0,
                    precondition_generation_seen=False,precondition_generation_count=0,
                    post_fire_successful_ticks=2,invalid_calls=0,
                    read_failures=0,unfinished_calls=0,late_baseline_inputs=0,first_success_source_tick=500,
                    last_success_source_tick=501,first_shot_source_tick=400,first_shot_tick=401,
                    samples=[dict(tick=at,caller_return=0x18fa4a,effect=0xe3450005,loop=0xe4560006,
                        effect_definition=0xe5670007,loop_definition=0xe6780008,eax=0xe3450001,
                        effect_object_owner=0xe7890009,effect_object_definition=0xe89a000a,
                        new_generation=True,precondition_generation_seen=False,precondition_first_seen_source_tick=0xffffffff,
                        output_bits=[0,0,0,0x3f800000,0,0,0x3f800000]) for at in (500,501)]))
            (evidence.parent/'smg-warthog-audio.json').write_text(json.dumps(dict(
                schema='HALO2_NATIVE_SMG_WARTHOG_AUDIO_REPLAY_V1',passed=True,expect='complete',
                runtime=str((build/'halo2_native').resolve()),runtime_sha256=engine.digest(build/'halo2_native'),
                map=str((test_maps/'coagulation.map').resolve()),map_sha256=reference['map_sha256'],
                shared_maps=str(test_maps.resolve()),input_script=reference['input_script'],
                input_script_sha256=reference['input_script_sha256'],reference_metadata=reference['reference_metadata'],
                reference_metadata_path=reference['reference_metadata_path'],reference_metadata_sha256=reference['reference_metadata_sha256'],
                capture_tick=tick,exit_code=0,frames=1399,ticks=tick,players=1,viewport=[640,480],graphics_mode='Original',
                runtime_cache_source='MCC',mute=False,audio_driver='dummy',scratch_removed=True,inputs_unchanged=True,
                exact_audio_owner_error=False,unknown_option_error=False,native_error_diagnostic=False,
                capture_frame_count=1,retained_large_artifacts=[],input_pins_before=pins,input_pins_after=pins,
                input_projection=dict(legacy_cli_projection_omits_accept=True),
                input_schedule=dict(rows=1399,first_key=0,last_key=1398,contiguous=True,first_fire_key=100,
                    fire_rows=300,reload_rows=1,grenade_rows=1,grenade_keys=[80]),native=native,
                retained_log=dict(bytes=smg_log.stat().st_size,sha256=engine.digest(smg_log),tail_truncated=False),
                shadow_draws={'world_native_shadow_generate_prepared_draws':20,
                              'world_native_shadow_apply_prepared_draws':25},
                capture=dict(width=640,height=480,bytes=640*480*3+len(b'P6\n640 480\n255\n'),sha256='a'*64),
                scope='Synthetic native SMG promotion policy fixture; no runtime or gameplay claim')))
            for kind, (map_name, grenade_type) in engine.GRENADE_SHADOW_CASES.items():
                positive = grenade_type is not None
                grenade_log = evidence.parent/f'grenade-shadow-{kind}.log'
                grenade_log.write_text('Synthetic native grenade/shadow promotion-policy log; no gameplay claim\n')
                (evidence.parent/f'grenade-shadow-{kind}.json').write_text(json.dumps(dict(
                    passed=True, case=kind, runtime=str((build/'halo2_native').resolve()),
                    runtime_sha256=engine.digest(build/'halo2_native'),
                    map=str((test_maps/map_name).resolve()), map_sha256=engine.digest(test_maps/map_name),
                    graphics_mode='Original', viewport=[1280,720], capture_tick=160,
                    exit_code=0,frames=161,ticks=160,players=1,
                    exact_missing_section_error=False,scratch_removed=True,runtime_and_map_unchanged=True,
                    checks={name:True for name in ('exact_tick_and_complete_surface',
                        'actual_shadow_gpu_submissions','native_grenade_lifecycle_or_neutral_control',
                        'no_missing_shadow_section')},
                    shadow_draws={'world_native_shadow_generate_prepared_draws':2,
                                  'world_native_shadow_apply_prepared_draws':3},
                    grenade_ticks=[23,24] if positive else [],
                    grenade_owners=[0xe1230042] if positive else [],
                    grenade_definitions=[0xe2340043] if positive else [],
                    grenade_types=[grenade_type] if positive else [],
                    native_grenade_observations=2 if positive else 0,
                    native_grenade_admitted_observations=2 if positive else 0,
                    detonation_serial=1 if positive else 0,
                    retained_log=dict(bytes=grenade_log.stat().st_size,sha256=engine.digest(grenade_log)),
                    capture=dict(bytes=1280*720*3+len(b'P6\n1280 720\n255\n'),sha256='a'*64),
                    scope='Synthetic grenade promotion-policy fixture; no gameplay claim')))
        return subprocess.CompletedProcess(command, 0)

    def attempt(self, case, build=None, *, reader=None):
        with patch.dict(os.environ, HALO2_TEST_SCRATCH=str(self.scratch)), \
             patch('gameplay_runtime_selection.selected_gameplay_runtime', side_effect=reader, return_value=None), \
             patch('mcc_onboarding.selected', return_value=(self.cache, {})), \
             patch.object(engine.subprocess, 'run', side_effect=build or self.build), \
             patch.object(engine, 'smoke', return_value=case):
            engine.worker(self.args)

    def test_canonical_shader_bank_exists_before_every_required_loading_check(self):
        self.args.prepare_only=True
        manifest=self.cache/'presentation/original-loading-v2/manifest.json'
        manifest.parent.mkdir(parents=True);manifest.write_text('{}')
        commands=[];staged_hashes=[]
        def phases(command,**kwargs):
            commands.append(command.copy())
            directory=Path(next(value.split('=',2)[2] for value in command
                if value.startswith('--cmake-arg=-DHALO2_RUNTIME_SHADER_DIR=')))
            if '--build-only' in command:
                self.assertFalse(directory.exists(),'Compilation must precede staging')
            else:
                self.assertFalse((self.scratch/'build/multiplayer-menu-startup.json').exists(),
                    'A previous menu receipt must be removed before the required CTest run')
                for capture_directory in [self.scratch/'build',self.args.output/'validation/evidence']:
                    for name in ('friends-footer-anchor.json','loop-audio-locations.json',
                                 'smg-warthog-audio.json','smg-warthog-audio.log'):
                        self.assertFalse((capture_directory/name).exists(),
                            'A previous component receipt must be removed before the required CTest run')
                for kind in engine.GRENADE_SHADOW_CASES:
                    for capture_directory in [self.scratch/'build',self.args.output/'validation/evidence']:
                        self.assertFalse((capture_directory/f'grenade-shadow-{kind}.json').exists(),
                            'A previous grenade receipt must be removed before the required CTest run')
                self.assertTrue((directory/'world.vert.spv').is_file(),
                    'Desktop loading tests need their compiled default shader bank before CTest')
                proof=native_shaders.validate_native_package(directory,executable=self.scratch/'build/halo2_native')
                staged_hashes.append(proof['package']['sha256'])
            result=self.build(command,**kwargs)
            if '--build-only' in command:
                (self.scratch/'build/multiplayer-menu-startup.json').write_text(json.dumps(dict(
                    passed=True,frames=99,runtime_sha256='0'*64,
                    scope='Synthetic stale promotion-policy fixture; no gameplay claim')))
                stale=self.args.output/'validation/evidence';stale.mkdir(parents=True,exist_ok=True)
                for capture_directory in [self.scratch/'build',stale]:
                    for name in ('friends-footer-anchor.json','loop-audio-locations.json',
                                 'smg-warthog-audio.json','smg-warthog-audio.log'):
                        (capture_directory/name).write_text('{}')
                for kind in engine.GRENADE_SHADOW_CASES:
                    for directory in [self.scratch/'build',stale]:
                        (directory/f'grenade-shadow-{kind}.json').write_text('{}')
            return result
        self.attempt(dict(passed=True),phases)
        self.assertEqual(len(commands),2)
        self.assertIn('--build-only',commands[0]);self.assertNotIn('--build-only',commands[1])
        filters=[command[command.index('--filter')+1] for command in commands]
        self.assertEqual(filters[0],filters[1])
        for name in ['original_loading_gpu','original_loading_gpu_4_3','h2ek_loading_onboarding',
                     'bundled_loading_ambience','native_vista_loading_adapter','original_vista_loading','original_vista_loading_thread',
                     'launcher_engine_promotion','native_frontend_launcher_routing',
                     'native_mcc_split_screen_menu_startup','native_mcc_grenade_shadow_neutral',
                     'native_mcc_grenade_shadow_frag','native_mcc_grenade_shadow_plasma',
                     'native_mcc_grenade_shadow_receiver',
                     'native_mcc_friends_footer_anchor','original_tu15_loop_audio_locations',
                     'native_mcc_smg_warthog_audio',
                     'mcc_resident_reflection_template','mcc_shared_env_illum',
                     'mcc_native_pc_world_programs','mcc_native_pc_world_programs_generator',
                     'mcc_native_pc_world_programs_source_controls','mcc_native_vehicle_restoration',
                     'mcc_native_vehicle_restoration_generator','mcc_native_character_shared',
                     'mcc_native_character_shared_generator','mcc_native_character_shared_actual_source',
                     'object_function_symbol_namespace','mcc_native_meter','mcc_native_meter_generator',
                     'mcc_native_meter_actual_source','mcc_frontend_submenus','mcc_frontend_selection_stability',
                     'workshop_navigation_content','original_workshop_camera_shared',
                     'mcc_native_env_illum_three_channel','mcc_native_env_illum_three_channel_generator',
                     'mcc_native_tiling_specular','mcc_native_tiling_specular_generator']:
            self.assertIn(name,filters[1])
        self.assertIn('multiplayer-menu-startup.json',commands[1])
        self.assertIn('friends-footer-anchor.json',commands[1])
        self.assertIn('loop-audio-locations.json',commands[1])
        self.assertIn('mcc_native_tiling_specular_probe',commands[1])
        self.assertIn('tiling.json',commands[1])
        self.assertIn('smg-warthog-audio.json',commands[1])
        self.assertIn('smg-warthog-audio.log',commands[1])
        self.assertIn('workshop_navigation_content_tests',commands[1])
        self.assertIn('workshop_camera_shared_tests',commands[1])
        self.assertIn('workshop-navigation-content.json',commands[1])
        report=json.loads((self.args.output/'report.json').read_text())
        runtime=self.root/report['runtime']
        self.assertTrue(report['qualified_candidate'])
        self.assertEqual(engine.digest(runtime.parent/'shaders/production-shaders.json'),staged_hashes[0],
            'Packaging must retain the exact bank exercised by the required tests')
        self.assertTrue(report['validation']['passed'])
        menu=report['split_screen_menu_startup']
        self.assertTrue(menu['passed']);self.assertEqual(menu['frames'],1)
        self.assertEqual(menu['report_sha256'],engine.digest(Path(menu['report'])))
        self.assertEqual(menu['runtime_sha256'],engine.digest(runtime))
        grenade=report['grenade_shadow_residency']
        self.assertTrue(grenade['passed']);self.assertEqual(len(grenade['cases']),4)
        self.assertEqual([case['case'] for case in grenade['cases']],['neutral','frag','plasma','receiver'])
        for case in grenade['cases']:
            self.assertEqual(case['report_sha256'],engine.digest(Path(case['report'])))
            self.assertEqual(case['runtime_sha256'],engine.digest(runtime))
            self.assertIn('grenade-shadow-'+case['case']+'.json',commands[1])
        footer=report['friends_footer_anchor']
        self.assertTrue(footer['passed']);self.assertEqual(footer['cases'],39)
        self.assertEqual(footer['rendered_cases'],0)
        self.assertEqual(footer['report_sha256'],engine.digest(Path(footer['report'])))
        self.assertEqual(footer['source_inventory_sha256'],report['source_cohort']['inventory_sha256'])
        self.assertNotIn('runtime_sha256',footer)
        audio=report['loop_audio_locations']
        self.assertTrue(audio['passed']);self.assertEqual(audio['cases'],22)
        self.assertEqual(audio['comparisons'],44)
        self.assertEqual(audio['report_sha256'],engine.digest(Path(audio['report'])))
        self.assertEqual(audio['source_inventory_sha256'],report['source_cohort']['inventory_sha256'])
        self.assertNotIn('runtime_sha256',audio)
        smg=report['smg_warthog_audio']
        self.assertTrue(smg['passed']);self.assertEqual(smg['capture_tick'],1400)
        self.assertEqual(smg['runtime_sha256'],engine.digest(runtime))
        self.assertEqual(smg['report_sha256'],engine.digest(Path(smg['report'])))
        self.assertEqual(smg['log_sha256'],engine.digest(Path(smg['log'])))
        self.assertEqual(smg['input_script_sha256'],engine.digest(self.smg_input))
        self.assertEqual(smg['reference_metadata_sha256'],engine.digest(self.smg_metadata))
        self.assertGreaterEqual(smg['post_fire_successful_ticks'],2)
        self.assertGreater(smg['new_generation_successful_returns'],0)
        self.assertGreater(smg['rounds_consumed'],60)
        self.assertGreater(smg['reload_transfers'],0)
        self.assertEqual(self.selection.read_bytes(),self.original_selection)

    def test_missing_failed_or_zero_frame_menu_startup_cannot_promote(self):
        for mode in ('missing','failed','zero_frames','missing_runtime_hash','wrong_runtime_hash'):
            with self.subTest(mode=mode):
                def invalid_menu(command,**kwargs):
                    result=self.build(command,**kwargs)
                    if '--build-only' not in command:
                        output=Path(command[command.index('--output')+1])
                        evidence=output/'evidence/multiplayer-menu-startup.json'
                        if mode=='missing': evidence.unlink()
                        else:
                            receipt=json.loads(evidence.read_text())
                            receipt.update(passed=mode!='failed',frames=0 if mode=='zero_frames' else 1)
                            if mode=='missing_runtime_hash':receipt.pop('runtime_sha256')
                            elif mode=='wrong_runtime_hash':receipt['runtime_sha256']='0'*64
                            evidence.write_text(json.dumps(receipt))
                    return result
                expected=FileNotFoundError if mode=='missing' else RuntimeError
                with self.assertRaises(expected):
                    self.attempt(dict(passed=True),invalid_menu)
                self.assertEqual(self.selection.read_bytes(),self.original_selection)
                for name,permissions in self.launcher_modes.items():
                    self.assertEqual((self.root/name).read_text(),'previous launcher')
                    self.assertEqual((self.root/name).stat().st_mode&0o777,permissions)
                self.assertEqual(list((self.root/'bin/engine-builds').iterdir()),[])
                report=json.loads((self.args.output/'report.json').read_text())
                self.assertFalse(report['passed']);self.assertFalse(report['qualified_candidate'])
                self.assertEqual(report.get('smoke',[]),[])
                self.assertNotIn('split_screen_menu_startup',report)
                shutil.rmtree(self.scratch/'source')

    def test_missing_failed_or_incomplete_friends_footer_cannot_promote(self):
        for mode in ('missing', 'failed', 'partial_cases', 'failed_case', 'malformed_case',
                     'oversized_report', 'invalid_rendered_count'):
            with self.subTest(mode=mode):
                def invalid_footer(command, **kwargs):
                    result = self.build(command, **kwargs)
                    if '--build-only' not in command:
                        output = Path(command[command.index('--output')+1])
                        evidence = output/'evidence/friends-footer-anchor.json'
                        if mode == 'missing':
                            evidence.unlink()
                        elif mode == 'oversized_report':
                            evidence.write_bytes(b'x'*(1024*1024+1))
                        else:
                            receipt = json.loads(evidence.read_text())
                            if mode == 'failed': receipt['passed'] = False
                            elif mode == 'partial_cases': receipt['cases'].pop()
                            elif mode == 'failed_case': receipt['cases'][20]['passed'] = False
                            elif mode == 'malformed_case': receipt['cases'][20] = 1
                            elif mode == 'invalid_rendered_count': receipt['rendered_cases'] = -1
                            evidence.write_text(json.dumps(receipt))
                    return result
                with self.assertRaisesRegex(RuntimeError, 'Friends footer qualification'):
                    self.attempt(dict(passed=True), invalid_footer)
                self.assertEqual(self.selection.read_bytes(), self.original_selection)
                for name, permissions in self.launcher_modes.items():
                    self.assertEqual((self.root/name).read_text(), 'previous launcher')
                    self.assertEqual((self.root/name).stat().st_mode&0o777, permissions)
                self.assertEqual(list((self.root/'bin/engine-builds').iterdir()), [])
                report = json.loads((self.args.output/'report.json').read_text())
                self.assertFalse(report['passed']);self.assertFalse(report['qualified_candidate'])
                self.assertEqual(report.get('smoke', []), [])
                self.assertNotIn('friends_footer_anchor', report)
                shutil.rmtree(self.scratch/'source')

    def test_missing_or_partial_loop_audio_owner_proof_cannot_promote(self):
        for mode in ('missing','failed','partial_cases','observer_not_compared',
                     'mode3_missing','wrong_original','malformed','oversized'):
            with self.subTest(mode=mode):
                def invalid_audio(command, **kwargs):
                    result = self.build(command, **kwargs)
                    if '--build-only' not in command:
                        output = Path(command[command.index('--output')+1])
                        evidence = output/'evidence/loop-audio-locations.json'
                        if mode == 'missing': evidence.unlink()
                        elif mode == 'oversized': evidence.write_bytes(b'x'*(1024*1024+1))
                        elif mode == 'malformed': evidence.write_text('[]')
                        else:
                            receipt = json.loads(evidence.read_text())
                            if mode == 'failed': receipt['passed'] = False
                            elif mode == 'partial_cases': receipt['cases'] = 21
                            elif mode == 'observer_not_compared': receipt['comparisons'] = 22
                            elif mode == 'mode3_missing': receipt['mode3_cases'] = 0
                            elif mode == 'wrong_original': receipt['original_xbe_sha256'] = '0'*64
                            evidence.write_text(json.dumps(receipt))
                    return result
                with self.assertRaisesRegex(RuntimeError, 'Loop audio location qualification'):
                    self.attempt(dict(passed=True), invalid_audio)
                self.assertEqual(self.selection.read_bytes(), self.original_selection)
                for name, permissions in self.launcher_modes.items():
                    self.assertEqual((self.root/name).read_text(), 'previous launcher')
                    self.assertEqual((self.root/name).stat().st_mode&0o777, permissions)
                self.assertEqual(list((self.root/'bin/engine-builds').iterdir()), [])
                report = json.loads((self.args.output/'report.json').read_text())
                self.assertFalse(report['passed']);self.assertFalse(report['qualified_candidate'])
                self.assertEqual(report.get('smoke', []), [])
                self.assertNotIn('loop_audio_locations', report)
                shutil.rmtree(self.scratch/'source')

    def test_build_only_receipt_cannot_replace_completed_qualification(self):
        def only_built(command,**kwargs):
            result=self.build(command,**kwargs)
            if '--build-only' not in command:
                output=Path(command[command.index('--output')+1])
                (output/'report.json').write_text(json.dumps(dict(
                    passed=False,status='built',validation_not_run=True,steps=[])))
            return result
        with self.assertRaisesRegex(RuntimeError,'completed passing CTest'):
            self.attempt(dict(passed=True),only_built)
        self.assertEqual(self.selection.read_bytes(),self.original_selection)
        self.assertEqual(list((self.root/'bin/engine-builds').iterdir()),[])
        report=json.loads((self.args.output/'report.json').read_text())
        self.assertFalse(report['qualified_candidate']);self.assertEqual(report.get('smoke',[]),[])

    def test_incomplete_or_foreign_smg_audio_proof_cannot_promote(self):
        modes = ('missing', 'failed', 'wrong_runtime_hash', 'wrong_runtime_path', 'wrong_map_hash',
            'wrong_map_path', 'wrong_input_hash', 'wrong_input_path', 'wrong_metadata_hash', 'wrong_metadata',
            'wrong_capture_tick', 'red_expectation', 'missing_native', 'zero_mode3', 'one_source_tick',
            'one_sample_tick', 'unbalanced_calls', 'unsalted_player', 'unsalted_effect', 'unsalted_loop', 'unsalted_definition',
            'foreign_target_owner', 'foreign_target_definition', 'ownerless_sample', 'unclassified_generation',
            'wrong_caller', 'false_al', 'nonfinite_output', 'wrong_output_extent', 'post_pin_non_smg',
            'no_round_consumption', 'no_shot_transition', 'no_reload', 'no_reload_audio', 'no_smg_audio',
            'no_projectiles', 'no_frames', 'no_shadow_generate', 'no_shadow_apply', 'incomplete_capture',
            'retained_scratch', 'changed_inputs', 'missing_input_pin', 'wrong_input_pin',
            'missing_smg_damage', 'false_smg_damage', 'no_smg_body_decrease', 'nonfinite_smg_body',
            'one_smg_body_sample', 'missing_smg_command_tick', 'early_smg_damage', 'pre_shot_smg_damage', 'late_smg_damage',
            'missing_precondition', 'unchanged_precondition', 'dead_precondition', 'no_grenade_command',
            'unsalted_frag', 'foreign_frag_owner', 'unreleased_frag', 'multiple_frag_generations',
            'missing_detonation', 'foreign_detonation', 'late_detonation', 'grenade_after_fire',
            'missing_log', 'changed_log', 'malformed', 'oversized')
        self.args.accept_smoke_failure = ['lockout.map:1', 'coagulation.map:4']
        for mode in modes:
            with self.subTest(mode=mode):
                def invalid_smg(command, **kwargs):
                    result = self.build(command, **kwargs)
                    if '--build-only' not in command:
                        output = Path(command[command.index('--output')+1])
                        evidence = output/'evidence/smg-warthog-audio.json'
                        if mode == 'missing': evidence.unlink()
                        elif mode == 'oversized': evidence.write_bytes(b'x' * (524288 + 1))
                        elif mode == 'malformed': evidence.write_text('[]')
                        else:
                            receipt = json.loads(evidence.read_text())
                            native = receipt['native']; player = native['player']; audio = native['audio']; calls = native['mode3']
                            if mode == 'failed': receipt['passed'] = False
                            elif mode == 'wrong_runtime_hash': receipt['runtime_sha256'] = '0'*64
                            elif mode == 'wrong_runtime_path': receipt['runtime'] = '/foreign/halo2_native'
                            elif mode == 'wrong_map_hash': receipt['map_sha256'] = '0'*64
                            elif mode == 'wrong_map_path': receipt['map'] = '/foreign/coagulation.map'
                            elif mode == 'wrong_input_hash': receipt['input_script_sha256'] = '0'*64
                            elif mode == 'wrong_input_path': receipt['input_script'] = '/foreign/input.tsv'
                            elif mode == 'wrong_metadata_hash': receipt['reference_metadata_sha256'] = '0'*64
                            elif mode == 'wrong_metadata': receipt['reference_metadata']['capture_tick'] = 1200
                            elif mode == 'wrong_capture_tick': receipt['capture_tick'] = 1200
                            elif mode == 'red_expectation': receipt['expect'] = 'missing-owner'
                            elif mode == 'missing_native': receipt.pop('native')
                            elif mode == 'zero_mode3': calls['new_generation_successful_returns'] = 0
                            elif mode == 'one_source_tick': calls['post_fire_successful_ticks'] = 1
                            elif mode == 'one_sample_tick': calls['samples'][1]['tick'] = calls['samples'][0]['tick']
                            elif mode == 'unbalanced_calls': calls['returns'] = 1
                            elif mode == 'unsalted_player': player['source_unit'] = 42
                            elif mode == 'unsalted_effect': calls['samples'][0]['effect'] = 42
                            elif mode == 'unsalted_loop': calls['samples'][0]['loop'] = 42
                            elif mode == 'unsalted_definition': calls['samples'][0]['effect_definition'] = 42
                            elif mode == 'foreign_target_owner': calls['samples'][0]['effect_object_owner'] = 0xe9990099
                            elif mode == 'foreign_target_definition': calls['samples'][0]['effect_object_definition'] = 0xe9990099
                            elif mode == 'ownerless_sample': calls['samples'][0]['effect_object_owner'] = 0xffffffff
                            elif mode == 'unclassified_generation': calls['samples'][0]['new_generation'] = False
                            elif mode == 'wrong_caller': calls['samples'][0]['caller_return'] = 0x18fa49
                            elif mode == 'false_al': calls['samples'][0]['eax'] &= 0xffffff00
                            elif mode == 'nonfinite_output': calls['samples'][0]['output_bits'][0] = 0x7fc00000
                            elif mode == 'wrong_output_extent': calls['samples'][0]['output_bits'].pop()
                            elif mode == 'post_pin_non_smg': player['post_pin_non_smg_inputs'] = 1
                            elif mode == 'no_round_consumption': player['rounds_consumed'] = 60
                            elif mode == 'no_shot_transition': player['shot_transitions'] = 0
                            elif mode == 'no_reload': player['reload_transfers'] = 0
                            elif mode == 'no_reload_audio': audio['reload_packets'] = 0
                            elif mode == 'no_smg_audio': audio['smg_packets'] = 0
                            elif mode == 'no_projectiles': native['native_projectile_publications'] = 0
                            elif mode == 'no_frames': native['submitted_frames'] = 0
                            elif mode == 'no_shadow_generate': native['shadow_generate_draws'] = 0
                            elif mode == 'no_shadow_apply': receipt['shadow_draws']['world_native_shadow_apply_prepared_draws'] = 0
                            elif mode == 'incomplete_capture': receipt['capture']['bytes'] -= 1
                            elif mode == 'retained_scratch': receipt['scratch_removed'] = False
                            elif mode == 'changed_inputs': receipt['input_pins_after'][0]['sha256'] = '0'*64
                            elif mode == 'missing_input_pin': receipt['input_pins_before'].pop(); receipt['input_pins_after'].pop()
                            elif mode == 'wrong_input_pin':
                                receipt['input_pins_before'][0]['sha256'] = '0'*64
                                receipt['input_pins_after'][0]['sha256'] = '0'*64
                            elif mode == 'missing_smg_damage': native['target'].pop('smg_damage_observed')
                            elif mode == 'false_smg_damage': native['target']['smg_damage_observed'] = False
                            elif mode == 'no_smg_body_decrease': native['target']['minimum_body_after_smg'] = native['target']['body_before_smg']
                            elif mode == 'nonfinite_smg_body': native['target']['body_before_smg'] = float('nan')
                            elif mode == 'one_smg_body_sample': native['target']['post_smg_vitality_samples'] = 1
                            elif mode == 'missing_smg_command_tick': native['target'].pop('first_smg_command_tick')
                            elif mode == 'early_smg_damage': native['target']['first_smg_damage_tick'] = native['target']['first_smg_command_tick'] - 1
                            elif mode == 'pre_shot_smg_damage': native['target'].update(first_smg_command_tick=1,first_smg_damage_tick=2)
                            elif mode == 'late_smg_damage': native['target']['first_smg_damage_tick'] = native['requested_capture_tick']
                            elif mode == 'missing_precondition': native.pop('precondition')
                            elif mode == 'unchanged_precondition': native['precondition']['body_after'] = 1.0
                            elif mode == 'dead_precondition': native['precondition']['body_after'] = 0.0
                            elif mode == 'no_grenade_command': native['precondition']['grenade_commands'] = 0
                            elif mode == 'unsalted_frag': native['precondition']['grenade_projectiles'][0]['projectile'] = 42
                            elif mode == 'foreign_frag_owner': native['precondition']['grenade_projectiles'][0]['player'] = 0xe9990099
                            elif mode == 'unreleased_frag': native['precondition']['grenade_projectiles'][0]['released'] = False
                            elif mode == 'multiple_frag_generations':
                                event = dict(native['precondition']['grenade_projectiles'][0]); event['projectile'] = 0xe9990099
                                native['precondition']['grenade_projectiles'].append(event)
                            elif mode == 'missing_detonation': native['precondition']['grenade_detonations'] = []
                            elif mode == 'foreign_detonation': native['precondition']['grenade_detonations'][0]['projectile'] = 0xe9990099
                            elif mode == 'late_detonation': native['precondition']['grenade_detonations'][0]['tick'] = calls['first_shot_tick']
                            elif mode == 'grenade_after_fire': receipt['input_schedule']['grenade_keys'] = [receipt['input_schedule']['first_fire_key']]
                            elif mode == 'missing_log': evidence.with_suffix('.log').unlink()
                            elif mode == 'changed_log': evidence.with_suffix('.log').write_text('foreign log')
                            evidence.write_text(json.dumps(receipt))
                    return result
                with self.assertRaisesRegex(RuntimeError, 'SMG Warthog audio qualification'):
                    self.attempt(dict(passed=True), invalid_smg)
                self.assertEqual(self.selection.read_bytes(), self.original_selection)
                for name, permissions in self.launcher_modes.items():
                    self.assertEqual((self.root/name).read_text(), 'previous launcher')
                    self.assertEqual((self.root/name).stat().st_mode&0o777, permissions)
                self.assertEqual(list((self.root/'bin/engine-builds').iterdir()), [])
                report = json.loads((self.args.output/'report.json').read_text())
                self.assertFalse(report['passed']);self.assertFalse(report['qualified_candidate'])
                self.assertEqual(report.get('smoke', []), [])
                self.assertNotIn('smg_warthog_audio', report)
                shutil.rmtree(self.scratch/'source')

    def existing_target_smg(self, command, **kwargs):
        result = self.build(command, **kwargs)
        if '--build-only' not in command:
            output = Path(command[command.index('--output')+1])
            evidence = output/'evidence/smg-warthog-audio.json'
            receipt = json.loads(evidence.read_text()); calls = receipt['native']['mode3']
            calls.update(entries=4,returns=4,validated_entries=4,successful_returns=4,
                new_generation_successful_returns=0,preexisting_target_owned_successful_returns=2,
                precondition_target_owned_successful_returns=2,precondition_generation_seen=True,
                precondition_generation_count=1)
            for sample in calls['samples']:
                sample.update(new_generation=False,precondition_generation_seen=True,
                    precondition_first_seen_source_tick=300)
            evidence.write_text(json.dumps(receipt))
        return result

    def test_existing_target_loop_requires_genuine_earlier_source_owner_progress(self):
        self.attempt(dict(passed=True), self.existing_target_smg)
        report = json.loads((self.args.output/'report.json').read_text())['smg_warthog_audio']
        self.assertTrue(report['passed'])
        self.assertEqual(report['new_generation_successful_returns'], 0)
        self.assertEqual(report['preexisting_target_owned_successful_returns'], 2)
        self.assertEqual(report['target_owned_post_fire_successful_returns'], 2)

    def test_existing_loop_without_matching_prior_source_observation_cannot_promote(self):
        for mode in ('missing_precondition', 'no_prior_return', 'no_prior_generation', 'false_sample_seen',
                     'same_shot_tick', 'future_seen_tick', 'wrong_owner', 'zero_target_returns'):
            with self.subTest(mode=mode):
                def invalid_existing(command, **kwargs):
                    result = self.existing_target_smg(command, **kwargs)
                    if '--build-only' not in command:
                        output = Path(command[command.index('--output')+1])
                        evidence = output/'evidence/smg-warthog-audio.json'
                        receipt = json.loads(evidence.read_text()); native = receipt['native']; calls = native['mode3']
                        if mode == 'missing_precondition': native['target']['precondition_observed'] = False
                        elif mode == 'no_prior_return': calls['precondition_target_owned_successful_returns'] = 0
                        elif mode == 'no_prior_generation': calls['precondition_generation_count'] = 0
                        elif mode == 'false_sample_seen': calls['samples'][0]['precondition_generation_seen'] = False
                        elif mode == 'same_shot_tick': calls['samples'][0]['precondition_first_seen_source_tick'] = calls['first_shot_source_tick']
                        elif mode == 'future_seen_tick': calls['samples'][0]['precondition_first_seen_source_tick'] = calls['first_shot_source_tick'] + 1
                        elif mode == 'wrong_owner': calls['samples'][0]['effect_object_owner'] = 0xe9990099
                        elif mode == 'zero_target_returns': calls['target_owned_post_fire_successful_returns'] = 0
                        evidence.write_text(json.dumps(receipt))
                    return result
                with self.assertRaisesRegex(RuntimeError, 'SMG Warthog audio qualification'):
                    self.attempt(dict(passed=True), invalid_existing)
                self.assertEqual(self.selection.read_bytes(), self.original_selection)
                self.assertEqual(list((self.root/'bin/engine-builds').iterdir()), [])
                shutil.rmtree(self.scratch/'source')

    def test_borrowed_smg_reference_change_during_qualification_cannot_promote(self):
        def changed_reference(command, **kwargs):
            result = self.build(command, **kwargs)
            if '--build-only' not in command:
                self.smg_input.write_text(self.smg_input.read_text() + '# Changed after qualification\n')
            return result
        with self.assertRaisesRegex(RuntimeError, 'SMG Warthog audio reference'):
            self.attempt(dict(passed=True), changed_reference)
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual(list((self.root/'bin/engine-builds').iterdir()), [])

    def test_smg_reference_requires_qualified_tape_map_and_metadata_tick(self):
        reference = engine.smg_warthog_reference(self.root, self.maps)
        self.assertEqual(reference['capture_tick'], 1400)
        original = self.smg_metadata.read_text()
        for field, value in [('capture_tick', 599), ('capture_tick', True),
                             ('map_sha256', '0'*64), ('input_script_sha256', '0'*64)]:
            with self.subTest(field=field, value=value):
                changed = json.loads(original); changed[field] = value
                self.smg_metadata.write_text(json.dumps(changed))
                with self.assertRaisesRegex(RuntimeError, 'SMG Warthog audio reference'):
                    engine.smg_warthog_reference(self.root, self.maps)
        self.smg_metadata.write_text(original)
        self.smg_metadata.unlink()
        with self.assertRaisesRegex(RuntimeError, 'SMG Warthog audio reference'):
            engine.smg_warthog_reference(self.root, self.maps)

    def test_incomplete_or_foreign_grenade_shadow_cases_cannot_promote(self):
        modes = [(kind, mode) for kind in engine.GRENADE_SHADOW_CASES
                 for mode in ('missing', 'failed', 'wrong_runtime_hash')]
        modes.extend([('frag', mode) for mode in ('wrong_runtime_path', 'wrong_map',
            'zero_frames', 'missing_native_grenade', 'unadmitted_grenade', 'unsalted_definition',
            'missing_detonation', 'missing_shadow_draws', 'incomplete_capture',
            'wrong_capture_tick', 'missing_log', 'changed_log', 'missing_check', 'retained_scratch')])
        modes.append(('neutral', 'unexpected_grenade'))
        modes.extend([('receiver', 'unexpected_grenade'), ('receiver', 'missing_shadow_draws')])
        self.args.accept_smoke_failure = ['lockout.map:1', 'coagulation.map:4']
        for kind, mode in modes:
            with self.subTest(case=kind, mode=mode):
                def invalid_grenade(command, **kwargs):
                    result = self.build(command, **kwargs)
                    if '--build-only' not in command:
                        output = Path(command[command.index('--output')+1])
                        evidence = output/'evidence'/f'grenade-shadow-{kind}.json'
                        if mode == 'missing':
                            evidence.unlink()
                        else:
                            receipt = json.loads(evidence.read_text())
                            if mode == 'failed': receipt['passed'] = False
                            elif mode == 'wrong_runtime_hash': receipt['runtime_sha256'] = '0'*64
                            elif mode == 'wrong_runtime_path': receipt['runtime'] = '/foreign/halo2_native'
                            elif mode == 'wrong_map': receipt['map_sha256'] = '0'*64
                            elif mode == 'zero_frames': receipt['frames'] = 0
                            elif mode == 'missing_native_grenade':
                                receipt.update(native_grenade_observations=0,
                                    native_grenade_admitted_observations=0,grenade_ticks=[],
                                    grenade_owners=[],grenade_definitions=[],grenade_types=[])
                            elif mode == 'unadmitted_grenade': receipt['native_grenade_admitted_observations'] = 0
                            elif mode == 'unsalted_definition': receipt['grenade_definitions'] = [42]
                            elif mode == 'missing_detonation': receipt['detonation_serial'] = 0
                            elif mode == 'missing_shadow_draws': receipt['shadow_draws']['world_native_shadow_generate_prepared_draws'] = 0
                            elif mode == 'incomplete_capture': receipt['capture']['bytes'] -= 1
                            elif mode == 'wrong_capture_tick': receipt['capture_tick'] = 20
                            elif mode == 'missing_log': evidence.with_suffix('.log').unlink()
                            elif mode == 'changed_log': evidence.with_suffix('.log').write_text('foreign log')
                            elif mode == 'missing_check': receipt['checks'].pop('native_grenade_lifecycle_or_neutral_control')
                            elif mode == 'retained_scratch': receipt['scratch_removed'] = False
                            elif mode == 'unexpected_grenade':
                                receipt.update(native_grenade_observations=2,
                                    native_grenade_admitted_observations=2,grenade_ticks=[23,24],
                                    grenade_owners=[0xe1230042],grenade_definitions=[0xe2340043],
                                    grenade_types=[0],detonation_serial=1)
                            evidence.write_text(json.dumps(receipt))
                    return result
                with self.assertRaisesRegex(RuntimeError, 'Grenade shadow qualification'):
                    self.attempt(dict(passed=True), invalid_grenade)
                self.assertEqual(self.selection.read_bytes(), self.original_selection)
                for name, permissions in self.launcher_modes.items():
                    self.assertEqual((self.root/name).read_text(), 'previous launcher')
                    self.assertEqual((self.root/name).stat().st_mode&0o777, permissions)
                self.assertEqual(list((self.root/'bin/engine-builds').iterdir()), [])
                report = json.loads((self.args.output/'report.json').read_text())
                self.assertFalse(report['passed']);self.assertFalse(report['qualified_candidate'])
                self.assertEqual(report.get('smoke', []), [])
                self.assertNotIn('grenade_shadow_residency', report)
                shutil.rmtree(self.scratch/'source')

    def test_failed_required_checks_remove_the_pre_staged_unqualified_bank(self):
        def failed(command,**kwargs):
            result=self.build(command,**kwargs)
            if '--build-only' not in command:
                return subprocess.CompletedProcess(command,8)
            return result
        with self.assertRaisesRegex(RuntimeError,'launcher or loading checks failed'):
            self.attempt(dict(passed=True),failed)
        self.assertEqual(self.selection.read_bytes(),self.original_selection)
        self.assertEqual((self.root/'Halo2-Launcher').read_text(),'previous launcher')
        self.assertEqual(list((self.root/'bin/engine-builds').iterdir()),[])

    def test_staged_shader_mutation_during_checks_is_rejected_without_publication(self):
        def changed(command,**kwargs):
            result=self.build(command,**kwargs)
            if '--build-only' not in command:
                directory=Path(next(value.split('=',2)[2] for value in command
                    if value.startswith('--cmake-arg=-DHALO2_RUNTIME_SHADER_DIR=')))
                (directory/'world.vert.spv').write_bytes(b'changed after submitted-frame checks')
            return result
        with self.assertRaisesRegex(ValueError,'package identity'):
            self.attempt(dict(passed=True),changed)
        self.assertEqual(self.selection.read_bytes(),self.original_selection)
        self.assertEqual(list((self.root/'bin/engine-builds').iterdir()),[])

    def test_executable_changed_after_staging_cannot_publish_a_mixed_candidate(self):
        def changed(command,**kwargs):
            result=self.build(command,**kwargs)
            if '--build-only' not in command:
                with (self.scratch/'build/halo2_native').open('ab') as file:file.write(b'changed')
            return result
        with self.assertRaisesRegex(RuntimeError,'executable changed'):
            self.attempt(dict(passed=True),changed)
        self.assertEqual(self.selection.read_bytes(),self.original_selection)
        self.assertEqual(list((self.root/'bin/engine-builds').iterdir()),[])

    def test_failed_frame_check_preserves_both_launchers_selection_and_removes_candidate(self):
        with self.assertRaisesRegex(RuntimeError, 'frame check'):
            self.attempt(dict(passed=False, submitted_frames=0))
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            self.assertEqual((self.root / name).read_text(), 'previous launcher')
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertFalse(report['passed'])
        self.assertEqual(len(report['requested_smoke_cases']), 3)
        self.assertEqual(len(report['smoke']), 3, 'One failed case must not hide the rest of the requested cohort')

    def test_interpreter_module_is_rejected_before_smoke_and_root_publication(self):
        def changed(command, **kwargs):
            result = self.build(command, **kwargs)
            # An unowned generic program cannot enter the declared production
            # bank even if the incremental build happens to contain it.
            inventory = self.scratch / 'build/production-shaders.txt'
            names = inventory.read_text().splitlines() + ['world.frag.spv']
            inventory.write_text('\n'.join(sorted(names)) + '\n')
            return result
        with self.assertRaisesRegex(ValueError, 'differs from its authored/compiler owners'):
            self.attempt(dict(passed=True), changed)
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            self.assertEqual((self.root / name).read_text(), 'previous launcher')
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertEqual(report.get('smoke', []), [])

    def test_failed_weapon_audio_preserves_selection_and_both_launchers(self):
        self.args.weapon_audio_reference_xbe = self.root / 'reference.xbe'
        self.args.weapon_audio_reference_xbe.write_bytes(b'synthetic reference identity')
        with patch.object(engine, 'weapon_audio', return_value=dict(passed=False)) as audio:
            with self.assertRaisesRegex(RuntimeError, 'weapon audio checks failed'):
                self.attempt(dict(passed=True, submitted_frames=4))
        audio.assert_called_once()
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            self.assertEqual((self.root / name).read_text(), 'previous launcher')
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])
        self.assertFalse(json.loads((self.args.output / 'report.json').read_text())['weapon_audio']['passed'])

    def test_frozen_source_change_cannot_promote_a_mixed_build(self):
        def changed(command, **kwargs):
            result = self.build(command, **kwargs)
            (self.scratch / 'source/CMakeLists.txt').write_text('changed while compiler was running')
            return result
        with self.assertRaisesRegex(RuntimeError, 'source changed'):
            self.attempt(dict(passed=True), changed)
        self.assertEqual(self.selection.read_bytes(), self.original_selection)

    def test_generator_change_cannot_promote_a_mixed_build(self):
        tools = self.root/'tools'
        tools.mkdir()
        emitter = tools/'native_world_fragment_kernel.py'
        emitter.write_text('source-owned combiner generator')
        def changed(command, **kwargs):
            result=self.build(command, **kwargs)
            (self.scratch/'source/tools'/emitter.name).write_text('generator changed during compile')
            return result
        with self.assertRaisesRegex(RuntimeError, 'source changed'):
            self.attempt(dict(passed=True), changed)
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual((self.root/'Halo2-Launcher').read_text(), 'previous launcher')

    def test_generator_pins_exclude_temporary_outputs_and_preserve_legacy_status(self):
        tools=self.root/'tools';tools.mkdir()
        emitter=tools/'compile_native_fragment_kernels.py';emitter.write_text('original generator')
        before=engine.source_hashes(self.root)
        for name in ('__pycache__/emitter.py','build/generated.cpp','.git/fixture.py',
                     'emitter.pyc','emitter.o','emitter.tmp.py'):
            path=tools/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text('temporary')
        self.assertEqual(engine.source_hashes(self.root),before)
        (self.scratch/'source-hashes.json').write_text(json.dumps(before))
        emitter.write_text('changed generator')
        self.assertIn('1 files differ',engine.source_status(self.root,self.runtime))
        legacy={name:pin for name,pin in before.items() if not name.startswith('tools/')}
        (self.scratch/'source-hashes.json').write_text(json.dumps(legacy))
        self.assertIn('lacks generator source provenance',engine.source_status(self.root,self.runtime))

    def test_eos_release_refuses_a_missing_runtime_library(self):
        self.args.eos_client_config = self.root / 'private-client.env'
        self.args.eos_client_config.write_text('synthetic shared client input')
        with self.assertRaisesRegex(RuntimeError, 'EOS runtime library'):
            self.attempt(dict(passed=True))
        self.assertEqual(self.selection.read_bytes(), self.original_selection)

    def test_missing_sdk_worker_cannot_publish_or_reach_smoke(self):
        def missing(command, **kwargs):
            result = self.build(command, **kwargs)
            (self.scratch / 'build/eos_sdk_worker').unlink()
            return result
        with self.assertRaises(FileNotFoundError):
            self.attempt(dict(passed=True), missing)
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])

    def test_eos_library_and_client_settings_reach_the_runtime_before_smoke(self):
        self.args.eos_client_config = self.root / 'private-client.env'
        self.args.eos_client_config.write_text('synthetic shared client input')
        def build(command, **kwargs):
            self.assertIn('--cmake-arg=-DHALO2_EOS_CLIENT_CONFIG=' + str(self.args.eos_client_config), command)
            result = self.build(command, **kwargs)
            for name in ('libEOSSDK-Linux-Shipping.so', 'eos-client.env'):
                (self.scratch / 'build' / name).write_text('synthetic packaged dependency')
            return result
        def smoke(runtime, *_args, **_kwargs):
            for name in ('libEOSSDK-Linux-Shipping.so', 'eos-client.env'):
                self.assertEqual((runtime.parent / name).read_text(), 'synthetic packaged dependency')
            return dict(passed=False)
        with patch.dict(os.environ, HALO2_TEST_SCRATCH=str(self.scratch)), \
             patch('gameplay_runtime_selection.selected_gameplay_runtime', return_value=None), \
             patch('mcc_onboarding.selected', return_value=(self.cache, {})), \
             patch.object(engine.subprocess, 'run', side_effect=build), \
             patch.object(engine, 'smoke', side_effect=smoke):
            with self.assertRaisesRegex(RuntimeError, 'frame check'):
                engine.worker(self.args)
        self.assertEqual(self.selection.read_bytes(), self.original_selection)

    def test_frozen_build_is_independent_of_later_engine_edits(self):
        destination = self.scratch / 'frozen'
        pins = engine.freeze_source(self.root, destination)
        (self.root / 'CMakeLists.txt').write_text('next engine revision')
        self.assertEqual(engine.source_hashes(destination), pins)
        self.assertNotEqual(engine.source_hashes(self.root), pins)

    def test_frozen_online_service_keeps_its_matching_coordinator_code(self):
        server = self.root / 'multiplayer/master_server'
        server.mkdir(parents=True)
        service = server / 'match_transport.py'
        service.write_text('operation = "room_setup_snapshot"\n')
        docs = self.root / 'multiplayer/docs'
        docs.mkdir()
        (docs / 'reference.txt').write_text('existing reference')
        destination = self.scratch / 'frozen'
        pins = engine.freeze_source(self.root, destination)
        self.assertFalse((destination / 'multiplayer').is_symlink())
        self.assertFalse((destination / 'multiplayer/master_server').is_symlink())
        self.assertTrue((destination / 'multiplayer/docs').is_symlink())
        service.write_text('operation = "room_state_get"\n')
        self.assertEqual((destination / 'multiplayer/master_server/match_transport.py').read_text(),
                         'operation = "room_setup_snapshot"\n')
        self.assertEqual(engine.source_hashes(destination), pins)
        self.assertNotEqual(engine.source_hashes(self.root), pins)

    def test_frozen_build_borrows_existing_vista_fixtures_without_copying_assets(self):
        vista = self.root / 'Halo 2 Vista installer'
        (vista / 'maps').mkdir(parents=True)
        fixture = vista / 'maps/01a_tutorial.map'
        fixture.write_bytes(b'existing reference fixture')
        destination = self.scratch / 'frozen'
        pins = engine.freeze_source(self.root, destination)
        self.assertTrue((destination / 'assets').is_symlink())
        self.assertTrue((destination / vista.name).is_symlink())
        self.assertEqual((destination / vista.name / 'maps/01a_tutorial.map').resolve(), fixture)
        self.assertEqual(engine.source_hashes(destination), pins)
        self.assertEqual(fixture.read_bytes(), b'existing reference fixture')

    def test_another_tasks_selection_is_not_overwritten(self):
        def changed(command, **kwargs):
            result = self.build(command, **kwargs)
            self.selection.write_text('another task selected its runtime')
            return result
        with self.assertRaisesRegex(RuntimeError, 'Another task'):
            self.attempt(dict(passed=True), changed)
        self.assertEqual(self.selection.read_text(), 'another task selected its runtime')

    def test_another_tasks_launcher_is_not_adopted_as_a_publication_preimage(self):
        def changed(command, **kwargs):
            result = self.build(command, **kwargs)
            (self.root / 'Halo2-Launcher').write_text('another task installed its launcher')
            return result
        with self.assertRaisesRegex(RuntimeError, 'Another task changed publication file'):
            self.attempt(dict(passed=True), changed)
        self.assertEqual((self.root / 'Halo2-Launcher').read_text(), 'another task installed its launcher')
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])

    def test_success_checks_the_actual_admission_reader_before_selecting_both_launchers(self):
        from gameplay_runtime_selection import selected_gameplay_runtime
        def reader(root, **kwargs):
            return selected_gameplay_runtime(root, **kwargs) if kwargs else None
        with patch.dict(os.environ, HALO2_TEST_SCRATCH=str(self.scratch)), \
             patch('gameplay_runtime_selection.selected_gameplay_runtime', side_effect=reader), \
             patch('mcc_onboarding.selected', return_value=(self.cache, {})), \
             patch.object(engine.subprocess, 'run', side_effect=self.build), \
             patch.object(engine, 'smoke', return_value=dict(passed=True, submitted_frames=4)):
            engine.worker(self.args)
        record = json.loads(self.selection.read_text())
        runtime = self.root / record['runtime']
        self.assertEqual(record['runtime_sha256'], engine.digest(runtime))
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            self.assertEqual((self.root / name).read_bytes(), (runtime.parent / name).read_bytes())
        self.assertTrue((runtime.parent / 'source-hashes.json').is_file())
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertTrue(report['passed'])
        self.assertTrue(report['qualified_candidate'])
        self.assertTrue(report['runtime_installed'])
        self.assertEqual(report['status'], 'published')
        self.assertNotEqual(report['scope'], 'qualified_candidate_only')

    def test_prepare_only_qualifies_retains_candidate_without_root_publication(self):
        from gameplay_runtime_selection import selected_gameplay_runtime
        self.args.prepare_only = True
        (self.maps / 'mainmenu.map').write_bytes(b'synthetic menu')
        (self.root / 'frontend-runtime-selection.json').write_text('previous menu owner')
        names = ('Halo2-Launcher', 'Halo2-Title-Launcher', self.selection.name,
                 'frontend-runtime-selection.json')
        original = {name: engine.publication_identity(self.root / name) for name in names}
        admitted = []

        def reader(root, **kwargs):
            if not kwargs:
                return None
            value = selected_gameplay_runtime(root, **kwargs)
            admitted.append(value)
            return value

        def menu(root, runtime, *_):
            return dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime))

        with patch.object(engine, 'publish_runtime_files') as publish, \
             patch('qualify_frontend_runtime.qualify', side_effect=menu):
            self.attempt(dict(passed=True), reader=reader)
        publish.assert_not_called()
        self.assertEqual({name: engine.publication_identity(self.root / name) for name in names}, original)
        self.assertEqual(len(admitted), 1, 'The actual candidate admission reader must still run')
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertTrue(report['passed'])
        self.assertTrue(report['qualified_candidate'])
        self.assertFalse(report['runtime_installed'])
        self.assertEqual(report['status'], 'prepared')
        self.assertEqual(report['scope'], 'qualified_candidate_only')
        runtime = self.root / report['runtime']
        self.assertTrue(runtime.is_file())
        self.assertEqual(report['runtime_sha256'], engine.digest(runtime))
        for name in ('selection.json', 'qualification.json', 'runtime-files.json',
                     'source-hashes.json', 'frontend-selection.json'):
            self.assertTrue((runtime.parent / name).is_file(), name)
        self.assertEqual(json.loads((runtime.parent / 'qualification.json').read_text()), report)

    def test_reused_older_cohort_receives_current_pinned_assets_without_a_cmake_asset_target(self):
        self.args.prepare_only = True
        self.args.reuse_owned_build = True
        self.args.worker = True
        self.omit_build_custom_assets = True
        source = self.scratch / 'source'
        frozen = engine.freeze_source(self.root, source)
        (self.root / 'CMakeLists.txt').write_text('Later working tree; keep the active frozen cohort')
        build = self.scratch / 'build'
        build.mkdir()
        (build / 'CMakeCache.txt').write_text('CMAKE_HOME_DIRECTORY:INTERNAL=' + str(source) + '\n')
        expected_assets = custom_assets.inventory(self.root)
        self.attempt(dict(passed=True))
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertTrue(report['passed'])
        self.assertEqual(report['source_cohort']['kind'], 'active_owned_frozen')
        self.assertEqual(engine.source_hashes(source), frozen)
        runtime = self.root / report['runtime']
        self.assertEqual(custom_assets.inventory(runtime.parent / 'custom-assets'), expected_assets)
        self.assertEqual(self.selection.read_bytes(), self.original_selection)

    def test_prepare_only_preserves_foreign_root_changes_during_validation(self):
        self.args.prepare_only = True
        (self.maps / 'mainmenu.map').write_bytes(b'synthetic menu')
        frontend = self.root / 'frontend-runtime-selection.json'
        frontend.write_text('previous menu owner')

        def changed(root, runtime, *_):
            self.selection.write_text('foreign gameplay owner')
            frontend.write_text('foreign frontend owner')
            (self.root / 'Halo2-Launcher').write_text('foreign launcher owner')
            return dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime))

        with patch('qualify_frontend_runtime.qualify', side_effect=changed):
            self.attempt(dict(passed=True))
        self.assertEqual(self.selection.read_text(), 'foreign gameplay owner')
        self.assertEqual(frontend.read_text(), 'foreign frontend owner')
        self.assertEqual((self.root / 'Halo2-Launcher').read_text(), 'foreign launcher owner')
        self.assertEqual((self.root / 'Halo2-Title-Launcher').read_text(), 'previous launcher')
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertTrue(report['qualified_candidate'])
        self.assertFalse(report['runtime_installed'])
        self.assertTrue((self.root / report['runtime']).is_file())

    def test_prepare_only_failed_frames_remove_unqualified_candidate(self):
        self.args.prepare_only = True
        with self.assertRaisesRegex(RuntimeError, 'frame check'):
            self.attempt(dict(passed=False))
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual((self.root / 'Halo2-Launcher').read_text(), 'previous launcher')
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertFalse(report['passed'])
        self.assertFalse(report['qualified_candidate'])
        self.assertFalse(report['runtime_installed'])
        self.assertEqual(report['status'], 'failed')

    def test_prepare_only_reader_rejection_removes_candidate(self):
        self.args.prepare_only = True

        def reject(root, **kwargs):
            if kwargs:
                raise ValueError('candidate admission rejected')
            return None

        with self.assertRaisesRegex(ValueError, 'candidate admission rejected'):
            self.attempt(dict(passed=True), reader=reject)
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertFalse(report['passed'])
        self.assertFalse(report['qualified_candidate'])
        self.assertFalse(report['runtime_installed'])

    def test_cache_candidate_preparation_uses_real_reader_without_selecting_cache(self):
        from gameplay_runtime_selection import selected_gameplay_runtime
        import mcc_onboarding
        self.args.prepare_only = True
        candidate = self.root / 'assets/imported/mcc-candidate'
        maps = candidate / 'halo2/h2_maps_win64_dx11'
        maps.mkdir(parents=True)
        for name in ('lockout.map', 'coagulation.map', 'midship.map', 'zanzibar.map', 'mainmenu.map'):
            (maps / name).write_bytes(('qualified candidate ' + name).encode())
        for name in ('shared.map', 'single_player_shared.map'):
            shutil.copyfile(self.maps / name, maps / name)
        metadata = json.loads(self.smg_metadata.read_text())
        metadata['map_sha256'] = engine.digest(maps / 'coagulation.map')
        self.smg_metadata.write_text(json.dumps(metadata))
        self.args.cache_candidate = candidate
        registry = self.root / mcc_onboarding.REGISTRY
        registry.write_text(mcc_onboarding.SCHEMA + '\ncache\t' + self.cache.relative_to(self.root).as_posix() + '\n')
        (self.root / 'frontend-runtime-selection.json').write_text('previous menu owner')
        names = ('Halo2-Launcher', 'Halo2-Title-Launcher', self.selection.name,
                 'frontend-runtime-selection.json', mcc_onboarding.REGISTRY)
        before = {name: engine.publication_identity(self.root / name) for name in names}
        prior = dict(maps={'lockout.map': 'old', 'coagulation.map': 'old'},
                     default_map='coagulation.map', local_players=3, video_driver='x11')

        def reader(root, **kwargs):
            return selected_gameplay_runtime(root, **kwargs) if kwargs else prior

        def menu(root, runtime, chosen, *_):
            self.assertEqual(chosen, candidate)
            return dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime))

        with patch('mcc_onboarding.private_cache', return_value=(candidate, {})) as verify, \
             patch('qualify_frontend_runtime.qualify', side_effect=menu), \
             patch.object(engine, 'publish_runtime_files') as publish:
            self.attempt(dict(passed=True), reader=reader)
        publish.assert_not_called()
        self.assertEqual({name: engine.publication_identity(self.root / name) for name in names}, before)
        self.assertEqual(verify.call_count, 2)
        self.assertTrue(all(call.kwargs['verify'] is True for call in verify.call_args_list))
        report = json.loads((self.args.output / 'report.json').read_text())
        selection = self.root / report['runtime']
        record_path = selection.parent / 'selection.json'
        record = json.loads(record_path.read_text())
        self.assertEqual(record['cache'], candidate.relative_to(self.root).as_posix())
        self.assertEqual(set(record['maps']), set(prior['maps']))
        for name in ('default_map', 'local_players', 'video_driver'):
            self.assertEqual(record[name], prior[name])
        self.assertIn(dict(map='coagulation.map', local_players=3), report['requested_smoke_cases'])
        self.assertIn('eos_worker', record)
        with patch('mcc_onboarding.selected', return_value=(self.cache, {})):
            with self.assertRaisesRegex(ValueError, 'qualified prepared MCC cache'):
                selected_gameplay_runtime(self.root, selection_name=record_path.relative_to(self.root).as_posix())
        with patch('mcc_onboarding.private_cache', return_value=(self.cache, {})):
            with self.assertRaisesRegex(ValueError, 'qualified prepared MCC cache'):
                selected_gameplay_runtime(self.root, selection_name=record_path.relative_to(self.root).as_posix(),
                                          cache_candidate=self.cache)

    def test_invalid_cache_candidate_does_not_build_or_publish(self):
        self.args.cache_candidate = self.root / 'unverified-cache'
        with patch('mcc_onboarding.private_cache', side_effect=ValueError('cache verification failed')), \
             patch.object(engine, 'publish_runtime_files') as publish:
            with self.assertRaisesRegex(ValueError, 'cache verification failed'):
                self.attempt(dict(passed=True))
        publish.assert_not_called()
        self.assertEqual(self.selection.read_bytes(), self.original_selection)

    def test_cache_candidate_publication_passes_the_real_normal_reader(self):
        from gameplay_runtime_selection import selected_gameplay_runtime
        import mcc_onboarding
        candidate = self.root / 'assets/imported/mcc-candidate'
        maps = candidate / 'halo2/h2_maps_win64_dx11'
        maps.mkdir(parents=True)
        for name in ('lockout.map', 'coagulation.map', 'midship.map', 'zanzibar.map', 'mainmenu.map'):
            (maps / name).write_bytes(('qualified candidate ' + name).encode())
        for name in ('shared.map', 'single_player_shared.map'):
            shutil.copyfile(self.maps / name, maps / name)
        metadata = json.loads(self.smg_metadata.read_text())
        metadata['map_sha256'] = engine.digest(maps / 'coagulation.map')
        self.smg_metadata.write_text(json.dumps(metadata))
        self.args.cache_candidate = candidate
        registry = self.root / mcc_onboarding.REGISTRY
        registry.write_text(mcc_onboarding.SCHEMA + '\ncache\t' + self.cache.relative_to(self.root).as_posix() + '\n')
        (self.root / 'frontend-runtime-selection.json').write_text('previous menu owner')
        reads = []

        def reader(root, **kwargs):
            reads.append(kwargs)
            # The synthetic previous selector intentionally has no admitted
            # runtime; all candidate and final admissions use the real reader.
            return None if len(reads) == 1 else selected_gameplay_runtime(root, **kwargs)

        def menu(root, runtime, chosen, *_):
            self.assertEqual(chosen, candidate)
            return dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime))

        def verified(root, chosen, **kwargs):
            self.assertIn(chosen, (self.cache, candidate))
            return chosen, {}

        with patch.dict(os.environ, HALO2_TEST_SCRATCH=str(self.scratch)), \
             patch('gameplay_runtime_selection.selected_gameplay_runtime', side_effect=reader), \
             patch('mcc_onboarding.private_cache', side_effect=verified), \
             patch('qualify_frontend_runtime.qualify', side_effect=menu), \
             patch.object(engine.subprocess, 'run', side_effect=self.build), \
             patch.object(engine, 'smoke', return_value=dict(passed=True, submitted_frames=4)):
            engine.worker(self.args)
            record = selected_gameplay_runtime(self.root)
        self.assertEqual(record['cache'], candidate.relative_to(self.root).as_posix())
        self.assertEqual(reads[-1], {}, 'Final admission must use the normal selected-cache reader')
        self.assertEqual(registry.read_text(), mcc_onboarding.SCHEMA + '\ncache\t' + record['cache'] + '\n')
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertTrue(report['passed'])
        self.assertTrue(report['publication']['complete'])
        self.assertEqual(report['publication']['files'][-1]['name'], mcc_onboarding.REGISTRY)

    def publication_fault(self, target_name, error=OSError, foreign=None):
        (self.maps / 'mainmenu.map').write_bytes(b'synthetic menu')
        (self.root / 'frontend-runtime-selection.json').write_text('previous menu owner')
        replace = os.replace
        injected = False

        def menu(root, runtime, *_):
            return dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime))

        def fail_after_replace(source, target):
            nonlocal injected
            replace(source, target)
            if Path(target) == self.root / target_name and not injected:
                injected = True
                if foreign:
                    foreign(replace)
                raise error('injected publication failure')

        with patch.object(engine.os, 'replace', side_effect=fail_after_replace), \
             patch('qualify_frontend_runtime.qualify', side_effect=menu):
            with self.assertRaisesRegex(error, 'injected publication failure'):
                self.attempt(dict(passed=True))
        self.assertTrue(injected)
        return json.loads((self.args.output / 'report.json').read_text())

    def assert_publication_restored(self, report):
        self.assertFalse(report['passed'])
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual((self.root / 'frontend-runtime-selection.json').read_text(), 'previous menu owner')
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            self.assertEqual((self.root / name).read_text(), 'previous launcher')
            self.assertEqual((self.root / name).stat().st_mode & 0o777, self.launcher_modes[name])
        self.assertFalse(report['publication']['rollback_failed'])
        self.assertFalse(report['publication']['candidate_retained'])
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])

    def test_interruption_after_first_launcher_restores_its_preimage(self):
        self.assert_publication_restored(self.publication_fault('Halo2-Launcher', SystemExit))

    def test_failure_after_second_launcher_restores_both_preimages(self):
        self.assert_publication_restored(self.publication_fault('Halo2-Title-Launcher'))

    def test_failure_after_gameplay_selector_restores_launchers_and_selector(self):
        self.assert_publication_restored(self.publication_fault('gameplay-runtime-selection.json'))

    def test_failure_after_frontend_selector_restores_the_whole_publication(self):
        self.assert_publication_restored(self.publication_fault('frontend-runtime-selection.json'))

    def test_foreign_same_bytes_launcher_is_preserved_and_its_candidate_retained(self):
        foreign_identity = None

        def foreign(replace):
            nonlocal foreign_identity
            target = self.root / 'Halo2-Launcher'
            temporary = self.root / 'foreign-launcher'
            temporary.write_bytes(target.read_bytes())
            temporary.chmod(0o755)
            replace(temporary, target)
            foreign_identity = engine.publication_identity(target)

        report = self.publication_fault('Halo2-Title-Launcher', foreign=foreign)
        self.assertEqual(engine.publication_identity(self.root / 'Halo2-Launcher'), foreign_identity)
        self.assertEqual((self.root / 'Halo2-Title-Launcher').read_text(), 'previous launcher')
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertTrue((self.root / report['runtime']).is_file())
        self.assertTrue(report['publication']['candidate_retained'])
        self.assertIn(dict(name='Halo2-Launcher', status='candidate_reference'),
                      report['publication']['candidate_references'])
        self.assertEqual(report['publication']['files'][0]['outcome'], 'foreign_or_missing_preserved')

    def test_foreign_selector_referencing_candidate_is_preserved_with_candidate(self):
        foreign_bytes = None

        def foreign(replace):
            nonlocal foreign_bytes
            value = json.loads(self.selection.read_text())
            value['foreign_owner'] = True
            temporary = self.root / 'foreign-selection'
            temporary.write_text(json.dumps(value))
            replace(temporary, self.selection)
            foreign_bytes = self.selection.read_bytes()

        report = self.publication_fault('gameplay-runtime-selection.json', foreign=foreign)
        self.assertEqual(self.selection.read_bytes(), foreign_bytes)
        self.assertEqual((self.root / 'Halo2-Launcher').read_text(), 'previous launcher')
        self.assertTrue((self.root / report['runtime']).is_file())
        self.assertTrue(report['publication']['candidate_retained'])
        self.assertIn(dict(name='gameplay-runtime-selection.json', status='candidate_reference'),
                      report['publication']['candidate_references'])

    def test_failed_rollback_never_deletes_the_still_referenced_candidate(self):
        replace = os.replace
        attempted = 0

        def fail_root_launcher(source, target):
            nonlocal attempted
            if Path(target) == self.root / 'Halo2-Launcher':
                attempted += 1
                if attempted == 1:
                    replace(source, target)
                    raise OSError('injected publication failure')
                raise PermissionError('injected rollback failure')
            replace(source, target)

        with patch.object(engine.os, 'replace', side_effect=fail_root_launcher):
            with self.assertRaisesRegex(OSError, 'injected publication failure'):
                self.attempt(dict(passed=True))
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertEqual(attempted, 2)
        self.assertTrue(report['publication']['rollback_failed'])
        self.assertTrue(report['publication']['candidate_retained'])
        self.assertTrue((self.root / report['runtime']).is_file())
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual((self.root / 'Halo2-Title-Launcher').read_text(), 'previous launcher')

    def test_publication_identity_ignores_read_access_time_only(self):
        path = self.root / 'Halo2-Launcher'
        before = path.lstat()
        values = list(before)
        values[7] += 1  # st_atime advances on a read; all content metadata stays fixed.
        after = os.stat_result(values, dict(st_atime_ns=before.st_atime_ns + 1_000_000_000,
                                           st_mtime_ns=before.st_mtime_ns, st_ctime_ns=before.st_ctime_ns))
        self.assertNotEqual(before, after)
        with patch.object(Path, 'lstat', side_effect=[before, after]):
            identity = engine.publication_identity(path)
        self.assertEqual(identity[-1], engine.digest(path))
        self.assertEqual(identity[:5], (before.st_dev, before.st_ino, before.st_size,
                                      before.st_mtime_ns, before.st_mode & 0o777))

    def test_menu_failure_preserves_the_installed_launcher_and_selection(self):
        (self.maps / 'mainmenu.map').write_bytes(b'synthetic menu')
        with patch('qualify_frontend_runtime.qualify', side_effect=RuntimeError('menu check failed')):
            with self.assertRaisesRegex(RuntimeError, 'menu check failed'):
                self.attempt(dict(passed=True))
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual((self.root / 'Halo2-Launcher').read_text(), 'previous launcher')
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])

    def test_future_engine_builds_select_the_same_qualified_menu_owner(self):
        (self.maps / 'mainmenu.map').write_bytes(b'synthetic menu')
        frontend = self.root / 'frontend-runtime-selection.json'
        frontend.write_text('previous menu owner')
        def menu(root, runtime, *_):
            return dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime))
        with patch('qualify_frontend_runtime.qualify', side_effect=menu):
            self.attempt(dict(passed=True))
        game = json.loads(self.selection.read_text())
        menu = json.loads(frontend.read_text())
        self.assertEqual(menu['runtime'], game['runtime'])
        self.assertEqual(menu['runtime_sha256'], game['runtime_sha256'])

    def test_concurrent_menu_selection_is_preserved_during_engine_validation(self):
        (self.maps / 'mainmenu.map').write_bytes(b'synthetic menu')
        frontend = self.root / 'frontend-runtime-selection.json'
        frontend.write_text('previous menu owner')
        def changed(*_):
            frontend.write_text('another task selected its menu')
            return dict(runtime='candidate')
        with patch('qualify_frontend_runtime.qualify', side_effect=changed):
            with self.assertRaisesRegex(RuntimeError, 'Another task changed the frontend'):
                self.attempt(dict(passed=True))
        self.assertEqual(frontend.read_text(), 'another task selected its menu')
        self.assertEqual(self.selection.read_bytes(), self.original_selection)

    def test_status_ignores_test_only_snapshot_files_and_detects_new_engine_modules(self):
        (self.scratch / 'source-hashes.json').write_text(json.dumps(
            dict(engine.source_hashes(self.root), **{'tests/legacy.cpp': 'old test hash'})))
        self.assertIn('matches', engine.source_status(self.root, self.runtime))
        (self.root / 'src').mkdir()
        (self.root / 'src/new.cpp').write_text('new engine code')
        self.assertIn('1 files differ', engine.source_status(self.root, self.runtime))

    def test_new_source_must_reach_the_native_engine_through_the_link_graph(self):
        (self.root / 'src').mkdir()
        (self.root / 'src/new.cpp').write_text('new engine code')
        graph = self.scratch / 'graph.txt'
        for graph_text, expected in (
                ('halo2_native|||src/app/main.cpp\n', False),
                ('halo2_native|||src/app/main.cpp\nnew_library|||src/new.cpp\n', False),
                ('halo2_native|new_library||src/app/main.cpp\nnew_library|||src/new.cpp\n', True)):
            graph.write_text(graph_text)
            self.assertEqual(engine.wiring(graph, self.root)['passed'], expected)

    def test_direct_object_sources_reach_the_engine_through_the_owning_archive(self):
        source = self.root / 'src/context.cpp'
        source.parent.mkdir()
        source.write_text('object implementation')
        graph = self.scratch / 'graph.txt'
        graph.write_text('halo2_native|halo2_systems||src/app/main.cpp\n'
                         'halo2_systems|||$<TARGET_OBJECTS:context>\n'
                         f'context|||{source}\n')
        result = engine.wiring(graph, self.root)
        self.assertTrue(result['passed'])
        self.assertIn('context', result['engine_targets'])
        self.assertIn('src/context.cpp', result['engine_sources'])
        self.assertNotIn('src/context.cpp', result['separate_components'])

    def test_cmake_parent_relative_absolute_sources_keep_their_real_owner(self):
        source = self.root / 'src/context.cpp'
        source.parent.mkdir()
        source.write_text('linked implementation')
        (self.root / 'cmake').mkdir(exist_ok=True)
        graph = self.scratch / 'graph.txt'
        graph.write_text('halo2_native|halo2_systems||src/app/main.cpp\n'
                         f'halo2_systems|||{self.root}/cmake/../src/context.cpp\n')
        result = engine.wiring(graph, self.root)
        self.assertTrue(result['passed'])
        self.assertIn('src/context.cpp', result['engine_sources'])
        self.assertNotIn('src/context.cpp', result['unregistered_sources'])

    def test_absolute_sources_outside_the_cohort_cannot_admit_a_local_source(self):
        source = self.root / 'src/context.cpp'
        source.parent.mkdir()
        source.write_text('local implementation without a linked owner')
        outside = self.root.parent / 'outside.cpp'
        outside.write_text('different implementation')
        graph = self.scratch / 'graph.txt'
        graph.write_text('halo2_native|||src/app/main.cpp;'
                         f'{self.root}/src/../../outside.cpp\n')
        result = engine.wiring(graph, self.root)
        self.assertFalse(result['passed'])
        self.assertEqual(result['unexpected_unwired_sources'], ['src/context.cpp'])
        self.assertFalse(any('..' in name for name in result['engine_sources']))

    def test_build_dependencies_and_conditional_object_sources_do_not_prove_membership(self):
        source = self.root / 'src/context.cpp'
        source.parent.mkdir()
        source.write_text('object implementation')
        graph = self.scratch / 'graph.txt'
        for owner in ('halo2_systems||context|\n',
                      'halo2_systems|||$<$<BOOL:0>:$<TARGET_OBJECTS:context>>\n',
                      'unlinked_test|||$<TARGET_OBJECTS:context>\n'):
            with self.subTest(owner=owner):
                graph.write_text('halo2_native|halo2_systems||src/app/main.cpp\n' +
                    owner + f'context|||{source}\n')
                result = engine.wiring(graph, self.root)
                self.assertFalse(result['passed'])
                self.assertNotIn('context', result['engine_targets'])
                self.assertEqual(result['unexpected_unwired_sources'], ['src/context.cpp'])

    def test_camera_reference_requires_the_linked_shared_runtime_owner(self):
        reference='src/campaign/scripting/source_workshop_camera_start_tu15_generated.cpp'
        production='src/campaign/scripting/source_workshop_script_builtins_tu15_generated.cpp'
        for name in (reference,production):
            path=self.root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text('native camera source')
        graph=self.scratch/'graph.txt'
        rest=f'halo2_systems|||{production}\ncamera_reference_test|||{reference}\n'
        graph.write_text('halo2_native|halo2_systems||src/app/main.cpp\n'+rest)
        result=engine.wiring(graph,self.root)
        self.assertTrue(result['passed'])
        self.assertIn(reference,result['component_boundaries'])
        self.assertIn(production,result['engine_sources'])
        self.assertNotIn(production,result['component_boundaries'])
        graph.write_text('halo2_native|||src/app/main.cpp\n'+rest)
        result=engine.wiring(graph,self.root)
        self.assertFalse(result['passed'])
        self.assertEqual(set(result['unexpected_unwired_sources']),{reference,production})

    def test_unadmitted_decorator_components_keep_an_explicit_boundary(self):
        components = (
            'src/rendering/world/source_decorator_decals_tu15.cpp',
            'src/rendering/world/source_decorator_owners_tu15_generated.cpp',
            'src/rendering/effects/source_decorator_decal_draw.cpp',
            'src/rendering/effects/source_decorator_decal_material.cpp')
        for name in components:
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('separate decorator component')
        graph = self.scratch / 'graph.txt'
        graph.write_text('halo2_native|||src/app/main.cpp\n')
        result = engine.wiring(graph, self.root)
        self.assertTrue(result['passed'])
        self.assertEqual(set(result['component_boundaries']), set(components))
        self.assertTrue(set(components).isdisjoint(result['engine_sources']))
        unknown = self.root / 'src/rendering/effects/source_decorator_unknown.cpp'
        unknown.write_text('unregistered production source')
        self.assertEqual(engine.wiring(graph, self.root)['unexpected_unwired_sources'],
                         ['src/rendering/effects/source_decorator_unknown.cpp'])

    def test_unadmitted_loading_components_keep_an_explicit_boundary(self):
        components = (
            'src/rendering/loading/source_loading_program_tu15_generated.cpp',
            'src/ui/source_loading_assets.cpp')
        for name in components:
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('loading component without a production caller')
        graph = self.scratch / 'graph.txt'
        graph.write_text('halo2_native|||src/app/main.cpp\n')
        result = engine.wiring(graph, self.root)
        self.assertTrue(result['passed'])
        self.assertEqual(set(result['component_boundaries']), set(components))
        self.assertTrue(set(components).isdisjoint(result['engine_sources']))
        for name in components:
            self.assertIn('without a production', result['component_boundaries'][name])
        unknown = ('src/rendering/loading/source_loading_unknown.cpp',
                   'src/ui/source_loading_unknown.cpp')
        for name in unknown:
            (self.root / name).write_text('unregistered loading source remains a wiring failure')
        result = engine.wiring(graph, self.root)
        self.assertFalse(result['passed'])
        self.assertEqual(result['unexpected_unwired_sources'], list(unknown))

    def test_native_motion_pose_component_keeps_the_original_virtual_owner_boundary(self):
        name = 'src/physics/native_tu15_motion_pose.cpp'
        source = self.root / name
        source.parent.mkdir(parents=True)
        source.write_text('standalone native pose component')
        graph = self.scratch / 'graph.txt'
        graph.write_text('halo2_native|||src/app/main.cpp\n')
        result = engine.wiring(graph, self.root)
        self.assertTrue(result['passed'])
        self.assertNotIn(name, result['engine_sources'])
        self.assertIn('original production virtual owner', result['component_boundaries'][name])
        unknown = source.with_name('native_tu15_motion_pose_unknown.cpp')
        unknown.write_text('unknown source remains a failure')
        self.assertEqual(engine.wiring(graph, self.root)['unexpected_unwired_sources'],
                         ['src/physics/native_tu15_motion_pose_unknown.cpp'])

    def test_unadmitted_typed_contact_and_seat_components_do_not_claim_runtime_delivery(self):
        names = ('src/gameplay/vehicles/native_tu15_seat_look.cpp',
                 'src/physics/native_tu15_contact_candidates.cpp')
        for name in names:
            path = self.root/name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('standalone component with no production caller')
        graph = self.scratch/'graph.txt'
        graph.write_text('halo2_native|||src/app/main.cpp\n')
        result = engine.wiring(graph, self.root)
        self.assertTrue(result['passed'])
        self.assertEqual(set(result['component_boundaries']), set(names))
        self.assertTrue(set(names).isdisjoint(result['engine_sources']))
        unknown = 'src/physics/native_tu15_contact_candidates_live.cpp'
        (self.root/unknown).write_text('new source still requires its own admission')
        self.assertEqual(engine.wiring(graph, self.root)['unexpected_unwired_sources'], [unknown])

    def test_unadmitted_title_and_physics_components_do_not_claim_runtime_delivery(self):
        names = ('src/campaign/source_workshop_titles_tu15_generated.cpp',
                 'src/physics/native_tu15_collision_component.cpp',
                 'src/physics/native_tu15_collision_surface_shape.cpp',
                 'src/physics/native_tu15_contact_point_removal.cpp',
                 'src/physics/native_tu15_contact_query.cpp',
                 'src/physics/native_tu15_convex_contact_query.cpp',
                 'src/physics/native_tu15_island_partition_graph.cpp',
                 'src/physics/native_tu15_pair_creation_queues.cpp',
                 'src/physics/native_tu15_phantom_pair_create.cpp',
                 'src/physics/native_tu15_simplex_face_selection.cpp')
        for name in names:
            path = self.root/name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('standalone component without an admitted production caller')
        graph = self.scratch/'graph.txt'
        graph.write_text('halo2_native|||src/app/main.cpp\n')
        result = engine.wiring(graph, self.root)
        self.assertTrue(result['passed'])
        self.assertEqual(set(result['component_boundaries']), set(names))
        self.assertTrue(set(names).isdisjoint(result['engine_sources']))
        unknown = 'src/physics/native_tu15_contact_query_live.cpp'
        (self.root/unknown).write_text('unregistered source still needs its own admission')
        result = engine.wiring(graph, self.root)
        self.assertFalse(result['passed'])
        self.assertEqual(result['unexpected_unwired_sources'], [unknown])

    def test_zoom_delivery_is_separate_only_in_an_explicit_selected_protocol_cohort(self):
        name = 'src/gameplay/weapons/native_weapon_zoom_audio_tu15.cpp'
        path = self.root/name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('production zoom delivery requires its state/wire cohort')
        graph = self.scratch/'graph.txt'
        graph.write_text('halo2_native|||src/app/main.cpp\n')
        self.assertEqual(engine.wiring(graph, self.root)['unexpected_unwired_sources'], [name])
        policy = self.root/'cmake/NativeWeaponEffectsTu15.cmake'
        marker = '# This selected-protocol cohort retains the later zoom journal delivery as\n'
        policy.write_text(marker)
        result = engine.wiring(graph, self.root)
        self.assertTrue(result['passed'])
        self.assertIn(name, result['component_boundaries'])
        policy.write_text(marker + 'target_sources(halo2_systems PRIVATE ' + name + ')\n')
        self.assertEqual(engine.wiring(graph, self.root)['unexpected_unwired_sources'], [name])

    def test_linked_zoom_delivery_never_becomes_a_separate_component(self):
        name = 'src/gameplay/weapons/native_weapon_zoom_audio_tu15.cpp'
        path = self.root/name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('ordinary production zoom delivery')
        graph = self.scratch/'graph.txt'
        graph.write_text('halo2_native|||src/app/main.cpp;' + name + '\n')
        result = engine.wiring(graph, self.root)
        self.assertTrue(result['passed'])
        self.assertIn(name, result['engine_sources'])
        self.assertNotIn(name, result['component_boundaries'])

    def test_unadmitted_broad_phase_and_solver_keep_explicit_residual_boundaries(self):
        components = (
            'src/physics/native_tu15_broad_phase.cpp',
            'src/physics/native_tu15_constraint_solver.cpp',
            'src/physics/native_tu15_constraint_solver_rows.cpp')
        for name in components:
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('standalone component without an admitted production caller')
        graph = self.scratch / 'graph.txt'
        graph.write_text('halo2_native|||src/app/main.cpp\n')
        result = engine.wiring(graph, self.root)
        self.assertTrue(result['passed'])
        self.assertEqual(set(result['unregistered_sources']), set(components))
        self.assertEqual(set(result['component_boundaries']), set(components))
        self.assertTrue(set(components).isdisjoint(result['engine_sources']))
        self.assertIn('0x321700 residual', result['component_boundaries'][components[0]])
        for name in components[1:]:
            self.assertIn('0x326A40 residual', result['component_boundaries'][name])
        unknown = self.root / 'src/physics/native_tu15_constraint_solver_unknown.cpp'
        unknown.write_text('unknown source remains a wiring failure')
        result = engine.wiring(graph, self.root)
        self.assertFalse(result['passed'])
        self.assertEqual(result['unexpected_unwired_sources'],
                         ['src/physics/native_tu15_constraint_solver_unknown.cpp'])

    def test_offline_xbox_asset_controls_keep_an_explicit_reference_boundary(self):
        components = (
            'src/ui/original_matchmaking_fonts.cpp',
            'src/ui/original_matchmaking_emblems.cpp',
            'src/ui/original_matchmaking_audio.cpp',
            'src/ui/original_loading_assets.cpp')
        for name in components:
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('offline Xbox reference control')
        graph = self.scratch / 'graph.txt'
        graph.write_text('halo2_native|||src/app/main.cpp\n' + ''.join(
            f'offline_reference_{index}|||{self.root / name}\n'
            for index, name in enumerate(components)))
        result = engine.wiring(graph, self.root)
        self.assertTrue(result['passed'])
        self.assertEqual(set(result['component_boundaries']), set(components))
        self.assertTrue(set(components).isdisjoint(result['engine_sources']))
        for index, name in enumerate(components):
            self.assertEqual(result['separate_components'][name], [f'offline_reference_{index}'])
            self.assertIn('excluded from the MCC runtime', result['component_boundaries'][name])
        unknown = self.root / 'src/ui/original_unknown.cpp'
        unknown.write_text('unregistered source remains a wiring failure')
        self.assertEqual(engine.wiring(graph, self.root)['unexpected_unwired_sources'],
                         ['src/ui/original_unknown.cpp'])


class CachePublicationTests(unittest.TestCase):
    def setUp(self):
        fixture = EnginePromotionTests()
        fixture.setUp()
        self.addCleanup(fixture.doCleanups)
        self.root, self.scratch = fixture.root, fixture.scratch
        self.destination = self.root / 'bin/engine-builds/candidate'
        self.destination.mkdir(parents=True)
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            (self.destination / name).write_text('new launcher')
        (self.root / 'frontend-runtime-selection.json').write_text('previous menu')
        self.registry = self.root / 'assets/mcc-source.tsv'
        self.registry.write_text('H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-test\n')
        self.candidate = self.root / 'assets/imported/mcc-candidate'
        self.candidate.mkdir()
        self.record = dict(runtime='bin/engine-builds/candidate/halo2_native',
                           cache=self.candidate.relative_to(self.root).as_posix())
        self.frontend = dict(runtime=self.record['runtime'])
        self.names = ('Halo2-Launcher', 'Halo2-Title-Launcher', 'gameplay-runtime-selection.json',
                      'frontend-runtime-selection.json', 'assets/mcc-source.tsv')
        self.previous = {name: (self.root / name).read_bytes() for name in self.names}
        self.baseline = {name: engine.publication_identity(self.root / name) for name in self.names}
        self.report = {}

    def publish(self, reader=None):
        def admitted(root):
            self.assertEqual(self.registry.read_text(),
                'H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-candidate\n')
            return json.loads((root / 'gameplay-runtime-selection.json').read_text())
        with patch('mcc_onboarding.private_cache', return_value=(self.candidate, {})) as verify, \
             patch('gameplay_runtime_selection.selected_gameplay_runtime', side_effect=reader or admitted) as admission:
            engine.publish_runtime_files(self.root, self.destination, self.scratch, self.record,
                                         self.frontend, self.report, self.baseline, cache_choice=self.candidate)
        verify.assert_called_once_with(self.root, self.candidate, verify=True)
        admission.assert_called_once_with(self.root)

    def restored(self):
        self.assertEqual({name: (self.root / name).read_bytes() for name in self.names}, self.previous)
        self.assertFalse(self.report['publication']['complete'])
        self.assertFalse(self.report['publication']['rollback_failed'])

    def test_success_publishes_raw_tsv_fifth_and_checks_normal_reader(self):
        self.publish()
        self.assertTrue(self.report['publication']['complete'])
        self.assertEqual([row['name'] for row in self.report['publication']['files']], list(self.names))
        self.assertEqual(json.loads((self.root / 'gameplay-runtime-selection.json').read_text()), self.record)

    def test_fifth_file_failure_before_and_after_committed_rename_rolls_back_all_files(self):
        replace = os.replace
        for committed in (False, True):
            with self.subTest(committed=committed):
                self.baseline = {name: engine.publication_identity(self.root / name) for name in self.names}
                injected = False

                def fail(source, target):
                    nonlocal injected
                    if Path(target) == self.registry and not injected:
                        injected = True
                        if committed:
                            replace(source, target)
                        raise OSError('fifth rename failed')
                    replace(source, target)

                with patch.object(engine.os, 'replace', side_effect=fail):
                    with self.assertRaisesRegex(OSError, 'fifth rename failed'):
                        self.publish()
                self.assertTrue(injected)
                self.restored()

    def test_foreign_registry_inode_survives_rollback_even_with_candidate_bytes(self):
        replace = os.replace
        foreign_identity = None
        injected = False

        def fail(source, target):
            nonlocal foreign_identity, injected
            replace(source, target)
            if Path(target) == self.registry and not injected:
                injected = True
                foreign = self.registry.with_name('foreign-choice')
                foreign.write_bytes(self.registry.read_bytes())
                replace(foreign, self.registry)
                foreign_identity = engine.publication_identity(self.registry)
                raise OSError('fifth rename interrupted')

        with patch.object(engine.os, 'replace', side_effect=fail):
            with self.assertRaisesRegex(OSError, 'fifth rename interrupted'):
                self.publish()
        self.assertEqual(engine.publication_identity(self.registry), foreign_identity)
        for name in self.names[:-1]:
            self.assertEqual((self.root / name).read_bytes(), self.previous[name])
        self.assertEqual(self.report['publication']['files'][-1]['outcome'], 'foreign_or_missing_preserved')

    def test_changed_registry_baseline_prevents_any_publication(self):
        self.registry.write_text('foreign cache choice')
        with self.assertRaisesRegex(RuntimeError, 'Another task changed publication file: assets/mcc-source.tsv'):
            self.publish()
        self.assertEqual(self.registry.read_text(), 'foreign cache choice')
        for name in self.names[:-1]:
            self.assertEqual((self.root / name).read_bytes(), self.previous[name])

    def test_failed_normal_reader_rolls_back_all_five_files(self):
        def reject(root):
            raise ValueError('normal admission failed')
        with self.assertRaisesRegex(ValueError, 'normal admission failed'):
            self.publish(reject)
        self.restored()


class Tu15GameplayReferenceTests(unittest.TestCase):
    def setUp(self):
        self.owner=tempfile.TemporaryDirectory();self.addCleanup(self.owner.cleanup)
        self.root=Path(self.owner.name)
        self.image=self.root/'halo2-native/imports/halo2-dlc-fixture/Halo 2 DLC/content/u5English/default.xbe'
        self.image.parent.mkdir(parents=True);self.image.write_bytes(b'explicit original-image identity fixture')
        self.pin='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'

    def test_explicit_owned_image_binds_exact_path_and_identity(self):
        with patch.object(engine,'digest',return_value=self.pin):
            result=engine.tu15_gameplay_reference(self.image)
        self.assertEqual(result,dict(path=str(self.image.resolve()),bytes=self.image.stat().st_size,sha256=self.pin))

    def test_explicit_other_image_is_rejected(self):
        with patch.object(engine,'digest',return_value='0'*64):
            with self.assertRaisesRegex(RuntimeError,'English TU1.5'):
                engine.tu15_gameplay_reference(self.image)

    def test_installed_import_is_discovered_without_empty_cmake_input(self):
        with patch.dict(os.environ,{'XDG_DATA_HOME':str(self.root),'HALO2_TU15_GAMEPLAY_XBE':''}),patch.object(engine,'digest',return_value=self.pin):
            result=engine.tu15_gameplay_reference()
        self.assertEqual(result['path'],str(self.image.resolve()))
        self.assertEqual(result['sha256'],self.pin)

    def test_unqualified_import_is_not_used(self):
        with patch.dict(os.environ,{'XDG_DATA_HOME':str(self.root),'HALO2_TU15_GAMEPLAY_XBE':''}),patch.object(engine,'digest',return_value='0'*64):
            self.assertIsNone(engine.tu15_gameplay_reference())


if __name__ == '__main__':
    unittest.main()
