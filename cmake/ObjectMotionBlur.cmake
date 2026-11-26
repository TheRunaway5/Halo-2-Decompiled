foreach(stage IN ITEMS vert frag)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/object_motion_blur.${stage}.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V ${PROJECT_SOURCE_DIR}/shaders/object_motion_blur.${stage}
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/object_motion_blur.${stage}.spv
        DEPENDS shaders/object_motion_blur.${stage} VERBATIM)
endforeach()
add_custom_target(object_motion_blur_shaders DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/object_motion_blur.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/object_motion_blur.frag.spv)
add_dependencies(halo2_shaders object_motion_blur_shaders)

add_library(halo2_object_motion_blur STATIC src/rendering/renderer/object_motion_blur_pass.cpp)
target_include_directories(halo2_object_motion_blur PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(halo2_object_motion_blur PUBLIC Vulkan::Vulkan halo2_glm)
add_dependencies(halo2_object_motion_blur object_motion_blur_shaders)

if(BUILD_TESTING)
    add_executable(object_motion_blur_history_tests tests/object_motion_blur_history_tests.cpp)
    target_include_directories(object_motion_blur_history_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(object_motion_blur_history_tests PRIVATE halo2_object_motion_blur halo2_path_tracing)
    add_test(NAME object_motion_blur_history COMMAND object_motion_blur_history_tests)

    add_executable(object_motion_blur_gpu_tests tests/object_motion_blur_gpu_tests.cpp)
    target_include_directories(object_motion_blur_gpu_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(object_motion_blur_gpu_tests PRIVATE halo2_object_motion_blur)
    add_test(NAME object_motion_blur_gpu COMMAND object_motion_blur_gpu_tests ${CMAKE_CURRENT_BINARY_DIR}/shaders)
    set_tests_properties(object_motion_blur_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
endif()
