#!/usr/bin/env python3
"""Build the production engine and promote an exercised local MCC runtime.

Compiler products and smoke outputs belong to run_test.py. Only a successful
runtime, its resources and small provenance reports leave that scratch owner.
Existing selections and installed runtimes survive failed builds or checks.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import signal
import stat
import subprocess
import sys
import uuid

from source_layout import source_file

ROOT = Path(__file__).resolve().parents[1]

# These are retained reference/component stages, with no admitted production
# caller. Their existence must not be mistaken for delivered functionality.
SEPARATE_COMPONENTS = {
    'src/gameplay/weapons/native_weapon_zoom_audio_tu15.cpp': 'Zoom journal delivery requires its complete multiplayer state/wire cohort; selected-protocol builds may retain this component without admitting that newer route',
    'src/campaign/scripting/source_workshop_unit_controls_tu15_generated.cpp': 'Standalone HS unit-control evaluators; live unit/input scheduling and MCC admission remain open',
    'src/campaign/scripting/source_workshop_camera_start_tu15_generated.cpp': 'Standalone camera HS reference; production wrapper uses the linked shared Workshop module; MCC camera admission remains open',
    'src/campaign/source_workshop_titles_tu15_generated.cpp': 'Standalone title setter/lifecycle component; shared World/presentation scheduling and script admission remain open',
    'src/physics/native_tu15_collision_component.cpp': 'Standalone surface-component resolver; shared/object-kind owners and an admitted production resolver caller remain open',
    'src/physics/native_tu15_collision_surface_shape.cpp': 'Standalone authored surface-shape constructor without an admitted production surface caller',
    'src/physics/native_tu15_contact_point_removal.cpp': 'Standalone contact-point removal/listener composition without an admitted production removal caller',
    'src/physics/native_tu15_contact_query.cpp': 'Standalone typed cached-contact query without an admitted production query caller',
    'src/physics/native_tu15_convex_contact_query.cpp': 'Standalone convex query/result composition without an admitted production query caller',
    'src/physics/native_tu15_island_partition_graph.cpp': 'Standalone partition graph constructor/union/compression without an admitted production graph caller',
    'src/physics/native_tu15_pair_creation_queues.cpp': 'Standalone body-pair merge/deferred queues without an admitted production queue caller',
    'src/physics/native_tu15_phantom_pair_create.cpp': 'Standalone phantom overlap creation without an admitted production pair caller',
    'src/physics/native_tu15_simplex_face_selection.cpp': 'Standalone simplex face selection/refinement without an admitted production simplex caller',
    'src/campaign/ai/native_campaign_ai_speech_visibility_tu15.cpp': 'Standalone typed speech-visibility component without an admitted production speech caller',
    'src/app/launcher.cpp': 'Launcher entry points, separate from the engine',
    'src/app/weapon_launcher.cpp': 'Separate weapons investigation launchers',
    'src/audio/dsp/sound_gp_cpu_tu15.cpp': 'GP instruction core requires its actual platform consumer',
    'src/audio/dsp/sound_gp_cpu_tu15_ffi.c': 'GP instruction-core bridge',
    'src/audio/dsp/sound_gp_dma_tu15.cpp': 'GP DMA requires its actual platform consumer',
    'src/audio/source_sound_configuration_tu15_generated.cpp': 'Sound configuration requires original output/policy admission',
    'src/audio/native_tu15_sound_configuration_data.cpp': 'Sound configuration component data',
    # source_campaign_screen_tu15.hpp and K12 of the native engine checklist
    # name the semantic replacement; linking both duplicates its entry points.
    'src/campaign/source_campaign_screen_tu15_generated.cpp': 'Superseded generated screen reference; production uses semantic lifecycle/timed kernels through source_campaign_screen_native_tu15.cpp',
    'src/core/native_tu15_function_evaluation.cpp': 'Superseded bounded evaluator reference; production contact-point binding uses the linked extended evaluator',
    'src/core/native_tu15_function_power_error.cpp': 'Standalone authored-power CRT error proposal without an admitted production caller',
    'src/core/native_tu15_function_power_special.cpp': 'Standalone infinite-operand authored-power proposal without an admitted production caller',
    'src/physics/native_tu15_motion_pose.cpp': 'Standalone native pose component; actual integrate_pose retains the original production virtual owner',
    'src/gameplay/vehicles/native_tu15_seat_look.cpp': 'Standalone typed seat-look component without an admitted production camera caller',
    'src/gameplay/vehicles/native_tu15_look_direction.cpp': 'Standalone typed look-direction component; production look/camera caller admission remains open',
    'src/physics/native_tu15_attached_camera_position.cpp': 'Unintegrated attached-camera proposal without a caller; the on-foot collision/crouch branch remains unimplemented',
    'src/physics/native_tu15_component_reset.cpp': 'Standalone typed component reset; production retains generated1D8470 and actual lifetime-service/World admission remains open',
    'src/physics/native_tu15_activation_agents.cpp': 'Standalone typed activation-agent composition without an admitted production activation caller',
    'src/physics/native_tu15_cached_motion.cpp': 'Qualified typed cached-motion component; live world activation remains a separate gate',
    'src/physics/native_tu15_compound_process.cpp': 'Standalone typed compound processing without an admitted production collision caller',
    'src/physics/native_tu15_contact_agent_dispatch.cpp': 'Offline typed contact-agent service composition; native factory and live creation coverage remain open',
    'src/physics/native_tu15_contact_agent_factory.cpp': 'Standalone typed contact-agent factory without an admitted production creation caller',
    'src/physics/native_tu15_contact_buffers.cpp': 'Standalone typed contact-buffer ownership without an admitted production contact caller',
    'src/physics/native_tu15_contact_candidate_buffers.cpp': 'Standalone typed candidate-buffer ownership without an admitted production candidate caller',
    'src/physics/native_tu15_contact_candidates.cpp': 'Typed candidate-update source review and live binding remain open',
    'src/physics/native_tu15_contact_producer.cpp': 'Standalone typed contact producer without an admitted production collision caller',
    'src/physics/native_tu15_contact_collector_destructor.cpp': 'Standalone typed3261D0 collector destructor; production allocation/lifetime caller admission remains open',
    'src/physics/native_tu15_contact_creation_gameplay.cpp': 'Standalone bounded creation listeners and bumper; only proposed private producer bindings consume this component',
    'src/physics/native_tu15_contact_records.cpp': 'Standalone typed contact records without an admitted production producer caller',
    'src/physics/native_tu15_conditional_contact_process.cpp': 'Standalone typed2F0F10 conditional parent used by private query compositions; no production caller',
    'src/physics/native_tu15_convex_contact_process.cpp': 'Standalone typed2F8B60 semantic parent; production solver/publication closure remains open',
    'src/physics/native_tu15_moving_shape_process.cpp': 'Standalone typed2E5320 moving-shape process; production resource/wake/depth/World admission remains open',
    'src/physics/native_tu15_reversed_query_process.cpp': 'Standalone typed2F1A50 reversed query wrapper; only private query-parent validators consume it',
    'src/physics/native_tu15_inverse_geometry.cpp': 'Qualified inverse-affine geometry component; native candidate-update integration remains open',
    'src/physics/native_tu15_island_activation.cpp': 'Standalone typed island activation without an admitted production world caller',
    'src/physics/native_tu15_array_allocator.cpp': 'Standalone typed array allocator requires the actual production allocator virtual owners',
    'src/physics/native_tu15_array_growth.cpp': 'Standalone typed array growth requires the actual production allocator virtual owners',
    'src/physics/native_tu15_array_reserve.cpp': 'Standalone typed array reserve requires its actual production allocator virtual owners',
    'src/physics/native_tu15_constraint_step.cpp': 'Standalone typed constraint step requires the actual production solver and commit owners',
    'src/physics/native_tu15_broad_phase.cpp': 'Standalone typed broad phase; production retains actual 0x321700 residual and its virtual service owners',
    'src/physics/native_tu15_constraint_solver.cpp': 'Standalone typed constraint solver pending positive-iteration physical qualification; production retains actual 0x326A40 residual',
    'src/physics/native_tu15_constraint_solver_rows.cpp': 'Standalone typed constraint row kernels pending their admitted solver consumer; production retains actual 0x326A40 residual',
    'src/physics/native_tu15_constraint_custom_schema.cpp': 'Standalone typed custom-schema callbacks require an admitted production solver consumer and physical qualification; production retains its existing custom-schema refusal/residual owner',
    'src/physics/native_tu15_solver_hull_edge.cpp': 'Standalone solver-hull edge selection used by physical validators; no admitted production solver caller',
    'src/physics/native_tu15_solver_hull_line.cpp': 'Standalone solver-hull line extension used by physical validators; no admitted production solver caller',
    'src/physics/native_tu15_solver_hull_polytope.cpp': 'Standalone minimum-facet/direction-sampling components; no admitted production solver caller',
    'src/physics/native_tu15_solver_hull_polytope_state.cpp': 'Standalone polytope-clear/facet-clip components; no admitted production solver caller',
    'src/ui/vista/vista_lobby_leave_return.cpp': 'Original Vista UI reference component without a frontend caller',
    'src/ui/vista/vista_network_browser.cpp': 'Original Vista UI reference component without a frontend caller',
    'src/ui/vista/vista_network_browser_back.cpp': 'Original Vista UI reference component without a frontend caller',
    'src/ui/original_matchmaking_fonts.cpp': 'Offline Xbox font reference control; excluded from the MCC runtime',
    'src/ui/original_matchmaking_emblems.cpp': 'Offline Xbox emblem reference control; excluded from the MCC runtime',
    'src/ui/original_matchmaking_audio.cpp': 'Offline Xbox matchmaking audio reference control; excluded from the MCC runtime',
    'src/ui/original_loading_assets.cpp': 'Offline Xbox loading asset reference control; excluded from the MCC runtime',
    'src/gameplay/multiplayer/multiplayer_locomotion_continuity.cpp': 'Locomotion group is not yet part of the production handoff frontier',
    'src/ui/widgets/native_vista_widget_image.cpp': 'Original Vista PE widget-image component without a production caller',
    'src/ui/widgets/vista_widget_backing.cpp': 'Vista widget heap component requires actual allocation providers',
    'src/ui/widgets/vista_widget_font_ambient.cpp': 'Vista ambient font environment component without a production caller',
    'src/rendering/world/source_sky_stencil.cpp': 'Frozen sky-stencil reference prototype requires actual production draw inputs',
    'src/rendering/world/source_decorator_decals_tu15.cpp': 'Decorator asset preparation requires an admitted production BSP scene consumer',
    'src/rendering/world/source_decorator_owners_tu15_generated.cpp': 'Decorator native owners require an admitted production BSP scene consumer',
    'src/rendering/effects/source_decorator_decal_draw.cpp': 'Decorator draw component requires the actual production BSP command bank',
    'src/rendering/effects/source_decorator_decal_material.cpp': 'Decorator material component requires the admitted production draw consumer',
    'src/rendering/loading/source_loading_program_tu15_generated.cpp': 'TU1.5 loading compositor component without a production entry/service caller',
    'src/ui/source_loading_assets.cpp': 'MCC loading bitmap adapter without a production loading-scene caller',
    'src/ui/menu_world_vertex_source.cpp': 'Offline native vertex code generation; production uses immutable kernel and metadata catalogs',
}


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def tu15_gameplay_reference(explicit=None):
    """Bind the owned English TU1.5 image used by original-data CTests."""
    explicit = explicit or os.environ.get('HALO2_TU15_GAMEPLAY_XBE')
    if explicit:
        candidates = [Path(explicit)]
    else:
        data = Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share')
        candidates = sorted((data / 'halo2-native/imports').glob(
            'halo2-dlc-*/Halo 2 DLC/content/u5English/default.xbe'))
    for candidate in candidates:
        path = candidate.expanduser().resolve(strict=True)
        if path.is_file() and 0 < path.stat().st_size <= 64 * 1024 * 1024:
            pin = digest(path)
            if pin == '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':
                return dict(path=str(path), bytes=path.stat().st_size, sha256=pin)
        if explicit:
            raise RuntimeError('Original-data qualification requires the owned English TU1.5 gameplay image')
    return None


TOOL_SOURCE_SUFFIXES = frozenset({'.py','.pyi','.sh','.bash','.bat','.c','.cc','.cpp','.cxx',
    '.h','.hpp','.hxx','.inl','.inc','.s','.S','.cmake','.json','.jsonl','.tsv','.csv',
    '.txt','.md','.rst','.yaml','.yml','.toml','.glsl','.vert','.frag','.geom','.comp',
    '.fx','.cs','.java','.xml','.ini','.config','.proto','.def','.idl','.rc','.tt','.ttinclude','.zlib'})

GRENADE_SHADOW_CASES = {
    'neutral': ('coagulation.map', None),
    'frag': ('coagulation.map', 0),
    'plasma': ('midship.map', 1),
    'receiver': ('zanzibar.map', None),
}
SMG_WARTHOG_INPUT = Path('tests/fixtures/smg_warthog_loop_audio_inputs.tsv')
SMG_WARTHOG_METADATA = SMG_WARTHOG_INPUT.with_suffix('.json')
SMG_WARTHOG_SHARED = ('mainmenu.map', 'shared.map', 'single_player_shared.map')


def smg_warthog_reference(source, maps):
    """Pin the actual qualified input tape borrowed by this frozen build."""
    script, metadata_path = source / SMG_WARTHOG_INPUT, source / SMG_WARTHOG_METADATA
    if (script.is_symlink() or not script.is_file() or not 0 < script.stat().st_size <= 1024 * 1024 or
            metadata_path.is_symlink() or not metadata_path.is_file() or
            not 0 < metadata_path.stat().st_size <= 65536):
        raise RuntimeError('SMG Warthog audio reference lacks its bounded qualified input tape and metadata')
    try:
        metadata = json.loads(metadata_path.read_text())
    except (ValueError, OSError) as error:
        raise RuntimeError('SMG Warthog audio reference has malformed metadata') from error
    script_pin, map_pin = digest(script), digest(maps / 'coagulation.map')
    if (not isinstance(metadata, dict) or type(metadata.get('capture_tick')) is not int or
            not 600 <= metadata['capture_tick'] <= 1800 or
            metadata.get('input_script_sha256') != script_pin or metadata.get('map_sha256') != map_pin):
        raise RuntimeError('SMG Warthog audio reference does not authenticate its tape, map and capture tick')
    return dict(input_script=str(script.resolve()), input_script_sha256=script_pin,
        reference_metadata_path=str(metadata_path.resolve()), reference_metadata=metadata,
        reference_metadata_sha256=digest(metadata_path), capture_tick=metadata['capture_tick'],
        map_sha256=map_pin)


def smg_warthog_audio_qualification(build, source, maps, output, executable_pin, reference):
    """Require positive native17E160 execution from the exact candidate ELF."""
    prefix = 'SMG Warthog audio qualification'
    path = output / 'validation/evidence/smg-warthog-audio.json'
    log = path.with_suffix('.log')
    if (path.is_symlink() or not path.is_file() or not 0 < path.stat().st_size <= 524288 or
            log.is_symlink() or not log.is_file() or not 0 < log.stat().st_size <= 131072):
        raise RuntimeError(prefix + ' lacks its bounded native report and log')
    try:
        data = json.loads(path.read_text())
    except (ValueError, OSError) as error:
        raise RuntimeError(prefix + ' has a malformed native report') from error
    if not isinstance(data, dict) or any(not isinstance(data.get(key), dict) for key in
            ('native', 'retained_log', 'input_projection', 'input_schedule', 'capture', 'shadow_draws')):
        raise RuntimeError(prefix + ' has an incomplete native report')
    positive = lambda value, minimum=1: type(value) is int and value >= minimum
    salted = lambda value: type(value) is int and 0 < value < 0xffffffff and value >> 16 != 0
    hashed = lambda value: isinstance(value, str) and bool(re.fullmatch('[0-9a-f]{64}', value))
    tick = reference['capture_tick']
    native, retained = data['native'], data['retained_log']
    valid = (data.get('schema') == 'HALO2_NATIVE_SMG_WARTHOG_AUDIO_REPLAY_V1' and
        data.get('passed') is True and data.get('expect') == 'complete' and type(data.get('exit_code')) is int and data['exit_code'] == 0 and
        data.get('runtime') == str((build / 'halo2_native').resolve()) and data.get('runtime_sha256') == executable_pin and
        data.get('map') == str((maps / 'coagulation.map').resolve()) and data.get('map_sha256') == reference['map_sha256'] and
        data.get('shared_maps') == str(maps.resolve()) and data.get('input_script') == reference['input_script'] and
        data.get('input_script_sha256') == reference['input_script_sha256'] and
        data.get('reference_metadata_path') == reference['reference_metadata_path'] and
        data.get('reference_metadata_sha256') == reference['reference_metadata_sha256'] and
        data.get('reference_metadata') == reference['reference_metadata'] and
        data.get('capture_tick') == tick and data.get('ticks') == tick and type(data.get('players')) is int and data['players'] == 1 and
        data.get('viewport') == [640, 480] and data.get('graphics_mode') == 'Original' and
        data.get('runtime_cache_source') == 'MCC' and data.get('mute') is False and data.get('audio_driver') == 'dummy' and
        data.get('scratch_removed') is True and data.get('inputs_unchanged') is True and
        data.get('exact_audio_owner_error') is False and data.get('unknown_option_error') is False and
        data.get('native_error_diagnostic') is False and positive(data.get('frames')) and
        type(data.get('capture_frame_count')) is int and data['capture_frame_count'] == 1 and data.get('retained_large_artifacts') == [] and
        retained.get('bytes') == log.stat().st_size and retained.get('sha256') == digest(log) and
        type(retained.get('tail_truncated')) is bool and
        data['input_projection'].get('legacy_cli_projection_omits_accept') is True)
    schedule = data['input_schedule']
    valid = valid and (positive(schedule.get('rows'), 2) and schedule['rows'] <= tick and
        schedule.get('first_key') == 0 and schedule.get('last_key') == schedule['rows'] - 1 and
        schedule.get('last_key') < tick and schedule.get('contiguous') is True and
        type(schedule.get('first_fire_key')) is int and 0 < schedule['first_fire_key'] < schedule['rows'] and
        positive(schedule.get('fire_rows'), 61) and positive(schedule.get('reload_rows')))
    expected_inputs = [build / 'halo2_native', source / SMG_WARTHOG_INPUT, maps / 'coagulation.map',
        *(maps / name for name in SMG_WARTHOG_SHARED), source / SMG_WARTHOG_METADATA]
    expected_pins = {str(item.resolve()): dict(path=str(item.resolve()), bytes=item.stat().st_size,
        sha256=digest(item)) for item in expected_inputs}
    before, after = data.get('input_pins_before'), data.get('input_pins_after')
    valid = valid and (isinstance(before, list) and before == after and len(before) == len(expected_pins) and
        all(isinstance(item, dict) and isinstance(item.get('path'), str) and
            item == expected_pins.get(item['path']) for item in before) and
        len({item['path'] for item in before}) == len(expected_pins))
    player, mode, audio, target = (native.get(key) for key in ('player', 'mode3', 'audio', 'target'))
    if not all(isinstance(item, dict) for item in (player, mode, audio, target)):
        raise RuntimeError(prefix + ' lacks native player, target, mode3 or audio evidence')
    valid = valid and (native.get('schema') == 'HALO2_NATIVE_LOOP_AUDIO_LOCATION_V1' and
        native.get('collector_schema') == 'HALO2_SOURCE_LOOP_AUDIO_LOCATION_TU15_V1' and
        native.get('passed') is True and native.get('qualified') is True and type(native.get('run_result')) is int and native['run_result'] == 0 and
        native.get('map') == str((maps / 'coagulation.map').resolve()) and native.get('graphics_mode') == 'Original' and
        native.get('runtime_cache_source') == 'MCC' and native.get('mute') is False and
        native.get('requested_capture_tick') == tick and native.get('final_tick') == tick and native.get('captured_tick') is True and
        all(positive(native.get(key)) for key in ('input_samples', 'submitted_frames', 'shadow_generate_draws', 'shadow_apply_draws')) and
        positive(native.get('native_projectile_publications'), 20) and
        all(salted(player.get(key)) for key in ('source_player', 'source_unit', 'source_weapon', 'weapon_definition', 'expected_smg_definition')) and
        player.get('weapon_definition') == player.get('expected_smg_definition') and
        positive(player.get('rounds_consumed'), 61) and positive(player.get('shot_transitions')) and
        all(positive(player.get(key)) for key in ('reload_commands', 'reload_transfers', 'reload_rounds')) and
        all(type(player.get(key)) is int and player[key] == 0 for key in
            ('identity_changes', 'post_pin_non_smg_inputs', 'input_errors', 'input_read_failures')) and
        all(positive(audio.get(key)) for key in ('packets', 'smg_packets', 'reload_packets')) and
        type(audio.get('invalid_packets')) is int and audio['invalid_packets'] == 0 and
        mode.get('observer_bound') is True and mode.get('baseline_recorded') is True and mode.get('baseline_frozen') is True and
        positive(mode.get('entries')) and positive(mode.get('returns')) and mode['entries'] == mode['returns'] and
        all(positive(mode.get(key)) for key in ('validated_entries', 'successful_returns',
            'post_fire_successful_returns', 'target_owned_post_fire_successful_returns')) and
        salted(target.get('owner')) and salted(target.get('definition')) and target.get('precondition_observed') is True and
        target.get('smg_damage_observed') is True and
        all(type(target.get(key)) in (int, float) and math.isfinite(target[key])
            for key in ('body_before_smg', 'minimum_body_after_smg')) and
        target['body_before_smg'] > target['minimum_body_after_smg'] >= 0 and
        positive(target.get('post_smg_vitality_samples'), 2) and
        all(type(target.get(key)) is int for key in ('first_smg_command_tick', 'first_smg_damage_tick')) and
        type(mode.get('first_shot_tick')) is int and
        0 < target['first_smg_command_tick'] <= target['first_smg_damage_tick'] < tick and
        mode['first_shot_tick'] <= target['first_smg_damage_tick'] and
        all(type(mode.get(key)) is int and mode[key] >= 0 for key in
            ('new_generation_successful_returns', 'preexisting_target_owned_successful_returns')) and
        positive(mode.get('post_fire_successful_ticks'), 2) and
        all(type(mode.get(key)) is int and mode[key] == 0 for key in
            ('invalid_calls', 'read_failures', 'unfinished_calls', 'late_baseline_inputs')))
    if valid:
        valid = (mode['new_generation_successful_returns'] + mode['preexisting_target_owned_successful_returns'] ==
            mode['target_owned_post_fire_successful_returns'] and
            mode['post_fire_successful_ticks'] <= mode['target_owned_post_fire_successful_returns'] <=
            mode['post_fire_successful_returns'] <= mode['successful_returns'] <= mode['validated_entries'] <= mode['entries'])
        if mode['preexisting_target_owned_successful_returns']:
            valid = valid and (target.get('precondition_observed') is True and
                positive(mode.get('precondition_target_owned_successful_returns')) and
                mode.get('precondition_generation_seen') is True and positive(mode.get('precondition_generation_count')))
        if target.get('precondition_observed') is True:
            precondition = native.get('precondition')
            grenade_keys = schedule.get('grenade_keys')
            valid = valid and (isinstance(precondition, dict) and precondition.get('observed') is True and
                type(precondition.get('grenade_commands')) is int and precondition['grenade_commands'] == 1 and
                type(schedule.get('grenade_rows')) is int and schedule['grenade_rows'] == 1 and
                isinstance(grenade_keys, list) and len(grenade_keys) == 1 and type(grenade_keys[0]) is int and
                0 <= grenade_keys[0] < schedule['first_fire_key'] and
                all(type(precondition.get(key)) in (int, float) and math.isfinite(precondition[key])
                    for key in ('body_before', 'body_after')) and
                precondition['body_before'] > precondition['body_after'] > 0)
            if valid:
                births, detonations = precondition.get('grenade_projectiles'), precondition.get('grenade_detonations')
                valid = (isinstance(births, list) and isinstance(detonations, list) and
                    0 < len(births) <= 64 and 0 < len(detonations) <= 64)
                if valid:
                    actual, released = set(), False
                    for event in births:
                        if not (isinstance(event, dict) and salted(event.get('projectile')) and salted(event.get('definition')) and
                            type(event.get('type')) is int and event['type'] == 0 and type(event.get('released')) is bool and
                            event.get('player') == player['source_player'] and event.get('unit') == player['source_unit']):
                            valid = False
                            break
                        actual.add((event['projectile'], event['definition']))
                        released |= event['released']
                    valid = valid and len(actual) == 1 and released
                    for event in detonations:
                        if not (isinstance(event, dict) and salted(event.get('projectile')) and salted(event.get('definition')) and
                            (event['projectile'], event['definition']) in actual and
                            event.get('player') == player['source_player'] and event.get('unit') == player['source_unit'] and
                            positive(event.get('serial')) and type(event.get('tick')) is int and 0 <= event['tick'] < tick and
                            type(mode.get('first_shot_tick')) is int and event['tick'] < mode['first_shot_tick']):
                            valid = False
                            break
    first, last, shot = (mode.get(key) for key in
        ('first_success_source_tick', 'last_success_source_tick', 'first_shot_source_tick'))
    valid = valid and all(type(value) is int for value in (first, last, shot)) and 0 <= shot <= first < last <= tick
    samples = mode.get('samples')
    valid = valid and isinstance(samples, list) and 0 < len(samples) <= 64
    if valid:
        for sample in samples:
            bits = sample.get('output_bits') if isinstance(sample, dict) else None
            if not (isinstance(sample, dict) and sample.get('caller_return') == 0x18fa4a and
                all(salted(sample.get(key)) for key in ('effect', 'loop', 'effect_definition', 'loop_definition',
                    'effect_object_owner', 'effect_object_definition')) and
                sample['effect_object_owner'] == target['owner'] and sample['effect_object_definition'] == target['definition'] and
                type(sample.get('new_generation')) is bool and type(sample.get('precondition_generation_seen')) is bool and
                (sample['new_generation'] or sample['precondition_generation_seen']) and
                (not sample['new_generation'] or positive(mode['new_generation_successful_returns'])) and
                (sample['new_generation'] or positive(mode['preexisting_target_owned_successful_returns'])) and
                type(sample.get('tick')) is int and first <= sample['tick'] <= last and
                type(sample.get('eax')) is int and 0 <= sample['eax'] <= 0xffffffff and sample['eax'] & 255 and
                isinstance(bits, list) and len(bits) == 7 and all(type(value) is int and
                    0 <= value <= 0xffffffff and value & 0x7f800000 != 0x7f800000 for value in bits)):
                valid = False
                break
            if sample['precondition_generation_seen'] and not (
                target.get('precondition_observed') is True and mode.get('precondition_generation_seen') is True and
                positive(mode.get('precondition_target_owned_successful_returns')) and positive(mode.get('precondition_generation_count')) and
                type(sample.get('precondition_first_seen_source_tick')) is int and
                0 <= sample['precondition_first_seen_source_tick'] < shot):
                valid = False
                break
        valid = valid and len({sample['tick'] for sample in samples}) >= 2
    capture = data['capture']
    valid = valid and (capture.get('width') == 640 and capture.get('height') == 480 and
        capture.get('bytes') == 640 * 480 * 3 + len(b'P6\n640 480\n255\n') and hashed(capture.get('sha256')) and
        all(positive(data['shadow_draws'].get(key)) for key in
            ('world_native_shadow_generate_prepared_draws', 'world_native_shadow_apply_prepared_draws')))
    if not valid:
        raise RuntimeError(prefix + ' did not complete its exact candidate, SMG, mode3, audio and rendered capture evidence')
    return dict(passed=True, report=str(path), report_sha256=digest(path), runtime_sha256=executable_pin,
        log=str(log), log_sha256=digest(log), map_sha256=reference['map_sha256'],
        input_script_sha256=reference['input_script_sha256'], reference_metadata_sha256=reference['reference_metadata_sha256'],
        capture_tick=tick, frames=data['frames'], native_projectile_publications=native['native_projectile_publications'],
        new_generation_successful_returns=mode['new_generation_successful_returns'],
        target_owned_post_fire_successful_returns=mode['target_owned_post_fire_successful_returns'],
        preexisting_target_owned_successful_returns=mode['preexisting_target_owned_successful_returns'],
        target_owner=target['owner'], target_definition=target['definition'],
        post_fire_successful_ticks=mode['post_fire_successful_ticks'], rounds_consumed=player['rounds_consumed'],
        reload_transfers=player['reload_transfers'],
        scope='Exact candidate ELF replays retained ordinary SMG inputs through actual mode3 audio owners and rendered capture; '
              'the legacy projection omits accept and does not assert complete Controls or Warthog damage identity')


def grenade_shadow_qualification(build, maps, output, executable_pin, map_pins):
    """Require completed native grenade/shadow cases from this exact executable."""
    cases = []
    for kind, (map_name, grenade_type) in GRENADE_SHADOW_CASES.items():
        path = output / 'validation/evidence' / f'grenade-shadow-{kind}.json'
        if path.is_symlink() or not path.is_file() or not 0 < path.stat().st_size <= 1024 * 1024:
            raise RuntimeError(f'Grenade shadow qualification lacks a bounded {kind} report')
        data = json.loads(path.read_text())
        if not isinstance(data, dict) or any(not isinstance(data.get(field), dict)
                for field in ('shadow_draws', 'checks', 'capture', 'retained_log')):
            raise RuntimeError(f'Grenade shadow qualification has a malformed {kind} report')
        draws = data.get('shadow_draws', {})
        ticks = data.get('grenade_ticks', [])
        owners = data.get('grenade_owners', [])
        definitions = data.get('grenade_definitions', [])
        retained_log = path.with_suffix('.log')
        log_receipt = data['retained_log']
        valid = (data.get('passed') is True and data.get('case') == kind and
            data.get('runtime') == str((build / 'halo2_native').resolve()) and
            data.get('runtime_sha256') == executable_pin and
            data.get('map') == str((maps / map_name).resolve()) and
            data.get('map_sha256') == map_pins[map_name] and
            data.get('graphics_mode') == 'Original' and data.get('viewport') == [1280, 720] and
            data.get('capture_tick') == 160 and data.get('exit_code') == 0 and
            type(data.get('frames')) is int and data['frames'] > 0 and
            type(data.get('ticks')) is int and data['ticks'] > 0 and data.get('players') == 1 and
            data.get('exact_missing_section_error') is False and data.get('scratch_removed') is True and
            data.get('runtime_and_map_unchanged') is True and
            isinstance(ticks, list) and isinstance(owners, list) and isinstance(definitions, list) and
            type(data.get('native_grenade_admitted_observations')) is int and
            data['native_grenade_admitted_observations'] == data.get('native_grenade_observations') and
            not retained_log.is_symlink() and retained_log.is_file() and
            0 < retained_log.stat().st_size <= 4 * 65536 and
            log_receipt.get('bytes') == retained_log.stat().st_size and
            log_receipt.get('sha256') == digest(retained_log) and
            all(data.get('checks', {}).get(check) is True for check in
                ('exact_tick_and_complete_surface', 'actual_shadow_gpu_submissions',
                 'native_grenade_lifecycle_or_neutral_control', 'no_missing_shadow_section')) and
            all(type(draws.get(field)) is int and draws[field] > 0 for field in
                ('world_native_shadow_generate_prepared_draws', 'world_native_shadow_apply_prepared_draws')))
        if grenade_type is None:
            valid = valid and type(data.get('native_grenade_observations')) is int and \
                data['native_grenade_observations'] == 0 and not ticks and not owners and not definitions and \
                data.get('grenade_types') == [] and data.get('detonation_serial') == 0
        else:
            valid = valid and type(data.get('native_grenade_observations')) is int and \
                data['native_grenade_observations'] >= 2 and data.get('grenade_types') == [grenade_type] and \
                type(data.get('detonation_serial')) is int and data['detonation_serial'] > 0 and \
                len(ticks) >= 2 and all(type(tick) is int and 20 <= tick <= 160 for tick in ticks) and \
                ticks == sorted(set(ticks)) and \
                isinstance(owners, list) and bool(owners) and all(type(owner) is int and
                    0 < owner < 0xffffffff and owner >> 16 for owner in owners) and \
                bool(definitions) and all(type(definition) is int and
                    0 < definition < 0xffffffff and definition >> 16 for definition in definitions)
        capture = data.get('capture', {})
        valid = valid and capture.get('bytes') == 1280 * 720 * 3 + len(b'P6\n1280 720\n255\n') and \
            isinstance(capture.get('sha256'), str) and bool(re.fullmatch('[0-9a-f]{64}', capture['sha256']))
        if not valid:
            raise RuntimeError(f'Grenade shadow qualification did not complete the actual {kind} native case')
        cases.append(dict(case=kind, passed=True, map=map_name, map_sha256=map_pins[map_name],
            report=str(path), report_sha256=digest(path), runtime_sha256=executable_pin,
            frames=data['frames'], ticks=data['ticks'],
            native_grenade_observations=data['native_grenade_observations'],
            native_grenade_admitted_observations=data['native_grenade_admitted_observations'],
            log=str(retained_log), log_sha256=digest(retained_log),
            detonation_serial=data['detonation_serial']))
    return dict(passed=True, cases=cases,
        scope='Actual neutral/receiver controls, frag/plasma native lifecycles, and ordinary shadow GPU submission')


def friends_footer_qualification(output, source_cohort):
    """Require every footer fixture case from the frozen qualification build."""
    path = output / 'validation/evidence/friends-footer-anchor.json'
    if path.is_symlink() or not path.is_file() or not 0 < path.stat().st_size <= 1024 * 1024:
        raise RuntimeError('Friends footer qualification lacks a bounded report')
    data = json.loads(path.read_text())
    if (not isinstance(data, dict) or data.get('passed') is not True or
            not isinstance(data.get('cases'), list) or len(data['cases']) != 39 or
            not all(isinstance(case, dict) and case.get('passed') is True for case in data['cases']) or
            type(data.get('rendered_cases')) is not int or data['rendered_cases'] not in (0, 13)):
        raise RuntimeError('Friends footer qualification requires all 39 passing fixture cases')
    return dict(passed=True, report=str(path), report_sha256=digest(path), cases=39,
        rendered_cases=data['rendered_cases'], source_inventory_sha256=source_cohort['inventory_sha256'],
        scope='MCC Friends footer layout and pointer fixture in the frozen source/build cohort; '
              'rendered_cases=0 qualifies CPU layout and pointer behavior')


def loop_audio_location_qualification(output, source_cohort):
    """Require actual source-owner replay, including passive observer parity."""
    path = output / 'validation/evidence/loop-audio-locations.json'
    if path.is_symlink() or not path.is_file() or not 0 < path.stat().st_size <= 1024 * 1024:
        raise RuntimeError('Loop audio location qualification lacks a bounded report')
    data = json.loads(path.read_text())
    if (not isinstance(data, dict) or data.get('passed') is not True or
            any(type(data.get(key)) is not int or data[key] != expected for key, expected in
                (('cases', 22), ('comparisons', 44), ('mode3_cases', 6), ('mode2_cases', 16))) or
            data.get('original_xbe_sha256') != '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'):
        raise RuntimeError('Loop audio location qualification requires all 22 original-call cases with observer parity')
    return dict(passed=True, report=str(path), report_sha256=digest(path), cases=22,
        comparisons=44, mode3_cases=6, mode2_cases=16,
        source_inventory_sha256=source_cohort['inventory_sha256'],
        scope='Complete mode2/mode3 source owners in the frozen build; exact original traces, writes and hardware ABI')


def tool_source_path(path):
    """Generator code and authored inputs, excluding caches/build/editor outputs."""
    relative = Path(path)
    if any(part in {'__pycache__','target','build','CMakeFiles','node_modules','tmp','temp'} or
           part.startswith(('.', 'cmake-build-')) for part in relative.parts[:-1]):
        return False
    return (relative.suffix in TOOL_SOURCE_SUFFIXES and not relative.name.startswith('.') and
            not re.search(r'(?:^|[.-])(?:tmp|temp|bak|orig|rej)(?:[.-]|$)', relative.name))


def source_hashes(root):
    files = [root / 'CMakeLists.txt']
    for name in ('src', 'include', 'cmake', 'shaders', 'data', 'rust'):
        files.extend(p for p in (root / name).rglob('*') if p.is_file()
                     and 'target' not in p.relative_to(root / name).parts)
    files.extend(p for p in (root / 'tools').rglob('*') if p.is_file() and
                 tool_source_path(p.relative_to(root/'tools')))
    files.extend(p for p in (root / 'multiplayer/master_server').rglob('*') if p.is_file() and
                 tool_source_path(p.relative_to(root/'multiplayer/master_server')))
    return {p.relative_to(root).as_posix(): digest(p) for p in sorted(files)}


def freeze_source(root, destination):
    """Copy code once; borrow existing asset/reference trees without changing them."""
    before = source_hashes(root)
    destination.mkdir()
    for name in ('src', 'include', 'cmake', 'shaders', 'data', 'tools', 'third_party', 'external', 'rust'):
        if (root / name).is_dir():
            shutil.copytree(root / name, destination / name,
                            ignore=shutil.ignore_patterns('__pycache__', '.git', 'target'))
    for path in root.iterdir():
        if path.is_file() and path.suffix in ('.txt', '.cmake', '.py', '.sh'):
            shutil.copy2(path, destination / path.name)
    tests = destination / 'tests'
    tests.mkdir()
    for path in (root / 'tests').iterdir():
        if path.is_file():
            shutil.copy2(path, tests / path.name)
        elif path.is_dir() and path.name != '__pycache__':
            (tests / path.name).symlink_to(path, target_is_directory=True)
    # Native ONLINE validation executes this Python service with the frozen
    # Rust coordinator. Borrowing its code could mix two protocol revisions.
    multiplayer = root / 'multiplayer'
    if multiplayer.is_dir():
        (destination / 'multiplayer').mkdir()
        for path in multiplayer.iterdir():
            if path.name == 'master_server' and path.is_dir():
                shutil.copytree(path, destination / 'multiplayer/master_server',
                    ignore=shutil.ignore_patterns('__pycache__', '.pytest_cache', '.git', 'target'))
            elif path.is_file():
                shutil.copy2(path, destination / 'multiplayer' / path.name)
            elif path.is_dir() and path.name != '__pycache__':
                (destination / 'multiplayer' / path.name).symlink_to(path, target_is_directory=True)
    for name in ('assets', 'resources', 'analysis', '.tools', 'Halo 2 Vista installer'):
        if (root / name).exists():
            (destination / name).symlink_to(root / name, target_is_directory=True)
    if source_hashes(destination) != before or source_hashes(root) != before:
        raise RuntimeError('Engine source changed while freezing the build; retry the current revision')
    return before


def source_status(root, runtime):
    path = runtime.parent / 'source-hashes.json'
    if not path.is_file():
        return 'Engine source: no source inventory for the selected runtime'
    recorded = {n: h for n, h in json.loads(path.read_text()).items()
                if n == 'CMakeLists.txt' or n.startswith(('src/', 'include/', 'cmake/', 'shaders/', 'data/', 'rust/')) or
                   (n.startswith('tools/') and tool_source_path(n.removeprefix('tools/')))}
    current = source_hashes(root)
    legacy = not any(n.startswith('tools/') for n in recorded) and any(n.startswith('tools/') for n in current)
    if legacy:
        # This is a diagnostic only. Keep qualified older runtimes usable,
        # while avoiding a false claim that their generator provenance matches.
        current = {n:h for n,h in current.items() if not n.startswith('tools/')}
    changed = sum(recorded.get(n) != current.get(n) for n in recorded.keys() | current.keys())
    status = ('Engine source: matches the selected runtime' if not changed else
              f'Engine source: {changed} files differ from the selected runtime; use --build-engine')
    return status+'; selected runtime lacks generator source provenance' if legacy else status


def wiring(graph, root):
    root = root.resolve()
    targets = {}
    for line in graph.read_text().splitlines():
        name, links, dependencies, sources = line.split('|')
        normalized = []
        for source in sources.split(';'):
            path = Path(source)
            if path.is_absolute():
                # CMAKE_CURRENT_LIST_DIR commonly contributes /cmake/../src.
                # Compare the actual source path with the cohort root before
                # classifying it; lexical containment can also admit ../ exits.
                path = path.resolve()
                if path.is_relative_to(root):
                    source = path.relative_to(root).as_posix()
            normalized.append(source)
            # NativeTranslationUnits.cmake compiles unchanged body partitions
            # rather than the oversized authenticated source file itself.
            partition = re.search(r'/native-translations/([^/]+)/[^/]+\.cpp$', source)
            if partition:
                try:
                    original = source_file(root, partition[1] + '.cpp')
                except FileNotFoundError:
                    pass
                else:
                    normalized.append(original.relative_to(root).as_posix())
            specialized = re.search(r'/native-operands/(.+\.cpp)$', source)
            if specialized and (root / 'src' / specialized[1]).is_file():
                normalized.append('src/' + specialized[1])
        targets[name] = (links.split(';'), dependencies.split(';'), normalized)
    closure, pending = set(), ['halo2_native']
    while pending:
        name = pending.pop()
        if name not in targets or name in closure:
            continue
        closure.add(name)
        pending.extend(p.removeprefix('$<LINK_ONLY:').removesuffix('>') for p in targets[name][0])
        # target_sources can archive an OBJECT library without listing it in
        # LINK_LIBRARIES. Follow only an exact ownership expression from a
        # reachable target; build-only dependencies and conditional expressions
        # do not establish that an object's code reached the engine.
        for source in targets[name][2]:
            objects = re.fullmatch(r'\$<TARGET_OBJECTS:([^<>;]+)>', source)
            if objects and objects[1] in targets:
                pending.append(objects[1])
    linked = {s for t in closure for s in targets[t][2] if s.startswith('src/')}
    isolated = {}
    for name, (_, _, sources) in targets.items():
        for source in sources:
            if source.startswith('src/') and source.endswith(('.cpp', '.c')) and source not in linked:
                isolated.setdefault(source, []).append(name)
    registered = {s for row in targets.values() for s in row[2]}
    platform_sources = sorted(p.relative_to(root).as_posix() for p in (root / 'src/macos').rglob('*.cpp'))
    unregistered = sorted(p.relative_to(root).as_posix() for p in (root / 'src').rglob('*.cpp')
                          if p.relative_to(root).as_posix() not in platform_sources
                          if p.relative_to(root).as_posix() not in registered)
    boundaries = dict(SEPARATE_COMPONENTS)
    # The standalone camera wrapper is a reference for the shared runtime
    # body. Its boundary must not hide loss of that actual production owner.
    if 'src/campaign/scripting/source_workshop_script_builtins_tu15_generated.cpp' not in linked:
        boundaries.pop('src/campaign/scripting/source_workshop_camera_start_tu15_generated.cpp', None)
    # The ordinary checkout admits zoom delivery in halo2_systems. Only the
    # deliberately preserved older state/wire cohort may isolate that file;
    # an accidental missing production link must still fail the source audit.
    zoom = 'src/gameplay/weapons/native_weapon_zoom_audio_tu15.cpp'
    zoom_policy = root / 'cmake/NativeWeaponEffectsTu15.cmake'
    zoom_text = zoom_policy.read_text() if zoom_policy.is_file() else ''
    if ('# This selected-protocol cohort retains the later zoom journal delivery as' not in zoom_text or
            'target_sources(halo2_systems PRIVATE ' + zoom + ')' in zoom_text):
        boundaries.pop(zoom, None)
    missing = sorted((set(isolated) | set(unregistered)) - boundaries.keys())
    return dict(passed=not missing, engine_targets=sorted(closure), engine_sources=sorted(linked),
                separate_components=isolated,
                other_platform_sources=platform_sources,
                unregistered_sources=unregistered, unexpected_unwired_sources=missing,
                component_boundaries={s: boundaries[s] for s in sorted(set(isolated) | set(unregistered))
                                      if s in boundaries},
                scope='Transitive library links and direct object-source ownership; separate components require real callers before runtime acceptance')


def runtime_environment(scratch):
    # Qualification exercises the executable's production defaults. Diagnostic,
    # feature and artifact-retention overrides from a developer's shell must
    # not silently change either role. Callers add only their explicit owned
    # paths and declared controls; unrelated SDK/display settings remain.
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith('HALO2_')}
    environment['HALO2_TEST_SCRATCH'] = str(scratch)
    return environment


def qualification_maps(prior, additional):
    names = set(prior['maps'] if prior else ['lockout.map'])
    for name in additional:
        if not isinstance(name, str) or not re.fullmatch(r'[a-z0-9_]+\.map', name):
            raise ValueError('Additional qualification maps must be lowercase cache basenames')
        names.add(name)
    return sorted(names)


def smoke(runtime, maps, name, players, scratch, timeout, root=ROOT, video_driver=''):
    log = scratch / f'{Path(name).stem}-{players}.log'
    preferences = scratch / 'smoke-preferences' / uuid.uuid4().hex
    command = [str(runtime), '--multiplayer', '--graphics-mode', 'Original', '--map', str(maps / name),
               '--shared-maps', str(maps), '--local-players', str(players),
               '--frames', '4', '--mute', '--viewport', '640', '480']
    # Exercise ordinary SDL/Vulkan frame submission, with no Xbox input,
    # synthetic runtime backing, or imported UI overrides.
    environment = runtime_environment(scratch)
    environment.update(HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch / 'vulkan-cache'),
                       MESA_SHADER_CACHE_DIR=str(scratch / 'mesa-cache'),
                       __GL_SHADER_DISK_CACHE_PATH=str(scratch / 'nvidia-cache'),
                       XDG_DATA_HOME=str(preferences / 'data'),
                       XDG_CONFIG_HOME=str(preferences / 'config'),
                       XDG_CACHE_HOME=str(preferences / 'cache'))
    if video_driver:
        environment.setdefault('SDL_VIDEODRIVER', video_driver)
    with log.open('wb') as stream:
        try:
            result = subprocess.run(command, cwd=root, stdout=stream, stderr=subprocess.STDOUT,
                                    timeout=timeout, env=environment)
            status = result.returncode
        except subprocess.TimeoutExpired:
            status = 'timeout'
    tail = log.read_bytes()[-12000:].decode(errors='replace')
    submitted = re.search(r'Multiplayer frames=(\d+) ticks=(\d+) players=(\d+) connected=(\d+) peak_players=(\d+)', tail)
    return dict(map=name, local_players=players, command=command, exit_code=status,
                passed=status == 0 and submitted is not None and int(submitted[1]) == 4 and
                    int(submitted[2]) > 0 and int(submitted[3]) == players and int(submitted[4]) == 1 and
                    int(submitted[5]) == players,
                submitted_frames=int(submitted[1]) if submitted else 0,
                simulation_ticks=int(submitted[2]) if submitted else 0,
                session_joined=bool(int(submitted[4])) if submitted else False,
                peak_players=int(submitted[5]) if submitted else 0, log_tail=tail)


RENDERER_4K_CASES = ('4:90:0:0', '4:150:10:110', '1:150:10:110', '2:150:10:110')


def renderer_reference_controls(assignments):
    controls = {}
    for assignment in assignments:
        name, separator, value = assignment.partition('=')
        if (not separator or not re.fullmatch(r'HALO2_REFERENCE_[A-Z0-9_]+', name) or
                not value or any(c in value for c in ',@\r\n\0') or name in controls):
            raise ValueError('Renderer 4K reference controls require unique HALO2_REFERENCE_NAME=VALUE assignments without comma/@/line breaks')
        controls[name] = value
    return controls


def renderer_4k_qualification(source, runtime, maps, output, scratch, controls):
    """Compare the exact packaged executable before publishing its selection.

    This is submitted-frame equality, not a performance or all-map gate. The
    frozen parity driver owns capture processes, private preferences and raw
    frames. Only its bounded report and driver-log tail leave scratch.
    """
    source, runtime, maps = source.resolve(), runtime.resolve(strict=True), maps.resolve(strict=True)
    output.mkdir(parents=True, exist_ok=True)
    helper = source / 'tools/perf_4k60/parity.py'
    map_path = maps / 'coagulation.map'
    parity_report = output / 'parity.json'
    parity_report.unlink(missing_ok=True)
    log = scratch / 'renderer-4k-parity.log'
    receipt = dict(passed=False, scope='Original/Off 3840x2160 submitted-frame equality; no 4K60 claim',
                   runtime=str(runtime), reference_controls=controls, cases=list(RENDERER_4K_CASES),
                   parity_report=str(parity_report), timeout_seconds=6600)
    try:
        pins = dict(runtime_sha256=digest(runtime), helper_sha256=digest(helper), map_sha256=digest(map_path))
        receipt.update(pins)
        assignments = ','.join(f'{name}={value}' for name, value in sorted(controls.items()))
        command = [sys.executable, '-B', str(source / 'tools/run_test.py'),
                   '--scratch-root', str(scratch / 'renderer-4k-captures'), '--',
                   sys.executable, '-B', str(helper), str(parity_report),
                   assignments + '@' + str(runtime), str(runtime), *RENDERER_4K_CASES,
                   '--maps', str(maps)]
        receipt['command'] = command
        environment = runtime_environment(scratch)
        environment.update(H2_MAP='coagulation', H2_REPEAT_CANDIDATE='1')
        print('Checking exact packaged renderer: Original/Off 4K, four parity cases and repeats…', flush=True)
        with log.open('wb') as stream:
            result = subprocess.run(command, cwd=runtime.parent, env=environment,
                                    stdout=stream, stderr=subprocess.STDOUT, timeout=6600)
        receipt['exit_code'] = result.returncode
        if result.returncode:
            raise RuntimeError('Packaged renderer 4K parity process failed')
        if (parity_report.is_symlink() or not parity_report.is_file() or
                not 0 < parity_report.stat().st_size <= 1024 * 1024):
            raise RuntimeError('Packaged renderer 4K parity has no bounded direct report')
        data = json.loads(parity_report.read_text())
        if (data.get('passed') is not True or data.get('reference') != str(runtime) or
                data.get('candidate') != str(runtime) or data.get('reference_env') != controls or
                data.get('candidate_env') != {} or data.get('maps') != str(maps) or
                data.get('map') != 'coagulation' or data.get('map_path') != str(map_path) or
                data.get('native_size') != [3840, 2160] or
                any(data.get(name) != value for name, value in pins.items() if name != 'runtime_sha256') or
                data.get('reference_sha256') != pins['runtime_sha256'] or
                data.get('candidate_sha256') != pins['runtime_sha256']):
            raise RuntimeError('Packaged renderer 4K report does not qualify this executable/helper/map/control cohort')
        cases = data.get('cases', [])
        if (not isinstance(cases, list) or any(not isinstance(case, dict) for case in cases) or
                [case.get('case') for case in cases] != list(RENDERER_4K_CASES)):
            raise RuntimeError('Packaged renderer 4K parity did not complete the four required cases')
        expected_bytes = 3840 * 2160 * 3 + len(b'P6\n3840 2160\n255\n')
        for index, case in enumerate(cases):
            roles = ['reference', 'reference-repeat', 'candidate', 'candidate-repeat'] if index == 0 else ['reference', 'candidate']
            runs = case.get('runs', [])
            if not isinstance(runs, list) or any(not isinstance(run, dict) for run in runs):
                raise RuntimeError('Packaged renderer 4K capture records are malformed')
            hashes = [run.get('sha256') for run in runs]
            if (case.get('passed') is not True or case.get('captures_valid') is not True or
                    case.get('identical') is not True or [run.get('role') for run in runs] != roles or
                    not hashes or any(not isinstance(pin, str) or not re.fullmatch('[0-9a-f]{64}', pin) for pin in hashes) or
                    len(set(hashes)) != 1 or
                    any(run.get('valid') is not True or type(run.get('exit_code')) is not int or
                        run['exit_code'] != 0 or run.get('timed_out') or run.get('bytes') != expected_bytes
                        for run in runs) or
                    (index == 0 and (case.get('reference_deterministic') is not True or
                                     case.get('candidate_deterministic') is not True))):
                raise RuntimeError('Packaged renderer 4K parity has invalid, differing or incomplete captures/repeats')
        if pins != dict(runtime_sha256=digest(runtime), helper_sha256=digest(helper), map_sha256=digest(map_path)):
            raise RuntimeError('Packaged renderer 4K executable/helper/map changed during qualification')
        receipt.update(passed=True, parity_report_sha256=digest(parity_report))
    except (OSError, ValueError, KeyError, TypeError, RuntimeError, subprocess.TimeoutExpired) as error:
        receipt['error'] = str(error)[-1500:]
    finally:
        if (not parity_report.is_symlink() and parity_report.is_file() and
                parity_report.stat().st_size > 1024 * 1024):
            receipt['discarded_oversized_report_bytes'] = parity_report.stat().st_size
            parity_report.unlink()
        if log.is_file():
            with log.open('rb') as stream:
                stream.seek(max(0, log.stat().st_size - 16384))
                tail = stream.read()
            (output / 'driver.log').write_bytes(tail)
        atomic_json(output / 'report.json', receipt)
    return receipt


def atomic_json(path, value):
    temporary = path.with_name('.' + path.name + '-' + uuid.uuid4().hex)
    try:
        temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def publication_identity(path):
    """Read a regular publication file without adopting a concurrent rewrite."""
    try:
        before = path.lstat()
    except FileNotFoundError:
        return None
    if not stat.S_ISREG(before.st_mode):
        raise RuntimeError('Publication file is not regular: ' + path.name)
    pin = digest(path)
    after = path.lstat()
    stable = lambda value: (value.st_dev, value.st_ino, value.st_size, value.st_mtime_ns,
                            value.st_ctime_ns, value.st_mode)
    if stable(before) != stable(after):
        raise RuntimeError('Publication file changed while reading: ' + path.name)
    # Rename may change ctime. The incoming inode, contents, mtime and mode
    # remain usable even if os.replace commits and then raises/interruption.
    return (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns,
            stat.S_IMODE(after.st_mode), pin)


def publish_runtime_files(root, destination, scratch, record, frontend_record, report, expected,
                          *, cache_choice=None):
    """Rollback only this transaction's still-owned root files on any failure."""
    if report.get('custom_asset_input_cohort', {}).get('kind') == 'owned_frozen':
        from custom_assets import inventory as custom_asset_inventory
        manifest = destination / 'custom-assets/custom-assets.json'
        if (digest(manifest) != report['custom_assets']['sha256'] or
                json.loads(manifest.read_text())['files'] != custom_asset_inventory(root)):
            raise RuntimeError('Current custom assets differ from the frozen qualified candidate; existing runtime retained')
    backup = scratch / ('engine-publication-' + uuid.uuid4().hex)
    backup.mkdir()
    state = report['publication'] = dict(complete=False, rollback_failed=False, files=[])
    changes, temporary_files = [], []
    entries = [(root / name, destination / name, None)
               for name in ('Halo2-Launcher', 'Halo2-Title-Launcher')]
    entries.append((root / 'gameplay-runtime-selection.json', None, record))
    if frontend_record:
        entries.append((root / 'frontend-runtime-selection.json', None, frontend_record))
    if cache_choice is not None:
        from mcc_onboarding import REGISTRY, SCHEMA, private_cache
        candidate = Path(cache_choice)
        cache, _ = private_cache(root, candidate if candidate.is_absolute() else root / candidate,
                                 verify=True)
        relative = cache.relative_to(root).as_posix()
        if any(c in relative for c in '\t\r\n\0') or record.get('cache') != relative:
            raise ValueError('Publication cache differs from its qualified runtime')
        # Onboarding's selection is a two-line TSV contract, not JSON.
        entries.append((root / REGISTRY, None, (SCHEMA + '\ncache\t' + relative + '\n').encode()))
    try:
        for target, source, value in entries:
            name = target.relative_to(root).as_posix()
            previous = expected[name]
            if publication_identity(target) != previous:
                raise RuntimeError('Another task changed publication file: ' + name)
            saved = backup / target.name
            if previous is not None:
                shutil.copy2(target, saved)
                if digest(saved) != previous[-1] or publication_identity(target) != previous:
                    raise RuntimeError('Publication backup changed: ' + target.name)
            row = dict(name=name, previous_sha256=previous[-1] if previous else None,
                       candidate_sha256=None, outcome='unchanged')
            state['files'].append(row)
            changes.append(dict(target=target, source=source, value=value, previous=previous,
                                saved=saved, candidate=None, row=row))
        for change in changes:
            target = change['target']
            temporary = target.with_name('.' + target.name + '-' + uuid.uuid4().hex)
            temporary_files.append(temporary)
            if change['source'] is not None:
                shutil.copy2(change['source'], temporary)
            elif isinstance(change['value'], bytes):
                temporary.write_bytes(change['value'])
            else:
                temporary.write_text(json.dumps(change['value'], indent=2, sort_keys=True) + '\n')
            change['candidate'] = publication_identity(temporary)
            change['row']['candidate_sha256'] = change['candidate'][-1]
            if publication_identity(target) != change['previous']:
                raise RuntimeError('Another task changed publication file: ' + target.name)
            # Register the exact incoming inode before rename, including the
            # failure window after rename but before this call returns.
            change['row']['outcome'] = 'attempted'
            os.replace(temporary, target)
            change['row']['outcome'] = 'published'
        if cache_choice is not None:
            from gameplay_runtime_selection import selected_gameplay_runtime
            if selected_gameplay_runtime(root) != record:
                raise RuntimeError('Published gameplay/cache selection failed final admission')
        state['complete'] = True
    except BaseException:
        # If cancellation interrupts rollback itself, conservatively keep the
        # candidate; a later finally must not delete a referenced runtime.
        state['rollback_failed'] = True
        failed = False
        for change in reversed(changes):
            if change['candidate'] is None:
                continue
            target = change['target']
            try:
                if publication_identity(target) != change['candidate']:
                    change['row']['outcome'] = 'foreign_or_missing_preserved'
                    continue
                if change['previous'] is None:
                    target.unlink()
                else:
                    temporary = target.with_name('.' + target.name + '-restore-' + uuid.uuid4().hex)
                    temporary_files.append(temporary)
                    shutil.copy2(change['saved'], temporary)
                    if digest(temporary) != change['previous'][-1]:
                        raise RuntimeError('Publication backup is no longer authentic')
                    if publication_identity(target) != change['candidate']:
                        change['row']['outcome'] = 'foreign_or_missing_preserved'
                        continue
                    os.replace(temporary, target)
                change['row']['outcome'] = 'rolled_back'
            except BaseException as error:
                failed = True
                change['row'].update(outcome='rollback_failed', error_class=type(error).__name__)
        state['rollback_failed'] = failed
        raise
    finally:
        for path in temporary_files:
            path.unlink(missing_ok=True)


def candidate_references(root, destination, publication):
    """Fail closed for root files that still contain a candidate path/binary."""
    expected = {row['name']: row['candidate_sha256'] for row in publication.get('files', [])}
    needles = (str(destination).encode(), destination.relative_to(root).as_posix().encode())
    references = []
    for name in ('Halo2-Launcher', 'Halo2-Title-Launcher',
                 'gameplay-runtime-selection.json', 'frontend-runtime-selection.json'):
        path = root / name
        try:
            identity = publication_identity(path)
            if identity is None:
                continue
            found = identity[-1] == expected.get(name)
            with path.open('rb') as stream:
                tail = b''
                while not found and (part := stream.read(65536)):
                    data = tail + part
                    found = any(needle in data for needle in needles)
                    tail = data[-max(map(len, needles)):]
            if publication_identity(path) != identity:
                raise RuntimeError('Publication reference changed while reading')
            if found:
                references.append(dict(name=name, status='candidate_reference'))
        except BaseException as error:
            references.append(dict(name=name, status='reference_unknown', error_class=type(error).__name__))
    return references


def weapon_audio(build, source, maps, image, output, build_report):
    """Require the actual repeated local weapon scenario before promotion."""
    helper = source / 'tools/validate_weapon_audio_runtime.py'
    test = build / 'multiplayer_audio_runtime_tests'
    pins = {name: digest(source / name) for name in (
        'tools/validate_weapon_audio_runtime.py', 'tests/multiplayer_audio_runtime_tests.cpp',
        'tests/multiplayer_audio_tests.cpp', 'tests/mcc_sound_rate_tests.cpp')}
    image_pin, binary_pin = digest(image), digest(test)
    command = [sys.executable, '-B', str(helper), '--build-report', str(build_report),
               '--maps', str(maps), '--image', str(image), '--output', str(output),
               '--cycles', '3', '--timeout', '600']
    print('Checking repeated weapon shots/reloads on Coagulation (1/2/4 players) and Lockout…', flush=True)
    result = subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    report = json.loads((output / 'report.json').read_text())
    expected = {'coagulation1', 'coagulation2', 'coagulation4', 'lockout1'}
    cases = report.get('cases', [])
    unchanged = digest(image) == image_pin and digest(test) == binary_pin and all(
        digest(source / name) == pin for name, pin in pins.items())
    report['passed'] = bool(result.returncode == 0 and report.get('passed') and unchanged and
        len(cases) == len(expected) and {case['name'] for case in cases} == expected and
        all(case['passed'] and case['exit_code'] == 0 for case in cases) and
        report.get('binary_sha256') == binary_pin and report.get('image_sha256') == image_pin)
    report.update(test_source_sha256=pins, inputs_unchanged=unchanged)
    atomic_json(output / 'report.json', report)
    return report


def eos_client_configuration(root, explicit, prior):
    """Keep configured ONLINE builds configured across ordinary rebuilds."""
    if explicit is not None:
        return Path(explicit).resolve(strict=True)
    candidates = []
    if prior is not None and prior.get('runtime'):
        candidates.append((root / prior['runtime']).with_name('eos-client.env'))
    candidates.append(root / '.local/eos-setup/client.env')
    return next((path for path in candidates if path.is_file()), None)


def worker(args):
    from gameplay_runtime_selection import SELECTION, selected_gameplay_runtime
    from mcc_onboarding import REGISTRY, private_cache, selected
    root = args.root.resolve(strict=True)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    prepare_only = getattr(args, 'prepare_only', False)
    renderer_controls = renderer_reference_controls(getattr(args, 'renderer_4k_reference_control', []))
    # Preserve the exact existing selection if another task promotes a runtime
    # while this build is in flight.
    selection = root / SELECTION
    previous = selection.read_bytes() if selection.is_file() else None
    frontend_selection = root / 'frontend-runtime-selection.json'
    previous_frontend = frontend_selection.read_bytes() if frontend_selection.is_file() else None
    cache_candidate = getattr(args, 'cache_candidate', None)
    publication_names = ('Halo2-Launcher', 'Halo2-Title-Launcher', SELECTION, 'frontend-runtime-selection.json')
    if cache_candidate is not None:
        publication_names += (REGISTRY,)
    publication_baseline = {} if prepare_only else {
        name: publication_identity(root / name) for name in
        publication_names}
    baseline_selection = getattr(args, 'baseline_selection', None)
    baseline_cache = getattr(args, 'baseline_cache', None)
    baseline_admission = {}
    if baseline_selection:
        baseline_admission['selection_name'] = baseline_selection
    if baseline_cache is not None:
        if not prepare_only or not baseline_selection:
            raise ValueError('A baseline cache requires a named preparation baseline')
        baseline_admission['cache_candidate'] = baseline_cache
    prior = selected_gameplay_runtime(root, **baseline_admission)
    if cache_candidate is None:
        cache = selected(root)
    else:
        candidate = Path(cache_candidate)
        cache = private_cache(root, candidate if candidate.is_absolute() else root / candidate,
                              verify=True)
    if cache is None:
        raise RuntimeError('Prepare the private MCC cache before building the launcher engine')
    maps = cache[0] / 'halo2/h2_maps_win64_dx11'
    reference_cache = cache[0]
    reference_pins = {}
    reference_input = getattr(args, 'native_reference_cache', None)
    if reference_input is not None:
        if not prepare_only:
            raise ValueError('A separate native reference cache requires candidate preparation')
        candidate = reference_input if reference_input.is_absolute() else root / reference_input
        reference_cache, _ = private_cache(root, candidate, verify=True)
        for relative in ('provenance.json', 'halo2/h2_maps_win64_dx11/shared.map',
                         'native-shaders/coagulation/library.json', 'native-shaders/01b_spacestation/library.json'):
            path = reference_cache / relative
            if path.resolve(strict=True) != path or not path.is_file():
                raise ValueError('Native reference cache lacks a direct reference file: ' + relative)
            reference_pins[relative] = digest(path)
    audio_image = getattr(args, 'weapon_audio_reference_xbe', None)
    if audio_image:
        audio_image = audio_image.resolve(strict=True)
    names = qualification_maps(prior, getattr(args, 'qualify_map', []))
    default_map = prior['default_map'] if prior else 'lockout.map'
    players = prior['local_players'] if prior else 1
    source = scratch / 'source'
    reuse_owned_build = getattr(args, 'reuse_owned_build', False)
    configuration = getattr(args, 'config', 'Release')
    if reuse_owned_build:
        # A finite run_test.py coordinator already owns this frozen source
        # and build. Reuse that exact tree, including its declared refreshes;
        # never admit a checkout, installed runtime or unrelated build here.
        if not args.worker or source.is_symlink() or not source.is_dir():
            raise RuntimeError('Engine reuse requires this active scratch owner\'s private source')
        configured = scratch / 'build/CMakeCache.txt'
        expected = 'CMAKE_HOME_DIRECTORY:INTERNAL=' + str(source.resolve())
        if not configured.is_file() or expected not in configured.read_text().splitlines():
            raise RuntimeError('Engine reuse requires the same owned CMake source/build pair')
        # Component snapshots borrow authored data. Production bundlers require
        # a private regular tree, so materialize only this declared alias before
        # pinning the engine snapshot. The original data directory stays owned
        # by the checkout and is never removed or modified.
        data = source / 'data'
        if data.is_symlink():
            if data.resolve() != (root / 'data').resolve():
                raise RuntimeError('Owned engine data alias has a foreign source')
            files = [p for p in data.rglob('*') if p.is_file()]
            if any(p.is_symlink() for p in data.rglob('*')):
                raise RuntimeError('Owned engine data contains a nested symlink')
            expected_data = {p.relative_to(data).as_posix(): digest(p) for p in files}
            private = scratch / 'engine-private-data'
            shutil.copytree(data, private)
            copied = {p.relative_to(private).as_posix(): digest(p)
                      for p in private.rglob('*') if p.is_file()}
            if copied != expected_data or any(digest(data / p) != pin for p, pin in expected_data.items()):
                shutil.rmtree(private)
                raise RuntimeError('Authored data changed while freezing the owned engine bundle')
            data.unlink()
            private.rename(data)
        pins = source_hashes(source)
    else:
        pins = freeze_source(root, source)
    from custom_assets import inventory as custom_asset_inventory, install as install_custom_assets
    asset_root = root
    frozen_asset_root = getattr(args, 'frozen_custom_asset_root', None)
    asset_manifest_pin = None
    if frozen_asset_root is not None:
        from installation_paths import resource_root
        asset_root = frozen_asset_root.resolve(strict=True)
        if (not prepare_only or not reuse_owned_build or frozen_asset_root.is_symlink() or
                asset_root.parent != scratch.resolve() or asset_root in (source, scratch / 'build') or
                resource_root(asset_root) != asset_root):
            raise RuntimeError('Frozen custom assets require their own direct directory in the active preparation owner')
        manifest = asset_root / 'custom-assets.json'
        if manifest.is_symlink():
            raise RuntimeError('Frozen custom asset receipt must be a regular owned file')
        asset_manifest_pin = digest(manifest)
    asset_pins = custom_asset_inventory(asset_root)
    if asset_manifest_pin is not None:
        if (json.loads(manifest.read_text())['files'] != asset_pins or
                custom_asset_inventory(root) != asset_pins or digest(manifest) != asset_manifest_pin):
            raise RuntimeError('Frozen custom assets must match their receipt and current Root inputs before qualification')
    map_pins = {name: digest(maps / name) for name in names}
    grenade_map_pins = {name: digest(maps / name)
                        for name, _ in GRENADE_SHADOW_CASES.values()}
    smg_reference = smg_warthog_reference(source, maps)
    gameplay_reference = tu15_gameplay_reference(getattr(args, 'tu15_gameplay_reference_xbe', None))
    destination = root / 'bin/engine-builds' / uuid.uuid4().hex
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    atomic_json(output / 'source-hashes.json', pins)
    atomic_json(output / 'custom-asset-source.json', asset_pins)
    report = dict(passed=False,
                  scope='qualified_candidate_only' if prepare_only else
                      'Owned frozen source cohort, launcher routing checks and MCC submitted-frame smoke'
                      if reuse_owned_build else
                      'Current source build, launcher routing checks and MCC submitted-frame smoke',
                  status='building', qualified_candidate=False, runtime_installed=False,
                  runtime=destination.relative_to(root).as_posix() + '/halo2_native',
                  unresolved_case_failures=[], accepted_case_failures=[],
                  limits=['No full gameplay, Xbox framebuffer or audio parity claim'])
    report['reused_active_owned_build'] = reuse_owned_build
    if asset_manifest_pin is not None:
        report['custom_asset_input_cohort'] = dict(kind='owned_frozen', source=str(asset_root),
            manifest_sha256=asset_manifest_pin, inventory_sha256=digest(output / 'custom-asset-source.json'))
    report['build_configuration'] = configuration
    report['requested_additional_maps'] = sorted(set(getattr(args, 'qualify_map', [])))
    report['grenade_shadow_maps'] = grenade_map_pins
    report['smg_warthog_reference'] = smg_reference
    report['tu15_gameplay_reference'] = gameplay_reference
    # A reused qualification belongs to this declared source/build pair, which
    # can intentionally differ from a concurrently edited working tree. Record
    # its exact inventory rather than calling that tree the current sources.
    report['source_cohort'] = dict(
        kind='active_owned_frozen' if reuse_owned_build else 'working_tree_snapshot',
        source=str(source.resolve()), build=str((scratch / 'build').resolve()),
        inventory_sha256=digest(output / 'source-hashes.json'))
    if cache_candidate is not None:
        report['cache_candidate'] = cache[0].relative_to(root).as_posix()
    if reference_pins:
        report['native_reference_cache'] = dict(path=reference_cache.relative_to(root).as_posix(),
            sha256=reference_pins)
    promoted = False
    prepared = False
    candidate_qualified = False
    try:
        print('Building the complete engine and both launchers in disposable storage…', flush=True)
        command = [sys.executable, '-B', str(root / 'tools/run_tests.py'), '--build-worker',
                   '--source', str(source), '--output', str(output / 'validation'),
                   '--target', 'halo2_native', '--target', 'halo2_launcher',
                   '--target', 'halo2_title_launcher', '--target', 'halo2_onboarding',
                   '--target', 'shadow_array_swap_tu15_tests',
                   '--target', 'multiplayer_tests', '--target', 'native_request_protocol_tests',
                   '--target', 'frontend_custom_variant_settings_tests',
                   '--target', 'frontend_custom_match_settings_tests',
                   '--target', 'frontend_match_customization_tests',
                   '--target', 'frontend_online_tests',
                   '--target', 'loop_audio_locations_tu15_tests',
                   '--target', 'native_weapon_effect_iterator_tu15_tests',
                   '--target', 'native_weapon_effect_iterator_callers_tu15_tests',
                   '--target', 'mcc_resident_reflection_template_tests',
                   '--target', 'frontend_mcc_submenus_tests',
                   '--target', 'steam_workshop_menu_tests', '--target', 'steam_workshop_cache_tests',
                   '--target', 'workshop_mod_probe',
                   '--target', 'workshop_bloom_tests', '--target', 'workshop_script_contract_tests',
                   '--target', 'workshop_ai_activation_tests', '--target', 'workshop_ai_activation_contract_tests',
                   '--target', 'workshop_unit_entry_tests',
                   '--target', 'workshop_ai_actors_tests',
                   '--target', 'campaign_ai_scalar_defaults_tu15_tests', '--target', 'native_tu15_cold_data_tests',
                   '--target', 'native_tu15_base_lock_tests', '--target', 'workshop_shape_type_tests',
                   '--target', 'workshop_wrapped_agent_tests',
                   '--target', 'workshop_collection_agent_tests', '--target', 'workshop_collection_filter_tests',
                   '--target', 'cinematic_bloom_tests', '--target', 'mcc_script_layout_tests',
                   '--target', 'native_mcc_script_builtin_tests', '--target', 'native_mcc_script_command_tests',
                   '--target', 'script_mcc_legacy_command_tests', '--target', 'script_clock_builtin_tests',
                   '--target', 'native_mcc_script_world_tests',
                   '--target', 'native_mcc_scenery_animation_world_tests',
                   '--target', 'native_mcc_scenery_animation_gpu_tests',
                   '--target', 'workshop_scenery_start_tests',
                   '--target', 'object_name_lookup_tu15_tests',
                   '--target', 'workshop_empty_prop_content_tests',
                   '--target', 'workshop_navigation_content_tests',
                   '--target', 'workshop_camera_shared_tests',
                   '--target', 'native_tu15_world_residuals_tests',
                   '--target', 'native_tu15_overlap_query_handoff_tests',
                   '--target', 'native_mcc_gravlift_contact_tu15_tests',
                   '--target', 'native_mcc_custom_mechanics_tu15_tests',
                   '--target', 'custom_mode_local_launch_tests',
                   '--target', 'native_first_person_registry_tu15_tests',
                   '--target', 'native_source_membership_tests',
                   '--target', 'native_mcc_unit_equipment_profile_tu15_tests',
                   '--target', 'native_mcc_custom_powerup_tu15_tests',
                   '--target', 'source_bsp_breakable_visibility_tu15_tests',
                   '--target', 'source_model_depth_input_tests',
                   '--target', 'world_effects_tests',
                   '--target', 'native_late_effect_light_tu15_tests',
                   '--target', 'native_tu15_ui_script_startup_tests',
                   '--target', 'mcc_native_tex_bump_tests',
                   '--target', 'mcc_native_pc_world_programs_probe',
                   '--target', 'mcc_native_vehicle_restoration_probe',
                   '--target', 'mcc_native_character_shared_probe',
                   '--target', 'object_function_symbol_tu15_tests',
                   '--target', 'mcc_native_meter_tests',
                   '--target', 'mcc_native_sky_cooked_probe', '--target', 'workshop_sky_curve_tests',
                   '--target', 'mcc_native_world_animated_probe',
                   '--target', 'mcc_native_env_illum_three_channel_probe',
                   '--target', 'mcc_native_tiling_specular_probe',
                   '--target', 'mcc_native_alpha_environment_probe',
                   '--filter', '^(launcher_first_run|title_launcher_first_run|bundled_launcher_first_run|'
                   'launcher_automatic_build|launcher_engine_promotion|native_gameplay_launcher_routing|native_frontend_launcher_routing|original_tu15_shadow_array_swap|native_mcc_split_screen_menu_startup|'
                   'native_mcc_grenade_shadow_neutral|native_mcc_grenade_shadow_frag|native_mcc_grenade_shadow_plasma|'
                   'native_mcc_grenade_shadow_receiver|'
                   'native_mcc_friends_footer_anchor|'
                   'original_tu15_loop_audio_locations|'
                   'native_weapon_effect_iterator_tu15|native_weapon_effect_iterator_frame_tu15|'
                   'native_weapon_effect_iterator_support_tu15|native_weapon_effect_iterator_death_tu15|'
                   'native_mcc_smg_warthog_audio|'
                   'mcc_resident_reflection_template|'
                   'mcc_shared_env_illum|mcc_native_pc_world_programs|mcc_native_pc_world_programs_generator|'
                   'mcc_native_pc_world_programs_source_controls|mcc_native_vehicle_restoration|mcc_native_vehicle_restoration_generator|'
                   'mcc_native_character_shared|mcc_native_character_shared_generator|mcc_native_character_shared_actual_source|'
                   'object_function_symbol_namespace|'
                   'mcc_native_meter|mcc_native_meter_generator|mcc_native_meter_actual_source|'
                   'mcc_frontend_submenus|mcc_frontend_selection_stability|'
                   'steam_workshop_content|steam_workshop_sweep_acceptance|steam_workshop_menu|steam_workshop_cache|'
                   'workshop_bloom_controls|workshop_script_contracts|original_cinematic_bloom_tu15|'
                   'workshop_ai_activation|workshop_ai_activation_contracts|workshop_unit_entry|'
                   'workshop_ai_actors|'
                   'mcc_native_sky_cooked|mcc_native_sky_cooked_generator|workshop_sky_curves|'
                   'native_tu15_base_lock_reference|workshop_shape_types|'
                   'workshop_wrapped_agent|'
                   'workshop_collection_agent|workshop_collection_filter|'
                   'mcc_native_world_animated|mcc_native_world_animated_generator|'
                   'mcc_native_env_illum_three_channel|mcc_native_env_illum_three_channel_generator|'
                   'mcc_native_tiling_specular|mcc_native_tiling_specular_generator|'
                   'mcc_native_alpha_environment|mcc_native_alpha_environment_generator|'
                   'original_campaign_ai_scalar_defaults_tu15|original_workshop_ai_aim_scalar_defaults_tu15|native_tu15_cold_data|'
                   'mcc_script_signature_layout|mcc_script_signature_profile|'
                   'native_mcc_script_clock_getters|native_mcc_script_commands|native_mcc_script_installed_commands|'
                   'mcc_script_legacy_command_simulation|mcc_script_clock_initializer_runtime|'
                   'native_mcc_script_world_commands|'
                   'native_mcc_scenery_animation_(bootstrap|hs|named_hs|lifecycle|render_consumer|relative_consumer|inactive_control|gpu|relative_gpu)|'
                   'workshop_scenery_start|'
                   'original_object_name_lookup_tu15|original_workshop_camera_shared|'
                   'workshop_empty_prop_content|workshop_navigation_content|native_world_residuals_model_tu15|native_overlap_query_handoff_tu15|'
                   'native_mcc_gravlift_contact_(lockout|midship|coagulation)_[12]|'
                   'native_mcc_custom_mechanics_tu15|'
                   'native_mcc_custom_mode_(local|render)_launch|'
                   'native_mcc_fp_registry_local_human|'
                   'native_source_nonplaying_membership|'
                   'native_cli_gameplay_routes_tu15|'
                   'native_mcc_unit_(equipment_profile|vitality_observer)_tu15|'
                   'native_mcc_custom_powerup_tu15|'
                   'source_bsp_breakable_visibility_tu15|native_source_model_depth_owners|'
                   'native_world_light_material_catalog|native_late_effect_light_creation|'
                   'original_tu15_hs_scalar_cast_constructor|'
                   'native_request_protocol|native_session_configuration|native_session_configuration_loading|native_custom_variant_settings|'
                   'original_frontend_custom_match_settings|native_mcc_frontend_custom_match_settings|original_frontend_match_customization|native_mcc_frontend_match_customization)$',
                   '--jobs', str(args.jobs), '--test-jobs', '1',
                   '--config', configuration,
                   '--cmake-arg=-DHALO2_BUILD_LAUNCHERS_WITH_NATIVE=ON',
                   '--cmake-arg=-DHALO2_STAGE_BUILD_OUTPUT=OFF',
                   '--cmake-arg=-DHALO2_STAGE_CUSTOM_ASSETS=ON',
                   '--cmake-arg=-DHALO2_CUSTOM_ASSET_ROOT=' + str(asset_root),
                   '--cmake-arg=-DHALO2_TEST_ASSET_SOURCE_ROOT=' + str(root),
                   '--cmake-arg=-DHALO2_WORKSHOP_TEST_CACHE=' + str(cache[0]),
                   '--cmake-arg=-DHALO2_MCC_MULTIPLAYER_ENGINE_MAP=' + str(maps / default_map),
                   '--cmake-arg=-DHALO2_MCC_ONLINE_PRESENTATION_MAP=' + str(maps / default_map),
                   '--cmake-arg=-DHALO2_MCC_GRAVLIFT_MAP_DIRECTORY=' + str(maps),
                   '--cmake-arg=-DHALO2_OBJECT_FUNCTION_MCC_MAP=' + str(maps / 'coagulation.map'),
                   '--cmake-arg=-DHALO2_MCC_NATIVE_LIBRARY_DIRECTORY=' + str(reference_cache / 'native-shaders'),
                   '--cmake-arg=-DHALO2_MCC_PARTY_UI_TEST_CACHE=' + str(cache[0]),
                   '--cmake-arg=-DHALO2_MCC_WATER_MAP_DIRECTORY=' + str(maps),
                   '--cmake-arg=-DHALO2_BUNDLE_MOVIES=OFF',
                   '--cmake-arg=-DHALO2_LAUNCHER_RUNTIME_DIR=' + str(destination),
                   '--cmake-arg=-DHALO2_RUNTIME_SHADER_DIR=' + str(destination / 'shaders'),
                   '--retain-report', 'engine-targets.txt',
                   '--retain-report', 'multiplayer-menu-startup.json',
                   '--retain-report', 'custom-mechanics.json',
                   '--retain-report', 'custom-mode-local-launch.json',
                   '--retain-report', 'custom-mode-render-launch.json',
                   '--retain-report', 'native_mcc_fp_registry_local_human.json',
                   '--retain-report', 'native-cli-gameplay-routes.json',
                   '--retain-report', 'native-unit-equipment-profile.json',
                   '--retain-report', 'native-unit-vitality-observer.json',
                   '--retain-report', 'native-custom-powerup.json',
                   '--retain-report', 'friends-footer-anchor.json',
                   '--retain-report', 'loop-audio-locations.json',
                   '--retain-report', 'smg-warthog-audio.json',
                   '--retain-report', 'smg-warthog-audio.log',
                   '--retain-report', 'mcc-submenus.json',
                   '--retain-report', 'mcc-selection-stability.json',
                   '--retain-report', 'workshop-empty-prop-content.json',
                   '--retain-report', 'workshop-navigation-content.json',
                   '--retain-report', 'tiling.json',
                   '--retain-report', 'alpha-environment.json',
                   '--retain-report', 'native-late-effect-light-creation.json',
                   '--retain-report', 'source-model-depth-owners.json']
        if gameplay_reference:
            command.append('--cmake-arg=-DHALO2_TU15_GAMEPLAY_XBE=' + gameplay_reference['path'])
        for kind in GRENADE_SHADOW_CASES:
            command.extend(['--retain-report', f'grenade-shadow-{kind}.json',
                            '--retain-report', f'grenade-shadow-{kind}.log'])
        # A present kit bank changes the actual loading path. Require its
        # submitted-frame handoffs and the fresh/reuse onboarding regressions
        # before a candidate can replace either selected launcher runtime.
        if (cache[0] / 'presentation/original-loading-v2/manifest.json').is_file():
            command.extend(['--target', 'original_loading_gpu_tests',
                '--target', 'native_loading_presentation_tests',
                '--target', 'bundled_loading_ambience_tests', '--target', 'vista_loading_tests',
                '--target', 'vista_loading_thread_tests',
                '--cmake-arg=-DHALO2_ORIGINAL_LOADING_CACHE=' + str(cache[0])])
            at = command.index('--filter') + 1
            command[at] = command[at][:-2] + ('|original_loading_gpu|original_loading_gpu_4_3|native_vista_loading_adapter|h2ek_loading_onboarding|'
                'bundled_loading_ambience|original_vista_loading|original_vista_loading_thread)$')
        if getattr(args, 'terminal_display_checks', False):
            command.extend(['--target', 'source_machine_scene_tests', '--target', 'source_machine_portal_tests',
                            '--target', 'source_object_lod_scene_tests'])
            at = command.index('--filter') + 1
            command[at] = command[at][:-2] + '|retail_source_machine_scene|retail_source_machine_portal|retail_source_object_lod_scene)$'
            report['terminal_display_checks_requested'] = True
        if audio_image:
            command.extend(['--target', 'multiplayer_audio_runtime_tests',
                            '--target', 'multiplayer_audio_tests', '--target', 'mcc_sound_rate_tests',
                            '--retain-report', 'Testing/Temporary/LastTest.log'])
            at = command.index('--filter') + 1
            command[at] = command[at][:-2] + '|multiplayer_authored_audio|mcc_sound_codec_rates)$'
        sdk_archive = root / 'EOS-SDK-58105819-Release-v1.19.2.1.zip'
        if sdk_archive.is_file():
            command.append('--cmake-arg=-DHALO2_EOS_SDK_ARCHIVE=' + str(sdk_archive))
        client_config = eos_client_configuration(root, getattr(args, 'eos_client_config', None), prior)
        if client_config:
            command.append('--cmake-arg=-DHALO2_EOS_CLIENT_CONFIG=' + str(client_config.resolve(strict=True)))
        # The executables/tests bind their compiled default to the canonical
        # candidate shader bank. Build first without claiming validation, then
        # make that bank available before any required submitted-frame tests.
        build_command=command.copy()
        build_output=output/'build-only'
        build_command[build_command.index('--output')+1]=str(build_output)
        build_command.append('--build-only')
        result = subprocess.run(build_command, cwd=root, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError('Engine build failed; see ' + str(build_output))
        build_report=json.loads((build_output/'report.json').read_text())
        if (build_report.get('status')!='built' or build_report.get('passed')is not False or
                build_report.get('validation_not_run')is not True):
            raise RuntimeError('Engine build-only receipt must not claim test acceptance')
        if source_hashes(source) != pins:
            raise RuntimeError('Frozen engine source changed during the build')
        build = scratch / 'build'
        destination.mkdir(parents=True,exist_ok=False)
        from stage_native_renderer_shaders import stage_production_shaders
        from native_renderer_shaders import validate_native_package
        shader_package=stage_production_shaders(build,destination/'shaders')
        staged_shader_proof=validate_native_package(destination/'shaders',executable=build/'halo2_native')
        executable_pin=digest(build/'halo2_native')
        report['native_renderer_shader_package']={
            'passed':shader_package['passed'],
            'module_count':shader_package['module_count'],
            'inventory_sha256':shader_package['inventory_sha256'],
            'report':str((destination/'shaders/production-shaders.json').relative_to(root)),
            'report_sha256':staged_shader_proof['package']['sha256']}
        print('Checking the complete qualification suite against the staged production shader bank…',flush=True)
        required_captures = ['multiplayer-menu-startup.json', 'friends-footer-anchor.json',
                             'loop-audio-locations.json', 'smg-warthog-audio.json', 'smg-warthog-audio.log']
        required_captures.extend(f'grenade-shadow-{kind}.{suffix}'
            for kind in GRENADE_SHADOW_CASES for suffix in ('json', 'log'))
        for name in required_captures:
            (build/name).unlink(missing_ok=True)
            (output/'validation/evidence'/name).unlink(missing_ok=True)
        result=subprocess.run(command,cwd=root,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError('Engine launcher or loading checks failed; see '+str(output/'validation'))
        validation_report=json.loads((output/'validation/report.json').read_text())
        if (validation_report.get('passed')is not True or validation_report.get('status')!='passed' or
                validation_report.get('validation_not_run')is not False or
                not any(step.get('name')=='tests' and step.get('exit_code')==0
                        for step in validation_report.get('steps',[]))):
            raise RuntimeError('Engine qualification requires completed passing CTest checks')
        if source_hashes(source)!=pins:
            raise RuntimeError('Frozen engine source changed during qualification')
        if any(digest(reference_cache / relative) != pin for relative, pin in reference_pins.items()):
            raise RuntimeError('Native reference cache changed during qualification')
        if smg_warthog_reference(source, maps) != smg_reference:
            raise RuntimeError('SMG Warthog audio reference changed during qualification')
        if digest(build/'halo2_native')!=executable_pin:
            raise RuntimeError('Engine executable changed after its shader bank was staged')
        if validate_native_package(destination/'shaders',
                expected_sha256=staged_shader_proof['package']['sha256'],
                executable=build/'halo2_native')!=staged_shader_proof:
            raise RuntimeError('Staged production shader bank changed during qualification')
        report['validation']=dict(passed=True,report=str(output/'validation/report.json'),
            report_sha256=digest(output/'validation/report.json'),
            build_only_report=str(build_output/'report.json'),
            build_only_report_sha256=digest(build_output/'report.json'))
        menu_startup_path=output/'validation/evidence/multiplayer-menu-startup.json'
        menu_startup=json.loads(menu_startup_path.read_text())
        if (menu_startup.get('passed') is not True or menu_startup.get('frames',0)<1 or
                menu_startup.get('runtime_sha256')!=executable_pin):
            raise RuntimeError('Engine qualification requires the real non-reference split-screen menu Start')
        report['split_screen_menu_startup']=dict(passed=True,report=str(menu_startup_path),
            report_sha256=digest(menu_startup_path),frames=menu_startup['frames'],
            runtime_sha256=menu_startup['runtime_sha256'])
        report['grenade_shadow_residency'] = grenade_shadow_qualification(
            build, maps, output, executable_pin, grenade_map_pins)
        report['friends_footer_anchor'] = friends_footer_qualification(output, report['source_cohort'])
        report['loop_audio_locations'] = loop_audio_location_qualification(output, report['source_cohort'])
        report['smg_warthog_audio'] = smg_warthog_audio_qualification(
            build, source, maps, output, executable_pin, smg_reference)
        # An active frozen cohort can predate CMake's custom-source target.
        # Asset delivery has its own current, pinned owner, so stage it through
        # this coordinator even when that older build did not create a bank.
        staged_assets = install_custom_assets(asset_root, build / 'custom-assets')
        if staged_assets['files'] != asset_pins:
            raise RuntimeError('Custom assets changed during the engine build; existing runtime retained')
        report['wiring'] = wiring(build / 'engine-targets.txt', source)
        if not report['wiring']['passed']:
            raise RuntimeError('Engine sources are missing from the runtime link graph: ' +
                               ', '.join(report['wiring']['unexpected_unwired_sources']))
        for name in ('halo2_native', 'Halo2-Launcher', 'Halo2-Title-Launcher', 'eos_sdk_worker'):
            shutil.copy2(build / name, destination / name)
        for name in ('libEOSSDK-Linux-Shipping.so', 'eos-client.env'):
            if (build / name).is_file():
                shutil.copy2(build / name, destination / name)
        if client_config and not all((destination / name).is_file() for name in ('libEOSSDK-Linux-Shipping.so', 'eos-client.env')):
            raise RuntimeError('EOS runtime library or packaged game configuration was not delivered')
        for name in ('data', 'upscalers', 'lib'):
            if (build / name).is_dir():
                shutil.copytree(build / name, destination / name)
        custom_assets = install_custom_assets(build / 'custom-assets', destination / 'custom-assets')
        if custom_assets['files'] != asset_pins:
            raise RuntimeError('Custom assets changed during the engine build; existing runtime retained')
        report['custom_assets'] = dict(
            path='custom-assets/custom-assets.json',
            sha256=digest(destination / 'custom-assets/custom-assets.json'),
            files=len(asset_pins), bytes=custom_assets['bytes'],
            scope='Authenticated custom asset sources; gameplay admission remains separate')
        runtime = destination / 'halo2_native'
        from native_gameplay_test import eos_worker_descriptor, validate_eos_worker
        report['eos_worker'] = eos_worker_descriptor(runtime)
        if audio_image:
            report['weapon_audio'] = weapon_audio(build, source, maps, audio_image,
                output / 'weapon-audio', output / 'validation/report.json')
            if not report['weapon_audio']['passed']:
                raise RuntimeError('Actual weapon audio checks failed; existing launcher selection retained')
        report['smoke'] = []
        cases = [(name, 1) for name in names]
        cases.extend((default_map, n) for n in sorted({players, 2, 4}) if n != 1)
        report['requested_smoke_cases'] = [dict(map=name, local_players=count) for name, count in cases]
        for name, count in cases:
            print(f'Checking {name}, {count} local player(s), four submitted frames…', flush=True)
            case = smoke(runtime, maps, name, count, scratch, args.timeout, root,
                         prior.get('video_driver', '') if prior else '')
            report['smoke'].append(case)
            if not case['passed']:
                key = f'{name}:{count}'
                # An explicitly accepted case stays recorded as failed in the
                # qualification; it no longer blocks this one publication.
                if key in (getattr(args, 'accept_smoke_failure', None) or []):
                    print(f'Accepted failing smoke case {key}; recorded in the qualification', flush=True)
                    report['accepted_case_failures'].append(key)
                else:
                    report['unresolved_case_failures'].append(key)
        if report['unresolved_case_failures']:
            raise RuntimeError('Current engine did not complete its actual MCC frame check: ' +
                               ', '.join(report['unresolved_case_failures']))
        frontend_record = None
        if (maps / 'mainmenu.map').is_file():
            from qualify_frontend_runtime import qualify
            frontend_record = qualify(root, runtime, cache[0], output / 'frontend', scratch,
                                      report['eos_worker'])
            atomic_json(destination / 'frontend-selection.json', frontend_record)
            report['frontend'] = dict(passed=True, runtime=frontend_record['runtime'])
        if renderer_controls:
            report['renderer_4k'] = dict(passed=False, report=str(output / 'renderer-4k/report.json'))
            report['renderer_4k'].update(renderer_4k_qualification(
                source, runtime, maps, output / 'renderer-4k', scratch, renderer_controls))
            if report['renderer_4k'].get('passed') is not True:
                raise RuntimeError('Packaged renderer 4K frame qualification failed; existing launcher selection retained')
            if digest(runtime) != executable_pin:
                raise RuntimeError('Packaged renderer executable changed after its compiled shader qualification')
        if (source_hashes(source) != pins or custom_asset_inventory(asset_root) != asset_pins or
                (asset_manifest_pin is not None and digest(manifest) != asset_manifest_pin) or
                any(digest(maps / name) != pin for name, pin in map_pins.items()) or
                any(digest(maps / name) != pin for name, pin in grenade_map_pins.items())):
            raise RuntimeError('Frozen engine source or MCC maps changed during validation; existing runtime retained')
        if not prepare_only and (selection.read_bytes() if selection.is_file() else None) != previous:
            raise RuntimeError('Another task changed the launcher selection during validation; its selection retained')
        if not prepare_only and frontend_record and (frontend_selection.read_bytes() if frontend_selection.is_file() else None) != previous_frontend:
            raise RuntimeError('Another task changed the frontend selection during validation; its selection retained')
        atomic_json(destination / 'source-hashes.json', pins)
        validate_eos_worker(runtime, report['eos_worker'])
        if digest(runtime) != executable_pin:
            raise RuntimeError('Packaged renderer executable changed before final qualification sealing')
        inventory = {p.relative_to(destination).as_posix(): dict(sha256=digest(p), bytes=p.stat().st_size)
                     for p in destination.rglob('*') if p.is_file()}
        atomic_json(destination / 'runtime-files.json', inventory)
        report.update(passed=True, status='prepared', qualified_candidate=True, runtime_sha256=executable_pin,
                      retained_bytes=sum(p.stat().st_size for p in destination.rglob('*') if p.is_file()))
        atomic_json(destination / 'qualification.json', report)
        record = dict(schema='H2_LOCAL_GAMEPLAY_RUNTIME_SELECTION_V1', runtime=report['runtime'],
                      runtime_sha256=report['runtime_sha256'],
                      qualification=(destination / 'qualification.json').relative_to(root).as_posix(),
                      qualification_sha256=digest(destination / 'qualification.json'),
                      inventory=(destination / 'runtime-files.json').relative_to(root).as_posix(),
                      inventory_sha256=digest(destination / 'runtime-files.json'),
                      eos_worker=report['eos_worker'],
                      cache=cache[0].relative_to(root).as_posix(), maps=map_pins,
                      default_map=default_map, local_players=players)
        if prior and prior.get('video_driver'):
            record['video_driver'] = prior['video_driver']
        candidate_selection = destination / 'selection.json'
        atomic_json(candidate_selection, record)
        admission = dict(selection_name=candidate_selection.relative_to(root).as_posix())
        if cache_candidate is not None:
            admission['cache_candidate'] = cache[0]
        selected_gameplay_runtime(root, **admission)
        candidate_qualified = True
        if prepare_only:
            # Qualification and the actual admission reader have passed.
            # Keep this explicit candidate without adopting any ROOT selection.
            prepared = True
            print(f'Qualified candidate prepared at {runtime}\nROOT launchers/selections were not published; '
                  f'retained runtime/resources: {report["retained_bytes"]} bytes', flush=True)
        else:
            # Existing launchers can finish using their inode; the publication
            # transaction also preserves them if a later root write fails.
            publication = dict(cache_choice=cache[0]) if cache_candidate is not None else {}
            publish_runtime_files(root, destination, scratch, record, frontend_record, report,
                                  publication_baseline, **publication)
            promoted = True
            report.update(status='published', runtime_installed=True)
            print(f'Both launchers now select {runtime}\nRetained runtime/resources: {report["retained_bytes"]} bytes', flush=True)
    except BaseException as error:
        report.update(passed=False, status='failed', qualified_candidate=candidate_qualified, error=str(error))
        raise
    finally:
        if not (promoted or prepared) and destination.exists():
            publication = report.setdefault('publication', {})
            references = candidate_references(root, destination, publication)
            publication['candidate_references'] = references
            publication['candidate_retained'] = bool(references or publication.get('rollback_failed'))
            if publication['candidate_retained']:
                report['retained_bytes'] = sum(p.stat().st_size for p in destination.rglob('*') if p.is_file())
            else:
                shutil.rmtree(destination)
        atomic_json(output / 'report.json', report)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/launcher-engine-build/latest')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--config', choices=('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'),
                        default='Release', help='CMake build configuration (default: Release)')
    parser.add_argument('--timeout', type=float, default=180)
    parser.add_argument('--qualify-map', action='append', default=[], metavar='NAME.map',
                        help='Also require this MCC multiplayer cache to pass submitted-frame qualification')
    parser.add_argument('--status', action='store_true')
    parser.add_argument('--runtime', type=Path)
    parser.add_argument('--eos-client-config', type=Path,
                        help='Override shared game settings; defaults to the selected ONLINE runtime or local EOS setup')
    parser.add_argument('--weapon-audio-reference-xbe', type=Path,
                        help='Require component and repeated MCC weapon audio checks using this original reference XBE')
    parser.add_argument('--tu15-gameplay-reference-xbe', type=Path,
                        help='Owned English TU1.5 image for original-data CTests; defaults to the authenticated installed DLC import')
    parser.add_argument('--worker', action='store_true', help=argparse.SUPPRESS)
    parser.add_argument('--reuse-owned-build', action='store_true',
                        help='Reuse source/build inside an active run_test.py coordinator; requires --worker')
    parser.add_argument('--accept-smoke-failure', action='append', default=[], metavar='MAP:PLAYERS',
                        help='Publish even if this smoke case fails (for example elongation.map:1); '
                             'the failure is recorded as accepted in the qualification')
    parser.add_argument('--prepare-only', action='store_true',
                        help='Qualify and retain a candidate without publishing ROOT launchers or selections')
    parser.add_argument('--baseline-selection',
                        help='Validate this project-relative qualified selection as the preparation baseline; requires --prepare-only')
    parser.add_argument('--baseline-cache', type=Path,
                        help='Verify the named baseline against its own private MCC cache; requires --baseline-selection')
    parser.add_argument('--frozen-custom-asset-root', type=Path,
                        help='Use a verified staged custom asset source inside the active owned preparation')
    parser.add_argument('--terminal-display-checks', action='store_true',
                        help='Require the selected terminal machine, portal and object LOD reference checks')
    parser.add_argument('--cache-candidate', type=Path,
                        help='Qualify this verified private MCC cache without selecting it during preparation')
    parser.add_argument('--native-reference-cache', type=Path,
                        help='Use this verified private MCC cache for shader reference checks; requires --prepare-only')
    parser.add_argument('--renderer-4k-reference-control', action='append', default=[], metavar='NAME=VALUE',
                        help='Require four Original/Off 4K parity cases on this exact packaged runtime versus these reference environment controls before publication (repeatable)')
    args = parser.parse_args()
    if args.reuse_owned_build and (not args.worker or not os.environ.get('HALO2_TEST_SCRATCH')):
        parser.error('--reuse-owned-build requires --worker inside an active run_test.py coordinator')
    if args.baseline_selection and not args.prepare_only:
        parser.error('--baseline-selection requires --prepare-only')
    if args.baseline_cache and not args.baseline_selection:
        parser.error('--baseline-cache requires --baseline-selection')
    if args.native_reference_cache and not args.prepare_only:
        parser.error('--native-reference-cache requires --prepare-only')
    if args.frozen_custom_asset_root and not (args.prepare_only and args.reuse_owned_build):
        parser.error('--frozen-custom-asset-root requires --prepare-only and --reuse-owned-build')
    if args.status:
        if args.runtime and (args.root / 'CMakeLists.txt').is_file():
            print(source_status(args.root, args.runtime))
        return 0
    if args.jobs < 1 or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error('Positive build jobs and a finite positive smoke timeout are required')
    try:
        renderer_reference_controls(args.renderer_4k_reference_control)
    except ValueError as error:
        parser.error(str(error))
    if sys.platform != 'linux':
        parser.error('Source engine promotion currently supports Linux only')
    if not args.worker:
        return subprocess.call([sys.executable, '-B', str(ROOT / 'tools/run_test.py'),
                                '--scratch-root', str(args.root / '.local/test-scratch'), '--',
                                sys.executable, '-B', str(Path(__file__).resolve()), '--worker', *sys.argv[1:]])
    signal.signal(signal.SIGTERM, lambda number, frame: sys.exit(128 + number))
    try:
        worker(args)
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        print('Launcher engine build: ' + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
