if(BUILD_TESTING)
    add_custom_target(custom_asset_tests)
    add_test(NAME custom_asset_sources COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_custom_assets.py)
    add_test(NAME custom_ui_assets COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_custom_ui_assets.py)
    add_test(NAME custom_map_assets COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_custom_map_assets.py)
    add_test(NAME h2ek_loading_ambience COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_h2ek_loading_ambience.py)
    add_test(NAME mcc_original_loading_assets COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_loading_bank_assets.py)
endif()
