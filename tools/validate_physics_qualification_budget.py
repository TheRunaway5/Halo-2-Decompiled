#!/usr/bin/env python3
"""Regression proof for owned-session cleanup reserves and absolute expiry."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
from physics_qualification_budget import QualificationBudget, QualificationBudgetExhausted, QualificationWatchdog

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True, exist_ok=False)
    (args.output / 'progress.json').write_text(json.dumps({'owned_scratch':str(scratch),'validation_started':True},indent=2)+'\n')
    class Clock:
        value = 0.
        def __call__(self):
            return self.value
    clock = Clock()
    budget = QualificationBudget(1000, 60, 2000, clock=clock, wall_clock=lambda: 10000.)
    plan = budget.plan_case()
    assert plan.native_seconds == 180 and plan.wrapper_seconds == 270
    assert sum([plan.native_seconds, plan.replay_seconds, plan.transport_seconds,
                plan.retention_seconds, budget.cleanup_seconds]) <= 1000
    clock.value = 450
    try:
        budget.plan_case()
        raise AssertionError('Insufficient total budget admitted a full case')
    except QualificationBudgetExhausted:
        pass
    clock.value = 800
    assert budget.timeout(180, reserve_after=30) == 110
    assert budget.extend(revision=1, total_seconds=1500, reason='Explicit test extension', authorization='Budget regression fixture')
    assert not budget.extend(revision=1, total_seconds=1600, reason='Repeated revision', authorization='Fixture')
    try:
        budget.extend(revision=2, total_seconds=2001, reason='Too long', authorization='Fixture')
        raise AssertionError('Maximum finite owner limit was bypassed')
    except ValueError:
        pass
    clock.value = 1440
    try:
        budget.extend(revision=2, total_seconds=1800, reason='Revive expired owner', authorization='Fixture')
        raise AssertionError('Expired owner was revived')
    except QualificationBudgetExhausted:
        pass
    control_dir = scratch / 'control'
    control_dir.mkdir()
    other_clock = Clock()
    other = QualificationBudget(1000, 60, 2000, clock=other_clock)
    observed = []
    guard = QualificationWatchdog(other, control_dir, interval=.01, expire=lambda: observed.append(True)).start()
    try:
        (control_dir / 'budget-control.json').write_text(json.dumps(dict(revision=1, total_limit_seconds=1200,
            reason='Explicit authorized change', authorization='Regression fixture')))
        end = time.monotonic() + 3
        while other.revision != 1 and time.monotonic() < end:
            time.sleep(.01)
        assert other.total_seconds == 1200
        other_clock.value = 1140
        assert guard.expired.wait(3)
        guard.thread.join(timeout=3)
        assert not guard.thread.is_alive() and observed == [True]
    finally:
        guard.close()
    probe = scratch / 'absolute-expiry.py'
    probe.write_text('''from pathlib import Path
import json,os,subprocess,sys,time
sys.path.insert(0,sys.argv[1])
from physics_qualification_budget import QualificationBudget,QualificationWatchdog,QualificationBudgetExhausted
report=Path(sys.argv[2]);began=time.monotonic();budget=QualificationBudget(3,1,6);guard=QualificationWatchdog(budget,report.parent,interval=.02).start()
child=subprocess.Popen([sys.executable,'-c','import time;time.sleep(30)'])
result={'scratch':os.environ['HALO2_TEST_SCRATCH'],'pid':os.getpid(),'child_pid':child.pid,'expired':False}
try:
 while True:time.sleep(.05)
except QualificationBudgetExhausted:
 result['expired']=True
finally:
 guard.close()
 if child.poll()is None:child.terminate()
 child.wait(timeout=3)
 result['elapsed_seconds']=time.monotonic()-began
 report.write_text(json.dumps(result))
''')
    probe_result = scratch / 'absolute-result.json'
    command = [sys.executable, str(ROOT / 'tools/run_test.py'), '--scratch-root', str(scratch / 'nested'),
               '--', sys.executable, '-B', str(probe), str(ROOT / 'tools'), str(probe_result)]
    executed = subprocess.run(command, check=False, capture_output=True, text=True, timeout=15)
    assert executed.returncode == 0, executed.stdout + executed.stderr
    actual = json.loads(probe_result.read_text())
    assert actual['expired'] and actual['elapsed_seconds'] < 3
    assert not Path(actual['scratch']).exists()
    assert not Path(f"/proc/{actual['child_pid']}").exists()
    report = dict(passed=True, whole_case_budget_admission=True, replay_remaining_time_clamp=True,
                  finite_authorized_extension=True, expired_owner_not_revived=True,
                  absolute_real_process_expiry=actual, nested_owner_removed=True,
                  child_process_removed=True, owned_scratch=str(scratch),
                  source_hashes={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                    for p in [ROOT / 'tools/physics_qualification_budget.py', Path(__file__)]},
                  retained_large_artifacts=[])
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
