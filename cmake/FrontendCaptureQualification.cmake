if(BUILD_TESTING)
    add_executable(frontend_intro_capture_tests tests/frontend_intro_capture_tests.cpp)
    target_include_directories(frontend_intro_capture_tests PRIVATE include)
    add_test(NAME frontend_intro_capture_budget COMMAND frontend_intro_capture_tests)
    add_test(NAME native_frontend_capture_qualification COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_frontend_capture_qualification.py)
endif()
