if(NOT TARGET halo2_native_tu15_data_array_delete)
    add_library(halo2_native_tu15_data_array_delete STATIC
        "${CMAKE_CURRENT_LIST_DIR}/../src/core/native_tu15_data_array_delete.cpp")
    target_include_directories(halo2_native_tu15_data_array_delete PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/../include")
    target_compile_features(halo2_native_tu15_data_array_delete PUBLIC cxx_std_20)
    set_target_properties(halo2_native_tu15_data_array_delete PROPERTIES
        POSITION_INDEPENDENT_CODE ON)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(halo2_native_tu15_data_array_delete PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(halo2_native_tu15_data_array_delete INTERFACE
            -fsanitize=address,undefined)
    endif()
endif()
