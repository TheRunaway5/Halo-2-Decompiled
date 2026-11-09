# Pinned native ICE/DTLS/SCTP stack. Only unordered, zero-retransmit UDP data
# channels are used. Media and WebSocket implementations are not bundled.
include(FetchContent)
function(halo2_load_peer_stack)
set(BUILD_SHARED_LIBS OFF)
set(NO_MEDIA ON)
set(NO_WEBSOCKET ON)
set(NO_EXAMPLES ON)
set(NO_TESTS ON)
set(BUILD_SHARED_DEPS_LIBS OFF)
FetchContent_Declare(halo2_datachannel
    GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel.git
    GIT_TAG 6b1e2e620f1e37f0eafeee702eaea0043cb305fd
    GIT_SUBMODULES deps/libjuice deps/plog deps/usrsctp
    GIT_SHALLOW FALSE
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(halo2_datachannel)
execute_process(COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/apply_online_peer_patches.py" "${halo2_datachannel_SOURCE_DIR}"
    RESULT_VARIABLE peer_patch_result COMMAND_ERROR_IS_FATAL ANY)
endfunction()
halo2_load_peer_stack()
# Native result observations compile in the gameplay library; their only
# policy entry is the generated Rust bridge.
set_property(SOURCE src/network/online_postgame.cpp APPEND PROPERTY
    INCLUDE_DIRECTORIES "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_dependencies(halo2_systems halo2_online_protocol_rust_build)
target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking_lobby.cpp)
set_property(SOURCE src/ui/frontend_matchmaking_lobby.cpp APPEND PROPERTY
    INCLUDE_DIRECTORIES "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_library(halo2_frontend_lobby_view src/ui/frontend_lobby.cpp src/ui/frontend_lobby_game_bridge.cpp)
target_include_directories(halo2_frontend_lobby_view PUBLIC include PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
target_compile_features(halo2_frontend_lobby_view PUBLIC cxx_std_20)
target_link_libraries(halo2_frontend_lobby_view PRIVATE halo2_online_protocol halo2_game_variant)
add_dependencies(halo2_frontend_lobby_view halo2_online_protocol_rust_build)
target_link_libraries(halo2_systems PRIVATE halo2_frontend_lobby_view)
# Some legacy targets are registered after this module is included. Finalize
# their ownership after the parent directory finishes declaring its targets.
# Keep the migration local to this module, including frozen validation cohorts.
function(halo2_finalize_frontend_lobby_view)
    get_target_property(lobby_view_system_sources halo2_systems SOURCES)
    list(REMOVE_ITEM lobby_view_system_sources src/ui/frontend_lobby.cpp)
    set_property(TARGET halo2_systems PROPERTY SOURCES "${lobby_view_system_sources}")
    if(TARGET frontend_lobby_game_tests)
        target_link_libraries(frontend_lobby_game_tests PRIVATE halo2_frontend_lobby_view)
    endif()
    if(TARGET frontend_lobby_setup_tests)
        target_link_libraries(frontend_lobby_setup_tests PRIVATE halo2_frontend_lobby)
    endif()
endfunction()
cmake_language(DEFER CALL halo2_finalize_frontend_lobby_view)
add_library(halo2_frontend_lobby src/ui/frontend_matchmaking_lobby_bridge.cpp)
target_include_directories(halo2_frontend_lobby PUBLIC include PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
target_compile_features(halo2_frontend_lobby PUBLIC cxx_std_20)
target_link_libraries(halo2_frontend_lobby PRIVATE halo2_online_protocol)
add_dependencies(halo2_frontend_lobby halo2_online_protocol_rust_build)
target_link_libraries(halo2_systems PRIVATE halo2_frontend_lobby)
add_library(halo2_frontend_search src/ui/frontend_matchmaking_search_bridge.cpp)
target_include_directories(halo2_frontend_search PUBLIC include PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
target_compile_features(halo2_frontend_search PUBLIC cxx_std_20)
target_link_libraries(halo2_frontend_search PRIVATE halo2_online_protocol)
add_dependencies(halo2_frontend_search halo2_online_protocol_rust_build)
target_link_libraries(halo2_systems PRIVATE halo2_frontend_search)
add_library(halo2_frontend_progress src/ui/frontend_matchmaking_policy_bridge.cpp)
target_include_directories(halo2_frontend_progress PUBLIC include PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
target_compile_features(halo2_frontend_progress PUBLIC cxx_std_20)
target_link_libraries(halo2_frontend_progress PRIVATE halo2_online_protocol)
add_dependencies(halo2_frontend_progress halo2_online_protocol_rust_build)
target_link_libraries(halo2_systems PRIVATE halo2_frontend_progress)
add_library(halo2_frontend_presentation src/ui/frontend_matchmaking_presentation_bridge.cpp)
target_include_directories(halo2_frontend_presentation PUBLIC include PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
target_compile_features(halo2_frontend_presentation PUBLIC cxx_std_20)
target_link_libraries(halo2_frontend_presentation PRIVATE halo2_online_protocol)
add_dependencies(halo2_frontend_presentation halo2_online_protocol_rust_build)
target_link_libraries(halo2_systems PRIVATE halo2_frontend_presentation)
target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking_presentation.cpp)
add_library(halo2_frontend_party src/ui/frontend_matchmaking_party_bridge.cpp)
target_include_directories(halo2_frontend_party PUBLIC include PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
target_compile_features(halo2_frontend_party PUBLIC cxx_std_20)
target_link_libraries(halo2_frontend_party PRIVATE halo2_online_protocol)
add_dependencies(halo2_frontend_party halo2_online_protocol_rust_build)
target_link_libraries(halo2_systems PRIVATE halo2_frontend_party)
add_library(halo2_frontend_party_flow src/ui/frontend_matchmaking_party_flow_bridge.cpp)
target_include_directories(halo2_frontend_party_flow PUBLIC include PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
target_compile_features(halo2_frontend_party_flow PUBLIC cxx_std_20)
target_link_libraries(halo2_frontend_party_flow PRIVATE halo2_online_protocol)
add_dependencies(halo2_frontend_party_flow halo2_online_protocol_rust_build)
target_link_libraries(halo2_systems PRIVATE halo2_frontend_party_flow)
target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking_party_flow.cpp)
target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking_party_view.cpp)
target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking_lobby_query.cpp)
target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking_account_flow.cpp)
target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking_friends.cpp)
target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking_clans.cpp)
target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking_couch.cpp)
set_property(SOURCE src/ui/frontend_matchmaking_couch.cpp APPEND PROPERTY
    INCLUDE_DIRECTORIES "${HALO2_ONLINE_BRIDGE_INCLUDE}")
target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking_signin.cpp)
set_property(SOURCE src/ui/frontend_matchmaking_signin.cpp APPEND PROPERTY
    INCLUDE_DIRECTORIES "${HALO2_ONLINE_BRIDGE_INCLUDE}")
target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking_postgame.cpp)
set_property(SOURCE src/ui/frontend_matchmaking_postgame.cpp APPEND PROPERTY
    INCLUDE_DIRECTORIES "${HALO2_ONLINE_BRIDGE_INCLUDE}")
set_property(SOURCE src/ui/frontend_matchmaking_clans.cpp APPEND PROPERTY
    INCLUDE_DIRECTORIES "${HALO2_ONLINE_BRIDGE_INCLUDE}")
set_property(SOURCE src/ui/frontend_matchmaking_friends.cpp APPEND PROPERTY
    INCLUDE_DIRECTORIES "${HALO2_ONLINE_BRIDGE_INCLUDE}")
set_property(SOURCE src/ui/frontend_matchmaking_account_flow.cpp APPEND PROPERTY
    INCLUDE_DIRECTORIES "${HALO2_ONLINE_BRIDGE_INCLUDE}")
set_property(SOURCE src/ui/frontend_matchmaking_lobby_query.cpp APPEND PROPERTY
    INCLUDE_DIRECTORIES "${HALO2_ONLINE_BRIDGE_INCLUDE}")
set_property(SOURCE src/ui/frontend_matchmaking_party_view.cpp APPEND PROPERTY
    INCLUDE_DIRECTORIES "${HALO2_ONLINE_BRIDGE_INCLUDE}")
set_property(SOURCE src/ui/frontend_matchmaking_party_flow.cpp APPEND PROPERTY
    INCLUDE_DIRECTORIES "${HALO2_ONLINE_BRIDGE_INCLUDE}")

target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking_search.cpp)
set_property(SOURCE src/ui/frontend_matchmaking_search.cpp APPEND PROPERTY
    INCLUDE_DIRECTORIES "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_library(halo2_online_peers src/network/online_peer_transport.cpp)
add_library(halo2_authority_transfer src/network/peer_authority_transfer.cpp)
add_library(halo2_source_admissions src/network/peer_source_admissions.cpp)
add_library(halo2_peer_mesh_quality src/network/peer_mesh_quality.cpp)
add_library(halo2_peer_host_agreement src/network/peer_host_agreement.cpp)
add_library(halo2_peer_common_cut src/network/peer_common_cut.cpp)
add_library(halo2_peer_survivor_selection src/network/peer_survivor_selection.cpp src/network/peer_prospective_role.cpp src/network/peer_migration_preparation.cpp)
target_include_directories(halo2_peer_survivor_selection PUBLIC include PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_dependencies(halo2_peer_survivor_selection halo2_online_protocol_rust_build)
target_compile_features(halo2_peer_survivor_selection PUBLIC cxx_std_20)
target_link_libraries(halo2_peer_survivor_selection PRIVATE halo2_online_protocol Threads::Threads)
target_link_libraries(halo2_online_matchmaking PRIVATE halo2_peer_survivor_selection)
target_include_directories(halo2_peer_common_cut PUBLIC include)
target_include_directories(halo2_peer_common_cut PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_dependencies(halo2_peer_common_cut halo2_online_protocol_rust_build)
target_compile_features(halo2_peer_common_cut PUBLIC cxx_std_20)
target_link_libraries(halo2_peer_common_cut PRIVATE halo2_online_protocol Threads::Threads)
target_include_directories(halo2_peer_host_agreement PUBLIC include)
target_include_directories(halo2_peer_host_agreement PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_dependencies(halo2_peer_host_agreement halo2_online_protocol_rust_build)
target_compile_features(halo2_peer_host_agreement PUBLIC cxx_std_20)
target_link_libraries(halo2_peer_host_agreement PRIVATE halo2_online_protocol Threads::Threads)
if(HALO2_SANITIZERS AND NOT MSVC)
    foreach(owner halo2_peer_host_agreement halo2_peer_common_cut halo2_peer_survivor_selection halo2_source_admissions halo2_online_peers)
        target_compile_options(${owner} PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${owner} PUBLIC -fsanitize=address,undefined)
    endforeach()
endif()
target_include_directories(halo2_peer_mesh_quality PUBLIC include)
target_include_directories(halo2_peer_mesh_quality PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_dependencies(halo2_peer_mesh_quality halo2_online_protocol_rust_build)
target_compile_features(halo2_peer_mesh_quality PUBLIC cxx_std_20)
target_link_libraries(halo2_peer_mesh_quality PRIVATE halo2_online_protocol Threads::Threads)
target_include_directories(halo2_source_admissions PUBLIC include)
target_include_directories(halo2_source_admissions PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_dependencies(halo2_source_admissions halo2_online_protocol_rust_build)
target_compile_features(halo2_source_admissions PUBLIC cxx_std_20)
target_link_libraries(halo2_source_admissions PRIVATE halo2_online_protocol Threads::Threads)
target_include_directories(halo2_authority_transfer PUBLIC include)
target_include_directories(halo2_authority_transfer PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_dependencies(halo2_authority_transfer halo2_online_protocol_rust_build)
target_compile_features(halo2_authority_transfer PUBLIC cxx_std_20)
target_link_libraries(halo2_authority_transfer PRIVATE halo2_online_protocol OpenSSL::Crypto Threads::Threads)
if(BUILD_TESTING)
    set(_frontend_party_before "${CMAKE_CURRENT_BINARY_DIR}/frontend_party_before.inc")
    set(_frontend_party_after "${CMAKE_CURRENT_BINARY_DIR}/frontend_party_after.inc")
    add_custom_command(OUTPUT "${_frontend_party_before}" "${_frontend_party_after}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_party_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_party_before_rust_20261102"
            --current "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/frontend_party.cpp"
            --before "${_frontend_party_before}" --after "${_frontend_party_after}"
        DEPENDS tests/generate_frontend_party_reference.py src/ui/frontend_party.cpp
            tests/reference/frontend_party_before_rust_20261102/frontend_party.cpp.reference
            tests/reference/frontend_party_before_rust_20261102/provenance.json
        VERBATIM)
    add_executable(native_frontend_party_rust_tests tests/native_frontend_party_rust_tests.cpp "${_frontend_party_before}" "${_frontend_party_after}")
    target_include_directories(native_frontend_party_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_features(native_frontend_party_rust_tests PRIVATE cxx_std_20)
    target_link_libraries(native_frontend_party_rust_tests PRIVATE halo2_frontend_party halo2_frontend_party_flow nlohmann_json::nlohmann_json)
    add_test(NAME frontend_party_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_party_rust.py" $<TARGET_FILE:native_frontend_party_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-party-policy.json")
    set_tests_properties(frontend_party_cpp_rust PROPERTIES TIMEOUT 240)
    set(_lobby_view_before "${CMAKE_CURRENT_BINARY_DIR}/frontend_lobby_view_before.inc")
    add_custom_command(OUTPUT "${_lobby_view_before}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_lobby_view_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_party_lobby_view_before_rust_20261102"
            --output "${_lobby_view_before}"
        DEPENDS tests/generate_frontend_lobby_view_reference.py
            tests/reference/frontend_party_lobby_view_before_rust_20261102/frontend_lobby.cpp.reference
            tests/reference/frontend_party_lobby_view_before_rust_20261102/frontend_lobby_game.hpp.reference
            tests/reference/frontend_party_lobby_view_before_rust_20261102/provenance.json
        VERBATIM)
    add_executable(native_frontend_lobby_view_rust_tests tests/native_frontend_lobby_view_rust_tests.cpp "${_lobby_view_before}")
    target_include_directories(native_frontend_lobby_view_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_features(native_frontend_lobby_view_rust_tests PUBLIC cxx_std_20)
    target_link_libraries(native_frontend_lobby_view_rust_tests PRIVATE halo2_frontend_lobby_view nlohmann_json::nlohmann_json)
    add_test(NAME frontend_lobby_view_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_lobby_view_rust.py" $<TARGET_FILE:native_frontend_lobby_view_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-lobby-view-policy.json")
    set_tests_properties(frontend_lobby_view_cpp_rust PROPERTIES TIMEOUT 240)
    set(_lobby_query_before "${CMAKE_CURRENT_BINARY_DIR}/frontend_lobby_query_before.inc")
    set(_lobby_query_after "${CMAKE_CURRENT_BINARY_DIR}/frontend_lobby_query_after.inc")
    add_custom_command(OUTPUT "${_lobby_query_before}" "${_lobby_query_after}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_lobby_query_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_lobby_query_before_rust_20261102"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --before "${_lobby_query_before}" --after "${_lobby_query_after}"
        DEPENDS tests/generate_frontend_lobby_query_reference.py src/ui/frontend_matchmaking_lobby_query.cpp
            src/ui/frontend_state.cpp tests/reference/frontend_lobby_query_before_rust_20261102/frontend_state.cpp.reference
            tests/reference/frontend_lobby_query_before_rust_20261102/frontend_lobby_team.hpp.reference
            tests/reference/frontend_lobby_query_before_rust_20261102/frontend_lobby_ready.hpp.reference
            tests/reference/frontend_lobby_query_before_rust_20261102/provenance.json
        VERBATIM)
    add_executable(native_frontend_lobby_query_rust_tests tests/native_frontend_lobby_query_rust_tests.cpp
        "${_lobby_query_before}" "${_lobby_query_after}")
    target_include_directories(native_frontend_lobby_query_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_compile_features(native_frontend_lobby_query_rust_tests PUBLIC cxx_std_20)
    target_link_libraries(native_frontend_lobby_query_rust_tests PRIVATE halo2_frontend_lobby_view halo2_frontend_party_flow nlohmann_json::nlohmann_json)
    add_test(NAME frontend_lobby_query_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_lobby_query_rust.py" $<TARGET_FILE:native_frontend_lobby_query_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-lobby-query-policy.json")
    set_tests_properties(frontend_lobby_query_cpp_rust PROPERTIES TIMEOUT 240)
    set(_party_view_before "${CMAKE_CURRENT_BINARY_DIR}/frontend_party_view_before.inc")
    set(_party_view_after "${CMAKE_CURRENT_BINARY_DIR}/frontend_party_view_after.inc")
    set(_party_view_adapter "${CMAKE_CURRENT_BINARY_DIR}/frontend_party_view_adapter.inc")
    add_custom_command(OUTPUT "${_party_view_before}" "${_party_view_after}" "${_party_view_adapter}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_party_view_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_party_view_before_rust_20261102"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --before "${_party_view_before}"
            --after "${_party_view_after}" --adapter "${_party_view_adapter}"
        DEPENDS tests/generate_frontend_party_view_reference.py src/ui/frontend_party.cpp
            src/ui/frontend_matchmaking_party_view.cpp
            tests/reference/frontend_party_view_before_rust_20261102/frontend_party.cpp.reference
            tests/reference/frontend_party_view_before_rust_20261102/provenance.json
        VERBATIM)
    add_executable(native_frontend_party_view_rust_tests tests/native_frontend_party_view_rust_tests.cpp
        "${_party_view_before}" "${_party_view_after}" "${_party_view_adapter}")
    target_include_directories(native_frontend_party_view_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_compile_features(native_frontend_party_view_rust_tests PUBLIC cxx_std_20)
    target_link_libraries(native_frontend_party_view_rust_tests PRIVATE halo2_frontend_party_flow halo2_frontend_lobby_view nlohmann_json::nlohmann_json)
    add_test(NAME frontend_party_view_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_party_view_rust.py" $<TARGET_FILE:native_frontend_party_view_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-party-view-policy.json")
    set_tests_properties(frontend_party_view_cpp_rust PROPERTIES TIMEOUT 240)
    set(_party_flow_before "${CMAKE_CURRENT_BINARY_DIR}/frontend_party_flow_before.inc")
    set(_party_flow_after "${CMAKE_CURRENT_BINARY_DIR}/frontend_party_flow_after.inc")
    add_custom_command(OUTPUT "${_party_flow_before}" "${_party_flow_after}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_party_flow_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_party_flow_before_rust_20261102"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --before "${_party_flow_before}" --after "${_party_flow_after}"
        DEPENDS tests/generate_frontend_party_flow_reference.py src/ui/frontend_party.cpp
            src/ui/frontend_matchmaking_party_flow.cpp
            tests/reference/frontend_party_flow_before_rust_20261102/frontend_party.cpp.reference
            tests/reference/frontend_party_flow_before_rust_20261102/frontend_state.cpp.reference
            tests/reference/frontend_party_flow_before_rust_20261102/frontend_clans.cpp.reference
            tests/reference/frontend_party_flow_before_rust_20261102/provenance.json
        VERBATIM)
    add_executable(native_frontend_party_flow_rust_tests tests/native_frontend_party_flow_rust_tests.cpp "${_party_flow_before}" "${_party_flow_after}")
    target_include_directories(native_frontend_party_flow_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_compile_features(native_frontend_party_flow_rust_tests PRIVATE cxx_std_20)
    target_link_libraries(native_frontend_party_flow_rust_tests PRIVATE halo2_frontend_party_flow halo2_frontend_party nlohmann_json::nlohmann_json)
    add_test(NAME frontend_party_flow_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_party_flow_rust.py" $<TARGET_FILE:native_frontend_party_flow_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-party-flow.json")
    set_tests_properties(frontend_party_flow_cpp_rust PROPERTIES TIMEOUT 240)
    set(_account_flow_before "${CMAKE_CURRENT_BINARY_DIR}/frontend_account_flow_before.inc")
    set(_account_flow_after "${CMAKE_CURRENT_BINARY_DIR}/frontend_account_flow_after.inc")
    add_custom_command(OUTPUT "${_account_flow_before}" "${_account_flow_after}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_account_flow_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_account_flow_before_rust_20261102"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --before "${_account_flow_before}" --after "${_account_flow_after}"
        DEPENDS tests/generate_frontend_account_flow_reference.py src/ui/frontend_clans.cpp
            src/ui/frontend_friends.cpp src/ui/frontend_matchmaking_account_flow.cpp
            tests/reference/frontend_account_flow_before_rust_20261102/src__ui__frontend_clans.cpp.reference
            tests/reference/frontend_account_flow_before_rust_20261102/src__ui__frontend_friends.cpp.reference
            tests/reference/frontend_account_flow_before_rust_20261102/provenance.json
        VERBATIM)
    add_executable(native_frontend_account_flow_rust_tests tests/native_frontend_account_flow_rust_tests.cpp
        "${_account_flow_before}" "${_account_flow_after}")
    target_include_directories(native_frontend_account_flow_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_compile_features(native_frontend_account_flow_rust_tests PRIVATE cxx_std_20)
    target_link_libraries(native_frontend_account_flow_rust_tests PRIVATE halo2_frontend_party_flow halo2_frontend_party nlohmann_json::nlohmann_json)
    add_test(NAME frontend_account_flow_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_account_flow_rust.py" $<TARGET_FILE:native_frontend_account_flow_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-account-flow-policy.json")
    set_tests_properties(frontend_account_flow_cpp_rust PROPERTIES TIMEOUT 240)
    set(_friends_before "${CMAKE_CURRENT_BINARY_DIR}/frontend_friends_before.inc")
    set(_friends_after "${CMAKE_CURRENT_BINARY_DIR}/frontend_friends_after.inc")
    add_custom_command(OUTPUT "${_friends_before}" "${_friends_after}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_friends_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_friends_before_rust_20261102"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --before "${_friends_before}" --after "${_friends_after}"
        DEPENDS tests/generate_frontend_friends_reference.py src/ui/frontend_friends.cpp
            src/ui/frontend_matchmaking_friends.cpp
            tests/reference/frontend_friends_before_rust_20261102/src__ui__frontend_friends.cpp.reference
            tests/reference/frontend_friends_before_rust_20261102/provenance.json
        VERBATIM)
    add_executable(native_frontend_friends_rust_tests tests/native_frontend_friends_rust_tests.cpp
        "${_friends_before}" "${_friends_after}")
    target_include_directories(native_frontend_friends_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_compile_features(native_frontend_friends_rust_tests PRIVATE cxx_std_20)
    target_link_libraries(native_frontend_friends_rust_tests PRIVATE halo2_frontend_party_flow nlohmann_json::nlohmann_json)
    add_test(NAME frontend_friends_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_friends_rust.py" $<TARGET_FILE:native_frontend_friends_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-friends-policy.json")
    set_tests_properties(frontend_friends_cpp_rust PROPERTIES TIMEOUT 240)
    set(_clans_before "${CMAKE_CURRENT_BINARY_DIR}/frontend_clans_before.inc")
    set(_clans_after "${CMAKE_CURRENT_BINARY_DIR}/frontend_clans_after.inc")
    add_custom_command(OUTPUT "${_clans_before}" "${_clans_after}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_clans_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_clans_before_rust_20261102"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --before "${_clans_before}" --after "${_clans_after}"
        DEPENDS tests/generate_frontend_clans_reference.py src/ui/frontend_clans.cpp
            src/ui/frontend_matchmaking_clans.cpp src/ui/frontend_state.cpp
            tests/reference/frontend_clans_before_rust_20261102/src__ui__frontend_clans.cpp.reference
            tests/reference/frontend_clans_before_rust_20261102/src__ui__frontend_state.cpp.reference
            tests/reference/frontend_clans_before_rust_20261102/provenance.json
        VERBATIM)
    add_executable(native_frontend_clans_rust_tests tests/native_frontend_clans_rust_tests.cpp
        "${_clans_before}" "${_clans_after}")
    target_include_directories(native_frontend_clans_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_compile_features(native_frontend_clans_rust_tests PRIVATE cxx_std_20)
    target_link_libraries(native_frontend_clans_rust_tests PRIVATE halo2_frontend_party_flow nlohmann_json::nlohmann_json)
    add_test(NAME frontend_clans_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_clans_rust.py" $<TARGET_FILE:native_frontend_clans_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-clans-policy.json")
    set_tests_properties(frontend_clans_cpp_rust PROPERTIES TIMEOUT 240)
    set(_couch_before "${CMAKE_CURRENT_BINARY_DIR}/frontend_couch_before.inc")
    set(_couch_after "${CMAKE_CURRENT_BINARY_DIR}/frontend_couch_after.inc")
    add_custom_command(OUTPUT "${_couch_before}" "${_couch_after}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_couch_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_couch_before_rust_20261102"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --before "${_couch_before}" --after "${_couch_after}"
        DEPENDS tests/generate_frontend_couch_reference.py src/ui/frontend_couch.cpp
            include/halo2/frontend.hpp
            tests/reference/frontend_couch_before_rust_20261102/include__halo2__frontend.hpp.reference
            src/ui/frontend_matchmaking_couch.cpp src/ui/frontend_state.cpp
            tests/reference/frontend_couch_before_rust_20261102/src__ui__frontend_couch.cpp.reference
            tests/reference/frontend_couch_before_rust_20261102/src__ui__frontend_state.cpp.reference
            tests/reference/frontend_couch_before_rust_20261102/provenance.json
        VERBATIM)
    add_executable(native_frontend_couch_rust_tests tests/native_frontend_couch_rust_tests.cpp
        "${_couch_before}" "${_couch_after}")
    target_include_directories(native_frontend_couch_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_compile_features(native_frontend_couch_rust_tests PRIVATE cxx_std_20)
    target_link_libraries(native_frontend_couch_rust_tests PRIVATE halo2_frontend_party_flow nlohmann_json::nlohmann_json)
    add_test(NAME frontend_couch_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_couch_rust.py" $<TARGET_FILE:native_frontend_couch_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-couch-policy.json")
    set_tests_properties(frontend_couch_cpp_rust PROPERTIES TIMEOUT 240)
    set(_signin_before "${CMAKE_CURRENT_BINARY_DIR}/frontend_signin_before.inc")
    set(_signin_after "${CMAKE_CURRENT_BINARY_DIR}/frontend_signin_after.inc")
    add_custom_command(OUTPUT "${_signin_before}" "${_signin_after}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_signin_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_signin_before_rust_20261102"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --before "${_signin_before}" --after "${_signin_after}"
        DEPENDS tests/generate_frontend_signin_reference.py
            include/halo2/frontend.hpp
            tests/reference/frontend_signin_before_rust_20261102/include__halo2__frontend.hpp.reference
            src/ui/frontend_matchmaking_signin.cpp src/ui/frontend_state.cpp
            tests/reference/frontend_signin_before_rust_20261102/src__ui__frontend_state.cpp.reference
            tests/reference/frontend_signin_before_rust_20261102/provenance.json
        VERBATIM)
    add_executable(native_frontend_signin_rust_tests tests/native_frontend_signin_rust_tests.cpp
        "${_signin_before}" "${_signin_after}")
    target_include_directories(native_frontend_signin_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_compile_features(native_frontend_signin_rust_tests PRIVATE cxx_std_20)
    target_link_libraries(native_frontend_signin_rust_tests PRIVATE halo2_frontend_party_flow nlohmann_json::nlohmann_json)
    add_test(NAME frontend_signin_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_signin_rust.py" $<TARGET_FILE:native_frontend_signin_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-signin-policy.json")
    set_tests_properties(frontend_signin_cpp_rust PROPERTIES TIMEOUT 240)
    set(_postgame_before "${CMAKE_CURRENT_BINARY_DIR}/frontend_postgame_before.inc")
    set(_postgame_after "${CMAKE_CURRENT_BINARY_DIR}/frontend_postgame_after.inc")
    set(_postgame_helpers "${CMAKE_CURRENT_BINARY_DIR}/frontend_postgame_helpers.inc")
    add_custom_command(OUTPUT "${_postgame_before}" "${_postgame_after}" "${_postgame_helpers}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_postgame_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_postgame_before_rust_20261102"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --before "${_postgame_before}" --after "${_postgame_after}" --helpers "${_postgame_helpers}"
        DEPENDS tests/generate_frontend_postgame_reference.py
            include/halo2/frontend.hpp
            tests/reference/frontend_postgame_before_rust_20261102/include__halo2__frontend.hpp.reference
            src/ui/frontend_matchmaking_postgame.cpp src/ui/frontend_state.cpp src/network/online_account.cpp
            tests/reference/frontend_postgame_before_rust_20261102/src__ui__frontend_state.cpp.reference
            tests/reference/frontend_postgame_before_rust_20261102/src__network__online_account.cpp.reference
            tests/reference/frontend_postgame_before_rust_20261102/provenance.json
        VERBATIM)
    add_executable(native_frontend_postgame_rust_tests tests/native_frontend_postgame_rust_tests.cpp
        "${_postgame_before}" "${_postgame_after}" "${_postgame_helpers}")
    target_include_directories(native_frontend_postgame_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_compile_features(native_frontend_postgame_rust_tests PRIVATE cxx_std_20)
    target_link_libraries(native_frontend_postgame_rust_tests PRIVATE halo2_frontend_party_flow nlohmann_json::nlohmann_json)
    add_test(NAME frontend_postgame_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_postgame_rust.py" $<TARGET_FILE:native_frontend_postgame_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-postgame-policy.json")
    set_tests_properties(frontend_postgame_cpp_rust PROPERTIES TIMEOUT 240)
    set(_frontend_presentation_reference "${CMAKE_CURRENT_BINARY_DIR}/frontend_presentation_reference.inc")
    add_custom_command(OUTPUT "${_frontend_presentation_reference}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_presentation_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_presentation_before_rust_20261102"
            --output "${_frontend_presentation_reference}"
        DEPENDS tests/generate_frontend_presentation_reference.py
            tests/reference/frontend_presentation_before_rust_20261102/frontend_matchmaking.cpp.reference
        VERBATIM)
    add_executable(native_frontend_presentation_rust_tests tests/native_frontend_presentation_rust_tests.cpp "${_frontend_presentation_reference}")
    target_include_directories(native_frontend_presentation_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_features(native_frontend_presentation_rust_tests PRIVATE cxx_std_20)
    target_link_libraries(native_frontend_presentation_rust_tests PRIVATE halo2_frontend_presentation halo2_frontend_progress nlohmann_json::nlohmann_json)
    add_test(NAME frontend_presentation_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_presentation_rust.py" $<TARGET_FILE:native_frontend_presentation_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-presentation-policy.json")
    set_tests_properties(frontend_presentation_cpp_rust PROPERTIES TIMEOUT 240)
    # This standalone original-oracle consumer is declared later by the root
    # build. Its former inline countdown helper now calls the Rust owner.
    cmake_language(DEFER CALL target_link_libraries party_countdown_kernel_tests PRIVATE halo2_frontend_progress)
    set(_frontend_progress_reference "${CMAKE_CURRENT_BINARY_DIR}/frontend_progress_reference.inc")
    add_custom_command(OUTPUT "${_frontend_progress_reference}"
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_frontend_progress_reference.py"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/frontend_progress_before_rust_20261102"
            --output "${_frontend_progress_reference}"
        DEPENDS tests/generate_frontend_progress_reference.py
            tests/reference/frontend_progress_before_rust_20261102/frontend_matchmaking.cpp.reference
        VERBATIM)
    add_executable(native_frontend_progress_rust_tests tests/native_frontend_progress_rust_tests.cpp "${_frontend_progress_reference}")
    target_include_directories(native_frontend_progress_rust_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_features(native_frontend_progress_rust_tests PRIVATE cxx_std_20)
    target_link_libraries(native_frontend_progress_rust_tests PRIVATE halo2_frontend_progress nlohmann_json::nlohmann_json)
    add_test(NAME frontend_progress_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_progress_rust.py" $<TARGET_FILE:native_frontend_progress_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-progress-policy.json")
    set_tests_properties(frontend_progress_cpp_rust PROPERTIES TIMEOUT 240)
    add_executable(native_frontend_search_rust_tests tests/native_frontend_search_rust_tests.cpp)
    target_compile_features(native_frontend_search_rust_tests PRIVATE cxx_std_20)
    target_include_directories(native_frontend_search_rust_tests PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_link_libraries(native_frontend_search_rust_tests PRIVATE halo2_frontend_search halo2_online_protocol nlohmann_json::nlohmann_json)
    add_dependencies(native_frontend_search_rust_tests halo2_online_protocol_rust_build)
    add_test(NAME frontend_search_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_frontend_search_rust.py" $<TARGET_FILE:native_frontend_search_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-search-policy.json")
    set_tests_properties(frontend_search_cpp_rust PROPERTIES TIMEOUT 240)
    add_executable(frontend_matchmaking_lobby_rust_tests tests/frontend_matchmaking_lobby_rust_tests.cpp)
    target_compile_features(frontend_matchmaking_lobby_rust_tests PRIVATE cxx_std_20)
    target_include_directories(frontend_matchmaking_lobby_rust_tests PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_link_libraries(frontend_matchmaking_lobby_rust_tests PRIVATE halo2_frontend_lobby halo2_online_protocol nlohmann_json::nlohmann_json)
    add_dependencies(frontend_matchmaking_lobby_rust_tests halo2_online_protocol_rust_build)
    add_test(NAME frontend_lobby_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_frontend_lobby_policy.py" $<TARGET_FILE:frontend_matchmaking_lobby_rust_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/frontend-lobby-policy.json")
    set_tests_properties(frontend_lobby_cpp_rust PROPERTIES TIMEOUT 240)
    add_executable(peer_survivor_selection_tests tests/peer_survivor_selection_tests.cpp)
    target_link_libraries(peer_survivor_selection_tests PRIVATE halo2_peer_survivor_selection halo2_peer_host_agreement halo2_peer_common_cut)
    add_test(NAME peer_survivor_selection COMMAND peer_survivor_selection_tests)
    add_executable(peer_survivor_session_tests tests/peer_survivor_session_tests.cpp)
    target_link_libraries(peer_survivor_session_tests PRIVATE halo2_online_matchmaking nlohmann_json::nlohmann_json)
    foreach(selection_case normal mixed abort preparation)
        add_test(NAME peer_survivor_selection_${selection_case}_session COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/peer_host_session_http_test.py" $<TARGET_FILE:peer_survivor_session_tests>)
        set_tests_properties(peer_survivor_selection_${selection_case}_session PROPERTIES TIMEOUT 100
            ENVIRONMENT "HALO2_PEER_SELECTION_CASE=${selection_case};HALO2_PEER_MIGRATION_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/peer-selection-${selection_case}")
    endforeach()
    add_executable(online_eos_bootstrap_tests tests/online_eos_bootstrap_tests.cpp)
    target_include_directories(online_eos_bootstrap_tests PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_link_libraries(online_eos_bootstrap_tests PRIVATE halo2_online_protocol)
    add_dependencies(online_eos_bootstrap_tests halo2_online_protocol_rust_build)
    halo2_bundle_eos(online_eos_bootstrap_tests)
    add_test(NAME online_eos_release_bootstrap COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_eos_bootstrap.py" $<TARGET_FILE:online_eos_bootstrap_tests>)
    # Forwarding SDK observers use the actual official headers and GNU linker
    # seam. No successful external credential or cloud account is needed.
    if(TARGET halo2_eos_sdk AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_executable(online_eos_account_http_tests tests/online_eos_account_http_tests.cpp)
        # SDK archive extraction happens in halo2_add_online_protocol's scope;
        # its imported target retains the actual selected library location.
        get_target_property(eos_account_sdk_library halo2_eos_sdk IMPORTED_LOCATION)
        get_filename_component(eos_account_sdk_bin "${eos_account_sdk_library}" DIRECTORY)
        # Observe real child SDK entries without linking any game/RTC owner
        # into the dedicated worker. Parent --wrap observers remain below.
        add_library(eos_sdk_worker_observer SHARED tests/eos_sdk_worker_observer.cpp)
        target_compile_features(eos_sdk_worker_observer PRIVATE cxx_std_20)
        target_include_directories(eos_sdk_worker_observer PRIVATE "${eos_account_sdk_bin}/../Include")
        target_link_libraries(eos_sdk_worker_observer PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
        target_compile_definitions(online_eos_account_http_tests PRIVATE
            HALO2_EOS_WORKER_OBSERVER="$<TARGET_FILE:eos_sdk_worker_observer>")
        target_include_directories(online_eos_account_http_tests PRIVATE "${eos_account_sdk_bin}/../Include" "${HALO2_ONLINE_BRIDGE_INCLUDE}")
        add_dependencies(online_eos_account_http_tests halo2_online_protocol_rust_build eos_sdk_worker_observer)
        target_link_libraries(online_eos_account_http_tests PRIVATE halo2_online_matchmaking nlohmann_json::nlohmann_json halo2_eos_sdk)
        foreach(eos_call EOS_Connect_CreateDeviceId EOS_Connect_Login EOS_Connect_CreateUser
                         EOS_P2P_AddNotifyPeerConnectionRequest EOS_Platform_Tick)
            target_link_options(online_eos_account_http_tests PRIVATE "-Wl,--wrap=${eos_call}")
        endforeach()
        halo2_bundle_eos(online_eos_account_http_tests)
        foreach(auth_case denied unavailable stalled)
            add_test(NAME online_eos_account_http_${auth_case} COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/online_eos_account_http_test.py"
                $<TARGET_FILE:online_eos_account_http_tests> ${auth_case})
            set_tests_properties(online_eos_account_http_${auth_case} PROPERTIES TIMEOUT 30)
        endforeach()
    endif()
    add_executable(peer_common_cut_tests tests/peer_common_cut_tests.cpp)
    target_link_libraries(peer_common_cut_tests PRIVATE halo2_peer_common_cut)
    add_test(NAME peer_common_cut COMMAND peer_common_cut_tests)
    if(UNIX)
        get_target_property(peer_sctp_source usrsctp SOURCE_DIR)
        get_directory_property(peer_sctp_definitions DIRECTORY "${peer_sctp_source}" COMPILE_DEFINITIONS)
        add_executable(peer_sctp_callout_tests tests/peer_sctp_callout_tests.c)
        target_compile_features(peer_sctp_callout_tests PRIVATE c_std_11)
        target_include_directories(peer_sctp_callout_tests PRIVATE "${peer_sctp_source}")
        target_compile_definitions(peer_sctp_callout_tests PRIVATE ${peer_sctp_definitions})
        target_link_libraries(peer_sctp_callout_tests PRIVATE usrsctp Threads::Threads)
        add_test(NAME peer_sctp_callout COMMAND peer_sctp_callout_tests)
        set_tests_properties(peer_sctp_callout PROPERTIES TIMEOUT 30)
    endif()
    add_executable(peer_host_agreement_tests tests/peer_host_agreement_tests.cpp)
    target_link_libraries(peer_host_agreement_tests PRIVATE halo2_peer_host_agreement)
    add_test(NAME peer_host_agreement COMMAND peer_host_agreement_tests)
    add_executable(peer_host_agreement_direct_tests tests/peer_host_agreement_direct_tests.cpp)
    target_link_libraries(peer_host_agreement_direct_tests PRIVATE halo2_peer_host_agreement halo2_online_peers)
    add_test(NAME peer_host_agreement_direct_udp COMMAND peer_host_agreement_direct_tests)
    set_tests_properties(peer_host_agreement_direct_udp PROPERTIES TIMEOUT 60)
    add_executable(peer_host_session_tests tests/peer_host_session_tests.cpp)
    target_link_libraries(peer_host_session_tests PRIVATE halo2_online_matchmaking nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(peer_host_session_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(peer_host_session_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME peer_host_native_session COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/peer_host_session_http_test.py" $<TARGET_FILE:peer_host_session_tests>)
    set_tests_properties(peer_host_native_session PROPERTIES TIMEOUT 100)
    add_test(NAME peer_host_aborted_cut COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/peer_host_session_http_test.py" $<TARGET_FILE:peer_host_session_tests>)
    set_tests_properties(peer_host_aborted_cut PROPERTIES TIMEOUT 100
        ENVIRONMENT "HALO2_PEER_ABORT_HANDOFF=1;HALO2_PEER_MIGRATION_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/peer-aborted-cut")
    add_test(NAME peer_host_measured_migration COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/peer_host_session_http_test.py" $<TARGET_FILE:peer_host_session_tests>)
    set_tests_properties(peer_host_measured_migration PROPERTIES TIMEOUT 100
        ENVIRONMENT "HALO2_PEER_AUTO_SELECT=1;HALO2_PEER_MIGRATION_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/peer-measured")
    add_test(NAME peer_host_audio_session COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/peer_host_session_http_test.py" $<TARGET_FILE:peer_host_session_tests>)
    set_tests_properties(peer_host_audio_session PROPERTIES TIMEOUT 100
        ENVIRONMENT "HALO2_PEER_AUDIO_HANDOFF=1;HALO2_PEER_MIGRATION_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/peer-audio")
    add_executable(peer_mesh_quality_tests tests/peer_mesh_quality_tests.cpp)
    target_link_libraries(peer_mesh_quality_tests PRIVATE halo2_peer_mesh_quality)
    add_test(NAME peer_mesh_quality COMMAND peer_mesh_quality_tests)
    add_executable(peer_mesh_quality_cpp_reference tests/peer_mesh_quality_replay.cpp tests/reference/peer_mesh_quality_before_rust_20261101.cpp)
    target_compile_features(peer_mesh_quality_cpp_reference PRIVATE cxx_std_20)
    target_include_directories(peer_mesh_quality_cpp_reference BEFORE PRIVATE tests/reference/peer_mesh_quality_before_rust_20261101 include)
    target_link_libraries(peer_mesh_quality_cpp_reference PRIVATE nlohmann_json::nlohmann_json Threads::Threads)
    add_executable(peer_mesh_quality_rust_replay tests/peer_mesh_quality_replay.cpp)
    target_compile_features(peer_mesh_quality_rust_replay PRIVATE cxx_std_20)
    target_link_libraries(peer_mesh_quality_rust_replay PRIVATE halo2_peer_mesh_quality nlohmann_json::nlohmann_json)
    add_test(NAME peer_mesh_quality_cpp_rust_replay COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_peer_mesh_quality_replay.py"
        --reference $<TARGET_FILE:peer_mesh_quality_cpp_reference> --rust $<TARGET_FILE:peer_mesh_quality_rust_replay>
        --output "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/peer-mesh-cpp-rust.json")
    set_tests_properties(peer_mesh_quality_cpp_rust_replay PROPERTIES TIMEOUT 180)
    add_executable(peer_mesh_quality_direct_tests tests/peer_mesh_quality_direct_tests.cpp)
    target_link_libraries(peer_mesh_quality_direct_tests PRIVATE halo2_peer_mesh_quality halo2_online_peers)
    add_test(NAME peer_mesh_quality_direct_udp COMMAND peer_mesh_quality_direct_tests)
    add_executable(peer_mesh_quality_session_tests tests/peer_mesh_quality_session_tests.cpp)
    target_link_libraries(peer_mesh_quality_session_tests PRIVATE halo2_online_matchmaking nlohmann_json::nlohmann_json)
    add_test(NAME peer_mesh_quality_native_session COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/peer_mesh_quality_session_http_test.py" $<TARGET_FILE:peer_mesh_quality_session_tests>)
    set_tests_properties(peer_mesh_quality_native_session PROPERTIES TIMEOUT 60)
    add_test(NAME online_eos_matchmaking_live COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/peer_mesh_quality_session_http_test.py" $<TARGET_FILE:peer_mesh_quality_session_tests>)
    set_tests_properties(online_eos_matchmaking_live PROPERTIES TIMEOUT 110 SKIP_RETURN_CODE 77
        ENVIRONMENT "H2_EOS_MATCH_TEST=1;HALO2_PEER_QUALITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/eos-match-quality")
    add_custom_target(online_peer_quality_qualification DEPENDS
        peer_mesh_quality_tests peer_mesh_quality_direct_tests peer_mesh_quality_session_tests online_peer_transport_tests)
    add_executable(peer_source_admissions_tests tests/peer_source_admissions_tests.cpp)
    target_link_libraries(peer_source_admissions_tests PRIVATE halo2_source_admissions OpenSSL::Crypto)
    add_test(NAME peer_source_admissions COMMAND peer_source_admissions_tests)
    add_executable(peer_source_admissions_cpp_reference tests/peer_source_admissions_replay.cpp tests/reference/peer_source_admissions_before_rust_20261101.cpp)
    target_compile_features(peer_source_admissions_cpp_reference PRIVATE cxx_std_20)
    target_include_directories(peer_source_admissions_cpp_reference BEFORE PRIVATE tests/reference/peer_source_admissions_before_rust_20261101 include)
    target_link_libraries(peer_source_admissions_cpp_reference PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto Threads::Threads)
    add_executable(peer_source_admissions_rust_replay tests/peer_source_admissions_replay.cpp)
    target_compile_features(peer_source_admissions_rust_replay PRIVATE cxx_std_20)
    target_link_libraries(peer_source_admissions_rust_replay PRIVATE halo2_source_admissions nlohmann_json::nlohmann_json)
    add_test(NAME peer_source_admissions_cpp_rust_replay COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_peer_source_admissions_replay.py"
        --reference $<TARGET_FILE:peer_source_admissions_cpp_reference> --rust $<TARGET_FILE:peer_source_admissions_rust_replay>
        --output "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/source-admissions-cpp-rust.json")
    set_tests_properties(peer_source_admissions_cpp_rust_replay PROPERTIES TIMEOUT 180)
    add_executable(native_session_api_tests tests/native_session_api_tests.cpp)
    add_executable(native_result_authority_tests tests/native_result_authority_tests.cpp)
    target_compile_features(native_result_authority_tests PRIVATE cxx_std_20)
    target_include_directories(native_result_authority_tests PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    add_dependencies(native_result_authority_tests halo2_online_protocol_rust_build)
    target_link_libraries(native_result_authority_tests PRIVATE halo2_online_protocol nlohmann_json::nlohmann_json)
    add_test(NAME native_result_authority_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_result_authority.py"
        --binary $<TARGET_FILE:native_result_authority_tests>
        --output "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/native-result-authority.json")
    set_tests_properties(native_result_authority_cpp_rust PROPERTIES TIMEOUT 240)
    target_compile_features(native_session_api_tests PRIVATE cxx_std_20)
    target_include_directories(native_session_api_tests PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    add_dependencies(native_session_api_tests halo2_online_protocol_rust_build)
    target_link_libraries(native_session_api_tests PRIVATE halo2_online_matchmaking nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_test(NAME native_session_api_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_session_api.py"
        --binary $<TARGET_FILE:native_session_api_tests>
        --output "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/native-session-api.json")
    set_tests_properties(native_session_api_cpp_rust PROPERTIES TIMEOUT 240)
    add_executable(native_migration_callers_tests tests/native_migration_callers_tests.cpp)
    target_compile_features(native_migration_callers_tests PRIVATE cxx_std_20)
    target_include_directories(native_migration_callers_tests PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    add_dependencies(native_migration_callers_tests halo2_online_protocol_rust_build)
    target_link_libraries(native_migration_callers_tests PRIVATE halo2_online_matchmaking nlohmann_json::nlohmann_json)
    add_test(NAME native_migration_callers_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_migration_callers.py"
        --binary $<TARGET_FILE:native_migration_callers_tests>
        --output "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/native-migration-callers.json")
    set_tests_properties(native_migration_callers_cpp_rust PROPERTIES TIMEOUT 240)
    add_executable(native_session_access_tests tests/native_session_access_tests.cpp)
    target_compile_features(native_session_access_tests PRIVATE cxx_std_20)
    target_include_directories(native_session_access_tests PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    add_dependencies(native_session_access_tests halo2_online_protocol_rust_build)
    target_link_libraries(native_session_access_tests PRIVATE halo2_online_matchmaking nlohmann_json::nlohmann_json)
    add_test(NAME native_session_access_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_session_access.py"
        --binary $<TARGET_FILE:native_session_access_tests>
        --output "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/native-session-access.json")
    set_tests_properties(native_session_access_cpp_rust PROPERTIES TIMEOUT 240)
    add_executable(native_receive_dispatch_tests tests/native_receive_dispatch_tests.cpp)
    target_compile_features(native_receive_dispatch_tests PRIVATE cxx_std_20)
    target_include_directories(native_receive_dispatch_tests PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    add_dependencies(native_receive_dispatch_tests halo2_online_protocol_rust_build)
    target_link_libraries(native_receive_dispatch_tests PRIVATE halo2_online_matchmaking nlohmann_json::nlohmann_json)
    add_test(NAME native_receive_dispatch_cpp_rust COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_receive_dispatch.py"
        --binary $<TARGET_FILE:native_receive_dispatch_tests>
        --output "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/native-receive-dispatch.json")
    set_tests_properties(native_receive_dispatch_cpp_rust PROPERTIES TIMEOUT 240)
    add_executable(peer_authority_transfer_tests tests/peer_authority_transfer_tests.cpp)
    target_link_libraries(peer_authority_transfer_tests PRIVATE halo2_authority_transfer)
    add_test(NAME peer_authority_transfer COMMAND peer_authority_transfer_tests)
    add_executable(peer_authority_transfer_cpp_reference tests/peer_authority_transfer_replay.cpp tests/reference/peer_authority_transfer_before_rust_20261101.cpp)
    target_compile_features(peer_authority_transfer_cpp_reference PRIVATE cxx_std_20)
    target_include_directories(peer_authority_transfer_cpp_reference BEFORE PRIVATE tests/reference/peer_authority_transfer_before_rust_20261101 include "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    add_dependencies(peer_authority_transfer_cpp_reference halo2_online_protocol_rust_build)
    target_link_libraries(peer_authority_transfer_cpp_reference PRIVATE halo2_online_protocol nlohmann_json::nlohmann_json OpenSSL::Crypto Threads::Threads)
    add_executable(peer_authority_transfer_rust_replay tests/peer_authority_transfer_replay.cpp)
    target_compile_features(peer_authority_transfer_rust_replay PRIVATE cxx_std_20)
    target_link_libraries(peer_authority_transfer_rust_replay PRIVATE halo2_authority_transfer nlohmann_json::nlohmann_json)
    add_test(NAME peer_authority_transfer_cpp_rust_replay COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_peer_authority_transfer_replay.py"
        --reference $<TARGET_FILE:peer_authority_transfer_cpp_reference> --rust $<TARGET_FILE:peer_authority_transfer_rust_replay>
        --output "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/authority-transfer-cpp-rust.json")
    set_tests_properties(peer_authority_transfer_cpp_rust_replay PROPERTIES TIMEOUT 240)
endif()
target_include_directories(halo2_online_peers PUBLIC include)
target_include_directories(halo2_online_peers PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_dependencies(halo2_online_peers halo2_online_protocol_rust_build)
target_link_libraries(halo2_online_peers PRIVATE halo2_online_protocol LibDataChannel::LibDataChannelStatic Threads::Threads)
target_compile_options(halo2_online_peers PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
add_executable(online_peer_transport_tests tests/online_peer_transport_tests.cpp)
target_link_libraries(online_peer_transport_tests PRIVATE halo2_online_peers)
add_test(NAME online_peer_direct_udp COMMAND online_peer_transport_tests)
if(BUILD_TESTING)
    add_executable(online_eos_transport_tests tests/online_eos_transport_tests.cpp)
    target_include_directories(online_eos_transport_tests PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_link_libraries(online_eos_transport_tests PRIVATE halo2_online_peers)
    halo2_bundle_eos(online_eos_transport_tests)
    add_test(NAME online_eos_relay_live COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_online_eos_test.py" $<TARGET_FILE:online_eos_transport_tests>
        "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/eos-relay-live.json")
    set_tests_properties(online_eos_relay_live PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 90)
    add_executable(online_peer_runtime_tests tests/online_peer_runtime_tests.cpp)
    target_link_libraries(online_peer_runtime_tests PRIVATE halo2_online_peers)
    add_test(NAME online_peer_runtime COMMAND online_peer_runtime_tests)
    set_tests_properties(online_peer_runtime PROPERTIES TIMEOUT 90)
endif()
if(UNIX)
    add_executable(online_peer_wan_probe tools/online_peer_wan_probe.cpp)
    target_link_libraries(online_peer_wan_probe PRIVATE halo2_online_peers nlohmann_json::nlohmann_json)
endif()

if(BUILD_TESTING)
    add_executable(native_account_client_tests tests/native_account_client_tests.cpp)
    target_link_libraries(native_account_client_tests PRIVATE halo2_online_account nlohmann_json::nlohmann_json)
    add_test(NAME native_account_client_late_grant COMMAND native_account_client_tests)
    set_tests_properties(native_account_client_late_grant PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE)
    add_executable(native_account_client_cpp_reference tests/native_account_client_tests.cpp
        tests/reference/account_client_before_rust_20261101/online_account_before.cpp)
    target_include_directories(native_account_client_cpp_reference PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_link_libraries(native_account_client_cpp_reference PRIVATE halo2_online_account nlohmann_json::nlohmann_json)
    add_test(NAME native_account_client_cpp_before COMMAND native_account_client_cpp_reference --expect-before)
    set_tests_properties(native_account_client_cpp_before PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE)
    add_executable(account_snapshot_cpp_replay tests/account_snapshot_cpp_replay.cpp)
    target_include_directories(account_snapshot_cpp_replay PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_link_libraries(account_snapshot_cpp_replay PRIVATE halo2_online_account nlohmann_json::nlohmann_json)
    add_test(NAME account_snapshot_cpp_rust_replay COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_account_snapshot_replay.py"
        --reference $<TARGET_FILE:account_snapshot_cpp_replay> --rust-source "${CMAKE_CURRENT_SOURCE_DIR}/rust/online_protocol"
        --output "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/account-snapshot-cpp-rust.json")
    set_tests_properties(account_snapshot_cpp_rust_replay PROPERTIES TIMEOUT 240)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND HALO2_MCC_PARTY_UI_TEST_CACHE)
        add_executable(frontend_account_cpp_reference tests/frontend_online_tests.cpp
            tests/reference/account_client_before_rust_20261101/online_account_before.cpp)
        target_include_directories(frontend_account_cpp_reference PRIVATE include src "${HALO2_ONLINE_BRIDGE_INCLUDE}")
        target_link_libraries(frontend_account_cpp_reference PRIVATE halo2_renderer CURL::libcurl nlohmann_json::nlohmann_json)
        halo2_bundle_discord_social(frontend_account_cpp_reference)
        add_dependencies(frontend_account_cpp_reference halo2_shaders)
        add_test(NAME frontend_account_cpp_rust_diagnostics COMMAND ${Python3_EXECUTABLE} -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_account_frontend_comparison.py"
            --reference $<TARGET_FILE:frontend_account_cpp_reference> --candidate $<TARGET_FILE:frontend_online_tests>
            --source "${CMAKE_CURRENT_SOURCE_DIR}" --mcc-root "${HALO2_MCC_PARTY_UI_TEST_CACHE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/account-frontend-comparison.json")
        set_tests_properties(frontend_account_cpp_rust_diagnostics PROPERTIES TIMEOUT 240 RUN_SERIAL TRUE)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        foreach(account_target native_account_client_tests native_account_client_cpp_reference)
            target_compile_definitions(${account_target} PRIVATE HALO2_ACCOUNT_CURL_OBSERVER=1)
            target_link_options(${account_target} PRIVATE "-Wl,--wrap=curl_easy_perform" "-Wl,--wrap=RAND_bytes")
        endforeach()
        add_test(NAME native_account_client_real_curl COMMAND ${Python3_EXECUTABLE} -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_account_client.py"
            --reference $<TARGET_FILE:native_account_client_cpp_reference> --candidate $<TARGET_FILE:native_account_client_tests>
            --output "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/native-account-client-http.json")
        set_tests_properties(native_account_client_real_curl PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
    endif()
endif()
