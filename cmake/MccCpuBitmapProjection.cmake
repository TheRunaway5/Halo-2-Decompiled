# Real MCC CPU image owners used by retained TU1.5 model/lighting bodies.
# These CPU aliases imply no Vulkan/Xbox GPU residency or substituted shader.
target_sources(halo2_systems PRIVATE
    src/mcc_cpu_bitmap_owner.cpp src/mcc_cpu_bitmap_backing.cpp
    src/mcc_cpu_bitmap_projection.cpp src/source_model_inline_cpu_bitmap_tu15.cpp)
target_link_libraries(halo2_systems PRIVATE halo2_mcc_lens_bitmap_layout)
# Reuse the authenticated small startup CRT bodies; no generated owner rewrite.
target_sources(halo2_physics_world_runtime PRIVATE
    src/source_environment_crt_pair_tu15.cpp src/source_ratio_threshold_crt_tu15.cpp)
if(MSVC)
    set_source_files_properties(src/mcc_cpu_bitmap_owner.cpp src/mcc_cpu_bitmap_backing.cpp
        src/mcc_cpu_bitmap_projection.cpp src/source_model_inline_cpu_bitmap_tu15.cpp
        src/physics_live_world_tu15.cpp src/source_environment_crt_pair_tu15.cpp
        src/source_ratio_threshold_crt_tu15.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/mcc_cpu_bitmap_owner.cpp src/mcc_cpu_bitmap_backing.cpp
        src/mcc_cpu_bitmap_projection.cpp src/source_model_inline_cpu_bitmap_tu15.cpp
        src/physics_live_world_tu15.cpp src/source_environment_crt_pair_tu15.cpp
        src/source_ratio_threshold_crt_tu15.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(HALO2_MCC_CPU_BITMAP_DONOR "" CACHE FILEPATH "Optional owned MCC Lockout map for CPU bitmap ownership/lifecycle checks")
    add_executable(mcc_cpu_bitmap_lifecycle_tests tests/mcc_cpu_bitmap_lifecycle_tests.cpp)
    target_link_libraries(mcc_cpu_bitmap_lifecycle_tests PRIVATE halo2_systems)
    add_test(NAME mcc_cpu_bitmap_world_lifecycle COMMAND mcc_cpu_bitmap_lifecycle_tests
        "${HALO2_MCC_CPU_BITMAP_DONOR}" "@HALO2_TEST_SCRATCH@/mcc-cpu-bitmap-world")
    set_tests_properties(mcc_cpu_bitmap_world_lifecycle PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_executable(mcc_cpu_bitmap_backing_tests tests/mcc_cpu_bitmap_backing_tests.cpp)
    target_include_directories(mcc_cpu_bitmap_backing_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(mcc_cpu_bitmap_backing_tests PRIVATE halo2_systems)
    add_test(NAME mcc_cpu_bitmap_backing_recovery COMMAND mcc_cpu_bitmap_backing_tests
        "${HALO2_MCC_CPU_BITMAP_DONOR}")
    set_tests_properties(mcc_cpu_bitmap_backing_recovery PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()

if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(mcc_opening_cpu_bitmap_tests tests/mcc_opening_cpu_bitmap_tests.cpp)
    target_link_libraries(mcc_opening_cpu_bitmap_tests PRIVATE halo2_systems)
    add_test(NAME mcc_opening_cpu_bitmap COMMAND mcc_opening_cpu_bitmap_tests "${HALO2_MCC_OPENING_MODEL_DONOR}")
    set_tests_properties(mcc_opening_cpu_bitmap PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
