#!/usr/bin/env python3
"""Fold literal executable operands using the canonical generated mapping.

Only a build-local copy changes. Dynamic/unmapped calls retain the original
checked function, and each replacement is recorded in a bounded receipt. The
original instruction order, state operations and service calls stay intact.
"""
import argparse
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import re

HEADER = 'std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image)'
MAPPING = re.compile(re.escape(HEADER) +
    r'\{if\(image==SourcePhysicsContactImage::MapPack11\)return p;switch\(p\)\{'
    r'(?P<cases>(?:case 0x[0-9a-f]+u:return 0x[0-9a-f]+u;)+)'
    r'default:throw std::runtime_error\("[^"\n]*"\);}}')
CASE = re.compile(r'case (0x[0-9a-f]+)u:return (0x[0-9a-f]+)u;')
CALL = re.compile(r'\bselected\(\s*(0x[0-9a-f]+)u\s*,\s*'
                  r'([A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*)\s*\)')
INSTRUCTION = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')
REGISTER_POP = re.compile(r'^s\.instruction_literal\((0x[0-9a-f]+)u\); s\.set\(([0123567]),s\.pop\(\),4,0\);$', re.M)
NON_CODE = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
CURSOR_STATE_HEADERS = ('physics/runtime/physics_contact_runtime_internal.hpp', 'physics/runtime/physics_world_runtime_internal.hpp',
    'physics/vehicles/physics_vehicle_runtime_internal.hpp', 'physics/vehicles/physics_vehicle_motion_internal.hpp',
    'physics/vehicles/physics_vehicle_motion_math_internal.hpp')


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def group_literal_stack_pops(output):
    # Full-line matching retains labels and their independently reachable
    # entries. The +1 source PC check is the encoding for these register POPs.
    service_sites = {0x38846e, 0x21f590, 0x33b595}
    code = NON_CODE.sub(lambda match: ' ' * len(match[0]), output)
    runs = []
    for match in REGISTER_POP.finditer(code):
        pc = int(match[1], 16)
        if pc in service_sites:
            continue
        if runs and runs[-1][-1].end() + 1 == match.start() and int(runs[-1][-1][1], 16) + 1 == pc:
            runs[-1].append(match)
        else:
            runs.append([match])
    parts, start, groups = [], 0, 0
    for run in runs:
        if not 2 <= len(run) <= 5:
            continue
        first, last = run[0], run[-1]
        registers = ','.join(match[2] for match in run)
        # Keep fallback lines outside the canonical grammar so processing a
        # prepared source with other retained callbacks stays idempotent.
        fallback = '\n'.join('    ' + line for line in output[first.start():last.end()].splitlines())
        parts.extend((output[start:first.start()], f'if(!s.try_pop_registers<{registers}>({last[1]}u)){{\n',
                      fallback, '\n}'))
        start = last.end()
        groups += 1
    parts.append(output[start:])
    return ''.join(parts), groups


# Native leaf kernels (src/physics/runtime/native_tu15_matrix_leaves.hpp),
# bound to the exact canonical bodies they replace (state type normalized to
# STATE). A changed or unknown body keeps its translated route.
LEAF_INCLUDE = '#include "physics/runtime/native_tu15_matrix_leaves.hpp"\n'
BSP_LEAF_INCLUDE = '#include "physics/runtime/native_tu15_bsp_leaves.hpp"\n'
BROADPHASE_LEAF_INCLUDE = '#include "physics/runtime/native_tu15_broadphase_leaves.hpp"\n'
PORTAL_LEAF_INCLUDE = '#include "physics/runtime/native_tu15_portal_predicate_leaves.hpp"\n'
BOUNDS_LEAF_INCLUDE = '#include "physics/runtime/native_tu15_broadphase_bounds_leaves.hpp"\n'
TEMPORARY_ABI_INCLUDE = '#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"\n'
ARRAY_ABI_INCLUDE = '#include "physics/runtime/native_tu15_array_abi.hpp"\n'
BROADPHASE_REMOVE_INCLUDE = '#include "physics/runtime/native_tu15_broadphase_remove.hpp"\n'
BROADPHASE_REMOVE_PIN = '05e43e089a2d68b9de10cdd71ea033cb1942290cf70a61a3c645332e2ee13b28'
BROADPHASE_ADD_INCLUDE = '#include "physics/runtime/native_tu15_broadphase_add.hpp"\n'
BROADPHASE_INSERT_PIN = 'd6345ef66de6fb1b13280d965e7110b8c306f22ea61c60c06b5935d47afc6c2f'
BROADPHASE_ADDITION_INCLUDE = '#include "physics/runtime/native_tu15_broadphase_add_object.hpp"\n'
BROADPHASE_ADDITION_PIN = 'eefb2790b72700eef45d327ca0048a1efd2ab9bd5b97b1a89842d6979cb26bb3'
POOL_ALLOCATOR_INCLUDE = '#include "physics/runtime/native_tu15_pool_allocator_abi.hpp"\n'
POOL_RELEASE_PIN = '6db190f9f0c502059c3959d31c52f045182f642bb9adbe6e2ae231196408404c'
POOL_ALLOCATION_PIN = '798542b1fe41446e7eccb7618950a624dbff19a9612066611a03c37ce82f479b'
ARRAY_ALLOCATOR_INCLUDE = '#include "physics/runtime/native_tu15_array_allocator_abi.hpp"\n'
ARRAY_ALLOCATOR_ENTRIES = {
    '148a10': ('72002f4da21c2acb36286b36809b9f5dcf92a41bb3a7dcf1cd4c6b424fc69b68','allocate','149da0'),
    '148a70': ('cdef2db04a49c45c541f9aacd44839ec8036a532e87622bdbc75a6319ab7339a','release','149e00'),
}
STACK_ARENA_INCLUDE = '#include "physics/runtime/native_tu15_stack_arena_abi.hpp"\n'
STACK_ARENA_ENTRIES = {
    '2e10a0': ('c1ca294932e90476c71618e9c24faf2c912632c0a9e1ee6c29b7f3a5ddb9076a','allocate'),
    '2e1110': ('d377b74d4008703ea138d1604770181a53cbd262950bdd06223df60d41385afb','release'),
}
MAIN_HEAP_INCLUDE = '#include "physics/runtime/native_tu15_main_heap_abi.hpp"\n'
MAIN_HEAP_ENTRIES = {
    '22e380': ('3ab21efb89135cb4f33b43af186b40ad9dc68d389f2a7f0f6e60951649fa6657','remove_allocation','232c80'),
    '22e420': ('059a0daf55e7e819cbca46b43679e443d77cea578f2df0467ad287c123ba27c8','release','232d20'),
}
MAIN_HEAP_ALLOCATION_INCLUDE = '#include "physics/runtime/native_tu15_main_heap_allocate_abi.hpp"\n'
MAIN_HEAP_ALLOCATION_ENTRIES = {
    '22e1d0': ('f9daae5d1a669d1eaf5d47d741e7852cc31b0eaa8be298fb306bb76db2832730','allocate','232ad0'),
    '22e400': ('666a315b334fe888a6fdf9c53a517e1ae5aaf279fe02c9138aa0bad4c89e2200','allocate_sized','232d00'),
    '22e430': ('601dd12505fe3191bd7a875709276985589476bc516eba97acd3f4f4e9881edd','probe_gap','232d30'),
}
RAW_ALLOCATOR_INCLUDE = '#include "physics/runtime/native_tu15_raw_allocator_abi.hpp"\n'
RAW_ALLOCATOR_ENTRIES = {
    '149bf0': ('03ac6fd16ecb8c3503f31f371e46eb0783b8baae5dde715a209e26bb940ef6ae','allocate'),
    '149c40': ('5d1237c330a03699fde8f3bd593359ebd7e03c8cd3fb3d8b5ef877efd4bf9dba','release'),
    '233160': ('ca120647f371729b1b296b451f436d85262a7c8fa77bc7dc80ec53cf597641f1','allocate_tagged'),
    '2331a0': ('eec890a6d0fde139830e4b0533f52f66c10c5ddc79ae11edba4858b197277539','release_tagged'),
}
ALLOCATOR_OWNER_INCLUDE = '#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"\n'
ALLOCATOR_OWNER_DISPATCH_PIN = '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'
# The fresh generator additionally routes the already recovered FA5D0 body.
# Its exact removal must restore the original complete dispatcher. This is
# routing authentication; the seven-instruction FA5D0 owner stays translated.
ALLOCATOR_OWNER_FRESH_DISPATCH_PIN = 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'
ALLOCATOR_OWNER_FRESH_CASE = 'case 0xfa5d0u:run_fa5d0(s);return;'
ALLOCATOR_OWNER_FRESH_BODY_PIN = 'caa1c7398c487d028f00ed11ed4a20a15937a234e7b424a4ea15406150796346'
# Original TU1.5 bytes for actual owner entries lacking a generated body.
# The independent physical validator authenticates each whole byte sequence.
ALLOCATOR_ACTUAL_OWNER_BYTES = {
    0x232aa0:'8b5424088b01528b542408526a01ff5008c20800',
    0x232ac0:'8b01ff600c',0x2e1030:'e92ba5eeff',
}
TICK_CONVERSION_ACTUAL_OWNER_IMAGES = {
    0x22a5c0:(100,'32d5a2d8e508ef41dc6e366efaf26e8aab00aa9f10156a9c5f4aad550e5a9830'),
    0x32ae70:(170,'8964dcf0d769cb61dd4354feac00491ae8c1b34eb9627a4c5ec2d4350243d395'),
    0x32acd0:(52,'3d0898d998acc8af3ea17032ff56fd1b963c685b072f669c52d4b8267f9d0a78'),
}
PHYSICAL_EPOCH_ACTUAL_OWNER_IMAGES = {
    0x12f040:(122,'ab1d322d207234f005c2eb22e9f29da5aa35e7d0990dc92e0eb03da160d3889c'),
    0x12f0c0:(73,'a60b8f3171bddd5813033c2adbd04502bae86a9b4bb2556372a35162918fd8fb'),
    0x13fd10:(120,'f6eee0fd7c7a53a33a058ab7903d7ef6b713e896f7d570f5f7e5a93012bbea29'),
}
BSP_SURFACE_2D_ACTUAL_OWNER_IMAGES = {
    0x2a0230:(94,'c362b64b0f3c686f7f63e8e15b269d313d89019acf82bcb33f6c78661f2cfa07'),
}
BSP_SURFACE_POLYGON_ACTUAL_OWNER_IMAGES = {
    0x1e6e80:(303,'bc6e0eca62f0fef7c1384acf728217722636e0fa37827dd294461b2568e890ed'),
}
DATUM_DELETE_ACTUAL_OWNER_IMAGES = {
    0x16e8a0:(146,'23b798b8683109fa7356d9ad3286103f8a10c6b359d4913263b3565af7392996'),
}
STACK_PROBE_ACTUAL_OWNER_IMAGES = {
    0x32a660:(65,'b77ad187f6f3db2a5ae4f8e214b023d7c62ce25a38ce769a993d2a8c81643760'),
}
RESOURCE_BUSY_ACTUAL_OWNER_IMAGES = {
    0x12fcb0:(83,'1df96e347163f4151ccb82bf996d1aea2abae0e06e9c425b1525e2b450b4861a'),
    0x4057f0:(130,'26b21d4d8c3d411324b2a5c9f96dff30ef8103fe860c412947cdd57bf6cd6e8b'),
}
OBJECT_PREDICTION_COPY_ACTUAL_OWNER_IMAGES = {
    0xadcd0:(27,'514e40a41f0f14fb43d196e57373d3444e54db20f032faa895eb6621c1494a2c'),
}
ARRAY_ABI_ENTRIES = {
    '2df710': ({'db5cb2be041e3c5e58e52ad598ab7dd1ec0661ed1d8507123bb0263b4e8861f0',
        '5193583aca1257afd2c4dfb91f2e4c53942a25081221bdccaeb8767e9a5f87cf'}, 'reserve', '2e31e0'),
    '2df780': ({'d6ecec0126c79e4b734918c7642be8218cea4e6b28292d75eca7b28261e61c1b',
        '6bc91e7a2f8a56f78f976cf2b6b69ee860aaab014c3cc5ab2c3b0e42cfecdbfc'}, 'grow', '2e3250'),
    '2e3250': ({'776370c7b81df39130c8d0d8595682fddeea1a2426bb7f544bc2a89d5c431a42'}, 'grow', '2e3250'),
    '2df360': ({'108d18b60e51fa6a6908ce08a1bf9e7b4fb87001b7347c6baf19ee7289481b92',
        '8ad8ca8252bd5ca0a5dbc6c45eb31e8e34ac7d696deb1d735700e8e2ae3585f9'}, 'copy', '2e2e30'),
    '2e2e30': ({'8b7462e723269692792bdc0754da9d9512474204c2eb2335ce34f6f6951d19c3'}, 'copy', '2e2e30'),
}
TEMPORARY_ABI_ENTRIES = {
    '2e7da0': ('4adb3703ae6017c570f5c9070e773390b74d2a037666d64eec5a352a62fed5f2', 'construct', '2eba20'),
    '2e8090': ('d38beefa42c7df62021c16fea3fab90b62ad0ee2d0947897c0abad3315d7ae3e', 'destroy', '2eb850'),
}
LEAVES = {
    # Second digests: the identical bodies with identity-mapped selected()
    # operands (AI frame, scene device, effect particle units).
    '145560': (('3ce39c1046f43fec27fe7f65c35755ca56024dcdae65bdebd4ec4e2377171b62', '6ab5af55d5cce6547098d13f85c4169b2dc1364b50c382161698df28bac38169'), 'kernel_145560', 0x14566e),
    '144cf0': (('c6d3ad70a57319f692018045a3de73c341b85bf996fbd1330fcd7e9afc1fc032', 'a12dcc069c07b58c31058558e0e6ea247781974bd56ca73d0c3872affb17cf58'), 'kernel_144cf0', 0x144e4e),
    '2e44d0': (('9f79312f94c4df9caf23dd8e6c012cefe64749e9f7548b557d7375d8618919f1',
                '8f1da631535132b0d8777a0a859b8a8b2aafb2088ff67ec211595c04c2c09c1f'), 'kernel_2e44d0', 0x2e4540),
    # The World copy uses canonical symbols and can publish selected TU1.5 PCs.
    '2e09e0': ('6a65335e40158ccee2ab6add74c0ed2d8ae4ce30791068a36abf4c293257747f', 'kernel_2e44d0', 0x2e4540),
    '2e46e0': ('198a8866775ff17db3d72b629e04d42e4b9772cddbe9e79257d986711ee667c9', 'kernel_2e46e0', 0x2e4753),
    '14cdb0': ('f747aad6a36fcee70a3af5ab504d7fa8d9027e392c2175bee747f0bbb2752e30', 'kernel_14cdb0', 0),
    '14ce30': ('e88ee3cb90b6c51bf61c5eeed47db65740c8eef039f31ae55adb65a05f55d069', 'kernel_14ce30', 0),
    '2e9df0': ('6d0173a5660575e2d10e605580b31fa4c5eaca12deb0f83607575e41b80dd3f7', 'kernel_2e9df0', 0x2e9ee6),
    '2e6240': ('2f8cc6c161f3f2f220916571455d2f5b57a11488928c48298d61b7c58ea086c0', 'kernel_2e9cf0', 0x2e9de6),
    '2e9fd0': ('19cb3e6987d8e8dab83d61963c5faf298ebf3839676d0a899d99e6079f3eb972', 'kernel_2e9fd0', 0x2ea020),
    '14cea0': (('8f35497269f2009c9b3c3d453e4b606b2d3abc8b09e573544fed5a69ebb628aa', 'f7ef481b3620719039ba8356992ba31ce37a8e980e418542ee0b65c9d74e9c74'), 'kernel_14cea0', 0),
    '123210': (('8728c0405f5059e10d21fbbba7ccdee7494e1cd16f0c272f2884a3357c0b43ae', 'be69075d1612d6662186c8845da8834e82af5919cf744361842417c378754ebb'), 'kernel_123210', 0),
    '243450': (('a0308dda34f7a7f07103c6863266b0ba6a263b9d73f4e6d6f140226a700bb659', 'c966df7ae118b3c92387ceadfb02d3368e01f0189cba0f89e0fa23387063266a'), 'kernel_243450', 0),
    '2ea050': ('b60d187abad84cd9544150fb40183325fa20cbd6e3af70d32e6883d159ded7c2', 'kernel_2ea050', 0x2ea146),
    '2eb0c0': ('46e52cdff199e798899b7fb86f9c83e8ff00c93a0819efc0546fb061d1e2470a', 'kernel_2eb0c0', 0x2ea146),
    # Scene device point/segment distance (SSE + x87 FSQRT/FABS/FSTP); its
    # two exits publish their own final cursor.
    '14dd70': ('9bd118cbb828a5c8926d33d736b5409c12a1f585699fb25fa2b43fc7bf3ec4e6', 'kernel_14dd70', 0),
}


def leaf_body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def spatial_owner_bindings(canonical,output):
    # Native spatial owners (PVS closure 1532A0, object sphere query BD080,
    # object tick B8F40/B9090/B9180/BE4A0/10DD10, object tick children
    # BE850/BE610/C0F60/10DD80/C0200/1D5790/1E15D0, cluster distance 131470,
    # collision ray 16C620).
    # Loaded beside this file so callers without tools on sys.path keep working.
    installed=[]
    for marker,name in (('static void run_1532a0(','native_pvs_flood_replacements'),
                        ('static void run_bd080(','native_object_sphere_query_replacements'),
                        ('static void run_b8f40(','native_object_tick_replacements'),
                        ('static void run_be850(','native_object_subtree_replacements'),
                        ('static void run_131470(','native_cluster_distance_replacements'),
                        ('void source_run_campaign_environment_tu15(','native_fog_accumulation_replacements'),
                        ('void source_run_campaign_environment_tu15(','native_fog_resolution_replacements'),
                        ('void source_run_campaign_environment_tu15(','native_fog_cluster_finish_replacements'),
                        ('void source_run_campaign_environment_tu15(','native_fog_projected_merge_replacements'),
                        ('void source_run_campaign_environment_tu15(','native_fog_cluster_sky_replacements'),
                        ('void source_run_campaign_environment_tu15(','native_fog_remaining_replacements'),
                        ('void source_run_campaign_environment_tu15(','native_fog_plane_classifier_replacements'),
                        ('void source_run_campaign_environment_tu15(','native_fog_plane_projection_replacements'),
                        ('void source_run_campaign_environment_tu15(','native_fog_plane_output_replacements'),
                        ('void source_run_campaign_environment_tu15(','native_fog_plane_transform_replacements'),
                        ('void source_run_campaign_environment_tu15(','native_fog_plane_call_replacements'),
                        ('void source_run_campaign_environment_tu15(','native_patchy_fog_replacements'),
                        ('static void run_16c620(','native_collision_ray_replacements'),
                        # Device update callbacks 1179C0/10B840 (scene device unit).
                        ('static void run_1179c0(','native_device_update_replacements'),
                        ('static void run_1c58e0(','native_campaign_ai_workspace_replacements'),
                        # Group L live-support leaves: the helper matches each pinned body
                        # in any unit (C9550, 1EC970, 1F7F60, 171010, C1E90, 110350).
                        ('static void run_','native_live_support_leaves_replacements'),
                        ('static void run_305560(State','native_collision_cache_305560_contact_replacements'),
                        ('void source_run_physics_live_support_tu15(','native_object_hierarchy_root_replacements'),
                        ('void source_run_physics_live_support_tu15(','native_collision_pair_2f88d0_replacements'),
                        # Group F frame/gameplay owners: the helper matches each pinned
                        # owner and its unit itself (1306B0, D7230, 1EEE80, ...).
                        ('static void run_','native_frame_misc_replacements'),
                        # Group E effect owners: the helper matches each pinned closure
                        # in its unit itself (20680, C35D0, 176A40).
                        ('static void run_','native_effect_owner_replacements'),
                        ('static void run_d73e0(ObservedDamageState','native_damage_update_entry_replacements'),
                        ('static void run_d73e0(ObservedDamageState','native_damage_update_return_replacements'),
                        ('static void run_d73e0(ObservedDamageState','native_damage_update_sections_replacements'),
                        ('static void run_d73e0(ObservedDamageState','native_damage_descriptor_refresh_replacements'),
                        ('static void run_d73e0(ObservedDamageState','native_damage_due_response_replacements'),
                        ('static void run_d73e0(ObservedDamageState','native_damage_response_amount_replacements'),
                        ('static void run_d73e0(ObservedDamageState','native_damage_response_secondary_replacements'),
                        ('static void run_d73e0(ObservedDamageState','native_damage_update_notification_replacements'),
                        ('static void run_d73e0(ObservedDamageState','native_damage_update_vitality_gate_replacements'),
                        ('static void run_d73e0(ObservedDamageState','native_damage_update_flag_route_replacements'),
                        ('static void run_d73e0(ObservedDamageState','native_damage_update_amounts_replacements'),
                        ('static void run_d73e0(ObservedDamageState','native_damage_update_shield_recovery_replacements'),
                        ('static void run_1778d0(WorldState','native_particle_system_update_replacements'),
                        ('static void run_1778d0(WorldState','native_particle_system_attachment_replacements'),
                        ('static void run_1778d0(WorldState','native_particle_system_selection_replacements'),
                        ('static void run_1778d0(WorldState','native_particle_system_contact_basis_replacements'),
                        ('static void run_1778d0(WorldState','native_particle_system_velocity_basis_replacements'),
                        ('static void run_178240(WorldState','native_particle_drain_counts_replacements'),
                        ('static void run_2c40d0(WorldState','native_particle_physics_shapes_replacements'),
                        ('static void run_250ac0(WorldState','native_particle_transform_basis_replacements'),
                        ('static void run_250ac0(WorldState','native_particle_transform_projection_replacements'),
                        ('static void run_250ac0(WorldState','native_particle_transform_matrix_calls_replacements'),
                        ('static void run_22ad60(WorldState','native_particle_emitter_bound_replacements'),
                        ('static void run_251f70(WorldState','native_particle_update_location_transforms_replacements'),
                        ('static void run_251650(WorldState','native_particle_cleanup_251650_replacements'),
                        ('static void run_20b90(WorldState','native_lens_query_walk_replacements'),
                        ('static void run_2c2d90(WorldState','native_particle_contact_walk_replacements'),
                        ('static void run_2c2d90(WorldState','native_particle_contact_flags_replacements'),
                        ('static void run_2c2d90(WorldState','native_particle_contact_query_replacements'),
                        ('static void run_2c2d90(WorldState','native_particle_contact_response_replacements'),
                        ('static void run_2c2d90(WorldState','native_particle_contact_actions_replacements'),
                        ('static void run_17bb10(WorldState','native_particle_location_update_replacements'),
                        ('static void run_1772c0(WorldState','native_particle_draw_entry_replacements'),
                        ('static void run_174bf0(WorldState','native_particle_observer_lens_replacements'),
                        ('static void run_178110(WorldState','native_particle_location_prepare_replacements'),
                        ('static void run_252480(WorldState','native_particle_velocity_properties_replacements'),
                        ('static void run_252480(WorldState','native_particle_velocity_direction_replacements'),
                        ('static void run_252480(WorldState','native_particle_velocity_angles_replacements'),
                        ('static void run_252480(WorldState','native_particle_velocity_remaining_replacements'),
                        ('static void run_251c80(WorldState','native_particle_observer_lod_replacements'),
                        ('static void run_251c80(WorldState','native_particle_location_emitters_replacements'),
                        ('static void run_251c80(WorldState','native_particle_location_entry_replacements'),
                        ('static void run_250c60(WorldState','native_particle_birth_prefix_replacements'),
                        ('static void run_250c60(WorldState','native_particle_birth_placement_replacements'),
                        ('static void run_250c60(WorldState','native_particle_birth_interface_replacements'),
                        ('static void run_250780(WorldState','native_emitter_transform_replacements'),
                        ('static void run_250780(WorldState','native_emitter_emission_replacements'),
                        ('static void run_250780(WorldState','native_emitter_orientation_replacements'),
                        # Group H physics world leaves (1F5FE0, 2E4550, 303BA0, 2E47B0),
                        # physics world unit only.
                        ('static void run_1f16f0(ObservedWorldState','native_world_leaves_replacements'),
                        ('static void run_2e0ba0(ObservedWorldState','native_world_position_transform_2e4690_replacements'),
                        ('static void run_22ecb0(ObservedWorldState','native_world_pressure_2335b0_replacements'),
                        ('static void run_32e35b(MathState','native_vehicle_x87_dispatch_331ecb_replacements'),
                        ('static void run_209a50(MotionState','native_vehicle_wheel_offset_20e414_replacements'),
                        ('static void run_209a50(MotionState','native_vehicle_wheel_cross_product_20e481_replacements'),
                        ('static void run_209a50(MotionState','native_vehicle_wheel_force_20e4f4_replacements'),
                        ('static void run_209a50(MotionState','native_vehicle_wheel_response_20e6c3_replacements'),
                        ('static void run_209a50(MotionState','native_vehicle_wheel_output_20e91f_replacements'),
                        ('static void run_209a50(MotionState','native_vehicle_scratch_reset_20ea32_replacements'),
                        ('static void run_209a50(MotionState','native_vehicle_wheel_curve_20e84a_replacements'),
                        ('static void run_209a50(MotionState','native_vehicle_contact_output_20f6e0_replacements'),
                        ('static void run_209a50(MotionState','native_vehicle_contact_projection_20ee92_replacements'),
                        ('static void run_209a50(MotionState','native_vehicle_contact_query_20f3a8_replacements'),
                        ('static void run_209a50(MotionState','native_vehicle_contact_collection_20ea64_replacements'),
                        ('static void run_1dc940(MotionState','native_vehicle_marker_emission_1e1200_replacements'),
                        ('static void run_b9c20(MotionState','native_vehicle_marker_matrix_bad60_replacements'),
                        ('static void run_3037f0(ObservedWorldState','native_soa_support_world_replacements'),
                        ('static void run_1c6c30(ObservedWorldState','native_collection_filter_world_replacements'),
                        ('static void run_2e22b0(ObservedWorldState','native_swept_support_world_replacements'),
                        # Group H 303EE0 contact record append (child 2E3250).
                        ('static void run_300400(ObservedWorldState','native_contact_array_replacements'),
                        # Group H 303C70/303D00 local shape buffers, then the 304350
                        # collection agent that runs them inline (after its children's binders).
                        ('static void run_3001d0(ObservedWorldState','native_local_buffers_replacements'),
                        ('static void run_300870(ObservedWorldState','native_collection_agent_replacements'),
                        # Group H 30CF40 manifold report and 304130 contact weld (304350's children).
                        ('static void run_309450(ObservedWorldState','native_manifold_replacements'),
                        ('static void run_309210(ObservedWorldState','native_contact_cache_update_30cd00_replacements'),
                        # Group H 2F33B0 key sort (2F36F0's child; recursion inline).
                        ('static void run_2ef8f0(ObservedWorldState','native_key_sort_replacements'),
                        # Group H 1F76A0 shape key filter (its jump table and 185AA0 call).
                        ('static void run_1f2db0(ObservedWorldState','native_shape_key_filter_replacements'),
                        # Group H 2F8630 closest point and 2E4FE0 swept pair (2E4550/2E4760 inline).
                        ('static void run_2f4bd0(ObservedWorldState','native_convex_pair_replacements'),
                        ('static void run_305560(ObservedWorldState','native_collision_cache_309060_replacements'),
                        # Havok active-island byte query in World and scene devices.
                        ('static void run_3121a0(ObservedWorldState','native_phantom_island_replacements'),
                        ('static void run_315c90(WorldState','native_phantom_island_replacements'),
                        ('static void run_3066e0(ObservedWorldState','native_mopp_quantized_aabb_30a1e0_replacements'),
                        # Group H 1F7C10 BSP child shape (1F76A0 and 1F5FE0 through their bound bodies).
                        ('static void run_1f3320(ObservedWorldState','native_bsp_child_shape_replacements'),
                        # Group H 1F6D10 structure AABB query (1F76A0 through its bound body; 2E4550 inline).
                        ('static void run_1f2420(ObservedWorldState','native_structure_aabb_query_replacements'),
                        # Group H 2F36F0 child agent update (2F33B0 through its bound body; 2E4550 and 30BAD0 inline).
                        ('static void run_2efc30(ObservedWorldState','native_child_agents_replacements'),
                        # Group H E8D80 object flag update (vehicle update unit; its update functions through the dispatch).
                        ('static void run_e7e60(VehicleState','native_object_flag_update_replacements'),
                        # Group H 1D7260 body motion snapshot (child 1D8B20 through the unit's route).
                        ('static void run_1d29a0(ObservedWorldState','native_body_motion_snapshot_replacements')):
        if marker not in canonical:continue
        spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(name+'.py'))
        helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
        if name == 'native_frame_misc_replacements':
            output, entries = helper.install(canonical, output, authored_residuals=True, authored_powered=True)
        else:
            output, entries = helper.install(canonical, output)
        installed += entries
    return output,installed


def page_window_bindings(text):
    current=leaf_body(text,'13fe00')
    if not current or 'if(halo2::native_page_window_detail::run(s))return;'not in current[0]:return[]
    # This module is also loaded directly with importlib by test and build
    # callers that do not put tools on sys.path. Resolve its optional sibling
    # against this owning file without changing the caller's import search.
    spec=importlib.util.spec_from_file_location('native_page_window_bindings',Path(__file__).with_name('native_page_window_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper.installed_entries(text)


def page_retire_bindings(text):
    current=leaf_body(text,'140310')
    if not current or 'if(halo2::native_page_retire_detail::run(s,'not in current[0]:return[]
    spec=importlib.util.spec_from_file_location('native_page_retire_bindings',Path(__file__).with_name('native_page_retire_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper.installed_entries(text)


def page_allocate_bindings(text):
    current=leaf_body(text,'13fe50')
    if not current or 'if(halo2::native_page_allocate_detail::run(s,'not in current[0]:return[]
    spec=importlib.util.spec_from_file_location('native_page_allocate_bindings',Path(__file__).with_name('native_page_allocate_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper.installed_entries(text)


def stack_probe_bindings(text):
    if 'if(halo2::native_stack_probe_detail::run(s'not in text:return[]
    spec=importlib.util.spec_from_file_location('native_stack_probe_bindings',Path(__file__).with_name('native_stack_probe_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper.installed_entries(text)


def resource_busy_bindings(text):
    if not any(current and 'if(halo2::native_resource_busy_detail::'in current[0]for current in(leaf_body(text,'12fcb0'),leaf_body(text,'4057f0'))):return[]
    spec=importlib.util.spec_from_file_location('native_resource_busy_bindings',Path(__file__).with_name('native_resource_busy_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper.installed_entries(text)


def halfword_bit_query_helper():
    spec=importlib.util.spec_from_file_location('native_halfword_bit_query_bindings',Path(__file__).with_name('native_halfword_bit_query_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper


def object_node_bindings(text):
    if 'if(halo2::native_object_node_rebuild_detail::bridge(s,'not in text and 'if(halo2::native_object_animation_detail::bridge(s,'not in text:return([],[])
    spec=importlib.util.spec_from_file_location('native_object_node_bindings',Path(__file__).with_name('native_object_node_rebuild_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    installed=helper.installed_entries(text)
    return([entry for entry in installed if entry=='bf0c0'],[entry for entry in installed if entry!='bf0c0'])


def install_halfword_bit_query(text,output):
    original=leaf_body(text,'fa5d0')
    if not original or original[1]!='WorldState':return output,[]
    helper=halfword_bit_query_helper();canonical=helper.canonical_body(original[0])
    # A second specialization may already have the exact cursor primitive.
    # Restore only that emitted statement shape before the whole-body pin.
    canonical=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',
        r'if(s.services.instruction){s.services.instruction(\1);}',canonical)
    if digest(canonical)!=helper.PIN:raise ValueError('Complete FA5D0 specialization body drift')
    current=leaf_body(output,'fa5d0');body=current[0];start=body.index('{')+1
    if not body[start:].startswith(helper.PREFIX):body=body[:start]+helper.PREFIX+body[start:]
    body=body.replace(helper.ORIGINAL_SHIFT,helper.ORDERED_SHIFT)
    output=output[:current.start()]+body+output[current.end():]
    if helper.INCLUDE not in output:output=helper.INCLUDE+output
    return output,['fa5d0']


def install_leaves(canonical, output):
    installed = []
    for name, (expected, kernel, final_pc) in LEAVES.items():
        original = leaf_body(canonical, name)
        if not original:
            continue
        state = original[1]
        pin = digest(original[0].replace(state + ' &s', 'STATE &s'))
        if pin not in (expected if isinstance(expected, tuple) else (expected,)):
            continue
        if name in ('14cea0', '123210', '243450', '145560', '144cf0', '14dd70', '2e44d0', '2e09e0'):
            mapping=MAPPING.search(canonical)
            addresses={int(a,16):int(b,16) for a,b in CASE.findall(mapping['cases'])}
            if any(addresses.get(int(call[1],16))!=int(call[1],16) for call in CALL.finditer(original[0])):
                continue
            if name=='2e09e0' and addresses.get(0x2e09e0)!=0x2e44d0:
                continue
        current = leaf_body(output, name)
        if not current:
            continue
        if name in ('2e44d0', '2e46e0'):
            # The recovered SSE destination owns dual-NaN payload selection.
            # Pin it in the translated reference as well as the native kernel;
            # callback removal otherwise lets the compiler swap operands.
            body = current[0].replace('_mm_add_ps(', 'halo2::native_tu15_leaves::ordered_add_ps(')
            body = body.replace('_mm_mul_ps(', 'halo2::native_tu15_leaves::ordered_mul_ps(')
            output = output[:current.start()] + body + output[current.end():]
            current = leaf_body(output, name)
        if name in ('14cea0', '243450'):
            body = current[0].replace('_mm_add_ss(', 'halo2::native_tu15_leaves::ordered_add_ss(')
            body = body.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')
            if name=='14cea0':
                body=body.replace('u(spill(s.fp.st(0))));s.fp.pop();',
                    'halo2::native_tu15_leaves::portal_spill_word(s.fp.st(0),s.fp.status));s.fp.pop();')
            output = output[:current.start()] + body + output[current.end():]
            current = leaf_body(output, name)
        if name=='2ea050':
            body=current[0].replace('u(spill(s.fp.st(0)))',
                'halo2::native_tu15_leaves::portal_spill_word(s.fp.st(0),s.fp.status)')
            output=output[:current.start()]+body+output[current.end():]
            current=leaf_body(output,name)
        if name in ('2e44d0','2e46e0','2ea050'):
            body=current[0].replace('vw(s.memory,','halo2::native_tu15_leaves::aligned_vw(s.memory,')
            body=body.replace('v(s.memory,','halo2::native_tu15_leaves::aligned_v(s.memory,')
            output=output[:current.start()]+body+output[current.end():]
            current=leaf_body(output,name)
        # An admission prefix inside the original body, as the normalize
        # bridge: the partitioner owns only hex-named bodies with the
        # canonical trailer, and the original instructions stay intact.
        start = current[0].index('{\n') + 2
        admission='s.selected_observation_pcs && ' if name in ('2e6240', '2e09e0') else ''
        if name=='14dd70':admission='halo2::native_tu15_leaves::x87_room(s,2u) && '
        override = ''
        if name=='2e09e0' or (name=='2e44d0' and pin==LEAVES['2e44d0'][0][1]):
            override = ('    static const bool reference_transform = [] {\n'
                        '        const auto* value = std::getenv("HALO2_REFERENCE_COLLISION_TRANSFORM_COMPOSE");\n'
                        '        return value && *value;\n'
                        '    }();\n')
            admission += '!reference_transform && '
        marker = (f'    // BEGIN native leaf{name}\n' + override +
                  f'    if({admission}halo2::native_tu15_leaves::run(s,[](auto &io,auto &m){{halo2::native_tu15_leaves::{kernel}(io,m);}},\n'
                  f'        [](auto &r){{run_{name}(r);}},0x{final_pc:x}u))return;\n'
                  f'    // END native leaf{name}\n')
        body = current[0][:start] + marker + current[0][start:]
        output = output[:current.start()] + body + output[current.end():]
        installed.append(name)
    if installed:
        output = LEAF_INCLUDE + output
        if any(name in installed for name in ('14cdb0', '14ce30')):
            output = BSP_LEAF_INCLUDE + output
        if any(name in installed for name in ('2e6240', '2e9df0', '2e9fd0')):
            output = BROADPHASE_LEAF_INCLUDE + output
        if any(name in installed for name in ('14cea0', '123210', '243450')):
            output = PORTAL_LEAF_INCLUDE + output
        if any(name in installed for name in ('2ea050','2eb0c0')):
            output = BOUNDS_LEAF_INCLUDE + output
    return output, installed


def install_temporary_abi(canonical,output):
    installed=[]
    bindings={0x37a5f8:0x37e0a8,0x487638:0x48c938,0x2e7df1:0x2eba71,
        0x2e80e0:0x2eb8a0,0x2e811a:0x2eb8da}
    mapping={int(a,16):int(b,16) for a,b in CASE.findall(MAPPING.search(canonical)['cases'])}
    for name,(pin,operation,actual) in TEMPORARY_ABI_ENTRIES.items():
        original=leaf_body(canonical,name)
        if not original or original[1]!='ObservedWorldState':
            continue
        if digest(original[0].replace(original[1]+' &s','STATE &s'))!=pin:
            continue
        if any(mapping.get(int(call[1],16))!=bindings.get(int(call[1],16)) for call in CALL.finditer(original[0])):
            continue
        current=leaf_body(output,name)
        start=current[0].index('{\n')+2
        prefix=(f'    if(halo2::native_partition_temporary_detail::{operation}(s,\n'
            '        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():]
        installed.append(actual)
    if installed:output=TEMPORARY_ABI_INCLUDE+output
    return output,installed


def install_array_abi(canonical,output):
    installed=[]
    bindings={0x487638:0x48c938,0x2df72f:0x2e31ff,0x2df745:0x2e3215,
        0x2df768:0x2e3238,0x2df7ae:0x2e327e,0x2df7c0:0x2e3290,0x2df7e3:0x2e32b3}
    mapping={int(a,16):int(b,16) for a,b in CASE.findall(MAPPING.search(canonical)['cases'])}
    for name,(pins,operation,actual) in ARRAY_ABI_ENTRIES.items():
        original=leaf_body(canonical,name)
        if not original or digest(original[0].replace(original[1]+' &s','STATE &s')) not in pins:
            continue
        if any(mapping.get(int(call[1],16))!=bindings.get(int(call[1],16),int(call[1],16)) for call in CALL.finditer(original[0])):
            continue
        actual_pcs='s.selected_observation_pcs' if original[1]=='ObservedWorldState' else ('true' if name.startswith('2e') else 'false')
        if operation=='copy':prefix=f'    if(halo2::native_array_abi_detail::copy(s,{actual_pcs}))return;\n'
        else:
            reserve='true' if operation=='reserve' else 'false'
            call_site='site+0x3ad0u' if name.startswith('2e') else 'site'
            prefix=('    if(halo2::native_array_abi_detail::run(s,\n'
                f'        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,{call_site},false);}}, {reserve},{actual_pcs}))return;\n')
        current=leaf_body(output,name);start=current[0].index('{\n')+2
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():];installed.append(actual)
    if installed:output=ARRAY_ABI_INCLUDE+output
    return output,installed


def specialize(text):
    mopp_aabb_entries=[]
    if 'static void run_305e40(ObservedWorldState &s)' in text:
        from native_mopp_aabb_replacements import installed_entries as mopp_aabb_bindings
        mopp_aabb_entries=mopp_aabb_bindings(text)
    mopp_aabb_canonical_entries=[]
    if 'static void run_305e40(State &s)' in text:
        from native_mopp_aabb_canonical_replacements import installed_entries as canonical_mopp_bindings
        mopp_aabb_canonical_entries=canonical_mopp_bindings(text)
    # Preserve the older complete MOPP owner's authenticated rejection order.
    polygon_support_entries=[]
    if 'halo2::native_polygon_support_detail::bridge(s)' in text or '#include "physics/runtime/native_tu15_polygon_support_frame.hpp"' in text:
        import native_polygon_support_replacements as polygon_support_owner
        from native_polygon_prepared_owners import installed as installed_polygon_owner
        installed_polygon_owner(text,polygon_support_owner,'support')
        polygon_support_entries=['1f6610']
    polygon_feature_entries=[]
    if 'halo2::native_polygon_features_detail::bridge(s)' in text or '#include "physics/runtime/native_tu15_polygon_features_abi.hpp"' in text:
        feature_spec=importlib.util.spec_from_file_location('native_polygon_feature_binding',Path(__file__).with_name('native_polygon_features_replacements.py'))
        feature_helper=importlib.util.module_from_spec(feature_spec);feature_spec.loader.exec_module(feature_helper)
        from native_polygon_prepared_owners import installed as installed_polygon_owner
        installed_polygon_owner(text,feature_helper,'features')
        polygon_feature_entries=['1f6690']
    simplex_triangle_entries=[]
    if 'halo2::native_simplex_triangle_detail::bridge(s)' in text or '#include "physics/runtime/native_tu15_simplex_triangle_source_owner.hpp"' in text:
        triangle_spec=importlib.util.spec_from_file_location('native_simplex_triangle_binding',Path(__file__).with_name('native_simplex_triangle_replacements.py'))
        triangle_helper=importlib.util.module_from_spec(triangle_spec);triangle_spec.loader.exec_module(triangle_helper)
        simplex_triangle_entries=triangle_helper.installed_entries(text)
    simplex_triangle_scene_entries=[]
    if 'halo2::native_simplex_triangle_detail::bridge_actual(s)' in text or '#include "physics/runtime/native_tu15_simplex_triangle_abi.hpp"' in text:
        scene_triangle_spec=importlib.util.spec_from_file_location('native_simplex_triangle_scene_binding',Path(__file__).with_name('native_simplex_triangle_scene_replacements.py'))
        scene_triangle_helper=importlib.util.module_from_spec(scene_triangle_spec);scene_triangle_spec.loader.exec_module(scene_triangle_helper)
        simplex_triangle_scene_entries=scene_triangle_helper.installed_entries(text)
    mapping = MAPPING.search(text)
    if not mapping or text.count(HEADER) != 1:
        raise ValueError('Unsupported canonical native operand mapping')
    pairs = [(int(a, 16), int(b, 16)) for a, b in CASE.findall(mapping['cases'])]
    addresses = dict(pairs)
    if len(addresses) != len(pairs):
        raise ValueError('Duplicate canonical native operand mapping')
    masked = NON_CODE.sub(lambda match: ' ' * len(match[0]), text)
    result, cursor, unknown = [], 0, 0
    counts = Counter()
    for call in CALL.finditer(masked):
        literal = int(call[1], 16)
        if literal not in addresses:
            unknown += 1
            continue
        translated = addresses[literal]
        replacement = (f'0x{literal:x}u' if literal == translated else
                       f'({call[2]}==SourcePhysicsContactImage::MapPack11?'
                       f'0x{literal:x}u:0x{translated:x}u)')
        result.extend((text[cursor:call.start()], replacement))
        cursor = call.end()
        counts[(literal, translated)] += 1
    result.append(text[cursor:])
    output = ''.join(result)
    cursor_calls = 0
    if any(f'#include "{header}"' in text for header in CURSOR_STATE_HEADERS):
        # These canonical states implement/inherit the same cursor contract.
        # Other generated modules keep their existing callback expression.
        masked = NON_CODE.sub(lambda match: ' ' * len(match[0]), output)
        pieces, cursor = [], 0
        for call in INSTRUCTION.finditer(masked):
            pieces.extend((output[cursor:call.start()], f's.instruction_literal({call[1]});'))
            cursor = call.end()
            cursor_calls += 1
        pieces.append(output[cursor:])
        output = ''.join(pieces)
    leaves = []
    if cursor_calls or '#include "physics/runtime/physics_world_runtime_internal.hpp"' in text:
        output, leaves = install_leaves(text, output)
    spatial_entries=[]
    if cursor_calls or '#include "physics/runtime/physics_world_runtime_internal.hpp"' in text:
        output,spatial_entries=spatial_owner_bindings(text,output)
    output,temporary_entries=install_temporary_abi(text,output)
    output,array_entries=install_array_abi(text,output)
    main_heap_entries=[]
    for name,(pin,operation,actual) in MAIN_HEAP_ENTRIES.items():
        original=leaf_body(text,name)
        if not original or digest(original[0].replace(original[1]+' &s','STATE &s'))!=pin:continue
        if not all(addresses.get(int(m[1],16))=={0x22e42a:0x232d2a}.get(int(m[1],16)) for m in CALL.finditer(original[0])):continue
        current=leaf_body(output,name);start=current[0].index('{\n')+2
        prefix=(f'    if(halo2::native_main_heap_detail::{operation}(s'+
            (',\n        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}' if operation=='release' else '')+'))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():];main_heap_entries.append(actual)
    if main_heap_entries:output=MAIN_HEAP_INCLUDE+output
    heap_allocation_entries=[]
    heap_allocation_addresses={0x22e253:0x232b53,0x22e367:0x232c67,0x22e35c:0x232c5c,
        0x22e411:0x232d11,0x22e47b:0x232d7b,0x22e470:0x232d70}
    for name,(pin,operation,actual) in MAIN_HEAP_ALLOCATION_ENTRIES.items():
        original=leaf_body(text,name)
        if not original or digest(original[0].replace(original[1]+' &s','STATE &s'))!=pin:continue
        if not all(addresses.get(int(m[1],16))==heap_allocation_addresses.get(int(m[1],16)) for m in CALL.finditer(original[0])):continue
        current=leaf_body(output,name);start=current[0].index('{\n')+2
        prefix=(f'    if(halo2::native_main_heap_allocation_detail::{operation}(s'+
            (',\n        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}' if operation!='probe_gap' else '')+'))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():];heap_allocation_entries.append(actual)
    if heap_allocation_entries:output=MAIN_HEAP_ALLOCATION_INCLUDE+output
    raw_entries=[]
    for name,(pin,operation) in RAW_ALLOCATOR_ENTRIES.items():
        original=leaf_body(text,name)
        if not original or digest(original[0].replace(original[1]+' &s','STATE &s'))!=pin:continue
        if any(CALL.finditer(original[0])):continue
        current=leaf_body(output,name);start=current[0].index('{\n')+2
        prefix=(f'    if(halo2::native_raw_allocator_detail::{operation}(s,\n'
            '        [](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);}))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():];raw_entries.append(name)
    if raw_entries:output=RAW_ALLOCATOR_INCLUDE+output
    allocator_owner_entries=[];tick_conversion_entries=[];physical_epoch_entries=[];bsp_surface_2d_entries=[];bsp_polygon_entries=[];datum_delete_entries=[];stack_probe_entries=[];resource_busy_entries=[];object_prediction_copy_entries=[]
    dispatch_pattern=r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}'
    original_dispatch=re.search(dispatch_pattern,text)
    fresh_dispatch=False
    if original_dispatch and digest(original_dispatch[0])==ALLOCATOR_OWNER_FRESH_DISPATCH_PIN:
        extra=leaf_body(text,'fa5d0')
        fresh_dispatch=(original_dispatch[0].count(ALLOCATOR_OWNER_FRESH_CASE)==1 and
            digest(original_dispatch[0].replace(ALLOCATOR_OWNER_FRESH_CASE,'',1))==ALLOCATOR_OWNER_DISPATCH_PIN and
            extra is not None and digest(halfword_bit_query_helper().canonical_body(extra[0]))==ALLOCATOR_OWNER_FRESH_BODY_PIN)
    if original_dispatch and(digest(original_dispatch[0])==ALLOCATOR_OWNER_DISPATCH_PIN or fresh_dispatch):
        current_dispatch=re.search(dispatch_pattern,output);body=current_dispatch[0]
        fallback='default:s.required(site,target,tail);return;}}'
        if body.count(fallback)!=1:raise ValueError('Authenticated native allocator dispatch fallback changed')
        adapter=('if(halo2::native_allocator_owner_detail::dispatch(s,target,'
            '[](auto &v,std::uint32_t owner,std::uint32_t call_site,bool is_tail){dispatch(v,owner,call_site,is_tail);}))return;')
        body=body.replace(fallback,'default:'+adapter+'s.required(site,target,tail);return;}}')
        output=ALLOCATOR_OWNER_INCLUDE+output[:current_dispatch.start()]+body+output[current_dispatch.end():]
        allocator_owner_entries=[f'{owner:x}' for owner in ALLOCATOR_ACTUAL_OWNER_BYTES]
        tick_conversion_entries=[f'{owner:x}' for owner in TICK_CONVERSION_ACTUAL_OWNER_IMAGES]
        physical_epoch_entries=[f'{owner:x}' for owner in PHYSICAL_EPOCH_ACTUAL_OWNER_IMAGES]
        bsp_surface_2d_entries=[f'{owner:x}' for owner in BSP_SURFACE_2D_ACTUAL_OWNER_IMAGES]
        bsp_polygon_entries=[f'{owner:x}' for owner in BSP_SURFACE_POLYGON_ACTUAL_OWNER_IMAGES]
        datum_delete_entries=[f'{owner:x}' for owner in DATUM_DELETE_ACTUAL_OWNER_IMAGES]
        stack_probe_entries=[f'{owner:x}' for owner in STACK_PROBE_ACTUAL_OWNER_IMAGES]
        resource_busy_entries=[f'{owner:x}' for owner in RESOURCE_BUSY_ACTUAL_OWNER_IMAGES]
        object_prediction_copy_entries=[f'{owner:x}' for owner in OBJECT_PREDICTION_COPY_ACTUAL_OWNER_IMAGES]
    arena_entries=[]
    for name,(pin,operation) in STACK_ARENA_ENTRIES.items():
        original=leaf_body(text,name)
        if not original or digest(original[0].replace(original[1]+' &s','STATE &s'))!=pin:continue
        if not all(addresses.get(int(m[1],16))==int(m[1],16) for m in CALL.finditer(original[0])):continue
        current=leaf_body(output,name);start=current[0].index('{\n')+2
        prefix=(f'    if(halo2::native_stack_arena_detail::{operation}(s,\n'
            '        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():];arena_entries.append(name)
    if arena_entries:output=STACK_ARENA_INCLUDE+output
    allocator_entries=[]
    allocator_bindings={0x480d98:0x486064,0x480d9c:0x486068,0x480da0:0x48606c,0x480da4:0x486070,
        0x148a2f:0x149dbf,0x148a40:0x149dd0,0x1485a0:0x149930,0x148a49:0x149dd9,
        0x148a52:0x149de2,0x148aa5:0x149e35,0x148adb:0x149e6b,0x148aed:0x149e7d,
        0xf0340:0xf0340,0x15030:0x15030}
    for name,(pin,operation,actual) in ARRAY_ALLOCATOR_ENTRIES.items():
        original=leaf_body(text,name)
        if not original or digest(original[0].replace(original[1]+' &s','STATE &s'))!=pin:continue
        if not all(addresses.get(int(m[1],16))==allocator_bindings.get(int(m[1],16)) for m in CALL.finditer(original[0])):continue
        current=leaf_body(output,name);start=current[0].index('{\n')+2
        prefix=(f'    if(halo2::native_array_allocator_detail::{operation}(s,\n'
            '        [](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);}))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():];allocator_entries.append(actual)
    if allocator_entries:output=ARRAY_ALLOCATOR_INCLUDE+output
    pool_entries=[]
    original=leaf_body(text,'22eb20')
    if original and digest(original[0].replace(original[1]+' &s','STATE &s'))==POOL_RELEASE_PIN:
        bindings={0xf0388:0xf0388,0x46927c:0x46e33c,0x22eb83:0x233483}
        if all(addresses.get(int(m[1],16))==bindings.get(int(m[1],16)) for m in CALL.finditer(original[0])):
            current=leaf_body(output,'22eb20');start=current[0].index('{\n')+2
            prefix=('    if(halo2::native_pool_allocator_detail::release(s,\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}))return;\n')
            body=current[0][:start]+prefix+current[0][start:]
            output=POOL_ALLOCATOR_INCLUDE+output[:current.start()]+body+output[current.end():];pool_entries=['233420']
    original=leaf_body(text,'22e950')
    if original and digest(original[0].replace(original[1]+' &s','STATE &s'))==POOL_ALLOCATION_PIN:
        if all(addresses.get(int(m[1],16))==int(m[1],16) for m in CALL.finditer(original[0])):
            current=leaf_body(output,'22e950');start=current[0].index('{\n')+2
            prefix='    if(halo2::native_pool_allocator_detail::allocate(s))return;\n'
            body=current[0][:start]+prefix+current[0][start:]
            output=output[:current.start()]+body+output[current.end():]
            if POOL_ALLOCATOR_INCLUDE not in output:output=POOL_ALLOCATOR_INCLUDE+output
            pool_entries.append('233250')
    addition_entries=[]
    original=leaf_body(text,'2e8a00')
    if original and digest(original[0].replace(original[1]+' &s','STATE &s'))==BROADPHASE_ADDITION_PIN:
        bindings={0x37a6b8:0x37e168,0x418c20:0x41c6a0,0x418c10:0x41c690,0x418c00:0x41c680,
            0x2e8b1e:0x2ec5ce,0x2e8b5b:0x2ec60b,0x2e8b78:0x2ec628,0x2e8b98:0x2ec648,
            0x2e8ba6:0x2ec656,0x2e8bc7:0x2ec677,0x2e8e37:0x2ec8e7,0x2e8e77:0x2ec927,0x2e8ede:0x2ec98e}
        if all(addresses.get(int(m[1],16))==bindings.get(int(m[1],16)) for m in CALL.finditer(original[0])):
            current=leaf_body(output,'2e8a00')
            body=current[0].replace('_mm_add_ps(','halo2::native_tu15_leaves::ordered_add_ps(')
            body=body.replace('_mm_mul_ps(','halo2::native_tu15_leaves::ordered_mul_ps(')
            body=body.replace('vw(s.memory,','halo2::native_tu15_leaves::aligned_vw(s.memory,')
            body=body.replace('v(s.memory,','halo2::native_tu15_leaves::aligned_v(s.memory,')
            output=output[:current.start()]+body+output[current.end():];current=leaf_body(output,'2e8a00');start=current[0].index('{\n')+2
            prefix=('    if(halo2::native_broadphase_add_detail::add_object(s,\n'
                '        [](auto &v){run_2df780(v);},[](auto &v){run_2e7500(v);},[](auto &v){run_2e6240(v);},\n'
                '        [](auto &v){run_2e7da0(v);},[](auto &v){run_2e8090(v);}))return;\n')
            body=current[0][:start]+prefix+current[0][start:]
            output=BROADPHASE_ADDITION_INCLUDE+output[:current.start()]+body+output[current.end():];addition_entries=['2ec4b0']
    insertion_entries=[]
    original=leaf_body(text,'2e7500')
    if original and digest(original[0].replace(original[1]+' &s','STATE &s'))==BROADPHASE_INSERT_PIN:
        if addresses.get(0x2e7527)==0x2eafd7:
            current=leaf_body(output,'2e7500');start=current[0].index('{\n')+2
            prefix=('    if(halo2::native_broadphase_add_detail::insert_edges(s,[](auto &v){run_2df710(v);}))return;\n')
            body=current[0][:start]+prefix+current[0][start:]
            output=BROADPHASE_ADD_INCLUDE+output[:current.start()]+body+output[current.end():];insertion_entries=['2eafb0']
    removal_entries=[]
    original=leaf_body(text,'2ec9b0')
    if original and digest(original[0].replace(original[1]+' &s','STATE &s'))==BROADPHASE_REMOVE_PIN:
        current=leaf_body(output,'2ec9b0')
        aligned=current[0].replace('vw(s.memory,','halo2::native_tu15_leaves::aligned_vw(s.memory,')
        output=output[:current.start()]+aligned+output[current.end():]
        current=leaf_body(output,'2ec9b0');start=current[0].index('{\n')+2
        prefix=('    if(halo2::native_broadphase_remove_detail::run(s,\n'
            '        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);},\n'
            '        [](auto &v){run_2e9fd0(v);},[](auto &v){run_2e9df0(v);}))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=BROADPHASE_REMOVE_INCLUDE+output[:current.start()]+body+output[current.end():]
        removal_entries=['2ec9b0']
    output,halfword_bit_query_entries=install_halfword_bit_query(text,output)
    receipt = dict(schema='native-operand-specialization-v1',
        canonical_sha256=digest(text), specialized_sha256=digest(output),
        mapping_sha256=digest(mapping[0]), folded_calls=sum(counts.values()),
        unmapped_literal_calls=unknown,
        instruction_cursor_calls=cursor_calls,
        native_leaves=leaves,
        native_temporary_abi_entries=temporary_entries,
        native_array_abi_entries=array_entries,
        native_broadphase_removal_entries=removal_entries,
        native_broadphase_insertion_entries=insertion_entries,
        native_broadphase_addition_entries=addition_entries,
        native_pool_allocator_entries=pool_entries,
        native_array_allocator_entries=allocator_entries,
        native_stack_arena_entries=arena_entries,
        native_main_heap_entries=main_heap_entries,
        native_main_heap_allocation_entries=heap_allocation_entries,
        native_raw_allocator_entries=raw_entries,
        native_halfword_bit_query_entries=halfword_bit_query_entries,
        native_allocator_actual_owner_entries=allocator_owner_entries,
        native_tick_conversion_actual_owner_entries=tick_conversion_entries,
        native_physical_epoch_actual_owner_entries=physical_epoch_entries,
        native_bsp_surface_2d_actual_owner_entries=bsp_surface_2d_entries,
        native_bsp_surface_polygon_actual_owner_entries=bsp_polygon_entries,
        native_datum_delete_actual_owner_entries=datum_delete_entries,
        native_stack_probe_actual_owner_entries=stack_probe_entries,
        native_resource_busy_actual_owner_entries=resource_busy_entries,
        native_object_prediction_copy_actual_owner_entries=object_prediction_copy_entries,
        native_actual_owner_dispatch_variant=('fresh-fa5d0'if fresh_dispatch else'canonical')if allocator_owner_entries else None,
        native_page_window_entries=page_window_bindings(text),
        native_page_retire_entries=page_retire_bindings(text),
        native_page_allocate_entries=page_allocate_bindings(text),
        native_stack_probe_entries=stack_probe_bindings(text),
        native_resource_busy_entries=resource_busy_bindings(text),
        native_mopp_aabb_entries=mopp_aabb_entries,
        native_mopp_aabb_canonical_entries=mopp_aabb_canonical_entries,
        native_polygon_support_entries=polygon_support_entries,
        native_polygon_feature_entries=polygon_feature_entries,
        native_simplex_triangle_entries=simplex_triangle_entries,
        native_simplex_triangle_scene_entries=simplex_triangle_scene_entries,
        bindings=[dict(canonical=a, tu15=b, uses=n) for (a, b), n in sorted(counts.items())])
    if '13de70' in spatial_entries:
        receipt['native_partial_frame_owners'] = {
            '13de70': 'Native authored entry, common cases, kind-10 preprocessing and shared clamp; kinds 2/3/9 and required children retain their own dispatch.'}
        spatial_entries = [entry for entry in spatial_entries if entry != '13de70']
    if 'd8330' in spatial_entries:
        damage_stages = {
            ('d8b58',): 'Return epilogues and notification',
            ('d84dc', 'd8a56'): 'pending-response advance and section depletion',
            ('d8a08', 'd8ae2'): 'depletion gate and notification handoff',
            ('d85cb', 'd85d9'): 'vitality gate and scale handoff',
            ('d84f4',): 'descriptor refresh',
            ('d8571',): 'response flag classification',
            ('d8650',): 'shield refill and armor input preparation',
            ('d86f2',): 'shield recovery and countdown gate',
            ('d842c',): 'due response',
            ('d87ce', 'd87da'): 'secondary response',
            ('d88f9', 'd892e', 'd8945'): 'response amount and notification handoffs',
        }
        installed = set(spatial_entries)
        damage_labels = {label for labels in damage_stages for label in labels}
        has_countdown = 'const auto status=pending_response_step_native_tu15(s,' in text
        if damage_labels <= installed and has_countdown:
            spatial_entries = [entry for entry in spatial_entries if entry not in damage_labels]
        else:
            paths = ['selected damage update entry, descriptor call setup, immunity timer and section selection']
            paths.extend(description for labels, description in damage_stages.items() if installed.issuperset(labels))
            receipt.setdefault('native_partial_frame_owners', {})['d8330'] = 'Native ' + ', '.join(paths) + '.'
            spatial_entries = [entry for entry in spatial_entries if entry != 'd8330' and entry not in damage_labels]
    if '251c80' in spatial_entries:
        parts = ['Native entry and forced LOD publication']
        if '251d00' in spatial_entries:
            parts.append('native observer LOD scan at 251D00')
        if '251e30' in spatial_entries:
            parts.append('native emitter tail at 251E30')
        spatial_entries = [entry for entry in spatial_entries if entry not in ('251d00', '251e30')]
        if len(parts) != 3:
            receipt.setdefault('native_partial_frame_owners', {})['251c80'] = '; '.join(parts) + '.'
            spatial_entries = [entry for entry in spatial_entries if entry != '251c80']
    if '251d00' in spatial_entries:
        receipt.setdefault('native_partial_frame_owners', {})['251c80'] = (
            'Native observer LOD scan at 251D00; translated owner resumes at 251E30.')
        spatial_entries = [entry for entry in spatial_entries if entry != '251d00']
    if '251e30' in spatial_entries:
        partial = receipt.setdefault('native_partial_frame_owners', {})
        partial['251c80'] = (
            'Native observer LOD at 251D00 and emitter tail at 251E30; entry and forced LOD publication remain translated.'
            if '251c80' in partial else
            'Native emitter tail at 251E30; entry and observer LOD remain translated.')
        spatial_entries = [entry for entry in spatial_entries if entry != '251e30']
    if '2508a5' in spatial_entries:
        parts = ['Native orientation and position offsets at 2508A5']
        if '2508a2' in spatial_entries:
            parts.append('native entry and transform publication through 2508A2')
        if '25097b' in spatial_entries:
            parts.append('native emission tail at 25097B')
        spatial_entries = [entry for entry in spatial_entries if entry not in ('2508a2', '2508a5', '25097b')]
        if len(parts) == 3:
            spatial_entries.append('250780')
        else:
            receipt.setdefault('native_partial_frame_owners', {})['250780'] = '; '.join(parts) + '.'
    if '25097b' in spatial_entries:
        receipt.setdefault('native_partial_frame_owners', {})['250780'] = (
            'Native transform publication through 2508A2 and emission tail at 25097B.'
            if '2508a2' in spatial_entries else 'Native emission tail at 25097B.')
        spatial_entries = [entry for entry in spatial_entries if entry not in ('2508a2', '25097b')]
    if '2508a2' in spatial_entries:
        receipt.setdefault('native_partial_frame_owners', {})['250780'] = (
            'Native entry and transform publication through 2508A2. Emitter update resumes at 2508A5 or 25097B.')
        spatial_entries = [entry for entry in spatial_entries if entry != '2508a2']
    if '25113a' in spatial_entries:
        parts = ['native interface selection and return from 25113A']
        if '250e67' in spatial_entries:
            parts.insert(0, 'native particle properties and placement through 251136')
        if '250e64' in spatial_entries:
            parts.insert(0, 'native allocation, linkage and RNG through 250E64')
        spatial_entries = [entry for entry in spatial_entries if entry not in ('250e64', '250e67', '25113a')]
        if len(parts) == 3:
            spatial_entries.append('250c60')
        else:
            receipt.setdefault('native_partial_frame_owners', {})['250c60'] = '; '.join(parts) + '.'
    if '250e67' in spatial_entries:
        receipt.setdefault('native_partial_frame_owners', {})['250c60'] = (
            'Native allocation, linkage and RNG through 250E64, followed by particle properties and placement through '
            '251136. Interface selection and exit remain translated.'
            if '250e64' in spatial_entries else
            'Native particle properties and placement through 251136. Allocation and exit retain their own routes.')
        spatial_entries = [entry for entry in spatial_entries if entry not in ('250e64', '250e67')]
    if '250e64' in spatial_entries:
        receipt.setdefault('native_partial_frame_owners', {})['250c60'] = (
            'Native allocation, linkage and RNG through 250E64. Property evaluation and the exit retain their own routes.')
        spatial_entries = [entry for entry in spatial_entries if entry != '250e64']
    if '131ac0' in spatial_entries:
        fog_stages = ('131c31', '131d38', '131fc7', '1320e5', '132188', '13262b')
        if not all(pc in spatial_entries for pc in fog_stages):
            raise ValueError('Complete 131AC0 requires all authenticated native fog stages')
        spatial_entries = [entry for entry in spatial_entries if entry not in fog_stages]
    elif any(pc in spatial_entries for pc in ('131c31', '131d38', '131fc7', '1320e5', '132188', '13262b')):
        parts = []
        fog_cluster_sky_stages = ('131c31', '1320e5')
        if any(pc in spatial_entries for pc in fog_cluster_sky_stages):
            if not all(pc in spatial_entries for pc in fog_cluster_sky_stages):
                raise ValueError('131AC0 fog cluster sky requires both exact stages')
            parts.append('cluster initialization and sky gate at 131C31 and 1320E5')
        if '131d38' in spatial_entries:
            parts.append('weighted fog accumulation at 131D38')
        if '131fc7' in spatial_entries:
            parts.append('cluster completion at 131FC7, 132084 and 13208C')
        if '132188' in spatial_entries:
            parts.append('projected fog merge at 132188')
        if '13262b' in spatial_entries:
            parts.append('fog resolution and return at 13262B')
        receipt.setdefault('native_partial_frame_owners', {})['131ac0'] = (
            'Native ' + ' and '.join(parts) + '. The earlier prologue and child calls retain their existing routes.')
        spatial_entries = [entry for entry in spatial_entries if entry not in ('131c31', '131d38', '131fc7', '1320e5', '132188', '13262b')]
    if '133b40' in spatial_entries:
        fog_plane_stages = ('133bd4', '133d8f', '134042', '133c2f')
        if not all(stage in spatial_entries for stage in fog_plane_stages):
            raise ValueError('Complete 133B40 requires all authenticated native fog plane stages')
        spatial_entries = [entry for entry in spatial_entries if entry not in fog_plane_stages]
    elif '133bd4' in spatial_entries:
        projection = ', repeated member projection' if '133d8f' in spatial_entries else ''
        output_tail = ', final output merge' if '134042' in spatial_entries else ''
        transform = ', x87 plane transform and plane advance' if '133c2f' in spatial_entries else ''
        receipt.setdefault('native_partial_frame_owners', {})['133b40'] = (
            'Native output initialization, plane classification, geometry arguments' + projection + output_tail + transform +
            '. The original 11F7E0 dispatch stays at 133D8A.')
        spatial_entries = [entry for entry in spatial_entries if entry not in ('133bd4', '133d8f', '134042', '133c2f')]
    if '177b7c' in spatial_entries:
        system_stages = {
            '177983': 'native setup, location walk and drain',
            '177985': 'native attachment selection and fixed contact bases',
            '177a2f': 'native computed contact basis',
            '177b7c': 'native computed velocity basis and final lane',
            '177cd9': 'native attachment preparation and recursive update',
        }
        installed_system_stages = [description for stage, description in system_stages.items()
                                   if stage in spatial_entries]
        spatial_entries = [entry for entry in spatial_entries if entry not in system_stages]
        if len(installed_system_stages) == len(system_stages):
            spatial_entries.append('1778d0')
        else:
            receipt.setdefault('native_partial_frame_owners', {})['1778d0'] = ', '.join(installed_system_stages) + '.'
    if '177a2f' in spatial_entries:
        parts = ['native attachment selection, fixed contact bases and computed contact basis']
        if '177983' in spatial_entries:
            parts.insert(0, 'native setup, location walk and drain')
        if '177cd9' in spatial_entries:
            parts.append('native attachment preparation and recursive update')
        receipt.setdefault('native_partial_frame_owners', {})['1778d0'] = ', '.join(parts) + '.'
        spatial_entries = [entry for entry in spatial_entries if entry not in ('177a2f', '177985', '177cd9', '177983')]
    if '177985' in spatial_entries:
        parts = ['native attachment selection and fixed contact bases']
        if '177983' in spatial_entries:
            parts.insert(0, 'native setup, location walk and drain')
        if '177cd9' in spatial_entries:
            parts.append('native attachment preparation and recursive update')
        receipt.setdefault('native_partial_frame_owners', {})['1778d0'] = ', '.join(parts) + '.'
        spatial_entries = [entry for entry in spatial_entries if entry not in ('177985', '177cd9', '177983')]
    if '177cd9' in spatial_entries:
        receipt.setdefault('native_partial_frame_owners', {})['1778d0'] = (
            'Native setup, location walk, attachment preparation, recursive update and drain. Attachment geometry remains translated.'
            if '177983' in spatial_entries else
            'Native attachment preparation and recursive update from 177CD9. Setup, geometry and drain retain their own routes.')
        spatial_entries = [entry for entry in spatial_entries if entry not in ('177cd9', '177983')]
    if '177983' in spatial_entries:
        receipt.setdefault('native_partial_frame_owners', {})['1778d0'] = (
            'Native system setup, location walk and 177DD7 drain. Attachment placement and recursion remain translated.')
        spatial_entries = [entry for entry in spatial_entries if entry != '177983']
    if '2c2e92' in spatial_entries:
        spatial_entries = [entry for entry in spatial_entries if entry != '2c2e92']
        receipt.setdefault('native_partial_frame_owners', {})['2c2d90'] = (
            'Native contact input setup, live particle walk, eligibility and countdown, continuation and return.')
    if '2c2ee0' in spatial_entries:
        spatial_entries = [entry for entry in spatial_entries if entry != '2c2ee0']
        receipt['native_partial_frame_owners']['2c2d90'] += ' Active particles also use the native input cache and collision flags.'
    if '2c2fd4' in spatial_entries:
        spatial_entries = [entry for entry in spatial_entries if entry != '2c2fd4']
        receipt['native_partial_frame_owners']['2c2d90'] += ' Native sweep inputs, real collision query and result gates.'
    if '2c30b8' in spatial_entries:
        spatial_entries = [entry for entry in spatial_entries if entry != '2c30b8']
        receipt['native_partial_frame_owners']['2c2d90'] += ' Native contact response and impulse.'
    if '2c3248' in spatial_entries:
        spatial_entries = [entry for entry in spatial_entries if entry != '2c3248'] + ['2c2d90']
        receipt['native_partial_frame_owners'].pop('2c2d90', None)
    if '2524cd' in spatial_entries:
        spatial_entries = [entry for entry in spatial_entries if entry != '2524cd']
        receipt.setdefault('native_partial_frame_owners', {})['252480'] = (
            'Native common particle lifetime and rotation setup, velocity normalization and return.')
    if '252628' in spatial_entries:
        spatial_entries = [entry for entry in spatial_entries if entry != '252628']
        receipt['native_partial_frame_owners']['252480'] += ' Native random-vector modes.'
    if '2524d4' in spatial_entries:
        spatial_entries = [entry for entry in spatial_entries if entry != '2524d4']
        receipt['native_partial_frame_owners']['252480'] += ' Native spatial and planar random-bearing modes.'
    if '252480' in spatial_entries:
        receipt['native_partial_frame_owners'].pop('252480', None)
    if spatial_entries:receipt['native_spatial_owners']=spatial_entries
    # Preserve the older owner-specific authenticated rejection order. New
    # object-node receipts follow the existing Page/Stack/Busy validations.
    object_node_entries,object_animation_entries=object_node_bindings(text)
    receipt.update(native_object_node_rebuild_entries=object_node_entries,
        native_object_animation_entries=object_animation_entries)
    # Validate this later owner after all older family guards, preserving their
    # established mutation diagnostics and complete dispatcher pins.
    simplex_face_refine_entries=[]
    if 'halo2::native_simplex_face_refine_owner_detail::bridge(s)' in text or '#include "physics/runtime/native_tu15_simplex_face_refine_source_owner.hpp"' in text:
        refine_spec=importlib.util.spec_from_file_location('native_simplex_face_refine_binding',Path(__file__).with_name('native_simplex_face_refine_replacements.py'))
        refine_helper=importlib.util.module_from_spec(refine_spec);refine_spec.loader.exec_module(refine_helper)
        simplex_face_refine_entries=refine_helper.installed_entries(text)
    receipt['native_simplex_face_refine_entries']=simplex_face_refine_entries
    # The later face-select owner follows all earlier strict guards so their
    # established dispatcher mutation diagnostics remain authoritative.
    simplex_face_select_entries=[]
    if 'halo2::native_simplex_face_select_owner_detail::bridge(s,' in text or '#include "physics/runtime/native_tu15_simplex_face_select_source_owner.hpp"' in text:
        face_select_spec=importlib.util.spec_from_file_location('native_simplex_face_select_binding',Path(__file__).with_name('native_simplex_face_select_replacements.py'))
        face_select_helper=importlib.util.module_from_spec(face_select_spec);face_select_spec.loader.exec_module(face_select_helper)
        simplex_face_select_entries=face_select_helper.installed_entries(text)
    receipt['native_simplex_face_select_entries']=simplex_face_select_entries
    closest_segments_entries=[];closest_segments_scene_entries=[]
    if '#include "physics/runtime/native_tu15_closest_segments_source_owner.hpp"' in text or 'halo2::native_closest_segments_owner_detail::bridge' in text:
        closest_spec=importlib.util.spec_from_file_location('native_closest_segments_binding',Path(__file__).with_name('native_closest_segments_replacements.py'))
        closest_helper=importlib.util.module_from_spec(closest_spec);closest_spec.loader.exec_module(closest_helper)
        closest_owner=closest_helper.BODY.search(text)
        entries=closest_helper.installed_entries(text)
        if closest_owner[1]=='312620':closest_segments_scene_entries=entries
        else:closest_segments_entries=entries
    receipt.update(native_closest_segments_entries=closest_segments_entries,native_closest_segments_scene_entries=closest_segments_scene_entries)
    simplex_paired_entries=[]
    if '#include "physics/runtime/native_tu15_simplex_paired_source_owner.hpp"' in text or 'halo2::native_simplex_paired_owner_detail::bridge' in text:
        paired_spec=importlib.util.spec_from_file_location('native_simplex_paired_binding',Path(__file__).with_name('native_simplex_paired_replacements.py'))
        paired_helper=importlib.util.module_from_spec(paired_spec);paired_spec.loader.exec_module(paired_helper)
        simplex_paired_entries=paired_helper.installed_entries(text)
    receipt['native_simplex_paired_entries']=simplex_paired_entries
    simplex_boundary_entries=[]
    if '#include "physics/runtime/native_tu15_simplex_boundary_source_owner.hpp"' in text or 'halo2::native_simplex_boundary_owner_detail::bridge' in text:
        boundary_spec=importlib.util.spec_from_file_location('native_simplex_boundary_binding',Path(__file__).with_name('native_simplex_boundary_replacements.py'))
        boundary_helper=importlib.util.module_from_spec(boundary_spec);boundary_spec.loader.exec_module(boundary_helper)
        simplex_boundary_entries=boundary_helper.installed_entries(text)
    receipt['native_simplex_boundary_entries']=simplex_boundary_entries
    solver_hull_append_entries=[]
    if any(marker in text for marker in ('#include "physics/runtime/native_tu15_solver_hull_append_source_owner.hpp"', 'native_solver_hull_append_owner_detail')):
        append_spec=importlib.util.spec_from_file_location('native_solver_hull_append_binding',Path(__file__).with_name('native_solver_hull_append_replacements.py'))
        append_helper=importlib.util.module_from_spec(append_spec);append_spec.loader.exec_module(append_helper)
        solver_hull_append_entries=append_helper.installed_entries(text)
    receipt['native_solver_hull_append_entries']=solver_hull_append_entries
    zero_shape_support_entries=[]
    if any(marker in text for marker in ('#include "physics/runtime/native_tu15_zero_shape_support_source_owner.hpp"', 'native_zero_shape_support_owner_detail')):
        zero_support_spec=importlib.util.spec_from_file_location('native_zero_shape_support_binding',Path(__file__).with_name('native_zero_shape_support_replacements.py'))
        zero_support_helper=importlib.util.module_from_spec(zero_support_spec);zero_support_spec.loader.exec_module(zero_support_helper)
        zero_shape_support_entries=zero_support_helper.installed_entries(text)
    receipt['native_zero_shape_support_entries']=zero_shape_support_entries
    box_shape_support_entries=[]
    if any(marker in text for marker in ('#include "physics/runtime/native_tu15_box_shape_support_source_owner.hpp"', 'native_box_shape_support_owner_detail')):
        box_support_spec=importlib.util.spec_from_file_location('native_box_shape_support_binding',Path(__file__).with_name('native_box_shape_support_replacements.py'))
        box_support_helper=importlib.util.module_from_spec(box_support_spec);box_support_spec.loader.exec_module(box_support_helper)
        box_shape_support_entries=box_support_helper.installed_entries(text)
    receipt['native_box_shape_support_entries']=box_shape_support_entries
    packed_rotation_entries=[]
    if '#include "physics/runtime/native_tu15_packed_rotation_source_owner.hpp"' in text or 'halo2::native_packed_rotation_owner_detail::bridge' in text:
        packed_rotation_spec=importlib.util.spec_from_file_location('native_packed_rotation_binding',Path(__file__).with_name('native_packed_rotation_replacements.py'))
        packed_rotation_helper=importlib.util.module_from_spec(packed_rotation_spec);packed_rotation_spec.loader.exec_module(packed_rotation_helper)
        packed_rotation_entries=packed_rotation_helper.installed_entries(text)
    receipt['native_packed_rotation_entries']=packed_rotation_entries
    codec_translation_entries=[]
    if '#include "physics/runtime/native_tu15_codec_translation_source_owner.hpp"' in text or 'halo2::native_codec_translation_owner_detail::bridge' in text:
        codec_translation_spec=importlib.util.spec_from_file_location('native_codec_translation_binding',Path(__file__).with_name('native_codec_translation_replacements.py'))
        codec_translation_helper=importlib.util.module_from_spec(codec_translation_spec);codec_translation_spec.loader.exec_module(codec_translation_helper)
        codec_translation_entries=codec_translation_helper.installed_entries(text)
    receipt['native_codec_translation_entries']=codec_translation_entries
    codec_scale_entries=[]
    if '#include "physics/runtime/native_tu15_codec_scale_source_owner.hpp"' in text or 'halo2::native_codec_scale_owner_detail::bridge' in text:
        codec_scale_spec=importlib.util.spec_from_file_location('native_codec_scale_binding',Path(__file__).with_name('native_codec_scale_replacements.py'))
        codec_scale_helper=importlib.util.module_from_spec(codec_scale_spec);codec_scale_spec.loader.exec_module(codec_scale_helper)
        codec_scale_entries=codec_scale_helper.installed_entries(text)
    receipt['native_codec_scale_entries']=codec_scale_entries
    # Rebind only overlays already authenticated in the input. The helper
    # restores the exact legacy spelling solely to authenticate prepared output.
    if '#include "gameplay/objects/native_tu15_object_body_source_owner.hpp"' in text or 'halo2::native_object_body_owner_detail::bridge' in text:
        format_spec=importlib.util.spec_from_file_location('format_native_object_body_replacements',Path(__file__).with_name('native_object_body_replacements.py'))
        format_helper=importlib.util.module_from_spec(format_spec);format_spec.loader.exec_module(format_helper)
        output=format_helper.retag_specialized(text,output)
    if '#include "gameplay/objects/native_tu15_object_material_lookup_source_owner.hpp"' in text or 'halo2::native_object_material_owner_detail::bridge' in text:
        format_spec=importlib.util.spec_from_file_location('format_native_object_material_lookup_replacements',Path(__file__).with_name('native_object_material_lookup_replacements.py'))
        format_helper=importlib.util.module_from_spec(format_spec);format_spec.loader.exec_module(format_helper)
        output=format_helper.retag_specialized(text,output)
    if '#include "gameplay/objects/native_tu15_object_lookup_copies_source_owner.hpp"' in text or 'halo2::native_object_lookup_copies_detail::bridge' in text:
        format_spec=importlib.util.spec_from_file_location('format_native_object_lookup_copies_replacements',Path(__file__).with_name('native_object_lookup_copies_replacements.py'))
        format_helper=importlib.util.module_from_spec(format_spec);format_spec.loader.exec_module(format_helper)
        output=format_helper.retag_specialized(text,output)
    if '#include "gameplay/objects/native_tu15_object_body_canonical_source_owner.hpp"' in text or 'halo2::native_object_body_canonical_owner_detail::bridge' in text:
        format_spec=importlib.util.spec_from_file_location('format_native_object_lookup_canonical_replacements',Path(__file__).with_name('native_object_lookup_canonical_replacements.py'))
        format_helper=importlib.util.module_from_spec(format_spec);format_spec.loader.exec_module(format_helper)
        output=format_helper.retag_specialized(text,output)
    object_body_entries=[]
    if '#include "gameplay/objects/native_tu15_object_body_source_owner.hpp"' in text or 'halo2::native_object_body_owner_detail::bridge' in text:
        object_body_spec=importlib.util.spec_from_file_location('native_object_body_binding',Path(__file__).with_name('native_object_body_replacements.py'))
        object_body_helper=importlib.util.module_from_spec(object_body_spec);object_body_spec.loader.exec_module(object_body_helper)
        object_body_entries=object_body_helper.installed_entries(output)
    receipt['native_object_body_entries']=object_body_entries
    object_material_lookup_entries=[]
    if '#include "gameplay/objects/native_tu15_object_material_lookup_source_owner.hpp"' in text or 'halo2::native_object_material_owner_detail::bridge' in text:
        material_lookup_spec=importlib.util.spec_from_file_location('native_object_material_lookup_binding',Path(__file__).with_name('native_object_material_lookup_replacements.py'))
        material_lookup_helper=importlib.util.module_from_spec(material_lookup_spec);material_lookup_spec.loader.exec_module(material_lookup_helper)
        object_material_lookup_entries=material_lookup_helper.installed_entries(output)
    receipt['native_object_material_lookup_entries']=object_material_lookup_entries
    lookup_copies_entries=[]
    if '#include "gameplay/objects/native_tu15_object_lookup_copies_source_owner.hpp"' in text or 'halo2::native_object_lookup_copies_detail::bridge' in text:
        lookup_copies_spec=importlib.util.spec_from_file_location('native_object_lookup_copies_binding',Path(__file__).with_name('native_object_lookup_copies_replacements.py'))
        lookup_copies_helper=importlib.util.module_from_spec(lookup_copies_spec);lookup_copies_spec.loader.exec_module(lookup_copies_helper)
        lookup_copies_entries=lookup_copies_helper.installed_entries(output)
    receipt['native_object_lookup_copies_entries']=lookup_copies_entries
    canonical_lookup_entries=[]
    if '#include "gameplay/objects/native_tu15_object_body_canonical_source_owner.hpp"' in text or 'halo2::native_object_body_canonical_owner_detail::bridge' in text:
        canonical_spec=importlib.util.spec_from_file_location('canonical_format_binding',Path(__file__).with_name('native_object_lookup_canonical_replacements.py'))
        canonical_helper=importlib.util.module_from_spec(canonical_spec);canonical_spec.loader.exec_module(canonical_helper)
        canonical_lookup_entries=canonical_helper.installed_entries(output)
    receipt['native_object_lookup_canonical_entries']=canonical_lookup_entries
    solver_hull_support_pair_entries=[]
    if any(marker in text for marker in ('#include "physics/runtime/native_tu15_solver_hull_support_pair_source_owner.hpp"', 'native_solver_hull_support_pair_owner_detail', 'native_support_pair_resume_')):
        support_pair_spec=importlib.util.spec_from_file_location('native_solver_hull_support_pair_binding',Path(__file__).with_name('native_solver_hull_support_pair_replacements.py'))
        support_pair_helper=importlib.util.module_from_spec(support_pair_spec);support_pair_spec.loader.exec_module(support_pair_helper)
        solver_hull_support_pair_entries=support_pair_helper.installed_entries(text)
    receipt['native_solver_hull_support_pair_entries']=solver_hull_support_pair_entries
    solver_hull_edge_entries=[]
    if any(marker in text for marker in ('#include "physics/runtime/native_tu15_solver_hull_edge_source_owner.hpp"', 'native_solver_hull_edge_owner_detail', 'native_hull_edge_resume_')):
        edge_spec=importlib.util.spec_from_file_location('native_solver_hull_edge_binding',Path(__file__).with_name('native_solver_hull_edge_replacements.py'))
        edge_helper=importlib.util.module_from_spec(edge_spec);edge_spec.loader.exec_module(edge_helper)
        edge_helper.installed_entries(text)
        output=edge_helper.rebind_prepared(output)
        solver_hull_edge_entries=edge_helper.installed_entries(output)
    receipt['native_solver_hull_edge_entries']=solver_hull_edge_entries
    solver_hull_initialize_entries=[]
    if any(marker in text for marker in ('#include "physics/runtime/native_tu15_solver_hull_initialize_source_owner.hpp"', 'native_solver_hull_initialize_owner_detail', 'native_hull_initialize_resume_')):
        initialize_spec=importlib.util.spec_from_file_location('native_solver_hull_initialize_binding',Path(__file__).with_name('native_solver_hull_initialize_replacements.py'))
        initialize_helper=importlib.util.module_from_spec(initialize_spec);initialize_spec.loader.exec_module(initialize_helper)
        initialize_helper.installed_entries(text)
        output=initialize_helper.rebind_prepared(output)
        solver_hull_initialize_entries=initialize_helper.installed_entries(output)
    receipt['native_solver_hull_initialize_entries']=solver_hull_initialize_entries
    solver_hull_line_entries=[]
    if any(marker in text for marker in ('#include "physics/runtime/native_tu15_solver_hull_line_source_owner.hpp"', 'native_solver_hull_line_owner_detail', 'native_hull_line_resume_')):
        line_spec=importlib.util.spec_from_file_location('native_solver_hull_line_binding',Path(__file__).with_name('native_solver_hull_line_replacements.py'))
        line_helper=importlib.util.module_from_spec(line_spec);line_spec.loader.exec_module(line_helper)
        line_helper.installed_entries(text)
        output=line_helper.rebind_prepared(output)
        solver_hull_line_entries=line_helper.installed_entries(output)
    receipt['native_solver_hull_line_entries']=solver_hull_line_entries
    solver_hull_sample_entries=[]
    if any(marker in text for marker in ('#include "physics/runtime/native_tu15_solver_hull_sample_source_owner.hpp"', 'native_solver_hull_sample_owner_detail', 'native_hull_sample_resume_')):
        sample_spec=importlib.util.spec_from_file_location('native_solver_hull_sample_binding',Path(__file__).with_name('native_solver_hull_sample_replacements.py'))
        sample_helper=importlib.util.module_from_spec(sample_spec);sample_spec.loader.exec_module(sample_helper)
        sample_helper.installed_entries(text)
        output=sample_helper.rebind_prepared(output)
        solver_hull_sample_entries=sample_helper.installed_entries(output)
    receipt['native_solver_hull_sample_entries']=solver_hull_sample_entries
    solver_hull_minimum_entries=[]
    if any(marker in output for marker in ('#include "physics/runtime/native_tu15_solver_hull_minimum_source_owner.hpp"','native_solver_hull_minimum_owner_detail','native_hull_minimum_resume_')):
        minimum_spec=importlib.util.spec_from_file_location('native_solver_hull_minimum_binding',Path(__file__).with_name('native_solver_hull_minimum_replacements.py'))
        minimum_helper=importlib.util.module_from_spec(minimum_spec);minimum_spec.loader.exec_module(minimum_helper)
        minimum_helper.installed_entries(text)
        output=minimum_helper.rebind_prepared(output)
        solver_hull_minimum_entries=minimum_helper.installed_entries(output)
    receipt['native_solver_hull_minimum_entries']=solver_hull_minimum_entries
    solver_hull_clear_entries=[]
    if any(marker in output for marker in ('#include "physics/runtime/native_tu15_solver_hull_clear_source_owner.hpp"','native_solver_hull_clear_owner_detail','native_hull_clear_resume_')):
        clear_spec=importlib.util.spec_from_file_location('native_solver_hull_clear_binding',Path(__file__).with_name('native_solver_hull_clear_replacements.py'))
        clear_helper=importlib.util.module_from_spec(clear_spec);clear_spec.loader.exec_module(clear_helper)
        clear_helper.installed_entries(text)
        output=clear_helper.rebind_prepared(output)
        solver_hull_clear_entries=clear_helper.installed_entries(output)
    receipt['native_solver_hull_clear_entries']=solver_hull_clear_entries
    solver_hull_clip_entries=[]
    if any(marker in output for marker in ('#include "physics/runtime/native_tu15_solver_hull_clip_source_owner.hpp"','native_solver_hull_clip_owner_detail','native_hull_clip_resume_')):
        clip_spec=importlib.util.spec_from_file_location('native_solver_hull_clip_binding',Path(__file__).with_name('native_solver_hull_clip_replacements.py'))
        clip_helper=importlib.util.module_from_spec(clip_spec);clip_spec.loader.exec_module(clip_helper)
        clip_helper.installed_entries(text)
        output=clip_helper.rebind_prepared(output)
        solver_hull_clip_entries=clip_helper.installed_entries(output)
    receipt['native_solver_hull_clip_entries']=solver_hull_clip_entries
    codec_rotation_interpolation_entries=[]
    if '#include "physics/runtime/native_tu15_codec_rotation_interpolation_source_owner.hpp"' in output or 'halo2::native_codec_rotation_interpolation_owner_detail::bridge' in output:
        interpolation_spec=importlib.util.spec_from_file_location('native_codec_rotation_interpolation_binding',Path(__file__).with_name('native_codec_rotation_interpolation_replacements.py'))
        interpolation_helper=importlib.util.module_from_spec(interpolation_spec);interpolation_spec.loader.exec_module(interpolation_helper)
        interpolation_helper.installed_entries(text)
        output=interpolation_helper.rebind_prepared(output)
        codec_rotation_interpolation_entries=interpolation_helper.installed_entries(output)
    receipt['native_codec_rotation_interpolation_entries']=codec_rotation_interpolation_entries
    object_type_query_entries=[]
    if '#include "gameplay/objects/native_tu15_object_type_query_source_owner.hpp"' in text or 'halo2::native_object_type_query_owner_detail::bridge' in text:
        query_spec=importlib.util.spec_from_file_location('native_object_type_query_binding',Path(__file__).with_name('native_object_type_query_replacements.py'))
        query_helper=importlib.util.module_from_spec(query_spec);query_spec.loader.exec_module(query_helper)
        query_helper.installed_entries(text)
        output=query_helper.retag_specialized(text,output)
        object_type_query_entries=query_helper.installed_entries(output)
    receipt['native_object_type_query_entries']=object_type_query_entries
    arena_world_entries=[]
    if '#include "physics/runtime/native_tu15_arena_world_initialize_owner.hpp"' in text or 'halo2::native_arena_world_initialize_owner_detail::bridge' in text:
        arena_world_spec=importlib.util.spec_from_file_location('native_arena_world_binding',Path(__file__).with_name('native_arena_world_initialize_replacements.py'))
        arena_world_helper=importlib.util.module_from_spec(arena_world_spec);arena_world_spec.loader.exec_module(arena_world_helper)
        arena_world_helper.installed_entries(text)
        output=arena_world_helper.retag_specialized(text,output)
        arena_world_entries=arena_world_helper.installed_entries(output)
    receipt['native_arena_world_initialize_entries']=arena_world_entries
    codec_translation_interpolation_entries=[]
    if '#include "physics/runtime/native_tu15_codec_translation_interpolation_source_owner.hpp"' in text or 'halo2::native_codec_translation_interpolation_owner_detail::bridge' in text:
        translation_interpolation_spec=importlib.util.spec_from_file_location('native_codec_translation_interpolation_binding',Path(__file__).with_name('native_codec_translation_interpolation_replacements.py'))
        translation_interpolation_helper=importlib.util.module_from_spec(translation_interpolation_spec);translation_interpolation_spec.loader.exec_module(translation_interpolation_helper)
        translation_interpolation_helper.installed_entries(text)
        output=translation_interpolation_helper.rebind_prepared(output)
        codec_translation_interpolation_entries=translation_interpolation_helper.installed_entries(output)
    receipt['native_codec_translation_interpolation_entries']=codec_translation_interpolation_entries
    codec_scale_interpolation_entries=[]
    if '#include "physics/runtime/native_tu15_codec_scale_interpolation_source_owner.hpp"' in text or 'halo2::native_codec_scale_interpolation_owner_detail::bridge' in text:
        scale_interpolation_spec=importlib.util.spec_from_file_location('native_codec_scale_interpolation_binding',Path(__file__).with_name('native_codec_scale_interpolation_replacements.py'))
        scale_interpolation_helper=importlib.util.module_from_spec(scale_interpolation_spec);scale_interpolation_spec.loader.exec_module(scale_interpolation_helper)
        scale_interpolation_helper.installed_entries(text)
        output=scale_interpolation_helper.rebind_prepared(output)
        codec_scale_interpolation_entries=scale_interpolation_helper.installed_entries(output)
    receipt['native_codec_scale_interpolation_entries']=codec_scale_interpolation_entries
    codec_copy_entries=[]
    if ('#include "physics/runtime/native_tu15_codec_copy_source_owner.hpp"' in text or
            'native_codec_copy_owner_detail' in text or 'native_codec_copy_resume_' in text):
        copy_spec=importlib.util.spec_from_file_location('native_codec_copy_binding',Path(__file__).with_name('native_codec_copy_replacements.py'))
        copy_helper=importlib.util.module_from_spec(copy_spec);copy_spec.loader.exec_module(copy_helper)
        copy_helper.installed_entries(text)
        output=copy_helper.rebind_prepared(output)
        codec_copy_entries=copy_helper.installed_entries(output)
    receipt['native_codec_copy_entries']=codec_copy_entries
    projectile_query_entries=[]
    if '#include "gameplay/objects/native_tu15_projectile_function_query_source_owner.hpp"' in text or 'halo2::native_projectile_function_query_owner_detail::bridge' in text:
        projectile_spec=importlib.util.spec_from_file_location('native_projectile_function_query_binding',Path(__file__).with_name('native_projectile_function_query_replacements.py'))
        projectile_helper=importlib.util.module_from_spec(projectile_spec);projectile_spec.loader.exec_module(projectile_helper)
        projectile_helper.installed_entries(text)
        output=projectile_helper.retag_specialized(text,output)
        projectile_query_entries=projectile_helper.installed_entries(output)
    receipt['native_projectile_function_query_entries']=projectile_query_entries
    definition_mode_entries=[]
    if any(marker in text for marker in ('native_tu15_object_definition_mode_query_source_owner.hpp',
            'native_object_definition_mode_query_owner_detail','native_object_definition_mode_query_detail')):
        definition_spec=importlib.util.spec_from_file_location('native_object_definition_mode_query_binding',
            Path(__file__).with_name('native_object_definition_mode_query_replacements.py'))
        definition_helper=importlib.util.module_from_spec(definition_spec);definition_spec.loader.exec_module(definition_helper)
        definition_helper.installed_entries(text)
        output=definition_helper.retag_specialized(text,output)
        definition_mode_entries=definition_helper.installed_entries(output)
    receipt['native_object_definition_mode_query_entries']=definition_mode_entries
    table_ratio_entries=[]
    if any(marker in text for marker in ('native_tu15_object_table_ratio_source_owner.hpp',
            'native_object_table_ratio_owner_detail','native_object_table_ratio_detail')):
        table_spec=importlib.util.spec_from_file_location('native_object_table_ratio_binding',
            Path(__file__).with_name('native_object_table_ratio_replacements.py'))
        table_helper=importlib.util.module_from_spec(table_spec);table_spec.loader.exec_module(table_helper)
        table_helper.installed_entries(text)
        output=table_helper.retag_specialized(text,output)
        table_ratio_entries=table_helper.installed_entries(output)
    receipt['native_object_table_ratio_entries']=table_ratio_entries
    map_world_property_entries=[]
    if '#include "gameplay/objects/native_tu15_map_world_property_owners.hpp"' in text or 'halo2::native_map_world_property_owner_detail::bridge_' in text:
        property_spec=importlib.util.spec_from_file_location('native_map_world_property_query_binding',Path(__file__).with_name('native_map_world_property_query_replacements.py'))
        property_helper=importlib.util.module_from_spec(property_spec);property_spec.loader.exec_module(property_helper)
        property_helper.installed_entries(text)
        output=property_helper.retag_specialized(text,output)
        map_world_property_entries=property_helper.installed_entries(output)
    receipt['native_map_world_property_query_entries']=map_world_property_entries
    device_query_entries=[]
    if any(marker in text for marker in ('native_tu15_device_property_query_source_owner.hpp',
            'native_device_property_query_owner_detail','native_device_property_query_detail')):
        device_spec=importlib.util.spec_from_file_location('native_device_property_query_binding',Path(__file__).with_name('native_device_property_query_replacements.py'))
        device_helper=importlib.util.module_from_spec(device_spec);device_spec.loader.exec_module(device_helper)
        device_helper.installed_entries(text)
        output=device_helper.retag_specialized(text,output)
        device_query_entries=device_helper.installed_entries(output)
    receipt['native_device_property_query_entries']=device_query_entries
    vehicle_render_root_entries=[]
    if 'native_tu15_vehicle_render_root_source_owner.hpp' in text or 'native_vehicle_render_root_detail' in text:
        root_spec=importlib.util.spec_from_file_location('native_vehicle_render_root_binding',Path(__file__).with_name('native_vehicle_render_root_replacements.py'))
        root_helper=importlib.util.module_from_spec(root_spec);root_spec.loader.exec_module(root_helper)
        root_helper.installed_entries(text)
        output=root_helper.rebind_prepared(output)
        vehicle_render_root_entries=root_helper.installed_entries(output)
    receipt['native_vehicle_render_root_entries']=vehicle_render_root_entries
    flight_query_entries=[]
    if any(marker in text for marker in ('native_tu15_flight_property_query_source_owner.hpp',
            'native_flight_property_query_owner_detail','native_flight_property_query_detail')):
        flight_spec=importlib.util.spec_from_file_location('native_flight_property_query_binding',
            Path(__file__).with_name('native_flight_property_query_replacements.py'))
        flight_helper=importlib.util.module_from_spec(flight_spec);flight_spec.loader.exec_module(flight_helper)
        flight_helper.installed_entries(text)
        output=flight_helper.retag_specialized(text,output)
        flight_query_entries=flight_helper.installed_entries(output)
    receipt['native_flight_property_query_entries']=flight_query_entries
    record_ratio_entries=[]
    if any(marker in text for marker in ('native_tu15_object_record_ratio_source_owner.hpp',
            'native_object_record_ratio_owner_detail','native_object_record_ratio_detail')):
        record_spec=importlib.util.spec_from_file_location('native_object_record_ratio_binding',
            Path(__file__).with_name('native_object_record_ratio_replacements.py'))
        record_helper=importlib.util.module_from_spec(record_spec);record_spec.loader.exec_module(record_helper)
        record_helper.installed_entries(text)
        output=record_helper.retag_specialized(text,output)
        record_ratio_entries=record_helper.installed_entries(output)
    receipt['native_object_record_ratio_entries']=record_ratio_entries
    ratio_caller_entries=[]
    if any(marker in text for marker in ('native_tu15_object_ratio_caller_source_owner.hpp',
            'native_object_ratio_caller_owner_detail','native_object_ratio_caller_detail')):
        caller_spec=importlib.util.spec_from_file_location('native_object_ratio_caller_binding',
            Path(__file__).with_name('native_object_ratio_caller_replacements.py'))
        caller=importlib.util.module_from_spec(caller_spec)
        caller_spec.loader.exec_module(caller)
        caller.installed_entries(text)
        output=caller.retag_specialized(text,output)
        ratio_caller_entries=caller.installed_entries(output)
    receipt['native_object_ratio_caller_entries']=ratio_caller_entries
    node_function_progress_entries=[]
    if any(marker in text for marker in ('native_tu15_node_function_progress_source_owner.hpp',
            'native_node_function_progress_owner_detail','native_node_function_progress_detail')):
        progress_spec=importlib.util.spec_from_file_location('native_node_function_progress_binding',
            Path(__file__).with_name('native_node_function_progress_replacements.py'))
        progress_helper=importlib.util.module_from_spec(progress_spec);progress_spec.loader.exec_module(progress_helper)
        progress_helper.installed_entries(text)
        output=progress_helper.retag_specialized(text,output)
        node_function_progress_entries=progress_helper.installed_entries(output)
    receipt['native_node_function_progress_entries']=node_function_progress_entries
    object_function_query_entries=[]
    if any(marker in text for marker in ('native_tu15_object_function_query_source_owner.hpp',
            'native_object_function_query_owner_detail','native_object_function_query_detail')):
        query_spec=importlib.util.spec_from_file_location('native_object_function_query_binding',
            Path(__file__).with_name('native_object_function_query_replacements.py'))
        query_helper=importlib.util.module_from_spec(query_spec);query_spec.loader.exec_module(query_helper)
        query_helper.installed_entries(text)
        output=query_helper.retag_specialized(text,output)
        object_function_query_entries=query_helper.installed_entries(output)
    receipt['native_object_function_query_entries']=object_function_query_entries
    object_function_scalar_entries=[]
    if any(marker in text for marker in ('native_tu15_object_function_scalar_source_owner.hpp',
            'native_object_function_scalar_owner_detail','native_object_function_scalar_detail',
            'native_object_function_scalar_ordered_detail')):
        scalar_spec=importlib.util.spec_from_file_location('native_object_function_scalar_binding',
            Path(__file__).with_name('native_object_function_scalar_replacements.py'))
        scalar_helper=importlib.util.module_from_spec(scalar_spec);scalar_spec.loader.exec_module(scalar_helper)
        scalar_helper.installed_entries(text)
        if scalar_helper.authenticate(text)[2]:
            scalar_helper.installed_entries(output)
        else:
            output=scalar_helper.retag_specialized(text,output)
        object_function_scalar_entries=scalar_helper.installed_entries(output)
    receipt['native_object_function_scalar_entries']=object_function_scalar_entries
    death_flight_entries=[]
    if any(marker in text for marker in ('native_tu15_death_cycle_flight_query_source_owner.hpp',
            'native_death_cycle_flight_query_owner_detail')):
        death_flight_spec=importlib.util.spec_from_file_location('native_death_flight_query_binding',
            Path(__file__).with_name('native_death_cycle_flight_query_replacements.py'))
        death_flight_helper=importlib.util.module_from_spec(death_flight_spec);death_flight_spec.loader.exec_module(death_flight_helper)
        death_flight_helper.installed_entries(text)
        output=death_flight_helper.retag_specialized(text,output)
        death_flight_entries=death_flight_helper.installed_entries(output)
    receipt['native_death_cycle_flight_query_entries']=death_flight_entries
    contact_predicate_entries=[]
    if any(marker in text for marker in ('native_tu15_contact_predicate_source_owner.hpp',
            'native_contact_predicate_owner_detail','native_contact_predicate_ordered_detail',
            'NativeContactPredicateResume_')):
        predicate_spec=importlib.util.spec_from_file_location('native_contact_predicate_binding',
            Path(__file__).with_name('native_contact_predicate_replacements.py'))
        predicate=importlib.util.module_from_spec(predicate_spec);predicate_spec.loader.exec_module(predicate)
        predicate.installed_entries(text)
        output=predicate.rebind_prepared(output)
        contact_predicate_entries=predicate.installed_entries(output)
    receipt['native_contact_predicate_entries']=contact_predicate_entries
    node_progress_entries=[]
    if any(marker in text for marker in ('native_tu15_node_progress_query_source_owner.hpp',
            'native_node_progress_query_owner_detail','native_node_progress_query_detail')):
        node_progress_spec=importlib.util.spec_from_file_location('native_node_progress_query_binding',
            Path(__file__).with_name('native_node_progress_query_replacements.py'))
        node_progress=importlib.util.module_from_spec(node_progress_spec);node_progress_spec.loader.exec_module(node_progress)
        node_progress.installed_entries(text)
        output=node_progress.retag_specialized(text,output)
        node_progress_entries=node_progress.installed_entries(output)
    receipt['native_node_progress_query_entries']=node_progress_entries
    vehicle_effect_ratio_entries=[]
    if any(marker in text for marker in ('native_tu15_vehicle_effect_ratio_source_owner.hpp',
            'native_vehicle_effect_ratio_owner_detail','native_vehicle_effect_ratio_ordered_detail')):
        ratio_spec=importlib.util.spec_from_file_location('native_vehicle_effect_ratio_binding',
            Path(__file__).with_name('native_vehicle_effect_ratio_replacements.py'))
        ratio=importlib.util.module_from_spec(ratio_spec);ratio_spec.loader.exec_module(ratio)
        ratio.installed_entries(text)
        output=ratio.rebind_prepared(output)
        vehicle_effect_ratio_entries=ratio.installed_entries(output)
    receipt['native_vehicle_effect_ratio_entries']=vehicle_effect_ratio_entries
    indexed_datum_entries=[]
    if 'native_indexed_datum_gate_owner_detail' in text or 'native_tu15_indexed_datum_gate_source_owner.hpp' in text:
        datum_spec=importlib.util.spec_from_file_location('native_indexed_datum_gate_binding',
            Path(__file__).with_name('native_indexed_datum_gate_replacements.py'))
        indexed_datum=importlib.util.module_from_spec(datum_spec);datum_spec.loader.exec_module(indexed_datum)
        output=indexed_datum.retag_specialized(text,output)
        indexed_datum_entries=indexed_datum.installed_entries(output)
    receipt['native_indexed_datum_gate_entries']=indexed_datum_entries
    particle_type_filter_entries=[]
    if any(marker in text for marker in ('native_tu15_particle_type_filter_source_owner.hpp',
            'native_particle_type_filter_owner_detail','native_particle_type_filter_detail')):
        filter_spec=importlib.util.spec_from_file_location('native_particle_type_filter_binding',
            Path(__file__).with_name('native_particle_type_filter_replacements.py'))
        type_filter=importlib.util.module_from_spec(filter_spec);filter_spec.loader.exec_module(type_filter)
        type_filter.installed_entries(text)
        output=type_filter.retag_specialized(text,output)
        particle_type_filter_entries=type_filter.installed_entries(output)
    receipt['native_particle_type_filter_entries']=particle_type_filter_entries
    particle_envelope_entries=[]
    if any(marker in text for marker in ('native_tu15_particle_envelope_source_owner.hpp',
            'native_particle_envelope_owner_detail','native_particle_envelope_detail')):
        envelope_spec=importlib.util.spec_from_file_location('native_particle_envelope_binding',
            Path(__file__).with_name('native_particle_envelope_replacements.py'))
        envelope=importlib.util.module_from_spec(envelope_spec);envelope_spec.loader.exec_module(envelope)
        envelope.installed_entries(text)
        output=envelope.retag_specialized(text,output)
        particle_envelope_entries=envelope.installed_entries(output)
    receipt['native_particle_envelope_entries']=particle_envelope_entries
    particle_record_entries=[]
    if any(marker in text for marker in ('native_tu15_particle_record_source_owner.hpp',
            'native_particle_record_owner_detail','native_particle_record_detail')):
        record_spec=importlib.util.spec_from_file_location('native_particle_record_binding',
            Path(__file__).with_name('native_particle_record_replacements.py'))
        particle_record=importlib.util.module_from_spec(record_spec);record_spec.loader.exec_module(particle_record)
        particle_record.installed_entries(text)
        output=particle_record.retag_specialized(text,output)
        particle_record_entries=particle_record.installed_entries(output)
    receipt['native_particle_record_entries']=particle_record_entries
    grouped_stack_pops=0
    if cursor_calls and ('void source_run_ai_frame_tu15(' in text or 'void source_run_physics_live_support_tu15(' in text):
        output,grouped_stack_pops=group_literal_stack_pops(output)
    receipt['grouped_stack_pops']=grouped_stack_pops
    if 'void source_run_ai_frame_tu15(' in text:
        # The path ring adds a caller marker to LiveState. The earlier
        # object-owner checks must authenticate the original State first.
        ring_spec=importlib.util.spec_from_file_location('native_campaign_ai_path_ring_binding',
            Path(__file__).with_name('native_campaign_ai_workspace_replacements.py'))
        ring_helper=importlib.util.module_from_spec(ring_spec);ring_spec.loader.exec_module(ring_helper)
        output,path_ring_entries=ring_helper.install_path_ring(text,output)
        if path_ring_entries:receipt.setdefault('native_spatial_owners',[]).extend(path_ring_entries)
    if 'void source_run_death_cycle_runtime_tu15(' in text:
        from native_death_cycle_opening_replacements import install as install_death_cycle_opening
        output, death_cycle_opening = install_death_cycle_opening(text, output)
        if death_cycle_opening:
            receipt.setdefault('native_partial_frame_owners', {})['1030f0'] = (
                'Native opening resolves the object and tag datums, sets up the original scale and '
                'override queries, and rejoins the retained owner at 1031CB.')
            from native_death_cycle_activity_replacements import install as install_death_cycle_activity
            output, death_cycle_activity = install_death_cycle_activity(text, output)
            if death_cycle_activity:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native activity flags, tag-entry iteration, and motion dispatch retain the original '
                    '102D50 call and both authored branch tables.')
            from native_death_cycle_transition_replacements import install as install_death_cycle_transition
            output, death_cycle_transition = install_death_cycle_transition(text, output)
            if death_cycle_transition:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native descriptor and activity transitions retain all six original child calls and rejoin at 1032A7.')
            from native_death_cycle_expiration_replacements import install as install_death_cycle_expiration
            output, death_cycle_expiration = install_death_cycle_expiration(text, output)
            if death_cycle_expiration:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native motion completion and per-record timer expiration use the original record table.')
            from native_death_cycle_motion_replacements import install as install_death_cycle_motion
            output, death_cycle_motion = install_death_cycle_motion(text, output)
            if death_cycle_motion:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native motion-result handling retains the original 107610 and 108040 calls.')
            from native_death_cycle_timer_tail_replacements import install as install_death_cycle_timer_tail
            output, death_cycle_timer_tail = install_death_cycle_timer_tail(text, output)
            if death_cycle_timer_tail:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native timer updates and record advancement retain the original conversion and notification '
                    'calls, loop back-edge, and return.')
            from native_death_cycle_object_timer_replacements import install as install_death_cycle_object_timer
            output, death_cycle_object_timer = install_death_cycle_object_timer(text, output)
            if death_cycle_object_timer:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native object timer updates retain the original FC8D0, 10A910, and 108A70 calls.')
            from native_death_cycle_timer_notification_replacements import install as install_death_cycle_timer_notification
            output, death_cycle_timer_notification = install_death_cycle_timer_notification(text, output)
            if death_cycle_timer_notification:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native timer notifications preserve the original threshold and effect calls before 103657.')
            from native_death_cycle_record_route_replacements import install as install_death_cycle_record_route
            output, death_cycle_record_route = install_death_cycle_record_route(text, output)
            if death_cycle_record_route:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native record handlers preserve four jump-table entries, nine original calls, '
                    'and the shared 104371 continuation.')
            from native_death_cycle_record_blend_replacements import install as install_death_cycle_record_blend
            output, death_cycle_record_blend = install_death_cycle_record_blend(text, output)
            if death_cycle_record_blend:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native record blend and its two original BC820 calls rejoin at 1044F4.')
            from native_death_cycle_motion_finalize_replacements import install as install_death_cycle_motion_finalize
            output, death_cycle_motion_finalize = install_death_cycle_motion_finalize(text, output)
            if death_cycle_motion_finalize:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native motion finalization retains five original calls and rejoins at 1040F0.')
            from native_death_cycle_motion_cleanup_replacements import install as install_death_cycle_motion_cleanup
            output, death_cycle_motion_cleanup = install_death_cycle_motion_cleanup(text, output)
            if death_cycle_motion_cleanup:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native active and pending record cleanup retains both table entries and five original calls.')
            from native_death_cycle_motion_link_replacements import install as install_death_cycle_motion_link
            output, death_cycle_motion_link = install_death_cycle_motion_link(text, output)
            if death_cycle_motion_link:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native motion linking preserves three table entries, ten original calls, and the shared timer join.')
            from native_death_cycle_record_effects_replacements import install as install_death_cycle_record_effects
            output, death_cycle_record_effects = install_death_cycle_record_effects(text, output)
            if death_cycle_record_effects:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native record effects retain both authored jump tables, fourteen original calls, and four joins.')
            # Authenticate the shared finalization labels before adding their native predecessors.
            from native_death_cycle_motion_records_10387c_replacements import install as install_death_cycle_motion_records
            output, death_cycle_motion_records = install_death_cycle_motion_records(text, output)
            if death_cycle_motion_records:
                receipt['native_partial_frame_owners']['1030f0'] += (
                    ' Native indexed motion records retain eight original calls and their 103CD3 and 1040F0 joins.')
            if all((death_cycle_transition, death_cycle_object_timer, death_cycle_timer_notification,
                    death_cycle_activity, death_cycle_motion, death_cycle_motion_records,
                    death_cycle_record_effects, death_cycle_motion_finalize, death_cycle_motion_cleanup,
                    death_cycle_motion_link, death_cycle_expiration, death_cycle_record_route,
                    death_cycle_record_blend, death_cycle_timer_tail)):
                receipt['native_partial_frame_owners'].pop('1030f0')
                receipt.setdefault('native_spatial_owners', []).append('1030f0')
    if 'static void run_1f0670(WorldState &);' in text:
        from native_player_finished_1f0670_replacements import install as install_player_finished
        output, player_finished = install_player_finished(text, output)
        if player_finished:
            receipt.setdefault('native_partial_frame_owners', {})['1f0670'] = (
                'Native common frame, authored slot table load, and shared return paths.')
            from native_player_finished_angle_slot_replacements import install as install_angle_slot
            output, angle_slot = install_angle_slot(output)
            if angle_slot:
                receipt['native_partial_frame_owners']['1f0670'] += (
                    ' Native wrapped angle-delta slot and its two result exits.')
            from native_player_finished_weapon_slot_replacements import install as install_weapon_slot
            output, weapon_slot = install_weapon_slot(output)
            if weapon_slot:
                receipt['native_partial_frame_owners']['1f0670'] += (
                    ' Native weapon-status slot with original weapon and HUD calls.')
            from native_player_finished_hot_slots_replacements import install as install_hot_slots
            output, hot_slots = install_hot_slots(output)
            if hot_slots:
                receipt['native_partial_frame_owners']['1f0670'] += (
                    ' Native completion latches and multiplayer player-state predicates with the shared positive exit.')
            from native_player_finished_cold_slots_replacements import install as install_cold_slots
            output, cold_slots = install_cold_slots(output)
            if cold_slots:
                receipt['native_partial_frame_owners'].pop('1f0670')
                receipt.setdefault('native_spatial_owners', []).append('1f0670')
    if 'static void run_30aa70(WorldState &);' in text:
        from native_mopp_ray_traversal_replacements import install_prepared_entry
        output,mopp_ray_entry_owner=install_prepared_entry(text,output)
        if mopp_ray_entry_owner:
            receipt.setdefault('native_partial_frame_owners',{})['30aa70']=(
                'Native prologue, resident empty/jump cases, narrow and wide plane setup and comparison, '
                'axial endpoint order, slab bounds, intervals and delta comparisons, byte scale translations, '
                'packed scale setup, scalar scale completion, decreasing-child resumption, opcode resumption, '
                'axis byte-node setup, shared returns, collector inputs and results, and recursive arguments.')
    if 'static void run_30b890(WorldState &);' in text:
        from native_mopp_ray_traversal_replacements import install_setup_call
        output, mopp_ray_setup_owner = install_setup_call(text, output)
        receipt.setdefault('native_partial_frame_owners', {})['30b890'] = (
            'Native ray setup and return in the ' + mopp_ray_setup_owner +
            ' layout, with the original 30AA70 child and exact provider continuations.')
    if 'static void run_30caa0(WorldState &);' in text:
        from native_contact_cache_update_30cd00_replacements import install_reduction_scan
        output = install_reduction_scan(text, output)
        receipt.setdefault('native_partial_frame_owners', {})['30caa0'] = (
            'Native contact-reduction selection scans, frame setup, and record removal with the original '
            'metric spills, indirect removal call, and provider continuations.')
    if 'static void run_2e8d70(WorldState &);' in text:
        from native_sphere_center_bounds_replacements import install as install_sphere_center_bounds
        output, sphere_leaf_entries = install_sphere_center_bounds(text, output)
        receipt.setdefault('native_sphere_leaf_entries', []).extend(sphere_leaf_entries)
    if 'void source_run_contrail_geometry_tu15(' in text:
        from native_contrail_adjacent_sample_replacements import install as install_adjacent_sample
        from native_contrail_bitmap_selector_replacements import install as install_bitmap_selector
        from native_contrail_effect_launch_replacements import install as install_effect_launch
        from native_contrail_effect_projection_replacements import install as install_effect_projection
        from native_contrail_effect_tagged_lookup_replacements import install as install_effect_tagged_lookup
        from native_contrail_effect_default_vector_replacements import install as install_effect_default_vector
        from native_contrail_effect_authored_vector_replacements import install as install_effect_authored_vector
        from native_contrail_effect_authored_offset_replacements import install as install_effect_authored_offset
        from native_contrail_camera_facing_replacements import install as install_camera_facing
        from native_contrail_object_color_replacements import install as install_object_color
        from native_contrail_object_tint_replacements import install as install_object_tint
        from native_contrail_orientation_setup_replacements import install as install_orientation_setup
        from native_contrail_draw_frame_replacements import install as install_draw_frame
        from native_contrail_planar_facing_replacements import install as install_planar_facing
        from native_contrail_point_sample_replacements import install as install_point_sample
        from native_contrail_point_update_replacements import install as install_point_update
        from native_contrail_transition_curve_replacements import install as install_transition_curve
        from native_contrail_vertex_emission_replacements import install as install_vertex_emission
        from native_contrail_view_fade_replacements import install as install_view_fade
        output=install_vertex_emission(text,output)
        output=install_point_sample(text,output)
        output=install_view_fade(text,output)
        output=install_transition_curve(text,output)
        output=install_point_update(text,output)
        output=install_camera_facing(text,output)
        output=install_adjacent_sample(text,output)
        output=install_object_tint(text,output)
        output=install_orientation_setup(text,output)
        output=install_draw_frame(text,output)
        output=install_planar_facing(text,output)
        output=install_object_color(text,output)
        output=install_bitmap_selector(text,output)
        output=install_effect_launch(text,output)
        output=install_effect_projection(text,output)
        output=install_effect_tagged_lookup(text,output)
        output=install_effect_default_vector(text,output)
        output=install_effect_authored_vector(text,output)
        output=install_effect_authored_offset(text,output)
        receipt.setdefault('native_partial_frame_owners',{})['219e40']=(
            'Native signed admission, authored lookups, vector blending, and all RET8 paths. The genuine 21A2A0 call stays at its original boundary.')
        receipt.setdefault('native_partial_frame_owners',{})['219e00']=(
            'Native tagged lookup, child arguments and RET8. The real 219E40 call stays at its source boundary.')
        receipt.setdefault('native_partial_frame_owners',{})['219330']=(
            'Native effect frame, authored lookup arguments, launch coefficients, weighting, and origin projection. '
            'The projection tail and status return are native. The real lookup and 11E600 calls and allocation continuation retain their source boundaries.')
        receipt.setdefault('native_spatial_owners',[]).extend(('42c40','17f8a0','17f3d0','bcd80','13a070','21a2a0'))
        receipt.setdefault('native_partial_frame_owners',{})['42ce0']=(
            'Native point sampling and adjacent blending, object tint, orientation inputs and all supported strip orientations, '
            'point-pair output, clamped view fade, packed color, chain advance, and draw frame. '
            'The real BCD80 and 42C40 calls and the orientation dispatch stay at their original boundaries.')
    if 'void source_run_physics_live_support_tu15(' in text:
        from native_convex_process_prefix_2f8b60_replacements import install
        output, convex_process_prefix = install(text, output)
        if convex_process_prefix:
            receipt.setdefault('native_partial_frame_owners', {})['2f8b60'] = (
                'Native frame, cached distance projection and inverse transform arguments. '
                'The authored transform and cache removal call edges retain their original dispatch.')
        from native_two_bone_solve_replacements import install as install_two_bone_solve
        output, two_bone_entries = install_two_bone_solve(text, output)
        receipt.setdefault('native_spatial_owners', []).extend(two_bone_entries)
        from native_polygon_intersection_replacements import install as install_polygon_intersection
        output, polygon_entries = install_polygon_intersection(text, output)
        receipt.setdefault('native_spatial_owners', []).extend(polygon_entries)
        from native_havok_plane_314a30_replacements import install as install_havok_plane
        output, havok_plane_entries = install_havok_plane(text, output)
        receipt.setdefault('native_spatial_owners', []).extend(havok_plane_entries)
        from native_weapon_animation_selection_replacements import install as install_weapon_animation_selection
        output, weapon_animation_entries = install_weapon_animation_selection(text, output)
        receipt.setdefault('native_spatial_owners', []).extend(weapon_animation_entries)
        from native_bitmap_residency_replacements import install as install_bitmap_residency
        output, bitmap_entries = install_bitmap_residency(text, output)
        receipt.setdefault('native_spatial_owners', []).extend(bitmap_entries)
        from native_physical_result_activity_replacements import install as install_physical_result_activity
        output, activity_entries = install_physical_result_activity(text, output)
        receipt.setdefault('native_spatial_owners', []).extend(activity_entries)
        from native_pose_setup_c85a0_replacements import install as install_pose_setup
        output, pose_setup_entries = install_pose_setup(text, output)
        for entry in pose_setup_entries:
            receipt.setdefault('native_partial_frame_owners', {})[entry] = (
                'Native datum gates, pose spills, first-axis reset and default vector, including the early RET4. '
                'Genuine C6E50 and CDF30 calls remain at their original boundaries.')
        from native_pose_angle_c8930_replacements import install as install_pose_angle
        output, pose_angle_entries = install_pose_angle(text, output)
        for entry in pose_angle_entries:
            receipt.setdefault('native_partial_frame_owners', {})[entry] += (
                ' Native angle conversion and second-axis reset retain the original 32CC0C and CDF30 calls.')
        from native_pose_completion_c8b65_replacements import install as install_pose_completion
        output, pose_completion_entries = install_pose_completion(text, output)
        for entry in pose_completion_entries:
            receipt.setdefault('native_partial_frame_owners', {})[entry] += (
                ' Native output comparison and frame return retain the original BDA50 dispatch.')
        from native_pose_normalization_c8827_replacements import install as install_pose_normalization
        output, pose_normalization_entries = install_pose_normalization(text, output)
        for entry in pose_normalization_entries:
            receipt.setdefault('native_partial_frame_owners', {})[entry] += (
                ' Native pose normalization and angle clamps retain the original 11F7E0 and 122B60 calls.')
        from native_pose_first_axis_c8704_replacements import install as install_pose_first_axis
        from native_pose_second_axis_c89e3_replacements import install as install_pose_second_axis
        output, first_axis_entries = install_pose_first_axis(text, output)
        output, second_axis_entries = install_pose_second_axis(text, output)
        for entry in first_axis_entries:
            if entry in second_axis_entries:
                receipt['native_partial_frame_owners'].pop(entry)
                receipt.setdefault('native_spatial_owners', []).append(entry)
        from native_animation_request_replacements import install as install_animation_request
        output, animation_request_entries = install_animation_request(text, output)
        receipt.setdefault('native_spatial_owners', []).extend(animation_request_entries)
        from native_physical_result_contact_e4dc0_replacements import install as install_contact_transition
        output, contact_entries = install_contact_transition(text, output)
        receipt.setdefault('native_spatial_owners', []).extend(contact_entries)
        from native_physical_result_decay_e51d6_replacements import install as install_physical_result_decay
        output, decay_entries = install_physical_result_decay(text, output)
        receipt.setdefault('native_spatial_owners', []).extend(decay_entries)
        from native_physical_result_classification_e4b81_replacements import install as install_physical_result_classification
        output, classification_entries = install_physical_result_classification(text, output)
        receipt.setdefault('native_spatial_owners', []).extend(classification_entries)
        from native_physical_result_final_branches_replacements import install as install_physical_result_final_branches
        output, final_result_entries = install_physical_result_final_branches(text, output)
        receipt.setdefault('native_spatial_owners', []).extend(final_result_entries)
    if 'void source_run_weapon_hud_tu15(' in text:
        from native_weapon_hud_status_1055f0_replacements import install as install_weapon_hud_status
        from native_weapon_hud_status_reload_1055f0_replacements import install as install_weapon_hud_reload
        output, weapon_status = install_weapon_hud_status(text, output)
        if weapon_status:
            output, weapon_reload = install_weapon_hud_reload(output)
            if weapon_reload:
                receipt.setdefault('native_spatial_owners', []).append('1055f0')
            else:
                receipt.setdefault('native_partial_frame_owners', {})['1055f0'] = (
                    'Native HUD status frame, ordered packet clear, authored magazine queries, and return.')
    # The exact live-support owner pin includes the grouped retained POPs.
    if 'static void run_17e250(WorldState' in text:
        from native_effect_observer_retire_17e250_replacements import install as install_observer_retirement
        output, retirement_entries = install_observer_retirement(text, output)
        if retirement_entries:
            receipt.setdefault('native_spatial_owners', []).extend(retirement_entries)
    receipt['specialized_sha256']=digest(output)
    return output, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.source.resolve() == args.output.resolve():
        parser.error('Specialized operands must use a separate build-local output')
    output, receipt = specialize(args.source.read_text())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    # A binder update can rerun specialization for many independent units.
    # Keep their compiled objects reusable when their generated bytes agree.
    receipt_path = args.output.with_suffix('.operands.json')
    for path, content in ((args.output, output), (receipt_path, json.dumps(receipt, indent=2) + '\n')):
        if not path.is_file() or path.read_text() != content:
            path.write_text(content)
    print(json.dumps({key: value for key, value in receipt.items() if key != 'bindings'}))


if __name__ == '__main__':
    main()
