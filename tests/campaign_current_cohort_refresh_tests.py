#!/usr/bin/env python3
"""Controller-only regressions; no client, campaign state or gameplay results."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('campaign_cohort', ROOT/'tools/test_campaign_current_cohort.py')
cohort = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cohort)


class RefreshTests(unittest.TestCase):
    def setUp(self):
        if 'HALO2_TEST_SCRATCH' not in os.environ:
            self.fail('Run through tools/run_test.py')
        self.work = Path(os.environ['HALO2_TEST_SCRATCH']) / self._testMethodName
        self.work.mkdir()
        self.root = self.work/'checkout'
        (self.root/'src').mkdir(parents=True)
        for name, value in [('CMakeLists.txt','project(example)\n'),('AGENTS.md','owned tests\n'),
                            ('.gitignore','build\n'),('src/a.cpp','old a\n'),('src/b.cpp','old b\n')]:
            (self.root/name).write_text(value)
        self.source = self.work/'source'
        shutil.copytree(self.root,self.source)
        self.root_patch = patch.object(cohort,'ROOT',self.root)
        self.directories_patch = patch.object(cohort,'SOURCE_DIRECTORIES',('src',))
        self.root_patch.start(); self.directories_patch.start()
        self.addCleanup(self.root_patch.stop); self.addCleanup(self.directories_patch.stop)
        self.inputs = cohort.current_input_manifest(set())

    def manifest(self, values=None):
        path = self.work/'refresh-manifest.json'
        path.write_text(json.dumps(cohort.current_input_manifest(set()) if values is None else values))
        return path,cohort.sha(path)

    def test_inventory_prunes_only_existing_exclusions(self):
        paths=('tools/reference/deep/original.xbe','tools/kept.py',
               'rust/target/deep/output.o','rust/crate/target/kept.cpp',
               'tests/fixtures/selected.json','tests/fixtures/selected_group/kept.json',
               'tests/fixtures/unselected/deep/large.json','tests/case.cpp',
               'third_party/nested/.git/deep/config','src/__pycache__/deep/cache.pyc')
        for name in paths:
            p=self.root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(name)
        (self.root/'src/alias.cpp').symlink_to(self.root/'src/a.cpp')
        (self.root/'src/directory_alias').symlink_to(self.root/'tools',target_is_directory=True)
        selected={'selected.json','selected_group'};directories=('src','tools','rust','tests','third_party')
        expected={'CMakeLists.txt','AGENTS.md','.gitignore'}
        with patch.object(cohort,'SOURCE_DIRECTORIES',directories):
            for directory in directories:
                for path in (self.root/directory).rglob('*'):
                    if not path.is_file():continue
                    name=str(path.relative_to(self.root))
                    try:cohort.refresh_input_path(name,selected)
                    except ValueError:continue
                    expected.add(name)
            visited=[];walk=os.walk
            def observed(*args,**kwargs):
                for parent,children,files in walk(*args,**kwargs):
                    visited.append(Path(parent).relative_to(self.root));yield parent,children,files
            with patch.object(cohort.os,'walk',observed):
                actual=cohort.current_input_manifest(selected)
        self.assertEqual(actual,{name:cohort.sha(self.root/name)for name in sorted(expected)})
        excluded=('tools/reference','rust/target','tests/fixtures/unselected',
                  'third_party/nested/.git','src/__pycache__','src/directory_alias')
        self.assertFalse([p for p in visited if any(p.is_relative_to(n)for n in excluded)])
        self.assertIn('rust/crate/target/kept.cpp',actual)

    def test_refresh_preserves_exact_bytes_and_removes_only_declared_owned_files(self):
        (self.root/'src/a.cpp').write_text('updated a\n')
        (self.root/'src/c.cpp').write_text('new c\n')
        (self.root/'src/b.cpp').unlink()
        manifest,digest = self.manifest()
        current,receipt = cohort.declared_refresh(self.source,self.inputs,manifest,digest,set())
        self.assertEqual(current,cohort.current_input_manifest(set()))
        self.assertEqual((self.source/'src/a.cpp').read_text(),'updated a\n')
        self.assertEqual((self.source/'src/c.cpp').read_text(),'new c\n')
        self.assertFalse((self.source/'src/b.cpp').exists())
        self.assertEqual(receipt['removed'],['src/b.cpp'])
        self.assertEqual(set(receipt['changed']),{'src/a.cpp','src/c.cpp'})

    def test_incomplete_manifest_and_bad_pin_rejected_before_publication(self):
        (self.root/'src/a.cpp').write_text('updated a\n')
        values=cohort.current_input_manifest(set()); del values['src/b.cpp']
        manifest,digest=self.manifest(values)
        with self.assertRaisesRegex(ValueError,'complete captured source set'):
            cohort.declared_refresh(self.source,self.inputs,manifest,digest,set())
        self.assertEqual((self.source/'src/a.cpp').read_text(),'old a\n')
        with self.assertRaisesRegex(ValueError,'SHA-256 differs'):
            cohort.declared_refresh(self.source,self.inputs,manifest,'0'*64,set())

    def test_borrowed_destination_and_unsafe_paths_rejected(self):
        (self.root/'src/a.cpp').write_text('updated a\n')
        manifest,digest=self.manifest()
        borrowed=self.work/'borrowed';shutil.move(self.source/'src',borrowed)
        (self.source/'src').symlink_to(borrowed,target_is_directory=True)
        with self.assertRaisesRegex(ValueError,'borrowed directory owner'):
            cohort.declared_refresh(self.source,self.inputs,manifest,digest,set())
        self.assertEqual((borrowed/'a.cpp').read_text(),'old a\n')
        for name in ('../src/a.cpp','/src/a.cpp','tools/reference/input.xbe','tests/fixtures/unselected.json'):
            with self.assertRaises(ValueError):cohort.refresh_input_path(name,set())

    def test_copy_race_leaves_entire_frozen_source_untouched(self):
        (self.root/'src/a.cpp').write_text('updated a\n')
        (self.root/'src/b.cpp').write_text('updated b\n')
        manifest,digest=self.manifest();copy=cohort.stage_refresh_file
        with (self.source/'CMakeLists.txt').open('a')as stream:stream.write('# prior owned test append\n')
        before={str(p.relative_to(self.source)):(p.read_bytes(),p.stat().st_mtime_ns)for p in self.source.rglob('*')if p.is_file()}
        def race(source,destination,*args,**kwargs):
            if Path(source)==self.root/'src/b.cpp':Path(source).write_text('concurrent undeclared b\n')
            return copy(source,destination,*args,**kwargs)
        with patch.object(cohort,'stage_refresh_file',race):
            with self.assertRaises(ValueError):
                cohort.declared_refresh(self.source,self.inputs,manifest,digest,set())
        after={str(p.relative_to(self.source)):(p.read_bytes(),p.stat().st_mtime_ns)for p in self.source.rglob('*')if p.is_file()}
        self.assertEqual(after,before,'A rejected checkout capture partially changed the frozen cohort')
        self.assertFalse(list(self.work.glob('refresh-stage-*')))

    def test_staging_byte_limit_rejects_without_publication(self):
        (self.root/'src/a.cpp').write_text('updated a\n');manifest,digest=self.manifest()
        with patch.object(cohort,'MAX_REFRESH_STAGED_BYTES',1):
            with self.assertRaisesRegex(cohort.RefreshCaptureRejected,'bounded byte budget'):
                cohort.declared_refresh(self.source,self.inputs,manifest,digest,set())
        self.assertEqual((self.source/'src/a.cpp').read_text(),'old a\n')
        self.assertFalse(list(self.work.glob('refresh-stage-*')))

    def test_partial_publication_io_error_rolls_back_bytes_and_mtimes(self):
        for name in('a','b'):(self.root/f'src/{name}.cpp').write_text(f'updated {name}\n')
        manifest,digest=self.manifest();replace=os.replace
        before={str(p.relative_to(self.source)):(p.read_bytes(),p.stat().st_mtime_ns)for p in self.source.rglob('*')if p.is_file()}
        def fail(source,destination):
            if Path(destination)==self.source/'src/b.cpp'and'inputs'in Path(source).parts:
                raise OSError('injected publication failure')
            return replace(source,destination)
        with patch.object(cohort.os,'replace',fail):
            with self.assertRaisesRegex(OSError,'publication failure'):
                cohort.declared_refresh(self.source,self.inputs,manifest,digest,set())
        after={str(p.relative_to(self.source)):(p.read_bytes(),p.stat().st_mtime_ns)for p in self.source.rglob('*')if p.is_file()}
        self.assertEqual(before,after);self.assertFalse(list(self.work.glob('refresh-stage-*')))

    def worker_setup(self):
        output=self.work/'output';output.mkdir()
        args=SimpleNamespace(refresh_control=self.work/'control.json',owned_refreshes=2,
            refresh_wait_seconds=1,output=output,mcc_root=self.work/'mcc',ticks=120,timeout=300)
        calls=[]
        def worker(command,cwd):
            path=Path(command[command.index('--output')+1]);path.mkdir()
            (path/'tests.log').write_text('current_campaign_new_game_frontend .... Passed\n'
                'current_campaign_new_game_startup .... Passed\ncurrent_campaign_opening_release .... Passed\n'
                'current_campaign_session_timers_world .... Passed\n')
            (path.parent/'new-game-startup.json').write_text('{"passed":true}')
            calls.append(command);return 0
        command=['python3','tools/run_tests.py','--output',str(output/'validation')]
        return args,calls,worker,command

    def test_capture_rejection_waits_for_explicit_same_sequence_replacement(self):
        args,calls,worker,command=self.worker_setup();report={};real_stage=cohort.stage_refresh_file
        declared=False;raced=False;frozen=None
        def stage(source,destination,*values):
            nonlocal raced
            if not raced and Path(source)==self.root/'src/b.cpp':
                Path(source).write_text('concurrent undeclared b\n');raced=True
            return real_stage(source,destination,*values)
        def save():
            nonlocal declared,frozen
            if not report['phase'].startswith('waiting'):return
            if len(report['iterations'])==2:
                args.refresh_control.write_text(json.dumps(dict(sequence=2,action='finish')));return
            if not declared:
                (self.root/'src/a.cpp').write_text('updated a\n');(self.root/'src/b.cpp').write_text('updated b\n')
                frozen={str(p.relative_to(self.source)):(p.read_bytes(),p.stat().st_mtime_ns)for p in self.source.rglob('*')if p.is_file()};declared=True
            elif report['refresh_capture_rejections']:
                current={str(p.relative_to(self.source)):(p.read_bytes(),p.stat().st_mtime_ns)for p in self.source.rglob('*')if p.is_file()}
                self.assertEqual(current,frozen)
            manifest,digest=self.manifest()
            args.refresh_control.write_text(json.dumps(dict(sequence=1,action='refresh',manifest=str(manifest),manifest_sha256=digest)))
        with patch.object(cohort,'stage_refresh_file',stage),patch.object(cohort.subprocess,'call',worker):
            result=cohort.owned_new_game_iterations(args,self.source,self.work/'maps',self.inputs,set(),command,
                (self.root/'CMakeLists.txt').read_text(),report,save)
        self.assertEqual(result,0);self.assertEqual(len(calls),2)
        self.assertEqual(len(report['refresh_capture_rejections']),1)
        self.assertEqual((self.source/'src/b.cpp').read_text(),'concurrent undeclared b\n')
        self.assertFalse(list(self.work.glob('refresh-stage-*')))

    def test_identical_rejected_manifest_is_not_retried_or_given_new_deadline(self):
        args,calls,worker,command=self.worker_setup();report={};clock=[0.]
        def save():
            if report['phase'].startswith('waiting')and not args.refresh_control.exists():
                manifest,digest=self.manifest();args.refresh_control.write_text(json.dumps(dict(sequence=1,
                    action='refresh',manifest=str(manifest),manifest_sha256=digest)))
        def sleep(seconds):clock[0]+=seconds
        with patch.object(cohort.subprocess,'call',worker),patch.object(cohort.time,'monotonic',lambda:clock[0]),patch.object(cohort.time,'sleep',sleep),patch.object(cohort,'declared_refresh',side_effect=cohort.RefreshCaptureRejected('raced'))as capture:
            result=cohort.owned_new_game_iterations(args,self.source,self.work/'maps',self.inputs,set(),command,
                (self.root/'CMakeLists.txt').read_text(),report,save)
        self.assertEqual(result,1);self.assertEqual(capture.call_count,1);self.assertEqual(len(calls),1)
        self.assertEqual(clock[0],1.);self.assertTrue(report['refresh_control_expired'])

    def test_three_explicit_capture_rejections_are_bounded(self):
        args,calls,worker,command=self.worker_setup();report={};pins=[]
        def save():
            if report['phase'].startswith('waiting'):
                manifest,digest=self.manifest();manifest.write_text(manifest.read_text()+' '*len(report['refresh_capture_rejections']))
                digest=cohort.sha(manifest);pins.append(digest)
                args.refresh_control.write_text(json.dumps(dict(sequence=1,action='refresh',manifest=str(manifest),manifest_sha256=digest)))
        with patch.object(cohort.subprocess,'call',worker),patch.object(cohort,'declared_refresh',side_effect=cohort.RefreshCaptureRejected('raced'))as capture:
            result=cohort.owned_new_game_iterations(args,self.source,self.work/'maps',self.inputs,set(),command,
                (self.root/'CMakeLists.txt').read_text(),report,save)
        self.assertEqual(result,1);self.assertEqual(capture.call_count,3);self.assertEqual(len(set(pins)),3)
        self.assertEqual(len(calls),1);self.assertEqual(report['phase'],'refresh capture rejection limit reached')

    def test_iteration_registration_uses_accepted_frozen_cmake(self):
        args,calls,worker,command=self.worker_setup();report={};real_refresh=cohort.declared_refresh
        def refresh(*values):
            result=real_refresh(*values)
            (self.root/'CMakeLists.txt').write_text('concurrent undeclared CMake\n')
            return result
        def save():
            if report['phase'].startswith('waiting'):
                if len(report['iterations'])==1:
                    (self.root/'src/a.cpp').write_text('updated a\n');manifest,digest=self.manifest()
                    args.refresh_control.write_text(json.dumps(dict(sequence=1,action='refresh',manifest=str(manifest),manifest_sha256=digest)))
                else:args.refresh_control.write_text(json.dumps(dict(sequence=2,action='finish')))
        with patch.object(cohort.subprocess,'call',worker),patch.object(cohort,'declared_refresh',refresh):
            result=cohort.owned_new_game_iterations(args,self.source,self.work/'maps',self.inputs,set(),command,
                (self.root/'CMakeLists.txt').read_text(),report,save)
        self.assertEqual(result,0);self.assertEqual(len(calls),2)
        self.assertNotIn('concurrent undeclared CMake',(self.source/'CMakeLists.txt').read_text())
        self.assertIn('CMakeLists.txt',report['checkout_drift'])

    def test_finite_two_iteration_worker_and_finish(self):
        output=self.work/'output';output.mkdir()
        control=self.work/'control.json'
        args=SimpleNamespace(refresh_control=control,owned_refreshes=2,refresh_wait_seconds=1,
                             output=output,mcc_root=self.work/'mcc',ticks=120,timeout=300)
        command=['python3','tools/run_tests.py','--output',str(output/'validation'),
                 '--target','halo2_native','--target','frontend_tests']
        report={};calls=[]
        def worker(command,cwd):
            self.assertEqual(command[2],'--build-worker')
            self.assertEqual(cwd,self.source)
            path=Path(command[command.index('--output')+1]);path.mkdir()
            (path/'tests.log').write_text('current_campaign_new_game_frontend .... Passed\n'
                                         'current_campaign_new_game_startup .... Passed\n'
                                         'current_campaign_opening_release .... Passed\n'
                                         'current_campaign_session_timers_world .... Passed\n')
            (path.parent/'new-game-startup.json').write_text('{"passed":true}\n')
            calls.append(command)
            return 0
        def save():
            if report['phase'].startswith('waiting'):
                sequence=len(report['iterations'])
                if sequence==1:
                    (self.root/'src/a.cpp').write_text('updated a\n')
                    manifest,digest=self.manifest()
                    instruction=dict(sequence=1,action='refresh',manifest=str(manifest),manifest_sha256=digest)
                else:instruction=dict(sequence=2,action='finish')
                temporary=control.with_suffix('.tmp');temporary.write_text(json.dumps(instruction));temporary.replace(control)
        with patch.object(cohort.subprocess,'call',worker):
            result=cohort.owned_new_game_iterations(args,self.source,self.work/'maps',self.inputs,set(),
                command,(self.root/'CMakeLists.txt').read_text(),report,save)
        self.assertEqual(result,0)
        self.assertEqual(len(calls),2)
        self.assertEqual(len(report['iterations']),2)
        self.assertTrue(all(row['passed'] for row in report['iterations']))
        self.assertTrue(all(not row['snapshot_changed'] for row in report['iterations']))
        self.assertEqual(set(report['iterations'][1]['declared_refresh']['changed']),{'src/a.cpp'})
        self.assertFalse(report['native_only_engine_qualified'])

    def test_graphics_success_finishes_without_an_idle_refresh(self):
        output=self.work/'output';output.mkdir()
        args=SimpleNamespace(refresh_control=self.work/'control.json',owned_refreshes=2,
            refresh_wait_seconds=1,output=output,mcc_root=self.work/'mcc',ticks=120,
            timeout=300,graphics_batch=True)
        report={};commands=[]
        def install(args,source,out):return dict(report=str(out/'graphics-batch.json'),passed=False)
        def worker(command,cwd):
            commands.append(command);path=Path(command[command.index('--output')+1]);path.mkdir()
            (path/'tests.log').write_text('current_campaign_new_game_frontend .... Passed\n'
                'current_campaign_new_game_startup .... Passed\ncurrent_campaign_opening_release .... Passed\n'
                'current_campaign_session_timers_world .... Passed\n')
            (path.parent/'new-game-startup.json').write_text('{"passed":true}')
            (path.parent/'graphics-batch.json').write_text('{"passed":true}')
            return 0
        with patch.object(cohort,'install_graphics_batch_hook',install),patch.object(cohort.subprocess,'call',worker):
            code=cohort.owned_new_game_iterations(args,self.source,self.work/'maps',self.inputs,set(),
                ['python3','tools/run_tests.py','--output',str(output/'validation')],
                (self.root/'CMakeLists.txt').read_text(),report,lambda:None)
        self.assertEqual(code,0);self.assertEqual(len(commands),1)
        self.assertEqual(len(report['iterations']),1);self.assertTrue(report['passed'])


if __name__=='__main__':
    unittest.main()
