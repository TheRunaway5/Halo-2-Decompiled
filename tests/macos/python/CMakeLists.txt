cmake_minimum_required(VERSION 3.24)
get_filename_component(HALO2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include("${HALO2_ROOT}/cmake/macos/Defaults.cmake")
project(Halo2MacOSPythonTests LANGUAGES CXX)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(launcher_sdk "${CMAKE_BINARY_DIR}/launcher-sdk")
if(NOT EXISTS "${launcher_sdk}/sdk/cmake/SDL2/SDL2Config.cmake")
    execute_process(COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/prepare_sdk.py"
        "${launcher_sdk}" COMMAND_ERROR_IS_FATAL ANY)
endif()
set(SDL2_DIR "${launcher_sdk}/sdk/cmake/SDL2")
find_package(SDL2 REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)
include("${HALO2_ROOT}/cmake/TestStorage.cmake")
include(CTest)
include("${HALO2_ROOT}/cmake/macos/Python.cmake")
halo2_macos_python_dependency()
include("${HALO2_ROOT}/cmake/macos/Application.cmake")
add_executable(macos_python_launcher "${HALO2_ROOT}/src/app/launcher.cpp")
target_compile_features(macos_python_launcher PRIVATE cxx_std_20)
target_link_libraries(macos_python_launcher PRIVATE SDL2::SDL2 nlohmann_json::nlohmann_json)
target_compile_definitions(macos_python_launcher PRIVATE HALO2_PROJECT_DIR="."
    HALO2_RUNTIME_DIR="build" HALO2_PYTHON_EXECUTABLE="${Python3_EXECUTABLE}")
halo2_macos_application(macos_python_launcher)
add_custom_target(macos_python_tests DEPENDS halo2_macos_python_sdk macos_python_launcher)
option(HALO2_MACOS_LOADING_CONVERTER_TEST "Include the real native converter in the relocated launcher" OFF)
set(HALO2_MACOS_LOADING_TEST_SOURCE "" CACHE PATH "Optional owned MCC source for bundled converter validation")
set(loading_arguments)
if(HALO2_MACOS_LOADING_CONVERTER_TEST)
    include("${HALO2_ROOT}/cmake/macos/Vorbis.cmake")
    halo2_macos_vorbis_dependency()
    add_executable(macos_setup_loading_converter
        "${HALO2_ROOT}/tools/convert_mcc_loading_audio.cpp"
        "${HALO2_ROOT}/external/ww2ogg/src/wwriff.cpp"
        "${HALO2_ROOT}/external/ww2ogg/src/codebook.cpp"
        "${HALO2_ROOT}/external/ww2ogg/src/crc.c")
    set_source_files_properties("${HALO2_ROOT}/external/ww2ogg/src/crc.c" PROPERTIES LANGUAGE CXX)
    target_compile_features(macos_setup_loading_converter PRIVATE cxx_std_20)
    target_include_directories(macos_setup_loading_converter PRIVATE "${HALO2_ROOT}/external/ww2ogg/src")
    target_link_libraries(macos_setup_loading_converter PRIVATE PkgConfig::VORBISFILE)
    add_dependencies(macos_python_tests macos_setup_loading_converter)
    list(APPEND loading_arguments --loading-converter "$<TARGET_FILE:macos_setup_loading_converter>"
        --vorbis-sdk "${CMAKE_BINARY_DIR}/macos-dependencies/vorbis")
    if(HALO2_MACOS_LOADING_TEST_SOURCE)
        list(APPEND loading_arguments --loading-source "${HALO2_MACOS_LOADING_TEST_SOURCE}")
    endif()
endif()
set(HALO2_MACOS_STEAM_SDK_INPUT "" CACHE PATH "Optional caller-owned private Steam SDK for combined launcher validation")
set(steam_arguments)
if(HALO2_MACOS_STEAM_SDK_INPUT)
    list(APPEND steam_arguments --steam-sdk "${HALO2_MACOS_STEAM_SDK_INPUT}")
endif()
add_test(NAME macos_python_relocation COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/relocation_test.py" "${HALO2_MACOS_PYTHON_PREFIX}"
    "${HALO2_ROOT}" "$<TARGET_FILE:macos_python_launcher>" "${launcher_sdk}"
    "${CMAKE_BINARY_DIR}/python-relocation.json" ${CMAKE_OSX_ARCHITECTURES} ${steam_arguments} ${loading_arguments})
set_tests_properties(macos_python_relocation PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
if(HALO2_MACOS_STEAM_SDK_INPUT)
    add_test(NAME steam_content_plan COMMAND ${Python3_EXECUTABLE} -B "${HALO2_ROOT}/tests/test_steam_content.py")
    add_test(NAME steam_content_session COMMAND ${Python3_EXECUTABLE} -B "${HALO2_ROOT}/tests/test_steam_session.py")
    option(HALO2_MACOS_STEAM_AUTH_PLAN "Run human-approved QR authentication and metadata-only planning" OFF)
    option(HALO2_MACOS_STEAM_DOWNLOAD_TEST "Run user-authorized persistent MCC download/resume validation" OFF)
    if(HALO2_MACOS_STEAM_AUTH_PLAN)
        add_test(NAME macos_steam_auth_plan COMMAND ${Python3_EXECUTABLE} -B
            "${CMAKE_CURRENT_SOURCE_DIR}/auth_plan_test.py" "${HALO2_MACOS_PYTHON_PREFIX}" "${HALO2_ROOT}"
            "$<TARGET_FILE:macos_python_launcher>" "${launcher_sdk}" "${HALO2_MACOS_STEAM_SDK_INPUT}"
            "${CMAKE_BINARY_DIR}/steam-auth-plan.json")
        set_tests_properties(macos_steam_auth_plan PROPERTIES RUN_SERIAL TRUE TIMEOUT 440)
    endif()
    if(HALO2_MACOS_STEAM_DOWNLOAD_TEST)
        add_test(NAME macos_steam_download_resume COMMAND ${Python3_EXECUTABLE} -B
            "${CMAKE_CURRENT_SOURCE_DIR}/download_resume_test.py" "${HALO2_MACOS_PYTHON_PREFIX}" "${HALO2_ROOT}"
            "$<TARGET_FILE:macos_python_launcher>" "${launcher_sdk}" "${HALO2_MACOS_STEAM_SDK_INPUT}"
            "${CMAKE_BINARY_DIR}/steam-download-resume.json")
        set_tests_properties(macos_steam_download_resume PROPERTIES RUN_SERIAL TRUE TIMEOUT 7800)
    endif()
endif()
