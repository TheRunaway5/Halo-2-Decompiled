# TU1.5 scenery lightmap lookup

The normal-launcher Zanzibar scene constructor reached original 10F94B ->
10F960 through C1120/10F900 and stopped because the latter body was missing.
Its complete unchanged English TU1.5 CFG is now included in
`physics_live_support_tu15_generated.cpp`. The pinned executable SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

10F960 reads the active lightmap's first lightmap group, searches its scenery
object records, and returns the first matching ordinal or -1. The source compares
unique ID, type, source and render-model checksum. It also compares origin BSP
when source is Structure(0); other sources do not compare that field. The
Assembly `Halo2/ltmp.xml` schema identifies root+80 Lightmap Groups and
group+58 Scenery Object Info with its 12-byte rows. This is a lightmap lookup,
not an effect-attachment creator. Earlier commentary used the wrong label;
the implementation and comparisons execute the original bytes.

`analysis/map-world-features/zanzibar-factory-20261013/verification.json`
records 512 original-executable comparisons with no mocked callees. Every
instruction, register, flag, stack return and stack store matches the compiled
body. Fixture inputs include absent/inactive lightmaps, empty banks, misses,
first matches, duplicate matches, differing BSP indices, source types and
checksums. `original_tu15_scenery_lightmap_lookup` replays that fixture bank.

The source generator, additional-entry manifest, generated source/object,
original fixture generator, source installation receipt and isolated runtime
link inputs are retained in that directory. The entry's real original call
instruction is `E8 10 00 00 00` at 10F94B. The normal root build was not replaced
with the isolated binary.

The subsequent isolated runtime includes later shared startup changes and
stops at original 1888A0, the newly requested local input/camera constructor,
before scene construction. Consequently this run does not prove live Zanzibar
construction gets past 10F960, complete lighting, or full gameplay. The earlier
failed normal-launcher capture and the later failed isolated attempt are both
preserved. Resolve the startup owner and rerun ordinary map construction and
interaction acceptance; diagnostic replay modes exclude the source world and
cannot establish that acceptance.
