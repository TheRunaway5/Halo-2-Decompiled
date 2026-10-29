#!/usr/bin/env python3
"""Log-parser/predicate tests only; no engine or gameplay outcome is supplied."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from test_campaign_new_game_runtime import (INTRODUCTION,ARMORY,parse_campaign_lines,
                                           startup_succeeded,opening_handoff_succeeded)


class ProbeTests(unittest.TestCase):
    def observed(self):
        lines=[f'Campaign scenario started: {INTRODUCTION} difficulty=1 tick=0 bsp=0 actors=0 scripts=4 script_faults=0 level_finished=0\n',
               'weapons=4 content_errors=0\n',
               'tick=6432 bsp=0 actors=0 scripts=3 script_faults=0 level_finished=1\n',
               f'Campaign next scenario: {ARMORY} difficulty=1\n',
               f'Campaign scenario started: {ARMORY} difficulty=1 tick=0 bsp=0 actors=0 scripts=9 script_faults=0 level_finished=0\n',
               'weapons=8 content_errors=0\n',
               'tick=5568 bsp=0 actors=7 scripts=5 script_faults=0 level_finished=0\n']
        report=parse_campaign_lines(lines)
        report.update(returncode=0,reference_inputs_opened=[],cache_write_attempts=[],trace_unavailable=False)
        return report

    def test_actual_ordered_map_clock_observations(self):
        report=self.observed()
        self.assertEqual([r['scenario'] for r in report['initial_scenarios']],[INTRODUCTION,ARMORY])
        self.assertEqual([r['tick'] for r in report['source_status']],[0,6432,0,5568])
        self.assertTrue(opening_handoff_succeeded(report))
        self.assertFalse(startup_succeeded(report,12000))
        self.assertEqual(report['natural_next_scenarios'],[dict(scenario=ARMORY,difficulty=1,line=4)])

    def test_missing_completion_wrong_catalog_difficulty_and_fresh_state_rejected(self):
        report=self.observed()
        controls=[('source_status',1,'level_finished',False),
                  ('natural_next_scenarios',0,'scenario',INTRODUCTION),
                  ('natural_next_scenarios',0,'difficulty',3),
                  ('initial_scenarios',1,'difficulty',2),
                  ('initial_scenarios',1,'tick',6432),
                  ('initial_scenarios',1,'scripts',0),
                  ('initial_scenarios',1,'line',2)]
        for field,index,key,value in controls:
            with self.subTest(control=(field,key)):
                changed=copy.deepcopy(report);changed[field][index][key]=value
                self.assertFalse(opening_handoff_succeeded(changed))
        for field in ('initial_scenarios','natural_next_scenarios','source_status'):
            changed=copy.deepcopy(report);changed[field]=[]
            self.assertFalse(opening_handoff_succeeded(changed))

    def test_real_failure_and_reference_contamination_rejected(self):
        for key,value in [('returncode',1),('timeout',True),('script_faults',['unsupported owner']),
                          ('content_error_counts',[0,1]),('trace_unavailable',True),
                          ('reference_inputs_opened',['default.xbe']),('cache_write_attempts',['O_RDWR'])]:
            with self.subTest(control=key):
                changed=self.observed();changed[key]=value
                self.assertFalse(opening_handoff_succeeded(changed))

    def test_single_map_startup_is_a_separate_budget_predicate(self):
        report=parse_campaign_lines(['tick=120 bsp=0 actors=0 scripts=4 script_faults=0 level_finished=0\n'])
        report.update(returncode=0,reference_inputs_opened=[],cache_write_attempts=[],trace_unavailable=False)
        self.assertTrue(startup_succeeded(report,120))
        self.assertFalse(opening_handoff_succeeded(report))
        report['source_status']=[]
        self.assertFalse(startup_succeeded(report,120))


if __name__=='__main__':
    unittest.main()
