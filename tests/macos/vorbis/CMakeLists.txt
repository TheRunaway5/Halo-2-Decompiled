cmake_minimum_required(VERSION 3.24)
get_filename_component(HALO2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include("${HALO2_ROOT}/cmake/macos/Defaults.cmake")
project(Halo2MacOSVorbisTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include("${HALO2_ROOT}/cmake/TestStorage.cmake")
include("${HALO2_ROOT}/cmake/macos/Vorbis.cmake")
include(CTest)
halo2_macos_vorbis_dependency()
set(HALO2_MCC_LOADING_TEST_MEDIA "" CACHE FILEPATH "Authenticated WEM in the enclosing test's scratch")
add_executable(halo2_mcc_loading_audio MACOSX_BUNDLE
    "${HALO2_ROOT}/tools/convert_mcc_loading_audio.cpp"
    "${HALO2_ROOT}/external/ww2ogg/src/wwriff.cpp"
    "${HALO2_ROOT}/external/ww2ogg/src/codebook.cpp"
    "${HALO2_ROOT}/external/ww2ogg/src/crc.c")
set_source_files_properties("${HALO2_ROOT}/external/ww2ogg/src/crc.c" PROPERTIES LANGUAGE CXX)
target_include_directories(halo2_mcc_loading_audio PRIVATE "${HALO2_ROOT}/external/ww2ogg/src")
target_link_libraries(halo2_mcc_loading_audio PRIVATE PkgConfig::VORBISFILE)
set_target_properties(halo2_mcc_loading_audio PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER net.halo2forever.loading-decoder-check)
set(arch_args)
foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
    list(APPEND arch_args --arch "${arch}")
endforeach()
add_custom_command(TARGET halo2_mcc_loading_audio POST_BUILD
    COMMAND ${Python3_EXECUTABLE} -B "${HALO2_ROOT}/tools/macos/bundle.py"
        --app "$<TARGET_BUNDLE_DIR:halo2_mcc_loading_audio>" ${arch_args}
        --license "Ogg=${CMAKE_BINARY_DIR}/macos-dependencies/vorbis/share/licenses/Ogg.txt"
        --license "Vorbis=${CMAKE_BINARY_DIR}/macos-dependencies/vorbis/share/licenses/Vorbis.txt"
        --license "ww2ogg=${HALO2_ROOT}/external/ww2ogg/COPYING"
    VERBATIM)
add_executable(macos_vorbis_dependency_tests MACOSX_BUNDLE dependency_tests.cpp)
target_link_libraries(macos_vorbis_dependency_tests PRIVATE halo2_macos_vorbisenc halo2_macos_vorbisfile)
set_target_properties(macos_vorbis_dependency_tests PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER net.halo2forever.vorbis-dependency-check)
add_custom_command(TARGET macos_vorbis_dependency_tests POST_BUILD
    COMMAND ${Python3_EXECUTABLE} -B "${HALO2_ROOT}/tools/macos/bundle.py"
        --app "$<TARGET_BUNDLE_DIR:macos_vorbis_dependency_tests>" ${arch_args}
        --license "Ogg=${CMAKE_BINARY_DIR}/macos-dependencies/vorbis/share/licenses/Ogg.txt"
        --license "Vorbis=${CMAKE_BINARY_DIR}/macos-dependencies/vorbis/share/licenses/Vorbis.txt" VERBATIM)
add_custom_target(macos_vorbis_tests DEPENDS halo2_mcc_loading_audio macos_vorbis_dependency_tests)
foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
    add_test(NAME macos_loading_audio_dependency_${arch} COMMAND /usr/bin/arch -${arch}
        $<TARGET_FILE:macos_vorbis_dependency_tests>
        "$<TARGET_BUNDLE_CONTENT_DIR:macos_vorbis_dependency_tests>/Frameworks")
endforeach()
add_test(NAME macos_loading_audio_dependency_relocation COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/relocation_test.py"
    "$<TARGET_BUNDLE_DIR:macos_vorbis_dependency_tests>" "${CMAKE_BINARY_DIR}/macos-dependencies/vorbis"
    ${CMAKE_OSX_ARCHITECTURES})
set_tests_properties(macos_loading_audio_dependency_relocation PROPERTIES RUN_SERIAL TRUE)
string(JOIN "," packaged_arches ${CMAKE_OSX_ARCHITECTURES})
set(packaged_media_args)
if(HALO2_MCC_LOADING_TEST_MEDIA)
    list(APPEND packaged_media_args "${HALO2_MCC_LOADING_TEST_MEDIA}")
endif()
add_test(NAME macos_loading_audio_packaged_runtime COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging_test.py"
    "$<TARGET_BUNDLE_DIR:halo2_mcc_loading_audio>" "${CMAKE_BINARY_DIR}/macos-dependencies/vorbis"
    "${CMAKE_BINARY_DIR}/loading-converter-package.json" "${packaged_arches}" ${packaged_media_args})
set_tests_properties(macos_loading_audio_packaged_runtime PROPERTIES RUN_SERIAL TRUE)
if(HALO2_MCC_LOADING_TEST_MEDIA)
    if(NOT EXISTS "${HALO2_MCC_LOADING_TEST_MEDIA}")
        message(FATAL_ERROR "The requested real loading WEM does not exist")
    endif()
    add_test(NAME macos_loading_audio_real_track COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/loading_audio_test.py"
        "$<TARGET_BUNDLE_DIR:halo2_mcc_loading_audio>" "${CMAKE_BINARY_DIR}/macos-dependencies/vorbis"
        "${HALO2_MCC_LOADING_TEST_MEDIA}" "${HALO2_ROOT}/external/ww2ogg/packed_codebooks_aoTuV_603.bin"
        "${CMAKE_BINARY_DIR}/loading-audio.json" ${CMAKE_OSX_ARCHITECTURES})
endif()
