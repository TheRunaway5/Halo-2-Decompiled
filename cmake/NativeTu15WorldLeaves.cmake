# Native TU1.5 physics world leaves (group H: 1F5FE0 surface polygon, 2E4550
# relative transform, 303BA0 contact record copy, 2E47B0 transposed rotation,
# the 303EE0 contact record append, the 303C70/303D00 local shape buffers,
# the 304350 welded collection agent, and its children the 30CF40 manifold
# report and 304130 contact weld, 2F36F0's child the 2F33B0 key sort, the
# 1F76A0 shape key filter, the 2F8630 closest point and 2E4FE0 swept pair,
# the 1F7C10 BSP child shape, the 1F6D10 structure AABB query, the 2F36F0
# child agent update and the E8D80 object flag update):
# seeded comparison of the logic and its boundary adapters against the pinned
# translated bodies, census and verify (record/replay) modes, branch coverage,
# sanitizer replay, production binding checks and the mutation screen.
if(BUILD_TESTING)
    add_test(NAME native_tu15_world_leaves
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_world_leaves_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-world-leaves.json" --jobs 2)
    set_tests_properties(native_tu15_world_leaves PROPERTIES TIMEOUT 2400)
    # 303EE0 contact record append (child boundary: 2E3250; 303BA0 inline).
    add_test(NAME native_tu15_contact_array
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_contact_array_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-contact-array.json" --jobs 2)
    set_tests_properties(native_tu15_contact_array PROPERTIES TIMEOUT 2400)
    # 303C70/303D00 local shape buffers (dispatch children: the thread memory).
    add_test(NAME native_tu15_local_buffers
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_local_buffers_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-local-buffers.json" --jobs 2)
    set_tests_properties(native_tu15_local_buffers PROPERTIES TIMEOUT 2400)
    # 304350 welded collection agent (children 2F36F0, 304130, 30CD00, 30CF40,
    # seven dispatch sites; 303C70, 303D00, 303EE0, 2E47B0 and 30BAD0 inline).
    add_test(NAME native_tu15_collection_agent
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_collection_agent_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-collection-agent.json" --cases 8000 --jobs 2)
    set_tests_properties(native_tu15_collection_agent PROPERTIES TIMEOUT 3600)
    # 30CF40 manifold report (children 2E31E0 and the contact manager's
    # removal) and 304130 contact weld (child 303800 through the dispatch).
    add_test(NAME native_tu15_manifold
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_manifold_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-manifold.json" --cases 10000 --jobs 2)
    set_tests_properties(native_tu15_manifold PROPERTIES TIMEOUT 3600)
    # 2F33B0 key sort (its recursion inline).
    add_test(NAME native_tu15_key_sort
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_key_sort_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-key-sort.json" --cases 10000 --jobs 2)
    set_tests_properties(native_tu15_key_sort PROPERTIES TIMEOUT 2400)
    # 1F76A0 shape key filter (its jump table; child 185AA0 through the dispatch).
    add_test(NAME native_tu15_shape_key_filter
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_shape_key_filter_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-shape-key-filter.json" --cases 10000 --jobs 2)
    set_tests_properties(native_tu15_shape_key_filter PROPERTIES TIMEOUT 2400)
    # 2F8630 closest point (children 30E930 and 30D350; 2E4550 and 2E4760
    # inline) and 2E4FE0 swept pair (children 283380, 309060, 2F8630 and the
    # dispatch; 2E4550 inline).
    add_test(NAME native_tu15_convex_pair
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_convex_pair_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-convex-pair.json" --cases 10000 --jobs 2)
    set_tests_properties(native_tu15_convex_pair PROPERTIES TIMEOUT 3600)
    # 1F7C10 BSP child shape (children 1F76A0, 1F5FE0, 283450 and five
    # dispatch targets).
    add_test(NAME native_tu15_bsp_child_shape
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_child_shape_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-child-shape.json" --cases 10000 --jobs 2)
    set_tests_properties(native_tu15_bsp_child_shape PROPERTIES TIMEOUT 2400)
    # 1F6D10 structure AABB query (children 2E9670, 1F6FA0, 1F76A0 and the
    # definition's queryObb through the dispatch; 2E4550 inline).
    add_test(NAME native_tu15_structure_aabb_query
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_structure_aabb_query_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-structure-aabb-query.json" --cases 10000 --jobs 2)
    set_tests_properties(native_tu15_structure_aabb_query PROPERTIES TIMEOUT 2400)
    # 2F36F0 child agent update (children 2E31E0, 2F3430, 2F34D0, 2F33B0 and
    # 17 dispatch sites; 2E4550 and 30BAD0 inline).
    add_test(NAME native_tu15_child_agents
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_child_agents_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-child-agents.json" --cases 10000 --jobs 2)
    set_tests_properties(native_tu15_child_agents PROPERTIES TIMEOUT 2400)
    # E8D80 object flag update (the vehicle update unit; its update functions
    # through the dispatch).
    add_test(NAME native_tu15_object_flag_update
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_flag_update_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-flag-update.json" --cases 10000 --jobs 2)
    set_tests_properties(native_tu15_object_flag_update PROPERTIES TIMEOUT 2400)
endif()
