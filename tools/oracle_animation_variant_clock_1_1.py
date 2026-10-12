#!/usr/bin/env python3
"""Variant observation on unchanged frozen ordinary animation clock schedules."""
import argparse
import json
from pathlib import Path
import oracle_animation_variant_1_1 as variant
import oracle_unit_animation_manager_1_1 as clock
from inspect_multiplayer import Cache


class ClockVariantTrace(variant.VariantObserver, clock.ClockTrace):
    def __init__(self):
        super().__init__()
        # Observer metadata only. The old fixture deliberately closes its own
        # parser; this reader verifies guest graph bytes against the same file.
        self.cache = Cache(clock.ROOT/'assets/disc/maps/lockout.map')
        self.variant_kind = 'unchanged_owning_clock_schedule'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();assert not (args.output/'report.json').exists()
    for path,pin in clock.PINS.items():assert clock.sha(clock.ROOT/path)==pin,path
    cases=[]
    for scenario,ticks in (('idle',75),('aim',12),('move_input',12),('fire',60),('reload',135),('empty_reload',225)):
        trace=ClockVariantTrace();result=trace.execute_unit_phase(ticks,scenario)
        trace.variant_finish(clock.STOP);assert not trace.variant_pending
        oldpath=clock.ROOT/'analysis/multiplayer/map-pack/spartan-equipped-unit-phase-suite'/f'{scenario}.json'
        old=json.loads(oldpath.read_text());result.pop('oracle_tool_sha256');old.pop('oracle_tool_sha256')
        assert result==old,('Unchanged clock schedule differs',scenario)
        report=dict(schema='original-animation-variant-clock-v1',tool_sha256=clock.sha(__file__),
            observer_tool_sha256=clock.sha(variant.__file__),source_pins=clock.PINS,
            original_runtime_correlation=dict(path=str(oldpath.relative_to(clock.ROOT)),sha256=clock.sha(oldpath),exact_fields=True),
            scenario=scenario,ticks=ticks,xbe_sha256=clock.PIN,
            calls=trace.variant_calls,stores=trace.variant_stores,reads=trace.variant_reads,
            graphs=list(trace.variant_graphs.values()),
            constants={f'{address:08x}':f'{trace.u(address):08x}'for address in (0x465030,0x4650c0,0x465028)},
            inherited_boundaries='Frozen preallocated unit/inventory, resource delivery, processed command,30Hz clock and RNG input. No new seed, chosen variant, readiness, pose or successful resource return.',
            new_guest_inputs=[],new_supplied_results=[],instruction_patches=[])
        path=args.output/f'{scenario}.json';clock.save(path,clock.encoded(report))
        cases.append(dict(scenario=scenario,ticks=ticks,file=path.name,sha256=clock.sha(path),calls=len(trace.variant_calls),
                          rng_stores=sum(row['instruction']in ('001e10d1','001e10ef')for row in trace.variant_stores)))
        print(json.dumps(cases[-1]),flush=True)
    clock.save(args.output/'report.json',clock.encoded(dict(schema='original-animation-variant-clock-index-v1',
        tool_sha256=clock.sha(__file__),observer_tool_sha256=clock.sha(variant.__file__),cases=cases,
        total_ticks=sum(case['ticks']for case in cases))))


if __name__=='__main__':main()
