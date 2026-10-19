# Release assets are local binary inputs; their portable checksums stay in Git.
set(HALO2_MOVIE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/movies" CACHE PATH
    "Directory containing the supplied intro_60.mp4 and credits_60.mp4")
set(_halo2_movies_default OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_CONFIGURATION_TYPES OR
   (EXISTS "${HALO2_MOVIE_SOURCE_DIR}/intro_60.mp4" AND EXISTS "${HALO2_MOVIE_SOURCE_DIR}/credits_60.mp4"))
    set(_halo2_movies_default ON)
endif()
option(HALO2_BUNDLE_MOVIES "Bundle the supplied upscaled intro and credits" ${_halo2_movies_default})
if(HALO2_BUNDLE_MOVIES)
    foreach(_movie intro_60.mp4 credits_60.mp4)
        if(NOT EXISTS "${HALO2_MOVIE_SOURCE_DIR}/${_movie}")
            message(FATAL_ERROR "Missing release movie ${_movie}. Run tools/bundle_movies.py --source /path/to/movies, set HALO2_MOVIE_SOURCE_DIR, or explicitly disable HALO2_BUNDLE_MOVIES for a build without upscaled movies.")
        endif()
    endforeach()
    set(_halo2_movies_output "${CMAKE_CURRENT_BINARY_DIR}/resources/movies")
    set(_halo2_movie_metadata_output "${CMAKE_CURRENT_BINARY_DIR}/resources/movie-manifest.json")
    if(CMAKE_CONFIGURATION_TYPES)
        set(_halo2_movies_output "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources/movies")
        set(_halo2_movie_metadata_output "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources/movie-manifest.json")
    endif()
    set(_halo2_movie_files "${_halo2_movies_output}/intro_60.mp4"
        "${_halo2_movies_output}/credits_60.mp4" "${_halo2_movies_output}/manifest.json")
    add_custom_command(OUTPUT ${_halo2_movie_files} "${_halo2_movie_metadata_output}"
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/bundle_movies.py"
            --source "${HALO2_MOVIE_SOURCE_DIR}" --output "${_halo2_movies_output}"
            --manifest "${CMAKE_CURRENT_SOURCE_DIR}/resources/movie-manifest.json"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/movie-manifest.json" "${_halo2_movie_metadata_output}"
        DEPENDS tools/bundle_movies.py resources/movie-manifest.json
            "${HALO2_MOVIE_SOURCE_DIR}/intro_60.mp4" "${HALO2_MOVIE_SOURCE_DIR}/credits_60.mp4"
        COMMENT "Bundling verified upscaled intro and credits" VERBATIM)
    add_custom_target(halo2_movies ALL DEPENDS ${_halo2_movie_files} "${_halo2_movie_metadata_output}")
    add_dependencies(halo2_native halo2_movies)
    install(FILES ${_halo2_movie_files} DESTINATION resources/movies COMPONENT movies)
    install(FILES "${_halo2_movie_metadata_output}" DESTINATION resources COMPONENT movies)
endif()
