find_program(SPIRV_DISASSEMBLER spirv-dis REQUIRED)
set(model_skin_shader "${CMAKE_CURRENT_BINARY_DIR}/shaders/model_skin.comp.spv")
add_custom_command(OUTPUT "${model_skin_shader}" "${CMAKE_CURRENT_BINARY_DIR}/model-skin-spirv.json"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.2
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/model_skin.comp" -o "${model_skin_shader}"
    COMMAND ${SPIRV_VALIDATOR} --target-env vulkan1.2 "${model_skin_shader}"
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_model_skin_spirv.py"
        --disassembler "${SPIRV_DISASSEMBLER}" --shader "${model_skin_shader}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/model-skin-spirv.json"
    DEPENDS shaders/model_skin.comp tools/verify_model_skin_spirv.py VERBATIM)
add_custom_target(model_skin_gpu_shader DEPENDS "${model_skin_shader}" "${CMAKE_CURRENT_BINARY_DIR}/model-skin-spirv.json")
target_sources(halo2_renderer PRIVATE src/rendering/models/model_skin_gpu.cpp)
add_dependencies(halo2_renderer model_skin_gpu_shader)
if(BUILD_TESTING)
    add_executable(model_skin_gpu_tests tests/model_skin_gpu_tests.cpp src/rendering/models/model_skin_gpu.cpp)
    target_include_directories(model_skin_gpu_tests PRIVATE include src)
    target_compile_features(model_skin_gpu_tests PRIVATE cxx_std_20)
    if(NOT MSVC)
        target_compile_options(model_skin_gpu_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
    target_link_libraries(model_skin_gpu_tests PRIVATE Vulkan::Vulkan)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(model_skin_gpu_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(model_skin_gpu_tests PRIVATE -fsanitize=address,undefined)
    endif()
    target_compile_definitions(model_skin_gpu_tests PRIVATE HALO2_MODEL_SKIN_GPU_SHADER="${model_skin_shader}"
        HALO2_MODEL_SKIN_GPU_REPORT="${CMAKE_CURRENT_BINARY_DIR}/model-skin-gpu.json")
    add_dependencies(model_skin_gpu_tests model_skin_gpu_shader)
    add_test(NAME native_model_skin_gpu COMMAND model_skin_gpu_tests)
    set_tests_properties(native_model_skin_gpu PROPERTIES TIMEOUT 60)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
        set_tests_properties(native_model_skin_gpu PROPERTIES ENVIRONMENT_MODIFICATION
            "VK_LAYER_PATH=set:${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=path_list_prepend:${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib")
    endif()
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT MSVC AND
   CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[xX]64)$")
    # The test supplies Vulkan entry points itself; linking the loader would
    # invalidate its deterministic constructor-failure controls.
    add_executable(model_skin_gpu_initialization_tests EXCLUDE_FROM_ALL
        tests/model_skin_gpu_initialization_tests.cpp
        src/rendering/models/model_skin_gpu.cpp)
    target_include_directories(model_skin_gpu_initialization_tests PRIVATE
        include src ${Vulkan_INCLUDE_DIRS})
    target_compile_features(model_skin_gpu_initialization_tests PRIVATE cxx_std_20)
    target_compile_options(model_skin_gpu_initialization_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    if(HALO2_SANITIZERS)
        target_compile_options(model_skin_gpu_initialization_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(model_skin_gpu_initialization_tests PRIVATE
            -fsanitize=address,undefined)
    endif()
    add_test(NAME native_model_skin_gpu_initialization COMMAND model_skin_gpu_initialization_tests)
    set_tests_properties(native_model_skin_gpu_initialization PROPERTIES TIMEOUT 60)
endif()
if(BUILD_TESTING AND NOT MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[xX]64)$")
    add_executable(model_skin_handoff_tests tests/model_skin_handoff_tests.cpp)
    target_include_directories(model_skin_handoff_tests PRIVATE include src)
    target_compile_options(model_skin_handoff_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(model_skin_handoff_tests PRIVATE halo2_systems)
    add_test(NAME native_model_skin_handoff COMMAND model_skin_handoff_tests)
    set_tests_properties(native_model_skin_handoff PROPERTIES TIMEOUT 60)
    set(HALO2_MODEL_SKIN_HANDOFF_MCC_MAP "" CACHE FILEPATH "Explicit prepared MCC map for deferred skin owner/selection tests")
    if(NOT "${HALO2_MODEL_SKIN_HANDOFF_MCC_MAP}" STREQUAL "")
        if(NOT IS_ABSOLUTE "${HALO2_MODEL_SKIN_HANDOFF_MCC_MAP}" OR
            NOT EXISTS "${HALO2_MODEL_SKIN_HANDOFF_MCC_MAP}" OR IS_DIRECTORY "${HALO2_MODEL_SKIN_HANDOFF_MCC_MAP}")
            message(FATAL_ERROR "HALO2_MODEL_SKIN_HANDOFF_MCC_MAP requires an existing absolute prepared MCC map")
        endif()
        add_test(NAME native_mcc_model_skin_handoff COMMAND model_skin_handoff_tests "${HALO2_MODEL_SKIN_HANDOFF_MCC_MAP}")
        set_tests_properties(native_mcc_model_skin_handoff PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE
            LABELS "genuine-mcc-input;selected-mcc-native;gpu-skin-handoff-component")
    endif()
endif()
