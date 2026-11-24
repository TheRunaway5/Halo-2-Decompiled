# Native TU1.5 frame/gameplay owners of group F
# (docs/native_replacement/FRAME_MISC_OWNERS.md): seeded comparison against
# the pinned translated bodies with genuine child stand-ins, census and
# verify (record/replay) modes, sanitizer replay and mutation screens.
if(BUILD_TESTING)
    foreach(owner IN ITEMS 1306b0 d7230 1eee80 41970 133a70 153ba0 131610 14d200 bd460 1ae2b0 1ae090 3d7e0 16b240 170730 1d30f0 1e5250 333f0 15c4e0 3f3e0 bcfe0 17faf0 180520 144bf0 179a40 1a8e50 1902e0 b1720 17fb60 1d2f10 17beb0 165860 157fe0 e9530 10e1f0 125790 1e1390 116060 aa200 aa270 b9390 1751b0 e65a0
        106230 1d9f50 144cb0 bac30 10e340 be7e0 bda50 1ce810 130d20 e0500 c0700 1eedb0 18bbc0 c93d0
        c92c0 1e1620 110fc0 1e1920 11fa50 dfd20 1a8f10 175300
        175280 175240 1752c0 b16c0 17c5c0 1ae500 212c70 30c40 11e060 d2330 105950
        10dde0 10b0f0 1d3f90 16e290 b8d90 bbff0 caaa0
        14fbc0 ce230 cfb40 13f620 11e780 e3310 15dd10
        bc330 bbd50 15c7f0 df820 111d10
        c5250 17c330 c9590 1e5d60 ba910
        d2b00 1062a0 1cd620 11e600 152e00 106040 115ff0 1e15d0 c6e50 15b470 1f0f90 17e100 161560
        1cee60 1059c0 144210 1452f0 ba7e0 b92c0 d3a80 31edf0 33210c 161530 1d4140 116480 e32c0 cb440
        172200 bcf70 111f00 140ec0 105fd0 c8c20 113c70 1e5160
        1e5df0 1cf070 1cefb0
        1e5a70 bbf20
        1d4e50 1fa110 102910
        1e5f20 1e7810 212d40
        1e5e40 1521b0 152d60
        c8cf0 1be90
        2a0100 2db80 1d3930 1dc610 bc190 10c5d0 dfe30 1e5a50 335b0 150990 25bc70 226b0
        189f20 158450 25bcc0 d2b60 1d9f10 43980 12f0c0 118760 1d6430 304c50 1582f0
        18a240 171ff0 20700 1f33c0 c5170 31500 10db40 2e9750
        243510 16e7f0 2e3250 158340 1d9080 1d9100 3d2b0 1e5360 244f80
        168d76 158790 161610 13da89 1540d0 15b620
        1ae5e0 15b2d0 154d00 13e670 11ef70 1262f0 1ae6f0 b95d0
        c9010 1dc680 1157d0 153360 2e2e30 1edda0 1ceb70 c3960 140cf0 c3a20 be2b0 15cab0
        118120 4b150 c3820 1617a0 1a6830
        1718c0 15b740 e53e0 e52d0 113b70 14fdc0
        17cd10 1ce950 ba630 1cec60 e06e0
        1cf700 15b4d0 1459b0 15be80 1181f0 c8e70 c8390 d35c0
        1601b0 160360 160570 164890 1d5970 cdcf0 69ad0 1afa90 e71a0
        1d45a0 191130 be3b0 175380 144910 1af970 1d39c0 111b10 16bbc0
        16bda0 1cebd0 185820 13b610 16e510 13dc44 1d8b90 1d9190 2e9140
        315190 2f20f0 1ddea0 168ca1 164ef0 164750 17bd40 13b6f0 154270 15e870 e79c0 1ae7f0 1adc90 1770c0 245060 c7dd0 113190 1f8710 17c370 1618b0 17ead0 14d790 cd4a0 1de8e0 1538b0 1700e0 14d410 1115d0 1adf60 1139d0 15d890 d2bd0 4a7b0)
        add_test(NAME native_tu15_frame_misc_${owner}
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_frame_misc_tu15.py"
            --owner ${owner} --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-frame-misc-${owner}.json" --jobs 2)
        set_tests_properties(native_tu15_frame_misc_${owner} PROPERTIES TIMEOUT 2400)
    endforeach()
endif()
