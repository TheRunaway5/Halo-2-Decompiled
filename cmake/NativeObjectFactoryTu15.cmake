# Native owners of TU1.5 object_new (B9B70) and its subtrees. No original-code
# execution; see docs/NATIVE_OBJECT_FACTORY.md. Track sources are globbed so
# parallel ports add files without editing this list:
#   src/gameplay/objects/native_object_factory_<track>_tu15.cpp   production owners
#   tests/native_object_factory_<track>_kernels.cpp              oracle harness kernels
# HALO2_NATIVE_OBJECT_FACTORY_TRACKS (a list of track names) restricts a build
# to the base owners plus those tracks, isolating parallel work in progress.
# Tracks call each other; a filtered build must list every track it reaches,
# for example nodebuild now needs "nodes;nodebuild;placement;core;weapon;
# deviceanim;units;havok;unitactions;objtypes". An empty value (the default
# and production) builds every track.
# They join the native script kernel set (shared world bank and B9390 marker)
# and need native 16E6A0/16E8A0.
include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/NativeTu15DataArrayAllocate.cmake")
set(HALO2_NATIVE_OBJECT_FACTORY_TRACKS "" CACHE STRING
    "Restrict native object factory tracks (empty: all tracks)")
file(GLOB _halo2_factory_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../src/gameplay/objects/native_object_factory*_tu15.cpp")
file(GLOB _halo2_factory_test_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../tests/native_object_factory_*_kernels.cpp")
set(HALO2_NATIVE_OBJECT_FACTORY_SOURCES)
set(HALO2_NATIVE_OBJECT_FACTORY_TEST_SOURCES)
foreach(_source IN LISTS _halo2_factory_sources)
    get_filename_component(_name "${_source}" NAME)
    string(REGEX REPLACE "^native_object_factory_?(.*)_tu15\\.cpp$" "\\1" _track "${_name}")
    if(_track STREQUAL "" OR NOT HALO2_NATIVE_OBJECT_FACTORY_TRACKS OR _track IN_LIST HALO2_NATIVE_OBJECT_FACTORY_TRACKS)
        list(APPEND HALO2_NATIVE_OBJECT_FACTORY_SOURCES "${_source}")
    endif()
endforeach()
foreach(_source IN LISTS _halo2_factory_test_sources)
    get_filename_component(_name "${_source}" NAME)
    string(REGEX REPLACE "^native_object_factory_(.*)_kernels\\.cpp$" "\\1" _track "${_name}")
    if(NOT HALO2_NATIVE_OBJECT_FACTORY_TRACKS OR _track IN_LIST HALO2_NATIVE_OBJECT_FACTORY_TRACKS)
        list(APPEND HALO2_NATIVE_OBJECT_FACTORY_TEST_SOURCES "${_source}")
    endif()
endforeach()
