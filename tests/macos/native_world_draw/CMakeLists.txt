# Use the actual root cache, packet, upload, draw, fragment and scene-copy owners.
# The original map is a required offline reference supplied inside the remote
# test owner; it is neither installed content nor a launcher runtime.
set(HALO2_MACOS_NATIVE_WORLD_SOURCE "" CACHE FILEPATH "Owned original opening-map reference for native Mac fragment tests")
if(NOT HALO2_MACOS_NATIVE_WORLD_SOURCE)
    return()
endif()
foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
    add_test(NAME macos_native_world_draw_${arch} COMMAND /usr/bin/arch -${arch}
        $<TARGET_FILE:hologram_draw_gpu_tests> "${HALO2_MACOS_NATIVE_WORLD_SOURCE}"
        "${CMAKE_BINARY_DIR}/shaders" "${CMAKE_BINARY_DIR}/macos-native-world-${arch}.json" --triangle-carrier)
    set_tests_properties(macos_native_world_draw_${arch} PROPERTIES RUN_SERIAL TRUE TIMEOUT 180)
endforeach()
