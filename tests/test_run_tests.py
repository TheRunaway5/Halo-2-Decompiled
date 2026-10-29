"""Build-only runner policy without executing CMake, CTest, or the engine."""
import argparse
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import run_tests as runner


class BuildOnlyRunnerTests(unittest.TestCase):
    def setUp(self):
        self.owner=tempfile.TemporaryDirectory(prefix='build-only-policy-',
            dir=os.environ['HALO2_TEST_SCRATCH'])
        self.addCleanup(self.owner.cleanup)
        self.root=Path(self.owner.name);self.scratch=self.root/'scratch';self.scratch.mkdir()
        self.source=self.root/'source';self.source.mkdir()
        self.output=self.root/'reports';self.output.mkdir()
        self.args=argparse.Namespace(source=self.source,output=self.output,
            scratch_root=self.root/'private',target=['example_target'],filter='^required_check$',
            jobs=2,test_jobs=1,config='Release',sanitizers=False,cmake_arg=[],
            retain_report=[],keep_artifacts=False,build_worker=True,build_only=False)
        self.commands=[]

    def step(self,command,log,**kwargs):
        self.commands.append(command)
        log.write_text('synthetic successful command\n')
        return 0

    def run_worker(self,side_effect=None):
        with patch.dict(os.environ,HALO2_TEST_SCRATCH=str(self.scratch)), \
             patch.object(runner,'run_step',side_effect=side_effect or self.step):
            return runner.build_and_test(self.args)

    def report(self):
        return json.loads((self.output/'report.json').read_text())

    def test_build_only_runs_configure_and_build_without_claiming_test_acceptance(self):
        self.args.build_only=True
        self.assertEqual(self.run_worker(),0)
        report=self.report()
        self.assertFalse(report['passed']);self.assertEqual(report['status'],'built')
        self.assertIs(report['validation_not_run'],True)
        self.assertEqual([step['name'] for step in report['steps']],['configure','build'])
        self.assertEqual([command[0] for command in self.commands],['cmake','cmake'])
        self.assertTrue(all('--tests-regex' not in command for command in self.commands))
        self.assertIn('example_target',self.commands[1])
        self.assertFalse((self.output/'tests.log').exists())

    def test_ordinary_worker_requires_ctest_before_marking_passed(self):
        self.assertEqual(self.run_worker(),0)
        report=self.report()
        self.assertTrue(report['passed']);self.assertEqual(report['status'],'passed')
        self.assertFalse(report.get('validation_not_run',False))
        self.assertEqual([step['name'] for step in report['steps']],['configure','build','tests'])
        self.assertEqual(self.commands[-1][0],'ctest')
        self.assertIn('--no-tests=error',self.commands[-1])
        self.assertEqual(self.commands[-1][-2:],['--tests-regex','^required_check$'])

    def test_build_only_retains_small_reports_for_the_later_owned_test_phase(self):
        self.args.build_only=True;self.args.retain_report=['engine-targets.txt']
        build=self.scratch/'build';build.mkdir()
        (build/'engine-targets.txt').write_text('native|renderer||source.cpp\n')
        (build/'private-build-owner.txt').write_text('reused by subsequent owned phase')
        self.assertEqual(self.run_worker(),0)
        self.assertEqual((self.output/'evidence/engine-targets.txt').read_text(),
            'native|renderer||source.cpp\n')
        self.assertTrue((build/'private-build-owner.txt').exists())
        self.assertEqual(self.report()['retained_small_reports'][0]['status'],'retained')
        self.assertFalse(self.report()['passed'])

    def test_configure_failure_stops_before_build_or_tests(self):
        self.args.build_only=True
        def fail(command,log,**kwargs):
            self.step(command,log);return 3
        self.assertEqual(self.run_worker(fail),3)
        report=self.report();self.assertEqual(report['status'],'failed');self.assertFalse(report['passed'])
        self.assertEqual([step['name'] for step in report['steps']],['configure'])
        self.assertEqual(report['steps'][0]['exit_code'],3)

    def test_build_failure_is_not_successful_build_only_or_test_acceptance(self):
        self.args.build_only=True
        def fail(command,log,**kwargs):
            self.step(command,log);return 7 if '--build' in command else 0
        self.assertEqual(self.run_worker(fail),7)
        report=self.report();self.assertEqual(report['status'],'failed');self.assertFalse(report['passed'])
        self.assertEqual([step['name'] for step in report['steps']],['configure','build'])
        self.assertEqual(report['steps'][-1]['exit_code'],7)

    def test_test_failure_never_becomes_acceptance_after_a_successful_build(self):
        def fail(command,log,**kwargs):
            self.step(command,log);return 8 if command[0]=='ctest' else 0
        self.assertEqual(self.run_worker(fail),8)
        report=self.report();self.assertEqual(report['status'],'failed');self.assertFalse(report['passed'])
        self.assertEqual(report['steps'][-1]['name'],'tests')
        self.assertEqual(report['steps'][-1]['exit_code'],8)

    def test_signal_exit_and_exception_remain_failures_with_diagnostics(self):
        self.args.build_only=True
        def killed(command,log,**kwargs):
            self.step(command,log);return -15 if '--build' in command else 0
        self.assertEqual(self.run_worker(killed),143)
        self.assertFalse(self.report()['passed']);self.assertEqual(self.report()['status'],'failed')
        with self.assertRaisesRegex(OSError,'compiler unavailable'):
            self.run_worker(lambda *_:(_ for _ in ()).throw(OSError('compiler unavailable')))
        report=self.report();self.assertFalse(report['passed']);self.assertEqual(report['status'],'failed')
        self.assertIn('compiler unavailable',report['error'])

    def test_outer_cli_forwards_build_only_through_the_cleanup_owner(self):
        command=['run_tests.py','--build-only','--source',str(self.source),
            '--output',str(self.output),'--scratch-root',str(self.args.scratch_root),
            '--target','example_target','--filter','^required_check$']
        with patch.object(sys,'argv',command), \
             patch.object(runner,'run_with_cleanup',return_value=17) as owned, \
             patch.object(runner,'build_and_test') as worker:
            self.assertEqual(runner.main(),17)
        worker.assert_not_called();owned.assert_called_once()
        forwarded=owned.call_args.args[0]
        self.assertIn('--build-worker',forwarded);self.assertIn('--build-only',forwarded)
        self.assertIn('--scratch-root',forwarded)
        self.assertIn(str(self.args.scratch_root.resolve()),forwarded)

    def test_worker_cli_exposes_build_only_to_the_owned_worker(self):
        command=['run_tests.py','--build-worker','--build-only','--source',str(self.source),
            '--output',str(self.output),'--target','example_target']
        with patch.object(sys,'argv',command),patch.object(runner.signal,'signal'), \
             patch.object(runner,'build_and_test',return_value=0) as worker, \
             patch.object(runner,'run_with_cleanup') as owned:
            self.assertEqual(runner.main(),0)
        owned.assert_not_called();worker.assert_called_once()
        self.assertIs(worker.call_args.args[0].build_only,True)

    def bundle(self,root):
        layers=root/'.tools/validation/usr/share/vulkan/explicit_layer.d'
        libraries=root/'.tools/validation/usr/lib'
        layers.mkdir(parents=True);libraries.mkdir(parents=True)
        return layers,libraries

    def test_validation_discovery_preserves_explicit_layer_and_library_overrides(self):
        self.bundle(self.source)
        for override in ['/explicit/layers','']:
            env={'VK_LAYER_PATH':override,'LD_LIBRARY_PATH':'/explicit/lib','OTHER':'preserve'}
            selected,metadata=runner.validation_environment(self.source,env)
            self.assertEqual(selected,env)
            self.assertEqual(metadata,{'source':'explicit environment','layer_path':override})
            self.assertEqual(env['LD_LIBRARY_PATH'],'/explicit/lib')

    def test_validation_discovery_prefers_the_source_bundle(self):
        layers,libraries=self.bundle(self.source)
        fallback=self.root/'fallback';self.bundle(fallback)
        original={'LD_LIBRARY_PATH':'/existing/lib','OTHER':'preserve'}
        with patch.object(runner,'ROOT',fallback):
            selected,metadata=runner.validation_environment(self.source,original)
        self.assertEqual(selected['VK_LAYER_PATH'],str(layers))
        self.assertEqual(selected['LD_LIBRARY_PATH'],str(libraries)+os.pathsep+'/existing/lib')
        self.assertEqual(selected['OTHER'],'preserve')
        self.assertNotIn('VK_LAYER_PATH',original,'Discovery must not mutate the supplied environment')
        self.assertEqual(metadata['source'],'repository bundle')

    def test_validation_discovery_uses_runner_bundle_for_a_frozen_source_and_deduplicates_libraries(self):
        fallback=self.root/'fallback';layers,libraries=self.bundle(fallback)
        with patch.object(runner,'ROOT',fallback):
            selected,metadata=runner.validation_environment(self.source,{'LD_LIBRARY_PATH':'/system/lib'})
            again,_=runner.validation_environment(self.source,
                {'LD_LIBRARY_PATH':str(libraries)+os.pathsep+'/system/lib'})
        self.assertEqual(selected['VK_LAYER_PATH'],str(layers))
        self.assertEqual(selected['LD_LIBRARY_PATH'],str(libraries)+os.pathsep+'/system/lib')
        self.assertEqual(again['LD_LIBRARY_PATH'],selected['LD_LIBRARY_PATH'])
        self.assertEqual(metadata['library_path'],str(libraries))

    def test_validation_discovery_without_a_bundle_uses_the_existing_loader_environment(self):
        fallback=self.root/'fallback';fallback.mkdir()
        env={'LD_LIBRARY_PATH':'/system/lib','OTHER':'preserve'}
        with patch.object(runner,'ROOT',fallback):
            selected,metadata=runner.validation_environment(self.source,env)
        self.assertEqual(selected,env)
        self.assertEqual(metadata,{'source':'system Vulkan loader discovery'})

    def test_only_ctest_receives_the_discovered_validation_environment(self):
        layers,libraries=self.bundle(self.source);environments=[]
        def record(command,log,**kwargs):
            environments.append(kwargs.get('env'));return self.step(command,log)
        with patch.dict(os.environ,{},clear=True):
            self.assertEqual(self.run_worker(record),0)
        self.assertEqual(environments[:2],[None,None])
        self.assertEqual(environments[2]['VK_LAYER_PATH'],str(layers))
        self.assertEqual(environments[2]['LD_LIBRARY_PATH'],str(libraries))
        self.assertEqual(self.report()['validation_environment']['source'],'repository bundle')


if __name__=='__main__':unittest.main()
