#!/usr/bin/env python3
"""Controller-only tests: failed six-frame gate must never invoke the long case."""
import json
import os
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from test_campaign_new_game_graphics_batch import run_cases
from install_campaign_new_game_graphics_hook import hook_text


class BatchTests(unittest.TestCase):
    def cases(self):
        root=Path(os.environ['HALO2_TEST_SCRATCH'])/self._testMethodName
        root.mkdir()
        return [dict(acceptance=mode,command=[mode],report=str(root/(mode+'.json')),passed=False)
                for mode in ('graphics_startup','opening_handoff')]

    def test_cold_failure_stops_before_long_case(self):
        cases=self.cases();report=dict(passed=False,cases=[]);seen=[]
        def failed(command):seen.append(command);return 1
        self.assertEqual(run_cases(cases,report,lambda:None,failed),1)
        self.assertEqual(seen,[['graphics_startup']]);self.assertFalse(report['passed'])

    def test_wrong_mode_report_cannot_supply_a_success(self):
        cases=self.cases();report=dict(passed=False,cases=[]);seen=[]
        def wrong(command):
            seen.append(command)
            Path(cases[0]['report']).write_text('{"passed":true,"acceptance_criterion":"opening_handoff"}')
            return 0
        self.assertEqual(run_cases(cases,report,lambda:None,wrong),1)
        self.assertEqual(len(seen),1)

    def test_two_explicit_phases_and_nonzero_process_rejection(self):
        cases=self.cases();report=dict(passed=False,cases=[])
        def good(command):
            case=next(c for c in cases if c['command']==command)
            Path(case['report']).write_text(json.dumps(dict(passed=True,acceptance_criterion=case['acceptance'])))
            return 0
        self.assertEqual(run_cases(cases,report,lambda:None,good),0)
        self.assertTrue(report['passed']);self.assertEqual(len(report['cases']),2)
        report=dict(passed=False,cases=[])
        self.assertEqual(run_cases(cases,report,lambda:None,lambda command:1),1)
        self.assertFalse(report['passed'])

    def test_hook_quotes_paths_and_rejects_cmake_list_injection(self):
        text=hook_text(['/path with spaces/python','--acceptance','opening_handoff'])
        self.assertIn('"/path with spaces/python"',text)
        self.assertIn('list(APPEND CTEST_CUSTOM_PRE_TEST [[',text)
        for invalid in ('a;b','a"b','a\nb','a]]b'):
            with self.assertRaises(ValueError):hook_text([invalid])


if __name__=='__main__':
    unittest.main()
