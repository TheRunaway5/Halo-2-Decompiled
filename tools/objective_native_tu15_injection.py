"""Install reviewed semantic objective owners beside their literal controls.

This only composes named C++ kernels. It neither derives rule logic nor emits
instruction implementations. The original controls remain available for traces
and offline differential validation.
"""
import re

OWNERS = {
    0x249440: ('family', 's'),
    0x24a6c0: ('selected_team', 's,dispatch'),
    0x24b390: ('team_available', 's'),
    0x24b2f0: ('default_relationship', 's'),
    0x24be50: ('marker_owner', 's'),
    0x24b4c0: ('home_marker', 's,dispatch'),
    0x24a180: ('registered_carrier', 's'),
    0x249460: ('initialize', 's,dispatch'),
    0x24b300: ('carrier_trait', 's,dispatch'),
    0x24cd70: ('team_eligible', 's,dispatch'),
    0x24a810: ('pickup_allowed', 's,dispatch'),
    0x249590: ('activate', 's,dispatch'),
    0x24a2e0: ('update_object_color', 's,dispatch'),
    0x24c670: ('validate_objectives', 's,dispatch'),
    0x24cd10: ('object_color', 's,dispatch'),
    0x24b500: ('spawn_objective', 's,dispatch'),
    0x24b5c0: ('reset_team', 's,dispatch'),
    0x24c5d0: ('set_selected_team', 's,dispatch'),
    0x24b2b0: ('rotate_team', 's,dispatch'),
    0x24c680: ('entity_initialize', 's,dispatch'),
    0x24c6f0: ('entity_difference', 's,dispatch'),
    0x24ca90: ('entity_apply', 's,dispatch'),
    0x24a620: ('overtime', 's'),
    0x24a130: ('carrier_speed', 's,dispatch'),
    0x24a290: ('carrier_event', 's,dispatch'),
    0x24a1e0: ('carrier_statistic', 's,dispatch'),
    0x24b3c0: ('carrier_indicator', 's,dispatch'),
    0x24b800: ('planted_goal_team', 's,dispatch'),
    0x249600: ('player_spawn', 's,dispatch'),
    0x24a6f0: ('spawn_filter', 's,dispatch'),
    0x24ce00: ('select_marker', 's,dispatch'),
    0x2496b0: ('player_tick', 's,dispatch'),
    0x24c540: ('announce_selected_team', 's,dispatch'),
    0x24c0f0: ('announce_capture', 's,dispatch'),
    0x24abf0: ('allied_carrier', 's,dispatch'),
    0x24d530: ('initialize_region', 's,dispatch'),
    0x24d920: ('draw_goal_marker', 's,dispatch'),
    0x249a40: ('select_carrier_leader', 's,dispatch'),
    0x24b6e0: ('reconcile_registry', 's,dispatch'),
    0x24a2f0: ('objective_picked_up', 's,dispatch'),
    0x24a4b0: ('objective_dropped', 's,dispatch'),
    0x24c490: ('update_overtime_latch', 's,dispatch'),
    0x24a070: ('round_tick', 's,dispatch'),
    0x24c390: ('announce_capture_blocked', 's,dispatch'),
    0x24d390: ('append_region', 's,dispatch'),
    0x24d1c0: ('initialize_region_style', 's,dispatch'),
    0x24c1e0: ('goal_at_position', 's,dispatch'),
    0x24d5f0: ('player_near_objective', 's,dispatch'),
    0x24d970: ('inside_arming_goal', 's,dispatch'),
    0x24dae0: ('reconcile_home_positions', 's,dispatch'),
    0x24cef0: ('calculate_goal_extents', 's,dispatch'),
    0x24be90: ('plant_bomb', 's,dispatch'),
    0x24a910: ('objective_touched', 's,dispatch'),
    0x24aa90: ('draw_assault_goals', 's,dispatch'),
    0x24b950: ('return_objective', 's,dispatch'),
    0x249b20: ('objective_tick', 's,dispatch'),
    0x24acb0: ('objective_indicators', 's,dispatch'),
}


def inject_native_objective_owners(source):
    marker='static void run_25c4cf(WorldState &s) {\n'
    guard='if(source_objective_output_tu15_detail::waypoint(s))return;\n'
    if marker in source and marker+guard not in source:
        source=source.replace(marker,marker+guard,1)
        source=source.replace('#include "gameplay/multiplayer/source_objective_native_tu15_internal.hpp"\n',
            '#include "gameplay/multiplayer/source_objective_native_tu15_internal.hpp"\n#include "gameplay/multiplayer/source_objective_output_tu15_internal.hpp"\n',1)
    for entry,name in [('231064','quad'),('13bda5','emblem'),('41480','goal'),('124eb0','font')]:
        marker=f'static void run_{entry}(WorldState &s) {{\n'
        guard=f'if(source_objective_output_tu15_detail::{name}(s))return;\n'
        if marker in source and marker+guard not in source:
            source=source.replace(marker,marker+guard,1)
    active=[]
    for entry,(name,args) in OWNERS.items():
        marker=f'static void run_{entry:x}(WorldState &s) {{\n'
        if marker not in source:
            continue
        assert source.count(marker)==1
        guard=f'if(source_objective_native_tu15_detail::{name}({args}))return;\n'
        if marker+guard not in source:
            assert guard not in source
            source=source.replace(marker,marker+guard,1)
        active.append(entry)
    if active:
        include='#include "gameplay/multiplayer/source_objective_native_tu15_internal.hpp"\n'
        if include not in source:
            source=include+source
    if marker+guard in source:
        include='#include "gameplay/multiplayer/source_objective_output_tu15_internal.hpp"\n'
        if include not in source:
            source=include+source
    return source
