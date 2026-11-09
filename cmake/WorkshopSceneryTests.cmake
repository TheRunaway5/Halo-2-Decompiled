add_executable(native_mcc_scenery_animation_world_tests tests/native_mcc_scenery_animation_world_tests.cpp)
target_link_libraries(native_mcc_scenery_animation_world_tests PRIVATE halo2_systems)
add_test(NAME native_mcc_scenery_animation_bootstrap COMMAND native_mcc_scenery_animation_world_tests
    "${HALO2_WORKSHOP_TEST_CACHE}/halo2/h2_maps_win64_dx11/00a_introduction.map")
set_tests_properties(native_mcc_scenery_animation_bootstrap PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
add_test(NAME native_mcc_scenery_animation_hs COMMAND native_mcc_scenery_animation_world_tests
    "${HALO2_WORKSHOP_TEST_CACHE}/halo2/h2_maps_win64_dx11/00a_introduction.map" --hs)
set_tests_properties(native_mcc_scenery_animation_hs PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
add_test(NAME native_mcc_scenery_animation_named_hs COMMAND native_mcc_scenery_animation_world_tests
    "${HALO2_WORKSHOP_TEST_CACHE}/halo2/h2_maps_win64_dx11/00a_introduction.map" --named-hs)
set_tests_properties(native_mcc_scenery_animation_named_hs PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
add_test(NAME native_mcc_scenery_animation_lifecycle COMMAND native_mcc_scenery_animation_world_tests
    "${HALO2_WORKSHOP_TEST_CACHE}/halo2/h2_maps_win64_dx11/00a_introduction.map" --exercise)
set_tests_properties(native_mcc_scenery_animation_lifecycle PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
add_test(NAME native_mcc_scenery_animation_render_consumer COMMAND native_mcc_scenery_animation_world_tests
    "${HALO2_WORKSHOP_TEST_CACHE}/halo2/h2_maps_win64_dx11/00a_introduction.map" --render)
set_tests_properties(native_mcc_scenery_animation_render_consumer PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
add_test(NAME native_mcc_scenery_animation_relative_consumer COMMAND native_mcc_scenery_animation_world_tests
    "${HALO2_WORKSHOP_TEST_CACHE}/halo2/h2_maps_win64_dx11/00a_introduction.map" --relative-render)
set_tests_properties(native_mcc_scenery_animation_relative_consumer PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
add_test(NAME native_mcc_scenery_animation_inactive_control COMMAND native_mcc_scenery_animation_world_tests
    "${HALO2_WORKSHOP_TEST_CACHE}/halo2/h2_maps_win64_dx11/00a_introduction.map" --inactive)
set_tests_properties(native_mcc_scenery_animation_inactive_control PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)

add_executable(native_mcc_scenery_animation_gpu_tests tests/native_mcc_scenery_animation_world_tests.cpp
    tests/native_mcc_scenery_animation_gpu.cpp)
target_link_libraries(native_mcc_scenery_animation_gpu_tests PRIVATE halo2_renderer)
target_compile_definitions(native_mcc_scenery_animation_gpu_tests PRIVATE HALO2_SCENERY_GPU=1
    HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
find_program(HALO2_SCENERY_XVFB_RUN xvfb-run)
if(HALO2_SCENERY_XVFB_RUN)
    add_test(NAME native_mcc_scenery_animation_gpu COMMAND ${HALO2_SCENERY_XVFB_RUN} -a
        $<TARGET_FILE:native_mcc_scenery_animation_gpu_tests>
        "${HALO2_WORKSHOP_TEST_CACHE}/halo2/h2_maps_win64_dx11/00a_introduction.map" --gpu)
    add_test(NAME native_mcc_scenery_animation_relative_gpu COMMAND ${HALO2_SCENERY_XVFB_RUN} -a
        $<TARGET_FILE:native_mcc_scenery_animation_gpu_tests>
        "${HALO2_WORKSHOP_TEST_CACHE}/halo2/h2_maps_win64_dx11/00a_introduction.map" --gpu-relative)
    set_tests_properties(native_mcc_scenery_animation_gpu native_mcc_scenery_animation_relative_gpu
        PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180 ENVIRONMENT "SDL_VIDEODRIVER=x11")
endif()

# Complete object-name lookup; the actual World caller regression is above.
add_executable(object_name_lookup_tu15_tests tests/object_name_lookup_tu15_tests.cpp)
target_compile_features(object_name_lookup_tu15_tests PRIVATE cxx_std_20)
target_link_libraries(object_name_lookup_tu15_tests PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
add_test(NAME original_object_name_lookup_tu15 COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/workshop_unit_controls_tests.py
    --fixture ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/object_name_lookup_tu15.json.zlib
    --test-executable $<TARGET_FILE:object_name_lookup_tu15_tests>)

# Wrapper-only regression; actual MCC effect and mission gates stay separate.
add_executable(workshop_scenery_start_tests tests/workshop_scenery_start_tests.cpp)
target_compile_features(workshop_scenery_start_tests PRIVATE cxx_std_20)
target_link_libraries(workshop_scenery_start_tests PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
add_test(NAME workshop_scenery_start COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/workshop_unit_controls_tests.py
    --fixture ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/workshop_scenery_start_tu15.json.zlib
    --test-executable $<TARGET_FILE:workshop_scenery_start_tests>)

# Independently qualified wrapper only; deliberately separate from World routing.
add_executable(workshop_camera_start_tests tests/workshop_camera_start_tests.cpp
    src/campaign/scripting/source_workshop_camera_start_tu15_generated.cpp)
target_compile_features(workshop_camera_start_tests PRIVATE cxx_std_20)
target_include_directories(workshop_camera_start_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(workshop_camera_start_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME original_workshop_camera_start COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/workshop_unit_controls_tests.py
    --fixture ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/workshop_camera_start_tu15.json.zlib
    --test-executable $<TARGET_FILE:workshop_camera_start_tests>)

add_executable(workshop_empty_prop_content_tests tests/workshop_empty_prop_content_tests.cpp)
target_link_libraries(workshop_empty_prop_content_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
add_test(NAME workshop_empty_prop_content COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_workshop_prop_content.py
    --test-executable $<TARGET_FILE:workshop_empty_prop_content_tests>
    --base-cache "${HALO2_WORKSHOP_TEST_CACHE}"
    --output "${CMAKE_CURRENT_BINARY_DIR}/workshop-empty-prop-content.json")
set_tests_properties(workshop_empty_prop_content PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)

add_executable(workshop_navigation_content_tests tests/workshop_navigation_content_tests.cpp)
target_link_libraries(workshop_navigation_content_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
add_test(NAME workshop_navigation_content COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_workshop_navigation_content.py
    --test-executable $<TARGET_FILE:workshop_navigation_content_tests>
    --base-cache "${HALO2_WORKSHOP_TEST_CACHE}"
    --output "${CMAKE_CURRENT_BINARY_DIR}/workshop-navigation-content.json")
set_tests_properties(workshop_navigation_content PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)

# Native wrapper/dispatch proof; MCC camera admission needs actual consumers.
add_executable(workshop_camera_shared_tests tests/workshop_camera_start_tests.cpp)
target_compile_features(workshop_camera_shared_tests PRIVATE cxx_std_20)
target_compile_definitions(workshop_camera_shared_tests PRIVATE HALO2_CAMERA_SHARED_MODULE=1)
target_link_libraries(workshop_camera_shared_tests PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
add_test(NAME original_workshop_camera_shared COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/workshop_unit_controls_tests.py
    --fixture ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/workshop_camera_start_tu15.json.zlib
    --test-executable $<TARGET_FILE:workshop_camera_shared_tests>)

# Complete original looping-prediction/cache effects. Actual native HS and
# MCC streaming consumers must pass before the script declaration is admitted.
add_executable(workshop_looping_prediction_tests
    tests/workshop_looping_prediction_tests.cpp
    src/campaign/scripting/source_workshop_script_builtins_tu15_generated.cpp
    src/audio/native_tu15_sound_kernels_resources.cpp
    src/core/native_tu15_data_array_allocate_specific.cpp
    src/core/native_tu15_data_array_allocate.cpp
    src/core/native_tu15_data_array_delete.cpp
    src/core/native_tu15_page_allocator.cpp)
target_include_directories(workshop_looping_prediction_tests PRIVATE include src)
target_compile_features(workshop_looping_prediction_tests PRIVATE cxx_std_20)
target_link_libraries(workshop_looping_prediction_tests PRIVATE nlohmann_json::nlohmann_json)
if(NOT MSVC)
    target_compile_options(workshop_looping_prediction_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
endif()
add_test(NAME original_workshop_looping_prediction COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/workshop_unit_controls_tests.py
    --fixture ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/workshop_looping_prediction.json.zlib
    --test-executable $<TARGET_FILE:workshop_looping_prediction_tests>)
set_tests_properties(original_workshop_looping_prediction PROPERTIES TIMEOUT 60)

# Closed raw resource storage does not extend the CPU pixel-sampling domain.
add_executable(workshop_prediction_raw_bitmap_tests
    tests/workshop_prediction_raw_bitmap_tests.cpp
    src/rendering/mcc_cpu_bitmap_owner.cpp
    src/rendering/effects/mcc_lens_bitmap_layout.cpp)
target_include_directories(workshop_prediction_raw_bitmap_tests PRIVATE include src)
target_compile_features(workshop_prediction_raw_bitmap_tests PRIVATE cxx_std_20)
target_link_libraries(workshop_prediction_raw_bitmap_tests PRIVATE halo2_cache)
if(NOT MSVC)
    target_compile_options(workshop_prediction_raw_bitmap_tests PRIVATE
        -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_options(workshop_prediction_raw_bitmap_tests PRIVATE -Wl,--gc-sections)
endif()
add_test(NAME workshop_prediction_raw_bitmap_storage COMMAND workshop_prediction_raw_bitmap_tests)
set_tests_properties(workshop_prediction_raw_bitmap_storage PROPERTIES TIMEOUT 60)

# Actual installed sound metadata and retained encoded resource owners. This
# Cache-only test does not admit the HS builtin or replace native cache I/O.
add_executable(workshop_prediction_delivery_tests tests/native_workshop_prediction_delivery_tests.cpp)
target_compile_features(workshop_prediction_delivery_tests PRIVATE cxx_std_20)
target_link_libraries(workshop_prediction_delivery_tests PRIVATE halo2_cache)
add_test(NAME workshop_prediction_sound_resources COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_workshop_prediction_content.py
    --test-executable $<TARGET_FILE:workshop_prediction_delivery_tests>
    --base-cache "${HALO2_WORKSHOP_TEST_CACHE}"
    --output "${CMAKE_CURRENT_BINARY_DIR}/workshop-prediction-sound-resources.json")
set_tests_properties(workshop_prediction_sound_resources PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)

# Complete native prediction/allocation/read over the actual two E3 music
# tracks, followed by resident replay and real resource-identity refusals.
add_executable(workshop_prediction_cold_tests
    tests/native_workshop_prediction_delivery_tests.cpp
    src/campaign/scripting/source_workshop_script_builtins_tu15_generated.cpp
    src/audio/native_tu15_sound_kernels_resources.cpp
    src/audio/source_sound_cache_io_tu15.cpp
    src/core/native_tu15_data_array_allocate_specific.cpp
    src/core/native_tu15_data_array_allocate.cpp
    src/core/native_tu15_data_array_delete.cpp
    src/core/native_tu15_page_allocator.cpp)
target_include_directories(workshop_prediction_cold_tests PRIVATE include src tests)
target_compile_features(workshop_prediction_cold_tests PRIVATE cxx_std_20)
target_compile_definitions(workshop_prediction_cold_tests PRIVATE HALO2_WORKSHOP_PREDICTION_COLD_CACHE=1)
target_link_libraries(workshop_prediction_cold_tests PRIVATE halo2_cache)
if(NOT MSVC)
    target_compile_options(workshop_prediction_cold_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
endif()
add_test(NAME native_workshop_prediction_cold_resources COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_workshop_prediction_content.py
    --test-executable $<TARGET_FILE:workshop_prediction_cold_tests>
    --base-cache "${HALO2_WORKSHOP_TEST_CACHE}"
    --output "${CMAKE_CURRENT_BINARY_DIR}/workshop-prediction-cold-resources.json")
set_tests_properties(native_workshop_prediction_cold_resources PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
