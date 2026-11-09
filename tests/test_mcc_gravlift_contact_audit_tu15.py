"""Controls for the live audit's two-entrant acceptance, without a game fixture."""
import importlib.util
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import unittest


TOOL = Path(__file__).resolve().parents[1] / 'tools/test_mcc_gravlift_contact_audit_tu15.py'
SPEC = importlib.util.spec_from_file_location('lift_audit', TOOL)
audit = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(audit)


def completed_cycle(two=True):
    row = {'id':'fixture', 'status':'completed', 'unit':0xE1000001,
           'case':{'entrance':[0,0,0], 'second_entrance':[.4,0,0],
                   'leave_position':[5,0,0], 'classification':'actual_force_lift'},
           'phases':{}, 'samples':[]}
    if two:row['second_unit'] = 0xE2000002
    for name, count in [('entry',90), ('leave',30), ('reentry',90)]:
        for suffix, calls in [('before',0), ('after',count)]:
            row['phases'][name+'-'+suffix] = {
                'native_lift_scalar_calls':[calls,0,0,0],
                'native_lift_velocity_completed':[calls,calls],
                'native_lift_observation_refusals':0, 'native_lift_velocity_refusals':0}
        for tick in range(count):
            sample = {'phase':name, 'tick':tick, 'slot':0, 'unit':row['unit'],
                      'position':[0,0,1], 'velocity':[0,0,1],
                      'selected_field_member':name!='leave'}
            if two:
                sample['second_entrant'] = {'slot':1, 'unit':row['second_unit'],
                    'position':[.4,0,1], 'velocity':[0,0,1],
                    'selected_field_member':name!='leave'}
            row['samples'].append(sample)
    return row


class TwoEntrantAcceptanceTests(unittest.TestCase):
    @unittest.skipUnless(Path('/proc').is_dir(), 'Linux owned-session lifecycle control')
    def test_cleanup_retires_inferior_in_a_distinct_process_group(self):
        with tempfile.TemporaryDirectory(prefix='lift-audit-cleanup-',dir=os.environ.get('HALO2_TEST_SCRATCH'))as directory:
            pidfile=Path(directory)/'inferior.pid'
            code=('import subprocess,sys,time;from pathlib import Path;'
                  'child=subprocess.Popen([sys.executable,"-c","import os,time;os.setpgrp();time.sleep(60)"]);'
                  'Path(sys.argv[1]).write_text(str(child.pid));time.sleep(60)')
            process=subprocess.Popen([sys.executable,'-c',code,str(pidfile)],start_new_session=True)
            inferior=None
            try:
                deadline=time.monotonic()+5
                while not pidfile.is_file()and time.monotonic()<deadline:time.sleep(.01)
                self.assertTrue(pidfile.is_file(),'Owned inferior did not start')
                inferior=int(pidfile.read_text())
                self.assertEqual(os.getsid(inferior),process.pid)
                while os.getpgid(inferior)!=inferior and time.monotonic()<deadline:time.sleep(.01)
                self.assertEqual(os.getpgid(inferior),inferior)
                audit.stop_owned_session(process)
                self.assertIsNotNone(process.poll())
                self.assertFalse(audit.owned_session_members(process.pid))
            finally:
                # Preserve cleanup even if the control itself rejects a bug.
                for pid in (inferior,process.pid):
                    if pid is not None:
                        try:os.kill(pid,signal.SIGKILL)
                        except ProcessLookupError:pass
                process.wait()

    def test_requires_simultaneous_membership_not_disjoint_visits(self):
        row = completed_cycle()
        for sample in row['samples']:
            if sample['phase']!='leave':
                sample['selected_field_member'] = sample['tick']<45
                sample['second_entrant']['selected_field_member'] = sample['tick']>=45
        result = audit.evaluate(row)
        self.assertFalse(result['passed'])
        self.assertTrue(any('never shared exact selected field membership' in e for e in result['errors']))

    def test_requires_both_bodies_and_distinct_generations(self):
        for mutation in ('missing_body','same_generation','no_second_motion','no_second_leave'):
            with self.subTest(mutation=mutation):
                row = completed_cycle()
                if mutation=='same_generation':row['second_unit']=row['unit']
                for sample in row['samples']:
                    if mutation=='missing_body':sample.pop('second_entrant')
                    elif mutation=='no_second_motion':sample['second_entrant']['velocity']=[0,0,0]
                    elif mutation=='no_second_leave':sample['second_entrant']['selected_field_member']=True
                self.assertFalse(audit.evaluate(row)['passed'])

    def test_completed_shared_cycle_passes_with_honest_force_scope(self):
        result = audit.evaluate(completed_cycle())
        self.assertTrue(result['passed'],result['errors'])
        self.assertEqual(result['entered_slots'],[0,1])
        self.assertEqual(result['phases']['entry']['simultaneous_selected_member_samples'],90)
        self.assertFalse(result['force_attributed_to_selected_rule'])

    def test_single_entrant_reports_remain_compatible(self):
        result = audit.evaluate(completed_cycle(two=False))
        self.assertTrue(result['passed'],result['errors'])
        self.assertEqual(result['entered_slots'],[0])
        self.assertNotIn('simultaneous_selected_member_samples',result['phases']['entry'])

    def test_native_lifecycle_is_separate_from_force_cycle(self):
        row=completed_cycle(two=False)
        row['samples']=row['samples'][:20]
        row['status']='native-lifecycle-completed'
        row['lifecycle']={'continued_world_entries':90,
            'entrants':[{'unit':row['unit'],'current_player_unit':0xffffffff,
                         'same_generation_allocated':False}]}
        row['phases']['lifecycle-after']=dict(row['phases']['entry-before'])
        result=audit.evaluate(row)
        self.assertFalse(result['passed'])
        self.assertFalse(result['force_cycle_qualified'])
        self.assertTrue(result['native_contact_lifecycle_qualified'])
        self.assertTrue(result['crash_safety_passed'])
        self.assertEqual(result['disposition'],'safe_native_contact_lifecycle')

    def test_native_lifecycle_requires_contact_retirement_continuation_and_no_refusal(self):
        for mutation in('no_contact','alive_release','too_few_phases','missing_activity','refusal'):
            with self.subTest(mutation=mutation):
                row=completed_cycle(two=False);row['samples']=row['samples'][:20]
                row['status']='native-lifecycle-completed'
                row['lifecycle']={'continued_world_entries':90,
                    'entrants':[{'unit':row['unit'],'current_player_unit':0xffffffff,
                                 'same_generation_allocated':False}]}
                row['phases']['lifecycle-after']=dict(row['phases']['entry-before'])
                if mutation=='no_contact':
                    for sample in row['samples']:sample['selected_field_member']=False
                elif mutation=='alive_release':row['lifecycle']['entrants'][0]['same_generation_allocated']=True
                elif mutation=='too_few_phases':row['lifecycle']['continued_world_entries']=89
                elif mutation=='missing_activity':row['phases'].pop('lifecycle-after')
                elif mutation=='refusal':row['phases']['lifecycle-after']['native_lift_velocity_refusals']=1
                result=audit.evaluate(row)
                self.assertFalse(result['crash_safety_passed'])
                self.assertTrue(result['lifecycle_crash_safety_errors'])


if __name__ == '__main__':
    unittest.main()
