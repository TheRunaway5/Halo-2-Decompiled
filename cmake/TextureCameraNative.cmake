# Native texture-camera production is source-owned; active GPU comparison
# requires its separately retained final-source offline reference reports.
if(BUILD_TESTING)
    add_executable(native_water_reflection_projection_tests EXCLUDE_FROM_ALL
        tests/native_water_reflection_projection_tests.cpp)
    target_link_libraries(native_water_reflection_projection_tests PRIVATE
        halo2_cache halo2_vertex_program_reference nlohmann_json::nlohmann_json)
    if(MSVC)
        target_compile_options(native_water_reflection_projection_tests PRIVATE /fp:strict)
    else()
        target_compile_options(native_water_reflection_projection_tests PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    add_test(NAME native_water_reflection_projection_tu15
        COMMAND native_water_reflection_projection_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_campaign_character_vertex.tsv"
        "@HALO2_TEST_SCRATCH@/reflection-projection.json")
    add_test(NAME source_water_reflection_projection_fresh
        COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_source_water_reflection_projection.py" --check)
    add_executable(native_texture_camera_active_gpu_tests EXCLUDE_FROM_ALL
        tests/texture_camera/native_texture_camera_active_gpu.cpp)
    target_link_libraries(native_texture_camera_active_gpu_tests PRIVATE
        halo2_renderer nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_dependencies(native_texture_camera_active_gpu_tests halo2_shaders)
    if(MSVC)
        target_compile_options(native_texture_camera_active_gpu_tests PRIVATE /fp:strict)
    else()
        target_compile_options(native_texture_camera_active_gpu_tests PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    set(HALO2_TEXTURE_CAMERA_PREPARED_MAPS "" CACHE PATH "Actual prepared MCC maps for finite active-camera GPU comparison")
    set(HALO2_TEXTURE_CAMERA_PREPARED_ROOT "" CACHE PATH "Authenticated prepared owner root")
    set(HALO2_TEXTURE_CAMERA_ORIGINAL_PRODUCER "" CACHE FILEPATH "Original06a source input authority")
    set(HALO2_TEXTURE_CAMERA_REFERENCE_ROOT "" CACHE PATH "Final-source offline camera reference reports, by consumer map stem")
    if(HALO2_TEXTURE_CAMERA_REFERENCE_ROOT)
        foreach(required HALO2_TEXTURE_CAMERA_PREPARED_MAPS HALO2_TEXTURE_CAMERA_PREPARED_ROOT HALO2_TEXTURE_CAMERA_ORIGINAL_PRODUCER)
            if(NOT EXISTS "${${required}}")
                message(FATAL_ERROR "Active camera acceptance requires existing ${required}")
            endif()
        endforeach()
        foreach(consumer 06a_sentinelwalls 07b_forerunnership)
            if(NOT EXISTS "${HALO2_TEXTURE_CAMERA_REFERENCE_ROOT}/${consumer}/report.json")
                message(FATAL_ERROR "Missing final-source actual camera reference ${consumer}")
            endif()
            add_test(NAME native_texture_camera_active_${consumer}
                COMMAND native_texture_camera_active_gpu_tests
                --producer "${HALO2_TEXTURE_CAMERA_PREPARED_MAPS}/06a_sentinelwalls.map"
                --consumer "${HALO2_TEXTURE_CAMERA_PREPARED_MAPS}/${consumer}.map"
                --original-producer "${HALO2_TEXTURE_CAMERA_ORIGINAL_PRODUCER}"
                --shared-dir "${HALO2_TEXTURE_CAMERA_PREPARED_MAPS}"
                --prepared-root "${HALO2_TEXTURE_CAMERA_PREPARED_ROOT}"
                --shaders "${CMAKE_CURRENT_BINARY_DIR}/shaders"
                --compare-reference "${HALO2_TEXTURE_CAMERA_REFERENCE_ROOT}/${consumer}/report.json"
                --output "@HALO2_TEST_SCRATCH@/native-camera-${consumer}")
            set_tests_properties(native_texture_camera_active_${consumer} PROPERTIES
                RUN_SERIAL TRUE TIMEOUT 900 ENVIRONMENT "HALO2_CAPTURE_BSP_DRAW_BINDINGS=1")
        endforeach()
    endif()
endif()
