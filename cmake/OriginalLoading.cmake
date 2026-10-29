# Offline Xbox source controls only. Production loading assets must come from
# admitted MCC/Mod Kit onboarding; the DVD provider is never linked into Cache.
target_sources(halo2_cache PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src/ui/source_loading_assets.cpp)
target_sources(halo2_renderer PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src/platform/media/native_source_loading_wait.cpp)
if(BUILD_TESTING)
  add_test(NAME h2ek_loading_onboarding COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_LIST_DIR}/../tests/test_h2ek_loading_ambience.py)
  add_executable(source_loading_wait_gpu_tests ${CMAKE_CURRENT_LIST_DIR}/../tests/source_loading_wait_gpu_tests.cpp)
  target_link_libraries(source_loading_wait_gpu_tests PRIVATE halo2_renderer)
  add_dependencies(source_loading_wait_gpu_tests halo2_shaders)
  add_executable(original_loading_gpu_tests ${CMAKE_CURRENT_LIST_DIR}/../tests/original_loading_gpu_tests.cpp)
  target_link_libraries(original_loading_gpu_tests PRIVATE halo2_renderer)
  target_compile_definitions(original_loading_gpu_tests PRIVATE
    HALO2_TEST_LOADING_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
  add_dependencies(original_loading_gpu_tests halo2_shaders)
  # Imported MCC cache whose setup staged the H2EK original loading bank.
  set(HALO2_ORIGINAL_LOADING_CACHE "" CACHE PATH "Imported MCC cache with presentation/original-loading-v2")
  set(HALO2_LOADING_FLOW_TU15_IMAGE "" CACHE FILEPATH "Read-only TU1.5 reference for native loading-flow controls")
  if(HALO2_ORIGINAL_LOADING_CACHE)
    add_test(NAME original_loading_gpu COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_LIST_DIR}/../tools/run_test.py -- $<TARGET_FILE:original_loading_gpu_tests>
      "${HALO2_ORIGINAL_LOADING_CACHE}" @HALO2_TEST_SCRATCH@/original-loading)
    add_test(NAME original_loading_gpu_4_3 COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_LIST_DIR}/../tools/run_test.py -- $<TARGET_FILE:original_loading_gpu_tests>
      "${HALO2_ORIGINAL_LOADING_CACHE}" @HALO2_TEST_SCRATCH@/original-loading-4-3)
    set_tests_properties(original_loading_gpu original_loading_gpu_4_3
      PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    set_tests_properties(original_loading_gpu_4_3 PROPERTIES ENVIRONMENT "HALO2_TEST_LOADING_4_3=1")
    if(HALO2_LOADING_FLOW_TU15_IMAGE)
      set(loading_flow_maps "${HALO2_ORIGINAL_LOADING_CACHE}/halo2/h2_maps_win64_dx11")
      add_test(NAME original_loading_local_roundtrip COMMAND native_vista_flow_tests
        "${loading_flow_maps}" "${loading_flow_maps}/mainmenu.map" "${HALO2_LOADING_FLOW_TU15_IMAGE}"
        @HALO2_TEST_SCRATCH@/loading-local-roundtrip)
      add_test(NAME original_loading_split_screen_roundtrip COMMAND native_vista_flow_tests
        "${loading_flow_maps}" "${loading_flow_maps}/mainmenu.map" "${HALO2_LOADING_FLOW_TU15_IMAGE}"
        @HALO2_TEST_SCRATCH@/loading-split-screen-roundtrip --two-players)
      set_tests_properties(original_loading_local_roundtrip original_loading_split_screen_roundtrip
        PROPERTIES TIMEOUT 600)
    endif()
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
