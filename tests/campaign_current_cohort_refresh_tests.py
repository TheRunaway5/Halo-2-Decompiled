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
                                         'current_campaign_opening_release .... Passed\n')
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
                'current_campaign_new_game_startup .... Passed\ncurrent_campaign_opening_release .... Passed\n')
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
