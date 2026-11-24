# Remaining Translated Execution Checklist (2026-11-19)

This document is the actionable `.MD` checklist for agents continuing the Halo 2 TU1.5 translated-to-native replacement effort. See [REMAINING_TRANSLATED_20261115.md](REMAINING_TRANSLATED_20261115.md) for the historical progress log across earlier waves.

## Ownership change, 2026-11-23

The user handed the remaining native replacement to the Claude performance
session (session `halo-2-decompiled-c9`): "take over and get it done". It
works from multiplayer Zanzibar, the heaviest MP map, as well as the Armory.
Other agents: before porting an owner, check this section and the progress
log below for claims.

Multiplayer census on 2026-11-23: `build-perf-4k60`, Zanzibar, 400 frames,
1280x720, `HALO2_CENSUS_NATIVE_BRIDGES=1`:

| | 1p | 4p |
|---|---|---|
| Translated instructions per frame | 96,429 | 268,435 |
| Combat-action targeting cluster (`14D200`, `1AE2B0`, `BD460`, `1AE090`) | 6,021 | 59,037 |

The combat cluster is about 22% of 4p translated work and runs per local
player per tick. The next routines are `3D7E0` (shadow frame; 5,401 per frame
in 1p and 10,083 in 4p), `30AA70`, `16B240`, `170730`, `1D30F0` and `333F0`.

Claimed by the Claude performance session, in order: the combat targeting
cluster, then `3D7E0` and `333F0`.

Progress (2026-11-23). Both owners below are proven with
`tools/validate_native_frame_misc_tu15.py --owner NAME` and bound through
`tools/native_frame_misc_replacements.py` (`BOUND`, CTest list in
`cmake/NativeTu15FrameMisc.cmake`).

| Owner | What it is | Files | Switches | Proof |
|---|---|---|---|---|
| `14D200` | Aim cluster flood: the BSP clusters an aim ray sees into through cluster portals (depth-first with a 512-entry frame stack, visit stamp 4F3CE4/4F3CE8, cone test on each portal's bounding sphere) | `src/physics/runtime/native_tu15_aim_clusters_systems.hpp`, `native_tu15_aim_clusters.hpp`, `tests/native_tu15_aim_clusters_tests.cpp` | `HALO2_REFERENCE_AIM_CLUSTERS`, `HALO2_VERIFY_AIM_CLUSTERS` | 8,000 cases (4.7M ordered stores, 800 faults), census, verify, 22/22 branches, ASan/UBSan, 40/40 mutants (`build/test-results/aim-clusters-20261123/validator.json`) |
| `BD460` | Cluster object query: the unique objects a cluster list references in the collideable and/or non-collideable partitions (query stamp 4EABCC/4EABD0) | `native_tu15_cluster_objects_systems.hpp`, `native_tu15_cluster_objects.hpp`, `tests/native_tu15_cluster_objects_tests.cpp` | `HALO2_REFERENCE_CLUSTER_OBJECTS`, `HALO2_VERIFY_CLUSTER_OBJECTS` | 8,000 cases (682 faults), census, verify, 30/30 branches, ASan/UBSan, 38/38 mutants (`validator-bd460.json`) |

| `1AE2B0` | Aim target chain: an object, its siblings and (for targets) its attached objects; candidate types, unit limit, the 14D200 sphere cone test, 1AE500/1AE7F0, self-recursive | `native_tu15_aim_targets_systems.hpp`, `native_tu15_aim_targets.hpp`, `tests/native_tu15_aim_targets_tests.cpp` | `HALO2_REFERENCE_AIM_TARGETS`, `HALO2_VERIFY_AIM_TARGETS` | 8,000 cases, 25/25 branches, 53/53 mutants |
| `1AE090` | Weapon targeting search: widest angle/farthest range, x87 FSIN/FCOS cone, 14D200, BD460, players' outermost parents, 1AE2B0 per object | `native_tu15_aim_search_systems.hpp`, `native_tu15_aim_search.hpp`, `tests/native_tu15_aim_search_tests.cpp` | `HALO2_REFERENCE_AIM_SEARCH`, `HALO2_VERIFY_AIM_SEARCH` | 8,000 cases, 30/30, 48/48 |
| `3D7E0` | Object shadow fade: x87 camera distance, 4BAE0 level, age fade, camouflage, parent recursion, unit camouflage | `src/rendering/shadows/native_tu15_shadow_fade{_systems,}.hpp`, `tests/native_tu15_shadow_fade_tests.cpp` | `HALO2_REFERENCE_SHADOW_FADE`, `HALO2_VERIFY_SHADOW_FADE` | 8,000 cases, 25/25, 60/60 |
| `16B240` | Node matrix gather (REP MOVSD with the direction flag, or 145560 with a transform) | `src/gameplay/objects/native_tu15_node_gather{_systems,}.hpp`, `tests/native_tu15_node_gather_tests.cpp` | `HALO2_REFERENCE_NODE_GATHER`, `HALO2_VERIFY_NODE_GATHER` | 8,000 cases, 8/8, 28/28 |
| `170730` | Node hierarchy transform (parent into each child's matrix, recursive) | `native_tu15_node_hierarchy{_systems,}.hpp`, `tests/native_tu15_node_hierarchy_tests.cpp` | `HALO2_REFERENCE_NODE_HIERARCHY`, `HALO2_VERIFY_NODE_HIERARCHY` | 8,000 cases, 10/10, 22/22 |
| `1D30F0` | Cluster partition removal (references and owned chain entries freed through 16E8A0) | `native_tu15_partition_removal{_systems,}.hpp`, `tests/native_tu15_partition_removal_tests.cpp` | `HALO2_REFERENCE_PARTITION_REMOVAL`, `HALO2_VERIFY_PARTITION_REMOVAL` | 8,000 cases, 10/10, 20/20 |
| `1E5250` | Keyed record iterator (four stages of two-level 1E5810 lookups, jump table) | `native_tu15_record_iterator{_systems,}.hpp`, `tests/native_tu15_record_iterator_tests.cpp` | `HALO2_REFERENCE_RECORD_ITERATOR`, `HALO2_VERIFY_RECORD_ITERATOR` | 8,000 cases, 13/13, 26/26 |
| `333F0` | Shadow caster collection (mode filter, 3D7E0, projected size, 3E9B0, 4A7B0/4AC20) | `src/rendering/shadows/native_tu15_shadow_casters{_systems,}.hpp`, `tests/native_tu15_shadow_casters_tests.cpp` | `HALO2_REFERENCE_SHADOW_CASTERS`, `HALO2_VERIFY_SHADOW_CASTERS` | 8,000 cases, 19/19, 41/41 |
| `15C4E0` | Multiplayer object cleanup (30-tick slots, item/corpse/other timeouts, BBBC0/BA570) | `src/gameplay/multiplayer/native_tu15_object_cleanup{_systems,}.hpp`, `tests/native_tu15_object_cleanup_tests.cpp` | `HALO2_REFERENCE_OBJECT_CLEANUP`, `HALO2_VERIFY_OBJECT_CLEANUP` | 8,000 cases, 30/30, 46/46 |
| `3F3E0` | Index marks (the bit table of a list's indices, count flags) | `src/rendering/effects/native_tu15_index_marks{_systems,}.hpp` | `HALO2_*_INDEX_MARKS` | 8,000 cases, full branches, all mutants |
| `BCFE0` | Object ancestry (whether one object is within another's parent chain) | `src/physics/runtime/native_tu15_object_ancestry{_systems,}.hpp` | `HALO2_*_OBJECT_ANCESTRY` | 8,000 cases, full branches, all mutants |
| `17FAF0` | Effect chain byte (the group byte of an effect chain's head, 17FB60 accept) | `src/gameplay/units/native_tu15_chain_byte{_systems,}.hpp` | `HALO2_*_CHAIN_BYTE` | 8,000 cases, full branches, all mutants |
| `180520` | Effect record walk | `src/gameplay/units/native_tu15_record_walk{_systems,}.hpp` | `HALO2_*_RECORD_WALK` | 8,000 cases, full branches, all mutants |
| `144BF0` | Basis matrix (two units: ai frame, scene device constructor) | `src/physics/runtime/native_tu15_basis_matrix{_systems,}.hpp` | `HALO2_*_BASIS_MATRIX` | both copies validated |
| `179A40` | Marked records (two units: shadow frame, lens flare) | `src/rendering/shadows/native_tu15_marked_records{_systems,}.hpp` | `HALO2_*_MARKED_RECORDS` | 9/9, 17/17, both copies |
| `1A8E50` | Filtered data array iterator (two units: live support, death cycle) | `src/physics/runtime/native_tu15_filtered_iterator{_systems,}.hpp` | `HALO2_*_FILTERED_ITERATOR` | 7/7, 16/16, both copies |
| `1902E0` | Player relation (local players' units in an object's hierarchy; two units) | `src/physics/runtime/native_tu15_player_relation{_systems,}.hpp` | `HALO2_*_PLAYER_RELATION` | 10/10, 21/21, both copies |
| `B1720` | Unit vector check (x87 tolerance; two units) | `src/physics/runtime/native_tu15_unit_vector_check{_systems,}.hpp` | `HALO2_*_UNIT_VECTOR_CHECK` | 8/8, 18/18, both copies |
| `17FB60` | Effect record fade (expiry with the counted chain, fade byte through 32CC0C) | `src/gameplay/units/native_tu15_record_fade{_systems,}.hpp` | `HALO2_*_RECORD_FADE` | 14/14, 26/26 |
| `1D2F10` | Cluster partition insert (14D080 set / 14D0E0 sphere / single cluster; REP MOVS payload) | `src/physics/runtime/native_tu15_partition_insert{_systems,}.hpp` | `HALO2_*_PARTITION_INSERT` | 19/19, 34/34 (a set DF is refused) |
| `17BEB0` | Effect event parts (environment/mode rules, element slots, 17BD40/17BB10) | `src/gameplay/units/native_tu15_effect_parts{_systems,}.hpp` | `HALO2_*_EFFECT_PARTS` | 25/25, 39/39 |
| `165860` | Player mark counters (per local player, 1A8E50 walk, 53780 gate) | `src/gameplay/multiplayer/native_tu15_player_marks{_systems,}.hpp` | `HALO2_*_PLAYER_MARKS` | 19/19, 34/34 |

| `157FE0` | Player directors (per local player request/state, mode jump table through `canonical_branch`, callbacks, timer, 171FF0) | `src/gameplay/vehicles/native_tu15_player_directors{_systems,}.hpp` | `HALO2_*_PLAYER_DIRECTORS` | 20/20, 36/36 (a set DF is refused) |
| `E9530` | Object flag bit (bit block at the int16 +0x346) | `src/physics/runtime/native_tu15_object_flag_bit{_systems,}.hpp` | `HALO2_*_OBJECT_FLAG_BIT` | 2/2, 9/9 |
| `10E1F0` | Object type callbacks (+0x74 of each chained type) | `native_tu15_object_type_callbacks{_systems,}.hpp` | `HALO2_*_OBJECT_TYPE_CALLBACKS` | 5/5, 11/11 |
| `125790` | Tag iterator (next tag of a group, 125810 parent groups) | `native_tu15_tag_iterator{_systems,}.hpp` | `HALO2_*_TAG_ITERATOR` | 11/11, 13/13 |
| `1E1390` | Node chain pose (x87 weight test on the logical stack, SSE blend, 122BE0/145560/170730) | `native_tu15_node_chain{_systems,}.hpp` | `HALO2_*_NODE_CHAIN` | 5/5, 27/27 |
| `116060` | Marker chains (keyed record walk, BAD60 markers, 1E1390 per node) | `native_tu15_marker_chains{_systems,}.hpp` | `HALO2_*_MARKER_CHAINS` | 15/15, 32/32 |
| `AA200`, `AA270` | Tolerance checks (unit length, near; x87 FABS/FCOMIP), three units each | `native_tu15_tolerance_checks{_systems,}.hpp` | `HALO2_*_TOLERANCE_CHECKS` | 4/4 each, 11/11 and 10/10, all copies |
| `B9390` | Wake chain (header awake flag up the parent chain), three units | `src/gameplay/objects/native_tu15_wake_chain{_systems,}.hpp` | `HALO2_*_WAKE_CHAIN` | 5/5, 10/10 |
| `1751B0` | World bounds (each coordinate finite and within 44EA9C..44EAA0) | `native_tu15_world_bounds{_systems,}.hpp` | `HALO2_*_WORLD_BOUNDS` | 4/4, 10/10 |
| `E65A0` | Unit readiness (flags, FISTP-rounded tick threshold, state, parent) | `native_tu15_unit_ready{_systems,}.hpp` | `HALO2_*_UNIT_READY` | 7/7, 15/15 |

Live checks of the block 157FE0..E65A0 (2026-11-23, binary 897f1296): Armory
three roles identical (frame `acd375e77ffb11c9`); MP reference vs verify 8/8
identical, each owner compared thousands of live calls with no mismatch.

| `106230` | Slot kinds | `src/physics/runtime/native_tu15_slot_kinds{_systems,}.hpp` | `HALO2_*_SLOT_KINDS` | 4/4, 10 mutants |
| `1D9F50` | Entry kind (virtual call through the route) | `native_tu15_entry_kind{_systems,}.hpp` | `HALO2_*_ENTRY_KIND` | 6/6, 12 |
| `144CB0` | Matrix reset (144910 dispatched) | `native_tu15_matrix_reset{_systems,}.hpp` | `HALO2_*_MATRIX_RESET` | 1/1, 9 (no live calls in the checked scenes) |
| `BAC30` | Node matrices (count and address) | `native_tu15_node_matrices{_systems,}.hpp` | `HALO2_*_NODE_MATRICES` | 1/1, 8 |
| `10E340` | Object type events | `native_tu15_object_type_events{_systems,}.hpp` | `HALO2_*_OBJECT_TYPE_EVENTS` | 5/5, 11 |
| `BE7E0`, `BDA50` | Wake children, wake tree | `src/gameplay/objects/native_tu15_wake_{children,tree}.hpp`, `native_tu15_wake_tree_systems.hpp` | `HALO2_*_WAKE_TREE` | 5/5 and 4/4, 10 each |
| `1CE810` | Animation frame clamp | `src/gameplay/units/native_tu15_animation_clamp{_systems,}.hpp` | `HALO2_*_ANIMATION_CLAMP` | 9/9, 17 |
| `130D20` | Geometry cache entry (four of five units; the shadow frame copy publishes differently) | `src/rendering/native_tu15_cache_entry{_systems,}.hpp` | `HALO2_*_CACHE_ENTRY` | 5/5, 14 |
| `E0500` | Unit motion (x87 speed chain) | `src/physics/runtime/native_tu15_unit_motion{_systems,}.hpp` | `HALO2_*_UNIT_MOTION` | 11/11, 20 |
| `C0700` | Object function scale (BCB70 dispatched) | `src/rendering/shadows/native_tu15_function_scale{_systems,}.hpp` | `HALO2_*_FUNCTION_SCALE` | 4/4, 14 (no live calls in the checked scenes) |
| `1EEDB0` | Player input (REP MOVSD) | `src/gameplay/players/native_tu15_player_input{_systems,}.hpp` | `HALO2_*_PLAYER_INPUT` | 6/6, 18 |
| `18BBC0` | Packed colour (FISTP channels) | `src/rendering/effects/native_tu15_packed_color{_systems,}.hpp` | `HALO2_*_PACKED_COLOR` | 9/9, 15 |
| `C93D0` | AI state reset (REP STOSD) | `src/campaign/ai/native_tu15_state_reset{_systems,}.hpp` | `HALO2_*_STATE_RESET` | 1/1, 13 |

Live checks of the block 106230..C93D0 (2026-11-23, binary e3a88634): Armory
three roles identical (frame `acd375e77ffb11c9`, trace `df61945af603`); MP
reference vs verify 8/8 identical; the verified owners compared between 450
and 155,000 live calls each with no mismatch.

| `C92C0` | AI state apply (two units) | `src/campaign/ai/native_tu15_state_apply{_systems,}.hpp` | `HALO2_*_STATE_APPLY` | 4/4, 16 |
| `1E1620` | Transition fraction (curve jump table; 110FC0 direct, 1E1920 through each unit's dispatch) | `src/physics/runtime/native_tu15_transition_fraction{_systems,}.hpp` | `HALO2_*_TRANSITION_FRACTION` | 10/10, 25 |
| `110FC0`, `1E1920` | Smoothstep, smootherstep (clamped to 0..1; upper lanes carried) | `native_tu15_smooth_curves_systems.hpp`, `native_tu15_smooth{step,erstep}.hpp` | `HALO2_*_SMOOTH_CURVES` | 3/3, 10 each |
| `11FA50` | Perpendicular (x87 horizontal length, SSE cross; live support copy only, the AI frame copy spills through `native_single_spill`) | `native_tu15_perpendicular{_systems,}.hpp` | `HALO2_*_PERPENDICULAR` | 3/3, 19 |
| `DFD20` | Unit speed scale (2647E0 adjusts the slot) | `native_tu15_unit_speed_scale{_systems,}.hpp` | `HALO2_*_UNIT_SPEED_SCALE` | 12/12, 26 |
| `1A8F10` | Linked iterator (the filtered iterator's logic, generalised over its filter; 1A8E50 re-validated) | `native_tu15_linked_iterator.hpp`, `native_tu15_filtered_iterator_systems.hpp` | `HALO2_*_LINKED_ITERATOR` | 6/6, 10 |
| `175300` | Placement check (seven part checks in order) | `native_tu15_placement_check{_systems,}.hpp` | `HALO2_*_PLACEMENT_CHECK` | 10/10, 16 |

Live checks of the block C92C0..175300 (2026-11-23, binary 9d64bfc5): Armory
three roles identical; MP reference vs verify 8/8 identical; every owner but
the smootherstep (no curve 2 in the checked scenes) compared between 16,000
and 179,000 live calls with no mismatch.

| `175280`, `175240`, `1752C0` | Scalar range checks (finite and within each range) | `native_tu15_finite_checks_systems.hpp`, `native_tu15_scalar_range.hpp` | `HALO2_*_SCALAR_RANGE` | 4/4 each, 8, 6 and 4 |
| `B16C0` | Finite vector | `native_tu15_finite_vector.hpp` | `HALO2_*_FINITE_VECTOR` | 4/4, 7 |
| `17C5C0` | Effect activity (two units; all four children through each unit's dispatch) | `src/gameplay/units/native_tu15_effect_activity{_systems,}.hpp` | `HALO2_*_EFFECT_ACTIVITY` | 10/10, 21 |
| `1AE500` | Action target check (1AE5E0 direct, 1E7810 dispatched) | `src/gameplay/units/native_tu15_action_target_check{_systems,}.hpp` | `HALO2_*_ACTION_TARGET_CHECK` | 6/6, 20 |
| `212C70` | Object model view (two units; BC190 dispatched) | `src/rendering/models/native_tu15_object_model_view{_systems,}.hpp` | `HALO2_*_OBJECT_MODEL_VIEW` | 3/3, 18 |
| `30C40` | Object origin | `src/rendering/shadows/native_tu15_object_origin{_systems,}.hpp` | `HALO2_*_OBJECT_ORIGIN` | 1/1, 8 |
| `11E060` | Kind entry (jump table) | `src/physics/runtime/native_tu15_kind_entry{_systems,}.hpp` | `HALO2_*_KIND_ENTRY` | 6/6, 11 |
| `D2330` | Flagged weapon slot | `src/gameplay/weapons/native_tu15_flagged_slot{_systems,}.hpp` | `HALO2_*_FLAGGED_SLOT` | 4/4, 9 |
| `105950` | Active entry | `src/physics/runtime/native_tu15_active_entry{_systems,}.hpp` | `HALO2_*_ACTIVE_ENTRY` | 5/5, 12 |

Live checks of the block 175280..105950 (2026-11-23, binary 34872ded):
Armory three roles identical; MP reference vs verify 8/8 identical; every
group compared between 8,000 and 131,000 live calls with no mismatch.

| `10DDE0` | Object type query chain (+0x48 queries until one answers) | `src/physics/runtime/native_tu15_object_type_query_chain{_systems,}.hpp` | `HALO2_*_OBJECT_TYPE_QUERY_CHAIN` | 4/4, 11 |
| `10B0F0` | Linked object query (15C7F0) | `native_tu15_linked_object_query{_systems,}.hpp` | `HALO2_*_LINKED_OBJECT_QUERY` | 6/6, 12 |
| `1D3F90` | Record iterator start (1E5250 dispatched) | `src/gameplay/objects/native_tu15_record_iterator_start{_systems,}.hpp` | `HALO2_*_RECORD_ITERATOR_START` | 1/1, 9 |
| `16E290` | Object state bits | `native_tu15_object_state_bits{_systems,}.hpp` | `HALO2_*_OBJECT_STATE_BITS` | 6/6, 14 |
| `B8D90` | Object cluster test (machines through 10DDE0) | `native_tu15_object_cluster_test{_systems,}.hpp` | `HALO2_*_OBJECT_CLUSTER_TEST` | 5/5, 13 |
| `BBFF0` | Object axes (two units; the parent's node matrix) | `native_tu15_object_axes{_systems,}.hpp` | `HALO2_*_OBJECT_AXES` | 5/5, 16 |
| `CAAA0` | Unit free test (E9530, 105950) | `src/gameplay/units/native_tu15_unit_free{_systems,}.hpp` | `HALO2_*_UNIT_FREE` | 6/6, 15 |

Live checks of the block 10DDE0..CAAA0 (2026-11-23, binary 7e6f2070):
Armory three roles identical; MP reference vs verify 8/8 identical; every
owner compared between 6,000 and 52,000 live calls with no mismatch.

| `14FBC0` | Record sanity check | `src/physics/runtime/native_tu15_record_sanity{_systems,}.hpp` | `HALO2_*_RECORD_SANITY` | 3/3, 20 |
| `CE230`, `CFB40` | Slot handle, both slots (two units each) | `native_tu15_object_slots_systems.hpp`, `native_tu15_{slot_handle,both_slots}.hpp` | `HALO2_*_OBJECT_SLOTS` | 2/2 and 3/3, 7 and 6 |
| `13F620`, `11E780`, `E3310`, `15DD10` | Game hold, cluster visibility, unit override, session variant | `native_tu15_small_queries_systems.hpp`, `native_tu15_{game_hold,cluster_visible,unit_override,session_variant}.hpp` | `HALO2_*_SMALL_QUERIES` | 2/2, 2/2, 4/4, 5/5 |

Live checks of the block 14FBC0..15DD10 (2026-11-23, binary 5aab663e):
Armory three roles identical; MP reference vs verify 8/8 identical; every
owner compared between 16,000 and 119,000 live calls with no mismatch.

| `BC330`, `BBD50` | Object root words, root flag | `native_tu15_object_root_systems.hpp`, `native_tu15_{root_words,root_flag}.hpp` | `HALO2_*_OBJECT_ROOT` | 3/3, 3/3 |
| `15C7F0` | Team query (session variant, mode 7, team mask) | `src/physics/runtime/native_tu15_team_query{_systems,}.hpp` | `HALO2_*_TEAM_QUERY` | 5/5 |
| `DF820` | Code class (compare tree) | `src/physics/runtime/native_tu15_code_class{_systems,}.hpp` | `HALO2_*_CODE_CLASS` | 4/4 |
| `111D10` | Animation words (live support and AI frame units) | `src/gameplay/units/native_tu15_animation_words{_systems,}.hpp` | `HALO2_*_ANIMATION_WORDS` | 4/4 |
| `C5250` | Partition unlink (through 1D30F0) | `src/physics/runtime/native_tu15_partition_unlink{_systems,}.hpp` | `HALO2_*_PARTITION_UNLINK` | 2/2 |
| `17C330`, `C9590`, `1E5D60` | Header unanswered (BBD10 through dispatch in both units), special animation (two units), entry refresh | `native_tu15_small_calls_systems.hpp`, `native_tu15_{header_unanswered,special_animation,entry_refresh}.hpp` | `HALO2_*_SMALL_CALLS` | 3/3 each |
| `BA910` | Object summary (state bits through 16E290) | `src/gameplay/objects/native_tu15_object_summary{_systems,}.hpp` | `HALO2_*_OBJECT_SUMMARY` | 2/2 |

Live checks of `BC330`..`111D10` (2026-11-23, binary 2c8295db): Armory three
roles identical; MP reference vs verify 8/8 identical; between 20,000 and
62,000 live calls each with no mismatch. A first build of this round failed
to link on `path_tracing::Render::record_acceleration` while a peer's
renderer edit was mid-write; the rebuild linked. `C5250`..`BA910` are bound
in the next round (validators passed).

Live checks of `C5250`..`BA910` (2026-11-23, binary da820cd5): Armory three
roles identical; MP reference vs verify 8/8 identical; between 2,300 (entry
refresh) and 87,000 (partition unlink) live calls each with no mismatch.

| `D2B00` | Unit chain step (the next biped or vehicle along an object chain) | `src/gameplay/objects/native_tu15_unit_chain{_systems,}.hpp` | `HALO2_*_UNIT_CHAIN` | 4/4 |
| `1062A0` | Entry kind query (combat action unit) | `src/gameplay/units/native_tu15_entry_kind{_systems,}.hpp` | `HALO2_*_ENTRY_KIND` | 4/4 |
| `1CD620` | Cluster notice (flags 1, 7, 8 and two byte arguments, through 1DC270) | `src/physics/runtime/native_tu15_cluster_notify{_systems,}.hpp` | `HALO2_*_CLUSTER_NOTIFY` | 6/6 |
| `11E600` | Point location (leaf through 14CDB0, cluster, BSP index) | `src/physics/runtime/native_tu15_point_location{_systems,}.hpp` | `HALO2_*_POINT_LOCATION` | 3/3 |
| `152E00` | Player biped query (through E65A0) | `src/gameplay/units/native_tu15_player_biped{_systems,}.hpp` | `HALO2_*_PLAYER_BIPED` | 6/6 |
| `106040` | Inactive choice query (through 105950) | `src/gameplay/objects/native_tu15_inactive_choice{_systems,}.hpp` | `HALO2_*_INACTIVE_CHOICE` | 3/3 |
| `115FF0`, `1E15D0` | Timer sweep (five byte timers through 1E15D0) and the byte timer step | `native_tu15_timer_sweep{_systems,}.hpp`, `native_tu15_byte_timer{_systems,}.hpp` | `HALO2_*_TIMER_SWEEP`, `HALO2_*_BYTE_TIMER` | 2/2, 6/6 |
| `C6E50`, `15B470` | Uncontrolled unit (through C6DD0), player live (two units, through 160570) | `native_tu15_player_queries_systems.hpp`, `native_tu15_{uncontrolled_unit,player_live}.hpp` | `HALO2_*_UNCONTROLLED_UNIT`, `HALO2_*_PLAYER_LIVE` | 4/4, 7/7 |
| `1F0F90` | Repeat tracker | `src/gameplay/objects/native_tu15_repeat_tracker{_systems,}.hpp` | `HALO2_*_REPEAT_TRACKER` | 4/4 |
| `17E100` | Role check (death cycle unit) | `src/gameplay/units/native_tu15_role_check{_systems,}.hpp` | `HALO2_*_ROLE_CHECK` | 6/6 |
| `161560` | Team admission (multiplayer engines unit) | `src/gameplay/multiplayer/native_tu15_team_admission{_systems,}.hpp` | `HALO2_*_TEAM_ADMISSION` | 4/4 |

Live checks of `D2B00`..`161560` (2026-11-23, binary 7e7a3e14): Armory three
roles identical; MP reference vs verify 8/8 identical; byte timer 221,810,
point location 86,569, cluster notice 44,555, entry kind 27,100 live calls
down to the role check's 3, with no mismatch (the unit chain step ran no
live call in these scenarios).

| `1CEE60` | Indexed tag reference | `src/physics/runtime/native_tu15_indexed_tag{_systems,}.hpp` | `HALO2_*_INDEXED_TAG` | 4/4 |
| `1059C0` | Seat sum (four unrolled seats, separate last-seat epilogue, through 10B0F0) | `src/gameplay/units/native_tu15_seat_sum{_systems,}.hpp` | `HALO2_*_SEAT_SUM` | 11/11 |
| `144210`, `1452F0` | Orientation frame built from forward and up; vector projected onto a frame's axes (scalar SSE) | `native_tu15_orientation_frame_systems.hpp`, `native_tu15_{orientation_frame,frame_projection}.hpp` | `HALO2_*_ORIENTATION_FRAME` | 1/1, 2/2 |
| `BA7E0` | Object disconnect (C0F60, 1D30F0 on one of two partitions) | `src/physics/runtime/native_tu15_object_disconnect{_systems,}.hpp` | `HALO2_*_OBJECT_DISCONNECT` | 2/2 |
| `B92C0` | Object activation (two units, 1CC3F0 through dispatch) | `src/physics/runtime/native_tu15_object_activate{_systems,}.hpp` | `HALO2_*_OBJECT_ACTIVATE` | 6/6 |
| `D3A80` | Seat pair check (106710 twice, E8E10) | `src/gameplay/units/native_tu15_seat_pair{_systems,}.hpp` | `HALO2_*_SEAT_PAIR` | 4/4 |
| `31EDF0`, `33210C`, `161530`, `1D4140`, `116480`, `E32C0` | Tiny leaves: six, double exponent, variant option, definition word, block ready, tick seconds | `native_tu15_tiny_leaves_systems.hpp` and one adapter each | `HALO2_*_TINY_LEAVES` | all branches |
| `CB440` | Child unit search | `src/gameplay/objects/native_tu15_child_unit{_systems,}.hpp` | `HALO2_*_CHILD_UNIT` | 3/3 |

Live checks of `1CEE60`..`CB440` (2026-11-23, binary f0a0f54f): Armory three
roles identical; MP reference vs verify 8/8 identical; object activation
47,736, object disconnect 46,469, indexed tag 40,800, seat pair 19,476,
six 15,504 live calls down to the frame projection's 1,051, with no
mismatch.

| `172200`, `BCF70`, `111F00` | Indexed record, attachment root (three units), ready slot | `native_tu15_object_leaves_systems.hpp`, `native_tu15_{indexed_record,attachment_root,ready_slot}.hpp` | `HALO2_*_OBJECT_LEAVES` | 3/3 each |
| `140EC0` | Bit vector union | `src/physics/runtime/native_tu15_bit_union{_systems,}.hpp` | `HALO2_*_BIT_UNION` | 2/2 |
| `105FD0` | Charge gate (COMISS against 1) | `src/gameplay/units/native_tu15_charge_gate{_systems,}.hpp` | `HALO2_*_CHARGE_GATE` | 6/6 |
| `C8C20` | Unit countdowns (1D1F30, D2500 through dispatch) | `src/gameplay/units/native_tu15_unit_countdowns{_systems,}.hpp` | `HALO2_*_UNIT_COUNTDOWNS` | 5/5 |
| `113C70` | Animation mask query (1E3150 twice) | `src/gameplay/units/native_tu15_animation_mask{_systems,}.hpp` | `HALO2_*_ANIMATION_MASK` | 3/3 |
| `1E5160` | Record cursor step (1E5C80) | `src/physics/runtime/native_tu15_record_cursor{_systems,}.hpp` | `HALO2_*_RECORD_CURSOR` | 3/3 |

Live checks of `172200`..`1E5160` (2026-11-23, round 58): Armory three roles
identical; MP reference vs verify 8/8 identical; indexed record 45,124,
attachment root 34,191, unit countdowns 23,090, animation mask 17,200,
ready slot 16,050 live calls down to the charge gate's 5,167, with no
mismatch.

| `1E5DF0` | Animation record (1E3150, 1E5A70; two units) | `src/gameplay/units/native_tu15_animation_record{_systems,}.hpp` | `HALO2_*_ANIMATION_RECORD` | 2/2 |
| `1CF070` | Reference pose (a stack record through 1E5DF0, then 212D40) | `src/gameplay/units/native_tu15_reference_pose{_systems,}.hpp` | `HALO2_*_REFERENCE_POSE` | 2/2 |
| `1CEFB0` | Scaled reference pose (the argument slot reused for floats, CVTTSS2SI, scalar SSE) | `src/gameplay/units/native_tu15_scaled_pose{_systems,}.hpp` | `HALO2_*_SCALED_POSE` | 2/2 |

Live checks of `1E5DF0`..`1CEFB0` (2026-11-23, round 59): Armory three roles
identical; MP reference vs verify 8/8 identical; about 16,500 live calls
each, with no mismatch.

| `1E5A70` | Animation entry value (two units, through 1262F0) | `src/gameplay/units/native_tu15_animation_entry{_systems,}.hpp` | `HALO2_*_ANIMATION_ENTRY` | 4/4 |
| `BBF20` | Root point (root's cluster entry, or BBE00) | `src/gameplay/objects/native_tu15_root_point{_systems,}.hpp` | `HALO2_*_ROOT_POINT` | 4/4 |

Live checks of `1E5A70` and `BBF20` (2026-11-23, round 60): Armory three
roles identical; MP reference vs verify 8/8 identical; 16,606 and 6,718
live calls, with no mismatch.

| `1D4E50` | Channel reset (three 32-byte channels; two units) | `src/gameplay/units/native_tu15_channel_reset{_systems,}.hpp` | `HALO2_*_CHANNEL_RESET` | 1/1 |
| `1FA110` | Root notice (player roots, 184EC0 through dispatch) | `src/gameplay/objects/native_tu15_root_notice{_systems,}.hpp` | `HALO2_*_ROOT_NOTICE` | 4/4 |
| `102910` | Flagged object visit (a stack iterator through BCEE0, 102980) | `src/gameplay/objects/native_tu15_flagged_objects{_systems,}.hpp` | `HALO2_*_FLAGGED_OBJECTS` | 3/3 |

Live checks of `1D4E50`, `1FA110` and `102910` (2026-11-23, round 61):
Armory three roles identical; MP reference vs verify 8/8 identical; 20,216,
16,490 and 10,500 live calls, with no mismatch.

| `1E5F20` | Animation fallbacks (five 1E5E40 lookups) | `src/gameplay/units/native_tu15_animation_fallbacks{_systems,}.hpp` | `HALO2_*_ANIMATION_FALLBACKS` | 3/3 |
| `1E7810` | Team hostility (alliance bits through C1E90, 160A60 through dispatch; two units) | `src/gameplay/multiplayer/native_tu15_team_hostility{_systems,}.hpp` | `HALO2_*_TEAM_HOSTILITY` | 5/5 |
| `212D40` | Pose sample (8/12/16-byte frames) | `src/gameplay/units/native_tu15_pose_sample{_systems,}.hpp` | `HALO2_*_POSE_SAMPLE` | 4/4 |

Live checks of `1E5F20`, `1E7810` and `212D40` (2026-11-24, round 62):
Armory three roles identical; MP reference vs verify 8/8 identical; 4,398,
18,727 and 16,484 live calls, with no mismatch.

| `1E5E40` | Animation select (three 1E5810 lookups, 126200 per listed item; three units) | `src/gameplay/units/native_tu15_animation_select{_systems,}.hpp` | `HALO2_*_ANIMATION_SELECT` | 5/5 |
| `1521B0` | Input edges (new buttons, trigger pressed/released, latches) | `src/gameplay/players/native_tu15_input_edges{_systems,}.hpp` | `HALO2_*_INPUT_EDGES` | 10/10 |
| `152D60` | Flat axes (forward via 30BD0 with fallbacks, left) | `src/gameplay/units/native_tu15_flat_axes{_systems,}.hpp` | `HALO2_*_FLAT_AXES` | 3/3 |

Live checks of `1E5E40`, `1521B0` and `152D60` (2026-11-24, round 63):
Armory three roles identical; MP reference vs verify 8/8 identical; 21,990,
16,024 and 11,170 live calls, with no mismatch.

| `C8CF0` | Unit camouflage (forced in unallied team games via C1E90; fades; B7E50 notice) | `src/gameplay/units/native_tu15_unit_camouflage{_systems,}.hpp` | `HALO2_*_UNIT_CAMOUFLAGE` | 7/7 |
| `1BE90` | Pixel scales (shadow frame unit) | `src/rendering/shadows/native_tu15_pixel_scales{_systems,}.hpp` | `HALO2_*_PIXEL_SCALES` | 2/2 |

Live checks of `C8CF0` and `1BE90` (2026-11-24, round 64): Armory three
roles identical; MP reference vs verify 8/8 identical; 23,090 and 15,874 live
calls, with no mismatch.

| `2A0100` | Animation wildcard search (three units; a bridge per pin: the live support copy asks the MCC class symbol) | `src/gameplay/units/native_tu15_animation_wildcard.hpp` | `HALO2_*_ANIMATION_WILDCARD` | 9/9 |
| `2DB80` | Lens request (lens flare unit; 2E210, 20990, 20B20) | `src/rendering/effects/native_tu15_lens_request.hpp` | `HALO2_*_LENS_REQUEST` | 8/8 |
| `1D3930` | Model switch (by distance) | `src/gameplay/objects/native_tu15_model_switch.hpp` | `HALO2_*_MODEL_SWITCH` | 3/3 |
| `1DC610` | Record frame (1D8B90 or BC190 through dispatch, then 1DC680) | `src/gameplay/objects/native_tu15_record_frame.hpp` | `HALO2_*_RECORD_FRAME` | 3/3 |
| `BC190` | Object frame (scene device constructor unit; 144BF0, 145560) | `src/gameplay/objects/native_tu15_object_frame.hpp` | `HALO2_*_OBJECT_FRAME` | 2/2 |
| `10C5D0` | Device tree (recursing through its own bridge; B9390) | `src/gameplay/objects/native_tu15_device_tree.hpp` | `HALO2_*_DEVICE_TREE` | 3/3 |
| `DFE30` | Object rate (DFD20, E65A0, E3310) | `src/gameplay/objects/native_tu15_object_rate.hpp` | `HALO2_*_OBJECT_RATE` | 5/5 |
| `1E5A50` | Reference clear (sandbox special unit) | `src/gameplay/native_tu15_reference_clear.hpp` | `HALO2_*_REFERENCE_CLEAR` | 1/1 |
| `335B0` | Shadow option (shadow frame unit) | `src/gameplay/native_tu15_shadow_option.hpp` | `HALO2_*_SHADOW_OPTION` | 3/3 |
| `150990` | Seat player (combat action and sandbox special units) | `src/gameplay/native_tu15_seat_player.hpp` | `HALO2_*_SEAT_PLAYER` | 2/2 |
| `25BC70` | Word change (multiplayer engines unit) | `src/gameplay/native_tu15_word_change.hpp` | `HALO2_*_WORD_CHANGE` | 2/2 |
| `226B0` | Cluster match (shadow frame unit) | `src/gameplay/native_tu15_cluster_match.hpp` | `HALO2_*_CLUSTER_MATCH` | 3/3 |

Live checks of `2A0100`..`226B0` (2026-11-24, round 65): Armory three roles
identical; MP reference vs verify 8/8 identical; live calls 96,584 animation
wildcard, 47,570 object frame, 36,031 model switch, 26,808 record frame,
21,000 seat player, 17,678 word change, 16,490 object rate, 13,868 reference
clear, 6,660 device tree, with no mismatch (lens request, shadow option and
cluster match were not reached by these scenarios).

| `189F20` | Director value (vehicle director unit; CAD60) | `src/gameplay/vehicles/native_tu15_director_value.hpp` | `HALO2_*_DIRECTOR_VALUE` | 4/4 |
| `158450` | Seat camera mode (158340 through dispatch, 158900) | `src/gameplay/vehicles/native_tu15_seat_camera_mode.hpp` | `HALO2_*_SEAT_CAMERA_MODE` | 4/4 |
| `25BCC0` | Option flag (multiplayer engines unit) | `src/gameplay/multiplayer/native_tu15_option_flag.hpp` | `HALO2_*_OPTION_FLAG` | 4/4 |
| `D2B60` | Carrier (the biped or vehicle carrying an object) | `src/gameplay/objects/native_tu15_carrier.hpp` | `HALO2_*_CARRIER` | 4/4 |
| `1D9F10` | Item notice (death cycle and live support units; 315CB0 through dispatch) | `src/gameplay/objects/native_tu15_item_notice.hpp` | `HALO2_*_ITEM_NOTICE` | 3/3 |
| `43980` | Stamp advance (combat action unit; 13FD10) | `src/gameplay/native_tu15_stamp_advance.hpp` | `HALO2_*_STAMP_ADVANCE` | 2/2 |
| `12F0C0` | Framed stamp advance (12F040, 13FD10) | `src/gameplay/native_tu15_stamp_framed.hpp` | `HALO2_*_STAMP_FRAMED` | 3/3 |
| `118760` | Datum visit (death cycle unit; 47860 through dispatch) | `src/gameplay/native_tu15_datum_visit.hpp` | `HALO2_*_DATUM_VISIT` | 2/2 |
| `1D6430` | Tagged play (three units; 1D45A0, 1E5FB0) | `src/gameplay/units/native_tu15_tagged_play.hpp` | `HALO2_*_TAGGED_PLAY` | 2/2 |
| `304C50` | Record view (2F4990) | `src/physics/runtime/native_tu15_record_view.hpp` | `HALO2_*_RECORD_VIEW` | 1/1 |
| `1582F0` | Mode action (combat action unit; 158340 through dispatch, 244F80, 245D10) | `src/gameplay/units/native_tu15_mode_action.hpp` | `HALO2_*_MODE_ACTION` | 2/2 |

Live checks of `189F20`..`1582F0` (2026-11-24, round 66): Armory three roles
identical; MP reference vs verify 8/8 identical; live calls 450 framed stamp
advance, 420 tagged play, 300 each director value, seat camera mode and
stamp advance, 4 item notice, with no mismatch (option flag, carrier, datum
visit, record view and mode action were not reached by these scenarios).

| `18A240` | Director player (vehicle director unit) | `src/gameplay/multiplayer/native_tu15_director_player.hpp` | `HALO2_*_DIRECTOR_PLAYER` | 2/2 |
| `171FF0` | Director bind (175380 through dispatch) | `src/gameplay/vehicles/native_tu15_director_bind.hpp` | `HALO2_*_DIRECTOR_BIND` | 2/2 |
| `20700` | Lens broadcast (lens flare unit; 20E30) | `src/rendering/effects/native_tu15_lens_broadcast.hpp` | `HALO2_*_LENS_BROADCAST` | 3/3 |
| `1F33C0` | Page scan | `src/physics/runtime/native_tu15_page_scan.hpp` | `HALO2_*_PAGE_SCAN` | 3/3 |
| `C5170` | Light admits (light portal unit) | `src/pstencil/native_tu15_light_admits.hpp` | `HALO2_*_LIGHT_ADMITS` | 4/4 |
| `31500` | Light gate (light portal unit; 3E9B0) | `src/pstencil/native_tu15_light_gate.hpp` | `HALO2_*_LIGHT_GATE` | 4/4 |
| `10DB40` | Type hooks (hook functions through dispatch by address) | `src/gameplay/objects/native_tu15_type_hooks.hpp` | `HALO2_*_TYPE_HOOKS` | 4/4 |
| `2E9750` | Aligned query (three units) | `src/physics/runtime/native_tu15_aligned_query.hpp` | `HALO2_*_ALIGNED_QUERY` | 1/1 |

Live checks of `18A240`..`2E9750` (2026-11-24, round 67): Armory three roles
identical; MP reference vs verify 8/8 identical; live calls 15,874 lens
broadcast, 15,874 director bind, 15,600 director player, 11,593 light gate,
11,593 light admits, 5,297 aligned query, 3,339 type hooks and 985 page scan,
with no mismatch.

| `243510` | Polygon inside (light portal and live support units) | `src/physics/runtime/native_tu15_polygon_inside.hpp` | `HALO2_*_POLYGON_INSIDE` | 3/3 |
| `16E7F0` | Datum claim (UI script startup and campaign runtime units; a bridge per pin) | `src/gameplay/native_tu15_datum_claim.hpp` | `HALO2_*_DATUM_CLAIM` | 4/4 |
| `2E3250` | Array grow (scene device constructor unit) | `src/rendering/renderer/native_tu15_array_grow.hpp` | `HALO2_*_ARRAY_GROW` | 2/2 |
| `158340` | Seat mode (live support, campaign camera and map world support units; a bridge per pin) | `src/gameplay/units/native_tu15_seat_mode.hpp` | `HALO2_*_SEAT_MODE` | 5/5 |
| `1D9080` | Item light place (scene device constructor unit) | `src/rendering/renderer/native_tu15_item_light_place.hpp` | `HALO2_*_ITEM_LIGHT_PLACE` | 3/3 |
| `1D9100` | Item light aim (scene device constructor unit) | `src/rendering/renderer/native_tu15_item_light_aim.hpp` | `HALO2_*_ITEM_LIGHT_AIM` | 3/3 |
| `3D2B0` | First-person unit (texture camera producer and first-person light pose units) | `src/gameplay/animation/native_tu15_first_person_unit.hpp` | `HALO2_*_FIRST_PERSON_UNIT` | 5/5 |
| `1E5360` | Element search (AI frame and live support units) | `src/campaign/ai/native_tu15_element_search.hpp` | `HALO2_*_ELEMENT_SEARCH` | 5/5 |
| `244F80` | Unit view (combat action unit) | `src/gameplay/units/native_tu15_unit_view.hpp` | `HALO2_*_UNIT_VIEW` | 5/5 |

Live checks of `243510`..`244F80` (2026-11-24, round 68): Armory three roles
identical; MP reference vs verify 8/8 identical; live calls 33,356 seat mode,
17,482 unit view, 15,874 first-person unit, 10,649 polygon inside, 9,999 item
light place, 9,526 datum claim, 436 element search and 327 item light aim,
with no mismatch (array grow was not reached by these scenarios).

| `168D76` | First-person model record (sandbox special unit; 16B240, 145560; REP MOVSD) | `src/gameplay/objects/native_tu15_model_record.hpp` | `HALO2_*_MODEL_RECORD` | 4/4 |
| `158790` | Seat camera death (vehicle director unit; 158450, 246CF0, 158900) | `src/gameplay/vehicles/native_tu15_seat_camera_death.hpp` | `HALO2_*_SEAT_CAMERA_DEATH` | 7/7 |
| `161610` | Seat status (multiplayer engines unit; 1618B0) | `src/gameplay/multiplayer/native_tu15_seat_status.hpp` | `HALO2_*_SEAT_STATUS` | 5/5 |
| `13DA89` | Weapon HUD state (weapon HUD unit; 1055F0) | `src/gameplay/weapons/native_tu15_weapon_hud_state.hpp` | `HALO2_*_WEAPON_HUD_STATE` | 1/1 |
| `1540D0` | Biped action test (combat action unit; 1E7810 through dispatch, E8110, E79C0, 153E10) | `src/gameplay/units/native_tu15_biped_action.hpp` | `HALO2_*_BIPED_ACTION` | 5/5 |
| `15B620` | Team count (multiplayer engines unit; 15B470 through dispatch) | `src/gameplay/multiplayer/native_tu15_team_count.hpp` | `HALO2_*_TEAM_COUNT` | 7/7 |

Live checks of `168D76`..`15B620` (2026-11-24, round 69): Armory three roles
identical; MP reference vs verify 8/8 identical; live calls 62,296 model
record, 17,805 biped action, 15,874 seat camera death, 15,678 weapon HUD
state, 4,800 team count and 4,800 seat status, with no mismatch.

| `1AE5E0` | Deferred action target check (combat action unit; 1AE500, 1AE6F0) | `src/gameplay/units/native_tu15_action_target_defer.hpp` | `HALO2_*_ACTION_TARGET_DEFER` | 5/5 |
| `15B2D0` | Player factor (live support unit; the original's jump table) | `src/gameplay/players/native_tu15_player_factor.hpp` | `HALO2_*_PLAYER_FACTOR` | 6/6 |
| `154D00` | Player timers (multiplayer engines unit; D3340 through dispatch) | `src/gameplay/players/native_tu15_player_timers.hpp` | `HALO2_*_PLAYER_TIMERS` | 3/3 |
| `13E670` | Curve value (lens draw and live support units; 13DE70) | `src/rendering/effects/native_tu15_curve_value.hpp` | `HALO2_*_CURVE_VALUE` | 5/5 |
| `11EF70` | Tag group fixups (live support unit; map load) | `src/physics/runtime/native_tu15_tag_fixups.hpp` | `HALO2_*_TAG_FIXUPS` | 7/7 |
| `1262F0` | Resource acquire (AI frame and live support units; steps per pin) | `src/gameplay/animation/native_tu15_resource_acquire.hpp` | `HALO2_*_RESOURCE_ACQUIRE` | 3/3 |
| `1AE6F0` | Rider check (combat action unit; D2B00, 1E7810 through dispatch) | `src/gameplay/units/native_tu15_rider_check.hpp` | `HALO2_*_RIDER_CHECK` | 3/3 |
| `B95D0` | Object placement (death cycle and live support units; steps per pin) | `src/gameplay/objects/native_tu15_object_placement.hpp` | `HALO2_*_OBJECT_PLACEMENT` | 3/3 |

Live checks of `1AE5E0`..`B95D0` (2026-11-24, round 70): Armory three roles
identical; MP reference vs verify 8/8 identical; live calls 43,910 object
placement, 29,861 curve value, 16,606 resource acquire, 16,050 player timers,
15,626 player factor, 1,255 rider check, 849 deferred action target check and
9 tag group fixups, with no mismatch.

| `C9010` | Unit actions (live support unit; the unit from the Kept) | `src/gameplay/units/native_tu15_unit_actions.hpp` | `HALO2_*_UNIT_ACTIONS` | all |
| `1DC680` | Record place (scene device constructor unit) | `src/gameplay/objects/native_tu15_record_place.hpp` | `HALO2_*_RECORD_PLACE` | 6/6 |
| `1157D0` | Unit animation update (live support unit; seven children) | `src/gameplay/units/native_tu15_unit_animation_update.hpp` | `HALO2_*_UNIT_ANIMATION_UPDATE` | 4/4 |
| `153360` | Player unit search (multiplayer engines unit; BD080, 1538B0) | `src/gameplay/multiplayer/native_tu15_player_unit_search.hpp` | `HALO2_*_PLAYER_UNIT_SEARCH` | 5/5 |
| `2E2E30` | Memory copy (scene device constructor unit) | `src/rendering/renderer/native_tu15_memory_copy.hpp` | `HALO2_*_MEMORY_COPY` | 1/1 |
| `1EDDA0` | State dispatch (live support unit; the original's jump table) | `src/physics/runtime/native_tu15_state_dispatch.hpp` | `HALO2_*_STATE_DISPATCH` | 4/4 |
| `1CEB70` | Countdown step (live support unit; 1CE950) | `src/physics/runtime/native_tu15_countdown_step.hpp` | `HALO2_*_COUNTDOWN_STEP` | 4/4 |
| `C3960` | Shape copy (four identical units) | `src/pstencil/native_tu15_shape_copy.hpp` | `HALO2_*_SHAPE_COPY` | 2/2 |
| `140CF0` | Pool reset (four units) | `src/rendering/renderer/native_tu15_pool_reset.hpp` | `HALO2_*_POOL_RESET` | 2/2 |
| `C3A20` | Light shape source (three units; steps per pin) | `src/pstencil/native_tu15_light_shape_source.hpp` | `HALO2_*_LIGHT_SHAPE_SOURCE` | 6/6 |
| `BE2B0` | Object allocation (live support unit; 16E6A0, 1910A0, 16E8A0) | `src/gameplay/objects/native_tu15_object_allocate.hpp` | `HALO2_*_OBJECT_ALLOCATE` | 3/3 |
| `15CAB0` | Kind slot (live support unit) | `src/gameplay/multiplayer/native_tu15_kind_slot.hpp` | `HALO2_*_KIND_SLOT` | 4/4 |

`C9010`..`15CAB0` passed their validators in round 70 and were bound in
round 71. `C9010`'s `push` mutant still named the adapter's old `gp[esi]`
push; its anchor now names the Kept unit (it passed again in round 71).

Round 71's perf build failed: the new C3A20 port shared the namespace
`halo2::native_light_shape`, its file names and the `HALO2_*_LIGHT_SHAPE`
switches with the older C3AB0 light volume builder
(`src/rendering/native_tu15_light_shape.hpp`), and both headers land in the
light shape unit. C3A20 is now `native_tu15_light_shape_source`
(`halo2::native_light_shape_source`, `HALO2_*_LIGHT_SHAPE_SOURCE`), 118120
`native_tu15_cluster_pairs` (it had taken B8D90's
`halo2::native_object_cluster_test` and test file), and
`tools/check_frame_misc_unit_headers.py` compiles every unit's prepended
headers from all binders together before a build. Three tests that a new
port's test had overwritten were restored: C3AB0's harness
`tests/native_tu15_light_shape_tests.cpp` from git (its 11-19 validation hash
matches), B8D90's `tests/native_tu15_object_cluster_test_tests.cpp` and
1D9F50's test (now `tests/native_tu15_entry_check_tests.cpp`; 1062A0 keeps
`tests/native_tu15_entry_kind_tests.cpp`) from the session transcripts; all
three pass again. Round 71's validators passed for `118120`, `4B150`,
`C3820`, `1617A0` and `1A6830` (bound in round 72); `1718C0`'s
`sample-size` survived because the adapter rebuilt the sample pointer from
registers (it now pushes the logic's), and `15B740`'s `handle` because
161530's stub almost never answered 0 (now half the time). Round 72 builds
and live-checks `C9010`..`15CAB0` and these five, and validates `1718C0`,
`15B740`, `118120`, `C3A20` (renamed) and the new ports:
- `E53E0` motion names (live support unit; 1D3BD0; the original's jump table
  at E54C0).
- `E52D0` biped recovery (live support unit; E7E80, 111B10, C1A70, 2031F0
  through dispatch).
- `113B70` seated blend (live support unit; C9550, 113C70; the original's
  binary search over the action names replayed for its flags).
- `14FDC0` player activation (live support unit; 69AD0, B7E50, the
  variant's method through the dispatch).

Live checks of `C9010`..`15CAB0`, `118120`, `4B150`, `C3820`, `1617A0` and
`1A6830` (2026-11-24, round 72; the switches file was rewritten after zsh
passed its name list as one word): Armory three roles identical; MP reference
vs verify 8/8 identical; live calls 34,727 countdown step, 20,585 unit
animation update, 17,833 unit actions, 9,726 kind slot, 6,172 cluster pairs,
4,826 player exclusion, 4,800 teleporter update, 3,339 object allocation,
2,980 state dispatch, 1,488 record place, 612 light shape source, 603 light
record, 450 player unit search, 300 shape copy and 27 pool reset, with no
mismatch (memory copy and record sort were not reached). Round 72's
validators passed for `1718C0`, `15B740`, `118120`, `C3A20` (after the
renames), `E53E0`, `E52D0`, `113B70` and `14FDC0`; the last six are bound in
round 73, which validates `17CD10` effect part places (shadow frame and lens
flare units; 17DD50, 145560 and 159690 through the dispatch) and re-validates
`B8D90` and `1D9F50` with their restored tests.

Live checks of `1718C0`, `15B740`, `E53E0`, `E52D0`, `113B70` and `14FDC0`
(2026-11-24, round 73): Armory three roles identical; MP reference vs verify
8/8 identical; live calls 17,200 seated blend, 16,490 motion names, 16,490
biped recovery, 5,250 player activation, 4,800 round end and 2,251 sound
readiness, with no mismatch. Round 73's validators passed for `17CD10`
(bound in round 74) and, with their restored tests, `B8D90` and `1D9F50`.
Round 74 validates:
- `1CE950` channel advance (live support unit; TU1.1 1CA090: 1CEE60, 1E3150,
  1CF700 three times, 1D3660, B8780 and 1CE810; the step and last frame in
  the first two argument slots).
- `BA630` object connect (live support unit; the cluster re-registration:
  11E600, BA910, 1D2F10, C0F60, B8D90, B92C0, B1870, BA570, B9330, 1CC290,
  1496F0 and 282C10 through dispatch; the all-cluster set by REP STOSD).
- `1CEC60` event window (live support and AI actor units; TU1.1 1CA3A0:
  1E3150; the original's four-wide event walk and UCOMISS/LAHF window side).
- `E06E0` aim offset (live support unit; 111F00, BAD60, CDF30, E05D0; the
  per-axis step clamp at 46A558/46A18C).

Live check of `17CD10` (2026-11-24, round 74): Armory three roles identical;
MP reference vs verify 8/8 identical; 103,475 live effect part places with no
mismatch. Round 74's validators passed for `1CE950`, `BA630`, `1CEC60` and
`E06E0` (bound in round 75). Round 75 validates:
- `1CF700` channel events (live support unit; TU1.1 1CAE40: the events,
  sounds and effects a step passes, each handed to the callback in EBX
  through the dispatch as a 20-byte record).
- `15B4D0` side count (multiplayer engines unit; the teams of a team game,
  four at a time, or the players in use).
- `1459B0` rotation product (scene device constructor unit; a 3x3 product,
  each element's terms in the original's operand order).

Live checks of `1CE950`, `BA630`, `1CEC60` and `E06E0` (2026-11-24, round
75): Armory three roles identical (acd375e77ffb11c9, trace df61945af603); MP
reference vs verify 8/8 identical; live calls 60,602 event window, 47,721
object connect, 42,001 channel advance and 16,490 aim offset, with no
mismatch. Round 75's validators passed for `1CF700`, `15B4D0` and `1459B0`
(bound in round 76). Round 76 validates:
- `15BE80` score lead (multiplayer engines unit).
- `1181F0` object lights (scene device constructor unit).
- `C8E70` object team (live support unit; D2B60 and D2BD0, the riders'
  teams through the iterator on the stack).
- `C8390` channel fades (live support unit; D3150, D05A0, E8E10; the weights
  faded in single precision, UCOMISS/LAHF at their ends).
- `D35C0` seat transform (live support unit; BAD60, BBE00; the marker's matrix
  by REP MOVSD in either direction, the pops between the last copy's loads and
  stores).
- `1601B0` respawn wait (live support unit; 1A8E50 twice over, 160570).
- `160360` team respawn wait (live support unit; 1A8E50, 160570, 1601B0,
  164890; byte team counts in the frame, the share at least 2).
- `160570` lives spent (live support and death cycle units, one pin each) and
  `164890` team active (live support unit), both leaves.
Deferred: `185670` (aligned stack, CRT `_controlfp` dispatches 32B3D6 and
32B3C5).

Live checks of `1CF700`, `15B4D0` and `1459B0` (2026-11-24, round 76):
Armory three roles identical (acd375e77ffb11c9); MP reference vs verify 8/8
identical; live calls 55,787 channel events, 25,310 rotation product and
4,800 side count, with no mismatch. Round 76's validators passed for all
nine (`15BE80`, `1181F0`, `C8E70`, `C8390`, `D35C0`, `1601B0`, `160360`,
`160570`, `164890`; bound in round 77). Round 77 validates:
- `1D5970` pose blend (live support unit; 1CEFB0 three ways, 1D61B0's factor
  in XMM0 and the ECX it returns; the factor in the argument's slot).
- `CDCF0` seated vector (live support unit; BBFF0, 30BD0 twice with the
  UCOMISS/LAHF fallback, 144210; the frame's matrix).
- `69AD0` machine address (live support unit; REPE CMPSW in either
  direction, the translated route reading EDI's word first; 6C3F0).
- `1AFA90` list slots (AI actor and final startup storage units; a jump
  table through canonical_branch).
- `E71A0` unit reaction (live support unit, through the dispatch; E72E0).
- `144910` quaternion matrix (AI frame and scene device constructor units;
  XMM0 to XMM7 as the original leaves them).
- `1D45A0` animation names (live support, AI actor and AI frame units, two
  pins; 1E5810).
- `191130` pool resize (live support unit; 33B590, 1910A0, 191220; REP
  MOVSD/MOVSB either way).
- `BE3B0` object extension (live support unit; 191130; REP STOSD/STOSB).
- `175380` placement repair (live support unit; 175300 twice, B1720, B16C0;
  the tail JMP 1722C0).
Live checks of `15BE80`, `1181F0`, `C8E70`, `C8390`, `D35C0`, `1601B0`,
`160360`, `160570` and `164890` (2026-11-24, round 77): Armory three roles
identical; MP reference vs verify 8/8 identical; live calls 23,090 each
seat transform, object team and channel fades, 15,698 each respawn wait and
team respawn wait, 13,260 score lead, 6,701 object lights and 316 lives
spent, with no mismatch. Round 77's validators passed for nine (bound in
round 78); `144910` left a `degenerate` mutant alive (a norm exactly at the
least value), so its fixture now puts unit quaternions on a least norm of 1
and round 78 re-validates it with `1AF970` behavior tables (AI actor and
final startup storage units; 1AFA90, an eight-byte aligned 0x2F0 frame, REP
STOSD/MOVSD), `1D39C0` animation step (live support unit; 1CEB70, 1E3150,
1D38A0, 1D3930, 1CE950), `111B10` animation change (live support and AI
actor units, two pins; 1D5320, 115E20, 1D4350, 115E90, BC380, 115AF0),
`16BDA0` point blocked (live support unit; 14CDB0, 16BBC0, 16E360, the
object iterator BA970/BA9E0, 16BC80) and `16BBC0` instance blocks (live
support and death cycle units, two pins; 16B810, 145200, 14CDB0).
Deferred: `19AE70` (the stack probe 32A660 under a 0x4568-byte frame).

Live checks of `1D5970`, `CDCF0`, `69AD0`, `1AFA90`, `E71A0`, `1D45A0`,
`191130`, `BE3B0` and `175380` (2026-11-24, round 78): Armory three roles
identical; MP reference vs verify 8/8 identical; live calls 31,898 seated
vector, 26,958 object extension, 20,710 animation names, 16,490 each unit
reaction and pose blend, 16,050 machine address, 9,372 pool resize, 2,880
list slots and 600 placement repair, with no mismatch. Round 78's
validators passed for `144910` (its fixture fixed), `1AF970`, `1D39C0`,
`111B10` and `16BBC0` (bound in round 79). `16BDA0` left `shape-size` and
`kind-mask` alive: its adapter computed the shape offset itself, and the
fixture's kinds stayed below 8; the logic now computes both offsets and
the kinds span 0 to 0x7F. Round 79 re-validates `16BDA0` with `1CEBD0`
channel seek (scene device constructor unit; 1E3150 and 1CE950 through the
dispatch), `185820` ray hit accept (live support unit; 186C10, 1D7530
through the dispatch) and `13B610` cluster visibility (live support unit;
1532A0, 140EC0, 16FD40 through the dispatch; REP STOSD).
Deferred: `20FBB0` and `21F260` (dispatch targets that live in no generated
unit, their stack cleanup unknown here), `CAD60` (tail jump to 106160).

Live checks of `144910`, `1AF970`, `1D39C0`, `111B10` and `16BBC0`
(2026-11-24, round 79): Armory three roles identical; MP reference vs
verify 8/8 identical; live calls 36,031 animation step, 10,279 instance
blocks, 4,835 animation change and 9 behavior tables, with no mismatch
(the quaternion matrix had no live calls in these runs; its validator
stands). Round 79's validators passed for `16BDA0` (7/7 branches, 16
mutants), `1CEBD0`, `185820` and `13B610` (bound in round 80).

Round 80 adds 16- and 8-byte vector accesses to the boundary framework
(`native_tu15_object_tick.hpp`): `Port::read_vector` (as the unit's `v()`,
recorded as four ordered 4-byte reads, the memory contract of
`read_vector`), `read_half`, `store_half` and `store_vector` (two 8-byte
stores, as `vw()`), `Memory::load128`/`load_half`/`store128`/`store_half`,
and verify events carrying up to 16 bytes. The frame misc verification
references pin `_mm_add_ps`/`_mm_mul_ps` to the original's destination
(`native_tu15_packed_sse.hpp`), as `tools/specialize_native_operands.py`
pins the matrix leaves: without instruction callbacks the compiler swapped
a MULPS's operands, so a dual-NaN product took the wrong payload. Round 80
validates `16E510` data array restore (campaign runtime unit; strncpy
32A4E0 through the dispatch, caller-cleaned; BA fill as REP STOSD/STOSB in
either direction), `13DC44` HUD weapon flags (weapon HUD unit; EBP frame;
BCDF0 and 1E7810 through the dispatch, 10B3D0 direct), `1D8B90` body
matrix and `1D9190` body place (scene device constructor unit; aligned
stack transforms; 2E4650 and 2E58F0 direct, 2E4470 and 2E4350 through the
dispatch) and `2E9140` shape ray hit (live support unit; two virtual calls;
the normal rotated by packed SSE). `145560` needs no port: the operand
specializer installs `kernel_145560` in that unit (the census lists it only
because census mode disables the leaf kernels).

Live checks of `16BDA0`, `1CEBD0`, `185820` and `13B610` (2026-11-24, round
80): Armory three roles identical; MP reference vs verify 8/8 identical;
live calls 10,500 cluster visibility, 6,669 channel seek, 4,754 ray hit
accept and 1,158 point blocked, with no mismatch. Round 80's validators
passed for `16E510`, `13DC44` (24 mutants), `1D8B90`, `1D9190` and `2E9140`
(bound in round 81), the first owners with 16-byte vector accesses.

The validator now compares an owner against its translated body with the
same packed-SSE pinning as the verification reference
(`native_frame_misc_replacements.pinned`): unpinned at -O2, GCC swapped a
MULPS in 315190's body, so a dual-NaN product took the wrong payload there
too. The reference generator replays `s.required` children (as
`tools/native_object_subtree_replacements.py` does), so an owner whose child
leaves the unit through the World's required owner binds with a route
`v.required(site,target,false)`. Round 81 validates `315190` body list ray
(live support; each body's shape cast, a packed transpose rotation),
`2F20F0` child agents (map world support; a C++ exception frame through
fs:[0], a stack wrapper shape, the dispatcher's agent table), `1DDEA0`
rigid body build (live support; four direct children with 31 arguments,
REP MOVS in either direction, the shape's release), `168CA1` player weapon
sync (sandbox special; 169C46 through the required owner), `164EF0` round
timer and `164750` players facing (multiplayer engines; scalar SSE).

Live checks of `16E510`, `13DC44`, `1D8B90`, `1D9190` and `2E9140`
(2026-11-24, round 81): Armory three roles identical; MP reference vs
verify 8/8 identical; live calls 28,424 body matrix, 25,143 shape ray hit,
16,278 HUD weapon flags, 6,755 body place and 9 data array restore, with no
mismatch (the first vector-access owners). Round 81's validators passed for
`315190`, `2F20F0`, `1DDEA0` (8/8 branches), `168CA1`, `164EF0` and
`164750` (bound in round 82). Round 82 validates `17BD40` effect element
(death cycle; six direct children, the reciprocal scale by DIVSS), `13B6F0`
cluster visibility update (live support; REP MOVSD copies, REPE CMPSD
comparisons, 0x44 listeners at 44AEEC), `154270` object reach and `E79C0`
weapon trade (combat action; scalar SSE reach and facing, CFC90 hands,
NEG/SBB, XMM5 to XMM7 kept across 11B560) and `15E870` round countdown
(multiplayer engines; CVTSI2SS/CVTTSS2SI, 15DDC0 through the required
owner). `BC200` needs no port: its translated body already opens with
`model_light_source_detail::bridge`.

Live checks of `315190`, `2F20F0`, `1DDEA0`, `168CA1`, `164EF0` and
`164750` (2026-11-24, round 82): Armory three roles identical; MP headlong
and midship 4p identical; the other six MP cases exited 1 in both the
reference and verify sweeps at map load ("Source tag alias has no owned
same-address payload", `src/tags/directory.cpp`), raised by a concurrent
session's uncommitted `src/rendering/path_tracing/lightmap_emission.cpp`
(`cache.tag_payload` on a zero-size shader alias), not by these owners;
those cases are to be re-run once that is fixed. Live calls in the maps
that ran: 2,282 rigid body build, 1,656 player weapon sync, 1,206 round
timer, 648 child agents, 465 body list ray and 357 players facing, every
verify line identical. Round 82's validators passed for `17BD40`,
`13B6F0`, `154270`, `15E870` and `E79C0` (bound in round 83). Round 83
validates `1AE7F0` action candidates (combat action; 0xF0 frame, up to 64
candidates, 1AEA30/1AECE0 direct, BAD60 through the dispatch), `1ADC90`
weapon ranges (combat action; DIVSS/MULSS ranges, primary and secondary
weapons; 106160 stays translated as its direct child) and `1770C0` effect
elements draw (effect particle draw; the element data array walk with
16EA60, the visible cluster bitmap at 557034). Deferred: `E4D30` (the
biped falling damage unit's literal constants are MapPack11 addresses,
mapped by `selected()` for TU1.5, while the harness stubs `selected()` as
identity) and `2F4990` (Havok monitor stream reads RDTSC through the
`timestamp_counter` service, under an SEH frame with a 0x398-byte
in-place array).

Live checks of `17BD40`, `13B6F0`, `154270`, `15E870` and `E79C0`
(2026-11-24, round 83): Armory three roles identical; MP headlong and
midship 4p identical, the other six MP cases again exiting 1 at map load
(the round 83 build compiled `lightmap_emission.cpp` before that session's
fix, which skips zero-size alias shader tags). Live calls: 7,036 effect
element, 5,682 weapon trade, 1,656 cluster visibility update, 1,206 round
countdown and 66 object reach, every verify line identical. Round 83's
validators passed for `1AE7F0` (11 mutants), `1ADC90` (10) and `1770C0`
(10), bound in round 84. Round 84 re-runs the full MP sweep for the
fourteen owners of rounds 82 to 84 on a build with the fix, and validates
`245060` director aim (vehicle director; the aim record's position,
forward and up from the unit's weapon marker (BAD60) or its parent's node
matrix (144BF0) and its transpose, XMM6/XMM7 kept across 11FA50) and
`C7DD0` unit weapon controls (physics live support; the timers at +0x1F5,
+0x1FA and +0x1FB counting down, the stance through 116480/1164D0, weapon
change, trigger and stun actions through E8E10, the grenade through
CE3A0, the zoom with its weapon or global sound through 18C000 scaled by
level / (levels - 1), and each hand's control word to 105500; 25
mutants).

Round 84 (2026-11-24) on a build with that session's fix: Armory three
roles identical and all eight MP cases identical in the reference and
verify sweeps for the fourteen owners of rounds 82 to 84, so rounds 82 and
83 are now fully live-checked. Live calls: 17,932 weapon ranges, 16,895
effect element, 16,464 weapon trade, 6,290 effect elements draw, 5,250
player weapon sync, 5,250 cluster visibility update, 4,800 round timer,
4,800 round countdown, 2,692 rigid body build and 613 players facing.
`live_check.py` keeps only the last eight verify lines, so the rest were
rerun on headlong and midship 4p with only their switches: 648 child
agents, 465 body list ray, 566 action candidates and 66 object reach, all
identical. Every verify line was identical. Validators passed for `245060`
(9 mutants) and `C7DD0` (25), bound in round 85, which validates `113190`
animation busy (physics live support and AI frame; the two compiled
string-id searches as name sets, with the adapter mapping an unlisted
channel animation to the original's epilogue; 15 mutants), `1F8710`
platform carry (physics live support; REP MOVSD rotation copies under the
direction flag, the found byte reloaded from the ESP 1FA1A0 left; 15),
`1618B0` respawn status (multiplayer engines; fifteen exits, each with its
own epilogue; 13) and `17C370` effect follow (physics live support and
death cycle; every child through the dispatch so both units bind; 14).

Round 85 (2026-11-24): `245060` and `C7DD0` live: Armory three roles
identical, all eight MP cases identical, 15,874 director aim and 23,090
unit weapon controls calls, every verify line identical. Validators passed
for `113190` (15 mutants), `1F8710` (15) and `17C370` (14), bound in round
86. `1618B0`'s validator stopped on its adapter having no `enum Branch`
(the exits were its branches); it now names them and validates again in
round 86, with `CD4A0` unit camera position (physics live support, AI
frame and sandbox special; the biped eye height, E18D0's move and two
16C620 rays with the plane test; the seat and head markers through BAD60;
19 mutants), `17EAD0` contrails update (contrail geometry; the frame-rate
loop keeps XMM1 to XMM5 across 17EE10, so its SSE results keep the
registers' upper lanes; 16) and `14D790` point surface (physics live
support, map world support, model render and combat action; the leaf's
references on the plane, their three vertices and 1211C0; 15).

Round 86 (2026-11-24): `113190`, `1F8710` and `17C370` live: Armory
identical, all eight MP cases identical, 23,740 animation busy, 16,490
platform carry and 131,530 effect follow calls, every verify line
identical. Validators passed for `1618B0` (15/15 branches, 13 mutants),
`17EAD0` (16) and `14D790` (15), bound in round 87. `CD4A0`'s
`back-off-order` mutant (swapping MULSS's operands, which differs only in
which of two NaNs survives) survived and became `back-off-scale`; it
validates again in round 87, with three new ports: `1DE8E0` group flood
(physics live support; recursive through its own native port, the pairs
read through the ESI and EDI each child returned; 18 mutants), `1538B0`
item pickup (multiplayer engines; the reach test, the four weapons'
ammunition, grenades and powerups, and the weapon handed over or
requested; 21) and `1700E0` permutation choice (physics live support; the
four-at-a-time listing on the frame, the weight kept in XMM4 across
170930 and 170B40; 17).

Round 87 (2026-11-24): `1618B0`, `17EAD0` and `14D790` live: Armory
identical, all eight MP cases identical, 15,600 respawn status, 5,092
contrails update and 1,865 point surface calls, every verify line
identical. Validators passed for `CD4A0` (12/12 branches, 19 mutants),
`1DE8E0` (11/11, 18), `1538B0` (15/15, 21) and `1700E0` (11/11, 17), bound
in round 88. Round 88 validates six more ports:
- `14D410` object point surface (physics live support, model render and
  combat action; the region's permutation and BSP, the point in the node's
  space through 145200 twice, the part the node names, the leaf's
  references on the plane and their triangle through 243F20 and 1211C0;
  17 mutants).
- `1115D0` unit state change (physics live support; the compiled compare
  tree on the state string id is the adapter's, the states it leads to the
  logic's; the byte counter that throttles 1D6430; 13).
- `1ADF60` aim choice (combat action; the stack probe, 14CDB0's leaf, the
  aim search 1AE090, qsort and the first candidate 1AF3E0 accepts, copied
  by REP MOVSD; 15).
- `1139D0` unit aim enabled (physics live support; two compiled string id
  searches, one through the code's jump table whose two reads it keeps, as
  sets; C9550 and 113C70; 13).
- `15D890` item points (multiplayer engines; weight 1.15M in the census:
  the per-tick share of item points, two IDIVs, the engine's first method,
  the vehicle-away count and the respawn timers scaled by the game's
  setting; 17).
- `D2BD0` rider iterator (physics live support; a leaf walk of the object
  tree for riding units; 10).
Not counted before: `15D890`, `19AE70`, `1115D0`, `1139D0`, `1ADF60` and
`D2BD0` were missing from the earlier plain-candidate list. `2E5760` and
`106160` use x87.

Round 88 (2026-11-24): all six validators passed (`14D410` 14/14
branches, 17 mutants; `1115D0` 9/9, 13; `1ADF60` 6/6, 15; `1139D0` 7/7,
13; `15D890` 15/15, 17; `D2BD0` 5/5, 10). The build failed before the
live check: the `4A7B0` port's first name, `shadow_casters`, reused the
bound `333F0` port's file names and overwrote its untracked files. `333F0`
was restored by replaying its creation commands from the session
transcript; all 40 of its validator mutant texts are present, and its test
gives 8,000 cases with 19/19 branches as recorded. `4A7B0` became
`shadow_record_merge`. Round 89 builds once and live-checks round 88's
owners and the six bound after its validators, then validates `4A7B0`
shadow record merge (shadow frame; a leaf whose exit state is EAX, ECX, the
EDX mask, XMM0 and the last compare; 13 mutants). Staged: `4AC20` shadow
schedule (shadow frame; partition, stale moves, the two sorts through
4B150, the four-at-a-time fade and the growth, the output; 11/11 branches,
16 mutants).

Round 89 (2026-11-24): one build, two live passes, Armory and all eight MP
cases identical in each, every verify line identical: 46,465 unit camera
position, 20,259 unit state change, 17,200 unit aim enabled, 15,574 aim
choice, 10,154 rider iterator, 4,800 item points, 2,728 group flood, 1,242
permutation choice and 162 item pickup calls (object point surface had no
live calls in these maps; its validator stands). `4A7B0`'s validator
passed (6/6 branches, 13 mutants); bound in round 90, which validates
`4AC20` shadow schedule and `1EFD60` action allowed (multiplayer engines,
the census's heaviest remaining owner at 2.05M: the 32-case jump table of
the player actions, each case's own epilogue; the weapon lookups through
EBX, EDI or EBP as each child returned them; 18 mutants).
Not ports: `B8780`, `B87B0`, `179DF0`, `C9070`, `130980` and others in the
census list are already native-headed. Flags for the rest: x87 (76 owners),
16-byte vectors (`1E1010`), tail jumps through the dispatch (`1CE8D0`).

Ports validated in round 71:
- `118120` cluster pairs (scene device constructor unit; the
  `empty_range` branch is unreachable: a positive int16 count added to the
  int16 first reference always ends past it).
- `1718C0` sound readiness (live support unit; 21F260).
- `4B150` record sort (shadow frame unit; a bubble sort whose pass counter
  lives in the first's slot; REP MOVSD both directions).
- `C3820` light record (map world support and light portal units; C3A20 and
  C3AB0, steps per pin).
- `15B740` round end (multiplayer engines unit; 15B4D0, 15B620, 161530
  through dispatch).
- `1617A0` player exclusion (live support unit; 1A8E50, 1601B0, 160360).
- `1A6830` teleporter update (teleporter unit; 1A8F10 through dispatch,
  1A6930, 1A6A40).
Also parked: `145560` (8-byte half-register SSE loads and stores),
`1262B0`, `126190`, `3EB10` and `16E860` (native-headed), `160A60` (tail
jump through a table). Parked: `2E5810` (16-byte MOVAPS accesses the trace events
cannot carry yet),
`17F8A0` (an 8-byte FLD the trace events cannot carry yet, and two copy
groups that differ in C1), `122BE0` (a register-allocated SSE solve: next
large port). B9460 needs binder support for `required(...)` children before
it can be ported in this framework.

Binding notes: the binder digests the `src/` generated text; every adapter
header must compile on its own (it is included at the top of the unit); a
failed `tools/build_perf.sh` leaves the old binary in place, so check its
first line and the binary's time before a live check.

Live checks (Armory three roles; MP reference vs native-verify on Zanzibar 1/4p,
Coagulation, Headlong, Waterworks, Turf, Burial Mounds, Midship 4p): identical for
every owner through 333F0 that has been bound; verify compared hundreds of
thousands of live calls per owner with no mismatch. Lockout 4p and Ascension 4p
are not deterministic run to run (the reference role differs from itself), so
they are not part of the sweep. Zanzibar 4p census after the four targeting
owners: 268k to 196k translated instructions per frame.

Census caution: routines whose translated body begins with an older native
kernel (30BD0 normalize, 125280, 17C140, ...) refuse under the census's
instruction callbacks and are counted although normal runs are native; rank
with those excluded.

Armory, 300 frames: default, reference and verify roles all give frame
`acd375e77ffb11c9` / trace `df61945af603`. The tree has moved since 11-19, so
the hashes differ from the 11-19 baseline, but the three roles agree. The
targeting search does not run in that room. Next: `1AE2B0` (16.5k/frame in
Zanzibar 4p: the candidate chain walk with the same cone test and children
1AE500/1AE7F0, self-recursive), then `1AE090` (11.0k; x87 FSIN/FCOS cone
setup, calls 14D200, BD460, 16EA60, 1AE2B0).

## November 20 particle motion and hierarchy update

The x86-64 Release performance binary `630d7fe83b6f432f900cfa815cb1f07038c941435e26787df8121784850d6074` runs the TU1.5 `2C2AD0` particle motion owner and binds the existing `BBD10` hierarchy kernel in live support. The particle kernel preserves the authored jump table, gravity and drag calculations, particle traversal, and the original child calls. The component comparison exercised 199 original instruction sites across 8,000 cases per mode, including 1,143 injected interruptions. All nine compiled mutations were detected.

| Workload | Owner | Translated callbacks with reference switches | Translated callbacks with native defaults |
|---|---|---:|---:|
| Coagulation, four players, 600 ticks | `2C2AD0` | 11,524,444 | 0 |
| Coagulation, four players, 600 ticks | `BBD10` | 1,144,899 | 0 |
| Armory, 300 frames | `2C2AD0` | 23,861 | 0 |
| Armory, 300 frames | `BBD10` | 129,029 | 0 |

The reference runs used the same binary with `HALO2_REFERENCE_EFFECT_OWNERS=2c2ad0` and `HALO2_REFERENCE_OBJECT_HIERARCHY_ROOT=1`. The latter restores the translated walk in every bound unit. Both workloads matched their reference frames, and Armory matched its reference trace. Coagulation verification replayed 16,093 particle motion calls. The largest remaining translated routine there was `1770C0` (17,919 callbacks per tick), followed among the unconverted routines by `1772C0` (10,776 per tick). The largest in Armory was `30E930` (4,750 per frame). Component and live results are in `build/test-results/particle-motion-20261120/`.

Nine scripted 600-tick multiplayer cases across eight maps matched their translated reference captures byte for byte. Four-player Lockout reached tick 600 on all four runs. Its two translated-reference frames differed from each other, as did its two native-verification frames. Both native runs matched their per-call reference replay. Lockout frame parity remains open. The first comparison and repeat evidence are in `live/summary.json` and `repeats/summary.json` under the same results directory.

---

## 1. Current Baseline & Summary of Completed Work (as of 2026-11-19)

- **Shared Performance Binary**: `build-perf-4k60/halo2_native` (`5b7ab5790a0de084b8ad8d36d451bf691237d1dc5e93c605132379eecccbaff2`)
- **Armory Reference Frame / Trace Hashes**: `5627a51badf47762` / `8a9b58b38bef` (300 frames, Original mode, `1280x720`)
- **Translated Instruction Census Progression (Armory 01a, `HALO2_CENSUS_NATIVE_BRIDGES=1`)**:
  - **2026-11-18 Reference Baseline**: `343,892.0` translated instructions/frame
  - **2026-11-19 Reference (15 recent switches set to `=1`)**: `205,891.0` translated instructions/frame (`61,767,151` total)
  - **2026-11-19 Native Default**: **`123,204.3` translated instructions/frame** (`36,961,290` total — **-64.2% vs 343.9k**, **-40.2% vs 205.9k**)
  - Raw census comparison: `build/test-results/shadow-frame-20261119/census/compare.txt`

### Recently Completed, Proven & Bound Owners (2026-11-18 to 2026-11-19)

| Entry | Group | Name / Role | Systems & Adapter Headers | Reference Switch | Proof Report | Census Delta (Armory) |
|---|---|---|---|---|---|---|
| `1362A0` | S | Render-object submission | `src/rendering/native_tu15_object_submit_systems.hpp` + `native_tu15_object_submit.hpp` | `HALO2_REFERENCE_OBJECT_SUBMIT` | `build/test-results/shadow-frame-20261119/object_submit.json` (963 physical cases, 180/180 sites, 16/16 mutants) | `6,706.5` -> **`0.0/frame`** |
| `134D30` | S | Cluster contents collector | `src/rendering/native_tu15_cluster_contents_systems.hpp` + `native_tu15_cluster_contents.hpp` | `HALO2_REFERENCE_CLUSTER_CONTENTS` | `build/test-results/shadow-frame-20261118/cluster-contents-sanitizers.json` | `5,686.5` -> **`0.0/frame`** |
| `4BAE0` | S | Object distance fade & LOD | `src/rendering/native_tu15_object_fade_systems.hpp` + `native_tu15_object_fade.hpp` | `HALO2_REFERENCE_OBJECT_FADE` | `build/test-results/shadow-frame-20261118/object_fade_sanitizers.json` (296 physical cases, 127/127 sites, 15/15 mutants) | `3,652.1` -> **`0.0/frame`** |
| `251A60` / `250450` | E | Particle location & instance update | `src/rendering/effects/native_tu15_particle_update_systems.hpp` + `native_tu15_particle_update.hpp` | `HALO2_REFERENCE_EFFECT_OWNERS` | `build/test-results/effect-owners-20261118/validator.json` (60/60 + 59/59 branches, 153/153 mutants) | `2,758.2` -> **`0.0/frame`** |
| `153BA0` | F | Player action builder (`153BA0`/`BD080`/`1540D0`/`153F20`/`154270`/`1541B0`) | `src/physics/runtime/native_tu15_player_action_systems.hpp` + `native_tu15_player_action.hpp` | `HALO2_REFERENCE_PLAYER_ACTION` | `build/test-results/frame-misc-20261118/final-153ba0.json` (8,000 cases, 19/19 branches, 0 surviving mutants) | `2,415.6` -> **`0.0/frame`** |
| `1365C0` | S | Render-model hierarchy & layer splitter (self-recursive) | `src/rendering/native_tu15_model_hierarchy_systems.hpp` + `native_tu15_model_hierarchy.hpp` | `HALO2_REFERENCE_MODEL_HIERARCHY` | `build/test-results/shadow-frame-20261119/model_hierarchy.json` (833 physical cases, 265/265 sites, 16/16 mutants) | `2,376.6` -> **`0.0/frame`** |
| `E8D80` | H | Vehicle update (`physics_vehicle_update_generated.cpp`) | `src/physics/runtime/native_tu15_world_leaves_systems.hpp` + `native_tu15_world_leaves.hpp` | `HALO2_REFERENCE_PHYSICS_WORLD_LEAVES` | `build/test-results/world-leaves-20261118/validator.json` | `2,204.2` -> **`0.0/frame`** |
| `136D50` | S | PRT lighting partition | `src/rendering/native_tu15_prt_partition_systems.hpp` + `native_tu15_prt_partition.hpp` | `HALO2_REFERENCE_PRT_PARTITION` | `build/test-results/shadow-frame-20261118/prt_partition_sanitizers.json` (220 physical cases, 65/65 sites, 13/13 mutants) | `1,845.0` -> **`0.0/frame`** |
| `C3AB0` | S/E | Light volume shape builder | `src/rendering/native_tu15_light_shape_systems.hpp` + `native_tu15_light_shape.hpp` | `HALO2_REFERENCE_LIGHT_SHAPE` | `build/test-results/shadow-frame-20261119/light_shape.json` (829 physical cases, 194/194 sites, 13/13 mutants) | `1,683.4` -> **`1.7/frame`** |
| `1D7260` | H | Havok broadphase / world leaf | `src/physics/runtime/native_tu15_world_leaves_systems.hpp` + `native_tu15_world_leaves.hpp` | `HALO2_REFERENCE_PHYSICS_WORLD_LEAVES` | `build/test-results/world-leaves-20261118/validator.json` | `1,494.3` -> **`0.0/frame`** |
| `135E00` | S | Object draw flags | `src/rendering/native_tu15_object_draw_flags_systems.hpp` + `native_tu15_object_draw_flags.hpp` | `HALO2_REFERENCE_OBJECT_DRAW_FLAGS` | `build/test-results/shadow-frame-20261119/object_draw_flags.json` (147 physical cases, 47/47 sites, 14/14 mutants) | `1,011.5` -> **`0.0/frame`** |
| `1373F0` | S | Render-view queue reset | `src/rendering/native_tu15_view_reset_systems.hpp` + `native_tu15_view_reset.hpp` | `HALO2_REFERENCE_VIEW_RESET` | `build/test-results/shadow-frame-20261119/view_reset.json` (156 physical cases, 35/35 sites, 10/10 mutants) | `707.5` -> **`0.0/frame`** |
| `136230` | S | Render-object submission loop | `src/rendering/native_tu15_object_pass_systems.hpp` + `native_tu15_object_pass.hpp` | `HALO2_REFERENCE_OBJECT_PASS` | `build/test-results/shadow-frame-20261119/object_pass.json` (338 physical cases, 42/42 sites, 12/12 mutants) | `605.7` -> **`0.0/frame`** |
| `131610` | F | Campaign environment fog mixers (`131610`/`137AC0`) | `src/physics/runtime/native_tu15_fog_mixers_systems.hpp` + `native_tu15_fog_mixers.hpp` | `HALO2_REFERENCE_FOG_MIXERS` | `build/test-results/frame-misc-20261118/final-131610.json` (8,000 cases, 26/26 branches, 30/30 mutants) | `561.0` -> **`0.0/frame`** |
| `BBD10` | S/L | Object hierarchy root walk (bound in `source_shadow_frame` & `source_ai_frame`) | `src/rendering/native_tu15_object_hierarchy_root_systems.hpp` + `native_tu15_object_hierarchy_root.hpp` | `HALO2_REFERENCE_OBJECT_HIERARCHY_ROOT` | `build/test-results/shadow-frame-20261119/object_hierarchy_root.json` (138 physical cases, 19/19 sites, 11/11 mutants) | `430.1` -> **`259.1/frame`** (`0.0` in shadow/AI) |
| `135690` | S | Render-view visibility setup | `src/rendering/native_tu15_view_setup_systems.hpp` + `native_tu15_view_setup.hpp` | `HALO2_REFERENCE_VIEW_SETUP` | `build/test-results/shadow-frame-20261119/view_setup.json` (902 physical cases, 120/120 sites, 12/12 mutants) | `396.0` -> **`0.0/frame`** |
| `2E4760` | H | Rotation into world (`2E0C70` canonical) | `src/physics/runtime/native_tu15_world_leaves_systems.hpp` + `native_tu15_world_leaves.hpp` | `HALO2_REFERENCE_PHYSICS_WORLD_LEAVES` | `build/test-results/world-leaves-20261118/validator.json` | `215.7` -> **`0.0/frame`** |

---

## 2. Mandatory Guardrails for Every Agent

### A. 64 GB RAM Memory Cap (Strict User Requirement)
The host has **64 GB of RAM** and will OOM-crash if multiple heavy C++ builds, sanitizer harnesses, or live game instances run in parallel.
1. **Run at most ONE heavy build, validator, or live check at a time.** Never launch concurrent `g++`/`cmake` compilations or parallel `halo2_native` instances.
2. **Cap parallel jobs**:
   - Shared perf binary build: `JOBS=4 tools/build_perf.sh`
   - Launcher engine build: `python3 tools/build_launcher_engine.py --jobs 2` (or `./Halo2-Launcher --build-engine`)
   - Validator mutation screens: `--jobs 2`
   - Live multiplayer sweep: `--workers 1` (`tools/perf_4k60/live_check.py sweep ... --workers 1`)
3. **Enforce `RLIMIT_AS` (1 GiB)** and ordered-store vector caps in every test/mutant harness so a bad pointer or infinite loop in a mutant cannot exhaust host RAM.

### B. Test Storage Lifecycle (`AGENTS.md` / `docs/TEST_STORAGE.md`)
- Use `tools/run_tests.py` for CMake validation targets.
- Use `tools/run_test.py` (with `--scratch-root .local/test-scratch` for live game runs) for one-off compiler, probe, oracle, or live-check scripts, writing temporary outputs under `$HALO2_TEST_SCRATCH`.
- Verify all temporary `/tmp/halo2-test-*` and `.local/test-scratch/halo2-test-*` directories are removed before finishing your turn.

### C. Whole-File SHA256 Pins (`tools/export_native_tu15_physics_callable_data.py`)
**CRITICAL**: `tools/export_native_tu15_physics_callable_data.py` pins the whole-file SHA256 of these **7 canonical `.cpp` files**:
1. `src/physics/runtime/physics_live_support_tu15_generated.cpp`
2. `src/physics/runtime/physics_world_runtime_generated.cpp`
3. `src/physics/runtime/physics_contact_runtime_generated.cpp`
4. `src/physics/vehicle/physics_vehicle_motion_math_generated.cpp`
5. `src/world/source_map_world_support_tu15_generated.cpp`
6. `src/campaign/death_cycle_runtime_tu15_generated.cpp`
7. `src/combat/source_combat_action_tu15_generated.cpp`

- **NEVER modify those 7 `.cpp` files directly in `src/`!** Doing so breaks `native_tu15_physics_callable_data_export` in CTest.
- Instead, bind native replacements into those 7 units **at build time** in their specialized copies via `spatial_owner_bindings` in `tools/specialize_native_operands.py` (using `tools/native_live_support_leaves_replacements.py`, `tools/native_world_leaves_replacements.py`, `tools/native_frame_misc_replacements.py`, or `tools/native_effect_owner_replacements.py`).
- Other generated units (such as `source_shadow_frame_tu15_generated.cpp`, `source_ai_frame_tu15_generated.cpp`, `native_light_shape_tu15_generated.cpp`, `source_effect_particle_draw_tu15_generated.cpp`) are **not** whole-file pinned by `export_native_tu15_physics_callable_data.py`; their prefixes are installed directly in `src/` and registered in `tools/native_source_prefixes.py` (verified by `native_source_membership_tests`).

### D. Code Standard & Proof Requirements
1. **Systems Logic (`*_systems.hpp`)**: Named structs (`static_assert(offsetof(...))`), named bitflags/constants, clean control flow, and an `enum class Access` naming every memory access role. No raw hex offsets, register arrays (`gp[]`), or processor flags in the systems header.
2. **Thin Boundary Adapter (`*.hpp`)**: Maps `Access` roles to exact TU1.5 instruction PCs, register updates, stack pushes/pops, EFLAGS, and x87/SSE state. Includes `HALO2_REFERENCE_*` switch and `HALO2_CENSUS_NATIVE_BRIDGES=1` uncursored admission (`native_census_detail::uncursored`).
3. **Proof**:
   - Physical i386 single-stepped oracle (or 8,000-case randomized oracle + boundary/NaN/denormal corpus) covering 100% of reachable owned instructions/branches.
   - Out-of-tree semantic mutation screen (`0` surviving mutants) + guard mutants (`uncertified-required-owner`, `census-without-switch`).
   - ASan + UBSan clean.
   - Live Armory parity (`5627a51badf47762` / `8a9b58b38bef`), live verify (`HALO2_VERIFY_*=1`), Armory census drop, and 8-map MP verify sweep.

---

## 3. Prioritized Checklist of Remaining Translated Routines by Group

Per-frame instruction counts below come directly from the 2026-11-19 Armory census (`build/test-results/shadow-frame-20261119/census/compare.txt`, `123,204/frame` total).

### Group H — Havok Physics World & Vehicle/Arena Runtime

> **Binding mechanism**: `physics_world_runtime_generated.cpp` is whole-file SHA256 pinned; bind via `tools/native_world_leaves_replacements.py` (or a sibling specializer binder in `tools/specialize_native_operands.py`). Note that `physics_world_runtime_generated.cpp` uses canonical (disc) function names (`run_<disc>`) and `ObservedWorldState` with `selected(0x..., s.image)`.

- [ ] **`30E930`** (`4,750/frame`) — `physics_world_runtime_generated.cpp`
  - **Notes**: Currently the #1 largest translated routine in the Armory census. Havok narrowphase/solver hot path. Check canonical entry address via `selected(..., s.image)` mapping in `physics_world_runtime_generated.cpp`.
- [ ] **`125280`** (`3,489/frame`) — `physics_game_arena_runtime_generated.cpp`
  - **Notes**: Load-time / initial-tick arena table initializer (~1M instructions amortized over 300 frames). Pure integer/table loop.
- [ ] **`1D9EC0`** (`1,645/frame`) — `physics_phantom_world_tu15_generated.cpp`
  - **Notes**: Havok phantom AABB / broadphase overlap update.
- [ ] **`2E2E30`** (`1,319/frame`) — `physics_world_runtime_generated.cpp`, `source_scene_device_constructor_tu15_generated.cpp`
  - **Notes**: Multi-unit Havok transform/constraint helper; bind across both owning units.
- [ ] **`1F6610`** (`1,070/frame`) — `physics_world_runtime_generated.cpp`
  - **Notes**: Havok collision agent / shape query helper in `physics_world_runtime_generated.cpp`.
- [ ] **`1F7000`** (`780/frame`) — `physics_world_runtime_generated.cpp`
  - **Notes**: Pairs with already-native `1F76A0` / `1F7C10` / `1F6D10` in `native_tu15_world_leaves_systems.hpp`.
- [ ] **`315C90`** (`722/frame`) — `physics_phantom_force_tu15_generated.cpp`, `physics_world_runtime_generated.cpp`, `source_scene_device_constructor_tu15_generated.cpp`
  - **Notes**: Phantom force / impulse accumulator; shared across 3 units.
- [ ] **`3077F0`** (`720/frame`) — `physics_world_runtime_generated.cpp`
- [ ] **`30CD00`** (`711/frame`) — `physics_world_runtime_generated.cpp`
  - **Notes**: Adjacent to already-native `30CF40` (`10,203 calls` in Armory verify).
- [ ] **`2E5DB0`** (`706/frame`) — `physics_world_runtime_generated.cpp`
- [ ] **`233250`** (`634/frame`) — `physics_world_runtime_generated.cpp`
- [ ] **`2E44D0`** (`520/frame`) — `physics_vehicle_environment_state_tu15_generated.cpp`, `physics_world_runtime_generated.cpp`, `source_collision_...`
  - **Notes**: Note comment in `tools/native_world_leaves_replacements.py` line 25 mentioning `2E44D0`/`2E46E0` explicit-destination packed SSE (`ordered_mul_ps`, `ordered_add_ps`, `ordered_sub_ps`).
- [ ] **`B9770`** (`491/frame`) — `physics_vehicle_update_generated.cpp`
  - **Notes**: Sibling of `E8D80` (`2,204/frame` -> `0.0/frame`) in `physics_vehicle_update_generated.cpp`.
- [ ] **`1D64C0`** (`447/frame`) — `physics_vehicle_motion_generated.cpp`, `physics_world_runtime_generated.cpp`
- [ ] **`1D8B20`** (`427/frame`) — `physics_world_runtime_generated.cpp`
- [ ] **`2E4F30`** (`418/frame`) — `physics_world_runtime_generated.cpp`
  - **Notes**: Adjacent to already-native `2E4FE0` (`9,571 calls` in Armory verify).
- [ ] **`2E3250`** (`407/frame`) — `physics_world_runtime_generated.cpp`, `source_scene_device_constructor_tu15_generated.cpp`

---

### Group F — Frame & Gameplay Misc

> **Binding mechanism**: Specializer binding via `tools/native_frame_misc_replacements.py` (invoked by `tools/specialize_native_operands.py`). `1306B0`, `D7230`, `1EEE80`, `41970`, `133A70`, `153BA0`, and `131610` are already bound there!

- [ ] **`131AC0` (+ `133B40` + `1342B0`)** (`1,962/frame`) — `source_campaign_environment_tu15_generated.cpp`
  - **Notes**: #3 largest remaining routine overall. Pure integer/SSE (`0` x87 instructions). Calls `133B40`, `1342B0`, `32A5C0`, `30BD0`, `1211C0`.
- [ ] **`D8330`** (`1,826/frame`) — `source_damage_delivery_runtime_generated.cpp`
  - **Notes**: #4 largest remaining routine overall. Object/material setup loop in damage delivery runtime.
- [ ] **`1EFD60`** (`1,314/frame`) — `source_multiplayer_engines_tu15_generated.cpp`
  - **Notes**: Direct child of already-native `1EEE80` (`native_tu15_engine_player_update.hpp`). Has `0` x87 instructions! Porting `1EFD60` and `1F0670` completes the `1EEE80` multiplayer engine player update closure.
- [ ] **`14D410`** (`664/frame`) — `physics_live_support_tu15_generated.cpp`, `source_combat_action_tu15_generated.cpp`, `source_model_render_tu15_generated.cpp`
  - **Notes**: Multi-unit leaf/helper across `live_support`, `combat_action`, and `model_render`.
- [ ] **`2B9F0`** (`640/frame`) — `source_texture_camera_producer_tu15_generated.cpp`
  - **Notes**: Texture camera producer frame update.
- [ ] **`1F0670`** (`438/frame`) — `source_multiplayer_engines_tu15_generated.cpp`
  - **Notes**: Second direct child of already-native `1EEE80` (`native_tu15_engine_player_update.hpp`).

---

### Group L — Live Support Remainder (`physics_live_support_tu15_generated.cpp` & Multi-Copy Leaves)

> The project pins the SHA256 of the complete live-support source. The operand specializer binds native owners in its build-local copy. The shared leaves use `tools/native_live_support_leaves_replacements.py`, and `BBD10` uses `tools/native_object_hierarchy_root_replacements.py`.

- [x] **Bind `BBD10` in `physics_live_support_tu15_generated.cpp`.** The build-time operand specializer installs the existing hierarchy-root bridge after checking the complete literal and prepared bodies. The native census counted zero callbacks for the walk in Armory and four-player Coagulation on November 20.
- [ ] **`1D6FB0`** (`1,601/frame`) — `physics_live_support_tu15_generated.cpp`
  - **Notes**: Largest remaining routine in `physics_live_support`.
- [ ] **`2F8B60`** (`842/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`BCDF0`** (`790/frame`) — `physics_vehicle_environment_state_tu15_generated.cpp`, `source_ai_frame_tu15_generated.cpp`, `source_damage_delivery_runtime_generated.cpp`
  - **Notes**: Multi-unit object/hierarchy leaf.
- [ ] **`2E5320`** (`789/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`E4A40`** (`758/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`303800`** (`635/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`DEAE0`** (`628/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`1F8B30`** (`622/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`C85A0`** (`604/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`113D30`** (`596/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`30A020`** (`569/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`314A30`** (`538/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`10E1F0`** (`536/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`1EDE60`** (`524/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`DF880`** (`507/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`D05A0`** (`475/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`E9530`** (`457/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`B9460`** (`432/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`113190`** (`388/frame`) — `physics_live_support_tu15_generated.cpp`, `source_ai_frame_tu15_generated.cpp`

---

### Group E — Effects & Lighting

> **Binding mechanism**: `death_cycle_runtime_tu15_generated.cpp` and `source_combat_action_tu15_generated.cpp` are whole-file SHA256 pinned; bind via `tools/native_effect_owner_replacements.py` (or `tools/native_live_support_leaves_replacements.py` for shared leaves). `source_effect_particle_draw_tu15_generated.cpp` and `native_light_constants_tu15_generated.cpp` can use direct source prefixes registered in `tools/native_source_prefixes.py`.

- [ ] **`30BD0` Census-Only Admission** (`1,575/frame` in census; **already native in production**)
  - **Notes**: `src/physics/runtime/native_tu15_normalize_vector_bridge.hpp` is already native in production, and is guarded by 22 mutation tests in `tools/validate_native_tu15_normalize_mutations.py` that pin exact line occurrences. It only falls back in the census when `cursor == nullptr`. If updating `native_tu15_normalize_vector_bridge.hpp` to admit `native_census_detail::uncursored(s, 0x30bd0u, 0x30c84u)`, update `validate_native_tu15_normalize_mutations.py` in lockstep.
- [ ] **`17C370`** (`978/frame`) — `death_cycle_runtime_tu15_generated.cpp`, `physics_live_support_tu15_generated.cpp`
  - **Notes**: Child of already-native `179CD0` (`CHILD_CALLS` in `tools/native_live_support_leaves_replacements.py`).
- [ ] **`49790`** (`755/frame`) — `native_light_constants_tu15_generated.cpp`
  - **Notes**: Light constants table/shader packaging routine.
- [ ] **`17C140`** (`743/frame`) — `death_cycle_runtime_tu15_generated.cpp`, `native_weapon_effect_advance_tu15_internal.hpp`, `physics_live_support_tu15_generated.cpp`
  - **Notes**: Child of already-native `179CD0`.
- [ ] **`178070` Census-Only Admission** (`693/frame` in census; **already native in production**)
  - **Notes**: `src/rendering/effects/native_tu15_effect_reference_element_admission.hpp` is already bound and native in production across 5 units; it refuses in the census only because `admitted(s)` requires `s.instruction_cursor == &s.services.instruction_cursor` when `s.services.instruction` is set and does not check `native_census_detail::uncursored`.
- [ ] **`1772C0`** (`543/frame`) — `source_effect_particle_draw_tu15_generated.cpp`
  - **Notes**: Effect particle draw routine.
- [ ] **`251C80`** (`530/frame`) — `death_cycle_runtime_tu15_generated.cpp`
  - **Notes**: Particle state / location sibling of `251A60`/`250450`.
- [ ] **`250780`** (`528/frame`) — `death_cycle_runtime_tu15_generated.cpp`
  - **Notes**: Particle state / location sibling of `251A60`/`250450`.
- [ ] **`17C5C0`** (`433/frame`) — `death_cycle_runtime_tu15_generated.cpp`, `physics_live_support_tu15_generated.cpp`
  - **Notes**: Child of already-native `179CD0`.

---

### Group S — Shadow Frame Remainder (`source_shadow_frame_tu15_generated.cpp`)

> **Status**: 10 routines in `source_shadow_frame_tu15_generated.cpp` (`134D30`, `136D50`, `4BAE0`, `135E00`, `136230`, `1365C0`, `1362A0`, `C3AB0`, `1373F0`, `BBD10`, `135690`) are now proven on physical i386 and bound! Only two larger routines (`166C90`, `C3DC0`) and the small `2CBD0` callers remain:

- [ ] **`166C90`** (`1,268/frame`) — `source_shadow_frame_tu15_generated.cpp`
  - **Notes**: 260 lines, 54 x87 lines; calls `1845C0`, `184690`, `134D30`, `166E60`, `17C840`, `166C00`, `136030`.
- [ ] **`C3DC0`** (`894/frame`) — `source_shadow_frame_tu15_generated.cpp`
  - **Notes**: 487 lines, 84 x87 lines; calls `C3A20`, `144090`, `C1E90`, `C3AB0` (now native), `C1BA0`, `C1FB0`, `167790`.
- [ ] **`135EA0` & `136030` (+ `2CBD0`)** (`<350/frame`) — `source_shadow_frame_tu15_generated.cpp`
  - **Notes**: `135EA0` (104 lines, 0 x87) and `136030` (83 lines, 0 x87) are short visibility/draw helpers, but unlike `136A80` (which calls `s.required(0x136ce0u, 0x2cbd0u, false)`), their generated bodies call `run_2cbd0(s)` directly (`2CBD0` is a 904-line routine inside `source_shadow_frame_tu15_generated.cpp`). Either port `2CBD0` first or stub/route `run_2cbd0(s)` through a boundary callback validated by `native_source_membership_tests`.

---

### AI & Campaign Runtime (`source_ai_frame_tu15_generated.cpp`, `source_campaign_runtime_tu15_generated.cpp`)

- [ ] **`1B0020`** (`1,519/frame`) — `source_ai_frame_tu15_generated.cpp`
- [ ] **`1B01E0`** (`1,475/frame`) — `source_ai_frame_tu15_generated.cpp`
- [ ] **`16E510`** (`806/frame`) — `source_campaign_runtime_tu15_generated.cpp`
- [ ] **`2AA6D0`** (`806/frame`) — `source_campaign_runtime_tu15_generated.cpp`
- [ ] **`209F40`** (`487/frame`) — `source_ai_frame_tu15_generated.cpp`
- [ ] **`1B08B0`** (`464/frame`) — `source_ai_frame_tu15_generated.cpp`

---

### Legibility Refactor Backlog (Already Native, Needs `*_systems.hpp` + Adapter Split)

- [ ] **`BD080`, `1532A0`, `14DD70`** matrix leaves (`src/physics/runtime/native_tu15_matrix_leaves.hpp`)
- [ ] **`135B60` (`native_tu15_object_admission.cpp`) & `136A80` (`native_tu15_model_draw_submission.cpp`)** renderer closures

---

## 4. Known Hardware & Physical-i386 Oracle Pitfalls (Read Before Porting!)

1. **RMW and x87 Store Pre-I/O Timing (`at(pc)` before store)**:
   - For read-modify-write instructions (`or [mem], reg`, `and [mem], imm`, `inc [mem]`), call `at(pc)` **once** before the read, then use `store(..., true)` (`positioned=true`) for the write so the instruction callback `before_(s_, pc)` fires only once and sees the pre-read register/flag state.
   - For `fstp dword ptr [mem]` / `fst dword ptr [mem]`, the hardware instruction callback `at(pc)` fires **before** the x87 store/pop changes `status`/`top`/`c1`. Always call `at(pc)` first, execute the x87 store (`fstp_f32` / `fst_f32`), and then record the memory write with `positioned=true`.
2. **C++ Function-Argument Evaluation Order**:
   - GCC evaluates function arguments right-to-left! Never write `store32(Access::store_foo, addr, load32(Access::load_bar, src))` or `call_child(read_arg(a), read_arg(b))` if both arguments touch `Access` roles or mutate registers/stack. Always bind each `load`/`read_arg` to a local `const auto` variable on its own statement in exact execution order.
3. **`comiss` vs `ucomiss` on QNaN**:
   - `comiss` signals `#IA` (setting `MXCSR.IE`, bit 0) on **both** QNaN and SNaN, whereas `ucomiss` only sets `MXCSR.IE` on SNaN. Pass `quiet=false` to `s_.flags.scalar_compare(a, b, false)` for `comiss` and `quiet=true` for `ucomiss`.
4. **Zero-Count `REP MOVSD` / `REP STOSD`**:
   - When `ECX == 0`, hardware `rep movsd` / `rep stosd` still executes the instruction once at `pc` as a no-op (advancing EIP without reading or writing memory). If the trace records instruction PCs or faults, ensure `ECX` is masked/initialized the same way as the caller (`movzx ebx, bx` vs full 32-bit `ebx`).
5. **Capstone Metadata Quirks (`tools/native_shadow_frame_oracle.py`)**:
   - Capstone reports `operand.access == 0` (`read=False, write=False`) on the memory operand of `fstp`/`fst` and `test [mem], reg` (even though `test [mem], imm` has `CS_AC_READ`). `tools/native_shadow_frame_oracle.py` now normalizes `fst`/`fstp` to `write=True` and any `(not read and not write)` memory operand to `read=True`.

---

## 5. Verification & Launcher Promotion Commands

### 1. Run Single-Owner Physical-i386 Validator
```bash
python3 tools/run_test.py -- .tools/python/bin/python -B tools/validate_native_<owner>_tu15.py \
  --xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe" \
  --compiler g++ --report build/test-results/shadow-frame-20261119/<owner>.json \
  --prefixes --require-binding --sanitizers --semantic-mutants
```

### 2. Verify Source-Prefix Membership (`tools/native_source_prefixes.py`)
```bash
python3 tools/run_tests.py --target native_source_membership_tests --filter '^native_source_nonplaying_membership$'
```

### 3. Build Shared Perf Binary & Run Live Armory + Census + MP Sweep
```bash
# 1. Build shared perf binary (capped at 4 jobs for 64 GB RAM safety)
JOBS=4 tools/build_perf.sh

# 2. Run Armory parity + live verify
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 tools/perf_4k60/live_check.py armory build-perf-4k60/halo2_native \
  --out build/test-results/shadow-frame-20261119/armory \
  default \
  reference:HALO2_REFERENCE_<OWNER>=1 \
  verify:HALO2_VERIFY_<OWNER>=1

# 3. Run Armory census
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 tools/perf_4k60/live_check.py census build-perf-4k60/halo2_native \
  --out build/test-results/shadow-frame-20261119/census \
  --reference HALO2_REFERENCE_<OWNER>=1 --owner <entry_hex>

# 4. Run 8-map Multiplayer verify sweep (--workers 1 for 64 GB RAM safety)
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 tools/perf_4k60/live_check.py sweep build-perf-4k60/halo2_native \
  --out build/test-results/shadow-frame-20261119/sweep --workers 1 \
  mp:coagulation:1 mp:coagulation:4 mp:headlong:4 mp:zanzibar:1 \
  mp:containment:4 mp:burial_mounds:4 mp:turf:1 mp:waterworks:4
```

### 4. Promote a New Qualified Build to `Halo2-Launcher`
```bash
./Halo2-Launcher --build-engine
./Halo2-Launcher --launcher-info
```
