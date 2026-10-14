#!/usr/bin/env python3
"""Live Linux/systemd containment checks; no game assets or GPU are needed."""
import fcntl
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from tag_match_memory_budget import LIMIT_BYTES, UNIT, run_in_memory_budget
import tag_match_memory_budget as guard


class MatchMemoryBudgetTests(unittest.TestCase):
    def setUp(self):
        retained = os.environ.get('HALO2_MEMORY_GUARD_TEST_OUTPUT')
        if retained:
            self.output = Path(retained).resolve() / self._testMethodName
        else:
            self.temp = tempfile.TemporaryDirectory(prefix='halo2-match-budget-')
            self.output = Path(self.temp.name)
            self.addCleanup(self.temp.cleanup)

    def test_released_cgroup_accepts_verified_active_exited_unit(self):
        # --remain-after-exit retains ActiveState=active after releasing the
        # cgroup. Reproduce the exact successful Zanzibar shutdown sequence.
        running = {'LoadState': 'loaded', 'ActiveState': 'active',
                   'SubState': 'running', 'ControlGroup': '/released',
                   'Result': 'success', 'ExecMainStatus': '0', 'MemoryPeak': '67108864'}
        exited = dict(running, SubState='exited', ControlGroup='')
        limits = {'memory.max': str(LIMIT_BYTES), 'memory.high': 'max',
                  'memory.swap.max': '0', 'memory.oom.group': '1'}
        with mock.patch.object(guard, '_properties', side_effect=[
                {'LoadState': 'not-found'}, running, running, exited]), \
             mock.patch.object(guard, '_limits', side_effect=[limits, FileNotFoundError('released')]), \
             mock.patch.object(guard, '_systemctl', return_value=subprocess.CompletedProcess([], 0, '', '')), \
             mock.patch.object(guard.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0, '', '')), \
             mock.patch.object(guard.time, 'sleep'):
            self.assertEqual(run_in_memory_budget(['/usr/bin/true'], self.output, 8), 0)
        report = json.loads((self.output / 'memory-budget.json').read_text())
        self.assertTrue(report['passed'])
        self.assertEqual(report['verified_limits'], limits)
        self.assertEqual(report['kernel']['SubState'], 'exited')

    def test_released_cgroup_without_verified_limits_fails_closed(self):
        running = {'LoadState': 'loaded', 'ActiveState': 'active',
                   'SubState': 'running', 'ControlGroup': '/released',
                   'Result': 'success', 'ExecMainStatus': '0'}
        exited = dict(running, SubState='exited', ControlGroup='')
        with mock.patch.object(guard, '_properties', side_effect=[
                {'LoadState': 'not-found'}, running, exited]), \
             mock.patch.object(guard, '_limits', side_effect=FileNotFoundError('released')), \
             mock.patch.object(guard, '_systemctl', return_value=subprocess.CompletedProcess([], 0, '', '')), \
             mock.patch.object(guard.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0, '', '')):
            self.assertNotEqual(run_in_memory_budget(['/usr/bin/true'], self.output, 8), 0)
        report = json.loads((self.output / 'memory-budget.json').read_text())
        self.assertFalse(report['passed'])
        self.assertNotIn('verified_limits', report)

    def test_host_reserve_rejects_before_starting_a_job(self):
        original_read = Path.read_text
        def read(path, *args, **kwargs):
            if path == Path('/proc/meminfo'):
                return 'MemAvailable: 1048576 kB\n'
            return original_read(path, *args, **kwargs)
        with mock.patch.object(guard, '_properties', return_value={'LoadState': 'not-found'}), \
             mock.patch.object(Path, 'read_text', read), \
             mock.patch.object(guard.subprocess, 'run', side_effect=AssertionError('reserve rejection launched a job')):
            self.assertNotEqual(run_in_memory_budget(['/usr/bin/true'], self.output, 8), 0)
        report = json.loads((self.output / 'memory-budget.json').read_text())
        self.assertIn('Insufficient spare host RAM', report['errors'][0])
        self.assertNotIn('kernel', report)

    def test_real_kernel_controls_and_peak_are_recorded(self):
        code = '''from pathlib import Path
import json
p=Path('/sys/fs/cgroup')/Path('/proc/self/cgroup').read_text().strip().split('0::')[1].lstrip('/')
Path(__import__('sys').argv[1]).write_text(json.dumps({k:(p/k).read_text().strip() for k in ('memory.max','memory.swap.max','memory.oom.group')}))
'''
        self.assertEqual(run_in_memory_budget(
            [sys.executable, '-c', code, str(self.output / 'controls.json')], self.output, 8), 0)
        controls = json.loads((self.output / 'controls.json').read_text())
        self.assertEqual(controls, {'memory.max': str(LIMIT_BYTES),
                                   'memory.swap.max': '0', 'memory.oom.group': '1'})
        report = json.loads((self.output / 'memory-budget.json').read_text())
        self.assertTrue(report['passed'])
        self.assertGreater(int(report['kernel']['MemoryPeak']), 0)

    def test_children_share_one_budget_and_group_is_killed(self):
        # Each child fits individually; together they exceed64MiB. Allocations
        # occur only inside the already limited cgroup, never on the host runner.
        code = '''import subprocess,sys
children=[subprocess.Popen([sys.executable,'-c','import time; b=bytearray(48*1024*1024); b[::4096]=b"x"*(len(b)//4096); time.sleep(30)']) for _ in range(2)]
for child in children: child.wait()
'''
        self.assertNotEqual(run_in_memory_budget([sys.executable, '-c', code], self.output, 8), 0)
        report = json.loads((self.output / 'memory-budget.json').read_text())
        self.assertFalse(report['passed'])
        self.assertEqual(report['kernel']['Result'], 'oom-kill')
        self.assertEqual(report['limit_bytes'], LIMIT_BYTES)
        self.assertEqual(report['swap_limit_bytes'], 0)
        # The failed cgroup and every descendant must be gone after cleanup.
        state = subprocess.run(['systemctl', '--user', 'show', UNIT, '-p', 'LoadState'],
                               text=True, capture_output=True)
        self.assertIn('LoadState=not-found', state.stdout)

    def test_second_job_cannot_start(self):
        lock_path = Path('/run/user') / str(os.getuid()) / 'halo2-tag-match-validation.lock'
        with lock_path.open('a') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            self.assertNotEqual(run_in_memory_budget(['/usr/bin/true'], self.output, 8), 0)
        report = json.loads((self.output / 'memory-budget.json').read_text())
        self.assertIn('already running', report['errors'][0])
        self.assertNotIn('kernel', report)

    def test_deadline_records_timeout_and_cleans_group(self):
        self.assertNotEqual(run_in_memory_budget(
            [sys.executable, '-c', 'import time; time.sleep(30)'], self.output, .25), 0)
        report = json.loads((self.output / 'memory-budget.json').read_text())
        self.assertEqual(report['kernel']['Result'], 'timeout')
        state = subprocess.run(['systemctl', '--user', 'show', UNIT, '-p', 'LoadState'],
                               text=True, capture_output=True)
        self.assertIn('LoadState=not-found', state.stdout)

    def test_environment_marker_cannot_bypass_kernel_verification(self):
        code = "from tag_match_memory_budget import enter_memory_budget; enter_memory_budget('.',8)"
        environment = dict(os.environ, PYTHONPATH=str(ROOT / 'tools'),
                           HALO2_TAG_MATCH_MEMORY_UNIT=UNIT)
        result = subprocess.run([sys.executable, '-c', code], env=environment,
                                text=True, capture_output=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('outside its required memory cgroup', result.stderr)


if __name__ == '__main__':
    unittest.main()
