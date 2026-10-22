cmake_minimum_required(VERSION 3.24)
get_filename_component(HALO2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include("${HALO2_ROOT}/cmake/macos/Defaults.cmake")
project(Halo2MacOSFFmpegTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)
find_program(HALO2_TEST_FFMPEG_EXECUTABLE ffmpeg REQUIRED)
find_path(Vulkan_INCLUDE_DIR vulkan/vulkan.h REQUIRED)
include("${HALO2_ROOT}/cmake/Glm.cmake")
include("${HALO2_ROOT}/cmake/TestStorage.cmake")
include("${HALO2_ROOT}/cmake/macos/FFmpeg.cmake")
include("${HALO2_ROOT}/cmake/macos/Opus.cmake")
include("${HALO2_ROOT}/cmake/macos/OpenSSL.cmake")
include(CTest)
halo2_macos_ffmpeg_dependency()
halo2_macos_opus_dependency()
halo2_macos_openssl_dependency()
add_executable(vulkan_portability_tests "${HALO2_ROOT}/tests/vulkan_portability_tests.cpp")
target_include_directories(vulkan_portability_tests PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src" "${Vulkan_INCLUDE_DIR}")
add_test(NAME vulkan_portability COMMAND vulkan_portability_tests)
add_library(playback STATIC "${HALO2_ROOT}/src/platform/media/movie.cpp" "${HALO2_ROOT}/src/audio/audio.cpp"
    "${HALO2_ROOT}/src/audio/mcc_audio.cpp" "${HALO2_ROOT}/src/audio/vista_audio_decode.cpp"
    "${HALO2_ROOT}/src/audio/vista_audio_framing.cpp" "${HALO2_ROOT}/src/audio/sound_fade.cpp"
    "${HALO2_ROOT}/src/audio/sound_classes.cpp")
target_include_directories(playback PUBLIC "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
target_link_libraries(playback PUBLIC PkgConfig::AVCODEC PkgConfig::OPUS OpenSSL::Crypto
    SDL2::SDL2 halo2_glm nlohmann_json::nlohmann_json Threads::Threads)
add_executable(macos_ffmpeg_dependency_tests MACOSX_BUNDLE dependency_tests.cpp)
target_link_libraries(macos_ffmpeg_dependency_tests PRIVATE playback)
set_target_properties(macos_ffmpeg_dependency_tests PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER net.halo2forever.ffmpeg-dependency-check)
set(arch_args)
foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
    list(APPEND arch_args --arch "${arch}")
endforeach()
add_custom_command(TARGET macos_ffmpeg_dependency_tests POST_BUILD
    COMMAND ${Python3_EXECUTABLE} -B "${HALO2_ROOT}/tools/macos/bundle.py"
        --app "$<TARGET_BUNDLE_DIR:macos_ffmpeg_dependency_tests>" ${arch_args}
        --license "FFmpeg-LGPL-2.1=${CMAKE_BINARY_DIR}/macos-dependencies/ffmpeg/share/licenses/FFmpeg-COPYING.LGPLv2.1"
        --license "FFmpeg-LGPL-3=${CMAKE_BINARY_DIR}/macos-dependencies/ffmpeg/share/licenses/FFmpeg-COPYING.LGPLv3"
        --license "FFmpeg-notices=${CMAKE_BINARY_DIR}/macos-dependencies/ffmpeg/share/licenses/FFmpeg-LICENSE.md"
        --license "Dav1d=${CMAKE_BINARY_DIR}/macos-dependencies/ffmpeg/share/licenses/Dav1d.txt"
        --license "Opus=${CMAKE_BINARY_DIR}/macos-dependencies/opus/share/licenses/Opus.txt"
        --license "OpenSSL=${CMAKE_BINARY_DIR}/macos-dependencies/openssl/share/licenses/OpenSSL.txt"
        --license "SDL=${HALO2_SDL_LICENSE}" VERBATIM)
add_executable(movie_tests "${HALO2_ROOT}/tests/movie_tests.cpp")
target_link_libraries(movie_tests PRIVATE playback)
set(fixtures "${CMAKE_BINARY_DIR}/fixtures")
add_custom_command(OUTPUT "${fixtures}/manifest.json" "${fixtures}/h264.mp4"
        "${fixtures}/hevc.mp4" "${fixtures}/av1.mp4"
    COMMAND ${Python3_EXECUTABLE} -B "${HALO2_ROOT}/tools/run_test.py" --
        ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/prepare_movies.py"
        "${HALO2_TEST_FFMPEG_EXECUTABLE}" "${fixtures}"
    DEPENDS prepare_movies.py VERBATIM)
add_custom_target(macos_ffmpeg_fixtures DEPENDS "${fixtures}/manifest.json")
add_custom_target(macos_ffmpeg_tests DEPENDS macos_ffmpeg_dependency_tests movie_tests macos_ffmpeg_fixtures)
foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
    add_test(NAME macos_ffmpeg_playback_${arch} COMMAND /usr/bin/arch -${arch}
        $<TARGET_FILE:macos_ffmpeg_dependency_tests> "${fixtures}"
        "$<TARGET_BUNDLE_CONTENT_DIR:macos_ffmpeg_dependency_tests>/Frameworks")
    add_test(NAME macos_ffmpeg_routing_${arch} COMMAND /usr/bin/arch -${arch} $<TARGET_FILE:movie_tests>)
endforeach()
add_test(NAME macos_ffmpeg_relocation COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/relocation_test.py"
    "$<TARGET_BUNDLE_DIR:macos_ffmpeg_dependency_tests>" "${CMAKE_BINARY_DIR}/macos-dependencies"
    "${fixtures}" "${CMAKE_BINARY_DIR}/ffmpeg-relocation.json" ${CMAKE_OSX_ARCHITECTURES})
set_tests_properties(macos_ffmpeg_relocation PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
