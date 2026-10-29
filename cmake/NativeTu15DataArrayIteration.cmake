include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/NativeTu15DataArrayDelete.cmake")
target_sources(halo2_native_tu15_data_array_delete PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../src/core/native_tu15_data_array_iteration.cpp")
