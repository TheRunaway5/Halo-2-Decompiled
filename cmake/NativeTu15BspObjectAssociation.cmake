# Sole integer2190C0/219150/2191D0/2190F0/BCEE0 semantic owner.
add_library(halo2_native_bsp_object_association STATIC
    "${CMAKE_CURRENT_LIST_DIR}/../src/physics/native_tu15_bsp_object_association.cpp")
target_include_directories(halo2_native_bsp_object_association PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../include")
target_compile_features(halo2_native_bsp_object_association PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(halo2_native_bsp_object_association PRIVATE /fp:strict)
else()
    target_compile_options(halo2_native_bsp_object_association PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
endif()
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_native_bsp_object_association PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_native_bsp_object_association PUBLIC -fsanitize=address,undefined)
endif()
target_link_libraries(halo2_systems PUBLIC halo2_native_bsp_object_association)
