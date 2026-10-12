# Whole sound creation with actual cache ownership

The complete `source_sound_create` owner now composes with
`source_sound_cache_request`, the original async queue/worker CPU owners,
real native mutex/semaphore owners and actual reads from the owned `shared.map`.
No gameplay or renderer source was changed by this checkpoint.

The original experiment captures three actual entry frames at `127750`, before
creation writes, while reexecuting the previously frozen original creation
experiment. The whole reexecution must equal that prior result. The captured
requests are the previously recorded ready, reload and empty-reload raw first
person buffers; their original admission had been rejected. Calling this owner
directly is an explicit ABI composition and does not change that admission.

Each case starts from the entire retained original constructor/map-prefix
state. The unchanged queue constructor produces its node lists and obtains real
native mutexes/semaphore. It stops before the unavailable original thread
constructor. Whole unchanged `127750` then runs naturally to its real return:
datum `EF730000`. Its cache request publishes real pending work, creation sets
source flag `+4` bit 1, and the subsequent flags-4 cache call retains that pending
entry. A ready completion byte is not required for this original retain path.
The source-created entry remains pending until the original worker reads the
actual owned file and writes completion. Subsequent cache flags 0, 4 and 5 return
the original values 2, 6 and 6.

The original worker runs synchronously in this serial checkpoint and stops at
the next actual semaphore with no permit. The native implementation follows
the same phases. Original file-token creation, thread TLS/priority/lifetime,
driver admission, listener construction and live sound scheduling remain
separate. The shared-file alias is an explicit native binding. No source
availability flag, voice identity, cache result, completion byte, permutation,
RNG seed or source datum is assigned to advance the experiment.

Two independent executions produce byte-identical original evidence. Native
normal and ASan/UBSan snapshot builds, followed by fresh production normal and
sanitizer builds, all match three complete creations, eighteen full phases,
8,678 ordered stores and 65,779,560 complete output bytes. Each reads the real
shared file once. The fixture exporter also preserves complete before states;
its 131,559,120-byte total counts both before and after buffers.

The original report SHA-256 is
`45f02efccebea71086c277526dab443b9fb52191f2b37d788069fc8060b55b9f`.
The literal fixture SHA-256 is
`ccd0d64396100d4a88c521c306a2afc70e238bf81ca0fb05320ac94dd7e1f77f`.
The production test is `original_sound_create_cache`; its command accepts the
fixture and the owned map directory. Missing owned inputs skip through the
existing fixture runner; a reported pass must contain all three actual reads.

The source snapshots, reports, repeats, fixture, production archives, binaries
and compilation flags are retained under
`analysis/multiplayer/integration/sound-create-cache-production-20261009/frozen`.
The initial warnings-as-errors build remains separately retained: two existing
indentation warnings in the immutable worker dependency rejected that stricter
compiler policy. The canonical standalone build uses the dependency's existing
warning policy and preserves its source unchanged.

This checkpoint verifies returned creation and pending-cache ownership for the
three actual Lockout ABI states on update 1.1. The imported final-update whole
body correspondence is retained upstream; this is not an executed final-update
or all-map creation suite. Concurrent creation, audible reload playback, the
live first person event route and the user's other gameplay reports remain
unfinished. No new launcher or gameplay screenshot is claimed.
