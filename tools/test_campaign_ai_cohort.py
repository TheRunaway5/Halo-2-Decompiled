#!/usr/bin/env python3
"""Qualify the existing original CRT route in an immutable native client cohort.

Reuse the cohort's exact private World headers and unchanged linked providers.
Compose the existing shared-CRT route and, when requested, complete original
placement/condition owners and audited immutable engine data. No supplied data/results, layout, tag
conversion, shader or installed-runtime changes are made.
The new object/client and all copied source stay under HALO2_TEST_SCRATCH.
"""
from source_layout import source_file
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
OLD = 'if(pstencil_native_light_portal_tu15_owns_entry(c.target)) {'
NEW = 'if(c.target!=0x32a990u && pstencil_native_light_portal_tu15_owns_entry(c.target)) {'
HEADER = '#include "halo2/source_campaign_ai_placement_settings_tu15.hpp"'
DISPATCH = '''        if(source_campaign_ai_placement_settings_tu15_owns_entry(c.target)) {
            source_run_campaign_ai_placement_settings_tu15(m,c.target,a,fp,services(),o);return;
        }
'''
CONDITION_HEADER = '#include "halo2/source_campaign_ai_conditions_tu15.hpp"'
CONDITION_DISPATCH = '''        if(source_campaign_ai_conditions_tu15_owns_entry(c.target)) {
            source_run_campaign_ai_conditions_tu15(m,c.target,a,fp,services(),o);return;
        }
'''
SCALAR_HEADER = '#include "halo2/source_campaign_ai_scalar_defaults_tu15.hpp"'
SCALAR_DISPATCH = '''        if(source_campaign_ai_scalar_defaults_tu15_owns_entry(c.target)) {
            source_run_campaign_ai_scalar_defaults_tu15(m,c.target,a,fp,services(),o);return;
        }
'''
SCALAR_STARTUP = '''        // Original _cinit scalar callbacks precede29BE40's AI geometry tables.
        for(std::uint32_t slot=0x46dd00u;slot<0x46dd0cu;slot+=4u)
            call_actual(word(bank,slot));
'''
DECISION_HEADER = '#include "halo2/source_campaign_ai_object_decisions_tu15.hpp"'
DECISION_DISPATCH = '''        if(source_campaign_ai_object_decisions_tu15_owns_entry(c.target)) {
            source_run_campaign_ai_object_decisions_tu15(m,c.target,a,fp,services(),o);return;
        }
'''
CALLBACK_HEADER = '#include "halo2/source_campaign_ai_object_callbacks_tu15.hpp"'
CALLBACK_DISPATCH = '''        if(source_campaign_ai_object_callbacks_tu15_owns_entry(c.target)) {
            source_run_campaign_ai_object_callbacks_tu15(m,c.target,a,fp,services(),o);return;
        }
'''
BIRTH_QUERY_HEADER = '#include "halo2/source_campaign_birth_query_variants_tu15.hpp"'
BIRTH_QUERY_DISPATCH = '''        if(source_campaign_birth_query_variants_tu15_owns_entry(c.target)) {
            source_run_campaign_birth_query_variants_tu15(m,c.target,a,fp,services(),o);return;
        }
'''
ACTOR_BEHAVIOR_HEADER = '#include "halo2/source_campaign_ai_actor_behavior_tu15.hpp"'
ACTOR_BEHAVIOR_DISPATCH = '''        if(source_campaign_ai_actor_behavior_tu15_owns_entry(c.target)) {
            source_run_campaign_ai_actor_behavior_tu15(m,c.target,a,fp,services(),o);return;
        }
'''


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cohort', type=Path, required=True)
    ap.add_argument('--world-receipt', type=Path, required=True)
    ap.add_argument('--mcc-root', type=Path, required=True)
    ap.add_argument('--squad', default='1st_hum_johnson')
    ap.add_argument('--additional-squad',action='append',default=[])
    ap.add_argument('--map',default='01b_spacestation.map')
    ap.add_argument('--ticks', type=int, default=4)
    ap.add_argument('--placement-settings', action='store_true')
    ap.add_argument('--ai-conditions', action='store_true')
    ap.add_argument('--physics-branch-data', action='store_true')
    ap.add_argument('--branch-receipt', type=Path)
    ap.add_argument('--ai-working-mask', action='store_true')
    ap.add_argument('--ai-decision-scratch', action='store_true')
    ap.add_argument('--ai-behavior-sentinels', action='store_true')
    ap.add_argument('--ai-child-inputs', action='store_true')
    ap.add_argument('--ai-property-inputs', action='store_true')
    ap.add_argument('--ai-dispatch-inputs', action='store_true')
    ap.add_argument('--ai-scalar-defaults', action='store_true')
    ap.add_argument('--ai-descriptor-callbacks', action='store_true')
    ap.add_argument('--ai-object-tracking-inputs', action='store_true')
    ap.add_argument('--ai-tracking-dispatch-inputs', action='store_true')
    ap.add_argument('--ai-object-decisions', action='store_true')
    ap.add_argument('--ai-workspace-leases', action='store_true')
    ap.add_argument('--ai-object-callbacks', action='store_true')
    ap.add_argument('--ai-object-callback-bodies', action='store_true')
    ap.add_argument('--ai-callback-numeric-inputs', action='store_true')
    ap.add_argument('--ai-prop-selector-inputs', action='store_true')
    ap.add_argument('--ai-birth-query-variants', action='store_true')
    ap.add_argument('--ai-actor-behavior', action='store_true')
    ap.add_argument('--ai-script-context', action='store_true')
    ap.add_argument('--ai-bsp-audibility-cache', action='store_true')
    ap.add_argument('--ai-difficulty-data', action='store_true')
    ap.add_argument('--camera-effect-types', action='store_true')
    ap.add_argument('--damage-readonly-data', action='store_true')
    ap.add_argument('--damage-reaction-data', action='store_true')
    ap.add_argument('--ai-prop-creation-storage', action='store_true')
    ap.add_argument('--hs-function-registry', action='store_true')
    ap.add_argument('--descriptor-source', type=Path)
    ap.add_argument('--factory-receipt', type=Path)
    ap.add_argument('--report-output', type=Path, required=True)
    args = ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Run through tools/run_test.py')
    if not 1 <= args.ticks <= 300:
        ap.error('Require 1..300 actual source ticks')
    squads=[args.squad,*args.additional_squad]
    if len(set(squads))!=len(squads) or not 1<=len(squads)<=16:
        ap.error('Require1..16 distinct authored squad cases')
    if args.placement_settings and not args.factory_receipt:
        ap.error('--placement-settings requires the exact cohort --factory-receipt')
    if args.ai_conditions and not args.placement_settings:
        ap.error('--ai-conditions requires the preceding --placement-settings increment')
    if args.physics_branch_data and not args.placement_settings:
        ap.error('--physics-branch-data requires the admitted --placement-settings factory')
    if args.ai_working_mask and not args.placement_settings:
        ap.error('--ai-working-mask requires the admitted --placement-settings factory')
    if args.ai_decision_scratch and not args.ai_working_mask:
        ap.error('--ai-decision-scratch requires the admitted --ai-working-mask increment')
    if args.ai_behavior_sentinels and not args.ai_decision_scratch:
        ap.error('--ai-behavior-sentinels requires the admitted --ai-decision-scratch increment')
    if args.ai_child_inputs and (not args.ai_behavior_sentinels or not args.descriptor_source):
        ap.error('--ai-child-inputs requires --ai-behavior-sentinels and the frozen --descriptor-source')
    if args.ai_property_inputs and not args.ai_child_inputs:
        ap.error('--ai-property-inputs requires --ai-child-inputs')
    if args.ai_dispatch_inputs and not args.ai_property_inputs:
        ap.error('--ai-dispatch-inputs requires --ai-property-inputs')
    if args.ai_scalar_defaults and (not args.ai_dispatch_inputs or not args.ai_conditions):
        ap.error('--ai-scalar-defaults requires --ai-dispatch-inputs and --ai-conditions')
    if args.ai_descriptor_callbacks and not args.ai_scalar_defaults:
        ap.error('--ai-descriptor-callbacks requires --ai-scalar-defaults')
    if args.ai_object_tracking_inputs and not args.ai_descriptor_callbacks:
        ap.error('--ai-object-tracking-inputs requires --ai-descriptor-callbacks')
    if args.ai_tracking_dispatch_inputs and not args.ai_object_tracking_inputs:
        ap.error('--ai-tracking-dispatch-inputs requires --ai-object-tracking-inputs')
    if args.ai_object_decisions and not args.ai_tracking_dispatch_inputs:
        ap.error('--ai-object-decisions requires --ai-tracking-dispatch-inputs')
    if args.ai_workspace_leases and not args.ai_object_decisions:
        ap.error('--ai-workspace-leases requires --ai-object-decisions')
    if args.ai_object_callbacks and not args.ai_workspace_leases:
        ap.error('--ai-object-callbacks requires --ai-workspace-leases')
    if args.ai_object_callback_bodies and not args.ai_object_callbacks:
        ap.error('--ai-object-callback-bodies requires --ai-object-callbacks')
    if args.ai_callback_numeric_inputs and not args.ai_object_callback_bodies:
        ap.error('--ai-callback-numeric-inputs requires --ai-object-callback-bodies')
    if args.ai_prop_selector_inputs and not args.ai_callback_numeric_inputs:
        ap.error('--ai-prop-selector-inputs requires --ai-callback-numeric-inputs')
    if args.ai_birth_query_variants and not args.ai_prop_selector_inputs:
        ap.error('--ai-birth-query-variants requires --ai-prop-selector-inputs')
    if args.ai_actor_behavior and not args.ai_birth_query_variants:
        ap.error('--ai-actor-behavior requires --ai-birth-query-variants')
    if args.ai_script_context and not args.ai_actor_behavior:
        ap.error('--ai-script-context requires --ai-actor-behavior')
    if args.ai_bsp_audibility_cache and not args.ai_actor_behavior:
        ap.error('--ai-bsp-audibility-cache requires --ai-actor-behavior')
    if args.ai_difficulty_data and not args.ai_actor_behavior:
        ap.error('--ai-difficulty-data requires --ai-actor-behavior')
    if args.camera_effect_types and not args.ai_child_inputs:
        ap.error('--camera-effect-types requires --ai-child-inputs')
    if args.damage_readonly_data and not args.ai_child_inputs:
        ap.error('--damage-readonly-data requires --ai-child-inputs')
    if args.damage_reaction_data and not args.damage_readonly_data:
        ap.error('--damage-reaction-data requires --damage-readonly-data')
    if args.ai_prop_creation_storage and not args.ai_object_decisions:
        ap.error('--ai-prop-creation-storage requires --ai-object-decisions')
    if args.hs_function_registry and not args.ai_script_context:
        ap.error('--hs-function-registry requires --ai-script-context')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.report_output = args.report_output.resolve()
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    cohort = json.loads(args.cohort.read_text())
    receipt = json.loads(args.world_receipt.read_text())
    command = list(receipt['reproduce_frozen_compile_command'])
    source = Path(command[command.index('-c')+1])
    prior_object = receipt['required_world']['path']
    assert cohort['link_inputs'][prior_object] == receipt['required_world']['sha256'] == sha(prior_object)
    assert all(Path(path).is_file() and sha(path) == digest
               for path, digest in cohort['link_inputs'].items()), 'Cohort provider changed'
    before = source.read_text()
    assert before.count(OLD) == 1 and NEW not in before
    current_world=(ROOT/'src/physics/physics_live_world_tu15.cpp').read_text()
    early_crt='''        if(c.target==0x32a990u) {
            source_run_process_heap_with_native_platform(bank,0x326e20u,a,image,fp,
                services(),virtual_memory,locks,lock_services(),o);return;
        }'''
    if NEW not in current_world:
        # Current world can instead admit the complete Process owner before
        # every private closure. Preserve that newer route in the checkout;
        # the frozen C55 uses its previously qualified portal exclusion.
        assert current_world.count(early_crt)==1 and current_world.index(early_crt)<current_world.index(OLD)
    after = before.replace(OLD, NEW)
    assert after.replace(NEW, OLD) == before
    leaf = None
    condition_leaf = None
    scalar_leaf = None
    decision_leaf = None
    callback_leaf = None
    birth_query_leaf = None
    actor_behavior_leaf = None
    if args.placement_settings:
        current = (ROOT/'src/physics/physics_live_world_tu15.cpp').read_text()
        assert HEADER in current and DISPATCH in current
        assert HEADER not in before and DISPATCH not in before
        anchor = '#include "halo2/source_ai_frame_tu15.hpp"'
        dispatch_anchor = '        if(source_campaign_runtime_tu15_owns_entry(c.target)) {'
        assert before.count(anchor) == 1 and before.count(dispatch_anchor) == 1
        after = after.replace(anchor, anchor+'\n'+HEADER).replace(dispatch_anchor, DISPATCH+dispatch_anchor)
        assert after.replace(anchor+'\n'+HEADER,anchor).replace(DISPATCH,'').replace(NEW,OLD) == before
        leaf_include = scratch/'leaf/include'
        declaration = leaf_include/'halo2/source_campaign_ai_placement_settings_tu15.hpp'
        declaration.parent.mkdir(parents=True)
        declaration.write_bytes((ROOT/'include/halo2/source_campaign_ai_placement_settings_tu15.hpp').read_bytes())
        leaf = scratch/'source_campaign_ai_placement_settings_tu15_generated.cpp'
        leaf.write_bytes((source_file(ROOT, leaf.name)).read_bytes())
        command.insert(1,'-I'+str(leaf_include))
        if args.ai_conditions:
            assert CONDITION_HEADER in current and CONDITION_DISPATCH in current
            assert CONDITION_HEADER not in before and CONDITION_DISPATCH not in before
            prior=after
            after=after.replace(HEADER,HEADER+'\n'+CONDITION_HEADER).replace(DISPATCH,CONDITION_DISPATCH+DISPATCH)
            assert after.replace(HEADER+'\n'+CONDITION_HEADER,HEADER).replace(CONDITION_DISPATCH,'')==prior
            condition_declaration=leaf_include/'halo2/source_campaign_ai_conditions_tu15.hpp'
            condition_declaration.write_bytes((ROOT/'include/halo2'/condition_declaration.name).read_bytes())
            condition_leaf=scratch/'source_campaign_ai_conditions_tu15_generated.cpp'
            condition_leaf.write_bytes((source_file(ROOT, condition_leaf.name)).read_bytes())
        if args.ai_scalar_defaults:
            assert all(part in current for part in (SCALAR_HEADER,SCALAR_DISPATCH,SCALAR_STARTUP))
            prior=after
            startup_anchor='''        for(std::uint32_t slot=0x46dad8u;slot<0x46dbf4u;slot+=4u)
            call_actual(word(bank,slot));
'''
            assert after.count(startup_anchor)==1 and SCALAR_HEADER not in after
            after=after.replace(CONDITION_HEADER,CONDITION_HEADER+'\n'+SCALAR_HEADER).replace(
                DISPATCH,SCALAR_DISPATCH+DISPATCH).replace(startup_anchor,startup_anchor+SCALAR_STARTUP)
            assert after.replace(CONDITION_HEADER+'\n'+SCALAR_HEADER,CONDITION_HEADER).replace(
                SCALAR_DISPATCH,'').replace(SCALAR_STARTUP,'')==prior
            scalar_declaration=leaf_include/'halo2/source_campaign_ai_scalar_defaults_tu15.hpp'
            scalar_declaration.write_bytes((ROOT/'include/halo2'/scalar_declaration.name).read_bytes())
            scalar_leaf=scratch/'source_campaign_ai_scalar_defaults_tu15.cpp'
            scalar_leaf.write_bytes((source_file(ROOT, scalar_leaf.name)).read_bytes())
        if args.ai_object_decisions:
            assert DECISION_HEADER in current and DECISION_DISPATCH in current
            prior=after
            after=after.replace(SCALAR_HEADER,SCALAR_HEADER+'\n'+DECISION_HEADER).replace(
                SCALAR_DISPATCH,SCALAR_DISPATCH+DECISION_DISPATCH)
            assert after.replace(SCALAR_HEADER+'\n'+DECISION_HEADER,SCALAR_HEADER).replace(DECISION_DISPATCH,'')==prior
            decision_declaration=leaf_include/'halo2/source_campaign_ai_object_decisions_tu15.hpp'
            decision_declaration.write_bytes((ROOT/'include/halo2'/decision_declaration.name).read_bytes())
            decision_leaf=scratch/'source_campaign_ai_object_decisions_tu15_generated.cpp'
            decision_leaf.write_bytes((source_file(ROOT, decision_leaf.name)).read_bytes())
        if args.ai_object_callback_bodies:
            assert CALLBACK_HEADER in current and CALLBACK_DISPATCH in current
            prior=after
            after=after.replace(DECISION_HEADER,DECISION_HEADER+'\n'+CALLBACK_HEADER).replace(
                DECISION_DISPATCH,DECISION_DISPATCH+CALLBACK_DISPATCH)
            assert after.replace(DECISION_HEADER+'\n'+CALLBACK_HEADER,DECISION_HEADER).replace(CALLBACK_DISPATCH,'')==prior
            callback_declaration=leaf_include/'halo2/source_campaign_ai_object_callbacks_tu15.hpp'
            callback_declaration.write_bytes((ROOT/'include/halo2'/callback_declaration.name).read_bytes())
            callback_leaf=scratch/'source_campaign_ai_object_callbacks_tu15_generated.cpp'
            callback_leaf.write_bytes((source_file(ROOT, callback_leaf.name)).read_bytes())
        if args.ai_birth_query_variants:
            assert BIRTH_QUERY_HEADER in current and BIRTH_QUERY_DISPATCH in current
            prior=after
            after=after.replace(CALLBACK_HEADER,CALLBACK_HEADER+'\n'+BIRTH_QUERY_HEADER).replace(
                CALLBACK_DISPATCH,CALLBACK_DISPATCH+BIRTH_QUERY_DISPATCH)
            assert after.replace(CALLBACK_HEADER+'\n'+BIRTH_QUERY_HEADER,CALLBACK_HEADER).replace(BIRTH_QUERY_DISPATCH,'')==prior
            birth_query_declaration=leaf_include/'halo2/source_campaign_birth_query_variants_tu15.hpp'
            birth_query_declaration.write_bytes((ROOT/'include/halo2'/birth_query_declaration.name).read_bytes())
            birth_query_leaf=scratch/'source_campaign_birth_query_variants_tu15_generated.cpp'
            birth_query_leaf.write_bytes((source_file(ROOT, birth_query_leaf.name)).read_bytes())
        if args.ai_actor_behavior:
            assert ACTOR_BEHAVIOR_HEADER in current and ACTOR_BEHAVIOR_DISPATCH in current
            prior=after
            after=after.replace(BIRTH_QUERY_HEADER,BIRTH_QUERY_HEADER+'\n'+ACTOR_BEHAVIOR_HEADER).replace(
                BIRTH_QUERY_DISPATCH,BIRTH_QUERY_DISPATCH+ACTOR_BEHAVIOR_DISPATCH)
            assert after.replace(BIRTH_QUERY_HEADER+'\n'+ACTOR_BEHAVIOR_HEADER,BIRTH_QUERY_HEADER).replace(ACTOR_BEHAVIOR_DISPATCH,'')==prior
            actor_behavior_declaration=leaf_include/'halo2/source_campaign_ai_actor_behavior_tu15.hpp'
            actor_behavior_declaration.write_bytes((ROOT/'include/halo2'/actor_behavior_declaration.name).read_bytes())
            actor_behavior_leaf=scratch/'source_campaign_ai_actor_behavior_tu15_generated.cpp'
            actor_behavior_leaf.write_bytes((source_file(ROOT, actor_behavior_leaf.name)).read_bytes())
    staged = scratch/'physics_live_world_tu15.cpp'
    staged.write_text(after)
    obj, dep, client = scratch/'world.o', scratch/'world.d', scratch/'halo2_native'
    command = [arg.replace('@HALO2_TEST_SCRATCH@', str(scratch)) for arg in command]
    command[command.index('-c')+1] = str(staged)
    command[command.index('-o')+1] = str(obj)
    command[command.index('-MF')+1] = str(dep)
    # Debug information is irrelevant to this private acceptance candidate.
    command = [arg for arg in command if arg != '-g']
    command.append('-g0')
    record = dict(passed=False, scope=__doc__, scratch=str(scratch),
        tool_sha256=sha(__file__),
        cohort=str(args.cohort.resolve()), cohort_sha256=sha(args.cohort),
        world_receipt=str(args.world_receipt.resolve()), world_receipt_sha256=sha(args.world_receipt),
        source={'path': str(source), 'sha256': sha(source), 'staged_sha256': sha(staged),
                'old': OLD, 'new': NEW, 'exact_reverse': True},
        current_world_sha256=sha(ROOT/'src/physics/physics_live_world_tu15.cpp'),
        current_world_shared_crt_route='portal exclusion' if NEW in current_world else 'early complete Process owner',
        placement_settings=args.placement_settings,
        ai_conditions=args.ai_conditions,
        physics_branch_data=args.physics_branch_data,
        ai_working_mask=args.ai_working_mask,
        ai_decision_scratch=args.ai_decision_scratch,
        ai_behavior_sentinels=args.ai_behavior_sentinels,
        ai_child_inputs=args.ai_child_inputs,
        ai_property_inputs=args.ai_property_inputs,
        ai_dispatch_inputs=args.ai_dispatch_inputs,
        ai_scalar_defaults=args.ai_scalar_defaults,
        ai_descriptor_callbacks=args.ai_descriptor_callbacks,
        ai_object_tracking_inputs=args.ai_object_tracking_inputs,
        ai_tracking_dispatch_inputs=args.ai_tracking_dispatch_inputs,
        ai_object_decisions=args.ai_object_decisions,
        ai_workspace_leases=args.ai_workspace_leases,
        ai_object_callbacks=args.ai_object_callbacks,
        ai_object_callback_bodies=args.ai_object_callback_bodies,
        ai_callback_numeric_inputs=args.ai_callback_numeric_inputs,
        ai_prop_selector_inputs=args.ai_prop_selector_inputs,
        ai_birth_query_variants=args.ai_birth_query_variants,
        ai_actor_behavior=args.ai_actor_behavior,
        ai_script_context=args.ai_script_context,
        ai_bsp_audibility_cache=args.ai_bsp_audibility_cache,
        ai_difficulty_data=args.ai_difficulty_data,
        camera_effect_types=args.camera_effect_types,
        damage_readonly_data=args.damage_readonly_data,
        damage_reaction_data=args.damage_reaction_data,
        ai_prop_creation_storage=args.ai_prop_creation_storage,
        hs_function_registry=args.hs_function_registry,
        retained_large_artifacts=[], installed_runtime_changed=False, steps=[])

    def save():
        args.report_output.write_text(json.dumps(record, indent=2)+'\n')

    def run(name, argv, timeout=600):
        log = scratch/(name+'.log')
        row = dict(name=name, command=argv)
        record['steps'].append(row)
        save()
        print(name, flush=True)
        with log.open('w') as stream:
            result = subprocess.run(argv, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT, timeout=timeout)
        row['returncode'] = result.returncode
        with log.open('rb') as stream:
            stream.seek(max(0, log.stat().st_size-16000))
            row['diagnostic'] = stream.read().decode('utf-8', errors='replace')
        save()
        if result.returncode:
            raise RuntimeError(name+': '+row['diagnostic'][-5000:])

    try:
        run('compile-existing-engine-route', command)
        dependencies = [Path(name) for name in shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1])]
        leaf_object = None
        factory_object = None
        condition_object = None
        branch_object = None
        child_data_object = None
        scalar_object = None
        decision_object = None
        callback_object = None
        birth_query_object = None
        actor_behavior_object = None
        if actor_behavior_leaf:
            actor_behavior_object,actor_behavior_dep=scratch/'actor-behavior.o',scratch/'actor-behavior.d'
            actor_behavior_command=list(command)
            actor_behavior_command[actor_behavior_command.index('-c')+1]=str(actor_behavior_leaf)
            actor_behavior_command[actor_behavior_command.index('-o')+1]=str(actor_behavior_object)
            actor_behavior_command[actor_behavior_command.index('-MF')+1]=str(actor_behavior_dep)
            run('compile-original-actor-behavior',actor_behavior_command)
            dependencies += [Path(name) for name in shlex.split(actor_behavior_dep.read_text().replace('\\\n',' ').split(':',1)[1])]
            record['actor_behavior_source_sha256']=sha(actor_behavior_leaf)
            record['actor_behavior_declaration_sha256']=sha(actor_behavior_declaration)
        if birth_query_leaf:
            birth_query_object,birth_query_dep=scratch/'birth-query-variants.o',scratch/'birth-query-variants.d'
            birth_query_command=list(command)
            birth_query_command[birth_query_command.index('-c')+1]=str(birth_query_leaf)
            birth_query_command[birth_query_command.index('-o')+1]=str(birth_query_object)
            birth_query_command[birth_query_command.index('-MF')+1]=str(birth_query_dep)
            run('compile-original-birth-query-variants',birth_query_command)
            dependencies += [Path(name) for name in shlex.split(birth_query_dep.read_text().replace('\\\n',' ').split(':',1)[1])]
            record['birth_query_source_sha256']=sha(birth_query_leaf)
            record['birth_query_declaration_sha256']=sha(birth_query_declaration)
        if callback_leaf:
            callback_object,callback_dep=scratch/'ai-object-callbacks.o',scratch/'ai-object-callbacks.d'
            callback_command=list(command)
            callback_command[callback_command.index('-c')+1]=str(callback_leaf)
            callback_command[callback_command.index('-o')+1]=str(callback_object)
            callback_command[callback_command.index('-MF')+1]=str(callback_dep)
            run('compile-original-ai-object-callbacks',callback_command)
            dependencies += [Path(name) for name in shlex.split(callback_dep.read_text().replace('\\\n',' ').split(':',1)[1])]
            record['ai_object_callbacks_source_sha256']=sha(callback_leaf)
            record['ai_object_callbacks_declaration_sha256']=sha(callback_declaration)
        if decision_leaf:
            decision_object,decision_dep=scratch/'ai-object-decisions.o',scratch/'ai-object-decisions.d'
            decision_command=list(command)
            decision_command[decision_command.index('-c')+1]=str(decision_leaf)
            decision_command[decision_command.index('-o')+1]=str(decision_object)
            decision_command[decision_command.index('-MF')+1]=str(decision_dep)
            run('compile-original-ai-object-decisions',decision_command)
            dependencies += [Path(name) for name in shlex.split(decision_dep.read_text().replace('\\\n',' ').split(':',1)[1])]
            record['ai_object_decisions_source_sha256']=sha(decision_leaf)
            record['ai_object_decisions_declaration_sha256']=sha(decision_declaration)
        if scalar_leaf:
            scalar_object,scalar_dep=scratch/'ai-scalar-defaults.o',scratch/'ai-scalar-defaults.d'
            scalar_command=list(command)
            scalar_command[scalar_command.index('-c')+1]=str(scalar_leaf)
            scalar_command[scalar_command.index('-o')+1]=str(scalar_object)
            scalar_command[scalar_command.index('-MF')+1]=str(scalar_dep)
            run('compile-original-ai-scalar-defaults',scalar_command)
            dependencies += [Path(name) for name in shlex.split(scalar_dep.read_text().replace('\\\n',' ').split(':',1)[1])]
            record['ai_scalar_defaults_source_sha256']=sha(scalar_leaf)
            record['ai_scalar_defaults_declaration_sha256']=sha(scalar_declaration)
            record['ai_scalar_defaults_startup']=dict(table=0x46dd00,bytes=12,callbacks=3,
                original_order_before_ai_storage=True,exact_reverse=True)
        if leaf:
            leaf_object, leaf_dep = scratch/'placement-settings.o', scratch/'placement-settings.d'
            leaf_command = list(command)
            leaf_command[leaf_command.index('-c')+1] = str(leaf)
            leaf_command[leaf_command.index('-o')+1] = str(leaf_object)
            leaf_command[leaf_command.index('-MF')+1] = str(leaf_dep)
            run('compile-original-placement-settings', leaf_command)
            dependencies += [Path(name) for name in shlex.split(leaf_dep.read_text().replace('\\\n',' ').split(':',1)[1])]
            record['placement_settings_source_sha256'] = sha(leaf)
            record['placement_settings_declaration_sha256'] = sha(declaration)
            if condition_leaf:
                condition_object,condition_dep=scratch/'ai-conditions.o',scratch/'ai-conditions.d'
                condition_command=list(command)
                condition_command[condition_command.index('-c')+1]=str(condition_leaf)
                condition_command[condition_command.index('-o')+1]=str(condition_object)
                condition_command[condition_command.index('-MF')+1]=str(condition_dep)
                run('compile-original-ai-conditions',condition_command)
                dependencies += [Path(name) for name in shlex.split(condition_dep.read_text().replace('\\\n',' ').split(':',1)[1])]
                record['ai_conditions_source_sha256']=sha(condition_leaf)
                record['ai_conditions_declaration_sha256']=sha(condition_declaration)
            if args.physics_branch_data:
                branch_header=leaf_include/'halo2/native_tu15_physics_branch_data.hpp'
                branch_header.write_bytes((ROOT/'include/halo2'/branch_header.name).read_bytes())
                branch_source=scratch/'native_tu15_physics_branch_data.cpp'
                branch_pins=None
                if args.branch_receipt:
                    prior_branch=json.loads(args.branch_receipt.read_text())
                    branch_pins={Path(path).name:digest for path,digest in prior_branch['compiled_dependencies_sha256'].items()}
                for name in ('native_tu15_physics_branch_data.cpp','native_tu15_physics_branch_data.inc',
                             'native_tu15_physics_callable_data.inc','native_tu15_physics_literal_data.inc',
                             'native_tu15_ai_literal_data.inc'):
                    data=(source_file(ROOT, name)).read_bytes()
                    if branch_pins and hashlib.sha256(data).hexdigest()!=branch_pins[name]:
                        assert name=='native_tu15_physics_branch_data.cpp','Qualified data bytes changed'
                        original_branch=data.decode()
                        data=original_branch.replace('#include "gameplay/world/native_tu15_teleporter_query_data.inc"\n','').replace(
                            'std::span<const BranchTable>,5>','std::span<const BranchTable>,4>').replace(
                            ',ai_literal_tables,teleporter_query_tables',',ai_literal_tables').encode()
                        assert hashlib.sha256(data).hexdigest()==branch_pins[name], 'Cannot recover exact qualified branch owner'
                        record['qualified_branch_source']=dict(receipt=str(args.branch_receipt.resolve()),
                            receipt_sha256=sha(args.branch_receipt),root_source_sha256=sha(source_file(ROOT, name)),
                            qualified_source_sha256=hashlib.sha256(data).hexdigest(),root_file_changed=False)
                    (scratch/name).write_bytes(data)
                branch_object,branch_dep=scratch/'branch-data.o',scratch/'branch-data.d'
                branch_command=list(command)
                branch_command[branch_command.index('-c')+1]=str(branch_source)
                branch_command[branch_command.index('-o')+1]=str(branch_object)
                branch_command[branch_command.index('-MF')+1]=str(branch_dep)
                run('compile-existing-immutable-physics-owner',branch_command)
                dependencies += [Path(name) for name in shlex.split(branch_dep.read_text().replace('\\\n',' ').split(':',1)[1])]
            factory_receipt=json.loads(args.factory_receipt.read_text())
            factory_command=[arg.replace('@HALO2_TEST_SCRATCH@',str(scratch))
                             for arg in factory_receipt['reproduce_frozen_compile_command']]
            if args.physics_branch_data:factory_command.insert(1,'-I'+str(leaf_include))
            prior_factory=factory_receipt['required_factory']['path']
            assert cohort['link_inputs'][prior_factory]==factory_receipt['required_factory']['sha256']==sha(prior_factory)
            factory_source=Path(factory_command[factory_command.index('-c')+1])
            factory_original=factory_source.read_text()
            anchor='#include "native_tu15_reused_campaign_runtime_tables.inc"'
            addition='#include "campaign/ai/native_tu15_campaign_ai_placement_settings_tables.inc"'
            assert factory_original.count(anchor)==1 and addition not in factory_original
            table_source=ROOT/'src/campaign/ai/native_tu15_campaign_ai_placement_settings_tables.inc'
            table_staged=scratch/table_source.name
            table_staged.write_bytes(table_source.read_bytes())
            factory_staged=scratch/'factory.cpp'
            factory_after=factory_original.replace(anchor,anchor+'\n'+addition)
            assert factory_after.replace(anchor+'\n'+addition,anchor)==factory_original
            if condition_leaf:
                condition_tables=ROOT/'src/campaign/ai/native_tu15_campaign_ai_conditions_tables.inc'
                (scratch/condition_tables.name).write_bytes(condition_tables.read_bytes())
                condition_addition='#include "campaign/ai/native_tu15_campaign_ai_conditions_tables.inc"'
                assert condition_addition not in factory_original
                factory_after=factory_after.replace(addition,addition+'\n'+condition_addition)
                assert factory_after.replace(addition+'\n'+condition_addition,addition).replace(anchor+'\n'+addition,anchor)==factory_original
                record['ai_conditions_tables_sha256']=sha(condition_tables)
            if args.physics_branch_data:
                branch_include='#include "halo2/native_tu15_physics_branch_data.hpp"\n'
                branch_call='    mount_native_tu15_physics_branch_data(memory);\n'
                assert branch_include not in factory_original and branch_call not in factory_original
                anchor_call='    mount_reused_native_tu15_ai_behavior_descriptor_data(memory);\n'
                assert factory_after.count(anchor_call)==1
                factory_after=branch_include+factory_after.replace(anchor_call,anchor_call+branch_call)
                reversed_factory=factory_after.replace(branch_include,'',1).replace(branch_call,'')
                if condition_leaf:reversed_factory=reversed_factory.replace(addition+'\n'+condition_addition,addition)
                assert reversed_factory.replace(anchor+'\n'+addition,anchor)==factory_original
            if args.ai_working_mask:
                # Compose exactly the canonical cold() row through its same
                # public mount operation. The old cohort storage provider is
                # unchanged; no live fields, mask bits or result are supplied.
                cold_source=ROOT/'src/platform/native_tu15_final_startup_storage_data.inc'
                assert cold_source.read_text().count('cold(memory,0x56ae84u,0x14u);')==1
                cold_anchor='    mount_native_tu15_final_startup_storage_data(memory);\n'
                cold_mount='''    memory.mount(0x56ae84u,std::vector<std::byte>(0x14u),SourceRuntimeMemory::Access::ReadWrite,
        "TU1.5 final startup loader-zeroed constructor destination");
'''
                assert factory_after.count(cold_anchor)==1 and cold_mount not in factory_original
                prior=factory_after
                factory_after=factory_after.replace(cold_anchor,cold_anchor+cold_mount)
                assert factory_after.replace(cold_mount,'')==prior
                record['ai_working_mask_storage']=dict(address=0x56ae84,bytes=20,
                    source=str(cold_source),source_sha256=sha(cold_source),exact_cold_mount=cold_mount)
                if args.ai_decision_scratch:
                    rows=((0x4fafc0,4),(0x4fafc8,3*131*24),(0x4fd4a0,131),(0x4fd524,5*4))
                    for address,size in rows:
                        assert cold_source.read_text().count(f'cold(memory,0x{address:x}u,0x{size:x}u);')==1
                    workspace_mount=''.join(f'    memory.mount(0x{address:x}u,std::vector<std::byte>(0x{size:x}u),SourceRuntimeMemory::Access::ReadWrite,\n'
                        '        "TU1.5 final startup loader-zeroed constructor destination");\n' for address,size in rows)
                    prior=factory_after
                    factory_after=factory_after.replace(cold_anchor+cold_mount,cold_anchor+cold_mount+workspace_mount)
                    assert factory_after.replace(workspace_mount,'')==prior
                    record['ai_decision_scratch_storage']=dict(rows=rows,source_sha256=sha(cold_source),
                        exact_cold_mounts=workspace_mount)
                    if args.ai_behavior_sentinels:
                        descriptor_source=ROOT/'src/campaign/ai/native_tu15_ai_behavior_descriptor_data.inc'
                        sentinel_row='literal(memory,0x47c1a4u,std::array<std::uint8_t,10>{0xff,0xff,0x00,0x00,0xfe,0xff,0x00,0x00,0xfd,0xff});'
                        assert descriptor_source.read_text().count(sentinel_row)==1
                        sentinel_mount='''    memory.mount(0x47c1a8u,std::array{std::byte{0xfe},std::byte{0xff},std::byte{0},std::byte{0},std::byte{0xfd},std::byte{0xff}},
        SourceRuntimeMemory::Access::ReadOnly,"TU1.5 final startup immutable constructor metadata");
'''
                        # The frozen owner already provides the first DWORD.
                        # Compose only the remaining six authenticated bytes.
                        prior=factory_after
                        factory_after=factory_after.replace(cold_anchor,cold_anchor+sentinel_mount)
                        assert factory_after.replace(sentinel_mount,'')==prior
                        record['ai_behavior_sentinels_storage']=dict(address=0x47c1a8,bytes=6,
                            original_first_dword_owner_preserved=True,source_sha256=sha(descriptor_source),
                            exact_literal_mount=sentinel_mount)
            if args.ai_scalar_defaults:
                assert cold_source.read_text().count('cold(memory,0x56e08cu,0x4u);')==1
                scalar_cold='''    memory.mount(0x56e08cu,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,
        "TU1.5 final startup loader-zeroed constructor destination");
'''
                prior=factory_after
                factory_after=factory_after.replace(cold_anchor,cold_anchor+scalar_cold)
                assert factory_after.replace(scalar_cold,'')==prior
                record['ai_scalar_default_storage']=dict(address=0x56e08c,bytes=4,source_sha256=sha(cold_source),
                    exact_cold_mount=scalar_cold,completed_scalar_seed=False)
            if args.ai_workspace_leases:
                rows=((0x50208c,24),(0x52be54,4),(0x52be58,1))
                for address,size in rows:
                    assert cold_source.read_text().count(f'cold(memory,0x{address:x}u,0x{size:x}u);')==1
                lease_mount=''.join(f'    memory.mount(0x{address:x}u,std::vector<std::byte>(0x{size:x}u),SourceRuntimeMemory::Access::ReadWrite,\n'
                    '        "TU1.5 final startup loader-zeroed constructor destination");\n' for address,size in rows)
                prior=factory_after
                factory_after=factory_after.replace(cold_anchor,cold_anchor+lease_mount)
                assert factory_after.replace(lease_mount,'')==prior
                record['ai_workspace_lease_storage']=dict(rows=rows,source_sha256=sha(cold_source),
                    exact_cold_mounts=lease_mount,completed_lease_seed=False)
            if args.ai_object_callback_bodies:
                rows=((0x52d874,2),(0x52d878,4))
                for address,size in rows:
                    assert cold_source.read_text().count(f'cold(memory,0x{address:x}u,0x{size:x}u);')==1
                context_mount=''.join(f'    memory.mount(0x{address:x}u,std::vector<std::byte>(0x{size:x}u),SourceRuntimeMemory::Access::ReadWrite,\n'
                    '        "TU1.5 final startup loader-zeroed constructor destination");\n' for address,size in rows)
                prior=factory_after
                factory_after=factory_after.replace(cold_anchor,cold_anchor+context_mount)
                assert factory_after.replace(context_mount,'')==prior
                record['ai_object_callback_context']=dict(rows=rows,source_sha256=sha(cold_source),
                    exact_cold_mounts=context_mount,source_producer_required=True,completed_context_seed=False)
            if args.ai_script_context:
                assert cold_source.read_text().count('cold(memory,0x50eee4u,0x8u);')==1
                script_context_mount='''    memory.mount(0x50eee4u,std::vector<std::byte>(8),SourceRuntimeMemory::Access::ReadWrite,
        "TU1.5 final startup loader-zeroed constructor destination");
'''
                prior=factory_after
                factory_after=factory_after.replace(cold_anchor,cold_anchor+script_context_mount)
                assert factory_after.replace(script_context_mount,'')==prior
                record['ai_script_context']=dict(address=0x50eee4,bytes=8,source_sha256=sha(cold_source),
                    exact_cold_mount=script_context_mount,source_producer_required=True,completed_context_seed=False)
            if args.ai_bsp_audibility_cache:
                assert cold_source.read_text().count('cold(memory,0x502200u,0x40u);')==1
                bsp_cache_mount='''    memory.mount(0x502200u,std::vector<std::byte>(0x40),SourceRuntimeMemory::Access::ReadWrite,
        "TU1.5 loader-zeroed BSP audibility cache; original203C00 produces its mask");
'''
                prior=factory_after
                factory_after=factory_after.replace(cold_anchor,cold_anchor+bsp_cache_mount)
                assert factory_after.replace(bsp_cache_mount,'')==prior
                record['ai_bsp_audibility_cache_storage']=dict(address=0x502200,bytes=64,source_sha256=sha(cold_source),
                    exact_cold_mount=bsp_cache_mount,source_producer_required=True,cached_mask_or_key_seeded=False)
            factory_staged.write_text(factory_after)
            factory_object,factory_dep=scratch/'factory.o',scratch/'factory.d'
            factory_command[factory_command.index('-c')+1]=str(factory_staged)
            factory_command[factory_command.index('-o')+1]=str(factory_object)
            factory_command[factory_command.index('-MF')+1]=str(factory_dep)
            factory_command=[arg for arg in factory_command if arg!='-g']+['-g0']
            run('compile-original-settings-table-owner',factory_command)
            dependencies += [Path(name) for name in shlex.split(factory_dep.read_text().replace('\\\n',' ').split(':',1)[1])]
            record['factory_settings_table'] = dict(receipt=str(args.factory_receipt.resolve()),
                receipt_sha256=sha(args.factory_receipt),original_source_sha256=sha(factory_source),
                staged_source_sha256=sha(factory_staged),tables_sha256=sha(table_staged),
                original_provider=prior_factory,exact_reverse=True)
        if args.ai_child_inputs:
            # Extend the same frozen descriptor publisher, retaining its exact
            # metadata and mutable caches. Only the new literal field family
            # is added; the current public mounter uses the identical helper.
            descriptor_source=args.descriptor_source.resolve()
            original=descriptor_source.read_text()
            old_include='#include "native_tu15_reused_ai_behavior_descriptor_data.inc"'
            new_include='#include "campaign/ai/native_tu15_ai_behavior_child_data.inc"'
            assert original.count(old_include)==1 and new_include not in original
            helper=original[original.index('template<std::size_t N> void literal'):original.index('\n}\nvoid mount_reused')]
            assert helper in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
            old_data=descriptor_source.parent/'native_tu15_reused_ai_behavior_descriptor_data.inc'
            sentinel_row='literal(memory,0x47c1a4u,std::array<std::uint8_t,10>{0xff,0xff,0x00,0x00,0xfe,0xff,0x00,0x00,0xfd,0xff});\n'
            assert (ROOT/'src/campaign/ai/native_tu15_ai_behavior_descriptor_data.inc').read_text().replace(sentinel_row,'')==old_data.read_text()
            (scratch/old_data.name).write_bytes(old_data.read_bytes())
            child_data=ROOT/'src/campaign/ai/native_tu15_ai_behavior_child_data.inc'
            (scratch/child_data.name).write_bytes(child_data.read_bytes())
            additions=new_include
            if args.ai_property_inputs:
                property_data=ROOT/'src/campaign/ai/native_tu15_ai_behavior_property_child_data.inc'
                property_include='#include "campaign/ai/native_tu15_ai_behavior_property_child_data.inc"'
                assert property_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/property_data.name).write_bytes(property_data.read_bytes())
                additions+='\n'+property_include
                if args.ai_dispatch_inputs:
                    dispatch_data=ROOT/'src/campaign/ai/native_tu15_ai_behavior_dispatch_data.inc'
                    dispatch_include='#include "campaign/ai/native_tu15_ai_behavior_dispatch_data.inc"'
                    assert dispatch_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                    (scratch/dispatch_data.name).write_bytes(dispatch_data.read_bytes())
                    additions+='\n'+dispatch_include
            if args.ai_scalar_defaults:
                scalar_data=ROOT/'src/campaign/ai/native_tu15_ai_scalar_defaults_data.inc'
                scalar_include='#include "campaign/ai/native_tu15_ai_scalar_defaults_data.inc"'
                assert scalar_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/scalar_data.name).write_bytes(scalar_data.read_bytes())
                additions+='\n'+scalar_include
            if args.ai_descriptor_callbacks:
                callback_data=ROOT/'src/campaign/ai/native_tu15_ai_behavior_callback_tail_data.inc'
                callback_include='#include "campaign/ai/native_tu15_ai_behavior_callback_tail_data.inc"'
                assert callback_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/callback_data.name).write_bytes(callback_data.read_bytes())
                additions+='\n'+callback_include
            if args.ai_object_tracking_inputs:
                tracking_data=ROOT/'src/campaign/ai/native_tu15_ai_object_tracking_data.inc'
                tracking_include='#include "campaign/ai/native_tu15_ai_object_tracking_data.inc"'
                assert tracking_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/tracking_data.name).write_bytes(tracking_data.read_bytes())
                additions+='\n'+tracking_include
            if args.ai_tracking_dispatch_inputs:
                tracking_dispatch_data=ROOT/'src/campaign/ai/native_tu15_ai_tracking_dispatch_data.inc'
                tracking_dispatch_include='#include "campaign/ai/native_tu15_ai_tracking_dispatch_data.inc"'
                assert tracking_dispatch_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/tracking_dispatch_data.name).write_bytes(tracking_dispatch_data.read_bytes())
                additions+='\n'+tracking_dispatch_include
            if args.ai_object_decisions:
                decision_data=ROOT/'src/campaign/ai/native_tu15_ai_object_decision_data.inc'
                decision_include='#include "campaign/ai/native_tu15_ai_object_decision_data.inc"'
                assert decision_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/decision_data.name).write_bytes(decision_data.read_bytes())
                additions+='\n'+decision_include
            if args.ai_object_callbacks:
                object_callbacks_data=ROOT/'src/campaign/ai/native_tu15_ai_object_callback_data.inc'
                object_callbacks_include='#include "campaign/ai/native_tu15_ai_object_callback_data.inc"'
                assert object_callbacks_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/object_callbacks_data.name).write_bytes(object_callbacks_data.read_bytes())
                additions+='\n'+object_callbacks_include
            if args.ai_object_callback_bodies:
                callback_body_data=ROOT/'src/campaign/ai/native_tu15_ai_object_callback_body_data.inc'
                callback_body_include='#include "campaign/ai/native_tu15_ai_object_callback_body_data.inc"'
                assert callback_body_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/callback_body_data.name).write_bytes(callback_body_data.read_bytes())
                additions+='\n'+callback_body_include
            if args.ai_callback_numeric_inputs:
                numeric_data=ROOT/'src/campaign/ai/native_tu15_ai_callback_numeric_data.inc'
                numeric_include='#include "campaign/ai/native_tu15_ai_callback_numeric_data.inc"'
                assert numeric_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/numeric_data.name).write_bytes(numeric_data.read_bytes())
                additions+='\n'+numeric_include
            if args.ai_prop_selector_inputs:
                prop_selector_data=ROOT/'src/campaign/ai/native_tu15_ai_prop_selector_data.inc'
                prop_selector_include='#include "campaign/ai/native_tu15_ai_prop_selector_data.inc"'
                assert prop_selector_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/prop_selector_data.name).write_bytes(prop_selector_data.read_bytes())
                additions+='\n'+prop_selector_include
            if args.ai_actor_behavior:
                actor_behavior_data=ROOT/'src/campaign/ai/native_tu15_campaign_ai_actor_behavior_data.inc'
                actor_behavior_include='#include "campaign/ai/native_tu15_campaign_ai_actor_behavior_data.inc"'
                assert actor_behavior_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/actor_behavior_data.name).write_bytes(actor_behavior_data.read_bytes())
                additions+='\n'+actor_behavior_include
            if args.ai_difficulty_data:
                difficulty_data=ROOT/'src/campaign/ai/native_tu15_campaign_ai_difficulty_data.inc'
                difficulty_include='#include "campaign/ai/native_tu15_campaign_ai_difficulty_data.inc"'
                assert difficulty_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/difficulty_data.name).write_bytes(difficulty_data.read_bytes())
                additions+='\n'+difficulty_include
            if args.camera_effect_types:
                camera_types_data=ROOT/'src/campaign/native_tu15_campaign_camera_effect_type_data.inc'
                camera_types_include='#include "campaign/native_tu15_campaign_camera_effect_type_data.inc"'
                assert camera_types_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/camera_types_data.name).write_bytes(camera_types_data.read_bytes())
                additions+='\n'+camera_types_include
            if args.damage_readonly_data:
                damage_data=ROOT/'src/campaign/native_tu15_campaign_damage_literal_data.inc'
                damage_include='#include "campaign/native_tu15_campaign_damage_literal_data.inc"'
                assert damage_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/damage_data.name).write_bytes(damage_data.read_bytes())
                additions+='\n'+damage_include
            if args.damage_reaction_data:
                reaction_data=ROOT/'src/campaign/native_tu15_campaign_damage_reaction_data.inc'
                reaction_include='#include "campaign/native_tu15_campaign_damage_reaction_data.inc"'
                assert reaction_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/reaction_data.name).write_bytes(reaction_data.read_bytes())
                additions+='\n'+reaction_include
            if args.ai_prop_creation_storage:
                prop_creation_data=ROOT/'src/campaign/ai/native_tu15_campaign_prop_creation_storage.inc'
                prop_creation_include='#include "campaign/ai/native_tu15_campaign_prop_creation_storage.inc"'
                assert prop_creation_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/prop_creation_data.name).write_bytes(prop_creation_data.read_bytes())
                additions+='\n'+prop_creation_include
            if args.hs_function_registry:
                hs_registry_data=ROOT/'src/campaign/ai/native_tu15_campaign_hs_function_data.inc'
                hs_registry_include='#include "campaign/ai/native_tu15_campaign_hs_function_data.inc"'
                assert hs_registry_include in (ROOT/'src/platform/native_tu15_final_startup_storage_data.cpp').read_text()
                (scratch/hs_registry_data.name).write_bytes(hs_registry_data.read_bytes())
                additions+='\n'+hs_registry_include
            staged_descriptor=scratch/descriptor_source.name
            staged_descriptor.write_text(original.replace(old_include,old_include+'\n'+additions))
            assert staged_descriptor.read_text().replace(old_include+'\n'+additions,old_include)==original
            providers=[p for p in cohort['link_inputs'] if Path(p).name=='native_tu15_ai_behavior_descriptor_mount.o']
            assert len(providers)==1
            prior_child_data=providers[0]
            child_data_object,child_dep=scratch/'child-data.o',scratch/'child-data.d'
            child_command=list(command)
            child_command[child_command.index('-c')+1]=str(staged_descriptor)
            child_command[child_command.index('-o')+1]=str(child_data_object)
            child_command[child_command.index('-MF')+1]=str(child_dep)
            run('compile-original-child-metadata-owner',child_command)
            dependencies += [Path(name) for name in shlex.split(child_dep.read_text().replace('\\\n',' ').split(':',1)[1])]
            record['ai_child_metadata']=dict(original_provider=prior_child_data,
                original_provider_sha256=cohort['link_inputs'][prior_child_data],
                frozen_source=str(descriptor_source),frozen_source_sha256=sha(descriptor_source),
                staged_source_sha256=sha(staged_descriptor),child_data_sha256=sha(child_data),
                identical_public_literal_helper=True,exact_reverse=True)
            if args.ai_property_inputs:
                record['ai_child_metadata']['property_child_data_sha256']=sha(property_data)
                if args.ai_dispatch_inputs:record['ai_child_metadata']['dispatch_data_sha256']=sha(dispatch_data)
            if args.ai_scalar_defaults:record['ai_child_metadata']['scalar_data_sha256']=sha(scalar_data)
            if args.ai_descriptor_callbacks:record['ai_child_metadata']['callback_tail_data_sha256']=sha(callback_data)
            if args.ai_object_tracking_inputs:record['ai_child_metadata']['object_tracking_data_sha256']=sha(tracking_data)
            if args.ai_tracking_dispatch_inputs:record['ai_child_metadata']['tracking_dispatch_data_sha256']=sha(tracking_dispatch_data)
            if args.ai_object_decisions:record['ai_child_metadata']['object_decision_data_sha256']=sha(decision_data)
            if args.ai_object_callbacks:record['ai_child_metadata']['object_callbacks_data_sha256']=sha(object_callbacks_data)
            if args.ai_object_callback_bodies:record['ai_child_metadata']['object_callback_body_data_sha256']=sha(callback_body_data)
            if args.ai_callback_numeric_inputs:record['ai_child_metadata']['callback_numeric_data_sha256']=sha(numeric_data)
            if args.ai_prop_selector_inputs:record['ai_child_metadata']['prop_selector_data_sha256']=sha(prop_selector_data)
            if args.ai_actor_behavior:record['ai_child_metadata']['actor_behavior_data_sha256']=sha(actor_behavior_data)
            if args.ai_difficulty_data:record['ai_child_metadata']['difficulty_data_sha256']=sha(difficulty_data)
            if args.camera_effect_types:record['ai_child_metadata']['camera_effect_types_data_sha256']=sha(camera_types_data)
            if args.damage_readonly_data:record['ai_child_metadata']['damage_readonly_data_sha256']=sha(damage_data)
            if args.damage_reaction_data:record['ai_child_metadata']['damage_reaction_data_sha256']=sha(reaction_data)
            if args.ai_prop_creation_storage:record['ai_child_metadata']['prop_creation_storage_sha256']=sha(prop_creation_data)
            if args.hs_function_registry:record['ai_child_metadata']['hs_registry_data_sha256']=sha(hs_registry_data)
        record['compiled_dependencies_sha256'] = {str(path): sha(path) for path in dependencies if path.is_file()}
        assert not any(path.resolve().is_relative_to(ROOT/'include') or path.resolve().is_relative_to(ROOT/'src')
                       for path in dependencies), 'Mutable root source/header entered the selected ABI'
        link = [arg for arg in cohort['link_command'] if not arg.startswith('-Wl,-Map,')]
        assert link.count(prior_object) == 1
        link[link.index(prior_object)] = str(obj)
        if leaf_object:
            link.insert(link.index('-o'), str(leaf_object))
        if condition_object:
            link.insert(link.index('-o'),str(condition_object))
        if scalar_object:
            link.insert(link.index('-o'),str(scalar_object))
        if decision_object:
            link.insert(link.index('-o'),str(decision_object))
        if callback_object:
            link.insert(link.index('-o'),str(callback_object))
        if birth_query_object:
            link.insert(link.index('-o'),str(birth_query_object))
        if actor_behavior_object:
            link.insert(link.index('-o'),str(actor_behavior_object))
        if branch_object:
            link.insert(link.index('-o'),str(branch_object))
        if factory_object:
            assert link.count(prior_factory)==1
            link[link.index(prior_factory)]=str(factory_object)
        if child_data_object:
            assert link.count(prior_child_data)==1
            link[link.index(prior_child_data)]=str(child_data_object)
        link[link.index('-o')+1] = str(client)
        run('link-private-coherent-client', link)
        record['runtime_sha256'] = sha(client)
        record['runtime_cases']=[]
        for ordinal,squad in enumerate(squads):
            suffix='-runtime.json' if ordinal==0 else f'-runtime-{ordinal+1}.json'
            child_report=args.report_output.with_name(args.report_output.stem+suffix)
            argv=[sys.executable,str(ROOT/'tools/run_test.py'),'--scratch-root',str(scratch/'runtime-cases'),'--',
                sys.executable,str(ROOT/'tools/test_campaign_ai_runtime.py'),
                '--runtime',str(client),'--mcc-root',str(args.mcc_root.resolve()),'--map',args.map,
                '--squad',squad,'--ticks',str(args.ticks),'--timeout','180','--report-output',str(child_report)]
            # Each case gets a clean process/config/trace lifetime. A failure
            # stays visible while later authored cases expose other owners.
            result=subprocess.run(argv,cwd=ROOT)
            record['runtime_cases'].append(dict(map=args.map,squad=squad,ticks=args.ticks,
                runtime_report=str(child_report),returncode=result.returncode))
            if ordinal==0:
                record['runtime_report']=str(child_report)
                record['runtime_returncode']=result.returncode
            save()
        record['passed']=all(c['returncode']==0 for c in record['runtime_cases'])
        record['all_original_provider_pins_unchanged'] = all(sha(path) == digest
            for path, digest in cohort['link_inputs'].items())
        assert record['all_original_provider_pins_unchanged']
        save()
        return 0 if record['passed'] else 1
    except Exception as error:
        record['error'] = str(error)
        save()
        raise


if __name__ == '__main__':
    raise SystemExit(main())
