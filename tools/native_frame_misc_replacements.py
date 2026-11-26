"""Bind the native TU1.5 frame/gameplay owners of group F
(docs/native_replacement/FRAME_MISC_OWNERS.md).

Each owner is installed only into the build-local specialized copy of its
own unit, and only when the complete canonical body matches its pin (state
type normalized), every direct child body the closure enters is present and
every dispatched child keeps its dispatch route. The verification reference
is the unmodified canonical body with only its child calls routed to the
replay (native_object_tick_detail::replay). A changed body keeps its
translated route.
"""
import hashlib
import re

DETAIL = 'halo2::native_object_tick_detail'
FIRST_BODY = re.compile(r'^static void run_[0-9a-f]+\(WorldState &s\) \{$', re.M)
DIRECT = re.compile(r'\brun_([0-9a-f]+)\(s\)')
DISPATCH = re.compile(r'\bdispatch\(s,(target|0x[0-9a-f]+u|selected\(0x[0-9a-f]+u,s\.image\)),0x[0-9a-f]+u,(?:false|true)\)')
REQUIRED = re.compile(r'\bs\.required\(0x[0-9a-f]+u,(0x[0-9a-f]+u),false\)')
# name -> owner record. 'unit' is a line only the owning unit carries;
# 'direct' children must have bodies in the unit, 'dispatched' must not.
OWNERS = {
    '2aa6d0': dict(
        pin='0af2f3072ecef8f34ee94573098ad383fc55613f9d1106573696d57532c91fb5',
        unit='#include "halo2/source_campaign_runtime_tu15.hpp"',
        include='#include "campaign/scripting/native_tu15_script_offset_rebase.hpp"\n',
        bridge='halo2::native_script_offset_rebase_detail::bridge(s,[](auto &v){{{reference}(v);}},'
               '[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=('16ea60',)),
    '1306b0': dict(
        pin='09a0ee065558df4dbc6c7faea11296532b0b495142bc2544fff9e0cd884323ce',
        unit='#include "halo2/source_geometry_cache_frame_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_geometry_cache_count.hpp"\n',
        bridge='halo2::native_geometry_cache_count_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'd7230': dict(
        pin='54490f7355105549c798f207a9c169806c9c020f585b65155bfaefa3f0f635b0',
        unit='#include "halo2/source_model_render_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_attachment_update.hpp"\n',
        bridge=('halo2::native_attachment_update_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    '1eee80': dict(
        pin='7d42f55d4aee93243d593357dbd7c9634f6d5db016c73ce668f37020f655cc52',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_engine_player_update.hpp"\n',
        bridge=('halo2::native_engine_player_update_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](std::uint32_t p,SourcePhysicsContactImage image){{return canonical_branch(p,image);}},\n'
                '        [](auto &v){{run_1efd60(v);}},[](auto &v){{run_1f0670(v);}},[](auto &v){{run_1f1030(v);}})'),
        direct=('1efd60', '1f0670', '1f1030'), dispatched=()),
    '41970': dict(
        pin='f1195d21e27893a26a86fe8b2312296de78385feffe5d2ca467ccef727bf6ef8',
        unit='#include "halo2/source_model_render_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_attachment_schedule.hpp"\n',
        bridge=('halo2::native_attachment_schedule_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    '133a70': dict(
        pin='666f7adec7a376e969303b6258e8f063d91ea9a979bb58d32161d82f0c375b88',
        unit='#include "halo2/source_campaign_environment_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_environment_palette.hpp"\n',
        bridge='halo2::native_environment_palette_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '153ba0': dict(
        pin='42aefdb2ede564565e3d5e7fd3b54a227a98530f6d93f00efea62f6ef2c41a0c',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_player_action.hpp"\n',
        bridge=('halo2::native_player_action_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](std::uint32_t p,SourcePhysicsContactImage image){{return canonical_branch(p,image);}},\n'
                '        [](auto &v){{run_bd080(v);}},[](auto &v){{run_1540d0(v);}},[](auto &v){{run_153f20(v);}},\n'
                '        [](auto &v){{run_154270(v);}},[](auto &v){{run_1541b0(v);}})'),
        direct=('bd080', '1540d0', '153f20', '154270', '1541b0'), dispatched=()),
    'bd460': dict(
        pin='209045307c9adc8c920b40a6a14f51b5b96f0238142a14a3a2a7a27441c3356e',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_cluster_objects.hpp"\n',
        bridge='halo2::native_cluster_objects_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '14d200': dict(
        pin='88763e8c68ddd158b34176b63f9b04b805c4956fbce0664dbcdf1619be1a1052',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_aim_clusters.hpp"\n',
        bridge='halo2::native_aim_clusters_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1ae2b0': dict(
        pin='d448985e30cf69721b7d13b2d49c73308b02e5cc02d53a18e442b99335076b16',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_aim_targets.hpp"\n',
        bridge=('halo2::native_aim_targets_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_1ae500(v);}},[](auto &v){{run_1ae7f0(v);}},[](auto &v){{run_1ae2b0(v);}})'),
        direct=('1ae500', '1ae7f0'), dispatched=()),
    '1ae090': dict(
        pin='044a0bbfb1e6a2fab4e4eeac72a2deca26bca09ece4dcb0a1440b5e668a28f1f',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_aim_search.hpp"\n',
        bridge=('halo2::native_aim_search_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](auto &v){{run_14d200(v);}},[](auto &v){{run_bd460(v);}},[](auto &v){{run_16ea60(v);}},\n'
                '        [](auto &v){{run_1ae2b0(v);}})'),
        direct=('14d200', 'bd460', '16ea60', '1ae2b0'), dispatched=('32a660',)),
    '3d7e0': dict(
        pin='b79e7a38c1e0c85a08ae43750bf4ea710d489259c147f3981c69c31b5959cdc5',
        unit='#include "halo2/source_shadow_frame_tu15.hpp"',
        include='#include "rendering/shadows/native_tu15_shadow_fade.hpp"\n',
        bridge=('halo2::native_shadow_fade_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_30c40(v);}},[](auto &v){{run_4bae0(v);}},[](auto &v){{run_bcfb0(v);}},\n'
                '        [](auto &v){{run_3d7e0(v);}})'),
        direct=('30c40', '4bae0', 'bcfb0'), dispatched=()),
    '16b240': dict(
        pin='891c3d2a1789d449c035411398f7e02747fb936058fce726c4af93d6cd9e037f',
        unit='#include "halo2/source_sandbox_special_tu15.hpp"',
        include='#include "gameplay/objects/native_tu15_node_gather.hpp"\n',
        bridge=('halo2::native_node_gather_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('145560',)),
    '170730': dict(
        pin='fd38dfa15e4bf2d2b962f1f9626c9b0e65a124630aaab94538b5d5e6d82f336c',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_node_hierarchy.hpp"\n',
        bridge=('halo2::native_node_hierarchy_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_145560(v);}},[](auto &v){{run_170730(v);}})'),
        direct=('145560',), dispatched=()),
    '1d2f10': dict(
        pin='8615ea67bcdc97e1a3b8645839009ab8f131925e725d1b50013a921c7b229253',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_partition_insert.hpp"\n',
        bridge=('halo2::native_partition_insert_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_14d080(v);}},[](auto &v){{run_14d0e0(v);}},[](auto &v){{run_16e6a0(v);}})'),
        direct=('14d080', '14d0e0', '16e6a0'), dispatched=()),
    '157fe0': dict(
        pin='26f8e18d8e91cfa6d85edc5e474d5d4615f4827d6c8f100f50a50a09768dd8e3',
        unit='#include "halo2/source_vehicle_director_tu15.hpp"',
        include='#include "gameplay/vehicles/native_tu15_player_directors.hpp"\n',
        bridge=('halo2::native_player_directors_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](auto &v){{run_158790(v);}},[](auto &v){{run_171ff0(v);}})'),
        direct=('158790', '171ff0'), dispatched=('1588b0',)),
    'e9530': dict(
        pin='c3a1334f4adf0991372a921f3779a09e5bba192d037bd985d7ab6087b9d5240e',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_object_flag_bit.hpp"\n',
        bridge='halo2::native_object_flag_bit_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '10e1f0': dict(
        pin='e480741d779be5b0cc80e6d3bd06261a76517d604fe018c43a3519064bb28d01',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_object_type_callbacks.hpp"\n',
        bridge=('halo2::native_object_type_callbacks_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    '125790': dict(
        pin='377489ca067fe590d1e80fe765a01f2d78b7f47f2a150310f56ad1f940d33e4e',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_tag_iterator.hpp"\n',
        bridge=('halo2::native_tag_iterator_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('125810',)),
    '1e1390': dict(
        pin='2a48feebe427498c0b51b6c90e6c409f5e97819150ab0d1f734fdbbc4e46a5ed',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_node_chain.hpp"\n',
        bridge=('halo2::native_node_chain_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](auto &v){{run_145560(v);}},[](auto &v){{run_122be0(v);}},[](auto &v){{run_170730(v);}})'),
        direct=('145560', '122be0', '170730'), dispatched=('144090',)),
    '116060': dict(
        pin='780f443b578d6e34409281920769ee0522a6c7b73208e3f7bf58c44d14fe36fc',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_marker_chains.hpp"\n',
        bridge=('halo2::native_marker_chains_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_1d4140(v);}},[](auto &v){{run_1e5250(v);}},[](auto &v){{run_1e51d0(v);}},\n'
                '        [](auto &v){{run_bad60(v);}},[](auto &v){{run_1e1390(v);}})'),
        direct=('1d4140', '1e5250', '1e51d0', 'bad60', '1e1390'), dispatched=()),
    'aa200': dict(
        pin=('ac5a27dde8fd84104755cece87372afaae57ec1023cc96e62940a3f615369bee',
             '339499051be0fb3e4ca29d26dcd50eba18dd98cb1ac1e176daaf02a958fc561e',
             '19849730e2547cf642c4c4a26b8c460c29b5e04c7053b4575559f62bdfc9d486'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/death_cycle_runtime_tu15.hpp"', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_tolerance_checks.hpp"\n',
        bridge='halo2::native_tolerance_checks_detail::bridge_unit_length(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'aa270': dict(
        pin=('83c7f22c5fd34e8ac336995689a6cfa464b6b5eabd45b88d6a5f01bbcdbd32a8',
             '6218ca4627ac52804907c91f89b8194a883f6a49757ee78414e52ef326e54644',
             '2944ea17d36c98658b8d06b7b488e3fb324bf0b751ce6ee3e2591286adbc9194'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/death_cycle_runtime_tu15.hpp"', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_tolerance_checks.hpp"\n',
        bridge='halo2::native_tolerance_checks_detail::bridge_near(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'b9390': dict(
        pin='50c08ffa11019c8366df7ed02d46781729eac509472631c22940e2d47e2eed1f',
        unit=('#include "halo2/source_scene_device_constructor_tu15.hpp"', '#include "halo2/source_ai_actor_tu15.hpp"',
              '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "gameplay/objects/native_tu15_wake_chain.hpp"\n',
        bridge='halo2::native_wake_chain_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1751b0': dict(
        pin='e91fd81757594e8c0f12dfb7629c52ccfc86a965820f673849c1d61ea159ddad',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_world_bounds.hpp"\n',
        bridge='halo2::native_world_bounds_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'e65a0': dict(
        pin='7eca5c7fce66639bba2446dd6a19f48cea0ae73658f6ebc261f5b65ea7501d77',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_unit_ready.hpp"\n',
        bridge='halo2::native_unit_ready_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '106230': dict(
        pin='36e640516f991bb5a1a569498604cd1b9a61da9f01ea75549a811e71beb088d4',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_slot_kinds.hpp"\n',
        bridge='halo2::native_slot_kinds_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1d9f50': dict(
        pin='55120dc828a790f066e80711e1e792be5138a4b722f80f707e50d5fd5b4c8c65',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_entry_kind.hpp"\n',
        bridge=('halo2::native_entry_kind_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    '144cb0': dict(
        pin='43be1bacd3fe4ec4203e8822d46ec6c823dd5ac43e1018944b50dd6312728083',
        unit='#include "halo2/source_sandbox_special_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_matrix_reset.hpp"\n',
        bridge=('halo2::native_matrix_reset_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('144910',)),
    'bac30': dict(
        pin=('2f3f45e6ae89be9b6a635862ac04b478c618ebd9116357117ac62276dcf063c5',
             '769a1be935b81c6c9a5c732eb9ef47fe70e39ff08f70f373a0364db6437f8e46'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_sandbox_special_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_node_matrices.hpp"\n',
        bridge='halo2::native_node_matrices_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'b9460': dict(
        pin='31307b523eef7419b0369ec53a02128fca703a0aef3b44169a11bdb0dab1bae5',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_object_motion_update.hpp"\n',
        bridge=('halo2::native_object_motion_update_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_ba7e0(v);}},[](auto &v){{run_b7e50(v);}},\n'
                '        [](auto &v){{run_1cd620(v);}},[](auto &v){{run_ba630(v);}})'),
        direct=('ba7e0', 'b7e50', '1cd620', 'ba630'), dispatched=()),
    '10dfa0': dict(
        pin=('b25e8ca933f98d493b44d25d0c2545aa77b662380387208ec7ba8ef0976ed86b',
             '4c6a257f0a47e476aac532d4e351a58a82d134ff405e161f46a20fb7377db659'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_combat_action_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_object_type_notification.hpp"\n',
        bridge=('halo2::native_object_type_notification_detail::bridge(s,[](auto &v){{{reference}(v);}},'
                '[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    '10e340': dict(
        pin=('5fd76825a04213d40a5747f42b7a48d1989b5d4c6ae2fbca426ab992f69fa687',
             '409ebc9670b189043b6aa22794850fa6cfd61f4eb85459c7b03e1f8ad00b9f5e'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/death_cycle_runtime_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_object_type_events.hpp"\n',
        bridge=('halo2::native_object_type_events_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    'be7e0': dict(
        pin='fcf7198485fdacf60891eb2116964e4dfe6d1dda7815e6c393aef352ad936138',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "gameplay/objects/native_tu15_wake_children.hpp"\n',
        bridge=('halo2::native_wake_children_detail::bridge_children(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_b9390(v);}},[](auto &v){{run_be7e0(v);}})'),
        direct=('b9390', 'be7e0'), dispatched=()),
    'bda50': dict(
        pin='493f45fa501084317ecc04558b8575321fae1c6a3eaf37e46d1ccc96ab2f8a96',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "gameplay/objects/native_tu15_wake_tree.hpp"\n',
        bridge=('halo2::native_wake_tree_detail::bridge_wake(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_bcfb0(v);}},[](auto &v){{run_b9390(v);}},[](auto &v){{run_be7e0(v);}})'),
        direct=('bcfb0', 'b9390', 'be7e0'), dispatched=()),
    '1ce810': dict(
        pin=('6fe05cd089151bf0e6a97caa320133ab6d410af1cd1ebabfc117e3c70d53788b',
             '27ab45fa9e4bdb079bb693f153851980ed72128595604c074d3191547faa800b'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_actor_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_animation_clamp.hpp"\n',
        bridge=('halo2::native_animation_clamp_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_1e3150(v);}},[](auto &v){{run_1cec60(v);}})'),
        direct=('1e3150', '1cec60'), dispatched=()),
    '145b70': dict(
        pin='943da3f610d98eb99483ff7bbc75b82cccb549447393d016556cc89871ece6aa',
        unit='#include "halo2/death_cycle_runtime_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_rotate_vector.hpp"\n',
        bridge='halo2::native_rotate_vector_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '130d20': dict(
        pin=('82c832bee4f52280d3dd0614e6b39f9aa0d6960a4fa7584be0faf9baebc3705d',
             'cbef59797604698119fbe6eccad4427c47f1e179d9ca48c2eff829f717f3048f',
             'bd6a32e505a5008cf72a19f3c1d28f926e4e384942ca9adf361568124d092d88'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_geometry_cache_frame_tu15.hpp"',
              '#include "halo2/source_model_render_tu15.hpp"', '#include "halo2/source_map_world_support_tu15.hpp"',
              '#include "halo2/source_shadow_frame_tu15.hpp"'),
        include='#include "rendering/native_tu15_cache_entry.hpp"\n',
        bridge=(
            'halo2::native_cache_entry_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_22a790(v);}})',
            'halo2::native_cache_entry_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_22a790(v);}})',
            ('halo2::native_cache_entry_detail::bridge(s,[](auto &v){{{reference}(v);}},'
             '[](auto &v){{dispatch(v,0x22a790u,0x130d92u,false);}})')),
        direct=(('22a790',), ('22a790',), ()), dispatched=((), (), ('22a790',))),
    'e0500': dict(
        pin='12b9de69ead77b762b4edf39085c5b658effc69bd3b6f2345fec315f31ffd098',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_unit_motion.hpp"\n',
        bridge='halo2::native_unit_motion_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_e65a0(v);}})',
        direct=('e65a0',), dispatched=()),
    'c0700': dict(
        pin='f65f4822ab692da003586311beb20460d2e3691059d7df1734d018f4ea5ee420',
        unit='#include "halo2/source_shadow_frame_tu15.hpp"',
        include='#include "rendering/shadows/native_tu15_function_scale.hpp"\n',
        bridge=('halo2::native_function_scale_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('bcb70',)),
    '1eedb0': dict(
        pin='a3349f389585db7785924eddc7548fefdf0bb2ff9c8b50426f29849bd7345f4e',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/players/native_tu15_player_input.hpp"\n',
        bridge='halo2::native_player_input_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1f0f90(v);}})',
        direct=('1f0f90',), dispatched=()),
    '18bbc0': dict(
        pin='0534df030f946a51ac7503adc6f165948415b0e0c400083862dc7d9b203a80b0',
        unit='#include "halo2/source_effect_particle_frame_tu15.hpp"',
        include='#include "rendering/effects/native_tu15_packed_color.hpp"\n',
        bridge='halo2::native_packed_color_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'c93d0': dict(
        pin=('7828d60494a0ec1124c4390e8439e683eb2ab7e4e4da56ec52c20beb08594e48',
             '1307428902db3733e2ce55ec8b01cdd423231faa65794784a5ffb945cc8236f7'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "campaign/ai/native_tu15_state_reset.hpp"\n',
        bridge='halo2::native_state_reset_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'c92c0': dict(
        pin=('87d76d3778cd774cebb55ebd74f5e8c9c818a6cbf349d7d12b95dd692f7dc0eb',
             '45aac1ad426e46daca4b0563f646a9bc930711db650772222e4ed297726ca376'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "campaign/ai/native_tu15_state_apply.hpp"\n',
        bridge='halo2::native_state_apply_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1e1620': dict(
        pin=('53687cf4c618def902e112025560bf799282f511276cc82941ab192e851b6318',
             'd91a892ecd1afbbee69eb5012a1fafcbac281141cd4de5cf14cfddc2f8a30e27'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_transition_fraction.hpp"\n',
        # 1E1920 is a direct child in live support and dispatched in the AI
        # frame; both units' dispatch reaches it.
        bridge=('halo2::native_transition_fraction_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](auto &v){{run_110fc0(v);}})'),
        direct=('110fc0',), dispatched=()),
    '11fa50': dict(
        # The AI frame copy spills through native_single_spill (C1 kept) and
        # stays translated until its own adapter variant exists.
        pin='123f997668a2ba1ffe55d036ede8b7ab13a130935d27e615f2d7a818e6917d2d',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_perpendicular.hpp"\n',
        bridge='halo2::native_perpendicular_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'dfd20': dict(
        pin='2d6531560bd6d04cbd59e91d375f8627996941988eba757e2c7afbafab86987f',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_unit_speed_scale.hpp"\n',
        bridge='halo2::native_unit_speed_scale_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_2647e0(v);}})',
        direct=('2647e0',), dispatched=()),
    '110fc0': dict(
        pin=('a0816815dc827b371c0aef4cd99a19d455251175d44367806cf07433c9b965b5',
             '9848b0d8d40d710de8a469c08dc4333edd665540c985299b73ff35b899483d55'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_smoothstep.hpp"\n',
        bridge='halo2::native_smoothstep_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1e1920': dict(
        pin='b46b7fc84480f421726bee12d1ebdd484f4a37e582d43246895ea40faa45b410',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_smootherstep.hpp"\n',
        bridge='halo2::native_smootherstep_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1a8f10': dict(
        pin='9b1df173f951f6d414cfc437660ca931358888043579cd4128098d3072626329',
        unit='#include "halo2/source_ctf_player_iterator_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_linked_iterator.hpp"\n',
        bridge=('halo2::native_linked_iterator_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('16ea60',)),
    '175300': dict(
        pin='535cc4d3ca9e62a08e377b09b0dd0049f2a53013f20419bc16dbf8aab9563718',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_placement_check.hpp"\n',
        bridge=('halo2::native_placement_check_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target){{switch(target){{case 0xb1720u:run_b1720(v);return;'
                'case 0x1751b0u:run_1751b0(v);return;case 0xb16c0u:run_b16c0(v);return;'
                'case 0x175280u:run_175280(v);return;case 0x175240u:run_175240(v);return;'
                'default:run_1752c0(v);return;}}}})'),
        direct=('b1720', '1751b0', 'b16c0', '175280', '175240', '1752c0'), dispatched=()),
    '175280': dict(
        pin='6ae7fd7d01cff6e6ddd8c58c7176f25929567d4d3d625444ea1b1740be0e58fd',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_scalar_range.hpp"\n',
        bridge='halo2::native_scalar_range_detail::bridge<0>(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '175240': dict(
        pin='185edc5329e1662ba3cf2bf52302fcc9e3901b95dae9b3c48d29794822ef29fd',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_scalar_range.hpp"\n',
        bridge='halo2::native_scalar_range_detail::bridge<1>(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1752c0': dict(
        pin='435f028fb09e101decd99281066400c1d554146dfc503dcbac4f28a178c381cb',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_scalar_range.hpp"\n',
        bridge='halo2::native_scalar_range_detail::bridge<2>(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'b16c0': dict(
        pin='7cf65a4ccab2ed88923b4ba60938e0aea9921120bf93cbb40a32435f0fc9136f',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_finite_vector.hpp"\n',
        bridge='halo2::native_finite_vector_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '17c5c0': dict(
        # The copies differ only in which children are direct; every unit's
        # dispatch reaches its own bodies, so all four go through it.
        pin=('998d30693c176dc6e89865d848a6dddeda6599c8808203e2aabe0bc1715de4bd',
             '1a3262a5bdf8e734ebb9bade3075fb8ce0c10dc8eece030be0c9a12856502173'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/death_cycle_runtime_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_effect_activity.hpp"\n',
        bridge=('halo2::native_effect_activity_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    '1ae500': dict(
        pin='fbbdea4de3c83892a32f25d0043ef8df2829dc3511f60d04feaae69bdb470ff5',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_action_target_check.hpp"\n',
        bridge=('halo2::native_action_target_check_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_1ae5e0(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('1ae5e0',), dispatched=('1e7810',)),
    '212c70': dict(
        pin=('0aafcaf62336f8ac38cc3c078987cfcfeda40b97384ab49897092cd12a8bfcae',
             'e67ecbce587fed74ed5df738faed4a0ded6f5e46a2e6bedb300d16791186a539'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_map_world_support_tu15.hpp"'),
        include='#include "rendering/models/native_tu15_object_model_view.hpp"\n',
        bridge=('halo2::native_object_model_view_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('bc190',)),
    '30c40': dict(
        pin='0975141def20874ba72dc0b441b03060edab46b70b048e3caa06d651490bbd15',
        unit='#include "halo2/source_shadow_frame_tu15.hpp"',
        include='#include "rendering/shadows/native_tu15_object_origin.hpp"\n',
        bridge='halo2::native_object_origin_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '11e060': dict(
        pin='f92ef191ead14f90a4e2e3370a17a9a25702022505162718adc9702e4dfc112d',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_kind_entry.hpp"\n',
        bridge='halo2::native_kind_entry_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'd2330': dict(
        pin='87225c5b1875cd5a01dc1ec3ae13a1306b85fb3767e849b86d104570df6d37b3',
        unit='#include "halo2/source_weapon_hud_tu15.hpp"',
        include='#include "gameplay/weapons/native_tu15_flagged_slot.hpp"\n',
        bridge='halo2::native_flagged_slot_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '105950': dict(
        pin='a48e49e89a7c908df54689755e2d6f556c1883a184494dce134fb75334fffc1f',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_active_entry.hpp"\n',
        bridge='halo2::native_active_entry_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '10dde0': dict(
        pin='fcb0fb5e188d6c27a0cc3ca9a5217a5dfa8a311a26beca22dbbc24ad1da40698',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_object_type_query_chain.hpp"\n',
        bridge=('halo2::native_object_type_query_chain_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    '10b0f0': dict(
        pin='590f70aae30b741772af19d6af2071da6eecd1db5195645c3448e7d0cf1a9f35',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_linked_object_query.hpp"\n',
        bridge='halo2::native_linked_object_query_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_15c7f0(v);}})',
        direct=('15c7f0',), dispatched=()),
    '1d3f90': dict(
        pin='d605f23ad2ad86e96f7fc75dd6e1a3ad48b073cad3d23325abd2e4b35ce72322',
        unit='#include "halo2/source_sandbox_special_tu15.hpp"',
        include='#include "gameplay/objects/native_tu15_record_iterator_start.hpp"\n',
        bridge=('halo2::native_record_iterator_start_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('1e5250',)),
    '16e290': dict(
        pin='c08d5f41f937bd572c46d98012a69604756530f1646392822d26959cb9c73284',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_object_state_bits.hpp"\n',
        bridge='halo2::native_object_state_bits_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'b8d90': dict(
        pin='2601ba08d2da9d6f63661bf6592c50c3644d2fea55a9ee931d93966642c23e48',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_object_cluster_test.hpp"\n',
        bridge='halo2::native_object_cluster_test_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_10dde0(v);}})',
        direct=('10dde0',), dispatched=()),
    'bbff0': dict(
        pin=('76ede9135d9fad1df5c86de95e4ef3fa8e66f0596fffbf606d4fe974b11998d3',
             'f2637d214d95b5e494573a577a7dda0a6b4770bc6e269f3483e2a49454517180'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_object_axes.hpp"\n',
        bridge='halo2::native_object_axes_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'caaa0': dict(
        pin='aa54b96f3d51c8d3487953649b82498b23e25f511f7af088ecc70ad6799d0d3b',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_unit_free.hpp"\n',
        bridge=('halo2::native_unit_free_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_e9530(v);}},[](auto &v){{run_105950(v);}})'),
        direct=('e9530', '105950'), dispatched=()),
    '14fbc0': dict(
        pin='7d7e36de209174e951d1b756fe23789a44e38787b62731ab2888a926e5714ab7',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_record_sanity.hpp"\n',
        bridge='halo2::native_record_sanity_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'ce230': dict(
        pin=('a3ca26ba1d139b7396eaf4ebc4c0af35c0fbafe83ca8fab7fb9f9ab40b292a59',
             'b6a08887cf850315238154e664c5b36b50f33c7723ed093257730745e3e96b89'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_slot_handle.hpp"\n',
        bridge='halo2::native_slot_handle_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'cfb40': dict(
        pin=('01b0b813493d976a38ba3667602a1faa7be116bed32378d1b97468f38a58550b',
             '9b60e5decc0d138e461d2f5286fc783e7bf33bda212f6254bfa08520e49abd31'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_actor_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_both_slots.hpp"\n',
        bridge='halo2::native_both_slots_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '13f620': dict(
        pin='9c531d4ae268cc54c15075b5f5411017aeb964c54b1806a0bbc331a9384c716e',
        unit='#include "halo2/source_sandbox_special_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_game_hold.hpp"\n',
        bridge='halo2::native_game_hold_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '11e780': dict(
        pin='a66f8e4d5711423384ff167b36ece8ff8eb40f71fbbb267ee3b965a810a4bb91',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_cluster_visible.hpp"\n',
        bridge='halo2::native_cluster_visible_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'e3310': dict(
        pin=('58096b526ef7eafa7e0a5480a4a9bdd5d310e7710e16501c1f256f7ebaec2b77',
             '0768512f428e2bedade71593108c4a0d15f4d9c8bf58f8c2302f9a2c6a6dc912'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_unit_override.hpp"\n',
        bridge='halo2::native_unit_override_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '15dd10': dict(
        pin=('fb54b62b8bf0ccd19f1ca896d43620345e8c959e183aa789f58b806325127789',
             '1722a761e4917e4b2368a29b6953988f301aee54abb6b2bc9d36450f7f43d1d5'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/death_cycle_runtime_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_session_variant.hpp"\n',
        bridge='halo2::native_session_variant_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'bc330': dict(
        pin=('7909ea7170d1296c0d55e4e5734c594ec62f1ea31f7c970f4b19eac54694ee83',
             '767ce3a56a34c1d5bd1520256102a4c6ebda04f7a084c036f60709296265ed28'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_sandbox_special_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_root_words.hpp"\n',
        bridge='halo2::native_root_words_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'bbd50': dict(
        pin='2fe79dc741f6219336d9bb5a907d6565326465c46876dd192800b9c9e3194e8d',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_root_flag.hpp"\n',
        bridge='halo2::native_root_flag_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '15c7f0': dict(
        pin='689c4391dbaa07282e3926eec6c4b6880c910dabdfdf10df5f08d8648ff970f5',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_team_query.hpp"\n',
        bridge='halo2::native_team_query_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'df820': dict(
        pin='2cc672fc0d2b38a16b394f2520bd0ca7633c0657379bf4f59cebf20917b7197c',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_code_class.hpp"\n',
        bridge='halo2::native_code_class_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '111d10': dict(
        pin=('8359d0de88f39372e9a75670ad01db1e5db8cc6de31acc6fd0f3b666cc859e1a',
             '62bdf94f2c0da1c3114d5a7b895d2e1187d73fd11515c5523b13bf3baaee23f7'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_animation_words.hpp"\n',
        bridge='halo2::native_animation_words_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'c5250': dict(
        pin='a5ab064c3878ea8c6e080dc6b873a597de4e1db6415867dd2152a5d799d4b246',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_partition_unlink.hpp"\n',
        bridge='halo2::native_partition_unlink_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1d30f0(v);}})',
        direct=('1d30f0',), dispatched=()),
    '17c330': dict(
        # The copies differ only in whether BBD10 is direct; every unit's
        # dispatch reaches its own body, so both go through it.
        pin=('26e05176cf8f8ed1fc23461c52ec3885c60634a826fbdaa7871003bfb9e65533',
             '12e73e01bac18b5623d7a1054b58a2c9ce3a6b6a862042c8306154ec84473a91'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/death_cycle_runtime_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_header_unanswered.hpp"\n',
        bridge=('halo2::native_header_unanswered_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    'c9590': dict(
        pin=('4682619583dcc035991b02f9d46ae1771773c6ec8b721ebdc3aa7be7adad39d1',
             'ed4ec0c6079150e2b1fc5208aa4998cbffe00c7c7d43b8a7be0c2c30e0c41b78'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_special_animation.hpp"\n',
        bridge='halo2::native_special_animation_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_c9550(v);}})',
        direct=('c9550',), dispatched=()),
    '1e5d60': dict(
        pin='c52b4264e4c2ea70b53bf130139a8f09331773face9f7c485c4c207c52ddb83f',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_entry_refresh.hpp"\n',
        bridge='halo2::native_entry_refresh_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_126200(v);}})',
        direct=('126200',), dispatched=()),
    'd2b00': dict(
        pin='2f6686677e8fb74ef982291a16d4f562fe929536fa36519f2faf0ea492edb707',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_unit_chain.hpp"\n',
        bridge='halo2::native_unit_chain_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1062a0': dict(
        pin='b74392fccce51f304eed0c001c633a75c38ef1ab6e1326e51baea3a722114a02',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_entry_kind.hpp"\n',
        bridge='halo2::native_entry_kind_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'ba910': dict(
        pin='e5610e162d78c000da4b78eff1ec2c2ad18d2d293cd6c904753710f80605b39d',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_object_summary.hpp"\n',
        bridge='halo2::native_object_summary_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_16e290(v);}})',
        direct=('16e290',), dispatched=()),
    '1cd620': dict(
        pin='96c7693313f3ad268c7424de8cdfbc585cdf567b1a8cb5f500b40c679141d62e',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_cluster_notify.hpp"\n',
        bridge='halo2::native_cluster_notify_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1dc270(v);}})',
        direct=('1dc270',), dispatched=()),
    '11e600': dict(
        pin='ae6296f7d151e9be5c1872e99220af43c3652065d7eb742945592d1cca331c9a',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_point_location.hpp"\n',
        bridge='halo2::native_point_location_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_14cdb0(v);}})',
        direct=('14cdb0',), dispatched=()),
    '152e00': dict(
        pin='8677baff676d43a3cc0c1856cdaf9423b52f7123154a18130075c0d0ec9ea4d0',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_player_biped.hpp"\n',
        bridge='halo2::native_player_biped_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_e65a0(v);}})',
        direct=('e65a0',), dispatched=()),
    '106040': dict(
        pin='0d9ca517f00cd9800816cf81c1d97c491737b7a1a283d9af9217378a654afc40',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_inactive_choice.hpp"\n',
        bridge='halo2::native_inactive_choice_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_105950(v);}})',
        direct=('105950',), dispatched=()),
    '115ff0': dict(
        pin='3f44cddccdbe5b899d2c70114e90c53fb9f67e8ac2727fc55aa49e2f68c84db1',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_timer_sweep.hpp"\n',
        bridge='halo2::native_timer_sweep_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e15d0(v);}})',
        direct=('1e15d0',), dispatched=()),
    '1e15d0': dict(
        pin='9dcfc8b473af87a7df592e58efb0792f9f3cb2c9e03cd1cfb3f0389e98a5128c',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_byte_timer.hpp"\n',
        bridge='halo2::native_byte_timer_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1cee60': dict(
        pin='9ba9d9c7fd79cb8adf70d0ac6dcb03c0ebe7d1df107b5ae98204dc42ef396eaa',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_indexed_tag.hpp"\n',
        bridge='halo2::native_indexed_tag_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'c6e50': dict(
        pin='13bba64b9e60714213c05430df5a57d5e1bc3f745b84742a7e74f881946e9583',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_uncontrolled_unit.hpp"\n',
        bridge='halo2::native_uncontrolled_unit_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_c6dd0(v);}})',
        direct=('c6dd0',), dispatched=()),
    '15b470': dict(
        pin=('eea417835ab34ca8f5688520464d0b52b38de9297fd1122bb8cecd3df538c1ee',
             '59cd8a39f514abf8e4718e7f1a13026ac0e6d10f91058901be30530b1185ff03'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/death_cycle_runtime_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_player_live.hpp"\n',
        bridge='halo2::native_player_live_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_160570(v);}})',
        direct=('160570',), dispatched=()),
    '1f0f90': dict(
        pin='dbf7948970c5ac3ec8c119950c5028a821c9d334be8ed36ce525906fbe48a28d',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_repeat_tracker.hpp"\n',
        bridge='halo2::native_repeat_tracker_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '17e100': dict(
        pin='1aa727b2b95e1fca95c6d9bbe20eee62a208f6c4ed9348d526b796a251bd800c',
        unit='#include "halo2/death_cycle_runtime_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_role_check.hpp"\n',
        bridge='halo2::native_role_check_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '161560': dict(
        pin='dec476a391f37aef8af45489a44fe9a20e95d76a110160dcf8b88561e7380240',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_team_admission.hpp"\n',
        bridge='halo2::native_team_admission_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1059c0': dict(
        pin='cc7e5513fe3d8f78ad985e551c6af04f72ad6c0d9c798dd78470f43ac5fa33c5',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_seat_sum.hpp"\n',
        bridge='halo2::native_seat_sum_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_10b0f0(v);}})',
        direct=('10b0f0',), dispatched=()),
    '172200': dict(
        pin=('afb62d4e3fb88cd5fbe83baa94624458dd5355adb9aa6da92aa813689da9aa1d',
             '58513751545f3b7dc97a599ceba24d417999a27c9507d5fee849c6fd24164e15'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_combat_action_tu15.hpp"'),
        include='#include "gameplay/objects/native_tu15_indexed_record.hpp"\n',
        bridge='halo2::native_indexed_record_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'bcf70': dict(
        pin=('1879504173b3055505cf6c831c73d8a2d628f70a1ce803c91a816fe66166f47a',
             'd0f2953f417e69821d45a56b7294f2088ee56aa9c3088cdf2f4c2a945231b801',
             'd0f2953f417e69821d45a56b7294f2088ee56aa9c3088cdf2f4c2a945231b801'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"', '#include "halo2/source_sandbox_special_tu15.hpp"'),
        include='#include "gameplay/objects/native_tu15_attachment_root.hpp"\n',
        bridge='halo2::native_attachment_root_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '111f00': dict(
        pin='67e45fdfbe26edf57ed708d5fee8f875d03ee7be59896b2c4d20bbd6cab189eb',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_ready_slot.hpp"\n',
        bridge='halo2::native_ready_slot_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '144210': dict(
        pin='82ea961f596d1da33e1eca50bb4c78bb78f28b2182736e7800e8d96ba8b08eb5',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_orientation_frame.hpp"\n',
        bridge='halo2::native_orientation_frame_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1452f0': dict(
        pin='8f5f42cb10f2214bf032e28792350efa470befe467ec765a5f0fba654097bcc5',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_frame_projection.hpp"\n',
        bridge='halo2::native_frame_projection_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'ba7e0': dict(
        pin='41693fa487fb730c562cdcc3bd38af1a6f1f6818a4d786524e7188b8fc54276e',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_object_disconnect.hpp"\n',
        bridge='halo2::native_object_disconnect_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_c0f60(v);}},[](auto &v){{run_1d30f0(v);}})',
        direct=('c0f60', '1d30f0'), dispatched=()),
    'b92c0': dict(
        # The copies differ only in whether 1CC3F0 is direct; every unit's
        # dispatch reaches its own body, so both go through it.
        pin=('85ffc6c7e4d05b74abff39cfbe4cbb72ef849d76789f5f28f7fd9e6769812292',
             'ff92361a9ef9d737db7ca88a95a66a6d6a791f6249f52d2242489a740e168f1d'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_object_activate.hpp"\n',
        bridge=('halo2::native_object_activate_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    'd3a80': dict(
        pin='afa11228691a8b4ccbb7de6d306eec0ce6d03b763f57f1afc0003253856fe0a7',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_seat_pair.hpp"\n',
        bridge='halo2::native_seat_pair_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_106710(v);}},[](auto &v){{run_e8e10(v);}})',
        direct=('106710', 'e8e10'), dispatched=()),
    '31edf0': dict(
        pin='41fd447a1f142a873656c7e1b649c9ba96cbd65e6e147c73eb8d535c487e6792',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_six.hpp"\n',
        bridge='halo2::native_six_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '33210c': dict(
        pin=('a42a346234b0c3a4de5a9fe9be82642f6428bcf8d59033c0950b9a7f816536bd',
             'c883243c93bd40c17a0c1f1f1e0ac54e4fc042f2acf2e6bdea14d87bd5c3ef36'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_double_exponent.hpp"\n',
        bridge='halo2::native_double_exponent_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '161530': dict(
        pin=('b256d62c3c2e01fdf6d55973a79bd301dedc20ffb7cd63d586dfb809b93cab19',
             '9db0633dc6f47a7d7e437def67cc58ec6030bdad4bd12d3866d4563475b5435e'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_sandbox_special_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_variant_option.hpp"\n',
        bridge='halo2::native_variant_option_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1d4140': dict(
        pin='1789d3ac96a71a02a9aae9c8384d2643e9db99b92e93e120a351005b2accaab9',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_definition_word.hpp"\n',
        bridge='halo2::native_definition_word_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '116480': dict(
        pin=('a44c3cdadbe923066f730fee641d4c13ab19136f4fbed6fb8fa8c8416f795eda',
             '672f7fb6b9d8133d6fcb17e6a75ee10bf9c067341b6224abf5e4445f033b2cab'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_block_ready.hpp"\n',
        bridge='halo2::native_block_ready_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'e32c0': dict(
        pin='815b361c93f7a32048c906a4ae2836ddb7d8da88462149505c23f1d5baaaf205',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_tick_seconds.hpp"\n',
        bridge='halo2::native_tick_seconds_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'cb440': dict(
        pin='60aace1f5316be44509d535233ecce42fa0743265296fe17e9aef1ac6652dcee',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_child_unit.hpp"\n',
        bridge='halo2::native_child_unit_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '140ec0': dict(
        pin='7f5a4ae2911cb95b59ebdfb8cd4381360d6259fc94859981bc95493e98f6515d',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_bit_union.hpp"\n',
        bridge='halo2::native_bit_union_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '105fd0': dict(
        pin='2587a5396cfe9303bbf7ace858648284778f81eda5b8a5e249701acfb66bdc69',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_charge_gate.hpp"\n',
        bridge='halo2::native_charge_gate_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'c8c20': dict(
        pin='d968de36a016709216ba58c28e8bf1bde6599da2bcb19d7c5f4cbc5f7acdc24c',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_unit_countdowns.hpp"\n',
        bridge=('halo2::native_unit_countdowns_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1d1f30(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('1d1f30',), dispatched=('d2500',)),
    '113c70': dict(
        pin='490b7f70e7debb59ee499a37a48d17adb00547b5dfd67872c4f650fdb4eb98e6',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_animation_mask.hpp"\n',
        bridge='halo2::native_animation_mask_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e3150(v);}})',
        direct=('1e3150',), dispatched=()),
    '1e5160': dict(
        pin='9fadad726e9f037fe5dafd69dba56086082929489d765216e89d18d458222115',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_record_cursor.hpp"\n',
        bridge='halo2::native_record_cursor_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e5c80(v);}})',
        direct=('1e5c80',), dispatched=()),
    '1e5a70': dict(
        pin=('377e6f1753ac5e855f976450d6a0a22ae9bcfac74693954afcfaa4124d2ca3af',
             '993340532fcf94fa1c57a9063af889f2da274e95706e86c0d7090f791491794f'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_animation_entry.hpp"\n',
        bridge='halo2::native_animation_entry_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1262f0(v);}})',
        direct=('1262f0',), dispatched=()),
    '1e5df0': dict(
        pin=('97c5bb44b63c2d89b65432084170931403103676fe735fefc8f828bc64fcec54',
             '714edabb0c09173628fb4c416c6978e76c79779a17d043e4def037f2f9fa1670'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_animation_record.hpp"\n',
        bridge='halo2::native_animation_record_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e3150(v);}},[](auto &v){{run_1e5a70(v);}})',
        direct=('1e3150', '1e5a70'), dispatched=()),
    '1cf070': dict(
        pin='bd66187df0ea4de9713330243d5b09139257e8fbfbfc303643a2033343ada6dd',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_reference_pose.hpp"\n',
        bridge='halo2::native_reference_pose_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e5df0(v);}},[](auto &v){{run_212d40(v);}})',
        direct=('1e5df0', '212d40'), dispatched=()),
    '1cefb0': dict(
        pin='05b649561cb8c47222f5794b766678ad958beec0f3ed05b38299bd9f685f1a3a',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_scaled_pose.hpp"\n',
        bridge='halo2::native_scaled_pose_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1cf070(v);}})',
        direct=('1cf070',), dispatched=()),
    '1d4e50': dict(
        pin=('319ddb80da2abe94071070b0cfed954fe59f292afb5a73359eb7205a720abcbc',
             '8e7ae73575019ca4c9d4b318211f8b61f2fffa9ac183fa2b8dff4a59147f6665'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_actor_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_channel_reset.hpp"\n',
        bridge='halo2::native_channel_reset_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1fa110': dict(
        pin='18712fbd3cda8d9c9e0d808e22e39e5ca1647473f1dc66860c47de5e54f6d5a9',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_root_notice.hpp"\n',
        bridge=('halo2::native_root_notice_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('184ec0',)),
    'bbf20': dict(
        pin='72daf87fee0ab25317a71a2da61ef1c70fb55a8a3bc7f271512679b676fbec03',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_root_point.hpp"\n',
        bridge='halo2::native_root_point_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_bbe00(v);}})',
        direct=('bbe00',), dispatched=()),
    '1e5f20': dict(
        pin=('bf56b0fb46a44121f087792ce78cbd1c8942c67fda35123cd0e182db9172ed44',
             '5693ecd513e7dc8470039a33cbec94eae958a15f2fdddb564e5ccdaaf3b2f768',
             '5693ecd513e7dc8470039a33cbec94eae958a15f2fdddb564e5ccdaaf3b2f768'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"', '#include "halo2/source_ai_actor_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_animation_fallbacks.hpp"\n',
        bridge='halo2::native_animation_fallbacks_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e5e40(v);}})',
        direct=('1e5e40',), dispatched=()),
    '102910': dict(
        pin='a65189968721ef4aa5c4cf75c076397c783feb24cd60495ff32fabdd15e97cdb',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_flagged_objects.hpp"\n',
        bridge=('halo2::native_flagged_objects_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('bcee0', '102980')),
    '1e7810': dict(
        # The copies differ only in whether 160A60 is direct; every unit's
        # dispatch reaches its own body, so both go through it.
        pin=('618c712c0b5a231291006b32807c43723215fbe7cff066eedb4d9b0f65c14022',
             '8186ea1b82b04bea0f9bb27e18e715160d7d6229413c6c2be65afc3e3ad32156'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "gameplay/multiplayer/native_tu15_team_hostility.hpp"\n',
        bridge=('halo2::native_team_hostility_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_c1e90(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('c1e90',), dispatched=()),
    '212d40': dict(
        pin='e14eb5551d1e5c07903692e4dda5ed0d0bdce7eaff43de376044fd0191abb398',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_pose_sample.hpp"\n',
        bridge='halo2::native_pose_sample_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1e5e40': dict(
        pin=('cfd2b65d71d7453db5bc848d86428a0fecedc5e6a31d92ac07121ab834a1164a',
             '9c7a75bdf06ae6bb42006360018e478b0b6e0d4ffb33f89bd8b79ba790dfb59d'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"', '#include "halo2/source_ai_actor_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_animation_select.hpp"\n',
        bridge=('halo2::native_animation_select_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e5810(v);}},\n'
                '        [](auto &v){{run_126200(v);}})'),
        direct=('1e5810', '126200'), dispatched=()),
    '1521b0': dict(
        pin='437da0b1ba64a42e5af09ff419ac3558c359af3d3d1d54e06c6a3248aef91e4a',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/players/native_tu15_input_edges.hpp"\n',
        bridge='halo2::native_input_edges_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '152d60': dict(
        pin='ca9a7fb49060d90535adf45eddce8977189de8431105593f8091c7ae3d798d77',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_flat_axes.hpp"\n',
        bridge='halo2::native_flat_axes_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_30bd0(v);}})',
        direct=('30bd0',), dispatched=()),
    '2a0100': dict(
        # The live support copy passes the second key through the MCC class
        # symbol scope; the AI copies pass it as it is: a bridge per pin.
        pin=('870c47a6ae03315785475869234f7523495c1294c09755a2c015422fce5e203f',
             'c4c09d913c3630c0da22e58c5bcc9ce8e71c183fa503fb7bd8468bec0600b761'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"', '#include "halo2/source_ai_actor_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_animation_wildcard.hpp"\n',
        bridge=('halo2::native_animation_wildcard_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e5810(v);}},\n'
                '        [](auto &v,std::uint32_t key){{return source_weapon_animation_scoped_class_symbol_tu15(v.memory,key);}})',
                'halo2::native_animation_wildcard_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e5810(v);}},\n'
                '        [](auto &,std::uint32_t key){{return key;}})'),
        direct=('1e5810',), dispatched=()),
    'c8cf0': dict(
        pin='98623f0785386cd4527875f6c40d5dc1901c51eae3ee1e367f585dae9362afae',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_unit_camouflage.hpp"\n',
        bridge=('halo2::native_unit_camouflage_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_c1e90(v);}},\n'
                '        [](auto &v){{run_b7e50(v);}})'),
        direct=('c1e90', 'b7e50'), dispatched=()),
    '2db80': dict(
        pin='8afe9c3b701e70ce45d51a21f1a4efbcb5815669c5a681bf28234c239fec9084',
        unit='#include "halo2/source_lens_flare_tu15.hpp"',
        include='#include "rendering/effects/native_tu15_lens_request.hpp"\n',
        bridge=('halo2::native_lens_request_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_2e210(v);}},\n'
                '        [](auto &v){{run_20990(v);}},[](auto &v){{run_20b20(v);}})'),
        direct=('2e210', '20990', '20b20'), dispatched=()),
    '1be90': dict(
        pin='55e7ab34f13bd70478187a1efb334b92b3363bfa42887995bbecfa0abf3cb58d',
        unit='#include "halo2/source_shadow_frame_tu15.hpp"',
        include='#include "rendering/shadows/native_tu15_pixel_scales.hpp"\n',
        bridge='halo2::native_pixel_scales_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1d3930': dict(
        pin='1682c9ad3a96f4a5d5f90fe3805f94f73ad3552035cd150441df609e26af36c7',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_model_switch.hpp"\n',
        bridge='halo2::native_model_switch_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1dc610': dict(
        pin='da2febaa31648447262e43b108d6287a49b7e8dcc1077dc67e06a5f7249b714c',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_record_frame.hpp"\n',
        bridge=('halo2::native_record_frame_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},[](auto &v){{run_1dc680(v);}})'),
        direct=('1dc680',), dispatched=('1d8b90', 'bc190')),
    'bc190': dict(
        pin='66413ec08faaa324709843f6cf45272eb7be2fb78c400ead310fb1662f1be3fc',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "gameplay/objects/native_tu15_object_frame.hpp"\n',
        bridge='halo2::native_object_frame_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_144bf0(v);}},[](auto &v){{run_145560(v);}})',
        direct=('144bf0', '145560'), dispatched=()),
    '10c5d0': dict(
        # 10C5D0 marks each child by calling itself: the child enters this
        # bridge again.
        pin='37a62ec4d1b7a19cc9c9a54aabb82c19273dcef7707b3ee10b1030e898a9bf1a',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "gameplay/objects/native_tu15_device_tree.hpp"\n',
        bridge='halo2::native_device_tree_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_b9390(v);}},[](auto &v){{run_10c5d0(v);}})',
        direct=('b9390', '10c5d0'), dispatched=()),
    'dfe30': dict(
        pin='082f5f6fd0777bc932476261f6479abbebcd00dc2b2e9b49bc4b4433a2c48b77',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_object_rate.hpp"\n',
        bridge=('halo2::native_object_rate_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_dfd20(v);}},\n'
                '        [](auto &v){{run_e65a0(v);}},[](auto &v){{run_e3310(v);}})'),
        direct=('dfd20', 'e65a0', 'e3310'), dispatched=()),
    '189f20': dict(
        pin='1501c42828c41975974ea13e5ae7b3576b4147e81e26d37719dc4cacc2cd57b3',
        unit='#include "halo2/source_vehicle_director_tu15.hpp"',
        include='#include "gameplay/vehicles/native_tu15_director_value.hpp"\n',
        bridge='halo2::native_director_value_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_cad60(v);}})',
        direct=('cad60',), dispatched=()),
    '158450': dict(
        pin='419d343ba0ef3a6043bdf24dcd62f61b7d4948f550ad1494db1f5170e019c447',
        unit='#include "halo2/source_vehicle_director_tu15.hpp"',
        include='#include "gameplay/vehicles/native_tu15_seat_camera_mode.hpp"\n',
        bridge=('halo2::native_seat_camera_mode_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},[](auto &v){{run_158900(v);}})'),
        direct=('158900',), dispatched=('158340',)),
    '1e5a50': dict(
        pin='920f14d54c49a44f110c73fe63e1a5799f6ffd4ff8d0ef03f20e5d30e3cab77a',
        unit='#include "halo2/source_sandbox_special_tu15.hpp"',
        include='#include "gameplay/native_tu15_reference_clear.hpp"\n',
        bridge='halo2::native_small_queries_detail::reference_clear::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '335b0': dict(
        pin='be384e382f61f971b0c059a674360a720c4b1f6d662e80d902acda820b523e96',
        unit='#include "halo2/source_shadow_frame_tu15.hpp"',
        include='#include "gameplay/native_tu15_shadow_option.hpp"\n',
        bridge='halo2::native_small_queries_detail::shadow_option::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '150990': dict(
        pin='9e314bd5a0b0b907d9572729e736f4cb0331572f570a8b3ed18e331e7eb72ab2',
        unit=('#include "halo2/source_combat_action_tu15.hpp"', '#include "halo2/source_sandbox_special_tu15.hpp"'),
        include='#include "gameplay/native_tu15_seat_player.hpp"\n',
        bridge='halo2::native_small_queries_detail::seat_player::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '25bc70': dict(
        pin='3b96199ca10f5c7226d3005114d03914ddf49fa7e66a3843c4756631893e7320',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/native_tu15_word_change.hpp"\n',
        bridge='halo2::native_small_queries_detail::word_change::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '226b0': dict(
        pin='e1dcf87a833a6927e995734ee0e28b06a351b1a4ef06f669af6fdf1919c29e2c',
        unit='#include "halo2/source_shadow_frame_tu15.hpp"',
        include='#include "gameplay/native_tu15_cluster_match.hpp"\n',
        bridge='halo2::native_small_queries_detail::cluster_match::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '25bcc0': dict(
        pin='2c8cbc477d1ca0e5dcff412588694fd7250dfd51cbbf050c62fb1d9f4d582463',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_option_flag.hpp"\n',
        bridge='halo2::native_engine_queries_detail::option_flag::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '18a240': dict(
        pin='735786bcb7c4a1d8981fadae8bf4dcce9074711016942cf95cca8b3a4768fc61',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_director_player.hpp"\n',
        bridge='halo2::native_engine_queries_detail::director_player::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'd2b60': dict(
        pin='3d61eeb53bc779ec3ca7a8cea08d7eaaef17a7f32fe508de0556c9c08ee72b47',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_carrier.hpp"\n',
        bridge='halo2::native_carrier_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '171ff0': dict(
        pin='851932a5c40fe91ef25d3438f7b0629e740b4fceb2b285c5fe63308eb717078b',
        unit='#include "halo2/source_vehicle_director_tu15.hpp"',
        include='#include "gameplay/vehicles/native_tu15_director_bind.hpp"\n',
        bridge=('halo2::native_director_bind_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('175380',)),
    '20700': dict(
        pin='6e63157eafd10e603bdc847bfc7d894bb73cbe2bdb1771eafe3f456faa79cdb3',
        unit='#include "halo2/source_lens_flare_tu15.hpp"',
        include='#include "rendering/effects/native_tu15_lens_broadcast.hpp"\n',
        bridge='halo2::native_lens_broadcast_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_20e30(v);}})',
        direct=('20e30',), dispatched=()),
    '1d9f10': dict(
        pin=('b37f49303b44733a0884706cd83a3821e072a3b9a39729e9c2e7e1fe3a0de3ae',
             '22f5bc0de7965469f682b27792902ee041e5e04ec6ffdb033a964dbbc47073d6'),
        unit=('#include "halo2/death_cycle_runtime_tu15.hpp"', 'void source_run_physics_live_support_tu15('),
        include='#include "gameplay/objects/native_tu15_item_notice.hpp"\n',
        bridge=('halo2::native_item_notice_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('315cb0',)),
    '43980': dict(
        pin='cd2b3969fb50643b926ee5f8281e8a80a355da1751f1b0c903e5fa252060bc1c',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/native_tu15_stamp_advance.hpp"\n',
        bridge='halo2::native_stamps_detail::stamp_advance::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_13fd10(v);}},[](auto &v){{run_12f040(v);}})',
        direct=('13fd10', '12f040'), dispatched=()),
    '12f0c0': dict(
        pin='77cbd3e259341a653878ecf66906d9de61583ed49be4de0bd254604c6039960e',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/native_tu15_stamp_framed.hpp"\n',
        bridge='halo2::native_stamps_detail::stamp_framed::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_13fd10(v);}},[](auto &v){{run_12f040(v);}})',
        direct=('13fd10', '12f040'), dispatched=()),
    '118760': dict(
        pin='e853978f61380184a7f69e80f15f2919af7418d190f25331ea39eb10c818f411',
        unit='#include "halo2/death_cycle_runtime_tu15.hpp"',
        include='#include "gameplay/native_tu15_datum_visit.hpp"\n',
        bridge=('halo2::native_datum_visit_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('47860',)),
    '1d6430': dict(
        pin=('36394fa3a4ccec292a5daa358c29b71ed00c5953f823ed3b1dd4bb4323d9899b',
             '3107410cea43f88185721fe20d797696ff4326f450b5ffe79333a1ce077fa721'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"', '#include "halo2/source_ai_actor_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_tagged_play.hpp"\n',
        bridge='halo2::native_tagged_play_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1d45a0(v);}},[](auto &v){{run_1e5fb0(v);}})',
        direct=('1d45a0', '1e5fb0'), dispatched=()),
    '304c50': dict(
        pin='a415fcedc34ca039bbe2b3ae0e4c11435c6bb9ea92dd170c601f75cfd38aeae1',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_record_view.hpp"\n',
        bridge='halo2::native_record_view_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_2f4990(v);}})',
        direct=('2f4990',), dispatched=()),
    '1f33c0': dict(
        pin='e14dd1d8e428881c36fc1b8bef4483142fca7f5e344b0d849d4036cc0d677f30',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_page_scan.hpp"\n',
        bridge=('halo2::native_page_scan_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('1f3460',)),
    '1582f0': dict(
        pin='f0a89badaf50ddb7b232c9ab388fb8223dff05bbaf29b2b866d5130da95a7645',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_mode_action.hpp"\n',
        bridge=('halo2::native_mode_action_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},[](auto &v){{run_244f80(v);}},[](auto &v){{run_245d10(v);}})'),
        direct=('244f80', '245d10'), dispatched=('158340',)),
    '243510': dict(
        pin='ba39070d97a615ecfc65e23ea425208d607cac34070873190ede76fcbcd2ba12',
        unit=('#include "halo2/pstencil/native_light_portal_tu15.hpp"', 'void source_run_physics_live_support_tu15('),
        include='#include "physics/runtime/native_tu15_polygon_inside.hpp"\n',
        bridge='halo2::native_polygon_inside_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'c5170': dict(
        pin='21627799c852cc060faa7978e11325c172b4163c4bb18ff6614a2b25c2e9a93e',
        unit='#include "halo2/pstencil/native_light_portal_tu15.hpp"',
        include='#include "pstencil/native_tu15_light_admits.hpp"\n',
        bridge='halo2::native_light_admits_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '31500': dict(
        pin='08205d954289c735534fbc698e4949256bfcf5a22bee47d930117fd8d9bfacf0',
        unit='#include "halo2/pstencil/native_light_portal_tu15.hpp"',
        include='#include "pstencil/native_tu15_light_gate.hpp"\n',
        bridge=('halo2::native_light_gate_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('3e9b0',)),
    '10db40': dict(
        # The hook functions are reached through the unit's dispatch by address.
        pin='8455549202b0626dee1a1a73cd758800557d0c6bde04565630335a3d90d14844',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_type_hooks.hpp"\n',
        bridge=('halo2::native_type_hooks_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    '16e7f0': dict(
        # 16E860 is direct in the UI copy and through the dispatch in the
        # campaign copy: a bridge per pin.
        pin=('4a273827b185f2e3b9b68eef72bd1ea466c536f6e383c6081740a179c3783414',
             'e4e98921734bb94a31a2bfe32e70e7610234d9aa572bcd10ffe8058d68b62972'),
        unit=('#include "halo2/source_ui_script_startup_tu15.hpp"', '#include "halo2/source_campaign_runtime_tu15.hpp"'),
        include='#include "gameplay/native_tu15_datum_claim.hpp"\n',
        bridge=('halo2::native_datum_claim_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_16e860(v);}})',
                'halo2::native_datum_claim_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{dispatch(v,0x16e860u,0x16e83au,false);}})'),
        direct=(), dispatched=()),
    '2e3250': dict(
        # The allocator's functions are reached through the unit's dispatch by address.
        pin='776370c7b81df39130c8d0d8595682fddeea1a2426bb7f544bc2a89d5c431a42',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "rendering/renderer/native_tu15_array_grow.hpp"\n',
        bridge=('halo2::native_array_grow_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},[](auto &v){{run_2e2e30(v);}})'),
        direct=('2e2e30',), dispatched=()),
    '2e9750': dict(
        pin=('4250c2c455eda136af2335ec66fea417b20a47e4f62b0860ce703d5ede3b2edc',
             'fba4dd605fde6f7fdd298630cde1896d4863a1e6b124fc4333997bae12b615b9'),
        unit=('#include "halo2/death_cycle_runtime_tu15.hpp"', '#include "halo2/source_ai_frame_tu15.hpp"', 'void source_run_physics_live_support_tu15('),
        include='#include "physics/runtime/native_tu15_aligned_query.hpp"\n',
        bridge='halo2::native_aligned_query_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_2e96a0(v);}},[](auto &v){{run_30b890(v);}})',
        direct=('2e96a0', '30b890'), dispatched=()),
    '158340': dict(
        # 111CD0 and C9550 are direct in the live support copy and through the
        # dispatch in the campaign camera and map world copies: a bridge per pin.
        pin=('c4bf268a1e3f5fac92dcabf2007dd939731b176d0ae1c5a0041142a432de1b11',
             '85ef90e86d1ceb347592c550a4cf7ef5cea273d27841ecc62894e8cd100abada',
             '546f036d7f45f24a4065ace5fe300f3c40f29b131b57c40cb7b93577ad5c67ba'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_campaign_camera_tu15.hpp"',
              '#include "halo2/source_map_world_support_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_seat_mode.hpp"\n',
        bridge=('halo2::native_seat_mode_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](auto &v){{run_111cd0(v);}},[](auto &v){{run_c9550(v);}})',
                'halo2::native_seat_mode_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](auto &v){{dispatch(v,0x111cd0u,0x158387u,false);}},[](auto &v){{dispatch(v,0xc9550u,0x1583ecu,false);}})',
                'halo2::native_seat_mode_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](auto &v){{dispatch(v,0x111cd0u,0x158387u,false);}},[](auto &v){{dispatch(v,0xc9550u,0x1583ecu,false);}})'),
        direct=(), dispatched=('bcdf0',)),
    '1d9080': dict(
        pin='8ddadb2bb98180b9a6b2426d6312145aa7409eabfb23c582621536dc79009727',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "rendering/renderer/native_tu15_item_light_place.hpp"\n',
        bridge=('halo2::native_item_light_detail::item_light_place::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_315c90(v);}},\n'
                '        [](auto &v){{run_315cb0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('315c90', '315cb0'), dispatched=()),
    '1d9100': dict(
        pin='da737feb3b611a6476138b1d88a6ebe92638093dbf22381c899c1d4db003f1cd',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "rendering/renderer/native_tu15_item_light_aim.hpp"\n',
        bridge=('halo2::native_item_light_detail::item_light_aim::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_315c90(v);}},\n'
                '        [](auto &v){{run_315cb0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('315c90', '315cb0'), dispatched=()),
    '3d2b0': dict(
        pin=('1902c419805d69dded14b66a0607cf4c07dcb4eea0db865c4f717ee82445059b',
             '1902c419805d69dded14b66a0607cf4c07dcb4eea0db865c4f717ee82445059b'),
        unit=('#include "halo2/source_texture_camera_producer_tu15.hpp"', '#include "halo2/source_first_person_light_pose_tu15.hpp"'),
        include='#include "gameplay/animation/native_tu15_first_person_unit.hpp"\n',
        bridge=('halo2::native_first_person_unit_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('bcdf0', '158290', '168ded')),
    '1e5360': dict(
        pin=('446b713a7a5a0400e3e8130c5c31f56c978b49741fa599737daf9c19f71c42df',
             '47005d5875487b48d237729061044ae8ad8d8c70405a28a4ca70a16a398e69ef'),
        unit=('void source_run_ai_frame_tu15(', 'void source_run_physics_live_support_tu15('),
        include='#include "campaign/ai/native_tu15_element_search.hpp"\n',
        bridge='halo2::native_element_search_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e5c80(v);}})',
        direct=('1e5c80',), dispatched=()),
    '168d76': dict(
        pin='ba5b9bb3f7ed71219e4ade05e702fc5dead54af13d19c4e020be2afe4c48f470',
        unit='#include "halo2/source_sandbox_special_tu15.hpp"',
        include='#include "gameplay/objects/native_tu15_model_record.hpp"\n',
        bridge=('halo2::native_model_record_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_16b240(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('16b240',), dispatched=('145560',)),
    '168ded': dict(
        pin='9ae52d7bd14cff355eb965bd92beb5befdf581b682d215a5487cce103102dc23',
        unit='#include "halo2/source_sandbox_special_tu15.hpp"',
        include='#include "gameplay/objects/native_tu15_first_person_model_preparation.hpp"\n',
        bridge=('halo2::native_first_person_model_preparation::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('bac30', '168d76', '16b4e8'), dispatched=('144bf0', '144090', '145560', '11e060')),
    '244f80': dict(
        pin='99d7a3e3d5bcb5a5c49fd5eef1da0c49c44d1fca6ad5f0a88b69121229cae108',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_unit_view.hpp"\n',
        bridge=('halo2::native_unit_view_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('cd4a0', 'bcdf0', 'bad60')),
    '158790': dict(
        pin='dc95461cf5d04a92e81a14eb7e0e0b764ff8b6dd55df70b1921f3613e1585ce1',
        unit='#include "halo2/source_vehicle_director_tu15.hpp"',
        include='#include "gameplay/vehicles/native_tu15_seat_camera_death.hpp"\n',
        bridge=('halo2::native_seat_camera_death_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_158450(v);}},\n'
                '        [](auto &v){{run_246cf0(v);}},[](auto &v){{run_158900(v);}})'),
        direct=('158450', '246cf0', '158900'), dispatched=()),
    '161610': dict(
        pin='95e50aee4bd3a03f60a30e074d477e84c57280423931c6db0cb042217f2dec8b',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_seat_status.hpp"\n',
        bridge='halo2::native_seat_status_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1618b0(v);}})',
        direct=('1618b0',), dispatched=()),
    '13da89': dict(
        pin='0cda52c5f2f46157b16652563c43124bb0b2f71f3239af718b7859eee4ed02ec',
        unit='#include "halo2/source_weapon_hud_tu15.hpp"',
        include='#include "gameplay/weapons/native_tu15_weapon_hud_state.hpp"\n',
        bridge='halo2::native_weapon_hud_state_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1055f0(v);}})',
        direct=('1055f0',), dispatched=()),
    '1540d0': dict(
        pin='da1ca43b8f6f585a6977bded5c97e7eea756bbd50bfdcd9337d37b0b54fc9545',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_biped_action.hpp"\n',
        bridge=('halo2::native_biped_action_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_e8110(v);}},\n'
                '        [](auto &v){{run_e79c0(v);}},[](auto &v){{run_153e10(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('e8110', 'e79c0', '153e10'), dispatched=('1e7810',)),
    '15b620': dict(
        pin='c5fd8b235e24d205bbf460819340e3c6c0fae52bc35bbe3c35a9696a7b9c1b86',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_team_count.hpp"\n',
        bridge=('halo2::native_team_count_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('15b470',)),
    '1ae5e0': dict(
        pin='c2097aa3e62d803e462a3314d569bd6936c8142cc89943dafb453a50cc95cd74',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_action_target_defer.hpp"\n',
        bridge='halo2::native_action_target_defer_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1ae500(v);}},[](auto &v){{run_1ae6f0(v);}})',
        direct=('1ae500', '1ae6f0'), dispatched=()),
    '15b2d0': dict(
        pin='3c00732bd7c7ee6b85639dac34a686d9d48c9edc149153ccaf43a0d4665add7f',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/players/native_tu15_player_factor.hpp"\n',
        bridge=('halo2::native_player_factor_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](std::uint32_t p,SourcePhysicsContactImage image){{return canonical_branch(p,image);}})'),
        direct=(), dispatched=()),
    '154d00': dict(
        pin='4d277d5da267ea0d49b206174bcbde72e40da0e9bfdf92a7a47a74205629c1fd',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/players/native_tu15_player_timers.hpp"\n',
        bridge=('halo2::native_player_timers_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('d3340',)),
    '13e670': dict(
        pin=('2635cbb9a1bd32e5db139644c9936ba5f75e6509dff5fa4a48f596067eb4880a',
             'e32af0851eb8fc3b0b328c50dc5b8a5e42fc9824168ebc3ab0b4c54416dd4de6'),
        unit=('#include "halo2/source_lens_draw_tu15.hpp"', 'void source_run_physics_live_support_tu15('),
        include='#include "rendering/effects/native_tu15_curve_value.hpp"\n',
        bridge='halo2::native_curve_value_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_13de70(v);}})',
        direct=('13de70',), dispatched=()),
    'c9010': dict(
        pin='6c086de170bb4a482396664f3c6e0de53dddb6c078420cedf8b968ebcac31f11',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_unit_actions.hpp"\n',
        bridge=('halo2::native_unit_actions_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target){{switch(target){{case 0xc6e90u:run_c6e90(v);break;case 0xc7dd0u:run_c7dd0(v);break;case 0xc85a0u:run_c85a0(v);break;case 0xc8390u:run_c8390(v);break;case 0xc8c20u:run_c8c20(v);break;case 0x117340u:run_117340(v);break;case 0xc8e70u:run_c8e70(v);break;case 0xc8cf0u:run_c8cf0(v);break;case 0xc7580u:run_c7580(v);break;}}}})'),
        direct=('c6e90', 'c7dd0', 'c85a0', 'c8390', 'c8c20', '117340', 'c8e70', 'c8cf0', 'c7580'), dispatched=()),
    '11ef70': dict(
        pin='75ff6b158aaf9140d7141f5411dc5b33e46a2397ba9d710e0a1795cdaa8277a5',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_tag_fixups.hpp"\n',
        bridge=('halo2::native_tag_fixups_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target){{switch(target){{case 0x125790u:run_125790(v);break;case 0x1caf40u:run_1caf40(v);break;case 0x21a580u:run_21a580(v);break;case 0x20d810u:run_20d810(v);break;case 0x11f060u:run_11f060(v);break;case 0x165b30u:run_165b30(v);break;case 0x1f2c70u:dispatch(v,target,0x11f02cu,false);break;}}}})'),
        direct=('125790', '1caf40', '21a580', '20d810', '11f060', '165b30'), dispatched=('1f2c70',)),
    '1262f0': dict(
        # 16E8A0 is direct in the AI frame copy, 16E770 and 1260C0 in the live
        # support copy; the others go through the dispatch: steps per pin.
        pin=('85831e46f50a4807a44e2a1039b0fd85e3fa843ebf00a5070072761ada0d33e0',
             'f29ed216048e1ab04ee6176ecd3a238b3853fab857cfe556b04e4e2a779e993a'),
        unit=('void source_run_ai_frame_tu15(', 'void source_run_physics_live_support_tu15('),
        include='#include "gameplay/animation/native_tu15_resource_acquire.hpp"\n',
        bridge=('halo2::native_resource_acquire_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
        '        [](auto &v,std::uint32_t target,std::uint32_t site){{switch(target){{case 0x126190u:run_126190(v);break;case 0x16e8a0u:run_16e8a0(v);break;case 0x1262b0u:run_1262b0(v);break;default:dispatch(v,target,site,false);break;}}}})',
                'halo2::native_resource_acquire_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
        '        [](auto &v,std::uint32_t target,std::uint32_t site){{switch(target){{case 0x126190u:run_126190(v);break;case 0x16e770u:run_16e770(v);break;case 0x1260c0u:run_1260c0(v);break;case 0x1262b0u:run_1262b0(v);break;default:dispatch(v,target,site,false);break;}}}})'),
        direct=(), dispatched=()),
    '1ae6f0': dict(
        pin='55628dd92763585c2c42862d0691e46a432080cb521a76ef94109f3fa7802b40',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_rider_check.hpp"\n',
        bridge=('halo2::native_rider_check_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('d2b00', '1e7810')),
    '1dc680': dict(
        pin='f2a1fbf9acfa2bfeefbe83dae51ac89d1e3319feddec2d344ad22a47b090812e',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_record_place.hpp"\n',
        bridge='halo2::native_record_place_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_e32c0(v);}},[](auto &v){{run_145560(v);}})',
        direct=('e32c0', '145560'), dispatched=()),
    'b95d0': dict(
        # B9460 and BA570 are direct in the live support copy and through the
        # dispatch in the death cycle copy: steps per pin.
        pin=('7ef654cd54d5fa20c345c7415e79db8de822b20083ce825786b3edf5a8a888f0',
             '4a31eec245187a249a2951d06107c8121e52fd756cd7fd0f25bb8eb36ee9c461'),
        unit=('#include "halo2/death_cycle_runtime_tu15.hpp"', 'void source_run_physics_live_support_tu15('),
        include='#include "gameplay/objects/native_tu15_object_placement.hpp"\n',
        bridge=('halo2::native_object_placement_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
        '        [](auto &v,std::uint32_t target,std::uint32_t site){{switch(target){{case 0x10e340u:run_10e340(v);break;default:dispatch(v,target,site,false);break;}}}})',
                'halo2::native_object_placement_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
        '        [](auto &v,std::uint32_t target,std::uint32_t site){{switch(target){{case 0x10e340u:run_10e340(v);break;case 0xb9460u:run_b9460(v);break;case 0xba570u:run_ba570(v);break;default:dispatch(v,target,site,false);break;}}}})'),
        direct=(), dispatched=()),
    '1157d0': dict(
        pin='238e9f3df315f25fa64617457ac72e8b4c8d89162580657fc2ad4e2fc35959de',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_unit_animation_update.hpp"\n',
        bridge=('halo2::native_unit_animation_update_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target){{switch(target){{case 0x113b70u:run_113b70(v);break;case 0x1139d0u:run_1139d0(v);break;case 0xcfb40u:run_cfb40(v);break;case 0x1e14f0u:run_1e14f0(v);break;case 0x116340u:run_116340(v);break;case 0xce230u:run_ce230(v);break;case 0x116280u:run_116280(v);break;}}}})'),
        direct=('113b70', '1139d0', 'cfb40', '1e14f0', '116340', 'ce230', '116280'), dispatched=()),
    '116280': dict(
        pin='fdf193e945d9f3c12d1bbc85b78d0f31db6200377494f2d89b9d41fe2a70555b',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_keyed_marker_pose.hpp"\n',
        bridge='halo2::native_keyed_marker_pose_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e1620(v);}},[](auto &v){{run_116060(v);}})',
        direct=('1e1620', '116060'), dispatched=()),
    '153360': dict(
        pin='bed5098ecd3e506f75bfdd23d657d4dd84335d1bd0162b949781b977a618d84f',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_player_unit_search.hpp"\n',
        bridge=('halo2::native_player_unit_search_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1538b0(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('1538b0',), dispatched=('bd080',)),
    '2e2e30': dict(
        pin='8b7462e723269692792bdc0754da9d9512474204c2eb2335ce34f6f6951d19c3',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "rendering/renderer/native_tu15_memory_copy.hpp"\n',
        bridge='halo2::native_memory_copy_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1edda0': dict(
        pin='392ff8ba200c3d5a2298f4c470e162628003e762d5515aae98f8a235524ecfb3',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_state_dispatch.hpp"\n',
        bridge=('halo2::native_state_dispatch_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](std::uint32_t p,SourcePhysicsContactImage image){{return canonical_branch(p,image);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{switch(target){{case 0x1f9720u:run_1f9720(v);break;default:dispatch(v,target,site,false);break;}}}})'),
        direct=('1f9720',), dispatched=('202ce0', '1f5230')),
    '1ceb70': dict(
        pin='7ac6c16b58360df0f1adbb923311cf9b7637cdaffde543f16986e829b3dae304',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_countdown_step.hpp"\n',
        bridge='halo2::native_countdown_step_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1ce950(v);}})',
        direct=('1ce950',), dispatched=()),
    'c3960': dict(
        pin=('6c36c871548628355ba35c6a00827313651fb8b52df988e93b8a0185d75dedfc',
             '6c36c871548628355ba35c6a00827313651fb8b52df988e93b8a0185d75dedfc',
             '6c36c871548628355ba35c6a00827313651fb8b52df988e93b8a0185d75dedfc',
             '6c36c871548628355ba35c6a00827313651fb8b52df988e93b8a0185d75dedfc'),
        unit=('#include "halo2/pstencil/native_light_shape_tu15.hpp"', '#include "halo2/death_cycle_runtime_tu15.hpp"',
              '#include "halo2/source_shadow_frame_tu15.hpp"', '#include "halo2/source_map_world_support_tu15.hpp"'),
        include='#include "pstencil/native_tu15_shape_copy.hpp"\n',
        bridge='halo2::native_shape_copy_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '140cf0': dict(
        pin=('0a366a310ffd6c6e2e91a2c7b65b00278d6f68d44e3801268a2aab1f17192856',
             '0a366a310ffd6c6e2e91a2c7b65b00278d6f68d44e3801268a2aab1f17192856',
             '7c5da7b0a78a0f706b85f1feb991be9efaff4db97b58be309e3518763adde66b',
             '0a366a310ffd6c6e2e91a2c7b65b00278d6f68d44e3801268a2aab1f17192856'),
        unit=('#include "halo2/source_renderer_transparent_storage_tu15.hpp"',
              '#include "halo2/source_scene_device_constructor_tu15.hpp"',
              '#include "halo2/source_effect_particle_draw_tu15.hpp"',
              '#include "halo2/source_final_startup_storage_tu15.hpp"'),
        include='#include "rendering/renderer/native_tu15_pool_reset.hpp"\n',
        bridge='halo2::native_pool_reset_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'c3a20': dict(
        # C39B0 goes through the dispatch in the death cycle copy: steps per pin.
        pin=('a42c8d2073278e5563e68d46afecb9c31a398a809020a5d76eb48e3129d2b1bf',
             'cecd4a24ba3826c6b084f6586f37d3da7c8651168a1e50a6b76530d9e8364731',
             'a42c8d2073278e5563e68d46afecb9c31a398a809020a5d76eb48e3129d2b1bf'),
        unit=('#include "halo2/pstencil/native_light_shape_tu15.hpp"',
              '#include "halo2/death_cycle_runtime_tu15.hpp"',
              '#include "halo2/source_map_world_support_tu15.hpp"'),
        include='#include "pstencil/native_tu15_light_shape_source.hpp"\n',
        bridge=('halo2::native_light_shape_source_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
        '        [](auto &v,std::uint32_t target,std::uint32_t site){{switch(target){{case 0xc3960u:run_c3960(v);break;case 0xc39b0u:run_c39b0(v);break;default:dispatch(v,target,site,false);break;}}}})',
                'halo2::native_light_shape_source_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
        '        [](auto &v,std::uint32_t target,std::uint32_t site){{switch(target){{case 0xc3960u:run_c3960(v);break;default:dispatch(v,target,site,false);break;}}}})',
                'halo2::native_light_shape_source_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
        '        [](auto &v,std::uint32_t target,std::uint32_t site){{switch(target){{case 0xc3960u:run_c3960(v);break;case 0xc39b0u:run_c39b0(v);break;default:dispatch(v,target,site,false);break;}}}})'),
        direct=(), dispatched=()),
    'be2b0': dict(
        pin='dcc626e012982d48718630cacf871dbc28df02b5a8ec18ea9a10730232022876',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_object_allocate.hpp"\n',
        bridge=('halo2::native_object_allocate_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_16e6a0(v);}},\n'
                '        [](auto &v){{run_1910a0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('16e6a0', '1910a0'), dispatched=('16e8a0',)),
    '15cab0': dict(
        pin='74a78330f0e7019f79cc70b8552cb252c2a5f763141762d6e79df5bf5622148f',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/multiplayer/native_tu15_kind_slot.hpp"\n',
        bridge='halo2::native_kind_slot_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '118120': dict(
        pin='e615e584868fe1b7d0cf6bbed8bb3000ccd4ec49b6a1944f5d9cf1ad8a1d1339',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "rendering/native_tu15_cluster_pairs.hpp"\n',
        bridge='halo2::native_cluster_pairs_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1718c0': dict(
        pin='01a24bdbf72f76ab7d842a2ffd7a562acd221072415dfa789a72a651549610e2',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "audio/native_tu15_sound_readiness.hpp"\n',
        bridge='halo2::native_sound_readiness_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_21f260(v);}})',
        direct=('21f260',), dispatched=()),
    '4b150': dict(
        pin='fce2fa8c9ec5bb3e0b49bd85c93634e18aa0ccb0d429fa5d11ed0099efba819e',
        unit='#include "halo2/source_shadow_frame_tu15.hpp"',
        include='#include "rendering/shadows/native_tu15_record_sort.hpp"\n',
        bridge='halo2::native_record_sort_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'c3820': dict(
        # C3A20 and C3AB0 are direct in the map world support copy, dispatched in the light portal's: steps per pin.
        pin=('2b69ba5d9a4242ab380822eccb418e93e4f4e4c96f0940dad6302aa0ef7fcc5f',
             'eb9152c377efc2714d0eeac5619c098fe57d052095835c58420dff195c994b38'),
        unit=('#include "halo2/source_map_world_support_tu15.hpp"',
              '#include "halo2/pstencil/native_light_portal_tu15.hpp"'),
        include='#include "pstencil/native_tu15_light_record.hpp"\n',
        bridge=('halo2::native_light_record_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
        '        [](auto &v,std::uint32_t target,std::uint32_t site){{switch(target){{case 0xc3a20u:run_c3a20(v);break;case 0xc3ab0u:run_c3ab0(v);break;default:dispatch(v,target,site,false);break;}}}})',
                'halo2::native_light_record_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
        '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    '15b740': dict(
        pin='343e793a38536b1432dddcd11735bea8925afbf4c6f11058d94945df29732ef5',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_round_end.hpp"\n',
        bridge=('halo2::native_round_end_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_15b4d0(v);}},[](auto &v){{run_15b620(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('15b4d0', '15b620'), dispatched=('161530',)),
    '1617a0': dict(
        pin='9409fc131d2a5578f7284d48d10389f7465e3ecab0522c473f439bb8dea8bc52',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/multiplayer/native_tu15_player_exclusion.hpp"\n',
        bridge=('halo2::native_player_exclusion_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1a8e50(v);}},\n'
                '        [](auto &v){{run_1601b0(v);}},[](auto &v){{run_160360(v);}})'),
        direct=('1a8e50', '1601b0', '160360'), dispatched=()),
    '1a6830': dict(
        pin='2280d6906d28c9f6a0674972fd77ff778b5a1e3c161753687645813641593563',
        unit='#include "halo2/source_teleporter_tu15.hpp"',
        include='#include "gameplay/world/native_tu15_teleporter_update.hpp"\n',
        bridge=('halo2::native_teleporter_update_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},[](auto &v){{run_1a6930(v);}},[](auto &v){{run_1a6a40(v);}})'),
        direct=('1a6930', '1a6a40'), dispatched=('1a8f10',)),
    'e53e0': dict(
        pin='3322da8c9ffe9ac1d1214e979e4bb1e128d828873b86a7fa3c2bc9dcdafc1215',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_motion_names.hpp"\n',
        bridge=('halo2::native_motion_names_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1d3bd0(v);}},\n'
                '        [](std::uint32_t p,SourcePhysicsContactImage image){{return canonical_branch(p,image);}})'),
        direct=('1d3bd0',), dispatched=()),
    'e52d0': dict(
        pin='2b91d1357fe0c2afdad049b9d83a6acc9c1da29ee376972c8d6f573e73e9dd28',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_biped_recovery.hpp"\n',
        bridge=('halo2::native_biped_recovery_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_e7e80(v);}},\n'
                '        [](auto &v){{run_111b10(v);}},[](auto &v){{run_c1a70(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('e7e80', '111b10', 'c1a70'), dispatched=('2031f0',)),
    '113b70': dict(
        pin='8c1da75a4b49ccb1f6fbdc334550b806219d0801e65375c0baa6a0d46ddc12a9',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_seated_blend.hpp"\n',
        bridge='halo2::native_seated_blend_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_c9550(v);}},[](auto &v){{run_113c70(v);}})',
        direct=('c9550', '113c70'), dispatched=()),
    '14fdc0': dict(
        pin='56e484d9fdca157b43516021dcd6bd1c79baed382a75078bac4a6fd978725e3a',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/players/native_tu15_player_activation.hpp"\n',
        bridge=('halo2::native_player_activation_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_69ad0(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},[](auto &v){{run_b7e50(v);}})'),
        direct=('69ad0', 'b7e50'), dispatched=()),
    '17cd10': dict(
        pin=('0b7e47c1beee53a3ae430663b7de69e1a1dcc85cf47557fc050189660543e82f',
             '0b7e47c1beee53a3ae430663b7de69e1a1dcc85cf47557fc050189660543e82f'),
        unit=('#include "halo2/source_shadow_frame_tu15.hpp"', '#include "halo2/source_lens_flare_tu15.hpp"'),
        include='#include "rendering/effects/native_tu15_effect_part_places.hpp"\n',
        bridge='halo2::native_effect_part_places_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=('17dd50', '145560', '159690')),
    '1ce950': dict(
        pin='1685cb49d88a1f4376ac493acf0c7a4018e2329f9148b19c721080029e4b3141',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/animation/native_tu15_channel_advance.hpp"\n',
        bridge=('halo2::native_channel_advance_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1cee60(v);}},\n'
                '        [](auto &v){{run_1e3150(v);}},[](auto &v){{run_1cf700(v);}},[](auto &v){{run_1d3660(v);}},[](auto &v){{run_b8780(v);}},\n'
                '        [](auto &v){{run_1ce810(v);}})'),
        direct=('1cee60', '1e3150', '1cf700', '1d3660', 'b8780', '1ce810'), dispatched=()),
    'ba630': dict(
        pin='8b90ee3b8326ddf7762c648332e676dc24f09808cc24d78e06fcaa4c731e3103',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_object_connect.hpp"\n',
        bridge=('halo2::native_object_connect_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target){{switch(target){{case 0x11e600u:run_11e600(v);break;case 0xba910u:run_ba910(v);break;case 0x1d2f10u:run_1d2f10(v);break;case 0xc0f60u:run_c0f60(v);break;case 0xb8d90u:run_b8d90(v);break;case 0xb92c0u:run_b92c0(v);break;case 0xb1870u:run_b1870(v);break;case 0xba570u:run_ba570(v);break;case 0xb9330u:run_b9330(v);break;case 0x1cc290u:run_1cc290(v);break;default:throw std::logic_error("Unbound object connect child");}}}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=('11e600', 'ba910', '1d2f10', 'c0f60', 'b8d90', 'b92c0', 'b1870', 'ba570', 'b9330', '1cc290'), dispatched=('1496f0', '282c10')),
    '1cec60': dict(
        pin=('8448626164e10b27f0a1b5114a188680bb43a1e29523fe4cf8c8d3e9989bf97c',
             '64d0434e065aa53da50c5799c3363c0418982495ea90afa812831685ebd9f408'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_actor_tu15.hpp"'),
        include='#include "gameplay/animation/native_tu15_event_window.hpp"\n',
        bridge='halo2::native_event_window_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e3150(v);}})',
        direct=('1e3150',), dispatched=()),
    '2e4350': dict(
        pin='c64f02c7d022d170042aee0098b3ee2a948a098737764e7a1d8e6072eab8177d',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_rotation_quaternion.hpp"\n',
        bridge='halo2::native_rotation_quaternion_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1ee3d0': dict(
        pin='f50361699048fd2a8450558ed8e39d9f1485db81638afed457226a95691f9c9b',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_unit_motion_parameters.hpp"\n',
        bridge='halo2::native_unit_motion_parameters_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'e05d0': dict(
        pin='52e5abc53009c5b0bb7fa499bfcd40f81efa0523b005203c0c838fbcb81b1343',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_camera_basis.hpp"\n',
        bridge='halo2::native_camera_basis_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'e06e0': dict(
        pin='65bbabdc7aaa2535385cd3356e31fef98b8227391e8f4faa68b3b415d0eb4154',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_aim_offset.hpp"\n',
        bridge=('halo2::native_aim_offset_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_111f00(v);}},\n'
                '        [](auto &v){{run_bad60(v);}},[](auto &v){{run_cdf30(v);}},[](auto &v){{run_e05d0(v);}})'),
        direct=('111f00', 'bad60', 'cdf30', 'e05d0'), dispatched=()),
    '1cf700': dict(
        pin='28670bbb1c747388edcac7fe060e7422e3dbf7f43bcab7ae18e074951dd465ef',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/animation/native_tu15_channel_events.hpp"\n',
        bridge='halo2::native_channel_events_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=()),
    '15b4d0': dict(
        pin='6f8aeeac74f16d01e14bb6a6f872ff1f2bcf43ae031fbd0441c73ce4c9c56f6c',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_side_count.hpp"\n',
        bridge='halo2::native_side_count_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1459b0': dict(
        pin='6d0126d2ec3e4b70d695a3ba966f1458851c3efbbf3ee575431feeb464015edc',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "rendering/renderer/native_tu15_rotation_product.hpp"\n',
        bridge='halo2::native_rotation_product_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '15be80': dict(
        pin='bdb17a595fd28fb426726eed55a1d50f27f1b061578b80cbe29cf643f557136a',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_score_lead.hpp"\n',
        bridge='halo2::native_score_lead_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=('15b210', '1648e0', '1a8e50')),
    '1181f0': dict(
        pin='b259c80c51095ee0c24737ace809ccf9b385d799dece79d382086c296faedfe5',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "rendering/renderer/native_tu15_object_lights.hpp"\n',
        bridge=('halo2::native_object_lights_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](auto &v){{run_1d9080(v);}},[](auto &v){{run_1d9100(v);}},[](auto &v){{run_1d9190(v);}})'),
        direct=('1d9080', '1d9100', '1d9190'), dispatched=('212c70', '1d8ed0')),
    'c8e70': dict(
        pin='9b61819481dc4910e63532b4309913f58f475ea8eff2cf864c87078487da865d',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_object_team.hpp"\n',
        bridge='halo2::native_object_team_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_d2b60(v);}},[](auto &v){{run_d2bd0(v);}})',
        direct=('d2b60', 'd2bd0'), dispatched=()),
    'c8390': dict(
        pin='eb7d9caa8229b4e97f69e182e6a36f73f521146010040cef9095943b0f06c717',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_channel_fades.hpp"\n',
        bridge='halo2::native_channel_fades_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_d3150(v);}},[](auto &v){{run_d05a0(v);}},[](auto &v){{run_e8e10(v);}})',
        direct=('d3150', 'd05a0', 'e8e10'), dispatched=()),
    'd35c0': dict(
        pin='893ebbf9d266fceed16cfe32fb210fcfac2b00d7f68d49f0cf08fa44c11aa0e9',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_seat_transform.hpp"\n',
        bridge='halo2::native_seat_transform_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_bad60(v);}},[](auto &v){{run_bbe00(v);}})',
        direct=('bad60', 'bbe00'), dispatched=()),
    '1601b0': dict(
        pin='d602d1e3e9c45f8c222dfc104a7b76748c524c8adacf72597e7bffae4bd3b63b',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/players/native_tu15_respawn_wait.hpp"\n',
        bridge='halo2::native_respawn_wait_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1a8e50(v);}},[](auto &v){{run_160570(v);}})',
        direct=('1a8e50', '160570'), dispatched=()),
    '160360': dict(
        pin='91824f61471f47195dcc51ae8948b1a0dd7071125339e16d1ee119b725af1028',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/players/native_tu15_team_respawn_wait.hpp"\n',
        bridge='halo2::native_team_respawn_wait_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1a8e50(v);}},[](auto &v){{run_160570(v);}},[](auto &v){{run_1601b0(v);}},[](auto &v){{run_164890(v);}})',
        direct=('1a8e50', '160570', '1601b0', '164890'), dispatched=()),
    '160570': dict(
        pin=('2beda2d6975389ed4f96aaa13563a44417cb16c675193650ebb7ae424e383dd8',
             'be8caab3037bca992ba2e79dffbb810f91b432a5389aa9c774aa1a39170eb205'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/death_cycle_runtime_tu15.hpp"'),
        include='#include "gameplay/players/native_tu15_lives_spent.hpp"\n',
        bridge='halo2::native_lives_spent_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '164890': dict(
        pin='eb56ce3f78c1b8be0a138be61f90244642bf519153f88285b79745f9f683ac0c',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/multiplayer/native_tu15_team_active.hpp"\n',
        bridge='halo2::native_team_active_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1d5970': dict(
        pin='69bbf45761f2a307164468fe1e55513d25fa4df9f5f486662869baf2963b532a',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_pose_blend.hpp"\n',
        bridge='halo2::native_pose_blend_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1cefb0(v);}},[](auto &v){{run_1d61b0(v);}})',
        direct=('1cefb0', '1d61b0'), dispatched=()),
    'cdcf0': dict(
        pin='18c4063c2de6c48d9469db6d27fc7b59ac2b0dd8dcddbc8075c02c73a1028ac4',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_seated_vector.hpp"\n',
        bridge='halo2::native_seated_vector_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_bbff0(v);}},[](auto &v){{run_30bd0(v);}},[](auto &v){{run_144210(v);}})',
        direct=('bbff0', '30bd0', '144210'), dispatched=()),
    '69ad0': dict(
        pin='7201facc3db257c06834d8bbb542fcab3c9e7f2f4a637f7a15c582feac802d91',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "runtime/native_tu15_machine_address.hpp"\n',
        bridge='halo2::native_machine_address_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_6c3f0(v);}})',
        direct=('6c3f0',), dispatched=()),
    '1afa90': dict(
        pin='dc12822b25148c84c54daf039cb754cf4f861953a8469bcb6e28e2741145c21a',
        unit=('#include "halo2/source_ai_actor_tu15.hpp"', '#include "halo2/source_final_startup_storage_tu15.hpp"'),
        include='#include "campaign/ai/native_tu15_list_slots.hpp"\n',
        bridge=('halo2::native_list_slots_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](std::uint32_t p,SourcePhysicsContactImage image){{return canonical_branch(p,image);}})'),
        direct=(), dispatched=()),
    'e71a0': dict(
        pin='4b5918ba40a135ad0b91974bd4b0294c6a05e8eaa8b98e96ee10df0cfd3d72c4',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_unit_reaction.hpp"\n',
        bridge='halo2::native_unit_reaction_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_e72e0(v);}})',
        direct=('e72e0',), dispatched=()),
    '144910': dict(
        pin='5096f893f3b7786ed96ff56d1c08046e41956b4c133b735c5a83ddeefd10c06c',
        unit=('#include "halo2/source_ai_frame_tu15.hpp"', '#include "halo2/source_scene_device_constructor_tu15.hpp"'),
        include='#include "rendering/renderer/native_tu15_quaternion_matrix.hpp"\n',
        bridge='halo2::native_quaternion_matrix_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1d45a0': dict(
        pin=('e834d2b09be373459554dc50740a148bd25072cd12042da6b4f239433d395cc7',
             'a36ad46402474b897dc218822afb06479131a39f49e1cf744d629dbf74483e5e'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_actor_tu15.hpp"', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "gameplay/animation/native_tu15_animation_names.hpp"\n',
        bridge='halo2::native_animation_names_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1e5810(v);}})',
        direct=('1e5810',), dispatched=()),
    '191130': dict(
        pin='f4c75d386281425e19b6e023068fbad818ae92525f208eb376b004c07a1937ac',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "runtime/native_tu15_pool_resize.hpp"\n',
        bridge='halo2::native_pool_resize_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_33b590(v);}},[](auto &v){{run_1910a0(v);}},[](auto &v){{run_191220(v);}})',
        direct=('33b590', '1910a0', '191220'), dispatched=()),
    'be3b0': dict(
        pin='5e7b5dbea847e2aebf400488d5c0c476d64245758caa5d31acd426c7ab7fc2f8',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/objects/native_tu15_object_extension.hpp"\n',
        bridge='halo2::native_object_extension_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_191130(v);}})',
        direct=('191130',), dispatched=()),
    '175380': dict(
        pin='23bf03d1803a0c25455382efe2fa02255d1e6fe454e8f0442af05472417c6c3d',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_placement_repair.hpp"\n',
        bridge='halo2::native_placement_repair_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_175300(v);}},[](auto &v){{run_b1720(v);}},[](auto &v){{run_b16c0(v);}},[](auto &v){{run_1722c0(v);}})',
        direct=('175300', 'b1720', 'b16c0', '1722c0'), dispatched=()),
    '1af970': dict(
        pin='7f069fdbdc7bd85bc33513a1983f9bc5ab8f7d47bfb82f934bbafed7564bde96',
        unit=('#include "halo2/source_ai_actor_tu15.hpp"', '#include "halo2/source_final_startup_storage_tu15.hpp"'),
        include='#include "campaign/ai/native_tu15_behavior_tables.hpp"\n',
        bridge='halo2::native_behavior_tables_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1afa90(v);}})',
        direct=('1afa90',), dispatched=()),
    '1d39c0': dict(
        pin='f7facb5899756d42017ee71caa126e6a527d31b515fe74499bcb5a5b0f2856b9',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/animation/native_tu15_animation_step.hpp"\n',
        bridge=('halo2::native_animation_step_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1ceb70(v);}},\n'
                '        [](auto &v){{run_1e3150(v);}},[](auto &v){{run_1d38a0(v);}},[](auto &v){{run_1d3930(v);}},[](auto &v){{run_1ce950(v);}})'),
        direct=('1ceb70', '1e3150', '1d38a0', '1d3930', '1ce950'), dispatched=()),
    '111b10': dict(
        pin=('44044a4d7a926c56bdac85aca960e7449c05b24df36e67ba1803b09d7dde2457',
             '93e66ffc95b8edaf82439ee8b9f946fff4b3fcce415742868f7fdc3cd52e4a6a'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_actor_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_animation_change.hpp"\n',
        bridge=('halo2::native_animation_change_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1d5320(v);}},\n'
                '        [](auto &v){{run_115e20(v);}},[](auto &v){{run_1d4350(v);}},[](auto &v){{run_115e90(v);}},[](auto &v){{run_bc380(v);}},\n'
                '        [](auto &v){{run_115af0(v);}})'),
        direct=('1d5320', '115e20', '1d4350', '115e90', 'bc380', '115af0'), dispatched=()),
    '16bda0': dict(
        pin='8de2fa088687ea14920df59993727d98943151541823fdef9735dc30094a322c',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_point_blocked.hpp"\n',
        bridge=('halo2::native_point_blocked_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_14cdb0(v);}},\n'
                '        [](auto &v){{run_16bbc0(v);}},[](auto &v){{run_16e360(v);}},[](auto &v){{run_ba970(v);}},[](auto &v){{run_16bc80(v);}},\n'
                '        [](auto &v){{run_ba9e0(v);}})'),
        direct=('14cdb0', '16bbc0', '16e360', 'ba970', '16bc80', 'ba9e0'), dispatched=()),
    '16bbc0': dict(
        pin=('d7a5696ba17196924dd8a4cd87223a85dd6bfcd56524d9d68d4f00722137b842',
             '8febc517ab421d9bcd59fa8f674a686904d8cbefff7d71ec2327a98631ff4cc9'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/death_cycle_runtime_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_instance_blocks.hpp"\n',
        bridge='halo2::native_instance_blocks_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_16b810(v);}},[](auto &v){{run_145200(v);}},[](auto &v){{run_14cdb0(v);}})',
        direct=('16b810', '145200', '14cdb0'), dispatched=()),
    '1cebd0': dict(
        pin='8281dd4e4bbb796077fd61aa04a11a7fc9a3476124e6c608d4d68c299008d9e8',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "gameplay/animation/native_tu15_channel_seek.hpp"\n',
        bridge='halo2::native_channel_seek_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=('1e3150', '1ce950')),
    '185820': dict(
        pin='7845be5d2957555fa9b01dea4acde4a83089dfcca1e786b0315dcc1b5f82810e',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_ray_hit_accept.hpp"\n',
        bridge='halo2::native_ray_hit_accept_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_186c10(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('186c10',), dispatched=('1d7530',)),
    '13b610': dict(
        pin='12d05e6fa2fa36f8183be5b57fe68957da1aff3e299adb7df2016e8c5890873c',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "rendering/renderer/native_tu15_cluster_visibility.hpp"\n',
        bridge='halo2::native_cluster_visibility_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1532a0(v);}},[](auto &v){{run_140ec0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('1532a0', '140ec0'), dispatched=('16fd40',)),
    '16e510': dict(
        pin='d9326e6c89fc9b5702dbc6d2b55148c350856be28f5a473b2b00eccb162fb5ef',
        unit='#include "halo2/source_campaign_runtime_tu15.hpp"',
        include='#include "runtime/native_tu15_data_array_restore.hpp"\n',
        bridge='halo2::native_data_array_restore_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=('32a4e0',)),
    '13dc44': dict(
        pin='f2780f3a23e44c3721cbe7253ba6192ab30ee67f23cee68a4c3edb503cad8399',
        unit='#include "halo2/source_weapon_hud_tu15.hpp"',
        include='#include "gameplay/weapons/native_tu15_hud_weapon_flags.hpp"\n',
        bridge='halo2::native_hud_weapon_flags_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_10b3d0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('10b3d0',), dispatched=('bcdf0', '1e7810')),
    '1d8b90': dict(
        pin='4d229e9c0aa7f4f1f548f134dea6cd043e55ecc4eedeb55ed23a1e93a8a8383a',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "rendering/renderer/native_tu15_body_matrix.hpp"\n',
        bridge='halo2::native_body_matrix_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_2e4650(v);}})',
        direct=('2e4650',), dispatched=()),
    '1d9190': dict(
        pin='afca38051ddf86ee1852367385c33ec6929cd3825458f0c3393e946efeee698a',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "rendering/renderer/native_tu15_body_place.hpp"\n',
        bridge='halo2::native_body_place_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_2e4650(v);}},[](auto &v){{run_2e58f0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('2e4650', '2e58f0'), dispatched=('2e4470', '2e4350')),
    '2e9140': dict(
        pin='a5d50744274aacc02a8580a39defe1fb466f85e0758803b8890b5704438f80e1',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_shape_ray_hit.hpp"\n',
        bridge='halo2::native_shape_ray_hit_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=()),
    '315190': dict(
        pin='b302393d9ef3b0eb0c875da140476482956275b1141c880d5e0d40d164af4e94',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_body_list_ray.hpp"\n',
        bridge='halo2::native_body_list_ray_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=()),
    '2f20f0': dict(
        pin='61cfa5329d29fa16454d9e287284c64797faedbe3c75ae1a6d6e523db5ef9dc6',
        unit='#include "halo2/source_map_world_support_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_child_agents.hpp"\n',
        bridge='halo2::native_child_agents_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=('2e8fc0', '2e8cf0', '2e12a0')),
    '1ddea0': dict(
        pin='b6f661f27b404585da04f3c97e6cf917df7b9885e438837835c746de502500aa',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_rigid_body_build.hpp"\n',
        bridge='halo2::native_rigid_body_build_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1de170(v);}},[](auto &v){{run_1dec80(v);}},[](auto &v){{run_1df380(v);}},[](auto &v){{run_1df5b0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('1de170', '1dec80', '1df380', '1df5b0'), dispatched=('283450',)),
    '168ca1': dict(
        pin='9242405f09a81c6b3919476a066cc1fbd1567613d6e5f74b6cf1254c4fdc4fdd',
        unit='#include "halo2/source_sandbox_special_tu15.hpp"',
        include='#include "gameplay/players/native_tu15_player_weapon_sync.hpp"\n',
        bridge='halo2::native_player_weapon_sync_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_150990(v);}},[](auto &v){{v.required(0x168d55u,0x169c46u,false);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('150990',), dispatched=('16b16a', 'ce230', '16ad45')),
    '164ef0': dict(
        pin='f3ee5136a759b323cc50d2dde73e1fa657186ecac42bacc9cbc646c9e32d5b76',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_round_timer.hpp"\n',
        bridge='halo2::native_round_timer_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=('b1e80', '1a8740')),
    '164750': dict(
        pin='451206dd86e5e490e2dca826c4af32a574a2bb1daded3e564ce96f474eebc566',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_players_facing.hpp"\n',
        bridge='halo2::native_players_facing_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=('bbe00', '1a8f10', '11f7e0')),
    '17bd40': dict(
        pin='c19cef60195474385ca7b4f649f080da8db51cc1df213f1d0a75b33a6c665413',
        unit='#include "halo2/death_cycle_runtime_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_effect_element.hpp"\n',
        bridge='halo2::native_effect_element_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_17e100(v);}},[](auto &v){{run_17dce0(v);}},[](auto &v){{run_17aeb0(v);}},[](auto &v){{run_176e70(v);}},[](auto &v){{run_177f10(v);}},[](auto &v){{run_17bb10(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('17e100', '17dce0', '17aeb0', '176e70', '177f10', '17bb10'), dispatched=('158290',)),
    '13b6f0': dict(
        pin='2b5347ba1311dc1afa7d28adc254e708307df7a37d44efa2f7995769d5e5a7a0',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "rendering/renderer/native_tu15_cluster_visibility_update.hpp"\n',
        bridge='halo2::native_cluster_visibility_update_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_13b610(v);}},[](auto &v){{run_1d1e00(v);}},[](auto &v){{run_140ec0(v);}},[](auto &v){{run_13b590(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('13b610', '1d1e00', '140ec0', '13b590'), dispatched=()),
    '154270': dict(
        pin='90b0e52adadb64dd2f836ecb8939702c67d28c8dcc7265250a87632c61003245',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_object_reach.hpp"\n',
        bridge='halo2::native_object_reach_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_153e10(v);}},[](auto &v){{run_c6920(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('153e10', 'c6920'), dispatched=('cfc90',)),
    '15e870': dict(
        pin='ba6494a81900768af122f4fbc2643e717d957f1fd94ab94819af7a0d4885da35',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_round_countdown.hpp"\n',
        bridge='halo2::native_round_countdown_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_164ef0(v);}},[](auto &v){{run_15dd50(v);}},[](auto &v){{v.required(0x15e9dfu,0x15ddc0u,false);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('164ef0', '15dd50'), dispatched=('b1a10', '1a87a0')),
    'e79c0': dict(
        pin='3886cc0bdbd063ed4401b12d1f8dc038148237a982271a2e64df13a93649eca9',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_weapon_trade.hpp"\n',
        bridge='halo2::native_weapon_trade_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_cd440(v);}},[](auto &v){{run_cdcc0(v);}},[](auto &v){{run_11b560(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('cd440', 'cdcc0', '11b560'), dispatched=('30bd0', 'cfc90')),
    '1ae7f0': dict(
        pin='acc4068d10b4ad5009ada53484e9d96d38a1f04a640175145d413d0dafd2eb8e',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_action_candidates.hpp"\n',
        bridge='halo2::native_action_candidates_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_1aea30(v);}},[](auto &v){{run_1aece0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('1aea30', '1aece0'), dispatched=('bad60',)),
    '1adc90': dict(
        pin='e29f882e4b1abb0edc3ccfb2011ee862fc07939c7925df333b8d813806bd1664',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_weapon_ranges.hpp"\n',
        bridge='halo2::native_weapon_ranges_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_106160(v);}},[](auto &v){{run_1062a0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('106160', '1062a0'), dispatched=('106230',)),
    '1770c0': dict(
        pin='bb4f77bb2e3802c0066ae58acfd0d13734855aa79b1445644ee1933a004dbdba',
        unit='#include "halo2/source_effect_particle_draw_tu15.hpp"',
        include='#include "rendering/effects/native_tu15_effect_elements_draw.hpp"\n',
        bridge='halo2::native_effect_elements_draw_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_17de60(v);}},[](auto &v){{run_1772c0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('17de60', '1772c0'), dispatched=('16ea60', '178070', '3eb10', '158290')),
    '245060': dict(
        pin='34392aedcc1bedf8a0c5d34de249ea5f486634e9a52a852da9e208b59df0b5e8',
        unit='#include "halo2/source_vehicle_director_tu15.hpp"',
        include='#include "gameplay/vehicles/native_tu15_director_aim.hpp"\n',
        bridge='halo2::native_director_aim_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=('11fa50', 'cd4a0', 'bc200', 'bcdf0', 'bad60', 'bbe00', '144bf0', '175380')),
    'c7dd0': dict(
        pin='28943d59857a772aedf2698b322ab43baa7ae7497dbdb3c8ee0f072f2d905f46',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_unit_weapon_controls.hpp"\n',
        bridge='halo2::native_unit_weapon_controls_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{switch(target){{case 0xe8e10u:run_e8e10(v);break;case 0x116480u:run_116480(v);break;case 0x113190u:run_113190(v);break;case 0xce3a0u:run_ce3a0(v);break;case 0xcaaa0u:run_caaa0(v);break;case 0x105500u:run_105500(v);break;case 0xd3a80u:run_d3a80(v);break;default:throw std::logic_error("Unbound unit weapon controls child");}}}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('e8e10', '116480', '113190', 'ce3a0', 'caaa0', '105500', 'd3a80'), dispatched=('1164d0', '116400', '116440', '18c000')),
    '113190': dict(
        pin=('1131a28fb1602c3817705fcc836861eadb79cfcc20736b9e61854d088fb55ad4', '72c9336bdbf42b07df6725ea45bd859431d51ad84f090c2347a82c3dc8854af8'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_animation_busy.hpp"\n',
        bridge='halo2::native_animation_busy_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_c9590(v);}})',
        direct=('c9590',), dispatched=()),
    '1f8710': dict(
        pin='80a25c6ccaee30c49ec46d8bb87137dd770814a00403777ec9b1c82e24cfcb9d',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_platform_carry.hpp"\n',
        bridge='halo2::native_platform_carry_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{switch(target){{case 0x1fa110u:run_1fa110(v);break;case 0x1fa1a0u:run_1fa1a0(v);break;case 0x1f8b30u:run_1f8b30(v);break;default:throw std::logic_error("Unbound platform carry child");}}}})',
        direct=('1fa110', '1fa1a0', '1f8b30'), dispatched=()),
    '1618b0': dict(
        pin='1264b1bcb3697235c7e031fbc6b8246ed98e979f763f2bcef0500cf94b2e54c8',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_respawn_status.hpp"\n',
        bridge='halo2::native_respawn_status_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{if(target!=0x15be80u)throw std::logic_error("Unbound respawn status child");run_15be80(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('15be80',), dispatched=('160570',)),
    '17c370': dict(
        pin=('c47474912a00447de3324272014901b69396a265deb7b0c57ba94261aabaf59c', 'd7493907fa7c4e978c8a11c528a33521fdaacd5ebae84700d2679ad16c8da003'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/death_cycle_runtime_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_effect_follow.hpp"\n',
        bridge='halo2::native_effect_follow_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=()),
    'cd4a0': dict(
        pin=('e318a21af0a147fcd558af448fb786d7db53e09c916921bfc608f1e62c2fbca9', 'aef9ce4ce0265b71f51da61f8c46c02c7c4a7233b5d45309a2e3e9a8bd6d0264', 'a432ffbefbc3c18de98006c7c28c8ca7da081f78afada0c6da6b23153c48403f'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"', '#include "halo2/source_sandbox_special_tu15.hpp"'),
        include='#include "gameplay/units/native_tu15_unit_camera_position.hpp"\n',
        bridge='halo2::native_unit_camera_position_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=()),
    '17ead0': dict(
        pin='80ef6fca25f8719a1525823f798747fd71e0e57b9d8e0af113f8e630d0e1b36b',
        unit='#include "halo2/source_contrail_geometry_tu15.hpp"',
        include='#include "rendering/effects/native_tu15_contrails_update.hpp"\n',
        bridge='halo2::native_contrails_update_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{if(target!=0x17f3d0u)throw std::logic_error("Unbound contrails update child");run_17f3d0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('17f3d0',), dispatched=('16ea60', 'bcb70', '17ef70', '17eed0', '17ee10', '17f710', '16e8a0')),
    '14d790': dict(
        pin=('ad0f981f736f4e00865410020e6a947c14e3eb8575d2fb32a3455102c8f521d2', '40693a428aa47cb054967b1ed33b68874fce345390e02cb8ec97523896a47152'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_map_world_support_tu15.hpp"', '#include "halo2/source_model_render_tu15.hpp"', '#include "halo2/source_combat_action_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_point_surface.hpp"\n',
        bridge='halo2::native_point_surface_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=()),
    '1de8e0': dict(
        pin='18394007604f209650774e5e47a42d62c47a1af94f0add2e79fb503ac9be3b25',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_group_flood.hpp"\n',
        bridge='halo2::native_group_flood_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{if(target==0xdda90u)run_dda90(v);else if(target==0xdda10u)run_dda10(v);else run_1de8e0(v);}})',
        direct=('dda90', 'dda10', '1de8e0'), dispatched=()),
    '1538b0': dict(
        pin='9709e35a615ff776497974281e00e53385959911df828f06e6d0872413c6eafd',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_item_pickup.hpp"\n',
        bridge='halo2::native_item_pickup_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{switch(target){{case 0x104d30u:run_104d30(v);break;case 0x197e9du:run_197e9d(v);break;case 0xcf080u:run_cf080(v);break;case 0x197e11u:run_197e11(v);break;case 0x161320u:run_161320(v);break;case 0xcf060u:run_cf060(v);break;case 0x13b360u:run_13b360(v);break;default:run_153850(v);break;}}}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('104d30', '197e9d', 'cf080', '197e11', '161320', 'cf060', '13b360', '153850'),
        dispatched=('c6920', 'bcdf0', '1549d0', 'cfc90', 'b3140', 'e8e50')),
    '1700e0': dict(
        pin='60104963fe3780a1a4ffb1116e2d410f67087d8e86b41f25d00784239a5f0b43',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_permutation_choice.hpp"\n',
        bridge='halo2::native_permutation_choice_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{if(target==0x170930u)run_170930(v);else run_170b40(v);}})',
        direct=('170930', '170b40'), dispatched=()),
    '14d410': dict(
        pin=('3dac428fbb1027e4d60ae9b3862c7e72b28011574d20b037f950f5b7103cf2b7', '3e56ff0f4007f18a7c4b5b1768ebde97966c8b3f5104b5df013a3e560566b403'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_model_render_tu15.hpp"', '#include "halo2/source_combat_action_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_object_point_surface.hpp"\n',
        bridge='halo2::native_object_point_surface_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=(), dispatched=()),
    '1115d0': dict(
        pin='4f97bea12c27e069a06b4e713c631d924b25bc231bbf2147d516322d971271fb',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_unit_state_change.hpp"\n',
        bridge='halo2::native_unit_state_change_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{if(target==0x111d10u)run_111d10(v);else run_1d6430(v);}})',
        direct=('111d10', '1d6430'), dispatched=()),
    '1adf60': dict(
        pin='aef9b0753a14a15a7eb12aa48d0eb5fff29e3b67dbb9ea5c65fd102e14a9e791',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_aim_choice.hpp"\n',
        bridge='halo2::native_aim_choice_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{if(target==0x1ae090u)run_1ae090(v);else if(target==0x32b560u)run_32b560(v);else run_1af3e0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('1ae090', '32b560', '1af3e0'), dispatched=('32a660', '14cdb0')),
    '1139d0': dict(
        pin='34cca14f44f52fc0c442908db00696f30ed785ccccbcba21e6a6f1d0e9e46f29',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "gameplay/units/native_tu15_unit_aim_enabled.hpp"\n',
        bridge='halo2::native_unit_aim_enabled_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{if(target==0xc9550u)run_c9550(v);else run_113c70(v);}})',
        direct=('c9550', '113c70'), dispatched=()),
    '15d890': dict(
        pin='0c014c91309c66b46211f4952e4cbd1869318b7ba649a5a9629a7f4576a9015d',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_item_points.hpp"\n',
        bridge='halo2::native_item_points_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{if(target==0x15ba10u)run_15ba10(v);else if(target==0x15d670u)run_15d670(v);else run_161ae0(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('15ba10', '15d670', '161ae0'), dispatched=('15cab0', '15d4f0', 'd9110')),
    'd2bd0': dict(
        pin='1ec0d98e5d8fc2c8f4b5dd1a92df6e8ce3877feb1407452b3669cd11951855ea',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_rider_iterator.hpp"\n',
        bridge='halo2::native_rider_iterator_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '4a7b0': dict(
        pin='e6ec12a15c7a4ecf23a389ffee5669c9691773691c6cc2b2cbe0ecc713ff85b3',
        unit='#include "halo2/source_shadow_frame_tu15.hpp"',
        include='#include "rendering/shadows/native_tu15_shadow_record_merge.hpp"\n',
        bridge='halo2::native_shadow_record_merge_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '4ac20': dict(
        pin='99d8112f9aa2877cb88c13a7460476e8bfbe658b567c404967ef7eafc1e5a4cf',
        unit='#include "halo2/source_shadow_frame_tu15.hpp"',
        include='#include "rendering/shadows/native_tu15_shadow_schedule.hpp"\n',
        bridge='halo2::native_shadow_schedule_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t){{run_4b150(v);}})',
        direct=('4b150',), dispatched=()),
    '1efd60': dict(
        pin='35f9b1ba810328a527757499d335ffcd5c05f24de3629a64fcadaa0f2508c49d',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_action_allowed.hpp"\n',
        bridge='halo2::native_action_allowed_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{switch(target){{case 0x1f1160u:run_1f1160(v);break;case 0x1efd10u:run_1efd10(v);break;case 0x1efcc0u:run_1efcc0(v);break;case 0x1ac79bu:run_1ac79b(v);break;case 0x1ac595u:run_1ac595(v);break;case 0xcad40u:run_cad40(v);break;case 0x165a50u:run_165a50(v);break;case 0x536e0u:run_536e0(v);break;case 0x1060a0u:run_1060a0(v);break;default:run_1060e0(v);break;}}}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('1f1160', '1efd10', '1efcc0', '1ac79b', '1ac595', 'cad40', '165a50', '536e0', '1060a0', '1060e0'),
        dispatched=('13f620', 'ce230', 'd2ad0', 'd2b00', '161530', 'bcdf0', '1055f0', '105fd0')),
    '1ddbf0': dict(
        pin='1b0c6e7fc6752649e9e523e3f6a63e5d8d5c9546774fd2d2b5b9aa0b06883ecc',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_physics_groups.hpp"\n',
        bridge='halo2::native_physics_groups_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target){{switch(target){{case 0x212c70u:run_212c70(v);break;case 0x184920u:run_184920(v);break;case 0x1ddea0u:run_1ddea0(v);break;case 0x1d8330u:run_1d8330(v);break;default:run_1ded80(v);break;}}}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('212c70', '184920', '1ddea0', '1d8330', '1ded80'),
        dispatched=('32a660', '2e31e0')),
    '106160': dict(
        pin=('a3ef2da841f89cb635e0e36d1f1384cd03d5745b0f9a8b166174eba9f13d3375',
             'aeb6c8c7d52807e381af27b2738151dfffbe349630d288ee016f0b4aaf227f9d'),
        unit=('#include "halo2/source_vehicle_director_tu15.hpp"',
              '#include "halo2/source_combat_action_tu15.hpp"'),
        include='#include "gameplay/vehicles/native_tu15_weapon_magnification.hpp"\n',
        bridge=(
            'halo2::native_weapon_magnification_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_37cde8(v);}})',
            'halo2::native_weapon_magnification_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{dispatch(v,0x37cde8u,0x10620du,false);}})'),
        direct=(('37cde8',), ()), dispatched=((), ('37cde8',))),
    'cad60': dict(
        pin='0f563fc25d6f2ba8ac54be5c2a78653a2197a3b3f97e85752671fac06a9aeb0a',
        unit='#include "halo2/source_vehicle_director_tu15.hpp"',
        include='#include "gameplay/vehicles/native_tu15_zoom_factor.hpp"\n',
        bridge='halo2::native_zoom_factor_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_106160(v);}},[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})',
        direct=('106160',), dispatched=('106040',)),
    '145560': dict(
        pin=('6ab5af55d5cce6547098d13f85c4169b2dc1364b50c382161698df28bac38169',
             '3ce39c1046f43fec27fe7f65c35755ca56024dcdae65bdebd4ec4e2377171b62'),
        unit=('#include "halo2/source_scene_device_constructor_tu15.hpp"',
              'void source_run_physics_live_support_tu15(',
              '#include "halo2/source_ai_frame_tu15.hpp"',
              '#include "halo2/source_effect_particle_frame_tu15.hpp"'),
        include='#include "rendering/renderer/native_tu15_transform_product.hpp"\n',
        bridge='halo2::native_transform_product_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '1e1010': dict(
        pin='e4a5880ae1825864f3b618ca77c4ed6376784b699391d5f6d3db899320ba12e8',
        unit=('#include "halo2/source_scene_device_constructor_tu15.hpp"',
              '#include "halo2/source_map_world_support_tu15.hpp"'),
        include='#include "rendering/renderer/native_tu15_vector_normalize.hpp"\n',
        bridge='halo2::native_vector_normalize_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '13de70': dict(
        pin=('2a4e7ff7446cc6e03d11146f2a8284591ca8fedfed3e2e05eddaa30aa2e7fb53',
             '30b54323490595e92005ba74229fe44029732c6c3b6c3be7e4bf9397a0f69343'),
        unit=('void source_run_physics_live_support_tu15(',
              '#include "halo2/source_lens_draw_tu15.hpp"'),
        include='#include "core/native_tu15_authored_function_bridge.hpp"\n',
        bridge=('halo2::native_authored_function_detail::bridge(s,'
                '[](auto &v){{return {reference}(v);}},'
                '[](std::uint32_t target,auto image){{return canonical_branch(target,image);}})'),
        direct=('17f8a0', '17f790'), dispatched=('13e9a0', '37cde8'),
        continuations=('13dee5', '13df3f', '13e035', '13e0ed', '13e146', '13e2a2')),
    '24ffe0': dict(
        pin='d7a7db231757a5b1b53cd64e94ab7f961a0b96e8925a7c84eabe7de084fbaf32',
        unit=('#include "halo2/source_effect_particle_frame_tu15.hpp"',
              '#include "halo2/death_cycle_runtime_tu15.hpp"',
              '#include "halo2/source_combat_action_tu15.hpp"'),
        include='#include "rendering/effects/native_tu15_particle_property.hpp"\n',
        bridge=('halo2::native_particle_property_detail::bridge(s,[](auto &v){{{reference}(v);}},'
                '[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('13de70',)),
    '2e56a0': dict(
        pin='1ea0b38005d365cc7c2651dbe8034b374cf85186eef1e2c2414e2692f96a325b',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "rendering/renderer/native_tu15_quaternion_relative_product.hpp"\n',
        bridge=('halo2::native_quaternion_relative_product_detail::bridge(s,[](auto &v){{{reference}(v);}},'
                '[](long double value,std::uint16_t &status){{return native_single_spill(value,status);}})'),
        direct=(), dispatched=()),
    '1e14f0': dict(
        pin=('2a1d7ae16c8c3729457e31eb07c23462fc021febcc32289123b3685d8def415f',
             'effba6d94cdc77560a54d9ef63cccbf9adb6daa54f2c7a3780b5f74a0f52a34b'),
        unit=('#include "halo2/source_ai_actor_tu15.hpp"',
              '#include "halo2/source_ai_frame_tu15.hpp"',
              'void source_run_physics_live_support_tu15('),
        include='#include "gameplay/units/native_tu15_animation_fade_start.hpp"\n',
        bridge='halo2::native_animation_fade_start_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '19ae70': dict(
        pin='24e6d20d3b1bae777542f1978ae5fa2a2d9da86c3098fc006e8f4896285cc373',
        unit='#include "halo2/source_player_departure_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_departure_remap.hpp"\n',
        bridge='halo2::native_departure_remap_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_32a660(v);}},[](auto &v){{run_19dd10(v);}})',
        direct=('32a660', '19dd10'), dispatched=()),
    '1ce8d0': dict(
        pin='3a44e9316afdca256ca9565edf550e41e155a8e6f6f2416d05484c5ce20c5000',
        unit='#include "halo2/source_scene_device_constructor_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_animation_fraction.hpp"\n',
        bridge='halo2::native_animation_fraction_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v,std::uint32_t target,std::uint32_t site,bool tail){{dispatch(v,target,site,tail);}})',
        direct=(), dispatched=('1e3150', '1ce810')),
    '1d30f0': dict(
        pin='f974465cf9476e0cfdbd0851a2440ae4857ba4c5ee7155012361945d8cae52cf',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_partition_removal.hpp"\n',
        bridge=('halo2::native_partition_removal_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=('16e8a0',)),
    '1e5250': dict(
        pin='01d907bb828cfae4478a92e8e80c65d893614cbfc33566ae888254169cdba5f4',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_record_iterator.hpp"\n',
        bridge=('halo2::native_record_iterator_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](std::uint32_t p,SourcePhysicsContactImage image){{return canonical_branch(p,image);}},\n'
                '        [](auto &v){{run_1e5810(v);}})'),
        direct=('1e5810',), dispatched=()),
    '333f0': dict(
        pin='121cbdff8ad5cce34e3ce789b77b289ef66b665acbc394547ca1ee1835bfc697',
        unit='#include "halo2/source_shadow_frame_tu15.hpp"',
        include='#include "rendering/shadows/native_tu15_shadow_casters.hpp"\n',
        bridge=('halo2::native_shadow_casters_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_3d7e0(v);}},[](auto &v){{run_3e9b0(v);}},[](auto &v){{run_4a7b0(v);}},\n'
                '        [](auto &v){{run_4ac20(v);}})'),
        direct=('3d7e0', '3e9b0', '4a7b0', '4ac20'), dispatched=()),
    '165860': dict(
        pin='be11df8f85a98fbf84857e5fb2d83e60c0c15a9a9b9f8cfa9e779ac705449856',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_player_marks.hpp"\n',
        bridge=('halo2::native_player_marks_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](auto &v){{run_53780(v);}})'),
        direct=('53780',), dispatched=('1a8e50',)),
    '15c4e0': dict(
        pin='453d30af9e9d04844148d691b1c1a28a68a77d560406a4735cde81787b706dfe',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "gameplay/multiplayer/native_tu15_object_cleanup.hpp"\n',
        bridge=('halo2::native_object_cleanup_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](auto &v){{run_164750(v);}})'),
        direct=('164750',), dispatched=('bcee0', '105fd0', 'bbbc0', 'ba570')),
    '3f3e0': dict(
        pin='02b921d8fb33661a45bd7c7c042d9ea669b9f5cdea68764b6985361e8a91d6b7',
        unit='#include "halo2/source_effect_particle_frame_tu15.hpp"',
        include='#include "rendering/effects/native_tu15_index_marks.hpp"\n',
        bridge='halo2::native_index_marks_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'bcfe0': dict(
        pin='ed961b699980ef9cd448600e2ff8e872903bdba60a7c0909394b00389f5b2d0b',
        unit='void source_run_physics_live_support_tu15(',
        include='#include "physics/runtime/native_tu15_object_ancestry.hpp"\n',
        bridge='halo2::native_object_ancestry_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '17fb60': dict(
        pin='ea1020b6e6575513b9d94cb53909607ff5d534e02ec502c15890d4359bf72598',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_record_fade.hpp"\n',
        bridge='halo2::native_record_fade_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_140310(v);}},[](auto &v){{run_32cc0c(v);}})',
        direct=('140310', '32cc0c'), dispatched=()),
    '17faf0': dict(
        pin='cbe8d5896ac39ffd1df55b69828f83fc89a92a262febae1aafe579e19172a2f1',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_chain_byte.hpp"\n',
        bridge='halo2::native_chain_byte_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_17fb60(v);}})',
        direct=('17fb60',), dispatched=()),
    '180520': dict(
        pin='a072f1e6dd02350cceaa5be9ce059bd14795bdc81a56ce29d62dad3c2d0b6d27',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_record_walk.hpp"\n',
        bridge=('halo2::native_record_walk_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_43980(v);}},[](auto &v){{run_17faf0(v);}})'),
        direct=('43980', '17faf0'), dispatched=()),
    '144bf0': dict(
        pin='244fbf0745b9bf9473779613c51c4ded38f292861e31bd47c627af10ba257364',
        unit=('#include "halo2/source_ai_frame_tu15.hpp"', '#include "halo2/source_scene_device_constructor_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_basis_matrix.hpp"\n',
        bridge='halo2::native_basis_matrix_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '179a40': dict(
        pin=('883875254a957e7acfc08cbe3c124f2787e6528078482e8a5ea86dfa6d82769f',
             '2ffb1173298e4039a70a2ef5fb4d66597b87f013d7d148843d54062b3a5240f2'),
        unit=('#include "halo2/source_shadow_frame_tu15.hpp"', '#include "halo2/source_lens_flare_tu15.hpp"'),
        include='#include "rendering/shadows/native_tu15_marked_records.hpp"\n',
        bridge=('halo2::native_marked_records_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](auto &v){{run_17cd10(v);}})'),
        direct=('17cd10',), dispatched=()),
    '17beb0': dict(
        pin='f1b1faf8f3c8b44f3d3a30dd52e534b4a293f56781399927b9373909b8c8b04f',
        unit='#include "halo2/death_cycle_runtime_tu15.hpp"',
        include='#include "gameplay/units/native_tu15_effect_parts.hpp"\n',
        bridge=('halo2::native_effect_parts_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](auto &v){{run_17bd40(v);}},[](auto &v){{run_17bb10(v);}})'),
        direct=('17bd40', '17bb10'), dispatched=('158290',)),
    '1a8e50': dict(
        pin=('d9f6d8db27b5e0b8cfa3495f92b24118211859303288616bcb05fcd70998e427',
             'f85f8d3c3fb3a565356737e4e87fe2a541a9387c533760e97a0c5fb4bdc11216'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/death_cycle_runtime_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_filtered_iterator.hpp"\n',
        bridge=('halo2::native_filtered_iterator_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    '1902e0': dict(
        pin=('bd12ffd0f7ce364066c0ba32607a92c6fdab67812cba38caa6d9032f8c924ad5',
             '346b2949c5320b69094f6a46b49ef750f0d77cce607d5b23012604a2c6660ea9'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_map_world_support_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_player_relation.hpp"\n',
        bridge=('halo2::native_player_relation_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    'b1720': dict(
        pin=('8d581f59035b5a72960f34609aa0d5c58c32d0adaf9b8a934724b5c1a5d75a7d',
             '8840a42109da383911309942dda52ee3ee506d83c8cd735fa8c9772a1376b5db'),
        unit=('void source_run_physics_live_support_tu15(', '#include "halo2/source_ai_frame_tu15.hpp"'),
        include='#include "physics/runtime/native_tu15_unit_vector_check.hpp"\n',
        bridge=('halo2::native_unit_vector_check_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v){{run_aa200(v);}},[](auto &v){{run_aa270(v);}})'),
        direct=('aa200', 'aa270'), dispatched=()),
    '131610': dict(
        pin='70daa4bb9b4d19f1985efeef65b2f22d32a213b9cd7eaf4388e9374571b0449c',
        unit='#include "halo2/source_campaign_environment_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_fog_mixers.hpp"\n',
        bridge='halo2::native_fog_mixers_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_137ac0(v);}})',
        direct=('137ac0',), dispatched=()),
}


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def reference_name(name):
    return f'native_frame_misc_reference_{name}'


def markers(name):
    return (f'// BEGIN native frame misc reference {name}\n', f'// END native frame misc reference {name}\n',
            f'\n    // BEGIN native frame misc {name}\n', f'\n    // END native frame misc {name}')


def bridge(name, found=None):
    """An owner's bridge; copies whose units differ in what the bridge passes
    (a child route, a symbol hook) give one bridge per pin, in pin order."""
    owner = OWNERS[name]
    if isinstance(owner['bridge'], str):
        return owner['bridge']
    if found is None:
        raise ValueError(f'{name} has a bridge per pin: name the copy')
    return owner['bridge'][alternatives(owner['pin']).index(digest(found))]


def children(name, found, route):
    """Resolve a single child list or per-pin lists for units with mixed routes."""
    owner = OWNERS[name]
    routes = owner[route]
    if routes and isinstance(routes[0], tuple):
        return routes[alternatives(owner['pin']).index(digest(found))]
    return routes


def prefix(name, found=None):
    begin, end = markers(name)[2:]
    invoke = bridge(name, found).format(reference=reference_name(name))
    continuations = OWNERS[name].get('continuations', ())
    if not continuations:
        return begin + f'    if({invoke})return;' + end
    # The original entry remains available. A partially native call resumes
    # with the already-published register state at its real source case.
    labels = ''.join(f'        case 0x{pc}u:goto L{pc};\n' for pc in continuations)
    return (begin + f'    switch({invoke}){{\n        case 0u:return;\n'
            + labels + '        case 0xffffffffu:break;\n'
            + '        default:throw std::logic_error("Invalid native authored function continuation");\n    }'
            + end)


# Packed SSE whose operands both hold NaNs takes the destination's payload.
# Without instruction callbacks the compiler may swap a commutative
# intrinsic's operands, so the verification reference pins the original's
# destination (as tools/specialize_native_operands.py pins the matrix
# leaves); the owner's adapter includes physics/runtime/native_tu15_packed_sse.hpp.
PACKED = {'_mm_add_ps(': 'halo2::native_packed_sse::add(', '_mm_mul_ps(': 'halo2::native_packed_sse::multiply('}

# REPE CMPSD in 19AE70 reads the player identity (ESI) before the roster
# identity (EDI). Spell out that order for both translated routes because C++
# does not specify the order of evaluation of the subtraction's arguments.
DEPARTURE_COMPARE = re.compile(
    r'(0x19af88u\);\}? while\(s\.gp\[1\]\)\{)'
    r's\.flags\.minus\(r\(s\.memory,s\.gp\[6\],4\),r\(s\.memory,s\.gp\[7\],4\),4\);')
DEPARTURE_COMPARE_ORDERED = (
    r'\g<1>const auto player=r(s.memory,s.gp[6],4);'
    'const auto roster=r(s.memory,s.gp[7],4);s.flags.minus(player,roster,4);')

# Scalar SSE preserves the destination's upper lanes and selects its NaN
# payload when both inputs are NaNs. Pin distinct-operand commutative
# instructions in the retained body and in its verification reference.
SCALAR_ARITHMETIC_SITES = {
    '_mm_mul_ss': ('halo2::native_tu15_leaves::ordered_mul_ss', {
        '145b70': (('145b96', 0), ('145ba0', 1), ('145bad', 1), ('145bbe', 0), ('145bc8', 1), ('145bd5', 1),
                   ('145be8', 0), ('145bf2', 1), ('145bff', 1)),
        '1ce8d0': (('1ce90d', 1),),
        'e05d0': (('e061c', 4), ('e0630', 4), ('e069e', 3), ('e06b2', 3)),
        '1ee3d0': (('1ee517', 0), ('1ee51b', 6), ('1ee535', 0), ('1ee539', 6),
                   ('1ee556', 0), ('1ee55f', 1), ('1ee5a7', 0), ('1ee5bb', 0)),
        '24ffe0': (('250038', 1), ('250065', 1)),
    }),
    '_mm_add_ss': ('halo2::native_world_leaves_detail::ordered_add_ss', {
        '145b70': (('145ba5', 0), ('145bb1', 0), ('145bcc', 0), ('145bda', 0), ('145bf6', 0), ('145c04', 0)),
        '1ee3d0': (('1ee4ac', 0), ('1ee4b7', 0), ('1ee51f', 0), ('1ee53d', 0), ('1ee563', 0)),
        '24ffe0': (('25003c', 1), ('250078', 1)),
    }),
}

X87_MEMORY_OPERANDS = {
    '2e4350': ('halo2::native_rotation_quaternion_detail::load_operand', 29),
    '24ffe0': ('halo2::native_particle_property_detail::load_operand', 2),
}

FIRST_PERSON_STACK_UPDATES = {
    '168dfb': ('fffffffc', 'logic(({read})&(selected(0x0u,s.image)),4)'),
    '168e54': ('fffffff8', 'increment({read},4,true)'),
    '168e9c': ('ffffffe4', 'logic(({read})&(selected(0x0u,s.image)),4)'),
    '168eac': ('ffffffec', 'logic(({read})&(selected(0x0u,s.image)),4)'),
    '16904d': ('fffffffc', 'increment({read},4,false)'),
    '169050': ('fffffff0', 'plus({read},selected(0xd0cu,s.image),4)'),
    '16905d': ('fffffff4', 'increment({read},4,true)'),
    '1690f2': ('fffffffc', 'increment({read},4,false)'),
}
FIRST_PERSON_IDENTITY_OPERAND = re.compile(r'selected\((0x[0-9a-f]+u),s\.image\)')

def retained(text, name):
    if name == '168ded':
        for pc, (offset, operation) in FIRST_PERSON_STACK_UPDATES.items():
            address = f'(selected(0x{offset}u,s.image)+s.gp[5])'
            old = operation.format(read=f'r(s.memory,{address},4)')
            literal = f'w(s.memory,s.observe,0x{pc}u,{address},4,s.flags.{old});'
            new = operation.format(read='value')
            replacement = ('halo2::native_first_person_model_preparation::modify'
                           f'(s,0x{pc}u,{address},[&](auto &flags,auto value){{return flags.{new};}});')
            if literal not in text:  # The production alias has resolved these identity operands.
                literal = FIRST_PERSON_IDENTITY_OPERAND.sub(r'\1', literal)
                replacement = FIRST_PERSON_IDENTITY_OPERAND.sub(r'\1', replacement)
            if text.count(literal) != 1:
                raise ValueError(f'168DED stack update at {pc} changed')
            text = text.replace(literal, replacement, 1)
    if name in X87_MEMORY_OPERANDS:
        # These x87 memory operands raise denormal exceptions even when
        # a compiler could lower a C++ extension through SSE.
        loader, expected = X87_MEMORY_OPERANDS[name]
        text, count = re.subn(r'ext\(f\((r\(s\.memory,[^\n]*?,4\))\)\)', rf'{loader}(\1)', text)
        if count != expected:
            raise ValueError(f'{name} retained x87 operands changed')
    if name == '19ae70':
        text, count = DEPARTURE_COMPARE.subn(DEPARTURE_COMPARE_ORDERED, text)
        if count != 1:
            raise ValueError('19ae70 retained REPE CMPSD changed')
    for intrinsic, (ordered, sites) in SCALAR_ARITHMETIC_SITES.items():
        for pc, register in sites.get(name, ()):
            instruction = rf'(0x{pc}u\);\}}? s\.xmm\[{register}\]=){intrinsic}\('
            text, count = re.subn(instruction, rf'\g<1>{ordered}(', text)
            if count != 1:
                raise ValueError(f'{name} retained {intrinsic} at {pc} changed')
    return text


def pinned(text, name):
    """Keep the original operand order in the translated verification body."""
    for packed, ordered in PACKED.items():
        text = text.replace(packed, ordered)
    return retained(text, name)


def authored_function_reference(found, continuations):
    # Both authenticated copies share this prefix and three leaf exits.
    # Stop at a retained target before its first effect, so its caller runs
    # the residual body only once, after replay has checked the native prefix.
    text = found[0]
    text = (text[:text.index('\nL13dee5:')]
            + text[text.index('\nL13e0d2:'):text.index('\nL13e0ed:')]
            + text[text.index('\nL13e5d5:'):])
    text = text.replace(f'static void run_13de70({found[1]} &s) {{',
                        f'std::uint32_t {reference_name("13de70")}(WorldState &s) {{', 1)
    if text.count('return;') != 3:
        raise ValueError('13de70 bounded reference return sites changed')
    text = text.replace('return;', 'return 0u;')
    for pc in continuations:
        edge = f'case 0x{pc}u:goto L{pc};'
        if text.count(edge) != 1:
            raise ValueError(f'13de70 continuation {pc} changed')
        text = text.replace(edge, f'case 0x{pc}u:return 0x{pc}u;', 1)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text):
        raise ValueError('13de70 bounded reference kept an unrouted child')
    return text + '\n'


# TU1.5 jump table 13E5F4 maps 4 to linear, 5 to ramp, 7/10 to
# cubic. 9 enters the separate power case at 13DF3F.
AUTHORED_LINEAR_RAMP_CASES = (('linear', '13e0ed', '13e146'), ('ramp', '13e146', '13e2a2'))
AUTHORED_CUBIC_CASES = (('input', '13e2a2', '13e2b0'), ('base', '13e3e8', '13e43e'),
                        ('refresh', '13e574', '13e5d5'), ('blend', '13e582', '13e5d5'))
AUTHORED_RESIDUAL_HEADERS = ('#include "core/native_tu15_authored_linear_ramp.hpp"\n'
                             '#include "core/native_tu15_authored_cubic.hpp"\n')
AUTHORED_SSE_SITES = {
    '_mm_mul_ss': (('13e0fc', 0), ('13e118', 2), ('13e130', 1), ('13e136', 2),
                   ('13e15c', 0), ('13e17e', 0), ('13e1a0', 0), ('13e1c0', 0),
                   ('13e1c9', 2), ('13e1d6', 2), ('13e202', 0), ('13e224', 0),
                   ('13e240', 1), ('13e25c', 4), ('13e265', 1), ('13e272', 0),
                   ('13e289', 2), ('13e28f', 4), ('13e3fc', 1), ('13e400', 3),
                   ('13e404', 4), ('13e414', 2), ('13e418', 4), ('13e577', 1),
                   ('13e57e', 2), ('13e587', 3), ('13e590', 1), ('13e5a5', 0),
                   ('13e5bc', 1), ('13e5c2', 3)),
    '_mm_add_ss': (('13e100', 0), ('13e11c', 2), ('13e13a', 2), ('13e1cd', 0),
                   ('13e1da', 0), ('13e1de', 0), ('13e26e', 4), ('13e276', 4),
                   ('13e280', 4), ('13e293', 4), ('13e408', 3), ('13e41c', 3),
                   ('13e420', 3), ('13e599', 3), ('13e5a9', 3), ('13e5b3', 3),
                   ('13e5c6', 3)),
}


def authored_residual_operands(text):
    # Pin two-address scalar operations only at the authenticated case PCs.
    for intrinsic, sites in AUTHORED_SSE_SITES.items():
        ordered = SCALAR_ARITHMETIC_SITES[intrinsic][0]
        for pc, register in sites:
            instruction = rf'(0x{pc}u\);\}}? s\.xmm\[{register}\]=){intrinsic}\('
            text, count = re.subn(instruction, rf'\g<1>{ordered}(', text)
            if count != 1:
                raise ValueError(f'13de70 retained {intrinsic} at {pc} changed')
    return text


def authored_site(text, pc, label=True):
    if label:
        markers = (f'\nL{pc}: ',)
    else:
        markers = (f'\nif(s.services.instruction){{s.services.instruction(0x{pc}u);',
                   f'\ns.instruction_literal(0x{pc}u);')
    present = [marker for marker in markers if marker in text]
    if len(present) != 1 or text.count(present[0]) != 1:
        raise ValueError(f'13de70 authored case site {pc} changed')
    return present[0]


def authored_case(text, start, end):
    end_label = end not in ('13e2b0', '13e43e')
    return text[text.index(authored_site(text, start)) + 1:text.index(authored_site(text, end, end_label)) + 1]


def authored_linear_ramp_reference(found):
    text = authored_residual_operands(found[0])
    references = []
    for case, start, end in AUTHORED_LINEAR_RAMP_CASES:
        section = authored_case(text, start, end)
        if section.count('goto L13e0d2;') != 2 or re.search(r'\bdispatch\(|\brun_[0-9a-f]+\(|required\(', section):
            raise ValueError(f'13de70 {case} reference no longer rejoins the shared clamp')
        references.append(f'void {reference_name("13de70")}_{case}(WorldState &s) {{\n'
                          f'    goto L{start};\n{section}L13e0d2: return;\n}}\n')
    return ''.join(references)


def authored_cubic_reference(found):
    text = authored_residual_operands(found[0])
    references = []
    continuation = 'halo2::native_authored_cubic::Continuation'
    for case, start, end in AUTHORED_CUBIC_CASES:
        section = authored_case(text, start, end).removeprefix(f'L{start}: ')
        if re.search(r'\bdispatch\(|\brun_[0-9a-f]+\(|required\(', section):
            raise ValueError(f'13de70 cubic {case} reference unexpectedly calls a child')
        name = f'{reference_name("13de70")}_cubic_{case}'
        returns = {
            'input': (('13e3e8', 'polynomial'),),
            'base': (('13e0d2', 'clamp'), ('13e582', 'blend')),
            'refresh': (('13e0d2', ''),),
            'blend': (('13e0d2', ''),),
        }
        for pc, route in returns[case]:
            source = f'goto L{pc};'
            if section.count(source) != 1:
                raise ValueError(f'13de70 cubic {case} continuation {pc} changed')
            section = section.replace(source, f'return {continuation}::{route};' if route else 'return;')
        entry = case in ('input', 'base')
        if entry:
            tail = 'powered_input' if case == 'input' else 'powered_layer'
            section += f'return {continuation}::{tail};\n'
        section = section.replace('L13e582: ', '')
        references.append(f'{continuation if entry else "void"} {name}(WorldState &s) {{\n{section}}}\n')
    return ''.join(references)


def authored_clamp_reference(found):
    text = found[0]
    low = authored_case(text, '13e0d2', '13e0ed').removeprefix('L13e0d2: ')
    high = text[text.index(authored_site(text, '13e5d5')) + 1:text.rindex('\n}')]
    if text.count('goto L13e5d5;') != 1 or low.count('goto L13e5d5;') != 1:
        raise ValueError('13de70 upper clamp gained a nonterminal entry')
    return f'void {reference_name("13de70")}_clamp(WorldState &s) {{\n{low}{high}\n}}\n'


def install_authored_residuals(text):
    text = authored_residual_operands(text)
    clamp = authored_site(text, '13e0d2')
    clamp_call = f'halo2::native_authored_function_detail::bridge_clamp(s,{reference_name("13de70")}_clamp)'
    text = text.replace(clamp, f'{clamp}if({clamp_call})return;\n', 1)
    for case, start, _ in AUTHORED_LINEAR_RAMP_CASES:
        label = authored_site(text, start)
        owner = 'halo2::native_authored_linear_ramp'
        reference = f'{reference_name("13de70")}_{case}'
        bridge = f'{owner}::bridge(s,{owner}::Kind::{case},{reference})'
        text = text.replace(label, f'{label}if({bridge})goto L13e0d2;\n', 1)
    for pc in ('13e2b0', '13e43e'):
        site = authored_site(text, pc, label=False)
        text = text.replace(site, f'\nL{pc}: {site[1:]}', 1)
    starts = {
        'input': (('powered_input', '13e2b0'), ('polynomial', '13e3e8')),
        'base': (('clamp', '13e0d2'), ('powered_layer', '13e43e'), ('blend', '13e582')),
    }
    owner = 'halo2::native_authored_cubic'
    for case, start, _ in AUTHORED_CUBIC_CASES:
        label = authored_site(text, start)
        reference = f'{reference_name("13de70")}_cubic_{case}'
        arguments = f's,{owner}::Entry::{case},{reference}'
        if case in starts:
            branches = ''.join(f'case {owner}::Continuation::{route}:goto L{pc};' for route, pc in starts[case])
            hook = f'switch({owner}::begin({arguments})){{{branches}default:break;}}'
        else:
            hook = f'if({owner}::blend({arguments}))goto L13e0d2;'
        text = text.replace(label, f'{label}{hook}\n', 1)
    return text


# The authored kind-10 parameter correction reaches the genuine control-point
# and CRT power dispatchers. The two original copies have different C1 spills.
AUTHORED_POWERED_CASES = (('base', '13e2b0', '13e3e8'), ('blend', '13e43e', '13e574'))
AUTHORED_POWERED_HEADER = '#include "core/native_tu15_authored_powered_parameter.hpp"\n'
AUTHORED_POWERED_ADDS = (('13e301', 1), ('13e33a', 3), ('13e48f', 1), ('13e4c7', 3))


def authored_powered_operands(text):
    ordered = SCALAR_ARITHMETIC_SITES['_mm_add_ss'][0]
    for pc, register in AUTHORED_POWERED_ADDS:
        pattern = rf'(0x{pc}u\);\}}? s\.xmm\[{register}\]=)_mm_add_ss\('
        text, count = re.subn(pattern, rf'\g<1>{ordered}(', text)
        if count != 1:
            raise ValueError(f'13de70 powered parameter ADDSS at {pc} changed')
    return text


def authored_powered_reference(found):
    text = authored_powered_operands(found[0])
    references = []
    for layer, start, end in AUTHORED_POWERED_CASES:
        section = text[text.index(authored_site(text, start, label=False)) + 1:text.index(authored_site(text, end)) + 1]
        section, children = re.subn(r'dispatch\(s,target,(0x[0-9a-f]+u),false\)', r'child(s,target,\1)', section)
        if children != 5 or section.count('source_estimate(s,') != 1 or section.count(f'goto L{end};') != 3:
            raise ValueError(f'13de70 powered {layer} child or continuation changed')
        section = section.replace('source_estimate(s,', 'estimate(s,', 1)
        name = f'{reference_name("13de70")}_powered_{layer}'
        references.append(f'template<class Child, class Estimate>\n'
                          f'void {name}(WorldState &s, Child child, Estimate estimate) {{\n'
                          f'{section}L{end}: return;\n}}\n')
    return ''.join(references)


def install_authored_powered(text, found, has_residual_labels):
    text = authored_powered_operands(text)
    pins = alternatives(OWNERS['13de70']['pin'])
    spill = ('world', 'lens')[pins.index(digest(found))]
    owner = 'halo2::native_authored_powered_parameter'
    for layer, start, end in AUTHORED_POWERED_CASES:
        site = authored_site(text, start, label=has_residual_labels)
        reference = f'{reference_name("13de70")}_powered_{layer}'
        callback = f'[](auto &v,auto child,auto estimate){{{reference}(v,child,estimate);}}'
        dispatch = '[](auto &v,std::uint32_t target,std::uint32_t pc){dispatch(v,target,pc,false);}'
        hook = f'if({owner}::bridge(s,{owner}::Layer::{layer},{owner}::Spill::{spill},{callback},{dispatch}))goto L{end};'
        if has_residual_labels:
            replacement = site + hook + '\n'
        else:
            replacement = '\n' + hook + site
        text = text.replace(site, replacement, 1)
    return text


def reference(name, found):
    if name == '13de70':
        return authored_function_reference(found, OWNERS[name]['continuations'])
    text = pinned(found[0].replace(f'static void run_{name}({found[1]} &s) {{', f'void {reference_name(name)}(WorldState &s) {{', 1), name)
    text = DIRECT.sub(lambda m: f'{DETAIL}::replay(s,0x{m[1]}u)', text)
    text = DISPATCH.sub(lambda m: f'{DETAIL}::replay(s,{m[1]})', text)
    # A child that leaves the unit through the World's required owner at its
    # site (as tools/native_object_subtree_replacements.py replays it).
    text = REQUIRED.sub(lambda m: f'{DETAIL}::replay(s,{m[1]})', text)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('void '):
        raise ValueError(f'Frame misc reference {name} kept an unrouted child')
    return text + '\n'


def alternatives(value):
    """A routine whose copies live in several units names each unit's marker
    line and each copy's pin; a single unit names one of each."""
    return value if isinstance(value, tuple) else (value,)


def admitted(canonical, name):
    owner = OWNERS[name]
    found = body(canonical, name)
    if not found or not any(unit in canonical for unit in alternatives(owner['unit'])):
        return False
    if digest(found) not in alternatives(owner['pin']):
        return False
    if name in ('2aa6d0', '145b70', '168ded') or (name == '130d20' and children(name, found, 'dispatched')):
        resolver = re.search(r'^std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*$',
                             canonical, re.M)
        operands = set(re.findall(r'\bselected\(0x([0-9a-f]+)u,s\.image\)', found[0])) | {name}
        if not resolver or any(resolver[0].count(f'case 0x{operand}u:return 0x{operand}u;') != 1
                               for operand in operands):
            return False
        if name == '168ded':
            dispatcher = re.search(r'^static void dispatch\(WorldState &s,[^\n]*$', canonical, re.M)
            routes = children(name, found, 'direct')
            if (not dispatcher or not dispatcher[0].endswith('s.required(site,target,tail);}') or
                any(dispatcher[0].count(f'if(target==selected(0x{child}u,s.image)){{run_{child}(s);return;}}') != 1
                    for child in routes)):
                return False
    if any(not body(canonical, child) for child in children(name, found, 'direct')):
        return False
    return not any(body(canonical, child) for child in children(name, found, 'dispatched'))


# Owners installed by default. Each closure is now written to the code standard
# of docs/native_replacement/REMAINING_TRANSLATED_20261115.md (game logic in
# its *_systems.hpp, a thin boundary adapter in the bridge's header) and
# re-proven by tools/validate_native_frame_misc_tu15.py (2026-11-16). Bound
# after the live checks on the restored MCC cache (2026-11-16 evening): Armory
# parity in every role, and the MP and campaign verify sweep with 0
# differences; see docs/native_replacement/REMAINING_TRANSLATED_20261115.md.
BOUND = ('1306b0', 'd7230', '1eee80', '41970', '133a70', '153ba0', '131610', '14d200', 'bd460', '1ae2b0', '1ae090', '3d7e0', '16b240', '170730', '1d30f0', '1e5250', '333f0', '15c4e0', '3f3e0', 'bcfe0', '17faf0', '180520', '144bf0', '179a40', '1a8e50', '1902e0', 'b1720', '17fb60', '1d2f10', '17beb0', '165860', '157fe0', 'e9530', '10e1f0', '125790', '1e1390', '116060', 'aa200', 'aa270', 'b9390', '1751b0', 'e65a0',
         '106230', '1d9f50', '144cb0', 'bac30', '10e340', 'be7e0', 'bda50', '1ce810', '130d20', '145b70', 'e0500', 'c0700', '1eedb0', '18bbc0', 'c93d0',
         'c92c0', '1e1620', '110fc0', '1e1920', '11fa50', 'dfd20', '1a8f10', '175300',
         '175280', '175240', '1752c0', 'b16c0', '17c5c0', '1ae500', '212c70', '30c40', '11e060', 'd2330', '105950',
         '10dde0', '10b0f0', '1d3f90', '16e290', 'b8d90', 'bbff0', 'caaa0',
         '14fbc0', 'ce230', 'cfb40', '13f620', '11e780', 'e3310', '15dd10',
         'bc330', 'bbd50', '15c7f0', 'df820', '111d10',
         'c5250', '17c330', 'c9590', '1e5d60', 'ba910',
         'd2b00', '1062a0', '1cd620', '11e600', '152e00', '106040', '115ff0', '1e15d0', 'c6e50', '15b470', '1f0f90', '17e100', '161560',
         '1cee60', '1059c0', '144210', '1452f0', 'ba7e0', 'b92c0', 'd3a80', '31edf0', '33210c', '161530', '1d4140', '116480', 'e32c0', 'cb440',
         '172200', 'bcf70', '111f00', '140ec0', '105fd0', 'c8c20', '113c70', '1e5160',
         '1e5df0', '1cf070', '1cefb0',
         '1e5a70', 'bbf20',
         '1d4e50', '1fa110', '102910',
         '1e5f20', '1e7810', '212d40',
         '1e5e40', '1521b0', '152d60',
         'c8cf0', '1be90',
         '2a0100', '2db80', '1d3930', '1dc610', 'bc190', '10c5d0', 'dfe30', '1e5a50', '335b0', '150990', '25bc70', '226b0',
         '189f20', '158450', '25bcc0', 'd2b60', '1d9f10', '43980', '12f0c0', '118760', '1d6430', '304c50', '1582f0',
         '18a240', '171ff0', '20700', '1f33c0', 'c5170', '31500', '10db40', '2e9750',
         '243510', '16e7f0', '2e3250', '158340', '1d9080', '1d9100', '3d2b0', '1e5360', '244f80',
         '168d76', '168ded', '158790', '161610', '13da89', '1540d0', '15b620',
         '1ae5e0', '15b2d0', '154d00', '13e670', '11ef70', '1262f0', '1ae6f0', 'b95d0',
         'c9010', '1dc680', '1157d0', '153360', '2e2e30', '1edda0', '1ceb70', 'c3960', '140cf0', 'c3a20', 'be2b0', '15cab0',
         '118120', '4b150', 'c3820', '1617a0', '1a6830',
         '1718c0', '15b740', 'e53e0', 'e52d0', '113b70', '14fdc0',
         '17cd10', '1ce950', 'ba630', '1cec60', 'e06e0',
         '1cf700', '15b4d0', '1459b0', '15be80', '1181f0', 'c8e70', 'c8390', 'd35c0',
         '1601b0', '160360', '160570', '164890', '1d5970', 'cdcf0', '69ad0', '1afa90', 'e71a0',
         '1d45a0', '191130', 'be3b0', '175380', '144910', '1af970', '1d39c0', '111b10', '16bbc0',
         '16bda0', '1cebd0', '185820', '13b610', '16e510', '13dc44', '1d8b90', '1d9190', '2e9140',
         '315190', '2f20f0', '1ddea0', '168ca1', '164ef0', '164750', '17bd40', '13b6f0', '154270', '15e870', 'e79c0', '1ae7f0', '1adc90', '1770c0', '245060', 'c7dd0', '113190', '1f8710', '17c370', '1618b0', '17ead0', '14d790', 'cd4a0', '1de8e0', '1538b0', '1700e0', '14d410', '1115d0', '1adf60', '1139d0', '15d890', 'd2bd0', '4a7b0',
         '4ac20', '1efd60', '1ddbf0', 'cad60', '1e1010', '19ae70', '1ce8d0', 'e05d0',
         '106160', '1ee3d0', '2e56a0', '1e14f0',
         '116280', '2e4350', '24ffe0', '10dfa0', '13de70', 'b9460', '2aa6d0')


def install(canonical, output, names=None, *, authored_residuals=False, authored_powered=False):
    """Return the installed entries; authored residuals are opt-in for validation."""
    installed = []
    for name in (BOUND if names is None else names):
        if not admitted(canonical, name):
            continue
        current = body(output, name)
        begin, end, prefix_begin, _ = markers(name)
        if not current or prefix_begin in current[0]:
            continue
        current_text = retained(current[0], name)
        authored = name == '13de70' and authored_residuals
        if authored:
            current_text = install_authored_residuals(current_text)
        powered = name == '13de70' and authored_powered
        if powered:
            current_text = install_authored_powered(current_text, body(canonical, name), authored)
        start = current_text.index('{') + 1
        output = output[:current.start()] + current_text[:start] + prefix(name, body(canonical, name)) + current_text[start:] + output[current.end():]
        first = FIRST_BODY.search(output)
        if not first or begin in output:
            raise ValueError(f'Frame misc reference {name} needs one unbound unit')
        original = body(canonical, name)
        reference_body = reference(name, original)
        if authored:
            reference_body += (authored_linear_ramp_reference(original) + authored_cubic_reference(original)
                               + authored_clamp_reference(original))
        if powered:
            reference_body += authored_powered_reference(original)
        block = begin + reference_body + end
        output = output[:first.start()] + block + output[first.start():]
        include = OWNERS[name]['include']
        if authored:
            include += AUTHORED_RESIDUAL_HEADERS
        if powered:
            include += AUTHORED_POWERED_HEADER
        if include not in output:
            output = include + output
        installed.append(name)
    return output, installed
