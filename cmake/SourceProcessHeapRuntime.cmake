# Authenticated source heap/SEH/copy arithmetic. Resource and synchronization
# calls retain the full caller ABI and remain required external owners.
add_library(halo2_source_process_heap_runtime STATIC src/source_process_heap_runtime_generated.cpp)
target_include_directories(halo2_source_process_heap_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_source_process_heap_runtime PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(halo2_source_process_heap_runtime PRIVATE /fp:strict)
else()
    target_compile_options(halo2_source_process_heap_runtime PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endif()
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_source_process_heap_runtime PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_source_process_heap_runtime PUBLIC -fsanitize=address,undefined)
endif()
target_link_libraries(halo2_systems PUBLIC halo2_source_process_heap_runtime)
add_library(halo2_source_process_heap_platform STATIC src/source_process_heap_platform.cpp src/source_process_shutdown.cpp
    src/source_process_startup_records.cpp)
target_include_directories(halo2_source_process_heap_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_source_process_heap_platform PUBLIC cxx_std_20)
target_link_libraries(halo2_source_process_heap_platform PUBLIC halo2_source_process_heap_runtime halo2_physics_platform_call)
target_link_libraries(halo2_systems PUBLIC halo2_source_process_heap_platform)
add_library(halo2_native_tu15_heap_data STATIC src/native_tu15_heap_data.cpp src/native_tu15_heap_bootstrap.cpp)
target_include_directories(halo2_native_tu15_heap_data PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_heap_data PUBLIC cxx_std_20)
target_link_libraries(halo2_native_tu15_heap_data PUBLIC halo2_source_process_heap_platform)
target_link_libraries(halo2_native_tu15_heap_data PUBLIC halo2_native_synchronization)
target_link_libraries(halo2_systems PRIVATE halo2_native_tu15_heap_data)
if(BUILD_TESTING)
    add_executable(native_tu15_heap_data_tests tests/native_tu15_heap_data_tests.cpp
        src/source_runtime_memory.cpp src/source_runtime_host_virtual_memory.cpp src/source_runtime_image.cpp)
    target_link_libraries(native_tu15_heap_data_tests PRIVATE halo2_snapshot_storage)
    target_link_libraries(native_tu15_heap_data_tests PRIVATE halo2_native_tu15_heap_data
        halo2_source_process_heap_platform nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_heap_bootstrap COMMAND native_tu15_heap_data_tests)
    add_test(NAME original_tu15_heap_bootstrap_data COMMAND native_tu15_heap_data_tests --reference-xbe
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_tu15_heap_bootstrap original_tu15_heap_bootstrap_data PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_process_startup_records_tests tests/source_process_startup_records_tests.cpp
        src/source_runtime_memory.cpp src/source_runtime_image.cpp src/source_runtime_host_virtual_memory.cpp)
    target_link_libraries(source_process_startup_records_tests PRIVATE halo2_snapshot_storage)
    target_link_libraries(source_process_startup_records_tests PRIVATE halo2_source_process_heap_platform nlohmann_json::nlohmann_json)
    add_test(NAME primary_native_process_startup_records COMMAND source_process_startup_records_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/overlay-pass/registration/startup-record-initializers/measured/standalone.json
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/overlay-pass/registration/startup-record-initializers/measured/original-prefix.json
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        /home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo\ 2\ DLC/content/u5English/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-ready/paired/report.json)
    set_tests_properties(primary_native_process_startup_records PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_process_shutdown_tests tests/source_process_shutdown_tests.cpp src/source_runtime_memory.cpp)
    target_link_libraries(source_process_shutdown_tests PRIVATE halo2_snapshot_storage)
    target_link_libraries(source_process_shutdown_tests PRIVATE halo2_source_process_heap_platform nlohmann_json::nlohmann_json)
    add_test(NAME primary_native_process_shutdown_registry COMMAND source_process_shutdown_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/process-shutdown-measured/fixture.json)
    set_tests_properties(primary_native_process_shutdown_registry PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_process_heap_startup_tests tests/source_process_heap_startup_tests.cpp
        src/source_runtime_memory.cpp src/source_runtime_image.cpp src/source_runtime_host_virtual_memory.cpp)
    target_link_libraries(source_process_heap_startup_tests PRIVATE halo2_snapshot_storage)
    target_link_libraries(source_process_heap_startup_tests PRIVATE halo2_source_process_heap_platform nlohmann_json::nlohmann_json)
    foreach(capture IN ITEMS first-boundary shutdown records)
        if(capture STREQUAL "first-boundary")
            set(startup_fixture process-heap-startup-first-boundary-measured)
        elseif(capture STREQUAL "shutdown")
            set(startup_fixture process-heap-startup-shutdown-measured)
        else()
            set(startup_fixture process-heap-startup-records-measured)
        endif()
        add_test(NAME original_source_process_heap_startup_${capture} COMMAND source_process_heap_startup_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/${startup_fixture}/fixture.json
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
            /home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo\ 2\ DLC/content/u5English/default.xbe)
        set_tests_properties(original_source_process_heap_startup_${capture} PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
    add_test(NAME original_runtime_image_tu15 COMMAND source_runtime_image_tests
        /home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo\ 2\ DLC/content/u5English/default.xbe "1.5")
    set_tests_properties(original_runtime_image_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_process_heap_platform_tests tests/source_process_heap_platform_tests.cpp
        src/source_runtime_memory.cpp src/source_runtime_host_virtual_memory.cpp)
    target_link_libraries(source_process_heap_platform_tests PRIVATE halo2_snapshot_storage)
    target_link_libraries(source_process_heap_platform_tests PRIVATE halo2_source_process_heap_platform)
    add_test(NAME native_source_process_heap_platform COMMAND source_process_heap_platform_tests)
    set_tests_properties(native_source_process_heap_platform PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_process_heap_runtime_tests tests/source_process_heap_runtime_tests.cpp
        src/source_runtime_memory.cpp src/source_runtime_image.cpp src/source_runtime_host_virtual_memory.cpp)
    target_link_libraries(source_process_heap_runtime_tests PRIVATE halo2_snapshot_storage)
    target_link_libraries(source_process_heap_runtime_tests PRIVATE halo2_source_process_heap_platform nlohmann_json::nlohmann_json)
    if(MSVC)
        target_compile_options(source_process_heap_runtime_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_process_heap_runtime_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
    add_test(NAME original_source_process_heap_runtime COMMAND source_process_heap_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/process-heap-measured/fixture.json
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(original_source_process_heap_runtime PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_source_process_heap_initializer COMMAND source_process_heap_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/process-heap-initializer-measured/fixture.json
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(original_source_process_heap_initializer PROPERTIES SKIP_RETURN_CODE 77)
endif()
