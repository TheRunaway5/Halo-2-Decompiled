#!/usr/bin/env python3
"""Compare actual SDL hand publications with retained TU1.5 weapon banks.

Run through tools/run_test.py. This does not replace original world execution,
framebuffer comparisons, or the complete native sequence's failure verdict.
"""
import argparse
import json
import os
from pathlib import Path
from original_xbox_pose_case import sha
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def signed_short(value):
    value &= 65535
    return value - 65536 if value & 32768 else value


def analyze(path):
    trace = json.loads(path.read_text())
    failures = []
    owners = {}
    previous = {}
    isolated = [[], []]
    matched = 0
    dual_samples = 0
    for sample in trace['samples']:
        hands = {hand['hand']: hand for hand in sample['hands']}
        if len(hands) == 2:
            dual_samples += 1
            if hands[0]['datum'] == hands[1]['datum']:
                failures.append(dict(tick=sample['tick'], reason='Both hands share a full weapon datum'))
        for hand, row in hands.items():
            raw = row['source_words_16c']
            assert len(raw) == 57
            expected = dict(definition=row['source_definition'], shot_tick=raw[55],
                            loaded=signed_short(raw[48]), reserve=signed_short(raw[47] >> 16))
            actual = dict(definition=row['definition'], shot_tick=row['published_shot_tick'],
                          loaded=row['published_loaded'], reserve=row['published_reserve'])
            matched += 1
            if actual != expected:
                failures.append(dict(tick=sample['tick'], hand=hand, expected=expected, actual=actual))
            owner = owners.setdefault(row['datum'], dict(datum=row['datum'], definition=row['definition'],
                hands=set(), samples=0, preparation_ticks=set(), serials=set(),
                loaded_values=set(), magazine_modes=set(), published_reload_ticks=set()))
            if owner['definition'] != row['definition']:
                failures.append(dict(tick=sample['tick'], reason='A live datum changed definition'))
            owner['samples'] += 1
            owner['hands'].add(hand)
            owner['loaded_values'].add(expected['loaded'])
            owner['magazine_modes'].add(signed_short(raw[46]))
            owner['published_reload_ticks'].add(row['published_reload_tick'])
            owner['serials'].add((raw[25] >> 16) & 255)
            if raw[55]:
                owner['preparation_ticks'].add(raw[55])
        if len(hands) == len(previous) == 2 and all(hands[h]['datum'] == previous[h]['datum'] for h in (0, 1)):
            for hand in (0, 1):
                other = 1 - hand
                fired = hands[hand]['source_words_16c'][55] != previous[hand]['source_words_16c'][55]
                idle = hands[other]['source_words_16c'][55] == previous[other]['source_words_16c'][55]
                if fired and idle:
                    isolated[hand].append(dict(tick=sample['tick'],
                        source_preparation_tick=hands[hand]['source_words_16c'][55],
                        active_datum=hands[hand]['datum'], idle_datum=hands[other]['datum'],
                        idle_preparation_tick=hands[other]['source_words_16c'][55],
                        fire=sample['fire'], fire_secondary=sample['fire_secondary']))
        previous = hands
    observations = trace.get('observations', {})
    complete_motion = all(observations.get(key, False) for key in
        ('map_pickup', 'switched', 'dual', 'right_fired', 'left_fired', 'reloaded', 'thrown', 'dropped'))
    for owner in owners.values():
        for key, value in owner.items():
            if isinstance(value, set):
                owner[key] = sorted(value)
    return dict(path=str(path), sha256=sha(path), representation=trace['representation'],
        render_loop_completed=trace['render_loop_completed'], matched_hand_samples=matched,
        dual_samples=dual_samples, publication_mismatches=failures,
        owner_histories=list(owners.values()), isolated_preparations=isolated,
        independent_hand_preparations_observed=all(isolated),
        complete_pickup_fire_reload_drop_observed=complete_motion,
        observations=observations,
        bounded_publication_passed=matched > 0 and dual_samples > 0 and not failures and all(isolated))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, action='append', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    original = Xbe(NEW_DEFAULT).read(0x1085f9, 20)
    assert original.hex() == 'a154df51008b48088a44241284c0898d48020000'
    cases = [analyze(path) for path in args.input]
    report = dict(passed=all(case['bounded_publication_passed'] for case in cases),
        component='Independent hand identity, ammunition and preparation timestamp publication',
        original_xbe_sha256=NEW_PIN, original_clock_and_store_bytes=original.hex(),
        original_store='108607', weapon_tick_offset='248', native_tick_adjustment=0,
        cases=cases, tool_sha256=sha(Path(__file__)), runner_scratch=os.environ['HALO2_TEST_SCRATCH'],
        original_instruction_patches=[], supplied_callee_results=[],
        full_weapon_parity=False, framebuffer_parity_verified=False, fully_certified_weapons=0,
        complete_native_failure_verdict_unchanged=True, large_artifacts_retained=[])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], cases=len(cases),
        matched_hand_samples=sum(case['matched_hand_samples'] for case in cases))), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
