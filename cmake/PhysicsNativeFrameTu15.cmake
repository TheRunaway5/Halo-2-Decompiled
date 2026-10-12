# The public frame entry uses the existing same-bank TU1.5 world owner.
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(physics_native_frame_tu15_tests tests/physics_native_frame_tu15_tests.cpp)
    target_link_libraries(physics_native_frame_tu15_tests PRIVATE halo2_systems)
    target_compile_options(physics_native_frame_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    foreach(map IN ITEMS lockout coagulation)
        add_test(NAME native_physics_frame_tu15_${map} COMMAND physics_native_frame_tu15_tests
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/${map}.map")
        set_tests_properties(native_physics_frame_tu15_${map} PROPERTIES TIMEOUT 120)
    endforeach()
endif()
