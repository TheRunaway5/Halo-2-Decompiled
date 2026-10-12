# Original sound driver update and effects reload

`sound_driver_update` ports the complete CPU callers `220D10` and `193020`.
Device work, real stream methods, section lifetime, DSP download and the named
source CPU callees remain mandatory providers. This is not transitive device
initialization or a claim of successful active updates, voice completion or
audible reload. No original service result or source field is chosen to make
an active path run.

The isolated checkpoint is
`analysis/multiplayer/integration/sound-driver-update-native/final-manifest.json`.
The source files are `include/halo2/sound_driver_update.hpp`,
`src/sound_driver_update.cpp` and `tests/sound_driver_update_tests.cpp`.

## Whole original authority

The owned 1.1 and authenticated owned u5 image hashes remain
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`
and `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The full-body correspondence and independent repeat are in `paired` and
`paired-repeat`. All instruction bytes are checked against those images.

| 1.1 | u5 | Original complete body |
|---|---|---|
| 220D10 | 225610 | Driver update CPU caller |
| 193020 | 1975B0 | Effects reload CPU caller |
| 550A0 | 55CD0 | Required source quiescence dependency |
| 53500 | 53540 | Required source restore dependency |
| 2B4BF0 | 2B85F0 | Required source packet-release dependency |
| 2DBDA1 | 2DF851 | Named section-load wrapper |
| 2DBDBC | 2DF86C | Named section-free wrapper |
| 381E84 | 385924 | DirectSoundDoWork boundary |
| 381EAD | 38594D | Effects-download boundary |

The two section wrappers have identical-copy ambiguity and are selected by
actual direct calls in the paired complete `193020` caller. The DSOUND bodies
are decoded from their actual DSOUND sections, not assumed to reside in .text.
The four-entry jump table in `220D10` is retained as literal bytes and checked:
all entries target the same continuation in both images. Thus the native code
retains the source dispatch-byte read without inventing a mode action.

The paired dependency bodies establish source interfaces and correspondence;
they are not claimed to have completed their downstream platform ownership.
The existing frozen packet-release module is reusable through its required
callback with actual retained cache/packet state. Quiescence and restore must
be their real full owners; they cannot be supplied empty successful callbacks.

## Actual original experiment

The oracle independently reexecutes all eight existing original driver
allocation prefixes: both images and four retained cursor residues. The three
changed cursor residues remain the earlier explicitly declared raw allocation
alignment controls. The original `2205C0` writes its own driver pointer, arena,
layout and fields and stops before DirectSoundCreate. No device result is
returned. This extension clones that whole original memory, rather than
assigning selected driver fields. u5 uses the same declared original 1.1 raw
startup arena boundary as the prior freeze; this is not full u5 title startup.

Two direct-ABI compositions then run independently on each whole prefix:

- Original `220D10` observes the actual zero stream count and equal zero
  generation values. It reaches the actual DirectSoundDoWork entry. No DoWork,
  device, GetStatus or voice result is supplied.
- Original `193020` reaches its first named XLoadSection request, with the actual
  literal `DSPImage` pointer. Section bytes existing in the image are not
  treated as a successful section lifetime/refcount/VM operation.

All sixteen controls stop before a persistent store and preserve every full
retained driver/global region. The complete before/after buffers total
4,588,448 bytes. This is precise unavailable-boundary evidence, not a positive
voiced path or a completed active update. The current source-created driver
cannot truthfully exercise GetStatus or effects-generation changes without
their real producers. No count, generation, callback, initialized, readiness,
clock or admission field is assigned to manufacture those cases.

Canonical original report SHA-256:
`0c09ccbd405ad4fd6a524eefec3aa41388163772f39eaee86dd97315f2f5422a`.
Fixture SHA-256:
`564fd5f3667adf0ddf8e24ecf50c1600ded721e75cb1c176606faa8639a0ab08`.
The independently reexecuted report, prefix listings, paired export and fixture
export are byte-identical. Both source images actually execute their measured
prefixes; the remaining branch correspondence is static authority.

## Required native contracts

`SoundDriverUpdateBindings` contains actual mutable operand addresses and the
verified section-name/quiescence-record pointers. `paired/providers.json`
retains their exact image mappings. None supplies an active source value:

| Field | 1.1 | u5 |
|---|---|---|
| driver_pointer | 526474 | 52D674 |
| effects_generation | 38D118 | 390BB8 |
| effects_context_pointer | 518480 | 51DF90 |
| optional_sound_flag | 4D0ED8 | 4D61D8 |
| quiesce_flag | 47E4BC | 48377C |
| quiesce_clear_word | 4D0E86 | 4D6186 |
| section_name | 45AEA0 | 45FCD0 |
| quiesce_record | 47E4B8 | 483778 |

`193020` requires genuine named section loading. An actual returned zero takes
the source return branch; an unavailable loader must stop instead of returning
a selected zero. After real loading, the source sets the two local effect
locations to 9 and10. If both original flags are nonzero it invokes complete
`550A0` with the original record and clears the original two-byte word. It then
requests effects download with the original name, those two locations, flags1
and the actual effects-context pointer, invokes complete `53500`, and releases
the named section. Download/free return values are unconsumed by this CPU
caller and are not invented by the module.

`220D10` compares driver `+2AD4` with the actual generation operand. If different,
it runs `193020`, rereads driver and generation, writes the word `+2A08 = 0101`
and stores the generation at `+2AD4`. Those writes occur after the real reload
owner returns, including its genuine load-failure return; an unavailable
provider cannot silently produce them. Genuine DirectSoundDoWork is mandatory
before the driver pointer and signed16 stream count are reread.

For each actual stream record at driver `+C + index*34`, only low-three-bit
mode1 queries GetStatus. The explicit actual identity/vtable/method and this
ABI are passed to `native_virtual_call`. Status uses retained caller-owned
four-byte backing, not a zero or a synthesized status. Source HRESULT is
unconsumed; the CPU consumes bit16 of that actual backing. If clear, it invokes
stream `+24` vtable slot18 with stack this, then stream `+30` vtable slot1C with
ECX this. It clears the mode's low bits, invokes complete `2B4BF0`, then clears
record byte `+3` bit2. The driver pointer/count are reread for the next stream.
No SDL completion is equated with this status contract without an owning
platform adapter that proves that mapping.

The module retains no memory span across external calls. Raw memory requires
the cooperating common CPU-entry gate; native blocking operations must use
its suspend/reacquire contract. No concurrent or atomic-visibility claim is
made by this stop-only suite. The independently frozen gate remains separate.

## Production registration, validation and limits

The narrow CMake change registers both this owner and the previously frozen
`sound_cache_lifecycle` in `halo2_cache`, with focused original-fixture CTests.
Current production normal and actual GCC ASan/UBSan builds pass both tests:

- Lifecycle:12 controls,18 phases,864 ordered stores,65,779,848 full output bytes,
  768 physical aliases and3 recorded real host-clock inputs.
- Driver update:16 original 1.1/u5 controls,2,294,224 complete output bytes,
  eight actual unavailable DoWork requests and eight section-load requests.

Fresh isolated current-source normal/sanitizer builds independently repeat
the driver checks. Their exact source snapshots and zero-warning compiler
outputs are retained. The production sanitizer flags actually contain
`-fsanitize=address,undefined`; no global sanitizer option was changed.

`production-v2` pins exact test binaries, linked archives, link commands,
flags, CMake caches, source snapshots and command logs. CMake changed during
the run only by the cooperating worker's independent
`sound_create_cache_gate_tests` registration. Before/after hashes and that
drift are recorded. The new owners/tests did not change. This is observed
production linkage, not a reproducible whole-tree or fresh-all-archive claim.
The earlier interrupted `production` copy stopped when recording an external
system archive path; it and preliminary `measured`/`validation` remain preserved
and excluded from the canonical freeze.

Canonical CLI:

```
sound_driver_update_tests /absolute/path/to/fixtures.tsv
```

Reproduction tools are `research_sound_driver_update_u5.py`,
`oracle_sound_driver_update_native.py`, `export_sound_driver_update_fixtures.py`,
`validate_sound_driver_update_native.py` and
`validate_sound_driver_update_production.py`. Every output directory is explicit;
prior immutable paths must not be overwritten. The audit checks 12,813 prior
pins,806 original instruction rows, exact repeats and actual production outputs
with zero mismatch.

The live next gate remains complete startup/device/routing ownership and map
activation, followed by real DoWork/status/packet processing. Source `+78` is
still owned by startup commit after real mandatory platform initialization;
`+7A` is owned by map activation. Neither is enabled by this module. This work
does not close live reload sound, all-map SFX playback or original DSP execution.
