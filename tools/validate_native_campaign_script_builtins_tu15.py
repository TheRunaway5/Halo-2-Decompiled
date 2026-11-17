#!/usr/bin/env python3
"""Physical unchanged-i386 reference for native TU1.5 campaign HS builtins.

Each case enters the authenticated evaluator immediately after its original
argument evaluation (212010) with declared argument/state banks and stops at
its hs_return call (211DA0). The ScriptVm owns argument evaluation and result
delivery; this proves the evaluator tail: every ordered store to declared
state, the returned value and the final banks. No original instruction is
patched and no callee outcome is supplied. The original ELF, observer and raw
banks live only in HALO2_TEST_SCRATCH.

Cases stream in observer-chunk batches: each is generated (cases()' seed and
order), observed, compared and released, so memory stays bounded with every
factory track loaded. --shard K/N checks every Nth batch of the same case set.
"""
import argparse
from collections import deque
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import queue
import random
import resource
import struct
import subprocess
import sys
import threading
import time

from capstone import CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

BUILTIN_TABLE = 0x480c00
HS_ARGUMENTS, HS_RETURN = 0x212010, 0x211da0
DATA, DATA2, DATA3, BANK_SIZE = 0x1000000, 0x1100000, 0x1200000, 0x4000
STACK, SP, STOP = 0x2000000, 0x200f000, 0x3000000
# Each data-array header needs 0x48 bytes (+0x24 size, +0x38 count, +0x44 rows).
POOL, RECORDS = DATA, DATA + 0x60
GROUP_POOL, GROUP_RECORDS = DATA + 0x100, DATA + 0x160
LIST_HEADERS, LIST_HEADER_ROWS = DATA + 0x1b0, DATA + 0x210
LIST_REFS, LIST_REF_ROWS = DATA + 0x240, DATA + 0x2a0
ARG = DATA + 0x3c0
BODIES, BODY_SIZE, ROWS = DATA + 0x400, 0x400, 12
PHYSICS, TIME, EFFECTS, GC, SEED = DATA2, DATA2 + 0x40, DATA2 + 0x80, DATA2 + 0xc0, DATA2 + 0xd0
SLOTS, PVS = DATA2 + 0x100, DATA2 + 0x400
SCENARIO, INTERPOLATOR_ROWS, INTERPOLATORS = DATA2 + 0x1700, DATA2 + 0x1b00, DATA2 + 0x1c00
# Presentation owners: garbage/cinematic flags, screen (0x400), director,
# cinematic globals, users, AI globals and the 515E2C byte target.
GC3, SCREEN, DIRECTOR, CINEMATIC = DATA3, DATA3 + 0x100, DATA3 + 0x500, DATA3 + 0x540
USERS, AI_GLOBALS, BYTE_TARGET = DATA3 + 0x560, DATA3 + 0x580, DATA3 + 0x5c0
# Absolute original .data/.bss blocks: clock, FOV defaults, 515E00 globals,
# texture camera block and the cinematic shadow table with its count.
ABSOLUTE = {0x5020b4: 0x10, 0x56dfa0: 0x20, 0x4920c0: 8, 0x55d6e4: 12, 0x515e00: 0x100, 0x4c6190: 0x30, 0x4c6894: 0xc4, 0x4f5500: 0x540,
            0x473990: 0x14, 0x474c24: 4, 0x4f3ce0: 0x30, 0x4eaba0: 0x80}
GLOBALS = {0x4ecbd0: POOL, 0x4ecbf8: GROUP_POOL, 0x5020b0: LIST_HEADERS, 0x5020ac: LIST_REFS,
           0x52be68: PHYSICS, 0x51df54: TIME, 0x51df5c: EFFECTS, 0x4f3cd8: SEED,
           0x51daf4: SLOTS, 0x4f3218: PVS, 0x4ecc20: SCENARIO, 0x4f3010: INTERPOLATORS,
           0x51df50: CINEMATIC, 0x51df6c: DIRECTOR, 0x4f54f0: USERS, 0x5020a8: AI_GLOBALS,
           0x4f0414: DATA3 + 0x1000, 0x4f54f4: DATA3 + 0x2400, 0x4ecc0c: DATA + 0x3000, 0x4ecc18: DATA + 0x30c0}
# AI owners: actor (0x888), squad (0x98), encounter-group (0x38) and prop
# (0x50) data arrays, scenario squad definitions and their starting locations.
AI, AI_SIZE = 0x1300000, 0x8000
ACTOR_POOL, SQUAD_POOL, AI_GROUP_POOL, PROP_POOL = AI, AI + 0x50, AI + 0xa0, AI + 0xf0
SQUAD_ROWS, AI_GROUP_ROWS, PROP_ROWS = AI + 0x200, AI + 0x600, AI + 0x780
SQUAD_DEFS, STARTS, ACTOR_ROWS = AI + 0x900, AI + 0xc00, AI + 0x1600
ACTORS, SQUADS, AI_GROUPS = 6, 6, 6
# Prop knowledge (0x50EEF4, 0xC4) and perception (0x50EEF0, 0x3C) owners.
KNOW_POOL, PERCEPTION_POOL, KNOW_ROWS, PERCEPTION_ROWS = AI + 0x140, AI + 0x190, AI + 0x4940, AI + 0x4e00
KNOWLEDGE, PERCEPTIONS = 6, 8
SWARM_POOL, SWARM_ROWS, DAMAGE_INFO = AI + 0x5000, AI + 0x5100, DATA2 + 0x3b00
CHARACTER_TAGS, EQUIPMENT_ROWS = AI + 0x5200, AI + 0x5500
# B7E50 network owner (4DC04C): +4 state, +8 mode; state +20A4 flags rows (8 bytes),
# +2098 entries (36 bytes; flags at +20). Laid out in the AI bank's tail.
NETWORK_OWNER, NETWORK_STATE = AI + 0x5800, AI + 0x5a00
NAV_TABLE, PLAYER_BITMAP = AI + 0x7000, DATA3 + 0x2460
FLOCK_POOL, FLOCK_ROWS, FLOCK_BITMAP, FLOCK_DEFS = AI + 0x7100, AI + 0x7200, AI + 0x7300, AI + 0x7400
FLOCK_NAMES = [0x9000001, 0x9000002, 0x9000003]
TARGET_ROWS = AI + 0x7700
# 1903A0: scenario (+324) and globals ([+C4]+C) sound effect collections,
# tag rows 16/17, 0x38-byte effect rows named at +0.
EFFECT_COLLECTIONS, EFFECT_ROWS, EFFECT_BLOCK = AI + 0x7a00, AI + 0x7a40, AI + 0x7e00
EFFECT_NAMES = [0x10003995, 0x11003999, 0x1200399a, 0xd0033b1]
# 14E7B0: profile record, scenario starting profiles (0x34 rows), game
# globals (+138 representation count) and the 4F63B8 user block (+A mask,
# +C14 slot of the 56DFA0 table).
PROFILE, PROFILE_ROWS, GAME_GLOBALS, USER_BLOCK = DATA3 + 0x3900, DATA3 + 0x3c00, DATA3 + 0x3a00, AI + 0x6000
START_NAMES = [0x1110000, 0x2220001, 0x3330002, 0x4440003]
SEAT_NAMES = [0x5550005, 0x6660006, 0x7770007]
GLOBALS.update({0x5020c8: ACTOR_POOL, 0x52be7c: SQUAD_POOL, 0x52be80: AI_GROUP_POOL, 0x50eef8: PROP_POOL,
                0x50eef4: KNOW_POOL, 0x50eef0: PERCEPTION_POOL, 0x510fa8: SWARM_POOL,
                0x4dc04c: NETWORK_OWNER, 0x4ecc1c: GAME_GLOBALS, 0x4f63b8: USER_BLOCK, 0x50eed0: NAV_TABLE,
                0x52d88c: FLOCK_POOL})
# Havok owners whose virtual calls the declared fixtures never reach (objects
# without +B4 physics components). Executing one escapes the authenticated
# sites, which the observer rejects.
UNREACHED_VIRTUAL_OWNERS = {0x322660, 0x2e3250, 0x3165d0, 0x1cd6a0, 0x1d9100, 0x1d9080, 0x1d8eb0}
# Bounded switch tables (address -> entries, after the routine's own CMP/JA).
JUMP_TABLES = {0x27d7b4: 4, 0x27da08: 4, 0x27da18: 4, 0x27da28: 4, 0x27c984: 4}
NONE = 0xffffffff
# name -> (argument types, result kind) for the admitted overloads.
KERNELS = {
    'pin': ([6, 6, 6], 'real'), 'random_range': ([7, 7], 'short'), 'real_random_range': ([6, 6], 'real'),
    'garbage_collect_now': ([], 'void'), 'garbage_collect_unsafe': ([], 'void'),
    'physics_set_gravity': ([6], 'void'), 'physics_set_velocity_frame': ([6, 6, 6], 'void'),
    'physics_disable_character_ground_adhesion_forces': ([6], 'void'),
    'player_effect_set_max_rotation': ([6, 6, 6], 'void'), 'player_effect_start': ([6, 6], 'void'),
    'player_effect_stop': ([6], 'void'), 'pvs_set_object': ([50], 'void'), 'pvs_clear': ([], 'void'),
    'object_get_parent': ([50], 'long'), 'object_set_phantom_power': ([50, 5], 'void'),
    'object_set_deleted_when_deactivated': ([50], 'void'), 'object_set_shield_stun_infinite': ([50], 'void'),
    'object_get_shield': ([50], 'real'), 'object_get_health': ([50], 'real'), 'object_set_shield': ([50, 6], 'void'), 'object_clear_all_function_variables': ([50], 'void'),
    'unit_kill': ([51], 'void'), 'unit_kill_silent': ([51], 'void'),
    'device_operates_automatically_set': ([54, 5], 'void'), 'device_closes_automatically_set': ([54, 5], 'void'),
    'device_one_sided_set': ([54, 5], 'void'), 'device_set_never_appears_locked': ([54, 5], 'void'),
    'device_group_change_only_once_more_set': ([18, 5], 'void'),
    'device_get_position': ([54], 'real'), 'device_group_get': ([18], 'real'),
    'device_set_position_immediate': ([54, 6], 'void'), 'device_group_set_immediate': ([18, 6], 'void'),
    'unit_in_vehicle': ([51], 'boolean'), 'unit_only_takes_damage_from_players_team': ([51, 5], 'void'),
    'unit_doesnt_drop_items': ([31], 'void'), 'unit_is_emitting': ([51], 'boolean'),
    'vehicle_driver': ([51], 'long'), 'list_count_not_dead': ([31], 'short'),
    'cheat_active_camouflage_by_player': ([7, 5], 'void'), 'time_code_show': ([5], 'void'),
    'time_code_start': ([5], 'void'), 'interpolator_start': ([11, 6, 6], 'long'),
    'device_group_from_ordinal': ([7], 'long'),
    'object_cinematic_lod': (['live', 5], 'void'), 'unit_set_enterable_by_player': (['live', 5], 'void'),
    'object_clear_function_variable': (['live', 'name'], 'void'), 'texture_camera_off': ([], 'void'),
    'cinematic_shadow_control': ([5, 'shadow', 'name', 6], 'void'), 'cinematic_depth_of_field': ([6] * 7, 'void'),
    'camera_set_field_of_view': ([6, 7], 'void'), 'cinematic_stop': ([], 'void'),
    'object_function_value': (['fv_object', 'fv_name', 'out_value', 'out_slot'], 'boolean'),
    'object_set_function_variable': (['fv_object', 'fv_name', 6, 6], 'void'),
    'unit_set_emotion': (['live_only', 'emotion', 6, 6], 'void'),
    'device_animate_position': (['device', 'unit_real', 6, 6, 6, 5], 'void'),
    'device_animate_overlay': (['device', 'unit_real', 6, 6, 6], 'void'),
    'original_strncpy': (['copy_target', 'copy_source', 'copy_count'], 'void'),
    'texture_camera_set_object_marker': (['live', 'name', 6], 'void'),
    'object_get_markers': (['live_only', 'marker_name', 'marker_count', 'flag_byte', 'marker_out'], 'short'),
    'camera_control': ([5], 'void'),
    'volume_test_object': ([13, 50], 'boolean'), 'volume_test_objects': ([13, 31], 'boolean'),
    'volume_test_objects_all': ([13, 31], 'boolean'),
    'scenario_location': (['point_ptr', 'location_out'], 'void'),
    'objects_in_sphere': (['sphere_flags', 'type_mask', 'location_ptr', 'center_ptr', 'sphere_radius', 'sphere_out', 'sphere_max'], 'short'),
    'volume_return_objects': ([13], 'list'), 'volume_return_objects_by_type': ([13, 8], 'list'),
    'objects_can_see_flag': ([31, 14, 6], 'boolean'), 'objects_can_see_object': ([31, 50, 6], 'boolean'),
    'ai_vehicle_get_from_starting_location': ([19], 'long'), 'ai_vehicle_get': ([19], 'long'),
    'object_get_ai': ([50], 'long'), 'ai_spawn_count': ([19], 'short'), 'ai_combat_status': ([19], 'short'),
    'ai_disposable': ([19, 5], 'void'), 'ai_set_blind': ([19, 5], 'void'), 'ai_set_deaf': ([19, 5], 'void'),
    'ai_suppress_combat': ([19, 5], 'void'), 'ai_disregard': ([31, 5], 'void'), 'ai_magically_see_object': ([19, 50], 'void'), 'ai_vitality_pinned': ([19], 'boolean'), 'ai_renew': ([19], 'void'), 'ai_vehicle_reserve': ([52, 5], 'boolean'),
    'ai_vehicle_reserve_seat': ([52, 11, 5], 'boolean'), 'vehicle_test_seat': ([52, 11, 51], 'boolean'),
    'vehicle_test_seat_list': ([52, 11, 31], 'boolean'), 'object_set_velocity': ([50, 6, 6, 6], 'void'),
    'player_profile_normalize': (['profile'], 'void'),
    'weapon_hold_trigger': ([53, 8, 5], 'void'), 'activate_team_nav_point_object': ([27, 45, 50, 6], 'void'),
    'deactivate_team_nav_point_object': ([45, 50], 'void'), 'activate_team_nav_point_flag': ([27, 45, 14, 6], 'void'),
    'deactivate_team_nav_point_flag': ([45, 14], 'void'), 'unit_set_active_camo': ([51, 5, 6], 'void'),
    'biped_ragdoll': ([51], 'void'), 'units_set_maximum_vitality': ([31, 6, 6], 'void'),
    'player_flashlight_on': ([], 'boolean'), 'ice_cream_flavor_available': ([8], 'boolean'),
    'ai_set_active_camo': ([19, 5], 'void'), 'ai_berserk': ([19, 5], 'void'),
    'flock_start': ([11], 'boolean'), 'flock_stop': ([11], 'boolean'), 'ai_magically_see': ([19, 19], 'void'),
    'object_model_targets_destroyed': ([50, 11], 'short'), 'object_at_marker': ([50, 11], 'long'),
    'sound_effect_reference': (['effect_name'], 'long'), 'ai_prefer_target': ([31, 5], 'void'),
    # Object factory owners (docs/NATIVE_OBJECT_FACTORY.md).
    'object_memory_allocate': (['factory_alloc'], 'boolean'), 'object_memory_resize': (['factory_resize'], 'boolean'),
    'object_allocate': (['factory_object'], 'long'), 'object_block_allocate': (['factory_block'], 'boolean'),
    'object_model_tag_pair': (['leaf_pair'], 'boolean'), 'object_damage_info': (['leaf_object'], 'long'),
    'object_root_header_bit6': (['leaf_live'], 'long'), 'object_clear_block_12a': (['leaf_live'], 'void'),
    'object_queue_definition_flagged': (['leaf_live'], 'void'),
    'object_definition_physics_capacity': (['leaf_definition'], 'boolean'), 'object_memory_locked': ([], 'long'),
    'havok_record_initialize': (['leaf_record'], 'void'), 'havok_record_reset': (['leaf_record'], 'void'),
    'havok_record_clear_links': (['leaf_record'], 'void'),
    'object_free': (['factory_free'], 'void'), 'definition_object_type': (['leaf_type'], 'long'),
    'model_variant_index': (['leaf_variant'], 'long'), 'object_set_variant': (['leaf_set_variant'], 'void'),
    'object_node_matrices': (['leaf_matrices'], 'long'), 'object_retire_lens_queries': (['leaf_lens'], 'void'),
}
# Original non-evaluator routines admitted by the native host: name -> entry,
# argument register, return kind. They return through the declared stack.
ROUTINES = {
    'device_group_from_ordinal': dict(entry=0x10c180, regs={'eax': 0}),
    'object_cinematic_lod': dict(entry=0xbdf70, regs={'eax': 0}, stack=[1]),
    'unit_set_enterable_by_player': dict(entry=0x11db00, regs={'eax': 0}, stack=[1]),
    'object_clear_function_variable': dict(entry=0x10fd50, regs={'edi': 0, 'esi': 1}),
    'texture_camera_off': dict(entry=0x255c0),
    'cinematic_shadow_control': dict(entry=0x30e10, regs={'ebx': 0, 'esi': 1, 'edi': 2}, stack=[3]),
    'cinematic_depth_of_field': dict(entry=0x1fc10, xmm={0: 0}, stack=[1, 2, 3, 4, 5, 6]),
    'camera_set_field_of_view': dict(entry=0x16f5a0, stack=[0, 1]),
    'cinematic_stop': dict(entry=0x13f560),
    'object_function_value': dict(entry=0x10fb70, regs={'ebx': 0}, stack=[1, 2, 3]),
    'object_set_function_variable': dict(entry=0x10fc30, xmm={4: 3}, stack=[0, 1, 2]),
    'unit_set_emotion': dict(entry=0x111080, regs={'eax': 0, 'ecx': 1}, stack=[2, 3]),
    'device_animate_position': dict(entry=0x10d6b0, regs={'esi': 0}, stack=[1, 2, 3, 4, 5]),
    'device_animate_overlay': dict(entry=0x10d720, regs={'esi': 0}, stack=[1, 2, 3, 4]),
    'original_strncpy': dict(entry=0x32a4e0, stack=[0, 1, 2]),
    'texture_camera_set_object_marker': dict(entry=0x255e0, regs={'esi': 0}, stack=[1, 2]),
}
NAMES, COPY_SOURCE, COPY_TARGET = DATA3 + 0x700, DATA3 + 0xd00, DATA3 + 0xe00
TAG_TABLE, OBJECT_TAGS, MODEL_TAGS, RENDER_TAGS = DATA3 + 0x1000, DATA3 + 0x2c80, DATA2 + 0x2c80, DATA3 + 0x1400
NAME_TABLE, TRIGGERS, MODEL_NODES = DATA2 + 0x2000, DATA2 + 0x2a00, DATA2 + 0x2f00
# Spatial queries: collision BSP (+4 nodes, +C planes), structure BSP (+18
# collision, +30 leaves, +60 portals, +A0 clusters), cluster object-reference
# tables/pools, and fresh HS list pools for whole 11ED20 evaluators.
COLLISION, BSP_NODES, BSP_PLANES = DATA + 0x3000, DATA + 0x3010, DATA + 0x3040
STRUCTURE, BSP_LEAVES, BSP_CLUSTERS = DATA + 0x30c0, DATA + 0x3180, DATA + 0x31a0
PORTAL_INDICES, PORTALS, PORTAL_VERTICES = DATA + 0x3460, DATA + 0x3480, DATA + 0x3520
REF_TABLES, REF_POOLS, REF_ROWS = DATA + 0x35e0, DATA + 0x3600, DATA + 0x36a0
FRESH_HEADERS, FRESH_REFS, FRESH_END = DATA2 + 0x3500, DATA2 + 0x3620, DATA2 + 0x3980
LOCATION, CENTER, SPHERE_OUT = DATA3 + 0x3800, DATA3 + 0x3810, DATA3 + 0x3820
FLAGS = DATA2 + 0x3a00
PLAYERS, SEATS, CAMERA_CONTROL = DATA3 + 0x2400, DATA3 + 0x3480, DATA3 + 0x3780
MARKER_GROUPS, MARKERS, MARKER_OUT = DATA3 + 0x1600, DATA3 + 0x1800, DATA3 + 0x2200
MARKER_NAMES = [0x4000095, 0x2000123, 0x3000456]
ROUTINES['object_get_markers'] = dict(entry=0xbad60, regs={'ecx': 3, 'ebx': 4}, stack=[0, 1, 2])
ROUTINES['camera_control'] = dict(entry=0x158560, stack=[0])
ROUTINES['scenario_location'] = dict(entry=0x11e600, regs={'edx': 0, 'esi': 1})
ROUTINES['objects_in_sphere'] = dict(entry=0xbd080, stack=[0, 1, 2, 3, 4, 5, 6])
ROUTINES['player_profile_normalize'] = dict(entry=0x14e7b0, regs={'edx': 0})
ROUTINES['sound_effect_reference'] = dict(entry=0x1903a0, regs={'eax': 0})
# Factory owners: 1910A0 (ecx pool, eax tag) and 191130 (eax size, ebx slot)
# on the object memory pool, BE2B0 object datum+body, BE3B0 body block.
ROUTINES['object_memory_allocate'] = dict(entry=0x1910a0, regs={'ecx': 3, 'eax': 4}, stack=[0, 1, 2])
ROUTINES['object_memory_resize'] = dict(entry=0x191130, regs={'eax': 3, 'ebx': 4}, stack=[0, 1, 2])
ROUTINES['object_allocate'] = dict(entry=0xbe2b0, stack=[0])
ROUTINES['object_block_allocate'] = dict(entry=0xbe3b0, regs={'eax': 3}, stack=[0, 1, 2])
ROUTINES['object_model_tag_pair'] = dict(entry=0xc19d0, regs={'ecx': 2}, stack=[0, 1])
ROUTINES['object_damage_info'] = dict(entry=0xd80a0, regs={'ecx': 0})
ROUTINES['object_root_header_bit6'] = dict(entry=0xbbd50, regs={'eax': 0})
ROUTINES['object_clear_block_12a'] = dict(entry=0xbc400, regs={'eax': 0})
ROUTINES['object_queue_definition_flagged'] = dict(entry=0x10e440, regs={'edx': 0})
ROUTINES['object_definition_physics_capacity'] = dict(entry=0x1cc4d0, regs={'ecx': 0})
ROUTINES['object_memory_locked'] = dict(entry=0xba850)
ROUTINES['havok_record_initialize'] = dict(entry=0x1d3180, regs={'eax': 0})
ROUTINES['havok_record_reset'] = dict(entry=0x1d3270, regs={'eax': 0})
ROUTINES['havok_record_clear_links'] = dict(entry=0x1d3350, regs={'eax': 0})
ROUTINES['object_free'] = dict(entry=0xbe330, stack=[0])
ROUTINES['definition_object_type'] = dict(entry=0x10d950, stack=[0])
ROUTINES['model_variant_index'] = dict(entry=0x16ffe0, regs={'ecx': 0, 'edi': 1})
ROUTINES['object_set_variant'] = dict(entry=0xbc570, regs={'eax': 0, 'ecx': 1})
ROUTINES['object_node_matrices'] = dict(entry=0xbac30, regs={'eax': 1}, stack=[0])
ROUTINES['object_retire_lens_queries'] = dict(entry=0x2e1c0, regs={'eax': 0})
TYPE_GROUPS = [0x62697064, 0x76656869, 0x77656170, 0x65716970, 0x67617262, 0x70726f6a, 0x7363656e, 0x6d616368,
               0x6374726c, 0x6c696669, 0x73736365, 0x626c6f63, 0x63726561]
VARIANT_NAMES = [0x1000101, 0x2000202, 0x3000303]
# Object memory pool bank: pool object, kernel tick, occupancy bitmap of the
# object data array, direct-allocation slots and the heap.
FACTORY, FACTORY_SIZE = 0x1400000, 0x4000
# A larger per-kernel fixture bank for factory track owners (tag graphs,
# node arrays): world.track, mapped as .bank6 in the reference.
TRACK, TRACK_SIZE = 0x1500000, 0x10000
MEMORY_POOL, TICK, OBJECT_BITMAP, POOL_SLOTS, HEAP = FACTORY, FACTORY + 0x80, FACTORY + 0x100, FACTORY + 0x180, FACTORY + 0x400
OUT_VALUE, OUT_SLOT = DATA3 + 0x600, DATA3 + 0x604
EMOTIONS = [0x6000084, 0x70001a4, 0x50001a5, 0xb0001a6, 0x80001a7, 0xc0001a8, 0x70001a9, 0x60001aa, 0x80001ab,
            0x70001ac, 0x50001ad, 0x70001ae, 0x40001af]
FP_CONTROLS = [(0x027f, 0x1f80), (0x027f, 0x1fc0), (0x007f, 0x1f80), (0x037f, 0x1f80), (0x067f, 0x3f80),
               (0x0a7f, 0x5f80), (0x0e7f, 0x7f80), (0x027f, 0x9fc0)]
HALF_STORES = {'movhps', 'movlps'}
X87_STORES = {'fst', 'fstp', 'fist', 'fistp', 'fisttp', 'fnstcw', 'fstcw', 'fnstsw', 'fstsw', 'fbstp'}
REGISTERS = {name: n for n, name in enumerate(('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi'))}


# Factory track plugins: tools/native_object_factory_cases_*.py modules each
# define register(module), adding KERNELS/ROUTINES/JUMP_TABLES entries,
# TRACK_ARGUMENTS[argument type] = fn(name, world, rng) -> argument words and
# TRACK_COUNTS[kernel] = cases. Loaded after this module's tables exist.
TRACK_ARGUMENTS = {}
TRACK_COUNTS = {name: 400 for name in ('device_get_position', 'device_group_get',
    'device_set_position_immediate', 'device_group_set_immediate')}
# Global closure stops (subtrees no case reaches; reaching one escapes the
# admitted sites and fails) and declared register-indirect jump targets.
CLOSURE_STOPS, REGISTER_JUMPS = set(), {}


def load_tracks():
    # HALO2_FACTORY_TRACKS (comma-separated names) restricts the loaded
    # tracks, matching the CMake HALO2_NATIVE_OBJECT_FACTORY_TRACKS filter.
    import importlib.util
    module = sys.modules[__name__]
    selected = {t for t in os.environ.get('HALO2_FACTORY_TRACKS', '').split(',') if t}
    for path in sorted(Path(__file__).resolve().parent.glob('native_object_factory_cases_*.py')):
        if selected and path.stem.removeprefix('native_object_factory_cases_') not in selected:
            continue
        spec = importlib.util.spec_from_file_location(path.stem, path)
        track = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(track)
        track.register(module)


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def registrations(image):
    """Authenticate every admitted descriptor against the in-repo MCC layout."""
    layout = json.loads((ROOT / 'tools/mcc-script-layout.json').read_text())
    if layout['xbox_tu15_sha256'] != NEW_PIN:
        raise ValueError('MCC layout no longer names the authenticated TU1.5 executable')
    found = {}
    for name, (types, _) in KERNELS.items():
        if name in ROUTINES:
            found[name] = dict(evaluator=ROUTINES[name]['entry'], routine=True)
            continue
        rows = [b for b in layout['builtins'] if b['name'] == name and b.get('xbox_arguments') == types]
        if len(rows) != 1:
            raise ValueError(f'No unique TU1.5 overload for {name}{types}')
        row = rows[0]
        descriptor = struct.unpack('<I', image.read(BUILTIN_TABLE + 4 * row['xbox_opcode'], 4))[0]
        if descriptor != int(row['xbox_descriptor'], 16):
            raise ValueError(f'{name}: original descriptor pointer changed')
        raw = image.read(descriptor, 14 + 2 * len(types))
        if struct.unpack_from('<H', raw)[0] != row['xbox_return_type'] or struct.unpack_from('<H', raw, 12)[0] != len(types):
            raise ValueError(f'{name}: original signature changed')
        if list(struct.unpack_from(f'<{len(types)}H', raw, 14)) != types:
            raise ValueError(f'{name}: original argument types changed')
        found[name] = dict(opcode=row['xbox_opcode'], descriptor=descriptor, bytes=raw.hex(),
                           evaluator=struct.unpack_from('<I', raw, 4)[0])
    return found


def tail(code, evaluator):
    """Entry after original argument evaluation, argument registers and hs_return sites."""
    # Linear listing up to the final RET past every forward local branch;
    # some evaluators are packed without INT3 padding.
    pc, listing, stops, reach = evaluator, [], [], evaluator
    while True:
        instruction = code.at(pc)
        if instruction.mnemonic == 'int3':
            break
        listing.append(instruction)
        if instruction.mnemonic == 'call' and instruction.operands[0].imm == HS_RETURN:
            stops.append(pc)
        if instruction.mnemonic.startswith('j') and instruction.operands[0].type == 2:
            reach = max(reach, instruction.operands[0].imm)
        pc += instruction.size
        if instruction.mnemonic == 'ret' and pc > reach:
            break
    calls = [n for n, i in enumerate(listing) if i.mnemonic == 'call' and i.operands[0].imm == HS_ARGUMENTS]
    if not calls:
        return dict(entry=evaluator, argument_registers=[], stops=stops)
    if len(calls) != 1:
        raise ValueError(f'{evaluator:x}: expected one argument evaluation')
    registers = ['eax']
    # Prologue zeroing (XOR r,r) before 212010 is live at the entry point.
    zeroed = [i.op_str.split(',')[0] for i in listing[:calls[0]]
              if i.mnemonic == 'xor' and len(set(x.strip() for x in i.op_str.split(','))) == 1]
    for instruction in listing[calls[0] + 1:]:
        if instruction.mnemonic == 'mov' and instruction.op_str.endswith(', eax'):
            registers.append(instruction.op_str.split(',')[0])
        if instruction.mnemonic == 'je':
            return dict(entry=instruction.address + instruction.size, argument_registers=registers, stops=stops,
                        zeroed=zeroed)
    raise ValueError(f'{evaluator:x}: no argument admission branch')


def metadata(image, evaluators):
    code = _Code(image, '.text')
    sites, owners = {}, {}
    def closure(entry, stops=()):
        # Whole bodies of every direct callee reached after argument
        # evaluation. The HS argument/return services are never executed.
        # A routine's declared closure_stops (subtrees its cases never reach)
        # are excluded; reaching one escapes the admitted geometry and fails.
        pending, seen, pcs = [entry], set(), set()
        while pending:
            current = pending.pop()
            if current in seen or current in (HS_ARGUMENTS, HS_RETURN) or current in stops or current in CLOSURE_STOPS:
                continue
            seen.add(current)
            body = complete_body(code, current)
            pcs |= set(body.code)
            for pc in body.code:
                instruction = code.at(pc)
                if pc in REGISTER_JUMPS:
                    # Declared targets of a register-indirect jump (e.g. a CRT
                    # dispatch); an undeclared target escapes the sites.
                    pending.extend(REGISTER_JUMPS[pc])
                elif instruction.mnemonic == 'jmp' and instruction.operands[0].type == X86_OP_MEM:
                    memory = instruction.operands[0].mem
                    if memory.base or memory.scale != 4 or memory.disp not in JUMP_TABLES:
                        raise ValueError(f'{pc:x}: unbounded indirect jump in the admitted closure')
                    table = image.read(memory.disp, 4 * JUMP_TABLES[memory.disp])
                    pending.extend(struct.unpack(f'<{JUMP_TABLES[memory.disp]}I', table))
                elif instruction.mnemonic == 'jmp' and instruction.operands[0].type == 2 \
                        and instruction.operands[0].imm not in body.code:
                    pending.append(instruction.operands[0].imm)  # tail call (27CA60 -> 20D070)
                elif instruction.mnemonic == 'call' and instruction.operands[0].type == 2:
                    pending.append(instruction.operands[0].imm)
                elif instruction.mnemonic == 'call' and current not in UNREACHED_VIRTUAL_OWNERS:
                    raise ValueError(f'{current:x}: indirect call outside the admitted closure')
        return pcs

    for name, evaluator in sorted(evaluators.items()):
        body_code = closure(evaluator, set(ROUTINES.get(name, {}).get('closure_stops', ())))
        listing = []
        for pc in sorted(body_code):
            instruction = code.at(pc)
            listing.append(f'{pc:x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n')
            operands = []
            for operand in instruction.operands:
                if operand.type != X86_OP_MEM or instruction.mnemonic == 'lea':
                    continue
                base, index = instruction.reg_name(operand.mem.base), instruction.reg_name(operand.mem.index)
                read, write = bool(operand.access & CS_AC_READ), bool(operand.access & CS_AC_WRITE)
                if instruction.mnemonic in X87_STORES:
                    # Capstone reports x87 store destinations as reads.
                    read, write = False, True
                elif instruction.mnemonic in HALF_STORES and instruction.operands[0].type == X86_OP_MEM:
                    # Likewise MOVHPS/MOVLPS m64 destinations.
                    read, write = False, True
                operands.append(dict(base=REGISTERS[base] if base else -1, index=REGISTERS[index] if index else -1,
                                     scale=operand.mem.scale, displacement=operand.mem.disp & 0xffffffff,
                                     bytes=operand.size, read=read, write=write))
            sites[pc] = dict(instruction=pc, bytes=instruction.bytes.hex(), operands=operands)
            if {0xf2, 0xf3} & set(instruction.prefix) and instruction.mnemonic.startswith(('rep', 'stos', 'movs')):
                # A REP string operation with ECX 0 performs no access.
                sites[pc]['repeat_ecx'] = True
        owners[name] = dict(evaluator=evaluator, sites=len(body_code),
                            listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest())
    return dict(sites=list(sites.values()), stop=STOP, owners=owners), code


def prepare(image, meta, scratch):
    owned = scratch / 'physical-original'
    owned.mkdir()
    text, rdata, data = image.section('.text'), image.section('.rdata'), image.section('.data')
    (owned / 'text.bin').write_bytes(image.section_bytes(text))
    (owned / 'rdata.bin').write_bytes(image.section_bytes(rdata))
    (owned / 'data.bin').write_bytes(image.section_bytes(data))
    (owned / 'metadata.json').write_text(json.dumps(meta))
    rdata_size, data_size = len(image.section_bytes(rdata)), len(image.section_bytes(data))
    gap = data.va - (rdata.va + rdata_size)
    image_end = max(data.va + data_size, 0x573860)
    if gap < 0:
        raise ValueError('Original .rdata/.data layout changed')
    (owned / 'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{owned / 'text.bin'}"
.section .image,"aw"
.incbin "{owned / 'rdata.bin'}"
.space {gap}
.incbin "{owned / 'data.bin'}"
.space {image_end - data.va - data_size}
.section .bank,"aw"
.space {BANK_SIZE}
.section .bank2,"aw"
.space {BANK_SIZE}
.section .bank3,"aw"
.space {BANK_SIZE}
.section .bank4,"aw"
.space {AI_SIZE}
.section .bank5,"aw"
.space {FACTORY_SIZE}
.section .bank6,"aw"
.space {TRACK_SIZE}
.section .stack,"aw"
.space 65536
.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
''')
    (owned / 'link.ld').write_text(f'''ENTRY(_start)
SECTIONS {{
 .original 0x{text.va:x}:{{*(.original)}}
 .image 0x{rdata.va:x}:{{*(.image)}}
 .bank 0x{DATA:x}:{{*(.bank)}}
 .bank2 0x{DATA2:x}:{{*(.bank2)}}
 .bank3 0x{DATA3:x}:{{*(.bank3)}}
 .bank4 0x{AI:x}:{{*(.bank4)}}
 .bank5 0x{FACTORY:x}:{{*(.bank5)}}
 .bank6 0x{TRACK:x}:{{*(.bank6)}}
 .stack 0x{STACK:x}:{{*(.stack)}}
 .stop 0x{STOP:x}:{{*(.stop)}}
 .bridge 0x70000000:{{*(.bridge)}}
}}
''')
    for command in (['as', '--32', str(owned / 'reference.s'), '-o', str(owned / 'reference.o')],
                    ['ld', '-melf_i386', '-T', str(owned / 'link.ld'), str(owned / 'reference.o'), '-o', str(owned / 'reference')],
                    ['c++', '-std=c++20', '-O2', str(ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp'), '-o', str(owned / 'observer')]):
        result = subprocess.run(command, text=True, capture_output=True, check=False)
        if result.returncode:
            raise RuntimeError(' '.join(command) + '\n' + result.stderr[-3000:])
    return owned


class World:
    """Declared TU1.5 object/device/list/global state for one case."""

    def __init__(self, rng):
        self.rng = rng
        self.factory = None
        self.track = None  # TRACK bank bytes for factory track fixtures
        self.extra = {}  # per-kernel image .bss blocks (address -> bytes)
        self.data = bytearray(rng.getrandbits(8) for _ in range(BANK_SIZE))
        self.data2 = bytearray(rng.getrandbits(8) for _ in range(BANK_SIZE))
        self.data3 = bytearray(rng.getrandbits(8) for _ in range(BANK_SIZE))
        self.ai = bytearray(rng.getrandbits(8) for _ in range(AI_SIZE))
        self.absolute = {a: bytearray(rng.getrandbits(8) for _ in range(n)) for a, n in ABSOLUTE.items()}
        self.salts, self.handles = [], []
        self.put(POOL + 0x24, 12)
        self.put(POOL + 0x38, ROWS)
        self.put(POOL + 0x44, RECORDS)
        types = [0, 0, 0, 1, 1, 2, 7, 8, 9, 6, 7, 0]
        for n in range(ROWS):
            salt = rng.choice([0x8001, 0xe2a3, 0x1234, 0x7fff]) + n if not (n == 4 and rng.random() < .3) else 0
            salt &= 0xffff
            self.salts.append(salt)
            self.handles.append((salt << 16) | n)
            row = RECORDS + 12 * n
            struct.pack_into('<H', self.data, row - DATA, salt)
            self.data[row + 2 - DATA] = rng.choice([0, 1, 2, 3, 0x82, 0xfd])
            self.data[row + 3 - DATA] = rng.choice([types[n], types[n] | 0x20]) if n != 9 else types[n]
            self.put(row + 8, 0 if n == ROWS - 1 else self.body(n))
        for n in range(ROWS - 1):
            parent = NONE if n == 0 or rng.random() < .5 else self.handles[rng.randrange(n)]
            self.put(self.body(n) + 0x14, parent)
            self.put(self.body(n) + 0x346, rng.choice([0, 0x10, 0x100, 0x3f0]) | (0 if rng.random() < .8 else 0))
        self.put(GROUP_POOL + 0x24, 12)
        self.put(GROUP_POOL + 0x44, GROUP_RECORDS)
        self.groups = [((rng.choice([0x5a5a, 0x1111, 0x8888, 0xc001]) + g) << 16) | g for g in range(6)]
        for g, group in enumerate(self.groups):
            struct.pack_into('<H', self.data, GROUP_RECORDS + 12 * g - DATA, group >> 16)
            self.data[GROUP_RECORDS + 12 * g + 2 - DATA] = rng.choice([0, 1, 2, 3, 4, 5, 0x84, 0xfb])
        for n in (6, 7, 8, 10):
            for offset in (0x130, 0x13c):
                self.put(self.body(n) + offset, rng.choice(self.groups + [NONE, 0x12345678]))
        self.put(LIST_HEADERS + 0x44, LIST_HEADER_ROWS)
        self.put(LIST_REFS + 0x24, 12)
        self.put(LIST_REFS + 0x44, LIST_REF_ROWS)
        self.lists, self.members = [], []
        reference = 0
        for list_index in range(4):
            count = rng.randrange(0, 7)
            members = []
            for _ in range(count):
                choice = rng.random()
                members.append(NONE if choice < .08 else self.handles[rng.randrange(ROWS - 1)])
            handle = ((0x6000 + list_index) << 16) | list_index
            first = NONE
            for member in reversed(members):
                ref = ((0x7000 + reference) << 16) | reference
                row = LIST_REF_ROWS + 12 * reference
                self.put(row + 4, member)
                self.put(row + 8, first)
                first, reference = ref, reference + 1
            self.put(LIST_HEADER_ROWS + 12 * list_index + 8, first)
            self.lists.append(handle)
            self.members.append(members)
        struct.pack_into('<h', self.data2, TIME + 2 - DATA2, rng.choice([30, 60, 30, -7, 32767, 0, 29]))
        self.put(SEED, SEED + 8)
        # Game-time globals for 137BF0: +0 initialized byte, +4 tick seconds, +8 ticks.
        self.data2[TIME - DATA2] = rng.choice([0, 1, 1, 0x80])
        struct.pack_into('<f', self.data2, TIME + 4 - DATA2, rng.choice([1 / 30, 1 / 60, 0.0333, 1e-41]))
        self.put(TIME + 8, rng.choice([0, 1, 3000, 0x7fffffff, 0x80000001, 16777217, rng.getrandbits(32)]))
        self.interpolators = [rng.choice([0, 0x1d000a, 0x2b0011, 0x5f00ff, 0x7]) for _ in range(rng.randrange(0, 7))]
        self.put(SCENARIO + 0x3c0, len(self.interpolators) if rng.random() < .9 else 0xffffffff)
        self.put(SCENARIO + 0x3c4, INTERPOLATOR_ROWS)
        self.put(SCENARIO + 0xa0, rng.choice([0, 1, 3, 6, 6, 6]))
        for n, name in enumerate(self.interpolators):
            self.put(INTERPOLATOR_ROWS + 0x18 * n, name)
        self.globals = dict(GLOBALS)
        self.stack_words = []
        self.put(0x515ec0, SCREEN if rng.random() < .85 else 0)
        self.put(0x515e2c, BYTE_TARGET)
        clock = rng.choice([0.0, 1.5, 123.456789, 3600.0000001, 1e9 + 0.3, 2.0 ** 40 + 0.5])
        self.absolute[0x4920c0][:] = struct.pack('<d', clock)
        for offset, value in ((0, rng.choice([1.2217305, 1.0471976, 0.0, 1e-40])), (4, rng.choice([0.0, 0.017453, 1e-41])),
                              (8, rng.choice([3.1415927, 2.0, 1.0471976]))):
            struct.pack_into('<f', self.absolute[0x55d6e4], offset, value)
        for offset in (4, 8, 0xc, 0x10):
            struct.pack_into('<f', self.data3, DIRECTOR + offset - DATA3,
                             rng.choice([0.0, 0.5, 1.0, 1.2217305, 2.0, -0.25, 1e-40, float('nan'), 0.0333]))
        self.data3[CINEMATIC + 5 - DATA3] = rng.choice([0, 1, 1, 0x80])
        self.shadow_objects = [rng.choice(self.handles[:4] + [0x13370001, NONE]) for _ in range(16)]
        count = rng.choice([0, 1, 2, 5, 15, 16])
        for n, obj in enumerate(self.shadow_objects):
            self.put(0x4c6894 + 12 * n, obj)
        self.put(0x4c6954, count)
        for slot in range(32):
            if rng.random() < .4:
                self.put(SLOTS + 0x280 + 4 * slot, rng.choice(self.handles[:3]))
                self.put(SLOTS + 0x14 * slot, rng.choice([0x1000a, 0x20003, 0]))
            elif rng.random() < .3:
                self.put(SLOTS + 0x280 + 4 * slot, NONE)
            entry = SLOTS + 0x14 * slot
            struct.pack_into('<ff', self.data2, entry + 4 - DATA2, rng.uniform(-4, 4), rng.choice([0.0, 1.0, -2.5, 1e-40, 3.25]))
            self.put(entry + 0xc, rng.choice([0, 100, 2990, 3000, 0x7fffffff, 0x80000000, rng.getrandbits(32)]))
            self.put(entry + 0x10, rng.choice([0, 1, 10, 30, 300, -5]))
        # Marker owners: tag instances (16-byte rows, data at +8), object tags
        # (+2 inherit flag, +38 model), models (+4 render model), render
        # models (+58 group count, +5C groups), 12-byte groups, 0x24 markers.
        def tag_row(index, data):
            self.put(TAG_TABLE + 16 * index + 8, data)
        for k in range(4):
            tag_row(k, OBJECT_TAGS + 0x200 * k)
            self.data3[OBJECT_TAGS + 0x200 * k + 2 - DATA3] = rng.choice([0, 0x10, 0x10, 0xef, 0xff])
            self.put(OBJECT_TAGS + 0x200 * k + 0x38, rng.choice([(0x5000 + 4 + k) << 16 | (4 + k), NONE]) if k else (0x5004 << 16 | 4))
            self.put(OBJECT_TAGS + 0x200 * k + 0x1cc, SEATS)
            tag_row(4 + k, MODEL_TAGS + 0x80 * k)
            self.put(MODEL_TAGS + 0x80 * k + 4, rng.choice([(0x6000 + 8 + k) << 16 | (8 + k), NONE]) if k else (0x6008 << 16 | 8))
            self.put(MODEL_TAGS + 0x80 * k + 0x78, rng.choice([0, 1, 2, 4]))
            self.put(MODEL_TAGS + 0x80 * k + 0x7c, MODEL_NODES + 4 * 0x5c * k)
            for node in range(4):
                self.put(MODEL_NODES + (4 * k + node) * 0x5c, rng.choice([0x11110001, 0x22220002, 0x33330003, 0]))
            tag_row(8 + k, RENDER_TAGS + 0x60 * k)
            groups = rng.randrange(0, 4)
            self.put(RENDER_TAGS + 0x60 * k + 0x58, groups)
            self.put(RENDER_TAGS + 0x60 * k + 0x5c, MARKER_GROUPS + 0x30 * k)
            names = rng.sample(MARKER_NAMES + [0x9999], 4)
            for g in range(4):
                row = MARKER_GROUPS + 0x30 * k + 12 * g
                count = rng.choice([0, 1, 2, 3, 4])
                self.put(row, names[g])
                self.put(row + 4, count)
                markers = MARKERS + (16 * k + 4 * g) * 0x24 // 4 * 4
                markers = MARKERS + (k * 4 + g) * 4 * 0x24
                self.put(row + 8, markers)
                for m in range(4):
                    at = markers + 0x24 * m
                    self.data3[at - DATA3] = rng.choice([0xff, 0, 1, 2])
                    self.data3[at + 1 - DATA3] = rng.choice([0, 1, 2])
                    self.data3[at + 2 - DATA3] = rng.choice([0, 1, 2, 3])
                    quaternion = rng.choice([(0.0, 0.0, 0.0, 1.0), (0.0, 0.0, 0.0, 0.0), (0.5, 0.5, 0.5, 0.5),
                                             tuple(rng.uniform(-1, 1) for _ in range(4)), (1e-3, 0.0, 0.0, 0.0)])
                    struct.pack_into('<3f', self.data3, at + 4 - DATA3, *(rng.uniform(-2, 2) for _ in range(3)))
                    struct.pack_into('<4f', self.data3, at + 0x10 - DATA3, *quaternion)
                    struct.pack_into('<f', self.data3, at + 0x20 - DATA3, rng.uniform(0, 1))
        for n in range(ROWS - 1):
            body = self.body(n)
            self.put(body, (0x4000 + n % 4) << 16 | (n % 4))
            struct.pack_into('<hh', self.data, body + 0x116 - DATA, 0x260, 0)
            struct.pack_into('<h', self.data, body + 0x11a - DATA, 0x3a0)
            for b in range(8):
                self.data[body + 0x3a0 + b - DATA] = rng.choice([0, 1, 2])
            for node in range(4):
                values = [rng.choice([1.0, 0.5, 2.0])] + [rng.uniform(-1, 1) for _ in range(9)] + [rng.uniform(-50, 50) for _ in range(3)]
                struct.pack_into('<13f', self.data, body + 0x260 + 0x34 * node - DATA, *values)
            self.put(body + 0x154, rng.choice(self.handles[:ROWS - 1] + [NONE]))
            self.data[body + 0xaa - DATA] = rng.choice([0, 2, 3, 4, 6])
        # Spatial queries. Tree: node0 splits x, nodes 1/2 split y, four
        # leaves (some NONE) in four clusters joined by square portals.
        self.put(0x4eaba0, NAME_TABLE)
        self.put(0x4eabc4, GC3)
        struct.pack_into('<h', self.absolute[0x474c24], 0, rng.choice([0, 0, 0, 2, -1]))
        self.put(0x4f3ce4, rng.choice([5, 0x7fffffff, 0]))
        for c in range(8):
            self.put(0x4f3ce8 + 4 * c, rng.choice([0, 5, 6, 0x80000000]))
        self.put(0x4eabcc, rng.choice([9, 0xffffffff, 0]))
        for o in range(16):
            self.put(0x4eabd0 + 4 * o, rng.choice([0, 9, 10]))
        self.put(COLLISION + 4, BSP_NODES)
        self.put(COLLISION + 0xc, BSP_PLANES)
        planes = [(1, 0, 0, 0), (0, 1, 0, 0), (0, 1, 0, 0), (1, 0, 0, 0), (0, 0, 1, 2), (0.6, 0.8, 0, 1), (0, 0, -1, 0), (1, 0, 0, 3)]
        for n, plane in enumerate(planes):
            struct.pack_into('<4f', self.data, BSP_PLANES + 16 * n - DATA, *plane)
        def child(value):
            return value & 0xffffff
        def leaf(n):
            return 0xffffff if n is None else (0x800000 | n)
        leaves = [0, 1, rng.choice([2, None]), 3]
        for n, (plane, a, b) in enumerate([(0, 1, 2), (1, leaf(leaves[0]), leaf(leaves[1])), (2, leaf(leaves[2]), leaf(leaves[3]))]):
            node = BSP_NODES + 8 * n
            struct.pack_into('<h', self.data, node - DATA, plane)
            self.data[node + 2 - DATA:node + 5 - DATA] = child(a).to_bytes(3, 'little')
            self.data[node + 5 - DATA:node + 8 - DATA] = child(b).to_bytes(3, 'little')
        self.put(STRUCTURE + 0x18, COLLISION)
        self.put(STRUCTURE + 0x30, BSP_LEAVES)
        self.put(STRUCTURE + 0x60, PORTALS)
        self.put(STRUCTURE + 0xa0, BSP_CLUSTERS)
        for n in range(4):
            struct.pack_into('<h', self.data, BSP_LEAVES + 8 * n - DATA, rng.choice([n, n, (n + 1) % 4]))
        portal_rows = [(0, 1, 1), (1, 3, 0), (2, 3, 1), (0, 2, 0)]
        for n, (front, back, plane) in enumerate(portal_rows):
            portal = PORTALS + 36 * n
            struct.pack_into('<hhI', self.data, portal - DATA, front, back, rng.choice([plane, 4, 5]))
            struct.pack_into('<4f', self.data, portal + 8 - DATA, *(rng.uniform(-4, 4) for _ in range(3)), rng.choice([2.0, 6.0, 0.5]))
            count = rng.choice([4, 4, 3, 0])
            self.put(portal + 0x1c, count)
            self.put(portal + 0x20, PORTAL_VERTICES + 48 * n)
            for v in range(4):
                struct.pack_into('<3f', self.data, PORTAL_VERTICES + 48 * n + 12 * v - DATA,
                                 *(rng.choice([-5.0, 5.0, rng.uniform(-6, 6)]) for _ in range(3)))
        for c in range(4):
            row = BSP_CLUSTERS + 0xb0 * c
            mine = [n for n, (f, b, _) in enumerate(portal_rows) if c in (f, b)]
            self.put(row + 0x8c, len(mine) if rng.random() < .9 else 0)
            self.put(row + 0x90, PORTAL_INDICES + 8 * c)
            for k, n in enumerate(mine):
                struct.pack_into('<h', self.data, PORTAL_INDICES + 8 * c + 2 * k - DATA, n)
        self.put(0x4eabb0, REF_TABLES)
        self.put(0x4eabb4, REF_POOLS)
        self.put(0x4eaba4, REF_TABLES + 0x10)
        self.put(0x4eaba8, REF_POOLS + 0x50)
        for pool in range(2):
            header = REF_POOLS + 0x50 * pool
            self.put(header + 0x24, 0x20)
            self.put(header + 0x44, REF_ROWS + 0x180 * pool)
            for c in range(4):
                chain = rng.sample(range(12), rng.randrange(0, 4))
                first = NONE
                for r in reversed(chain):
                    at = REF_ROWS + 0x180 * pool + 0x20 * r
                    obj = rng.choice(self.handles[:ROWS - 1])
                    self.put(at + 4, obj)
                    self.put(at + 8, first)
                    self.data[at + 0xc - DATA] = rng.choice([0, 1, 2, 6, 7, 0x22])
                    point = struct.unpack_from('<3f', self.data, self.body(obj & 0xffff) + 0x30 - DATA)
                    struct.pack_into('<4f', self.data, at + 0x10 - DATA, *point, rng.choice([0.5, 1.0, 3.0]))
                    first = (0x9000 + r) << 16 | r
                self.put(REF_TABLES + 0x10 * pool + 4 * c, first)
        for header, rows, capacity in ((FRESH_HEADERS, 16, 16), (FRESH_REFS, 64, 64)):
            self.put(header + 0x20, capacity)
            self.put(header + 0x24, 12)
            for offset in (0x34, 0x38, 0x3c):
                self.put(header + offset, 0)
            struct.pack_into('<H', self.data2, header + 0x40 - DATA2, 0x8000)
            self.put(header + 0x44, header + 0x50)
            self.put(header + 0x48, header + 0x50 + 12 * rows)
            for word in range(2):
                self.put(header + 0x50 + 12 * rows + 4 * word, 0)
        for volume in range(8):
            struct.pack_into('<f', self.data2, TRIGGERS + 0x44 * volume + 0x3c - DATA2, rng.choice([0.0, 5.0, 20.0]))
        # Cutscene flags (0x38 rows, position at +24) and unit aim vectors.
        self.put(SCENARIO + 0x1e0, rng.choice([0, 3, 4]))
        self.put(SCENARIO + 0x1e4, FLAGS)
        for flag in range(4):
            struct.pack_into('<3f', self.data2, FLAGS + 0x38 * flag + 0x24 - DATA2, *(rng.uniform(-30, 30) for _ in range(3)))
        for n in range(ROWS - 1):
            aim = [rng.uniform(-1, 1) for _ in range(3)]
            length = sum(a * a for a in aim) ** .5 or 1.0
            struct.pack_into('<3f', self.data, self.body(n) + 0x18c - DATA, *(a / length for a in aim))
        # Trigger volumes (0x44 rows at scenario +10C): attached object name,
        # node name, forward/up/position and extents; object points (+30)
        # straddle the boundaries. The name table maps 640 names to handles.
        self.put(SCENARIO + 0x10c, TRIGGERS)
        for name in range(640):
            self.put(NAME_TABLE + 4 * name, rng.choice(self.handles[:ROWS - 1] + [NONE]) if name < 8 else NONE)
        for volume in range(8):
            row = TRIGGERS + 0x44 * volume
            struct.pack_into('<h', self.data2, row + 4 - DATA2, rng.choice([-1, -1, -1, 0, 1, 7, 0x300, -2]))
            self.put(row + 8, rng.choice([0x11110001, 0x22220002, 0x99990009]))
            if rng.random() < .6:
                forward, up = (1.0, 0.0, 0.0), (0.0, 0.0, 1.0)
            else:
                forward = tuple(rng.uniform(-1, 1) for _ in range(3)); up = tuple(rng.uniform(-1, 1) for _ in range(3))
            struct.pack_into('<3f', self.data2, row + 0xc - DATA2, *forward)
            struct.pack_into('<3f', self.data2, row + 0x18 - DATA2, *up)
            struct.pack_into('<3f', self.data2, row + 0x24 - DATA2, *(rng.uniform(-3, 3) for _ in range(3)))
            struct.pack_into('<3f', self.data2, row + 0x30 - DATA2, *(rng.choice([10.0, 4.0, 0.0, 1e-40, 20.0]) for _ in range(3)))
        for n in range(ROWS - 1):
            struct.pack_into('<3f', self.data, self.body(n) + 0x30 - DATA, *(rng.choice([rng.uniform(-6, 16), 0.0, 10.0, 4.0]) for _ in range(3)))
        # Camera control: user player handles, 0x21C player rows (+2C unit),
        # 0xB0 seats (bit4 result, bit6 classification), animation blocks
        # (+68/+0/+6 validity, +7C mode) and the five 473990 mode entries.
        self.put(0x4f5504, CAMERA_CONTROL)
        self.put(PLAYERS + 0x44, PLAYERS + 0x100)
        for user in range(4):
            player = rng.choice([NONE, (0x7700 + user) << 16 | user])
            self.put(USERS + 0xc + 4 * user, player)
            self.put(PLAYERS + 0x100 + 0x21c * user + 0x2c, rng.choice(self.handles[:5] + [NONE, 0x31310009]))
        for seat in range(4):
            self.put(SEATS + 0xb0 * seat, rng.choice([0, 0x10, 0x40, 0x50, 0xffffffff, rng.getrandbits(32)]))
        modes = [0x400004a, 0x1000001, 0x2000002, 0x3000003, 0x4000004]
        for n in range(5):
            self.put(0x473990 + 4 * n, rng.choice(modes))
        for n in range(ROWS - 1):
            body = self.body(n)
            struct.pack_into('<h', self.data, body + 0x12a - DATA, 0x380)
            struct.pack_into('<h', self.data, body + 0x1fc - DATA, rng.choice([0, 1, 2, 3, -1]))
            animation = body + 0x380
            self.put(animation, rng.choice([NONE, 0x12340001, 0x12340002]))
            struct.pack_into('<H', self.data, animation + 6 - DATA, rng.choice([0xffff, 0, 3]))
            self.put(animation + 0x68, rng.choice([NONE, 0, 5]))
            self.put(animation + 0x7c, rng.choice(modes + [0x7777777]))
        # Scenario object names (36-byte rows) and strncpy sources with NULs
        # at every alignment and high-bit bytes for the zero-detect trick.
        self.put(SCENARIO + 0x4c, NAMES)
        for row in range(16):
            text = bytes(rng.choice(b'abcdefghijklmnopqrstuvwxyz_0123456789\x80\xff') for _ in range(rng.randrange(0, 34)))
            self.data3[NAMES + 36 * row - DATA3:NAMES + 36 * row + 36 - DATA3] = (text + b'\0' * 36)[:36]
        source = bytes(rng.choice(b'ab\x80\xfe\x01' + bytes(4)) for _ in range(0x100))
        self.data3[COPY_SOURCE - DATA3:COPY_SOURCE + 0x100 - DATA3] = source
        for n in range(ROWS - 1):
            struct.pack_into('<h', self.data, self.body(n) + 0xac - DATA, rng.randrange(0, 16))
        for n in (6, 7, 8, 10):
            body = self.body(n)
            for offset in (0x14c, 0x16c):
                struct.pack_into('<ff', self.data, body + offset - DATA, rng.choice([0.0, 0.3, 1.0, -0.2, 1.7, float('nan')]),
                                 rng.choice([0.0, 0.05, -0.1, 2.0]))
        for n in range(ROWS - 1):
            face = rng.choice([0x100, 0x200, 0x300])
            struct.pack_into('<h', self.data, self.body(n) + 0x33e - DATA, face)
            if rng.random() < .2:
                struct.pack_into('<H', self.data, self.body(n) + face + 0x5e - DATA, 0xffff)
        # Last: AI links overwrite earlier random body bytes (+3A4/+3A8/+12C/+AA).
        self.ai_owners(rng)

    def ai_owners(self, rng):
        put = self.put
        put(ACTOR_POOL + 0x24, 0x888)
        put(ACTOR_POOL + 0x38, ACTORS)
        put(ACTOR_POOL + 0x44, ACTOR_ROWS)
        put(SQUAD_POOL + 0x44, SQUAD_ROWS)
        put(AI_GROUP_POOL + 0x44, AI_GROUP_ROWS)
        put(PROP_POOL + 0x44, PROP_ROWS)
        self.data3[AI_GLOBALS + 1 - DATA3] = rng.choice([1, 1, 1, 0x80, 0])
        self.actors = []
        for n in range(ACTORS):
            row = ACTOR_ROWS + 0x888 * n
            salt = (rng.choice([0x8001, 0x1234, 0xe002]) + n) & 0xffff if rng.random() > .15 else 0
            struct.pack_into('<H', self.ai, row - AI, salt)
            self.actors.append((((salt or 0x5555) if rng.random() > .25 else 0x4242) << 16) | n)
            unit = rng.randrange(ROWS - 1)
            put(row + 0x18, self.handles[unit])
            if unit > 3 and rng.random() < .5:
                # Seat the unit in vehicle row 3 (parents stay lower rows: no cycles).
                put(self.body(unit) + 0x14, self.handles[3])
            put(row + 0x20, NONE)
            put(row + 0x38, rng.choice(START_NAMES + [0x9990009]))
            put(row + 0x7c, rng.choice([NONE, NONE, 0, 1, 2, 3]))
            struct.pack_into('<h', self.ai, row + 0x86 - AI, rng.choice([-1, 0, 1, 2, 3, 4, 0x7fff, -0x8000]))
        # Disjoint actor chains: each squad's +68 list and the unassigned chain.
        owners = [rng.choice(list(range(SQUADS)) + [None, None]) for _ in range(ACTORS)]
        for n, owner in enumerate(owners):
            squad = owner if owner is not None and rng.random() < .9 else rng.choice([0, 1, 2, 3, 4, 5, NONE])
            put(ACTOR_ROWS + 0x888 * n + 0x30, squad)
        chains = {}
        for n in rng.sample(range(ACTORS), ACTORS):
            chains.setdefault(owners[n], []).append(n)
        for owner in list(range(SQUADS)) + [None]:
            first = NONE
            for n in reversed(chains.get(owner, [])):
                put(ACTOR_ROWS + 0x888 * n + 0x20, first)
                first = self.actors[n]
            if owner is None:
                put(AI_GLOBALS + 0x14, first)
            else:
                put(SQUAD_ROWS + 0x98 * owner + 0x68, first)
        # Squads: flags byte, spawn count, vehicle chain, group chain.
        vehicles = [self.handles[n] for n in range(ROWS - 1)]
        for n in range(ROWS - 1):
            if rng.random() < .5:
                self.data[self.body(n) + 0xaa - DATA] = rng.choice([0, 1, 1, 2])
            if rng.random() < .5:
                put(self.body(n) + 0x12c, rng.choice([NONE, 0, 3, 0x12340005]))
        if rng.random() < .8:
            self.data[self.body(3) + 0xaa - DATA] = 1
        pool = rng.sample(vehicles, len(vehicles))
        for squad in range(SQUADS):
            row = SQUAD_ROWS + 0x98 * squad
            self.ai[row + 2 - AI] = rng.choice([0, 1, 2, 3, 0x80, 0xff])
            struct.pack_into('<h', self.ai, row + 0x24 - AI, rng.choice([0, 1, 2, 5, -1, 0x7fff, -0x8000]))
            chain, pool = pool[:rng.randrange(0, 3)], pool[2:]
            first = NONE
            for vehicle in reversed(chain):
                body = self.body(vehicle & 0xffff)
                put(body + 0x3a4, first)
                put(body + 0x3a8, rng.choice(START_NAMES + [0x8880008]))
                first = vehicle
            put(row + 0x70, first)
            struct.pack_into('<h', self.ai, row + 0x74 - AI, -1)
            put(row + 0x6c, rng.choice(list(range(AI_GROUPS)) + [NONE]))
        # Encounter-group forest: parents precede children; squads chain per group.
        parents = [NONE] + [rng.choice([NONE] + list(range(g))) for g in range(1, AI_GROUPS)]
        for g in range(AI_GROUPS):
            row = AI_GROUP_ROWS + 0x38 * g
            put(row + 4, NONE)
            put(row + 8, NONE)
            put(row + 0xc, NONE)
            put(row + 0x10, parents[g])
        for g in reversed(range(1, AI_GROUPS)):
            if parents[g] != NONE:
                parent = AI_GROUP_ROWS + 0x38 * parents[g]
                put(AI_GROUP_ROWS + 0x38 * g + 0xc, struct.unpack_from('<I', self.ai, parent + 4 - AI)[0])
                put(parent + 4, g | (rng.getrandbits(16) << 16 if rng.random() < .3 else 0))
        homes = [rng.choice(list(range(AI_GROUPS)) + [None]) for _ in range(SQUADS)]
        for squad in reversed(range(SQUADS)):
            if homes[squad] is None:
                continue
            group = AI_GROUP_ROWS + 0x38 * homes[squad]
            struct.pack_into('<h', self.ai, SQUAD_ROWS + 0x98 * squad + 0x74 - AI,
                             struct.unpack_from('<h', self.ai, group + 8 - AI)[0])
            put(group + 8, squad)
        # Prop knowledge chains (prop +14, knowledge +14) and perception chains
        # (knowledge +18, perception +34); every row belongs to at most one chain.
        put(KNOW_POOL + 0x44, KNOW_ROWS)
        put(PERCEPTION_POOL + 0x44, PERCEPTION_ROWS)
        knowledge = rng.sample(range(KNOWLEDGE), KNOWLEDGE)
        for prop in range(4):
            chain, knowledge = knowledge[:rng.randrange(0, 3)], knowledge[2:]
            first = NONE
            for k in reversed(chain):
                put(KNOW_ROWS + 0xc4 * k + 0x14, first)
                first = k | (rng.getrandbits(16) << 16 if rng.random() < .3 else 0)
            put(PROP_ROWS + 0x50 * prop + 0x14, first)
        perceptions = rng.sample(range(PERCEPTIONS), PERCEPTIONS)
        for k in range(KNOWLEDGE):
            row = KNOW_ROWS + 0xc4 * k
            put(row + 8, rng.choice([self.handles[0], self.handles[0], self.handles[1], NONE]))
            put(row + 0x1c, rng.choice(self.actors + [rng.randrange(ACTORS), NONE]))
            chain, perceptions = perceptions[:rng.randrange(0, 3)], perceptions[2:]
            first = NONE
            for p in reversed(chain):
                put(PERCEPTION_ROWS + 0x3c * p + 0x34, first)
                first = p
            put(row + 0x18, first)
        seated = [n for n, owner in enumerate(owners) if owner is not None] or [0]
        for p in range(PERCEPTIONS):
            actor = rng.choice(seated) if rng.random() < .8 else rng.randrange(ACTORS)
            put(PERCEPTION_ROWS + 0x3c * p + 4, actor | (rng.getrandbits(16) << 16))
        # CAF20 seat trees: per-definition seat counts/names, +10A bit 2,
        # child (+10)/sibling (+C) links consistent with the +14 parents, and
        # the seat-bearing child flag (+4 bit 26).
        for k in range(4):
            put(OBJECT_TAGS + 0x200 * k + 0x1c8, rng.choice([0, 1, 2, 3]))
        for seat in range(4):
            put(SEATS + 0xb0 * seat + 4, rng.choice(SEAT_NAMES))
        children = {n: [] for n in range(ROWS - 1)}
        for n in range(ROWS - 1):
            body = self.body(n)
            parent = struct.unpack_from('<I', self.data, body + 0x14 - DATA)[0]
            if parent != NONE:
                children[parent & 0xffff].append(n)
            self.data[body + 0x10a - DATA] = rng.choice([0, 0, 4, 0xfb, 0xff])
            flags = struct.unpack_from('<I', self.data, body + 4 - DATA)[0] & ~(1 << 26)
            put(body + 4, flags | ((1 << 26) if rng.random() < .6 else 0))
        for n, kids in children.items():
            first = NONE
            for child in reversed(kids):
                put(self.body(child) + 0xc, first)
                first = self.handles[child]
            put(self.body(n) + 0x10, first)
        # Scenario squad definitions and starting locations (0x64 rows).
        put(SCENARIO + 0x160, rng.choice([SQUADS, SQUADS, 3, 0]))
        put(SCENARIO + 0x164, SQUAD_DEFS)
        for squad in range(SQUADS):
            definition = SQUAD_DEFS + 0x74 * squad
            put(definition + 0x48, rng.choice([0, 1, 2, 4]))
            put(definition + 0x4c, STARTS + 0x190 * squad)
            for location in range(4):
                put(STARTS + 0x190 * squad + 0x64 * location, rng.choice(START_NAMES))

    def body(self, n):
        return BODIES + BODY_SIZE * n

    def put(self, address, value):
        extra = ((FACTORY, self.factory),) if self.factory is not None else ()
        extra += ((TRACK, self.track),) if self.track is not None else ()
        for base, bank in ((DATA, self.data), (DATA2, self.data2), (DATA3, self.data3), (AI, self.ai),
                           *extra, *self.extra.items(), *self.absolute.items()):
            if base <= address and address + 4 <= base + len(bank):
                struct.pack_into('<I', bank, address - base, value & 0xffffffff)
                return
        raise ValueError(f'Undeclared world address {address:x}')

    def banks(self, arguments):
        args = bytearray(32)
        for n, value in enumerate(arguments):
            struct.pack_into('<I', args, 4 * n, value & 0xffffffff)
        self.data[ARG - DATA:ARG - DATA + 32] = args
        banks = [dict(address=address, before=struct.pack('<I', value).hex()) for address, value in sorted(self.globals.items())]
        stack = bytearray(0x10000)
        for n, word in enumerate(self.stack_words):
            struct.pack_into('<I', stack, SP - STACK + 12 + 4 * n, word & 0xffffffff)
        banks += [dict(address=DATA, before=self.data.hex()), dict(address=DATA2, before=self.data2.hex()),
                  dict(address=DATA3, before=self.data3.hex()), dict(address=AI, before=self.ai.hex())]
        if self.factory is not None:
            banks.append(dict(address=FACTORY, before=self.factory.hex()))
        if self.track is not None:
            banks.append(dict(address=TRACK, before=self.track.hex()))
        banks += [dict(address=a, before=b.hex()) for a, b in sorted(self.extra.items())]
        banks += [dict(address=a, before=b.hex()) for a, b in sorted(self.absolute.items())]
        banks.append(dict(address=STACK, before=stack.hex()))
        return banks


def pinned_fixture(world, rng):
    """27CFF0 inputs: model damage-info flags, +10A/+EC unit state, swarms."""
    put = world.put
    for k in range(4):
        put(MODEL_TAGS + 0x80 * k + 0x60, rng.choice([0, 1, 2, NONE]))
        put(MODEL_TAGS + 0x80 * k + 0x64, DAMAGE_INFO + 4 * k)
        world.data2[DAMAGE_INFO + 4 * k - DATA2] = rng.choice([0x80, 0, 0xff, 0x7f, 0xc0])
    values = [0.0, 0.0002, 0.00019999999, 0.00020001, 1.0, -1.0, 1e-40, float('nan'), 0.5]
    for n in range(ROWS - 1):
        body = world.body(n)
        struct.pack_into('<H', world.data, body + 0x10a - DATA, rng.choice([0, 0x4000, 0x8000, 0xc000]))
        struct.pack_into('<f', world.data, body + 0xec - DATA, rng.choice(values))
    # Some actors are swarms: unit NONE, +1C names a 0x34 swarm row whose +18
    # object chain continues through object +13A extension +C while +134 is 0.
    put(SWARM_POOL + 0x44, SWARM_ROWS)
    members = rng.sample(range(ROWS - 1), ROWS - 1)
    for swarm in range(4):
        chain, members = members[:rng.randrange(0, 3)], members[2:]
        first = NONE
        for n in reversed(chain):
            body = world.body(n)
            put(body + 0x134, 0 if rng.random() < .8 else 0x10)
            struct.pack_into('<h', world.data, body + 0x13a - DATA, 0x1f0)
            put(body + 0x1fc, first)
            first = world.handles[n]
        put(SWARM_ROWS + 0x34 * swarm + 0x18, first)
    for n in range(ACTORS):
        if rng.random() < .3:
            put(ACTOR_ROWS + 0x888 * n + 0x18, NONE)
            put(ACTOR_ROWS + 0x888 * n + 0x1c, rng.choice([NONE, 0, 1, 2, 3]))


def seat_fixture(world, rng):
    """Seat unit u (a later unit row, so parents stay acyclic) in seat s of
    unit-type vehicle v, whose definition carries at least s + 1 seats."""
    v = rng.randrange(0, 4)
    u = rng.randrange(v + 1, 5)
    s = rng.randrange(0, 2)
    body = world.body(v)
    world.data[body + 0x10a - DATA] &= 0xfb
    definition = OBJECT_TAGS + 0x200 * (v % 4) + 0x1c8
    count = struct.unpack_from('<I', world.data3, definition - DATA3)[0]
    world.put(definition, max(count, s + 1) if count < 0x80000000 else s + 1)
    world.put(world.body(u) + 0x14, world.handles[v])
    struct.pack_into('<h', world.data, world.body(u) + 0x1fc - DATA, s)
    name = struct.unpack_from('<I', world.data3, SEATS + 0xb0 * s + 4 - DATA3)[0]
    return v, u, s, name


MISC_FIXTURE = {'flock_start', 'flock_stop', 'ai_set_active_camo', 'ai_berserk', 'weapon_hold_trigger', 'activate_team_nav_point_object', 'deactivate_team_nav_point_object',
                'activate_team_nav_point_flag', 'deactivate_team_nav_point_flag', 'unit_set_active_camo',
                'units_set_maximum_vitality', 'player_flashlight_on', 'ice_cream_flavor_available', 'biped_ragdoll'}


def marker_child_fixture(world, rng):
    """DAE40 model target rows (model +68/+6C, 0x1C rows) and 10FF10 child
    attachment nodes (+18) and parent-relative positions (+64)."""
    put = world.put
    for k in range(4):
        put(MODEL_TAGS + 0x80 * k + 0x68, rng.choice([0, 1, 2, 3, NONE]))
        put(MODEL_TAGS + 0x80 * k + 0x6c, TARGET_ROWS + 0x60 * k)
        for row in range(3):
            put(TARGET_ROWS + 0x60 * k + 0x1c * row, rng.choice(MARKER_NAMES + [0x7000001]))
    # Live children only: DAE40 dereferences the body of an invalid sibling.
    children = {n: [] for n in range(ROWS - 1)}
    for n in range(ROWS - 1):
        parent = struct.unpack_from('<I', world.data, world.body(n) + 0x14 - DATA)[0]
        if parent != NONE and world.salts[n]:
            children[parent & 0xffff].append(n)
    for n, kids in children.items():
        first = NONE
        for child in reversed(kids):
            put(world.body(child) + 0xc, first)
            first = world.handles[child]
        put(world.body(n) + 0x10, first)
    for n in range(ROWS - 1):
        body = world.body(n)
        world.data[body + 0x18 - DATA] = rng.choice([0, 0, 1, 2, 0xff])
        struct.pack_into('<3f', world.data, body + 0x64 - DATA,
                         *(rng.choice([rng.uniform(-0.8, 0.8), rng.uniform(-3, 3), 0.0]) for _ in range(3)))


def misc_fixture(world, rng):
    """Player data array (+24/+38/+48 bitmap, rows +28 user, +C0 team, +2C
    unit), per-user nav points at 50EED0, weapon trigger rows, camouflage and
    vitality state, flashlight intensity and the 5020B4 flavor bytes."""
    put = world.put
    velocity_fixture(world, rng)
    put(PLAYERS + 0x24, 0x21c)
    put(PLAYERS + 0x38, rng.choice([0, 2, 4, 4]))
    put(PLAYERS + 0x48, PLAYER_BITMAP)
    put(PLAYER_BITMAP, rng.choice([0, 1, 5, 0xf, 0xe, 0xffffffff]))
    for n in range(4):
        row = PLAYERS + 0x100 + 0x21c * n
        struct.pack_into('<HH', world.data3, row - DATA3, rng.choice([0, 0x7100 + n, 0xe200 + n]), 0)
        struct.pack_into('<h', world.data3, row + 0x28 - DATA3, rng.choice([-1, 0, 1, 2, 3]))
        world.data3[row + 0xc0 - DATA3] = rng.choice([0, 1, 2, 3, 0xff, 0x80])
    targets = world.handles[:4] + [NONE, 0, 1, 2, 0xffff]
    for entry in range(16):
        at = NAV_TABLE + 12 * entry
        struct.pack_into('<HHfI', world.ai, at - AI, rng.choice([0xffff, 0, 1, 2]),
                         rng.choice([0xf, 0xffff, 0, 1, 0x10, 0xfff1, 0x2f]), rng.uniform(-2, 2), rng.choice(targets))
    for k in range(4):
        put(OBJECT_TAGS + 0x200 * k + 0x2c8, rng.choice([0, 1, 2, 3, NONE]))
    for n in range(ROWS - 1):
        body = world.body(n)
        for trigger in range(3):
            put(body + 0x20c + 12 * trigger + 4, rng.getrandbits(32))
        put(body + 0x2b8, rng.getrandbits(32))
        struct.pack_into('<f', world.data, body + 0x264 - DATA, rng.choice([1.0, 0.0, 0.5, float('nan'), 1.0000001]))
        world.data[body + 0x10a - DATA] = rng.choice([0, 4, 0x40, 0xff])
    for n in range(ACTORS):
        row = ACTOR_ROWS + 0x888 * n
        world.ai[row + 7 - AI] = rng.choice([0, 0, 1])
        world.ai[row + 0x225 - AI] = rng.choice([0, 1, 0xff])
    put(FLOCK_POOL + 0x24, 0x28)
    put(FLOCK_POOL + 0x38, rng.choice([0, 2, 4]))
    put(FLOCK_POOL + 0x44, FLOCK_ROWS)
    put(FLOCK_POOL + 0x48, FLOCK_BITMAP)
    put(FLOCK_BITMAP, rng.choice([0, 1, 6, 0xf]))
    put(SCENARIO + 0x354, FLOCK_DEFS)
    for n in range(4):
        struct.pack_into('<Hh', world.ai, FLOCK_ROWS + 0x28 * n - AI, rng.choice([0, 0x4100 + n]), rng.randrange(4))
        world.ai[FLOCK_ROWS + 0x28 * n + 0xe - AI] = rng.choice([0, 1, 0xff])
        put(FLOCK_DEFS + 0x84 * n + 0x80, rng.choice(FLOCK_NAMES))
    for flavor in range(16):
        world.absolute[0x5020b4][flavor] = rng.choice([0, 1, 2, 0xff])
    put(PVS + 8, rng.choice([1, 1, 2]))


def effect_fixture(world, rng):
    put = world.put
    for c in range(2):
        put(TAG_TABLE + 16 * (16 + c) + 8, EFFECT_COLLECTIONS + 0x20 * c)
        put(EFFECT_COLLECTIONS + 0x20 * c, rng.choice([0, 1, 2, 4, NONE]))
        put(EFFECT_COLLECTIONS + 0x20 * c + 4, EFFECT_ROWS + 0x1c0 * c)
        for row in range(4):
            put(EFFECT_ROWS + 0x1c0 * c + 0x38 * row, rng.choice(EFFECT_NAMES))
    put(SCENARIO + 0x324, rng.choice([16, 16, NONE]))
    put(GAME_GLOBALS + 0xc0, rng.choice([0, 1, 1]))
    put(GAME_GLOBALS + 0xc4, rng.choice([EFFECT_BLOCK, EFFECT_BLOCK, 0]))
    put(EFFECT_BLOCK + 0xc, rng.choice([17, 17, NONE]))


def factory_fixture(world, rng):
    """Object memory pool at [4EABBC] with a linked block heap, the object
    data array's allocation header (+20 max, +34 cursor, +38 extent, +3C
    count, +40 salt, +48 bitmap) and object bodies owned by heap blocks."""
    world.factory = bytearray(rng.getrandbits(8) for _ in range(FACTORY_SIZE))
    put = world.put
    world.globals[0x41b774] = TICK
    put(TICK, rng.getrandbits(32))
    put(0x4eabbc, MEMORY_POOL)
    debug = rng.random() < .3
    lead = 0x10 if debug else 0
    base = HEAP + rng.choice([0, 0x10, 0x40])
    size = rng.choice([0x400, 0x1000, 0x2000, 0x3800, 0x3b00])
    blocks, cursor = [], base
    for _ in range(rng.choice([0, 1, 2, 3, 5, 8, 8])):
        cursor += rng.choice([0, 0, 0x10, 0x30, 0x80, 0x200])
        total = rng.choice([0x20, 0x30, 0x60, 0x100, 0x210]) + (0x10 if debug else 0)
        if cursor + total > base + size:
            break
        blocks.append((cursor, cursor + lead, total))
        cursor += total
    for n, (start, block, total) in enumerate(blocks):
        put(block, total)
        put(block + 4, rng.choice([0, POOL_SLOTS + 4 * n]))
        put(block + 8, blocks[n + 1][1] if n + 1 < len(blocks) else 0)
        put(block + 0xc, blocks[n - 1][1] if n else 0)
        put(POOL_SLOTS + 4 * n, block + 0x10)
    used = sum(total for _, _, total in blocks)
    if blocks and rng.random() < .3:
        # Tight pool: the last block ends the pool, so growing it relocates
        # into an earlier gap (191220's last-block unlink).
        size = blocks[-1][0] + blocks[-1][2] - base
    put(MEMORY_POOL + 0x28, base)
    put(MEMORY_POOL + 0x2c, size)
    put(MEMORY_POOL + 0x30, size - used if rng.random() < .8 else rng.getrandbits(32))
    put(MEMORY_POOL + 0x34, blocks[0][1] if blocks else 0)
    put(MEMORY_POOL + 0x38, blocks[-1][1] if blocks else 0)
    world.factory[MEMORY_POOL + 0x3c - FACTORY] = rng.choice([0, 0, 1, 0x80])
    world.factory[MEMORY_POOL + 0x3d - FACTORY] = rng.choice([0, 1, 1, 0x40])
    world.factory[MEMORY_POOL + 0x3f - FACTORY] = rng.choice([1, 0x20]) if debug else 0
    world.blocks = blocks
    # Object data array allocation header; rows stay inside RECORDS.
    put(POOL + 0x20, rng.choice([ROWS, ROWS, ROWS - 1, 0]))
    cursor = rng.randrange(0, ROWS + 1)
    put(POOL + 0x34, cursor)
    put(POOL + 0x38, rng.choice([ROWS - 1, rng.randrange(cursor, ROWS + 1), cursor]))
    put(POOL + 0x3c, rng.randrange(0, ROWS + 1))
    struct.pack_into('<H', world.data, POOL + 0x40 - DATA, rng.choice([0x8000, 0x8123, 0xfffe, 0xffff]))
    put(POOL + 0x48, OBJECT_BITMAP)
    put(OBJECT_BITMAP, rng.choice([0, 0xfff, 0x7ff, 0xbff, rng.getrandbits(32)]))
    # Object bodies inside large heap blocks; their block owner is row +8.
    world.heap_objects = []
    for n, (start, block, total) in enumerate(blocks):
        capacity = total - 0x10 - lead
        if capacity < 0x40 or rng.random() < .3 or len(world.heap_objects) >= ROWS - 1:
            continue
        row_index = len(world.heap_objects)
        row = RECORDS + 12 * row_index
        put(row + 8, block + 0x10)
        put(block + 4, row + 8)
        struct.pack_into('<h', world.data, row + 6 - DATA, capacity - rng.choice([0, 0, 4, 0x10, 0x20]))
        world.heap_objects.append(row_index)


def leaf_fixture(world, rng):
    """Factory leaf inputs: object tags/types/flags, model tag pairs, render
    model counts, damage-info rows, the 10E440 queue, physics capacity and
    lock globals, and a Havok record extent in the factory bank."""
    world.factory = bytearray(rng.getrandbits(8) for _ in range(FACTORY_SIZE))
    put = world.put
    for n in range(ROWS - 1):
        body = world.body(n)
        put(body, (rng.getrandbits(16) << 16) | rng.randrange(4))
        struct.pack_into('<h', world.data, body + 0x12a - DATA, rng.choice([0, 0x10, 0x100, 0x2f0, 0x398]))
    for k in range(4):
        definition = OBJECT_TAGS + 0x200 * k
        world.data3[definition - DATA3] = rng.choice(list(range(13)) + [0x20, 0x41, 0xff])
        world.data3[definition + 2 - DATA3] = rng.choice([0, 0x40, 0x50, 0xbf, 0xff])
        model = MODEL_TAGS + 0x80 * k
        put(model + 0xc, rng.choice([NONE, (0x6100 + k) << 16 | (8 + k), (0x6200 << 16) | 9]))
        put(model + 0x60, rng.choice([0, 1, 2, NONE, 0x7fffffff, 0x80000000]))
        put(model + 0x64, rng.getrandbits(32))
        render = RENDER_TAGS + 0x60 * k
        put(render + 0x74, rng.choice([0, 1, 2, NONE, 0x80000000]))
        put(render + 0x7c, rng.choice([0, 1, 3, NONE]))
    world.globals[0x51daf0] = FACTORY + 0x200 - 0x1d00
    put(FACTORY + 0x280, rng.choice([0, 1, 5, 31, 32, 33, NONE, 0x80000000]))
    world.globals[0x48b870] = rng.choice([0, 1, 0x100, 0xff])
    world.globals[0x52be44] = FACTORY + 0x300
    put(FACTORY + 0x300, rng.choice([0, 0x1ff, 0x200, 0x201, NONE]))
    world.globals[0x52be5c] = FACTORY + 0x340
    put(FACTORY + 0x360, rng.choice([0, 5, 0x40]))
    put(FACTORY + 0x37c, rng.choice([0, 4, 5, 6, NONE]))
    put(0x4eabc4, rng.choice([0, FACTORY + 0x380, FACTORY + 0x380]))
    world.factory[0x380] = rng.choice([0, 1, 0x80])


def variant_fixture(world, rng):
    """Definition default variants (+30), model variants (+50 count, +54 rows
    of 0x38 named at +0) and tag-instance group tags."""
    put = world.put
    for n in range(-1, 16):
        # Row -1 is what a sign-extended 0xFFFF tag index selects.
        put(TAG_TABLE + 16 * n, rng.choice(TYPE_GROUPS + [0x6d6f6465, 0]))
    for k in range(4):
        put(OBJECT_TAGS + 0x200 * k + 0x30, rng.choice(VARIANT_NAMES + [0, 0x7777]))
        put(MODEL_TAGS + 0x80 * k + 0x50, rng.choice([0, 1, 2, 3, NONE]))
        put(MODEL_TAGS + 0x80 * k + 0x54, FACTORY + 0x1000 + 0x100 * k)
        for row in range(3):
            put(FACTORY + 0x1000 + 0x100 * k + 0x38 * row, rng.choice(VARIANT_NAMES + [0x4444]))


def lens_fixture(world, rng):
    """2E1C0 lens query slots: count [531FBC], 0x20-byte records at 52DFDC
    (flags +0, owner index in bits 6..17) and the three visibility banks."""
    count = rng.choice([0, 1, 5, 20, 20])
    world.extra[0x52dfdc] = bytearray(rng.getrandbits(8) for _ in range(20 * 0x20))
    world.extra[0x531fbc] = bytearray(rng.getrandbits(8) for _ in range(0xc4))
    world.put(0x531fbc, count)
    for slot in range(20):
        owner = rng.choice([rng.randrange(ROWS), rng.randrange(0x1000)])
        flags = (rng.getrandbits(32) & ~0x3ffc0) | (owner << 6)
        world.put(0x52dfdc + 0x20 * slot, flags | (0x10 if rng.random() < .6 else 0) if rng.random() < .8 else flags & ~0x10)


def profile_fixture(world, rng):
    """14E7B0 inputs around the record's clamped bytes and words."""
    put = world.put
    put(PVS + 8, rng.choice([1, 1, 2, 2, 0, 3]))
    world.data2[PVS + 0x184 - DATA2] = rng.choice([0, 1, 0xfe])
    put(SCENARIO + 0x100, rng.choice([0, 1, 3, 4, NONE]))
    put(SCENARIO + 0x104, PROFILE_ROWS)
    for row in range(4):
        struct.pack_into('<h', world.data3, PROFILE_ROWS + 0x34 * row + 0x2c - DATA3, rng.choice([-1, -1, 0, 1, 5, 0x1ff]))
    put(GAME_GLOBALS + 0x138, rng.choice([0, 1, 2, 4, 6]))
    put(USER_BLOCK + 0xc14, rng.randrange(0, 8))
    struct.pack_into('<H', world.ai, USER_BLOCK + 0xa - AI, rng.choice([0, 1, 3, 0x10, 0xffff]))
    for slot in range(8):
        put(0x56dfa0 + 4 * slot, rng.choice([0, 0x12345678]))
    record = PROFILE
    bytes_ = lambda: rng.choice([0, 1, 2, 3, 5, 7, 8, 0x7f, 0x80, 0xfe, 0xff])
    for offset in (0x44, 0x7c, 0x7d, 0x7e, 0x7f, 0x81):
        world.data3[record + offset - DATA3] = bytes_()
    put(record + 0x84, rng.choice([NONE, 0, 3, 15, 16, 0x80000000]))
    put(record + 0x8c, rng.choice([0, 5, -1 & NONE, 0x40000000, 0x3fffffff, 0x7fffffff]))
    struct.pack_into('<hh', world.data3, record + 0x88 - DATA3, rng.choice([-1, 0, 5, 0x7f, 0x80, 0x7fff]),
                     rng.choice([-1, 0, 5, 0x7f, 0x80, -0x8000]))


def velocity_fixture(world, rng):
    """1100C0 without Havok components (+B4 NONE): +70 forward / +7C up,
    +C0/+C1 wake bytes, +D4 network entry with the B7E50 owner at 4DC04C."""
    put = world.put
    for n in range(ROWS - 1):
        body = world.body(n)
        forward = [rng.uniform(-1, 1) for _ in range(3)]
        up = [rng.uniform(-1, 1) for _ in range(3)]
        if rng.random() < .1:
            forward[rng.randrange(3)] = rng.choice([float('nan'), float('inf'), 1e-40, 0.0])
        struct.pack_into('<6f', world.data, body + 0x70 - DATA, *forward, *up)
        put(body + 0xb4, NONE)
        world.data[body + 0xc0 - DATA] = rng.choice([0, 0x40, 0xff])
        world.data[body + 0xc1 - DATA] = rng.choice([0, 1, 0xff])
        put(body + 0xd4, rng.choice([NONE, 3, 0x40005]))
        world.data[RECORDS + 12 * n + 2 - DATA] = rng.choice([0, 1, 2, 0x80, 0x81, 0x83])
    put(NETWORK_OWNER + 8, rng.choice([4, 4, 3, 5, 0]))
    put(NETWORK_OWNER + 4, NETWORK_STATE)
    put(NETWORK_STATE + 0x20a4, NETWORK_STATE + 0x40)
    for index in (3, 5):
        world.ai[NETWORK_STATE + 0x40 + index * 8 + 0x44 - AI] = rng.choice([0, 4, 0xff])


def renew_fixture(world, rng):
    """27F5C0/1ED690 inputs: character tags 12..14 (parent +8, 0x3C equipment
    rows at +E0 with type +4 and count bounds +34/+36) and unit +E4/+E8/+23C."""
    put = world.put
    for c in range(3):
        tag = CHARACTER_TAGS + 0x100 * c
        put(TAG_TABLE + 16 * (12 + c) + 8, tag)
        put(tag + 8, rng.choice([NONE] + [12 + p for p in range(c)]))
        put(tag + 0xdc, rng.choice([0, 1, 2, 3]))
        put(tag + 0xe0, EQUIPMENT_ROWS + 0xb4 * c)
        for r in range(3):
            row = EQUIPMENT_ROWS + 0xb4 * c + 0x3c * r
            low = rng.choice([0, 1, 2, -1, 5])
            struct.pack_into('<hhh', world.ai, row + 0x34 - AI, low, rng.choice([low, low + 1, low + 3, low - 1, 7]), 0)
            struct.pack_into('<h', world.ai, row + 4 - AI, rng.choice([0, 1, 2, 3]))
    for n in range(ACTORS):
        put(ACTOR_ROWS + 0x888 * n + 0x54, rng.choice([12, 13, 14, 12, NONE]))
    values = [0.0, 1.0, -1.0, 1e-40, 250.0, float('nan'), -0.0]
    for n in range(ROWS - 1):
        body = world.body(n)
        struct.pack_into('<ff', world.data, body + 0xe4 - DATA, rng.choice(values), rng.choice(values))
        world.data[body + 0x23c - DATA] = rng.choice([0xff, 0, 1, 2, 3])
        for slot in range(4):
            world.data[body + 0x23e + slot - DATA] = rng.choice([0, 1, 2, 4, 0xff, 0x80])


def floats(rng):
    special = [0., -0., .5, 1., 1.5, 2.5, -2.5, 3.75, 100., -1., 5., 5.000001, 1e-40, -1e-42, 3.4e38, float('inf'),
               float('-inf'), 0.05, 0.0333, 0.1, 7.1]
    value = rng.choice(special) if rng.random() < .6 else rng.uniform(-50, 50)
    return struct.unpack('<I', struct.pack('<f', value))[0]


def arguments(name, world, rng):
    living = [h for n, h in enumerate(world.handles[:ROWS - 1])]
    any_handle = world.handles + [NONE, (0x4444 << 16) | 3, (0x1234 << 16) | 0x7ff0]
    types = KERNELS[name][0]
    if types and types[0] in TRACK_ARGUMENTS:
        return TRACK_ARGUMENTS[types[0]](name, world, rng), []
    values, members = [], []
    if name in ('device_get_position', 'device_group_get',
                'device_set_position_immediate', 'device_group_set_immediate'):
        # Immediate setters visit both group memberships and recursively mark
        # child devices. Build a real acyclic hierarchy with siblings, mixed
        # device/non-device children and independently matching group salts.
        for n in range(ROWS - 1):
            body = world.body(n)
            world.put(body + 0xc, NONE)
            world.put(body + 0x10, NONE)
            world.data[body + 0xaa - DATA] = [0, 0, 0, 1, 1, 2, 7, 8, 9, 6, 7][n]
        if rng.random() < .8:
            for parent, children in ((6, [1, 7]), (7, [8, 3]), (8, [10])):
                world.put(world.body(parent) + 0x10, world.handles[children[0]])
                for index, child in enumerate(children):
                    world.put(world.body(child) + 0x14, world.handles[parent])
                    world.put(world.body(child) + 0xc,
                              world.handles[children[index + 1]] if index + 1 < len(children) else NONE)
            # Parents outside this tree terminate; no incidental parent cycle.
            world.put(world.body(6) + 0x14, NONE)
        group = rng.choice(world.groups)
        for n in (6, 7, 8, 10):
            for offset in (0x130, 0x13c):
                world.put(world.body(n) + offset, rng.choice([group, group, NONE, rng.choice(world.groups)]))
        if name == 'device_group_get':
            # The original getter has no NONE guard after argument resolution.
            return [group], []
        if name == 'device_get_position':
            return [rng.choice([world.handles[n] for n in (6, 7, 8, 10)] + [NONE])], []
        if name == 'device_group_set_immediate':
            return [rng.choice([group, group, NONE]), floats(rng)], []
        return [rng.choice([world.handles[n] for n in (6, 7, 8, 10)] + [NONE]), floats(rng)], []
    unchecked = name in ('object_model_targets_destroyed', 'object_at_marker', 'weapon_hold_trigger', 'activate_team_nav_point_object', 'deactivate_team_nav_point_object',
                         'unit_set_active_camo', 'biped_ragdoll', 'object_set_velocity', 'ai_vehicle_reserve', 'ai_vehicle_reserve_seat', 'vehicle_test_seat',
                         'vehicle_test_seat_list', 'ai_magically_see_object', 'object_get_ai', 'objects_can_see_object', 'volume_test_object', 'object_set_shield', 'object_get_parent', 'object_set_phantom_power', 'object_set_deleted_when_deactivated',
                         'object_set_shield_stun_infinite', 'object_clear_all_function_variables', 'unit_kill',
                         'unit_kill_silent', 'unit_in_vehicle', 'unit_only_takes_damage_from_players_team',
                         'unit_is_emitting')
    if name == 'object_memory_locked':
        leaf_fixture(world, rng)
    if name == 'ai_vitality_pinned':
        pinned_fixture(world, rng)
    if name == 'ai_renew':
        renew_fixture(world, rng)
    if name == 'object_set_velocity':
        velocity_fixture(world, rng)
    if name in MISC_FIXTURE:
        misc_fixture(world, rng)
    if name in ('object_model_targets_destroyed', 'object_at_marker'):
        marker_child_fixture(world, rng)
    seated = None
    if name in ('vehicle_test_seat', 'vehicle_test_seat_list') and rng.random() < .7:
        seated = seat_fixture(world, rng)
    for argument_type in types:
        if argument_type == 'live':
            values.append(rng.choice(living + [NONE]))
            continue
        if argument_type == 'name':
            values.append(rng.choice([0x1000a, 0x20003, 0, NONE, 0x7777]))
            continue
        if argument_type == 'point_ptr':
            struct.pack_into('<3f', world.data3, CENTER - DATA3, *(rng.choice([rng.uniform(-8, 8), 0.0]) for _ in range(3)))
            values.append(CENTER)
            continue
        if argument_type == 'location_out':
            values.append(LOCATION)
            continue
        if argument_type == 'sphere_flags':
            values.append(rng.choice([0, 1, 2, 3, 4]))
            continue
        if argument_type == 'type_mask':
            values.append(rng.choice([0, NONE, 1, 3, 0x40, 0x380, 0x45]))
            continue
        if argument_type == 'location_ptr':
            world.put(LOCATION, rng.choice([0, 1, 2, NONE]))
            struct.pack_into('<hh', world.data3, LOCATION + 4 - DATA3, rng.choice([0, 1, 2, 3, -1]), 0)
            values.append(LOCATION)
            continue
        if argument_type == 'center_ptr':
            struct.pack_into('<3f', world.data3, CENTER - DATA3, *(rng.uniform(-8, 8) for _ in range(3)))
            values.append(CENTER)
            continue
        if argument_type == 'sphere_radius':
            values.append(struct.unpack('<I', struct.pack('<f', rng.choice([0.0, -1.0, 1.0, 4.0, 12.0, 30.0, 1e-40])))[0])
            continue
        if argument_type == 'sphere_out':
            values.append(SPHERE_OUT)
            continue
        if argument_type == 'sphere_max':
            values.append(rng.choice([128, 3, 1, 0]))
            continue
        if name == 'ai_magically_see' and argument_type == 19 and rng.random() < .85:
            word = lambda bank, address, base: struct.unpack_from('<I', bank, address - base)[0]
            if not values:
                holders = [word(world.ai, ACTOR_ROWS + 0x888 * n + 0x30, AI) for n in range(ACTORS)
                           if word(world.ai, ACTOR_ROWS + 0x888 * n + 0x7c, AI) != NONE]
                holders = [h for h in holders if h < SQUADS]
                values.append(rng.choice(holders) if holders else rng.randrange(SQUADS))
            else:
                known = [word(world.ai, KNOW_ROWS + 0xc4 * k + 0x1c, AI) for k in range(KNOWLEDGE)]
                known = [k for k in known if k != NONE]
                pick = rng.choice(known) if known else rng.randrange(ACTORS)
                squad = word(world.ai, ACTOR_ROWS + 0x888 * (pick & 0xffff) + 0x30, AI)
                if rng.random() < .5 or squad >= SQUADS:
                    values.append((2 << 30) | (pick & 0xffff))
                else:
                    values.append(squad)
            continue
        if name == 'ai_magically_see_object' and argument_type == 19 and rng.random() < .85:
            # Mostly the squad of a member holding a prop.
            holders = [struct.unpack_from('<I', world.ai, ACTOR_ROWS + 0x888 * n + 0x30 - AI)[0] for n in range(ACTORS)
                       if struct.unpack_from('<I', world.ai, ACTOR_ROWS + 0x888 * n + 0x7c - AI)[0] != NONE]
            holders = [h for h in holders if h < SQUADS]
            values.append(rng.choice(holders) if holders and rng.random() < .8 else rng.randrange(SQUADS))
            continue
        if name == 'ai_magically_see_object' and argument_type == 50:
            values.append(rng.choice([world.handles[0], world.handles[0], world.handles[1], world.handles[5], NONE]))
            continue
        if seated and argument_type == 52:
            values.append(world.handles[seated[0]])
            continue
        if seated and argument_type == 51:
            values.append(world.handles[seated[1]] if rng.random() < .85 else rng.choice(living))
            continue
        if seated and argument_type == 11:
            values.append(seated[3] if rng.random() < .7 else rng.choice(SEAT_NAMES + [NONE]))
            continue
        if seated and argument_type == 31:
            index = rng.randrange(len(world.lists))
            values.append(world.lists[index])
            members = list(world.members[index])
            first = struct.unpack_from('<I', world.data, LIST_HEADER_ROWS + 12 * index + 8 - DATA)[0]
            if members and first != NONE and rng.random() < .8:
                world.put(LIST_REF_ROWS + 12 * (first & 0xffff) + 4, world.handles[seated[1]])
                members[0] = world.handles[seated[1]]
            continue
        if argument_type == 11 and name in ('ai_vehicle_reserve_seat', 'vehicle_test_seat', 'vehicle_test_seat_list'):
            values.append(rng.choice(SEAT_NAMES + ([NONE] if name != 'ai_vehicle_reserve_seat' else []) + [0x9990099]))
            continue
        if argument_type == 19:
            tag = rng.choice([0, 0, 1, 1, 2, 2, 3, 3, 3])
            if tag == 0:
                values.append(rng.randrange(SQUADS))
            elif tag == 1:
                values.append((1 << 30) | rng.randrange(AI_GROUPS))
            elif tag == 2:
                values.append((2 << 30) | rng.randrange(ACTORS) | (rng.getrandbits(14) << 16 if rng.random() < .3 else 0))
            else:
                values.append((3 << 30) | (rng.choice([0, 1, 2, 3, 4, 5, 6, 0x3fff]) << 16)
                              | rng.choice([0, 1, 2, 3, 0xffff, 0x8000]))
            if rng.random() < .05:
                values[-1] = NONE
            continue
        if argument_type == 27:
            values.append(rng.choice([0, 1, 2, 5, 0xffff]) | (rng.getrandbits(16) << 16 if rng.random() < .3 else 0))
            continue
        if argument_type == 45:
            values.append(rng.choice([0, 1, 2, 3, 0xffff, 0xff80]) | (rng.getrandbits(16) << 16 if rng.random() < .3 else 0))
            continue
        if argument_type == 50 and name == 'object_model_targets_destroyed':
            # DAE40 dereferences the body of an invalid root (the original faults).
            values.append(rng.choice([h for n, h in enumerate(world.handles[:ROWS - 1]) if world.salts[n]] + [NONE]))
            continue
        if argument_type == 11 and name in ('object_model_targets_destroyed', 'object_at_marker'):
            values.append(rng.choice(MARKER_NAMES + [0x7000001]))
            continue
        if argument_type == 11 and name in ('flock_start', 'flock_stop'):
            values.append(rng.choice(FLOCK_NAMES + [0x9999999]))
            continue
        if argument_type == 8 and name == 'weapon_hold_trigger':
            values.append(rng.choice([NONE, 0, 1, 2, 3, 0x80000000]))
            continue
        if argument_type == 8 and name == 'ice_cream_flavor_available':
            values.append(rng.choice([NONE, 0, 1, 7, 14, 15, 16, 0x80000000]))
            continue
        if argument_type == 'effect_name':
            effect_fixture(world, rng)
            values.append(rng.choice(EFFECT_NAMES + [0x300012a, 0, NONE, 0x7777777]))
            continue
        if isinstance(argument_type, str) and argument_type.startswith('leaf_'):
            leaf_fixture(world, rng)
            live = world.handles[:ROWS - 1]
            if argument_type == 'leaf_pair':
                values += [FACTORY + 0x100, FACTORY + 0x104, rng.choice(live)]
            elif argument_type == 'leaf_object':
                values.append(rng.choice(live + [NONE]))
            elif argument_type == 'leaf_live':
                values.append(rng.choice(live))
            elif argument_type == 'leaf_definition':
                values.append((rng.getrandbits(16) << 16) | rng.randrange(4))
            elif argument_type == 'leaf_type':
                variant_fixture(world, rng)
                values.append((rng.getrandbits(16) << 16) | rng.choice([0, 1, 2, 3, 5, 15, 0xffff]))
            elif argument_type == 'leaf_variant':
                variant_fixture(world, rng)
                values += [rng.choice([(0x5000 + 4 + k) << 16 | (4 + k) for k in range(4)] + [NONE]),
                           rng.choice(VARIANT_NAMES + [0, 0x4444, 0x5555])]
            elif argument_type == 'leaf_set_variant':
                variant_fixture(world, rng)
                values += [rng.choice(live), rng.choice(VARIANT_NAMES + [0, 0, 0x5555])]
            elif argument_type == 'leaf_matrices':
                body = world.body(rng.randrange(ROWS - 1))
                struct.pack_into('<hh', world.data, body + 0x114 - DATA, rng.choice([0, 0x34, 0x68, 0x33, 0x34 * 50, -0x34]),
                                 rng.choice([0, 0x200, -0x10]))
                values += [FACTORY + 0x110, world.handles[(body - BODIES) // BODY_SIZE]]
            elif argument_type == 'leaf_lens':
                lens_fixture(world, rng)
                values.append(rng.choice(live + [NONE]))
            else:
                values.append(FACTORY + 0x800 + rng.choice([0, 4, 0x10, 0x101]))
            continue
        if argument_type in ('factory_alloc', 'factory_resize', 'factory_object', 'factory_block', 'factory_free'):
            factory_fixture(world, rng)
            tag = lambda: rng.choice([0, 0, rng.getrandbits(32)])
            if argument_type == 'factory_alloc':
                values += [POOL_SLOTS + 0x40 + 4 * rng.randrange(8),
                           rng.choice([0, 1, 4, 0xf, 0x10, 0x11, 0x40, 0x100, 0x3f0, 0x1000, 0x3000]), tag(), MEMORY_POOL, tag()]
            elif argument_type == 'factory_resize':
                if not world.blocks:
                    return arguments(name, world, rng)
                n = len(world.blocks) - 1 if rng.random() < .4 else rng.randrange(len(world.blocks))
                payload = world.blocks[n][2] - (0x20 if world.factory[MEMORY_POOL + 0x3f - FACTORY] else 0x10)
                size = rng.choice([0, 0x10, max(payload - 1, 0), payload, payload + 1, payload + 0x40, 0x200, 0x1000])
                values += [MEMORY_POOL, tag(), tag(), size, POOL_SLOTS + 4 * n, SP + 8]
            elif argument_type == 'factory_free':
                if not world.heap_objects or rng.random() < .1:
                    values.append(world.handles[ROWS - 1])
                else:
                    values.append(world.handles[rng.choice(world.heap_objects)])
            elif argument_type == 'factory_object':
                values.append(rng.choice([0, 1, 3, 0x40, 0x154, 0x400, 0x1000, 0x3000]))
            else:
                if not world.heap_objects:
                    return arguments(name, world, rng)
                row_index = rng.choice(world.heap_objects)
                handle = (rng.getrandbits(16) << 16) | row_index
                values += [rng.choice([0, 4, 0x10, 0x24]), rng.choice([0, 1, 2, 3, 0xc, 0x40, 0x100, 0x400]),
                           rng.choice([0, 1, 2, 3, 4, 5]), handle, SP - 0x20]
            continue
        if argument_type == 'profile':
            profile_fixture(world, rng)
            values.append(PROFILE)
            continue
        if argument_type == 14:
            values.append(rng.choice([0, 1, 2, 3, 5, 0xffff, 0x8000]))
            continue
        if argument_type in ('trigger', 13):
            values.append(rng.randrange(0, 8) | (rng.getrandbits(16) << 16 if rng.random() < .3 else 0))
            continue
        if argument_type == 'marker_name':
            values.append(rng.choice(MARKER_NAMES + [0, NONE, 0x7000001]))
            continue
        if argument_type == 'marker_count':
            values.append(rng.choice([1, 1, 2, 4, 0, 0xffff]) | (rng.getrandbits(16) << 16 if rng.random() < .3 else 0))
            continue
        if argument_type == 'flag_byte':
            values.append(rng.choice([0, 1]) | (rng.getrandbits(24) << 8))
            continue
        if argument_type == 'marker_out':
            values.append(MARKER_OUT)
            continue
        if argument_type == 'copy_target':
            values.append(COPY_TARGET + rng.choice([0, 1, 2, 3, 4, 7]))
            continue
        if argument_type == 'copy_source':
            values.append(COPY_SOURCE + rng.choice([0, 1, 2, 3, 5]))
            continue
        if argument_type == 'copy_count':
            values.append(rng.choice([0, 1, 2, 3, 4, 5, 7, 8, 13, 31, 32, 33, 40]))
            continue
        if argument_type == 'device':
            values.append(rng.choice([world.handles[n] for n in (6, 7, 8, 10)] + [world.handles[0], NONE]))
            continue
        if argument_type == 'unit_real':
            values.append(struct.unpack('<I', struct.pack('<f', rng.choice([0.0, 0.25, 0.5, 1.0, 1.5, -0.5, 0.999999, 1e-40])))[0]
                          if rng.random() < .7 else floats(rng))
            continue
        if argument_type == 'live_only':
            values.append(rng.choice(living))
            continue
        if argument_type == 'fv_object':
            values.append(rng.choice(world.handles[:3] + [NONE]))
            continue
        if argument_type == 'fv_name':
            values.append(rng.choice([0x1000a, 0x20003, 0, NONE, 0x7777]))
            continue
        if argument_type == 'out_value':
            values.append(OUT_VALUE)
            continue
        if argument_type == 'out_slot':
            values.append(OUT_SLOT)
            continue
        if argument_type == 'emotion':
            values.append(rng.choice(EMOTIONS + [0x7000001, 0x7000001, 0x1234567]))
            continue
        if argument_type == 'shadow':
            values.append(rng.choice(world.shadow_objects + [NONE, 0x55550002]))
            continue
        if argument_type == 6:
            values.append(floats(rng))
        elif argument_type == 7 and name == 'device_group_from_ordinal':
            values.append(rng.choice([0, 1, 2, 3, 5, 6, 7, 0x7fff, 0xffff, 0x8000, 0xfffe]) | (rng.getrandbits(16) << 16))
        elif argument_type == 7:
            values.append(rng.choice([0, 1, 10, 0xffff, 0x7fff, 0x8000, 100, 360, 60]) | (rng.getrandbits(16) << 16 if rng.random() < .3 else 0))
        elif argument_type == 5:
            values.append(rng.choice([0, 1, 2, 0x80, 0xff, 0x100, 0xffffff00]))
        elif argument_type == 11:
            values.append(rng.choice(world.interpolators + [0, 0x1d000a, 0x31337]))
        elif argument_type == 18:
            values.append(rng.choice(world.groups + [NONE]))
        elif argument_type == 31:
            index = rng.randrange(len(world.lists))
            values.append(world.lists[index])
            members = world.members[index]
        elif unchecked:
            values.append(rng.choice(living + [NONE]))
        else:
            values.append(rng.choice(any_handle))
    if name in ('volume_return_objects', 'volume_return_objects_by_type'):
        # Whole evaluators allocate into fresh HS list pools.
        world.globals[0x5020b0], world.globals[0x5020ac] = FRESH_HEADERS, FRESH_REFS
    if name == 'interpolator_start':
        # Absent state bank, scenario or game-time owner branches.
        for address, probability in ((0x4f3010, .1), (0x4ecc20, .05), (0x51df54, .1)):
            if rng.random() < probability:
                world.globals[address] = 0
    if name == 'object_clear_function_variable':
        for slot in rng.sample(range(32), rng.randrange(0, 6)):
            world.put(SLOTS + 0x280 + 4 * slot, values[0] if rng.random() < .8 else world.handles[1])
            world.put(SLOTS + 0x14 * slot, values[1] if rng.random() < .8 else 0x4242)
    if name == 'object_clear_all_function_variables' and values[0] != NONE:
        for slot in range(32):
            world.put(SLOTS + 0x280 + 4 * slot, rng.choice([values[0], NONE, world.handles[0], 0]))
        n = values[0] & 0xffff
        world.data[world.body(n) + 0xc1 - DATA] |= rng.choice([0, 2])
    return values, members


def routine_row(row, info, world, values):
    """Installs a standalone routine's register, XMM and stack inputs."""
    # Long list/marker walks (view tests, seat trees, cluster floods) exceed
    # the observer's 5000-step default; 100000 is its declared maximum.
    row['max_steps'] = 100000
    spec = info.get('routine')
    if spec:
        for register, index in spec.get('regs', {}).items():
            row['gp'][REGISTERS[register]] = values[index]
        if spec.get('xmm'):
            lanes = [[0x6c000000 + row['id'] * 13 + 4 * r + k for k in range(4)] for r in range(8)]
            for register, index in spec['xmm'].items():
                lanes[register][0] = values[index]
            row['xmm'] = lanes
        stack = [values[i] for i in spec.get('stack', [])]
        row['matrix'] = stack[0] if len(stack) > 0 else 0
        row['source'] = stack[1] if len(stack) > 1 else 0
        world.stack_words = stack[2:]
    row['banks'] = world.banks(values)
    # HALO2_ORACLE_STRICT_WRITES=1: an original store outside every declared
    # bank fails the case instead of going uncompared.
    if os.environ.get('HALO2_ORACLE_STRICT_WRITES'):
        row['strict_writes'] = True
    return row


# While main() streams, the batch the validator's cases() returns, so the
# tracks' unchanged cases wrappers (each calls the cases() it wrapped, then
# adjusts rows in place) act on one batch at a time. None: generate them all.
_STREAMED_BATCH = None


def cases(seed, evaluators, tails):
    if _STREAMED_BATCH is not None:
        return _STREAMED_BATCH
    return list(generate_cases(seed, evaluators, tails))


def generate_cases(seed, evaluators, tails):
    """Yields every case in generation order; a case's id is its position."""
    rng = random.Random(seed)
    count = 0
    for name in sorted(KERNELS):
        for repeat in range({'original_strncpy': 400, 'object_get_markers': 400, 'camera_control': 200, 'volume_test_object': 400, 'volume_test_objects': 200, 'volume_test_objects_all': 200, 'scenario_location': 200, 'objects_in_sphere': 400, 'volume_return_objects': 400, 'volume_return_objects_by_type': 400, 'objects_can_see_flag': 400, 'objects_can_see_object': 400, 'ai_vehicle_get_from_starting_location': 300, 'ai_vehicle_get': 300, 'object_get_ai': 200, 'ai_spawn_count': 300, 'ai_combat_status': 300, 'ai_disposable': 300, 'ai_set_blind': 300, 'ai_set_deaf': 300, 'ai_suppress_combat': 300, 'ai_disregard': 200, 'ai_prefer_target': 200, 'ai_magically_see_object': 400, 'ai_vitality_pinned': 400, 'ai_renew': 400, 'ai_vehicle_reserve': 300, 'ai_vehicle_reserve_seat': 300, 'vehicle_test_seat': 400, 'vehicle_test_seat_list': 400, 'object_set_velocity': 400, 'player_profile_normalize': 600, 'weapon_hold_trigger': 300, 'activate_team_nav_point_object': 400, 'deactivate_team_nav_point_object': 400, 'activate_team_nav_point_flag': 300, 'deactivate_team_nav_point_flag': 300, 'unit_set_active_camo': 400, 'biped_ragdoll': 200, 'units_set_maximum_vitality': 300, 'player_flashlight_on': 300, 'ice_cream_flavor_available': 200, 'ai_set_active_camo': 400, 'ai_berserk': 400, 'flock_start': 300, 'flock_stop': 300, 'ai_magically_see': 600, 'object_model_targets_destroyed': 400, 'object_at_marker': 400, 'sound_effect_reference': 400, 'object_memory_allocate': 500, 'object_memory_resize': 500, 'object_allocate': 500, 'object_block_allocate': 500, 'object_model_tag_pair': 300, 'object_damage_info': 200, 'object_root_header_bit6': 200, 'object_clear_block_12a': 200, 'object_queue_definition_flagged': 300, 'object_definition_physics_capacity': 300, 'object_memory_locked': 100, 'havok_record_initialize': 60, 'havok_record_reset': 60, 'havok_record_clear_links': 60, 'object_free': 400, 'definition_object_type': 200, 'model_variant_index': 300, 'object_set_variant': 300, 'object_node_matrices': 200, 'object_retire_lens_queries': 300}.get(name, TRACK_COUNTS.get(name, 48))):
            world = World(rng)
            values, members = arguments(name, world, rng)
            x87, mxcsr = rng.choice(FP_CONTROLS)
            info = tails[name]
            gp = [0x5a010000 + count * 31 + k * 7 for k in range(8)]
            gp[4] = SP
            for register in info.get('zeroed', []):
                gp[REGISTERS[register]] = 0
            for register in info['argument_registers']:
                gp[REGISTERS[register]] = ARG
            yield routine_row(dict(id=count, name=f'{name}-{repeat}', kernel=name, args=values, members=members,
                              entry=info['entry'], stop_pc=STOP, stop_before_entries=info['stops'], return_pc=STOP,
                              gp=gp, matrix=0, source=0, before_fp=[x87, 0, mxcsr], fp=[x87, mxcsr], live_x87=[]),
                              info, world, values)
            count += 1
    # Deterministic grid: every live FP control against denormal, tie,
    # boundary and non-finite first operands of each floating-point kernel.
    edges = [1e-40, -1e-42, 1.1754942e-38, 0.05, 0.0333, 2.5, 1.5, -2.5, 5.0, 5.0000005, 3.4e38,
             float('nan'), float('inf'), -0.0]
    for name in ('real_random_range', 'physics_set_gravity', 'physics_disable_character_ground_adhesion_forces',
                 'player_effect_set_max_rotation', 'player_effect_start', 'player_effect_stop', 'pin',
                 'interpolator_start', 'cinematic_depth_of_field', 'camera_set_field_of_view',
                 'object_set_function_variable', 'unit_set_emotion', 'device_animate_position',
                 'device_animate_overlay', 'texture_camera_set_object_marker', 'object_set_shield',
                 'objects_can_see_flag', 'objects_can_see_object',
                 'device_set_position_immediate', 'device_group_set_immediate'):
        if name not in KERNELS:
            continue
        info = tails[name]
        for x87, mxcsr in FP_CONTROLS:
            for edge in edges:
                world = World(rng)
                values, members = arguments(name, world, rng)
                position = {'player_effect_start': 1, 'interpolator_start': 2, 'object_set_function_variable': 3,
                            'unit_set_emotion': 3, 'device_animate_position': 2, 'device_animate_overlay': 3,
                            'texture_camera_set_object_marker': 2, 'object_set_shield': 1,
                            'objects_can_see_flag': 2, 'objects_can_see_object': 2,
                            'device_set_position_immediate': 1, 'device_group_set_immediate': 1}.get(name, 0)
                if name == 'interpolator_start' and world.interpolators and rng.random() < .8:
                    values[0] = rng.choice([n for n in world.interpolators if n] or [0])
                values[position] = struct.unpack('<I', struct.pack('<f', edge))[0]
                struct.pack_into('<h', world.data2, TIME + 2 - DATA2, rng.choice([1, 2, 30, 60]))
                gp = [0x5b010000 + count * 29 + k * 5 for k in range(8)]
                gp[4] = SP
                for register in info.get('zeroed', []):
                    gp[REGISTERS[register]] = 0
                for register in info['argument_registers']:
                    gp[REGISTERS[register]] = ARG
                yield routine_row(dict(id=count, name=f'{name}-edge-{x87:x}-{mxcsr:x}-{edge}', kernel=name, args=values,
                                  members=members, entry=info['entry'], stop_pc=STOP, stop_before_entries=info['stops'],
                                  return_pc=STOP, gp=gp, matrix=0, source=0, before_fp=[x87, 0, mxcsr], fp=[x87, mxcsr],
                                  live_x87=[]), info, world, values)
                count += 1


def failing_case(owned, rows):
    """Bisect a failing observer chunk down to the first case it rejects."""
    def fails(subset):
        inputs = owned / 'bisect.json'
        inputs.write_text(json.dumps(subset, separators=(',', ':')))
        return subprocess.run([str(owned / 'observer'), str(owned / 'reference'), str(owned / 'metadata.json'),
                               str(inputs), str(owned / 'bisect-out.json')], capture_output=True,
                              timeout=900).returncode != 0
    while len(rows) > 1:
        half = rows[:len(rows) // 2]
        rows = half if fails(half) else rows[len(rows) // 2:]
    return rows[0]['name'] if rows else '?'


def physical(owned, rows, chunk=2000):
    # Bounded observer runs; each chunk restarts the unchanged reference.
    proof = None
    for start in range(0, len(rows), chunk):
        inputs = owned / 'inputs.json'
        inputs.write_text(json.dumps(rows[start:start + chunk], separators=(',', ':')))
        output = owned / 'observed.json'
        result = subprocess.run([str(owned / 'observer'), str(owned / 'reference'), str(owned / 'metadata.json'),
                                 str(inputs), str(output)], text=True, capture_output=True, timeout=900)
        if result.returncode:
            raise RuntimeError('Original execution failed at ' + failing_case(owned, rows[start:start + chunk]) + ': '
                               + (result.stdout + result.stderr)[-4000:])
        part = json.loads(output.read_text())
        if part['instruction_patches'] or part['supplied_original_outcomes']:
            raise ValueError('Reference supplied instructions or outcomes')
        if proof is None:
            proof = part
        else:
            proof['cases'].extend(part['cases'])
    return proof


def expected(row, original):
    if not original['completed']:
        raise ValueError('Original did not reach hs_return: ' + row['name'])
    if row['stop_before_entries'] and original.get('stopped_at') not in row['stop_before_entries']:
        raise ValueError('Original stopped outside hs_return: ' + row['name'])
    writes = [[w['address'], w['written']] for w in original['writes']
              if not STACK <= w['address'] < STACK + 0x10000 and not ARG <= w['address'] < ARG + 32]
    banks = [b for b in original['banks'] if b['address'] != STACK]
    kind = KERNELS[row['kernel']][1]
    if kind == 'list':
        # Walk the original HS list from the final banks; 1E7060 prepends,
        # so reverse to append order. List-pool stores are compared there.
        data2 = bytes.fromhex(next(b['after'] for b in original['banks'] if b['address'] == DATA2))
        word = lambda a: struct.unpack_from('<I', data2, a - DATA2)[0]
        handle = original['boundary_gp'][0]
        members = []
        if handle != NONE:
            ref = word(word(FRESH_HEADERS + 0x44) + (handle & 0xffff) * 12 + 8)
            while ref != NONE and len(members) < 200:
                at = word(FRESH_REFS + 0x44) + (ref & 0xffff) * 12
                members.append(word(at + 4))
                ref = word(at + 8)
        writes = [w for w in writes if not FRESH_HEADERS <= w[0] < FRESH_END]
        return dict(completed=True, writes=writes, list=list(reversed(members)))
    result = original['result_bits'] if kind == 'xmm0' else original['boundary_gp'][0]
    result = {'void': 0, 'boolean': result & 0xff, 'short': result & 0xffff}.get(kind, result)
    answer = dict(completed=True, result=result, writes=writes, banks_sha256=digest(banks))
    if row.get('observe_stop_entry'):
        answer['stopped_at'] = original['stopped_at']
    return answer


def compare(rows, golds, native, total=None):
    # A caller's running `total` chains the digest across streamed batches.
    total = hashlib.sha256() if total is None else total
    count = 0
    errors = Path(os.environ['HALO2_TEST_SCRATCH']) / 'native.stderr'
    with errors.open('w+') as stderr:
        process = subprocess.Popen([str(native)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=stderr, text=True)
        try:
            for row, gold in zip(rows, golds, strict=True):
                for publisher in (False, True):
                    native_row = dict(kernel=row['kernel'], args=row['args'], members=row['members'], fp=row['fp'],
                                      banks=[b for b in row['banks'] if b['address'] != STACK], publisher=publisher)
                    if row.get('observe_stop_entry'):
                        native_row['observe_stop_entry'] = True
                    process.stdin.write(json.dumps(native_row, separators=(',', ':')) + '\n')
                    process.stdin.flush()
                    line = process.stdout.readline()
                    if not line:
                        stderr.seek(0)
                        raise ValueError(row['name'] + ': native stopped: ' + stderr.read()[-3000:])
                    actual = json.loads(line)
                    native_banks = actual['banks']
                    actual['banks_sha256'] = digest(actual.pop('banks'))
                    actual.pop('fault', None)
                    if KERNELS[row['kernel']][1] == 'list':
                        actual.pop('banks_sha256')
                        actual.pop('result')
                    if KERNELS[row['kernel']][1] == 'void':
                        actual['result'] = 0
                    if actual != gold and actual.get('writes') == gold.get('writes') and row.get('_original_banks'):
                        for mine, theirs in zip(native_banks, row['_original_banks']):
                            if mine['after'] != theirs['after']:
                                a, b = bytes.fromhex(mine['after']), bytes.fromhex(theirs['after'])
                                k = next(i for i in range(len(a)) if a[i] != b[i])
                                print('first bank difference at', hex(mine['address'] + k), a[k:k+8].hex(), b[k:k+8].hex())
                                break
                    if actual != gold:
                        raise ValueError(row['name'] + ': native differs\nnative=' + json.dumps(actual) + '\noriginal=' + json.dumps(gold)
                                         + '\nargs=' + json.dumps(row['args']) + ' fp=' + json.dumps(row['fp']))
                    total.update(json.dumps(actual, sort_keys=True).encode())
                    count += 1
            process.stdin.close()
            if process.wait(timeout=30):
                stderr.seek(0)
                raise ValueError(stderr.read()[-3000:])
        finally:
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=5)
    return dict(comparisons=count, comparison_sha256=total.hexdigest())


# Cases per streamed batch: physical()'s observer chunk, so every observer run
# sees the same cases as a single whole-list physical() call would.
BATCH_CASES = 2000
# Batches observed at once, each by its own observer process and directory.
OBSERVERS = 2


def main():
    load_tracks()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--seed', type=int, default=0x5c41)
    parser.add_argument('--only', action='append', default=[], help='Diagnostic subset of kernel names')
    parser.add_argument('--shard', default='0/1', metavar='K/N',
                        help='Observe and compare only batches K, K+N, ... of %d cases; every case is still '
                             'generated in order, so N shards together check the unsharded case set' % BATCH_CASES)
    args = parser.parse_args()
    try:
        shard, shards = (int(part) for part in args.shard.split('/'))
    except ValueError:
        parser.error('--shard expects K/N')
    if not 0 <= shard < shards:
        parser.error('--shard expects 0 <= K < N')
    for name in list(KERNELS):
        if args.only and name not in args.only:
            del KERNELS[name]
    if not NEW_DEFAULT.is_file():
        print('SKIP missing owned TU1.5 executable')
        return 77
    if sha(NEW_DEFAULT) != NEW_PIN:
        raise ValueError('Wrong original executable')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    image = Xbe(NEW_DEFAULT)
    found = registrations(image)
    meta, code = metadata(image, {name: row['evaluator'] for name, row in found.items()})
    tails = {name: (dict(entry=row['evaluator'], argument_registers=[], stops=[], routine=ROUTINES[name])
                    if row.get('routine') else tail(code, row['evaluator'])) for name, row in found.items()}
    owned = prepare(image, meta, scratch)
    # Memory-bounded streaming. The main thread generates cases in cases()'
    # order and passes each full batch through the tracks' cases wrappers;
    # OBSERVERS threads observe batches and one thread compares them in batch
    # order, so results and the chained digest match one whole-list pass. At
    # most OBSERVERS + 2 batches are held; each is released once compared.
    kernels = {name: 0 for name in KERNELS}
    executed, chained = set(), hashlib.sha256()
    totals = dict(generated=0, batches=0, cases=0, comparisons=0, writes=0, instructions=0, query_joins=0)
    started, stop = time.monotonic(), threading.Event()
    lanes = queue.SimpleQueue()
    for n in range(OBSERVERS):
        lane = owned / f'lane{n}'
        lane.mkdir()
        for name in ('observer', 'reference', 'metadata.json'):
            (lane / name).symlink_to(owned / name)
        lanes.put(lane)

    def observe(rows):
        if stop.is_set():
            return None
        lane = lanes.get()
        try:
            proof = physical(lane, rows)
        finally:
            lanes.put(lane)
        golds = [expected(row, original) for row, original in zip(rows, proof['cases'], strict=True)]
        sites = set()
        # Cases with "query_names" report the translated route's MCC key
        # joins the observer applied (see the observer).
        joins = 0
        for row, original in zip(rows, proof['cases']):
            row['_original_banks'] = [b for b in original['banks'] if b['address'] != STACK]
            sites.update(original['executed_sites'])
            joins += len(original.get('query_joins', ()))
        return golds, sites, proof['original_instructions'], joins

    def verify(index, rows, observed):
        outcome = observed.result()
        if stop.is_set():
            return
        golds, sites, instructions, joins = outcome
        totals['query_joins'] += joins
        result = compare(rows, golds, args.native, chained)
        executed.update(sites)
        totals['cases'] += len(rows)
        totals['comparisons'] += result['comparisons']
        totals['writes'] += sum(len(g['writes']) for g in golds)
        totals['instructions'] += instructions
        for row in rows:
            kernels[row['kernel']] += 1
        golds.clear()
        if len(rows) == BATCH_CASES:
            print(f"batch {index + 1}: {totals['cases']} cases, {totals['comparisons']} native comparisons, "
                  f"{time.monotonic() - started:.0f} s", flush=True)
        rows.clear()

    def streamed(batch):
        global _STREAMED_BATCH
        expected_ids = [row['id'] for row in batch]
        _STREAMED_BATCH = batch
        try:
            rows = cases(args.seed, found, tails)
        finally:
            _STREAMED_BATCH = None
        if [row['id'] for row in rows] != expected_ids:
            raise RuntimeError('A track cases wrapper added, dropped or reordered cases')
        return rows

    with ThreadPoolExecutor(OBSERVERS) as observing, ThreadPoolExecutor(1) as comparing:
        pending = deque()

        def submit(batch):
            index = totals['batches']
            totals['batches'] += 1
            totals['generated'] += len(batch)
            if index % shards != shard:
                return
            rows = streamed(batch)
            pending.append(comparing.submit(verify, index, rows, observing.submit(observe, rows)))
            while len(pending) > OBSERVERS + 1:
                pending.popleft().result()

        try:
            batch = []
            for row in generate_cases(args.seed, found, tails):
                batch.append(row)
                if len(batch) == BATCH_CASES:
                    submit(batch)
                    batch = []
            if batch:
                submit(batch)
            del batch
            while pending:
                pending.popleft().result()
        except BaseException:
            stop.set()
            observing.shutdown(cancel_futures=True)
            comparing.shutdown(cancel_futures=True)
            raise
    if not totals['cases']:
        raise ValueError(f"No case selected ({totals['generated']} generated, shard {args.shard})")
    names = ['include/halo2/native_campaign_script_builtins_tu15.hpp',
             'src/campaign/scripting/native_campaign_script_builtins_tu15.cpp',
             'tests/native_campaign_script_builtins_tu15_tests.cpp',
             'tools/validate_native_campaign_script_builtins_tu15.py',
             'tools/observe_native_campaign_ai_vector_math_tu15.cpp']
    report = dict(passed=True, scope=__doc__.strip(), physical_cases=totals['cases'],
                  comparisons=totals['comparisons'], comparison_sha256=chained.hexdigest(), kernels=kernels,
                  writes=totals['writes'], executed_original_sites=len(executed),
                  original_instructions=totals['instructions'], registrations=found,
                  tails={name: dict(entry=t['entry'], stops=t['stops']) for name, t in tails.items()},
                  owners=meta['owners'], source_sha256={n: sha(ROOT / n) for n in names},
                  original_xbe_sha256=NEW_PIN, instruction_patches=[], supplied_original_outcomes=False,
                  translated_route_query_joins=totals['query_joins'],
                  retained_large_artifacts=[], shard=args.shard, generated_cases=totals['generated'],
                  batches=totals['batches'], batch_cases=BATCH_CASES,
                  peak_rss_kib=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
                  wall_seconds=round(time.monotonic() - started, 1))
    if os.environ.get('HALO2_ORACLE_EXECUTED_SITES'):
        # Per-instruction coverage: every executed original address.
        report['executed_sites'] = sorted(executed)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    if shards > 1:
        print(f"SHARD {args.shard}: batches {shard}, {shard + shards}, ... of {totals['batches']} "
              f"({totals['generated']} generated cases)")
    print(f"PASS {totals['cases']} physical original evaluator tails, {totals['comparisons']} native comparisons, "
          f"{totals['writes']} ordered original stores")
    if totals['query_joins']:
        print(f"  {totals['query_joins']} translated-route MCC key joins applied (query_names cases)")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
