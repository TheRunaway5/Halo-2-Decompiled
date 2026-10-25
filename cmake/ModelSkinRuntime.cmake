if(BUILD_TESTING)
    add_test(NAME model_skin_runtime_extent_driver COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_model_skin_runtime_extent_driver.py")
    set_tests_properties(model_skin_runtime_extent_driver PROPERTIES TIMEOUT 30 LABELS "gpu-skin-runtime-driver-contract")
    # The passive diagnostic sink currently qualifies its host-FP preservation
    # only on Linux x86-64. Keep the portable Python extent contract above.
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT MSVC AND
       CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[xX]64)$")
    add_executable(model_skin_runtime_tests EXCLUDE_FROM_ALL tests/model_skin_runtime_tests.cpp
        src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(model_skin_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm
        OpenSSL::Crypto nlohmann_json::nlohmann_json)
    target_compile_definitions(model_skin_runtime_tests PRIVATE
        HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    if(TARGET source_contrail_shaders)
        target_compile_definitions(model_skin_runtime_tests PRIVATE HALO2_SOURCE_CONTRAILS_AVAILABLE=1)
        add_dependencies(model_skin_runtime_tests source_contrail_shaders)
    endif()
    if(TARGET source_decal_shaders)
        target_compile_definitions(model_skin_runtime_tests PRIVATE HALO2_SOURCE_DECALS_AVAILABLE=1)
        add_dependencies(model_skin_runtime_tests source_decal_shaders)
    endif()
    set(HALO2_MODEL_SKIN_RUNTIME_MCC_MAP "" CACHE FILEPATH "Explicit prepared MCC Coagulation map for ordinary GPU skin regression")
    set(HALO2_MODEL_SKIN_RUNTIME_COHORT "" CACHE STRING "Optional runtime case-name substring; -fp- selects ordinary first-person cohorts")
    set(HALO2_MODEL_SKIN_RUNTIME_WIDTH "640" CACHE STRING "Requested ordinary skin runtime window/render/capture width,64..4096")
    set(HALO2_MODEL_SKIN_RUNTIME_HEIGHT "480" CACHE STRING "Requested ordinary skin runtime window/render/capture height,64..4096")
    foreach(_halo2_skin_dimension IN ITEMS HALO2_MODEL_SKIN_RUNTIME_WIDTH HALO2_MODEL_SKIN_RUNTIME_HEIGHT)
        if(NOT "${${_halo2_skin_dimension}}" MATCHES "^[0-9]+$" OR
            "${${_halo2_skin_dimension}}" LESS 64 OR "${${_halo2_skin_dimension}}" GREATER 4096)
            message(FATAL_ERROR "${_halo2_skin_dimension} must be an integer in64..4096")
        endif()
    endforeach()
    if(NOT "${HALO2_MODEL_SKIN_RUNTIME_MCC_MAP}" STREQUAL "")
        if(NOT IS_ABSOLUTE "${HALO2_MODEL_SKIN_RUNTIME_MCC_MAP}" OR
            NOT EXISTS "${HALO2_MODEL_SKIN_RUNTIME_MCC_MAP}" OR IS_DIRECTORY "${HALO2_MODEL_SKIN_RUNTIME_MCC_MAP}")
            message(FATAL_ERROR "HALO2_MODEL_SKIN_RUNTIME_MCC_MAP requires an existing absolute prepared MCC map")
        endif()
        add_test(NAME native_mcc_model_skin_runtime COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_model_skin_runtime.py" "$<TARGET_FILE:model_skin_runtime_tests>"
            "${HALO2_MODEL_SKIN_RUNTIME_MCC_MAP}" "@HALO2_TEST_SCRATCH@/ordinary-skin-runtime"
            "${CMAKE_CURRENT_BINARY_DIR}/model-skin-runtime.json" "--cohort=${HALO2_MODEL_SKIN_RUNTIME_COHORT}"
            "--width=${HALO2_MODEL_SKIN_RUNTIME_WIDTH}" "--height=${HALO2_MODEL_SKIN_RUNTIME_HEIGHT}")
        set_tests_properties(native_mcc_model_skin_runtime PROPERTIES TIMEOUT 1800 RUN_SERIAL TRUE
            LABELS "genuine-mcc-input;selected-mcc-native;gpu-skin-ordinary-runtime"
            ENVIRONMENT_MODIFICATION "HALO2_MODEL_SKIN_RUNTIME_POST_CONSTRUCTOR_COMMON_CUT=set:0")
        # Explicit control cohort: normal complete source/Match publication
        # after genuine construction. This never grants ordinary GPU acceptance.
        add_test(NAME native_mcc_model_skin_common_cut_runtime COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_model_skin_runtime.py" "$<TARGET_FILE:model_skin_runtime_tests>"
            "${HALO2_MODEL_SKIN_RUNTIME_MCC_MAP}" "@HALO2_TEST_SCRATCH@/common-cut-skin-runtime"
            "${CMAKE_CURRENT_BINARY_DIR}/model-skin-common-cut-runtime.json" "--cohort=${HALO2_MODEL_SKIN_RUNTIME_COHORT}"
            "--width=${HALO2_MODEL_SKIN_RUNTIME_WIDTH}" "--height=${HALO2_MODEL_SKIN_RUNTIME_HEIGHT}"
            --post-constructor-common-cut)
        set_tests_properties(native_mcc_model_skin_common_cut_runtime PROPERTIES TIMEOUT 1800 RUN_SERIAL TRUE
            LABELS "genuine-mcc-input;selected-mcc-native;gpu-skin-post-constructor-control"
            ENVIRONMENT_MODIFICATION "HALO2_MODEL_SKIN_RUNTIME_POST_CONSTRUCTOR_COMMON_CUT=set:1")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
            set_property(TEST native_mcc_model_skin_runtime native_mcc_model_skin_common_cut_runtime APPEND PROPERTY ENVIRONMENT_MODIFICATION
                "VK_LAYER_PATH=set:${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=path_list_prepend:${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib")
        endif()
    endif()
    endif()
endif()

if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT MSVC AND
   CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[xX]64)$")
    add_executable(ordinary_multiplayer_trace_tests EXCLUDE_FROM_ALL
        tests/ordinary_multiplayer_trace_tests.cpp
        src/runtime/source_runtime_memory.cpp
        src/runtime/source_runtime_snapshot_bytes.cpp
        src/assets/cache_file_snapshot.cpp
        src/gameplay/weapons/native_grenade.cpp
        src/gameplay/weapons/unit_grenade.cpp)
    target_include_directories(ordinary_multiplayer_trace_tests PRIVATE include src tests)
    target_compile_features(ordinary_multiplayer_trace_tests PRIVATE cxx_std_20)
    target_compile_options(ordinary_multiplayer_trace_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)
    target_link_options(ordinary_multiplayer_trace_tests PRIVATE -Wl,--gc-sections)
    target_link_libraries(ordinary_multiplayer_trace_tests PRIVATE
        SDL2::SDL2 halo2_glm OpenSSL::Crypto Threads::Threads)
    if(HALO2_SANITIZERS)
        target_compile_options(ordinary_multiplayer_trace_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(ordinary_multiplayer_trace_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME ordinary_multiplayer_trace_component COMMAND ordinary_multiplayer_trace_tests)
    set_tests_properties(ordinary_multiplayer_trace_component PROPERTIES
        TIMEOUT 60 LABELS "gpu-skin-runtime-trace-component")
    add_executable(ordinary_startup_inputs_tests EXCLUDE_FROM_ALL
        tests/ordinary_startup_inputs_tests.cpp)
    target_include_directories(ordinary_startup_inputs_tests PRIVATE include src tests)
    target_compile_features(ordinary_startup_inputs_tests PRIVATE cxx_std_20)
    target_compile_options(ordinary_startup_inputs_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(ordinary_startup_inputs_tests PRIVATE
        halo2_systems OpenSSL::Crypto nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(ordinary_startup_inputs_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(ordinary_startup_inputs_tests PRIVATE
            -fsanitize=address,undefined)
    endif()
    if(NOT "${HALO2_MODEL_SKIN_RUNTIME_MCC_MAP}" STREQUAL "")
        get_filename_component(_halo2_startup_maps "${HALO2_MODEL_SKIN_RUNTIME_MCC_MAP}" DIRECTORY)
        add_test(NAME ordinary_native_startup_inputs COMMAND ordinary_startup_inputs_tests
            "${HALO2_MODEL_SKIN_RUNTIME_MCC_MAP}" "${_halo2_startup_maps}"
            "${CMAKE_CURRENT_BINARY_DIR}/ordinary-startup-inputs.json")
        set_tests_properties(ordinary_native_startup_inputs PROPERTIES
            TIMEOUT 600 RUN_SERIAL TRUE LABELS "genuine-mcc-input;ordinary-startup-inputs")
    endif()
endif()
