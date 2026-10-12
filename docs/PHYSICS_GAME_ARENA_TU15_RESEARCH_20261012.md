# TU1.5 game arena and async startup ownership

The research checkpoint below is preserved. Subsequent
[native implementation and validation](PHYSICS_GAME_ARENA_TU15_20261012.md)
now execute the complete constructor, source worker and file startup in TU1.5.

This is source recovery for the next fresh-world dependency. It does not claim
that the arena constructor or its native async/file services run in TU1.5 yet.

[Complete recovery](../tools/research_physics_game_arena_tu15_20261012.py)
authenticates 17 whole routines and 730 instruction sites against the owned
English TU1.5 executable, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Acceptance requires complete normalized instruction/internal-branch matches.
Actual paired call or callback pointer operands constrain small ambiguous
bodies. No uniform relocation delta or attached decompiler annotation selects
an entry.

The report is retained at
[complete-report.json](../analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012/correspondence-v1/complete-report.json),
SHA256 `616bee89417916769418d9f2a70075ab036c2bba8a1decadd46698c5d57f1933`.
It includes complete original/selected instruction bytes, body listings and
relocated operands. Recovery is repeated independently in `correspondence-repeat`.

The authenticated bodies cover the arena constructor `125280`, partition/file
setup `217090`, queue construction `1221B0`, CRC owners `1656A0/165700`, allocator
`125E50`, formatting wrapper `11E0F0`, queue claim/insert/retire owners
`121FD0/122020/1220E0/122150`, thread entry/worker `122350/122360`, file open
callback `1A4250`, file extent callback/enqueue `1A4C20/1A4C70`, and queue ordering
`1A5080`. These are retained correspondence IDs, not alternate current parity
targets.

[External ownership inspection](../tools/inspect_physics_game_arena_tu15_20261012.py)
retains 18 distinct actual direct targets outside those bodies, one indirect
worker callback call, and all three source-selected worker/file callback
bindings. The earlier native physical partitioner `217020` / TU1.5 `21B920`
and name copy `326970` / `32A4E0` already have compiled selected-image bodies.
Other edges require their exact resource/compatibility ABI, rather than a
supplied queue-completion or successful file result.

The remaining direct targets include mutex, semaphore, thread creation,
priority, wait/release/yield, file open/read/extent, formatting/string and failure
helpers. The earlier TU1.1 death startup native owner supplies useful reference
structure for real Linux threads, synchronization and files, but its hardcoded
call frames and worker entry cannot be labeled TU1.5 without binding the actual
selected-image operands and checking the complete runtime interaction.

The next implementation must compile these selected bodies, join actual native
queue/worker/file resources and the already verified physical partitioner, and
compare fresh complete arena construction against unchanged TU1.5 execution.
It must retain the call to `217090` and its source completion/CRC/allocator tail.
No captured arena, skipped async work, forced completion byte, fabricated file
handle or default source scheduling result is an accepted replacement.
