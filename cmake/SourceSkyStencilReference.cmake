# Authenticate retained original inputs before compiling reference consumers.
function(halo2_verify_source_sky_references sky_source_root)
    # These are the exact consumed captured geometry/uniform subsets and
    # original depth/stencil snapshots, pinned by the frozen prototype proof.
    # Missing or changed references fail explicitly; no substitute is generated.
    set(sky_reference_files
        "analysis/multiplayer-shading/source-sky-stencil-prototype/8097-retained-strip.fixture|1676|0f3d25f06c02e05239875f2e7a34c9112ef25c3ad37cd9790345ba8518808aa8"
        "analysis/multiplayer-shading/source-sky-stencil-prototype/8097-retained-gsout.fixture|14884|49b98d0c439178a0c2fc202d587059bb690bc2474901a1bda5b3e6dadc313158"
        "analysis/multiplayer-shading/original20-sky-stencil-analysis/8097-before-packed-d24s8.bin|1228800|98728be819022c19654a4fbf2cfc6bc6727e8ad061988caba36b6b940b2c189e"
        "analysis/multiplayer-shading/original20-sky-stencil-analysis/8097-after-packed-d24s8.bin|1228800|5248ac22b7f3ddfd2a6f3e5fec2d6b6d839b4482b343aac7b9209aacc390b856"
        "analysis/multiplayer-shading/source-sky-stencil-prototype/8102-retained-strip.fixture|1192|3cbeef14711c441094a4d1cb8ca3c50a1218358853b3bb15a696424f7807c627"
        "analysis/multiplayer-shading/source-sky-stencil-prototype/8102-retained-gsout.fixture|9784|f9cb89cfe6a55d498eb4bcf78896676192bcfe2ed7dc99d6f8d3cc551e4df826"
        "analysis/multiplayer-shading/original20-sky-stencil-analysis/8102-before-packed-d24s8.bin|1228800|5248ac22b7f3ddfd2a6f3e5fec2d6b6d839b4482b343aac7b9209aacc390b856"
        "analysis/multiplayer-shading/original20-sky-stencil-analysis/8102-after-packed-d24s8.bin|1228800|5248ac22b7f3ddfd2a6f3e5fec2d6b6d839b4482b343aac7b9209aacc390b856"
        "analysis/multiplayer-shading/source-sky-stencil-prototype/8110-retained-strip.fixture|340|c4c98723ca9901adf05e033a79883376a996d48f6b90d199ccd6fd269a4bc302"
        "analysis/multiplayer-shading/source-sky-stencil-prototype/8110-retained-gsout.fixture|604|69b07a6681996b2db5df12f0ceff299320dd0f64f692d5bc0d4e798ee9104dc7"
        "analysis/multiplayer-shading/original20-sky-stencil-analysis/8110-before-packed-d24s8.bin|1228800|5248ac22b7f3ddfd2a6f3e5fec2d6b6d839b4482b343aac7b9209aacc390b856"
        "analysis/multiplayer-shading/original20-sky-stencil-analysis/8110-after-packed-d24s8.bin|1228800|5248ac22b7f3ddfd2a6f3e5fec2d6b6d839b4482b343aac7b9209aacc390b856"
    )
    foreach(sky_reference IN LISTS sky_reference_files)
        string(REPLACE "|" ";" sky_reference_parts "${sky_reference}")
        list(GET sky_reference_parts 0 sky_relative)
        list(GET sky_reference_parts 1 sky_expected_size)
        list(GET sky_reference_parts 2 sky_expected_hash)
        set(sky_reference_path "${sky_source_root}/${sky_relative}")
        if(NOT EXISTS "${sky_reference_path}")
            message(FATAL_ERROR "Retained original20 sky reference missing: ${sky_relative}")
        endif()
        file(SIZE "${sky_reference_path}" sky_actual_size)
        file(SHA256 "${sky_reference_path}" sky_actual_hash)
        if(NOT sky_actual_size EQUAL sky_expected_size OR
                NOT sky_actual_hash STREQUAL sky_expected_hash)
            message(FATAL_ERROR "Retained original20 sky reference identity changed: ${sky_relative}")
        endif()
    endforeach()
endfunction()
