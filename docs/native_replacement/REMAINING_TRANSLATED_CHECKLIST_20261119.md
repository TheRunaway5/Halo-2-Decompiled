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
| `130D20` | Geometry cache entry in all five units, including the shadow frame's dispatched `22A790` call | `src/rendering/native_tu15_cache_entry{_systems,}.hpp` | `HALO2_*_CACHE_ENTRY` | 5/5, 14 |
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

Continuation after round 90 (2026-11-24). The production binder now installs
eight more kernels across nine generated routine bodies. Their translated bodies
remain available for reference and admission checks.

| Entry | Kernel | Branches | Rejected mutations |
|---|---|---:|---:|
| `4AC20` | Shadow record scheduling | 11/11 | 16 |
| `1EFD60` | Multiplayer action eligibility | 7/7 | 18 |
| `1DDBF0` | Object physics-group construction | 29/29 | 20 |
| `CAD60` | Vehicle zoom factor | 7/7 | 14 |
| `1E1010` | Vector normalization in the scene and map-world units | 1/1 | 10 |
| `19AE70` | Player departure roster and statistics remap | 18/18 | 17 |
| `1CE8D0` | Animation fraction and frame-clamp tail dispatch | 5/5 | 17 |
| `E05D0` | Horizontal camera basis | 6/6 | 9 |

The focused CTest cohort passed both partition checks and all eight validators.
Each validator runs 8,000 seeded comparisons, ordered replay, census and
ASan/UBSan. The `19AE70` replay and retained path now sequence the player read
first at `19AF88`, matching fault priority in the physical 32-bit `REPE CMPSD`
probe with either direction flag. Both execution routes for `1CE8D0` and `E05D0`
pin the original scalar multiply destination for distinct NaN payloads. GCC
and Clang passed the targeted checks. The existing native `145560` matrix
kernel continues to own all four authenticated copies. Evidence is under
`build/test-results/native-kernel-continuation-20261124/qualification-final/`.

The disposable production-linked runtime matched all three Armory roles over
180 frames and both Zanzibar roles over 360 ticks at four local players. The
Zanzibar verifier matched 27,504 live calls across all eight kernels. With the
reference switches enabled, those kernels accounted for 1,852,167 translated
instructions. The native census counted zero at those eight entries and
107,448,556 across the same sequence, versus 109,300,919 in the reference run.
The final frame matched in both census runs. The runtime receipt and source
hashes are under `build/test-results/native-kernel-continuation-20261124/runtime/`.

### Native conversion goal batch (2026-11-24)

Four additional kernels now bind by default across seven generated bodies.

| Entry | Kernel | Bound copies | Branches | Rejected mutations |
|---|---|---:|---:|---:|
| `106160` | Weapon magnification | 2 | 8/8 | 15 |
| `1E14F0` | Animation fade start and counter rescale | 3 | 6/6 | 12 |
| `2E56A0` | Relative quaternion product | 1 | 1/1 | 12 |
| `1EE3D0` | Unit motion parameters | 1 | 15/15 | 11 |

Armory matched the frame and trace over 180 frames. Four-player Zanzibar
matched its final frame and verified 8,145 calls across the four kernels
over 360 ticks. These owners went from 408,094 translated instruction
observations to zero. The census recorded 108,729,169 total observations in
the native run and 109,137,923 with the four reference switches enabled.
The linked runtime, qualification, and census ranking receipts are under
`build/test-results/native-kernel-goal-20261124/`. The accounting generator
also prepared 279 aliases, covering 9,527 source-qualified functions across
213 canonical units, and its regression suite passed.

### Particle evaluation and physics pose batch (2026-11-24)

The production binder now installs three more kernels across five generated
bodies. The reference switches continue to exercise the pinned bodies.

| Entry | Kernel | Bound copies | Branches | Rejected mutations |
|---|---|---:|---:|---:|
| `116280` | Keyed marker pose | 1 | 7/7 | 17 |
| `2E4350` | Havok rotation to quaternion | 1 | 5/5 | 9 |
| `24FFE0` | Particle property and modifier evaluation | 3 | 9/9 | 14 |

GCC and Clang passed seeded differential, instruction census, ordered store
replay, and sanitizer checks. The particle property kernel calls the original
authored-curve owner through its regular dispatch. CTest passed both partition
checks, the execution-accounting suite, and all three validators. Receipts
are under `build/test-results/native-kernel-goal-20261124/physics-particle-qualification/`.

A disposable build linked the changed production objects against the Round 90
baseline. Armory matched its final frame and trace over 180 frames. Zanzibar
matched its final frame and verified 72,573 calls over 360 ticks with four
local players. The Zanzibar census went from 76,763 translated instruction
observations at `116280`, 279,962 at `2E4350`, and 2,778,288 at `24FFE0` to
zero at all three. The overall census recorded 106,139,955 observations with
native binding and 109,275,117 with the three reference switches. Runtime
receipts are under
`build/test-results/native-kernel-goal-20261124/physics-particle-runtime/`.

`10DFA0` now uses native object-type notification dispatch in physics live
support and combat action. The adapter preserves the signed type and slot
indices, callback order, and registers returned by each callback. GCC and
Clang agreed across both copies. The tests exercised all five branches and
rejected eleven mutations. Both production units compiled with their regular
flags, and the CTest validator and accounting check passed. A separate
disposable build matched Armory's final frame and trace over 180 frames and
Zanzibar's final frame over 360 ticks with four local players. Zanzibar
verified 35,553 calls and counted zero translated observations at `10DFA0`,
down from 1,654,179 with the reference switch enabled. Evidence is under
`build/test-results/native-kernel-goal-20261124/type-notification-qualification/`
and `build/test-results/native-kernel-goal-20261124/type-notification-runtime/`.

The production `13DE70` authored-function adapter covers null, empty, and invalid
inputs and kinds 0, 1, 6, and 8. Other kinds resume at the selected source
label after the native prologue has published its complete state. GCC and
Clang matched both retained source copies, covering all eleven branches and
rejecting fourteen mutations. The census fixture counted 845 native completions
and 659 resumptions with 35,387 translated instruction callbacks. Live Armory
matched its frame and trace, with 920 completed native calls and 541 resumptions.
Four-player Zanzibar matched the final frame and verified 67,441 calls, of
which 63,036 completed natively. The uncursored Zanzibar census went from
2,046,546 translated observations to 224,839. It retained 20 kind-4, 114
kind-5 and 4,271 shared kind-7/10 entries. The runtime and compiler receipts
are under `build/test-results/native-kernel-goal-20261124/authored-function-runtime/`
and `build/test-results/native-13de70-{gcc,clang}.json`.

The World support search at TU1.5 `3077F0` corresponds to Map Pack `3037F0`
and now binds natively for TU1.5. GCC and Clang each passed 48,000 comparisons
across literal, census, replay, and PC-remapping modes, including all 17
branches and nine rejected mutations. Armory verified 1,486 calls and matched
its final frame and trace. Four-player Zanzibar verified 17,760 calls and
matched its final frame. Its World census dropped from 3,353,515 translated
observations to zero. A probe using the actual legacy scheduler selected World
for all 82 entries exposed by Contact, including this one. Evidence is under
`build/test-results/native-kernel-goal-20261124/world-soa-runtime/`,
`world-soa-support/`, and `world-soa-support-production/`.

World swept-shape support `2E5DB0` now calls the wrapped shape through the
existing virtual dispatch, then selects the sweep endpoint and restores the
vertex feature tag after the two vector stores complete. GCC and Clang each
matched 48,000 comparisons and rejected eight mutations, with sanitizer
checks and verified fault prefixes at the vector halves and tag store.
Armory verified 5,577 calls and matched the frame and trace. Four-player
Zanzibar matched its final frame, verified 21,711 calls, and counted zero
translated observations at `2E5DB0`, down from 830,948. The differential
and runtime receipts are under `build/test-results/native-kernel-goal-20261124/`
in `world-sweep-support/` and `world-swept-support-runtime/`.

World collection filter `1CBE70` now binds natively. GCC and Clang each
passed 311 physical TU1.5 comparisons, 4,134 comparisons against the retained
source, sanitizer checks, and six mutation controls. Armory verified 465 calls
and matched its final frame and trace. Four-player Zanzibar verified 6,364
calls, matched its final frame, and went from 339,182 translated observations
to zero. Receipts are under `build/test-results/native-kernel-goal-20261124/`
in `world-collection-filter/` and `world-collection-filter-runtime/`.

### Authored, particle LOD and MOPP continuations (2026-11-25)

The default `13DE70` binding now covers successful paths for kinds 4, 5,
and 7 and the shared clamp, in addition to the prior native entry and leaf
paths. Powered preprocessing and other unconverted cases retain their
original branches and child dispatch. A 360-tick Zanzibar cohort with the
normal engine cursor recorded 67,840 activations. It reduced entries doing
translated work from 4,804 to 399 and translated statements from 226,679
to 10,650, compared with the previously active prefix. Armory matched its
frame and trace with 5,410 fewer translated statements. The generated aliases
keep all eight authenticated replay helpers separate from the live body.

In `251C80`, the observer distance and LOD scan at `251D00` now runs natively
before resuming at `251E30`. Zanzibar kept the same 5,723 owner activations
and reduced translated statements from 2,114,607 to 562,593. Armory matched
its frame and trace and verified the scan 603 times. GCC and Clang each
passed 12,876 component comparisons, including 5,676 fault prefixes, plus
sanitizer and mutation checks. The private emitter tail at `251E30` also
passed component and syntax checks. Its binding remains pending.

`30AA70` now uses the native prologue in the two authenticated Death and AI
owners. The original dispatcher starts at `30AAA0` after a valid entry.
An invalid entry resumes at `30AE16`. Zanzibar used the native prologue in
all 14,539 Death calls and went from 7,522,689 translated statements to
7,304,604. The reference switch sent those same entries through the retained
prologue. This live cohort exercised the Death owner. The AI owner passed
component and production syntax checks. Both Armory and Zanzibar matched their
reference outputs in the combined build. Runtime and CTest receipts are under
`build/test-results/native-kernel-goal-20261124/continuation-routes-runtime/`
and `continuation-routes-qualification/`.

The follow-up Zanzibar profile assigns 1,240,690 literal observations to the
source interval beginning at the scale tag `30B36A`. It also records 491,148
at the `30AE02` resume block. This profile is under
`build/test-results/native-kernel-goal-20261124/mopp-ray-frontier/`.

The native `1772C0` particle draw entry resolves the element definition and
stages its shape before resuming the original draw or descendant path. It
preserves all six original child call sites and the returned register state.
GCC and Clang each passed 24,000 component comparisons, sanitizer runs, and
six mutation controls. Armory matched its frame and trace and verified 539
calls. Four-player Zanzibar verified all 17,084 entries and matched its
final frame. Direct source accounting reduced statements in the owner from
3,179,885 to 2,359,853. The two subsequent position continuations passed
private differential checks and await production binding. Evidence is under
`build/test-results/native-kernel-goal-20261124/particle-draw-entry-runtime/`
and `particle-draw-frontier/`.

The earlier full-callback Zanzibar diagnostic counted 5,148,917 observations
at `30AA70`.
The private hosted MOPP controller now publishes two ordered 8-byte stores
per vector store and preserves the first half if the second fails. GCC and
Clang passed the hosted event probes. The physical reference suite passed
144 cases, including 120 alignment faults. A six-case prepared runtime-memory
probe matched the retained path's ordered stores, final memory, and ABI. Its
native path took longer in five of six cases. Production binding still needs
runtime qualification and timing. The hosted controller now commits ESP
before a retained PUSH or CALL store, and commits x87 conversion status
before the store but pops only after its observer completes. The hosted and
forced-scoped CTests passed, and the original physical suite passed again.
An isolated 8-byte warm-store experiment reduced the hosted scale fixture's
median from 2,813 ns to 1,004 ns. The retained route took 633 ns in the same
overlay. The hosted path remained slower in five of six fixtures. The
provider change was tested in scratch. Hosted key-base adds at `30B46B` and
`30B4B4` now publish flags before store callbacks. GCC and Clang pass the
fault matrix and reject delayed flag publication. The hosted controller now
defers x87 ownership until a callback or geometry stage needs it. With the
stock provider, the empty and jump fixtures took 92 ns and 102 ns in the
private native path, compared with 141 ns and 185 ns retained. Scale still
took 1,021 ns native versus 666 ns retained with the 8-byte overlay. A
native prologue resumes at the original `30AAA0` dispatcher and
matched both prepared owners, including injected provider and observer
faults. It improved all 13 timing fixtures in each owner. Eight unchanged
i386 executions independently matched the complete state at the scale
entry. The full native controller remains private.
Evidence is under `build/test-results/native-kernel-continuation-20261124/`
in the `mopp-ray-hosted-*` reports.

### Particle location, particle draw position and resident MOPP dispatch (2026-11-25)

The production `251C80` binding now covers entry and forced LOD, the
observer scan, and the emitter tail. The original resolver, allocator and
emitter-update child calls keep their dispatch. Armory ran all 603 entries
without translated statements in the owner. Four-player Zanzibar ran 5,633
of 5,723 entries without translated statements in the owner. Coagulation
ran 4,076 of 4,140. The remaining 154 multiplayer entries exercised the
retained body. Their admission state still needs diagnosis.

The `1772C0` position-copy and axis-offset continuations and the `30AA70`
resident empty and short-jump dispatcher arms now run natively in their
prepared owners. The linked build exercised the MOPP dispatcher in Death.
The AI owner passed component and production syntax checks. The same linked
build also includes the two authenticated `13DE70` kind-10 preprocessing
sites, with their original required callees. Component fixtures exercised
both sites. The three live cohorts had zero powered replay activations.

Incremental live literal counts use the same binary and the normal cursor.
The reference switches restore only the new stages. All three final frames
matched. The 180-frame Armory trace also matched. All 10 focused CTests
passed.

| Cohort | `251C80` reference to native | `1772C0` reference to native | `30AA70` reference to native |
| --- | ---: | ---: | ---: |
| Armory, 180 frames | 53,839 to 0 | 71,605 to 60,418 | 0 to 0 |
| Zanzibar, four local players, 360 ticks | 562,593 to 29,729 | 2,359,853 to 2,026,595 | 7,304,604 to 7,283,202 |
| Coagulation, four local players, 240 ticks | 432,568 to 22,474 | 2,760,508 to 2,462,659 | 1,320,761 to 1,317,503 |

The linked build retired 1,663,751 translated statement observations across
these runs. The linked candidate and raw captures were disposed of. The
small runtime and CTest receipts are under
`build/test-results/native-kernel-goal-20261124/native-frontier-runtime/`
and `build/test-results/native-kernel-goal-20261124/native-frontier-qualification/`.

The next `1772C0` visibility continuation at `177594` to `17764A` keeps
the original `167EF0` and `184690` call boundaries. GCC and Clang each
passed 24,000 component comparisons and 4,500 sanitized comparisons, plus
ordinary and accounted syntax on privately prepared aliases. The unapplied
patch and proof are under
`build/test-results/native-kernel-goal-20261124/particle-draw-frontier/visibility-stage*`.
The staged MOPP scale-byte conversion covers `30B37D` to `30B3AA`. Stock
provider timings saved 54–68 ns for Death and 89–97 ns for AI on scale calls.
Physical reference prefixes and component tests passed. That patch and
proof are under
`build/test-results/native-kernel-continuation-20261124/mopp-ray-scale-translation-*`.

### Particle visibility, emitter transforms and MOPP scale bytes (2026-11-25)

The visibility interval `177594` through `17764A` is now bound in `1772C0`.
The emitter update `250780` now binds its entry and transform publication
through `2508A2`. It keeps the three original children and resumes at
`2508A5` for a transform or `25097B` for emission without one. The
resident MOPP byte-scale stage is also bound in Death and AI. The paired
runtime comparison used a disposable linked binary and switches specific
to these three stages. All 13 focused CTests passed.

| Cohort | `1772C0` reference to native | `250780` reference to native | `30AA70` reference to native |
| --- | ---: | ---: | ---: |
| Armory, 180 frames | 60,418 to 42,295 | 99,547 to 45,880 | 0 to 0 |
| Zanzibar, four local players, 360 ticks | 2,026,595 to 1,359,448 | 1,130,509 to 604,964 | 7,283,202 to 7,012,506 |
| Coagulation, four local players, 240 ticks | 2,462,659 to 1,652,706 | 887,479 to 483,419 | 1,317,503 to 1,268,591 |

All three final frames and Armory's gameplay trace match the same-binary
reference and the previous paired build. These tranches retired 2,798,103
translated statement observations. Zanzibar exercised the native MOPP
scale stage 22,558 times and Coagulation 4,076 times, all in Death. Armory
and Zanzibar replay verified both the visibility and emitter transform
kernels. Runtime and focused test receipts are under
`build/test-results/native-kernel-goal-20261124/native-visibility-scale-runtime/`
and `native-visibility-scale-qualification/`.

The build counted every `251C80` entry refusal. All 90 Zanzibar refusals
and all 64 Coagulation refusals came from the store-observer policy.
Armory admitted all 603 calls. The owned observability contract needs to
be tested against the exact multiplayer observer before expanding this
binding.

Three follow-ups have private proofs and staged, unapplied registration:

- `1772C0` view preparation and selection at `177376` through `177399`
  and `177447` through `177498`, including the next-view entry at `177453`.
  The earlier Zanzibar profile measured 517,502 observations at these
  intervals. GCC and Clang each passed 24,000 component comparisons,
  4,500 sanitizer comparisons, and ordinary and accounted syntax checks.
  The patch and proof are under `particle-draw-frontier/view-preparation-stage*`.
- MOPP opcode resume `30AE02`. The resident stage loads the real opcode,
  attempts the original sentinel write, and preserves the original
  dispatcher and error routes. A refused write resumes the original store
  with the expected EAX and flags. Stock same-binary 64-jump chains saved
  12.0–12.6 ns per resume and unchanged i386 fixtures matched 10 cases.
  The patch and proof are under
  `build/test-results/native-kernel-continuation-20261124/mopp-ray-opcode-resume-*`.
- `13E9A0` control-point lookup has a dedicated required owner. It keeps the
  existing caller dispatch and authenticated legacy dispatch tables.
  GCC and Clang passed 73,600 physical-oracle comparisons, 8,096 sanitizer
  comparisons, and 11,776 linked through the particle-world exports.
  The patch and proof are under `build/test-results/native-13e9a0-control-point-*`.

### Particle view setup, emission, observer admission and MOPP resume (2026-11-25)

The default runtime now binds the `1772C0` view setup and selection,
the shared `250780` emission tail at `25097B`, and the resident `30AE02`
opcode resume in the MOPP dispatcher. The four production routers also
link the dedicated `13E9A0` required owner. Its original-byte and linked
particle API comparisons exercised both success and read faults.

The particle-location and emitter bridges accept the exact borrowed
weapon-effect birth-color collector. The callback receives the original
store preimages and writes in order. Component tests rejected copied,
wrapped, expired, and unqualified observers. The paired reference runs
restored the earlier 90 Zanzibar and 64 Coagulation fallbacks. All 10,466
`251C80` calls across the three default cohorts
returned without reaching a translated statement in that owner.

| Cohort | `1772C0` reference to native | `250780` reference to native | `251C80` reference to native | `30AA70` reference to native |
| --- | ---: | ---: | ---: | ---: |
| Armory, 180 frames | 42,295 to 26,125 | 45,880 to 27,135 | 0 to 0 | 0 to 0 |
| Zanzibar, four local players, 360 ticks | 1,359,448 to 841,946 | 604,964 to 274,397 | 29,729 to 0 | 7,012,506 to 6,521,358 |
| Coagulation, four local players, 240 ticks | 1,652,706 to 1,007,458 | 483,419 to 208,620 | 22,474 to 0 | 1,268,591 to 1,168,719 |

The paired runs retired 2,446,254 translated statement observations. All
three final frames and Armory's gameplay trace matched both the paired
reference and the previous cohort. Armory and Zanzibar also passed ordered
replay for the new particle stages. All 16 focused tests passed. Receipts
are under `build/test-results/native-kernel-goal-20261124/` in
`native-view-resume-emission-runtime/` and `native-view-resume-emission-qualification/`.

The next staged `250780` span covers the remaining orientation interval
`2508A5` through `250976`. With transform and emission enabled, all three
native spans cover the complete routine. The draw-distance stage in
`1772C0` covers `17764E` through `1776D7`. Each compiler passed 24,000
component and 4,500 sanitizer comparisons, including the x87 spill and
genuine `252EC0` child boundary. Their staged proofs are under
`particle-emitter-orientation-2508a5/` and `particle-draw-frontier/distance-stage*`.
The staged `131AC0` campaign fog-resolution tail has both source-pinned
comparisons and a linked public-entry proof under
`build/test-results/native-131ac0-fog-resolution-*`.

### Emitter update closure, particle birth and campaign fog (2026-11-25)

The `250780` orientation continuation at `2508A5` is now native. The
entry, orientation and emission spans cover the whole emitter update.
All 11,268 `250780` entries in the three default workloads completed with
zero translated statements in that owner. The native birth routine prefix
at `250C60` preserves its allocator and resolver calls and resumes at the
original property evaluation or early epilogue. The draw-distance stage in
`1772C0` keeps the original x87 spill and `252EC0` query. Death and AI both
bind the MOPP scale-property tail at `30B40D`, with exact original i386
comparisons and native and forced-reference continuation tests.

| Cohort | `250780` reference to native | `250C60` reference to native | `1772C0` reference to native | `30AA70` reference to native |
| --- | ---: | ---: | ---: | ---: |
| Armory, 180 frames | 27,135 to 0 | 22,540 to 10,764 | 26,125 to 15,565 | 0 to 0 |
| Zanzibar, four local players, 360 ticks | 274,397 to 0 | 1,017,264 to 467,376 | 841,946 to 522,546 | 6,521,358 to 6,250,662 |
| Coagulation, four local players, 240 ticks | 208,620 to 0 | 790,815 to 341,919 | 1,007,458 to 604,812 | 1,168,719 to 1,119,807 |

The campaign fog accumulation and resolution stages now bind in `131AC0`.
The weighted linked fixture exercised both stages and the genuine `1342B0`
child. Armory verified 180 live resolution calls and reduced the owner's
translated statements from 353,160 to 346,860. The live workloads did not
verify weighted accumulation. The MOPP stage ran in the live Death owner.

Across these runs, 2,579,226 translated statement observations were
retired. All three final frames and Armory's gameplay trace matched their
paired references and the prior default outputs. All 21 focused tests
passed. Runtime and component receipts are under
`build/test-results/native-kernel-goal-20261124/native-orientation-birth-fog-runtime/`
and `native-orientation-birth-fog-qualification/`.

The next staged draw changes cover view mask and iteration plus the
recursive descendant return. Both compilers passed 24,000 comparisons and
4,500 sanitizer comparisons per stage. The combined patch is under
`particle-draw-frontier/routing-subtree-stage-staging.patch`. The staged
`250C60` placement span at `250E67` to `251136` passed its independent
component and production syntax checks. The remaining birth interface and
epilogue are in progress. Campaign cluster completion and the entire
`1342B0` child have combined staged and linked proof under
`build/test-results/native-131ac0-fog-cluster-finish-*`. The staged
`30ADE0` MOPP plane comparison has production syntax and unchanged i386
proof under `build/test-results/native-kernel-continuation-20261124/mopp-ray-plane-compare-*`.

### Particle birth placement, draw routing and MOPP plane compare (2026-11-25)

The native `250C60` placement span reaches `251136` and preserves the
property, vector and collision calls. The particle draw `1772C0` now uses
native view mask and iteration paths, plus the recursive descendant stage.
The original descendant call and return ABI remain in use. The MOPP owner
now binds the `30ADE0` plane comparison in Death and AI. Its physical
reference comparisons and production fault tests passed.

The native fog-cluster completion stages at `131FC7`, `132084` and `13208C`
run through the original campaign dispatch. Armory verified 5,940 calls
and reduced `131AC0` from 346,860 to 262,800 translated statements. The
whole `1342B0` patchy-fog child also binds natively. The weighted linked
fixture exercised the genuine child call and matched all 247 ordered stores
across the public campaign call. That fixture covers an input absent from
the bounded live runs.

| Cohort | `250C60` reference to native | `1772C0` reference to native | `30AA70` reference to native |
| --- | ---: | ---: | ---: |
| Armory, 180 frames | 10,764 to 2,024 | 15,565 to 3,168 | 0 to 0 |
| Zanzibar, four local players, 360 ticks | 467,376 to 94,512 | 522,546 to 139,321 | 6,250,662 to 5,755,780 |
| Coagulation, four local players, 240 ticks | 341,919 to 77,154 | 604,812 to 128,561 | 1,119,807 to 1,010,123 |

The paired linked runs retired 2,206,868 translated statement observations.
All three final frames and Armory's gameplay trace matched their references
and the previous default outputs. `251C80` and `250780` kept zero translated
statements across the three cohorts. All 24 focused tests passed. Runtime
and test receipts are under `build/test-results/native-kernel-goal-20261124/`
in `native-placement-cluster-plane-runtime/` and
`native-placement-cluster-plane-qualification/`.

The next `1772C0` patch adds basis construction, local matrix application
and draw submission. The staged normal and accounted aliases compile with
GCC and Clang. All 349 original literal sites map to the 16 pinned replay
scopes, covering the complete source routine. The patch and component proofs
are under `particle-draw-frontier/complete-particle-draw-staging.*`.
The final attachment, notification and epilogue stages of `250C60` also
passed combined production syntax, exact fault tests and ordered store
comparisons with the earlier allocation and placement stages. The inactive
patch is under `particle-birth-interface-25113a/`. Both whole-routine
bindings passed the linked workload check below.

The next MOPP axial slab comparisons and campaign `133B40` classifier and
projection are ready for integration. Proofs are under
`build/test-results/native-kernel-continuation-20261124/mopp-ray-slab-interval-*`
and `build/test-results/native-133b40-fog-plane-projection-*`.

### Particle draw and birth completion (2026-11-25)

The production aliases bind all 349 original sites in `1772C0` and all
430 in `250C60`. Native draw preserves the original submission child calls.
Native birth preserves the ordered attachment, notification and return
effects and uses the typed birth collector. The linked default engine
returned from all 47,320 entries across both routines without entering
their translated bodies. `251C80` and `250780` also stayed at zero
translated statements in these workloads.

| Workload | `1772C0` native returns | `250C60` native returns | Translated statements retired in this batch |
| --- | ---: | ---: | ---: |
| Armory, 180 frames | 539 | 92 | 27,872 |
| Zanzibar, four local players, 360 ticks | 17,084 | 4,296 | 1,297,981 |
| Coagulation, four local players, 240 ticks | 21,802 | 3,507 | 393,700 |

The totals include the `30AA70` MOPP slab comparisons and packed scale
vectors. MOPP fell from 5,755,780 to 4,691,632 translated statements on
Zanzibar and from 1,010,123 to 822,138 on Coagulation with the same route
counts. The `133B40` fog plane classifier and member projection fell from
36,360 to 13,680 statements across 180 Armory entries. The paired runs
matched all three final frame hashes and the Armory gameplay trace, including
the previous defaults. All 29 focused tests passed.

Runtime and focused test receipts are under
`build/test-results/native-kernel-goal-20261124/native-draw-birth-complete-runtime/`
and `build/test-results/native-kernel-goal-20261124/native-draw-birth-complete-qualification/`.
Both launcher selection files still identify the same hashed installed
engine. Owned scratch directories were removed, with only small reports and
logs retained.

Staged changes cover Effect Frame `1778D0` setup and drain, `30B1F9`
MOPP endpoint order, and the remaining `133B40` fog plane transforms and
output. The selected damage-delivery prefix at `D8330` also has a staged
production build binding and pinned reference. GCC and Clang each passed
24,000 release and 4,500 sanitized comparisons and rejected eight source
and operand mutations. These proofs are under
`build/test-results/native-kernel-goal-20261124/damage-update-entry/`.

### Particle-system update, damage entry and fog closure (2026-11-25)

The active Effect Frame `1778D0` binding covers system setup, the location
walk, drain, attachment preparation and recursive updates. The linked
engine returned from all 24,485 measured entries without entering the
translated body. The attachment selection and computed geometry branches
outside these runs still require conversion.

The selected-observation damage owner now binds the `D8330` prefix in the
production damage library. It preserves the real descriptor child and
resumes at the original three destinations. Independent verification matched
96,703 ordered accesses across 7,502 Armory calls and 132,168 across 8,564
Zanzibar calls. The Death and AI owners also bind `30B1F9` MOPP endpoint
ordering and the mirrored slab delta comparisons at `30B264` and `30B2C7`.

| Workload | `1778D0` reference to native | `D8330` reference to native | `30AA70` reference to native | Batch total retired |
| --- | ---: | ---: | ---: | ---: |
| Armory, 180 frames | 35,493 to 0 | 349,466 to 154,606 | 0 to 0 | 243,673 |
| Zanzibar, four local players, 360 ticks | 944,355 to 0 | 925,627 to 656,884 | 4,691,632 to 4,285,597 | 1,619,133 |
| Coagulation, four local players, 240 ticks | 749,534 to 0 | 545,223 to 384,735 | 822,138 to 754,764 | 977,396 |

The matched frames and Armory gameplay trace cover 2,840,202 retired
translated statements in this batch and 4,559,755 across both batches.
The Armory total includes the `133B40` transform and output stages, which
reduced the owner from 13,680 statements to 360. The final `133D8A` call
handoff was enabled after this three-workload build. The current aliases
cover all 492 original sites and retain the genuine `11F7E0` child. A
separate linked proof matched five public fixtures, including an x87
stack-full fault, with zero translated `133B40` statements.

Ten focused tests passed. Four older damage tests skipped because their
reference JSON files are absent. The paired runtime, remaining-PC census
and test reports are under `build/test-results/native-kernel-goal-20261124/`
in `native-particle-system-damage-fog-runtime/` and
`native-particle-system-damage-fog-qualification/`. The full fog-plane
fixture proof is `build/test-results/native-133b40-fog-plane-call-accounted-linked.json`.
The test runners removed the owned scratch directories and retained only
small receipts. Both launcher selections still authenticate the installed
engine.

The staged `D8330` return epilogues preserve the real `DB0A0` notification.
GCC and Clang each passed 24,000 release and 4,500 sanitized comparisons in
literal, census and replay modes. A separate native diagnostic census found
54,463 Armory observations and 96,546 Zanzibar observations in these three
return spans. The staged patch and source proofs are under
`build/test-results/native-kernel-goal-20261124/damage-update-return/`.

### Fog handoff, damage returns and MOPP axis setup (2026-11-25)

The final `133D8A` handoff preserved the genuine `11F7E0` child in the
representative linked engine. All 180 `133B40` Armory entries returned
without entering the translated body. The `132188` projected fog merge
removed another 22,680 parent statements from `131AC0`. Two separate
linked fixtures populated all three merge groups and covered both outcomes
of the genuine optional `1342B0` call.

The three `D8330` return epilogues removed 54,463 translated statements in
Armory, 96,546 in Zanzibar, and 56,859 in Coagulation. The MOPP axis
setup and narrow offsets removed 602,144 in Zanzibar and 140,604 in
Coagulation. All three final frames and Armory's gameplay trace matched.
The paired run retired 973,656 translated statements in total. The
particle-system selection and fixed basis stages are bound, with zero
translated `1778D0` statements in either mode across these three workloads.

Fourteen tests passed and four older damage tests skipped for missing
reference fixtures. The matching runtime and focused reports are under
`build/test-results/native-kernel-goal-20261124/native-return-merge-selection-axis-runtime/`
and `build/test-results/native-kernel-goal-20261124/native-return-merge-selection-axis-qualification/`.
Both launcher selections passed checksum verification after scratch cleanup.

### Complete particle-system update and expanded damage and MOPP coverage (2026-11-25)

The Effect Frame owner `1778D0` now binds all 372 original instruction
sites. The ordinary build validator checks that nine replay definitions
cover exactly the complete original set. In the 900-tick Burial Mounds
run, verification exercised 509 contact bases, 601 velocity bases and
1,110 shared final lane operations. The two geometry switches restored
64,509 translated statements in the reference run. The native engine
returned from all 24,912 calls without entering the translated body, and
the final frame matched.

The `D8330` countdown advance and depletion loops cover 48 more original
sites. The pinned machine instructions at `D8AA1` and `D8AA5` confirmed
the FISTP rounding status. The native helper matched all 24 original i386
integer and x87 status results on GCC and Clang. The Death and AI `30AA70`
owners gained native short and wide slab bounds, plus wide plane setup.

| Workload | `131AC0` reference to native | `D8330` reference to native | `30AA70` reference to native | Batch total retired |
| --- | ---: | ---: | ---: | ---: |
| Armory, 180 frames | 240,120 to 18,180 | 100,143 to 73,711 | 0 to 0 | 248,372 |
| Zanzibar, four local players, 360 ticks | 0 to 0 | 560,338 to 369,755 | 3,683,453 to 2,923,364 | 950,672 |
| Coagulation, four local players, 240 ticks | 0 to 0 | 327,876 to 216,609 | 614,160 to 484,490 | 240,937 |

All three final frames and the Armory gameplay trace matched. This cohort
retired 1,439,981 translated statements. The separate Burial Mounds run
retired another 64,509. Twenty-one focused tests passed and four older
damage tests skipped because their reference JSON is absent. Runtime and
focused reports are under `build/test-results/native-kernel-goal-20261124/`
in `native-particle-system-closure-sections-fog-mopp-runtime/` and
`native-particle-system-closure-sections-fog-mopp-qualification/`.
The runners removed owned scratch files and both launcher selections
passed checksum verification.

The next staged damage depletion gate and notification handoff preserve
the real `DB0A0` call. GCC and Clang each passed 24,000 release and 4,500
sanitized comparisons across the 45 original sites. The pending patch and
proofs are under `build/test-results/native-kernel-goal-20261124/damage-update-notification/`.

### Complete campaign fog parent (2026-11-25)

The active `131AC0` binding covers all 786 original static instruction
sites through 14 authenticated replay definitions. The seven original
child call sites retain their production routes. Six linked public
fixtures exercised bloom, active bloom, patchy fog, environmental fog,
missing sky and uncovered sky. Each matched the reference state and
ordered stores with zero translated parent statements. All eleven
focused fog suites passed.

The linked Armory engine matched the final frame and gameplay trace in
default, forced-reference and verification modes. Default and verify each
returned from all 180 `131AC0` calls without entering the translated body.
The last stages retired 18,180 translated statements. The original
`133A70` cluster child ran 5,940 times and the original `133B40` child ran
180 times in each mode. The linked report is in
`build/test-results/native-kernel-goal-20261124/native-full-fog-armory-runtime/parity.json`.
The source-only validator and linked checks are in
`build/test-results/native-131ac0-fog-whole-validator.json` and
`build/test-results/native-131ac0-fog-whole-linked.json`.

The fog handoff, particle closure and full fog batches, including Burial
Mounds, eliminated 2,496,326 translated statement executions. The test
runners removed owned scratch and both launcher selections passed their
existing checksum checks.

The damage vitality gate at `D85CB` and `D85D9` is staged with the actual
`15B2D0` scale child preserved. GCC and Clang each passed 24,000 release and
4,500 sanitized comparisons. Proofs are under
`build/test-results/native-kernel-goal-20261124/damage-vitality-gate/`.
The `30B0D1` MOPP decreasing-child resume is staged with 144 unchanged
i386 comparisons and tests for faults at either half of its vector writes.
Its reports are under `build/test-results/native-kernel-continuation-20261124/mopp-ray-decreasing-resume-*`.

### Particle drain counts, damage notification and MOPP resume (2026-11-25)

Effect Frame `178240` and `252060` now bind all 24 and 18 original sites.
The outer count preserves the real call to the emitter count. Across
Armory, Zanzibar and Coagulation, all 49,111 entries returned without
entering the translated bodies. Those two owners account for 1,043,660
of the translated statements removed in this cohort.

The `D8330` depletion gate and notification handoff preserve the original
`DB0A0` child. The native vitality and shield gate also retains the
`15B2D0` scale call. Live verification matched the ordered accesses for
both stages. The `30B0D1–30B178` MOPP decreasing-child resume preserves
the real child and the individual fault boundaries of each split store.

| Workload | `178240` and `252060` native returns | `D8330` reference to native | `30AA70` reference to native | Batch total retired |
| --- | ---: | ---: | ---: | ---: |
| Armory, 180 frames | 540 and 539 | 73,711 to 35,097 | 0 to 0 | 61,262 |
| Zanzibar, four local players, 360 ticks | 13,282 and 13,672 | 369,755 to 158,378 | 2,923,364 to 2,857,224 | 850,683 |
| Coagulation, four local players, 240 ticks | 10,663 and 10,415 | 216,609 to 91,925 | 484,490 to 478,824 | 578,196 |

All three final frames and Armory's gameplay trace matched. This cohort
retired 1,490,141 translated statements. The runtime report is under
`build/test-results/native-kernel-goal-20261124/native-damage-notify-vitality-decreasing-runtime/`.
The finished runs removed their scratch paths and both launcher
selections passed checksum verification.

### Particle contact, damage descriptor and MOPP geometry (2026-11-25)

The first `2C2D90` native tranche covers 104 of 401 sites. It handles
contact setup, the linked particle walk, early checks, aging and return.
Zanzibar verification matched 2,676 setup and 4,811 traversal replays.
All 2,676 owner entries completed, and the final frame matched.

The `D8330` descriptor refresh, response flag dispatch and shield refill
and armor inputs now execute through the native bindings. The selected
image has 300 original sites covered by active replay definitions and 16
more sites in the previously native pending-response countdown. The
scope audit identifies the two outstanding spans at `D842C–D84D7` and
`D86F2–D8A05`. The Death and AI MOPP owners also bind `30AE55` plane-split
classification and `30AE75` geometry preparation. Both reference routes
retained ordered provider effects and exact x87 status on their physical
TU1.5 controls.

| Workload | `2C2D90` reference to native | `D8330` reference to native | `30AA70` reference to native | Batch total retired |
| --- | ---: | ---: | ---: | ---: |
| Armory, 180 frames | 0 to 0 | 35,097 to 4,672 | 0 to 0 | 30,425 |
| Zanzibar, four local players, 360 ticks | 683,788 to 252,500 | 158,378 to 28,401 | 2,857,224 to 2,354,197 | 1,064,292 |
| Coagulation, four local players, 240 ticks | 434,318 to 147,791 | 91,925 to 16,704 | 478,824 to 393,704 | 446,868 |

All three final frames and the Armory gameplay trace matched. Both
particle drain counts and the complete campaign fog owners stayed
native in the same runs. The two latest batches retired 3,031,726
translated statements. Thirty-four focused tests passed and four legacy
damage tests skipped because their reference fixtures are missing.
The linked runs, per-PC diagnostic profile and focused tests are under
`build/test-results/native-kernel-goal-20261124/` in
`native-contact-descriptor-geometry-runtime/` and
`native-contact-descriptor-geometry-qualification-final/`. Finished test
runs removed scratch and both launcher selections passed checksum verification.

The standalone `D86F2–D87CC` shield recovery stage preserves three real
child targets. It is staged under `damage-shield-recovery/` with 48 sites
and passing GCC and Clang release and sanitizer proof. The due-response
and response-amount proofs are `build/test-results/native-d8330-due-response-*`
and `build/test-results/native-d8330-response-amount-*`. Particle contact
flag and query stages are staged under `particle-contact-flags/` and
`particle-contact-query/`. The MOPP dispatcher table stage is under
`build/test-results/native-kernel-continuation-20261124/mopp-ray-dispatch-tables-*`.

### Damage update and particle contact closure (2026-11-25)

The selected TU1.5 `D8330` production binding now covers all 545 instruction
addresses. Twelve exact replays cover 530 placements, including the repeated
string observation at `D848D`. The existing native countdown covers the other
16 addresses and retains its diagnostic forwarding. The completed binding
preserves the original eligibility, scale, secondary-response, and notification
calls. The comparison uses the selected observation ABI in the live World route.

The complete particle contact binding covers all 401 addresses in `2C2D90`.
The six stages preserve the collision query, contact transform, notification
variants, indirect methods, and loop return. GCC and Clang checked the exact
site partition, ordered memory access and fault prefixes. The source audit for
both owners is in `native-damage-particle-closure-object-motion-runtime/static-closure.json`
under `build/test-results/native-kernel-goal-20261124/`.

The first paired cohort activated particle contact flags and query, damage
due response, shield recovery and amounts, and the MOPP cache-key copy. It
retired 529,009 translated statements across Armory, Zanzibar and Coagulation.
All final frames and the Armory trace matched. The damage update returned
natively across all 21,233 entries in the three workloads, and Zanzibar particle
contact dropped from 252,500 to 26,888 translated statements. The MOPP key copy
removed 107,214 in Zanzibar and 16,176 in Coagulation. The physical tables
test also revealed the initial direct-page lookup needed to accept separately
mounted opcode and target table regions in the live provider. These results are
in `native-contact-response-dispatch-runtime/` and
`native-contact-response-dispatch-qualification/`.

The `B9460` object relocation worker preserves the ordered position and
orientation stores, bounds tests, four genuine local children and the required
`BF050` node rebuild. Its focused validation covered all 13 branches and
rejected 11 mutations.
The link regression exposed non-void replay helpers with qualified return types
being emitted as non-inline definitions in every translation partition. The
splitter and its multi-object linkage test now cover those helpers.

| Workload | Completed `2C2D90` | Native `B9460` | `30AA70` with sparse table lookup | Final batch retired |
| --- | ---: | ---: | ---: | ---: |
| Armory, 180 frames | 0 to 0 | 99,697 to 0 | 0 to 0 | 99,697 |
| Zanzibar, four local players, 360 ticks | 26,888 to 0 | 964,017 to 0 | 2,246,983 to 1,972,331 | 1,265,557 |
| Coagulation, four local players, 240 ticks | 17,561 to 0 | 386,964 to 0 | 377,528 to 324,310 | 457,743 |

All three final frames and the Armory trace matched. Across the final runs,
`D8330` completed 21,233 calls, `2C2D90` completed 4,544, and `B9460`
completed 15,785 with zero translated statements. Zanzibar verification also
replayed 239 calls of each of the final two particle-contact stages and every
`B9460` call. The final secondary damage response was covered by the pinned
reference fixtures. The linked comparison is under
`native-damage-particle-closure-object-motion-runtime-relink/`.

The isolated `30AA70` normal-cursor probe attributed the entire collision
delta to `30AAA0` and `30AAA7`, with 137,326 executions of each in Zanzibar
and 26,609 in Coagulation. The decreasing clips still retained their original
paths because their constants also occupy sparse read-only regions. The
Zanzibar profile measured 164,888 statements in the two clips and 798,974
in the two slab delta regions. Those results are under
`native-mopp-sparse-dispatch-pc-runtime-final/`.

Together the two batches removed 2,352,006 translated statements. The final
focused suite passed 49 tests with four skips for missing legacy fixtures.
The sparse lookup and partition regressions also passed. The owned scratch
directories were removed and both launcher selection checksums matched. The
combined counts and cleanup are in
`native-damage-particle-closure-object-motion-runtime-relink/combined-proof.json`.

The current Linux Release File API inventory resolves 177 production targets
and 209 instruction-source units, including 180 generated units. The bounded
receipt records every selected source/alias and hashes the complete graph,
which is larger than the retained-report limit. The full configured alias
inventory separately identified 9,589 source-qualified functions across 280
variants and reported no canonical source drift. These inventories establish
the configured scope for a broader runtime census. They are in
`native-engine-production-inventory-bounded-20261125/` and
`native-engine-accounting-inventory-20261125/`.

### Sparse MOPP collision paths and particle locations (2026-11-25)

The `30AA70` collision traversal now uses native decreasing clips, slab delta
and decreasing resume on the production sparse constant regions. The adapter
borrows each operand at its authored width and preserves the original fault
continuation when a region refuses a borrow. Both mirrored slab paths keep
the original x87 spill before the epsilon read. The scale-input preamble at
`30B36A` also runs natively, with the first two bytes and endpoint consumed
before the spill and the final byte fetched afterward so overlapping memory
preserves its original behavior. GCC and Clang passed 1,536 comparisons per
profile for the scale inputs. On the stock memory provider its median dropped
from 36.7 to 19.2 ns for resident memory and from 39.0 to 20.7 ns for sparse
regions.

| Workload | Previous normal path | Current normal path | Translated executions removed |
| --- | ---: | ---: | ---: |
| Zanzibar, four local players, 360 ticks | 1,972,331 | 638,087 | 1,334,244 |
| Coagulation, four local players, 240 ticks | 324,310 | 105,940 | 218,370 |

Both final frames matched the previous normal and paired reference frames.
Every literal execution in the four converted regions was eliminated in
both workloads. A separate rollback deliberately disables all of the old
decreasing-resume bindings as well, which accounts for the higher rollback
totals. The exact per-PC data is under
`build/test-results/native-kernel-goal-20261124/native-mopp-sparse-branches-pc-runtime-20261125/`.

The `17BB10` location update now has native coverage at all 167 addresses.
The location preparation at `178110` covers 93 addresses in each of its two
production copies and preserves each copy's direct or dispatched child
routes. The `2F88D0` collision pair covers 187 addresses while retaining the
direct `309150`, seven World dispatch boundaries, and the actual timestamp
service. Its replay lives in the shared translation partition prefix.
The `252480` velocity-property prefix and shared return cover 65 of 579
sites, with the nine original mode bodies and reciprocal-estimate service
still in place. The focused qualification and partition checks are in
`native-sparse-mopp-particle-location-collision-qualification/` and
`native-scale-velocity-and-partition-qualification-final/`.

The frozen Release engine linked and passed ten live cases. Armory matched
the reference frame and gameplay trace, and four-player Zanzibar and
Coagulation matched the reference frames. One-player Zanzibar and
four-player Burial Mounds extended default-path accounting. Across the
five default workloads, the three fully converted routines returned from
50,790 calls with zero translated instructions. That includes 137 calls
to the Effect Frame copy of `178110` in Burial Mounds. Zanzibar verify
compared the exact order of 582,930 accesses in 7,509 collision pairs.

The paired runs measured 3,174,630 fewer translated executions across
the particle locations, velocity-property common paths and collision
pairs. The two MOPP comparisons above bring the total to 4,727,244 for
this batch. The reports and authenticated residual queue are under
`build/test-results/native-kernel-goal-20261124/full-accounted-particle-collision-final-20261125/`.
The build and live scratch directories were removed after qualification.

### Particle velocity, collision cache, and contrail output (2026-11-25)

The `252480` velocity routine now binds all 579 original instruction
sites, including the selector, modes 0–8, and the alternate target. The
authored curve and random-service calls keep their original routes. The
World `309060` cache initializer and `30A1E0` quantized AABB initializer
retain their real children. The cache also has a distinct native Contact
implementation for both retained image layouts, including the original Map
Pack instruction service. Map Pack and direct Contact were qualified with
component fixtures. The runtime campaign and multiplayer cases use TU1.5.

The shared `30AA70` traversal now uses native return handling, collector
arguments and results, recursive arguments, and the common plane-offset
tail. Component qualification passed the 15 registered cases after
updating the World fixture to check the new independent Contact route.

The frozen engine passed 12 runtime cases covering Armory, one- and
four-player Zanzibar, four-player Coagulation, and four-player Burial
Mounds. All five default results matched the preceding engine and the
paired reference outputs. The particle velocity, World collision cache,
and quantizer recorded zero translated instructions across their 48,546
default calls. The MOPP stages and those three routines removed 4,612,845
translated executions compared with the previous normal run. The retained
accounting receipt is `full-accounted-velocity-cache-traversal-20261125/cohort.json`
under `build/test-results/native-kernel-goal-20261124/`.

The `42CE0` contrail draw owner now prepares and appends each vertex pair,
clamps its fade, and packs its color natively. The genuine `42C40` view
fade follows the original stack call boundary. The 229 pinned geometry
cases matched their GP, SSE, flags, and output memory in both runs, and
7,126 component comparisons per GCC and Clang build profile included
3,030 injected access faults. The runtime overlay used the actual draw
caller and the original/reference path in the same linked binary.

| Zanzibar workload | Reference `42CE0` | Native `42CE0` | Frame |
| --- | ---: | ---: | --- |
| Four local players, 360 ticks | 4,218,430 | 3,394,782 | `8c8d413929aed299` |
| One local player, 360 ticks | 1,037,017 | 835,269 | `ecfb30eef7d0f269` |

The exact fixture, component and live counts are in
`contrail-vertex-emission-{reference,boundaries,live}-20261125/` under
`build/test-results/native-kernel-goal-20261124/`.

### Armory script offsets, particle lens, and stencil light shape (2026-11-25)

The `2AA6D0` script expression pass now rebases the loaded string offsets
and walks the active datum table natively. It still calls the original
`16EA60` provider. GCC and Clang matched the pinned body in 8,043
comparisons per build and observer profile, including 7,275 injected
access faults. The live Armory run drops its 241,776 translated
instructions to zero on the one startup call.

The `174BF0` particle observer lens uses its native transform and keeps
the original x87 stack effects. Its pinned prepared FPTAN helper now
declares the AX early clobber before the original conditional store.
The pStencil `C3AB0` caller supplies a typed written-byte collector and
now admits the native light-shape implementation. The `2F8B60` convex
contact prefix also runs natively up to its real transform or cache
removal call, with the actual timestamp service preserved.

| Armory owner | Calls | Forced reference executions | Default translated executions |
| --- | ---: | ---: | ---: |
| `2AA6D0` script offsets | 1 | 241,776 | 0 |
| `174BF0` observer lens | 180 | 64,440 | 0 |
| `C3AB0` stencil light shape | 1,803 | 302,904 | 0 |
| `2F8B60` convex contact | 2,619 | 247,244 | 118,364 |

The previous normal `C3AB0` route executed 241,920 translated
instructions. The remaining 60,984 in its forced reference represent
the calls that already had a native route. The linked Armory cases
matched frame `bb29686dff025902` and trace `9891936e0c24`. The accounting
receipt is under `armory-native-lens-shape-contact-script-20261125/`.
The verify run matched all 180 observer lenses, 100,521 ordered accesses
in the convex-contact prefix, and 48,988 ordered accesses in the script
rebase. The normal and accounted builds both recovered all 2,026 Live
Support bodies across the 32 production partitions.

The Effect Frame `2C40D0` shape iterator also runs natively. It calls the
real `2C2AD0` motion owner and reads the live shape count again after
each child. The linked reference, default, and verify runs matched
full frame hashes on Zanzibar and Burial Mounds. Across both default runs
all 19,089 shape-iterator calls executed zero translated statements, down
from 577,056, and all 22,242 motion calls remained present. The exact
call and replay evidence is in `particle-physics-shapes-2c40d0/live/`.

The World contact-cache update at actual `30CD00` (canonical `309210`)
now uses its native implementation. Zanzibar four-player reference,
default, and verify matched the full frame hash and the previous
qualified engine. All 4,427 default cache calls recorded zero translated
instructions, down from 545,645. The original transform `2E4690` ran
15,575 times and the original reducer `30CAA0` ran 442 times in every
role. Verification checked 349,220 ordered accesses, including 98,547
stores. `build/test-results/native-30cd00-contact-cache-update-runtime.json`
contains the linked qualification and scratch asset relocation details.

World's position transform `2E4690` (canonical `2E0BA0`) now uses the
native geometry math with the production memory read and store order.
The World collision-cache image fixtures and all four composed CTests
passed. The paired Zanzibar four-player run matched its full reference
frame, with zero translated instructions across 15,575 transforms.
All 4,427 `30CD00` calls also ran natively in both roles. The native
transform retired 404,950 translated instruction executions. The runtime
receipt is `build/test-results/native-2e4690-world-position-transform-runtime.json`.

Six `30AA70` axis byte-node variants now use native arithmetic before and
between their original memory boundaries. Component qualification covered
24,576 hosted cases, 600 write-guard comparisons, and 144 unchanged
i386 cases. The one-player Zanzibar reference reproduced the frozen
3,972,910 translated executions in `30AA70`. The native run dropped to
3,962,293 with the same frame, 13,566 traversal calls, and identical counts
at every other instruction site. The surviving scalar stores identify
where to investigate provider admission for this workload. The per-PC
receipt is in `build/test-results/native-kernel-continuation-20261124/`
under `mopp-ray-zanzibar-1p-pc/zanzibar-1p-paired-current.json`.

The `30ADE0` plane comparison preserves the real provider write and
rechecks admission after its callbacks. It borrows both comparison
operands from the current frame, including sparse ordinary storage. The
three registered CTests, 576 hosted provider comparisons, and 96 additional
physical i386 cases passed. One-player Zanzibar kept all 13,566 traversal
calls and the same frame. The scoped `30AA70` count fell from 3,962,293 to
3,579,474 and every instruction outside the targeted range kept its count.
The exact paired receipt is `mopp-ray-zanzibar-1p-pc/zanzibar-1p-plane-provider.json`.

The script-offset admission now checks each resolved operand and the
required-owner policy even when an instruction callback starts empty.
The final GCC and Clang runs reject operand remapping and unqualified
providers. Linked Armory again matches the original frame and trace with
zero translated `2AA6D0` executions and 48,988 verified ordered accesses.

### Contrail geometry and MOPP provider continuations (2026-11-25)

The `42C40` view fade and its local `17F8A0` transition curve now enter
native math from the production contrail presenter. The original curve and
CRT remainder calls keep their stack boundaries. Both ports matched the
physical TU1.5 fixtures and the GCC and Clang ordered-access, fault, and
late-observer checks. The linked Zanzibar reference and native pairs kept
the full frame hashes and eliminated 693,508 view-fade and 674,536
transition-curve translated executions across one and four local players.
Both routines reached zero translated executions in their native roles.
The paired receipts are `contrail-view-fade-live-20261125/proof.json` and
`contrail-transition-curve-live-20261125/proof.json` under
`build/test-results/native-kernel-goal-20261124/`.

The `42CE0` point sampler now resolves the authored sample and prepares
its interpolated channels natively. Both paths preserve scalar SSE operand
order, including NaN payloads. The physical fixtures matched, and each GCC
and Clang release and sanitizer profile passed 4,096 state comparisons and
2,586 injected memory faults. The separate `17F3D0` point-update port keeps
all six genuine call edges. Each optimized compiler profile matched 1,309
cases with 1,048 injected stops and 431,380 ordered access, observer, and
child events. The two Zanzibar frame pairs preserved those outputs and the
measured effect-child counts. `42CE0` retained 2,746,242 four-player and
676,227 one-player translated executions after sampling retired 807,582.
All 1,823 point updates ran natively, retiring another 788,465 executions.
The paired receipt and the remaining counts by instruction address are
in `contrail-point-sample-and-update-live-20261125/proof.json` and
`contrail-residual-per-pc-20261125/proof.json` under the same results root.

The camera-facing orientation at `432BD–43541` now constructs the strip
directions, offsets, and optional face normal natively. Both near-zero
normalization paths passed the targeted GCC and Clang suite with full
register and floating-point packets at each access boundary. The physical
geometry fixture matched and retired 87,090 reference statements. The two
Zanzibar pairs removed 1,787,792 executions and left all counts outside
the converted branch unchanged. The `42CE0` residual fell to 1,311,912
four-player and 322,765 one-player executions with the same final frames.
The exact per-address receipt is `contrail-camera-facing-live-20261125/proof.json`
under the same results root.

Four `30AA70` provider continuations now run their word-order, slab, and
interval calculations natively after the original guarded writes. The
Zanzibar one-player comparison kept all 13,566 traversal calls, the full
frame hash, and the count of every instruction outside the four spans.
The translated total fell from 3,579,474 to 2,705,652. Both the Death and
AI production partitions compiled with their 896 diagnostic fallback
sites. The paired per-address receipt is
`build/test-results/native-kernel-continuation-20261125/mopp-provider-continuations-live/zanzibar-1p-mopp-provider-continuations.json`.

The `30B385–30B432` scale setup now combines native coordinate
translation, packed ray scaling, and scalar updates around the real
guarded stores. The next one-player Zanzibar pair reproduced the previous
reference count at every address and kept the full frame hash and all
13,566 traversal calls. All 41 exercised scale addresses reached zero,
retiring 864,690 translated executions and leaving 1,840,962 in the
traversal. All 15 focused Release, sanitizer, and separate rollback tests
passed. The paired receipt is
`build/test-results/native-kernel-continuation-20261125/mopp-scale-provider-live/zanzibar-1p-mopp-scale-provider.json`.

Three independently reversible MOPP continuations now handle the
axis-plane setup, short slab bounds, and endpoint spills around their
original provider writes. Sixteen focused checks passed, including the
prior endpoint-comparison rollback with native endpoint spills. The
paired one-player Zanzibar run preserved the full frame, 13,566 traversal
calls, and all untargeted counts. It eliminated 798,560 translated
executions from 22 addresses and left 1,042,402 in `30AA70`. The current
concentrated residuals are wide-plane setup `30AD52–30AD99` and geometry
preparation `30AE96–30AEC3`. The per-address receipt is
`build/test-results/native-kernel-continuation-20261125/mopp-slab-plane-provider-live/zanzibar-1p-mopp-slab-plane-provider.json`.

Vehicle Math `331ECB` (canonical `32E35B`) now classifies its two x87
operands natively and dispatches through the original indirect tail table.
The live World and Draw providers use the authenticated TU1.5 instruction
remap. GCC and Clang differential and sanitizer checks passed, along with
the managed vehicle CTest and the original XBE check of all 45 static
sites. The Armory and four-player Zanzibar pairs recorded zero translated
statements across 17,357 classifier calls and retired 694,280 executions.
Both full frames, the Armory campaign trace, and the sibling Math rows
matched their references. The evidence is in
`build/test-results/native-331ecb-vehicle-x87-dispatch-runtime.json`.

The weapon-effect copy of quaternion matrix `144910` now uses native
normalization, products, and matrix writes. It retains the original
memory and stack boundaries and provides an independent
`HALO2_REFERENCE_WEAPON_MARKER_QUATERNION` rollback. The direct weapon
dispatch and composed marker graph CTests passed. The authenticated i386
test covered all 77 original sites, both normalization branches, and 388
injected access stops. It passed under GCC, Clang, and ASan/UBSan. Both
Zanzibar pairs matched the full frames and neighboring marker accounting
rows. All 21,938 quaternion calls ran with zero translated statements,
retiring 1,667,288 executions. That comparison is in
`build/test-results/native-kernel-continuation-20261125/weapon-marker-quaternion-live/proof.json`.

The ten scopes removed 9,650,523 translated executions across these
paired runs. The batch receipt records the scope totals, remaining
`42CE0` counts, and verified scratch cleanup. Both launcher selections
still resolve to the original selected build and checksum.
`build/test-results/native-kernel-continuation-20261125/native-runtime-qualification-20261125.json`

### Contrail draw preparation and wheel offsets (2026-11-25)

The wheel-relative position stage at `20E414–20E479` (canonical
`209B14–209B79`) now uses native coordinate subtraction and writes. It
rejoins the original cross-product path at `20E481`. GCC, Clang, and
ASan/UBSan matched the register and access boundaries, including injected
provider faults and observer changes. The paired four-player Zanzibar
and Coagulation runs exercised 1,164 native wheel visits and removed
22,116 translated executions. Both full frames and the surrounding
vehicle accounting matched. The shared normal and accounted source
aliases have the same hashes as the tested CMake and runtime aliases.
The linked results are in
`build/test-results/native-20e414-vehicle-wheel-offset-runtime.json`.

The `42EAB–42FC3` adjacent sample now blends the authored segment and
handles optional width scaling natively. At `42FC4–4307F`, the object tint
uses native argument preparation and channel products around the genuine
`BCD80` call. The tint rechecks the draw provider on return. Both scopes
passed the physical geometry fixtures and GCC and Clang fault checks.
The paired one- and four-player Zanzibar runs removed 754,331 adjacent
sample and 569,667 tint executions. The original color child ran 14,175
times across the two workloads. All other per-address counts and both
frames matched the paired references. The receipt is
`build/test-results/native-kernel-goal-20261124/contrail-adjacent-sample-and-object-tint-live-20261125/proof.json`.

The `43080–430C1` orientation setup now scales the width, writes both
texture-coordinate pairs, and validates the authored mode natively. The
original `430C7` table read and dispatch remain at their original bound
site. Independent checks exercised all five modes, out-of-range modes,
altered table entries, and access faults. The paired Zanzibar runs removed
195,832 executions. The remaining `42CE0` totals were 92,756 four-player
and 22,091 one-player, including the original object color, view fade, and
orientation dispatch boundaries. Both full frames and all other counts
matched. The per-address receipt is
`build/test-results/native-kernel-goal-20261124/contrail-orientation-setup-live-20261125/proof.json`.

The `30AD52–30AD99` MOPP wide-plane setup and `30AE96–30AEC3` geometry
preparation now run natively around their guarded stores. The previous
one-player Zanzibar MOPP baseline was reproduced at every address. The
new native run kept the full frame hash and 13,566 traversal calls, and
eliminated 459,597 executions across 34 newly native sites. The remaining
`30AA70` count in this workload was 582,805. Its receipt is
`build/test-results/native-kernel-continuation-20261125/mopp-wide-geometry-provider-live/zanzibar-1p-mopp-wide-geometry-provider.json`.

These additional scopes removed 2,001,543 translated executions, bringing
the two checkpoints to 11,652,066 across their paired cases. The follow-up
receipt records the remaining contrail sites, launcher checksums, and
scratch cleanup.
`build/test-results/native-kernel-continuation-20261125/native-runtime-qualification-followup-20261125.json`

### Contrail draw coverage, vehicle cross product, and MOPP shortening (2026-11-25)

The `42CE0` draw frame now opens, handles empty chains, clears the head
and tail alpha where requested, and returns with the original `RET 12`
contract natively. The `BCD80` RGB source uses the existing model-light
channel arithmetic with a live frame adapter that preserves saved EBX,
interleaved RGB reads and writes, flags, and fault state. The original
i386 model-light test and all four ordered BCD80 compiler profiles passed.
The Zanzibar pairs removed 70,544 draw-frame and 340,200 object-color
executions. All 14,175 `BCD80` calls took the native path. Both full frames
and the counts at the two original calls and orientation dispatch matched.
The live receipt is
`build/test-results/native-kernel-closure-20261125/contrail-color-and-draw-frame-live/proof.json`.

Planar orientation modes 0, 1, and 2 now use native strip math through
their next-point, predecessor, near-zero, and disabled-normal paths.
Mode 3 keeps its original table-selected exit. The physical geometry
fixture and GCC and Clang access-order checks passed. Across all 229
physical geometry cases, `42CE0` produced only the genuine table read at
`430C7` and the call to view fade at `43573`. The associated object is
absent in those fixtures. The live Zanzibar pairs also exercised the
original `4303D` object-color call. The full fixture ledger is
`build/test-results/native-42ce0-draw-closure-original-per-pc.json`.

The cross-product stage `20E481–20E4EF` (canonical `209B81–209BEF`) now
runs natively after the native wheel offset. The two four-player
Zanzibar and Coagulation runs removed 26,772 translated executions across
1,164 visits. Both full frames and sibling vehicle accounting matched.
The original XBE, GCC and Clang checks, sanitizers, and both wheel CTests
passed. The receipt is
`build/test-results/native-20e481-vehicle-wheel-cross-product-runtime.json`.

The decreasing-ray shortening, recursive argument setup, and key advance
now use native continuations around their original stores and call edges.
One-player Zanzibar kept the full frame and 13,566 traversal calls and
removed 245,484 executions from 38 targeted addresses, leaving 337,321
in `30AA70`. All other address counts matched. Both production layouts
compiled and all 15 focused tests passed. The receipt is
`build/test-results/native-kernel-continuation-20261125/mopp-shortening-arguments-provider-live/zanzibar-1p-mopp-shortening-arguments-provider.json`.

Weapon marker emission `1E1200` now handles marker selection, writes,
transform preparation, and loop advance natively. It calls the original
quaternion and transform providers at their original stack boundaries and
rechecks tracing and FP state after both calls. The physical reference
fixture passed all 181 cases under GCC, Clang, and ASan/UBSan, including
late tracing after each child and 199 unavailable-owner stops per profile.
The vehicle bridge's scoped cursor checked all 106 canonical-to-TU1.5 PC
identities against the original image. All 20,038 marker emission calls
in paired one- and four-player Zanzibar took the zero-literal path and
removed 1,981,828 translated executions. The full frames and both math
children's accounting matched. The receipt is
`build/test-results/native-kernel-closure-20261125/weapon-marker-emission-vehicle-live/proof.json`.

The contrail `13A070` bitmap selector now resolves contiguous and sparse
sequences natively, preserving both signed divisions and `RET 4`.
Physical i386 checks covered all 46 original sites in 192 cases. GCC and
Clang matched the ordered fault prefixes under release and sanitizers.
The two Zanzibar pairs retired 73,328 executions across 4,583 calls.
Frames and all three original `42CE0` boundary counts matched.
`build/test-results/native-13a070-bitmap-selector-live.json` records the
exact counts.

The `30AA70` wide slab and collector argument setup now use native paths
around their genuine owner dispatches. The next one-player Zanzibar pair
removed another 108,649 executions across 38 targeted sites. Its 13,566
calls, full frame, and all other address counts matched the rollback.
The remaining translated count in this workload is 228,672. See
`build/test-results/native-kernel-continuation-20261125/mopp-collector-wide-slab-provider-live/zanzibar-1p-mopp-collector-wide-slab-provider.json`.

This batch removed 2,846,805 translated executions across the paired
runtime scopes. The batch receipt records the original contrail edges,
qualified runtime paths, launcher checksums, and scratch cleanup.
`build/test-results/native-kernel-closure-20261125/native-runtime-batch-20261125.json`

### Weapon marker matrices, contrail launch, wheel response, and MOPP traversal (2026-11-25)

Weapon marker matrix selection `BAD60` is native in the Effect dispatcher
and the vehicle composition. The port preserves the three original child
calls, default matrix stores, direction-sensitive copy, reflection, and
both `RET 12` paths. Across four-player Zanzibar and Coagulation, all
27,298 calls finished without translated statements, removing 1,869,780
executions. Both full frames, the marker parent, the emission helper, and
the math child counts matched. The 505 physical cases, 389 ordered fault
prefixes, and 290 reopenings of instruction traces passed under GCC,
Clang, and sanitizers. The runtime receipt is
`build/test-results/native-kernel-matrix-20261125/marker-matrix-vehicle-frozen-cohort/proof.json`.

Effect location iterator `17DCE0` now advances the cursor before resolving
the node, preserves the real recursive frame and return stack, and resumes
at the original instruction when a memory callback opens tracing. All
46,737 calls in the two four-player pairs returned with zero translated
statements, removing 1,155,329 executions. Both full frames and dispatcher
sibling counts matched. The 156 unchanged original cases, 196,608
node/mode comparisons, and the late trace regression passed in the GCC
release and Clang sanitizer builds. The receipt is
`build/test-results/native-effect-location-iterator-20261125/final-live/proof.json`.

The `219330` contrail launch path builds its frame and prepares both
authored lookup calls natively. It keeps the original `2193C0→219E40`
and `2193D6→219E00` calls. The one- and four-player Zanzibar pairs
removed 432,328 translated executions across 4,040 completed effects.
Both full frames, the child counts, and every address outside the port
matched. The 414-case physical frame fixture, optimized GCC and Clang
comparisons, and ASan/UBSan passed. The receipt is
`build/test-results/native-219330-effect-launch-live.json`.

The projection and shared status return now run natively around
the original `219884→11E600` call. The same Zanzibar pairs removed an
additional 107,544 executions. Three original boundary sites account
for the remaining 12,120 observations across 4,040 effects. The
allocation continuation remains available for its original route.
The receipt is `build/test-results/native-219330-effect-projection-live.json`.

The `219E00` tagged lookup now resolves the signed tag instance and
prepares the real `219E32→219E40` child and original `RET 8` natively.
It removed 62,336 executions across 4,040 calls with identical full
frames and surrounding owner counts. All 17 original sites were checked
in 112 physical i386 cases, including negative tag IDs and NONE.
The `219E40` owner remains a separate conversion target. The receipt is
`build/test-results/native-219e00-effect-tagged-lookup-live.json`.

The `20E4F4–20E6BE` wheel force stage now clamps and prepares the force
for the genuine `185670` owner. Zanzibar and Coagulation removed 110,305
translated executions across 1,164 native visits with identical frames
and sibling counts. The original image and GCC/Clang comparisons cover
all four bypass routes and the genuine child boundary. The receipt is
`build/test-results/native-20e4f4-vehicle-wheel-force-runtime.json`.

The `20E6C3–20E844` response now runs natively at all 1,164 stage visits
in the same two workloads. It removed another 72,790 translated
executions with matching full frames and sibling motion counts.
The original image, both compiler and sanitizer profiles, and all four
wheel checks passed. The accepted-wheel x87 route at `20E84A–20E8E0`
and the common output and torque loop at `20E91F–20EA26` still execute
translated statements. The receipt is
`build/test-results/native-20e6c3-vehicle-wheel-response-runtime.json`.

The `30AA70` MOPP axis-byte staging, increasing-ray recursion and resume,
and key-field copy are native in both Death and AI. One-player Zanzibar
removed 107,532 executions from 68 addresses with the same frame and
13,566 calls. All other address counts matched. Fifteen focused tests
and 1,440 directed axis comparisons passed. The receipt is
`build/test-results/native-kernel-continuation-20261125/mopp-byte-node-increasing-resume-provider-live/zanzibar-1p-mopp-byte-node-increasing-resume-provider.json`.

The two slab-ratio branches and cached offset-key node remove another
54,639 executions from 42 addresses in the same Zanzibar workload.
The frame, all 13,566 calls, and all other address counts matched. Both
production layouts compiled and ten focused checks passed. The `30AA70`
count is now 66,501, including two-byte split nodes, shortening, and
terminal returns. The receipt is
`build/test-results/native-kernel-continuation-20261125/mopp-slab-ratio-offset-key-live/zanzibar-1p-mopp-slab-ratio-offset-key.json`.

This batch removed 3,972,583 translated executions in paired Original-mode
multiplayer workloads. The batch receipt records the individual proofs,
the launcher binary and configuration checksums, and the test cleanup.
`build/test-results/native-kernel-matrix-20261125/native-runtime-batch-20261125.json`

### Effect event scheduling, contrail point weighting, and wheel torque (2026-11-25)

The `17C140` effect event advance now schedules intervals, updates elapsed
time, and handles the original return paths natively. All 18,115 calls
in the four-player Zanzibar and Coagulation pairs returned with zero
translated statements. The two runs removed 1,147,643 translated executions.
Both frames, all neighboring accounting rows, and 27,530 genuine calls at
the nine original child sites matched the rollback. The 142 original
fixture cases, 845 original child ABI comparisons, late trace handoff,
and rejected read-modify-write check passed in GCC release and Clang
sanitizer builds. An independent physical i386 check verified the flags
before and after the write fault at `17C284`. Runtime and original-byte
receipts are under `build/test-results/native-effect-event-advance-20261125/`.

The 180-frame Armory campaign pair also matched its full frame and capture
trace. All 1,626 event advance calls returned natively, removing 130,789
translated executions and preserving 3,258 original child calls.
The same proof directory contains `armory-live/proof.json`.

The `219E40` contrail admission and default vectors use native C++ on
negative, out-of-range, and disabled inputs. The one- and four-player
Zanzibar pairs removed 60,600 executions across all 4,040 calls,
with identical frames and neighboring owner counts. The original
i386 proof covered all 35 converted sites and 114 fault prefixes.
The live receipt is `build/test-results/native-219e40-effect-default-vector-live.json`.

The authored branch now resolves the authored record and blends its
vector natively around the original `219EC2→21A2A0→219EC7` boundary.
The original i386 comparison matched both blend branches across 64
cases and all 52 authored sites, including overlapping output and
exceptional coefficients. GCC and Clang matched the call packet and
final GP, EFLAGS, XMM, MXCSR, and x87 state. The two Zanzibar workloads
exercised the default route and kept zero translated `219E40` statements.
The active authored path was exercised by the original fixture at
`build/test-results/native-219e40-effect-authored-vector-original-physical.json`.

The `20E91F–20EA26` wheel output and torque stage now updates the two
output vectors and advances the wheel loop natively. Both four-player
workloads had exact frame matches, 1,164 native stage visits, 776 loop
edges, and 388 fallthroughs. The port removed 66,348 translated executions,
leaving 954,931 in the enclosing wheel owner across the two workloads.
All 57 original addresses mapped to the live stage, and all five wheel
checks passed. The receipt is
`build/test-results/native-20e91f-vehicle-wheel-output-runtime.json`.

The `20EA32–20EA62` scratch reset now clears the original byte locals
and 16 strided 16-bit headers, preserving both branches and the genuine
`20EA78→185070` call. All 1,020 visits ran natively, removing another
74,460 executions while both frames and all 474 genuine child calls
matched. Both original entrances were exercised, and all six focused
wheel checks passed. The remaining wheel owner count is 880,471 in these
two workloads. The receipt is
`build/test-results/native-20ea32-vehicle-scratch-reset-runtime.json`.

The `30AA70` MOPP split-byte nodes, terminal key gathering, increasing
shortened-end path, and decreasing clip stages removed the remaining
66,501 translated executions in one-player Zanzibar. All 13,566 calls
returned natively with an identical frame and matching outside-address
counts. Both production layouts passed 1,778 pinned comparisons each,
with normal and sanitizer checks of fault, trace and rollback behavior.
This closes the measured one-player TU1.5 workload. The receipt is
`build/test-results/native-kernel-continuation-20261125/mopp-final-routes-live/zanzibar-1p-mopp-final-routes.json`.

The campaign and multiplayer pairs removed 1,546,341 translated
executions in this batch. The batch receipt tracks the exact scopes,
launcher checksums, and test cleanup.
`build/test-results/native-effect-event-advance-20261125/native-runtime-batch-20261125.json`

### Effect matrices, event selection, and authored contrail offsets (2026-11-25)

The `17DD50` and `17DDC0` effect location owners now select the
authored matrix and copy its thirteen words natively, retaining the
three original call sites. Each transfer preserves the source read,
destination write, direction flag, and cursor state. A completed write can
reopen tracing at the next repeated instruction without repeating any
committed word. The 138 original cases include forward and backward
aliases, 24 read stops, 22 write stops, and 74 real or composed call
frontiers. GCC and Clang sanitizer checks passed. Across Armory, Zanzibar,
and Coagulation, all 43,673 calls returned without executing a translated
body, removing 1,122,228 executions. Frame and Armory trace hashes matched.
The receipt is `build/test-results/native-effect-location-copy-20261125/live/proof.json`.

The `179ED0` effect systems owner now traverses the authored
location chain, prepares the system callbacks, and handles empty and
completed returns natively. The eight direct and two indirect calls
keep their original argument order and return addresses. The 193 original
fixture cases exercise 1,933 child boundaries, 21 memory faults, and
a trace that reopens after a virtual call. Both compiler profiles passed.
All 24,440 runtime calls across Armory, Zanzibar, and Coagulation returned
without translated statements, removing 1,076,905 executions. The 65,633
original child calls, full frames, and Armory trace matched the rollback.
The receipt is `build/test-results/native-effect-systems-20261125/final-live/proof.json`.

The `177830` particle frame now scans the bitmap of occupied slots,
updates or retires each live row, and restores the frame natively.
It preserves three original child call sites, the saved-pool reload, and
`RET 4`. The 118 original cases cover 13 memory faults and two bounded
trace prefixes. GCC and Clang sanitizer checks passed the saved-pointer
and child-return trace reopenings. All 778 measured frames matched the
rollback across Armory, Zanzibar, and Coagulation, retiring 1,043,311
translated executions. The 26,609 original child calls, full frames,
and Armory capture trace matched. The receipt is
`build/test-results/native-177830-particle-frame-runtime/proof.json`.

The `17C020` event selector now uses the authored effect loop, RNG
bank, and event test natively. It preserves alias-sensitive stores and
x87 state. The unchanged original image reproduced the 185-case
fixture byte-for-byte. GCC and Clang sanitizer checks passed 1,781 cases
each of reopening a trace and rejecting an access. The Zanzibar and
Coagulation pairs removed 169,960 translated executions across 3,146
calls, with the same frames and neighboring owner counts. The receipt is
`build/test-results/native-17c020-effect-next-event-runtime/proof.json`.

Contrail authored offset sampler `21A2A0` is native around the
original `219EC2→21A2A0→219EC7` boundary. All 64 composed original i386
cases matched the child entry, all ordered stores, and final register
state, covering all 70 child sites. The controlled compiled replay
removed all 8,320 translated child executions. Zanzibar replayed
the default branch and matched the frame hashes. The original proof is
`build/test-results/native-21a2a0-effect-authored-offset-original-physical.json`.

The four-player `30AA70` traversal pairs exercised the uncached
16-bit offset-key copy in Zanzibar and the diagonal byte-split path in
Coagulation. The native paths removed 1,155 and 342 executions, leaving
zero translated traversal statements across 16,180 measured calls.
All other address counts and both frame hashes matched. Both generated
layouts passed the pinned operand, rollback, fault, and sanitizer checks.
The receipt is
`build/test-results/native-kernel-continuation-20261125/mopp-multiplayer-routes-live/four-player-offset-diagonal-pair.json`.

The surrounding `30B890` caller now prepares the ray and restores the
original return frame natively around the genuine `30AA70` call.
Both multiplayer pairs reached zero translated setup statements across
1,747 calls, removing 111,808 executions. Both frames, all child counts,
and every normalized surrounding count matched. Repeated rollback runs
separately identified host cache polling as the source of the earlier
diagnostic count variance. All three layouts passed 1,730 pinned cases
under release and sanitizers, including dual-NaN operand ordering.
The receipt is
`build/test-results/native-kernel-continuation-20261125/mopp-setup-30b890-live/four-player-setup-30b890-qualified.json`.

The `20E84A` wheel curve preparation and response accumulation now
run natively around the original `20E88D→17F790→20E892` boundary.
The paired runtime kept all 1,079 original child calls, took the native
preparation and response on every call, and removed 45,403 translated
executions. Both frames and other motion owner counts matched. Physical
i386 checks matched the successful x87 store's C1 status and both signed
divide faults. A focused rejected-store check verified that the original
pending C1 and TOP survive a rejected write. All seven wheel checks
passed. The wheel owner still recorded 835,068 translated executions
across the two workloads. The receipt is
`build/test-results/native-20e84a-vehicle-wheel-curve-combined-alias.json`.

The verified campaign and multiplayer scopes removed 3,571,112 translated
executions in this batch. The authored contrail sampling result was
measured independently against the physical fixture. The batch records
the scope, launcher checksums, and managed scratch cleanup.
`build/test-results/native-effect-location-copy-20261125/native-runtime-batch-20261125.json`

### Effect frames, geometry cache, rotation, and contact stages (2026-11-25)

- `179DF0` effect-frame processing reached zero translated instructions
  over 778 calls, removing 890,761 executions. The three paired frames,
  exact Armory trace, and all 25,996 original child calls and tails matched.
  The native `16EA60` slot scan now preserves partial register state on
  faults and resumes at the next original PC when tracing activates.
  All 327 original effect-frame cases passed with the native child active,
  including the nested faults. The separate slot scan passed 1,190
  comparisons, 42 fault prefixes, and 84 trace activations in both release
  and sanitizer runs.
- The shadow-frame copy of `130D20` now uses the shared geometry cache
  kernel and the original `130D92→22A790` dispatch. It reached zero
  translated executions over 42,622 calls, removing 1,203,724 and preserving
  all 12,694 child calls. The fifth source copy's relocation guard rejects a
  changed target, and the differential suite compares ABI state on faults.
- `145B70` three-component rotation reached zero translated executions
  over 21,292 calls, removing 771,560. The 264 calls from instruction
  observers use the same native rotation with the original callback order.
  The pinned TU1.5 image confirms all 41 callback sites. Differential cases
  cover exact and partial aliasing, live MMX, NaN priority and rejected
  instruction or memory accesses. Both rendering owners matched frames in
  all three workloads and the exact Armory capture trace.
- Four `30CAA0` Havok contact-selection scans removed 184,720 in four-player
  Zanzibar. Exact counts outside the four ranges matched, with 46,872
  translated executions remaining in the owner. Coagulation exercised zero
  calls and matched its paired frame. The next removal and record-copy loop
  at `30CCA0–30CCF8` accounts for 26,982 of the remainder.
- The `20F6E0–20F8F2` vehicle force and torque stage removed 135,480
  across 1,365 admissions, with matching frames, original adjacent calls,
  and sibling Motion counts. Original i386 matched 288 profiles per
  compiler. The wheel owner still counted 699,588 translated executions in
  those runs. The contact projection region at `20EE92–20F095` accounts
  for 132,330 measured executions across the same workloads.

The paired runs retired 3,186,245 translated executions in these scopes.
The receipt records the independently reproduced native host-poll variance,
launcher fingerprints, and managed scratch cleanup.
`build/test-results/native-render-kernels-20261125/native-runtime-batch-20261125.json`

### First-person models, particle transforms, and contact projection (2026-11-25)

- `168DED` first-person model preparation removed 1,740,960 translated
  executions across 6,696 calls in Zanzibar, Coagulation, and Burial
  Mounds. All paired frames and neighboring counts matched. Coagulation
  live verification replayed 304,008 ordered accesses over 1,912 calls.
  Three fresh Lockout captures matched 19,395 original i386 instruction
  observations, register and floating-point state, ordered writes, and
  every source-bank byte. The original image authenticates 242 static
  instruction sites, nine child sites, and eight stack read-modify-write
  sites. The focused CTest exercises rejected stores, trace activation
  during a real child boundary, all seven covered outcomes, and unchanged
  model-record behavior.
- `30CAA0` contact reduction went from 46,872 remaining statements to
  19,890 after the removal loop and zero after frame setup and transitions
  in four-player Zanzibar. All three roles returned the same 442 calls,
  matched the frame, and kept the original indirect call at `30CCB5`.
  Both pinned layouts passed removal, frame, and prior-scan tests in
  release and ASan/UBSan. The generated aliases retain fallback sites
  outside this workload's eligibility. The next measured Havok leaf,
  `2E8D70` sphere center, accounts for 78,993 executions across the
  retained four-player Zanzibar and Coagulation runs.
- `20EE92–20F095` contact projection removed 132,330 executions across
  1,203 visits with zero fallback resumes and exact four-player frames.
  Both compilers matched the original i386 non-popping `FST` and complete
  stage results, including all three exits. The wheel owner retains
  567,258 translated executions across the two workloads.
- `250AC0` particle basis and projection together removed 830,758 across
  8,837 visits in the same three maps. Each paired frame and neighboring
  count matched. Coagulation and Burial Mounds reached zero. Zanzibar
  retained 32 translated statements at the two genuine matrix call sites,
  `250C0C→1458A0` and `250C17→1459B0`. Both compiler profiles passed
  388 original-image cases spanning ordered faults and trace resumes.

The paired runs retired 2,750,920 translated executions in these scopes.
The receipt records the original references, launcher fingerprints, and
managed scratch cleanup.
`build/test-results/native-168ded-model-preparation/native-runtime-batch-20261125.json`

### Two-bone solve, sphere leaves, and particle and vehicle dispatch (2026-11-25)

- `122BE0` now computes the two-bone pose natively from squared-length
  admission through the two basis updates and final matrix publication.
  It preserves all five original `30BD0` edges. Independent unchanged
  TU1.5 i386 checks covered 324 cases per GCC and Clang profile, including
  228 fault boundaries, 570 normalization entry/return pairs, all 317
  parent sites, and 2,604 x87 snapshots. Stack and x87 stores commit the
  original state before delivering an after-store observer. The same
  rule applies to each successful `REP MOVSD` transfer. The configured
  solver and `1E1390` node-chain CTests passed. Paired four-player
  Zanzibar, Coagulation, and Burial Mounds replays completed 5,371
  solves and retired 1,681,123 translated executions. The captured
  frames matched byte for byte, and the normalization counts and
  neighboring execution counters matched in each pair.
- Havok sphere center `2E8D70` and bounds `2E8D90` retired 116,953
  translated executions across 10,675 calls in Zanzibar and Coagulation.
  Both paired frames and the returned-call counts matched. The proof
  authenticates the Death-cycle and applicable Live Support bodies
  separately from the unrelated World bitset PC.
- The `22AD60` emitter bound retired 897,024 over 42,797 calls in five
  captures. The last two `250AC0` call sites retired the remaining
  32 executions in Zanzibar while retaining the original `1458A0`
  and `1459B0` callees and their returns. Frame hashes, the Armory
  gameplay trace, and unrelated accounted rows matched.
- The `20F3A8–20F62F` vehicle direction and query-record stage removed
  150,866 across 1,203 native admissions with matching frames and
  surrounding call counts. Both compilers matched 288 unchanged i386
  cases covering all three entries and both exits. The paired workloads
  still record 416,392 translated executions in `209A50`.

The batch retired 2,845,998 measured translated executions across the
paired workloads. The [batch receipt](../../build/test-results/native-122be0-two-bone-solve/native-runtime-batch-20261125.json)
records the per-scope counts, proof hashes, and launcher selection hashes.

### Polygon intersection, vehicle contact collection, and particle owners (2026-11-25)

- The Live Support `1F6220` polygon intersection now runs natively from
  cross product and plane intersection through vertex projection and
  result publication. It keeps the original dominant-axis `123210`
  and containment `243510` calls and the `RET 0xc` ABI. GCC and Clang
  each matched all 253 parent sites in 741 unchanged i386 cases, with
  597 fault boundaries, 517 original child call snapshots and 8,587
  x87 snapshots per profile. Four-player Zanzibar, Coagulation, and
  Burial Mounds replays retired 1,081,539 translated executions
  across 5,691 native returns. Frame hashes and all 36, 30, and 29
  accounted neighboring rows matched, respectively.
- Native stages at `20EA64–20ECF1` prepare and collect the vehicle
  contacts, including repeated-wheel entry at `20EA90`. The two
  original surface and contact calls keep their source boundaries.
  The paired Zanzibar and Coagulation workloads retired 166,893
  translated executions and record 249,499 remaining in `209A50`.
- The `251F70` particle location transforms retired 285,245 translated
  executions over 6,943 calls. The `251650` cleanup retired 219,428 over
  10,340 calls and preserved 143 original attachment-removal chains.
  Both owners reached zero translated execution in four multiplayer
  pairs with matching frames, child counts, and neighboring counters.

Together these paired workloads retired 1,753,105 translated
executions. The [batch receipt](../../build/test-results/native-1f6220-polygon-intersection/native-runtime-batch-20261125.json)
records the exact source and proof hashes and the verified launcher selection.

### Weapon animation, bitmap residency, Havok planes, and effect origin (2026-11-25)

- `1E3420` builds the authored animation query, handles fallback
  symbols, walks all twenty groups of three nested lookups, and
  enumerates graph overrides natively. The four original child owners
  retain their 83 call sites. The unchanged i386 image authenticates
  all 1,007 accounted sites and the three `RET 0x20` exits. The
  differential exercises the complete observed path and quiet cursor
  against the pinned TU1.5 parent. Armory and four multiplayer cohorts
  retired 1,524,797 translated executions over 7,791 native returns.
  Paired frame hashes, Armory gameplay traces, and neighboring
  execution rows matched. Unchanged i386 entry-to-first-call and
  absent-id checks matched 170 cases per compiler, including 122
  memory fault boundaries.
- `12F910` handles bitmap residency natively and preserves all four
  resource child routes. Four paired multiplayer replays retired
  1,648,470 translated executions over 8,768 returned calls. All
  frame hashes and neighboring rows matched. Physical i386 checks
  exercised all 223 parent sites and all five `RET 8` paths,
  with 179 genuine child entry and return comparisons. The native
  and rollback paths also matched i386 before and after all 13
  PUSH and CALL stores.
- `314A30` builds the Havok plane natively and invokes the original
  reciprocal estimate at `314A9A`. Five paired replays retired 927,036
  translated executions over 14,046 returns with matching estimate
  inputs and results. Unchanged TU1.5 i386 parity covered 251 cases
  including 155 fault boundaries. Provider tests covered another
  1,792 cases, including late trace activation and partial writes.
- `BBE00` computes both unattached effect origins and attached
  parent-matrix transforms natively. Five paired replays retired
  892,878 translated executions across 43,691 calls, including 2,661
  attached origins. Frames, the Armory gameplay trace, and neighboring
  counters matched. All five compiled dispatch routes passed the
  111-case original i386 corpus.
- World pool pressure `2335B0`, canonical `22ECB0`, sums the
  sixteen source slots natively. All 7,726 calls in five paired
  replays used the bulk reduction and retired 934,846 translated
  executions. Frame hashes, replay traces, and neighboring counters
  matched. The ordered provider path and rollback each matched
  345 unchanged i386 cases, including 265 fault boundaries.

Together these paired workloads retired 5,928,027 translated
executions. The [batch receipt](../../build/test-results/native-1e3420-weapon-animation-selection/native-runtime-batch-20261125.json)
records the measured scopes, proof hashes, and launcher selection.

### Animation requests, lens queries, and pose preparation (2026-11-25)

- The `113D30` animation request owner now runs natively from setup
  through activity selection, pose scheduling and `RET 8`. The original
  image authenticates all 703 sites and 47 genuine call boundaries.
  Pinned differential checks reached every site, including fault and
  observer cases. GCC and Clang also matched 196 unchanged i386 entry
  cases each through the first child boundary, including 148 memory
  faults. All 7,222 calls in the five paired workloads returned with
  zero translated owner instructions, retiring 1,359,151. Frames,
  replay inputs, the Armory trace and neighboring counters matched.
- The `20B90` lens-query owner runs natively through query issue and
  reuse while preserving its six genuine child call sites. Paired
  replays retired 867,846 translated executions over 8,974 calls
  with matching frames, replay traces and child counts. Configured
  tests cover both observation modes and the original physical checks
  include three genuine external call boundaries.
- The `C85A0` setup, second-axis angle and output stages now run
  natively at `C85A0–C86FF`, `C8930–C89DE`, and `C8B65–C8C19`.
  All three rollbacks enabled on the same binary counted 1,181,980
  translated executions across five replays. Default dispatch counted
  186,991, retiring 994,989 with matching frames, replay inputs and
  child counters. The remaining work lies in `C8704–C8823`,
  `C8827–C892A` and `C89E3–C8B60`.
- The `E4A40` physical-result activity owner now has 213 native sites
  in its opening, motion projection and scalar update. The original
  calls and joins at `E4B81`, `E4D6D`, `E5094`, `E51CD`,
  `E51D6` and `E52BD` remain intact. Five paired workloads retired
  791,162 translated executions over 4,250 calls, leaving 422,670.
  Frames, replay streams and neighboring counters matched.

The four scopes retired 4,013,148 measured translated executions.
The [batch receipt](../../build/test-results/native-113d30-animation-request/native-runtime-batch-20261125.json)
records source hashes, paired and physical proofs, and launcher selection.

### Death-cycle opening, pose normalization, contact transition, and multiplayer finish (2026-11-25)

- The `1030F0–1031C7` death-cycle opening resolves object and tag
  datums and prepares the scale and override queries. It keeps three
  genuine child calls and rejoins at `1031CB`. GCC and Clang matched
  all 60 sites on unchanged i386 over 48 clean and 113 fault cases.
  Four multiplayer replay pairs retired 190,004 executions across
  3,544 entries, leaving 1,076,740. Armory matched as a control with
  zero entries. All frames, replay inputs, and retained owner PCs
  matched. The Zanzibar pair's existing native resource-poll counter
  had a nine-call timing difference and zero translated statements.

- The `C8827–C892A` native pose normalization and clamps preserve
  four genuine `11F7E0` and `122B60` call sites. Five same-binary
  replay pairs retired 120,835 translated executions, leaving 66,156
  in `C85A0`. The remaining ranges are `C8704–C8823` and
  `C89E3–C8B60`. Unchanged i386 and pinned provider tests covered
  the original calls, normalized operands, flags, and fault edges.
- The `E4DC0–E4F8A` contact transition adds 123 native sites to
  `E4A40`. It keeps five original child call sites. Five replay
  pairs retired 261,776 translated executions, leaving 160,894.
  Physical i386 checked all 123 sites in 150 cases, including 74
  fault boundaries and all five child call and return edges.
- The multiplayer `1F0670` frame and table stage prepares the common
  frame, reads the authored slot table, and handles shared exits.
  Across Armory and four-player Zanzibar and Coagulation, 16,771
  entries retired 668,460 translated executions. The wrapped angle
  delta slot and its result exits retired another 76,131 across the
  same workloads. The remaining slot paths account for 112,595,
  including 71,186 in slot 23 at `1F0CB5–1F0D4C`.

The five scopes retired 1,317,206 measured translated executions.
The [batch receipt](../../build/test-results/native-1030f0-death-cycle-prologue/native-runtime-batch-20261125.json)
records the same-binary pairs, source hashes, physical checks and
launcher selection.

### Pose assembly closure (2026-11-25)

The `C8704–C8823` and `C89E3–C8B60` pose stages now run natively.
They retain the original `BBFF0`, `C9D20`, and `121A90` calls.
The two stages retired the remaining 66,156 measured instructions
in `C85A0`. All five same-binary pairs matched frames, replay data,
and child counters. All 7,222 calls bypassed the translated parent,
including both original return paths. The registered release
partition compiled and the full engine build is a separate check.
[Closure evidence](../../build/test-results/native-c85a0-pose-axes/closure.json)

### Death-cycle activity scan and motion dispatch (2026-11-25)

The `103657–1037E0` activity and motion stage runs 108 native sites
through the original `102D50` call and both authored branch tables.
Five paired workloads retired 326,803 translated executions and
leave 749,937 in `1030F0` at this checkpoint. Four multiplayer maps
exercised 3,544 owner entries and 3,451 genuine child calls. Armory
matched as a control with zero owner entries. All frame hashes,
replay traces and retained owner PCs matched. Two Zanzibar pairs
showed timing differences in the existing native resource poll,
with zero translated executions in that poll.
[Qualification evidence](../../build/test-results/native-103657-death-cycle-activity/qualification.json)

The registered descriptor transition at `1031CB–1032A3`, timer
expiration at `1040F0–1041FD`, motion result at `1037E7–103877`,
and timer tail at `1044F4–104646` cover another 255 sites.
They passed focused original and provider checks. The object timer at
`1032A7–103432` and record routing at `104204–10436C` add 216 sites
and passed original i386 comparisons, including fault and child call
boundaries. Across five paired workloads, the six stages retired
658,073 translated executions, leaving 91,864 in the sampled
workloads. The frames, replay streams, child counters and retained
instruction counts matched. The comparison uses the accepted
activity-conversion replay as its baseline.
[Six-stage replay evidence](../../build/test-results/native-1040f0-death-cycle-expiration/paired-six-runtime-verified.json)
[Object timer proof](../../build/test-results/native-1032a7-death-cycle-object-timer/physical-i386-parity.json)
[Record routing proof](../../build/test-results/native-104204-death-cycle-record-route/physical-i386-parity.json)

The timer notifications at `103436–103652` and record blend at
`104371–1044F0` add another 239 sites. Both are registered and passed
unchanged i386, pinned rollback and sanitizer checks. The combined
eight-stage alias passed preparation. Its production replay is queued.
[Timer notification proof](../../build/test-results/native-103436-death-cycle-timer-notification/physical-i386-parity.json)
[Record blend proof](../../build/test-results/native-104371-death-cycle-record-blend/physical-i386-parity.json)

The native motion finalization at `103CBD–103D67` adds 44 sites with
five original call sites. Unchanged i386 and pinned rollback matched
its calls, joins, memory faults and provider unwinds. This range
accounts for 1,852 translations in the remaining sample.
[Motion finalization proof](../../build/test-results/native-103cbd-death-cycle-motion-finalize/physical-i386-parity.json)

The record effects at `103A6D–103CBC`, cleanup at `103D68–103ED3`,
and motion links at `103ED4–1040EF` add 389 native sites. Original
i386 and pinned rollback checks covered every site, including the
original providers, authored table branches, and faults. Record
effects contributed 532 sampled executions. The two later regions
had zero hits in the sampled workloads.
[Record effects proof](../../build/test-results/native-103a6d-death-cycle-record-effects/physical-i386-parity.json)
[Cleanup proof](../../build/test-results/native-103d68-death-cycle-motion-cleanup/physical-i386-parity.json)
[Motion links proof](../../build/test-results/native-103ed4-death-cycle-motion-link/physical-i386-parity.json)

### Physical-result activity closure (2026-11-25)

The final `E4A40` classification, notification, impulse publication,
and decay stages retired 160,894 translated executions. All 4,250
calls across five same-binary replays returned with zero translated
owner executions. Frame, trace, replay and child counters matched.
[Qualification evidence](../../build/test-results/native-e4a40-final-branches/qualification.json)

### Multiplayer finish slot closure (2026-11-25)

The weapon and hot-slot batch retired the remaining 112,595 sampled
instructions in `1F0670`. All 16,771 calls in the three paired
workloads returned with zero translated owner executions. Frames,
the Armory trace, and genuine child-call counts matched. The active
binding now covers all 586 original sites. Unchanged i386 exercised
all 382 sites outside the paired workload coverage in 127 cases with
32 faults. The same 127 quiet cases executed via the current
specialized owner with zero translated statements. The whole-engine
replay with the final slot binding is queued.
[Active cold-slot proof](../../build/test-results/native-player-finished-1f0670/cold-slots-active-physical-proof.json)

The two measured batches retired 439,398 translated instruction
executions. [Batch receipt](../../build/test-results/native-103657-death-cycle-activity/native-runtime-batch-20261125.json)

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
groups that differ in C1). B9460 needs binder support for `required(...)` children before
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

- [x] **`30E930` production binding** — `physics_world_runtime_generated.cpp`
  - The current specializer installs the native simplex owner at canonical `30AE40` for TU1.5 `30E930`. The earlier Armory callback census measured 4,750 observations per frame. Current normal-cursor activation frequency needs a fresh measurement.
- [ ] **`125280`** (`3,489/frame`) — `physics_game_arena_runtime_generated.cpp`
  - **Notes**: Load-time / initial-tick arena table initializer (~1M instructions amortized over 300 frames). Pure integer/table loop.
- [ ] **`1D9EC0`** (`1,645/frame`) — `physics_phantom_world_tu15_generated.cpp`
  - **Notes**: Havok phantom AABB / broadphase overlap update.
- [ ] **`2E2E30`** (`1,319/frame`) — `physics_world_runtime_generated.cpp`, `source_scene_device_constructor_tu15_generated.cpp`
  - **Notes**: Multi-unit Havok transform/constraint helper; bind across both owning units.
- [x] **`1F6610` production binding** — `physics_world_runtime_generated.cpp`
  - The World route selects canonical owner `1F1D20`, whose native polygon support bridge covers the complete body. The earlier Zanzibar census forced full callbacks without the normal instruction cursor. Its 2,561,415 observations measure that diagnostic path. Current normal-cursor activation frequency needs a fresh measurement.
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
- [x] **Selected TU1.5 `D8330`** — `source_damage_delivery_runtime_generated.cpp`
  - All 545 selected addresses are covered, with zero translated statements across 21,233 measured calls.
- [ ] **`1EFD60`** (`1,314/frame`) — `source_multiplayer_engines_tu15_generated.cpp`
  - **Notes**: Direct child of already-native `1EEE80` (`native_tu15_engine_player_update.hpp`). Has `0` x87 instructions! Porting `1EFD60` and `1F0670` completes the `1EEE80` multiplayer engine player update closure.
- [ ] **`14D410`** (`664/frame`) — `physics_live_support_tu15_generated.cpp`, `source_combat_action_tu15_generated.cpp`, `source_model_render_tu15_generated.cpp`
  - **Notes**: Multi-unit leaf/helper across `live_support`, `combat_action`, and `model_render`.
- [ ] **`2B9F0`** (`640/frame`) — `source_texture_camera_producer_tu15_generated.cpp`
  - **Notes**: Texture camera producer frame update.
- [x] **`1F0670`** — all 586 original sites have native dispatch. Three paired workloads counted zero translated executions, and active-alias tests covered the unsampled slots.
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
- [x] **`E4A40`** — zero translated owner executions over 4,250 calls across five paired workloads after the final native stages.
- [ ] **`17E250`** — both observer-retirement copies are native and passed original i386 and active binding checks. The paired production replay will check the baseline of 1,142,814 translated executions across four multiplayer workloads.
- [ ] **`303800`** (`635/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`DEAE0`** (`628/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`1F8B30`** (`622/frame`) — `physics_live_support_tu15_generated.cpp`
- [x] **`C85A0`** — zero translated owner executions across five paired workloads after the final pose axis stages.
- [x] **`113D30`** — zero translated owner executions in all five paired workloads.
- [ ] **`30A020`** (`569/frame`) — `physics_live_support_tu15_generated.cpp`
- [x] **`314A30`** — zero translated owner executions in all five paired workloads.
- [ ] **`10E1F0`** (`536/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`1EDE60`** (`524/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`DF880`** (`507/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`D05A0`** (`475/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`E9530`** (`457/frame`) — `physics_live_support_tu15_generated.cpp`
- [x] **TU1.5 `B9460`** — `physics_live_support_tu15_generated.cpp`
  - Native object relocation completed 15,785 measured calls with zero translated statements.
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
