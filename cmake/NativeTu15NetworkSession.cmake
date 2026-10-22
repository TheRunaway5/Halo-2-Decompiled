add_library(halo2_network_session_startup_tu15 STATIC src/network/source_network_session_startup_tu15_generated.cpp)
target_include_directories(halo2_network_session_startup_tu15 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(halo2_network_session_startup_tu15 PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(halo2_network_session_startup_tu15 PRIVATE /fp:strict)
else()
    target_compile_options(halo2_network_session_startup_tu15 PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endif()
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_network_session_startup_tu15 PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_network_session_startup_tu15 PUBLIC -fsanitize=address,undefined)
endif()
target_link_libraries(halo2_systems PUBLIC halo2_network_session_startup_tu15)
target_sources(halo2_systems PRIVATE src/network/native_tu15_network_session_data.cpp)
target_sources(halo2_systems PRIVATE src/network/native_tu15_network_session_presence_data.cpp)
target_sources(halo2_systems PRIVATE src/network/native_tu15_network_sockets.cpp)
target_sources(halo2_systems PRIVATE src/input/native_tu15_profile_file.cpp)
if(BUILD_TESTING)
    target_sources(native_tu15_network_link_tests PRIVATE src/network/native_tu15_network_session_data.cpp)
    target_sources(native_tu15_network_link_tests PRIVATE src/network/native_tu15_network_session_presence_data.cpp)
    target_sources(native_tu15_network_link_tests PRIVATE src/network/native_tu15_network_sockets.cpp)
    target_sources(native_tu15_network_link_tests PRIVATE src/input/native_tu15_profile_file.cpp)
    target_link_libraries(native_tu15_network_link_tests PRIVATE halo2_network_session_startup_tu15)
    add_test(NAME native_tu15_network_session_callback COMMAND native_tu15_network_link_tests --frontier session-complete)
    add_test(NAME original_tu15_network_session_callback COMMAND native_tu15_network_link_tests --frontier session-complete
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-network-session-v6/report.json)
    set_tests_properties(native_tu15_network_session_callback original_tu15_network_session_callback PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(native_tu15_network_session_presence_tests
        tests/native_tu15_network_session_presence_tests.cpp
        src/network/native_tu15_network_session_presence_data.cpp
        src/network/native_tu15_network_session_data.cpp
        src/platform/native_tu15_static_registration_data.cpp
        src/runtime/source_runtime_memory.cpp)
    target_include_directories(native_tu15_network_session_presence_tests PRIVATE include)
    target_link_libraries(native_tu15_network_session_presence_tests PRIVATE halo2_snapshot_storage Threads::Threads)
    add_test(NAME native_tu15_network_session_presence_storage COMMAND native_tu15_network_session_presence_tests)
    set_tests_properties(native_tu15_network_session_presence_storage PROPERTIES TIMEOUT 20)
endif()
