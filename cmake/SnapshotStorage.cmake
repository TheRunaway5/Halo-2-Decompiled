# Independent complete byte snapshots shared by cache and source-memory owners.
# Keep the disk backend small so standalone memory tests need no game assets.
add_library(halo2_snapshot_storage STATIC src/cache_file_snapshot.cpp src/source_runtime_snapshot_bytes.cpp)
target_include_directories(halo2_snapshot_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_snapshot_storage PUBLIC cxx_std_20)
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_snapshot_storage PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_snapshot_storage PUBLIC -fsanitize=address,undefined)
endif()
