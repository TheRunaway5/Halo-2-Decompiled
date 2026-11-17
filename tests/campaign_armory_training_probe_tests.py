#!/usr/bin/env python3
"""Parser/acceptance controls only; these synthetic logs never enter an engine."""
import copy,json,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from test_campaign_armory_training import (ARMORY,CAIRO,parse_training_lines,
    recorded_input_sequence_matches,final_global_input_step_observed,
    training_progress_succeeded,armory_to_cairo_succeeded)

def initial(scenario):
    return f'Campaign scenario started: {scenario} difficulty=1 tick=0 bsp=0 actors=0 scripts=2 script_faults=0 level_finished=0'

class TrainingProbeTests(unittest.TestCase):
    def report(self,handoff=True):
        lines=[initial(ARMORY),'content_errors=0','Campaign input: tick=0 input_tick=0 yaw=0',
            'Campaign input diagnostic: '+json.dumps(dict(input_tick=0,tick=1)),
            'Script: expected authored milestone', 'Campaign input: tick=9000 input_tick=9000 yaw=0',
            'Campaign input diagnostic: '+json.dumps(dict(input_tick=9000,tick=9001))]
        if handoff:lines += ['tick=10000 bsp=0 actors=11 scripts=2 script_faults=0 level_finished=1',
            f'Campaign next scenario: {CAIRO} difficulty=1',initial(CAIRO),'content_errors=0']
        local_tick=1999 if handoff else 11999
        lines += [f'Campaign input: tick={local_tick} input_tick=11999 yaw=0',
            'Campaign input diagnostic: '+json.dumps(dict(input_tick=11999,tick=local_tick+1)),
            f'tick={local_tick+1} bsp=0 actors=0 scripts=9 script_faults=0 level_finished=0']
        report=parse_training_lines(lines)
        report.update(returncode=0,reference_inputs_opened=[],cache_write_attempts=[],runtime_unchanged=True,
            input_schedule=[dict(tick=t)for t in(0,9000,11999)],
            required_milestones=[dict(text='expected authored milestone',observed=True)])
        return report

    def test_natural_transition_accepts_global_budget_and_reset_map_clock(self):
        report=self.report()
        self.assertEqual([r['tick']for r in report['source_status']],[0,10000,0,2000])
        self.assertEqual([r['input_tick']for r in report['input_step_observations']],[0,9000,11999])
        self.assertTrue(final_global_input_step_observed(report,12000))
        self.assertTrue(armory_to_cairo_succeeded(report,12000))
        self.assertFalse(training_progress_succeeded(report,12000))
        self.assertFalse(armory_to_cairo_succeeded(report,12001))
        # Accepted Controls steps, not a sum of map clocks, own the budget.
        # An earlier ordinary checkpoint can reset the game clock.
        report['source_status'][1]['tick']=9900
        self.assertTrue(armory_to_cairo_succeeded(report,12000))

    def test_single_map_progress_cannot_pass_handoff_criterion(self):
        report=self.report(False)
        self.assertTrue(training_progress_succeeded(report,12000))
        self.assertFalse(armory_to_cairo_succeeded(report,12000))
        # Earlier final recording events remain valid for single-map progress,
        # as with the existing1810-ended look recording observed at2700.
        report['input_schedule'].pop();report['input_step_observations'].pop();report['input_delivery'].pop();report['input_diagnostics'].pop()
        self.assertTrue(training_progress_succeeded(report,12000))
        self.assertFalse(final_global_input_step_observed(report,12000))

    def test_unfinished_stale_or_wrong_handoff_rejected(self):
        controls=[('source_status',1,'level_finished',False),('source_status',1,'tick',0),
            ('source_status',1,'line',100),('natural_next_scenarios',0,'scenario',ARMORY),
            ('natural_next_scenarios',0,'difficulty',2),('natural_next_scenarios',0,'line',1),
            ('initial_scenarios',1,'scenario',ARMORY),('initial_scenarios',1,'tick',10000),
            ('initial_scenarios',1,'scripts',0),('initial_scenarios',1,'level_finished',True),
            ('initial_scenarios',1,'bsp',2),('initial_scenarios',1,'difficulty',3),
            ('source_status',3,'tick',0),('source_status',3,'level_finished',True),
            ('source_status',3,'scenario',ARMORY),('source_status',3,'phase','initial')]
        for field,index,key,value in controls:
            with self.subTest(control=(field,index,key,value)):
                report=self.report();report[field][index][key]=value
                self.assertFalse(armory_to_cairo_succeeded(report,12000))
        for field in ('initial_scenarios','natural_next_scenarios','source_status'):
            report=self.report();report[field]=[]
            self.assertFalse(armory_to_cairo_succeeded(report,12000))
        report=self.report();report['natural_next_scenarios'].append(copy.deepcopy(report['natural_next_scenarios'][0]))
        self.assertFalse(armory_to_cairo_succeeded(report,12000))

    def test_exact_delivered_global_steps_required(self):
        for mutation in ('drop','duplicate','wrong_tick','out_of_order','extra_raw','missing_diag','stale_diag','pre_cairo','missing_last_event'):
            with self.subTest(control=mutation):
                report=self.report()
                if mutation=='drop':report['input_step_observations'].pop()
                if mutation=='duplicate':report['input_step_observations'][1]['input_tick']=0
                if mutation=='wrong_tick':report['input_step_observations'][-1]['input_tick']=11998
                if mutation=='out_of_order':report['input_step_observations'].reverse()
                if mutation=='extra_raw':report['input_delivery'].append('Campaign input: unparsed')
                if mutation=='missing_diag':report['input_diagnostics']=[]
                if mutation=='stale_diag':report['input_diagnostics'][-1]['tick']=12000
                if mutation=='pre_cairo':report['input_step_observations'][-1]['line']=2
                if mutation=='missing_last_event':
                    for field in ('input_schedule','input_step_observations','input_delivery','input_diagnostics'):report[field].pop()
                self.assertFalse(armory_to_cairo_succeeded(report,12000))
        for value in (True,-1,12000,'11999'):
            report=self.report();report['input_schedule'][-1]['tick']=value
            self.assertFalse(recorded_input_sequence_matches(report,12000))

    def test_existing_failure_milestone_and_input_audit_checks_preserved(self):
        for key,value in [('returncode',1),('timeout',True),('script_faults',['actual native refusal']),
            ('content_error_counts',[0,1]),('reference_inputs_opened',['default.xbe']),
            ('cache_write_attempts',['O_RDWR']),('runtime_unchanged',False),
            ('required_milestones',[]),('required_milestones',[dict(observed=False)])]:
            with self.subTest(control=key):
                for handoff in(False,True):
                    report=self.report(handoff);report[key]=value
                    predicate=armory_to_cairo_succeeded if handoff else training_progress_succeeded
                    self.assertFalse(predicate(report,12000))
        for n in range(4):
            report=self.report();report['source_status'][n]['script_faults']=1
            self.assertFalse(armory_to_cairo_succeeded(report,12000))

if __name__=='__main__':unittest.main()
