# Offline Xbox source controls only. Production loading assets must come from
# admitted MCC/Mod Kit onboarding; the DVD provider is never linked into Cache.
target_sources(halo2_cache PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src/ui/source_loading_assets.cpp)
target_sources(halo2_renderer PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src/platform/media/native_source_loading_wait.cpp)
if(BUILD_TESTING)
  add_executable(source_loading_wait_gpu_tests ${CMAKE_CURRENT_LIST_DIR}/../tests/source_loading_wait_gpu_tests.cpp)
  target_link_libraries(source_loading_wait_gpu_tests PRIVATE halo2_renderer)
  add_dependencies(source_loading_wait_gpu_tests halo2_shaders)
  add_executable(original_loading_gpu_tests ${CMAKE_CURRENT_LIST_DIR}/../tests/original_loading_gpu_tests.cpp)
  target_link_libraries(original_loading_gpu_tests PRIVATE halo2_renderer)
  add_dependencies(original_loading_gpu_tests halo2_shaders)
  # Imported MCC cache whose setup staged the H2EK original loading bank.
  set(HALO2_ORIGINAL_LOADING_CACHE "" CACHE PATH "Imported MCC cache with presentation/original-loading-v2")
  if(HALO2_ORIGINAL_LOADING_CACHE)
    add_test(NAME original_loading_gpu COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_LIST_DIR}/../tools/run_test.py -- $<TARGET_FILE:original_loading_gpu_tests>
      "${HALO2_ORIGINAL_LOADING_CACHE}" @HALO2_TEST_SCRATCH@/original-loading)
    set_tests_properties(original_loading_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()
  add_executable(original_loading_tests
    ${CMAKE_CURRENT_LIST_DIR}/../tests/original_loading_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/ui/original_loading_assets.cpp)
  target_include_directories(original_loading_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src)
  target_link_libraries(original_loading_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json OpenSSL::Crypto ZLIB::ZLIB)
  add_test(NAME original_loading_source COMMAND original_loading_tests
    ${CMAKE_CURRENT_LIST_DIR}/../assets/disc/maps/mainmenu.map
    ${CMAKE_CURRENT_LIST_DIR}/../tests/fixtures/tu15_loading_progress_original.json)
  set_tests_properties(original_loading_source PROPERTIES SKIP_RETURN_CODE 77)
  set(HALO2_ORIGINAL_LOADING_MCC_MENU "" CACHE FILEPATH "Admitted MCC mainmenu for full original loading page/mip identity")
  if(HALO2_ORIGINAL_LOADING_MCC_MENU)
    add_test(NAME original_loading_mcc_source COMMAND original_loading_tests
      ${CMAKE_CURRENT_LIST_DIR}/../assets/disc/maps/mainmenu.map
      ${CMAKE_CURRENT_LIST_DIR}/../tests/fixtures/tu15_loading_progress_original.json
      "${HALO2_ORIGINAL_LOADING_MCC_MENU}")
    add_test(NAME native_source_loading_wait_gpu COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_LIST_DIR}/../tools/run_test.py -- $<TARGET_FILE:source_loading_wait_gpu_tests>
      "${HALO2_ORIGINAL_LOADING_MCC_MENU}" @HALO2_TEST_SCRATCH@/source-loading-wait)
    set_tests_properties(native_source_loading_wait_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 90)
  endif()
endif()
