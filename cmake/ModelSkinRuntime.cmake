if(BUILD_TESTING)
    add_test(NAME model_skin_runtime_extent_driver COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_model_skin_runtime_extent_driver.py")
    set_tests_properties(model_skin_runtime_extent_driver PROPERTIES TIMEOUT 30 LABELS "gpu-skin-runtime-driver-contract")
    add_executable(model_skin_runtime_tests EXCLUDE_FROM_ALL tests/model_skin_runtime_tests.cpp
        src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(model_skin_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm
        OpenSSL::Crypto nlohmann_json::nlohmann_json)
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
            LABELS "genuine-mcc-input;selected-mcc-native;gpu-skin-ordinary-runtime")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
            set_tests_properties(native_mcc_model_skin_runtime PROPERTIES ENVIRONMENT_MODIFICATION
                "VK_LAYER_PATH=set:${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=path_list_prepend:${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib")
        endif()
    endif()
endif()
