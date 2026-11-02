"""Acceptance controls for the installed-mod sweep, without launching a game."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from test_installed_workshop_mods import completed_simulation


class SimulationAcceptanceTests(unittest.TestCase):
    def result(self,stdout,stderr='',exit_code=0,passed=True):
        return dict(stdout=stdout,stderr=stderr,exit_code=exit_code,passed=passed)

    def test_early_clean_campaign_exit_does_not_complete_ninety_ticks(self):
        row=self.result('tick=12 bsp=0 script_faults=0\nShutdown handler: cleanup.\n')
        self.assertFalse(completed_simulation(row,90))
        self.assertEqual(row['completed_ticks'],12)
        self.assertEqual(row['requested_ticks'],90)

    def test_complete_campaign_and_multiplayer_progress(self):
        for text,ticks in [('tick=90 bsp=0\n',90),('Multiplayer ticks=91 players=1 finished=0\n',91)]:
            with self.subTest(text=text):
                row=self.result(text)
                self.assertTrue(completed_simulation(row,90))
                self.assertEqual(row['completed_ticks'],ticks)

    def test_only_final_progress_record_counts(self):
        self.assertFalse(completed_simulation(self.result('tick=90 bsp=0\ntick=12 bsp=1\n'),90))

    def test_other_diagnostics_do_not_supply_native_progress(self):
        for text in ['Script audio packet: tick=90\n','native_tick=90\n','tick=90broken\n','tick=0\n','']:
            with self.subTest(text=text):
                self.assertFalse(completed_simulation(self.result(text),90))

    def test_script_faults_in_either_stream_remain_failures(self):
        for stdout,stderr in [('tick=90\n','Script fault: test\n'),('tick=90\nScript fault: test\n','')]:
            with self.subTest(stdout=stdout,stderr=stderr):
                self.assertFalse(completed_simulation(self.result(stdout,stderr),90))

    def test_failed_exit_or_prior_check_cannot_be_overridden(self):
        for exit_code,passed in [(1,True),(None,False),(0,False)]:
            with self.subTest(exit_code=exit_code,passed=passed):
                self.assertFalse(completed_simulation(self.result('tick=90\n',exit_code=exit_code,passed=passed),90))

    def test_invalid_request_is_rejected(self):
        for ticks in [0,-1]:
            with self.assertRaises(ValueError):
                completed_simulation(self.result('tick=90\n'),ticks)


if __name__=='__main__':unittest.main()
